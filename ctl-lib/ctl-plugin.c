/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, something express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Reference:
 *   Json load using json_unpack https://jansson.readthedocs.io/en/2.9/apiref.html#parsing-and-validating-values
 */

#define _GNU_SOURCE
#include <string.h>
#include <dlfcn.h>

#include "ctl-config.h"

static  CtlPluginT *ctlPlugins=NULL;

PUBLIC int PluginGetCB (AFB_ApiT apiHandle, CtlActionT *action , json_object *callbackJ) {
    const char *plugin=NULL, *function=NULL;
    json_object *argsJ;
    int idx;

    if (!ctlPlugins) {
        AFB_ApiError(apiHandle, "PluginGetCB plugin section missing cannot call '%s'", json_object_get_string(callbackJ));
        goto OnErrorExit;        
    }
      
    int err = wrap_json_unpack(callbackJ, "{ss,ss,s?s,s?o!}", "plugin", &plugin, "function", &function, "args", &argsJ); 
    if (err) {
        AFB_ApiError(apiHandle, "PluginGet missing plugin|function|[args] in %s", json_object_get_string(callbackJ));
        goto OnErrorExit;
    }
    
    for (idx=0; ctlPlugins[idx].label != NULL; idx++) {
        if (!strcasecmp (ctlPlugins[idx].label, plugin)) break;
    }
    
    if (!ctlPlugins[idx].label) {
        AFB_ApiError(apiHandle, "PluginGetCB no plugin with label=%s", plugin);
        goto OnErrorExit;
    }
    
    action->exec.cb.funcname = function; 
    action->exec.cb.callback = dlsym(ctlPlugins[idx].dlHandle, function); 
    action->exec.cb.plugin= &ctlPlugins[idx];
    
    if (!action->exec.cb.callback) {
       AFB_ApiError(apiHandle, "PluginGetCB no plugin=%s no function=%s", plugin, function); 
       goto OnErrorExit;
    }
    return 0; 

OnErrorExit:
    return 1;
    
}

// Wrapper to Lua2c plugin command add context and delegate to LuaWrapper
STATIC int DispatchOneL2c(void* luaState, char *funcname, Lua2cFunctionT callback) {
#ifndef CONTROL_SUPPORT_LUA
    fprintf(stderr, "CTL-ONE-L2C: LUA support not selected (cf:CONTROL_SUPPORT_LUA) in config.cmake");
    return 1;
#else
    int err=Lua2cWrapper(luaState, funcname, callback);
    return err;
#endif
}

STATIC int PluginLoadOne (AFB_ApiT apiHandle, CtlPluginT *ctlPlugin, json_object *pluginJ, void* handle) {
    json_object *lua2csJ = NULL;
    const char*ldSearchPath = NULL, *basename = NULL;
    void *dlHandle;

    
    // plugin initialises at 1st load further init actions should be place into onload section
    if (!pluginJ) return 0;
    
    int err = wrap_json_unpack(pluginJ, "{ss,s?s,s?s,s?s !}",
            "label", &ctlPlugin->label,  "info", &ctlPlugin->info, "ldpath", &ldSearchPath, "basename", &basename);
    if (err) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE Plugin missing label|[info]|[basename]|[ldpath] in:\n-- %s", json_object_get_string(pluginJ));
        goto OnErrorExit;
    }
    
    // default basename equal label
    if (!basename) basename=ctlPlugin->label;

    // if search path not in Json config file, then try default
    if (!ldSearchPath) ldSearchPath = CONTROL_PLUGIN_PATH;

    // search for default policy config file
    json_object *pluginPathJ = ScanForConfig(ldSearchPath, CTL_SCAN_RECURSIVE, basename, CTL_PLUGIN_EXT);
    if (!pluginPathJ || json_object_array_length(pluginPathJ) == 0) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE Missing plugin=%s*%s (config ldpath?) search=\n-- %s", basename, CTL_PLUGIN_EXT, ldSearchPath);
        goto OnErrorExit;
    }

    char *filename;
    char*fullpath;
    err = wrap_json_unpack(json_object_array_get_idx(pluginPathJ, 0), "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
    if (err) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE HOOPs invalid plugin file path=\n-- %s", json_object_get_string(pluginPathJ));
        goto OnErrorExit;
    }

    if (json_object_array_length(pluginPathJ) > 1) {
        AFB_ApiWarning(apiHandle, "CTL-PLUGIN-LOADONE plugin multiple instances in searchpath will use %s/%s", fullpath, filename);
    }

    char pluginpath[CONTROL_MAXPATH_LEN];
    strncpy(pluginpath, fullpath, sizeof (pluginpath));
    strncat(pluginpath, "/", sizeof (pluginpath));
    strncat(pluginpath, filename, sizeof (pluginpath));
    dlHandle = dlopen(pluginpath, RTLD_NOW);
    if (!dlHandle) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE Fail to load pluginpath=%s err= %s", pluginpath, dlerror());
        goto OnErrorExit;
    }

    CtlPluginMagicT *ctlPluginMagic = (CtlPluginMagicT*) dlsym(dlHandle, "CtlPluginMagic");
    if (!ctlPluginMagic || ctlPluginMagic->magic != CTL_PLUGIN_MAGIC) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE symbol'CtlPluginMagic' missing or !=  CTL_PLUGIN_MAGIC plugin=%s", pluginpath);
        goto OnErrorExit;
    } else {
        AFB_ApiNotice(apiHandle, "CTL-PLUGIN-LOADONE %s successfully registered", ctlPluginMagic->label);
    }
    
    // store dlopen handle to enable onload action at exec time
    ctlPlugin->dlHandle = dlHandle;
    
#ifndef AFB_BINDING_PREV3
    // Jose hack to make verbosity visible from sharelib with API-V2
    struct afb_binding_data_v2 *afbHidenData = dlsym(dlHandle, "afbBindingV2data");
    if (afbHidenData) *afbHidenData = afbBindingV2data;
#endif

    // Push lua2cWrapper @ into plugin
    Lua2cWrapperT *lua2cInPlug = dlsym(dlHandle, "Lua2cWrap");
#ifndef CONTROL_SUPPORT_LUA
    if (lua2cInPlug) *lua2cInPlug = NULL;
#else
    // Lua2cWrapper is part of binder and not expose to dynamic link
    if (lua2csJ && lua2cInPlug) {
        *lua2cInPlug = DispatchOneL2c;

        int Lua2cAddOne(luaL_Reg *l2cFunc, const char* l2cName, int index) {
            char funcName[CONTROL_MAXPATH_LEN];
            strncpy(funcName, "lua2c_", sizeof (funcName));
            strncat(funcName, l2cName, sizeof (funcName));

            Lua2cFunctionT l2cFunction = (Lua2cFunctionT) dlsym(dlHandle, funcName);
            if (!l2cFunction) {
                AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE symbol'%s' missing err=%s", funcName, dlerror());
                return 1;
            }
            l2cFunc[index].func = (void*) l2cFunction;
            l2cFunc[index].name = strdup(l2cName);

            return 0;
        }

        int errCount = 0;
        luaL_Reg *l2cFunc = NULL;

        // look on l2c command and push them to LUA
        if (json_object_get_type(lua2csJ) == json_type_array) {
            int length = json_object_array_length(lua2csJ);
            l2cFunc = calloc(length + 1, sizeof (luaL_Reg));
            for (int count = 0; count < length; count++) {
                int err;
                const char *l2cName = json_object_get_string(json_object_array_get_idx(lua2csJ, count));
                err = Lua2cAddOne(l2cFunc, l2cName, count);
                if (err) errCount++;
            }
        } else {
            l2cFunc = calloc(2, sizeof (luaL_Reg));
            const char *l2cName = json_object_get_string(lua2csJ);
            errCount = Lua2cAddOne(l2cFunc, l2cName, 0);
        }
        if (errCount) {
            AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE %d symbols not found in plugin='%s'", errCount, pluginpath);
            goto OnErrorExit;
        }
    }
#endif
    DispatchPluginInstallCbT ctlPluginOnload = dlsym(dlHandle, "CtlPluginOnload");
    if (ctlPluginOnload) {
        ctlPlugin->api = apiHandle;
        ctlPlugin->context = (*ctlPluginOnload) (ctlPlugin, handle);
    }
    return 0;
    
OnErrorExit:
    return 1;
}


PUBLIC int PluginConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *pluginsJ) {
    int err=0;
    
    if (json_object_get_type(pluginsJ) == json_type_array) {
        int length = json_object_array_length(pluginsJ);
        ctlPlugins = calloc (length+1, sizeof(CtlPluginT));
        for (int idx=0; idx < length; idx++) {
            json_object *pluginJ = json_object_array_get_idx(pluginsJ, idx);
            err += PluginLoadOne(apiHandle, &ctlPlugins[idx], pluginJ, section->handle);
        }
    } else {
        ctlPlugins = calloc (2, sizeof(CtlPluginT));
        err += PluginLoadOne(apiHandle, &ctlPlugins[0], pluginsJ, section->handle);       
    }
    
    return err;
}
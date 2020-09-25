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

void* getPluginContext(CtlPluginT *plugin) {
    return plugin->context;
}

void setPluginContext(CtlPluginT *plugin, void *context) {
    plugin->context = context;
}

int PluginGetCB (afb_api_t apiHandle, CtlActionT *action , json_object *callbackJ) {
    const char *plugin=NULL, *function=NULL;
    json_object *argsJ;
    int idx;

    CtlConfigT *ctlConfig = (CtlConfigT *) afb_api_get_userdata(apiHandle);
    CtlPluginT *ctlPlugins = ctlConfig ? ctlConfig->ctlPlugins : NULL;

    if (!ctlPlugins) {
        AFB_API_ERROR(apiHandle, "PluginGetCB plugin section missing cannot call '%s'", json_object_get_string(callbackJ));
        return 1;
    }

    int err = wrap_json_unpack(callbackJ, "{ss,ss,s?o!}",
        "plugin", &plugin,
        "function", &function,
        "args", &argsJ);
    if (err) {
        AFB_API_ERROR(apiHandle, "PluginGet missing plugin|function|[args] in %s", json_object_get_string(callbackJ));
        return 1;
    }

    for (idx=0; ctlPlugins[idx].uid != NULL; idx++) {
        if (!strcasecmp (ctlPlugins[idx].uid, plugin)) break;
    }

    if (!ctlPlugins[idx].uid) {
        AFB_API_ERROR(apiHandle, "PluginGetCB no plugin with uid=%s", plugin);
        return 1;
    }

    action->exec.cb.funcname = function;
    action->exec.cb.callback = dlsym(ctlPlugins[idx].dlHandle, function);
    action->exec.cb.plugin= &ctlPlugins[idx];

    if (!action->exec.cb.callback) {
       AFB_API_ERROR(apiHandle, "PluginGetCB no plugin=%s no function=%s", plugin, function);
       return 1;
    }
    return 0;
}

// Wrapper to Lua2c plugin command add context and delegate to LuaWrapper
static int DispatchOneL2c(void* luaState, char *funcname, Lua2cFunctionT callback) {
#ifndef CONTROL_SUPPORT_LUA
    fprintf(stderr, "CTL-ONE-L2C: LUA support not selected (cf:CONTROL_SUPPORT_LUA) in config.cmake");
    return 1;
#else
    int err=Lua2cWrapper(luaState, funcname, callback);
    return err;
#endif
}

int Lua2cAddOne(afb_api_t apiHandle, CtlPluginT *ctlPlugin, void *dlHandle, luaL_Reg *l2cFunc, const char* l2cName, int index) {
    if(ctlPlugin->ctlL2cFunc->l2cCount)
        {index += ctlPlugin->ctlL2cFunc->l2cCount+1;}
    char *funcName;
    size_t p_length = 6 + strlen(l2cName);
    funcName = malloc(p_length + 1);

    strncpy(funcName, "lua2c_", p_length + 1);
    strncat(funcName, l2cName, p_length - strlen (funcName) + 1);

    Lua2cFunctionT l2cFunction = (Lua2cFunctionT) dlsym(dlHandle, funcName);
    if (!l2cFunction) {
        AFB_API_ERROR(apiHandle, "CTL-PLUGIN-LOADONE symbol'%s' missing err=%s", funcName, dlerror());
        return 1;
    }
    l2cFunc[index].func = (void*) l2cFunction;
    l2cFunc[index].name = strdup(l2cName);

    return 0;
}

static int PluginLoadCOne(afb_api_t apiHandle, const char *pluginpath, json_object *lua2csJ, const char *lua2c_prefix, void * handle, CtlPluginT *ctlPlugin)
{
    void *dlHandle = dlopen(pluginpath, RTLD_NOW);

    if (!dlHandle) {
        AFB_API_ERROR(apiHandle, "CTL-PLUGIN-LOADONE Fail to load pluginpath=%s err= %s", pluginpath, dlerror());
        return -1;
    }

    CtlPluginMagicT *ctlPluginMagic = (CtlPluginMagicT*) dlsym(dlHandle, "CtlPluginMagic");
    if (!ctlPluginMagic) {
        AFB_API_ERROR(apiHandle, "CTL-PLUGIN-LOADONE symbol'CtlPluginMagic' missing %s", pluginpath);
        return -1;
    } else {
        AFB_API_NOTICE(apiHandle, "CTL-PLUGIN-LOADONE %s successfully registered", ctlPluginMagic->uid);
    }

    // store dlopen handle to enable onload action at exec time
    ctlPlugin->dlHandle = dlHandle;

    // Plugins using the binder are weakly link the symbol "afbBindingV3root" that is used as the
    // default API handle for any afb_api_... operation. To allow the plugins to use default API
    // features (verbosity LOG by example), this is needed to supply a correct value for the plugin.
    // The default API is a good default choice.
    afb_api_t *afbHidenData = dlsym(dlHandle, "afbBindingV3root");
    if (afbHidenData) *afbHidenData = afbBindingV3root;

    // Push lua2cWrapper @ into plugin
    Lua2cWrapperT *lua2cInPlug = dlsym(dlHandle, "Lua2cWrap");
#ifndef CONTROL_SUPPORT_LUA
    if (lua2cInPlug) *lua2cInPlug = NULL;
#else
    // Lua2cWrapper is part of binder and not expose to dynamic link
    if (lua2csJ && lua2cInPlug) {
        *lua2cInPlug = (Lua2cWrapperT)DispatchOneL2c;

        int count = 0, errCount = 0;
        luaL_Reg *l2cFunc = NULL;
        if(!ctlPlugin->ctlL2cFunc) {
            ctlPlugin->ctlL2cFunc = calloc(1, sizeof(CtlLua2cFuncT));
        }

        ctlPlugin->ctlL2cFunc->prefix = (lua2c_prefix) ?
            lua2c_prefix :
            ctlPlugin->uid;

        // look on l2c command and push them to LUA
        if (json_object_get_type(lua2csJ) == json_type_array) {
            size_t length = json_object_array_length(lua2csJ);
            l2cFunc = calloc(length + ctlPlugin->ctlL2cFunc->l2cCount + 1, sizeof (luaL_Reg));
            for (count = 0; count < length; count++) {
                int err;
                const char *l2cName = json_object_get_string(json_object_array_get_idx(lua2csJ, count));
                err = Lua2cAddOne(apiHandle, ctlPlugin, dlHandle, l2cFunc, l2cName, count);
                if (err) errCount++;
            }
        } else {
            l2cFunc = calloc(2 + ctlPlugin->ctlL2cFunc->l2cCount, sizeof (luaL_Reg));
            const char *l2cName = json_object_get_string(lua2csJ);
            errCount = Lua2cAddOne(apiHandle, ctlPlugin, dlHandle, l2cFunc, l2cName, count);
            count++;
        }
        if (errCount) {
            AFB_API_ERROR(apiHandle, "CTL-PLUGIN-LOADONE %d symbols not found in plugin='%s'", errCount, pluginpath);
            return -1;
        }
        int total = ctlPlugin->ctlL2cFunc->l2cCount + count;
        if(ctlPlugin->ctlL2cFunc->l2cCount) {
            for (int offset = ctlPlugin->ctlL2cFunc->l2cCount; offset < total; offset++)
            {
                int index = offset - ctlPlugin->ctlL2cFunc->l2cCount;
                l2cFunc[index] = ctlPlugin->ctlL2cFunc->l2cFunc[index];
            }
            free(ctlPlugin->ctlL2cFunc->l2cFunc);
        }
        ctlPlugin->ctlL2cFunc->l2cFunc = l2cFunc;
        ctlPlugin->ctlL2cFunc->l2cCount = total;

        LuaL2cNewLib(ctlPlugin->ctlL2cFunc->l2cFunc, ctlPlugin->ctlL2cFunc->l2cCount, ctlPlugin->ctlL2cFunc->prefix);
    }
#endif
    ctlPlugin->api = apiHandle;
    DispatchPluginInstallCbT ctlPluginOnload = dlsym(dlHandle, "CtlPluginOnload");
    if (ctlPluginOnload) {
        if((*ctlPluginOnload) (ctlPlugin, handle)) {
            AFB_API_ERROR(apiHandle, "Plugin Onload function hasn't finish well. Abort initialization");
            return -1;
        }
    }

    return 0;
}

static int LoadFoundPlugins(afb_api_t apiHandle, json_object *scanResult, json_object *lua2csJ, const char *lua2c_prefix, void *handle, CtlPluginT *ctlPlugin)
{
    char pluginpath[CONTROL_MAXPATH_LEN];
    char *filename;
    char *fullpath;
    char *ext;
    int len;
    json_object *object = NULL;

    pluginpath[CONTROL_MAXPATH_LEN - 1] = '\0';

    if (!json_object_is_type(scanResult, json_type_array))
        return -1;

    len = (int)json_object_array_length(scanResult);

    // TODO/Proposal RFOR: load a plugin after a first fail.
    if(len) {
        object = json_object_array_get_idx(scanResult, 0);
        int err = wrap_json_unpack(object, "{s:s, s:s !}",
                "fullpath", &fullpath,
                "filename", &filename);

        if (err) {
            AFB_API_ERROR(apiHandle, "HOOPs invalid plugin file path=\n-- %s", json_object_get_string(scanResult));
            return -1;
        }

        ext = strrchr(filename, '.');
        strncpy(pluginpath, fullpath, CONTROL_MAXPATH_LEN - 1);
        strncat(pluginpath, "/", CONTROL_MAXPATH_LEN - strlen(pluginpath) - 1);
        strncat(pluginpath, filename, CONTROL_MAXPATH_LEN - strlen (pluginpath) - 1);

        if(ext && !strcasecmp(ext, CTL_PLUGIN_EXT) && PluginLoadCOne(apiHandle, pluginpath, lua2csJ, lua2c_prefix, handle, ctlPlugin)) {
            return -1;
        }
        else if(ext && !strcasecmp(ext, CTL_SCRIPT_EXT)) {
            ctlPlugin->api = apiHandle;
            ctlPlugin->context = handle;
            if(LuaLoadScript(apiHandle, pluginpath))
                return -1;
        }
    }

    if(len > 1)
        AFB_API_WARNING(apiHandle, "Plugin multiple instances in searchpath will use %s/%s", fullpath, filename);

    return 0;
}

char *GetBindingParentDirPath(afb_api_t apiHandle)
{
    int ret;
    char *bindingDirPath, *bindingParentDirPath = NULL;

    if(! apiHandle)
        return NULL;

    bindingDirPath = GetRunningBindingDirPath(apiHandle);
    if(! bindingDirPath)
        return NULL;

    ret = asprintf(&bindingParentDirPath, "%s/..", bindingDirPath);
    free(bindingDirPath);
    if(ret <= 3)
        return NULL;

    return bindingParentDirPath;
}

char *GetDefaultPluginSearchPath(afb_api_t apiHandle)
{
    size_t searchPathLength;
    char *searchPath, *binderRootDirPath, *bindingParentDirPath;

    if(! apiHandle)
        return NULL;

    binderRootDirPath = GetAFBRootDirPath(apiHandle);
    if(! binderRootDirPath)
        return NULL;

    bindingParentDirPath = GetBindingParentDirPath(apiHandle);
    if(! bindingParentDirPath) {
        free(binderRootDirPath);
        return NULL;
    }

    /* Allocating with the size of binding root dir path + binding parent directory path
     * + 1 character for the NULL terminating character + 1 character for the additional separator
     * between binderRootDirPath and bindingParentDirPath.
     */
    searchPathLength = strlen(binderRootDirPath) + strlen(bindingParentDirPath) + 2;

    searchPath = malloc(searchPathLength);
    if(! searchPath) {
        free(binderRootDirPath);
        free(bindingParentDirPath);
        return NULL;
    }

    snprintf(searchPath, searchPathLength, "%s:%s", binderRootDirPath, bindingParentDirPath);

    free(binderRootDirPath);
    free(bindingParentDirPath);

    return searchPath;
}

int isCharIsAllowedInEnvironmentVariable(char envChar)
{
    if(envChar < '0')
        return 0;

    if((envChar >= '0' && envChar <= '9') ||
       (envChar >= 'A' && envChar <= 'Z') ||
       (envChar >= 'a' && envChar <= 'z') ||
       envChar == '_')
        return 1;

    return 0;
}

char *ExpandPath(const char *path)
{
    int cpt,
        pathLength,
        pathPartCount = 0,
        currentPartNb = -1,
        currentPartBegin,
        currentlyInEnvVar,
        newPartDetected = 1,
        expandedPathLength = 0,
        expandedPathIdx = 0;
    int *pathPartLengths;

    char currentChar;
    char *pathToExpand, *expandedpath;
    char **pathPartStrings;

    if(! path)
        return NULL;

    pathLength = (int) strlen(path);
    if(! pathLength)
        return NULL;

    pathToExpand = strdup(path);
    if(! pathToExpand)
        return NULL;

    for(cpt = 1; cpt <= pathLength; cpt++) {
        if(newPartDetected) {
            newPartDetected = 0;
            currentlyInEnvVar = (pathToExpand[cpt - 1] == '$') ? 1 : 0;
            pathPartCount++;
        }

        if((currentlyInEnvVar && ! isCharIsAllowedInEnvironmentVariable(pathToExpand[cpt])) ||
           (! currentlyInEnvVar && pathToExpand[cpt] == '$'))
            newPartDetected = 1;
    }

    if(pathPartCount == 1) {
        if(currentlyInEnvVar) {
            expandedpath = getenv(&pathToExpand[1]);
            free(pathToExpand);
            if(! expandedpath)
                return NULL;
            return strdup(expandedpath);
        }
        else {
            return pathToExpand;
        }
    }

    pathPartLengths = (int *) alloca(pathPartCount * (sizeof(int)));
    if(! pathPartLengths) {
        free(pathToExpand);
        return NULL;
    }

    pathPartStrings = (char **) alloca(pathPartCount * (sizeof(char *)));
    if(! pathPartStrings) {
        free(pathToExpand);
        return NULL;
    }

    newPartDetected = 1;
    currentChar = pathToExpand[0];

    for(cpt = 1; cpt <= pathLength; cpt++) {
        if(newPartDetected) {
            newPartDetected = 0;
            currentlyInEnvVar = (currentChar == '$') ? 1 : 0;
            currentPartNb++;
            currentPartBegin = cpt - 1;
        }

        currentChar = pathToExpand[cpt];

        if(currentlyInEnvVar && ! isCharIsAllowedInEnvironmentVariable(currentChar)) {
            newPartDetected = 1;

            pathToExpand[cpt] = '\0';

            pathPartStrings[currentPartNb] = getenv(&pathToExpand[currentPartBegin + 1]);
            if(! pathPartStrings[currentPartNb]) {
                free(pathToExpand);
                return NULL;
            }

            pathToExpand[cpt] = currentChar;

            pathPartLengths[currentPartNb] = (int) strlen(pathPartStrings[currentPartNb]);
        }

        if(! currentlyInEnvVar &&
           (! currentChar || currentChar == '$')) {
            newPartDetected = 1;

            pathToExpand[cpt] = '\0';

            pathPartStrings[currentPartNb] = &pathToExpand[currentPartBegin];
            pathPartLengths[currentPartNb] = cpt - currentPartBegin;
        }

        if(newPartDetected)
            expandedPathLength += pathPartLengths[currentPartNb];
    }

    expandedPathLength++;

    expandedpath = malloc(expandedPathLength * sizeof(char));
    if(! expandedpath) {
        free(pathToExpand);
        return NULL;
    }

    for(cpt = 0; cpt < pathPartCount; cpt++) {
        strcpy(&expandedpath[expandedPathIdx], pathPartStrings[cpt]);
        expandedPathIdx += pathPartLengths[cpt];
    }

    free(pathToExpand);

    return expandedpath;
}

char *ExpandPluginSearchPath(afb_api_t apiHandle, const char *sPath)
{
    char *binderRootDirPath = NULL,
         *bindingParentDirPath = NULL,
         *sPathToExpand = NULL,
         *currentPathToExpand,
         *expandedPath = NULL,
         *expandedAbsolutePath,
         *expandedSearchPath = NULL,
         *newExpandedSearchPath;

    if(! apiHandle || ! sPath) {
        AFB_API_ERROR(apiHandle, "Invalid argument(s)");
        goto OnErrorExit;
    }

    binderRootDirPath = GetAFBRootDirPath(apiHandle);
    if(! binderRootDirPath) {
        AFB_API_ERROR(apiHandle, "An error happened when tried to get binder root directory path");
        goto OnErrorExit;
    }

    bindingParentDirPath = GetBindingParentDirPath(apiHandle);
    if(! bindingParentDirPath) {
        AFB_API_ERROR(apiHandle, "An error happened when tried to get binding parent directory path");
        goto OnErrorExit;
    }

    sPathToExpand = strdup(sPath);
    if(! sPathToExpand) {
        AFB_API_ERROR(apiHandle, "Error while allocating copy of paths to expand");
        goto OnErrorExit;
    }

    for(currentPathToExpand = strtok(sPathToExpand, ":");
        currentPathToExpand && *currentPathToExpand;
        currentPathToExpand = strtok(NULL, ":")) {
        expandedPath = ExpandPath(currentPathToExpand);
        if(! expandedPath) {
            AFB_API_NOTICE(apiHandle,
                           "An error happened when tried to expand plugin path '%s' string, will be ignored when searching for plugins",
                           currentPathToExpand);
            continue;
        }

        expandedAbsolutePath = NULL;
        if(expandedPath[0] == '/') {
            expandedAbsolutePath = strdup(expandedPath);
        }
        else if(asprintf(&expandedAbsolutePath, "%s/%s:%s/%s", binderRootDirPath, expandedPath, bindingParentDirPath, expandedPath) < 7) {
            AFB_API_ERROR(apiHandle, "Error while allocating whole absolute expanded path");
            goto OnErrorExit;
        }

        if(! expandedAbsolutePath) {
            AFB_API_ERROR(apiHandle, "Error while allocating absolute expanded path");
            goto OnErrorExit;
        }

        free(expandedPath);
        expandedPath = NULL;

        newExpandedSearchPath = NULL;
        if(! expandedSearchPath) {
            newExpandedSearchPath = strdup(expandedAbsolutePath);
        }
        else if((asprintf(&newExpandedSearchPath, "%s:%s", expandedSearchPath, expandedAbsolutePath) < 3) ||
                ! newExpandedSearchPath) {
            AFB_API_ERROR(apiHandle, "Error while allocating new search paths string");
            free(expandedAbsolutePath);
            goto OnErrorExit;
        }

        free(expandedAbsolutePath);
        free(expandedSearchPath);

        expandedSearchPath = newExpandedSearchPath;
    }

    free(binderRootDirPath);
    free(bindingParentDirPath);
    free(sPathToExpand);

    return expandedSearchPath;

OnErrorExit:
    free(binderRootDirPath);
    free(bindingParentDirPath);
    free(sPathToExpand);
    free(expandedPath);
    free(expandedSearchPath);
    return NULL;
}

static int FindPlugins(afb_api_t apiHandle, const char *searchPath, const char *file, json_object **pluginPathJ)
{
    *pluginPathJ = ScanForConfig(searchPath, CTL_SCAN_RECURSIVE, file, NULL);
    if (!*pluginPathJ || json_object_array_length(*pluginPathJ) == 0) {
        AFB_API_ERROR(apiHandle, "CTL-PLUGIN-LOADONE Missing plugin=%s* (config ldpath?) search=\n-- %s", file, searchPath);
        return -1;
    }

    return 0;
}

static int PluginLoad (afb_api_t apiHandle, CtlPluginT *ctlPlugin, json_object *pluginJ, void *handle)
{
    int err = 0, i = 0;
    char *searchPath = NULL;
    const char *sPath = NULL, *file = NULL, *lua2c_prefix = NULL;
    json_object *luaJ = NULL, *lua2csJ = NULL, *fileJ = NULL, *pluginPathJ = NULL;

    // plugin initialises at 1st load further init actions should be place into onload section
    if (!pluginJ) return 0;

    err = wrap_json_unpack(pluginJ, "{ss,s?s,s?s,s?o,s?o,s?o !}",
            "uid", &ctlPlugin->uid,
            "info", &ctlPlugin->info,
            "spath", &sPath,
            "libs", &fileJ,
            "lua", &luaJ,
            "params", &ctlPlugin->paramsJ
            );
    if (err) {
        AFB_API_ERROR(apiHandle, "CTL-PLUGIN-LOADONE Plugin missing uid|[info]|libs|[spath]|[lua]|[params] in:\n-- %s", json_object_get_string(pluginJ));
        return 1;
    }

    if(luaJ) {
        err = wrap_json_unpack(luaJ, "{ss,s?o !}",
            "prefix", &lua2c_prefix,
            "functions", &lua2csJ);
        if(err) {
            AFB_API_ERROR(apiHandle, "CTL-PLUGIN-LOADONE Missing 'function' in:\n-- %s", json_object_get_string(pluginJ));
            return 1;
        }
    }

    // if search path not in Json config file, then try default
    if(sPath)
        searchPath = ExpandPluginSearchPath(apiHandle, sPath);

    if(!searchPath)
        searchPath = GetDefaultPluginSearchPath(apiHandle);

    AFB_API_DEBUG(apiHandle, "Plugin search path : '%s'", searchPath);

    // default file equal uid
    if (!fileJ) {
        file = ctlPlugin->uid;
        if(FindPlugins(apiHandle, searchPath, file, &pluginPathJ)) {
            free(searchPath);
            if(pluginPathJ)
                json_object_put(pluginPathJ); // No more needs for that json_object.
            return 1;
        }
        LoadFoundPlugins(apiHandle, pluginPathJ, lua2csJ, lua2c_prefix, handle, ctlPlugin);
    }
    else if(json_object_is_type(fileJ, json_type_string)) {
        file = json_object_get_string(fileJ);
        if(FindPlugins(apiHandle, searchPath, file, &pluginPathJ)) {
            free(searchPath);
            json_object_put(pluginPathJ); // No more needs for that json_object.
            return 1;
        }
        LoadFoundPlugins(apiHandle, pluginPathJ, lua2csJ, lua2c_prefix, handle, ctlPlugin);
    }
    else if(json_object_is_type(fileJ, json_type_array)) {
        for(i = 0; i < json_object_array_length(fileJ);++i) {
            file = json_object_get_string(json_object_array_get_idx(fileJ, i));
            if(FindPlugins(apiHandle, searchPath, file, &pluginPathJ)) {
                free(searchPath);
                json_object_put(pluginPathJ); // No more needs for that json_object.
                return 1;
            }
            LoadFoundPlugins(apiHandle, pluginPathJ, lua2csJ, lua2c_prefix, handle, ctlPlugin);
        }
    }

    if(err) {
        free(searchPath);
        json_object_put(pluginPathJ); // No more needs for that json_object.
        return 1;
    }

    free(searchPath);
    json_object_put(pluginPathJ); // No more needs for that json_object.
    return 0;
}

static int PluginParse(afb_api_t apiHandle, CtlSectionT *section, json_object *pluginsJ) {
    int err = 0, idx = 0, pluginToAddNumber, totalPluginNumber, initialPluginNumber;

    CtlConfigT *ctlConfig = (CtlConfigT *) afb_api_get_userdata(apiHandle);
    CtlPluginT *ctlPluginsNew, *ctlPluginsOrig = ctlConfig ? ctlConfig->ctlPlugins : NULL;

    while(ctlPluginsOrig && ctlPluginsOrig[idx].uid != NULL)
        idx++;

    initialPluginNumber = idx;

    switch (json_object_get_type(pluginsJ)) {
        case json_type_array: {
            pluginToAddNumber = (int) json_object_array_length(pluginsJ);
            break;
        }
        case json_type_object: {
            pluginToAddNumber = 1;
            break;
        }
        default: {
            AFB_API_ERROR(apiHandle, "Wrong JSON object passed: %s", json_object_get_string(pluginsJ));
            return -1;
        }
    }

    totalPluginNumber = initialPluginNumber + pluginToAddNumber;

    ctlPluginsNew = calloc (totalPluginNumber + 1, sizeof(CtlPluginT));
    memcpy(ctlPluginsNew, ctlPluginsOrig, idx * sizeof(CtlPluginT));

    while(idx < totalPluginNumber) {
        json_object *pluginJ = json_object_is_type(pluginsJ, json_type_array) ?
                               json_object_array_get_idx(pluginsJ, idx - initialPluginNumber) : pluginsJ;
        err += PluginLoad(apiHandle, &ctlPluginsNew[idx], pluginJ, section->handle);
        idx++;
    }

    ctlConfig->ctlPlugins = ctlPluginsNew;
    free(ctlPluginsOrig);

    return err;
}

int PluginConfig(afb_api_t apiHandle, CtlSectionT *section, json_object *pluginsJ) {
    int err = 0, idx = 0;

    CtlConfigT *ctlConfig = (CtlConfigT *) afb_api_get_userdata(apiHandle);
    CtlPluginT *ctlPlugins = ctlConfig ? ctlConfig->ctlPlugins : NULL;

    // First plugins load
    if(pluginsJ) {
        err = PluginParse(apiHandle, section, pluginsJ);
    }
    // Code executed executed at Controller ConfigExec step
    else if (ctlPlugins) {
        while(ctlPlugins[idx].uid != NULL)
        {
            // Calling plugin Init function
            DispatchPluginInstallCbT ctlPluginInit = dlsym(ctlPlugins[idx].dlHandle, "CtlPluginInit");
            if (ctlPluginInit) {
                if((*ctlPluginInit) (&ctlPlugins[idx], ctlPlugins[idx].context)) {
                    AFB_API_ERROR(apiHandle, "Plugin Init function hasn't finish well. Abort initialization");
                    return -1;
                }
            }
            idx++;
        }
    }

    return err;
}

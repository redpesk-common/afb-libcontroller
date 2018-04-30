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
#include <stdio.h>
#include <string.h>

#include "ctl-config.h"

PUBLIC int ActionLabelToIndex(CtlActionT*actions, const char* actionLabel) {
    for (int idx = 0; actions[idx].uid; idx++) {
        if (!strcasecmp(actionLabel, actions[idx].uid)) return idx;
    }
    return -1;
}

PUBLIC void ActionExecUID(AFB_ReqT request, CtlConfigT *ctlConfig, const char *uid, json_object *queryJ)
{
    for(int i=0; ctlConfig->sections[i].key != NULL; i++)
    {
        if(ctlConfig->sections[i].actions)
        {
            for(int j=0; ctlConfig->sections[i].actions[j].uid != NULL; j++)
            {
                if(strcmp(ctlConfig->sections[i].actions[j].uid, uid) == 0)
                {
                    CtlSourceT source;
                    source.uid = ctlConfig->sections[i].actions[j].uid;
                    source.api  = ctlConfig->sections[i].actions[j].api;
                    source.request = request;

                    ActionExecOne(&source, &ctlConfig->sections[i].actions[j], queryJ);
                }
            }
        }
    }
}

PUBLIC void ActionExecOne(CtlSourceT *source, CtlActionT* action, json_object *queryJ) {
    int err = 0;

    switch (action->type) {
        case CTL_TYPE_API:
        {
            json_object *returnJ, *subcallArgsJ = json_object_new_object();

            if(queryJ) {
                json_object_object_foreach(queryJ, key, val) {
                    json_object_object_add(subcallArgsJ, key, val);
                }
            }

            if (action->argsJ) {
                // Merge queryJ and argsJ before sending request
                if (json_object_get_type(action->argsJ) == json_type_object) {

                    json_object_object_foreach(action->argsJ, key, val) {
                        json_object_get(val);
                        json_object_object_add(subcallArgsJ, key, val);
                    }
                } else {
                    json_object_get(action->argsJ);
                    json_object_object_add(subcallArgsJ, "args", action->argsJ);
                }
            }

            json_object_object_add(subcallArgsJ, "uid", json_object_new_string(source->uid));

            /* AFB Subcall will release the json_object doing the json_object_put() call */
            int err = AFB_ServiceSync(action->api, action->exec.subcall.api, action->exec.subcall.verb, subcallArgsJ, &returnJ);
            if (err) {
                AFB_ApiError(action->api, "ActionExecOne(AppFw) uid=%s api=%s verb=%s args=%s", source->uid, action->exec.subcall.api, action->exec.subcall.verb, json_object_get_string(action->argsJ));
            }
            break;
        }

#ifdef CONTROL_SUPPORT_LUA
        case CTL_TYPE_LUA:
            err = LuaCallFunc(source, action, queryJ);
            if (err) {
                AFB_ApiError(action->api, "ActionExecOne(Lua) uid=%s func=%s args=%s", source->uid, action->exec.lua.funcname, json_object_get_string(action->argsJ));
            }
            json_object_put(queryJ);
            break;
#endif

        case CTL_TYPE_CB:
            err = (*action->exec.cb.callback) (source, action->argsJ, queryJ);
            if (err) {
                AFB_ApiError(action->api, "ActionExecOne(Callback) uid%s plugin=%s function=%s args=%s", source->uid, action->exec.cb.plugin->uid, action->exec.cb.funcname, json_object_get_string(action->argsJ));
            }
            json_object_put(queryJ);
            break;

        default:
        {
            AFB_ApiError(action->api, "ActionExecOne(unknown) API type uid=%s", source->uid);
            json_object_put(queryJ);
            break;
        }
    }
}


// Direct Request Call in APIV3
#ifdef AFB_BINDING_PREV3
STATIC void ActionDynRequest (AFB_ReqT request) {

   // retrieve action handle from request and execute the request
   json_object *queryJ = afb_request_json(request);
   CtlActionT* action  = (CtlActionT*)afb_request_get_vcbdata(request);

    CtlSourceT source;
    source.uid = action->uid;
    source.request = request;
    source.api  = action->api;

   // provide request and execute the action
   ActionExecOne(&source, action, queryJ);
}
#endif

/*** This function will fill the CtlActionT pointer given in parameters for a
 * given api using an 'uri' that specify the C plugin to use and the name of
 * the function
 *
 */
static int BuildPluginAction(AFB_ApiT apiHandle, const char *uri, const char *function, CtlActionT *action)
{
    json_object *callbackJ = NULL;

    if(!action) {
        AFB_ApiError(apiHandle, "Action not valid");
        return -1;
    }

    action->type = CTL_TYPE_CB;

    if(uri && function) {
        if(wrap_json_pack(&callbackJ, "{ss,ss,s?o*}",
                "plugin", uri,
                "function", function,
                "args", action->argsJ)) {
            AFB_ApiError(apiHandle, "Error packing Callback JSON object for plugin %s and function %s", uri, function);
            return -1;
        }
        else {
            return PluginGetCB(apiHandle, action, callbackJ);
        }
    }
    else {
        AFB_ApiError(apiHandle, "Miss something uri or function.");
        return -1;
    }

    return 0;
}

/*** This function will fill the CtlActionT pointer given in parameters for a
 * given api using an 'uri' that specify the API to use and the name of the
 * verb
 *
 * Be aware that 'uri' and 'function' could be null but will result in
 * unexpected result.
 *
 */
static int BuildApiAction(AFB_ApiT apiHandle, const char *uri, const char *function, CtlActionT *action)
{
    if(!action) {
        AFB_ApiError(apiHandle, "Action not valid");
        return -1;
    }

    action->type = CTL_TYPE_API;
    action->exec.subcall.api = uri;
    action->exec.subcall.verb = function;

    return 0;
}

/*** This function will fill the CtlActionT pointer given in parameters for a
 * given api using an 'uri' that specify the Lua plugin to use and the name of
 * the function.
 *
 * Be aware that 'uri' and 'function' could be null but will result in
 * unexpected result.
 *
 */
#ifdef CONTROL_SUPPORT_LUA
static int BuildLuaAction(AFB_ApiT apiHandle, const char *uri, const char *function, CtlActionT *action)
{
    if(!action) {
        AFB_ApiError(apiHandle, "Action not valid");
        return -1;
    }

    action->type = CTL_TYPE_LUA;
    action->exec.lua.plugin = uri;
    action->exec.lua.funcname = function;

    return 0;
}
#endif

static int BuildOneAction(AFB_ApiT apiHandle, CtlActionT *action, const char *uri, const char *function) {
    size_t lua_pre_len = strlen(LUA_ACTION_PREFIX);
    size_t api_pre_len = strlen(API_ACTION_PREFIX);
    size_t plugin_pre_len = strlen(PLUGIN_ACTION_PREFIX);

    if(uri && function && action) {
        if(! strncasecmp(uri, LUA_ACTION_PREFIX, lua_pre_len)) {
#ifdef CONTROL_SUPPORT_LUA
            return BuildLuaAction(apiHandle, &uri[lua_pre_len], function, action);
#else
            AFB_ApiError(apiHandle, "LUA support not selected at build. Feature disabled");
            return -1;
#endif
        }
        else if(! strncasecmp(uri, API_ACTION_PREFIX, api_pre_len)) {
            return BuildApiAction(apiHandle, &uri[api_pre_len], function, action);
        }
        else if(! strncasecmp(uri, PLUGIN_ACTION_PREFIX, plugin_pre_len)) {
            return BuildPluginAction(apiHandle, &uri[plugin_pre_len], function, action);
        }
        else {
            AFB_ApiError(apiHandle, "Wrong uri specified. You have to specified 'lua://', 'plugin://' or 'api://'. (%s)", function);
            return -1;
        }
    }

    AFB_ApiError(apiHandle, "Uri, Action or function not valid");
    return -1;
}

// unpack individual action object
PUBLIC int ActionLoadOne(AFB_ApiT apiHandle, CtlActionT *action, json_object *actionJ, int exportApi) {
    int err = 0;
    const char *uri = NULL, *function = NULL;

    memset(action, 0, sizeof(CtlActionT));

    // save per action api handle
    action->api = apiHandle;

    // in API V3 each control is optionally map to a verb
#ifdef AFB_BINDING_PREV3
    if (apiHandle && exportApi) {
        err = afb_dynapi_add_verb(apiHandle, action->uid, action->info, ActionDynRequest, action, NULL, 0);
        if (err) {
            AFB_ApiError(apiHandle,"ACTION-LOAD-ONE fail to register API verb=%s", action->uid);
            goto OnErrorExit;
        }
        action->api = apiHandle;
    }
#endif

    if(actionJ) {
        err = wrap_json_unpack(actionJ, "{ss,s?s,ss,ss,s?s,s?o !}",
            "uid", &action->uid,
            "info", &action->info,
            "uri", &uri,
            "function", &function,
            "privileges", &action->privileges,
            "args", &action->argsJ);
        if(!err) {
            err = BuildOneAction(apiHandle, action, uri, function);
        }
        else {
            AFB_ApiError(apiHandle, "Wrong action JSON object parameter: (%s)", json_object_to_json_string(actionJ));
            err = -1;
        }
    }
    else {
        AFB_ApiError(apiHandle, "Wrong action JSON object parameter: (%s)", json_object_to_json_string(actionJ));
        err = -1;
    }

    return err;
}

PUBLIC CtlActionT *ActionConfig(AFB_ApiT apiHandle, json_object *actionsJ, int exportApi) {
    int err;
    CtlActionT *actions;

    // action array is close with a nullvalue;
    if (json_object_is_type(actionsJ, json_type_array)) {
        size_t count = json_object_array_length(actionsJ);
        actions = calloc(count + 1, sizeof (CtlActionT));

        for (int idx = 0; idx < count; idx++) {
            json_object *actionJ = json_object_array_get_idx(actionsJ, idx);

            err = ActionLoadOne(apiHandle, &actions[idx], actionJ, exportApi);
            if (err) goto OnErrorExit;
        }

    } else {
        actions = calloc(2, sizeof (CtlActionT));
        err = ActionLoadOne(apiHandle, &actions[0], actionsJ, exportApi);
        if (err) goto OnErrorExit;
    }

    return actions;

OnErrorExit:
    return NULL;
}

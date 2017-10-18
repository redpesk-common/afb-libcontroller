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

    for (int idx = 0; actions[idx].label; idx++) {
        if (!strcasecmp(actionLabel, actions[idx].label)) return idx;
    }
    return -1;
}


PUBLIC void ActionExecOne(CtlSourceT *source, CtlActionT* action, json_object *queryJ) {
    int err;
    
    switch (action->type) {
        case CTL_TYPE_API:
        {
            json_object *returnJ;

            // if query is empty increment usage count and pass args
            if (!queryJ || json_object_get_type(queryJ) != json_type_object) {
                json_object_get(action->argsJ);
                queryJ = action->argsJ;
            } else if (action->argsJ) {

                // Merge queryJ and argsJ before sending request
                if (json_object_get_type(action->argsJ) == json_type_object) {

                    json_object_object_foreach(action->argsJ, key, val) {
                        json_object_object_add(queryJ, key, val);
                    }
                } else {
                    json_object_object_add(queryJ, "args", action->argsJ);
                }
            }
            
            json_object_object_add(queryJ, "label", json_object_new_string(source->label));

            int err = AFB_ServiceSync(action->api, action->exec.subcall.api, action->exec.subcall.verb, queryJ, &returnJ);
            if (err) {
                AFB_ApiError(action->api, "ActionExecOne(AppFw) label=%s api=%s verb=%s args=%s", source->label, action->exec.subcall.api, action->exec.subcall.verb, json_object_get_string(action->argsJ));
            }
            break;
        }

#ifdef CONTROL_SUPPORT_LUA
        case CTL_TYPE_LUA:
            err = LuaCallFunc(source, action, queryJ);
            if (err) {
                AFB_ApiError(action->api, "ActionExecOne(Lua) label=%s func=%s args=%s", source->label, action->exec.lua.funcname, json_object_get_string(action->argsJ));
            }
            break;
#endif

        case CTL_TYPE_CB:
            err = (*action->exec.cb.callback) (source, action->argsJ, queryJ);
            if (err) {
                AFB_ApiError(action->api, "ActionExecOne(Callback) label%s plugin=%s function=%s args=%s", source->label, action->exec.cb.plugin->label, action->exec.cb.funcname, json_object_get_string(action->argsJ));
            }
            break;

        default:
        {
            AFB_ApiError(action->api, "ActionExecOne(unknown) API type label=%s", source->label);
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
    source.label = action->label;
    source.request = request;
    source.api  = action->api;
    
   // provide request and execute the action
   ActionExecOne(&source, action, queryJ);   
}
#endif

// unpack individual action object

PUBLIC int ActionLoadOne(AFB_ApiT apiHandle, CtlActionT *action, json_object *actionJ, int exportApi) {
    int err, modeCount = 0;
    json_object *callbackJ=NULL, *luaJ=NULL, *subcallJ=NULL;

    err = wrap_json_unpack(actionJ, "{ss,s?s,s?o,s?o,s?o,s?o !}"
            , "label", &action->label, "info", &action->info, "callback", &callbackJ, "lua", &luaJ, "subcall", &subcallJ, "args", &action->argsJ);
    if (err) {
        AFB_ApiError(apiHandle,"ACTION-LOAD-ONE Action missing label|[info]|[callback]|[lua]|[subcall]|[args] in:\n--  %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }
    
    // save per action api handle
    action->api = apiHandle;
    
    // in API V3 each control is optionally map to a verb            
#ifdef AFB_BINDING_PREV3
    if (apiHandle && exportApi) {
        err = afb_dynapi_add_verb(apiHandle, action->label, action->info, ActionDynRequest, action, NULL, 0);
        if (err) {
            AFB_ApiError(apiHandle,"ACTION-LOAD-ONE fail to register API verb=%s", action->label);
            goto OnErrorExit;
        }
        action->api = apiHandle;
    }
#endif   

    if (luaJ) {        
        modeCount++;
        action->type = CTL_TYPE_LUA;
        err = wrap_json_unpack(luaJ, "{s?s,s:s !}", "load", &action->exec.lua.load, "func", &action->exec.lua.funcname);
        if (err) {
            AFB_ApiError(apiHandle,"ACTION-LOAD-ONE Lua missing [load]|func in:\n--  %s", json_object_get_string(luaJ));
            goto OnErrorExit;
        }
    }

    if (subcallJ) {
        modeCount++;
        action->type = CTL_TYPE_API;
        
        err = wrap_json_unpack(luaJ, "{s?s,s:s !}", "api", &action->exec.subcall.api, "verb", &action->exec.subcall.verb);
        if (err) {
            AFB_ApiError(apiHandle,"ACTION-LOAD-ONE Subcall missing [load]|func in:\n--  %s", json_object_get_string(luaJ));
            goto OnErrorExit;
        }        
    }

    if (callbackJ) {
        modeCount++;
        action->type = CTL_TYPE_CB;
        modeCount++;        
        err = PluginGetCB (apiHandle, action, callbackJ);
        if (err) goto OnErrorExit;       
    }

    // make sure at least one mode is selected
    if (modeCount == 0) {
        AFB_ApiError(apiHandle,"ACTION-LOAD-ONE No Action Selected lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }

    if (modeCount > 1) {
        AFB_ApiError(apiHandle,"ACTION-LOAD-ONE:ToMany arguments lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }
    return 0;

OnErrorExit:
    return 1;
};

PUBLIC CtlActionT *ActionConfig(AFB_ApiT apiHandle, json_object *actionsJ, int exportApi) {
    int err;
    CtlActionT *actions;

    // action array is close with a nullvalue;
    if (json_object_get_type(actionsJ) == json_type_array) {
        int count = json_object_array_length(actionsJ);
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

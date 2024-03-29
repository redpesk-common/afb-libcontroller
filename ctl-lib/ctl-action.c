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

int ActionLabelToIndex(CtlActionT*actions, const char* actionLabel) {
    if (actions) {
        for (int idx = 0; actions[idx].uid; idx++) {
            if (!strcasecmp(actionLabel, actions[idx].uid)) return idx;
        }
    }
    return -1;
}

void ActionExecUID(afb_req_t request, CtlConfigT *ctlConfig, const char *uid, json_object *queryJ) {
    for (int i = 0; ctlConfig->sections[i].key != NULL; i++) {
        if (ctlConfig->sections[i].actions) {
            for (int j = 0; ctlConfig->sections[i].actions[j].uid != NULL; j++) {
                if (strcmp(ctlConfig->sections[i].actions[j].uid, uid) == 0) {
                    CtlSourceT source;
                    source.uid = ctlConfig->sections[i].actions[j].uid;
                    source.api = ctlConfig->sections[i].actions[j].api;
                    source.request = request;

                    ActionExecOne(&source, &ctlConfig->sections[i].actions[j], queryJ);
                }
            }
        }
    }
}

void HandleApiCallActionResponseFromVerbCall(void *data, struct json_object *responseJ, const char *error, const char *info, afb_req_t request) {
    CtlActionT *action = (CtlActionT *) data;

    if (!action) {
        afb_req_fail_f(request,
                       "Action is null, won't be able to handle the response, error=%s info=%s response=%s",
                       error ? error : "NULL",
                       info ? info : "NULL",
                       responseJ ? json_object_get_string(responseJ) : "NULL");
    }
    else if (error) {
        afb_req_fail_f(request,
                       "subcall-fail",
                       "An error happened during verb call, error=%s info=%s uid=%s api=%s verb=%s args=%s response=%s",
                       error ? error : "NULL",
                       info ? info : "NULL",
                       action->uid,
                       action->exec.subcall.api,
                       action->exec.subcall.verb,
                       json_object_get_string(action->argsJ),
                       responseJ ? json_object_get_string(responseJ) : "NULL");
    }
    else {
        afb_req_success(request, responseJ ? json_object_get(responseJ) : NULL, info);
    }
}

void HandleApiCallActionResponseFromEvent(void *data, struct json_object *responseJ, const char *error, const char *info, afb_api_t apiHandle) {
    CtlActionT *action = (CtlActionT *) data;

    if (!action) {
        AFB_API_ERROR(apiHandle,
                      "[action-null], won't be able to handle the response, error=%s info=%s response=%s",
                      error ? error : "NULL",
                      info ? info : "NULL",
                      responseJ ? json_object_get_string(responseJ) : "NULL");
    }
    else if (error) {
        AFB_API_ERROR(apiHandle,
                      "[subcall-fail], error=%s info=%s uid=%s api=%s verb=%s args=%s response=%s",
                      error ? error : "NULL",
                      info ? info : "NULL",
                      action->uid,
                      action->exec.subcall.api,
                      action->exec.subcall.verb,
                      json_object_get_string(action->argsJ),
                      responseJ ? json_object_get_string(responseJ) : "NULL");
    }
    else {
        AFB_API_DEBUG(apiHandle,
                      "[subcall-ok], error=%s info=%s uid=%s api=%s verb=%s args=%s response=%s",
                      error ? error : "NULL",
                      info ? info : "NULL",
                      action->uid,
                      action->exec.subcall.api,
                      action->exec.subcall.verb,
                      json_object_get_string(action->argsJ),
                      responseJ ? json_object_get_string(responseJ) : "NULL");
    }
}

int ActionExecOne(CtlSourceT *source, CtlActionT* action, json_object *queryJ) {
    int err = 0;

    switch (action->type) {
        case CTL_TYPE_API:
        {
            json_object *extendedQueryJ = NULL;

            if (action->argsJ) {
                switch(json_object_get_type(queryJ)) {
                    case json_type_object: {
                        extendedQueryJ = wrap_json_clone(queryJ);

                        wrap_json_object_add(extendedQueryJ, action->argsJ);
                        break;
                    }

                    case json_type_null:
                        extendedQueryJ = json_object_get(action->argsJ);
                        break;

                    default:
                        AFB_API_ERROR(action->api, "ActionExecOne(queryJ should be an object) uid=%s args=%s", source->uid, json_object_get_string(queryJ));
                        return -1;
                }
            }
            else {
                extendedQueryJ = json_object_get(queryJ);
            }

            /* AFB Subcall will release the json_object doing the json_object_put() call */
            if (!source->request) {
                afb_api_call(action->api,
                             action->exec.subcall.api,
                             action->exec.subcall.verb,
                             extendedQueryJ,
                             &HandleApiCallActionResponseFromEvent,
                             (void *) action);
            }
            else {
                afb_req_subcall(source->request,
                                action->exec.subcall.api,
                                action->exec.subcall.verb,
                                extendedQueryJ,
                                afb_req_subcall_pass_events | afb_req_subcall_on_behalf,
                                &HandleApiCallActionResponseFromVerbCall,
                                (void *) action);
            }
            break;
        }

#ifdef CONTROL_SUPPORT_LUA
        case CTL_TYPE_LUA:
            err = LuaCallFunc(source, action, queryJ);
            if (err) {
                AFB_API_ERROR(action->api, "ActionExecOne(Lua) uid=%s func=%s args=%s", source->uid, action->exec.lua.funcname, json_object_get_string(action->argsJ));
            }
            break;
#endif

        case CTL_TYPE_CB:
            source->plugin = action->exec.cb.plugin;
            err = (*action->exec.cb.callback) (source, action->argsJ, queryJ);
            if (err) {
                AFB_API_ERROR(action->api, "ActionExecOne(Callback) uid%s plugin=%s function=%s args=%s", source->uid, action->exec.cb.plugin->uid, action->exec.cb.funcname, json_object_get_string(action->argsJ));
            }
            break;

        default:
        {
            AFB_API_ERROR(action->api, "ActionExecOne(unknown) API type uid=%s", source->uid);
            break;
        }
    }

    return err;
}


// Direct Request Call in APIV3
static void ActionDynRequest(afb_req_t request) {
    // retrieve action handle from request and execute the request
    json_object *queryJ = afb_req_json(request);
    CtlActionT* action = (CtlActionT*) afb_req_get_vcbdata(request);

    CtlSourceT source;
    source.uid = action->uid;
    source.request = request;
    source.api = action->api;

    // provide request and execute the action
    ActionExecOne(&source, action, queryJ);
}

void ParseURI(const char *uri, char **first, char **second) {
    char *tmp;

    if(! uri || ! first || ! second) {
        return;
    }

    tmp = strdup(uri);
    if (!tmp) {
        *first = NULL;
        *second = NULL;
        return;
    }

    *first = tmp;

    tmp = strchrnul(tmp, '#');
    if(tmp[0] == '\0') {
        *second = NULL;
    }
    else {
        tmp[0] = '\0';
        *second = &tmp[1];
    }
}

/*** This function will fill the CtlActionT pointer given in parameters for a
 * given api using an 'uri' that specify the C plugin to use and the name of
 * the function
 *
 */
static int BuildPluginAction(afb_api_t apiHandle, const char *uri, CtlActionT *action) {
    char *plugin = NULL, *function = NULL;
    json_object *callbackJ = NULL;

    if (!action) {
        AFB_API_ERROR(apiHandle, "Action not valid");
        return -1;
    }

    ParseURI(uri, &plugin, &function);

    action->type = CTL_TYPE_CB;

    if (plugin && function) {
        if (wrap_json_pack(&callbackJ, "{ss,ss,s?o*}",
                "plugin", plugin,
                "function", function,
                "args", action->argsJ)) {
            AFB_API_ERROR(apiHandle, "Error packing Callback JSON object for plugin %s and function %s", uri, function);
            return -1;
        } else {
            return PluginGetCB(apiHandle, action, callbackJ);
        }
    } else {
        AFB_API_ERROR(apiHandle, "Miss something uri or function.");
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
static int BuildApiAction(afb_api_t apiHandle, const char *uri, CtlActionT *action) {
    char *api = NULL, *verb = NULL;

    if (!action) {
        AFB_API_ERROR(apiHandle, "Action not valid");
        return -1;
    }

    ParseURI(uri, &api, &verb);

    if(!api || !verb) {
        AFB_API_ERROR(apiHandle, "Error parsing the action string");
        return -1;
    }

    action->type = CTL_TYPE_API;
    action->exec.subcall.api = api;
    action->exec.subcall.verb = verb;

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

static int BuildLuaAction(afb_api_t apiHandle, const char *uri, CtlActionT *action) {
    char *plugin = NULL, *function = NULL;

    if (!action) {
        AFB_API_ERROR(apiHandle, "Action not valid");
        return -1;
    }

    ParseURI(uri, &plugin, &function);

    if(!plugin || !function) {
        AFB_API_ERROR(apiHandle, "Error parsing the action string");
        return -1;
    }

    action->type = CTL_TYPE_LUA;
    action->exec.lua.plugin = plugin;
    action->exec.lua.funcname = function;

    return 0;
}
#endif

static int BuildOneAction(afb_api_t apiHandle, CtlActionT *action, const char *uri) {
    size_t lua_pre_len = strlen(LUA_ACTION_PREFIX);
    size_t api_pre_len = strlen(API_ACTION_PREFIX);
    size_t plugin_pre_len = strlen(PLUGIN_ACTION_PREFIX);

    if (uri && action) {
        if (!strncasecmp(uri, LUA_ACTION_PREFIX, lua_pre_len)) {
#ifdef CONTROL_SUPPORT_LUA
            return BuildLuaAction(apiHandle, &uri[lua_pre_len], action);
#else
            AFB_API_ERROR(apiHandle, "LUA support not selected at build. Feature disabled");
            return -1;
#endif
        } else if (!strncasecmp(uri, API_ACTION_PREFIX, api_pre_len)) {
            return BuildApiAction(apiHandle, &uri[api_pre_len], action);
        } else if (!strncasecmp(uri, PLUGIN_ACTION_PREFIX, plugin_pre_len)) {
            return BuildPluginAction(apiHandle, &uri[plugin_pre_len], action);
        } else {
            AFB_API_ERROR(apiHandle, "Wrong uri specified. You have to specified 'lua://', 'plugin://' or 'api://'.");
            return -1;
        }
    }

    AFB_API_ERROR(apiHandle, "Uri, Action or function not valid");
    return -1;
}

// unpack individual action object

int ActionLoadOne(afb_api_t apiHandle, CtlActionT *action, json_object *actionJ, int exportApi) {
    int err = 0;
    const char *uri = NULL;

    memset(action, 0, sizeof (CtlActionT));

    if (actionJ) {
        err = wrap_json_unpack(actionJ, "{ss,s?s,ss,s?s,s?o}",
                "uid", &action->uid,
                "info", &action->info,
                "action", &uri,
                "privileges", &action->privileges,
                "args", &action->argsJ);
        if (!err) {
            if(!apiHandle)
                return -1;
            action->api = apiHandle;
            if (exportApi) {
                err = afb_api_add_verb(apiHandle, action->uid, action->info, ActionDynRequest, action, NULL, 0, 0);
                if (err) {
                    AFB_API_ERROR(apiHandle, "ACTION-LOAD-ONE fail to register API verb=%s", action->uid);
                    return -1;
                }
            }
            err = BuildOneAction(apiHandle, action, uri);
        } else {
            AFB_API_ERROR(apiHandle, "Fail to parse action JSON : (%s)", json_object_to_json_string(actionJ));
            err = -1;
        }
    } else {
        AFB_API_ERROR(apiHandle, "Wrong action JSON object parameter: (%s)", json_object_to_json_string(actionJ));
        err = -1;
    }

    return err;
}

CtlActionT *ActionConfig(afb_api_t apiHandle, json_object *actionsJ, int exportApi) {
    int err;
    CtlActionT *actions;

    // action array is close with a nullvalue;
    if (json_object_is_type(actionsJ, json_type_array)) {
        int count = (int)json_object_array_length(actionsJ);
        actions = calloc(count + 1, sizeof (CtlActionT));

        for (int idx = 0; idx < count; idx++) {
            json_object *actionJ = json_object_array_get_idx(actionsJ, idx);

            err = ActionLoadOne(apiHandle, &actions[idx], actionJ, exportApi);
            if (err)
                return NULL;
        }

    } else {
        actions = calloc(2, sizeof (CtlActionT));
        err = ActionLoadOne(apiHandle, &actions[0], actionsJ, exportApi);
        if (err)
            return NULL;
    }

    return actions;
}

int AddActionsToSection(afb_api_t apiHandle, CtlSectionT *section, json_object *actionsJ, int exportApi) {
    if (actionsJ || section) {
        CtlActionT *actions = NULL;
        if(section->actions) {
            int actionsNb = 0, idx = 0, jdx = 0;
            CtlActionT *savedActions = section->actions;
            CtlActionT *newActions = ActionConfig(apiHandle, actionsJ, exportApi);

            while(savedActions[actionsNb].uid)
                actionsNb++;
            while(newActions[actionsNb].uid)
                actionsNb++;

            actions = calloc(actionsNb + 1, sizeof(CtlActionT));

            while(savedActions[idx].uid) {
                actions[idx] = savedActions[idx];
                idx++;
            }
            while(newActions[jdx].uid && idx <= actionsNb) {
                actions[idx] = newActions[jdx];
                idx++;
                jdx++;
            }
            free(savedActions);
            free(newActions);
        }
        else {
            actions = ActionConfig(apiHandle, actionsJ, exportApi);
        }
        section->actions = actions;
    }
    else {
        AFB_API_ERROR (apiHandle, "Missing actions to add or the section object");
        return 1;
    }

    if (!section->actions) {
        AFB_API_ERROR (apiHandle, "Adding '%s' fails to section %s", json_object_get_string(actionsJ), section->uid);
        return 1;
    }
        return 0;
}

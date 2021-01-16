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
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "ctl-config.h"

// in V3+V4 event are call directly by afb-binder
void ExecOneEvent (void *ctx, const char *name, json_object *eventJ, afb_api_t apiHandle) {
    CtlActionT *action = (CtlActionT*) ctx;
    CtlSourceT source;
    source.uid = action->uid;
    source.api  = action->api;
    source.request = NULL;

    (void) ActionExecOne (&source, action, eventJ);
}

// onload section receive one action or an array of actions
int EventConfig(afb_api_t apiHandle, CtlSectionT *section, json_object *eventsJ) {
    int err = 0;
    // Load time parse actions in config file
    if (eventsJ != NULL) {
        if ( (err= AddActionsToSection(apiHandle, section, eventsJ, 0)) ) {
            AFB_API_ERROR (apiHandle, "EventLoad config fail processing actions for section %s", section->uid);
            return err;
        }
    } else if (section->actions){
        // register event to corresponding actions
        CtlActionT* actions= section->actions;
        for (int idx=0; actions[idx].uid; idx++) {
            afb_api_event_handler_add (apiHandle, actions[idx].uid, ExecOneEvent, &actions[idx]);
        }
    }

    return err;
}

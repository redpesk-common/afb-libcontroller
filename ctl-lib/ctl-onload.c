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

// onload section receive one action or an array of actions
int OnloadConfig(afb_api_t apiHandle, CtlSectionT *section, json_object *actionsJ) {
    int err = 0;

    if (actionsJ != NULL) {
        // Load time parse actions in control file
        err = AddActionsToSection(apiHandle, section, actionsJ, 0);
        if (err < 0) {
            AFB_API_ERROR (apiHandle, "OnloadConfig control fail processing actions for section %s", section->uid);
        }
    } else {
        // Exec time process onload action now
        if (!section->actions) {
            AFB_API_NOTICE (apiHandle, "OnloadConfig Cannot Exec Non Existing Onload Action");
            return 1;
        }

        for (int idx=0; section->actions[idx].uid != NULL; idx ++) {
            CtlSourceT source;
            source.uid = section->actions[idx].uid;
            source.api  = section->actions[idx].api;
            source.request = NULL;

            err = ActionExecOne(&source, &section->actions[idx], NULL);
            if(err < 0) {
                AFB_API_ERROR(apiHandle, "Onload action execution failed on: %s", source.uid);
                return err;
            }
        }
    }

    return err;
}

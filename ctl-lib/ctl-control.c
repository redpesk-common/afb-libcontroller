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
int ControlConfig(afb_api_t apiHandle, CtlSectionT *section, json_object *actionsJ) {
    int err = 0;
    if (actionsJ) {
        if ( (err= AddActionsToSection(apiHandle, section, actionsJ, 1)) ) {
            AFB_API_ERROR (apiHandle, "ControlLoad config fail processing actions for section %s", section->uid);
            return err;
        }
    }
    return err;
}

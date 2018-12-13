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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ctl-binding.h"



// Include Binding Stub generated from Json OpenAPI
#include "ctl-apidef.h"


PUBLIC void ctlapi_monitor (afb_req request) {

    // subscribe Client to event
    int err = afb_req_subscribe(request, TimerEvtGet());
    if (err != 0) {
        afb_req_fail_f(request, "register-event", "Fail to subscribe binder event");
        goto OnErrorExit;
    }

    afb_req_success(request, NULL, NULL);

 OnErrorExit:
    return;
}

// Create Binding Event at Init
PUBLIC int CtlBindingInit () {

    int errcount=0;

    errcount += TimerEvtInit();
    errcount += DispatchInit();
#ifdef CONTROL_SUPPORT_LUA
    errcount += LuaLibInit();
#endif

    const char *profile= getenv("CONTROL_ONLOAD_PROFILE");
    if (!profile) profile=CONTROL_ONLOAD_PROFILE;

    // now that everything is initialised execute the onload action
    if (!errcount)
        errcount += DispatchOnLoad(CONTROL_ONLOAD_PROFILE);

    AFB_DEBUG ("Audio Policy Control Binding Done errcount=%d", errcount);
    return errcount;
}


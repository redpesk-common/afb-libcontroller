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
#include <sys/time.h>


#include "filescan-utils.h"
#include "ctl-config.h"


// Load control config file

PUBLIC int CtlConfigMagicNew() {
  static int InitRandomDone=0;

  if (!InitRandomDone) {
    struct timeval tv;
    InitRandomDone=1;
    gettimeofday(&tv,NULL);
    srand ((int)tv.tv_usec);
  }
  
  return ((long)rand());
}

PUBLIC  json_object* CtlConfigScan(const char *dirList, const char *prefix) {
    char controlFile [CONTROL_MAXPATH_LEN];
    strncpy(controlFile, prefix, CONTROL_MAXPATH_LEN);
    strncat(controlFile, GetBinderName(), CONTROL_MAXPATH_LEN);

    // search for default dispatch config file
    json_object* responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, controlFile, ".json");

    return responseJ;
}

PUBLIC char* CtlConfigSearch(AFB_ApiT apiHandle, const char *dirList, const char *prefix) {
    int index, err;
    
    // search for default dispatch config file
    json_object* responseJ = CtlConfigScan (dirList, prefix);

    // We load 1st file others are just warnings
    for (index = 0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ = json_object_array_get_idx(responseJ, index);

        char *filename;
        char*fullpath;
        err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ApiError(apiHandle, "CTL-INIT HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
            return NULL;
        }

        if (index == 0) {
            char filepath[CONTROL_MAXPATH_LEN];
            strncpy(filepath, fullpath, sizeof (filepath));
            strncat(filepath, "/", sizeof (filepath));
            strncat(filepath, filename, sizeof (filepath));
            return (strdup(filepath));
        }
    }

    // no config found
    return NULL;
}

PUBLIC int CtlConfigExec(AFB_ApiT apiHandle, CtlConfigT *ctlConfig) {
    
    // best effort to initialise everything before starting
    if (ctlConfig->requireJ) {

        void DispatchRequireOneApi(json_object * bindindJ) {
            const char* requireBinding = json_object_get_string(bindindJ);
            int err = AFB_RequireApi(apiHandle, requireBinding, 1);
            if (err) {
                AFB_ApiWarning(apiHandle, "CTL-LOAD-CONFIG:REQUIRE Fail to get=%s", requireBinding);
            }
        }

        if (json_object_get_type(ctlConfig->requireJ) == json_type_array) {
            for (int idx = 0; idx < json_object_array_length(ctlConfig->requireJ); idx++) {
                DispatchRequireOneApi(json_object_array_get_idx(ctlConfig->requireJ, idx));
            }
        } else {
            DispatchRequireOneApi(ctlConfig->requireJ);
        }
    }

#ifdef CONTROL_SUPPORT_LUA
    int err= LuaConfigExec(apiHandle);
    if (err) goto OnErrorExit;
#endif

    // Loop on every section and process config
    int errcount=0;
    for (int idx = 0; ctlConfig->sections[idx].key != NULL; idx++) {
        errcount += ctlConfig->sections[idx].loadCB(apiHandle, &ctlConfig->sections[idx], NULL);
    }
    return errcount;

OnErrorExit:
    return 1;
}

PUBLIC CtlConfigT *CtlLoadMetaData(AFB_ApiT apiHandle, const char* filepath) {
    json_object *ctlConfigJ;
    CtlConfigT *ctlHandle=NULL;
    int err;

    // Load JSON file
    ctlConfigJ = json_object_from_file(filepath);
    if (!ctlConfigJ) {
        AFB_ApiError(apiHandle, "CTL-LOAD-CONFIG Not invalid JSON %s ", filepath);
        goto OnErrorExit;
    }

    AFB_ApiInfo(apiHandle, "CTL-LOAD-CONFIG: loading config filepath=%s", filepath);

    json_object *metadataJ;
    int done = json_object_object_get_ex(ctlConfigJ, "metadata", &metadataJ);
    if (done) {
        ctlHandle = calloc(1, sizeof (CtlConfigT));
        err = wrap_json_unpack(metadataJ, "{ss,ss,ss,s?s,s?o !}", "label", &ctlHandle->label, "version", &ctlHandle->version
                , "api", &ctlHandle->api, "info", &ctlHandle->info, "require", &ctlHandle->requireJ);
        if (err) {
            AFB_ApiError(apiHandle, "CTL-LOAD-CONFIG:METADATA Missing something label|api|version|[info]|[require] in:\n-- %s", json_object_get_string(metadataJ));
            goto OnErrorExit;
        }
    }
    
    ctlHandle->configJ = ctlConfigJ;   
    return ctlHandle;
    
OnErrorExit:
    if (ctlHandle) free(ctlHandle);
    return NULL;
}

PUBLIC int CtlLoadSections(AFB_ApiT apiHandle, CtlConfigT *ctlHandle, CtlSectionT *sections) {
    int err;

#ifdef CONTROL_SUPPORT_LUA
    err= LuaConfigLoad(apiHandle);
    if (err) goto OnErrorExit;
#endif

    err = 0;
    ctlHandle->sections = sections;
    for (int idx = 0; sections[idx].key != NULL; idx++) {
        json_object * sectionJ;
        int done = json_object_object_get_ex(ctlHandle->configJ, sections[idx].key, &sectionJ);
        if (!done) {
            AFB_ApiError(apiHandle, "CtlConfigLoad: fail to find '%s' section in config '%s'", sections[idx].key, ctlHandle->label);
            err++;
        } else {
            err += sections[idx].loadCB(apiHandle, &sections[idx], sectionJ);
        }
    }
    if (err) goto OnErrorExit;

    return 0;

OnErrorExit:
    return 1;
}





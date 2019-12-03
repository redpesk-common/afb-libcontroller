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

extern void* getExternalData(CtlConfigT *ctlConfig) {
    return ctlConfig->external;
}

extern void setExternalData(CtlConfigT *ctlConfig, void *data) {
    ctlConfig->external = data;
}
int CtlConfigMagicNew() {
  static int InitRandomDone=0;

  if (!InitRandomDone) {
    struct timeval tv;
    InitRandomDone=1;
    gettimeofday(&tv,NULL);
    srand ((int)tv.tv_usec);
  }

  return ((long)rand());
}

json_object* CtlConfigScan(const char *dirList, const char *prefix) {
    char controlFile[CONTROL_MAXPATH_LEN];
    const char *binderName = GetBinderName();

    controlFile[CONTROL_MAXPATH_LEN - 1] = '\0';

    if(prefix && prefix[0] != '\0') {
        strncpy(controlFile, prefix, CONTROL_MAXPATH_LEN - 1);
        strncat(controlFile, "-", CONTROL_MAXPATH_LEN - strlen(controlFile) - 1);
        strncat(controlFile, binderName, CONTROL_MAXPATH_LEN - strlen(controlFile) - 1);
    }
    else {
        strncpy(controlFile, binderName, CONTROL_MAXPATH_LEN - 1);
    }
    // search for default dispatch config file
    json_object* responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, controlFile, ".json");

    return responseJ;
}

char* ConfigSearch(afb_api_t apiHandle, json_object *responseJ) {
    // We load 1st file others are just warnings
    size_t p_length;
    char *filepath = NULL;
    const char *filename;
    const char*fullpath;

    for (int index = 0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ = json_object_array_get_idx(responseJ, index);

        int err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_API_ERROR(apiHandle, "CTL-INIT HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
        }

        if (index == 0) {
            p_length = strlen(fullpath) + 1 + strlen(filename);
            filepath = malloc(p_length + 1);

            strncpy(filepath, fullpath, p_length);
            strncat(filepath, "/", p_length - strlen(filepath));
            strncat(filepath, filename, p_length - strlen(filepath));
        }
        else {
            AFB_API_WARNING(apiHandle, "CTL-INIT JSON file found but not used : %s/%s", fullpath, filename);
        }
    }

    json_object_put(responseJ);
    return filepath;
}

char* CtlConfigSearch(afb_api_t apiHandle, const char *dirList, const char *prefix) {
    // search for default dispatch config file
    json_object* responseJ = CtlConfigScan (dirList, prefix);

    if(responseJ)
        return ConfigSearch(apiHandle, responseJ);

    return NULL;
}

static int DispatchRequireOneApi(afb_api_t apiHandle, json_object * bindindJ) {
    const char* requireBinding = json_object_get_string(bindindJ);
    int err = afb_api_require_api(apiHandle, requireBinding, 1);
    if (err) {
        AFB_API_WARNING(apiHandle, "CTL-LOAD-CONFIG:REQUIRE Fail to get=%s", requireBinding);
    }

    return err;
}

/**
 * @brief Best effort to initialise everything before starting
 * Call afb_require_api at the first call or if there was an error because
 * the CtlConfigExec could be called anywhere and not in binding init.
 * So you could call this function at init time.
 *
 * @param apiHandle : a afb_daemon api handle, see afb_api_t in afb_definitions.h
 * @param requireJ : json_object array of api name required.
 */
void DispatchRequireApi(afb_api_t apiHandle, json_object * requireJ) {
    static int init = 0, err = 0;
    int idx;

    if ( (! init || err) && requireJ) {
        if (json_object_get_type(requireJ) == json_type_array) {
            for (idx = 0; idx < json_object_array_length(requireJ); idx++) {
                err += DispatchRequireOneApi(apiHandle, json_object_array_get_idx(requireJ, idx));
            }
        } else {
            err += DispatchRequireOneApi(apiHandle, requireJ);
        }
    }

    init = 1;
}

int CtlConfigExec(afb_api_t apiHandle, CtlConfigT *ctlConfig) {

    DispatchRequireApi(apiHandle, ctlConfig->requireJ);
#ifdef CONTROL_SUPPORT_LUA
    // load static LUA utilities
    LuaConfigExec(apiHandle);
#endif

    // Loop on every section and process config
    int error;
    for (int idx = 0; ctlConfig->sections[idx].key != NULL; idx++) {
        if (!ctlConfig->sections[idx].loadCB) {
            AFB_API_NOTICE(apiHandle, "Notice empty section '%s'", ctlConfig->sections[idx].key);
            continue;
        }

        error = ctlConfig->sections[idx].loadCB(apiHandle, &ctlConfig->sections[idx], NULL);
        if (error < 0) {
            AFB_API_ERROR(apiHandle, "Error %i caught during call to '%s' section callback", error, ctlConfig->sections[idx].key);
            return -(idx + 1);
        }
        else if (error > 0) {
            AFB_API_WARNING(apiHandle, "Warning %i raised during call to '%s' section callback", error, ctlConfig->sections[idx].key);
        }
    }

    return 0;
}

CtlConfigT *CtlLoadMetaDataJson(afb_api_t apiHandle, json_object *ctlConfigJ) {
    json_object *metadataJ;
    CtlConfigT *ctlHandle=NULL;

    int done = json_object_object_get_ex(ctlConfigJ, "metadata", &metadataJ);
    if (done) {
        ctlHandle = calloc(1, sizeof (CtlConfigT));
        if (wrap_json_unpack(metadataJ, "{ss,ss,ss,s?s,s?o,s?s,s?s !}",
                                        "uid", &ctlHandle->uid,
                                        "version", &ctlHandle->version,
                                        "api", &ctlHandle->api,
                                        "info", &ctlHandle->info,
                                        "require", &ctlHandle->requireJ,
                                        "author", &ctlHandle->author,
                                        "date", &ctlHandle->date)) {
            AFB_API_ERROR(apiHandle, "CTL-LOAD-CONFIG:METADATA Missing something uid|api|version|[info]|[require]|[author]|[date] in:\n-- %s", json_object_get_string(metadataJ));
            free(ctlHandle);
            return NULL;
        }
        ctlHandle->configJ = ctlConfigJ;
    }

    return ctlHandle;
}

CtlConfigT *CtlLoadMetaData(afb_api_t apiHandle,const char* filepath) {
    json_object *ctlConfigJ;


    // Load JSON file
    ctlConfigJ = json_object_from_file(filepath);
    if (!ctlConfigJ) {
        AFB_API_ERROR(apiHandle, "CTL-LOAD-CONFIG Not invalid JSON %s ", filepath);
        return NULL;
    }

    AFB_API_INFO(apiHandle, "CTL-LOAD-CONFIG: loading config filepath=%s", filepath);

    return CtlLoadMetaDataJson(apiHandle, ctlConfigJ);
}

void wrap_json_array_add(void* array, json_object *val) {
    json_object_array_add(array, (json_object*)val);
}

json_object* LoadAdditionalsFiles(afb_api_t apiHandle, CtlConfigT *ctlHandle, const char *key, json_object *sectionJ);

json_object* CtlUpdateSectionConfig(afb_api_t apiHandle, CtlConfigT *ctlHandle, const char *key, json_object *sectionJ, json_object *filesJ) {

    json_object *sectionArrayJ;
    char *oneFile = NULL;
    const char *bindingParentPath = GetBindingParentDirPath(apiHandle);

    if(! json_object_is_type(sectionJ, json_type_array)) {
        sectionArrayJ = json_object_new_array();
        if(json_object_object_length(sectionJ) > 0)
            json_object_array_add(sectionArrayJ, sectionJ);
    }
    else sectionArrayJ = sectionJ;

    json_object_get(sectionJ);
    json_object_object_del(ctlHandle->configJ, key);
    json_object_object_add(ctlHandle->configJ, key, sectionArrayJ);

    if (json_object_get_type(filesJ) == json_type_array) {
        int length = (int)json_object_array_length(filesJ);
        for (int idx=0; idx < length; idx++) {
            json_object *oneFileJ = json_object_array_get_idx(filesJ, idx);
            json_object *responseJ =
                ScanForConfig(bindingParentPath, CTL_SCAN_RECURSIVE, json_object_get_string(oneFileJ), ".json");
            if(!responseJ) {
                AFB_API_ERROR(apiHandle, "No config files found in search path. No changes has been made\n -- %s", bindingParentPath);
                return sectionArrayJ;
            }
            oneFile = ConfigSearch(apiHandle, responseJ);
            if (oneFile) {
                json_object *newSectionJ, *newFileJ = json_object_from_file(oneFile);
                json_object_object_get_ex(newFileJ, key, &newSectionJ);
                json_object_get(newSectionJ);
                json_object_put(newFileJ);
                LoadAdditionalsFiles(apiHandle, ctlHandle, key, newSectionJ);
                json_object_object_get_ex(ctlHandle->configJ, key, &sectionArrayJ);
                wrap_json_optarray_for_all(newSectionJ, wrap_json_array_add, sectionArrayJ);
            }
        }
    } else {
        json_object *responseJ =
            ScanForConfig(bindingParentPath, CTL_SCAN_RECURSIVE, json_object_get_string(filesJ), ".json");
        if(!responseJ) {
            AFB_API_ERROR(apiHandle, "No config files found in search path. No changes has been made\n -- %s", bindingParentPath);
            return sectionArrayJ;
        }
        oneFile = ConfigSearch(apiHandle, responseJ);
        json_object *newSectionJ = json_object_from_file(oneFile);
        LoadAdditionalsFiles(apiHandle, ctlHandle, key, newSectionJ);
        wrap_json_optarray_for_all(newSectionJ, wrap_json_array_add, sectionArrayJ);
    }

    free(oneFile);
    return sectionArrayJ;
}

json_object* LoadAdditionalsFiles(afb_api_t apiHandle, CtlConfigT *ctlHandle, const char *key, json_object *sectionJ)
{
    json_object *filesJ, *filesArrayJ = json_object_new_array();
    if (json_object_get_type(sectionJ) == json_type_array) {
        int length = (int)json_object_array_length(sectionJ);
        for (int idx=0; idx < length; idx++) {
            json_object *obj = json_object_array_get_idx(sectionJ, idx);
            int hasFiles = json_object_object_get_ex(obj, "files", &filesJ);
            if(hasFiles) {
                // Clean files key as we don't want to make infinite loop
                json_object_get(filesJ);
                json_object_object_del(obj, "files");
                if(json_object_is_type(filesJ, json_type_array))
                    wrap_json_array_for_all(filesJ, wrap_json_array_add, filesArrayJ);
                else
                    json_object_array_add(filesArrayJ, filesJ);
            }
        }
    } else {
        int hasFiles = json_object_object_get_ex(sectionJ, "files", &filesJ);
        if(hasFiles) {
            // Clean files key as we don't want to make infinite loop
            json_object_get(filesJ);
            json_object_object_del(sectionJ, "files");
            if(json_object_is_type(filesJ, json_type_array))
                filesArrayJ = filesJ;
            else
                json_object_array_add(filesArrayJ, filesJ);
        }
    }

    if(json_object_array_length(filesArrayJ) > 0)
        sectionJ = CtlUpdateSectionConfig(apiHandle, ctlHandle, key, sectionJ, filesArrayJ);
    json_object_put(filesArrayJ);
    return sectionJ;
}

int CtlLoadSections(afb_api_t apiHandle, CtlConfigT *ctlHandle, CtlSectionT *sections) {
    int error;

#ifdef CONTROL_SUPPORT_LUA
    if (LuaConfigLoad(apiHandle))
        return -1;
#endif

    ctlHandle->sections = sections;
    for (int idx = 0; sections[idx].key != NULL; idx++) {
        json_object * sectionJ;
        if (json_object_object_get_ex(ctlHandle->configJ, sections[idx].key, &sectionJ)) {
            json_object* updatedSectionJ = LoadAdditionalsFiles(apiHandle, ctlHandle, sections[idx].key, sectionJ);

            if (!sections[idx].loadCB) {
                AFB_API_NOTICE(apiHandle, "Notice empty section '%s'", sections[idx].key);
                continue;
            }

            error = sections[idx].loadCB(apiHandle, &sections[idx], updatedSectionJ);
            if (error < 0) {
                AFB_API_ERROR(apiHandle, "Error %i caught during call to '%s' section callback", error, sections[idx].key);
                return -(idx + 1);
            }
            else if (error > 0) {
                AFB_API_WARNING(apiHandle, "Warning %i raised during call to '%s' section callback", error, sections[idx].key);
            }
        }
    }

    return 0;
}

char *GetDefaultConfigSearchPath(afb_api_t apiHandle)
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
     * between binderRootDirPath and bindingParentDirPath + 2*4 char for '/etc suffixes'.
     */
    searchPathLength = strlen(binderRootDirPath) + strlen(bindingParentDirPath) + 10;

    searchPath = malloc(searchPathLength);
    if(! searchPath) {
        free(binderRootDirPath);
        free(bindingParentDirPath);
        return NULL;
    }

    snprintf(searchPath, searchPathLength, "%s/etc:%s/etc", binderRootDirPath, bindingParentDirPath);

    free(binderRootDirPath);
    free(bindingParentDirPath);

    return searchPath;
}

/*
 * Copyright (C) 2016-2018 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 * Contrib Jonathan Aillet <jonathan.aillet@iot.bzh>
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

#ifndef _AFB_DEFINITIONS_INCLUDE_
#define _AFB_DEFINITIONS_INCLUDE_

// Legacy DYNAPI support. Deprecated and will be removed.
#if((AFB_BINDING_VERSION == 0) && defined(AFB_BINDING_WANT_DYNAPI))
    #include <afb/afb-binding.h>

    #define AFB_BINDING_PREV3
    #define AFB_ReqNone NULL
    typedef struct afb_request* AFB_ReqT;
    typedef struct afb_dynapi*  AFB_ApiT;

    typedef struct afb_eventid* AFB_EventT;
    #define AFB_EventIsValid(eventid) eventid
    #define AFB_EventPush afb_eventid_push
    #define AFB_ReqSubscribe  afb_request_subscribe
    #define AFB_EventMake(api, name) afb_dynapi_make_eventid(api, name)

    #define AFB_ReqJson(request) afb_request_json(request)

    #define AFB_ReqSuccess  afb_request_success
    #define AFB_ReqSuccessF afb_request_success_f
    #define AFB_ReqFail    afb_request_fail
    #define AFB_ReqFailF   afb_request_fail_f

    #define AFB_ReqNotice(request, ...)   AFB_REQUEST_NOTICE (request, __VA_ARGS__)
    #define AFB_ReqWarning(request, ...)  AFB_REQUEST_WARNING (request, __VA_ARGS__)
    #define AFB_ReqDebug(request, ...)    AFB_REQUEST_DEBUG (request, __VA_ARGS__)
    #define AFB_ReqError(request, ...)    AFB_REQUEST_ERROR (request, __VA_ARGS__)
    #define AFB_ReqInfo(request, ...)     AFB_REQUEST_INFO (request, __VA_ARGS__)

    #define AFB_ApiVerbose(api, level, ...) afb_dynapi_verbose(api, level, __VA_ARGS__)
    #define AFB_ApiNotice(api, ...)   AFB_DYNAPI_NOTICE (api, __VA_ARGS__)
    #define AFB_ApiWarning(api, ...)  AFB_DYNAPI_WARNING (api, __VA_ARGS__)
    #define AFB_ApiDebug(api, ...)    AFB_DYNAPI_DEBUG (api, __VA_ARGS__)
    #define AFB_ApiError(api, ...)    AFB_DYNAPI_ERROR (api, __VA_ARGS__)
    #define AFB_ApiInfo(api, ...)     AFB_DYNAPI_INFO (api, __VA_ARGS__)

    #define AFB_GetApiSettings(api) json_object_new_object()

    #define AFB_ReqIsValid(request)   request
    #define AFB_EvtIsValid(evtHandle) evtHandle

    #define AFB_ServiceCall(api, ...) afb_dynapi_call(api, __VA_ARGS__)
    #define AFB_ServiceSync(api, ...) afb_dynapi_call_sync(api, __VA_ARGS__)

    #define AFB_RequireApi(api, ...) afb_dynapi_require_api(api, __VA_ARGS__)


    #define AFB_GetEventLoop(api) afb_dynapi_get_event_loop(api)
    #define AFB_RootDirGetFD(api) afb_dynapi_rootdir_get_fd(api)
    #define AFB_ReqVCBData afb_request_get_vcbdata
    #define AFB_ReqGetApi afb_request_get_dynapi

    #define AFB_ClientCtxSet(request, replace, createCB, freeCB, handle) afb_request_context(request, replace, createCB, freeCB, handle)
    #define AFB_ClientCtxClear(request) afb_request_context_clear(request)

    #define AFB_ReqSetLOA(request, level) afb_request_session_set_LOA(request, level)

    #define AFB_NewApi afb_dynapi_new_api

    #define  AFB_ApiAddVerb(a,b,c,d,e,f,g,h) afb_dynapi_add_verb(a,b,c,d,e,f,g)

    #define AFB_ApiSetUserData afb_dynapi_set_userdata
    #define AFB_ApiGetUserData afb_dynapi_get_userdata

    #define AFB_ApiOnEvent afb_dynapi_on_event
    #define AFB_ApiOnInit afb_dynapi_on_init
    #define AFB_ApiSeal afb_dynapi_seal

    typedef struct {
            const char *verb;                       /* name of the verb, NULL only at end of the array */
            void (*callback)(AFB_ReqT req);   /* callback function implementing the verb */
            const struct afb_auth *auth;		/* required authorisation, can be NULL */
            const char *info;			/* some info about the verb, can be NULL */
            uint32_t session;
    } AFB_ApiVerbs;
#elif(AFB_BINDING_VERSION == 3)
    #include <afb/afb-binding.h>

    #define AFB_ReqNone NULL
    typedef afb_req_t AFB_ReqT;
    typedef afb_api_t  AFB_ApiT;

    typedef afb_event_t AFB_EventT;
    #define AFB_EventIsValid(eventid) eventid
    #define AFB_EventPush afb_event_push
    #define AFB_ReqSubscribe  afb_req_subscribe
    #define AFB_EventMake(api, name) afb_api_make_event(api, name)

    #define AFB_ReqJson(request) afb_req_json(request)

    #define AFB_ReqSuccess  afb_req_success
    #define AFB_ReqSuccessF afb_req_success_f
    #define AFB_ReqFail    afb_req_fail
    #define AFB_ReqFailF   afb_req_fail_f

    #define AFB_ReqNotice(request, ...)   AFB_REQ_NOTICE (request, __VA_ARGS__)
    #define AFB_ReqWarning(request, ...)  AFB_REQ_WARNING (request, __VA_ARGS__)
    #define AFB_ReqDebug(request, ...)    AFB_REQ_DEBUG (request, __VA_ARGS__)
    #define AFB_ReqError(request, ...)    AFB_REQ_ERROR (request, __VA_ARGS__)
    #define AFB_ReqInfo(request, ...)     AFB_REQ_INFO (request, __VA_ARGS__)

    #define AFB_ApiVerbose(api, level, ...) afb_api_verbose(api, level, __VA_ARGS__)
    #define AFB_ApiNotice(api, ...)   AFB_API_NOTICE (api, __VA_ARGS__)
    #define AFB_ApiWarning(api, ...)  AFB_API_WARNING (api, __VA_ARGS__)
    #define AFB_ApiDebug(api, ...)    AFB_API_DEBUG (api, __VA_ARGS__)
    #define AFB_ApiError(api, ...)    AFB_API_ERROR (api, __VA_ARGS__)
    #define AFB_ApiInfo(api, ...)     AFB_API_INFO (api, __VA_ARGS__)

    #define AFB_GetApiSettings afb_api_settings

    #define AFB_ReqIsValid(request)   request
    #define AFB_EvtIsValid(evtHandle) evtHandle

/*
 * Binder version < FF.RC4, we miss two defined calls.
 * This is for compatibility purpose that we defined them here.
 */
#if ! defined(afb_service_call_legacy) || ! defined(afb_service_call_sync_legacy)
    #define afb_service_call_legacy afb_api_x3_call_legacy
    #define afb_service_call_sync_legacy afb_api_x3_call_sync_legacy
#endif

    #define AFB_ServiceCall(api, ...) afb_api_call_legacy(api, __VA_ARGS__)
    #define AFB_ServiceSync(api, ...) afb_api_call_sync_legacy(api, __VA_ARGS__)

    #define AFB_ApiCall(api, ...) afb_api_call(api, __VA_ARGS__)
    #define AFB_ApiSync(api, ...) afb_api_call_sync(api, __VA_ARGS__)

    #define AFB_RequireApi(api, ...) afb_api_require_api(api, __VA_ARGS__)

    #define AFB_GetEventLoop(api) afb_api_get_event_loop(api)
    #define AFB_RootDirGetFD(api) afb_api_rootdir_get_fd(api)
    #define AFB_ReqVCBData afb_req_get_vcbdata
    #define AFB_ReqGetApi afb_req_get_api

    #define AFB_ClientCtxSet(request, replace, createCB, freeCB, handle) afb_req_context(request, replace, createCB, freeCB, handle)
    #define AFB_ClientCtxClear(request) afb_req_context_clear(request)

    #define AFB_ReqSetLOA(request, level) afb_req_session_set_LOA(request, level)

    #define AFB_NewApi afb_api_new_api

    #define  AFB_ApiAddVerb afb_api_add_verb

    #define AFB_ApiSetUserData afb_api_set_userdata
    #define AFB_ApiGetUserData afb_api_get_userdata

    #define AFB_ApiOnEvent afb_api_on_event
    #define AFB_ApiOnInit afb_api_on_init
    #define AFB_ApiSeal afb_api_seal

    typedef struct {
            const char *verb;                       /* name of the verb, NULL only at end of the array */
            void (*callback)(AFB_ReqT req);   /* callback function implementing the verb */
            const struct afb_auth *auth;		/* required authorisation, can be NULL */
            const char *info;			/* some info about the verb, can be NULL */
            uint32_t session;
    } AFB_ApiVerbs;

#else
    #include <afb/afb-binding.h>

    typedef struct afb_req AFB_ReqT;
    typedef void* AFB_ApiT;
    #define AFB_ReqNone (struct afb_req){0,0}

    typedef struct afb_event AFB_EventT;
    #define AFB_EventPush afb_event_push
    #define AFB_ReqSubscribe  afb_req_subscribe
    #define AFB_EventIsValid(event) afb_event_is_valid(event)
    #define AFB_EventMake(api, name) afb_daemon_make_event(name)

    #define AFB_ReqJson(request) afb_req_json(request)
    #define AFB_ReqSuccess  afb_req_success
    #define AFB_ReqSuccessF afb_req_success_f
    #define AFB_ReqFail    afb_req_fail
    #define AFB_ReqFailF   afb_req_fail_f

    #define AFB_ReqNotice(request, ...)   AFB_NOTICE (__VA_ARGS__)
    #define AFB_ReqWarning(request, ...)  AFB_WARNING (__VA_ARGS__)
    #define AFB_ReqDebug(request, ...)    AFB_DEBUG (__VA_ARGS__)
    #define AFB_ReqError(request, ...)    AFB_ERROR (__VA_ARGS__)
    #define AFB_ReqInfo(request, ...)     AFB_INFO (__VA_ARGS__)

    #define AFB_ApiVerbose(api, level, ...) afb_daemon_verbose_v2(level,__VA_ARGS__)
    #define AFB_ApiNotice(api, ...)   AFB_NOTICE (__VA_ARGS__)
    #define AFB_ApiWarning(api, ...)  AFB_WARNING (__VA_ARGS__)
    #define AFB_ApiDebug(api, ...)    AFB_DEBUG (__VA_ARGS__)
    #define AFB_ApiError(api, ...)    AFB_ERROR (__VA_ARGS__)
    #define AFB_ApiInfo(api, ...)     AFB_INFO (__VA_ARGS__)

    #define AFB_GetApiSettings(api) json_object_new_object()

    #define AFB_ReqIsValid(request)   afb_req_is_valid(request)
    #define AFB_EvtIsValid(evtHandle) afb_event_is_valid(evtHandle)

    #define AFB_ServiceCall(api, ...) afb_service_call(__VA_ARGS__)
    #define AFB_ServiceSync(api, ...) afb_service_call_sync(__VA_ARGS__)

    #define AFB_RequireApi(api, ...)  afb_daemon_require_api(__VA_ARGS__)

    #define AFB_GetEventLoop(api) afb_daemon_get_event_loop()
    #define AFB_RootDirGetFD(api) afb_daemon_rootdir_get_fd()
    #define AFB_ReqVCBData afb_req_get_vcbdata

    #define AFB_ReqSetLOA(request, level) afb_req_session_set_LOA(request, level)

    static inline void* AFB_ClientCtxSet(afb_req request, int replace, void *(*create_context)(void *closure), void (*free_context)(void*), void *closure)
    {
        void *ctx = create_context(closure);
        if(ctx)
            {afb_req_context_set(request, ctx, free_context);}
        return ctx;
    }

    #define AFB_ClientCtxClear(request) afb_req_context_clear(request)

    #define AFB_ApiVerbs afb_verb_v2
#endif

#endif /* _AFB_DEFINITIONS_INCLUDE_ */

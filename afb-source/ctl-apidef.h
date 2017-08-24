
static const char _afb_description_v2_aaaa[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http:iot.bzh/download/openapi/schem"
    "a-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\"c"
    "ontroler\",\"version\":\"1.0\",\"x-binding-c-generator\":{\"api\":\"aaaa"
    "\",\"version\":2,\"prefix\":\"ctlapi_\",\"postfix\":\"\",\"start\":null,"
    "\"onevent\":\"DispatchOneEvent\",\"init\":\"CtlBindingInit\",\"scope\":\""
    "\",\"private\":false}},\"servers\":[{\"url\":\"ws://{host}:{port}/api/po"
    "lctl\",\"description\":\"Unicens2 API.\",\"variables\":{\"host\":{\"defa"
    "ult\":\"localhost\"},\"port\":{\"default\":\"1234\"}},\"x-afb-events\":["
    "{\"$ref\":\"#/components/schemas/afb-event\"}]}],\"components\":{\"schem"
    "as\":{\"afb-reply\":{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\"a"
    "fb-event\":{\"$ref\":\"#/components/schemas/afb-event-v2\"},\"afb-reply-"
    "v2\":{\"title\":\"Generic response.\",\"type\":\"object\",\"required\":["
    "\"jtype\",\"request\"],\"properties\":{\"jtype\":{\"type\":\"string\",\""
    "const\":\"afb-reply\"},\"request\":{\"type\":\"object\",\"required\":[\""
    "status\"],\"properties\":{\"status\":{\"type\":\"string\"},\"info\":{\"t"
    "ype\":\"string\"},\"token\":{\"type\":\"string\"},\"uuid\":{\"type\":\"s"
    "tring\"},\"reqid\":{\"type\":\"string\"}}},\"response\":{\"type\":\"obje"
    "ct\"}}},\"afb-event-v2\":{\"type\":\"object\",\"required\":[\"jtype\",\""
    "event\"],\"properties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-"
    "event\"},\"event\":{\"type\":\"string\"},\"data\":{\"type\":\"object\"}}"
    "}},\"x-permissions\":{\"control\":{\"permission\":\"urn:AGL:permission:a"
    "udio:public:control\"},\"dispatch\":{\"permission\":\"urn:AGL:permission"
    ":audio:public:dispatch\"},\"request\":{\"permission\":\"urn:AGL:permissi"
    "on:audio:public:request\"},\"debug\":{\"permission\":\"urn:AGL:permissio"
    "n:audio:public:debug\"}},\"responses\":{\"200\":{\"description\":\"A com"
    "plex object array response\",\"content\":{\"application/json\":{\"schema"
    "\":{\"$ref\":\"#/components/schemas/afb-reply\"}}}}}},\"paths\":{\"/ping"
    "\":{\"description\":\"Ping Binding\",\"get\":{\"x-permissions\":{\"$ref\""
    ":\"#/components/x-permissions/control\"},\"parameters\":[{\"in\":\"query"
    "\",\"name\":\"args\",\"required\":false,\"schema\":{\"type\":\"json\"}}]"
    ",\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}},\"/"
    "subscribe\":{\"description\":\"Subcribe Audio Agent Policy Control End\""
    ",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions/contr"
    "ol\"},\"parameters\":[{\"in\":\"query\",\"name\":\"event_patern\",\"requ"
    "ired\":true,\"schema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\""
    "$ref\":\"#/components/responses/200\"}}}},\"/dispatch\":{\"description\""
    ":\"Request Access to Navigation Audio Channel.\",\"get\":{\"x-permission"
    "s\":{\"$ref\":\"#/components/x-permissions/dispatch\"},\"parameters\":[{"
    "\"in\":\"query\",\"name\":\"zone\",\"required\":false,\"schema\":{\"type"
    "\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/components/respons"
    "es/200\"}}}},\"/request\":{\"description\":\"Execute LUA string script.\""
    ",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions/reque"
    "st\"},\"parameters\":[{\"in\":\"query\",\"name\":\"func\",\"required\":t"
    "rue,\"schema\":{\"type\":\"string\"}},{\"in\":\"query\",\"name\":\"args\""
    ",\"required\":false,\"schema\":{\"type\":\"array\"}}],\"responses\":{\"2"
    "00\":{\"$ref\":\"#/components/responses/200\"}}}},\"/debuglua\":{\"descr"
    "iption\":\"Execute LUA string script.\",\"get\":{\"x-permissions\":{\"$r"
    "ef\":\"#/components/x-permissions/debug\"},\"parameters\":[{\"in\":\"que"
    "ry\",\"name\":\"filename\",\"required\":true,\"schema\":{\"type\":\"stri"
    "ng\"}}],\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}"
    "}}}}}"
;

static const struct afb_auth _afb_auths_v2_aaaa[] = {
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:audio:public:control" },
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:audio:public:dispatch" },
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:audio:public:request" },
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:audio:public:debug" }
};

 void ctlapi_ping(struct afb_req req);
 void ctlapi_subscribe(struct afb_req req);
 void ctlapi_dispatch(struct afb_req req);
 void ctlapi_request(struct afb_req req);
 void ctlapi_debuglua(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_aaaa[] = {
    {
        .verb = "ping",
        .callback = ctlapi_ping,
        .auth = &_afb_auths_v2_aaaa[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "subscribe",
        .callback = ctlapi_subscribe,
        .auth = &_afb_auths_v2_aaaa[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "dispatch",
        .callback = ctlapi_dispatch,
        .auth = &_afb_auths_v2_aaaa[1],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "request",
        .callback = ctlapi_request,
        .auth = &_afb_auths_v2_aaaa[2],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "debuglua",
        .callback = ctlapi_debuglua,
        .auth = &_afb_auths_v2_aaaa[3],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    { .verb = NULL }
};

const struct afb_binding_v2 afbBindingV2 = {
    .api = "aaaa",
    .specification = _afb_description_v2_aaaa,
    .info = NULL,
    .verbs = _afb_verbs_v2_aaaa,
    .preinit = NULL,
    .init = CtlBindingInit,
    .onevent = DispatchOneEvent,
    .noconcurrency = 0
};


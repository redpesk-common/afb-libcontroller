# Controller binding configuration

By default the controller searches for a config filename with the same 'middlename' as the daemon process. As an example if your process name is afb-daemon then middle name is 'daemon'. In addition, if your process name is afb-daemon-audio the middle name is also 'daemon'. Moreover the prefix is chosen when you call the [CtlConfigSearch](<#4)_Do_controller_config_parsing_at_binding_pre-init>) function, see below:

```bash
CtlConfigSearch(AFB_ApiT apiHandle, const char *dirList, const char *prefix)
```

```bash
# Middlename is taken from process middlename.
(prefix-)middlename*.json
```

You may overload the config search path with environment variables

* **CONTROL_CONFIG_PATH**: change default search path for configuration. You may provide multiple directories separated by ':'.
* **CONTROL_LUA_PATH**: same as CONTROL_CONFIG_PATH but for Lua script files.

Example: to load a config named '(prefix-)myconfig-test.json' do

```bash
afb-daemon --name myconfig --verbose ...'
```

The configuration is loaded dynamically during startup time. The controller scans **CONTROL_CONFIG_PATH** for a file corresponding to the pattern
"(prefix-)bindermiddlename*.json". The first file found in the path is loaded,
any other file corresponding to the same path is ignored and only generates a warning.

Each block in the configuration file is defined with

* **uid**: mandatory, it is used either for debugging or as input for the action (eg: signal name, control name, ...)
* **info**:  optional, it is used for documentation purpose only

> **Note**: by default the controller config search path is defined at compilation time, but the path might be overloaded with the **CONTROL_CONFIG_PATH**
> environment variable.

## Config is organized in sections

* **metadata**: describes the configuration
* **plugins or resources**: defines the set of functions provided by the plugins allowing to load additional resources (compiled C or lua)
* **onload**: a collection of actions meant to be executed at startup time
* **control**: sets the controls with a collection of actions, in dynamic api it could also specify the verbs of the api
* **event**: a collection of actions meant to be executed when receiving a given signal
* **personal sections**: personal section

Callbacks to parse sections are documented in [Declare your controller config section in your binding](./Usage.html#declare-your-controller-config-section-in-your-binding) section. You can use the callback defined in controller or define your own callback.

## Metadata

As today metadata is only used for documentation purpose.

* **uid**: mandatory
* **version**: mandatory
* **api**: mandatory
* **info**: optional
* **require**: optional
* **author**: optional
* **date**: optional

## OnLoad section

Onload section defines startup time configuration. Onload may provide multiple initialization
profiles, each with a different uid.

You can define the following keys or arrays of the following keys:

* **uid**: mandatory.
* **info**: optional
* **action**: mandatory
* **args**: optional

## Control section

Control section defines a list of controls that are accessible.

You can define the following keys or arrays of the following keys, moreover
this section could be verb api:

* **uid**: mandatory
* **info**: optional
* **action**: the list of actions is mandatory

## Event section

Event section defines a list of actions to be executed on event reception. Event can do
anything a controller can (change state, send back signal, ...)
eg: if a controller subscribes to vehicle speed, then speed-event may adjust
master-volume to speed.

You can define the following keys or arrays of the following keys, moreover you can define an event from an another API with the following syntax "API/event".

* **uid**: mandatory
* **info**: optional
* **action**: the list of actions  is mandatory

## Plugin section

Plugin section defines plugins used with this controller. A plugin is a C/C++ program meant to
execute some tasks after an event or on demand. This easily extends intrinsic
binding logic for ad-hoc needs.

You can define the following keys or arrays of the following keys:

* **uid**: mandatory
* **info**: optional
* **spath**: optional, semicolon separated paths where to find the plugin. This could be a compiled shared library or LUA scripts. Could be specified using CONTROL_PLUGIN_PATH environment variable also.
* **libs**: mandatory, Plugin file or LUA scripts to load
* **lua**: optional, C functions that could be called from a LUA script

## Personal sections

* **uid**: mandatory
* **info**: optional
* **action**: mandatory
* **any keys wanted**: optional

You can define your own sections and add your own callbacks into the
CtlSectionT structure, see
[Declare your controller config section in your binding](<#3_Declare_your_controller_config_section_in_your_binding>) section.

## Actions Categories

Controller supports three categories of actions. Each action returns a status
where 0=success and 1=failure.

* **AppFw API** provides a generic model to request other bindings. Requested bindings can be local (eg: ALSA/UCM) or external (eg: vehicle signaling).
  * `"action": "api://API_NAME#verb_name"`
* C-API, when defined in the onload section, the plugin may provide C native API with `CTLP-CAPI(apiname, uid, args, query, context)`. Plugin may also create Lua command with `CTLP-LUA2C(LuaFuncName, uid, args, query, context)`. Where `args`+`query` are JSON-C object and context is the returned value from `CTLP_ONLOAD` function. Any missing value is set to NULL.
  * `"action": "plugin://plugin_name#function_name"`
* Lua-API, when compiled with Lua option, the controller supports action defined directly in Lua script. During "*onload*" phase, the controller searches in `CONTROL_LUA_PATH` file with pattern "(prefix-)bindermiddlename*.lua". Any file corresponding to this pattern is automatically loaded. Any function defined in those Lua scripts can be called through a controller action. Lua functions receive three parameters (uid, args, query).
  * `"action": "lua://plugin_name#function_name"`

You also can add the **privileges** property that handles AGL permission
needed to be able to call this action.

> **Note**: Lua added functions are systematically prefixed. AGL standard AppFw
functions are prefixed with AGL: (eg: AFB:notice(), AFB:success(), ...).
> User Lua functions added through the plugin and CTLP_LUA2C are prefixed with
the plugin uid or the one you defined in your config (eg: MyPlug:HelloWorld1).

## Available Application Framework Commands

Each Lua AppFw commands should be prefixed by AFB:

* `AFB:notice ("format", arg1,... argn)` directly printed LUA tables as json string with '%s'.
   `AFB:error`, `AFB:warning`, `AFB:info`, `AFB:debug` work on the same model. Printed messages are limited to 512 characters.

* `AFB:service ('API', 'VERB', {query}, "Lua_Callback_Name", {context})` is an asynchronous call to another binding. When empty, query/context should be set to '{}'
   and not to 'nil'. When 'nil', Lua does not send 'NULL' value but removes arguments to calling stack. WARNING:"Callback"
   is the name of the callback as a string and not a pointer to the callback. (If someone as a solution to fix this, please
   let me known). Callback is call as LUA "function Alsa_Get_Hal_CB (error, result, context)" where:
  * error is a Boolean
  * result is the full answer from AppFw (do not forget to extract the response)
  * context is a copy of the Lua table pass as an argument (warning it's a copy not a pointer to original table)

* `error,result=AFB:servsync('API', 'VERB', {query})` is saved as previous but for synchronous call. Note that Lua accepts multiple
   returns. AFB:servsync returns both the error message and the response as a Lua table. Like for AFB:service, the user should not
   forget to extract response from result.

* `AFB:success(request, response)` is the success request. request is the opaque handle passes when Lua is called from (api="control", verb="docall").
   Response is a Lua table that will be returned to the client.

* `AFB:fail(request, response)` is the same as for success. Note that LUA generates automatically the error code from Lua function name.
   The response is transformed into a json string before being returned to the client.

* `EventHandle=AFB:evtmake("MyEventName")` creates an event and returns the handle as an opaque handle. Note that due to a limitation
   of json_object, this opaque handle cannot be passed as an argument in a callback context.

* `AFB:subscribe(request, MyEventHandle)` subscribes a given client to a previously created event.

* `AFB:evtpush (MyEventHandle, MyEventData)` pushes an event to every subscribed client. MyEventData is a Lua table that will be
   sent as a json object to the corresponding clients.

* `timerHandle=AFB:timerset (MyTimer, "Timer_Test_CB", context)` initialises a timer from MyTimer Lua table. This table should contains 3 elements:
   MyTimer={[l"abel"]="MyTimerName", ["delay"]=timeoutInMs, ["count"]=nBOfCycles}. Note that if count==0 then timer is cycled
   infinitely. Context is a standard Lua table. This function returns an opaque handle to be used to further control the timer.

* `AFB:timerclear(timerHandle)` kills an existing timer. Returns an error when timer does not exit.

* `MyTimer=AFB:timerget(timerHandle)` returns uid, delay and count of an active timer. Returns an error when timerHandle does not
   point on an active timer.

* `AFB:GetEventLoop()` retrieves the common systemd's event loop of AFB.

* `AFB:RootDirGetFD()` gets the root directory file descriptor. This file descriptor can be used with functions 'openat', 'fstatat', ...

> **Note**: Except for functions call during binding initialisation period. Lua calls are protected and should returned clean messages
> even when they are improperly used. If you find bug please report.

## Adding Lua command from User Plugin

User Plugin is optional and may provide either native C-action accessible directly from controller actions as defined in
JSON config file, or alternatively may provide a set of Lua commands usable inside any script (onload, control,event). A simple
plugin that provides both notice C API and Lua commands is provided as example (see ctl-plugin-sample.c). Technically a
plugin is a simple sharelibrary and any code fitting in sharelib might be used as a plugin. Developer should nevertheless
not forget that except when no-concurrency flag was at binding construction time, any binding should to be thread safe.

A plugin must be declared with `CTLP_REGISTER("MyCtlSamplePlugin")`. This entry point defines a special structure that is checked
at plugin load time by the controller. Then you have an optional init routine declare with `CTLP_ONLOAD(plugin, handle)`.
 The init routine may create
a plugin context that is later presented to every plugin API, this for both LUA and native C ones. Then each:

* C API declare with `CTLP_CAPI(MyCFunction, source, argsJ, queryJ) {your code}`. Where:
  * **MyFunction** is your function
  * **source** is the structure config
  * **argsJ** a json_object containing the argument attaches to this control in JSON config file
  * **queryJ** a json_object

* Lua API declare with `CTLP_LUA2C(MyLuaCFunction, source, argsJ, responseJ) {your code}`. Where
  * **MyLuaCFunction** is both the name of your C function and Lua command
  * **source** is the structure config
  * **argsJ** the arguments passed this time from Lua script and not from Json config file.
  * **responseJ** if success the argument is passed into the request.

> **Warning**: Lua samples use with controller enforce strict mode. As a result every variable should be declared either as
> local or as global. Unfortunately "luac" is not smart enough to handle strict mode at build time and errors only appear
> at run time. Because of this strict mode every global variables (which include functions) should be prefixed by '_'.
> Note that LUA requires an initialisation value for every variables and declaring something like "local myvar" will not
> allocate "myvar".

## Debugging Facilities

Controller Lua scripts are checked for syntax from CMAKE template with Luac. When needed to go further, a developer API should be allowed to
execute directly Lua commands within the controller context from Rest/Ws (api=control, verb=lua_doscript). DoScript API takes two
other optional arguments func=xxxx where xxxx is the function to execute within Lua script and args, a JSON object to provide
input parameters. When funcname is not given by default, the controller tries to execute middle filename doscript-xxxx-????.lua.

When executed from the controller, Lua script may use any AppFw Apis as well as any L2C user defines commands in plugin.

## Running as Standalone Controller

The controller is a standard binding. It can be started with the following command:

```bash
afb-daemon --name=yourname --port=1234 --workdir=. --roothttp=./htdocs --tracereq=common --token= --verbose --binding=pathtoyourbinding.so --monitoring
```

Afb-Daemon only loads controller bindings without searching for the other
binding. In this case, the controller binding will search for a configuration file
name '(prefix-)bindermiddlename*.json'. This model can be used to implement for testing
purpose or simply to act as the glue between a UI and other binder/services.

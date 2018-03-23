# Controller Utilities

* Object: Generic Controller Utilities to handle Policy, Small Business Logic, Glue in between components, ...
* Status: Release Candidate
* Author: Fulup Ar Foll fulup@iot.bzh
* Date  : October-2017

## Usage

1) Add ctl-utilities as a submodule to include in your project

```bash
git submodule add https://gerrit.automotivelinux.org/gerrit/apps/app-controller-submodule
```

2) Add ctl-utilities as a static library to your binding

```cmake
    # Library dependencies (include updates automatically)
    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        ctl-utilities
        ... other dependencies ....
    )
```

3) Declare your controller config section in your binding

```C
// CtlSectionT syntax:
// key: "section name in config file"
// loadCB: callback to process section
// handle: a void* pass to callback when processing section
static CtlSectionT ctlSections[]= {
    {.key="plugins" , .loadCB= PluginConfig, .handle= &halCallbacks},
    {.key="onload"  , .loadCB= OnloadConfig},
    {.key="halmap"  , .loadCB= MapConfigLoad},
    {.key=NULL}
};

```

3) Do controller config parsing at binding pre-init

```C
   // check if config file exist
    const char *dirList= getenv("CTL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;

    ctlConfig = CtlConfigLoad(dirList, ctlSections);
    if (!ctlConfig) goto OnErrorExit;
```

4) Exec controller config during binding init

```C
  int err = CtlConfigExec (ctlConfig);
```

For sample usage look at https://gerrit.automotivelinux.org/gerrit/gitweb?p=apps/app-controller-submodule.git

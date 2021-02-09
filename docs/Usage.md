# Usage

## Add afb-libcontroller as a static library to your binding

In your `config.cmake` file, add a dependency to the controller library, i.e:

```cmake
set(PKG_REQUIRED_LIST
	json-c
	afb-daemon
	afb-libcontroller --> this is the controller library dependency name.
)
```

Or you can also use the [FIND_PACKAGE](https://cmake.org/cmake/help/v3.6/command/find_package.html?highlight=find_package)
CMake command to add it.

## Declare your controller config section in your binding

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

The section count is unlimited. When declaring sections, the
rules below must be followed:

- the end of the array is marked with a section having a NULL key.
- if more than one section have the same key, this is an error silently
  ignored but only the first section of that name is used.

## Do the controller config parsing at binding pre-init

```C
   // check if config file exist
    const char *dirList= getenv("CTL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;

    const char *configPath = CtlConfigSearch(apiHandle, dirList, "prefix");
    if(!confiPath) return -1;

    ctlConfig = CtlLoadMetaData(apiHandle, configPath);
    if (!ctlConfig) return -1;

    sts = CtlLoadSections(apiHandle, ctlConfig, ctlSections);
    if (sts < 0) return -1;
```

## Execute the controller config during binding init

```C
  int err = CtlConfigExec (ctlConfig);
```


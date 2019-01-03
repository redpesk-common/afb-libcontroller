# Usage

## (Optional) Remove the git submodule version

If you already use the controller component but use the submodule version then
you have to get rid of it to be sure to link and use the library version. To do
so, you have to do the following:

* deinitialize the submodule using `git`

```bash
# This example assumes that the git submodule is named app-afb-helpers-submodule
# and is located at your root project repository.
git submodule deinit app-afb-helpers-submodule
```

* remove the relative submodule lines from the `.gitmodules` file

```bash
vim .gitmodules
```

* remove the `ctl-utilities` target link from any CMake target you specified.
 Those lines look like:

```bash
TARGET_LINK_LIBRARIES(${TARGET_NAME}
    ctl-utilities # REMOVE THIS LINE
    ${link_libraries}
    )
```

## Add libappcontroller as a static library to your binding

In your `config.cmake` file, add a dependency to the controller library, i.e:

```cmake
set(PKG_REQUIRED_LIST
	json-c
	afb-daemon
	ctl-utilities --> this is the controller library dependency name
)
```

Or you can also use the `FIND_PACKAGE` CMake command to add it.

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

## Do the controller config parsing at binding pre-init

```C
   // check if config file exist
    const char *dirList= getenv("CTL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;

    const char *configPath = CtlConfigSearch(apiHandle, dirList, "prefix");
    if(!confiPath) return -1;

    ctlConfig = CtlConfigLoad(dirList, ctlSections);
    if (!ctlConfig) return -1;
```

## Execute the controller config during binding init

```C
  int err = CtlConfigExec (ctlConfig);
```

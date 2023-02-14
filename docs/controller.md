# Controller

* Object: Generic Controller to handle Policy,Small Business Logic, Glue in between components, ...
* Status: Release Candidate
* Author: Fulup Ar Foll fulup@iot.bzh
* Date  : Dec-2020
* Require : afb-binder version >= 4.0.0 (handle only bindings at least v3)

## Features

* Create a controller application from a JSON config file
* Each control (eg: navigation, multimedia, ...) is a suite of actions. When all actions succeed
 control is granted, if one fails control access is denied.
* Actions can either be:
  * Invocation of an other binding API, either internal or external (eg: a policy service, Alsa UCM, ...)
  * C routines from a user provided plugin (eg: policy routine, proprietary code, ...)
  * Lua script function. Lua provides access to every AGL appfw functionality and can be extended by
   plugins written in C.

## Installation

* Controller can easily be included as a separate library in any AGL service or application binder.
* Dependencies: the only dependencies are [Application Framework]({% chapter_link afb_binder.overview %})
 and [afb-libhelpers](https://github.com/redpesk-common/afb-libhelpers).
* Controller relies on Lua-5.3, when not needed Lua might be removed at compilation time.

The controller library is integrated by default in the redpesk SDK and is also
available as a package for the redpesk supported linux distributions.

You could find the SDK build which embed the afb-libhelpers library in our
packages repositories for the major Linux distribution OS:

* **Ubuntu [20.04](https://download.redpesk.bzh/redpesk-lts/{{ site.redpesk-os.latest }}/sdk/Ubuntu_20.04/), [22.04](https://download.redpesk.bzh/redpesk-lts/{{ site.redpesk-os.latest }}/sdk/Ubuntu_22.04/)**
* **Fedora [36](https://download.redpesk.bzh/redpesk-lts/{{ site.redpesk-os.latest }}/sdk/Fedora_36/), [37](https://download.redpesk.bzh/redpesk-lts/{{ site.redpesk-os.latest }}/sdk/Fedora_37/)**
* **openSUSE Leap [15.3](https://download.redpesk.bzh/redpesk-lts/{{ site.redpesk-os.latest }}/sdk/openSUSE_Leap_15.3/), [15.4](https://download.redpesk.bzh/redpesk-lts/{{ site.redpesk-os.latest }}/sdk/openSUSE_Leap_15.4/)**

To install the native package please refer to [this chapter]({% chapter_link host-configuration-doc.setup-your-build-host %})
in the redpesk documentation to install the redpesk native repositories for your distribution.

Then use your package manager to install the library.

### OpenSuse

```bash
sudo zypper ref
sudo zypper install afb-libcontroller-devel
```

### Fedora

```bash
sudo dnf ref
sudo dnf install afb-libcontroller-devel
```

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install afb-libcontroller-dev
```

## Monitoring

* The default test HTML page expect the monitoring HTML page to be accessible under /monitoring with
 the --monitoring option.
* The monitoring HTML pages are installed with the app framework binder in a subdirectory called
 monitoring.
* You can add other HTML pages with the alias options e.g:
 afb-binder --port=1234 --monitoring --alias=/path1/to/htmlpages:/path2/to/htmlpages --ldpaths=. --workdir=. --roothttp=../htdocs
* The monitoring is accessible at http://localhost:1234/monitoring.

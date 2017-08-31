Format: 1.0
Source: agl-afb-controller
Binary: agl-afb-controller-bin
Architecture: any
Version: 2.0-0
Maintainer: Fulup Ar Foll <fulup@iot.bzh>
Standards-Version: 3.8.2
Homepage: https://github.com/iotbzh/controller-binding
Build-Depends: debhelper (>= 5),
 pkg-config,
 cmake,
 gcc,
 g++,
 libjson-c-dev ,
 libsystemd-dev  (>= 222),
 agl-app-framework-binder-dev ,
 agl-libmicrohttpd-dev  (>= 0.9.55),
 agl-app-framework-binder-dev ,
 liblua5.3-dev  (>= 5.3),
 lua5.3
Debtransform-Tar: agl-afb-controller_1.0.orig.tar.gz

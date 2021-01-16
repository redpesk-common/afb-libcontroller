###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
#
# author: Fulup Ar Foll <fulup@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################
set(TARGET_NAME "afb-libcontroller")
set(AGLVERSION 6.99 CACHE STRING "Current AGL Version")
set(PROJECT_VERSION ${AGLVERSION} CACHE STRING "Project version can override AGLVERSION")
set(PROJECT_PRETTY_NAME "Controller")
set(PROJECT_DESCRIPTION "controller")
set(PROJECT_URL "http://git.ovh.iot/redpesk/redpesk-common/afb-libcontroller.git")
set(PROJECT_AUTHOR "Ar Foll, Fulup")
set(PROJECT_AUTHOR_MAIL "fulup@iot.bzh")
set(PROJECT_LICENSE "APL2.0")
set(PROJECT_LANGUAGES "C")

if(DEFINED ENV{SDKTARGETSYSROOT})
file(STRINGS $ENV{SDKTARGETSYSROOT}/usr/include/linux/version.h LINUX_VERSION_CODE_LINE REGEX "LINUX_VERSION_CODE")
set(BUILD_ENV_SYSROOT $ENV{SDKTARGETSYSROOT})
elseif(DEFINED ENV{PKG_CONFIG_SYSROOT_DIR})
file(STRINGS $ENV{PKG_CONFIG_SYSROOT_DIR}/usr/include/linux/version.h LINUX_VERSION_CODE_LINE REGEX "LINUX_VERSION_CODE")
set(BUILD_ENV_SYSROOT $ENV{PKG_CONFIG_SYSROOT_DIR})
else()
file(STRINGS /usr/include/linux/version.h LINUX_VERSION_CODE_LINE REGEX "LINUX_VERSION_CODE")
set(BUILD_ENV_SYSROOT "")
endif()

# Get the os type
# Used to package .deb
set(OS_RELEASE_PATH "${BUILD_ENV_SYSROOT}/etc/os-release")
if(EXISTS ${OS_RELEASE_PATH})
	execute_process(COMMAND bash "-c" "grep -E '^ID(_LIKE)?=' ${OS_RELEASE_PATH} | tail -n 1"
		OUTPUT_VARIABLE TMP_OSRELEASE
	)

	if (NOT TMP_OSRELEASE STREQUAL "")
		string(REGEX REPLACE ".*=\"?([0-9a-z\._-]*)( .*)?\"?\n" "\\1" OSDETECTED ${TMP_OSRELEASE})
	else()
		set(OSDETECTED "NOT COMPATIBLE !")
	endif()
elseif("${BUILD_ENV_SYSROOT}" STREQUAL "$ENV{PKG_CONFIG_SYSROOT_DIR}")
	set(OSDETECTED "yocto-build")
else()
	set(OSDETECTED "NOT COMPATIBLE ! Missing ${OS_RELEASE_PATH} file.")
endif()
message("Distribution detected ${OSDETECTED}")

if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()
add_definitions(-DAFB_BINDING_VERSION=3)
add_definitions(-DLUA_GLOB_PATTERN="/var/?.lua\\\;")
add_definitions(-DSUSE_LUA_INCDIR)

INCLUDE(FindPkgConfig)

set (PKG_REQUIRED_LIST
        afb-libhelpers
    )

set(LUA_PKGCONFIG lua)
if(${OSDETECTED} STREQUAL "debian")
    set(LUA_PKGCONFIG lua53)
endif()
list(APPEND PKG_REQUIRED_LIST ${LUA_PKGCONFIG})

# Loop on required package and add options
foreach (PKG_CONFIG ${PKG_REQUIRED_LIST})
    string(REGEX REPLACE "[<>]?=.*$" "" XPREFIX ${PKG_CONFIG})
    PKG_CHECK_MODULES(${XPREFIX} REQUIRED ${PKG_CONFIG})

    INCLUDE_DIRECTORIES(${${XPREFIX}_INCLUDE_DIRS})
    list(APPEND link_libraries ${${XPREFIX}_LDFLAGS})
    add_compile_options (${${XPREFIX}_CFLAGS})
endforeach(PKG_CONFIG)

# add dependency to afb-binding or afb-daemon
pkg_check_modules(AFB afb-binding)
if(NOT ${AFB_FOUND})
    pkg_check_modules(AFB REQUIRED afb-daemon)
endif()
INCLUDE_DIRECTORIES(${AFB_INCLUDE_DIRS})
list(APPEND link_libraries ${AFB_LDFLAGS})
add_compile_options (${AFB_CFLAGS})


set(CTL_LUA_SOURCE ctl-lua.c ctl-lua-utils.c)
ADD_COMPILE_OPTIONS(-g -DCONTROL_SUPPORT_LUA -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-sign-compare -Wno-sign-conversion -Werror=maybe-uninitialized -Werror=implicit-function-declaration -ffunction-sections -fdata-sections -fPIC)

set(CONTROLLER_HEADERS
    ctl-config.h
    ctl-lua.h
    ctl-plugin.h
    )

# Define project Targets
ADD_LIBRARY(${TARGET_NAME} STATIC ctl-action.c ctl-config.c ctl-onload.c ctl-plugin.c ctl-control.c ctl-event.c ${CTL_LUA_SOURCE})

# Library dependencies (include updates automatically)
TARGET_LINK_LIBRARIES(${TARGET_NAME}
    afb-libhelpers
    ${link_libraries}
    )

# Define target includes for this target client
TARGET_INCLUDE_DIRECTORIES(${TARGET_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    )

CONFIGURE_FILE(${TARGET_NAME}.pc.in ${TARGET_NAME}.pc @ONLY)
INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.pc
    DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )

INSTALL(TARGETS ${TARGET_NAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
       )
INSTALL(FILES ${CONTROLLER_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
       )

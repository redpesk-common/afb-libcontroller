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
set(TARGET_NAME "ctl-utilities")
set(PROJECT_PRETTY_NAME "Controller")
set(PROJECT_DESCRIPTION "controller")
set(PROJECT_URL "https://gerrit.automotivelinux.org:29418/apps/app-controller-submodule")
set(PROJECT_AUTHOR "Ar Foll, Fulup")
set(PROJECT_AUTHOR_MAIL "fulup@iot.bzh")
set(PROJECT_LICENSE "APL2.0")
set(PROJECT_LANGUAGES "C")

if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()
add_definitions(-DAFB_BINDING_VERSION=3)
add_definitions(-DLUA_GLOB_PATTERN="/var/?.lua\\\;")
add_definitions(-DSUSE_LUA_INCDIR)


INCLUDE(FindPkgConfig)
set (PKG_REQUIRED_LIST
    afb-daemon
    afb-helpers
    lua
    )

# Loop on required package and add options
foreach (PKG_CONFIG ${PKG_REQUIRED_LIST})
    string(REGEX REPLACE "[<>]?=.*$" "" XPREFIX ${PKG_CONFIG})
    PKG_CHECK_MODULES(${XPREFIX} REQUIRED ${PKG_CONFIG})

    INCLUDE_DIRECTORIES(${${XPREFIX}_INCLUDE_DIRS})
    list(APPEND link_libraries ${${XPREFIX}_LDFLAGS})
    add_compile_options (${${XPREFIX}_CFLAGS})
endforeach(PKG_CONFIG)


set(CTL_LUA_SOURCE ctl-lua.c ctl-lua-utils.c)
ADD_COMPILE_OPTIONS(-DCONTROL_SUPPORT_LUA -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-sign-compare -Wno-sign-conversion -Werror=maybe-uninitialized -Werror=implicit-function-declaration -ffunction-sections -fdata-sections -fPIC)

set(CONTROLLER_HEADERS
    ctl-config.h
    ctl-lua.h
    ctl-plugin.h
    )

# Define project Targets
ADD_LIBRARY(${TARGET_NAME} STATIC ctl-action.c ctl-config.c ctl-onload.c ctl-plugin.c ctl-control.c ctl-event.c ${CTL_LUA_SOURCE})

# Library dependencies (include updates automatically)
TARGET_LINK_LIBRARIES(${TARGET_NAME}
    afb-helpers
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

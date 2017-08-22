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

# Include LUA only when requested
if(CONTROL_SUPPORT_LUA)
   message(STATUS "Notice: LUA Controler Support Selected")
   set(CTL_LUA_SOURCE ctl-lua.c)
   ADD_COMPILE_OPTIONS(-DCONTROL_SUPPORT_LUA)
   ADD_COMPILE_OPTIONS(-DCONTROL_LUA_EVENT="luaevt")
   ADD_COMPILE_OPTIONS(-DCONTROL_LUA_PATH="${CMAKE_SOURCE_DIR}/conf.d/project/lua.d:${CMAKE_INSTALL_PREFIX}/controller-plugins/ctl-lua.d")
else(CONTROL_SUPPORT_LUA)
   message(STATUS "Warning: LUA Without Support ")
endif(CONTROL_SUPPORT_LUA)


PROJECT_TARGET_ADD(audio-plugin-sample)

    # Define targets
    ADD_LIBRARY(${TARGET_NAME} MODULE ctl-plugin-sample.c)

    # Alsa Plugin properties
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	PREFIX ${CTL_PLUGIN_PRE}
        SUFFIX ${CTL_PLUGIN_EXT}
        OUTPUT_NAME ${TARGET_NAME}
    )

    # Library dependencies (include updates automatically)
    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        afb-utilities
        ${link_libraries}
    )

    include_directories("../afb-source")




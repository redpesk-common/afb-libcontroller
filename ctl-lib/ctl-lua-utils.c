/*
 * Copyright (C) 2016 \"IoT.bzh\"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
  Licensed under the Apache License, Version 2.0 (the \"License\");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an \"AS IS\" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ref:
 *  (manual) https://www.lua.org/manual/5.3/manual.html
 *  (lua->C) http://www.troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm#_Anatomy_of_a_Lua_Call
 *  (lua/C Var) http://acamara.es/blog/2012/08/passing-variables-from-lua-5-2-to-c-and-vice-versa/
 *  (Lua/C Lib)https://john.nachtimwald.com/2014/07/12/wrapping-a-c-library-in-lua/
 *  (Lua/C Table) https://gist.github.com/SONIC3D/10388137
 *  (Lua/C Nested table) https://stackoverflow.com/questions/45699144/lua-nested-table-from-lua-to-c
 *  (Lua/C Wrapper) https://stackoverflow.com/questions/45699950/lua-passing-handle-to-function-created-with-newlib
 *
 */

const char *lua_utils = "--===================================================\n\
--=  Niklas Frykholm\n\
-- basically if user tries to create global variable\n\
-- the system will not let them!!\n\
-- call GLOBAL_lock(_G)\n\
-- \n\
--===================================================\n\
function GLOBAL_lock(t)\n\
  local mt = getmetatable(t) or {}\n\
  mt.__newindex = lock_new_index\n\
  setmetatable(t, mt)\n\
end\n\
\n\
--===================================================\n\
-- call GLOBAL_unlock(_G)\n\
-- to change things back to normal.\n\
--===================================================\n\
function GLOBAL_unlock(t)\n\
  local mt = getmetatable(t) or {}\n\
  mt.__newindex = unlock_new_index\n\
  setmetatable(t, mt)\n\
end\n\
\n\
function lock_new_index(t, k, v)\n\
  if (string.sub(k,1,1) ~= \"_\") then\n\
    GLOBAL_unlock(_G)\n\
    error(\"GLOBALS are locked -- \" .. k ..\n\
          \" must be declared local or prefix with '_' for globals.\", 2)\n\
  else\n\
    rawset(t, k, v)\n\
  end\n\
end\n\
\n\
function unlock_new_index(t, k, v)\n\
  rawset(t, k, v)\n\
end\n\
\n\
-- return serialised version of printable table\n\
function Dump_Table(o)\n\
   if type(o) == 'table' then\n\
      local s = '{ '\n\
      for k,v in pairs(o) do\n\
         if type(k) ~= 'number' then k = '\"'..k..'\"' end\n\
         s = s .. '['..k..'] = ' .. Dump_Table(v) .. ','\
      end\n\
      return s .. '} '\n\
   else\n\
      return tostring(o)\n\
   end\n\
end\n\
\n\
-- simulate C prinf function\n\
printf = function(s,...)\n\
    io.write(s:format(...))\n\
    io.write(\"\")\n\
    return\n\
end\n\
\n\
-- lock global variable\n\
GLOBAL_lock(_G)\n\
";

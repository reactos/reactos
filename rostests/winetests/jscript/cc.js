/*
 * Copyright 2010 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

eval("@_jscript_version");

var tmp;

/*@ */
//@cc_on @*/

@_jscript_version;

@cc_on
@*/

// Standard predefined variabled
if(isWin64) {
    ok(@_win64 === true, "@_win64 = " + @_win64);
    ok(@_amd64 === true, "@_amd64 = " + @_amd64);
    ok(isNaN(@_win32), "@_win32 = " + @_win32);
    ok(isNaN(@_x86), "@_x86 = " + @_x86);
}else {
    ok(@_win32 === true, "@_win32 = " + @_win32);
    ok(@_x86 === true, "@_x86 = " + @_x86);
    ok(isNaN(@_win64), "@_win64 = " + @_win64);
    ok(isNaN(@_amd64), "@_amd64 = " + @_amd64);
}

ok(@_jscript === true, "@_jscript = " + @_jscript);
ok(@_jscript_build === ScriptEngineBuildVersion(),
   "@_jscript_build = " + @_jscript_build + " expected " + ScriptEngineBuildVersion());
tmp = ScriptEngineMajorVersion() + ScriptEngineMinorVersion()/10;
ok(@_jscript_version === tmp, "@_jscript_version = " + @_jscript_version + " expected " + tmp);
ok(isNaN(@_win16), "@_win16 = " + @_win16);
ok(isNaN(@_mac), "@_mac = " + @_mac);
ok(isNaN(@_alpha), "@_alpha = " + @_alpha);
ok(isNaN(@_mc680x0), "@_mc680x0 = " + @_mc680x0);
ok(isNaN(@_PowerPC), "@_PowerPC = " + @_PowerPC);

// Undefined variable
ok(isNaN(@xxx), "@xxx = " + @xxx);
ok(isNaN(@x$_xx), "@x$_xx = " + @x$_xx);

tmp = false;
try {
    eval("/*@cc_on */");
}catch(e) {
    tmp = true;
}
ok(tmp, "expected syntax exception");

tmp = false;
try {
    eval("/*@_jscript_version */");
}catch(e) {
    tmp = true;
}
ok(tmp, "expected syntax exception");

reportSuccess();

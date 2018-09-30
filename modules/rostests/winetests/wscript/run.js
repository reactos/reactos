/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

var winetest = new ActiveXObject("Wine.Test");

function ok(expr, msg) {
    winetest.ok(expr, msg);
}

ok(typeof(WScript) === "object", "typeof(WScript) = " + typeof(WScript));
ok(typeof(WSH) === "object", "typeof(WSH) = " + typeof(WSH));
ok(WScript === WSH, "WScript !== WSH");
ok(WScript.Name === "Windows Script Host", "WScript.Name = " + WScript.Name);
ok(typeof(WScript.Version) === "string", "typeof(WScript.Version) = " + typeof(WScript.Version));
ok(typeof(WScript.BuildVersion) === "number", "typeof(WScript.BuildVersion) = " + typeof(WScript.BuildVersion));
ok(WScript.FullName.toUpperCase() === winetest.wscriptFullName.toUpperCase(), "WScript.FullName = " + WScript.FullName);
ok(WScript.Path.toUpperCase() === winetest.wscriptPath.toUpperCase(), "WScript.Path = " + WScript.Path);
ok(WScript.ScriptName === winetest.wscriptScriptName, "WScript.ScriptName = " + WScript.ScriptName);
ok(WScript.ScriptFullName === winetest.wscriptScriptFullName, "WScript.ScriptFullName = " + WScript.ScriptFullName);
ok(typeof(WScript.Arguments) === "object", "typeof(WScript.Arguments) = " + typeof(WScript.Arguments));
ok(WScript.Arguments.Item(0) === "arg1", "WScript.Arguments.Item(0) = " + WScript.Arguments.Item(0));
ok(WScript.Arguments.Item(1) === "2", "WScript.Arguments.Item(1) = " + WScript.Arguments.Item(1));
ok(WScript.Arguments.Item(2) === "ar3", "WScript.Arguments.Item(2) = " + WScript.Arguments.Item(2));
try {
    WScript.Arguments.Item(3);
    ok(false, "expected exception");
}catch(e) {}
ok(WScript.Arguments.Count() === 3, "WScript.Arguments.Count() = " + WScript.Arguments.Count());
ok(WScript.Arguments.length === 3, "WScript.Arguments.length = " + WScript.Arguments.length);
ok(WScript.Interactive === true, "WScript.Interactive = " + WScript.Interactive);
WScript.Interactive = false;
ok(WScript.Interactive === false, "WScript.Interactive = " + WScript.Interactive);
WScript.Interactive = true;
ok(WScript.Interactive === true, "WScript.Interactive = " + WScript.Interactive);
ok(WScript.Application === WScript, "WScript.Application = " + WScript.Application);

var obj = WScript.CreateObject("Wine.Test");
obj.ok(true, "Broken WScript.CreateObject object?");

try {
    obj = WScript.CreateObject("nonexistent");
    ok(false, "Expected exception for CreateObject('nonexistent')");
}catch(e) {}

winetest.reportSuccess();

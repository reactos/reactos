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

ok(isNaN(@test), "@test = " + @test);

@set @test = 1
ok(@test === 1, "@test = " + @test);

@set @test = 0
ok(@test === 0, "@test = " + @test);

tmp = false
@set @test = @test tmp=true
ok(@test === 0, "@test = " + @test);
ok(tmp, "expr after @set not evaluated");

@set @test = !@test
ok(@test === true, "@test = " + @test);

@set @test = (@test+1+true)
ok(@test === 3, "@test = " + @test);

@set
 @test
 =
 2
ok(@test === 2, "@test = " + @test);

@set
 @test
 =
 (
 2
 +
 2
 )
ok(@test === 4, "@test = " + @test);

@set @test = 2.5
ok(@test === 2.5, "@test = " + @test);

@set @test = 0x4
ok(@test === 4, "@test = " + @test);

@set @test = (2 + 2/2)
ok(@test === 3, "@test = " + @test);

@set @test = (false+false)
ok(@test === 0, "@test = " + @test);

@set @test = ((1+1)*((3)+1))
ok(@test === 8, "@test = " + @test);

@set @_test = true
ok(@_test === true, "@_test = " + @_test);

@set @$test = true
ok(@$test === true, "@$test = " + @$test);

@set @newtest = (@newtest != @newtest)
ok(@newtest === true, "@newtest = " + @newtest);

@set @test = (false != 0)
ok(@test === false, "@test = " + @test);

@set @test = (1 != true)
ok(@test === false, "@test = " + @test);

@set @test = (0 != true)
ok(@test === true, "@test = " + @test);

@set @test = (true-2)
ok(@test === -1, "@test = " + @test);

@set @test = (true-@_jscript)
ok(@test === 0, "@test = " + @test);

@set @test = (true==1)
ok(@test === true, "@test = " + @test);

@set @test = (1==false+1)
ok(@test === true, "@test = " + @test);

function expect(val, exval) {
    ok(val === exval, "got " + val + " expected " + exval);
}

@set @test = (false < 0.5)
expect(@test, true);

@set @test = (true == 0 < 0.5)
expect(@test, true);

@set @test = (false < 0)
expect(@test, false);

@set @test = (false > 0.5)
expect(@test, false);

@set @test = (1 < true)
expect(@test, false);

@set @test = (1 <= true)
expect(@test, true);

@set @test = (1 >= true)
expect(@test, true);

@set @test = (1 >= true-1)
expect(@test, true);

@set @test = (true && true)
expect(@test, true);

@set @test = (false && true)
expect(@test, false);

@set @test = (true && false)
expect(@test, false);

@set @test = (false && false)
expect(@test, false);

if(!isWin64) {
@set @test = (@_win32&&@_jscript_version>=5)
expect(@test, true);
}

@if (false)
    this wouldn not parse
"@end

@if (false) "@end

tmp = "@if (false) @end";
ok(tmp.length === 16, "tmp.length = " + tmp.length);

@if(true)
tmp = true
@end
ok(tmp === true, "tmp = " + tmp);

@if(false)
@if this would not CC parse
this will not parse
@elif(true)
this will also not parse
@else
this also will not parse
@if let me complicate things a bit
@end enough
@end
@end

@if(false)
this will not parse
@else
tmp = 2
@else
this will not be parsed
@else
also this
@end
ok(tmp === 2, "tmp = " + tmp);

@if(true)
tmp = 3;
@else
just skip this
@end
ok(tmp === 3, "tmp = " + tmp);

@if(true)
tmp = 4;
@elif(true)
this will not parse
@elif nor this
@else
just skip this
@end
ok(tmp === 4, "tmp = " + tmp);

@if(false)
this will not parse
@elif(false)
nor this would
@elif(true)
tmp = 5;
@elif nor this
@else
just skip this
@end
ok(tmp === 5, "tmp = " + tmp);

@if (!@_jscript)
this would not parse
@if(true)
@else
@if(false)
@end
@end
@elif (@_jscript)
tmp = 6;
@elif (true)
@if xxx
@else
@if @elif @elif @else @end
@end
@else
this would not parse
@end
ok(tmp === 6, "tmp = " + tmp);

@if(true)
@if(false)
@else
tmp = 7;
@end
@else
this would not parse
@end
ok(tmp === 7, "tmp = " + tmp);

var exception_map = {
    JS_E_SYNTAX:               {type: "SyntaxError", number: -2146827286},
    JS_E_MISSING_LBRACKET:     {type: "SyntaxError", number: -2146827283},
    JS_E_EXPECTED_IDENTIFIER:  {type: "SyntaxError", number: -2146827278},
    JS_E_EXPECTED_ASSIGN:      {type: "SyntaxError", number: -2146827277},
    JS_E_EXPECTED_CCEND:       {type: "SyntaxError", number: -2146827259},
    JS_E_EXPECTED_AT:          {type: "SyntaxError", number: -2146827256}
};

function testException(src, id) {
    var ex = exception_map[id];
    var ret = "", num = "";

    try {
        eval(src);
    } catch(e) {
        ret = e.name;
        num = e.number;
    }

    ok(ret === ex.type, "Exception test, ret = " + ret + ", expected " + ex.type +". Executed code: " + src);
    ok(num === ex.number, "Exception test, num = " + num + ", expected " + ex.number + ". Executed function: " + src);
}

testException("@set test=true", "JS_E_EXPECTED_AT");
testException("@set @1=true", "JS_E_EXPECTED_IDENTIFIER");
testException("@set @test x=true", "JS_E_EXPECTED_ASSIGN");
testException("@if false\n@end", "JS_E_MISSING_LBRACKET");
testException("@if (false)\n", "JS_E_EXPECTED_CCEND");
testException("@end\n", "JS_E_SYNTAX");
testException("@elif\n", "JS_E_SYNTAX");
testException("@else\n", "JS_E_SYNTAX");
testException("@if false\n@elif true\n@end", "JS_E_MISSING_LBRACKET");

reportSuccess();

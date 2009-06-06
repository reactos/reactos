/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

var tmp;

ok(true, "true is not true?");
ok(!false, "!false is not true");
ok(!undefined, "!undefined is not true");
ok(!null, "!null is not true");
ok(!0, "!0 is not true");
ok(!0.0, "!0.0 is not true");

ok(1 === 1, "1 === 1 is false");
ok(!(1 === 2), "!(1 === 2) is false");
ok(1.0 === 1, "1.0 === 1 is false");
ok("abc" === "abc", "\"abc\" === \"abc\" is false");
ok(true === true, "true === true is false");
ok(null === null, "null === null is false");
ok(undefined === undefined, "undefined === undefined is false");
ok(!(undefined === null), "!(undefined === null) is false");
ok(1E0 === 1, "1E0 === 1 is false");
ok(1000000*1000000 === 1000000000000, "1000000*1000000 === 1000000000000 is false");
ok(8.64e15 === 8640000000000000, "8.64e15 !== 8640000000000000"+8.64e15);
ok(1e2147483648 === Infinity, "1e2147483648 !== Infinity");

ok(1 !== 2, "1 !== 2 is false");
ok(null !== undefined, "null !== undefined is false");

ok(1 == 1, "1 == 1 is false");
ok(!(1 == 2), "!(1 == 2) is false");
ok(1.0 == 1, "1.0 == 1 is false");
ok("abc" == "abc", "\"abc\" == \"abc\" is false");
ok(true == true, "true == true is false");
ok(null == null, "null == null is false");
ok(undefined == undefined, "undefined == undefined is false");
ok(undefined == null, "undefined == null is false");
ok(true == 1, "true == 1 is false");
ok(!(true == 2), "true == 2");
ok(0 == false, "0 == false is false");

ok(1 != 2, "1 != 2 is false");
ok(false != 1, "false != 1 is false");

var trueVar = true;
ok(trueVar, "trueVar is not true");

ok(ScriptEngine.length === 0, "ScriptEngine.length is not 0");

function testFunc1(x, y) {
    ok(this !== undefined, "this is undefined");
    ok(x === true, "x is not 1");
    ok(y === "test", "y is not \"test\"");
    ok(arguments.length === 2, "arguments.length is not 2");
    ok(arguments["0"] === true, "arguments[0] is not true");
    ok(arguments["1"] === "test", "arguments[1] is not \"test\"");

    return true;
}

ok(testFunc1.length === 2, "testFunc1.length is not 2");

ok(Object.prototype !== undefined, "Object.prototype is undefined");
ok(Object.prototype.prototype === undefined, "Object.prototype is not undefined");
ok(String.prototype !== undefined, "String.prototype is undefined");
ok(Array.prototype !== undefined, "Array.prototype is undefined");
ok(Boolean.prototype !== undefined, "Boolean.prototype is undefined");
ok(Number.prototype !== undefined, "Number.prototype is undefined");
ok(RegExp.prototype !== undefined, "RegExp.prototype is undefined");
ok(Math !== undefined, "Math is undefined");
ok(Math.prototype === undefined, "Math.prototype is not undefined");
ok(Function.prototype !== undefined, "Function.prototype is undefined");
ok(Function.prototype.prototype === undefined, "Function.prototype is not undefined");
ok(Date.prototype !== undefined, "Date.prototype is undefined");
ok(Date.prototype.prototype === undefined, "Date.prototype is not undefined");

Function.prototype.test = true;
ok(testFunc1.test === true, "testFunc1.test !== true");
ok(Function.test === true, "Function.test !== true");

ok(typeof(0) === "number", "typeof(0) is not number");
ok(typeof(1.5) === "number", "typeof(1.5) is not number");
ok(typeof("abc") === "string", "typeof(\"abc\") is not string");
ok(typeof("") === "string", "typeof(\"\") is not string");
ok(typeof(true) === "boolean", "typeof(true) is not boolean");
ok(typeof(null) === "object", "typeof(null) is not object");
ok(typeof(undefined) === "undefined", "typeof(undefined) is not undefined");
ok(typeof(Math) === "object", "typeof(Math) is not object");
ok(typeof(String.prototype) === "object", "typeof(String.prototype) is not object");
ok(typeof(testFunc1) === "function", "typeof(testFunc1) is not function");
ok(typeof(String) === "function", "typeof(String) is not function");
ok(typeof(ScriptEngine) === "function", "typeof(ScriptEngine) is not function");
ok(typeof(this) === "object", "typeof(this) is not object");

ok(testFunc1(true, "test") === true, "testFunc1 not returned true");

var obj1 = new Object();
ok(typeof(obj1) === "object", "typeof(obj1) is not object");
obj1.test = true;
obj1.func = function () {
    ok(this === obj1, "this is not obj1");
    ok(this.test === true, "this.test is not true");
    ok(arguments.length === 1, "arguments.length is not 1");
    ok(arguments["0"] === true, "arguments[0] is not true");

    return "test";
};

ok(obj1.func(true) === "test", "obj1.func(true) is not \"test\"");

function testConstr1() {
    this.var1 = 1;

    ok(this !== undefined, "this is undefined");
    ok(arguments.length === 1, "arguments.length is not 1");
    ok(arguments["0"] === true, "arguments[0] is not 1");

    return false;
}

testConstr1.prototype.pvar = 1;

var obj2 = new testConstr1(true);
ok(typeof(obj2) === "object", "typeof(obj2) is not object");
ok(obj2.pvar === 1, "obj2.pvar is not 1");

testConstr1.prototype.pvar = 2;
ok(obj2.pvar === 2, "obj2.pvar is not 2");

obj2.pvar = 3;
testConstr1.prototype.pvar = 1;
ok(obj2.pvar === 3, "obj2.pvar is not 3");

var obj3 = new Object;
ok(typeof(obj3) === "object", "typeof(obj3) is not object");

for(var iter in "test")
    ok(false, "unexpected forin call, test = " + iter);

for(var iter in null)
    ok(false, "unexpected forin call, test = " + iter);

for(var iter in false)
    ok(false, "unexpected forin call, test = " + iter);

tmp = 0;
if(true)
    tmp = 1;
else
    ok(false, "else evaluated");
ok(tmp === 1, "tmp !== 1, if not evaluated?");

tmp = 0;
if(1 === 0)
    ok(false, "if evaluated");
else
    tmp = 1;
ok(tmp === 1, "tmp !== 1, if not evaluated?");

if(false)
    ok(false, "if(false) evaluated");

tmp = 0;
if(true)
    tmp = 1;
ok(tmp === 1, "tmp !== 1, if(true) not evaluated?");

if(false) {
}else {
}

var obj3 = { prop1: 1,  prop2: typeof(false) };
ok(obj3.prop1 === 1, "obj3.prop1 is not 1");
ok(obj3.prop2 === "boolean", "obj3.prop2 is not \"boolean\"");

{
    var blockVar = 1;
    blockVar = 2;
}
ok(blockVar === 2, "blockVar !== 2");

ok((true ? 1 : 2) === 1, "conditional expression true is not 1");
ok((0 === 2 ? 1 : 2) === 2, "conditional expression true is not 2");

ok(getVT(undefined) === "VT_EMPTY", "getVT(undefined) is not VT_EMPTY");
ok(getVT(null) === "VT_NULL", "getVT(null) is not VT_NULL");
ok(getVT(0) === "VT_I4", "getVT(0) is not VT_I4");
ok(getVT(0.5) === "VT_R8", "getVT(1.5) is not VT_R8");
ok(getVT("test") === "VT_BSTR", "getVT(\"test\") is not VT_BSTR");
ok(getVT(Math) === "VT_DISPATCH", "getVT(Math) is not VT_DISPATCH");
ok(getVT(false) === "VT_BOOL", "getVT(false) is not VT_BOOL");

tmp = 2+2;
ok(tmp === 4, "2+2 !== 4");
ok(getVT(tmp) === "VT_I4", "getVT(2+2) !== VT_I4");

tmp = 2+2.5;
ok(tmp === 4.5, "2+2.5 !== 4.5");
ok(getVT(tmp) === "VT_R8", "getVT(2+2.5) !== VT_R8");

tmp = 1.5+2.5;
ok(tmp === 4, "1.4+2.5 !== 4");
ok(getVT(tmp) === "VT_I4", "getVT(1.5+2.5) !== VT_I4");

tmp = 4-2;
ok(tmp === 2, "4-2 !== 2");
ok(getVT(tmp) === "VT_I4", "getVT(4-2) !== VT_I4");

tmp = 4.5-2;
ok(tmp === 2.5, "4.5-2 !== 2.5");
ok(getVT(tmp) === "VT_R8", "getVT(4-2) !== VT_R8");

tmp = -2;
ok(tmp === 0-2, "-2 !== 0-2");
ok(getVT(tmp) === "VT_I4", "getVT(-2) !== VT_I4");

tmp = 2*3;
ok(tmp === 6, "2*3 !== 6");
ok(getVT(tmp) === "VT_I4", "getVT(2*3) !== VT_I4");

tmp = 2*3.5;
ok(tmp === 7, "2*3.5 !== 7");
ok(getVT(tmp) === "VT_I4", "getVT(2*3.5) !== VT_I4");

tmp = 2.5*3.5;
ok(tmp === 8.75, "2.5*3.5 !== 8.75");
ok(getVT(tmp) === "VT_R8", "getVT(2.5*3.5) !== VT_R8");

tmp = 4/2;
ok(tmp === 2, "4/2 !== 2");
ok(getVT(tmp) === "VT_I4", "getVT(4/2) !== VT_I4");

tmp = 4.5/1.5;
ok(tmp === 3, "4.5/1.5 !== 3");
ok(getVT(tmp) === "VT_I4", "getVT(4.5/1.5) !== VT_I4");

tmp = 3/2;
ok(tmp === 1.5, "3/2 !== 1.5");
ok(getVT(tmp) === "VT_R8", "getVT(3/2) !== VT_R8");

tmp = 3%2;
ok(tmp === 1, "3%2 = " + tmp);

tmp = 4%2;
ok(tmp ===0, "4%2 = " + tmp);

tmp = 3.5%1.5;
ok(tmp === 0.5, "3.5%1.5 = " + tmp);

tmp = 3%true;
ok(tmp === 0, "3%true = " + tmp);

tmp = "ab" + "cd";
ok(tmp === "abcd", "\"ab\" + \"cd\" !== \"abcd\"");

tmp = 1;
ok((tmp += 1) === 2, "tmp += 1 !== 2");
ok(tmp === 2, "tmp !== 2");

tmp = 2;
ok((tmp -= 1) === 1, "tmp -= 1 !== 1");
ok(tmp === 1, "tmp !=== 1");

tmp = 2;
ok((tmp *= 1.5) === 3, "tmp *= 1.5 !== 3");
ok(tmp === 3, "tmp !=== 3");

tmp = 5;
ok((tmp /= 2) === 2.5, "tmp /= 2 !== 2.5");
ok(tmp === 2.5, "tmp !=== 2.5");

tmp = 3;
ok((tmp %= 2) === 1, "tmp %= 2 !== 1");
ok(tmp === 1, "tmp !== 1");

tmp = 8;
ok((tmp <<= 1) === 16, "tmp <<= 1 !== 16");

tmp = 8;
ok((tmp >>= 1) === 4, "tmp >>= 1 !== 4");

tmp = 8;
ok((tmp >>>= 1) === 4, "tmp >>>= 1 !== 4");

tmp = 3 || ok(false, "second or expression called");
ok(tmp === 3, "3 || (...) is not 3");

tmp = false || 2;
ok(tmp === 2, "false || 2 is not 2");

tmp = 0 && ok(false, "second and expression called");
ok(tmp === 0, "0 && (...) is not 0");

tmp = true && "test";
ok(tmp === "test", "true && \"test\" is not \"test\"");

tmp = true && 0;
ok(tmp === 0, "true && 0 is not 0");

tmp = 3 | 4;
ok(tmp === 7, "3 | 4 !== 7");
ok(getVT(tmp) === "VT_I4", "getVT(3|4) = " + getVT(tmp));

tmp = 3.5 | 0;
ok(tmp === 3, "3.5 | 0 !== 3");
ok(getVT(tmp) === "VT_I4", "getVT(3.5|0) = " + getVT(tmp));

tmp = -3.5 | 0;
ok(tmp === -3, "-3.5 | 0 !== -3");
ok(getVT(tmp) === "VT_I4", "getVT(3.5|0) = " + getVT(tmp));

tmp = 10;
ok((tmp |= 0x10) === 26, "tmp(10) |= 0x10 !== 26");
ok(getVT(tmp) === "VT_I4", "getVT(tmp |= 10) = " + getVT(tmp));

tmp = 3 & 5;
ok(tmp === 1, "3 & 5 !== 1");
ok(getVT(tmp) === "VT_I4", "getVT(3|5) = " + getVT(tmp));

tmp = 3.5 & 0xffff;
ok(tmp === 3, "3.5 & 0xffff !== 3 ");
ok(getVT(tmp) === "VT_I4", "getVT(3.5&0xffff) = " + getVT(tmp));

tmp = (-3.5) & 0xffffffff;
ok(tmp === -3, "-3.5 & 0xffff !== -3");
ok(getVT(tmp) === "VT_I4", "getVT(3.5&0xffff) = " + getVT(tmp));

tmp = 2 << 3;
ok(tmp === 16, "2 << 3 = " + tmp);

tmp = 2 << 35;
ok(tmp === 16, "2 << 35 = " + tmp);

tmp = 8 >> 2;
ok(tmp === 2, "8 >> 2 = " + tmp);

tmp = -64 >> 4;
ok(tmp === -4, "-64 >> 4 = " + tmp);

tmp = 8 >>> 2;
ok(tmp === 2, "8 >> 2 = " + tmp);

tmp = -64 >>> 4;
ok(tmp === 0x0ffffffc, "-64 >>> 4 = " + tmp);

tmp = 10;
ok((tmp &= 8) === 8, "tmp(10) &= 8 !== 8");
ok(getVT(tmp) === "VT_I4", "getVT(tmp &= 8) = " + getVT(tmp));

tmp = 0xf0f0^0xff00;
ok(tmp === 0x0ff0, "0xf0f0^0xff00 !== 0x0ff0");
ok(getVT(tmp) === "VT_I4", "getVT(0xf0f0^0xff00) = " + getVT(tmp));

tmp = 5;
ok((tmp ^= 3) === 6, "tmp(5) ^= 3 !== 6");
ok(getVT(tmp) === "VT_I4", "getVT(tmp ^= 3) = " + getVT(tmp));

tmp = ~1;
ok(tmp === -2, "~1 !== -2");
ok(getVT(tmp) === "VT_I4", "getVT(~1) = " + getVT(tmp));

ok((3,4) === 4, "(3,4) !== 4");

ok(+3 === 3, "+3 !== 3");
ok(+true === 1, "+true !== 1");
ok(+false === 0, "+false !== 0");
ok(+null === 0, "+null !== 0");
ok(+"0" === 0, "+'0' !== 0");
ok(+"3" === 3, "+'3' !== 3");
ok(+"-3" === -3, "+'-3' !== -3");
ok(+"0xff" === 255, "+'0xff' !== 255");
ok(+"3e3" === 3000, "+'3e3' !== 3000");

tmp = new Number(1);
ok(+tmp === 1, "ToNumber(new Number(1)) = " + (+tmp));
tmp = new String("1");
ok(+tmp === 1, "ToNumber(new String('1')) = " + (+tmp));

ok("" + 0 === "0", "\"\" + 0 !== \"0\"");
ok("" + 123 === "123", "\"\" + 123 !== \"123\"");
ok("" + (-5) === "-5", "\"\" + (-5) !== \"-5\"");
ok("" + null === "null", "\"\" + null !== \"null\"");
ok("" + undefined === "undefined", "\"\" + undefined !== \"undefined\"");
ok("" + true === "true", "\"\" + true !== \"true\"");
ok("" + false === "false", "\"\" + false !== \"false\"");
ok("" + 0.5 === "0.5", "'' + 0.5 = " + 0.5);
ok("" + (-0.5432) === "-0.5432", "'' + (-0.5432) = " + (-0.5432));

ok(1 < 3.4, "1 < 3.4 failed");
ok(!(3.4 < 1), "3.4 < 1");
ok("abc" < "abcd", "abc < abcd failed");
ok("abcd" < "abce", "abce < abce failed");
ok("" < "x", "\"\" < \"x\" failed");
ok(!(0 < 0), "0 < 0");

ok(1 <= 3.4, "1 <= 3.4 failed");
ok(!(3.4 <= 1), "3.4 <= 1");
ok("abc" <= "abcd", "abc <= abcd failed");
ok("abcd" <= "abce", "abce <= abce failed");
ok("" <= "x", "\"\" <= \"x\" failed");
ok(0 <= 0, "0 <= 0 failed");

ok(3.4 > 1, "3.4 > 1 failed");
ok(!(1 > 3.4), "1 > 3.4");
ok("abcd" > "abc", "abc > abcd failed");
ok("abce" > "abcd", "abce > abce failed");
ok("x" > "", "\"x\" > \"\" failed");
ok(!(0 > 0), "0 > 0");

ok(3.4 >= 1, "3.4 >= 1 failed");
ok(!(1 >= 3.4), "1 >= 3.4");
ok("abcd" >= "abc", "abc >= abcd failed");
ok("abce" >= "abcd", "abce >= abce failed");
ok("x" >= "", "\"x\" >= \"\" failed");
ok(0 >= 0, "0 >= 0");

tmp = 1;
ok(++tmp === 2, "++tmp (1) is not 2");
ok(tmp === 2, "incremented tmp is not 2");
ok(--tmp === 1, "--tmp (2) is not 1");
ok(tmp === 1, "decremented tmp is not 1");
ok(tmp++ === 1, "tmp++ (1) is not 1");
ok(tmp === 2, "incremented tmp(1) is not 2");
ok(tmp-- === 2, "tmp-- (2) is not 2");
ok(tmp === 1, "decremented tmp is not 1");

String.prototype.test = true;
ok("".test === true, "\"\".test is not true");

Boolean.prototype.test = true;
ok(true.test === true, "true.test is not true");

Number.prototype.test = true;
ok((0).test === true, "(0).test is not true");
ok((0.5).test === true, "(0.5).test is not true");

var state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
}catch(ex) {
    ok(false, "unexpected catch");
}
ok(state === "try", "state = " + state + " expected try");

state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
}finally {
    ok(state === "try", "funally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
}catch(ex) {
    ok(false, "unexpected catch");
}finally {
    ok(state === "try", "funally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

var state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
    throw "except";
}catch(ex) {
    ok(state === "try", "catch: state = " + state);
    ok(ex === "except", "ex is not \"except\"");
    state = "catch";
}
ok(state === "catch", "state = " + state + " expected catch");

var state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
    throw true;
}catch(ex) {
    ok(state === "try", "catch: state = " + state);
    ok(ex === true, "ex is not true");
    state = "catch";
}finally {
    ok(state === "catch", "funally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

var state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
    try { throw true; } finally {}
}catch(ex) {
    ok(state === "try", "catch: state = " + state);
    ok(ex === true, "ex is not true");
    state = "catch";
}finally {
    ok(state === "catch", "funally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

var state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
    try { throw "except"; } catch(ex) { throw true; }
}catch(ex) {
    ok(state === "try", "catch: state = " + state);
    ok(ex === true, "ex is not true");
    state = "catch";
}finally {
    ok(state === "catch", "funally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

function throwFunc(x) {
    throw x;
}

var state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
    throwFunc(true);
}catch(ex) {
    ok(state === "try", "catch: state = " + state);
    ok(ex === true, "ex is not true");
    state = "catch";
}finally {
    ok(state === "catch", "funally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

state = "";
switch(1) {
case "1":
    ok(false, "unexpected case \"1\"");
case 1:
    ok(state === "", "case 1: state = " + state);
    state = "1";
default:
    ok(state === "1", "default: state = " + state);
    state = "default";
case false:
    ok(state === "default", "case false: state = " + state);
    state = "false";
}
ok(state === "false", "state = " + state);

state = "";
switch("") {
case "1":
case 1:
    ok(false, "unexpected case 1");
default:
    ok(state === "", "default: state = " + state);
    state = "default";
case false:
    ok(state === "default", "case false: state = " + state);
    state = "false";
}
ok(state === "false", "state = " + state);

state = "";
switch(1) {
case "1":
    ok(false, "unexpected case \"1\"");
case 1:
    ok(state === "", "case 1: state = " + state);
    state = "1";
default:
    ok(state === "1", "default: state = " + state);
    state = "default";
    break;
case false:
    ok(false, "unexpected case false");
}
ok(state === "default", "state = " + state);

tmp = eval("1");
ok(tmp === 1, "eval(\"1\") !== 1");
eval("{ ok(tmp === 1, 'eval: tmp !== 1'); } tmp = 2;");
ok(tmp === 2, "tmp !== 2");

ok(eval(false) === false, "eval(false) !== false");
ok(eval() === undefined, "eval() !== undefined");

tmp = eval("1", "2");
ok(tmp === 1, "eval(\"1\", \"2\") !== 1");

var state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
    eval("throwFunc(true);");
}catch(ex) {
    ok(state === "try", "catch: state = " + state);
    ok(ex === true, "ex is not true");
    state = "catch";
}finally {
    ok(state === "catch", "funally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

tmp = [,,1,2,,,true];
ok(tmp.length === 7, "tmp.length !== 7");
ok(tmp["0"] === undefined, "tmp[0] is not undefined");
ok(tmp["3"] === 2, "tmp[3] !== 2");
ok(tmp["6"] === true, "tmp[6] !== true");
ok(tmp[2] === 1, "tmp[2] !== 1");

ok([1,].length === 2, "[1,].length !== 2");
ok([,,].length === 3, "[,,].length !== 3");
ok([,].length === 2, "[].length != 2");
ok([].length === 0, "[].length != 0");

tmp = 0;
while(tmp < 4) {
    ok(tmp < 4, "tmp >= 4");
    tmp++;
}
ok(tmp === 4, "tmp !== 4");

tmp = 0;
while(true) {
    ok(tmp < 4, "tmp >= 4");
    tmp++;
    if(tmp === 4) {
        break;
        ok(false, "break did not break");
    }
}
ok(tmp === 4, "tmp !== 4");

tmp = 0;
do {
    ok(tmp < 4, "tmp >= 4");
    tmp++;
} while(tmp < 4);
ok(tmp === 4, "tmp !== 4");

tmp = 0;
do {
    ok(tmp === 0, "tmp !=== 0");
    tmp++;
} while(false);
ok(tmp === 1, "tmp !== 1");

tmp = 0;
while(tmp < 4) {
    tmp++;
    if(tmp === 2) {
        continue;
        ok(false, "break did not break");
    }
    ok(tmp <= 4 && tmp != 2, "tmp = " + tmp);
}
ok(tmp === 4, "tmp !== 4");

for(tmp=0; tmp < 4; tmp++)
    ok(tmp < 4, "tmp = " + tmp);
ok(tmp === 4, "tmp !== 4");

for(tmp=0; tmp < 4; tmp++) {
    if(tmp === 2)
        break;
    ok(tmp < 2, "tmp = " + tmp);
}
ok(tmp === 2, "tmp !== 2");

for(tmp=0; tmp < 4; tmp++) {
    if(tmp === 2)
        continue;
    ok(tmp < 4 && tmp != 2, "tmp = " + tmp);
}
ok(tmp === 4, "tmp !== 4");

for(var fi=0; fi < 4; fi++)
    ok(fi < 4, "fi = " + fi);
ok(fi === 4, "fi !== 4");

ok((void 1) === undefined, "(void 1) !== undefined");

var inobj = new Object();

for(var iter in inobj)
    ok(false, "unexpected iter = " + iter);

inobj.test = true;
tmp = 0;
for(iter in inobj) {
    ok(iter == "test", "unexpected iter = " + iter);
    tmp++;
}
ok(tmp === 1, "for..in tmp = " + tmp);

function forinTestObj() {}

forinTestObj.prototype.test3 = true;

var arr = new Array();
inobj = new forinTestObj();
inobj.test1 = true;
inobj.test2 = true;

tmp = 0;
for(iter in inobj) {
    arr[iter] = true;
    tmp++;
}

ok(tmp === 3, "for..in tmp = " + tmp);
ok(arr["test1"] === true, "arr[test1] !== true");
ok(arr["test2"] === true, "arr[test2] !== true");
ok(arr["test3"] === true, "arr[test3] !== true");

tmp = new Object();
tmp.test = false;
ok((delete tmp.test) === true, "delete returned false");
ok(typeof(tmp.test) === "undefined", "tmp.test type = " + typeof(tmp.test));
for(iter in tmp)
    ok(false, "tmp has prop " + iter);

tmp.testWith = true;
with(tmp)
    ok(testWith === true, "testWith !== true");

if(false) {
    var varTest1 = true;
}

ok(varTest1 === undefined, "varTest1 = " + varTest1);
ok(varTest2 === undefined, "varTest2 = " + varTest1);

var varTest2;

function varTestFunc(varTest3) {
    var varTest3;

    ok(varTest3 === 3, "varTest3 = " + varTest3);
    ok(varTest4 === undefined, "varTest4 = " + varTest4);

    var varTest4;
}

varTestFunc(3);

deleteTest = 1;
delete deleteTest;
try {
    tmp = deleteTest;
    ok(false, "deleteTest not throwed exception?");
}catch(ex) {}

if (false)
    if (true)
        ok(false, "if evaluated");
    else
        ok(false, "else should be associated with nearest if statement");

if (true)
    if (false)
        ok(false, "if evaluated");
    else
        ok(true, "else should be associated with nearest if statement");

ok(isNaN(NaN) === true, "isNaN(NaN) !== true");
ok(isNaN(0.5) === false, "isNaN(0.5) !== false");
ok(isNaN(Infinity) === false, "isNaN(Infinity) !== false");
ok(isNaN() === true, "isNaN() !== true");
ok(isNaN(NaN, 0) === true, "isNaN(NaN, 0) !== true");
ok(isNaN(0.5, NaN) === false, "isNaN(0.5, NaN) !== false");
ok(isNaN(+undefined) === true, "isNaN(+undefined) !== true");

ok(isFinite(0.5) === true, "isFinite(0.5) !== true");
ok(isFinite(Infinity) === false, "isFinite(Infinity) !== fals");
ok(isFinite(-Infinity) === false, "isFinite(Infinity) !== fals");
ok(isFinite(NaN) === false, "isFinite(NaN) !== false");
ok(isFinite(0.5, NaN) === true, "isFinite(0.5, NaN) !== true");
ok(isFinite(NaN, 0.5) === false, "isFinite(NaN, 0.5) !== false");
ok(isFinite() === false, "isFinite() !== false");

ok((1 < NaN) === false, "(1 < NaN) !== false");
ok((1 > NaN) === false, "(1 > NaN) !== false");
ok((1 <= NaN) === false, "(1 <= NaN) !== false");
ok((1 >= NaN) === false, "(1 >= NaN) !== false");
ok((NaN < 1) === false, "(NaN < 1) !== false");
ok((NaN > 1) === false, "(NaN > 1) !== false");
ok((NaN <= 1) === false, "(NaN <= 1) !== false");
ok((NaN >= 1) === false, "(NaN >= 1) !== false");
ok((Infinity < 2) === false, "(Infinity < 2) !== false");
ok((Infinity > 2) === true, "(Infinity > 2) !== true");
ok((-Infinity < 2) === true, "(-Infinity < 2) !== true");

ok(isNaN(+"test") === true, "isNaN(+'test') !== true");
ok(isNaN(+"123t") === true, "isNaN(+'123t') !== true");
ok(isNaN(+"Infinity x") === true, "isNaN(+'Infinity x') !== true");
ok(+"Infinity" === Infinity, "+'Infinity' !== Infinity");
ok(+" Infinity " === Infinity, "+' Infinity ' !== Infinity");
ok(+"-Infinity" === -Infinity, "+'-Infinity' !== -Infinity");

ok((NaN !== NaN) === true, "(NaN !== NaN) !== true");
ok((NaN === NaN) === false, "(NaN === NaN) !== false");
ok((Infinity !== NaN) === true, "(Infinity !== NaN) !== true");
ok((Infinity !== NaN) === true, "(Infinity !== NaN) !== true");
ok((0 === NaN) === false, "(0 === NaN) !== false");

ok((NaN != NaN) === true, "(NaN !== NaN) != true");
ok((NaN == NaN) === false, "(NaN === NaN) != false");
ok((Infinity != NaN) === true, "(Infinity != NaN) !== true");
ok((Infinity != NaN) === true, "(Infinity != NaN) !== true");
ok((0 == NaN) === false, "(0 === NaN) != false");


ok(typeof(testFunc2) === "function", "typeof(testFunc2) = " + typeof(testFunc2));
tmp = testFunc2(1);
ok(tmp === 2, "testFunc2(1) = " + tmp);
function testFunc2(x) { return x+1; }

ok(typeof(testFunc3) === "function", "typeof(testFunc3) = " + typeof(testFunc3));
tmp = testFunc3(1);
ok(tmp === 3, "testFunc3(1) = " + tmp);
tmp = function testFunc3(x) { return x+2; };

tmp = testFunc4(1);
ok(tmp === 5, "testFunc4(1) = " + tmp);
tmp = function testFunc4(x) { return x+3; };
tmp = testFunc4(1);
testFunc4 = 1;
ok(testFunc4 === 1, "testFunc4 = " + testFunc4);
ok(tmp === 5, "testFunc4(1) = " + tmp);
tmp = function testFunc4(x) { return x+4; };
ok(testFunc4 === 1, "testFunc4 = " + testFunc4);

function testEmbededFunctions() {
    ok(typeof(testFunc5) === "function", "typeof(testFunc5) = " + typeof(testFunc5));
    tmp = testFunc5(1);
    ok(tmp === 3, "testFunc5(1) = " + tmp);
    tmp = function testFunc5(x) { return x+2; };

    tmp = testFunc6(1);
    ok(tmp === 5, "testFunc6(1) = " + tmp);
    tmp = function testFunc6(x) { return x+3; };
    tmp = testFunc6(1);
    ok(tmp === 5, "testFunc6(1) = " + tmp);
    tmp = function testFunc6(x) { return x+4; };
    testFunc6 = 1;
    ok(testFunc6 === 1, "testFunc4 = " + testFunc6);
}

testEmbededFunctions();

reportSuccess();

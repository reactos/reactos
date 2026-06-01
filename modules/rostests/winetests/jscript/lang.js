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
ok(8.64e15 === 8640000000000000, "8.64e15 !== 8640000000000000");
ok(1e2147483648 === Infinity, "1e2147483648 !== Infinity");

ok(00 === 0, "00 != 0");
ok(010 === 8, "010 != 8");
ok(077 === 63, "077 != 63");
ok(080 === 80, "080 != 80");
ok(090 === 90, "090 != 90");
ok(018 === 18, "018 != 18");
tmp = 07777777777777777777777;
ok(typeof(tmp) === "number" && tmp > 0xffffffff, "tmp = " + tmp);
tmp = 07777777779777777777777;
ok(typeof(tmp) === "number" && tmp > 0xffffffff, "tmp = " + tmp);
ok(0xffffffff === 4294967295, "0xffffffff = " + 0xffffffff);
tmp = 0x10000000000000000000000000000000000000000000000000000000000000000;
ok(tmp === Math.pow(2, 256), "0x1000...00 != 2^256");

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

ok(this === test, "this !== test");
eval('ok(this === test, "this !== test");');

var trueVar = true;
ok(trueVar, "trueVar is not true");

ok(ScriptEngine.length === 0, "ScriptEngine.length is not 0");

function testFunc1(x, y) {
    ok(this !== undefined, "this is undefined");
    ok(x === true, "x is not true");
    ok(y === "test", "y is not \"test\"");
    ok(arguments.length === 2, "arguments.length is not 2");
    ok(arguments["0"] === true, "arguments[0] is not true");
    ok(arguments["1"] === "test", "arguments[1] is not \"test\"");
    ok(arguments.callee === testFunc1, "arguments.calee !== testFunc1");
    ok(testFunc1.arguments === arguments, "testFunc1.arguments = " + testFunc1.arguments);

    x = false;
    ok(arguments[0] === false, "arguments[0] is not false");
    arguments[1] = "x";
    ok(y === "x", "y = " + y);
    ok(arguments[1] === "x", "arguments[1] = " + arguments[1]);

    ok(arguments["0x0"] === undefined, "arguments['0x0'] = " + arguments["0x0"]);
    ok(arguments["x"] === undefined, "arguments['x'] = " + arguments["x"]);

    ok(this === test, "this !== test");
    eval('ok(this === test, "this !== test");');

    tmp = delete arguments;
    ok(tmp === false, "arguments deleted");
    ok(typeof(arguments) === "object", "typeof(arguments) = " + typeof(arguments));

    x = 2;
    ok(x === 2, "x = " + x);
    ok(arguments[0] === 2, "arguments[0] = " + arguments[0]);

    return true;
}

ok(testFunc1.length === 2, "testFunc1.length is not 2");
ok(testFunc1.arguments === null, "testFunc1.arguments = " + testFunc1.arguments);

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
ok(Function.prototype.prototype === undefined, "Function.prototype.prototype is not undefined");
ok(Date.prototype !== undefined, "Date.prototype is undefined");
ok(Date.prototype.prototype === undefined, "Date.prototype is not undefined");

function testConstructor(constr, name, inst) {
    ok(constr.prototype.constructor === constr, name + ".prototype.constructor !== " + name);
    ok(constr.prototype.hasOwnProperty("constructor"), name + ".prototype.hasOwnProperty('constructor')");

    if(!inst)
        inst = new constr();

    ok(inst.constructor === constr, "(new " + name + "()).constructor !== " + name);
    ok(!inst.hasOwnProperty("constructor"), "(new " + name + "()).hasOwnProperty('constructor')");
}

testConstructor(Object, "Object");
testConstructor(String, "String");
testConstructor(Array, "Array");
testConstructor(Boolean, "Boolean");
testConstructor(Number, "Number");
testConstructor(RegExp, "RegExp", /x/);
testConstructor(Function, "Function");
testConstructor(Date, "Date");
testConstructor(VBArray, "VBArray", new VBArray(createArray()));
testConstructor(Error, "Error");
testConstructor(EvalError, "EvalError");
testConstructor(RangeError, "RangeError");
testConstructor(ReferenceError, "ReferenceError");
testConstructor(RegExpError, "RegExpError");
testConstructor(SyntaxError, "SyntaxError");
testConstructor(TypeError, "TypeError");
testConstructor(URIError, "URIError");

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
ok(typeof(doesnotexist) === "undefined", "typeof(doesnotexist) = " + typeof(doesnotexist));
tmp = typeof((new Object()).doesnotexist);
ok(tmp === "undefined", "typeof((new Object).doesnotexist = " + tmp);
tmp = typeof(testObj.onlyDispID);
ok(tmp === "unknown", "typeof(testObj.onlyDispID) = " + tmp);

ok("\0\0x\0\0".length === 5, "\"\\0\\0x\\0\\0\".length = " + "\0\0x\0\0".length);
ok("\0\0x\0\0" === String.fromCharCode(0) + "\0x\0" + String.fromCharCode(0), "\"\\0\\0x\\0\\0\" unexpected");

ok(testFunc1(true, "test") === true, "testFunc1 not returned true");

ok(testFunc1.arguments === null, "testFunc1.arguments = " + testFunc1.arguments);

(tmp) = 3;
ok(tmp === 3, "tmp = " + tmp);

(function() {
    /* VT_DATE handling */
    var d, e;
    ok(getVT(v_date(0)) === "VT_DATE", "vt v_date(0) = " + getVT(v_date(0)));
    d = v_date(0);
    e = Date.parse("Sat Dec 30 00:00:00 1899");
    ok(getVT(d) === "VT_DATE", "vt v_date(0) = " + getVT(d));
    ok(getVT(+d) === "VT_R8", "vt +v_date(0) = " + getVT(d));
    ok(getVT(d / d) === "VT_I4", "vt v_date(0) / v_date(0) = " + getVT(d / d));
    ok((+d) === e, "+v_date(0) = " + (+d) + " expected " + e);
    ok(("" + d).match(/^Sat Dec 30 00:00:00 .* 1899$/) != null, "+v_date(0) = " + d);
    ok(d != d, "date d == d");

    d = v_date(2.5);
    e = Date.parse("Mon Jan 1 12:00:00 1900");
    ok((+d) === e, "+v_date(2.5) = " + (+d));
    ok(("" + d).match(/^Mon Jan 1 12:00:00 .* 1900$/) != null, "+v_date(2.5) = " + d);

    d = v_date(42091);
    e = Date.parse("Sat Mar 28 00:00:00 2015");
    ok((+d) === e, "+v_date(2015y) = " + (+d) + " expected " + e);
    ok(("" + d).match(/^Sat Mar 28 00:00:00 .* 2015$/) != null, "+v_date(2015y) = " + d);
    ok(d != d, "date d == d");
})();

(function() {
    /* VT_CY handling */
    var d;
    todo_wine_ok(getVT(v_cy(0)) === "VT_R8", "vt v_cy(0) = " + getVT(v_cy(0)));
    todo_wine_ok(getVT(v_cy(10000)) === "VT_R8", "vt v_cy(10000) = " + getVT(v_cy(0)));
    d = v_cy(0);
    todo_wine_ok(getVT(d) === "VT_R8", "vt v_cy(0) = " + getVT(d));
    todo_wine_ok(getVT(+d) === "VT_R8", "vt +v_cy(0) = " + getVT(d));
    ok(d == 0, "v_cy(0) != 0\n");
    ok(d === 0, "v_cy(0) !== 0\n");
    ok("" + d === "0", "str(v_cy(0)) = " + d);
    ok(d === d, "date d !== d");

    d = v_cy(1000);
    ok(getVT(d) === "VT_R8", "vt v_cy(1000) = " + getVT(d));
    ok(getVT(+d) === "VT_R8", "vt +v_cy(1000) = " + getVT(d));
    ok(d == 0.1, "v_cy(1000) != 0, d = " + d);
    ok(d === 0.1, "v_cy(1000) !== 0.1\n");
    ok("" + d === "0.1", "str(v_cy(1000)) = " + d);
    ok(d === d, "date d !== d");

    d = v_cy(25000);
    ok(getVT(d) === "VT_R8", "vt v_cy(25000) = " + getVT(d));
    ok(getVT(+d) === "VT_R8", "vt +v_cy(25000) = " + getVT(d));
    ok(d === 2.5, "v_cy(25000) !== 2.5\n");
    ok("" + d === "2.5", "str(v_cy(25000)) = " + d);
})();

function testRecFunc(x) {
    ok(testRecFunc.arguments === arguments, "testRecFunc.arguments = " + testRecFunc.arguments);
    if(x) {
        testRecFunc(false);
        ok(testRecFunc.arguments === arguments, "testRecFunc.arguments = " + testRecFunc.arguments);
        ok(testRecFunc.arguments[0] === true, "testRecFunc.arguments.x = " + testRecFunc.arguments[0]);
    }
}

testRecFunc.arguments = 5;
ok(testRecFunc.arguments === null, "testRecFunc.arguments = " + testRecFunc.arguments);
testRecFunc(true);
ok(testRecFunc.arguments === null, "testRecFunc.arguments = " + testRecFunc.arguments);

function argumentsTest() {
    var save = arguments;
    with({arguments: 1}) {
        ok(arguments === 1, "arguments = " + arguments);
        (function() {
            ok(argumentsTest.arguments === save, "unexpected argumentsTest.arguments");
        })();
    }
    eval('ok(arguments === save, "unexpected arguments");');
    [1,2].sort(function() {
        ok(argumentsTest.arguments === save, "unexpected argumentsTest.arguments");
        return 1;
    });
}

argumentsTest();

// arguments object detached from its execution context
(function() {
    var args, get_x, set_x;

    function test_args(detached) {
        ok(args[0] === 1, "args[0] = " + args[0]);
        set_x(2);
        ok(args[0] === (detached ? 1 : 2), "args[0] = " + args[0] + " expected " + (detached ? 1 : 2));
        args[0] = 3;
        ok(get_x() === (detached ? 2 : 3), "get_x() = " + get_x());
        ok(args[0] === 3, "args[0] = " + args[0]);
    }

    (function(x) {
        args = arguments;
        get_x = function() { return x; };
        set_x = function(v) { x = v; };

        test_args(false);
        x = 1;
    })(1);

    test_args(true);
})();

// arguments is a regular variable, it may be overwritten
(function() {
    ok(typeof(arguments) === "object", "typeof(arguments) = " + typeof(arguments));
    arguments = 1;
    ok(arguments === 1, "arguments = " + arguments);
})();

// duplicated argument names are shadowed by the last argument with the same name
(function() {
    var args, get_a, set_a;

    (function(a, a, b, c) {
        get_a = function() { return a; }
        set_a = function(v) { a = v; }
        ok(get_a() === 2, "function(a, a, b, c) get_a() = " + get_a());
        ok(a === 2, "function(a, a, b, c) a = " + a);
        ok(b === 3, "function(a, a, b, c) b = " + b);
        ok(c === 4, "function(a, a, b, c) c = " + c);
        a = 42;
        ok(arguments[0] === 1, "function(a, a, b, c) arguments[0] = " + arguments[0]);
        ok(arguments[1] === 42, "function(a, a, b, c) arguments[1] = " + arguments[1]);
        ok(get_a() === 42, "function(a, a, b, c) get_a() = " + get_a() + " expected 42");
        args = arguments;
    })(1, 2, 3, 4);

    ok(get_a() === 42, "function(a, a, b, c) get_a() after detach = " + get_a());
    set_a(100);
    ok(get_a() === 100, "function(a, a, b, c) get_a() = " + get_a() + " expected 100");
    ok(args[0] === 1, "function(a, a, b, c) detached args[0] = " + args[0]);
    ok(args[1] === 42, "function(a, a, b, c) detached args[1] = " + args[1]);

    (function(a, a) {
        eval("var a = 7;");
        ok(a === 7, "function(a, a) a = " + a);
        ok(arguments[0] === 5, "function(a, a) arguments[0] = " + arguments[0]);
        ok(arguments[1] === 7, "function(a, a) arguments[1] = " + arguments[1]);
    })(5, 6);
})();

(function callAsExprTest() {
    ok(callAsExprTest.arguments === null, "callAsExprTest.arguments = " + callAsExprTest.arguments);
})(1,2);

tmp = ((function() { var f = function() {return this}; return (function() { return f(); }); })())();
ok(tmp === this, "detached scope function call this != global this");

tmp = (function() {1;})();
ok(tmp === undefined, "tmp = " + tmp);
tmp = eval("1;");
ok(tmp === 1, "tmp = " + tmp);
tmp = eval("1,2;");
ok(tmp === 2, "tmp = " + tmp);
tmp = eval("testNoRes(),2;");
ok(tmp === 2, "tmp = " + tmp);
tmp = eval("if(true) {3}");
ok(tmp === 3, "tmp = " + tmp);
eval("testRes(); testRes()");
tmp = eval("3; if(false) {4;} else {};;;")
ok(tmp === 3, "tmp = " + tmp);
tmp = eval("try { 1; } finally { 2; }")
ok(tmp === 2, "tmp = " + tmp);

testNoRes();
testRes() && testRes();
testNoRes(), testNoRes();

(function() {
    eval("var x=1;");
    ok(x === 1, "x = " + x);
})();

(function() {
    var e = eval;
    var r = e(1);
    ok(r === 1, "r = " + r);
    (function(x, a) { x(a); })(eval, "2");
})();

(function(r) {
    r = eval("1");
    ok(r === 1, "r = " + r);
    (function(x, a) { x(a); })(eval, "2");
})();

tmp = (function(){ return testNoRes(), testRes();})();

var f1, f2;

ok(funcexpr() == 2, "funcexpr() = " + funcexpr());

f1 = function funcexpr() { return 1; }
ok(f1 != funcexpr, "f1 == funcexpr");
ok(f1() === 1, "f1() = " + f1());

f2 = function funcexpr() { return 2; }
ok(f2 != funcexpr, "f2 != funcexpr");
ok(f2() === 2, "f2() = " + f2());

f1 = null;
for(i = 0; i < 3; i++) {
    f2 = function funcexpr2() {};
    ok(f1 != f2, "f1 == f2");
    f1 = f2;
}

f1 = null;
for(i = 0; i < 3; i++) {
    f2 = function() {};
    ok(f1 != f2, "f1 == f2");
    f1 = f2;
}

(function() {
    ok(infuncexpr() == 2, "infuncexpr() = " + infuncexpr());

    f1 = function infuncexpr() { return 1; }
    ok(f1 != funcexpr, "f1 == funcexpr");
    ok(f1() === 1, "f1() = " + f1());

    f2 = function infuncexpr() { return 2; }
    ok(f2 != funcexpr, "f2 != funcexpr");
    ok(f2() === 2, "f2() = " + f2());

    f1 = null;
    for(i = 0; i < 3; i++) {
        f2 = function infuncexpr2() {};
        ok(f1 != f2, "f1 == f2");
        f1 = f2;
    }

    f1 = null;
    for(i = 0; i < 3; i++) {
        f2 = function() {};
        ok(f1 != f2, "f1 == f2");
        f1 = f2;
    }
})();

var obj1 = new Object();
ok(typeof(obj1) === "object", "typeof(obj1) is not object");
ok(obj1.constructor === Object, "unexpected obj1.constructor");
obj1.test = true;
obj1.func = function () {
    ok(this === obj1, "this is not obj1");
    ok(this.test === true, "this.test is not true");
    ok(arguments.length === 1, "arguments.length is not 1");
    ok(arguments["0"] === true, "arguments[0] is not true");
    ok(typeof(arguments.callee) === "function", "typeof(arguments.calee) = " + typeof(arguments.calee));
    ok(arguments.caller === null, "arguments.caller = " + arguments.caller);
    function test_caller() { ok(arguments.caller === foobar.arguments, "nested arguments.caller = " + arguments.caller); }
    function foobar() { test_caller(); } foobar();

    return "test";
};

ok(obj1.func(true) === "test", "obj1.func(true) is not \"test\"");

function testConstr1() {
    this.var1 = 1;

    ok(this !== undefined, "this is undefined");
    ok(arguments.length === 1, "arguments.length is not 1");
    ok(arguments["0"] === true, "arguments[0] is not 1");
    ok(arguments.callee === testConstr1, "arguments.calee !== testConstr1");

    return false;
}

testConstr1.prototype.pvar = 1;
ok(testConstr1.prototype.constructor === testConstr1, "testConstr1.prototype.constructor !== testConstr1");

var obj2 = new testConstr1(true);
ok(typeof(obj2) === "object", "typeof(obj2) is not object");
ok(obj2.constructor === testConstr1, "unexpected obj2.constructor");
ok(obj2.pvar === 1, "obj2.pvar is not 1");
ok(!obj2.hasOwnProperty('constructor'), "obj2.hasOwnProperty('constructor')");

testConstr1.prototype.pvar = 2;
ok(obj2.pvar === 2, "obj2.pvar is not 2");

obj2.pvar = 3;
testConstr1.prototype.pvar = 1;
ok(obj2.pvar === 3, "obj2.pvar is not 3");

obj1 = new Object();
function testConstr3() {
    return obj1;
}

obj2 = new testConstr3();
ok(obj1 === obj2, "obj1 != obj2");

function testConstr4() {
    return 2;
}

obj2 = new testConstr3();
ok(typeof(obj2) === "object", "typeof(obj2) = " + typeof(obj2));

var obj3 = new Object;
ok(typeof(obj3) === "object", "typeof(obj3) is not object");

(function() {
    ok(typeof(func) === "function", "typeof(func) = " + typeof(func));
    function func() {}
    ok(typeof(func) === "function", "typeof(func) = " + typeof(func));
    func = 0;
    ok(typeof(func) === "number", "typeof(func) = " + typeof(func));
})();

(function(f) {
    ok(typeof(f) === "function", "typeof(f) = " + typeof(f));
    function f() {};
    ok(typeof(f) === "function", "typeof(f) = " + typeof(f));
})(1);

for(var iter in "test")
    ok(false, "unexpected forin call, test = " + iter);

for(var iter in null)
    ok(false, "unexpected forin call, test = " + iter);

for(var iter in false)
    ok(false, "unexpected forin call, test = " + iter);

for(var iter in pureDisp)
    ok(false, "unexpected forin call in pureDisp object");

tmp = new Object();
ok(!tmp.nonexistent, "!tmp.nonexistent = " + !tmp.nonexistent);
ok(!("nonexistent" in tmp), "nonexistent is in tmp after '!' expression")

tmp = new Object();
ok((~tmp.nonexistent) === -1, "!tmp.nonexistent = " + ~tmp.nonexistent);
ok(!("nonexistent" in tmp), "nonexistent is in tmp after '~' expression")

tmp = new Object();
ok(isNaN(+tmp.nonexistent), "!tmp.nonexistent = " + (+tmp.nonexistent));
ok(!("nonexistent" in tmp), "nonexistent is in tmp after '+' expression")

tmp = new Object();
tmp[tmp.nonexistent];
ok(!("nonexistent" in tmp), "nonexistent is in tmp after array expression")

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
ok(obj3.constructor === Object, "unexpected obj3.constructor");

if(invokeVersion >= 2) {
    eval("tmp = {prop: 'value',}");
    ok(tmp.prop === "value", "tmp.prop = " + tmp.prop);
    eval("tmp = {prop: 'value',second:2,}");
    ok(tmp.prop === "value", "tmp.prop = " + tmp.prop);
}else {
    try {
        eval("tmp = {prop: 'value',}");
    }catch(e) {
        tmp = true;
    }
    ok(tmp === true, "exception not fired");
}

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
ok(getVT(0.5) === "VT_R8", "getVT(0.5) is not VT_R8");
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
ok(getVT(tmp) === "VT_R8", "getVT(4.5-2) !== VT_R8");

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
/* FIXME: the parser loses precision */
/* ok(tmp === 8.75, "2.5*3.5 !== 8.75"); */
ok(tmp > 8.749999 && tmp < 8.750001, "2.5*3.5 !== 8.75");
ok(getVT(tmp) === "VT_R8", "getVT(2.5*3.5) !== VT_R8");

tmp = 2*.5;
ok(tmp === 1, "2*.5 !== 1");
ok(getVT(tmp) == "VT_I4", "getVT(2*.5) !== VT_I4");

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

tmp = 0 | NaN;
ok(tmp === 0, "0 | NaN = " + tmp);

tmp = 0 | Infinity;
ok(tmp === 0, "0 | NaN = " + tmp);

tmp = 0 | (-Infinity);
ok(tmp === 0, "0 | NaN = " + tmp);

tmp = 10;
ok((tmp |= 0x10) === 26, "tmp(10) |= 0x10 !== 26");
ok(getVT(tmp) === "VT_I4", "getVT(tmp |= 10) = " + getVT(tmp));

tmp = (123 * Math.pow(2,32) + 2) | 0;
ok(tmp === 2, "123*2^32+2 | 0 = " + tmp);

tmp = (-123 * Math.pow(2,32) + 2) | 0;
ok(tmp === 2, "123*2^32+2 | 0 = " + tmp);

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

tmp = 4 >>> NaN;
ok(tmp === 4, "4 >>> NaN = " + tmp);

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
ok(+tmp === 1, "+(new Number(1)) = " + (+tmp));
ok(tmp.constructor === Number, "unexpected tmp.constructor");
tmp = new String("1");
ok(+tmp === 1, "+(new String('1')) = " + (+tmp));
ok(tmp.constructor === String, "unexpected tmp.constructor");

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

tmp = new Object();
tmp.iii++;
ok(isNaN(tmp.iii), "tmp.iii = " + tmp.iii);

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
    ok(state === "try", "finally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

state = "";
try {
    try {
        throw 0;
    }finally {
        state = "finally";
    }
}catch(e) {
    ok(state === "finally", "state = " + state + " expected finally");
    state = "catch";
}
ok(state === "catch", "state = " + state + " expected catch");

try {
    try {
        throw 0;
    }finally {
        throw 1;
    }
}catch(e) {
    ok(e === 1, "e = " + e);
}

state = "";
try {
    ok(state === "", "try: state = " + state);
    state = "try";
}catch(ex) {
    ok(false, "unexpected catch");
}finally {
    ok(state === "try", "finally: state = " + state);
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
    ok(state === "catch", "finally: state = " + state);
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
    ok(state === "catch", "finally: state = " + state);
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
    ok(state === "catch", "finally: state = " + state);
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
    ok(state === "catch", "finally: state = " + state);
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

switch(1) {
case 2:
    ok(false, "unexpected case 2");
case 3:
    ok(false, "unexpected case 3");
}

switch(1) {
case 2:
    ok(false, "unexpected case 2");
    break;
default:
    /* empty default */
}

switch(2) {
default:
    ok(false, "unexpected default");
    break;
case 2:
    /* empty case */
};

switch(2) {
default:
    ok(false, "unexpected default");
    break;
case 1:
case 2:
case 3:
    /* empty case */
};

(function() {
    var i=0;

    switch(1) {
    case 1:
        i++;
    }
    return i;
})();

(function() {
    var ret, x;

    function unreachable() {
        ok(false, "unreachable");
    }

    function expect(value, expect_value) {
        ok(value === expect_value, "got " + value + " expected " + expect_value);
    }

    ret = (function() {
        try {
            return "try";
            unreachable();
        }catch(e) {
            unreachable();
        }finally {
            return "finally";
            unreachable();
        }
        unreachable();
    })();
    expect(ret, "finally");

    x = "";
    ret = (function() {
        try {
            x += "try,";
            return x;
            unreachable();
        }catch(e) {
            unreachable();
        }finally {
            x += "finally,";
        }
        unreachable();
    })();
    expect(ret, "try,");
    expect(x, "try,finally,");

    x = "";
    ret = (function() {
        try {
            x += "try,"
            throw 1;
            unreachable();
        }catch(e) {
            x += "catch,";
            return "catch";
            unreachable();
        }finally {
            x += "finally,";
            return "finally";
            unreachable();
        }
        unreachable();
    })();
    expect(ret, "finally");
    expect(x, "try,catch,finally,");

    x = "";
    ret = (function() {
        try {
            x += "try,"
            throw 1;
            unreachable();
        }catch(e) {
            x += "catch,";
            return "catch";
            unreachable();
        }finally {
            x += "finally,";
        }
        unreachable();
    })();
    expect(ret, "catch");
    expect(x, "try,catch,finally,");

    x = "";
    ret = (function() {
        try {
            x += "try,"
            try {
                x += "try2,";
                return "try2";
            }catch(e) {
                unreachable();
            }finally {
                x += "finally2,";
            }
            unreachable();
        }catch(e) {
            unreachable();
        }finally {
            x += "finally,";
        }
        unreachable();
    })();
    expect(ret, "try2");
    expect(x, "try,try2,finally2,finally,");

    x = "";
    ret = (function() {
        while(true) {
            try {
                x += "try,"
                try {
                    x += "try2,";
                    break;
                }catch(e) {
                    unreachable();
                }finally {
                    x += "finally2,";
                }
                unreachable();
            }catch(e) {
                unreachable();
            }finally {
                x += "finally,";
            }
            unreachable();
        }
        x += "ret";
        return "ret";
    })();
    expect(ret, "ret");
    expect(x, "try,try2,finally2,finally,ret");

    x = "";
    ret = (function() {
        while(true) {
            try {
                x += "try,"
                try {
                    x += "try2,";
                    continue;
                }catch(e) {
                    unreachable();
                }finally {
                    x += "finally2,";
                }
                unreachable();
            }catch(e) {
                unreachable();
            }finally {
                x += "finally,";
                break;
            }
            unreachable();
        }
        x += "ret";
        return "ret";
    })();
    expect(ret, "ret");
    expect(x, "try,try2,finally2,finally,ret");

    ret = (function() {
        try {
            return "try";
            unreachable();
        }catch(e) {
            unreachable();
        }finally {
            new Object();
            var tmp = (function() {
                var s = new String();
                try {
                    s.length;
                }finally {
                    return 1;
                }
            })();
        }
        unreachable();
    })();
    expect(ret, "try");
})();

(function() {
    var e;
    var E_FAIL = -2147467259;
    var JS_E_SUBSCRIPT_OUT_OF_RANGE = -2146828279;

    try {
        throwInt(E_FAIL);
    }catch(ex) {
        e = ex;
    }
    ok(e.name === "Error", "e.name = " + e.name);
    ok(e.message === "", "e.message = " + e.message);
    ok(e.number === E_FAIL, "e.number = " + e.number);

    try {
        throwInt(JS_E_SUBSCRIPT_OUT_OF_RANGE);
    }catch(ex) {
        e = ex;
    }
    ok(e.name === "RangeError", "e.name = " + e.name);
    ok(e.number === JS_E_SUBSCRIPT_OUT_OF_RANGE, "e.number = " + e.number);

    try {
        throwEI(JS_E_SUBSCRIPT_OUT_OF_RANGE);
    }catch(ex) {
        e = ex;
    }
    ok(e.name === "RangeError", "e.name = " + e.name);
    ok(e.number === JS_E_SUBSCRIPT_OUT_OF_RANGE, "e.number = " + e.number);
})();

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
    ok(state === "catch", "finally: state = " + state);
    state = "finally";
}
ok(state === "finally", "state = " + state + " expected finally");

tmp = [,,1,2,,,true];
ok(tmp.length === 7, "tmp.length !== 7");
ok(tmp["0"] === undefined, "tmp[0] is not undefined");
ok(tmp["3"] === 2, "tmp[3] !== 2");
ok(tmp["6"] === true, "tmp[6] !== true");
ok(tmp[2] === 1, "tmp[2] !== 1");
ok(!("0" in tmp), "0 found in array");
ok(!("1" in tmp), "1 found in array");
ok("2" in tmp, "2 not found in array");
ok(!("2" in [1,,,,]), "2 found in [1,,,,]");

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
do {
    ok(tmp < 4, "tmp >= 4");
    tmp++;
} while(tmp < 4)
ok(tmp === 4, "tmp !== 4")

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

tmp = true;
obj1 = new Object();
for(obj1.nonexistent; tmp; tmp = false)
    ok(!("nonexistent" in obj1), "nonexistent added to obj1");

obj1 = new Object();
for(tmp in obj1.nonexistent)
    ok(false, "for(tmp in obj1.nonexistent) called with tmp = " + tmp);
ok(!("nonexistent" in obj1), "nonexistent added to obj1 by for..in loop");


var i, j;

/* Previous versions have broken finally block implementation */
if(ScriptEngineMinorVersion() >= 8) {
    tmp = "";
    i = 0;
    while(true) {
        tmp += "1";
        for(i = 1; i < 3; i++) {
            switch(i) {
            case 1:
                tmp += "2";
                continue;
            case 2:
                tmp += "3";
                try {
                    throw null;
                }finally {
                    tmp += "4";
                    break;
                }
            default:
                ok(false, "unexpected state");
            }
            tmp += "5";
        }
        with({prop: "6"}) {
            tmp += prop;
            break;
        }
    }
    ok(tmp === "123456", "tmp = " + tmp);
}

tmp = "";
i = 0;
for(j in [1,2,3]) {
    tmp += "1";
    for(;;) {
        with({prop: "2"}) {
            tmp += prop;
            try {
                throw "3";
            }catch(e) {
                tmp += e;
                with([0])
                    break;
            }
        }
        ok(false, "unexpected state");
    }
    while(true) {
        tmp += "4";
        break;
    }
    break;
}
ok(tmp === "1234", "tmp = " + tmp);

tmp = 0;
for(var iter in [1,2,3]) {
    tmp += +iter;
    continue;
}
ok(tmp === 3, "tmp = " + tmp);

tmp = false;
for(var iter in [1,2,3]) {
    switch(+iter) {
    case 1:
        tmp = true;
        try {
            continue;
        }finally {}
    default:
        continue;
    }
}
ok(tmp, "tmp = " + tmp);

loop_label:
while(true) {
    while(true)
        break loop_label;
}

loop_label: {
    tmp = 0;
    while(true) {
        while(true)
            break loop_label;
    }
    ok(false, "unexpected evaluation 1");
}

while(true) {
    some_label: break;
    ok(false, "unexpected evaluation 2");
}

just_label: tmp = 1;
ok(tmp === 1, "tmp != 1");

some_label: break some_label;

other_label: {
    break other_label;
    ok(false, "unexpected evaluation 3");
}

loop_label:
do {
    while(true)
        continue loop_label;
}while(false);

loop_label:
for(i = 0; i < 3; i++) {
    while(true)
        continue loop_label;
}

loop_label:
other_label:
for(i = 0; i < 3; i++) {
    while(true)
        continue loop_label;
}

loop_label:
for(tmp in {prop: false}) {
    while(true)
        continue loop_label;
}

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
    forinTestObj.prototype.test4 = true;
    arr[iter] = true;
    tmp++;
}

ok(tmp === 3, "for..in tmp = " + tmp);
ok(arr["test1"] === true, "arr[test1] !== true");
ok(arr["test2"] === true, "arr[test2] !== true");
ok(arr["test3"] === true, "arr[test3] !== true");
ok(arr["test4"] !== true, "arr[test4] === true");

ok((delete inobj.test1) === true, "delete inobj.test1 returned false");
ok(!("test1" in inobj), "test1 is still in inobj after delete");
ok((delete inobj.test3) === true, "delete inobj.test3 returned false");
ok("test3" in inobj, "test3 is not in inobj after delete");
ok((delete forinTestObj.prototype.test3) === true, "delete forinTestObj.prototype.test3 returned false");
ok(!("test3" in inobj), "test3 is still in inobj after delete on prototype");

tmp = new Object();
tmp.test = false;
ok((delete tmp.test) === true, "delete returned false");
ok(typeof(tmp.test) === "undefined", "tmp.test type = " + typeof(tmp.test));
ok(!("test" in tmp), "test is still in tmp after delete?");
for(iter in tmp)
    ok(false, "tmp has prop " + iter);
ok((delete tmp.test) === true, "deleting test didn't return true");
ok((delete tmp.nonexistent) === true, "deleting nonexistent didn't return true");
ok((delete nonexistent) === true, "deleting nonexistent didn't return true");

tmp = new Object();
tmp.test = false;
ok((delete tmp["test"]) === true, "delete returned false");
ok(typeof(tmp.test) === "undefined", "tmp.test type = " + typeof(tmp.test));
ok(!("test" in tmp), "test is still in tmp after delete?");

arr = [1, 2, 3];
ok(arr.length === 3, "arr.length = " + arr.length);
ok((delete arr.length) === false, "delete arr.length returned true");
ok("reverse" in arr, "reverse not in arr");
ok((delete Array.prototype.reverse) === true, "delete Array.prototype.reverse returned false");
ok(!("reverse" in arr), "reverse is still in arr after delete from prototype");

tmp.testWith = true;
with(tmp)
    ok(testWith === true, "testWith !== true");

function withScopeTest()
{
    var a = 3;
    with({a : 2})
    {
        ok(a == 2, "withScopeTest: a != 2");
        function func()
        {
            ok(a == 3, "withScopeTest: func: a != 3");
        }
        func();
        eval('ok(a == 2, "withScopeTest: eval: a != 2");');
    }
}
withScopeTest();

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
    ok(false, "deleteTest did not throw an exception?");
}catch(ex) {}

(function() {
    var to_delete = 2;
    var r = delete to_delete;
    ok(r === false, "delete 1 returned " + r);
    if(r)
        return;
    ok(to_delete === 2, "to_delete = " + to_delete);

    to_delete = new Object();
    r = delete to_delete;
    ok(r === false, "delete 2 returned " + r);
    ok(typeof(to_delete) === "object", "typeof(to_delete) = " + typeof(to_delete));
})();

(function(to_delete) {
    var r = delete to_delete;
    ok(r === false, "delete 3 returned " + r);
    ok(to_delete === 2, "to_delete = " + to_delete);

    to_delete = new Object();
    r = delete to_delete;
    ok(r === false, "delete 4 returned " + r);
    ok(typeof(to_delete) === "object", "typeof(to_delete) = " + typeof(to_delete));
})(2);

(function() {
    with({to_delete: new Object()}) {
        var r = delete to_delete;
        ok(r === true, "delete returned " + r);
    }
})();

(function() {
    function constr() {}
    constr.prototype = { prop: 1 };
    var o = new constr(), r;
    ok(o.prop === 1, "o.prop = " + o.prop);
    r = delete constr.prototype.prop;
    ok(r === true, "delete returned " + r);
    ok(o.prop === undefined, "o.prop = " + o.prop);
    r = delete o["prop"];
    ok(r === true, "delete returned " + r);
})();

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

function instanceOfTest() {}
tmp = new instanceOfTest();

ok((tmp instanceof instanceOfTest) === true, "tmp is not instance of instanceOfTest");
ok((tmp instanceof Object) === true, "tmp is not instance of Object");
ok((tmp instanceof String) === false, "tmp is instance of String");
ok(instanceOfTest.isPrototypeOf(tmp) === false, "instanceOfTest is prototype of tmp");
ok(instanceOfTest.prototype.isPrototypeOf(tmp) === true, "instanceOfTest.prototype is not prototype of tmp");
ok(Object.prototype.isPrototypeOf(tmp) === true, "Object.prototype is not prototype of tmp");

instanceOfTest.prototype = new Object();
ok((tmp instanceof instanceOfTest) === false, "tmp is instance of instanceOfTest");
ok((tmp instanceof Object) === true, "tmp is not instance of Object");
ok(instanceOfTest.prototype.isPrototypeOf(tmp) === false, "instanceOfTest.prototype is prototype of tmp");

ok((1 instanceof Object) === false, "1 is instance of Object");
ok((false instanceof Boolean) === false, "false is instance of Boolean");
ok(("" instanceof Object) === false, "'' is instance of Object");
ok(Number.prototype.isPrototypeOf(1) === false, "Number.prototype is prototype of 1");
ok(String.prototype.isPrototypeOf("") === false, "String.prototype is prototype of ''");

ok(tmp.isPrototypeOf(null) === false, "tmp is prototype of null");
ok(tmp.isPrototypeOf(undefined) === false, "tmp is prototype of undefined");
ok(Object.prototype.isPrototypeOf.call(tmp) === false, "tmp is prototype of no argument");
ok(Object.prototype.isPrototypeOf.call(test, Object) === false, "test is prototype of Object");
ok(Object.prototype.isPrototypeOf.call(testObj, Object) === false, "testObj is prototype of Object");
ok(Object.prototype.isPrototypeOf(test) === false, "Object.prototype is prototype of test");
ok(Object.prototype.isPrototypeOf(testObj) === false, "Object.prototype is prototype of testObj");

(function () {
    ok((arguments instanceof Object) === true, "argument is not instance of Object");
    ok((arguments instanceof Array) === false, "argument is not instance of Array");
    ok(arguments.toString() === "[object Object]", "arguments.toString() = " + arguments.toString());
})(1,2);

obj = new String();
ok(("length" in obj) === true, "length is not in obj");
ok(("isPrototypeOf" in obj) === true, "isPrototypeOf is not in obj");
ok(("abc" in obj) === false, "test is in obj");
obj.abc = 1;
ok(("abc" in obj) === true, "test is not in obj");
ok(("1" in obj) === false, "1 is in obj");

obj = [1,2,3];
ok((1 in obj) === true, "1 is not in obj");

obj = new Object();
try {
    obj.prop["test"];
    ok(false, "expected exception");
}catch(e) {}
ok(!("prop" in obj), "prop in obj");

if(invokeVersion >= 2) {
    ok("test"[0] === "t", '"test"[0] = ' + test[0]);
    ok("test"[5] === undefined, '"test"[0] = ' + test[0]);

    tmp = "test";
    ok(tmp[1] === "e", "tmp[1] = " + tmp[1]);
    tmp[1] = "x";
    ok(tmp[1] === "e", "tmp[1] = " + tmp[1]);
    ok(tmp["1"] === "e", "tmp['1'] = " + tmp["1"]);
    ok(tmp["0x1"] === undefined, "tmp['0x1'] = " + tmp["0x1"]);
}else {
    ok("test"[0] === undefined, '"test"[0] = ' + test[0]);
}

ok(isNaN(NaN) === true, "isNaN(NaN) !== true");
ok(isNaN(0.5) === false, "isNaN(0.5) !== false");
ok(isNaN(Infinity) === false, "isNaN(Infinity) !== false");
ok(isNaN() === true, "isNaN() !== true");
ok(isNaN(NaN, 0) === true, "isNaN(NaN, 0) !== true");
ok(isNaN(0.5, NaN) === false, "isNaN(0.5, NaN) !== false");
ok(isNaN(+undefined) === true, "isNaN(+undefined) !== true");

ok(isFinite(0.5) === true, "isFinite(0.5) !== true");
ok(isFinite(Infinity) === false, "isFinite(Infinity) !== false");
ok(isFinite(-Infinity) === false, "isFinite(Infinity) !== false");
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

// escape tests
var escapeTests = [
    ["\'", "\\'", 39],
    ["\"", "\\\"", 34],
    ["\\", "\\\\", 92],
    ["\b", "\\b", 8],
    ["\t", "\\t", 9],
    ["\n", "\\n", 10],
    ["\v", "\\v", 118],
    ["\f", "\\f", 12],
    ["\r", "\\r", 13],
    ["\xf3", "\\xf3", 0xf3],
    ["\u1234", "\\u1234", 0x1234],
    ["\a", "\\a", 97],
    ["\?", "\\?", 63]
];

for(i=0; i<escapeTests.length; i++) {
    tmp = escapeTests[i][0].charCodeAt(0);
    ok(tmp === escapeTests[i][2], "escaped '" + escapeTests[i][1] + "' = " + tmp + " expected " + escapeTests[i][2]);
}

tmp = !+"\v1";
ok(tmp === true, '!+"\v1" = ' + tmp);

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

function testEmbeddedFunctions() {
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

testEmbeddedFunctions();

date = new Date();
date.toString = function() { return "toString"; }
ok(""+date === "toString", "''+date = " + date);
date.toString = function() { return this; }
ok(""+date === ""+date.valueOf(), "''+date = " + date);

str = new String("test");
str.valueOf = function() { return "valueOf"; }
ok(""+str === "valueOf", "''+str = " + str);
str.valueOf = function() { return new Date(); }
ok(""+str === "test", "''+str = " + str);

ok((function (){return 1;})() === 1, "(function (){return 1;})() = " + (function (){return 1;})());

(function() {
    var order = "", o = {};
    o[order += "1,", { toString: function() { order += "2,"; } }] = (order += "3");
    ok(order === "1,2,3", "array expression order = " + order);
})();

var re = /=(\?|%3F)/g;
ok(re.source === "=(\\?|%3F)", "re.source = " + re.source);

tmp = new Array();
for(var i=0; i<2; i++)
    tmp[i] = /b/;
ok(tmp[0] != tmp[1], "tmp[0] == tmp [1]");

ok(isNullBSTR(getNullBSTR()), "isNullBSTR(getNullBSTR()) failed\n");
ok(getNullBSTR() === '', "getNullBSTR() !== ''");
ok(+getNullBSTR() === 0 , "+getNullBTR() !=== 0");

ok(getVT(nullDisp) === "VT_DISPATCH", "getVT(nullDisp) = " + getVT(nullDisp));
ok(typeof(nullDisp) === "object", "typeof(nullDisp) = " + typeof(nullDisp));
ok(nullDisp === nullDisp, "nullDisp !== nullDisp");
ok(nullDisp !== re, "nullDisp === re");
ok(nullDisp === null, "nullDisp === null");
ok(nullDisp == null, "nullDisp == null");
ok(getVT(true && nullDisp) === "VT_DISPATCH",
   "getVT(0 && nullDisp) = " + getVT(true && nullDisp));
ok(!nullDisp === true, "!nullDisp = " + !nullDisp);
ok(String(nullDisp) === "null", "String(nullDisp) = " + String(nullDisp));
ok(+nullDisp === 0, "+nullDisp !== 0");
ok(''+nullDisp === "null", "''+nullDisp !== null");
ok(nullDisp != new Object(), "nullDisp == new Object()");
ok(new Object() != nullDisp, "new Object() == nullDisp");
ok((typeof Object(nullDisp)) === "object", "typeof Object(nullDisp) !== 'object'");
tmp = getVT(Object(nullDisp));
ok(tmp === "VT_DISPATCH", "getVT(Object(nullDisp) = " + tmp);
tmp = Object(nullDisp).toString();
ok(tmp === "[object Object]", "Object(nullDisp).toString() = " + tmp);

function testNullPrototype() {
    this.x = 13;
}
tmp = new testNullPrototype();
ok(tmp.x === 13, "tmp.x !== 13");
ok(!("y" in tmp), "tmp has 'y' property");
testNullPrototype.prototype.y = 10;
ok("y" in tmp, "tmp does not have 'y' property");
tmp = new testNullPrototype();
ok(tmp.y === 10, "tmp.y !== 10");
testNullPrototype.prototype = nullDisp;
tmp = new testNullPrototype();
ok(tmp.x === 13, "tmp.x !== 13");
ok(!("y" in tmp), "tmp has 'y' property");
ok(!tmp.hasOwnProperty("y"), "tmp has 'y' property");
ok(!tmp.propertyIsEnumerable("y"), "tmp has 'y' property enumerable");
ok(tmp.toString() == "[object Object]", "tmp.toString returned " + tmp.toString());
testNullPrototype.prototype = null;
tmp = new testNullPrototype();
ok(!tmp.hasOwnProperty("y"), "tmp has 'y' property");
ok(!tmp.propertyIsEnumerable("y"), "tmp has 'y' property enumerable");
ok(tmp.toString() == "[object Object]", "tmp.toString returned " + tmp.toString());

testNullPrototype.prototype = 42;
tmp = new testNullPrototype();
ok(tmp.hasOwnProperty("x"), "tmp does not have 'x' property");
ok(!tmp.hasOwnProperty("y"), "tmp has 'y' property");
ok(tmp.toString() == "[object Object]", "tmp.toString returned " + tmp.toString());

testNullPrototype.prototype = true;
tmp = new testNullPrototype();
ok(tmp.hasOwnProperty("x"), "tmp does not have 'x' property");
ok(!tmp.hasOwnProperty("y"), "tmp has 'y' property");
ok(tmp.toString() == "[object Object]", "tmp.toString returned " + tmp.toString());

testNullPrototype.prototype = "foobar";
tmp = new testNullPrototype();
ok(tmp.hasOwnProperty("x"), "tmp does not have 'x' property");
ok(!tmp.hasOwnProperty("y"), "tmp has 'y' property");
ok(tmp.toString() == "[object Object]", "tmp.toString returned " + tmp.toString());

function do_test() {}
function nosemicolon() {} nosemicolon();
function () {} nosemicolon();

if(false) {
    function in_if_false() { return true; } ok(false, "!?");
}

ok(in_if_false(), "in_if_false failed");

(function() { newValue = 1; })();
ok(newValue === 1, "newValue = " + newValue);

obj = {undefined: 3};

ok(typeof(name_override_func) === "function", "typeof(name_override_func) = " + typeof(name_override_func));
name_override_func = 3;
ok(name_override_func === 3, "name_override_func = " + name_override_func);
function name_override_func() {};
ok(name_override_func === 3, "name_override_func = " + name_override_func);

tmp = (function() {
    var ret = false;
    with({ret: true})
        return ret;
})();
ok(tmp, "tmp = " + tmp);

tmp = (function() {
    for(var iter in [1,2,3,4]) {
        var ret = false;
        with({ret: true})
            return ret;
    }
})();
ok(tmp, "tmp = " + tmp);

(function() {
    ok(typeof(func) === "function", "typeof(func)  = " + typeof(func));
    with(new Object()) {
        var x = false && function func() {};
    }
    ok(typeof(func) === "function", "typeof(func)  = " + typeof(func));
})();

(function() {
    ok(x === undefined, "x = " + x); // x is declared, but never initialized
    with({x: 1}) {
        ok(x === 1, "x = " + x);
        var x = 2;
        ok(x === 2, "x = " + x);
    }
    ok(x === undefined, "x = " + x);
})();

var get, set;

/* NoNewline rule parser tests */
while(true) {
    if(true) break
    tmp = false
}

while(true) {
    if(true) break /*
                    * no semicolon, but comment present */
    tmp = false
}

while(true) {
    if(true) break // no semicolon, but comment present
    tmp = false
}

while(true) {
    break
    continue
    tmp = false
}

function returnTest() {
    return
    true;
}

ok(returnTest() === undefined, "returnTest = " + returnTest());

ActiveXObject = 1;
ok(ActiveXObject === 1, "ActiveXObject = " + ActiveXObject);

Boolean = 1;
ok(Boolean === 1, "Boolean = " + Boolean);

Object = 1;
ok(Object === 1, "Object = " + Object);

Array = 1;
ok(Array === 1, "Array = " + Array);

Date = 1;
ok(Date === 1, "Date = " + Date);

Error = 1;
ok(Error === 1, "Error = " + Error);

/* Keep this test in the end of file */
undefined = 6;
ok(undefined === 6, "undefined = " + undefined);

NaN = 6;
ok(NaN === 6, "NaN !== 6");

Infinity = 6;
ok(Infinity === 6, "Infinity !== 6");

Math = 6;
ok(Math === 6, "NaN !== 6");

reportSuccess();

function test_es5_keywords() {
    var let = 1
    var tmp
    ok(let == 1, "let != 1");

    tmp = false
    try {
        eval('var var = 1;');
    }
    catch(e) {
        tmp = true
    }
    ok(tmp === true, "Expected exception for 'var var = 1;'");

    tmp = false
    try {
        eval('var const = 1;');
    }
    catch(e) {
        tmp = true
    }
    ok(tmp === true, "Expected exception for 'var const = 1;'");

    tmp = false
    try {
        eval('const c1 = 1;');
    }
    catch(e) {
        tmp = true
    }
    ok(tmp === true, "Expected exception for 'const c1 = 1;'");
}
test_es5_keywords();

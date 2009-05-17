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

var tmp, i;

i = parseInt("0");
ok(i === 0, "parseInt('0') = " + i);
i = parseInt("123");
ok(i === 123, "parseInt('123') = " + i);
i = parseInt("-123");
ok(i === -123, "parseInt('-123') = " + i);
i = parseInt("0xff");
ok(i === 0xff, "parseInt('0xff') = " + i);
i = parseInt("11", 8);
ok(i === 9, "parseInt('11', 8) = " + i);
i = parseInt("1j", 22);
ok(i === 41, "parseInt('1j', 32) = " + i);
i = parseInt("123", 0);
ok(i === 123, "parseInt('123', 0) = " + i);
i = parseInt("123", 10, "test");
ok(i === 123, "parseInt('123', 10, 'test') = " + i);
i = parseInt("11", "8");
ok(i === 9, "parseInt('11', '8') = " + i);

tmp = encodeURI("abc");
ok(tmp === "abc", "encodeURI('abc') = " + tmp);
tmp = encodeURI("{abc}");
ok(tmp === "%7Babc%7D", "encodeURI('{abc}') = " + tmp);
tmp = encodeURI("");
ok(tmp === "", "encodeURI('') = " + tmp);
tmp = encodeURI("\01\02\03\04");
ok(tmp === "%01%02%03%04", "encodeURI('\\01\\02\\03\\04') = " + tmp);
tmp = encodeURI("{#@}");
ok(tmp === "%7B#@%7D", "encodeURI('{#@}') = " + tmp);
tmp = encodeURI("\xa1 ");
ok(tmp === "%C2%A1%20", "encodeURI(\\xa1 ) = " + tmp);
tmp = encodeURI("\xffff");
ok(tmp.length === 8, "encodeURI('\\xffff').length = " + tmp.length);
tmp = encodeURI("abcABC123;/?:@&=+$,-_.!~*'()");
ok(tmp === "abcABC123;/?:@&=+$,-_.!~*'()", "encodeURI('abcABC123;/?:@&=+$,-_.!~*'()') = " + tmp);
tmp = encodeURI();
ok(tmp === "undefined", "encodeURI() = " + tmp);
tmp = encodeURI("abc", "test");
ok(tmp === "abc", "encodeURI('abc') = " + tmp);

tmp = "" + new Object();
ok(tmp === "[object Object]", "'' + new Object() = " + tmp);

ok("".length === 0, "\"\".length = " + "".length);
ok(getVT("".length) == "VT_I4", "\"\".length = " + "".length);
ok("abc".length === 3, "\"abc\".length = " + "abc".length);
ok(String.prototype.length === 0, "String.prototype.length = " + String.prototype.length);

tmp = "".toString();
ok(tmp === "", "''.toString() = " + tmp);
tmp = "test".toString();
ok(tmp === "test", "''.toString() = " + tmp);
tmp = "test".toString(3);
ok(tmp === "test", "''.toString(3) = " + tmp);

tmp = "".valueOf();
ok(tmp === "", "''.valueOf() = " + tmp);
tmp = "test".valueOf();
ok(tmp === "test", "''.valueOf() = " + tmp);
tmp = "test".valueOf(3);
ok(tmp === "test", "''.valueOf(3) = " + tmp);

var str = new String("test");
ok(str.toString() === "test", "str.toString() = " + str.toString());
var str = new String();
ok(str.toString() === "", "str.toString() = " + str.toString());
var str = new String("test", "abc");
ok(str.toString() === "test", "str.toString() = " + str.toString());

tmp = "value " + str;
ok(tmp === "value test", "'value ' + str = " + tmp);

tmp = String();
ok(tmp === "", "String() = " + tmp);
tmp = String(false);
ok(tmp === "false", "String(false) = " + tmp);
tmp = String(null);
ok(tmp === "null", "String(null) = " + tmp);
tmp = String("test");
ok(tmp === "test", "String('test') = " + tmp);
tmp = String("test", "abc");
ok(tmp === "test", "String('test','abc') = " + tmp);

tmp = "abc".charAt(0);
ok(tmp === "a", "'abc',charAt(0) = " + tmp);
tmp = "abc".charAt(1);
ok(tmp === "b", "'abc',charAt(1) = " + tmp);
tmp = "abc".charAt(2);
ok(tmp === "c", "'abc',charAt(2) = " + tmp);
tmp = "abc".charAt(3);
ok(tmp === "", "'abc',charAt(3) = " + tmp);
tmp = "abc".charAt(4);
ok(tmp === "", "'abc',charAt(4) = " + tmp);
tmp = "abc".charAt();
ok(tmp === "a", "'abc',charAt() = " + tmp);
tmp = "abc".charAt(-1);
ok(tmp === "", "'abc',charAt(-1) = " + tmp);
tmp = "abc".charAt(0,2);
ok(tmp === "a", "'abc',charAt(0.2) = " + tmp);

tmp = "abc".charCodeAt(0);
ok(tmp === 0x61, "'abc'.charCodeAt(0) = " + tmp);
tmp = "abc".charCodeAt(1);
ok(tmp === 0x62, "'abc'.charCodeAt(1) = " + tmp);
tmp = "abc".charCodeAt(2);
ok(tmp === 0x63, "'abc'.charCodeAt(2) = " + tmp);
tmp = "abc".charCodeAt();
ok(tmp === 0x61, "'abc'.charCodeAt() = " + tmp);
tmp = "abc".charCodeAt(true);
ok(tmp === 0x62, "'abc'.charCodeAt(true) = " + tmp);
tmp = "abc".charCodeAt(0,2);
ok(tmp === 0x61, "'abc'.charCodeAt(0,2) = " + tmp);

tmp = "abcd".substring(1,3);
ok(tmp === "bc", "'abcd'.substring(1,3) = " + tmp);
tmp = "abcd".substring(-1,3);
ok(tmp === "abc", "'abcd'.substring(-1,3) = " + tmp);
tmp = "abcd".substring(1,6);
ok(tmp === "bcd", "'abcd'.substring(1,6) = " + tmp);
tmp = "abcd".substring(3,1);
ok(tmp === "bc", "'abcd'.substring(3,1) = " + tmp);
tmp = "abcd".substring(2,2);
ok(tmp === "", "'abcd'.substring(2,2) = " + tmp);
tmp = "abcd".substring(true,"3");
ok(tmp === "bc", "'abcd'.substring(true,'3') = " + tmp);
tmp = "abcd".substring(1,3,2);
ok(tmp === "bc", "'abcd'.substring(1,3,2) = " + tmp);
tmp = "abcd".substring();
ok(tmp === "abcd", "'abcd'.substring() = " + tmp);

tmp = "abcd".slice(1,3);
ok(tmp === "bc", "'abcd'.slice(1,3) = " + tmp);
tmp = "abcd".slice(1,-1);
ok(tmp === "bc", "'abcd'.slice(1,-1) = " + tmp);
tmp = "abcd".slice(-3,3);
ok(tmp === "bc", "'abcd'.slice(-3,3) = " + tmp);
tmp = "abcd".slice(-6,3);
ok(tmp === "abc", "'abcd'.slice(-6,3) = " + tmp);
tmp = "abcd".slice(3,1);
ok(tmp === "", "'abcd'.slice(3,1) = " + tmp);
tmp = "abcd".slice(true,3);
ok(tmp === "bc", "'abcd'.slice(true,3) = " + tmp);
tmp = "abcd".slice();
ok(tmp === "abcd", "'abcd'.slice() = " + tmp);
tmp = "abcd".slice(1);
ok(tmp === "bcd", "'abcd'.slice(1) = " + tmp);

tmp = "abc".concat(["d",1],2,false);
ok(tmp === "abcd,12false", "concat returned " + tmp);
var arr = new Array(2,"a");
arr.concat = String.prototype.concat;
tmp = arr.concat("d");
ok(tmp === "2,ad", "arr.concat = " + tmp);

m = "a+bcabc".match("a+");
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "a", "m[0] is not \"ab\"");

r = "- [test] -".replace("[test]", "success");
ok(r === "- success -", "r = " + r + " expected '- success -'");

r = "- [test] -".replace("[test]", "success", "test");
ok(r === "- success -", "r = " + r + " expected '- success -'");

r = "test".replace();
ok(r === "test", "r = " + r + " expected 'test'");

function replaceFunc3(m, off, str) {
    ok(arguments.length === 3, "arguments.length = " + arguments.length);
    ok(m === "[test]", "m = " + m + " expected [test1]");
    ok(off === 1, "off = " + off + " expected 0");
    ok(str === "-[test]-", "str = " + arguments[3]);
    return "ret";
}

r = "-[test]-".replace("[test]", replaceFunc3);
ok(r === "-ret-", "r = " + r + " expected '-ret-'");

r = "-[test]-".replace("[test]", replaceFunc3, "test");
ok(r === "-ret-", "r = " + r + " expected '-ret-'");

r = "1,2,3".split(",");
ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);


r = "1,2,3".split(",*");
ok(r.length === 1, "r.length = " + r.length);
ok(r[0] === "1,2,3", "r[0] = " + r[0]);

r = "123".split("");
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);

r = "123".split(2);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "3", "r[1] = " + r[1]);

r = "1,2,".split(",");
ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "", "r[2] = " + r[2]);

tmp = "abcd".indexOf("bc",0);
ok(tmp === 1, "indexOf = " + tmp);
tmp = "abcd".indexOf("bc",1);
ok(tmp === 1, "indexOf = " + tmp);
tmp = "abcd".indexOf("bc");
ok(tmp === 1, "indexOf = " + tmp);
tmp = "abcd".indexOf("ac");
ok(tmp === -1, "indexOf = " + tmp);
tmp = "abcd".indexOf("bc",2);
ok(tmp === -1, "indexOf = " + tmp);
tmp = "abcd".indexOf("a",0);
ok(tmp === 0, "indexOf = " + tmp);
tmp = "abcd".indexOf("bc",0,"test");
ok(tmp === 1, "indexOf = " + tmp);
tmp = "abcd".indexOf();
ok(tmp == -1, "indexOf = " + tmp);

tmp = "".toLowerCase();
ok(tmp === "", "''.toLowerCase() = " + tmp);
tmp = "test".toLowerCase();
ok(tmp === "test", "''.toLowerCase() = " + tmp);
tmp = "test".toLowerCase(3);
ok(tmp === "test", "''.toLowerCase(3) = " + tmp);
tmp = "tEsT".toLowerCase();
ok(tmp === "test", "''.toLowerCase() = " + tmp);
tmp = "tEsT".toLowerCase(3);
ok(tmp === "test", "''.toLowerCase(3) = " + tmp);

tmp = "".toUpperCase();
ok(tmp === "", "''.toUpperCase() = " + tmp);
tmp = "TEST".toUpperCase();
ok(tmp === "TEST", "''.toUpperCase() = " + tmp);
tmp = "TEST".toUpperCase(3);
ok(tmp === "TEST", "''.toUpperCase(3) = " + tmp);
tmp = "tEsT".toUpperCase();
ok(tmp === "TEST", "''.toUpperCase() = " + tmp);
tmp = "tEsT".toUpperCase(3);
ok(tmp === "TEST", "''.toUpperCase(3) = " + tmp);

tmp = "".big();
ok(tmp === "<BIG></BIG>", "''.big() = " + tmp);
tmp = "".big(3);
ok(tmp === "<BIG></BIG>", "''.big(3) = " + tmp);
tmp = "test".big();
ok(tmp === "<BIG>test</BIG>", "'test'.big() = " + tmp);
tmp = "test".big(3);
ok(tmp === "<BIG>test</BIG>", "'test'.big(3) = " + tmp);

tmp = "".blink();
ok(tmp === "<BLINK></BLINK>", "''.blink() = " + tmp);
tmp = "".blink(3);
ok(tmp === "<BLINK></BLINK>", "''.blink(3) = " + tmp);
tmp = "test".blink();
ok(tmp === "<BLINK>test</BLINK>", "'test'.blink() = " + tmp);
tmp = "test".blink(3);
ok(tmp === "<BLINK>test</BLINK>", "'test'.blink(3) = " + tmp);

tmp = "".bold();
ok(tmp === "<B></B>", "''.bold() = " + tmp);
tmp = "".bold(3);
ok(tmp === "<B></B>", "''.bold(3) = " + tmp);
tmp = "test".bold();
ok(tmp === "<B>test</B>", "'test'.bold() = " + tmp);
tmp = "test".bold(3);
ok(tmp === "<B>test</B>", "'test'.bold(3) = " + tmp);

tmp = "".fixed();
ok(tmp === "<TT></TT>", "''.fixed() = " + tmp);
tmp = "".fixed(3);
ok(tmp === "<TT></TT>", "''.fixed(3) = " + tmp);
tmp = "test".fixed();
ok(tmp === "<TT>test</TT>", "'test'.fixed() = " + tmp);
tmp = "test".fixed(3);
ok(tmp === "<TT>test</TT>", "'test'.fixed(3) = " + tmp);

tmp = "".italics();
ok(tmp === "<I></I>", "''.italics() = " + tmp);
tmp = "".italics(3);
ok(tmp === "<I></I>", "''.italics(3) = " + tmp);
tmp = "test".italics();
ok(tmp === "<I>test</I>", "'test'.italics() = " + tmp);
tmp = "test".italics(3);
ok(tmp === "<I>test</I>", "'test'.italics(3) = " + tmp);

tmp = "".small();
ok(tmp === "<SMALL></SMALL>", "''.small() = " + tmp);
tmp = "".small(3);
ok(tmp === "<SMALL></SMALL>", "''.small(3) = " + tmp);
tmp = "test".small();
ok(tmp === "<SMALL>test</SMALL>", "'test'.small() = " + tmp);
tmp = "test".small(3);
ok(tmp === "<SMALL>test</SMALL>", "'test'.small(3) = " + tmp);

tmp = "".strike();
ok(tmp === "<STRIKE></STRIKE>", "''.strike() = " + tmp);
tmp = "".strike(3);
ok(tmp === "<STRIKE></STRIKE>", "''.strike(3) = " + tmp);
tmp = "test".strike();
ok(tmp === "<STRIKE>test</STRIKE>", "'test'.strike() = " + tmp);
tmp = "test".strike(3);
ok(tmp === "<STRIKE>test</STRIKE>", "'test'.strike(3) = " + tmp);

tmp = "".sub();
ok(tmp === "<SUB></SUB>", "''.sub() = " + tmp);
tmp = "".sub(3);
ok(tmp === "<SUB></SUB>", "''.sub(3) = " + tmp);
tmp = "test".sub();
ok(tmp === "<SUB>test</SUB>", "'test'.sub() = " + tmp);
tmp = "test".sub(3);
ok(tmp === "<SUB>test</SUB>", "'test'.sub(3) = " + tmp);

tmp = "".sup();
ok(tmp === "<SUP></SUP>", "''.sup() = " + tmp);
tmp = "".sup(3);
ok(tmp === "<SUP></SUP>", "''.sup(3) = " + tmp);
tmp = "test".sup();
ok(tmp === "<SUP>test</SUP>", "'test'.sup() = " + tmp);
tmp = "test".sup(3);
ok(tmp === "<SUP>test</SUP>", "'test'.sup(3) = " + tmp);

var arr = new Array();
ok(typeof(arr) === "object", "arr () is not object");
ok((arr.length === 0), "arr.length is not 0");
ok(arr["0"] === undefined, "arr[0] is not undefined");

var arr = new Array(1, 2, "test");
ok(typeof(arr) === "object", "arr (1,2,test) is not object");
ok((arr.length === 3), "arr.length is not 3");
ok(arr["0"] === 1, "arr[0] is not 1");
ok(arr["1"] === 2, "arr[1] is not 2");
ok(arr["2"] === "test", "arr[2] is not \"test\"");

arr["7"] = true;
ok((arr.length === 8), "arr.length is not 8");

tmp = "" + [];
ok(tmp === "", "'' + [] = " + tmp);
tmp = "" + [1,true];
ok(tmp === "1,true", "'' + [1,true] = " + tmp);

var arr = new Array(6);
ok(typeof(arr) === "object", "arr (6) is not object");
ok((arr.length === 6), "arr.length is not 6");
ok(arr["0"] === undefined, "arr[0] is not undefined");

ok(arr.push() === 6, "arr.push() !== 6");
ok(arr.push(1) === 7, "arr.push(1) !== 7");
ok(arr[6] === 1, "arr[6] != 1");
ok(arr.length === 7, "arr.length != 10");
ok(arr.push(true, 'b', false) === 10, "arr.push(true, 'b', false) !== 10");
ok(arr[8] === "b", "arr[8] != 'b'");
ok(arr.length === 10, "arr.length != 10");

arr = [3,4,5];
tmp = arr.pop();
ok(arr.length === 2, "arr.length = " + arr.length);
ok(tmp === 5, "pop() = " + tmp);
tmp = arr.pop(2);
ok(arr.length === 1, "arr.length = " + arr.length);
ok(tmp === 4, "pop() = " + tmp);
tmp = arr.pop();
ok(arr.length === 0, "arr.length = " + arr.length);
ok(tmp === 3, "pop() = " + tmp);
for(tmp in arr)
    ok(false, "not deleted " + tmp);
tmp = arr.pop();
ok(arr.length === 0, "arr.length = " + arr.length);
ok(tmp === undefined, "tmp = " + tmp);
arr = [,,,,,];
tmp = arr.pop();
ok(arr.length === 5, "arr.length = " + arr.length);
ok(tmp === undefined, "tmp = " + tmp);

arr = [1,2,null,false,undefined,,"a"];

tmp = arr.join();
ok(tmp === "1,2,,false,,,a", "arr.join() = " + tmp);
tmp = arr.join(";");
ok(tmp === "1;2;;false;;;a", "arr.join(';') = " + tmp);
tmp = arr.join(";","test");
ok(tmp === "1;2;;false;;;a", "arr.join(';') = " + tmp);
tmp = arr.join("");
ok(tmp === "12falsea", "arr.join('') = " + tmp);

tmp = arr.toString();
ok(tmp === "1,2,,false,,,a", "arr.toString() = " + tmp);
tmp = arr.toString("test");
ok(tmp === "1,2,,false,,,a", "arr.toString() = " + tmp);

arr = [5,true,2,-1,3,false,"2.5"];
tmp = arr.sort(function(x,y) { return y-x; });
ok(tmp === arr, "tmp !== arr");
tmp = [5,3,"2.5",2,true,false,-1];
for(var i=0; i < arr.length; i++)
    ok(arr[i] === tmp[i], "arr[" + i + "] = " + arr[i] + " expected " + tmp[i]);

arr = [5,false,2,0,"abc",3,"a",-1];
tmp = arr.sort();
ok(tmp === arr, "tmp !== arr");
tmp = [-1,0,2,3,5,"a","abc",false];
for(var i=0; i < arr.length; i++)
    ok(arr[i] === tmp[i], "arr[" + i + "] = " + arr[i] + " expected " + tmp[i]);

arr = ["a", "b", "ab"];
tmp = ["a", "ab", "b"];
ok(arr.sort() === arr, "arr.sort() !== arr");
for(var i=0; i < arr.length; i++)
    ok(arr[i] === tmp[i], "arr[" + i + "] = " + arr[i] + " expected " + tmp[i]);

var num = new Number(6);
arr = [0,1,2];
tmp = arr.concat(3, [4,5], num);
ok(tmp !== arr, "tmp === arr");
for(var i=0; i<6; i++)
    ok(tmp[i] === i, "tmp[" + i + "] = " + tmp[i]);
ok(tmp[6] === num, "tmp[6] !== num");
ok(tmp.length === 7, "tmp.length = " + tmp.length);

arr = [].concat();
ok(arr.length === 0, "arr.length = " + arr.length);

arr = [1,];
tmp = arr.concat([2]);
ok(tmp.length === 3, "tmp.length = " + tmp.length);
ok(tmp[1] === undefined, "tmp[1] = " + tmp[1]);

var num = new Number(2);
ok(num.toString() === "2", "num(2).toString !== 2");
var num = new Number();
ok(num.toString() === "0", "num().toString !== 0");

ok(Number() === 0, "Number() = " + Number());
ok(Number(false) === 0, "Number(false) = " + Number(false));
ok(Number("43") === 43, "Number('43') = " + Number("43"));

tmp = (new Number(1)).valueOf();
ok(tmp === 1, "(new Number(1)).valueOf = " + tmp);
tmp = (new Number(1,2)).valueOf();
ok(tmp === 1, "(new Number(1,2)).valueOf = " + tmp);
tmp = (new Number()).valueOf();
ok(tmp === 0, "(new Number()).valueOf = " + tmp);
tmp = Number.prototype.valueOf();
ok(tmp === 0, "Number.prototype.valueOf = " + tmp);

tmp = Math.min(1);
ok(tmp === 1, "Math.min(1) = " + tmp);

tmp = Math.min(1, false);
ok(tmp === 0, "Math.min(1, false) = " + tmp);

tmp = Math.min();
ok(tmp === Infinity, "Math.min() = " + tmp);

tmp = Math.min(1, NaN, -Infinity, false);
ok(isNaN(tmp), "Math.min(1, NaN, -Infinity, false) is not NaN");

tmp = Math.min(1, false, true, null, -3);
ok(tmp === -3, "Math.min(1, false, true, null, -3) = " + tmp);

tmp = Math.max(1);
ok(tmp === 1, "Math.max(1) = " + tmp);

tmp = Math.max(true, 0);
ok(tmp === 1, "Math.max(true, 0) = " + tmp);

tmp = Math.max(-2, false, true, null, 1);
ok(tmp === 1, "Math.max(-2, false, true, null, 1) = " + tmp);

tmp = Math.max();
ok(tmp === -Infinity, "Math.max() = " + tmp);

tmp = Math.max(true, NaN, 0);
ok(isNaN(tmp), "Math.max(true, NaN, 0) is not NaN");

tmp = Math.round(0.5);
ok(tmp === 1, "Math.round(0.5) = " + tmp);

tmp = Math.round(-0.5);
ok(tmp === 0, "Math.round(-0.5) = " + tmp);

tmp = Math.round(1.1);
ok(tmp === 1, "Math.round(1.1) = " + tmp);

tmp = Math.round(true);
ok(tmp === 1, "Math.round(true) = " + tmp);

tmp = Math.round(1.1, 3, 4);
ok(tmp === 1, "Math.round(1.1, 3, 4) = " + tmp);

tmp = Math.round();
ok(isNaN(tmp), "Math.round() is not NaN");

tmp = Math.ceil(0.5);
ok(tmp === 1, "Math.ceil(0.5) = " + tmp);

tmp = Math.ceil(-0.5);
ok(tmp === 0, "Math.ceil(-0.5) = " + tmp);

tmp = Math.ceil(1.1);
ok(tmp === 2, "Math.round(1.1) = " + tmp);

tmp = Math.ceil(true);
ok(tmp === 1, "Math.ceil(true) = " + tmp);

tmp = Math.ceil(1.1, 3, 4);
ok(tmp === 2, "Math.ceil(1.1, 3, 4) = " + tmp);

tmp = Math.ceil();
ok(isNaN(tmp), "ceil() is not NaN");

tmp = Math.floor(0.5);
ok(tmp === 0, "Math.floor(0.5) = " + tmp);

tmp = Math.floor(-0.5);
ok(tmp === -1, "Math.floor(-0.5) = " + tmp);

tmp = Math.floor(1.1);
ok(tmp === 1, "Math.floor(1.1) = " + tmp);

tmp = Math.floor(true);
ok(tmp === 1, "Math.floor(true) = " + tmp);

tmp = Math.floor(1.1, 3, 4);
ok(tmp === 1, "Math.floor(1.1, 3, 4) = " + tmp);

tmp = Math.floor();
ok(isNaN(tmp), "floor is not NaN");

tmp = Math.abs(3);
ok(tmp === 3, "Math.abs(3) = " + tmp);

tmp = Math.abs(-3);
ok(tmp === 3, "Math.abs(-3) = " + tmp);

tmp = Math.abs(true);
ok(tmp === 1, "Math.abs(true) = " + tmp);

tmp = Math.abs();
ok(isNaN(tmp), "Math.abs() is not NaN");

tmp = Math.abs(NaN);
ok(isNaN(tmp), "Math.abs() is not NaN");

tmp = Math.abs(-Infinity);
ok(tmp === Infinity, "Math.abs(-Infinite) = " + tmp);

tmp = Math.abs(-3, 2);
ok(tmp === 3, "Math.abs(-3, 2) = " + tmp);

tmp = Math.cos(0);
ok(tmp === 1, "Math.cos(0) = " + tmp);

tmp = Math.cos(Math.PI/2);
ok(Math.floor(tmp*100) === 0, "Math.cos(Math.PI/2) = " + tmp);

tmp = Math.cos(-Math.PI/2);
ok(Math.floor(tmp*100) === 0, "Math.cos(-Math.PI/2) = " + tmp);

tmp = Math.cos(Math.PI/3, 2);
ok(Math.floor(tmp*100) === 50, "Math.cos(Math.PI/3, 2) = " + tmp);

tmp = Math.cos(true);
ok(Math.floor(tmp*100) === 54, "Math.cos(true) = " + tmp);

tmp = Math.cos(false);
ok(tmp === 1, "Math.cos(false) = " + tmp);

tmp = Math.cos();
ok(isNaN(tmp), "Math.cos() is not NaN");

tmp = Math.cos(NaN);
ok(isNaN(tmp), "Math.cos(NaN) is not NaN");

tmp = Math.cos(Infinity);
ok(isNaN(tmp), "Math.cos(Infinity) is not NaN");

tmp = Math.cos(-Infinity);
ok(isNaN(tmp), "Math.cos(-Infinity) is not NaN");

tmp = Math.pow(2, 2);
ok(tmp === 4, "Math.pow(2, 2) = " + tmp);

tmp = Math.pow(4, 0.5);
ok(tmp === 2, "Math.pow(2, 2) = " + tmp);

tmp = Math.pow(2, 2, 3);
ok(tmp === 4, "Math.pow(2, 2, 3) = " + tmp);

tmp = Math.random();
ok(typeof(tmp) == "number", "typeof(tmp) = " + typeof(tmp));
ok(0 <= tmp && tmp <= 1, "Math.random() = " + tmp);

tmp = Math.random(100);
ok(typeof(tmp) == "number", "typeof(tmp) = " + typeof(tmp));
ok(0 <= tmp && tmp <= 1, "Math.random(100) = " + tmp);

var func = function  (a) {
        var a = 1;
        if(a) return;
    }.toString();
ok(func.toString() === "function  (a) {\n        var a = 1;\n        if(a) return;\n    }",
   "func.toString() = " + func.toString());
ok("" + func === "function  (a) {\n        var a = 1;\n        if(a) return;\n    }",
   "'' + func.toString() = " + func);

function testFuncToString(x,y) {
    return x+y;
}

ok(testFuncToString.toString() === "function testFuncToString(x,y) {\n    return x+y;\n}",
   "testFuncToString.toString() = " + testFuncToString.toString());
ok("" + testFuncToString === "function testFuncToString(x,y) {\n    return x+y;\n}",
   "'' + testFuncToString = " + testFuncToString);

var date = new Date();

date = new Date(100);
ok(date.getTime() === 100, "date.getTime() = " + date.getTime());
ok(Date.prototype.getTime() === 0, "date.prototype.getTime() = " + Date.prototype.getTime());

ok(typeof(Math.PI) === "number", "typeof(Math.PI) = " + typeof(Math.PI));
ok(Math.floor(Math.PI*100) === 314, "Math.PI = " + Math.PI);
Math.PI = "test";
ok(Math.floor(Math.PI*100) === 314, "modified Math.PI = " + Math.PI);

ok(typeof(Math.E) === "number", "typeof(Math.E) = " + typeof(Math.E));
ok(Math.floor(Math.E*100) === 271, "Math.E = " + Math.E);
Math.E = "test";
ok(Math.floor(Math.E*100) === 271, "modified Math.E = " + Math.E);

ok(typeof(Math.LOG2E) === "number", "typeof(Math.LOG2E) = " + typeof(Math.LOG2E));
ok(Math.floor(Math.LOG2E*100) === 144, "Math.LOG2E = " + Math.LOG2E);
Math.LOG2E = "test";
ok(Math.floor(Math.LOG2E*100) === 144, "modified Math.LOG2E = " + Math.LOG2E);

ok(typeof(Math.LOG10E) === "number", "typeof(Math.LOG10E) = " + typeof(Math.LOG10E));
ok(Math.floor(Math.LOG10E*100) === 43, "Math.LOG10E = " + Math.LOG10E);
Math.LOG10E = "test";
ok(Math.floor(Math.LOG10E*100) === 43, "modified Math.LOG10E = " + Math.LOG10E);

ok(typeof(Math.LN2) === "number", "typeof(Math.LN2) = " + typeof(Math.LN2));
ok(Math.floor(Math.LN2*100) === 69, "Math.LN2 = " + Math.LN2);
Math.LN2 = "test";
ok(Math.floor(Math.LN2*100) === 69, "modified Math.LN2 = " + Math.LN2);

reportSuccess();

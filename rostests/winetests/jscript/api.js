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
(tmp = new Array).f = Object.prototype.toString;
ok(tmp.f() === "[object Array]", "tmp.f() = " + tmp.f());
(tmp = new Boolean).f = Object.prototype.toString;
ok(tmp.f() === "[object Boolean]", "tmp.f() = " + tmp.f());
(tmp = new Date).f = Object.prototype.toString;
ok(tmp.f() === "[object Date]", "tmp.f() = " + tmp.f());
(tmp = function() {}).f = Object.prototype.toString;
ok(tmp.f() === "[object Function]", "tmp.f() = " + tmp.f());
Math.f = Object.prototype.toString;
ok(Math.f() === "[object Math]", "tmp.f() = " + tmp.f());
(tmp = new Number).f = Object.prototype.toString;
ok(tmp.f() === "[object Number]", "tmp.f() = " + tmp.f());
(tmp = new RegExp("")).f = Object.prototype.toString;
ok(tmp.f() === "[object RegExp]", "tmp.f() = " + tmp.f());
(tmp = new String).f = Object.prototype.toString;
ok(tmp.f() === "[object String]", "tmp.f() = " + tmp.f());

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

tmp = "".anchor();
ok(tmp === "<A NAME=\"undefined\"></A>", "''.anchor() = " + tmp);
tmp = "".anchor(3);
ok(tmp === "<A NAME=\"3\"></A>", "''.anchor(3) = " + tmp);
tmp = "".anchor("red");
ok(tmp === "<A NAME=\"red\"></A>", "''.anchor('red') = " + tmp);
tmp = "test".anchor();
ok(tmp === "<A NAME=\"undefined\">test</A>", "'test'.anchor() = " + tmp);
tmp = "test".anchor(3);
ok(tmp === "<A NAME=\"3\">test</A>", "'test'.anchor(3) = " + tmp);
tmp = "test".anchor("green");
ok(tmp === "<A NAME=\"green\">test</A>", "'test'.anchor('green') = " + tmp);

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

tmp = "".fontcolor();
ok(tmp === "<FONT COLOR=\"undefined\"></FONT>", "''.fontcolor() = " + tmp);
tmp = "".fontcolor(3);
ok(tmp === "<FONT COLOR=\"3\"></FONT>", "''.fontcolor(3) = " + tmp);
tmp = "".fontcolor("red");
ok(tmp === "<FONT COLOR=\"red\"></FONT>", "''.fontcolor('red') = " + tmp);
tmp = "test".fontcolor();
ok(tmp === "<FONT COLOR=\"undefined\">test</FONT>", "'test'.fontcolor() = " + tmp);
tmp = "test".fontcolor(3);
ok(tmp === "<FONT COLOR=\"3\">test</FONT>", "'test'.fontcolor(3) = " + tmp);
tmp = "test".fontcolor("green");
ok(tmp === "<FONT COLOR=\"green\">test</FONT>", "'test'.fontcolor('green') = " + tmp);

tmp = "".fontsize();
ok(tmp === "<FONT SIZE=\"undefined\"></FONT>", "''.fontsize() = " + tmp);
tmp = "".fontsize(3);
ok(tmp === "<FONT SIZE=\"3\"></FONT>", "''.fontsize(3) = " + tmp);
tmp = "".fontsize("red");
ok(tmp === "<FONT SIZE=\"red\"></FONT>", "''.fontsize('red') = " + tmp);
tmp = "test".fontsize();
ok(tmp === "<FONT SIZE=\"undefined\">test</FONT>", "'test'.fontsize() = " + tmp);
tmp = "test".fontsize(3);
ok(tmp === "<FONT SIZE=\"3\">test</FONT>", "'test'.fontsize(3) = " + tmp);
tmp = "test".fontsize("green");
ok(tmp === "<FONT SIZE=\"green\">test</FONT>", "'test'.fontsize('green') = " + tmp);

tmp = ("".fontcolor()).fontsize();
ok(tmp === "<FONT SIZE=\"undefined\"><FONT COLOR=\"undefined\"></FONT></FONT>", "(''.fontcolor()).fontsize() = " + tmp);

tmp = "".italics();
ok(tmp === "<I></I>", "''.italics() = " + tmp);
tmp = "".italics(3);
ok(tmp === "<I></I>", "''.italics(3) = " + tmp);
tmp = "test".italics();
ok(tmp === "<I>test</I>", "'test'.italics() = " + tmp);
tmp = "test".italics(3);
ok(tmp === "<I>test</I>", "'test'.italics(3) = " + tmp);

tmp = "".link();
ok(tmp === "<A HREF=\"undefined\"></A>", "''.link() = " + tmp);
tmp = "".link(3);
ok(tmp === "<A HREF=\"3\"></A>", "''.link(3) = " + tmp);
tmp = "".link("red");
ok(tmp === "<A HREF=\"red\"></A>", "''.link('red') = " + tmp);
tmp = "test".link();
ok(tmp === "<A HREF=\"undefined\">test</A>", "'test'.link() = " + tmp);
tmp = "test".link(3);
ok(tmp === "<A HREF=\"3\">test</A>", "'test'.link(3) = " + tmp);
tmp = "test".link("green");
ok(tmp === "<A HREF=\"green\">test</A>", "'test'.link('green') = " + tmp);

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

ok(String.fromCharCode() === "", "String.fromCharCode() = " + String.fromCharCode());
ok(String.fromCharCode(65,"66",67) === "ABC", "String.fromCharCode(65,'66',67) = " + String.fromCharCode(65,"66",67));
ok(String.fromCharCode(1024*64+65, -1024*64+65) === "AA",
        "String.fromCharCode(1024*64+65, -1024*64+65) = " + String.fromCharCode(1024*64+65, -1024*64+65));
ok(String.fromCharCode(65, NaN, undefined).length === 3,
        "String.fromCharCode(65, NaN, undefined).length = " + String.fromCharCode(65, NaN, undefined).length);

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

arr = ["1", "2", "3"];
arr.length = 1;
ok(arr.length === 1, "arr.length = " + arr.length);
arr.length = 3;
ok(arr.length === 3, "arr.length = " + arr.length);
ok(arr.toString() === "1,,", "arr.toString() = " + arr.toString());

arr = Array("a","b","c");
ok(arr.toString() === "a,b,c", "arr.toString() = " + arr.toString());

ok(arr.valueOf === Object.prototype.valueOf, "arr.valueOf !== Object.prototype.valueOf");
ok(arr === arr.valueOf(), "arr !== arr.valueOf");

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

arr = [1,false,'a',null,undefined,'a'];
ok(arr.slice(0,6).toString() === "1,false,a,,,a", "arr.slice(0,6).toString() = " + arr.slice(0,6));
ok(arr.slice(0,6).length === 6, "arr.slice(0,6).length = " + arr.slice(0,6).length);
ok(arr.slice().toString() === "1,false,a,,,a", "arr.slice().toString() = " + arr.slice());
ok(arr.slice("abc").toString() === "1,false,a,,,a", "arr.slice(\"abc\").toString() = " + arr.slice("abc"));
ok(arr.slice(3,8).toString() === ",,a", "arr.slice(3,8).toString() = " + arr.slice(3,8));
ok(arr.slice(3,8).length === 3, "arr.slice(3,8).length = " + arr.slice(3,8).length);
ok(arr.slice(1).toString() === "false,a,,,a", "arr.slice(1).toString() = " + arr.slice(1));
ok(arr.slice(-2).toString() === ",a", "arr.slice(-2).toString() = " + arr.slice(-2));
ok(arr.slice(3,1).toString() === "", "arr.slice(3,1).toString() = " + arr.slice(3,1));
tmp = arr.slice(0,6);
for(var i=0; i < arr.length; i++)
    ok(arr[i] === tmp[i], "arr[" + i + "] = " + arr[i] + " expected " + tmp[i]);
arr[12] = 2;
ok(arr.slice(5).toString() === "a,,,,,,,2", "arr.slice(5).toString() = " + arr.slice(5).toString());
ok(arr.slice(5).length === 8, "arr.slice(5).length = " + arr.slice(5).length);

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

function equals(val, base) {
    var i;
    var num = 0;
    var str = val.toString(base);

    for(i=0; i<str.length; i++) {
        if(str.substring(i, i+1) == '(') break;
        if(str.substring(i, i+1) == '.') break;
        num = num*base + parseInt(str.substring(i, i+1));
    }

    if(str.substring(i, i+1) == '.') {
        var mult = base;
        for(i++; i<str.length; i++) {
            if(str.substring(i, i+1) == '(') break;
            num += parseInt(str.substring(i, i+1))/mult;
            mult *= base;
        }
    }

    if(str.substring(i, i+1) == '(') {
        exp = parseInt(str.substring(i+2));
        num *= Math.pow(base, exp);
    }

    ok(num>val-val/1000 && num<val+val/1000, "equals: num = " + num);
}

ok((10).toString(11) === "a", "(10).toString(11) = " + (10).toString(11));
ok((213213433).toString(17) === "8e2ddcb", "(213213433).toString(17) = " + (213213433).toString(17));
ok((-3254343).toString(33) === "-2oicf", "(-3254343).toString(33) = " + (-3254343).toString(33));
ok((NaN).toString(12) === "NaN", "(NaN).toString(11) = " + (NaN).toString(11));
ok((Infinity).toString(13) === "Infinity", "(Infinity).toString(11) = " + (Infinity).toString(11));
for(i=2; i<10; i++) {
    equals(1.123, i);
    equals(2305843009200000000, i);
    equals(5.123, i);
    equals(21711, i);
    equals(1024*1024*1024*1024*1024*1024*1.9999, i);
    equals(748382, i);
    equals(0.6, i);
    equals(4.65661287308e-10, i);
    ok((0).toString(i) === "0", "(0).toString("+i+") = " + (0).toString(i));
}

ok(parseFloat('123') === 123, "parseFloat('123') = " + parseFloat('123'));
ok(parseFloat('-13.7') === -13.7, "parseFloat('-13.7') = " + parseFloat('-13.7'));
ok(parseFloat('-0.01e-2') === -0.01e-2, "parseFloat('-0.01e-2') = " + parseFloat('-0.01e-2'));
ok(parseFloat('-12e+5') === -12e+5, "parseFloat('-12e+5') = " + parseFloat('-12e+5'));
ok(parseFloat('1E5 not parsed') === 1E5, "parseFloat('1E5 not parsed') = " + parseFloat('1E5 not parsed'));
ok(isNaN(parseFloat('not a number')), "parseFloat('not a number') is not NaN");
ok(parseFloat('+13.2e-3') === 13.2e-3, "parseFloat('+13.2e-3') = " + parseFloat('+13.2e-3'));
ok(parseFloat('.12') === 0.12, "parseFloat('.12') = " + parseFloat('.12'));
ok(parseFloat('1e') === 1, "parseFloat('1e') = " + parseFloat('1e'));

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

tmp = Math.pow(2);
ok(isNaN(tmp), "Math.pow(2) is not NaN");

tmp = Math.pow();
ok(isNaN(tmp), "Math.pow() is not NaN");

tmp = Math.random();
ok(typeof(tmp) == "number", "typeof(tmp) = " + typeof(tmp));
ok(0 <= tmp && tmp <= 1, "Math.random() = " + tmp);

tmp = Math.random(100);
ok(typeof(tmp) == "number", "typeof(tmp) = " + typeof(tmp));
ok(0 <= tmp && tmp <= 1, "Math.random(100) = " + tmp);

tmp = Math.acos(0);
ok(Math.floor(tmp*100) === 157, "Math.acos(0) = " + tmp);

tmp = Math.acos(1);
ok(Math.floor(tmp*100) === 0, "Math.acos(1) = " + tmp);

tmp = Math.acos(-1);
ok(Math.floor(tmp*100) === 314, "Math.acos(-1) = " + tmp);

tmp = Math.acos(Math.PI/4, 2);
ok(Math.floor(tmp*100) === 66, "Math.acos(Math.PI/4, 2) = " + tmp);

tmp = Math.acos(true);
ok(Math.floor(tmp*100) === 0, "Math.acos(true) = " + tmp);

tmp = Math.acos(false);
ok(Math.floor(tmp*100) === 157, "Math.acos(false) = " + tmp);

tmp = Math.acos(1.1);
ok(isNaN(tmp), "Math.acos(1.1) is not NaN");

tmp = Math.acos();
ok(isNaN(tmp), "Math.acos() is not NaN");

tmp = Math.acos(NaN);
ok(isNaN(tmp), "Math.acos(NaN) is not NaN");

tmp = Math.acos(Infinity);
ok(isNaN(tmp), "Math.acos(Infinity) is not NaN");

tmp = Math.acos(-Infinity);
ok(isNaN(tmp), "Math.acos(-Infinity) is not NaN");

tmp = Math.asin(0);
ok(Math.floor(tmp*100) === 0, "Math.asin(0) = " + tmp);

tmp = Math.asin(1);
ok(Math.floor(tmp*100) === 157, "Math.asin(1) = " + tmp);

tmp = Math.asin(-1);
ok(Math.floor(tmp*100) === -158, "Math.asin(-1) = " + tmp);

tmp = Math.asin(Math.PI/4, 2);
ok(Math.floor(tmp*100) === 90, "Math.asin(Math.PI/4, 2) = " + tmp);

tmp = Math.asin(true);
ok(Math.floor(tmp*100) === 157, "Math.asin(true) = " + tmp);

tmp = Math.asin(false);
ok(Math.floor(tmp*100) === 0, "Math.asin(false) = " + tmp);

tmp = Math.asin(1.1);
ok(isNaN(tmp), "Math.asin(1.1) is not NaN");

tmp = Math.asin();
ok(isNaN(tmp), "Math.asin() is not NaN");

tmp = Math.asin(NaN);
ok(isNaN(tmp), "Math.asin(NaN) is not NaN");

tmp = Math.asin(Infinity);
ok(isNaN(tmp), "Math.asin(Infinity) is not NaN");

tmp = Math.asin(-Infinity);
ok(isNaN(tmp), "Math.asin(-Infinity) is not NaN");

tmp = Math.atan(0);
ok(Math.floor(tmp*100) === 0, "Math.atan(0) = " + tmp);

tmp = Math.atan(1);
ok(Math.floor(tmp*100) === 78, "Math.atan(1) = " + tmp);

tmp = Math.atan(-1);
ok(Math.floor(tmp*100) === -79, "Math.atan(-1) = " + tmp);

tmp = Math.atan(true);
ok(Math.floor(tmp*100) === 78, "Math.atan(true) = " + tmp);

tmp = Math.atan(false);
ok(Math.floor(tmp*100) === 0, "Math.atan(false) = " + tmp);

tmp = Math.atan();
ok(isNaN(tmp), "Math.atan() is not NaN");

tmp = Math.atan(NaN);
ok(isNaN(tmp), "Math.atan(NaN) is not NaN");

tmp = Math.atan(Infinity);
ok(Math.floor(tmp*100) === 157, "Math.atan(Infinity) = " + tmp);

tmp = Math.atan(-Infinity);
ok(Math.floor(tmp*100) === -158, "Math.atan(Infinity) = " + tmp);

tmp = Math.atan2(0, 0);
ok(Math.floor(tmp*100) === 0, "Math.atan2(0, 0) = " + tmp);

tmp = Math.atan2(0, 1);
ok(Math.floor(tmp*100) === 0, "Math.atan2(0, 1) = " + tmp);

tmp = Math.atan2(0, Infinity);
ok(Math.floor(tmp*100) === 0, "Math.atan2(0, Infinity) = " + tmp);

tmp = Math.atan2(0, -1);
ok(Math.floor(tmp*100) === 314, "Math.atan2(0, -1) = " + tmp);

tmp = Math.atan2(0, -Infinity);
ok(Math.floor(tmp*100) === 314, "Math.atan2(0, -Infinity) = " + tmp);

tmp = Math.atan2(1, 0);
ok(Math.floor(tmp*100) === 157, "Math.atan2(1, 0) = " + tmp);

tmp = Math.atan2(Infinity, 0);
ok(Math.floor(tmp*100) === 157, "Math.atan2(Infinity, 0) = " + tmp);

tmp = Math.atan2(-1, 0);
ok(Math.floor(tmp*100) === -158, "Math.atan2(-1, 0) = " + tmp);

tmp = Math.atan2(-Infinity, 0);
ok(Math.floor(tmp*100) === -158, "Math.atan2(-Infinity, 0) = " + tmp);

tmp = Math.atan2(1, 1);
ok(Math.floor(tmp*100) === 78, "Math.atan2(1, 1) = " + tmp);

tmp = Math.atan2(-1, -1);
ok(Math.floor(tmp*100) === -236, "Math.atan2(-1, -1) = " + tmp);

tmp = Math.atan2(-1, 1);
ok(Math.floor(tmp*100) === -79, "Math.atan2(-1, 1) = " + tmp);

tmp = Math.atan2(Infinity, Infinity);
ok(Math.floor(tmp*100) === 78, "Math.atan2(Infinity, Infinity) = " + tmp);

tmp = Math.atan2(Infinity, -Infinity, 1);
ok(Math.floor(tmp*100) === 235, "Math.atan2(Infinity, -Infinity, 1) = " + tmp);

tmp = Math.atan2();
ok(isNaN(tmp), "Math.atan2() is not NaN");

tmp = Math.atan2(1);
ok(isNaN(tmp), "Math.atan2(1) is not NaN");

tmp = Math.exp(0);
ok(tmp === 1, "Math.exp(0) = " + tmp);

tmp = Math.exp(1);
ok(Math.floor(tmp*100) === 271, "Math.exp(1) = " + tmp);

tmp = Math.exp(-1);
ok(Math.floor(tmp*100) === 36, "Math.exp(-1) = " + tmp);

tmp = Math.exp(true);
ok(Math.floor(tmp*100) === 271, "Math.exp(true) = " + tmp);

tmp = Math.exp(1, 1);
ok(Math.floor(tmp*100) === 271, "Math.exp(1, 1) = " + tmp);

tmp = Math.exp();
ok(isNaN(tmp), "Math.exp() is not NaN");

tmp = Math.exp(NaN);
ok(isNaN(tmp), "Math.exp(NaN) is not NaN");

tmp = Math.exp(Infinity);
ok(tmp === Infinity, "Math.exp(Infinity) = " + tmp);

tmp = Math.exp(-Infinity);
ok(tmp === 0, "Math.exp(-Infinity) = " + tmp);

tmp = Math.log(1);
ok(Math.floor(tmp*100) === 0, "Math.log(1) = " + tmp);

tmp = Math.log(-1);
ok(isNaN(tmp), "Math.log(-1) is not NaN");

tmp = Math.log(true);
ok(Math.floor(tmp*100) === 0, "Math.log(true) = " + tmp);

tmp = Math.log(1, 1);
ok(Math.floor(tmp*100) === 0, "Math.log(1, 1) = " + tmp);

tmp = Math.log();
ok(isNaN(tmp), "Math.log() is not NaN");

tmp = Math.log(NaN);
ok(isNaN(tmp), "Math.log(NaN) is not NaN");

tmp = Math.log(Infinity);
ok(tmp === Infinity, "Math.log(Infinity) = " + tmp);

tmp = Math.log(-Infinity);
ok(isNaN(tmp), "Math.log(-Infinity) is not NaN");

tmp = Math.sin(0);
ok(tmp === 0, "Math.sin(0) = " + tmp);

tmp = Math.sin(Math.PI/2);
ok(tmp === 1, "Math.sin(Math.PI/2) = " + tmp);

tmp = Math.sin(-Math.PI/2);
ok(tmp === -1, "Math.sin(-Math.PI/2) = " + tmp);

tmp = Math.sin(Math.PI/3, 2);
ok(Math.floor(tmp*100) === 86, "Math.sin(Math.PI/3, 2) = " + tmp);

tmp = Math.sin(true);
ok(Math.floor(tmp*100) === 84, "Math.sin(true) = " + tmp);

tmp = Math.sin(false);
ok(tmp === 0, "Math.sin(false) = " + tmp);

tmp = Math.sin();
ok(isNaN(tmp), "Math.sin() is not NaN");

tmp = Math.sin(NaN);
ok(isNaN(tmp), "Math.sin(NaN) is not NaN");

tmp = Math.sin(Infinity);
ok(isNaN(tmp), "Math.sin(Infinity) is not NaN");

tmp = Math.sin(-Infinity);
ok(isNaN(tmp), "Math.sin(-Infinity) is not NaN");

tmp = Math.sqrt(0);
ok(tmp === 0, "Math.sqrt(0) = " + tmp);

tmp = Math.sqrt(4);
ok(tmp === 2, "Math.sqrt(4) = " + tmp);

tmp = Math.sqrt(-1);
ok(isNaN(tmp), "Math.sqrt(-1) is not NaN");

tmp = Math.sqrt(2, 2);
ok(Math.floor(tmp*100) === 141, "Math.sqrt(2, 2) = " + tmp);

tmp = Math.sqrt(true);
ok(tmp === 1, "Math.sqrt(true) = " + tmp);

tmp = Math.sqrt(false);
ok(tmp === 0, "Math.sqrt(false) = " + tmp);

tmp = Math.sqrt();
ok(isNaN(tmp), "Math.sqrt() is not NaN");

tmp = Math.sqrt(NaN);
ok(isNaN(tmp), "Math.sqrt(NaN) is not NaN");

tmp = Math.sqrt(Infinity);
ok(tmp === Infinity, "Math.sqrt(Infinity) = " + tmp);

tmp = Math.sqrt(-Infinity);
ok(isNaN(tmp), "Math.sqrt(-Infinity) is not NaN");

tmp = Math.tan(0);
ok(tmp === 0, "Math.tan(0) = " + tmp);

tmp = Math.tan(Math.PI);
ok(Math.floor(tmp*100) === -1, "Math.tan(Math.PI) = " + tmp);

tmp = Math.tan(2, 2);
ok(Math.floor(tmp*100) === -219, "Math.tan(2, 2) = " + tmp);

tmp = Math.tan(true);
ok(Math.floor(tmp*100) === 155, "Math.tan(true) = " + tmp);

tmp = Math.tan(false);
ok(tmp === 0, "Math.tan(false) = " + tmp);

tmp = Math.tan();
ok(isNaN(tmp), "Math.tan() is not NaN");

tmp = Math.tan(NaN);
ok(isNaN(tmp), "Math.tan(NaN) is not NaN");

tmp = Math.tan(Infinity);
ok(isNaN(tmp), "Math.tan(Infinity) is not NaN");

tmp = Math.tan(-Infinity);
ok(isNaN(tmp), "Math.tan(-Infinity) is not NaN");

var func = function  (a) {
        var a = 1;
        if(a) return;
    };
ok(func.toString() === "function  (a) {\n        var a = 1;\n        if(a) return;\n    }",
   "func.toString() = " + func.toString());
ok("" + func === "function  (a) {\n        var a = 1;\n        if(a) return;\n    }",
   "'' + func.toString() = " + func);

ok(func.valueOf === Object.prototype.valueOf, "func.valueOf !== Object.prototype.valueOf");
ok(func === func.valueOf(), "func !== func.valueOf()");

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
date = new Date(8.64e15);
ok(date.getTime() === 8.64e15, "date.getTime() = " + date.getTime());
date = new Date(8.64e15+1);
ok(isNaN(0+date.getTime()), "date.getTime() is not NaN");
date = new Date(Infinity);
ok(isNaN(0+date.getTime()), "date.getTime() is not NaN");
date = new Date("3 July 2009 22:28:00 UTC+0100");
ok(date.getTime() === 1246656480000, "date.getTime() = " + date.getTime());
date = new Date(1984, 11, 29, 13, 51, 24, 120);
ok(date.getFullYear() === 1984, "date.getFullYear() = " + date.getFullYear());
ok(date.getMonth() === 11, "date.getMonth() = " + date.getMonth());
ok(date.getDate() === 29, "date.getDate() = " + date.getDate());
ok(date.getHours() === 13, "date.getHours() = " + date.getHours());
ok(date.getMinutes() === 51, "date.getMinutes() = " + date.getMinutes());
ok(date.getSeconds() === 24, "date.getSeconds() = " + date.getSeconds());
ok(date.getMilliseconds() === 120, "date.getMilliseconds() = " + date.getMilliseconds());
date = new Date(731, -32, 40, -1, 70, 65, -13);
ok(date.getFullYear() === 728, "date.getFullYear() = " + date.getFullYear());
ok(date.getMonth() === 5, "date.getMonth() = " + date.getMonth());
ok(date.getDate() === 9, "date.getDate() = " + date.getDate());
ok(date.getHours() === 0, "date.getHours() = " + date.getHours());
ok(date.getMinutes() === 11, "date.getMinutes() = " + date.getMinutes());
ok(date.getSeconds() === 4, "date.getSeconds() = " + date.getSeconds());
ok(date.getMilliseconds() === 987, "date.getMilliseconds() = " + date.getMilliseconds());

ok(date.setTime(123) === 123, "date.setTime(123) !== 123");
ok(date.setTime("123", NaN) === 123, "date.setTime(\"123\") !== 123");
ok(isNaN(date.setTime(NaN)), "date.setTime(NaN) is not NaN");

ok(date.setTime(0) === date.getTime(), "date.setTime(0) !== date.getTime()");
ok(date.getUTCFullYear() === 1970, "date.getUTCFullYear() = " + date.getUTCFullYear());
ok(date.getUTCMonth() === 0, "date.getUTCMonth() = " + date.getUTCMonth());
ok(date.getUTCDate() === 1, "date.getUTCDate() = " + date.getUTCDate());
ok(date.getUTCDay() === 4, "date.getUTCDay() = " + date.getUTCDay());
ok(date.getUTCHours() === 0, "date.getUTCHours() = " + date.getUTCHours());
ok(date.getUTCMinutes() === 0, "date.getUTCMinutes() = " + date.getUTCMinutes());
ok(date.getUTCSeconds() === 0, "date.getUTCSeconds() = " + date.getUTCSeconds());
ok(date.getUTCMilliseconds() === 0, "date.getUTCMilliseconds() = " + date.getUTCMilliseconds());
date.setTime(60*24*60*60*1000);
ok(date.getUTCFullYear() === 1970, "date.getUTCFullYear() = " + date.getUTCFullYear());
ok(date.getUTCMonth() === 2, "date.getUTCMonth() = " + date.getUTCMonth());
ok(date.getUTCDate() === 2, "date.getUTCDate() = " + date.getUTCDate());
ok(date.getUTCDay() === 1, "date.getUTCDay() = " + date.getUTCDay());
ok(date.getUTCHours() === 0, "date.getUTCHours() = " + date.getUTCHours());
ok(date.getUTCMinutes() === 0, "date.getUTCMinutes() = " + date.getUTCMinutes());
ok(date.getUTCSeconds() === 0, "date.getUTCSeconds() = " + date.getUTCSeconds());
ok(date.getUTCMilliseconds() === 0, "date.getUTCMilliseconds() = " + date.getUTCMilliseconds());
date.setTime(59*24*60*60*1000 + 4*365*24*60*60*1000 + 60*60*1000 + 2*60*1000 + 2*1000 + 640);
ok(date.getUTCFullYear() === 1974, "date.getUTCFullYear() = " + date.getUTCFullYear());
ok(date.getUTCMonth() === 1, "date.getUTCMonth() = " + date.getUTCMonth());
ok(date.getUTCMonth(123) === 1, "date.getUTCMonth() = " + date.getUTCMonth());
ok(date.getUTCDate() === 28, "date.getUTCDate() = " + date.getUTCDate());
ok(date.getUTCDay() === 4, "date.getUTCDay() = " + date.getUTCDay());
ok(date.getUTCHours() === 1, "date.getUTCHours() = " + date.getUTCHours());
ok(date.getUTCMinutes() === 2, "date.getUTCMinutes() = " + date.getUTCMinutes());
ok(date.getUTCSeconds() === 2, "date.getUTCSeconds() = " + date.getUTCSeconds());
ok(date.getUTCMilliseconds() === 640, "date.getUTCMilliseconds() = " + date.getUTCMilliseconds());
date.setTime(Infinity);
ok(isNaN(date.getUTCFullYear()), "date.getUTCFullYear() is not NaN");
ok(isNaN(date.getUTCMonth()), "date.getUTCMonth() is not NaN");
ok(isNaN(date.getUTCDate()), "date.getUTCDate() is not NaN");
ok(isNaN(date.getUTCDay()), "date.getUTCDay() is not NaN");
ok(isNaN(date.getUTCHours()), "date.getUTCHours() is not NaN");
ok(isNaN(date.getUTCMinutes()), "date.getUTCMinutes() is not NaN");
ok(isNaN(date.getUTCSeconds()), "date.getUTCSeconds() is not NaN");
ok(isNaN(date.getUTCMilliseconds()), "date.getUTCMilliseconds() is not NaN");
ok(isNaN(date.setMilliseconds(0)), "date.setMilliseconds() is not NaN");

date.setTime(0);
date.setUTCMilliseconds(-10, 2);
ok(date.getUTCMilliseconds() === 990, "date.getUTCMilliseconds() = " + date.getUTCMilliseconds());
date.setUTCMilliseconds(10);
ok(date.getUTCMilliseconds() === 10, "date.getUTCMilliseconds() = " + date.getUTCMilliseconds());
date.setUTCSeconds(-10);
ok(date.getUTCSeconds() === 50, "date.getUTCSeconds() = " + date.getUTCSeconds());
date.setUTCMinutes(-10);
ok(date.getUTCMinutes() === 50, "date.getUTCMinutes() = " + date.getUTCMinutes());
date.setUTCHours(-10);
ok(date.getUTCHours() === 14, "date.getUTCHours() = " + date.getUTCHours());
date.setUTCHours(-123);
ok(date.getTime() === -612549990, "date.getTime() = " + date.getTime());
date.setUTCHours(20);
ok(date.getUTCHours() === 20, "date.getUTCHours() = " + date.getUTCHours());
date.setUTCDate(32);
ok(date.getUTCDate() === 1, "date.getUTCDate() = " + date.getUTCDate());
date.setUTCMonth(22, 37);
ok(date.getTime() === 60987050010, "date.getTime() = " + date.getTime());
date.setUTCFullYear(83, 21, 321);
ok(date.getTime() === -59464984149990, "date.getTime() = " + date.getTime());
ok(Math.abs(date) === 59464984149990, "Math.abs(date) = " + Math.abs(date));
ok(getVT(date+1) === "VT_BSTR", "getVT(date+1) = " + getVT(date+1));

ok(isNaN(Date.parse()), "Date.parse() is not NaN");
ok(isNaN(Date.parse("")), "Date.parse(\"\") is not NaN");
ok(isNaN(Date.parse("Jan Jan 20 2009")), "Date.parse(\"Jan Jan 20 2009\") is not NaN");
ok(Date.parse("Jan 20 2009 UTC") === 1232409600000, "Date.parse(\"Jan 20 2009 UTC\") = " + Date.parse("Jan 20 2009 UTC"));
ok(Date.parse("Jan 20 2009 GMT") === 1232409600000, "Date.parse(\"Jan 20 2009 GMT\") = " + Date.parse("Jan 20 2009 GMT"));
ok(Date.parse("Jan 20 2009 UTC-0") === 1232409600000, "Date.parse(\"Jan 20 2009 UTC-0\") = " + Date.parse("Jan 20 2009 UTC-0"));
ok(Date.parse("Jan 20 2009 UTC+0000") === 1232409600000, "Date.parse(\"Jan 20 2009 UTC+0000\") = " + Date.parse("Jan 20 2009 UTC+0000"));
ok(Date.parse("Ju 13 79 UTC") === 300672000000, "Date.parse(\"Ju 13 79 UTC\") = " + Date.parse("Ju 13 79 UTC"));
ok(Date.parse("12Au91 UTC") === 681955200000, "Date.parse(\"12Au91 UTC\") = " + Date.parse("12Au91 UTC"));
ok(Date.parse("7/02/17 UTC") === -1656806400000, "Date.parse(\"7/02/17 UTC\") = " + Date.parse("7/02/17 UTC"));
ok(Date.parse("Se001   70 12:31:17 UTC") === 21040277000, "Date.parse(\"Se001   70 12:31:17 UTC\") = " + Date.parse("Se001   70 12:31:17 UTC"));
ok(Date.parse("February 31   UTC, 2000 12:31:17 PM") === 952000277000,
        "Date.parse(\"February 31   UTC, 2000 12:31:17 PM\") = " + Date.parse("February 31   UTC, 2000 12:31:17 PM"));
ok(Date.parse("71 11:32AM Dec 12 UTC BC ") === -64346358480000, "Date.parse(\"71 11:32AM Dec 12 UTC BC \") = " + Date.parse("71 11:32AM Dec 12 UTC BC "));
ok(Date.parse("23/71/2000 11::32::UTC") === 1010662320000, "Date.parse(\"23/71/2000 11::32::UTC\") = " + Date.parse("23/71/2000 11::32::UTC"));

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

ok(typeof(Math.LN10) === "number", "typeof(Math.LN10) = " + typeof(Math.LN10));
ok(Math.floor(Math.LN10*100) === 230, "Math.LN10 = " + Math.LN10);
Math.LN10 = "test";
ok(Math.floor(Math.LN10*100) === 230, "modified Math.LN10 = " + Math.LN10);

ok(typeof(Math.SQRT2) === "number", "typeof(Math.SQRT2) = " + typeof(Math.SQRT2));
ok(Math.floor(Math.SQRT2*100) === 141, "Math.SQRT2 = " + Math.SQRT2);
Math.SQRT2 = "test";
ok(Math.floor(Math.SQRT2*100) === 141, "modified Math.SQRT2 = " + Math.SQRT2);

ok(typeof(Math.SQRT1_2) === "number", "typeof(Math.SQRT1_2) = " + typeof(Math.SQRT1_2));
ok(Math.floor(Math.SQRT1_2*100) === 70, "Math.SQRT1_2 = " + Math.SQRT1_2);
Math.SQRT1_2 = "test";
ok(Math.floor(Math.SQRT1_2*100) === 70, "modified Math.SQRT1_2 = " + Math.SQRT1_2);

var bool = new Boolean();
ok(bool.toString() === "false", "bool.toString() = " + bool.toString());
var bool = new Boolean("false");
ok(bool.toString() === "true", "bool.toString() = " + bool.toString());
ok(bool.valueOf() === Boolean(1), "bool.valueOf() = " + bool.valueOf());
ok(bool.toLocaleString() === bool.toString(), "bool.toLocaleString() = " + bool.toLocaleString());

ok(Error.prototype !== TypeError.prototype, "Error.prototype === TypeError.prototype");
ok(RangeError.prototype !== TypeError.prototype, "RangeError.prototype === TypeError.prototype");
ok(Error.prototype.toLocaleString === Object.prototype.toLocaleString,
        "Error.prototype.toLocaleString !== Object.prototype.toLocaleString");
err = new Error();
ok(err.valueOf === Object.prototype.valueOf, "err.valueOf !== Object.prototype.valueOf");
ok(Error.prototype.name === "Error", "Error.prototype.name = " + Error.prototype.name);
ok(err.name === "Error", "err.name = " + err.name);
EvalError.prototype.message = "test";
ok(err.toString !== Object.prototype.toString, "err.toString === Object.prototype.toString");
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
err = new EvalError();
ok(EvalError.prototype.name === "EvalError", "EvalError.prototype.name = " + EvalError.prototype.name);
ok(err.name === "EvalError", "err.name = " + err.name);
ok(err.toString === Error.prototype.toString, "err.toString !== Error.prototype.toString");
ok(err.message === "", "err.message != ''");
err.message = date;
ok(err.message === date, "err.message != date");
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
ok(err.toString !== Object.prototype.toString, "err.toString === Object.prototype.toString");
err = new RangeError();
ok(RangeError.prototype.name === "RangeError", "RangeError.prototype.name = " + RangeError.prototype.name);
ok(err.name === "RangeError", "err.name = " + err.name);
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
err = new ReferenceError();
ok(ReferenceError.prototype.name === "ReferenceError", "ReferenceError.prototype.name = " + ReferenceError.prototype.name);
ok(err.name === "ReferenceError", "err.name = " + err.name);
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
err = new SyntaxError();
ok(SyntaxError.prototype.name === "SyntaxError", "SyntaxError.prototype.name = " + SyntaxError.prototype.name);
ok(err.name === "SyntaxError", "err.name = " + err.name);
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
err = new TypeError();
ok(TypeError.prototype.name === "TypeError", "TypeError.prototype.name = " + TypeError.prototype.name);
ok(err.name === "TypeError", "err.name = " + err.name);
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
err = new URIError();
ok(URIError.prototype.name === "URIError", "URIError.prototype.name = " + URIError.prototype.name);
ok(err.name === "URIError", "err.name = " + err.name);
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
err = new Error("message");
ok(err.message === "message", "err.message !== 'message'");
ok(err.toString() === "[object Error]", "err.toString() = " + err.toString());
err = new Error(123);
ok(err.number === 123, "err.number = " + err.number);
err = new Error(0, "message");
ok(err.number === 0, "err.number = " + err.number);
ok(err.message === "message", "err.message = " + err.message);
ok(err.description === "message", "err.description = " + err.description);

function exception_test(func, type, number) {
    ret = "";
    num = "";
    try {
        func();
    } catch(e) {
        ret = e.name;
        num = e.number;
    }
    ok(ret === type, "Exception test, ret = " + ret + ", expected " + type +". Executed function: " + func.toString());
    ok(num === number, "Exception test, num = " + num + ", expected " + number + ". Executed function: " + func.toString());
}
exception_test(function() {arr.toString = Date.prototype.toString; arr.toString();}, "TypeError", -2146823282);
exception_test(function() {Array(-3);}, "RangeError", -2146823259);
exception_test(function() {arr.toString = Boolean.prototype.toString; arr.toString();}, "TypeError", -2146823278);
exception_test(function() {date.setTime();}, "TypeError", -2146827839);
exception_test(function() {arr.test();}, "TypeError", -2146827850);
exception_test(function() {arr.toString = Number.prototype.toString; arr.toString();}, "TypeError", -2146823287);
exception_test(function() {(new Number(3)).toString(1);}, "TypeError", -2146828283);
exception_test(function() {not_existing_variable.something();}, "TypeError", -2146823279);
exception_test(function() {arr.toString = Function.prototype.toString; arr.toString();}, "TypeError", -2146823286);
exception_test(function() {date();}, "TypeError", -2146823286);
exception_test(function() {arr();}, "TypeError", -2146823286);
exception_test(function() {eval("for(i=0;) {}");}, "SyntaxError", -2146827286);
exception_test(function() {eval("function {};");}, "SyntaxError", -2146827283);
exception_test(function() {eval("if");}, "SyntaxError", -2146827283);
exception_test(function() {eval("do i=0; while");}, "SyntaxError", -2146827283);
exception_test(function() {eval("while");}, "SyntaxError", -2146827283);
exception_test(function() {eval("for");}, "SyntaxError", -2146827283);
exception_test(function() {eval("with");}, "SyntaxError", -2146827283);
exception_test(function() {eval("switch");}, "SyntaxError", -2146827283);
exception_test(function() {eval("if(false");}, "SyntaxError", -2146827282);
exception_test(function() {eval("for(i=0; i<10; i++");}, "SyntaxError", -2146827282);
exception_test(function() {eval("while(true");}, "SyntaxError", -2146827282);
exception_test(function() {test = function() {}}, "ReferenceError", -2146823280);
exception_test(function() {eval("for(i=0")}, "SyntaxError", -2146827284);
exception_test(function() {eval("for(i=0;i<10")}, "SyntaxError", -2146827284);
exception_test(function() {eval("while(")}, "SyntaxError", -2146827286);
exception_test(function() {eval("if(")}, "SyntaxError", -2146827286);
exception_test(function() {eval("'unterminated")}, "SyntaxError", -2146827273);

reportSuccess();

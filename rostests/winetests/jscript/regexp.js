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


var m;

m = "abcabc".match(/ca/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ca", "m[0] is not \"ca\"");

m = "abcabc".match(/ab/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");

m = "abcabc".match(/ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(/Ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m === null, "m is not null");

m = "abcabc".match(/Ab/gi);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "aaabcabc".match(/a+b/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "aaab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "aaa\\\\cabc".match(/\\/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "\\", "m[0] is not \"\\\"");
ok(m["1"] === "\\", "m[1] is not \"\\\"");

m = "abcabc".match(new RegExp("ab"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");

m = "abcabc".match(new RegExp("ab","g"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp(/ab/g));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp("ab","g", "test"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

r = "- [test] -".replace(/\[([^\[]+)\]/g, "success");
ok(r === "- success -", "r = " + r + " expected '- success -'");

r = "[test] [test]".replace(/\[([^\[]+)\]/g, "aa");
ok(r === "aa aa", "r = " + r + "aa aa");

r = "[test] [test]".replace(/\[([^\[]+)\]/, "aa");
ok(r === "aa [test]", "r = " + r + " expected 'aa [test]'");

r = "- [test] -".replace(/\[([^\[]+)\]/g);
ok(r === "- undefined -", "r = " + r + " expected '- undefined -'");

r = "- [test] -".replace(/\[([^\[]+)\]/g, true);
ok(r === "- true -", "r = " + r + " expected '- true -'");

r = "- [test] -".replace(/\[([^\[]+)\]/g, true, "test");
ok(r === "- true -", "r = " + r + " expected '- true -'");

var tmp = 0;

function replaceFunc1(m, off, str) {
    ok(arguments.length === 3, "arguments.length = " + arguments.length);

    switch(tmp) {
    case 0:
        ok(m === "[test1]", "m = " + m + " expected [test1]");
        ok(off === 0, "off = " + off + " expected 0");
        break;
    case 1:
        ok(m === "[test2]", "m = " + m + " expected [test2]");
        ok(off === 8, "off = " + off + " expected 8");
        break;
    default:
        ok(false, "unexpected call");
    }

    ok(str === "[test1] [test2]", "str = " + arguments[3]);
    return "r" + tmp++;
}

r = "[test1] [test2]".replace(/\[[^\[]+\]/g, replaceFunc1);
ok(r === "r0 r1", "r = " + r + " expected 'r0 r1'");

tmp = 0;

function replaceFunc2(m, subm, off, str) {
    ok(arguments.length === 4, "arguments.length = " + arguments.length);

    switch(tmp) {
    case 0:
        ok(subm === "test1", "subm = " + subm);
        ok(m === "[test1]", "m = " + m + " expected [test1]");
        ok(off === 0, "off = " + off + " expected 0");
        break;
    case 1:
        ok(subm === "test2", "subm = " + subm);
        ok(m === "[test2]", "m = " + m + " expected [test2]");
        ok(off === 8, "off = " + off + " expected 8");
        break;
    default:
        ok(false, "unexpected call");
    }

    ok(str === "[test1] [test2]", "str = " + arguments[3]);
    return "r" + tmp++;
}

r = "[test1] [test2]".replace(/\[([^\[]+)\]/g, replaceFunc2);
ok(r === "r0 r1", "r = '" + r + "' expected 'r0 r1'");

r = "1,,2,3".split(/,+/g);
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);

r = "1,,2,3".split(/,+/);
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);

r = "1,,2,".split(/,+/);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);

reportSuccess();

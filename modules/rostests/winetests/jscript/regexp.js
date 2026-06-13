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


var m, re, b, i, obj;

ok(RegExp.leftContext === "", "RegExp.leftContext = " + RegExp.leftContext);
RegExp.leftContext = "abc";
ok(RegExp.leftContext === "", "RegExp.leftContext = " + RegExp.leftContext);

re = /a+/;
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = re.exec(" aabaaa");
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aa", "m[0] = " + m[0]);
ok(RegExp.leftContext === " ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "baaa", "RegExp.rightContext = " + RegExp.rightContext);

m = re.exec(" aabaaa");
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aa", "m[0] = " + m[0]);
ok(m.propertyIsEnumerable("0"), "m.0 is not enumerable");
ok(m.propertyIsEnumerable("input"), "m.input is not enumerable");
ok(m.propertyIsEnumerable("index"), "m.index is not enumerable");
ok(m.propertyIsEnumerable("lastIndex"), "m.lastIndex is not enumerable");
ok(m.propertyIsEnumerable("length") === false, "m.length is not enumerable");
ok(RegExp.leftContext === " ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "baaa", "RegExp.rightContext = " + RegExp.rightContext);

m = /^[^<]*(<(.|\s)+>)[^>]*$|^#(\w+)$/.exec(
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
ok(m === null, "m is not null");

re = /a+/g;
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = re.exec(" aabaaa");
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.lastIndex == 3, "m.lastIndex = " + m.lastIndex);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aa", "m[0] = " + m[0]);

m = re.exec(" aabaaa");
ok(re.lastIndex === 7, "re.lastIndex = " + re.lastIndex);
ok(m.index === 4, "m.index = " + m.index);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aaa", "m[0] = " + m[0]);

m = re.exec(" aabaaa");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);
ok(m === null, "m is not null");

re.exec("               a");
ok(re.lastIndex === 16, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "               ",
   "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "", "RegExp.rightContext = " + RegExp.rightContext);

m = re.exec(" a");
ok(m === null, "m is not null");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = re.exec(" a");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex);

m = re.exec();
ok(m === null, "m is not null");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = /(a|b)+|(c)/.exec("aa");
ok(m[0] === "aa", "m[0] = " + m[0]);
ok(m[1] === "a", "m[1] = " + m[1]);
ok(m[2] === "", "m[2] = " + m[2]);

b = re.test("  a ");
ok(b === true, "re.test('  a ') returned " + b);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "  ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === " ", "RegExp.rightContext = " + RegExp.rightContext);

b = re.test(" a ");
ok(b === false, "re.test(' a ') returned " + b);
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "  ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === " ", "RegExp.rightContext = " + RegExp.rightContext);

re = /\[([^\[]+)\]/g;
m = re.exec(" [test]  ");
ok(re.lastIndex === 7, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.input === " [test]  ", "m.input = " + m.input);
ok(m.length === 2, "m.length = " + m.length);
ok(m[0] === "[test]", "m[0] = " + m[0]);
ok(m[1] === "test", "m[1] = " + m[1]);

b = /a*/.test();
ok(b === true, "/a*/.test() returned " + b);
ok(RegExp.leftContext === "", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "undefined", "RegExp.rightContext = " + RegExp.rightContext);

b = /f/.test();
ok(b === true, "/f/.test() returned " + b);
ok(RegExp.leftContext === "unde", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "ined", "RegExp.rightContext = " + RegExp.rightContext);

b = /abc/.test();
ok(b === false, "/abc/.test() returned " + b);
ok(RegExp.leftContext === "unde", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "ined", "RegExp.rightContext = " + RegExp.rightContext);

m = "abcabc".match(re = /ca/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ca", "m[0] is not \"ca\"");
ok(m.constructor === Array, "unexpected m.constructor");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "ab", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "bc", "RegExp.rightContext = " + RegExp.rightContext);

m = "abcabc".match(/ab/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(RegExp.leftContext === "", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "cabc", "RegExp.rightContext = " + RegExp.rightContext);

m = "abcabc".match(/ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");
/* ok(m.input === "abcabc", "m.input = " + m.input); */

m = "abcabc".match(/Ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m === null, "m is not null");

m = "abcabc".match(/Ab/gi);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");
ok(RegExp.leftContext === "abc", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "c", "RegExp.rightContext = " + RegExp.rightContext);

m = "aaabcabc".match(/a+b/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "aaab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "".match(/a*/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "", "m[0] is not \"\"");

m = "aaa".match(/a*/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "aaa", "m[0] is not \"aaa\"");
ok(m["1"] === "", "m[1] is not \"\"");

m = "b".match(/a*/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "", "m[0] is not \"\"");
ok(m["1"] === "", "m[1] is not \"\"");

m = "".match(/a?/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "", "m[0] is not \"\"");

m = "aaa".match(/a?/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 4, "m.length is not 4");
ok(m["0"] === "a", "m[0] is not \"a\"");
ok(m["1"] === "a", "m[1] is not \"a\"");
ok(m["2"] === "a", "m[2] is not \"a\"");
ok(m["3"] === "", "m[3] is not \"\"");

m = "b".match(/a?/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "", "m[0] is not \"\"");
ok(m["1"] === "", "m[1] is not \"\"");

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
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");
ok(RegExp.leftContext === "abc", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "c", "RegExp.rightContext = " + RegExp.rightContext);

m = "abcabc".match(new RegExp(/ab/g));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp("ab","g", "test"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");
ok(m.index === 3, "m.index = " + m.index);
ok(m.input === "abcabc", "m.input = " + m.input);
ok(m.lastIndex === 5, "m.lastIndex = " + m.lastIndex);

m = "abcabcg".match("ab", "g");
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(RegExp.leftContext === "", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "cabcg", "RegExp.rightContext = " + RegExp.rightContext);

m = "abcabc".match();
ok(m === null, "m is not null");
ok(RegExp.leftContext === "", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "cabcg", "RegExp.rightContext = " + RegExp.rightContext);

m = "abcabc".match(/(a)(b)cabc/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 3, "m.length is not 3");
ok(m[0] === "abcabc", "m[0] is not \"abc\"");
ok(m[1] === "a", "m[1] is not \"a\"");
ok(m[2] === "b", "m[2] is not \"b\"");

re = /(a)bcabc/;
re.lastIndex = -3;
m = "abcabc".match(re);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length = " + m.length + "expected 3");
ok(m[0] === "abcabc", "m[0] is not \"abc\"");
ok(m[1] === "a", "m[1] is not \"a\"");
ok(re.lastIndex === 6, "re.lastIndex = " + re.lastIndex);

re = /(a)bcabc/;
re.lastIndex = 2;
m = "abcabcxxx".match(re);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length = " + m.length + "expected 3");
ok(m[0] === "abcabc", "m[0] is not \"abc\"");
ok(m[1] === "a", "m[1] is not \"a\"");
ok(m.input === "abcabcxxx", "m.input = " + m.input);
ok(re.lastIndex === 6, "re.lastIndex = " + re.lastIndex);

r = "- [test] -".replace(re = /\[([^\[]+)\]/g, "success");
ok(r === "- success -", "r = " + r + " expected '- success -'");
ok(re.lastIndex === 8, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "- ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === " -", "RegExp.rightContext = " + RegExp.rightContext);

r = "[test] [test]".replace(/\[([^\[]+)\]/g, "aa");
ok(r === "aa aa", "r = " + r + "aa aa");
ok(RegExp.leftContext === "[test] ",
   "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "",
   "RegExp.rightContext = " + RegExp.rightContext);

r = "[test] [test]".replace(/\[([^\[]+)\]/, "aa");
ok(r === "aa [test]", "r = " + r + " expected 'aa [test]'");

r = "- [test] -".replace(/\[([^\[]+)\]/g);
ok(r === "- undefined -", "r = " + r + " expected '- undefined -'");

r = "- [test] -".replace(/\[([^\[]+)\]/g, true);
ok(r === "- true -", "r = " + r + " expected '- true -'");

r = "- [test] -".replace(/\[([^\[]+)\]/g, true, "test");
ok(r === "- true -", "r = " + r + " expected '- true -'");
ok(RegExp.leftContext === "- ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === " -", "RegExp.rightContext = " + RegExp.rightContext);

var tmp = 0;

function replaceFunc1(m, off, str) {
    ok(arguments.length === 3, "arguments.length = " + arguments.length);

    switch(tmp) {
    case 0:
        ok(m === "[test1]", "m = " + m + " expected [test1]");
        ok(off === 0, "off = " + off + " expected 0");
        ok(RegExp.leftContext === "- ",
           "RegExp.leftContext = " + RegExp.leftContext);
        ok(RegExp.rightContext === " -",
           "RegExp.rightContext = " + RegExp.rightContext);
        break;
    case 1:
        ok(m === "[test2]", "m = " + m + " expected [test2]");
        ok(off === 8, "off = " + off + " expected 8");
        ok(RegExp.leftContext === "- ",
           "RegExp.leftContext = " + RegExp.leftContext);
        ok(RegExp.rightContext === " -",
           "RegExp.rightContext = " + RegExp.rightContext);
        break;
    default:
        ok(false, "unexpected call");
    }

    ok(str === "[test1] [test2]", "str = " + arguments[3]);
    return "r" + tmp++;
}

r = "[test1] [test2]".replace(/\[[^\[]+\]/g, replaceFunc1);
ok(r === "r0 r1", "r = " + r + " expected 'r0 r1'");
ok(RegExp.leftContext === "[test1] ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "", "RegExp.rightContext = " + RegExp.rightContext);

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

r = "$1,$2".replace(/(\$(\d))/g, "$$1-$1$2");
ok(r === "$1-$11,$1-$22", "r = '" + r + "' expected '$1-$11,$1-$22'");
ok(RegExp.leftContext === "$1,", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "", "RegExp.rightContext = " + RegExp.rightContext);

r = "abc &1 123".replace(/(\&(\d))/g, "$&");
ok(r === "abc &1 123", "r = '" + r + "' expected 'abc &1 123'");
ok(RegExp.leftContext === "abc ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === " 123", "RegExp.rightContext = " + RegExp.rightContext);

r = "abc &1 123".replace(/(\&(\d))/g, "$'");
ok(r === "abc  123 123", "r = '" + r + "' expected 'abc  123 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$`");
ok(r === "abc abc  123", "r = '" + r + "' expected 'abc abc  123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$3");
ok(r === "abc $3 123", "r = '" + r + "' expected 'abc $3 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$");
ok(r === "abc $ 123", "r = '" + r + "' expected 'abc $ 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$a");
ok(r === "abc $a 123", "r = '" + r + "' expected 'abc $a 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$11");
ok(r === "abc &11 123", "r = '" + r + "' expected 'abc &11 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$0");
ok(r === "abc $0 123", "r = '" + r + "' expected 'abc $0 123'");

/a/.test("a");
r = "1 2 3".replace("2", "$&");
ok(r === "1 $& 3", "r = '" + r + "' expected '1 $& 3'");
ok(RegExp.leftContext === "", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "", "RegExp.rightContext = " + RegExp.rightContext);

r = "1 2 3".replace("2", "$'");
ok(r === "1 $' 3", "r = '" + r + "' expected '1 $' 3'");

r = "1,,2,3".split(/,+/g);
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);
ok(RegExp.leftContext === "1,,2", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "3", "RegExp.rightContext = " + RegExp.rightContext);

r = "1,,2,3".split(/,+/g, 2);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(RegExp.leftContext === "1,,2", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "3", "RegExp.rightContext = " + RegExp.rightContext);

r = "1,,2,3".split(/,+/g, 1);
ok(r.length === 1, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(RegExp.leftContext === "1", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "2,3", "RegExp.rightContext = " + RegExp.rightContext);

r = "1,,2,3".split(/,+/);
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);

r = "1,,2,".split(/,+/);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);

re = /,+/;
r = "1,,2,".split(re);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(re.lastIndex === 5, "re.lastIndex = " + re.lastIndex);

re = /,+/g;
r = "1,,2,".split(re);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(re.lastIndex === 5, "re.lastIndex = " + re.lastIndex);

r = "1 12 \t3".split(re = /\s+/).join(";");
ok(r === "1;12;3", "r = " + r);
ok(re.lastIndex === 6, "re.lastIndex = " + re.lastIndex);

r = "123".split(re = /\s+/).join(";");
ok(r === "123", "r = " + r);
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

r = "1ab2aab3".split(/(a+)b/);
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);

r = "A<B>bold</B>and<CODE>coded</CODE>".split(/<(\/)?([^<>]+)>/) ;
ok(r.length === 4, "r.length = " + r.length);

/* another standard violation */
r = "1 12 \t3".split(re = /(\s)+/g).join(";");
ok(r === "1;12;3", "r = " + r);
ok(re.lastIndex === 6, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "1 12", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "3", "RegExp.rightContext = " + RegExp.rightContext);

re = /,+/;
re.lastIndex = 4;
r = "1,,2,".split(re);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(re.lastIndex === 5, "re.lastIndex = " + re.lastIndex);

re = /abc[^d]/g;
ok(re.source === "abc[^d]", "re.source = '" + re.source + "', expected 'abc[^d]'");

re = /a\bc[^d]/g;
ok(re.source === "a\\bc[^d]", "re.source = '" + re.source + "', expected 'a\\bc[^d]'");

re = /abc/;
ok(re === RegExp(re), "re !== RegExp(re)");

re = RegExp("abc[^d]", "g");
ok(re.source === "abc[^d]", "re.source = '" + re.source + "', expected 'abc[^d]'");

re = /abc/;
ok(re === RegExp(re, undefined), "re !== RegExp(re, undefined)");

re = /abc/;
ok(re === RegExp(re, undefined, 1), "re !== RegExp(re, undefined, 1)");

re = /a/g;
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");

m = re.exec(" a   ");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex + " expected 2");
ok(m.index === 1, "m.index = " + m.index + " expected 1");
ok(RegExp.leftContext === " ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "   ", "RegExp.rightContext = " + RegExp.rightContext);

m = re.exec(" a   ");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m === null, "m = " + m + " expected null");

re.lastIndex = 2;
m = re.exec(" a a ");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex + " expected 4");
ok(m.index === 3, "m.index = " + m.index + " expected 3");

re.lastIndex = "2";
ok(re.lastIndex === "2", "re.lastIndex = " + re.lastIndex + " expected '2'");
m = re.exec(" a a ");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex + " expected 4");
ok(m.index === 3, "m.index = " + m.index + " expected 3");

var li = 0;
var obj = new Object();
obj.valueOf = function() { return li; };

re.lastIndex = obj;
ok(re.lastIndex === obj, "re.lastIndex = " + re.lastIndex + " expected obj");
li = 2;
m = re.exec(" a a ");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex + " expected 2");
ok(m.index === 1, "m.index = " + m.index + " expected 1");

re.lastIndex = 3;
re.lastIndex = "test";
ok(re.lastIndex === "test", "re.lastIndex = " + re.lastIndex + " expected 'test'");
m = re.exec(" a a ");
ok(re.lastIndex === 2 || re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 2 or 0");
if(re.lastIndex != 0)
    ok(m.index === 1, "m.index = " + m.index + " expected 1");
else
    ok(m === null, "m = " + m + " expected null");

re.lastIndex = 0;
re.lastIndex = 3.9;
ok(re.lastIndex === 3.9, "re.lastIndex = " + re.lastIndex + " expected 3.9");
m = re.exec(" a a ");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex + " expected 4");
ok(m.index === 3, "m.index = " + m.index + " expected 3");

obj.valueOf = function() { throw 0; }
re.lastIndex = obj;
ok(re.lastIndex === obj, "unexpected re.lastIndex");
m = re.exec(" a a ");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex + " expected 2");
ok(m.index === 1, "m.index = " + m.index + " expected 1");

re.lastIndex = -3;
ok(re.lastIndex === -3, "re.lastIndex = " + re.lastIndex + " expected -3");
m = re.exec(" a a ");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m === null, "m = " + m + " expected null");

re.lastIndex = -1;
ok(re.lastIndex === -1, "re.lastIndex = " + re.lastIndex + " expected -1");
m = re.exec("  ");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m === null, "m = " + m + " expected null");

re = /a/;
re.lastIndex = -3;
ok(re.lastIndex === -3, "re.lastIndex = " + re.lastIndex + " expected -3");
m = re.exec(" a a ");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m.index === 1, "m = " + m + " expected 1");
ok(RegExp.leftContext === " ", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === " a ", "RegExp.rightContext = " + RegExp.rightContext);

re.lastIndex = -1;
ok(re.lastIndex === -1, "re.lastIndex = " + re.lastIndex + " expected -1");
m = re.exec("  ");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m === null, "m = " + m + " expected null");

re = /aa/g;
i = 'baacd'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "b", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "cd", "RegExp.rightContext = " + RegExp.rightContext);

re.lastIndex = 2;
i = 'baacdaa'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);

re = /aa/;
i = 'baacd'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);

re.lastIndex = 2;
i = 'baacdaa'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "b", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "cdaa", "RegExp.rightContext = " + RegExp.rightContext);

re = /d/g;
re.lastIndex = 1;
i = 'abc'.search(re);
ok(i === -1, "'abc'.search(/d/g) = " + i);
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);
ok(RegExp.leftContext === "b", "RegExp.leftContext = " + RegExp.leftContext);
ok(RegExp.rightContext === "cdaa", "RegExp.rightContext = " + RegExp.rightContext);

i = 'abcdde'.search(/[df]/);
ok(i === 3, "'abc'.search(/[df]/) = " + i);

i = 'abcdde'.search(/[df]/, "a");
ok(i === 3, "'abc'.search(/[df]/) = " + i);

i = 'abcdde'.search("[df]");
ok(i === 3, "'abc'.search(/d*/) = " + i);

obj = {
    toString: function() { return "abc"; }
};
i = String.prototype.search.call(obj, "b");
ok(i === 1, "String.prototype.seatch.apply(obj, 'b') = " + i);

i = " undefined ".search();
ok(i === null, "' undefined '.search() = " + i);

tmp = "=)".replace(/=/, "?");
ok(tmp === "?)", "'=)'.replace(/=/, '?') = " + tmp);

tmp = "   ".replace(/^\s*|\s*$/g, "y");
ok(tmp === "yy", '"   ".replace(/^\s*|\s*$/g, "y") = ' + tmp);

tmp = "xxx".replace(/^\s*|\s*$/g, "");
ok(tmp === "xxx", '"xxx".replace(/^\s*|\s*$/g, "y") = ' + tmp);

tmp = "xxx".replace(/^\s*|\s*$/g, "y");
ok(tmp === "yxxxy", '"xxx".replace(/^\s*|\s*$/g, "y") = ' + tmp);

tmp = "x/y".replace(/[/]/, "*");
ok(tmp === "x*y", '"x/y".replace(/[/]/, "*") = ' + tmp);

tmp = "x/y".replace(/[xy/]/g, "*");
ok(tmp === "***", '"x/y".replace(/[xy/]/, "*") = ' + tmp);

tmp = /()/.exec("")[1];
ok(tmp === "", "/()/ captured: " + tmp);
tmp = /()?/.exec("")[1];
ok(tmp === "", "/()?/ captured: " + tmp);
tmp = /()??/.exec("")[1];
ok(tmp === "", "/()??/ captured: " + tmp);
tmp = /()*/.exec("")[1];
ok(tmp === "", "/()*/ captured: " + tmp);
tmp = /()??()/.exec("");
ok(tmp[1] === "", "/()??()/ [1] captured: " + tmp);
ok(tmp[2] === "", "/()??()/ [2] captured: " + tmp);

try {
    tmp = new RegExp("(?<a>b)", "g");
    ok(false, "expected exception with /(?<a>b)/ regex");
}catch(e) {
    ok(e.number === 0xa1399 - 0x80000000, "/(?<a>b)/ regex threw " + e.number);
}

/(b)/.exec("abc");
ok(RegExp.$1 === "b", "RegExp.$1 = " + RegExp.$1);
ok("$2" in RegExp, "RegExp.$2 doesn't exist");
ok(RegExp.$2 === "", "RegExp.$2 = " + RegExp.$2);
ok(RegExp.$9 === "", "RegExp.$9 = " + RegExp.$9);
ok(!("$10" in RegExp), "RegExp.$10 exists");

/(b)(b)(b)(b)(b)(b)(b)(b)(b)(b)(b)/.exec("abbbbbbbbbbbc");
ok(RegExp.$1 === "b", "RegExp.$1 = " + RegExp.$1);
ok(RegExp.$2 === "b", "[2] RegExp.$2 = " + RegExp.$2);
ok(RegExp.$9 === "b", "RegExp.$9 = " + RegExp.$9);
ok(!("$10" in RegExp), "RegExp.$10 exists");

/(b)/.exec("abc");
ok(RegExp.$1 === "b", "RegExp.$1 = " + RegExp.$1);
ok("$2" in RegExp, "RegExp.$2 doesn't exist");
ok(RegExp.$2 === "", "RegExp.$2 = " + RegExp.$2);
ok(RegExp.$9 === "", "RegExp.$9 = " + RegExp.$9);
ok(!("$10" in RegExp), "RegExp.$10 exists");

RegExp.$1 = "a";
ok(RegExp.$1 === "b", "RegExp.$1 = " + RegExp.$1);

ok(/abc/.toString() === "/abc/", "/abc/.toString() = " + /abc/.toString());
ok(/\//.toString() === "/\\//", "/\//.toString() = " + /\//.toString());
tmp = new RegExp("abc/");
ok(tmp.toString() === "/abc//", "(new RegExp(\"abc/\")).toString() = " + tmp.toString());
ok(/abc/g.toString() === "/abc/g", "/abc/g.toString() = " + /abc/g.toString());
ok(/abc/i.toString() === "/abc/i", "/abc/i.toString() = " + /abc/i.toString());
ok(/abc/ig.toString() === "/abc/ig", "/abc/ig.toString() = " + /abc/ig.toString());
ok(/abc/mgi.toString() === "/abc/igm", "/abc/mgi.toString() = " + /abc/mgi.toString());
tmp = new RegExp("abc/", "mgi");
ok(tmp.toString() === "/abc//igm", "(new RegExp(\"abc/\")).toString() = " + tmp.toString());
ok(/abc/.toString(1, false, "3") === "/abc/", "/abc/.toString(1, false, \"3\") = " + /abc/.toString());

re = /x/;
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === false, "re.global = " + re.global);
re = /x/i;
ok(re.ignoreCase === true, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === false, "re.global = " + re.global);
re = new RegExp("xxx", "gi");
ok(re.ignoreCase === true, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === true, "re.global = " + re.global);
re = /x/mg;
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === true, "re.multiline = " + re.multiline);
ok(re.global === true, "re.global = " + re.global);

re = new RegExp(undefined);
ok(re.source === "", "re.source = " + re.source);
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === false, "re.global = " + re.global);

re = new RegExp();
ok(re.source === "", "re.source = " + re.source);
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === false, "re.global = " + re.global);

re = new RegExp(true);
ok(re.source === "true", "re.source = " + re.source);
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === false, "re.global = " + re.global);

re = new RegExp({ toString: function() { return "test"; } });
ok(re.source === "test", "re.source = " + re.source);
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === false, "re.global = " + re.global);

re = new RegExp("test", undefined);
ok(re.source === "test", "re.source = " + re.source);
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === false, "re.multiline = " + re.multiline);
ok(re.global === false, "re.global = " + re.global);

re = new RegExp("test", { toString: function() { return "mg"; } });
ok(re.source === "test", "re.source = " + re.source);
ok(re.ignoreCase === false, "re.ignoreCase = " + re.ignoreCase);
ok(re.multiline === true, "re.multiline = " + re.multiline);
ok(re.global === true, "re.global = " + re.global);

reportSuccess();

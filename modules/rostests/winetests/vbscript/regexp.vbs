'
' Copyright 2013 Piotr Caban for CodeWeavers
'
' This library is free software; you can redistribute it and/or
' modify it under the terms of the GNU Lesser General Public
' License as published by the Free Software Foundation; either
' version 2.1 of the License, or (at your option) any later version.
'
' This library is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
' Lesser General Public License for more details.
'
' You should have received a copy of the GNU Lesser General Public
' License along with this library; if not, write to the Free Software
' Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
'

Option Explicit

Dim x, matches, match, submatch, r

Set x = CreateObject("vbscript.regexp")
Call ok(getVT(x.Pattern) = "VT_BSTR", "getVT(RegExp.Pattern) = " & getVT(x.Pattern))
Call ok(x.Pattern = "", "RegExp.Pattern = " & x.Pattern)
Call ok(getVT(x.IgnoreCase) = "VT_BOOL", "getVT(RegExp.IgnoreCase) = " & getVT(x.IgnoreCase))
Call ok(x.IgnoreCase = false, "RegExp.IgnoreCase = " & x.IgnoreCase)
Call ok(getVT(x.Global) = "VT_BOOL", "getVT(RegExp.Global) = " & getVT(x.Global))
Call ok(x.Global = false, "RegExp.Global = " & x.Global)
Call ok(getVT(x.Multiline) = "VT_BOOL", "getVT(RegExp.Multiline) = " & getVT(x.Multiline))
Call ok(x.Multiline = false, "RegExp.Multiline = " & x.Multiline)

x.Pattern = "a+"
matches = x.Test(" aabaaa")
Call ok(matches = true, "RegExp.Test returned: " & matches)
Set matches = x.Execute(" aabaaa")
Call ok(getVT(matches.Count) = "VT_I4", "getVT(matches.Count) = " & getVT(matches.Count))
Call ok(matches.Count = 1, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "aa", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 1, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 2, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)

x.Global = true
Set matches = x.Execute(" aabaaa")
Call ok(matches.Count = 2, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "aa", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 1, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 2, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)
Set match = matches.Item(1)
Call ok(match.Value = "aaa", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 4, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 3, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)

Set matches = x.Execute(" aabaaa")
Call ok(matches.Count = 2, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "aa", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 1, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 2, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)

x.Pattern = "^[^<]*(<(.|\s)+>)[^>]*$|^#(\w+)$"
x.Global = false
Set matches = x.Execute("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
Call ok(matches.Count = 0, "matches.Count = " & matches.Count)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)

x.Pattern = "(a|b)+|(c)"
Set matches = x.Execute("aa")
Call ok(matches.Count = 1, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "aa", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 0, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 2, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 2, "submatch.Count = " & submatch.Count)
Call ok(getVT(submatch.Item(0)) = "VT_BSTR", "getVT(submatch.Item(0)) = " & getVT(submatch.Item(0)))
Call ok(submatch.Item(0) = "a", "submatch.Item(0) = " & submatch.Item(0))
Call ok(getVT(submatch.Item(1)) = "VT_EMPTY", "getVT(submatch.Item(1)) = " & getVT(submatch.Item(1)))
Call ok(submatch.Item(1) = "", "submatch.Item(0) = " & submatch.Item(1))

matches = x.Test("  a ")
Call ok(matches = true, "RegExp.Test returned: " & matches)
matches = x.Test("  a ")
Call ok(matches = true, "RegExp.Test returned: " & matches)

x.Pattern = "\[([^\[]+)\]"
x.Global = true
Set matches = x.Execute(" [test]  ")
Call ok(matches.Count = 1, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "[test]", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 1, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 6, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 1, "submatch.Count = " & submatch.Count)
Call ok(submatch.Item(0) = "test", "submatch.Item(0) = " & submatch.Item(0))

x.Pattern = "Ab"
x.IgnoreCase = true
Set matches = x.Execute("abcaBc")
Call ok(matches.Count = 2, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "ab", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 0, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 2, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)
Set match = matches.Item(1)
Call ok(match.Value = "aB", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 3, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 2, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)

x.Pattern = "a+b"
x.IgnoreCase = false
Set matches = x.Execute("aaabcabc")
Call ok(matches.Count = 2, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "aaab", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 0, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 4, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)
Set match = matches.Item(1)
Call ok(match.Value = "ab", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 5, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 2, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)

x.Pattern = "\\"
Set matches = x.Execute("aaa\\cabc")
Call ok(matches.Count = 2, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "\", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 3, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 1, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)
Set match = matches.Item(1)
Call ok(match.Value = "\", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 4, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 1, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 0, "submatch.Count = " & submatch.Count)

x.Pattern = "(a)(b)cabc"
Set matches = x.Execute("abcabc")
Call ok(matches.Count = 1, "matches.Count = " & matches.Count)
Set match = matches.Item(0)
Call ok(match.Value = "abcabc", "match.Value = " & match.Value)
Call ok(match.FirstIndex = 0, "match.FirstIndex = " & match.FirstIndex)
Call ok(match.Length = 6, "match.Length = " & match.Length)
Set submatch = match.SubMatches
Call ok(submatch.Count = 2, "submatch.Count = " & submatch.Count)
Call ok(submatch.Item(0) = "a", "submatch.Item(0) = " & submatch.Item(0))
Call ok(submatch.Item(1) = "b", "submatch.Item(0) = " & submatch.Item(1))

Set x = new regexp
Call ok(x.Pattern = "", "RegExp.Pattern = " & x.Pattern)
Call ok(x.IgnoreCase = false, "RegExp.IgnoreCase = " & x.IgnoreCase)
Call ok(x.Global = false, "RegExp.Global = " & x.Global)
Call ok(x.Multiline = false, "RegExp.Multiline = " & x.Multiline)

set matches = x.execute("test")
Call ok(matches.Count = 1, "matches.Count = " & matches.Count)
x.pattern = ""
set matches = x.execute("test")
Call ok(matches.Count = 1, "matches.Count = " & matches.Count)
set match = matches.item(0)
Call ok(match.Value = "", "match.Value = " & match.Value)
x.global = true
set matches = x.execute("test")
Call ok(matches.Count = 5, "matches.Count = " & matches.Count)
set match = matches.item(0)
Call ok(match.Value = "", "match.Value = " & match.Value)
set match = matches.item(4)
Call ok(match.Value = "", "match.Value = " & match.Value)
matches = x.test("test")
Call ok(matches = true, "matches = " & matches)

dim test_global

sub test_replace(pattern, string, rep, exp)
    dim x, re
    set re = new regexp
    re.pattern = pattern
    re.global = test_global
    x = re.replace(string, rep)
    call ok(x = exp, "replace returned " & x & " expected " & exp)
end sub

test_global = true
test_replace "xxx", "xxxx", "y", "yx"
test_replace "\[([^\[]+)\]", "- [test] -", "success", "- success -"
test_replace "\[([^\[]+)\]", "[test] [test]", "aa", "aa aa"
test_replace "(\&(\d))", "abc &1 123", "$'", "abc  123 123"
test_replace "(\&(\d))", "abc &1 123", "$`", "abc abc  123"
test_replace "(\&(\d))", "abc &1 123", "$3", "abc $3 123"
test_replace "\[([^\[]+)\]", "- [test] -", true, "- -1 -"
test_replace "\[([^\[]+)\]", "- [test] -", 6, "- 6 -"
test_replace "(\$(\d))", "$1,$2", "$$1-$1$2", "$1-$11,$1-$22"
test_replace "b", "abc", "x$&z", "axbzc"

test_global = false
test_replace "\[([^\[]+)\]", "[test] [test]", "aa", "aa [test]"

set r = new regexp
x = r.replace("xxx", "y")
call ok(x = "yxxx", "x = " & x)
r.global = true
x = r.replace("xxx", "y")
call ok(x = "yxyxyxy", "x = " & x)

Call reportSuccess()

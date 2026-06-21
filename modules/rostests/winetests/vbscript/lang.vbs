'
' Copyright 2011 Jacek Caban for CodeWeavers
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

OPTION EXPLICIT  : : DIM W

dim x, y, z, e, hi
Dim obj

call ok(true, "true is not true?")
ok true, "true is not true?"
call ok((true), "true is not true?")

ok not false, "not false but not true?"
ok not not true, "not not true but not true?"

Call ok(true = true, "true = true is false")
Call ok(false = false, "false = false is false")
Call ok(not (true = false), "true = false is true")
Call ok("x" = "x", """x"" = ""x"" is false")
Call ok(empty = empty, "empty = empty is false")
Call ok(empty = "", "empty = """" is false")
Call ok(0 = 0.0, "0 <> 0.0")
Call ok(16 = &h10&, "16 <> &h10&")
Call ok(010 = 10, "010 <> 10")
Call ok(10. = 10, "10. <> 10")
Call ok(&hffFFffFF& = -1, "&hffFFffFF& <> -1")
Call ok(&hffFFffFF& = -1, "&hffFFffFF& <> -1")
Call ok(34e5 = 3400000, "34e5 <> 3400000")
Call ok(34e+5 = 3400000, "34e+5 <> 3400000")
Call ok(56.789e5 = 5678900, "56.789e5 = 5678900")
Call ok(56.789e-2 = 0.56789, "56.789e-2 <> 0.56789")
Call ok(1e-94938484 = 0, "1e-... <> 0")
Call ok(34e0 = 34, "34e0 <> 34")
Call ok(34E1 = 340, "34E0 <> 340")
Call ok(.5 = 0.5, ".5 <> 0.5")
Call ok(.5e1 = 5, ".5e1 <> 5")
Call ok(--1 = 1, "--1 = " & --1)
Call ok(-empty = 0, "-empty = " & (-empty))
Call ok(true = -1, "! true = -1")
Call ok(false = 0, "false <> 0")
Call ok(&hff = 255, "&hff <> 255")
Call ok(&Hff = 255, "&Hff <> 255")
Call ok(&hffff = -1, "&hffff <> -1")
Call ok(&hfffe = -2, "&hfffe <> -2")
Call ok(&hffff& = 65535, "&hffff& <> -1")
Call ok(&hfffe& = 65534, "&hfffe& <> -2")
Call ok(&hffffffff& = -1, "&hffffffff& <> -1")
Call ok((&h01or&h02)=3,"&h01or&h02 <> 3")

' Test concat when no space and var begins with h
hi = "y"
x = "x" &hi
Call ok(x = "xy", "x = " & x & " expected ""xy""")

W = 5
Call ok(W = 5, "W = " & W & " expected " & 5)

x = "xx"
Call ok(x = "xx", "x = " & x & " expected ""xx""")

Dim public1 : public1 = 42
Call ok(public1 = 42, "public1=" & public1 & " expected & " & 42)
Private priv1 : priv1 = 43
Call ok(priv1 = 43, "priv1=" & priv1 & " expected & " & 43)
Public pub1 : pub1 = 44
Call ok(pub1 = 44, "pub1=" & pub1 & " expected & " & 44)

Call ok(true <> false, "true <> false is false")
Call ok(not (true <> true), "true <> true is true")
Call ok(not ("x" <> "x"), """x"" <> ""x"" is true")
Call ok(not (empty <> empty), "empty <> empty is true")
Call ok(x <> "x", "x = ""x""")
Call ok("true" <> true, """true"" = true is true")

Call ok("" = true = false, """"" = true = false is false")
Call ok(not(false = true = ""), "false = true = """" is true")
Call ok(not (false = false <> false = false), "false = false <> false = false is true")
Call ok(not ("" <> false = false), """"" <> false = false is true")

Call ok(getVT(false) = "VT_BOOL", "getVT(false) is not VT_BOOL")
Call ok(getVT(true) = "VT_BOOL", "getVT(true) is not VT_BOOL")
Call ok(getVT("") = "VT_BSTR", "getVT("""") is not VT_BSTR")
Call ok(getVT("test") = "VT_BSTR", "getVT(""test"") is not VT_BSTR")
Call ok(getVT(Empty) = "VT_EMPTY", "getVT(Empty) is not VT_EMPTY")
Call ok(getVT(null) = "VT_NULL", "getVT(null) is not VT_NULL")
Call ok(getVT(0) = "VT_I2", "getVT(0) is not VT_I2")
Call ok(getVT(1) = "VT_I2", "getVT(1) is not VT_I2")
Call ok(getVT(0.5) = "VT_R8", "getVT(0.5) is not VT_R8")
Call ok(getVT(.5) = "VT_R8", "getVT(.5) is not VT_R8")
Call ok(getVT(0.0) = "VT_R8", "getVT(0.0) is not VT_R8")
Call ok(getVT(2147483647) = "VT_I4", "getVT(2147483647) is not VT_I4")
Call ok(getVT(2147483648) = "VT_R8", "getVT(2147483648) is not VT_R8")
Call ok(getVT(&h10&) = "VT_I2", "getVT(&h10&) is not VT_I2")
Call ok(getVT(&h10000&) = "VT_I4", "getVT(&h10000&) is not VT_I4")
Call ok(getVT(&H10000&) = "VT_I4", "getVT(&H10000&) is not VT_I4")
Call ok(getVT(&hffFFffFF&) = "VT_I2", "getVT(&hffFFffFF&) is not VT_I2")
Call ok(getVT(&hffFFffFE&) = "VT_I2", "getVT(&hffFFffFE &) is not VT_I2")
Call ok(getVT(&hffF&) = "VT_I2", "getVT(&hffFF&) is not VT_I2")
Call ok(getVT(&hffFF&) = "VT_I4", "getVT(&hffFF&) is not VT_I4")
Call ok(getVT(# 1/1/2011 #) = "VT_DATE", "getVT(# 1/1/2011 #) is not VT_DATE")
Call ok(getVT(1e2) = "VT_R8", "getVT(1e2) is not VT_R8")
Call ok(getVT(1e0) = "VT_R8", "getVT(1e0) is not VT_R8")
Call ok(getVT(0.1e2) = "VT_R8", "getVT(0.1e2) is not VT_R8")
Call ok(getVT(1 & 100000) = "VT_BSTR", "getVT(1 & 100000) is not VT_BSTR")
Call ok(getVT(-empty) = "VT_I2", "getVT(-empty) = " & getVT(-empty))
Call ok(getVT(-null) = "VT_NULL", "getVT(-null) = " & getVT(-null))
Call ok(getVT(y) = "VT_EMPTY*", "getVT(y) = " & getVT(y))
Call ok(getVT(nothing) = "VT_DISPATCH", "getVT(nothing) = " & getVT(nothing))
set x = nothing
Call ok(getVT(x) = "VT_DISPATCH*", "getVT(x=nothing) = " & getVT(x))
x = true
Call ok(getVT(x) = "VT_BOOL*", "getVT(x) = " & getVT(x))
Call ok(getVT(false or true) = "VT_BOOL", "getVT(false) is not VT_BOOL")
x = "x"
Call ok(getVT(x) = "VT_BSTR*", "getVT(x) is not VT_BSTR*")
x = 0.0
Call ok(getVT(x) = "VT_R8*", "getVT(x) = " & getVT(x))

Call ok(isNullDisp(nothing), "nothing is not nulldisp?")

x = "xx"
Call ok("ab" & "cd" = "abcd", """ab"" & ""cd"" <> ""abcd""")
Call ok("ab " & null = "ab ", """ab"" & null = " & ("ab " & null))
Call ok("ab " & empty = "ab ", """ab"" & empty = " & ("ab " & empty))
Call ok(1 & 100000 = "1100000", "1 & 100000 = " & (1 & 100000))
Call ok("ab" & x = "abxx", """ab"" & x = " & ("ab"&x))

if(isEnglishLang) then
    Call ok("" & true = "True", """"" & true = " & true)
    Call ok(true & false = "TrueFalse", "true & false = " & (true & false))
end if

call ok(true and true, "true and true is not true")
call ok(true and not false, "true and not false is not true")
call ok(not (false and true), "not (false and true) is not true")
call ok(getVT(null and true) = "VT_NULL", "getVT(null and true) = " & getVT(null and true))

call ok(false or true, "false or uie is false?")
call ok(not (false or false), "false or false is not false?")
call ok(false and false or true, "false and false or true is false?")
call ok(true or false and false, "true or false and false is false?")
call ok(null or true, "null or true is false")

call ok(true xor false, "true xor false is false?")
call ok(not (false xor false), "false xor false is true?")
call ok(not (true or false xor true), "true or false xor true is true?")
call ok(not (true xor false or true), "true xor false or true is true?")

call ok(false eqv false, "false does not equal false?")
call ok(not (false eqv true), "false equals true?")
call ok(getVT(false eqv null) = "VT_NULL", "getVT(false eqv null) = " & getVT(false eqv null))

call ok(true imp true, "true does not imp true?")
call ok(false imp false, "false does not imp false?")
call ok(not (true imp false), "true imp false?")
call ok(false imp null, "false imp null is false?")

Call ok(2 >= 1, "! 2 >= 1")
Call ok(2 >= 2, "! 2 >= 2")
Call ok(2 => 1, "! 2 => 1")
Call ok(2 => 2, "! 2 => 2")
Call ok(not(true >= 2), "true >= 2 ?")
Call ok(2 > 1, "! 2 > 1")
Call ok(false > true, "! false < true")
Call ok(0 > true, "! 0 > true")
Call ok(not (true > 0), "true > 0")
Call ok(not (0 > 1 = 1), "0 > 1 = 1")
Call ok(1 < 2, "! 1 < 2")
Call ok(1 = 1 < 0, "! 1 = 1 < 0")
Call ok(1 <= 2, "! 1 <= 2")
Call ok(2 <= 2, "! 2 <= 2")
Call ok(1 =< 2, "! 1 =< 2")
Call ok(2 =< 2, "! 2 =< 2")
Call ok(not (2 >< 2), "2 >< 2")
Call ok(2 >< 1, "! 2 >< 1")
Call ok(not (2 <> 2), "2 <> 2")
Call ok(2 <> 1, "! 2 <> 1")

Call ok(isNull(0 = null), "'(0 = null)' is not null")
Call ok(isNull(null = 1), "'(null = 1)' is not null")
Call ok(isNull(0 > null), "'(0 > null)' is not null")
Call ok(isNull(null > 1), "'(null > 1)' is not null")
Call ok(isNull(0 < null), "'(0 < null)' is not null")
Call ok(isNull(null < 1), "'(null < 1)' is not null")
Call ok(isNull(0 <> null), "'(0 <> null)' is not null")
Call ok(isNull(null <> 1), "'(null <> 1)' is not null")
Call ok(isNull(0 >= null), "'(0 >= null)' is not null")
Call ok(isNull(null >= 1), "'(null >= 1)' is not null")
Call ok(isNull(0 <= null), "'(0 <= null)' is not null")
Call ok(isNull(null <= 1), "'(null <= 1)' is not null")

x = 3
Call ok(2+2 = 4, "2+2 = " & (2+2))
Call ok(false + 6 + true = 5, "false + 6 + true <> 5")
Call ok(getVT(2+null) = "VT_NULL", "getVT(2+null) = " & getVT(2+null))
Call ok(2+empty = 2, "2+empty = " & (2+empty))
Call ok(x+x = 6, "x+x = " & (x+x))

Call ok(5-1 = 4, "5-1 = " & (5-1))
Call ok(3+5-true = 9, "3+5-true <> 9")
Call ok(getVT(2-null) = "VT_NULL", "getVT(2-null) = " & getVT(2-null))
Call ok(2-empty = 2, "2-empty = " & (2-empty))
Call ok(2-x = -1, "2-x = " & (2-x))

Call ok(9 Mod 6 = 3, "9 Mod 6 = " & (9 Mod 6))
Call ok(11.6 Mod 5.5 = False, "11.6 Mod 5.5 = " & (11.6 Mod 5.5 = 0.6))
Call ok(7 Mod 4+2 = 5, "7 Mod 4+2 <> 5")
Call ok(getVT(2 mod null) = "VT_NULL", "getVT(2 mod null) = " & getVT(2 mod null))
Call ok(getVT(null mod 2) = "VT_NULL", "getVT(null mod 2) = " & getVT(null mod 2))
'FIXME: Call ok(empty mod 2 = 0, "empty mod 2 = " & (empty mod 2))

Call ok(5 \ 2 = 2, "5 \ 2 = " & (5\2))
Call ok(4.6 \ 1.5 = 2, "4.6 \ 1.5 = " & (4.6\1.5))
Call ok(4.6 \ 1.49 = 5, "4.6 \ 1.49 = " & (4.6\1.49))
Call ok(2+3\4 = 2, "2+3\4 = " & (2+3\4))

Call ok(2*3 = 6, "2*3 = " & (2*3))
Call ok(3/2 = 1.5, "3/2 = " & (3/2))
Call ok(5\4/2 = 2, "5\4/2 = " & (5\2/1))
Call ok(12/3\2 = 2, "12/3\2 = " & (12/3\2))
Call ok(5/1000000 = 0.000005, "5/1000000 = " & (5/1000000))

Call ok(2^3 = 8, "2^3 = " & (2^3))
Call ok(2^3^2 = 64, "2^3^2 = " & (2^3^2))
Call ok(-3^2 = 9, "-3^2 = " & (-3^2))
Call ok(2*3^2 = 18, "2*3^2 = " & (2*3^2))

x =_
    3
x _
    = 3

x = 3

if true then y = true : x = y
ok x, "x is false"

x = true : if false then x = false
ok x, "x is false, if false called?"

if not false then x = true
ok x, "x is false, if not false not called?"

if not false then x = "test" : x = true
ok x, "x is false, if not false not called?"

if false then x = y : call ok(false, "if false .. : called")

if false then x = y : call ok(false, "if false .. : called") else x = "else"
Call ok(x = "else", "else not called?")

if true then x = y else y = x : Call ok(false, "in else?")

if false then :

if false then x = y : if true then call ok(false, "embedded if called")

if false then x=1 else x=2 end if
if true then x=1 end if

x = false
if false then x = true : x = true
Call ok(x = false, "x <> false")

if false then
    ok false, "if false called"
end if

x = true
if x then
    x = false
end if
Call ok(not x, "x is false, if not evaluated?")

x = false
If false Then
   Call ok(false, "inside if false")
Else
   x = true
End If
Call ok(x, "else not called?")

' Else without following newline
x = false
If false Then
   Call ok(false, "inside if false")
Else x = true
End If
Call ok(x, "else not called?")

' Else with colon before statement following newline
x = false
If false Then
   Call ok(false, "inside if false")
Else
: x = true
End If
Call ok(x, "else not called?")

x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not True Then
   Call ok(false, "inside elseif not true")
Else
   x = true
End If
Call ok(x, "else not called?")

x = false
If false Then
   Call ok(false, "inside if false")
   x = 1
   y = 10+x
ElseIf not False Then
   x = true
Else
   Call ok(false, "inside else not true")
End If
Call ok(x, "elseif not called?")

x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not False Then
   x = true
End If
Call ok(x, "elseif not called?")

' ElseIf with statement on same line
x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not False Then x = true
End If
Call ok(x, "elseif not called?")

' ElseIf with statement following statement separator
x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not False Then
: x = true
End If
Call ok(x, "elseif not called?")

x = false
if 1 then x = true
Call ok(x, "if 1 not run?")

x = false
if &h10000& then x = true
Call ok(x, "if &h10000& not run?")

x = false
y = false
while not (x and y)
    if x then
        y = true
    end if
    x = true
wend
call ok((x and y), "x or y is false after while")

if false then
' empty body
end if

if false then
    x = false
elseif true then
' empty body
end if

if false then
    x = false
else
' empty body
end if

while false
wend

if empty then
   ok false, "if empty executed"
end if

while empty
   ok false, "while empty executed"
wend

x = 0
if "0" then
   ok false, "if ""0"" executed"
else
   x = 1
end if
Call ok(x = 1, "if ""0"" else not executed")

x = 0
if "-1" then
   x = 1
else
   ok false, "if ""-1"" else executed"
end if
Call ok(x = 1, "if ""-1"" not executed")

x = 0
if 0.1 then
   x = 1
else
   ok false, "if ""0.1"" else executed"
end if
Call ok(x = 1, "if ""0.1"" not executed")

x = 0
if "TRUE" then
   x = 1
else
   ok false, "if ""TRUE"" else executed"
end if
Call ok(x = 1, "if ""TRUE"" not executed")

x = 0
if "#TRUE#" then
   x = 1
else
   ok false, "if ""#TRUE#"" else executed"
end if
Call ok(x = 1, "if ""#TRUE#"" not executed")

x = 0
if (not "#FALSE#") then
   x = 1
else
   ok false, "if ""not #FALSE#"" else executed"
end if
Call ok(x = 1, "if ""not #FALSE#"" not executed")

Class ValClass
    Public myval

    Public default Property Get defprop
        defprop = myval
    End Property
End Class

Dim MyObject
Set MyObject = New ValClass

MyObject.myval = 1
Call ok(CBool(MyObject) = True, "CBool(MyObject) = " & CBool(MyObject))
x = 0
if MyObject then
   x = 1
else
   ok false, "if ""MyObject(1)"" else executed"
end if
Call ok(x = 1, "if ""MyObject(1)"" not executed")

MyObject.myval = 0
Call ok(CBool(MyObject) = False, "CBool(MyObject) = " & CBool(MyObject))
x = 0
if not MyObject then
   x = 1
else
   ok false, "if ""MyObject(0)"" else executed"
end if
Call ok(x = 1, "if ""MyObject(0)"" not executed")

x = 0
WHILE x < 3 : x = x + 1 : Wend
Call ok(x = 3, "x not equal to 3")

x = 0
WHILE x < 3 : x = x + 1
Wend
Call ok(x = 3, "x not equal to 3")

z = 2
while z > -4 :


z = z -2
wend

x = false
y = false
do while not (x and y)
    if x then
        y = true
    end if
    x = true
loop
call ok((x and y), "x or y is false after while")

do while false
loop

do while false : loop

do while true
    exit do
    ok false, "exit do didn't work"
loop

x = 0
Do While x < 2 : x = x + 1
Loop
Call ok(x = 2, "x not equal to 2")

x = 0
Do While x < 2 : x = x + 1: Loop
Call ok(x = 2, "x not equal to 2")

x = 0
Do While x >= -2 :
x = x - 1
Loop
Call ok(x = -3, "x not equal to -3")

x = false
y = false
do until x and y
    if x then
        y = true
    end if
    x = true
loop
call ok((x and y), "x or y is false after do until")

do until true
loop

do until false
    exit do
    ok false, "exit do didn't work"
loop

x = 0
Do: :: x = x + 2
Loop Until x = 4
Call ok(x = 4, "x not equal to 4")

x = 0
Do: :: x = x + 2 :::  : Loop Until x = 4
Call ok(x = 4, "x not equal to 4")

x = 5
Do: :

: x = x * 2
Loop Until x = 40
Call ok(x = 40, "x not equal to 40")


x = false
do
    if x then exit do
    x = true
loop
call ok(x, "x is false after do..loop?")

x = 0
Do :If x = 6 Then
        Exit Do
    End If
    x = x + 3
Loop
Call ok(x = 6, "x not equal to 6")

x = false
y = false
do
    if x then
        y = true
    end if
    x = true
loop until x and y
call ok((x and y), "x or y is false after while")

do
loop until true

do
    exit do
    ok false, "exit do didn't work"
loop until false

x = false
y = false
do
    if x then
        y = true
    end if
    x = true
loop while not (x and y)
call ok((x and y), "x or y is false after while")

do
loop while false

do
    exit do
    ok false, "exit do didn't work"
loop while true

y = "for1:"
for x = 5 to 8
    y = y & " " & x
next
Call ok(y = "for1: 5 6 7 8", "y = " & y)

y = "for2:"
for x = 5 to 8 step 2
    y = y & " " & x
next
Call ok(y = "for2: 5 7", "y = " & y)

y = "for3:"
x = 2
for x = x+3 to 8
    y = y & " " & x
next
Call ok(y = "for3: 5 6 7 8", "y = " & y)

y = "for4:"
for x = 5 to 4
    y = y & " " & x
next
Call ok(y = "for4:", "y = " & y)

y = "for5:"
for x = 5 to 3 step true
    y = y & " " & x
next
Call ok(y = "for5: 5 4 3", "y = " & y)

y = "for6:"
z = 4
for x = 5 to z step 3-4
    y = y & " " & x
    z = 0
next
Call ok(y = "for6: 5 4", "y = " & y)

y = "for7:"
z = 1
for x = 5 to 8 step z
    y = y & " " & x
    z = 2
next
Call ok(y = "for7: 5 6 7 8", "y = " & y)

z = 0
For x = 10 To 18 Step 2 : : z = z + 1
Next
Call ok(z = 5, "z not equal to 5")

y = "for8:"
for x = 5 to 8
    y = y & " " & x
    x = x+1
next
Call ok(y = "for8: 5 7", "y = " & y)

function testfor( startvalue, endvalue, stepvalue, steps)
    Dim s
    s=0
    for x=startvalue to endvalue step stepvalue
        s = s + 1
    Next
    Call ok( s = steps, "counted " & s & " steps in for loop, expected " & steps)
end function

Call testfor (1, 2, 1, 2)
Call testfor ("1", 2, 1, 2)
Call testfor (1, "2", 1, 2)
Call testfor (1, 2, "1", 2)
Call testfor ("1", "2", "1", 2)
if (isEnglishLang) then
    Call testfor (1, 2, 0.5, 3)
    Call testfor (1, 2.5, 0.5, 4)
    Call testfor ("1", 2,  0.5, 3)
    Call testfor ("1", 2.5, 0.5, 4)
    Call testfor (1, "2",  0.5, 3)
    Call testfor (1, "2.5", 0.5, 4)
    Call testfor (1, 2, "0.5", 3)
    Call testfor (1, 2.5, "0.5", 4)
    Call testfor ("1", "2", "0.5", 3)
    Call testfor ("1", "2.5", "0.5", 4)
end if

for x = 1.5 to 1
    Call ok(false, "for..to called when unexpected")
next

for x = 1 to 100
    exit for
    Call ok(false, "exit for not escaped the loop?")
next

for x = 1 to 5 :
:
:   :exit for
    Call ok(false, "exit for not escaped the loop?")
next

dim a1(8)
a1(6)=8
for x=1 to 8:a1(x)=x-1:next
Call ok(a1(6) = 5, "colon used in for loop")

a1(6)=8
for x=1 to 8:y=1
a1(x)=x-2:next
Call ok(a1(6) = 4, "colon used in for loop")

do while true
    for x = 1 to 100
        exit do
    next
loop

if null then call ok(false, "if null evaluated")

while null
    call ok(false, "while null evaluated")
wend

Call collectionObj.reset()
y = 0
for each x in collectionObj :

   :y = y + 3
next
Call ok(y = 9, "y = " & y)

Call collectionObj.reset()
y = 0
x = 10
z = 0
for each x in collectionObj : z = z + 2
    y = y+1
    Call ok(x = y, "x <> y")
next
Call ok(y = 3, "y = " & y)
Call ok(z = 6, "z = " & z)
Call ok(getVT(x) = "VT_EMPTY*", "getVT(x) = " & getVT(x))

Call collectionObj.reset()
y = 0
x = 10
z = 0
for each x in collectionObj : z = z + 2 : y = y+1 ::
Call ok(x = y, "x <> y") : next
Call ok(y = 3, "y = " & y)
Call ok(z = 6, "z = " & z)

Call collectionObj.reset()
y = false
for each x in collectionObj
    if x = 2 then exit for
    y = 1
next
Call ok(y = 1, "y = " & y)
Call ok(x = 2, "x = " & x)

Set obj = collectionObj
Call obj.reset()
y = 0
x = 10
for each x in obj
    y = y+1
    Call ok(x = y, "x <> y")
next
Call ok(y = 3, "y = " & y)
Call ok(getVT(x) = "VT_EMPTY*", "getVT(x) = " & getVT(x))

x = false
select case 3
    case 2
        Call ok(false, "unexpected case")
    case 2
        Call ok(false, "unexpected case")
    case 4
        Call ok(false, "unexpected case")
    case "test"
    case "another case"
        Call ok(false, "unexpected case")
    case 0, false, 2+1, 10
        x = true
    case ok(false, "unexpected case")
        Call ok(false, "unexpected case")
    case else
        Call ok(false, "unexpected case")
end select
Call ok(x, "wrong case")

x = false
select case 3
    case 3
        x = true
end select
Call ok(x, "wrong case")

x = false
select case 2+2
    case 3
        Call ok(false, "unexpected case")
    case else
        x = true
end select
Call ok(x, "wrong case")

y = "3"
x = false
select case y
    case "3"
        x = true
    case 3
        Call ok(false, "unexpected case")
end select
Call ok(x, "wrong case")

select case 0
    case 1
        Call ok(false, "unexpected case")
    case "2"
        Call ok(false, "unexpected case")
end select

select case 0
end select

x = false
select case 2
    case 3,1,2,4: x = true
    case 5,6,7
        Call ok(false, "unexpected case")
end select
Call ok(x, "wrong case")

x = false
select case 2: case 5,6,7: Call ok(false, "unexpected case")
    case 2,1,2,4
        x = true
    case else: Call ok(false, "unexpected case else")
end select
Call ok(x, "wrong case")

x = False
select case 1  :

    :case 3, 4 :


    case 5
:
        Call ok(false, "unexpected case") :
    Case Else:

        x = True
end select
Call ok(x, "wrong case")

select case 0
    case 1
    case else
       'empty else with comment test
end select

select case 0 : case 1 : case else : end select

' Case without separator
function SelectCaseTest(x)
    select case x
        case 0: SelectCaseTest = 100
        case 1  SelectCaseTest = 200
        case 2
                SelectCaseTest = 300
        case 3
        case 4  SelectCaseTest = 400
        case else SelectCaseTest = 500
    end select
end function

call ok(SelectCaseTest(0) = 100, "Unexpected case " & SelectCaseTest(0))
call ok(SelectCaseTest(1) = 200, "Unexpected case " & SelectCaseTest(1))
call ok(SelectCaseTest(2) = 300, "Unexpected case " & SelectCaseTest(2))
call ok(SelectCaseTest(3) = vbEmpty, "Unexpected case " & SelectCaseTest(3))
call ok(SelectCaseTest(4) = 400, "Unexpected case " & SelectCaseTest(4))
call ok(SelectCaseTest(5) = 500, "Unexpected case " & SelectCaseTest(5))

if false then
Sub testsub
    x = true
End Sub
end if

x = false
Call testsub
Call ok(x, "x is false, testsub not called?")

if false then
Sub testsub_one_line() x = true End Sub
end if

x = false
Call testsub_one_line
Call ok(x, "x is false, testsub_one_line not called?")

Sub SubSetTrue(v)
    Call ok(not v, "v is not true")
    v = true
End Sub

x = false
SubSetTrue x
Call ok(x, "x was not set by SubSetTrue")

SubSetTrue false
Call ok(not false, "false is no longer false?")

Sub SubSetTrue2(ByRef v)
    Call ok(not v, "v is not true")
    v = true
End Sub

x = false
SubSetTrue2 x
Call ok(x, "x was not set by SubSetTrue")

Sub TestSubArgVal(ByVal v)
    Call ok(not v, "v is not false")
    v = true
    Call ok(v, "v is not true?")
End Sub

x = false
Call TestSubArgVal(x)
Call ok(not x, "x is true after TestSubArgVal call?")

Sub TestSubMultiArgs(a,b,c,d,e)
    Call ok(a=1, "a = " & a)
    Call ok(b=2, "b = " & b)
    Call ok(c=3, "c = " & c)
    Call ok(d=4, "d = " & d)
    Call ok(e=5, "e = " & e)
End Sub

Sub TestSubExit(ByRef a)
    If a Then
        Exit Sub
    End If
    Call ok(false, "Exit Sub not called?")
End Sub

Call TestSubExit(true)

Sub TestSubExit2
    for x = 1 to 100
        Exit Sub
    next
End Sub
Call TestSubExit2

TestSubMultiArgs 1, 2, 3, 4, 5
Call TestSubMultiArgs(1, 2, 3, 4, 5)

Sub TestSubLocalVal
    x = false
    Call ok(not x, "local x is not false?")
    Dim x
    Dim a,b, c
End Sub

x = true
y = true
Call TestSubLocalVal
Call ok(x, "global x is not true?")

Public Sub TestPublicSub
End Sub
Call TestPublicSub

Private Sub TestPrivateSub
End Sub
Call TestPrivateSub

Public Sub TestSeparatorSub : :
:
End Sub
Call TestSeparatorSub

if false then
Function testfunc
    x = true
End Function
end if

x = false
Call TestFunc
Call ok(x, "x is false, testfunc not called?")

if false then
Function testfunc_one_line() x = true End Function
end if

x = false
Call testfunc_one_line
Call ok(x, "x is false, testfunc_one_line not called?")

Function FuncSetTrue(v)
    Call ok(not v, "v is not true")
    v = true
End Function

x = false
FuncSetTrue x
Call ok(x, "x was not set by FuncSetTrue")

FuncSetTrue false
Call ok(not false, "false is no longer false?")

Function FuncSetTrue2(ByRef v)
    Call ok(not v, "v is not true")
    v = true
End Function

x = false
FuncSetTrue2 x
Call ok(x, "x was not set by FuncSetTrue")

Function TestFuncArgVal(ByVal v)
    Call ok(not v, "v is not false")
    v = true
    Call ok(v, "v is not true?")
End Function

x = false
Call TestFuncArgVal(x)
Call ok(not x, "x is true after TestFuncArgVal call?")

const c10 = 10
Sub TestParamvsConst(c10)
    Call ok( c10 = 42, "precedence between const and parameter wrong!")
End Sub
Call TestParamvsConst(42)

Sub TestDimVsConst
    dim c10
    c10 = 42
    Call ok( c10 = 42, "precedence between const and dim is wrong")
End Sub
Call TestDimVsConst

Function TestFuncMultiArgs(a,b,c,d,e)
    Call ok(a=1, "a = " & a)
    Call ok(b=2, "b = " & b)
    Call ok(c=3, "c = " & c)
    Call ok(d=4, "d = " & d)
    Call ok(e=5, "e = " & e)
End Function

TestFuncMultiArgs 1, 2, 3, 4, 5
Call TestFuncMultiArgs(1, 2, 3, 4, 5)

Function TestFuncLocalVal
    x = false
    Call ok(not x, "local x is not false?")
    Dim x
End Function

x = true
y = true
Call TestFuncLocalVal
Call ok(x, "global x is not true?")

Function TestFuncExit(ByRef a)
    If a Then
        Exit Function
    End If
    Call ok(false, "Exit Function not called?")
End Function

Call TestFuncExit(true)

Function TestFuncExit2(ByRef a)
    For x = 1 to 100
        For y = 1 to 100
            Exit Function
        Next
    Next
    Call ok(false, "Exit Function not called?")
End Function

Call TestFuncExit2(true)

Sub SubParseTest
End Sub : x = false
Call SubParseTest

Function FuncParseTest
End Function : x = false

Function ReturnTrue
     ReturnTrue = false
     ReturnTrue = true
End Function

Call ok(ReturnTrue(), "ReturnTrue returned false?")

Function SetVal(ByRef x, ByVal v)
    x = v
    SetVal = x
    Exit Function
End Function

x = false
ok SetVal(x, true), "SetVal returned false?"
Call ok(x, "x is not set to true by SetVal?")

Public Function TestPublicFunc
End Function
Call TestPublicFunc

Private Function TestPrivateFunc
End Function
Call TestPrivateFunc

Public Function TestSepFunc(ByVal a) : :
: TestSepFunc = a
End Function
Call ok(TestSepFunc(1) = 1, "Function did not return 1")

ok duplicatedfunc() = 2, "duplicatedfunc = " & duplicatedfunc()

function duplicatedfunc
    ok false, "duplicatedfunc called"
end function

sub duplicatedfunc
    ok false, "duplicatedfunc called"
end sub

function duplicatedfunc
    duplicatedfunc = 2
end function

ok duplicatedfunc() = 2, "duplicatedfunc = " & duplicatedfunc()

' Stop has an effect only in debugging mode
Stop

set x = testObj
Call ok(getVT(x) = "VT_DISPATCH*", "getVT(x=testObj) = " & getVT(x))

Set obj = New EmptyClass
Call ok(getVT(obj) = "VT_DISPATCH*", "getVT(obj) = " & getVT(obj))

Class EmptyClass
End Class

Set x = obj
Call ok(getVT(x) = "VT_DISPATCH*", "getVT(x) = " & getVT(x))

Class TestClass
    Public publicProp
    Public publicArrayProp

    Private privateProp

    Public Function publicFunction()
        privateSub()
        publicFunction = 4
    End Function

    Public Property Get gsProp()
        gsProp = privateProp
        funcCalled = "gsProp get"
        exit property
        Call ok(false, "exit property not returned?")
    End Property

    Public Default Property Get DefValGet
        DefValGet = privateProp
        funcCalled = "GetDefVal"
    End Property

    Public Property Let DefValGet(x)
    End Property

    Public publicProp2

    Public Sub publicSub
    End Sub

    Public Property Let gsProp(val)
        privateProp = val
        funcCalled = "gsProp let"
        exit property
        Call ok(false, "exit property not returned?")
    End Property

    Public Property Set gsProp(val)
        funcCalled = "gsProp set"
        exit property
        Call ok(false, "exit property not returned?")
    End Property

    Public Sub setPrivateProp(x)
        privateProp = x
    End Sub

    Function getPrivateProp
        getPrivateProp = privateProp
    End Function

    Private Sub privateSub
    End Sub

    Public Sub Class_Initialize
        publicProp2 = 2
        ReDim publicArrayProp(2)

        publicArrayProp(0) = 1
        publicArrayProp(1) = 2

        privateProp = true
        Call ok(getVT(privateProp) = "VT_BOOL*", "getVT(privateProp) = " & getVT(privateProp))
        Call ok(getVT(publicProp2) = "VT_I2*", "getVT(publicProp2) = " & getVT(publicProp2))
        Call ok(getVT(Me.publicProp2) = "VT_I2", "getVT(Me.publicProp2) = " & getVT(Me.publicProp2))
    End Sub

    Property Get gsGetProp(x)
        gsGetProp = x
    End Property
End Class

Call testDisp(new testClass)

Set obj = New TestClass

Call ok(obj.publicFunction = 4, "obj.publicFunction = " & obj.publicFunction)
Call ok(obj.publicFunction() = 4, "obj.publicFunction() = " & obj.publicFunction())

obj.publicSub()
Call obj.publicSub
Call obj.publicFunction()

Call ok(getVT(obj.publicProp) = "VT_EMPTY", "getVT(obj.publicProp) = " & getVT(obj.publicProp))
obj.publicProp = 3
Call ok(getVT(obj.publicProp) = "VT_I2", "getVT(obj.publicProp) = " & getVT(obj.publicProp))
Call ok(obj.publicProp = 3, "obj.publicProp = " & obj.publicProp)
obj.publicProp() = 3

Call ok(obj.getPrivateProp() = true, "obj.getPrivateProp() = " & obj.getPrivateProp())
Call obj.setPrivateProp(6)
Call ok(obj.getPrivateProp = 6, "obj.getPrivateProp = " & obj.getPrivateProp)

Dim funcCalled
funcCalled = ""
Call ok(obj.gsProp = 6, "obj.gsProp = " & obj.gsProp)
Call ok(funcCalled = "gsProp get", "funcCalled = " & funcCalled)
obj.gsProp = 3
Call ok(funcCalled = "gsProp let", "funcCalled = " & funcCalled)
Call ok(obj.getPrivateProp = 3, "obj.getPrivateProp = " & obj.getPrivateProp)
Set obj.gsProp = New testclass
Call ok(funcCalled = "gsProp set", "funcCalled = " & funcCalled)

x = obj
Call ok(x = 3, "(x = obj) = " & x)
Call ok(funcCalled = "GetDefVal", "funcCalled = " & funcCalled)
funcCalled = ""
Call ok(obj = 3, "(x = obj) = " & obj)
Call ok(funcCalled = "GetDefVal", "funcCalled = " & funcCalled)

Call obj.Class_Initialize
Call ok(obj.getPrivateProp() = true, "obj.getPrivateProp() = " & obj.getPrivateProp())

'Accessing array property by index
Call ok(obj.publicArrayProp(0) = 1, "obj.publicArrayProp(0) = " & obj.publicArrayProp(0))
Call ok(obj.publicArrayProp(1) = 2, "obj.publicArrayProp(1) = " & obj.publicArrayProp(1))
x = obj.publicArrayProp(2)
Call ok(getVT(x) = "VT_EMPTY*", "getVT(x) = " & getVT(x))
Call ok(obj.publicArrayProp("0") = 1, "obj.publicArrayProp(0) = " & obj.publicArrayProp("0"))

x = (New testclass).publicProp

Class TermTest
    Public Sub Class_Terminate()
        funcCalled = "terminate"
    End Sub
End Class

Set obj = New TermTest
funcCalled = ""
Set obj = Nothing
Call ok(funcCalled = "terminate", "funcCalled = " & funcCalled)

Set obj = New TermTest
funcCalled = ""
Call obj.Class_Terminate
Call ok(funcCalled = "terminate", "funcCalled = " & funcCalled)
funcCalled = ""
Set obj = Nothing
Call ok(funcCalled = "terminate", "funcCalled = " & funcCalled)

Call (New testclass).publicSub()
Call (New testclass).publicSub

class PropTest
    property get prop0()
        prop0 = 1
    end property

    property get prop1(x)
        prop1 = x+1
    end property

    property get prop2(x, y)
        prop2 = x+y
    end property
end class

set obj = new PropTest

call ok(obj.prop0 = 1, "obj.prop0 = " & obj.prop0)
call ok(obj.prop1(3) = 4, "obj.prop1(3) = " & obj.prop1(3))
call ok(obj.prop2(3,4) = 7, "obj.prop2(3,4) = " & obj.prop2(3,4))
call obj.prop0()
call obj.prop1(2)
call obj.prop2(3,4)

x = "following ':' is correct syntax" :
x = "following ':' is correct syntax" :: :
:: x = "also correct syntax"
rem another ugly way for comments
x = "rem as simplestatement" : rem rem comment
:

Set obj = new EmptyClass
Set x = obj
Set y = new EmptyClass

Call ok(obj is x, "obj is not x")
Call ok(x is obj, "x is not obj")
Call ok(not (obj is y), "obj is not y")
Call ok(not obj is y, "obj is not y")
Call ok(not (x is Nothing), "x is 1")
Call ok(Nothing is Nothing, "Nothing is not Nothing")
Call ok(x is obj and true, "x is obj and true is false")

Class TestMe
    Public Sub Test(MyMe)
        Call ok(Me is MyMe, "Me is not MyMe")
    End Sub
End Class

Set obj = New TestMe
Call obj.test(obj)

Call ok(getVT(test) = "VT_DISPATCH", "getVT(test) = " & getVT(test))
Call ok(Me is Test, "Me is not Test")

Const c1 = 1, c2 = 2, c3 = -3
Private Const c4 = 4
Public Const c5 = 5
Call ok(c1 = 1, "c1 = " & c1)
Call ok(getVT(c1) = "VT_I2", "getVT(c1) = " & getVT(c1))
Call ok(c3 = -3, "c3 = " & c3)
Call ok(getVT(c3) = "VT_I2", "getVT(c3) = " & getVT(c3))
Call ok(c4 = 4, "c4 = " & c4)
Call ok(getVT(c4) = "VT_I2", "getVT(c4) = " & getVT(c4))
Call ok(c5 = 5, "c5 = " & c5)
Call ok(getVT(c5) = "VT_I2", "getVT(c5) = " & getVT(c5))

Const cb = True, cs = "test", cnull = null
Call ok(cb, "cb = " & cb)
Call ok(getVT(cb) = "VT_BOOL", "getVT(cb) = " & getVT(cb))
Call ok(cs = "test", "cs = " & cs)
Call ok(getVT(cs) = "VT_BSTR", "getVT(cs) = " & getVT(cs))
Call ok(isNull(cnull), "cnull = " & cnull)
Call ok(getVT(cnull) = "VT_NULL", "getVT(cnull) = " & getVT(cnull))

Call ok(+1 = 1, "+1 != 1")
Call ok(+true = true, "+1 != 1")
Call ok(getVT(+true) = "VT_BOOL", "getVT(+true) = " & getVT(+true))
Call ok(+"true" = "true", """+true"" != true")
Call ok(getVT(+"true") = "VT_BSTR", "getVT(+""true"") = " & getVT(+"true"))
Call ok(+obj is obj, "+obj != obj")
Call ok(+--+-+1 = -1, "+--+-+1 != -1")

if false then Const conststr = "str"
Call ok(conststr = "str", "conststr = " & conststr)
Call ok(getVT(conststr) = "VT_BSTR", "getVT(conststr) = " & getVT(conststr))
Call ok(conststr = "str", "conststr = " & conststr)

Sub ConstTestSub
    Const funcconst = 1
    Call ok(c1 = 1, "c1 = " & c1)
    Call ok(funcconst = 1, "funcconst = " & funcconst)
End Sub

Call ConstTestSub
Dim funcconst

' Property may be used as an identifier (although it's a keyword)
Sub TestProperty
    Dim Property
    PROPERTY = true
    Call ok(property, "property = " & property)

    for property = 1 to 2
    next
End Sub

Call TestProperty

Class Property
    Public Sub Property()
    End Sub

    Sub Test(byref property)
    End Sub
End Class

Class Property2
    Function Property()
    End Function

    Sub Test(property)
    End Sub

    Sub Test2(byval property)
    End Sub
End Class

Class SeparatorTest : : Dim varTest1
:
    Private Sub Class_Initialize : varTest1 = 1
    End Sub ::

    Property Get Test1() :
        Test1 = varTest1
    End Property ::
: :
    Property Let Test1(a) :
        varTest1 = a
    End Property :

    Public Function AddToTest1(ByVal a)  :: :
        varTest1 = varTest1 + a
        AddToTest1 = varTest1
    End Function :    End Class : ::   Set obj = New SeparatorTest

Call ok(obj.Test1 = 1, "obj.Test1 is not 1")
obj.Test1 = 6
Call ok(obj.Test1 = 6, "obj.Test1 is not 6")
obj.AddToTest1(5)
Call ok(obj.Test1 = 11, "obj.Test1 is not 11")

set obj = unkObj
set x = obj
call ok(getVT(obj) = "VT_UNKNOWN*", "getVT(obj) = " & getVT(obj))
call ok(getVT(x) = "VT_UNKNOWN*", "getVT(x) = " & getVT(x))
call ok(getVT(unkObj) = "VT_UNKNOWN", "getVT(unkObj) = " & getVT(unkObj))
call ok(obj is unkObj, "obj is not unkObj")

' Array tests

Call ok(getVT(arr) = "VT_EMPTY*", "getVT(arr) = " & getVT(arr))

Dim arr(3)
Dim arr2(4,3), arr3(5,4,3), arr0(0), noarr()

Call ok(getVT(arr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(arr) = " & getVT(arr))
Call ok(getVT(arr2) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(arr2) = " & getVT(arr2))
Call ok(getVT(arr0) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(arr0) = " & getVT(arr0))
Call ok(getVT(noarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(noarr) = " & getVT(noarr))

Call testArray(1, arr)
Call testArray(2, arr2)
Call testArray(3, arr3)
Call testArray(0, arr0)
Call testArray(-1, noarr)

Call ok(getVT(arr(1)) = "VT_EMPTY*", "getVT(arr(1)) = " & getVT(arr(1)))
Call ok(getVT(arr2(1,2)) = "VT_EMPTY*", "getVT(arr2(1,2)) = " & getVT(arr2(1,2)))
Call ok(getVT(arr3(1,2,2)) = "VT_EMPTY*", "getVT(arr3(1,2,3)) = " & getVT(arr3(1,2,2)))
Call ok(getVT(arr(0)) = "VT_EMPTY*", "getVT(arr(0)) = " & getVT(arr(0)))
Call ok(getVT(arr(3)) = "VT_EMPTY*", "getVT(arr(3)) = " & getVT(arr(3)))
Call ok(getVT(arr0(0)) = "VT_EMPTY*", "getVT(arr0(0)) = " & getVT(arr0(0)))

arr(2) = 3
Call ok(arr(2) = 3, "arr(2) = " & arr(2))
Call ok(getVT(arr(2)) = "VT_I2*", "getVT(arr(2)) = " & getVT(arr(2)))

arr3(3,2,1) = 1
arr3(1,2,3) = 2
Call ok(arr3(3,2,1) = 1, "arr3(3,2,1) = " & arr3(3,2,1))
Call ok(arr3(1,2,3) = 2, "arr3(1,2,3) = " & arr3(1,2,3))
arr2(4,3) = 1
Call ok(arr2(4,3) = 1, "arr2(4,3) = " & arr2(4,3))

x = arr3
Call ok(x(3,2,1) = 1, "x(3,2,1) = " & x(3,2,1))

Function getarr()
    Dim arr(3)
    arr(2) = 2
    getarr = arr
    arr(3) = 3
End Function

x = getarr()
Call ok(getVT(x) = "VT_ARRAY|VT_VARIANT*", "getVT(x) = " & getVT(x))
Call ok(x(2) = 2, "x(2) = " & x(2))
Call ok(getVT(x(3)) = "VT_EMPTY*", "getVT(x(3)) = " & getVT(x(3)))

x(1) = 1
Call ok(x(1) = 1, "x(1) = " & x(1))
x = getarr()
Call ok(getVT(x(1)) = "VT_EMPTY*", "getVT(x(1)) = " & getVT(x(1)))
Call ok(x(2) = 2, "x(2) = " & x(2))

x(1) = 1
y = x
x(1) = 2
Call ok(y(1) = 1, "y(1) = " & y(1))

for x=1 to 1
    Dim forarr(3)
    if x=1 then
        Call ok(getVT(forarr(1)) = "VT_EMPTY*", "getVT(forarr(1)) = " & getVT(forarr(1)))
    else
        Call ok(forarr(1) = x, "forarr(1) = " & forarr(1))
    end if
    forarr(1) = x+1
next

x=1
Call ok(forarr(x) = 2, "forarr(x) = " & forarr(x))

sub accessArr()
    ok arr(1) = 1, "arr(1) = " & arr(1)
    arr(1) = 2
end sub
arr(1) = 1
call accessArr
ok arr(1) = 2, "arr(1) = " & arr(1)

sub accessArr2(x,y)
    ok arr2(x,y) = 1, "arr2(x,y) = " & arr2(x,y)
    x = arr2(x,y)
    arr2(x,y) = 2
end sub
arr2(1,2) = 1
call accessArr2(1, 2)
ok arr2(1,2) = 2, "arr2(1,2) = " & arr2(1,2)

x = Array(Array(3))
call ok(x(0)(0) = 3, "x(0)(0) = " & x(0)(0))

function seta0(arr)
    arr(0) = 2
    seta0 = 1
end function

x = Array(1)
seta0 x
ok x(0) = 2, "x(0) = " & x(0)

x = Array(1)
seta0 (x)
ok x(0) = 1, "x(0) = " & x(0)

x = Array(1)
call (((seta0))) ((x))
ok x(0) = 1, "x(0) = " & x(0)

x = Array(1)
call (((seta0))) (x)
ok x(0) = 2, "x(0) = " & x(0)

x = Array(Array(3))
seta0 x(0)
call ok(x(0)(0) = 2, "x(0)(0) = " & x(0)(0))

x = Array(Array(3))
seta0 (x(0))
call ok(x(0)(0) = 3, "x(0)(0) = " & x(0)(0))

y = (seta0)(x)
ok y = 1, "y = " & y

y = ((x))(0)
ok y = 2, "y = " & y

sub changearg(x)
    x = 2
end sub

x = Array(1)
changearg x(0)
ok x(0) = 2, "x(0) = " & x(0)
ok getVT(x) = "VT_ARRAY|VT_VARIANT*", "getVT(x) after redim = " & getVT(x)

x = Array(1)
changearg (x(0))
ok x(0) = 1, "x(0) = " & x(0)

x = Array(1)
redim x(4)
ok ubound(x) = 4, "ubound(x) = " & ubound(x)
ok x(0) = empty, "x(0) = " & x(0)

x = 1
redim x(3)
ok ubound(x) = 3, "ubound(x) = " & ubound(x)

x(0) = 1
x(1) = 2
x(2) = 3
x(2) = 4

redim preserve x(1)
ok ubound(x) = 1, "ubound(x) = " & ubound(x)
ok x(0) = 1, "x(0) = " & x(1)
ok x(1) = 2, "x(1) = " & x(1)

redim preserve x(2)
ok ubound(x) = 2, "ubound(x) = " & ubound(x)
ok x(0) = 1, "x(0) = " & x(0)
ok x(1) = 2, "x(1) = " & x(1)
ok x(2) = vbEmpty, "x(2) = " & x(2)

on error resume next
redim preserve x(2,2)
e = err.number
on error goto 0
ok e = 9, "e = " & e ' VBSE_OUT_OF_BOUNDS, cannot change cDims

x = Array(1, 2)
redim x(-1)
ok lbound(x) = 0, "lbound(x) = " & lbound(x)
ok ubound(x) = -1, "ubound(x) = " & ubound(x)

redim x(3, 2)
ok ubound(x) = 3, "ubound(x) = " & ubound(x)
ok ubound(x, 1) = 3, "ubound(x, 1) = " & ubound(x, 1)
ok ubound(x, 2) = 2, "ubound(x, 2) = " & ubound(x, 2) & " expected 2"

redim x(1, 3)
x(0,0) = 1.1
x(0,1) = 1.2
x(0,2) = 1.3
x(0,3) = 1.4
x(1,0) = 2.1
x(1,1) = 2.2
x(1,2) = 2.3
x(1,3) = 2.4

redim preserve x(1,1)
ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
ok ubound(x, 2) = 1, "ubound(x, 2) = " & ubound(x, 2)
ok x(0,0) = 1.1, "x(0,0) = " & x(0,0)
ok x(0,1) = 1.2, "x(0,1) = " & x(0,1)
ok x(1,0) = 2.1, "x(1,0) = " & x(1,0)
ok x(1,1) = 2.2, "x(1,1) = " & x(1,1)

redim preserve x(1,2)
ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
ok ubound(x, 2) = 2, "ubound(x, 2) = " & ubound(x, 2)
ok x(0,0) = 1.1, "x(0,0) = " & x(0,0)
ok x(0,1) = 1.2, "x(0,1) = " & x(0,1)
ok x(1,0) = 2.1, "x(1,0) = " & x(1,0)
ok x(1,1) = 2.2, "x(1,1) = " & x(1,1)
ok x(0,2) = vbEmpty, "x(0,2) = " & x(0,2)
ok x(1,2) = vbEmpty, "x(1,2) = " & x(1,1)

on error resume next
redim preserve x(2,2)
e = err.number
on error goto 0
ok e = 9, "e = " & e ' VBSE_OUT_OF_BOUNDS, can only change rightmost dimension

sub TestReDimFixed
    on error resume next

    dim staticarray(4)
    err.clear
    redim staticarray(3)
    call ok(err.number = 10, "err.number = " & err.number)
    call ok(isArrayFixed(staticarray), "Expected fixed size array")

    err.clear
    redim staticarray("abc")
    call ok(err.number = 10, "err.number = " & err.number)

    dim staticarray2(4)
    err.clear
    redim preserve staticarray2(5)
    call ok(err.number = 10, "err.number = " & err.number)
    call ok(isArrayFixed(staticarray2), "Expected fixed size array")

    err.clear
    redim preserve staticarray2("abc")
    ' Win10+ builds return INVALID_CALL (5)
    call ok(err.number = 5 or err.number = 13, "err.number = " & err.number)
end sub
Call TestRedimFixed

sub TestRedimInputArg
    on error resume next

    dim x

    x = Array(1)
    err.clear
    redim x("abc")
    call ok(err.number = 13, "err.number = " & err.number)

    err.clear
    redim preserve x("abc")
    ' Win10+ builds return INVALID_CALL (5)
    call ok(err.number = 5 or err.number = 13, "err.number = " & err.number)
end sub
Call TestRedimInputArg

sub TestReDimList
    dim x, y

    x = Array(1)
    y = Array(1)
    redim x(1, 3), y(2)
    ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
    ok ubound(x, 2) = 3, "ubound(x, 2) = " & ubound(x, 2)
    ok ubound(y, 1) = 2, "ubound(y, 1) = " & ubound(y, 1)

    x(0,0) = 1.1
    x(0,1) = 1.2
    x(0,2) = 1.3
    x(0,3) = 1.4
    x(1,0) = 2.1
    x(1,1) = 2.2
    x(1,2) = 2.3
    x(1,3) = 2.4

    y(0) = 2.1
    y(1) = 2.2
    y(2) = 2.3

    redim preserve x(1,1), y(3)
    ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
    ok ubound(x, 2) = 1, "ubound(x, 2) = " & ubound(x, 2)
    ok x(0,0) = 1.1, "x(0,0) = " & x(0,0)
    ok x(0,1) = 1.2, "x(0,1) = " & x(0,1)
    ok x(1,0) = 2.1, "x(1,0) = " & x(1,0)
    ok x(1,1) = 2.2, "x(1,1) = " & x(1,1)
    ok ubound(y, 1) = 3, "ubound(y, 1) = " & ubound(y, 1)
    ok y(0) = 2.1, "y(0) = " & y(0)
    ok y(1) = 2.2, "y(1) = " & y(1)
    ok y(2) = 2.3, "y(2) = " & y(2)
    ok y(3) = vbEmpty, "y(3) = " & y(3)
end sub
call TestReDimList

dim rx
redim rx(4)
sub TestReDimByRef(byref x)
   ok ubound(x) = 4, "ubound(x) = " & ubound(x)
   redim x(6)
   ok ubound(x) = 6, "ubound(x) = " & ubound(x)
end sub
call TestReDimByRef(rx)
ok ubound(rx) = 6, "ubound(rx) = " & ubound(rx)

redim rx(5)
rx(3)=2
sub TestReDimPreserveByRef(byref x)
   ok ubound(x) = 5, "ubound(x) = " & ubound(x)
   ok x(3) = 2, "x(3) = " & x(3)
   redim preserve x(7)
   ok ubound(x) = 7, "ubound(x) = " & ubound(x)
   ok x(3) = 2, "x(3) = " & x(3)
end sub
call TestReDimPreserveByRef(rx)
ok ubound(rx) = 7, "ubound(rx) = " & ubound(rx)
ok rx(3) = 2, "rx(3) = " & rx(3)

Class ArrClass
    Dim classarr(3)
    Dim classnoarr()
    Dim var

    Private Sub Class_Initialize
        Call ok(getVT(classarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(classarr) = " & getVT(classarr))
        Call testArray(-1, classnoarr)
        classarr(0) = 1
        classarr(1) = 2
        classarr(2) = 3
        classarr(3) = 4
    End Sub

    Public Sub testVarVT
        Call ok(getVT(var) = "VT_ARRAY|VT_VARIANT*", "getVT(var) = " & getVT(var))
    End Sub
End Class

Set obj = new ArrClass
Call ok(getVT(obj.classarr) = "VT_ARRAY|VT_VARIANT", "getVT(obj.classarr) = " & getVT(obj.classarr))
'todo_wine Call ok(obj.classarr(1) = 2, "obj.classarr(1) = " & obj.classarr(1))

obj.var = arr
Call ok(getVT(obj.var) = "VT_ARRAY|VT_VARIANT", "getVT(obj.var) = " & getVT(obj.var))
Call obj.testVarVT

Sub arrarg(byref refarr, byval valarr, byref refarr2, byval valarr2)
    Call ok(getVT(refarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(refarr) = " & getVT(refarr))
    Call ok(getVT(valarr) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr) = " & getVT(valarr))
    Call ok(getVT(refarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(refarr2) = " & getVT(refarr2))
    Call ok(getVT(valarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr2) = " & getVT(valarr2))
End Sub

Call arrarg(arr, arr, obj.classarr, obj.classarr)

Sub arrarg2(byref refarr(), byval valarr(), byref refarr2(), byval valarr2())
    Call ok(getVT(refarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(refarr) = " & getVT(refarr))
    Call ok(getVT(valarr) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr) = " & getVT(valarr))
    Call ok(getVT(refarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(refarr2) = " & getVT(refarr2))
    Call ok(getVT(valarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr2) = " & getVT(valarr2))
End Sub

Call arrarg2(arr, arr, obj.classarr, obj.classarr)

Sub testarrarg(arg(), vt)
    Call ok(getVT(arg) = vt, "getVT() = " & getVT(arg) & " expected " & vt)
End Sub

Call testarrarg(1, "VT_I2*")
Call testarrarg(false, "VT_BOOL*")
Call testarrarg(Empty, "VT_EMPTY*")

Sub modifyarr(arr)
    Call ok(arr(0) = "not modified", "arr(0) = " & arr(0))
    arr(0) = "modified"
End Sub

arr(0) = "not modified"
Call modifyarr(arr)
Call ok(arr(0) = "modified", "arr(0) = " & arr(0))

arr(0) = "not modified"
modifyarr(arr)
Call ok(arr(0) = "not modified", "arr(0) = " & arr(0))

for x = 0 to UBound(arr)
    arr(x) = x
next
y = 0
for each x in arr
    Call ok(x = y, "x = " & x & ", expected " & y)
    Call ok(arr(y) = y, "arr(" & y & ") = " & arr(y))
    arr(y) = 1
    x = 1
    y = y+1
next
Call ok(y = 4, "y = " & y & " after array enumeration")

for x=0 to UBound(arr2, 1)
    for y=0 to UBound(arr2, 2)
        arr2(x, y) = x + y*(UBound(arr2, 1)+1)
    next
next
y = 0
for each x in arr2
    Call ok(x = y, "x = " & x & ", expected " & y)
    y = y+1
next
Call ok(y = 20, "y = " & y & " after array enumeration")

for each x in noarr
    Call ok(false, "Empty array contains: " & x)
next

' It's allowed to declare non-builtin RegExp class...
class RegExp
     public property get Global()
         Call ok(false, "Global called")
         Global = "fail"
     end property
end class

' ...but there is no way to use it because builtin instance is always created
set x = new RegExp
Call ok(x.Global = false, "x.Global = " & x.Global)

sub test_nothing_errors
    dim x
    on error resume next

    x = 1
    err.clear
    x = nothing
    call ok(err.number = 91, "err.number = " & err.number)
    call ok(x = 1, "x = " & x)

    err.clear
    x = not nothing
    call ok(err.number = 91, "err.number = " & err.number)
    call ok(x = 1, "x = " & x)

    err.clear
    x = "" & nothing
    call ok(err.number = 91, "err.number = " & err.number)
    call ok(x = 1, "x = " & x)
end sub
call test_nothing_errors()

sub test_identifiers
    ' test keywords that can also be a declared identifier
    Dim default
    default = "xx"
    Call ok(default = "xx", "default = " & default & " expected ""xx""")

    Dim error
    error = "xx"
    Call ok(error = "xx", "error = " & error & " expected ""xx""")

    Dim explicit
    explicit = "xx"
    Call ok(explicit = "xx", "explicit = " & explicit & " expected ""xx""")

    Dim step
    step = "xx"
    Call ok(step = "xx", "step = " & step & " expected ""xx""")

    Dim property
    property = "xx"
    Call ok(property = "xx", "property = " & property & " expected ""xx""")
end sub
call test_identifiers()

Class class_test_identifiers_as_function_name
    Sub Property ( par )
    End Sub

    Function Error( par )
    End Function

    Sub Default ()
    End Sub

    Function Explicit (par)
        Explicit = par
    End Function

    Sub Step ( default )
    End Sub
End Class

Class class_test_identifiers_as_property_name
    Public Property Get Property()
    End Property

    Public Property Let Error(par)
        Error = par
    End Property

    Public Property Set Default(par)
        Set Default = par
    End Property
End Class

sub test_dotIdentifiers
    ' test keywords that can also be an identifier after a dot
    Call ok(testObj.rem = 10, "testObj.rem = " & testObj.rem & " expected 10")
    Call ok(testObj.true = 10, "testObj.true = " & testObj.true & " expected 10")
    Call ok(testObj.false = 10, "testObj.false = " & testObj.false & " expected 10")
    Call ok(testObj.not = 10, "testObj.not = " & testObj.not & " expected 10")
    Call ok(testObj.and = 10, "testObj.and = " & testObj.and & " expected 10")
    Call ok(testObj.or = 10, "testObj.or = " & testObj.or & " expected 10")
    Call ok(testObj.xor = 10, "testObj.xor = " & testObj.xor & " expected 10")
    Call ok(testObj.eqv = 10, "testObj.eqv = " & testObj.eqv & " expected 10")
    Call ok(testObj.imp = 10, "testObj.imp = " & testObj.imp & " expected 10")
    Call ok(testObj.is = 10, "testObj.is = " & testObj.is & " expected 10")
    Call ok(testObj.mod = 10, "testObj.mod = " & testObj.mod & " expected 10")
    Call ok(testObj.call = 10, "testObj.call = " & testObj.call & " expected 10")
    Call ok(testObj.dim = 10, "testObj.dim = " & testObj.dim & " expected 10")
    Call ok(testObj.sub = 10, "testObj.sub = " & testObj.sub & " expected 10")
    Call ok(testObj.function = 10, "testObj.function = " & testObj.function & " expected 10")
    Call ok(testObj.get = 10, "testObj.get = " & testObj.get & " expected 10")
    Call ok(testObj.let = 10, "testObj.let = " & testObj.let & " expected 10")
    Call ok(testObj.const = 10, "testObj.const = " & testObj.const & " expected 10")
    Call ok(testObj.if = 10, "testObj.if = " & testObj.if & " expected 10")
    Call ok(testObj.else = 10, "testObj.else = " & testObj.else & " expected 10")
    Call ok(testObj.elseif = 10, "testObj.elseif = " & testObj.elseif & " expected 10")
    Call ok(testObj.end = 10, "testObj.end = " & testObj.end & " expected 10")
    Call ok(testObj.then = 10, "testObj.then = " & testObj.then & " expected 10")
    Call ok(testObj.exit = 10, "testObj.exit = " & testObj.exit & " expected 10")
    Call ok(testObj.while = 10, "testObj.while = " & testObj.while & " expected 10")
    Call ok(testObj.wend = 10, "testObj.wend = " & testObj.wend & " expected 10")
    Call ok(testObj.do = 10, "testObj.do = " & testObj.do & " expected 10")
    Call ok(testObj.loop = 10, "testObj.loop = " & testObj.loop & " expected 10")
    Call ok(testObj.until = 10, "testObj.until = " & testObj.until & " expected 10")
    Call ok(testObj.for = 10, "testObj.for = " & testObj.for & " expected 10")
    Call ok(testObj.to = 10, "testObj.to = " & testObj.to & " expected 10")
    Call ok(testObj.each = 10, "testObj.each = " & testObj.each & " expected 10")
    Call ok(testObj.in = 10, "testObj.in = " & testObj.in & " expected 10")
    Call ok(testObj.select = 10, "testObj.select = " & testObj.select & " expected 10")
    Call ok(testObj.case = 10, "testObj.case = " & testObj.case & " expected 10")
    Call ok(testObj.byref = 10, "testObj.byref = " & testObj.byref & " expected 10")
    Call ok(testObj.byval = 10, "testObj.byval = " & testObj.byval & " expected 10")
    Call ok(testObj.option = 10, "testObj.option = " & testObj.option & " expected 10")
    Call ok(testObj.nothing = 10, "testObj.nothing = " & testObj.nothing & " expected 10")
    Call ok(testObj.empty = 10, "testObj.empty = " & testObj.empty & " expected 10")
    Call ok(testObj.null = 10, "testObj.null = " & testObj.null & " expected 10")
    Call ok(testObj.class = 10, "testObj.class = " & testObj.class & " expected 10")
    Call ok(testObj.set = 10, "testObj.set = " & testObj.set & " expected 10")
    Call ok(testObj.new = 10, "testObj.new = " & testObj.new & " expected 10")
    Call ok(testObj.public = 10, "testObj.public = " & testObj.public & " expected 10")
    Call ok(testObj.private = 10, "testObj.private = " & testObj.private & " expected 10")
    Call ok(testObj.next = 10, "testObj.next = " & testObj.next & " expected 10")
    Call ok(testObj.on = 10, "testObj.on = " & testObj.on & " expected 10")
    Call ok(testObj.resume = 10, "testObj.resume = " & testObj.resume & " expected 10")
    Call ok(testObj.goto = 10, "testObj.goto = " & testObj.goto & " expected 10")
    Call ok(testObj.with = 10, "testObj.with = " & testObj.with & " expected 10")
    Call ok(testObj.redim = 10, "testObj.redim = " & testObj.redim & " expected 10")
    Call ok(testObj.preserve = 10, "testObj.preserve = " & testObj.preserve & " expected 10")
    Call ok(testObj.property = 10, "testObj.property = " & testObj.property & " expected 10")
    Call ok(testObj.me = 10, "testObj.me = " & testObj.me & " expected 10")
    Call ok(testObj.stop = 10, "testObj.stop = " & testObj.stop & " expected 10")
end sub
call test_dotIdentifiers

' Test End statements not required to be preceded by a newline or separator
Sub EndTestSub
    x = 1 End Sub

Sub EndTestSubWithCall
    x = 1
    Call ok(x = 1, "x = " & x)End Sub
Call EndTestSubWithCall()

Function EndTestFunc(x)
    Call ok(x > 0, "x = " & x)End Function
EndTestFunc(1)

Class EndTestClassWithStorageId
    Public x End Class

Class EndTestClassWithDim
    Dim x End Class

Class EndTestClassWithFunc
    Function test(ByVal x)
        x = 0 End Function End Class

Class EndTestClassWithProperty
    Public x
    Public default Property Get defprop
        defprop = x End Property End Class

class TestPropSyntax
    public prop

    function getProp()
        set getProp = prop
    end function

    public default property get def()
        def = ""
    end property
end class

Class TestPropParam
    Public oDict
    Public gotNothing
    Public m_obj

    Public Property Set bar(obj)
        Set m_obj = obj
    End Property
    Public Property Set foo(par,obj)
        Set m_obj = obj
        if obj is Nothing Then gotNothing = True
        oDict = par
    End Property
    Public Property Let Key(oldKey,newKey)
        oDict = oldKey & newKey
    End Property
    Public Property Let three(uno,due,tre)
        oDict = uno & due & tre
    End Property
    Public Property Let ten(a,b,c,d,e,f,g,h,i,j)
        oDict = a & b & c & d & e & f & g & h & i & j
    End Property
End Class

Set x = new TestPropParam
x.key("old") = "new"
call ok(x.oDict = "oldnew","x.oDict = " & x.oDict & " expected oldnew")
x.three(1,2) = 3
call ok(x.oDict = "123","x.oDict = " & x.oDict & " expected 123")
x.ten(1,2,3,4,5,6,7,8,9) = 0
call ok(x.oDict = "1234567890","x.oDict = " & x.oDict & " expected 1234567890")
Set x.bar = Nothing
call ok(x.gotNothing=Empty,"x.gotNothing = " & x.gotNothing  & " expected Empty")
Set x.foo("123") = Nothing
call ok(x.oDict = "123","x.oDict = " & x.oDict & " expected 123")
call ok(x.gotNothing=True,"x.gotNothing = " & x.gotNothing  & " expected true")

set x = new TestPropSyntax
set x.prop = new TestPropSyntax
set x.prop.prop = new TestPropSyntax
x.prop.prop.prop = 2
call ok(x.getProp().getProp.prop = 2, "x.getProp().getProp.prop = " & x.getProp().getProp.prop)
x.getprop.getprop().prop = 3
call ok(x.getProp.prop.prop = 3, "x.getProp.prop.prop = " & x.getProp.prop.prop)
set x.getprop.getprop().prop = new emptyclass
set obj = new emptyclass
set x.getprop.getprop().prop = obj
call ok(x.getprop.getprop().prop is obj, "x.getprop.getprop().prop is not obj (emptyclass)")

ok getVT(x) = "VT_DISPATCH*", "getVT(x) = " & getVT(x)
todo_wine_ok getVT(x()) = "VT_BSTR", "getVT(x()) = " & getVT(x())

funcCalled = ""
class DefaultSubTest1
 Public  default Sub init(a)
    funcCalled = "init" & a
 end sub
end class

Set obj = New DefaultSubTest1
obj.init(1)
call ok(funcCalled = "init1","funcCalled=" & funcCalled)
funcCalled = ""
obj(2)
call ok(funcCalled = "init2","funcCalled=" & funcCalled)

class DefaultSubTest2
 Public Default Function init
    funcCalled = "init"
 end function
end class

Set obj = New DefaultSubTest2
funcCalled = ""
obj.init()
call ok(funcCalled = "init","funcCalled=" & funcCalled)
funcCalled = ""
' todo this is not yet supported
'funcCalled = ""
'obj()
'call ok(funcCalled = "init","funcCalled=" & funcCalled)

with nothing
end with

set x = new TestPropSyntax
with x
     .prop = 1
     ok .prop = 1, ".prop = "&.prop
end with
ok x.prop = 1, "x.prop = " & x.prop

with new TestPropSyntax
     .prop = 1
     ok .prop = 1, ".prop = "&.prop
end with

function testsetresult(x, y)
    set testsetresult = new TestPropSyntax
    testsetresult.prop = x
    y = testsetresult.prop + 1
end function

set x = testsetresult(1, 2)
ok x.prop = 1, "x.prop = " & x.prop

set arr(0) = new TestPropSyntax
arr(0).prop = 1
ok arr(0).prop = 1, "arr(0) = " & arr(0).prop

function recursingfunction(x)
    if (x) then exit function
    recursingfunction = 2
    dim y
    y = recursingfunction
    call ok(y = 2, "y = " & y)
    recursingfunction = 1
    call recursingfunction(True)
end function
call ok(recursingfunction(False) = 1, "unexpected return value " & recursingfunction(False))

x = false
function recursingfunction2
    if (x) then exit function
    recursingfunction2 = 2
    dim y
    y = recursingfunction2
    call ok(y = 2, "y = " & y)
    recursingfunction2 = 1
    x = true
    recursingfunction2()
end function
call ok(recursingfunction2() = 1, "unexpected return value " & recursingfunction2())

function f2(x,y)
end function

f2 1 = 1, 2

function f1(x)
    ok x = true, "x = " & x
end function

f1 1 = 1
f1 1 = (1)
f1 not 1 = 0

arr (0) = 2 xor -2

reportSuccess()

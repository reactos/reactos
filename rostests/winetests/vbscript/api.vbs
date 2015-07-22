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

Option Explicit

Dim x

Class EmptyClass
End Class

Call ok(vbSunday = 1, "vbSunday = " & vbSunday)
Call ok(getVT(vbSunday) = "VT_I2", "getVT(vbSunday) = " & getVT(vbSunday))
Call ok(vbMonday = 2, "vbMonday = " & vbMonday)
Call ok(getVT(vbMonday) = "VT_I2", "getVT(vbMonday) = " & getVT(vbMonday))
Call ok(vbTuesday = 3, "vbTuesday = " & vbTuesday)
Call ok(getVT(vbTuesday) = "VT_I2", "getVT(vbTuesday) = " & getVT(vbTuesday))
Call ok(vbWednesday = 4, "vbWednesday = " & vbWednesday)
Call ok(getVT(vbWednesday) = "VT_I2", "getVT(vbWednesday) = " & getVT(vbWednesday))
Call ok(vbThursday = 5, "vbThursday = " & vbThursday)
Call ok(getVT(vbThursday) = "VT_I2", "getVT(vbThursday) = " & getVT(vbThursday))
Call ok(vbFriday = 6, "vbFriday = " & vbFriday)
Call ok(getVT(vbFriday) = "VT_I2", "getVT(vbFriday) = " & getVT(vbFriday))
Call ok(vbSaturday = 7, "vbSaturday = " & vbSaturday)
Call ok(getVT(vbSaturday) = "VT_I2", "getVT(vbSaturday) = " & getVT(vbSaturday))

Sub TestConstant(name, val, exval)
    Call ok(val = exval, name & " =  " & val & " expected " & exval)
    Call ok(getVT(val) = "VT_I2*", "getVT(" & name & ") = " & getVT(val))
End Sub

Sub TestConstantI4(name, val, exval)
    Call ok(val = exval, name & " =  " & val & " expected " & exval)
    Call ok(getVT(val) = "VT_I4*", "getVT(" & name & ") = " & getVT(val))
End Sub

Sub TestConstantBSTR(name, val, exval)
    Call ok(val = exval, name & " =  " & val & " expected " & exval)
    Call ok(getVT(val) = "VT_BSTR*", "getVT(" & name & ") = " & getVT(val))
End Sub

TestConstant "vbEmpty", vbEmpty, 0
TestConstant "vbNull", vbNull, 1
TestConstant "vbLong", vbLong, 3
TestConstant "vbSingle", vbSingle, 4
TestConstant "vbDouble", vbDouble, 5
TestConstant "vbCurrency", vbCurrency, 6
TestConstant "vbDate", vbDate, 7
TestConstant "vbString", vbString, 8
TestConstant "vbObject", vbObject, 9
TestConstant "vbError", vbError, 10
TestConstant "vbBoolean", vbBoolean, 11
TestConstant "vbVariant", vbVariant, 12
TestConstant "vbDataObject", vbDataObject, 13
TestConstant "vbDecimal", vbDecimal, 14
TestConstant "vbByte", vbByte, 17
TestConstant "vbArray", vbArray, 8192
TestConstant "vbCritical", vbCritical, 16
TestConstant "vbQuestion", vbQuestion, 32
TestConstant "vbExclamation", vbExclamation, 48
TestConstant "vbInformation", vbInformation, 64
TestConstant "vbDefaultButton1", vbDefaultButton1, 0
TestConstant "vbDefaultButton2", vbDefaultButton2, 256
TestConstant "vbDefaultButton3", vbDefaultButton3, 512
TestConstant "vbDefaultButton4", vbDefaultButton4, 768
TestConstant "vbApplicationModal", vbApplicationModal, 0
TestConstant "vbSystemModal", vbSystemModal, 4096
TestConstant "vbUseSystem", vbUseSystem, 0
TestConstant "vbUseSystemDayOfWeek", vbUseSystemDayOfWeek, 0
TestConstant "vbFirstJan1", vbFirstJan1, 1
TestConstant "vbFirstFourDays", vbFirstFourDays, 2
TestConstant "vbFirstFullWeek", vbFirstFullWeek, 3
TestConstant "vbTrue", vbTrue, -1
TestConstant "vbFalse", vbFalse, 0
TestConstantI4 "vbMsgBoxHelpButton", vbMsgBoxHelpButton, 16384
TestConstantI4 "vbMsgBoxSetForeground", vbMsgBoxSetForeground, 65536
TestConstantI4 "vbMsgBoxRight", vbMsgBoxRight, 524288
TestConstantI4 "vbMsgBoxRtlReading", vbMsgBoxRtlReading, 1048576
TestConstant "vbUseDefault", vbUseDefault, -2
TestConstant "vbBinaryCompare", vbBinaryCompare, 0
TestConstant "vbTextCompare", vbTextCompare, 1
TestConstant "vbDatabaseCompare", vbDatabaseCompare, 2
TestConstant "vbGeneralDate", vbGeneralDate, 0
TestConstant "vbLongDate", vbLongDate, 1
TestConstant "vbShortDate", vbShortDate, 2
TestConstant "vbLongTime", vbLongTime, 3
TestConstant "vbShortTime", vbShortTime, 4
TestConstantI4 "vbObjectError", vbObjectError, &h80040000&
TestConstantI4 "vbBlack", vbBlack, 0
TestConstantI4 "vbBlue", vbBlue, &hff0000&
TestConstantI4 "vbCyan", vbCyan, &hffff00&
TestConstantI4 "vbGreen", vbGreen, &h00ff00&
TestConstantI4 "vbMagenta", vbMagenta, &hff00ff&
TestConstantI4 "vbRed", vbRed, &h0000ff&
TestConstantI4 "vbWhite", vbWhite, &hffffff&
TestConstantI4 "vbYellow", vbYellow, &h00ffff&
TestConstantBSTR "vbCr", vbCr, Chr(13)
TestConstantBSTR "vbCrLf", vbCrLf, Chr(13)&Chr(10)
TestConstantBSTR "vbNewLine", vbNewLine, Chr(13)&Chr(10)
TestConstantBSTR "vbFormFeed", vbFormFeed, Chr(12)
TestConstantBSTR "vbLf", vbLf, Chr(10)
TestConstantBSTR "vbNullChar", vbNullChar, Chr(0)
TestConstantBSTR "vbNullString", vbNullString, ""
TestConstantBSTR "vbTab", vbTab, chr(9)
TestConstantBSTR "vbVerticalTab", vbVerticalTab, chr(11)

Sub TestCStr(arg, exval)
    dim x
    x = CStr(arg)
    Call ok(getVT(x) = "VT_BSTR*", "getVT(x) = " & getVT(x))
    Call ok(x = exval, "CStr(" & arg & ") = " & x)
End Sub

TestCStr "test", "test"
TestCStr 3, "3"
if isEnglishLang then TestCStr 3.5, "3.5"
if isEnglishLang then TestCStr true, "True"

Call ok(getVT(Chr(120)) = "VT_BSTR", "getVT(Chr(120)) = " & getVT(Chr(120)))
Call ok(getVT(Chr(255)) = "VT_BSTR", "getVT(Chr(255)) = " & getVT(Chr(255)))
Call ok(Chr(120) = "x", "Chr(120) = " & Chr(120))
Call ok(Chr(0) <> "", "Chr(0) = """"")
Call ok(Chr(120.5) = "x", "Chr(120.5) = " & Chr(120.5))
Call ok(Chr(119.5) = "x", "Chr(119.5) = " & Chr(119.5))
Call ok(Chr("120") = "x", "Chr(""120"") = " & Chr("120"))

sub testChrError
    on error resume next

    if isEnglishLang then
        call Err.clear()
        call Chr(-1)
        call ok(Err.number = 5, "Err.number = " & Err.number)

        call Err.clear()
        call Chr(256)
        call ok(Err.number = 5, "Err.number = " & Err.number)
    end if

    call Err.clear()
    call Chr(65536)
    call ok(Err.number = 5, "Err.number = " & Err.number)

    call Err.clear()
    call Chr(-32769)
    call ok(Err.number = 5, "Err.number = " & Err.number)
end sub

call testChrError

Call ok(isObject(new EmptyClass), "isObject(new EmptyClass) is not true?")
Set x = new EmptyClass
Call ok(isObject(x), "isObject(x) is not true?")
Call ok(isObject(Nothing), "isObject(Nothing) is not true?")
Call ok(not isObject(true), "isObject(true) is true?")
Call ok(not isObject(4), "isObject(4) is true?")
Call ok(not isObject("x"), "isObject(""x"") is true?")
Call ok(not isObject(Null), "isObject(Null) is true?")

Call ok(not isEmpty(new EmptyClass), "isEmpty(new EmptyClass) is true?")
Set x = new EmptyClass
Call ok(not isEmpty(x), "isEmpty(x) is true?")
x = empty
Call ok(isEmpty(x), "isEmpty(x) is not true?")
Call ok(isEmpty(empty), "isEmpty(empty) is not true?")
Call ok(not isEmpty(Nothing), "isEmpty(Nothing) is not true?")
Call ok(not isEmpty(true), "isEmpty(true) is true?")
Call ok(not isEmpty(4), "isEmpty(4) is true?")
Call ok(not isEmpty("x"), "isEmpty(""x"") is true?")
Call ok(not isEmpty(Null), "isEmpty(Null) is true?")

Call ok(not isNull(new EmptyClass), "isNull(new EmptyClass) is true?")
Set x = new EmptyClass
Call ok(not isNull(x), "isNull(x) is true?")
x = null
Call ok(isNull(x), "isNull(x) is not true?")
Call ok(not isNull(empty), "isNull(empty) is true?")
Call ok(not isNull(Nothing), "isNull(Nothing) is true?")
Call ok(not isNull(true), "isNull(true) is true?")
Call ok(not isNull(4), "isNull(4) is true?")
Call ok(not isNull("x"), "isNull(""x"") is true?")
Call ok(isNull(Null), "isNull(Null) is not true?")

Call ok(isNumeric(Empty), "isNumeric(empty) is not true?")
Call ok(not isNumeric(Null), "isNumeric(Null) is not true?")
Call ok(isNumeric(32767), "isNumeric(32767) is true?")
Call ok(isNumeric(32768), "isNumeric(32768) is true?")
Call ok(isNumeric(CSng(3242.4)), "isNumeric(CSng(3242.4)) is true?")
Call ok(isNumeric(32768.4), "isNumeric(32768.4) is true?")
Call ok(isNumeric(CCur(32768.4)), "isNumeric(CCur(32768.4)) is true?")
Call ok(isNumeric("44"), "isNumeric(""44"") is true?")
Call ok(not isNumeric("rwrf"), "isNumeric(""rwrf"") is not true?")
Call ok(not isNumeric(Nothing), "isNumeric(Nothing) is not true?")
Call ok(not isNumeric(New EmptyClass), "isNumeric(New EmptyClass) is not true?")
Call ok(isNumeric(true), "isNumeric(true) is true?")
Call ok(isNumeric(CByte(32)), "isNumeric(CByte(32)) is true?")
Dim arr(2)
arr(0) = 2
arr(1) = 3
Call ok(not isNumeric(arr), "isNumeric(arr) is not true?")

Dim newObject
Set newObject = New ValClass
newObject.myval = 1
Call ok(isNumeric(newObject), "isNumeric(newObject) is true?")
newObject.myval = "test"
Call ok(not isNumeric(newObject), "isNumeric(newObject) is not true?")

Call ok(getVT(err) = "VT_DISPATCH", "getVT(err) = " & getVT(err))

Sub TestHex(x, ex)
    Call ok(hex(x) = ex, "hex(" & x & ") = " & hex(x) & " expected " & ex)
End Sub

TestHex 0, "0"
TestHex 6, "6"
TestHex 16, "10"
TestHex &hdeadbeef&, "DEADBEEF"
TestHex -1, "FFFF"
TestHex -16, "FFF0"
TestHex -934859845, "C8472BBB"
TestHex empty, "0"
TestHex "17", "11"
TestHex 228.5, "E4"
TestHex -32767, "8001"
TestHex -32768, "FFFF8000"
TestHex 2147483647.49, "7FFFFFFF"
TestHex -2147483647.5, "80000000"
newObject.myval = 30.5
TestHex newObject, "1E"
newObject.myval = "27"
TestHex newObject, "1B"


Call ok(getVT(hex(null)) = "VT_NULL", "getVT(hex(null)) = " & getVT(hex(null)))
Call ok(getVT(hex(empty)) = "VT_BSTR", "getVT(hex(empty)) = " & getVT(hex(empty)))

Sub TestHexError(num, err_num)
    On Error Resume Next
    Call Hex(num)
    Call ok(Err.number = err_num, "Hex(" & num & ") returns error number " & Err.number & " expected " & err_num)
End Sub

TestHexError 2147483647.5, 6
TestHexError 2147483648.51, 6
TestHexError "test", 13

Sub TestOct(x, ex, res_type)
    Call ok(Oct(x) = ex, "Oct(" & x & ") = " & Oct(x) & " expected " & ex)
    Call ok(getVT(Oct(x)) = res_type, "getVT(Oct(" &x & ")) = " & getVT(Oct(x)) & "expected " & res_type)
End Sub

Sub TestOctError(num, err_num)
    On error resume next
    Call Oct(num)
    Call ok(Err.number = err_num, "Oct(" & num & ") error number is " & Err.number & " expected " & err_num)
End Sub

TestOct empty, "0", "VT_BSTR"
TestOct 0, "0", "VT_BSTR"
TestOct 9, "11", "VT_BSTR"
TestOct "9", "11", "VT_BSTR"
TestOct 8.5, "10", "VT_BSTR"
TestOct 9.5, "12", "VT_BSTR"
TestOct -1, "177777", "VT_BSTR"
TestOct -32767, "100001", "VT_BSTR"
TestOct -32768, "37777700000", "VT_BSTR"
TestOct 2147483647.49, "17777777777", "VT_BSTR"
TestOct -2147483648.5, "20000000000", "VT_BSTR"
Call ok(getVT(Oct(null)) = "VT_NULL", "getVT(Oct(null)) = " & getVT(Oct(null)))
newObject.myval = 5
TestOct newObject, "5", "VT_BSTR"

TestOctError 2147483647.5, 6
TestOctError -2147483648.51, 6
TestOctError "test", 13

x = InStr(1, "abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abcd", "bc")
Call ok(x = 2, "InStr returned " & x)
Call ok(getVT(x) = "VT_I4*", "getVT(InStr) returned " & getVT(x))

x = InStr("abc", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abcbc", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("bcabc", "bc")
Call ok(x = 1, "InStr returned " & x)

x = InStr(3, "abcd", "bc")
Call ok(x = 0, "InStr returned " & x)

x = InStr("abcd", "bcx")
Call ok(x = 0, "InStr returned " & x)

x = InStr(5, "abcd", "bc")
Call ok(x = 0, "InStr returned " & x)

x = "abcd"
x = InStr(x, "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abcd", null)
Call ok(isNull(x), "InStr returned " & x)
x = InStr(null, "abcd")
Call ok(isNull(x), "InStr returned " & x)
x = InStr(2, null, "abcd")
Call ok(isNull(x), "InStr returned " & x)

x = InStr(1.3, "abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr(2.3, "abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr(2.6, "abcd", "bc")
Call ok(x = 0, "InStr returned " & x)


x = InStrRev("bcabcd", "bc")
Call ok(x = 4, "InStrRev returned " & x)
Call ok(getVT(x) = "VT_I4*", "getVT(InStrRev) returned " & getVT(x))

x = InStrRev("bcabcd", "bc", 6)
Call ok(x = 4, "InStrRev returned " & x)

x = InStrRev("abcd", "bcx")
Call ok(x = 0, "InStrRev returned " & x)

x = InStrRev("abcd", "bc", 3)
Call ok(x = 2, "InStrRev returned " & x)

x = InStrRev("abcd", "bc", 2)
Call ok(x = 0, "InStrRev returned " & x)

x = InStrRev("abcd", "b", 2)
Call ok(x = 2, "InStrRev returned " & x)

x = InStrRev("abcd", "bc", 5)
Call ok(x = 0, "InStrRev returned " & x)

x = InStrRev("abcd", "bc", 15)
Call ok(x = 0, "InStrRev returned " & x)

x = "abcd"
x = InStrRev(x, "bc")
Call ok(x = 2, "InStrRev returned " & x)

x = InStrRev("abcd", "bc", 1.3)
Call ok(x = 0, "InStrRev returned " & x)

x = InStrRev("abcd", "bc", 2.3)
Call ok(x = 0, "InStrRev returned " & x)

x = InStrRev("abcd", "bc", 2.6)
Call ok(x = 2, "InStrRev returned " & x)

x = InStrRev("1234", 34)
Call ok(x = 3, "InStrRev returned " & x)

x = InStrRev(1234, 34)
Call ok(x = 3, "InStrRev returned " & x)

Sub testInStrRevError(arg1, arg2, arg3, error_num)
    on error resume next
    Dim x

    Call Err.clear()
    x = InStrRev(arg1, arg2, arg3)
    Call ok(Err.number = error_num, "Err.number = " & Err.number)
End Sub

call testInStrRevError("abcd", null, 2, 94)
call testInStrRevError(null, "abcd", 2, 94)
call testInStrRevError("abcd", "abcd", null, 94)

Sub TestMid(str, start, len, ex)
    x = Mid(str, start, len)
    Call ok(x = ex, "Mid(" & str & ", " & start & ", " & len & ") = " & x & " expected " & ex)
End Sub

Sub TestMid2(str, start, ex)
    x = Mid(str, start)
    Call ok(x = ex, "Mid(" & str & ", " & start & ") = " & x & " expected " & ex)
End Sub

TestMid "test", 2, 2, "es"
TestMid "test", 2, 4, "est"
TestMid "test", 1, 2, "te"
TestMid "test", 1, 0, ""
TestMid "test", 1, 0, ""
TestMid "test", 5, 2, ""
TestMid2 "test", 1, "test"
TestMid2 "test", 2, "est"
TestMid2 "test", 4, "t"
TestMid2 "test", 5, ""

Sub TestUCase(str, ex)
    x = UCase(str)
    Call ok(x = ex, "UCase(" & str & ") = " & x & " expected " & ex)
End Sub

TestUCase "test", "TEST"
TestUCase "123aBC?", "123ABC?"
TestUCase "", ""
TestUCase 1, "1"
if isEnglishLang then TestUCase true, "TRUE"
TestUCase 0.123, doubleAsString(0.123)
TestUCase Empty, ""
Call ok(getVT(UCase(Null)) = "VT_NULL", "getVT(UCase(Null)) = " & getVT(UCase(Null)))

Sub TestLCase(str, ex)
    x = LCase(str)
    Call ok(x = ex, "LCase(" & str & ") = " & x & " expected " & ex)
End Sub

TestLCase "test", "test"
TestLCase "123aBC?", "123abc?"
TestLCase "", ""
TestLCase 1, "1"
if isEnglishLang then TestLCase true, "true"
TestLCase 0.123, doubleAsString(0.123)
TestLCase Empty, ""
Call ok(getVT(LCase(Null)) = "VT_NULL", "getVT(LCase(Null)) = " & getVT(LCase(Null)))

Call ok(Len("abc") = 3, "Len(abc) = " & Len("abc"))
Call ok(Len("") = 0, "Len() = " & Len(""))
Call ok(Len(1) = 1, "Len(1) = " & Len(1))
Call ok(isNull(Len(null)), "Len(null) = " & Len(null))
Call ok(Len(empty) = 0, "Len(empty) = " & Len(empty))
Call ok(getVT(Len("abc")) = "VT_I4", "getVT(Len(abc)) = " & getVT(Len("abc")))

Call ok(Space(1) = " ", "Space(1) = " & Space(1) & """")
Call ok(Space(0) = "", "Space(0) = " & Space(0) & """")
Call ok(Space(false) = "", "Space(false) = " & Space(false) & """")
Call ok(Space(5) = "     ", "Space(5) = " & Space(5) & """")
Call ok(Space(5.2) = "     ", "Space(5.2) = " & Space(5.2) & """")
Call ok(Space(5.8) = "      ", "Space(5.8) = " & Space(5.8) & """")
Call ok(Space(5.5) = "      ", "Space(5.5) = " & Space(5.5) & """")
Call ok(Space(4.5) = "    ", "Space(4.5) = " & Space(4.5) & """")
Call ok(Space(0.5) = "", "Space(0.5) = " & Space(0.5) & """")
Call ok(Space(1.5) = "  ", "Space(1.5) = " & Space(1.5) & """")
Call ok(Space("1") = " ", "Space(""1"") = " & Space("1") & """")

Sub TestStrReverse(str, ex)
    Call ok(StrReverse(str) = ex, "StrReverse(" & str & ") = " & StrReverse(str))
End Sub

TestStrReverse "test", "tset"
TestStrReverse "", ""
TestStrReverse 123, "321"
if isEnglishLang then TestStrReverse true, "eurT"

Sub TestLeft(str, len, ex)
    Call ok(Left(str, len) = ex, "Left(" & str & ", " & len & ") = " & Left(str, len))
End Sub

TestLeft "test", 2, "te"
TestLeft "test", 5, "test"
TestLeft "test", 0, ""
TestLeft 123, 2, "12"
TestLeft "123456", 1.5, "12"
TestLeft "123456", 2.5, "12"
TestLeft "test", "2", "te"
if isEnglishLang then TestLeft true, 2, "Tr"

Sub TestRight(str, len, ex)
    Call ok(Right(str, len) = ex, "Right(" & str & ", " & len & ") = " & Right(str, len))
End Sub

TestRight "test", 2, "st"
TestRight "test", 5, "test"
TestRight "test", 0, ""
TestRight 123, 2, "23"
if isEnglishLang then TestRight true, 2, "ue"

Sub TestTrim(str, exstr)
    Call ok(Trim(str) = exstr, "Trim(" & str & ") = " & Trim(str))
End Sub

TestTrim "   test    ", "test"
TestTrim "test    ", "test"
TestTrim "   test", "test"
TestTrim "test", "test"
TestTrim "", ""
TestTrim 123, "123"
if isEnglishLang then TestTrim true, "True"

Sub TestLTrim(str, exstr)
    Call ok(LTrim(str) = exstr, "LTrim(" & str & ") = " & LTrim(str))
End Sub

TestLTrim "   test    ", "test    "
TestLTrim "test    ", "test    "
TestLTrim "   test", "test"
TestLTrim "test", "test"
TestLTrim "", ""
TestLTrim 123, "123"
if isEnglishLang then TestLTrim true, "True"

Sub TestRound(val, exval, vt)
    Call ok(Round(val) = exval, "Round(" & val & ") = " & Round(val))
    Call ok(getVT(Round(val)) = vt, "getVT(Round(" & val & ")) = " & getVT(Round(val)))
End Sub

Sub TestRTrim(str, exstr)
    Call ok(RTrim(str) = exstr, "RTrim(" & str & ") = " & RTrim(str))
End Sub

TestRTrim "   test    ", "   test"
TestRTrim "test    ", "test"
TestRTrim "   test", "   test"
TestRTrim "test", "test"
TestRTrim "", ""
TestRTrim 123, "123"
if isEnglishLang then TestRTrim true, "True"

TestRound 3, 3, "VT_I2"
TestRound 3.3, 3, "VT_R8"
TestRound 3.8, 4, "VT_R8"
TestRound 3.5, 4, "VT_R8"
TestRound -3.3, -3, "VT_R8"
TestRound -3.5, -4, "VT_R8"
TestRound "2", 2, "VT_R8"
TestRound true, true, "VT_BOOL"
TestRound false, false, "VT_BOOL"

if isEnglishLang then
    Call ok(WeekDayName(1) = "Sunday", "WeekDayName(1) = " & WeekDayName(1))
    Call ok(WeekDayName(3) = "Tuesday", "WeekDayName(3) = " & WeekDayName(3))
    Call ok(WeekDayName(7) = "Saturday", "WeekDayName(7) = " & WeekDayName(7))
    Call ok(WeekDayName(1.1) = "Sunday", "WeekDayName(1.1) = " & WeekDayName(1.1))
    Call ok(WeekDayName(1, false) = "Sunday", "WeekDayName(1, false) = " & WeekDayName(1, false))
    Call ok(WeekDayName(1, true) = "Sun", "WeekDayName(1, true) = " & WeekDayName(1, true))
    Call ok(WeekDayName(1, 10) = "Sun", "WeekDayName(1, 10) = " & WeekDayName(1, 10))
    Call ok(WeekDayName(1, true, 0) = "Sun", "WeekDayName(1, true, 0) = " & WeekDayName(1, true, 0))
    Call ok(WeekDayName(1, true, 2) = "Mon", "WeekDayName(1, true, 2) = " & WeekDayName(1, true, 2))
    Call ok(WeekDayName(1, true, 2.5) = "Mon", "WeekDayName(1, true, 2.5) = " & WeekDayName(1, true, 2.5))
    Call ok(WeekDayName(1, true, 1.5) = "Mon", "WeekDayName(1, true, 1.5) = " & WeekDayName(1, true, 1.5))
    Call ok(WeekDayName(1, true, 7) = "Sat", "WeekDayName(1, true, 7) = " & WeekDayName(1, true, 7))
    Call ok(WeekDayName(1, true, 7.1) = "Sat", "WeekDayName(1, true, 7.1) = " & WeekDayName(1, true, 7.1))

    Call ok(MonthName(1) = "January", "MonthName(1) = " & MonthName(1))
    Call ok(MonthName(12) = "December", "MonthName(12) = " & MonthName(12))
    Call ok(MonthName(1, 0) = "January", "MonthName(1, 0) = " & MonthName(1, 0))
    Call ok(MonthName(12, false) = "December", "MonthName(12, false) = " & MonthName(12, false))
    Call ok(MonthName(1, 10) = "Jan", "MonthName(1, 10) = " & MonthName(1, 10))
    Call ok(MonthName(12, true) = "Dec", "MonthName(12, true) = " & MonthName(12, true))
end if

Call ok(getVT(Now()) = "VT_DATE", "getVT(Now()) = " & getVT(Now()))

Call ok(vbOKOnly = 0, "vbOKOnly = " & vbOKOnly)
Call ok(getVT(vbOKOnly) = "VT_I2", "getVT(vbOKOnly) = " & getVT(vbOKOnly))
Call ok(vbOKCancel = 1, "vbOKCancel = " & vbOKCancel)
Call ok(getVT(vbOKCancel) = "VT_I2", "getVT(vbOKCancel) = " & getVT(vbOKCancel))
Call ok(vbAbortRetryIgnore = 2, "vbAbortRetryIgnore = " & vbAbortRetryIgnore)
Call ok(getVT(vbAbortRetryIgnore) = "VT_I2", "getVT(vbAbortRetryIgnore) = " & getVT(vbAbortRetryIgnore))
Call ok(vbYesNoCancel = 3, "vbYesNoCancel = " & vbYesNoCancel)
Call ok(getVT(vbYesNoCancel) = "VT_I2", "getVT(vbYesNoCancel) = " & getVT(vbYesNoCancel))
Call ok(vbYesNo = 4, "vbYesNo = " & vbYesNo)
Call ok(getVT(vbYesNo) = "VT_I2", "getVT(vbYesNo) = " & getVT(vbYesNo))
Call ok(vbRetryCancel = 5, "vbRetryCancel = " & vbRetryCancel)
Call ok(getVT(vbRetryCancel) = "VT_I2", "getVT(vbRetryCancel) = " & getVT(vbRetryCancel))

Call ok(vbOK = 1, "vbOK = " & vbOK)
Call ok(getVT(vbOK) = "VT_I2", "getVT(vbOK) = " & getVT(vbOK))
Call ok(vbCancel = 2, "vbCancel = " & vbCancel)
Call ok(getVT(vbCancel) = "VT_I2", "getVT(vbCancel) = " & getVT(vbCancel))
Call ok(vbAbort = 3, "vbAbort = " & vbAbort)
Call ok(getVT(vbAbort) = "VT_I2", "getVT(vbAbort) = " & getVT(vbAbort))
Call ok(vbRetry = 4, "vbRetry = " & vbRetry)
Call ok(getVT(vbRetry) = "VT_I2", "getVT(vbRetry) = " & getVT(vbRetry))
Call ok(vbIgnore = 5, "vbIgnore = " & vbIgnore)
Call ok(getVT(vbIgnore) = "VT_I2", "getVT(vbIgnore) = " & getVT(vbIgnore))
Call ok(vbYes = 6, "vbYes = " & vbYes)
Call ok(getVT(vbYes) = "VT_I2", "getVT(vbYes) = " & getVT(vbYes))
Call ok(vbNo = 7, "vbNo = " & vbNo)
Call ok(getVT(vbNo) = "VT_I2", "getVT(vbNo) = " & getVT(vbNo))

Call ok(CInt(-36.75) = -37, "CInt(-36.75) = " & CInt(-36.75))
Call ok(getVT(CInt(-36.75)) = "VT_I2", "getVT(CInt(-36.75)) = " & getVT(CInt(-36.75)))
Call ok(CInt(-36.50) = -36, "CInt(-36.50) = " & CInt(-36.50))
Call ok(getVT(CInt(-36.50)) = "VT_I2", "getVT(CInt(-36.50)) = " & getVT(CInt(-36.50)))
Call ok(CInt(-36.25) = -36, "CInt(-36.25) = " & CInt(-36.25))
Call ok(getVT(CInt(-36.25)) = "VT_I2", "getVT(CInt(-36.25)) = " & getVT(CInt(-36.25)))
Call ok(CInt(-36) = -36, "CInt(-36) = " & CInt(-36))
Call ok(getVT(CInt(-36)) = "VT_I2", "getVT(CInt(-36)) = " & getVT(CInt(-36)))
Call ok(CInt(0) = 0, "CInt(0) = " & CInt(0))
Call ok(getVT(CInt(0)) = "VT_I2", "getVT(CInt(0)) = " & getVT(CInt(0)))
Call ok(CInt(0.0) = 0, "CInt(0.0) = " & CInt(0))
Call ok(getVT(CInt(0.0)) = "VT_I2", "getVT(CInt(0.0)) = " & getVT(CInt(0.0)))
Call ok(CInt(0.5) = 0, "CInt(0.5) = " & CInt(0))
Call ok(getVT(CInt(0.5)) = "VT_I2", "getVT(CInt(0.5)) = " & getVT(CInt(0.5)))
Call ok(CInt(36) = 36, "CInt(36) = " & CInt(36))
Call ok(getVT(CInt(36)) = "VT_I2", "getVT(CInt(36)) = " & getVT(CInt(36)))
Call ok(CInt(36.25) = 36, "CInt(36.25) = " & CInt(36.25))
Call ok(getVT(CInt(36.25)) = "VT_I2", "getVT(CInt(36.25)) = " & getVT(CInt(36.25)))
Call ok(CInt(36.50) = 36, "CInt(36.50) = " & CInt(36.50))
Call ok(getVT(CInt(36.50)) = "VT_I2", "getVT(CInt(36.50)) = " & getVT(CInt(36.50)))
Call ok(CInt(36.75) = 37, "CInt(36.75) = " & CInt(36.75))
Call ok(getVT(CInt(36.75)) = "VT_I2", "getVT(CInt(36.75)) = " & getVT(CInt(36.75)))


Call ok(CBool(5) = true, "CBool(5) = " & CBool(5))
Call ok(getVT(CBool(5)) = "VT_BOOL", "getVT(CBool(5)) = " & getVT(CBool(5)))
Call ok(CBool(0) = false, "CBool(0) = " & CBool(0))
Call ok(getVT(CBool(0)) = "VT_BOOL", "getVT(CBool(0)) = " & getVT(CBool(0)))
Call ok(CBool(-5) = true, "CBool(-5) = " & CBool(-5))
Call ok(getVT(CBool(-5)) = "VT_BOOL", "getVT(CBool(-5)) = " & getVT(CBool(-5)))

Sub testCBoolError(strings, error_num)
    on error resume next

    Call Err.clear()
    Call CBool(strings)
    Call ok(Err.number = error_num, "Err.number = " & Err.number)
End Sub

Class ValClass
    Public myval

    Public default Property Get defprop
        defprop = myval
    End Property
End Class

Dim MyObject
Set MyObject = New ValClass

Call ok(CBool(Empty) = False, "CBool(Empty) = " & CBool(Empty))
Call ok(getVT(CBool(Empty)) = "VT_BOOL", "getVT(CBool(Empty)) = " & getVT(CBool(Empty)))
Call ok(CBool(1) = True, "CBool(1) = " & CBool(1))
Call ok(getVT(CBool(1)) = "VT_BOOL", "getVT(CBool(1)) = " & getVT(CBool(1)))
Call ok(CBool(0) = False, "CBool(0) = " & CBool(0))
Call ok(getVT(CBool(0)) = "VT_BOOL", "getVT(CBool(0)) = " & getVT(CBool(0)))
Call ok(CBool(-0.56) = True, "CBool(-0.56) = " & CBool(-0.56))
Call ok(getVT(CBool(-0.56)) = "VT_BOOL", "getVT(CBool(-0.56)) = " & getVT(CBool(-0.56)))
Call testCBoolError("", 13)
Call ok(CBool("0") = False, "CBool(""0"") = " & CBool("0"))
Call ok(getVT(CBool("0")) = "VT_BOOL", "getVT(CBool(""0"")) = " & getVT(CBool("0")))
If isEnglishLang Then
    Call ok(CBool("0.1") = True, "CBool(""0.1"") = " & CBool("0.1"))
    Call ok(getVT(CBool("0.1")) = "VT_BOOL", "getVT(CBool(""0.1"")) = " & getVT(CBool("0.1")))
End If
    Call ok(CBool("true") = True, "CBool(""true"") = " & CBool("true"))
Call ok(getVT(CBool("true")) = "VT_BOOL", "getVT(CBool(""true"")) = " & getVT(CBool("true")))
Call ok(CBool("false") = False, "CBool(""false"") = " & CBool("false"))
Call ok(getVT(CBool("false")) = "VT_BOOL", "getVT(CBool(""false"")) = " & getVT(CBool("false")))
Call ok(CBool("TRUE") = True, "CBool(""TRUE"") = " & CBool("TRUE"))
Call ok(getVT(CBool("TRUE")) = "VT_BOOL", "getVT(CBool(""TRUE"")) = " & getVT(CBool("TRUE")))
Call ok(CBool("FALSE") = False, "CBool(""FALSE"") = " & CBool("FALSE"))
Call ok(getVT(CBool("FALSE")) = "VT_BOOL", "getVT(CBool(""FALSE"")) = " & getVT(CBool("FALSE")))
Call ok(CBool("#TRUE#") = True, "CBool(""#TRUE#"") = " & CBool("#TRUE#"))
Call ok(getVT(CBool("#TRUE#")) = "VT_BOOL", "getVT(CBool(""#TRUE#"")) = " & getVT(CBool("#TRUE#")))
Call ok(CBool("#FALSE#") = False, "CBool(""#FALSE#"") = " & CBool("#FALSE#"))
Call ok(getVT(CBool("#FALSE#")) = "VT_BOOL", "getVT(CBool(""#FALSE#"")) = " & getVT(CBool("#FALSE#")))
Call ok(CBool(MyObject) = False, "CBool(MyObject) = " & CBool(MyObject))
Call ok(getVT(CBool(MyObject)) = "VT_BOOL", "getVT(CBool(MyObject)) = " & getVT(CBool(MyObject)))
MyObject.myval = 1
Call ok(CBool(MyObject) = True, "CBool(MyObject) = " & CBool(MyObject))
Call ok(getVT(CBool(MyObject)) = "VT_BOOL", "getVT(CBool(MyObject)) = " & getVT(CBool(MyObject)))
MyObject.myval = 0
Call ok(CBool(MyObject) = False, "CBool(MyObject) = " & CBool(MyObject))
Call ok(getVT(CBool(MyObject)) = "VT_BOOL", "getVT(CBool(MyObject)) = " & getVT(CBool(MyObject)))

Sub testCByteError(strings, error_num1,error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = CByte(strings)
    Call ok(Err.number = error_num1, "Err.number = " & Err.number)

    Call Err.clear()
    Call CByte(strings)
    Call ok(Err.number = error_num2, "Err.number = " & Err.number)
End Sub

Call ok(CByte(Empty) = 0, "CByte(Empty) = " & CByte(Empty))
Call ok(getVT(CByte(Empty)) = "VT_UI1", "getVT(CByte(Empty)) = " & getVT(CByte(Empty)))
Call ok(CByte(255) = 255, "CByte(255) = " & CByte(255))
Call ok(getVT(CByte(255)) = "VT_UI1", "getVT(CByte(255)) = " & getVT(CByte(255)))
Call ok(CByte(255.49) = 255, "CByte(255.49) = " & CByte(255.49))
Call ok(getVT(CByte(255.49)) = "VT_UI1", "getVT(CByte(255.49)) = " & getVT(CByte(255.49)))
Call testCByteError(1, 0, 458)
Call testCByteError("", 13, 13)
Call testCByteError("-1", 6, 6)
Call testCByteError("258", 6, 6)
Call testCByteError("TRUE", 13, 13)
Call testCByteError("FALSE", 13, 13)
Call testCByteError("#TRue#", 13, 13)
Call testCByteError("#fAlSE#", 13, 13)
If isEnglishLang Then
    Call ok(CByte("-0.5") = 0, "CByte(""-0.5"") = " & CByte("-0.5"))
    Call ok(getVT(CByte("-0.5")) = "VT_UI1", "getVT(CByte(""-0.5"")) = " & getVT(CByte("-0.5")))
End If
Call ok(CByte(True) = 255, "CByte(True) = " & CByte(True))
Call ok(getVT(CByte(True)) = "VT_UI1", "getVT(CByte(True)) = " & getVT(CByte(True)))
Call ok(CByte(False) = 0, "CByte(False) = " & CByte(False))
Call ok(getVT(CByte(False)) = "VT_UI1", "getVT(CByte(False)) = " & getVT(CByte(False)))
Call ok(CByte(MyObject) = 0, "CByte(MyObject) = " & CByte(MyObject))
Call ok(getVT(CByte(MyObject)) = "VT_UI1", "getVT(CByte(MyObject)) = " & getVT(CByte(MyObject)))
MyObject.myval = 1
Call ok(CByte(MyObject) = 1, "CByte(MyObject) = " & CByte(MyObject))
Call ok(getVT(CByte(MyObject)) = "VT_UI1", "getVT(CByte(MyObject)) = " & getVT(CByte(MyObject)))
MyObject.myval = 0
Call ok(CByte(MyObject) = 0, "CByte(MyObject) = " & CByte(MyObject))
Call ok(getVT(CByte(MyObject)) = "VT_UI1", "getVT(CByte(MyObject)) = " & getVT(CByte(MyObject)))

Sub testCCurError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = CCur(strings)
    Call ok(Err.number = error_num1, "Err.number = " & Err.number)

    Call Err.clear()
    Call CCur(strings)
    Call ok(Err.number = error_num2, "Err.number = " & Err.number)
End Sub

Call ok(CCur(Empty) = 0, "CCur(Empty) = " & CCur(Empty))
Call ok(getVT(CCur(Empty)) = "VT_CY", "getVT(CCur(Empty)) = " & getVT(CCur(Empty)))
Call ok(CCur(-32768) = -32768, "CCur(-32768) = " & CCur(-32768))
Call ok(getVT(CCur(-32768)) = "VT_CY", "getVT(CCur(-32768)) = " & getVT(CCur(-32768)))
Call ok(CCur(32768) = 32768, "CCur(32768) = " & CCur(32768))
Call ok(getVT(CCur(32768)) = "VT_CY", "getVT(CCur(32768)) = " & getVT(CCur(32768)))
Call ok(CCur(0.000149) = 0.0001, "CCur(0.000149) = " & CCur(0.000149))
Call ok(getVT(CCur(0.000149)) = "VT_CY", "getVT(CCur(0.000149)) = " & getVT(CCur(0.000149)))
Call ok(CCur(2147483647.99) = 2147483647.99, "CCur(2147483647.99) = " & CCur(2147483647.99))
Call ok(getVT(CCur(2147483647.99)) = "VT_CY", "getVT(CCur(2147483647.99)) = " & getVT(CCur(2147483647.99)))
Call ok(CCur("-1") = -1, "CCur(""-1"") = " & CCur("-1"))
Call ok(getVT(CCur("-1")) = "VT_CY", "getVT(CCur(""-1"")) = " & getVT(CCur("-1")))
If isEnglishLang Then
    Call ok(CCur("-0.5") = -0.5, "CCur(""-0.5"") = " & CCur("-0.5"))
    Call ok(getVT(CCur("-0.5")) = "VT_CY", "getVT(CCur(""-0.5"")) = " & getVT(CCur("-0.5")))
End If
Call testCCurError("", 13, 13)
Call testCCurError("-1", 0, 458)
Call testCCurError("TRUE", 13, 13)
Call testCCurError("FALSE", 13, 13)
Call testCCurError("#TRue#", 13, 13)
Call testCCurError("#fAlSE#", 13, 13)
Call testCCurError(1, 0, 458)
Call ok(CCur(True) = -1, "CCur(True) = " & CCur(True))
Call ok(getVT(CCur(True)) = "VT_CY", "getVT(CCur(True)) = " & getVT(CCur(True)))
Call ok(CCur(False) = 0, "CCur(False) = " & CCur(False))
Call ok(getVT(CCur(False)) = "VT_CY", "getVT(CCur(False)) = " & getVT(CCur(False)))
MyObject.myval = 0.1
Call ok(CCur(MyObject) = 0.1, "CCur(MyObject) = " & CCur(MyObject))
Call ok(getVT(CCur(MyObject)) = "VT_CY", "getVT(CCur(MyObject)) = " & getVT(CCur(MyObject)))
MyObject.myval = 0
Call ok(CCur(MyObject) = 0, "CCur(MyObject) = " & CCur(MyObject))
Call ok(getVT(CCur(MyObject)) = "VT_CY", "getVT(CCur(MyObject)) = " & getVT(CCur(MyObject)))

Sub testCDblError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = CDbl(strings)
    Call ok(Err.number = error_num1, "Err.number = " & Err.number)

    Call Err.clear()
    Call CDbl(strings)
    Call ok(Err.number = error_num2, "Err.number = " & Err.number)
End Sub

Call ok(CDbl(Empty) = 0, "CDbl(Empty) = " & CDbl(Empty))
Call ok(getVT(CDbl(Empty)) = "VT_R8", "getVT(CDbl(Empty)) = " & getVT(CDbl(Empty)))
Call ok(CDbl(CByte(0)) = 0, "CDbl(CByte(0)) = " & CDbl(CByte(0)))
Call ok(getVT(CDbl(CCur(0))) = "VT_R8", "getVT(CDbl(CCur(0))) = " & getVT(CDbl(CCur(0))))
Call ok(CDbl(CCur(0)) = 0, "CDbl(CCur(0)) = " & CDbl(CCur(0)))
Call ok(getVT(CDbl(CCur(0))) = "VT_R8", "getVT(CDbl(CCur(0))) = " & getVT(CDbl(CCur(0))))
Call ok(CDbl(0) = 0, "CDbl(0) = " & CDbl(0))
Call ok(getVT(CDbl(0)) = "VT_R8", "getVT(CDbl(0)) = " & getVT(CDbl(0)))
Call ok(CDbl(32768) = 32768, "CDbl(32768) = " & CDbl(32768))
Call ok(getVT(CDbl(32768)) = "VT_R8", "getVT(CDbl(32768)) = " & getVT(CDbl(32768)))
Call ok(CDbl(0.001 * 0.001) = 0.000001, "CDbl(0.001 * 0.001) = " & CDbl(0.001 * 0.001))
Call ok(getVT(CDbl(0.001 * 0.001)) = "VT_R8", "getVT(CDbl(0.001 * 0.001)) = " & getVT(CDbl(0.001 * 0.001)))
Call ok(CDbl("-1") = -1, "CDbl(""-1"") = " & CDbl("-1"))
Call ok(getVT(CDbl("-1")) = "VT_R8", "getVT(CDbl(""-1"")) = " & getVT(CDbl("-1")))
If isEnglishLang Then
    Call ok(CDbl("-0.5") = -0.5, "CDbl(""-0.5"") = " & CDbl("-0.5"))
    Call ok(getVT(CDbl("-0.5")) = "VT_R8", "getVT(CDbl(""-0.5"")) = " & getVT(CDbl("-0.5")))
End If
Call testCDblError("", 13, 13)
Call testCDblError("TRUE", 13, 13)
Call testCDblError("FALSE", 13, 13)
Call testCDblError("#TRue#", 13, 13)
Call testCDblError("#fAlSE#", 13, 13)
Call testCDblError(1, 0, 458)
Call ok(CDbl(True) = -1, "CDbl(True) = " & CDbl(True))
Call ok(getVT(CDbl(True)) = "VT_R8", "getVT(CDbl(True)) = " & getVT(CDbl(True)))
Call ok(CDbl(False) = 0, "CDbl(False) = " & CDbl(False))
Call ok(getVT(CDbl(False)) = "VT_R8", "getVT(CDbl(False)) = " & getVT(CDbl(False)))
MyObject.myval = 0.1
Call ok(CDbl(MyObject) = 0.1, "CDbl(MyObject) = " & CDbl(MyObject))
Call ok(getVT(CDbl(MyObject)) = "VT_R8", "getVT(CDbl(MyObject)) = " & getVT(CDbl(MyObject)))
MyObject.myval = 0
Call ok(CDbl(MyObject) = 0, "CDbl(MyObject) = " & CDbl(MyObject))
Call ok(getVT(CDbl(MyObject)) = "VT_R8", "getVT(CDbl(MyObject)) = " & getVT(CDbl(MyObject)))

Sub testCLngError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = CLng(strings)
    Call ok(Err.number = error_num1, "Err.number = " & Err.number)

    Call Err.clear()
    Call CLng(strings)
    Call ok(Err.number = error_num2, "Err.number = " & Err.number)
End Sub

Call ok(CLng(Empty) = 0, "CLng(Empty) = " & CLng(Empty))
Call ok(getVT(CLng(Empty)) = "VT_I4", "getVT(CLng(Empty)) = " & getVT(CLng(Empty)))
Call ok(CLng(CByte(0)) = 0, "CLng(CByte(0)) = " & CLng(CByte(0)))
Call ok(getVT(CLng(CCur(0))) = "VT_I4", "getVT(CLng(CCur(0))) = " & getVT(CLng(CCur(0))))
Call ok(CLng(CCur(0)) = 0, "CLng(CCur(0)) = " & CLng(CCur(0)))
Call ok(getVT(CLng(CCur(0))) = "VT_I4", "getVT(CLng(CCur(0))) = " & getVT(CLng(CCur(0))))
Call ok(CLng(0) = 0, "CLng(0) = " & CLng(0))
Call ok(getVT(CLng(0)) = "VT_I4", "getVT(CLng(0)) = " & getVT(CLng(0)))
Call ok(CLng(0.49) = 0, "CLng(0.49) = " & CLng(0.49))
Call ok(getVT(CLng(0.49)) = "VT_I4", "getVT(CLng(0.49)) = " & getVT(CLng(0.49)))
Call ok(CLng(0.5) = 0, "CLng(0.5) = " & CLng(0.5))
Call ok(getVT(CLng(0.5)) = "VT_I4", "getVT(CLng(0.5)) = " & getVT(CLng(0.5)))
Call ok(CLng(0.51) = 1, "CLng(0.51) = " & CLng(0.51))
Call ok(getVT(CLng(0.51)) = "VT_I4", "getVT(CLng(0.51)) = " & getVT(CLng(0.51)))
Call ok(CLng(1.49) = 1, "CLng(1.49) = " & CLng(1.49))
Call ok(getVT(CLng(1.49)) = "VT_I4", "getVT(CLng(1.49)) = " & getVT(CLng(1.49)))
Call ok(CLng(1.5) = 2, "CLng(1.5) = " & CLng(1.5))
Call ok(getVT(CLng(1.5)) = "VT_I4", "getVT(CLng(1.5)) = " & getVT(CLng(1.5)))
Call ok(CLng(1.51) = 2, "CLng(1.51) = " & CLng(1.51))
Call ok(getVT(CLng(1.51)) = "VT_I4", "getVT(CLng(1.51)) = " & getVT(CLng(1.51)))
Call ok(CLng("-1") = -1, "CLng(""-1"") = " & CLng("-1"))
Call ok(getVT(CLng("-1")) = "VT_I4", "getVT(CLng(""-1"")) = " & getVT(CLng("-1")))
If isEnglishLang Then
    Call ok(CLng("-0.5") = 0, "CLng(""-0.5"") = " & CLng("-0.5"))
    Call ok(getVT(CLng("-0.5")) = "VT_I4", "getVT(CLng(""-0.5"")) = " & getVT(CLng("-0.5")))
End If
Call testCLngError("", 13, 13)
Call testCLngError("TRUE", 13, 13)
Call testCLngError("FALSE", 13, 13)
Call testCLngError("#TRue#", 13, 13)
Call testCLngError("#fAlSE#", 13, 13)
Call testCLngError(1, 0, 458)
Call ok(CLng(True) = -1, "CLng(True) = " & CLng(True))
Call ok(getVT(CLng(True)) = "VT_I4", "getVT(CLng(True)) = " & getVT(CLng(True)))
Call ok(CLng(False) = 0, "CLng(False) = " & CLng(False))
Call ok(getVT(CLng(False)) = "VT_I4", "getVT(CLng(False)) = " & getVT(CLng(False)))
MyObject.myval = 1
Call ok(CLng(MyObject) = 1, "CLng(MyObject) = " & CLng(MyObject))
Call ok(getVT(CLng(MyObject)) = "VT_I4", "getVT(CLng(MyObject)) = " & getVT(CLng(MyObject)))
MyObject.myval = 0
Call ok(CLng(MyObject) = 0, "CLng(MyObject) = " & CLng(MyObject))
Call ok(getVT(CLng(MyObject)) = "VT_I4", "getVT(CLng(MyObject)) = " & getVT(CLng(MyObject)))

Sub testCIntError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = CInt(strings)
    Call ok(Err.number = error_num1, "Err.number = " & Err.number)

    Call Err.clear()
    Call CInt(strings)
    Call ok(Err.number = error_num2, "Err.number = " & Err.number)
End Sub

Call ok(CInt(Empty) = 0, "CInt(Empty) = " & CInt(Empty))
Call ok(getVT(CInt(Empty)) = "VT_I2", "getVT(CInt(Empty)) = " & getVT(CInt(Empty)))
Call ok(CInt(CByte(0)) = 0, "CInt(CByte(0)) = " & CInt(CByte(0)))
Call ok(getVT(CInt(CByte(0))) = "VT_I2", "getVT(CInt(CByte(0))) = " & getVT(CInt(CByte(0))))
Call ok(CInt(CCur(0)) = 0, "CInt(CCur(0)) = " & CInt(CCur(0)))
Call ok(getVT(CInt(CCur(0))) = "VT_I2", "getVT(CInt(CCur(0))) = " & getVT(CInt(CCur(0))))
Call ok(CInt(0.49) = 0, "CInt(0.49) = " & CInt(0.49))
Call ok(getVT(CInt(0.49)) = "VT_I2", "getVT(CInt(0.49)) = " & getVT(CInt(0.49)))
Call ok(CInt(0.5) = 0, "CInt(0.5) = " & CInt(0.5))
Call ok(getVT(CInt(0.5)) = "VT_I2", "getVT(CInt(0.5)) = " & getVT(CInt(0.5)))
Call ok(CInt(0.51) = 1, "CInt(0.51) = " & CInt(0.51))
Call ok(getVT(CInt(0.51)) = "VT_I2", "getVT(CInt(0.51)) = " & getVT(CInt(0.51)))
Call ok(CInt(1.49) = 1, "CInt(0.49) = " & CInt(0.49))
Call ok(getVT(CInt(0.49)) = "VT_I2", "getVT(CInt(0.49)) = " & getVT(CInt(0.49)))
Call ok(CInt(1.5) = 2, "CInt(1.5) = " & CInt(1.5))
Call ok(getVT(CInt(1.5)) = "VT_I2", "getVT(CInt(1.5)) = " & getVT(CInt(1.5)))
Call ok(CInt(1.51) = 2, "CInt(1.51) = " & CInt(1.51))
Call ok(getVT(CInt(1.51)) = "VT_I2", "getVT(CInt(1.51)) = " & getVT(CInt(1.51)))
Call ok(CInt("-1") = -1, "CInt(""-1"") = " & CInt("-1"))
Call ok(getVT(CInt("-1")) = "VT_I2", "getVT(CInt(""-1"")) = " & getVT(CInt("-1")))
If isEnglishLang Then
    Call ok(CInt("-0.5") = 0, "CInt(""-0.5"") = " & CInt("-0.5"))
    Call ok(getVT(CInt("-0.5")) = "VT_I2", "getVT(CInt(""-0.5"")) = " & getVT(CInt("-0.5")))
End If
Call testCIntError("", 13, 13)
Call testCIntError("-1", 0, 458)
Call testCIntError("TRUE", 13, 13)
Call testCIntError("FALSE", 13, 13)
Call testCIntError("#TRue#", 13, 13)
Call testCIntError("#fAlSE#", 13, 13)
Call testCIntError(1, 0, 458)
Call testCIntError(32767.49, 0, 458)
Call testCIntError(32767.5, 6, 6)
Call testCIntError(-32768.5, 0, 458)
Call testCIntError(-32768.51, 6, 6)
Call ok(CInt(True) = -1, "CInt(True) = " & CInt(True))
Call ok(getVT(CInt(True)) = "VT_I2", "getVT(CInt(True)) = " & getVT(CInt(True)))
Call ok(CInt(False) = 0, "CInt(False) = " & CInt(False))
Call ok(getVT(CInt(False)) = "VT_I2", "getVT(CInt(False)) = " & getVT(CInt(False)))
MyObject.myval = 2.5
Call ok(CInt(MyObject) = 2, "CInt(MyObject) = " & CInt(MyObject))
Call ok(getVT(CInt(MyObject)) = "VT_I2", "getVT(CInt(MyObject)) = " & getVT(CInt(MyObject)))
MyObject.myval = 1.5
Call ok(CInt(MyObject) = 2, "CInt(MyObject) = " & CInt(MyObject))
Call ok(getVT(CInt(MyObject)) = "VT_I2", "getVT(CInt(MyObject)) = " & getVT(CInt(MyObject)))

Sub testCSngError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = CSng(strings)
    Call ok(Err.number = error_num1, "Err.number = " & Err.number)

    Call Err.clear()
    Call CSng(strings)
    Call ok(Err.number = error_num2, "Err.number = " & Err.number)
End Sub

Call ok(CSng(Empty) = 0, "CSng(Empty) = " & CSng(Empty))
Call ok(getVT(CSng(Empty)) = "VT_R4", "getVT(CSng(Empty)) = " & getVT(CSng(Empty)))
Call ok(CSng(CByte(0)) = 0, "CSng(CByte(0)) = " & CSng(CByte(0)))
Call ok(getVT(CSng(CCur(0))) = "VT_R4", "getVT(CSng(CCur(0))) = " & getVT(CSng(CCur(0))))
Call ok(CSng(CCur(0)) = 0, "CSng(CCur(0)) = " & CSng(CCur(0)))
Call ok(getVT(CSng(CCur(0))) = "VT_R4", "getVT(CSng(CCur(0))) = " & getVT(CSng(CCur(0))))
Call ok(CSng(0) = 0, "CSng(0) = " & CSng(0))
Call ok(getVT(CSng(0)) = "VT_R4", "getVT(CSng(0)) = " & getVT(CSng(0)))
Call ok(CSng(32768) = 32768, "CSng(32768) = " & CSng(32768))
Call ok(getVT(CSng(32768)) = "VT_R4", "getVT(CSng(32768)) = " & getVT(CSng(32768)))
Call ok(CSng(0.001 * 0.001) = 0.000001, "CSng(0.001 * 0.001) = " & CSng(0.001 * 0.001))
Call ok(getVT(CSng(0.001 * 0.001)) = "VT_R4", "getVT(CSng(0.001 * 0.001)) = " & getVT(CSng(0.001 * 0.001)))
Call ok(CSng("-1") = -1, "CSng(""-1"") = " & CSng("-1"))
Call ok(getVT(CSng("-1")) = "VT_R4", "getVT(CSng(""-1"")) = " & getVT(CSng("-1")))
If isEnglishLang Then
    Call ok(CSng("-0.5") = -0.5, "CSng(""-0.5"") = " & CSng("-0.5"))
    Call ok(getVT(CSng("-0.5")) = "VT_R4", "getVT(CSng(""-0.5"")) = " & getVT(CSng("-0.5")))
End If
Call testCSngError("", 13, 13)
Call testCSngError("TRUE", 13, 13)
Call testCSngError("FALSE", 13, 13)
Call testCSngError("#TRue#", 13, 13)
Call testCSngError("#fAlSE#", 13, 13)
Call testCSngError(1, 0, 458)
Call ok(CSng(True) = -1, "CSng(True) = " & CSng(True))
Call ok(getVT(CSng(True)) = "VT_R4", "getVT(CSng(True)) = " & getVT(CSng(True)))
Call ok(CSng(False) = 0, "CSng(False) = " & CSng(False))
Call ok(getVT(CSng(False)) = "VT_R4", "getVT(CSng(False)) = " & getVT(CSng(False)))
MyObject.myval = 0.1
Call ok(CSng(MyObject) = 0.1, "CSng(MyObject) = " & CSng(MyObject))
Call ok(getVT(CSng(MyObject)) = "VT_R4", "getVT(CSng(MyObject)) = " & getVT(CSng(MyObject)))
MyObject.myval = 0
Call ok(CSng(MyObject) = 0, "CSng(MyObject) = " & CSng(MyObject))
Call ok(getVT(CSng(MyObject)) = "VT_R4", "getVT(CSng(MyObject)) = " & getVT(CSng(MyObject)))

Call ok(TypeName(Empty) = "Empty", "TypeName(MyEmpty) = " & TypeName(Empty))
Call ok(getVT(TypeName(Empty)) = "VT_BSTR", "getVT(TypeName(Empty)) = " & getVT(TypeName(Empty)))
Call ok(TypeName(Null) = "Null", "TypeName(Null) = " & TypeName(Null))
Call ok(getVT(TypeName(Null)) = "VT_BSTR", "getVT(TypeName(Null)) = " & getVT(TypeName(Null)))
Call ok(TypeName(CByte(255)) = "Byte", "TypeName(CByte(255)) = " & TypeName(CByte(255)))
Call ok(getVT(TypeName(CByte(255))) = "VT_BSTR", "getVT(TypeName(CByte(255))) = " & getVT(TypeName(CByte(255))))
Call ok(TypeName(255) = "Integer", "TypeName(255) = " & TypeName(255))
Call ok(getVT(TypeName(255)) = "VT_BSTR", "getVT(TypeName(255)) = " & getVT(TypeName(255)))
Call ok(TypeName(32768) = "Long", "TypeName(32768) = " & TypeName(32768))
Call ok(getVT(TypeName(32768)) = "VT_BSTR", "getVT(TypeName(32768)) = " & getVT(TypeName(32768)))
Call ok(TypeName(CSng(0.5)) = "Single", "TypeName(CSng(0.5)) = " & TypeName(CSng(0.5)))
Call ok(getVT(TypeName(CSng(0.5))) = "VT_BSTR", "getVT(TypeName(CSng(0.5))) = " & getVT(TypeName(CSng(0.5))))
Call ok(TypeName(-0.5) = "Double", "TypeName(-0.5) = " & TypeName(-0.5))
Call ok(getVT(TypeName(-0.5)) = "VT_BSTR", "getVT(TypeName(-0.5)) = " & getVT(TypeName(-0.5)))
Call ok(TypeName(CCur(0.5)) = "Currency", "TypeName(CCur(0.5)) = " & TypeName(CCur(0.5)))
Call ok(getVT(TypeName(CCur(0.5))) = "VT_BSTR", "getVT(TypeName(CCur(0.5))) = " & getVT(TypeName(CCur(0.5))))
Call ok(TypeName(CStr(0.5)) = "String", "TypeName(CStr(0.5)) = " & TypeName(CStr(0.5)))
Call ok(getVT(TypeName(CStr(0.5))) = "VT_BSTR", "getVT(TypeName(CStr(0.5))) = " & getVT(TypeName(CStr(0.5))))
Call ok(TypeName(True) = "Boolean", "TypeName(True) = " & TypeName(True))
Call ok(getVT(TypeName(True)) = "VT_BSTR", "getVT(TypeName(True)) = " & getVT(TypeName(True)))

Call ok(VarType(Empty) = vbEmpty, "VarType(Empty) = " & VarType(Empty))
Call ok(getVT(VarType(Empty)) = "VT_I2", "getVT(VarType(Empty)) = " & getVT(VarType(Empty)))
Call ok(VarType(Null) = vbNull, "VarType(Null) = " & VarType(Null))
Call ok(getVT(VarType(Null)) = "VT_I2", "getVT(VarType(Null)) = " & getVT(VarType(Null)))
Call ok(VarType(255) = vbInteger, "VarType(255) = " & VarType(255))
Call ok(getVT(VarType(255)) = "VT_I2", "getVT(VarType(255)) = " & getVT(VarType(255)))
Call ok(VarType(32768) = vbLong, "VarType(32768) = " & VarType(32768))
Call ok(getVT(VarType(32768)) = "VT_I2", "getVT(VarType(32768)) = " & getVT(VarType(32768)))
Call ok(VarType(CSng(0.5)) = vbSingle, "VarType(CSng(0.5)) = " & VarType(CSng(0.5)))
Call ok(getVT(VarType(CSng(0.5))) = "VT_I2", "getVT(VarType(CSng(0.5))) = " & getVT(VarType(CSng(0.5))))
Call ok(VarType(-0.5) = vbDouble, "VarType(-0.5) = " & VarType(-0.5))
Call ok(getVT(VarType(-0.5)) = "VT_I2", "getVT(VarType(-0.5)) = " & getVT(VarType(-0.5)))
Call ok(VarType(CCur(0.5)) = vbCurrency, "VarType(CCur(0.5)) = " & VarType(CCur(0.5)))
Call ok(getVT(VarType(CCur(0.5))) = "VT_I2", "getVT(VarType(CCur(0.5))) = " & getVT(VarType(CCur(0.5))))
Call ok(VarType(CStr(0.5)) = vbString, "VarType(CStr(0.5)) = " & VarType(CStr(0.5)))
Call ok(getVT(VarType(CStr(0.5))) = "VT_I2", "getVT(VarType(CStr(0.5))) = " & getVT(VarType(CStr(0.5))))
Call ok(VarType(CBool(0.5)) = vbBoolean, "VarType(CBool(0.5)) = " & VarType(CBool(0.5)))
Call ok(getVT(VarType(CBool(0.5))) = "VT_I2", "getVT(VarType(CBool(0.5))) = " & getVT(VarType(CBool(0.5))))
Call ok(VarType(CByte(255)) = vbByte, "VarType(CByte(255)) = " & VarType(CByte(255)))
Call ok(getVT(VarType(CByte(255))) = "VT_I2", "getVT(VarType(CByte(255))) = " & getVT(VarType(CByte(255))))

Call ok(Sgn(Empty) = 0, "Sgn(MyEmpty) = " & Sgn(Empty))
Call ok(getVT(Sgn(Empty)) = "VT_I2", "getVT(Sgn(MyEmpty)) = " & getVT(Sgn(Empty)))
Call ok(Sgn(0) = 0, "Sgn(0) = " & Sgn(0))
Call ok(getVT(Sgn(0)) = "VT_I2", "getVT(Sgn(0)) = " & getVT(Sgn(0)))
Call ok(Sgn(-32769) = -1, "Sgn(-32769) = " & Sgn(-32769))
Call ok(getVT(Sgn(-32769)) = "VT_I2", "getVT(Sgn(-32769)) = " & getVT(Sgn(-32769)))
Call ok(Sgn(CSng(-0.5)) = -1, "Sgn(CSng(-0.5)) = " & Sgn(CSng(-0.5)))
Call ok(getVT(Sgn(CSng(-0.5))) = "VT_I2", "getVT(Sgn(CSng(-0.5))) = " & getVT(Sgn(CSng(-0.5))))
Call ok(Sgn(0.5) = 1, "Sgn(0.5) = " & Sgn(0.5))
Call ok(getVT(Sgn(0.5)) = "VT_I2", "getVT(Sgn(0.5)) = " & getVT(Sgn(0.5)))
Call ok(Sgn(CCur(-1)) = -1, "Sgn(CCur(-1)) = " & Sgn(CCur(-1)))
Call ok(getVT(Sgn(CCur(-1))) = "VT_I2", "getVT(Sgn(CCur(-1))) = " & getVT(Sgn(CCur(-1))))
Call ok(Sgn(CStr(-1)) = -1, "Sgn(CStr(-1)) = " & Sgn(CStr(-1)))
Call ok(getVT(Sgn(CStr(-1))) = "VT_I2", "getVT(Sgn(CStr(-1))) = " & getVT(Sgn(CStr(-1))))
Call ok(Sgn(False) = 0, "Sgn(False) = " & Sgn(False))
Call ok(getVT(Sgn(False)) = "VT_I2", "getVT(Sgn(False)) = " & getVT(Sgn(False)))
Call ok(Sgn(True) = -1, "Sgn(True) = " & Sgn(True))
Call ok(getVT(Sgn(True)) = "VT_I2", "getVT(Sgn(True)) = " & getVT(Sgn(True)))
Call ok(Sgn(CByte(1)) = 1, "Sgn(CByte(1)) = " & Sgn(CByte(1)))
Call ok(getVT(Sgn(CByte(1))) ="VT_I2", "getVT(Sgn(CByte(1))) = " & getVT(Sgn(CByte(1))))

Sub testSgnError(strings, error_num)
    on error resume next

    Call Err.clear()
    Call Sgn(strings)
    Call ok(Err.number = error_num, "Err.number = " & Err.number)
End Sub

Call testSgnError(Null, 94)

Call ok(Abs(Empty) = 0, "Abs(Empty) = " & Abs(Empty))
Call ok(getVT(Abs(Empty)) = "VT_I2", "getVT(Abs(Empty)) = " & getVT(Abs(Empty)))
Call ok(IsNull(Abs(Null)), "Is Abs(Null) not Null?")
Call ok(getVT(Abs(Null)) = "VT_NULL", "getVT(Abs(Null)) = " & getVT(Abs(Null)))
Call ok(Abs(0) = 0, "Abs(0) = " & Abs(0))
Call ok(getVT(Abs(0)) = "VT_I2", "getVT(Abs(0)) = " & getVT(Abs(0)))
Call ok(Abs(-32769) = 32769, "Abs(-32769) = " & Abs(-32769))
Call ok(getVT(Abs(-32769)) = "VT_I4", "getVT(Abs(-32769)) = " & getVT(Abs(-32769)))
Call ok(Abs(CSng(-0.5)) = 0.5, "Abs(CSng(-0.5)) = " & Abs(CSng(-0.5)))
Call ok(getVT(Abs(CSng(-0.5))) = "VT_R4", "getVT(Abs(CSng(-0.5))) = " & getVT(Abs(CSng(-0.5))))
Call ok(Abs(0.5) = 0.5, "Abs(0.5) = " & Abs(0.5))
Call ok(getVT(Abs(0.5)) = "VT_R8", "getVT(Abs(0.5)) = " & getVT(Abs(0.5)))
Call ok(Abs(CCur(-1)) = 1, "Abs(CCur(-1)) = " & Abs(CCur(-1)))
Call ok(getVT(Abs(CCur(-1))) = "VT_CY", "getVT(Abs(CCur(-1))) = " & getVT(Abs(CCur(-1))))
Call ok(Abs("-1") = 1, "Abs(""-1"") = " & Abs("-1"))
Call ok(getVT(Abs("-1")) = "VT_R8", "getVT(Abs(""-1"")) = " & getVT(Abs("-1")))
Call ok(Abs(False) = 0, "Abs(False) = " & Abs(False))
Call ok(getVT(Abs(False)) = "VT_I2", "getVT(Abs(False)) = " & getVT(Abs(False)))
Call ok(Abs(True) = 1, "Abs(True) = " & Abs(True))
Call ok(getVT(Abs(True)) = "VT_I2", "getVT(Abs(True)) = " & getVT(Abs(True)))
Call ok(Abs(CByte(1)) = 1, "Abs(CByte(1)) = " & Abs(CByte(1)))
Call ok(getVT(Abs(CByte(1))) = "VT_UI1", "getVT(Abs(CByte(1))) = " & getVT(Abs(CByte(1))))

Sub testAbsError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = Abs(strings)
    Call ok(Err.number = error_num1, "Err.number1 = " & Err.number)

    Call Err.clear()
    Call Abs(strings)
    Call ok(Err.number = error_num2, "Err.number2 = " & Err.number)
End Sub

Call testAbsError("strings", 13, 13)
Call testAbsError(-4, 0, 0)

Call ok(ScriptEngine = "VBScript", "Is scriptengine not VBScript?")
Call ok(getVT(ScriptEngine) = "VT_BSTR", "getVT(ScriptEngine) = " & getVT(ScriptEngine))

Call ok(getVT(ScriptEngineBuildVersion) = "VT_I4", "getVT(ScriptEngineBuildVersion) = " & getVT(ScriptEngineBuildVersion))

Call ok(getVT(ScriptEngineMajorVersion) = "VT_I4", "getVT(ScriptEngineMajorVersion) = " & getVT(ScriptEngineMajorVersion))

Call ok(getVT(ScriptEngineMinorVersion) = "VT_I4", "getVT(ScriptEngineMinorVersion) = " & getVT(ScriptEngineMinorVersion))

Call ok(Fix(Empty) = 0, "Fix(Empty) = " & Fix(Empty))
Call ok(getVT(Fix(Empty)) = "VT_I2", "getVT(Fix(Empty)) = " & getVT(Fix(Empty)))
Call ok(Fix(CCur(-0.99)) = 0, "Fix(CCur(-0.99)) = " & Fix(CCur(-0.99)))
Call ok(getVT(Fix(CCur(-0.99))) = "VT_CY", "getVT(Fix(CCur(-0.99))) = " & getVT(Fix(CCur(-0.99))))
Call ok(Fix(1.99) = 1, "Fix(1.99) = " & Fix(1.99))
Call ok(getVT(Fix(1.99)) = "VT_R8", "getVT(Fix(1.99)) = " & getVT(Fix(1.99)))
Call ok(Fix(-1.99) = -1, "Fix(-1.99) = " & Fix(-1.99))
Call ok(getVT(Fix(-1.99)) = "VT_R8", "getVT(Fix(-1.99)) = " & getVT(Fix(-1.99)))
If isEnglishLang Then
    Call ok(Fix("1.99") = 1, "Fix(""1.99"") = " & Fix("1.99"))
    Call ok(getVT(Fix("1.99")) = "VT_R8", "getVT(Fix(""1.99"")) = " & getVT(Fix("1.99")))
    Call ok(Fix("-1.99") = -1, "Fix(""-1.99"") = " & Fix("-1.99"))
    Call ok(getVT(Fix("-1.99")) = "VT_R8", "getVT(Fix(""-1.99"")) = " & getVT(Fix("-1.99")))
End If
Call ok(Fix(True) = -1, "Fix(True) = " & Fix(True))
Call ok(getVT(Fix(True)) = "VT_I2", "getVT(Fix(True)) = " & getVT(Fix(True)))
Call ok(Fix(False) = 0, "Fix(False) = " & Fix(False))
Call ok(getVT(Fix(False)) = "VT_I2", "getVT(Fix(False)) = " & getVT(Fix(False)))
MyObject.myval = 2.5
Call ok(Fix(MyObject) = 2, "Fix(MyObject) = " & Fix(MyObject))
Call ok(getVT(Fix(MyObject)) = "VT_R8", "getVT(Fix(MyObject)) = " & getVT(Fix(MyObject)))
MyObject.myval = -2.5
Call ok(Fix(MyObject) = -2, "Fix(MyObject) = " & Fix(MyObject))
Call ok(getVT(Fix(MyObject)) = "VT_R8", "getVT(Fix(MyObject)) = " & getVT(Fix(MyObject)))

Call ok(Int(Empty) = 0, "Int(Empty) = " & Int(Empty))
Call ok(getVT(Int(Empty)) = "VT_I2", "getVT(Int(Empty)) = " & getVT(Int(Empty)))
Call ok(Int(CCur(-0.99)) = -1, "Int(CCur(-0.99)) = " & Int(CCur(-0.99)))
Call ok(getVT(Int(CCur(-0.99))) = "VT_CY", "getVT(Int(CCur(-0.99))) = " & getVT(Int(CCur(-0.99))))
Call ok(Int(1.99) = 1, "Int(1.99) = " & Int(1.99))
Call ok(getVT(Int(1.99)) = "VT_R8", "getVT(Int(1.99)) = " & getVT(Int(1.99)))
Call ok(Int(-1.99) = -2, "Int(-1.99) = " & Int(-1.99))
Call ok(getVT(Int(-1.99)) = "VT_R8", "getVT(Int(-1.99)) = " & getVT(Int(-1.99)))
If isEnglishLang Then
    Call ok(Int("1.99") = 1, "Int(""1.99"") = " & Int("1.99"))
    Call ok(getVT(Int("1.99")) = "VT_R8", "getVT(Int(""1.99"")) = " & getVT(Int("1.99")))
    Call ok(Int("-1.99") = -2, "Int(""-1.99"") = " & Int("-1.99"))
    Call ok(getVT(Int("-1.99")) = "VT_R8", "getVT(Int(""-1.99"")) = " & getVT(Int("-1.99")))
End If
Call ok(Int(True) = -1, "Int(True) = " & Int(True))
Call ok(getVT(Int(True)) = "VT_I2", "getVT(Int(True)) = " & getVT(Int(True)))
Call ok(Int(False) = 0, "Int(False) = " & Int(False))
Call ok(getVT(Int(False)) = "VT_I2", "getVT(Int(False)) = " & getVT(Int(False)))
MyObject.myval = 2.5
Call ok(Int(MyObject) = 2, "Int(MyObject) = " & Int(MyObject))
Call ok(getVT(Int(MyObject)) = "VT_R8", "getVT(Int(MyObject)) = " & getVT(Int(MyObject)))
MyObject.myval = -2.5
Call ok(Int(MyObject) = -3, "Int(MyObject) = " & Int(MyObject))
Call ok(getVT(Int(MyObject)) = "VT_R8", "getVT(Int(MyObject)) = " & getVT(Int(MyObject)))

Sub testSqrError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = Sqr(strings)
    Call ok(Err.number = error_num1, "Err.number1 = " & Err.number)

    Call Err.clear()
    Call Sqr(strings)
    Call ok(Err.number = error_num2, "Err.number2 = " & Err.number)
End Sub

Call testSqrError(-2, 5, 5)
Call testSqrError(True, 5, 5)

Call ok(Sqr(Empty) = 0, "Sqr(Empty) = " & Sqr(Empty))
Call ok(getVT(Sqr(Empty)) = "VT_R8", "getVT(Sqr(Empty)) = " & getVT(Sqr(Empty)))
Call ok(Sqr(0) = 0, "Sqr(0) = " & Sqr(0))
Call ok(getVT(Sqr(0)) = "VT_R8", "getVT(Sqr(0)) = " & getVT(Sqr(0)))
Call ok(Sqr(1) = 1, "Sqr(1) = " & Sqr(1))
Call ok(getVT(Sqr(1)) = "VT_R8", "getVT(Sqr(1)) = " & getVT(Sqr(1)))
Call ok(Sqr(CSng(121)) = 11, "Sqr(CSng(121)) = " & Sqr(CSng(121)))
Call ok(getVT(Sqr(CSng(121))) = "VT_R8", "getVT(Sqr(CSng(121))) = " & getVT(Sqr(CSng(121))))
Call ok(Sqr(36100) = 190, "Sqr(36100) = " & Sqr(36100))
Call ok(getVT(Sqr(36100)) = "VT_R8", "getVT(Sqr(36100)) = " & getVT(Sqr(36100)))
Call ok(Sqr(CCur(0.0625)) = 0.25, "Sqr(CCur(0.0625)) = " & Sqr(CCur(0.0625)))
Call ok(getVT(Sqr(CCur(0.0625))) = "VT_R8", "getVT(Sqr(CCur(0.0625))) = " & getVT(Sqr(CCur(0.0625))))
Call ok(Sqr("100000000") = 10000, "Sqr(""100000000"") = " & Sqr("100000000"))
Call ok(getVT(Sqr("100000000")) = "VT_R8", "getVT(Sqr(""100000000"")) = " & getVT(Sqr("100000000")))
Call ok(Sqr(False) = 0, "Sqr(False) = " & Sqr(False))
Call ok(getVT(Sqr(False)) = "VT_R8", "getVT(Sqr(False)) = " & getVT(Sqr(False)))
Call ok(Sqr(CByte(225)) = 15, "Sqr(CByte(225)) = " & Sqr(CByte(225)))
Call ok(getVT(Sqr(CByte(225))) = "VT_R8", "getVT(Sqr(CByte(225))) = " & getVT(Sqr(CByte(225))))

Function Approch(func, res)
    If Abs(func - res) < 0.001 Then
        Approch = True
    Else
        Approch = False
    End If
End Function

Const PI = 3.1415926

Call ok(Approch(Cos(Empty), 1), "Cos(Empty) = " & Cos(Empty))
Call ok(getVT(Cos(Empty)) = "VT_R8", "getVT(Cos(Empty)) = " & getVT(Cos(Empty)))
Call ok(Approch(Cos(PI / 6), Sqr(3) / 2), "Cos(PI / 6) = " & Cos(PI / 6))
Call ok(getVT(Cos(PI / 6)) = "VT_R8", "getVT(Cos(PI / 6)) = " & getVT(Cos(PI / 6)))
Call ok(Approch(Cos(CCur(PI / 4)), Sqr(2) / 2), "Cos(CCur(PI / 4)) = " & Cos(CCur(PI / 4)))
Call ok(getVT(Cos(CCur(PI / 4))) = "VT_R8", "getVT(Cos(CCur(PI / 4))) = " & getVT(Cos(CCur(PI / 4))))
Call ok(Approch(Cos(CSng(PI / 3)), 1 / 2), "Cos(CSng(PI / 3)) = " & Cos(CSng(PI / 3)))
Call ok(getVT(Cos(CSng(PI / 3))) = "VT_R8", "getVT(Cos(CSng(PI))) = " & getVT(Cos(CSng(PI))))
Call ok(Approch(Cos(PI / 2), 0), "Cos(0) = " & Cos(PI / 2))
Call ok(getVT(Cos(PI / 2)) = "VT_R8", "getVT(Cos(PI / 2)) = " & getVT(Cos(PI / 2)))
Call ok(Approch(Cos(PI), -1), "Cos(PI) = " & Cos(PI))
Call ok(getVT(Cos(PI)) = "VT_R8", "getVT(Cos(PI)) = " & getVT(Cos(PI)))
Call ok(Approch(Cos(5 * PI / 4), -Sqr(2) / 2), "Cos(5 * PI / 4) = " & Cos(5 * PI / 4))
Call ok(getVT(Cos(5 * PI / 4)) = "VT_R8", "getVT(Cos(5 * PI / 4)) = " & getVT(Cos(5 * PI / 4)))
Call ok(Approch(Cos(3 * PI / 2), 0), "Cos(3 * PI / 2) = " & Cos(3 * PI / 2))
Call ok(getVT(Cos(3 * PI / 2)) = "VT_R8", "getVT(Cos(3 * PI / 2)) = " & getVT(Cos(3 * PI / 2)))
Call ok(Approch(Cos(2 * PI), 1), "Cos(2 * PI) = " & Cos(2 * PI))
Call ok(getVT(Cos(2 * PI)) = "VT_R8", "getVT(Cos(2 * PI)) = " & getVT(Cos(2 * PI)))
Call ok(Approch(Cos("-32768"), 0.3729), "Cos(""-32768"") = " & Cos("-32768"))
Call ok(getVT(Cos("-32768")) = "VT_R8", "getVT(Cos(""-32768"")) = " & getVT(Cos("-32768")))
Call ok(Approch(Cos(False), 1), "Cos(False) = " & Cos(False))
Call ok(getVT(Cos(False)) = "VT_R8", "getVT(Cos(False)) = " & getVT(Cos(False)))
Call ok(Approch(Cos(True), 0.5403), "Cos(True) = " & Cos(True))
Call ok(getVT(Cos(True)) = "VT_R8", "getVT(Cos(True)) = " & getVT(Cos(True)))
Call ok(Approch(Cos(CByte(255)), -0.8623), "Cos(CByte(255)) = " & Cos(CByte(255)))
Call ok(getVT(Cos(CByte(255))) = "VT_R8", "getVT(Cos(CByte(255))) = " & getVT(Cos(CByte(255))))

Call ok(Approch(Sin(Empty), 0), "Sin(Empty) = " & Sin(Empty))
Call ok(getVT(Sin(Empty)) = "VT_R8", "getVT(Sin(Empty)) = " & getVT(Sin(Empty)))
Call ok(Approch(Sin(PI / 6), 1 / 2), "Sin(PI / 6) = " & Sin(PI / 6))
Call ok(getVT(Sin(PI / 6)) = "VT_R8", "getVT(Sin(PI / 6)) = " & getVT(Sin(PI / 6)))
Call ok(Approch(Sin(CCur(PI / 4)), Sqr(2) / 2), "Sin(CCur(PI / 4)) = " & Sin(CCur(PI / 4)))
Call ok(getVT(Sin(CCur(PI / 4))) = "VT_R8", "getVT(Sin(CCur(PI / 4))) = " & getVT(Sin(CCur(PI / 4))))
Call ok(Approch(Sin(CSng(PI / 3)), Sqr(3) / 2), "Sin(CSng(PI / 3)) = " & Sin(CSng(PI / 3)))
Call ok(getVT(Sin(CSng(PI / 3))) = "VT_R8", "getVT(Sin(CSng(PI))) = " & getVT(Sin(CSng(PI))))
Call ok(Approch(Sin(PI / 2), 1), "Sin(0) = " & Sin(PI / 2))
Call ok(getVT(Sin(PI / 2)) = "VT_R8", "getVT(Sin(PI / 2)) = " & getVT(Sin(PI / 2)))
Call ok(Approch(Sin(PI), 0), "Sin(PI) = " & Sin(PI))
Call ok(getVT(Sin(PI)) = "VT_R8", "getVT(Sin(PI)) = " & getVT(Sin(PI)))
Call ok(Approch(Sin(5 * PI / 4), -Sqr(2) / 2), "Sin(5 * PI / 4) = " & Sin(5 * PI / 4))
Call ok(getVT(Sin(5 * PI / 4)) = "VT_R8", "getVT(Sin(5 * PI / 4)) = " & getVT(Sin(5 * PI / 4)))
Call ok(Approch(Sin(3 * PI / 2), -1), "Sin(3 * PI / 2) = " & Sin(3 * PI / 2))
Call ok(getVT(Sin(3 * PI / 2)) = "VT_R8", "getVT(Sin(3 * PI / 2)) = " & getVT(Sin(3 * PI / 2)))
Call ok(Approch(Sin(2 * PI), 0), "Sin(2 * PI) = " & Sin(2 * PI))
Call ok(getVT(Sin(2 * PI)) = "VT_R8", "getVT(Sin(2 * PI)) = " & getVT(Sin(2 * PI)))
Call ok(Approch(Sin("-32768"), -0.9278), "Sin(""-32768"") = " & Sin("-32768"))
Call ok(getVT(Sin("-32768")) = "VT_R8", "getVT(Sin(""-32768"")) = " & getVT(Sin("-32768")))
Call ok(Approch(Sin(False), 0), "Sin(False) = " & Sin(False))
Call ok(getVT(Sin(False)) = "VT_R8", "getVT(Sin(False)) = " & getVT(Sin(False)))
Call ok(Approch(Sin(True), -0.84147), "Sin(True) = " & Sin(True))
Call ok(getVT(Sin(True)) = "VT_R8", "getVT(Sin(True)) = " & getVT(Sin(True)))
Call ok(Approch(Sin(CByte(255)), -0.5063), "Sin(CByte(255)) = " & Sin(CByte(255)))
Call ok(getVT(Sin(CByte(255))) = "VT_R8", "getVT(Sin(CByte(255))) = " & getVT(Sin(CByte(255))))

Call ok(Approch(Tan(Empty), 0), "Tan(Empty) = " & Tan(Empty))
Call ok(getVT(Tan(Empty)) = "VT_R8", "getVT(Tan(Empty)) = " & getVT(Tan(Empty)))
Call ok(Approch(Tan(PI / 6), Sqr(3) / 3), "Tan(PI / 6) = " & Tan(PI / 6))
Call ok(getVT(Tan(PI / 6)) = "VT_R8", "getVT(Tan(PI / 6)) = " & getVT(Tan(PI / 6)))
Call ok(Approch(Tan(CCur(PI / 4)), 1), "Tan(CCur(PI / 4)) = " & Tan(CCur(PI / 4)))
Call ok(getVT(Tan(CCur(PI / 4))) = "VT_R8", "getVT(Tan(CCur(PI / 4))) = " & getVT(Tan(CCur(PI / 4))))
Call ok(Approch(Tan(CSng(PI / 3)), Sqr(3)), "Tan(CSng(PI / 3)) = " & Tan(CSng(PI / 3)))
Call ok(getVT(Tan(CSng(PI / 3))) = "VT_R8", "getVT(Tan(CSng(PI))) = " & getVT(Tan(CSng(PI))))
Call ok(Approch(Tan(PI), 0), "Tan(PI) = " & Tan(PI))
Call ok(getVT(Tan(PI)) = "VT_R8", "getVT(Tan(PI)) = " & getVT(Tan(PI)))
Call ok(Approch(Tan(3 * PI / 4), -1), "Tan(3 * PI / 4) = " & Tan(3 * PI / 4))
Call ok(getVT(Tan(3 * PI / 4)) = "VT_R8", "getVT(Tan(3 * PI / 4)) = " & getVT(Tan(3 * PI / 4)))
Call ok(Approch(Tan(5 * PI / 4), 1), "Tan(5 * PI / 4) = " & Tan(5 * PI / 4))
Call ok(getVT(Tan(5 * PI / 4)) = "VT_R8", "getVT(Tan(5 * PI / 4)) = " & getVT(Tan(5 * PI / 4)))
Call ok(Approch(Tan(2 * PI), 0), "Tan(2 * PI) = " & Tan(2 * PI))
Call ok(getVT(Tan(2 * PI)) = "VT_R8", "getVT(Tan(2 * PI)) = " & getVT(Tan(2 * PI)))
Call ok(Approch(Tan("-32768"), -2.4879), "Tan(""-32768"") = " & Tan("-32768"))
Call ok(getVT(Tan("-32768")) = "VT_R8", "getVT(Tan(""-32768"")) = " & getVT(Tan("-32768")))
Call ok(Approch(Tan(False), 0), "Tan(False) = " & Tan(False))
Call ok(getVT(Tan(False)) = "VT_R8", "getVT(Tan(False)) = " & getVT(Tan(False)))
Call ok(Approch(Tan(True), -1.5574), "Tan(True) = " & Tan(True))
Call ok(getVT(Tan(True)) = "VT_R8", "getVT(Tan(True)) = " & getVT(Tan(True)))
Call ok(Approch(Tan(CByte(255)), 0.5872), "Tan(CByte(255)) = " & Tan(CByte(255)))
Call ok(getVT(Tan(CByte(255))) = "VT_R8", "getVT(Tan(CByte(255))) = " & getVT(Tan(CByte(255))))

Call ok(Approch(Atn(Empty), 0), "Atn(Empty) = " & Atn(Empty))
Call ok(getVT(Atn(Empty)) = "VT_R8", "getVT(Atn(Empty)) = " & getVT(Atn(Empty)))
Call ok(Approch(Atn(Sqr(3) / 3), PI / 6), "Atn(Sqr(3) / 3) = " & Atn(Sqr(3) / 3))
Call ok(getVT(Atn(Sqr(3) / 3)) = "VT_R8", "getVT(Atn(Sqr(3) / 3)) = " & getVT(Atn(Sqr(3) / 3)))
Call ok(Approch(Atn(CCur(1)), PI / 4), "Atn(CCur(1)) = " & Atn(CCur(1)))
Call ok(getVT(Atn(CCur(1))) = "VT_R8", "getVT(Atn(CCur(1))) = " & getVT(Atn(CCur(1))))
Call ok(Approch(Atn(CSng(Sqr(3))), PI / 3), "Atn(CSng(Sqr(3))) = " & Atn(CSng(Sqr(3))))
Call ok(getVT(Atn(CSng(Sqr(3)))) = "VT_R8", "getVT(Atn(CSng(PI))) = " & getVT(Atn(CSng(PI))))
Call ok(Approch(Atn(0), 0), "Atn(0) = " & Atn(0))
Call ok(getVT(Atn(0)) = "VT_R8", "getVT(Atn(0)) = " & getVT(Atn(0)))
Call ok(Approch(Atn(-1), -PI / 4), "Atn(-1) = " & Atn(-1))
Call ok(getVT(Atn(-1)) = "VT_R8", "getVT(Atn(-1)) = " & getVT(Atn(-1)))
Call ok(Approch(Atn("-32768"), -1.5707), "Atn(""-32768"") = " & Atn("-32768"))
Call ok(getVT(Atn("-32768")) = "VT_R8", "getVT(Atn(""-32768"")) = " & getVT(Atn("-32768")))
Call ok(Approch(Atn(False), 0), "Atn(False) = " & Atn(False))
Call ok(getVT(Atn(False)) = "VT_R8", "getVT(Atn(False)) = " & getVT(Atn(False)))
Call ok(Approch(Atn(True), -0.7853), "Atn(True) = " & Atn(True))
Call ok(getVT(Atn(True)) = "VT_R8", "getVT(Atn(True)) = " & getVT(Atn(True)))
Call ok(Approch(Atn(CByte(255)), 1.5668), "Atn(CByte(255)) = " & Atn(CByte(255)))
Call ok(getVT(Atn(CByte(255))) = "VT_R8", "getVT(Atn(CByte(255))) = " & getVT(Atn(CByte(255))))

Call ok(Approch(Exp(Empty), 1), "Exp(Empty) = " & Exp(Empty))
Call ok(getVT(Exp(Empty)) = "VT_R8", "getVT(Exp(Empty)) = " & getVT(Exp(Empty)))
Call ok(Approch(Exp(1), 2.7182), "Exp(1) = " & Exp(1))
Call ok(getVT(Exp(1)) = "VT_R8", "getVT(Exp(1)) = " & getVT(Exp(1)))
Call ok(Approch(Exp(CCur(-1)), 0.3678), "Exp(CCur(-1)) = " & Exp(CCur(-1)))
Call ok(getVT(Exp(CCur(-1))) = "VT_R8", "getVT(Exp(CCur(-1))) = " & getVT(Exp(CCur(-1))))
Call ok(Approch(Exp(CSng(0.5)), 1.6487), "Exp(CSng(0.5)) = " & Exp(CSng(0.5)))
Call ok(getVT(Exp(CSng(0.5))) = "VT_R8", "getVT(Exp(CSng(PI))) = " & getVT(Exp(CSng(PI))))
Call ok(Approch(Exp(-0.5), 0.6065), "Exp(-0.5) = " & Exp(-0.5))
Call ok(getVT(Exp(-0.5)) = "VT_R8", "getVT(Exp(-0.5)) = " & getVT(Exp(-0.5)))
Call ok(Approch(Exp("-2"), 0.1353), "Exp(""-2"") = " & Exp("-2"))
Call ok(getVT(Exp("-2")) = "VT_R8", "getVT(Exp(""-2"")) = " & getVT(Exp("-2")))
Call ok(Approch(Exp(False), 1), "Exp(False) = " & Exp(False))
Call ok(getVT(Exp(False)) = "VT_R8", "getVT(Exp(False)) = " & getVT(Exp(False)))
Call ok(Approch(Exp(True), 0.3678), "Exp(True) = " & Exp(True))
Call ok(getVT(Exp(True)) = "VT_R8", "getVT(Exp(True)) = " & getVT(Exp(True)))
Call ok(Approch(Exp(CByte(2)), 7.389), "Exp(CByte(2)) = " & Exp(CByte(2)))
Call ok(getVT(Exp(CByte(2))) = "VT_R8", "getVT(Exp(CByte(2))) = " & getVT(Exp(CByte(2))))

Sub testLogError(strings, error_num1, error_num2)
    on error resume next
    Dim x

    Call Err.clear()
    x = Log(strings)
    Call ok(Err.number = error_num1, "Err.number1 = " & Err.number)

    Call Err.clear()
    Call Log(strings)
    Call ok(Err.number = error_num2, "Err.number2 = " & Err.number)
End Sub

Call testLogError(0, 5, 5)
Call testLogError(-2, 5, 5)
Call testLogError(False, 5, 5)
Call testLogError(True, 5, 5)
Call ok(Approch(Log(1), 0), "Log(1) = " & Log(1))
Call ok(getVT(Log(1)) = "VT_R8", "getVT(Log(1)) = " & getVT(Log(1)))
Call ok(Approch(Log(CCur(0.5)), -0.6931), "Log(CCur(0.5)) = " & Log(CCur(0.5)))
Call ok(getVT(Log(CCur(0.5))) = "VT_R8", "getVT(Log(CCur(0.5))) = " & getVT(Log(CCur(0.5))))
Call ok(Approch(Log(CSng(2.7182)), 1), "Log(CSng(2.7182)) = " & Log(CSng(2.7182)))
Call ok(getVT(Log(CSng(2.7182))) = "VT_R8", "getVT(Log(CSng(PI))) = " & getVT(Log(CSng(PI))))
Call ok(Approch(Log(32768), 10.3972), "Log(32768) = " & Log(32768))
Call ok(getVT(Log(32768)) = "VT_R8", "getVT(Log(32768)) = " & getVT(Log(32768)))
Call ok(Approch(Log("10"), 2.3025), "Log(""10"") = " & Log("10"))
Call ok(getVT(Log("10")) = "VT_R8", "getVT(Log(""10"")) = " & getVT(Log("10")))
Call ok(Approch(Log(CByte(2)), 0.6931), "Log(CByte(2)) = " & Log(CByte(2)))
Call ok(getVT(Log(CByte(2))) = "VT_R8", "getVT(Log(CByte(2))) = " & getVT(Log(CByte(2))))

Call ok(getVT(Date) = "VT_DATE", "getVT(Date) = " & getVT(Date))
Call ok(getVT(Time) = "VT_DATE", "getVT(Time) = " & getVT(Time))

Sub testRGBError(arg1, arg2, arg3, error_num)
    on error resume next
    Dim x

    Call Err.clear()
    x = RGB(arg1, arg2, arg3)
    Call ok(Err.number = error_num, "Err.number1 = " & Err.number)

    Call Err.clear()
    Call RGB(arg1, arg2, arg3)
    Call ok(Err.number = error_num, "Err.number2 = " & Err.number)
End Sub

Call ok(RGB(0, &h1f&, &hf1&) =  &hf11f00&, "RGB(0, &h1f&, &hf1&) = " & RGB(0, &h1f&, &hf1&))
Call ok(getVT(RGB(0, &h1f&, &hf1&)) = "VT_I4", "getVT(RGB(&hf1&, &h1f&, &hf1&)) = " & getVT(RGB(&hf1&, &h1f&, &hf1&)))
Call ok(RGB(&hef&, &hab&, &hcd&) =  &hcdabef&, "RGB(&hef&, &hab&, &hcd&) = " & RGB(&hef&, &hab&, &hcd&))
Call ok(getVT(RGB(&hef&, &hab&, &hcd&)) = "VT_I4", "getVT(RGB(&hef&, &hab&, &hcd&)) = " & getVT(RGB(&hef&, &hab&, &hcd&)))
Call ok(RGB(&h1&, &h100&, &h111&) =  &hffff01&, "RGB(&h1&, &h100&, &h111&) = " & RGB(&h1&, &h100&, &h111&))
Call ok(getVT(RGB(&h1&, &h100&, &h111&)) = "VT_I4", "getVT(RGB(&h1&, &h100&, &h111&)) = " & getVT(RGB(&h1&, &h100&, &h111&)))
Call testRGBError(-1, &h1e&, &h3b&, 5)
Call testRGBError(&h4d&, -2, &h2f&, 5)

Call ok(getVT(Timer) = "VT_R4", "getVT(Timer) = " & getVT(Timer))

Call reportSuccess()

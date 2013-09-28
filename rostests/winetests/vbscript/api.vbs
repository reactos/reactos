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

Call ok(getVT(hex(null)) = "VT_NULL", "getVT(hex(null)) = " & getVT(hex(null)))
Call ok(getVT(hex(empty)) = "VT_BSTR", "getVT(hex(empty)) = " & getVT(hex(empty)))

x = InStr(1, "abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

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

Call reportSuccess()

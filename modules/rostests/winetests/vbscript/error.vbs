'
' Copyright 2014 Jacek Caban for CodeWeavers
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

const E_TESTERROR = &h80080008&

const VB_E_FORLOOPNOTINITIALIZED = 92
const VB_E_OBJNOTCOLLECTION = 451

const E_NOTIMPL = &h80004001&
const E_NOINTERFACE = &h80004002&
const DISP_E_UNKNOWNINTERFACE = &h80020001&
const DISP_E_MEMBERNOTFOUND = &h80020003&
const DISP_E_PARAMNOTFOUND = &h80020004&
const DISP_E_TYPEMISMATCH = &h80020005&
const DISP_E_UNKNOWNNAME = &h80020006&
const DISP_E_NONAMEDARGS = &h80020007&
const DISP_E_BADVARTYPE = &h80020008&
const DISP_E_OVERFLOW = &h8002000A&
const DISP_E_BADINDEX = &h8002000B&
const DISP_E_UNKNOWNLCID = &h8002000C&
const DISP_E_ARRAYISLOCKED = &h8002000D&
const DISP_E_BADPARAMCOUNT = &h8002000E&
const DISP_E_PARAMNOTOPTIONAL = &h8002000F&
const DISP_E_NOTACOLLECTION = &h80020011&
const TYPE_E_DLLFUNCTIONNOTFOUND = &h8002802F&
const TYPE_E_TYPEMISMATCH = &h80028CA0&
const TYPE_E_OUTOFBOUNDS = &h80028CA1&
const TYPE_E_IOERROR = &h80028CA2&
const TYPE_E_CANTCREATETMPFILE = &h80028CA3&
const STG_E_FILENOTFOUND = &h80030002&
const STG_E_PATHNOTFOUND = &h80030003&
const STG_E_TOOMANYOPENFILES = &h80030004&
const STG_E_ACCESSDENIED = &h80030005&
const STG_E_INSUFFICIENTMEMORY = &h80030008&
const STG_E_NOMOREFILES = &h80030012&
const STG_E_DISKISWRITEPROTECTED = &h80030013&
const STG_E_WRITEFAULT = &h8003001D&
const STG_E_READFAULT = &h8003001E&
const STG_E_SHAREVIOLATION = &h80030020&
const STG_E_LOCKVIOLATION = &h80030021&
const STG_E_FILEALREADYEXISTS = &h80030050&
const STG_E_MEDIUMFULL = &h80030070&
const STG_E_INVALIDNAME = &h800300FC&
const STG_E_INUSE = &h80030100&
const STG_E_NOTCURRENT = &h80030101&
const STG_E_CANTSAVE = &h80030103&
const REGDB_E_CLASSNOTREG = &h80040154&
const MK_E_UNAVAILABLE = &h800401E3&
const MK_E_INVALIDEXTENSION = &h800401E6&
const MK_E_CANTOPENFILE = &h800401EA&
const CO_E_CLASSSTRING = &h800401F3&
const CO_E_APPNOTFOUND = &h800401F5&
const O_E_APPDIDNTREG = &h800401FE&
const E_ACCESSDENIED = &h80070005&
const E_OUTOFMEMORY = &h8007000E&
const E_INVALIDARG = &h80070057&
const RPC_S_SERVER_UNAVAILABLE = &h800706BA&
const CO_E_SERVER_EXEC_FAILURE = &h80080005&

call ok(Err.Number = 0, "Err.Number = " & Err.Number)
call ok(getVT(Err.Number) = "VT_I4", "getVT(Err.Number) = " & getVT(Err.Number))

class emptyclass
end class

class propclass
    public prop
end class

dim calledFunc

sub returnTrue
    calledFunc = true
    returnTrue = true
end sub

sub testThrow
    on error resume next

    dim x, y

    call throwInt(1000)
    call ok(Err.Number = 0, "Err.Number = " & Err.Number)

    call throwInt(E_TESTERROR)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call throwInt(1000)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    call ok(Err.Number = 0, "Err.Number = " & Err.Number)

    x = 6
    calledFunc = false
    x = throwInt(E_TESTERROR) and returnTrue()
    call ok(x = 6, "x = " & x)
    call ok(not calledFunc, "calledFunc = " & calledFunc)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    if false and throwInt(E_TESTERROR) then
        x = true
    else
        call ok(false, "unexpected if else branch on throw")
    end if
    call ok(x, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    if throwInt(E_TESTERROR) then x = true
    call ok(x, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    if false then
        call ok(false, "unexpected if else branch on throw")
    elseif throwInt(E_TESTERROR) then
        x = true
    else
        call ok(false, "unexpected if else branch on throw")
    end if
    call ok(x, "elseif branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    if true then
        call throwInt(E_TESTERROR)
    else
        call ok(false, "unexpected if else branch on throw")
    end if
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    do while throwInt(E_TESTERROR)
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        x = true
        exit do
    loop
    call ok(x, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = 0
    call Err.clear()
    do
        x = x+1
        call ok(Err.Number = 0, "Err.Number = " & Err.Number)
    loop while throwInt(E_TESTERROR)
    call ok(x = 1, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = 0
    call Err.clear()
    do
        x = x+1
        call ok(Err.Number = 0, "Err.Number = " & Err.Number)
    loop until throwInt(E_TESTERROR)
    call ok(x = 1, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    x = 0
    while x < 2
        x = x+1
        call throwInt(E_TESTERROR)
    wend
    call ok(x = 2, "x = " & x)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    x = 2
    y = 0
    for each x in throwInt(E_TESTERROR)
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        y = y+1
    next
    call ok(x = 2, "x = " & x)
    call ok(y = 1, "y = " & y)
    call todo_wine_ok(Err.Number = VB_E_OBJNOTCOLLECTION, "Err.Number = " & Err.Number)

    Err.clear()
    y = 0
    x = 6
    for x = throwInt(E_TESTERROR) to 100
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        call ok(x = 6, "x = " & x)
        y = y+1
    next
    call ok(y = 1, "y = " & y)
    call ok(x = 6, "x = " & x)
    call todo_wine_ok(Err.Number = VB_E_FORLOOPNOTINITIALIZED, "Err.Number = " & Err.Number)

    Err.clear()
    y = 0
    x = 6
    for x = 100 to throwInt(E_TESTERROR)
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        call todo_wine_ok(x = 6, "x = " & x)
        y = y+1
    next
    call ok(y = 1, "y = " & y)
    call todo_wine_ok(x = 6, "x = " & x)
    call todo_wine_ok(Err.Number = VB_E_FORLOOPNOTINITIALIZED, "Err.Number = " & Err.Number)

    select case throwInt(E_TESTERROR)
    case true
         call ok(false, "unexpected case true")
    case false
         call ok(false, "unexpected case false")
    case empty
         call ok(false, "unexpected case empty")
    case else
         call ok(false, "unexpected case else")
    end select
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    select case false
    case true
         call ok(false, "unexpected case true")
    case throwInt(E_TESTERROR)
         x = true
    case else
         call ok(false, "unexpected case else")
    end select
    call ok(x, "case not executed")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    'Exception in non-trivial stack context
    for x = 1 to 1
        for each y in collectionObj
            select case 3
            case 1
                call ok(false, "unexpected case")
            case throwInt(E_TESTERROR)
                exit for
            case 2
                call ok(false, "unexpected case")
            end select
        next
    next
end sub

call testThrow

dim x

sub testOnError(resumeNext)
    if resumeNext then
        on error resume next
    else
        on error goto 0
    end if
    x = 1
    throwInt(E_TESTERROR)
    x = 2
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
end sub

sub callTestOnError(resumeNext)
    on error resume next
    call testOnError(resumeNext)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
end sub

x = 0
call callTestOnError(true)
call ok(x = 2, "x = " & x)

x = 0
call callTestOnError(false)
call ok(x = 1, "x = " & x)

sub testOnErrorClear()
    on error resume next
    call ok(Err.Number = 0, "Err.Number = " & Err.Number)
    throwInt(E_TESTERROR)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    on error goto 0
    call ok(Err.Number = 0, "Err.Number = " & Err.Number)
    x = "ok"
end sub

call testOnErrorClear()
call ok(x = "ok", "testOnErrorClear failed")

sub testForEachError()
    on error resume next

    dim x, y, z
    y = false
    z = false
    for each x in empty
        y = true
    next
    z = true
    call ok(y, "for each not executed")
    call ok(z, "line after next not executed")
    call todo_wine_ok(Err.Number = VB_E_OBJNOTCOLLECTION, "Err.Number = " & Err.Number)
end sub

call testForEachError()

sub testWithError()
    on error resume next
    dim x

    err.clear
    x = false
    with throwInt(E_TESTERROR)
        ok Err.Number = E_TESTERROR, "Err.Number = " & Err.Number
        x = true
    end with
    ok x, "with statement body not executed"

    err.clear
    x = false
    with throwInt(E_TESTERROR)
        x = true
        .prop = 1
        todo_wine_ok Err.Number = 424, "Err.Number = " & Err.Number
    end with
    ok x, "with statement body not executed"

    err.clear
    x = false
    with empty
        .prop = 1
        todo_wine_ok Err.Number = 424, "Err.Number = " & Err.Number
        x = true
    end with
    ok x, "with statement body not executed"
end sub

sub testWithError2()
    on error resume next
    dim x

    err.clear
    x = false
    with new emptyclass
        .prop = 1
        ok Err.Number = 438, "Err.Number = " & Err.Number
        x = true
    end with
    ok x, "with statement body not executed"

    'dot expression can reference only inner-most with statement
    err.clear
    x = false
    with new propclass
        with new emptyclass
            .prop = 1
            ok Err.Number = 438, "Err.Number = " & Err.Number
            x = true
        end with
    end with
    ok x, "with statement body not executed"

    err.clear
    .prop
    ok Err.Number = 505, "Err.Number = " & Err.Number & " description """ & err.description & """"
end sub

call testWithError()
call testWithError2()

sub testHresMap(hres, code)
    on error resume next

    call Err.Clear()
    call throwInt(hres)
    call ok(Err.Number = code, "throw(" & hex(hres) & ") Err.Number = " & Err.Number)
end sub

testHresMap E_NOTIMPL, 445
testHresMap E_NOINTERFACE, 430
testHresMap DISP_E_UNKNOWNINTERFACE, 438
testHresMap DISP_E_MEMBERNOTFOUND, 438
testHresMap DISP_E_PARAMNOTFOUND, 448
testHresMap DISP_E_TYPEMISMATCH, 13
testHresMap DISP_E_UNKNOWNNAME, 438
testHresMap DISP_E_NONAMEDARGS, 446
testHresMap DISP_E_BADVARTYPE, 458
testHresMap DISP_E_OVERFLOW, 6
testHresMap DISP_E_BADINDEX, 9
testHresMap DISP_E_UNKNOWNLCID, 447
testHresMap DISP_E_ARRAYISLOCKED, 10
testHresMap DISP_E_BADPARAMCOUNT, 450
testHresMap DISP_E_PARAMNOTOPTIONAL, 449
testHresMap DISP_E_NOTACOLLECTION, 451
testHresMap TYPE_E_DLLFUNCTIONNOTFOUND, 453
testHresMap TYPE_E_TYPEMISMATCH, 13
testHresMap TYPE_E_OUTOFBOUNDS, 9
testHresMap TYPE_E_IOERROR, 57
testHresMap TYPE_E_CANTCREATETMPFILE, 322
testHresMap STG_E_FILENOTFOUND, 432
testHresMap STG_E_PATHNOTFOUND, 76
testHresMap STG_E_TOOMANYOPENFILES, 67
testHresMap STG_E_ACCESSDENIED, 70
testHresMap STG_E_INSUFFICIENTMEMORY, 7
testHresMap STG_E_NOMOREFILES, 67
testHresMap STG_E_DISKISWRITEPROTECTED, 70
testHresMap STG_E_WRITEFAULT, 57
testHresMap STG_E_READFAULT, 57
testHresMap STG_E_SHAREVIOLATION, 75
testHresMap STG_E_LOCKVIOLATION, 70
testHresMap STG_E_FILEALREADYEXISTS, 58
testHresMap STG_E_MEDIUMFULL, 61
testHresMap STG_E_INVALIDNAME, 53
testHresMap STG_E_INUSE, 70
testHresMap STG_E_NOTCURRENT, 70
testHresMap STG_E_CANTSAVE, 57
testHresMap REGDB_E_CLASSNOTREG, 429
testHresMap MK_E_UNAVAILABLE, 429
testHresMap MK_E_INVALIDEXTENSION, 432
testHresMap MK_E_CANTOPENFILE, 432
testHresMap CO_E_CLASSSTRING, 429
testHresMap CO_E_APPNOTFOUND, 429
testHresMap O_E_APPDIDNTREG, 429
testHresMap E_ACCESSDENIED, 70
testHresMap E_OUTOFMEMORY, 7
testHresMap E_INVALIDARG, 5
testHresMap RPC_S_SERVER_UNAVAILABLE, 462
testHresMap CO_E_SERVER_EXEC_FAILURE, 429

sub testVBErrorCodes()
    on error resume next

    Err.clear()
    throwInt(&h800a00aa&)
    call ok(Err.number = 170, "Err.number = " & Err.number)

    Err.clear()
    throwInt(&h800a10aa&)
    call ok(Err.number = 4266, "Err.number = " & Err.number)
end sub

call testVBErrorCodes

on error resume next

throwWithDesc
ok err.number = &hdeadbeef&, "err.number = " & hex(err.number)
ok err.description = "test", "err.description = " & err.description
ok err.helpcontext = 10, "err.helpcontext = " & err.helpcontext
ok err.helpfile = "test.chm", "err.helpfile = " & err.helpfile

throwWithDesc = 1
ok err.number = &hdeadbeef&, "err.number = " & hex(err.number)
ok err.description = "test", "err.description = " & err.description
ok err.helpcontext = 10, "err.helpcontext = " & err.helpcontext
ok err.helpfile = "test.chm", "err.helpfile = " & err.helpfile

on error goto 0

call reportSuccess()

/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for _tcstoul
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define WIN32_NO_STATUS
#include <wine/test.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

#define StartSeh()              ExceptionStatus = STATUS_SUCCESS; _SEH2_TRY {
#define EndSeh(ExpectedStatus)  } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { ExceptionStatus = _SEH2_GetExceptionCode(); } _SEH2_END; ok(ExceptionStatus == ExpectedStatus, "Exception %lx, expected %lx\n", ExceptionStatus, ExpectedStatus)

#define ok_ulong(expression, result) \
    do { \
        unsigned long _value = (expression); \
        ok(_value == (result), "Wrong value for '%s', expected: " #result " (%lu), got: %lu\n", \
           #expression, (unsigned long)(result), _value); \
    } while (0)

START_TEST(tcstoul)
{
    NTSTATUS ExceptionStatus;
    ULONG Result;
    _TCHAR Dummy;
    _TCHAR *End;
    struct
    {
        const _TCHAR *Input;
        int ExpectedLength0;
        ULONG Expected0;
        int ExpectedLength10;
        ULONG Expected10;
    } Tests[] =
    {
        { _T(""),            0,   0,  0,   0 },
        { _T(" "),           0,   0,  0,   0 },
        { _T(" 0"),          2,   0,  2,   0 },
        { _T("0 "),          1,   0,  1,   0 },
        { _T("0"),           1,   0,  1,   0 },
        { _T("1"),           1,   1,  1,   1 },
        { _T("10"),          2,  10,  2,  10 },
        { _T("01"),          2,   1,  2,   1 },
        { _T("010"),         3,   8,  3,  10 },
        { _T("08"),          1,   0,  2,   8 },
        { _T("008"),         2,   0,  3,   8 },
        { _T("-1"),          2,  -1,  2,  -1 },
        { _T("+1"),          2,   1,  2,   1 },
        { _T("--1"),         0,   0,  0,   0 },
        { _T("++1"),         0,   0,  0,   0 },
        { _T("0a"),          1,   0,  1,   0 },
        { _T("0x"),          0,   0,  1,   0 },
        { _T("0x0"),         3,   0,  1,   0 },
        { _T("0xFFFFFFFF"), 10,  -1,  1,   0 },
        { _T("0xFFFFFFFFF"),11,  -1,  1,   0 },
        { _T("4294967295"), 10,  -1, 10,  -1 },
        { _T("4294967296"), 10,  -1, 10,  -1 },
        { _T("4294967297"), 10,  -1, 10,  -1 },
        { _T("42949672951"),11,  -1, 11,  -1 },
    };
    const INT TestCount = sizeof(Tests) / sizeof(Tests[0]);
    INT i;

    StartSeh()
        Result = _tcstoul(NULL, NULL, 0);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        Result = _tcstoul(_T(""), NULL, 0);
        ok_ulong(Result, 0);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Result = _tcstoul(_T("1"), NULL, 0);
        ok_ulong(Result, 1);
    EndSeh(STATUS_SUCCESS);

    Result = _tcstoul(_T("1"), &End, 0);
    ok_ulong(Result, 1);

    for (i = 0; i < TestCount; i++)
    {
#ifdef _UNICODE
        trace("%d: '%ls'\n", i, Tests[i].Input);
#else
        trace("%d: '%s'\n", i, Tests[i].Input);
#endif
        End = &Dummy;
        Result = _tcstoul(Tests[i].Input, &End, 0);
        ok_ulong(Result, Tests[i].Expected0);
        ok_ptr(End, Tests[i].Input + Tests[i].ExpectedLength0);

        End = &Dummy;
        Result = _tcstoul(Tests[i].Input, &End, 10);
        ok_ulong(Result, Tests[i].Expected10);
        ok_ptr(End, Tests[i].Input + Tests[i].ExpectedLength10);
    }
}

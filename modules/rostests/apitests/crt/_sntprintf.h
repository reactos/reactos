/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for _sntprintf
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define WIN32_NO_STATUS
#include <wine/test.h>
#include <stdio.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

#define StartSeh()              ExceptionStatus = STATUS_SUCCESS; _SEH2_TRY {
#define EndSeh(ExpectedStatus)  } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { ExceptionStatus = _SEH2_GetExceptionCode(); } _SEH2_END; ok(ExceptionStatus == ExpectedStatus, "Exception %lx, expected %lx\n", ExceptionStatus, ExpectedStatus)

/* winetest_platform is "windows" for us, so broken() doesn't do what it should :( */
#undef broken
#define broken(x) 0

START_TEST(_sntprintf)
{
    NTSTATUS ExceptionStatus;
    _TCHAR Buffer[128];
    size_t BufferSize = sizeof(Buffer) / sizeof(Buffer[0]);
    int Result;

    StartSeh()
        Result = _sntprintf(NULL, 0, _T("Hello"));
#ifdef TEST_CRTDLL
        ok_int(Result, -1);
#else
        ok_int(Result, 5);
#endif
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Result = _sntprintf(NULL, 1, _T("Hello"));
        ok(Result == 5 ||
           broken(Result == -1) /* Win7 */, "Result = %d\n", Result);
#if defined(_UNICODE) || defined(TEST_CRTDLL)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        FillMemory(Buffer, sizeof(Buffer), 0x55);
        Result = _sntprintf(Buffer, BufferSize, _T("Hello"));
        ok_int(Result, 5);
        ok(Buffer[0] == _T('H'), "\n");
        ok(Buffer[1] == _T('e'), "\n");
        ok(Buffer[2] == _T('l'), "\n");
        ok(Buffer[3] == _T('l'), "\n");
        ok(Buffer[4] == _T('o'), "\n");
        ok(Buffer[5] == _T('\0'), "\n");
        ok(Buffer[6] == (_TCHAR)0x5555, "\n");
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        FillMemory(Buffer, sizeof(Buffer), 0x55);
        Result = _sntprintf(Buffer, 5, _T("Hello"));
        ok_int(Result, 5);
        ok(Buffer[0] == _T('H'), "\n");
        ok(Buffer[1] == _T('e'), "\n");
        ok(Buffer[2] == _T('l'), "\n");
        ok(Buffer[3] == _T('l'), "\n");
        ok(Buffer[4] == _T('o'), "\n");
        ok(Buffer[5] == (_TCHAR)0x5555, "\n");
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        FillMemory(Buffer, sizeof(Buffer), 0x55);
        Result = _sntprintf(Buffer, 1, _T("Hello"));
        ok_int(Result, -1);
        ok(Buffer[0] == _T('H'), "\n");
        ok(Buffer[1] == (_TCHAR)0x5555, "\n");
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        FillMemory(Buffer, sizeof(Buffer), 0x55);
        Result = _sntprintf(Buffer, 0, _T("Hello"));
        ok_int(Result, -1);
        ok(Buffer[0] == (_TCHAR)0x5555, "\n");
    EndSeh(STATUS_SUCCESS);
}

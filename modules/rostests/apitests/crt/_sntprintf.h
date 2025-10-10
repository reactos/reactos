/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for _sntprintf
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <stdio.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

#ifndef TEST_STATIC_CRT

static PFN_sntprintf p_sntprintf;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_sntprintf = (PFN_sntprintf)GetProcAddress(hdll, str_sntprintf);
    ok(p_sntprintf != NULL, "Failed to load %s from %s\n", str_sntprintf, TEST_DLL_NAME);
    return (p_sntprintf != NULL);
}

#undef _sntprintf
#define _sntprintf p_sntprintf

#endif // !TEST_STATIC_CRT

/* winetest_platform is "windows" for us, so broken() doesn't do what it should :( */
#undef broken
#define broken(x) 0

START_TEST(_sntprintf)
{
    _TCHAR Buffer[128];
    size_t BufferSize = sizeof(Buffer) / sizeof(Buffer[0]);
    int Result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because %s is not available\n", str_sntprintf);
        return;
    }
#endif

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
        ok_int(Result, (GetNTVersion() >= _WIN32_WINNT_VISTA) ? -1 : 5);
#if defined(TEST_CRTDLL)
    EndSeh(STATUS_ACCESS_VIOLATION);
#elif defined(_UNICODE)
    EndSeh((GetNTVersion() >= _WIN32_WINNT_VISTA) ? 0 : STATUS_ACCESS_VIOLATION);
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

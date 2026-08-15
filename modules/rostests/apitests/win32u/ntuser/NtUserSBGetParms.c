/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtUserSBGetParms
 * COPYRIGHT:   Copyright 2026 Max Korostil <mrmks04@yandex.ru>
 */

#include <apitest.h>
#include <wincon.h>

#include "../win32nt.h"

START_TEST(NtUserSBGetParms)
{
    BOOL ret;
    DWORD error;
    SBDATA sbData;
    SCROLLINFO si;
    HWND hWnd = GetConsoleWindow();
    
    // 1
    SetLastError(0);
    ret = NtUserSBGetParms(hWnd, SB_CTL, &sbData, &si);
    error = GetLastError();

    ok_int(ret, TRUE);
    ok_int(error, ERROR_SUCCESS);

    // 2
    SetLastError(0);
    ret = NtUserSBGetParms(hWnd, SB_CTL, &sbData, NULL);
    error = GetLastError();

    ok_int(ret, FALSE);
    ok_int(error, ERROR_NOACCESS);

    // 3
    SetLastError(0);
    ret = NtUserSBGetParms(hWnd, SB_CTL, &sbData, (LPSCROLLINFO)(ULONG_PTR)0xDEADBEEF);
    error = GetLastError();

    ok_int(ret, FALSE);
    ok_int(error, ERROR_NOACCESS);
    
    // 4
    SetLastError(0);
    ret = NtUserSBGetParms(hWnd, SB_CTL, NULL, &si);
    error = GetLastError();

    ok_int(ret, FALSE);
    ok_int(error, ERROR_NOACCESS);

    // 5
    SetLastError(0);
    ret = NtUserSBGetParms(hWnd, SB_CTL, (PSBDATA)(ULONG_PTR)0xDEADBEEF, &si);
    error = GetLastError();

    ok_int(ret, FALSE);
    ok_int(error, ERROR_NOACCESS);

    // 6
    SetLastError(0);
    ret = NtUserSBGetParms(NULL, SB_CTL, &sbData, &si);
    error = GetLastError();

    ok_int(ret, FALSE);
    ok_int(error, ERROR_INVALID_WINDOW_HANDLE);

    // 7
    ret = NtUserSBGetParms(hWnd, SB_CTL, &sbData, (LPSCROLLINFO)MAXULONG_PTR);
    error = GetLastError();

    ok_int(ret, FALSE);
    ok_int(error, ERROR_NOACCESS);

    // 8
    ret = NtUserSBGetParms(hWnd, SB_CTL, (PSBDATA)(ULONG_PTR)MAXULONG_PTR, &si);
    error = GetLastError();

    ok_int(ret, FALSE);
    ok_int(error, ERROR_NOACCESS);
}

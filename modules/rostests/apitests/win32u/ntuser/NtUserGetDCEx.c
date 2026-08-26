/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtUserGetDCEx
 * COPYRIGHT:   Copyright 2026 Max Korostil (mrmks04@yandex.ru)
 */
 
#include <apitest.h>
#include <wincon.h>

#include "../win32nt.h"
#include "undocuser.h"

#define DCX_VALID (DCX_WINDOW | DCX_CACHE | DCX_NORESETATTRS | DCX_CLIPCHILDREN | \
                   DCX_CLIPSIBLINGS | DCX_PARENTCLIP | DCX_EXCLUDERGN | DCX_INTERSECTRGN | \
                   DCX_EXCLUDEUPDATE | DCX_INTERSECTUPDATE | DCX_LOCKWINDOWUPDATE | DCX_VALIDATE | \
                   DCX_USESTYLE | DCX_KEEPCLIPRGN)

START_TEST(NtUserGetDCEx)
{
    HDC hdc;
    DWORD error;
    HRGN region;
    HWND hWnd = GetConsoleWindow();

    // 1 - Invalid flags
    SetLastError(0);
    hdc = NtUserGetDCEx(NULL, (HRGN)1, 0xFFFFFFFF);
    error = GetLastError();

    ok_hdl(hdc, NULL);
    ok_int(error, ERROR_INVALID_PARAMETER);

    // 2 - Invalid region
    SetLastError(0);
    hdc = NtUserGetDCEx(NULL, (HRGN)1, DCX_WINDOW);
    error = GetLastError();

    ok_hdl(hdc, NULL);
    ok_int(error, ERROR_INVALID_PARAMETER);

    // 3 - HDC for desktop
    SetLastError(0);
    hdc = NtUserGetDCEx(NULL, (HRGN)0, DCX_WINDOW);
    error = GetLastError();

    ok_int(error, ERROR_SUCCESS);
    ok(hdc != NULL, "hdc is NULL\n");
    if (hdc != NULL)
    {
        ReleaseDC(NULL, hdc);
    }

    // 4 - HDC for window with circle region
    SetLastError(0);
    region = CreateEllipticRgn(0, 0, 500, 500);
    ok(region != NULL, "region is NULL\n");
    
    if (region != NULL)
    {
        hdc = NtUserGetDCEx(hWnd, region, DCX_INTERSECTRGN);
        error = GetLastError();

        ok_int(error, ERROR_SUCCESS);
        ok(hdc != NULL, "hdc is NULL\n");
    
        if (hdc != NULL)
        {
            ReleaseDC(hWnd, hdc);
        }
    }

    // 5 - Check valid flags
    for(int i = 0; i < 32; ++i)
    {
        DWORD flag = (1 << i);
        SetLastError(0);
        hdc = NtUserGetDCEx(NULL, (HANDLE)0, flag);
        error = GetLastError();
        
        if (flag & DCX_VALID)
        {
            ok_int(error, ERROR_SUCCESS);
            ok(hdc != NULL, "hdc is NULL. Flag 0x%X\n", flag);
            if (hdc != NULL)
                ReleaseDC(NULL, hdc);
        }
        else
        {
            ok(error == ERROR_INVALID_PARAMETER, "Flag 0x%X\n", flag);
            ok_hdl(hdc, NULL);
        }        
    }    
}

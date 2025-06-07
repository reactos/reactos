/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for TrackPopupMenuEx
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <versionhelpers.h>

#define VALID_TPM_FLAGS ( \
    TPM_LAYOUTRTL | TPM_NOANIMATION | TPM_VERNEGANIMATION | TPM_VERPOSANIMATION | \
    TPM_HORNEGANIMATION | TPM_HORPOSANIMATION | TPM_RETURNCMD | \
    TPM_NONOTIFY | TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_VCENTERALIGN | \
    TPM_RIGHTALIGN | TPM_CENTERALIGN | TPM_RIGHTBUTTON | TPM_RECURSE \
)

#ifndef TPM_WORKAREA
#define TPM_WORKAREA 0x10000
#endif

static VOID
TEST_InvalidFlags(VOID)
{
    HWND hwnd = GetDesktopWindow();
    HMENU hMenu = CreatePopupMenu();
    BOOL ret;

    ret = AppendMenuW(hMenu, MF_STRING, 100, L"(Dummy)");
    ok_int(ret, TRUE);

    INT iBit;
    UINT uFlags;
    for (iBit = 0; iBit < sizeof(DWORD) * CHAR_BIT; ++iBit)
    {
        uFlags = (1 << iBit);
        if (uFlags & ~VALID_TPM_FLAGS)
        {
            SetLastError(0xBEEFCAFE);
            ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, NULL);
            ok_int(ret, FALSE);
            if (uFlags == TPM_WORKAREA && IsWindows7OrGreater())
                ok_err(ERROR_INVALID_PARAMETER);
            else
                ok_err(ERROR_INVALID_FLAGS);
        }
    }

    DestroyMenu(hMenu);
}

static VOID
TEST_InvalidSize(VOID)
{
    HWND hwnd = GetDesktopWindow();
    HMENU hMenu = CreatePopupMenu();
    TPMPARAMS params;
    UINT uFlags = TPM_RIGHTBUTTON;
    BOOL ret;

    ZeroMemory(&params, sizeof(params));

    ret = AppendMenuW(hMenu, MF_STRING, 100, L"(Dummy)");
    ok_int(ret, TRUE);

    SetLastError(0xBEEFCAFE);
    params.cbSize = 0;
    ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, &params);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    params.cbSize = sizeof(params) - 1;
    ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, &params);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    params.cbSize = sizeof(params) + 1;
    ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, &params);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    DestroyMenu(hMenu);
}

START_TEST(TrackPopupMenuEx)
{
    TEST_InvalidFlags();
    TEST_InvalidSize();
}

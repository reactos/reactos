/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for MapWin32ErrorToSTG
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>

typedef HRESULT (WINAPI *FN_MapWin32ErrorToSTG)(HRESULT);
static FN_MapWin32ErrorToSTG s_fnMapWin32ErrorToSTG = NULL;

START_TEST(MapWin32ErrorToSTG)
{
    HRESULT hr;
    HINSTANCE hSHLWAPI;

    hSHLWAPI = GetModuleHandleA("shlwapi");
    s_fnMapWin32ErrorToSTG = (FN_MapWin32ErrorToSTG)
        GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(485));
    if (!s_fnMapWin32ErrorToSTG)
    {
        skip("MapWin32ErrorToSTG not found\n");
        return;
    }

    hr = s_fnMapWin32ErrorToSTG(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
    ok_long(hr, STG_E_ACCESSDENIED);

    hr = s_fnMapWin32ErrorToSTG(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    ok_long(hr, STG_E_FILENOTFOUND);

    hr = s_fnMapWin32ErrorToSTG(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
    ok_long(hr, STG_E_FILENOTFOUND);

    hr = s_fnMapWin32ErrorToSTG(HRESULT_FROM_WIN32(ERROR_FILE_EXISTS));
    ok_long(hr, STG_E_FILEALREADYEXISTS);

    hr = s_fnMapWin32ErrorToSTG(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
    ok_long(hr, STG_E_FILEALREADYEXISTS);

    hr = s_fnMapWin32ErrorToSTG(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    ok_long(hr, HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
}

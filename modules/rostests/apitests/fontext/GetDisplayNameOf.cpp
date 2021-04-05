/*
 * PROJECT:     fontext_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for fontext GetDisplayNameOf behavior
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <ntndk.h>
#include <atlbase.h>
#include <atlcom.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellutils.h>
#include "wine/test.h"

static HRESULT Initialize(CComPtr<IShellFolder>& spFolder)
{
    WCHAR Path[MAX_PATH] = {0};
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, Path);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return hr;

    CComPtr<IShellFolder> desktopFolder;
    hr = SHGetDesktopFolder(&desktopFolder);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return hr;

    CComHeapPtr<ITEMIDLIST> pidl;
    hr = desktopFolder->ParseDisplayName(NULL, NULL, Path, NULL, &pidl, NULL);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return hr;

    hr = desktopFolder->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &spFolder));
    ok_hex(hr, S_OK);
    return hr;
}

static void Test_GetDisplayNameOf(CComPtr<IShellFolder>& spFolder)
{
    CComPtr<IEnumIDList> fontsEnum;
    HRESULT hr = spFolder->EnumObjects(NULL, SHCONTF_NONFOLDERS, &fontsEnum);

    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return;

    CComHeapPtr<ITEMIDLIST> fontPidl;
    ULONG fetched = 0;
    hr = fontsEnum->Next(1, &fontPidl, &fetched);
    STRRET strret;
    hr = spFolder->GetDisplayNameOf(fontPidl, SHGDN_FORPARSING, &strret);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return;

    WCHAR Buf[MAX_PATH];
    hr = StrRetToBufW(&strret, fontPidl, Buf, _countof(Buf));
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return;

    // On 2k3 where this is not a custom IShellFolder, it will return something like:
    // 'C:\\WINDOWS\\Fonts\\arial.ttf'
    // On Vista+ this results in something like:
    // 'C:\\Windows\\Fonts\\System Bold'
    BOOL bRelative = PathIsRelativeW(Buf);
    trace("Path: %s\n", wine_dbgstr_w(Buf));
    ok(bRelative == FALSE, "Path not relative? (%s)\n", wine_dbgstr_w(Buf));
}


START_TEST(GetDisplayNameOf)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    {
        CComPtr<IShellFolder> spFolder;
        HRESULT hr = Initialize(spFolder);
        if (SUCCEEDED(hr))
        {
            Test_GetDisplayNameOf(spFolder);
        }
    }

    CoUninitialize();
}

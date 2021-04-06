/*
 * PROJECT:     fontext_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for fontext GetDisplayNameOf behavior
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
#include <versionhelpers.h>
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
    if (FAILED(hr))
    {
        skip("BindToObject failed\n");
        return hr;
    }

    return hr;
}

static void
Test_GetDisplayNameOf(CComPtr<IShellFolder>& spFolder,
                      DWORD dwFlags, LPCWSTR text, BOOL fRelative)
{
    CComPtr<IEnumIDList> fontsEnum;
    HRESULT hr = spFolder->EnumObjects(NULL, SHCONTF_NONFOLDERS, &fontsEnum);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
    {
        skip("EnumObjects failed\n");
        return;
    }

    BOOL bFound = FALSE;
    for (;;)
    {
        CComHeapPtr<ITEMIDLIST> fontPidl;
        ULONG fetched = 0;
        hr = fontsEnum->Next(1, &fontPidl, &fetched);
        if (FAILED(hr) || hr == S_FALSE)
            break;

        STRRET strret;
        hr = spFolder->GetDisplayNameOf(fontPidl, dwFlags, &strret);
        if (FAILED(hr))
            continue;

        WCHAR Buf[MAX_PATH];
        hr = StrRetToBufW(&strret, fontPidl, Buf, _countof(Buf));
        if (FAILED(hr))
            continue;

        trace("Path: %s\n", wine_dbgstr_w(Buf));
        if (lstrcmpiW(text, Buf) == 0)
        {
            bFound = TRUE;
            ok_int(PathIsRelativeW(Buf), fRelative);
            break;
        }
    }

    ok_int(bFound, TRUE);
}


START_TEST(GetDisplayNameOf)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+\n");
        return;
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    {
        CComPtr<IShellFolder> spFolder;
        HRESULT hr = Initialize(spFolder);
        if (SUCCEEDED(hr))
        {
            WCHAR szPath[MAX_PATH];
            SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, szPath);
            PathAppendW(szPath, L"arial.ttf");

            trace("SHGDN_NORMAL\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_NORMAL, L"arial.ttf", TRUE);

            trace("SHGDN_INFOLDER\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_INFOLDER, L"arial.ttf", TRUE);

            trace("SHGDN_FORPARSING\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_FORPARSING, szPath, FALSE);

            trace("SHGDN_INFOLDER | SHGDN_FORPARSING\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_INFOLDER | SHGDN_FORPARSING, L"arial.ttf", TRUE);

            trace("SHGDN_FORADDRESSBAR\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_FORADDRESSBAR, L"arial.ttf", TRUE);

            trace("SHGDN_INFOLDER | SHGDN_FORADDRESSBAR\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_INFOLDER | SHGDN_FORADDRESSBAR, L"arial.ttf", TRUE);

            trace("SHGDN_FORPARSING | SHGDN_FORADDRESSBAR\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szPath, FALSE);

            trace("SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR\n");
            Test_GetDisplayNameOf(spFolder, SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, L"arial.ttf", TRUE);
        }
    }

    CoUninitialize();
}

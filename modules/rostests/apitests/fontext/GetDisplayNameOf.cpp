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
#include <versionhelpers.h>
#include "wine/test.h"

static const CLSID CLSID_FontExt = { 0xBD84B380, 0x8CA2, 0x1069, { 0xAB, 0x1D, 0x08, 0x00, 0x09, 0x48, 0xF5, 0x34 } };
static BOOL g_bFontFolderWithShellView = FALSE;
static BOOL g_bVistaWorkaround = FALSE;

static HRESULT GetDisplayName(CComPtr<IShellFolder>& spFolder, LPCITEMIDLIST pidlRelative, SHGDNF uFlags, WCHAR Buf[MAX_PATH])
{
    STRRET strret;
    HRESULT hr = spFolder->GetDisplayNameOf(pidlRelative, uFlags, &strret);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return hr;

    hr = StrRetToBufW(&strret, pidlRelative, Buf, MAX_PATH);
    ok_hex(hr, S_OK);
    return hr;
}

static HRESULT Initialize(CComPtr<IShellFolder>& spFolder, WCHAR FolderName[MAX_PATH])
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
        return hr;


    CComPtr<IShellFolder> psfParent;
    LPCITEMIDLIST pidlRelative = NULL;
    hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psfParent), &pidlRelative);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return hr;

    return GetDisplayName(psfParent, pidlRelative, SHGDN_NORMAL, FolderName);
}

static void Test_GetDisplayNameOf(CComPtr<IShellFolder>& spFolder, const WCHAR* FolderName)
{
    CComPtr<IEnumIDList> fontsEnum;
    HRESULT hr = spFolder->EnumObjects(NULL, SHCONTF_NONFOLDERS, &fontsEnum);

    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return;

    CComHeapPtr<ITEMIDLIST> fontPidl;

    // Get the first item from the folder
    ULONG fetched = 0;
    hr = fontsEnum->Next(1, &fontPidl, &fetched);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return;


    WCHAR Buf[MAX_PATH], *Ptr;
    if (FAILED(GetDisplayName(spFolder, fontPidl, SHGDN_FORPARSING, Buf)))
        return;

    // On 2k3 where this is not a custom IShellFolder, it will return something like:
    // Same for Vista??
    // 'C:\\WINDOWS\\Fonts\\arial.ttf'
    // On 7+ this results is:
    // 'C:\\Windows\\Fonts\\System Bold'
    BOOL bRelative = PathIsRelativeW(Buf);
    Ptr = PathFindNextComponentW(Buf);
    trace("Path: %s\n", wine_dbgstr_w(Buf));
    ok(bRelative == FALSE, "Path not absolute? (%s)\n", wine_dbgstr_w(Buf));
    ok(Ptr != (Buf + wcslen(Buf)), "Did not find a separator in '%s'!\n", wine_dbgstr_w(Buf));

    // Expected 'arial.ttf' (2k3), 'Arial' (Vista) or 'System Bold' (7+), 'System \0388\03bd\03c4\03bf\03bd\03b7 \03b3\03c1\03b1\03c6\03ae' in greek
    if (FAILED(GetDisplayName(spFolder, fontPidl, SHGDN_INFOLDER, Buf)))
        return;

    bRelative = PathIsRelativeW(Buf);
    Ptr = PathFindNextComponentW(Buf);
    trace("Path: %s\n", wine_dbgstr_w(Buf));
    ok(bRelative != FALSE, "Path not relative? (%s)\n", wine_dbgstr_w(Buf));
    ok(Ptr == (Buf + wcslen(Buf)), "Found a separator in '%s'!\n", wine_dbgstr_w(Buf));

    // Expected 'arial.ttf' (2k3) or 'Arial' (Vista), 'System Bold' (7+), 'System \0388\03bd\03c4\03bf\03bd\03b7 \03b3\03c1\03b1\03c6\03ae' in greek
    if (FAILED(GetDisplayName(spFolder, fontPidl, SHGDN_NORMAL, Buf)))
        return;

    bRelative = PathIsRelativeW(Buf);
    Ptr = PathFindNextComponentW(Buf);
    trace("Path: %s\n", wine_dbgstr_w(Buf));
    ok(bRelative != FALSE, "Path not relative? (%s)\n", wine_dbgstr_w(Buf));
    ok(Ptr == (Buf + wcslen(Buf)), "Found a separator in '%s'!\n", wine_dbgstr_w(Buf));

    // Expected 'arial.ttf' (2k3), 'C:\\WINDOWS\\Fonts\\arial.ttf' (Vista), 'System Bold' (7+)
    if (FAILED(GetDisplayName(spFolder, fontPidl, SHGDN_INFOLDER | SHGDN_FORPARSING, Buf)))
        return;

    bRelative = PathIsRelativeW(Buf);
    Ptr = PathFindNextComponentW(Buf);
    trace("Path: %s\n", wine_dbgstr_w(Buf));
    if (g_bVistaWorkaround)
    {
        // Vista is the odd one here
        ok(bRelative == FALSE, "Path not absolute? (%s)\n", wine_dbgstr_w(Buf));
        ok(Ptr != (Buf + wcslen(Buf)), "Did not find a separator in '%s'!\n", wine_dbgstr_w(Buf));
    }
    else
    {
        ok(bRelative != FALSE, "Path not relative? (%s)\n", wine_dbgstr_w(Buf));
        ok(Ptr == (Buf + wcslen(Buf)), "Found a separator in '%s'!\n", wine_dbgstr_w(Buf));
    }

    // Expected 'arial.ttf' (2k3), 'Arial' (Vista) or 'Fonts\\System Bold' (7+), 'Fonts\\System \0388\03bd\03c4\03bf\03bd\03b7 \03b3\03c1\03b1\03c6\03ae' in greek
    if (FAILED(GetDisplayName(spFolder, fontPidl, SHGDN_FORADDRESSBAR, Buf)))
        return;

    bRelative = PathIsRelativeW(Buf);
    Ptr = PathFindNextComponentW(Buf);
    trace("Path: %s\n", wine_dbgstr_w(Buf));
    ok(bRelative != FALSE, "Path not relative? (%s)\n", wine_dbgstr_w(Buf));

    // 2k3 does not have a custom IShellFolder, so there this weird behavior does not exist:
    // (And vista's version is ???)
    if (g_bFontFolderWithShellView || g_bVistaWorkaround)
    {
        ok(Ptr == (Buf + wcslen(Buf)), "Found a separator in '%s'!\n", wine_dbgstr_w(Buf));
    }
    else
    {
        // For some reason, there is 'Fonts\\' in front of the fontname...
        ok(Ptr != (Buf + wcslen(Buf)), "Did not find a separator in '%s'!\n", wine_dbgstr_w(Buf));
        ok(!_wcsnicmp(FolderName, Buf, wcslen(FolderName)), "Result (%s) does not start with fonts folder (%s)\n",
            wine_dbgstr_w(Buf), wine_dbgstr_w(FolderName));
    }
}


START_TEST(GetDisplayNameOf)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Detect if this is an old (2k3) style fontext, or a new one.
    // The old one has an IShellView, the new one an IShellFolder
    {
        CComPtr<IShellView> spView;
        HRESULT hr = CoCreateInstance(CLSID_FontExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellView, &spView));
        g_bFontFolderWithShellView = SUCCEEDED(hr);
    }

    g_bVistaWorkaround = IsWindowsVistaOrGreater() && !IsWindows7OrGreater();

    trace("Has shellview: %d, Vista: %d\n", g_bFontFolderWithShellView, g_bVistaWorkaround);
    {
        CComPtr<IShellFolder> spFolder;
        WCHAR FolderName[MAX_PATH];
        HRESULT hr = Initialize(spFolder, FolderName);
        if (SUCCEEDED(hr))
        {
            Test_GetDisplayNameOf(spFolder, FolderName);
        }
    }

    CoUninitialize();
}

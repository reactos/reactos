/*
 * PROJECT:     fontext_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for fontext shell extension behavior
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <ntndk.h>
#include <atlbase.h>
#include <atlcom.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shellutils.h>
#include "wine/test.h"

const CLSID CLSID_FontExt = { 0xBD84B380, 0x8CA2, 0x1069, { 0xAB, 0x1D, 0x08, 0x00, 0x09, 0x48, 0xF5, 0x34 } };
static DWORD g_WinVersion;


static HRESULT Initialize(CComPtr<IPersistFolder>& spFolder, LPCWSTR Path)
{
    CComHeapPtr<ITEMIDLIST> pidl;
    CComPtr<IShellFolder> spDesktop;

    HRESULT hr = SHGetDesktopFolder(&spDesktop);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return hr;

    DWORD Attributes = 0, chEaten = 0;
    hr = spDesktop->ParseDisplayName(NULL, NULL, (LPOLESTR)Path, &chEaten, &pidl, &Attributes);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return hr;

    return spFolder->Initialize(pidl);
}


static void CreateObjectsFromPersistFolder()
{
    WCHAR Path[MAX_PATH] = { 0 };

    CComPtr<IPersistFolder> spFolder;
    HRESULT hr = CoCreateInstance(CLSID_FontExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPersistFolder, &spFolder));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, Path);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return;

    // Initializing this in another folder fails
    hr = Initialize(spFolder, Path);
    ok_hex(hr, E_FAIL);

    hr = SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, Path);
    ok_hex(hr, S_OK);
    if (FAILED(hr))
        return;

    // Initializing it in the font folder works
    hr = Initialize(spFolder, Path);
    ok_hex(hr, S_OK);

    // For ros we do not implement the ShellView, but go directly to the ShellFolder.
    // So we detect this special case
    if (g_WinVersion < _WIN32_WINNT_VISTA)
    {
        CComPtr<IShellFolder> spShellFolder;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IShellFolder, &spShellFolder));

        if (SUCCEEDED(hr))
        {
            trace("Got IShellFolder on < Vista, faking 0x601\n");
            g_WinVersion = _WIN32_WINNT_WIN7;
        }
    }

    if (g_WinVersion < _WIN32_WINNT_VISTA)
    {
        // A view is present
        CComPtr<IShellView> spView;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IShellView, &spView));
        ok_hex(hr, S_OK);

        // No shell folder
        CComPtr<IShellFolder> spShellFolder;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IShellFolder, &spShellFolder));
        ok_hex(hr, E_NOINTERFACE);

        // Ask the view:
        if (spView)
        {
            CComPtr<IObjectWithSite> spObjectWithSite;
            hr = spView->QueryInterface(IID_PPV_ARG(IObjectWithSite, &spObjectWithSite));
            ok_hex(hr, E_NOINTERFACE);

            CComPtr<IInternetSecurityManager> spISM;
            hr = spView->QueryInterface(IID_PPV_ARG(IInternetSecurityManager, &spISM));
            ok_hex(hr, E_NOINTERFACE);
        }


        CComPtr<IDropTarget> spDropTarget;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IDropTarget, &spDropTarget));
        ok_hex(hr, S_OK);

        CComPtr<IExtractIconW> spExtractIcon;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IExtractIconW, &spExtractIcon));
        ok_hex(hr, E_NOINTERFACE);
    }
    else
    {
        // Here we have a shell folder
        CComPtr<IShellFolder> spShellFolder;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IShellFolder, &spShellFolder));
        ok_hex(hr, S_OK);

        // But no view anymore
        CComPtr<IShellView> spView;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IShellView, &spView));
        ok_hex(hr, E_NOINTERFACE);
        spView.Release();


        CComPtr<IDropTarget> spDropTarget;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IDropTarget, &spDropTarget));
        ok_hex(hr, E_NOINTERFACE);

        CComPtr<IExtractIconW> spExtractIcon;
        hr = spFolder->QueryInterface(IID_PPV_ARG(IExtractIconW, &spExtractIcon));
        ok_hex(hr, E_NOINTERFACE);
    }
}


static void CreateDropTarget()
{
    CComPtr<IDropTarget> spDropTarget;
    HRESULT hr = CoCreateInstance(CLSID_FontExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IDropTarget, &spDropTarget));
    ok_hex(hr, E_NOINTERFACE);
}

static void CreateExtractIcon()
{
    if (g_WinVersion < _WIN32_WINNT_VISTA)
    {
        CComPtr<IExtractIconA> spExtractIconA;
        HRESULT hr = CoCreateInstance(CLSID_FontExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IExtractIconA, &spExtractIconA));
        ok_hex(hr, S_OK);

        CComPtr<IExtractIconW> spExtractIconW;
        hr = CoCreateInstance(CLSID_FontExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IExtractIconW, &spExtractIconW));
        ok_hex(hr, S_OK);
    }
    else
    {
        CComPtr<IExtractIconA> spExtractIconA;
        HRESULT hr = CoCreateInstance(CLSID_FontExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IExtractIconA, &spExtractIconA));
        ok_hex(hr, E_NOINTERFACE);

        CComPtr<IExtractIconW> spExtractIconW;
        hr = CoCreateInstance(CLSID_FontExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IExtractIconW, &spExtractIconW));
        ok_hex(hr, E_NOINTERFACE);
    }
}



START_TEST(shellext)
{
    RTL_OSVERSIONINFOEXW rtlinfo = {0};

    rtlinfo.dwOSVersionInfoSize = sizeof(rtlinfo);
    RtlGetVersion((PRTL_OSVERSIONINFOW)&rtlinfo);
    g_WinVersion = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;

    trace("g_WinVersion=0x%x\n", g_WinVersion);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    CreateObjectsFromPersistFolder();
    CreateDropTarget();
    CreateExtractIcon();

    CoUninitialize();
}

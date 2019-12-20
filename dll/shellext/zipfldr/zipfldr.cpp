/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     zipfldr entrypoint
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

HMODULE g_hModule = NULL;
LONG g_ModuleRefCnt = 0;

#include <initguid.h>

DEFINE_GUID(CLSID_ZipFolderStorageHandler, 0xe88dcce0, 0xb7b3, 0x11d1, 0xa9, 0xf0, 0x00, 0xaa, 0x00, 0x60, 0xfa, 0x31);
DEFINE_GUID(CLSID_ZipFolderSendTo,         0x888dca60, 0xfc0a, 0x11cf, 0x8f, 0x0f, 0x00, 0xc0, 0x4f, 0xd7, 0xd0, 0x62);
DEFINE_GUID(CLSID_ZipFolderContextMenu,    0xb8cdcb65, 0xb1bf, 0x4b42, 0x94, 0x28, 0x1d, 0xfd, 0xb7, 0xee, 0x92, 0xaf);
DEFINE_GUID(CLSID_ZipFolderRightDragHandler,0xbd472f60, 0x27fa, 0x11cf, 0xb8, 0xb4, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(CLSID_ZipFolderDropHandler,    0xed9d80b9, 0xd157, 0x457b, 0x91, 0x92, 0x0e, 0x72, 0x80, 0x31, 0x3b, 0xf0);

/* IExplorerCommand: Extract All */
DEFINE_GUID(CLSID_ZipFolderExtractAllCommand, 0xc3d9647b, 0x8fd9, 0x4ee6, 0x8b, 0xc7, 0x82, 0x7, 0x80, 0x9, 0x10, 0x5a);


class CZipFldrModule : public CComModule
{
public:
};


BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ZipFolderStorageHandler, CZipFolder)
    OBJECT_ENTRY(CLSID_ZipFolderContextMenu, CZipFolder)
    OBJECT_ENTRY(CLSID_ZipFolderSendTo, CSendToZip)
END_OBJECT_MAP()

CZipFldrModule gModule;


#include "minizip/ioapi.h"
#include "minizip/iowin32.h"

zlib_filefunc64_def g_FFunc;

static void init_zlib()
{
    fill_win32_filefunc64W(&g_FFunc);
}

EXTERN_C
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        g_hModule = hInstance;
        gModule.Init(ObjectMap, hInstance, NULL);
        init_zlib();
        break;
    }

    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    if (g_ModuleRefCnt)
        return S_FALSE;
    return gModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    HRESULT hr;

    hr = gModule.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_ZIPFLDR, TRUE, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    HRESULT hr;

    hr = gModule.DllUnregisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_ZIPFLDR, FALSE, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

EXTERN_C
BOOL WINAPI
RouteTheCall(
    IN HWND hWndOwner,
    IN HINSTANCE hInstance,
    IN LPCSTR lpStringArg,
    IN INT Show)
{
    CStringW path = lpStringArg;
    PathRemoveBlanksW(path.GetBuffer());
    path.ReleaseBuffer();
    path = L"\"" + path + L"\"";
    ShellExecuteW(NULL, L"open", L"explorer.exe", path.GetString(), NULL, SW_SHOWNORMAL);
    return TRUE;
}

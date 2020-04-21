/*
 * PROJECT:     sendmail
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     DeskLink implementation
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(sendmail);

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_DeskLinkDropHandler, CDeskLinkDropHandler)
END_OBJECT_MAP()

CComModule gModule;
LONG g_ModuleRefCnt = 0;
HINSTANCE g_hModule;

HRESULT CreateSendToDeskLink(LPCWSTR pszSendTo);
HRESULT CreateSendToMyDocuments(LPCWSTR pszSendTo);

STDAPI DllCanUnloadNow(void)
{
    if (g_ModuleRefCnt)
        return S_FALSE;
    return gModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("CLSID:%s,IID:%s\n", wine_dbgstr_guid(&rclsid), wine_dbgstr_guid(&riid));

    HRESULT hr = gModule.DllGetClassObject(rclsid, riid, ppv);

    TRACE("-- pointer to class factory: %p\n", *ppv);

    return hr;
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr = gModule.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    WCHAR szSendTo[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szSendTo, CSIDL_SENDTO, TRUE);
    CreateSendToMyDocuments(szSendTo);
    CreateSendToDeskLink(szSendTo);
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = gModule.DllUnregisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT
CreateShellLink(
    LPCWSTR pszLinkPath,
    LPCWSTR pszTargetPath OPTIONAL,
    LPCITEMIDLIST pidlTarget OPTIONAL,
    LPCWSTR pszArg OPTIONAL,
    LPCWSTR pszDir OPTIONAL,
    LPCWSTR pszIconPath OPTIONAL,
    INT iIconNr OPTIONAL,
    LPCWSTR pszComment OPTIONAL)
{
    CComPtr<IShellLinkW> psl;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IShellLinkW, &psl));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (pszTargetPath)
    {
        hr = psl->SetPath(pszTargetPath);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    else if (pidlTarget)
    {
        hr = psl->SetIDList(pidlTarget);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    else
    {
        ERR("invalid argument\n");
        return E_INVALIDARG;
    }

    if (pszArg)
        hr = psl->SetArguments(pszArg);

    if (pszDir)
        hr = psl->SetWorkingDirectory(pszDir);

    if (pszIconPath)
        hr = psl->SetIconLocation(pszIconPath, iIconNr);

    if (pszComment)
        hr = psl->SetDescription(pszComment);

    CComPtr<IPersistFile> ppf;
    hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = ppf->Save(pszLinkPath, TRUE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
}

HRESULT CreateSendToMyDocuments(LPCWSTR pszSendTo)
{
    WCHAR szTarget[MAX_PATH], szSendToFile[MAX_PATH], szShell32[MAX_PATH];

    HRESULT hr = CoInitialize(NULL);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ERR("CoInitialize failed: %08X\n", hr);
        return hr;
    }

    SHGetSpecialFolderPathW(NULL, szTarget, CSIDL_MYDOCUMENTS, TRUE);

    StringCbCopyW(szSendToFile, sizeof(szSendToFile), pszSendTo);
    PathAppendW(szSendToFile, PathFindFileNameW(szTarget));
    StringCbCatW(szSendToFile, sizeof(szSendToFile), L".lnk");

    GetSystemDirectoryW(szShell32, _countof(szShell32));
    PathAppendW(szShell32, L"shell32.dll");

#define IDI_SHELL_MY_DOCUMENTS 235
    hr = CreateShellLink(szSendToFile, szTarget, NULL, NULL, NULL,
                         szShell32, -IDI_SHELL_MY_DOCUMENTS, NULL);
#undef IDI_SHELL_MY_DOCUMENTS
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ERR("CreateShellLink(%S, %S) failed!\n", szSendToFile, szTarget);
    }

    CoUninitialize();
    return hr;
}

static BOOL
CreateEmptyFile(LPCWSTR pszFile)
{
    HANDLE hFile;
    hFile = CreateFileW(pszFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(hFile);
    return hFile != INVALID_HANDLE_VALUE;
}

HRESULT CreateSendToDeskLink(LPCWSTR pszSendTo)
{
    WCHAR szTarget[MAX_PATH], szSendToFile[MAX_PATH];

    LoadStringW(g_hModule, IDS_DESKLINK, szTarget, _countof(szTarget));
    StringCbCatW(szTarget, sizeof(szTarget), L".DeskLink");

    StringCbCopyW(szSendToFile, sizeof(szSendToFile), pszSendTo);
    PathAppendW(szSendToFile, szTarget);

    if (!CreateEmptyFile(szSendToFile))
    {
        ERR("CreateEmptyFile('%ls')\n", szSendToFile);
        return E_FAIL;
    }
    return S_OK;
}

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hInstance, dwReason, fImpLoad);
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hInstance;
        gModule.Init(ObjectMap, hInstance, NULL);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        gModule.Term();
    }
    return TRUE;
}

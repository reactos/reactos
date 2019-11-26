#include "precomp.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(sendmail);

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_DeskLinkDropHandler, CDeskLinkDropHandler)
END_OBJECT_MAP()

CComModule gModule;
HINSTANCE sendmail_hInstance = NULL;

STDAPI DllCanUnloadNow()
{
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
    HRESULT hr = gModule.DllRegisterServer(TRUE);
    if (FAILED(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_DESKLINK, TRUE, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = gModule.DllUnregisterServer(TRUE);
    if (FAILED(hr))
        return hr;

    hr = gModule.UpdateRegistryFromResource(IDR_DESKLINK, FALSE, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

HRESULT
CreateShellLink(
    LPCWSTR pszLinkPath,
    LPCWSTR pszCmd,
    LPCWSTR pszArg OPTIONAL,
    LPCWSTR pszDir OPTIONAL,
    LPCWSTR pszIconPath OPTIONAL,
    INT iIconNr OPTIONAL,
    LPCWSTR pszComment OPTIONAL)
{
    CComPtr<IShellLinkW> psl;
    CComPtr<IPersistFile> ppf;

    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IShellLinkW, &psl));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psl->SetPath(pszCmd);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (pszArg)
        hr = psl->SetArguments(pszArg);

    if (pszDir)
        hr = psl->SetWorkingDirectory(pszDir);

    if (pszIconPath)
        hr = psl->SetIconLocation(pszIconPath, iIconNr);

    if (pszComment)
        hr = psl->SetDescription(pszComment);

    hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
    if (SUCCEEDED(hr))
    {
        hr = ppf->Save(pszLinkPath, TRUE);
    }

    return hr;
}

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hInstance, dwReason, fImpLoad);
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        sendmail_hInstance = hInstance;
        gModule.Init(ObjectMap, hInstance, &LIBID_SendMail);

        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        gModule.Term();
        sendmail_hInstance = NULL;
    }
    return TRUE;
}

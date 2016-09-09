#pragma once

#define USE_CUSTOM_MENUBAND 1
#define USE_CUSTOM_MERGEDFOLDER 1
#define USE_CUSTOM_ADDRESSBAND 1
#define USE_CUSTOM_ADDRESSEDITBOX 1
#define USE_CUSTOM_BANDPROXY 1
#define USE_CUSTOM_BRANDBAND 1
#define USE_CUSTOM_EXPLORERBAND 1
#define USE_CUSTOM_INTERNETTOOLBAR 1

/* Constructors for the classes that are not exported */
HRESULT CTravelLog_CreateInstance(REFIID riid, void **ppv);
HRESULT CBaseBar_CreateInstance(REFIID riid, void **ppv, BOOL vertical);
HRESULT CBaseBarSite_CreateInstance(REFIID riid, void **ppv, BOOL bVertical);
HRESULT CToolsBand_CreateInstance(REFIID riid, void **ppv);

static inline
HRESULT CAddressBand_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_ADDRESSBAND
    return ShellObjectCreator<CAddressBand>(riid, ppv);
#else
    return CoCreateInstance(CLSID_SH_AddressBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, toolsBar));
#endif
}

static inline
HRESULT CAddressEditBox_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_ADDRESSEDITBOX
    return ShellObjectCreator<CAddressEditBox>(riid, ppv);
#else
    return CoCreateInstance(CLSID_AddressEditBox, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(riid, &ppv));
#endif
}

static inline
HRESULT CBandProxy_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_BANDPROXY
    return ShellObjectCreator<CBandProxy>(riid, ppv);
#else
    return CoCreateInstance(CLSID_BandProxy, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(riid, &ppv));
#endif
}

static inline
HRESULT CBrandBand_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_BRANDBAND
    return ShellObjectCreator<CBrandBand>(riid, ppv);
#else
    return CoCreateInstance(CLSID_BrandBand, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
#endif
}

static inline
HRESULT WINAPI CExplorerBand_CreateInstance(REFIID riid, LPVOID *ppv)
{
#if USE_CUSTOM_EXPLORERBAND
    return ShellObjectCreator<CExplorerBand>(riid, ppv);
#else
    return CoCreateInstance(CLSID_ExplorerBand, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
#endif
}

static inline
HRESULT CInternetToolbar_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_INTERNETTOOLBAR
    return ShellObjectCreator<CInternetToolbar>(riid, ppv);
#else
    return CoCreateInstance(CLSID_InternetToolbar, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
#endif
}

typedef HRESULT(WINAPI * PMENUBAND_CONSTRUCTOR)(REFIID riid, void **ppv);
typedef HRESULT(WINAPI * PMERGEDFOLDER_CONSTRUCTOR)(REFIID riid, void **ppv);

static inline
HRESULT CMergedFolder_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_MERGEDFOLDER
    HMODULE hRShell = GetModuleHandle(L"rshell.dll");
    if (!hRShell)
        hRShell = LoadLibrary(L"rshell.dll");

    if (hRShell)
    {
        PMERGEDFOLDER_CONSTRUCTOR pCMergedFolder_Constructor = (PMERGEDFOLDER_CONSTRUCTOR)
             GetProcAddress(hRShell, "CMergedFolder_Constructor");

        if (pCMergedFolder_Constructor)
        {
            return pCMergedFolder_Constructor(riid, ppv);
        }
    }
#endif
    return CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
}

static inline
HRESULT CMenuBand_CreateInstance(REFIID iid, LPVOID *ppv)
{
#if USE_CUSTOM_MENUBAND
    HMODULE hRShell = GetModuleHandleW(L"rshell.dll");

    if (!hRShell) 
        hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PMENUBAND_CONSTRUCTOR func = (PMENUBAND_CONSTRUCTOR) GetProcAddress(hRShell, "CMenuBand_Constructor");
        if (func)
        {
            return func(iid , ppv);
        }
    }
#endif
    return CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, iid, ppv);
}

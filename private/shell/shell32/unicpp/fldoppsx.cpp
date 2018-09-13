//
// fldoppsx - Folder Options Property Sheet Extension
//

#include "stdafx.h"
#pragma hdrstop

CFolderOptionsPsx::CFolderOptionsPsx() : m_cRef(1)
{
    DllAddRef();
}

CFolderOptionsPsx::~CFolderOptionsPsx()
{
    ATOMICRELEASE(m_pbs2);
    ATOMICRELEASE(m_pgfs);
    DllRelease();
}

ULONG CFolderOptionsPsx::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CFolderOptionsPsx::Release()
{
    ULONG ulResult = InterlockedDecrement(&m_cRef);

    if (ulResult == 0) {
        delete this;
    }

    return ulResult;
}

HRESULT CFolderOptionsPsx::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CFolderOptionsPsx, IShellPropSheetExt),
        QITABENT(CFolderOptionsPsx, IShellExtInit),
        QITABENT(CFolderOptionsPsx, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

UINT CALLBACK CFolderOptionsPsx::PropCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    CFolderOptionsPsx *pfolder = (CFolderOptionsPsx *)ppsp->lParam;

    switch (uMsg)
    {
    case PSPCB_RELEASE:
        pfolder->Release();
        break;
    }

    return TRUE;
}



//
//  We add two pages.
//
//  1. "General"    - options.cpp
//  2. "View"       - advanced.cpp
//
//  The rule for IShellPropSheetExt is that AddPages can be called
//  only once, so we don't have to worry about a second call accidentally
//  screwing up our IBrowserService pointer.
//
//
HRESULT CFolderOptionsPsx::AddPages(LPFNADDPROPSHEETPAGE AddPage, LPARAM lParam)
{
    if (!m_pgfs)
    {
        HRESULT hres = CoCreateInstance(CLSID_GlobalFolderSettings, NULL, CLSCTX_INPROC_SERVER, 
            IID_IGlobalFolderSettings, (void **)&m_pgfs);
        if (FAILED(hres))
            return hres;
    }

    /*
     *  We can limp along without an IBrowserService.  It means that
     *  we are only modifying global settings, not per-folder settings.
     */

    if (!m_pbs2) 
    {
        IUnknown_QueryService(_punkSite, SID_SShellBrowser,
                              IID_IBrowserService2, (void **)&m_pbs2);
    }

    PROPSHEETPAGE psp;

    /*
     *  We used to do this only if we aren't a rooted Explorer,
     *  but TOuzts says to do it always.
     *
     *  The lParam is a pointer back to ourselves so the page
     *  can figure out why it was created and so the two pages
     *  can talk to each other.
     */
    psp.dwSize = SIZEOF(psp);
    psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    psp.hInstance = HINST_THISDLL;
    psp.pfnCallback = CFolderOptionsPsx::PropCallback;
    psp.lParam = (LPARAM)this;


    // "General" page.
    psp.pszTemplate = MAKEINTRESOURCE(IDD_FOLDEROPTIONS);
    psp.pfnDlgProc = FolderOptionsDlgProc;
    HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);
    if (hpage)
    {
        AddRef();
        if (!AddPage(hpage, lParam))
        {
            DestroyPropertySheetPage(hpage);
            Release();
        }
    }

    // "View" page.
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ADVANCEDOPTIONS);
    psp.pfnDlgProc = AdvancedOptionsDlgProc;

    hpage = CreatePropertySheetPage(&psp);
    if (hpage)
    {
        AddRef();
        if (!AddPage(hpage, lParam))
        {
            DestroyPropertySheetPage(hpage);
            Release();
        }
    }

    return S_OK;
}

HRESULT CFolderOptionsPsx::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    return S_OK;
}

HRESULT CFolderOptionsPsx::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    return S_OK;
}

STDAPI CFolderOptionsPsx_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    CFolderOptionsPsx *pfopt = new CFolderOptionsPsx();
    if (pfopt) 
    {
        HRESULT hres = pfopt->QueryInterface(riid, ppvOut);
        pfopt->Release();
        return hres;
    }

    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}


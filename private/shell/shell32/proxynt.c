#include "shellprv.h"
#pragma  hdrstop

#include "defext.h"

//==========================================================================
// CProxyPage: Class definition
//==========================================================================

typedef struct // shprxy
{
    CCommonUnknown              cunk;
    CCommonShellExtInit         cshx;
    CKnownShellPropSheetExt     kspx;
} CProxyPage;

//==========================================================================
// CProxyPage: Member prototype
//==========================================================================
STDMETHODIMP CProxyPage_QueryInterface(IUnknown *punk, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) CProxyPage_AddRef(IUnknown *punk);
STDMETHODIMP_(ULONG) CProxyPage_Release(IUnknown *punk);
STDMETHODIMP CProxyPage_AddPages(IShellPropSheetExt *pspx,
                                 LPFNADDPROPSHEETPAGE lpfnAddPage,
                                 LPARAM lParam);
STDMETHODIMP CProxyPage_ReplacePage(IShellPropSheetExt *pspx,
                                 UINT uPageID,
                                 LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                 LPARAM lParam);

//==========================================================================
// CProxyPage: VTable
//==========================================================================

IUnknownVtbl c_CProxyPageVtbl =
{
    CProxyPage_QueryInterface, CProxyPage_AddRef, CProxyPage_Release,
};

IShellPropSheetExtVtbl c_CProxyPageSPXVtbl =
{
    Common_QueryInterface, Common_AddRef, Common_Release,
    CProxyPage_AddPages,
    CProxyPage_ReplacePage,
};


HRESULT CProxyPage_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    CProxyPage *pshprxy = (CProxyPage *)LocalAlloc(LPTR, SIZEOF(*pshprxy));
    if (pshprxy)
    {
        // Initialize CommonUnknown
        pshprxy->cunk.unk.lpVtbl = &c_CProxyPageVtbl;
        pshprxy->cunk.cRef = 1;

        // Initialize CCommonShellExtInit
        CCommonShellExtInit_Init(&pshprxy->cshx, &pshprxy->cunk);

        // Initialize CKnonwnPropSheetExt
        pshprxy->kspx.unk.lpVtbl = &c_CProxyPageSPXVtbl;
        pshprxy->kspx.nOffset = (int)((INT_PTR)&pshprxy->kspx - (INT_PTR)&pshprxy->cunk);

        // Initialize pif stuff
        PifMgrDLL_Init();

        hres = CProxyPage_QueryInterface(&pshprxy->cunk.unk, riid, ppvOut);
        CProxyPage_Release(&pshprxy->cunk.unk);
    }
    else
    {
        *ppvOut = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

//==========================================================================
// CProxyPage: Members
//==========================================================================
//
// CProxyPage::QueryInterface
//
STDMETHODIMP CProxyPage_QueryInterface(IUnknown *punk, REFIID riid, void **ppvObj)
{
    CProxyPage *this = IToClass(CProxyPage, cunk.unk, punk);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *((IUnknown **)ppvObj) = &this->cunk.unk;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit) || 
        IsEqualIID(riid, &CLSID_CCommonShellExtInit))
    {
        *((IShellExtInit **)ppvObj) = &this->cshx.kshx.unk;
    }
    else if (IsEqualIID(riid, &IID_IShellPropSheetExt))
    {
        *((IShellPropSheetExt **)ppvObj) = &this->kspx.unk;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    this->cunk.cRef++;
    return NOERROR;
}

//
// CProxyPage::AddRef
//
STDMETHODIMP_(ULONG) CProxyPage_AddRef(IUnknown *punk)
{
    CProxyPage *this = IToClass(CProxyPage, cunk.unk, punk);

    this->cunk.cRef++;
    return this->cunk.cRef;
}

//
// CProxyPage::Release
//
STDMETHODIMP_(ULONG) CProxyPage_Release(IUnknown *punk)
{
    CProxyPage *this = IToClass(CProxyPage, cunk.unk, punk);

    this->cunk.cRef--;
    if (this->cunk.cRef > 0)
    {
        return this->cunk.cRef;
    }

    CCommonShellExtInit_Delete(&this->cshx);

    LocalFree((HLOCAL)this);
    return 0;
}

//
// CProxyPage::AddPages
//
STDMETHODIMP CProxyPage_AddPages(IShellPropSheetExt *pspx,
                                 LPFNADDPROPSHEETPAGE pfnAddPage,
                                 LPARAM lParam)
{
    CProxyPage *this = IToClass(CProxyPage, kspx.unk, pspx);
    HRESULT hres;

    if (this->cshx.pdtobj)
    {
        STGMEDIUM medium;
        FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT,    -1, TYMED_HGLOBAL};

        hres = this->cshx.pdtobj->lpVtbl->GetData(this->cshx.pdtobj, &fmte, &medium);
        if (SUCCEEDED(hres))
        {
            PifPropGetPages( medium.hGlobal ? GlobalLock(medium.hGlobal) : NULL, pfnAddPage, lParam);
            if (medium.hGlobal)
                GlobalUnlock( medium.hGlobal );
            ReleaseStgMedium(&medium);
        }
    }
    else
    {
        hres = E_INVALIDARG;
    }

    return hres;
}

//
// CProxyPage::ReplacePage
//
STDMETHODIMP CProxyPage_ReplacePage(IShellPropSheetExt *pspx, UINT uPageID, 
                                    LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    return  E_NOTIMPL;
}

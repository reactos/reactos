#include "shellprv.h"
#pragma  hdrstop

#include "defext.h"

STDMETHODIMP CCommonShellExtInit_Initialize(IShellExtInit *pshx,
                                            LPCITEMIDLIST pidlFolder, 
                                            IDataObject *pdtobj, 
                                            HKEY hkeyProgID)
{
    HRESULT hres;
    CCommonShellExtInit * this=IToClass(CCommonShellExtInit, kshx.unk, pshx);
    FORMATETC fmte = { g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    // Just in case, we initialized twice.
    CCommonShellExtInit_Delete(this);

    // Duplicate the handle
    if (hkeyProgID)
        RegOpenKeyEx(hkeyProgID, NULL, 0L, MAXIMUM_ALLOWED, &this->hkeyProgID);

    this->pdtobj = pdtobj;
    pdtobj->lpVtbl->AddRef(pdtobj);

    hres = pdtobj->lpVtbl->GetData(pdtobj, &fmte, &this->medium);

    // Did the request of g_cfHIDA failed?
    if (FAILED(hres))
    {
        // Yes, then let's request mounteddrive format
        fmte.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);
        hres = pdtobj->lpVtbl->GetData(pdtobj, &fmte, &this->medium);
    }

    return hres;
}

IShellExtInitVtbl c_CCommonShellExtInitVtbl =
{
    Common_QueryInterface, Common_AddRef, Common_Release,
    CCommonShellExtInit_Initialize,
};

void CCommonShellExtInit_Init(CCommonShellExtInit *pcshx, CCommonUnknown *pcunk)
{
    pcshx->kshx.unk.lpVtbl = &c_CCommonShellExtInitVtbl;
    pcshx->kshx.nOffset = (int)((INT_PTR)pcshx - (INT_PTR)pcunk);
    ASSERT(pcshx->hkeyProgID==NULL);
    ASSERT(pcshx->medium.hGlobal==NULL);
    ASSERT(pcshx->medium.pUnkForRelease==NULL);
}

void CCommonShellExtInit_Delete(CCommonShellExtInit *pcshx)
{
    if (pcshx->hkeyProgID) 
    {
        RegCloseKey(pcshx->hkeyProgID);
        pcshx->hkeyProgID = NULL;
    }

    if (pcshx->medium.hGlobal) 
    {
        ReleaseStgMedium(&pcshx->medium);
        pcshx->medium.hGlobal = NULL;
        pcshx->medium.pUnkForRelease = NULL;
    }

    if (pcshx->pdtobj)
    {
        pcshx->pdtobj->lpVtbl->Release(pcshx->pdtobj);
        pcshx->pdtobj = NULL;
    }
}

STDMETHODIMP CCommonShellPropSheetExt_AddPages(IShellPropSheetExt *pspx, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HRESULT hres = NOERROR;
    CCommonShellPropSheetExt * this = IToClass(CCommonShellPropSheetExt, kspx, pspx);

    if (this->lpfnAddPages)
    {
        //
        // We need to get the data object from CCommonShellExtInit.
        //
        CCommonShellExtInit * pcshx;
        if (SUCCEEDED(this->kspx.unk.lpVtbl->QueryInterface(&this->kspx.unk, &CLSID_CCommonShellExtInit, &pcshx)))
        {
            hres = this->lpfnAddPages(pcshx->pdtobj, pfnAddPage, lParam);
            pcshx->kshx.unk.lpVtbl->Release(&pcshx->kshx.unk);
        }
    }
    return hres;
}

STDMETHODIMP CCommonShellPropSheetExt_ReplacePage(IShellPropSheetExt *pspx, UINT uPageID,
                                                 LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                                 LPARAM lParam)
{
    return E_NOTIMPL;
}

IShellPropSheetExtVtbl c_CCommonShellPropSheetExtVtbl =
{
    Common_QueryInterface, Common_AddRef, Common_Release,
    CCommonShellPropSheetExt_AddPages,
    CCommonShellPropSheetExt_ReplacePage,
};

void CCommonShellPropSheetExt_Init(CCommonShellPropSheetExt *pcspx, CCommonUnknown *pcunk, LPFNADDPAGES pfnAddPages)
{
    pcspx->kspx.unk.lpVtbl = &c_CCommonShellPropSheetExtVtbl;
    pcspx->kspx.nOffset = (int)((INT_PTR)pcspx - (INT_PTR)pcunk);
    pcspx->lpfnAddPages = pfnAddPages;
}

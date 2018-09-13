#include "shellprv.h"
#pragma  hdrstop

#include "defext.h"

//==========================================================================
// CProxyPage: Class definition
//==========================================================================

typedef struct
{
    CCommonUnknown              cunk;
    CCommonShellExtInit         cshx;
    CKnownShellPropSheetExt     kspx;

    TCHAR                       pszDllEntry[1];
} CProxyPage;

//==========================================================================
// CProxyPage: VTable
//==========================================================================
extern IShellPropSheetExtVtbl c_CProxyPageSPXVtbl; // forward

//==========================================================================
// CProxyPage: Members
//==========================================================================
STDMETHODIMP CProxyPage_QueryInterface(IUnknown *punk, REFIID riid, LPVOID * ppvObj)
{
    CProxyPage *this = IToClass(CProxyPage, cunk.unk, punk);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)&this->cunk.unk;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit) || 
             IsEqualIID(riid, &CLSID_CCommonShellExtInit))
    {
        *ppvObj = (void *)&this->cshx.kshx.unk;
    }
    else if (IsEqualIID(riid, &IID_IShellPropSheetExt))
    {
        *ppvObj = (void *)&this->kspx.unk;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    this->cunk.cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CProxyPage_AddRef(IUnknown *punk)
{
    CProxyPage *this = IToClass(CProxyPage, cunk.unk, punk);

    this->cunk.cRef++;
    return this->cunk.cRef;
}

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

const IUnknownVtbl c_CProxyPageVtbl =
{
    CProxyPage_QueryInterface, CProxyPage_AddRef, CProxyPage_Release,
};

HRESULT CPage16_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut, LPCTSTR pszDllEntry)
{
    HRESULT hres;
    CProxyPage *pshprxy = (CProxyPage *)LocalAlloc(LPTR, SIZEOF(*pshprxy) + (lstrlen(pszDllEntry) * SIZEOF(TCHAR)));
    if (pshprxy)
    {
        lstrcpy(pshprxy->pszDllEntry, pszDllEntry);

        // Initialize CommonUnknown
        pshprxy->cunk.unk.lpVtbl = &c_CProxyPageVtbl;
        pshprxy->cunk.cRef = 1;

        // Initialize CCommonShellExtInit
        CCommonShellExtInit_Init(&pshprxy->cshx, &pshprxy->cunk);

        // Initialize CKnonwnPropSheetExt
        pshprxy->kspx.unk.lpVtbl = &c_CProxyPageSPXVtbl;
        pshprxy->kspx.nOffset = (int)&pshprxy->kspx - (int)&pshprxy->cunk;

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

HRESULT CProxyPage_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    return CPage16_CreateInstance(punkOuter, riid, ppvOut, TEXT("pifmgr.dll,PifPropGetPages"));
}

//
// REVIEW: This function is introduced to support 16-bit property
//  sheet extensions for M5. We need to come up with a unified
//  DLL handling mechanism for the final product.
//
FARPROC HandlerFromString16(LPCTSTR pszBuffer, HINSTANCE * phinst16)
{
    TCHAR szBuffer[MAX_PATH + 80];
    LPTSTR pszProcNameSpecified;
    FARPROC lpfn16 = NULL;

    *phinst16 = NULL;

    lstrcpyn(szBuffer, pszBuffer, ARRAYSIZE(szBuffer));
    pszProcNameSpecified = StrChr(szBuffer, TEXT(','));

    if (pszProcNameSpecified)
    {
        HINSTANCE hinst16;
        *pszProcNameSpecified++ = TEXT('\0');
        PathRemoveBlanks(pszProcNameSpecified);
        PathRemoveBlanks(szBuffer);
        hinst16 = LoadLibrary16(szBuffer);
        if (ISVALIDHINST16(hinst16))
        {
#ifdef UNICODE
            {
                LPSTR lpProcNameAnsi;
                UINT cchLength;

                cchLength = lstrlen(pszProcNameSpecified)+1;

                lpProcNameAnsi = (LPSTR)alloca(cchLength*2);    // 2 for DBCS

                WideCharToMultiByte(CP_ACP, 0, pszProcNameSpecified, cchLength, lpProcNameAnsi, cchLength*2, NULL, NULL);

                lpfn16 = GetProcAddress16(hinst16, lpProcNameAnsi);
            }
#else
            lpfn16 = GetProcAddress16(hinst16, pszProcNameSpecified);
#endif
            if (lpfn16)
            {
                *phinst16 = hinst16;
            }
            else
            {
                FreeLibrary16(hinst16);
            }
        }
    }
    return lpfn16;
}


// add the pages for a given 16bit dll specified
//
// hDrop        list of files to add pages for
// pszDllEntry  DLLNAME,EntryPoint string
// lpfnAddPage  32bit add page callback
// lParam       data for 32bit page callback
//

UINT WINAPI SHAddPages16(HGLOBAL hGlobal, LPCSTR pszDllEntry, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    TCHAR szDllEntry[MAX_PATH];
    UINT ipage = 0;
    HINSTANCE hinst16;
    FARPROC lpfn16;

    SHAnsiToTChar(pszDllEntry, szDllEntry, ARRAYSIZE(szDllEntry));
    lpfn16 = HandlerFromString16(szDllEntry, &hinst16);
    if (lpfn16)
    {
        PAGEARRAY apg;
        apg.cpages = 0;
        // this is a thunk to shell.dll prt16.c
        CallAddPropSheetPages16((LPFNADDPROPSHEETPAGES)lpfn16, hGlobal ? GlobalLock(hGlobal) : NULL, &apg);
        for (ipage = 0; ipage < apg.cpages; ipage++)
        {
            // Notes: We store hinst16 to the first
            //  page only. This assumes the order
            //  we destroy pages (reverse order).
            HPROPSHEETPAGE hpage = CreateProxyPage(apg.ahpage[ipage], hinst16);
            if (hpage)
            {
                if (!pfnAddPage(hpage, lParam))
                {
                    DestroyPropertySheetPage(hpage);
                    break;
                }
                hinst16 = NULL; // unload it on delete.
            }
        }
        if (hGlobal)
            GlobalUnlock(hGlobal);

        //
        // Only if we haven't add any 16-bit pages, we should
        // unload the DLL immediately.
        //
        if (hinst16)
            FreeLibrary16(hinst16);
    }

    return ipage;
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
            SHAddPages16(medium.hGlobal, this->pszDllEntry, pfnAddPage, lParam);
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
    return E_NOTIMPL;
}

IShellPropSheetExtVtbl c_CProxyPageSPXVtbl =
{
    Common_QueryInterface, Common_AddRef, Common_Release,
    CProxyPage_AddPages,
    CProxyPage_ReplacePage,
};

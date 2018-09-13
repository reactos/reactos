#include "shellprv.h"
#pragma  hdrstop

//========================================================================
// CDefExtIcon Class definition
//========================================================================
typedef struct
{
    IExtractIcon        xi;
#ifdef UNICODE
    IExtractIconA       xiA;
#endif
    LONG                cRef;
    int                 iIcon;
    int                 iIconOpen;
    UINT                uFlags; // GIL_SIMULATEDOC/PERINSTANCE/PERCLASS
    TCHAR               achModule[1];
} CDefExtIcon;

STDMETHODIMP CDefExtIcon_QueryInterface(IExtractIcon *pxi, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) CDefExtIcon_AddRef(IExtractIcon *pxi);
STDMETHODIMP_(ULONG) CDefExtIcon_Release(IExtractIcon *pxi);


STDAPI SHCreateDefExtIcon(LPCTSTR pszModule, int iIcon, int iIconOpen, UINT uFlags, REFIID riid, void **ppvOut)
{
    return SHCreateDefExtIconKey(NULL, pszModule, iIcon, iIconOpen, uFlags, riid, ppvOut);
}

extern IExtractIconVtbl c_CDefExtIconVtbl;  // forward
#ifdef UNICODE
extern IExtractIconAVtbl c_CDefExtIconAVtbl; // forward
#endif

STDAPI SHCreateDefExtIconKey(HKEY hkey, LPCTSTR pszModule, int iIcon, int iIconOpen, UINT uFlags, REFIID riid, void **pxiOut)
{
    HRESULT hresSuccess = NOERROR;
    HRESULT hres = E_OUTOFMEMORY;      // assume error;
    CDefExtIcon *pdxi;
    TCHAR szModule[MAX_PATH];
    DWORD cb = SIZEOF(szModule);

    if (hkey)
    {
        if (SHRegQueryValue(hkey, c_szDefaultIcon, szModule, &cb) == ERROR_SUCCESS && szModule[0])
        {
            iIcon = PathParseIconLocation(szModule);
            iIconOpen = iIcon;
            pszModule = szModule;
        }
        else
            hresSuccess = S_FALSE;
    }

    if (pszModule == NULL)
    {
        // REVIEW: We should be able to make it faster!
        GetModuleFileName(HINST_THISDLL, szModule, ARRAYSIZE(szModule));
        pszModule = szModule;
    }

    pdxi = (void*)LocalAlloc(LPTR, SIZEOF(CDefExtIcon) + (lstrlen(pszModule) * SIZEOF(TCHAR)));
    if (pdxi)
    {
        pdxi->xi.lpVtbl = &c_CDefExtIconVtbl;
#ifdef UNICODE
        pdxi->xiA.lpVtbl = &c_CDefExtIconAVtbl;
#endif
        pdxi->cRef = 1;
        pdxi->iIcon = iIcon;
        pdxi->iIconOpen = iIconOpen;
        pdxi->uFlags = uFlags;
        lstrcpy(pdxi->achModule, pszModule);

        if (SUCCEEDED(CDefExtIcon_QueryInterface(&pdxi->xi, riid, pxiOut)))
            hres = hresSuccess;
        CDefExtIcon_Release(&pdxi->xi);
    }
    return hres;
}

STDMETHODIMP CDefExtIcon_QueryInterface(IExtractIcon *pxi, REFIID riid, void **ppvObj)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xi, pxi);
    if (IsEqualIID(riid, &IID_IExtractIcon) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = pxi;
    }
#ifdef UNICODE
    else if (IsEqualIID(riid, &IID_IExtractIconA))
    {
        *ppvObj = &this->xiA;
    }
#endif
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    CDefExtIcon_AddRef(&this->xi);
    return NOERROR;
}

STDMETHODIMP_(ULONG) CDefExtIcon_AddRef(IExtractIcon *pxi)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xi, pxi);
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CDefExtIcon_Release(IExtractIcon *pxi)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xi, pxi);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CDefExtIcon_GetIconLocation(IExtractIcon *pxi, UINT uFlags, 
                                         LPTSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xi, pxi);
    HRESULT hres = S_FALSE;
    int iIcon = (uFlags & GIL_OPENICON) ? this->iIconOpen : this->iIcon;
    if (iIcon != (UINT)-1)
    {
        lstrcpyn(szIconFile, this->achModule, cchMax);
        *piIndex = iIcon;
        *pwFlags = this->uFlags;
        hres = NOERROR;
    }

    return hres;
}

STDMETHODIMP CDefExtIcon_Extract(IExtractIcon *pxi, LPCTSTR pszFile, UINT nIconIndex, 
                                 HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xi, pxi);

    if (this->uFlags & GIL_NOTFILENAME)
    {
        //
        //  "*" as the file name means iIndex is already a system
        //  icon index, we are done.
        //
        //  defview never calls us in this case, but external people will.
        //
        if (pszFile[0] == TEXT('*') && pszFile[1] == 0)
        {
            DebugMsg(DM_TRACE, TEXT("DefExtIcon::Extract handling '*' for backup"));

            if (g_himlIcons == NULL)
            {
                FileIconInit( FALSE );
            }
        
            if (phiconLarge)
                *phiconLarge = ImageList_GetIcon(g_himlIcons, nIconIndex, 0);

            if (phiconSmall)
                *phiconSmall = ImageList_GetIcon(g_himlIconsSmall, nIconIndex, 0);

            return S_OK;
        }

        //  this is the case where nIconIndex is a unique id for the
        //  file.  always get the first icon.

        nIconIndex = 0;
    }

    return SHDefExtractIcon(pszFile, nIconIndex, this->uFlags,
            phiconLarge, phiconSmall, nIconSize);
}

IExtractIconVtbl c_CDefExtIconVtbl = {
    CDefExtIcon_QueryInterface, CDefExtIcon_AddRef, CDefExtIcon_Release,
    CDefExtIcon_GetIconLocation,
    CDefExtIcon_Extract,
};


#ifdef UNICODE

STDMETHODIMP CDefExtIconA_QueryInterface(LPEXTRACTICONA pxiA, REFIID riid, void **ppvObj)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xiA, pxiA);
    return CDefExtIcon_QueryInterface(&this->xi, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDefExtIconA_AddRef(LPEXTRACTICONA pxiA)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xiA, pxiA);
    return CDefExtIcon_AddRef(&this->xi);
}

STDMETHODIMP_(ULONG) CDefExtIconA_Release(LPEXTRACTICONA pxiA)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xiA, pxiA);
    return CDefExtIcon_Release(&this->xi);
}

STDMETHODIMP CDefExtIconA_GetIconLocation(LPEXTRACTICONA pxiA,
                    UINT uFlags, LPSTR pszIconFile, UINT cchMax,
                    int  *piIndex, UINT *pwFlags)
{
    WCHAR wsz[MAX_PATH];
    CDefExtIcon *this = IToClass(CDefExtIcon, xiA, pxiA);
    HRESULT hres = CDefExtIcon_GetIconLocation(&this->xi, uFlags, 
                                               wsz, ARRAYSIZE(wsz), piIndex, pwFlags);
    if (SUCCEEDED(hres) && hres != S_FALSE)
    {
        // We don't want to copy the icon file name on the S_FALSE case
        SHUnicodeToAnsi(wsz, pszIconFile, cchMax);
    }
    return hres;
}

STDMETHODIMP CDefExtIconA_Extract(LPEXTRACTICONA pxiA,
                    LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge,
                    HICON  *phiconSmall, UINT nIconSize)
{
    CDefExtIcon *this = IToClass(CDefExtIcon, xiA, pxiA);
    WCHAR wsz[MAX_PATH];

    SHAnsiToUnicode(pszFile, wsz, ARRAYSIZE(wsz));
    return CDefExtIcon_Extract(&this->xi, wsz,nIconIndex, phiconLarge, phiconSmall, nIconSize);
}

IExtractIconAVtbl c_CDefExtIconAVtbl = {
    CDefExtIconA_QueryInterface, CDefExtIconA_AddRef, CDefExtIconA_Release,
    CDefExtIconA_GetIconLocation,
    CDefExtIconA_Extract,
};

#endif  // UNICODE

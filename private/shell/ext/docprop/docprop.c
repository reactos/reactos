#include "priv.h"
#pragma hdrstop


const GUID CLSID_CDocProp = {0x3EA48300L, 0x8CF6, 0x101B, 0x84, 0xFB, 0x66, 0x6C, 0xCB, 0x9B, 0xCD, 0x32};
HRESULT CDocProp_CreateInstance(IUnknown *punkOuter, REFIID riid, void **);

// Global variables

UINT g_cRefDll = 0;         // Reference count of this DLL.
HANDLE g_hmodThisDll = NULL;    // Handle to this DLL itself.

STDAPI_(BOOL) DllEntry(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hmodThisDll = hDll;
        DisableThreadLibraryCalls(hDll);
        break;
    }
    return TRUE;
}

typedef struct {
    const IClassFactoryVtbl *cf;
    const CLSID *pclsid;
    HRESULT (*pfnCreate)(IUnknown *, REFIID, void **);
} OBJ_ENTRY;

extern const IClassFactoryVtbl c_CFVtbl;        // forward

//
// we always do a linear search here so put your most often used things first
//
const OBJ_ENTRY c_clsmap[] = {
    { &c_CFVtbl, &CLSID_CDocProp,   CDocProp_CreateInstance},
    // add more entries here
    { NULL, NULL, NULL}
};

// static class factory (no allocs!)

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    DllAddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    return this->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CClassFactory_QueryInterface, CClassFactory_AddRef, CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        const OBJ_ENTRY *pcls;
        for (pcls = c_clsmap; pcls->pclsid; pcls++)
        {
            if (IsEqualIID(rclsid, pcls->pclsid))
            {
                *ppv = (void *)&(pcls->cf);
                DllAddRef();    // Class Factory keeps dll in memory
                return NOERROR;
            }
        }
    }
    // failure
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}

STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}


typedef struct
{
    IShellExtInit           _ei;
    IShellPropSheetExt      _pse;
    int                     _cRef;                  // reference count
    IDataObject *           _pdtobj;                // data object
    TCHAR                   _szFile[MAX_PATH];
} CDocProp;


STDMETHODIMP_(UINT) CDocProp_PSE_AddRef(IShellPropSheetExt *pei)
{
    CDocProp *this = IToClass(CDocProp, _pse, pei);
    return ++this->_cRef;
}

STDMETHODIMP_(UINT) CDocProp_PSE_Release(IShellPropSheetExt *pei)
{
    CDocProp *this = IToClass(CDocProp, _pse, pei);

    if (--this->_cRef)
        return this->_cRef;

    if (this->_pdtobj)
        this->_pdtobj->lpVtbl->Release(this->_pdtobj);

    LocalFree((HLOCAL)this);
    DllRelease();
    return 0;
}

STDMETHODIMP CDocProp_PSE_QueryInterface(IShellPropSheetExt *pei, REFIID riid, void **ppvOut)
{
    CDocProp *this = IToClass(CDocProp, _pse, pei);

    if (IsEqualIID(riid, &IID_IShellPropSheetExt) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvOut = (void *)pei;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppvOut = (void *)&this->_ei;
    }
    else
    {
        *ppvOut = NULL;
        return E_NOINTERFACE;
    }

    this->_cRef++;
    return NOERROR;
}

#ifdef _WIN2000_DOCPROP_
#define NUM_PAGES 1
#else  //_WIN2000_DOCPROP_
#define NUM_PAGES 4
#endif //_WIN2000_DOCPROP_

UINT CALLBACK PSPCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE psp)
{
    switch (uMsg) {
    case PSPCB_RELEASE:
        if (psp && psp->lParam)
        {
            LPALLOBJS lpallobjs = (LPALLOBJS)psp->lParam;
            if (0 == --lpallobjs->uPageRef)
            {
                if (lpallobjs->fOleInit)
                    CoUninitialize();

                // Free our structure so hope we don't get it again!
                FOfficeDestroyObjects(&lpallobjs->lpSIObj, &lpallobjs->lpDSIObj, &lpallobjs->lpUDObj);

                GlobalFree(lpallobjs);
            }
        }
        DllRelease();
        break;
    }
    return 1;
}

STDMETHODIMP CDocProp_PSE_AddPages(IShellPropSheetExt *ppse, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    CDocProp *this = IToClass(CDocProp, _pse, ppse);
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hres = this->_pdtobj->lpVtbl->GetData(this->_pdtobj, &fmte, &medium);
    if (hres == S_OK && (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0) == 1))
    {
        WCHAR wszPath[MAX_PATH];
        TCHAR szPath[MAX_PATH];
        DWORD grfStgMode;
        IStorage *pstg = NULL;

        DragQueryFile((HDROP)medium.hGlobal, 0, szPath, ARRAYSIZE(szPath));

#ifdef UNICODE
        lstrcpyn(wszPath, szPath, MAX_PATH);
#else
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));
#endif

        // Load the properties for this file
        grfStgMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;

#ifdef WINNT
        if( GetFileAttributes( szPath ) & FILE_ATTRIBUTE_OFFLINE )
        {
            ReleaseStgMedium(&medium);
            return HRESULT_FROM_WIN32(ERROR_FILE_OFFLINE);
        }

        hres = StgOpenStorageEx(wszPath, grfStgMode, STGFMT_STORAGE, 0, NULL, NULL, &IID_IStorage, (void**)&pstg);
#else
        hres = StgOpenStorage( wszPath, NULL, grfStgMode, NULL, 0L, &pstg ) ;
#endif
        if (FAILED(hres))
        {
            // if we failed to open the file, try w/READ ONLY access
            grfStgMode = STGM_SHARE_EXCLUSIVE | STGM_READ;
#ifdef WINNT
            hres = StgOpenStorageEx(wszPath, grfStgMode, STGFMT_STORAGE, 0, NULL, NULL, &IID_IStorage, (void**)&pstg);
#else
            hres = StgOpenStorage( wszPath, NULL, grfStgMode, NULL, 0L, &pstg ) ;
#endif
        }


        if (SUCCEEDED(hres))
        {
            int i;

            // Allocate our main structure and make sure it is zero filled!
            LPALLOBJS lpallobjs = (LPALLOBJS)GlobalAlloc(GPTR, sizeof(ALLOBJS));
            if (lpallobjs)
            {
                PROPSHEETPAGE psp[NUM_PAGES];

                lstrcpyn(lpallobjs->szPath, szPath, ARRAYSIZE(lpallobjs->szPath));

                // Initialize Office property code
#ifdef _WIN2000_DOCPROP_
                FOfficeCreateAndInitObjects( NULL, NULL, &lpallobjs->lpUDObj);
#else  _WIN2000_DOCPROP_
                FOfficeCreateAndInitObjects(&lpallobjs->lpSIObj, &lpallobjs->lpDSIObj, &lpallobjs->lpUDObj);
#endif _WIN2000_DOCPROP_

                lpallobjs->lpfnDwQueryLinkData = NULL;
                lpallobjs->dwMask = 0;

                // Fill in some stuff for the Office code
                lpallobjs->fFiledataInit = FALSE;

                // Initialize OLE
                lpallobjs->fOleInit = SUCCEEDED(CoInitialize(0));

                // Initialize the PropertySheets we're going to add
                FOfficeInitPropInfo(psp, PSP_USECALLBACK, (LPARAM)lpallobjs, PSPCallback);
                FLoadTextStrings();

#ifdef _WIN2000_DOCPROP_
                DwOfficeLoadProperties(pstg, NULL, NULL, lpallobjs->lpUDObj, 0, grfStgMode);
#else _WIN2000_DOCPROP_
                DwOfficeLoadProperties(pstg, lpallobjs->lpSIObj, lpallobjs->lpDSIObj, lpallobjs->lpUDObj, 0, grfStgMode);
#endif _WIN2000_DOCPROP_

                // Try to add our new property pages
                for (i = 0; i < NUM_PAGES; i++) 
                {
                    HPROPSHEETPAGE  hpage = CreatePropertySheetPage(&psp[i]);
                    if (hpage) 
                    {
                        DllAddRef();            // matched in PSPCB_RELEASE
                        if (lpfnAddPage(hpage, lParam))
                        {
                            FAttach( lpallobjs, psp + i, hpage );
                            lpallobjs->uPageRef++;
                        }
                        else 
                            DestroyPropertySheetPage(hpage);
                    }
                }

                if (lpallobjs->uPageRef == 0)
                {
                    if (lpallobjs->fOleInit)
                        CoUninitialize();

                    // Free our structures
                    FOfficeDestroyObjects(&lpallobjs->lpSIObj, &lpallobjs->lpDSIObj, &lpallobjs->lpUDObj);
                    GlobalFree(lpallobjs);
                }

            }   // if (lpallobjs)
        }   // StgOpenStorage ... if (SUCCEEDED(hres))

        if (NULL != pstg )
        {
            pstg->lpVtbl->Release(pstg);
            pstg = NULL;
        }
        ReleaseStgMedium(&medium);
    }
    return S_OK;
}

STDMETHODIMP CDocProp_SEI_Initialize(IShellExtInit *pei, LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdtobj, HKEY hkeyProgID)
{
    CDocProp *this = IToClass(CDocProp, _ei, pei);

    // Initialize can be called more than once.
    if (this->_pdtobj)
        this->_pdtobj->lpVtbl->Release(this->_pdtobj);

    // Duplicate the pdtobj pointer
    if (pdtobj) 
    {
        this->_pdtobj = pdtobj;
        pdtobj->lpVtbl->AddRef(pdtobj);
    }

    return NOERROR;
}

STDMETHODIMP_(UINT) CDocProp_SEI_AddRef(IShellExtInit *pei)
{
    CDocProp *this = IToClass(CDocProp, _ei, pei);
    return CDocProp_PSE_AddRef(&this->_pse);
}

STDMETHODIMP_(UINT) CDocProp_SEI_Release(IShellExtInit *pei)
{
    CDocProp *this = IToClass(CDocProp, _ei, pei);
    return CDocProp_PSE_Release(&this->_pse);
}

STDMETHODIMP CDocProp_SEI_QueryInterface(IShellExtInit *pei, REFIID riid, void **ppv)
{
    CDocProp *this = IToClass(CDocProp, _ei, pei);
    return CDocProp_PSE_QueryInterface(&this->_pse, riid, ppv);
}


extern IShellExtInitVtbl           c_CDocProp_SXIVtbl;
extern IShellPropSheetExtVtbl      c_CDocProp_SPXVtbl;

HRESULT CDocProp_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    CDocProp *pdp;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    pdp = LocalAlloc(LPTR, sizeof(CDocProp));
    if (pdp)
    {
        HRESULT hres;

        DllAddRef();

        pdp->_ei.lpVtbl = &c_CDocProp_SXIVtbl;
        pdp->_pse.lpVtbl = &c_CDocProp_SPXVtbl;
        pdp->_cRef = 1;

        hres = CDocProp_PSE_QueryInterface(&pdp->_pse, riid, ppvOut);
        CDocProp_PSE_Release(&pdp->_pse);

        return hres;        // S_OK or E_NOINTERFACE
    }
    return E_OUTOFMEMORY;
}

IShellPropSheetExtVtbl c_CDocProp_SPXVtbl = {
    CDocProp_PSE_QueryInterface, CDocProp_PSE_AddRef, CDocProp_PSE_Release,
    CDocProp_PSE_AddPages
};

IShellExtInitVtbl c_CDocProp_SXIVtbl = {
    CDocProp_SEI_QueryInterface, CDocProp_SEI_AddRef, CDocProp_SEI_Release,
    CDocProp_SEI_Initialize
};


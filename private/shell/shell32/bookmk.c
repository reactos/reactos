#include "shellprv.h"
#pragma  hdrstop

#include "fstreex.h"
#include "bookmk.h"


// *** WARNING *** 
//
// Scrap_CreateFromDataObject is a TCHAR export from SHSCRAP.DLL, if you change its calling convention, you
// must modify PFNSCRAPCREATEFROMDATAOBJECT and the wrapper fn. below 
//
// *** WARNING ***
typedef HRESULT (CALLBACK *PFNSCRAPCREATEFROMDATAOBJECT)(LPCTSTR pszPath, IDataObject *pDataObj, BOOL fLink, LPTSTR pszNewFile);


STDAPI Scrap_CreateFromDataObject(LPCTSTR pszPath, IDataObject *pDataObj, BOOL fLink, LPTSTR pszNewFile)
{
    static PFNSCRAPCREATEFROMDATAOBJECT pfn = (PFNSCRAPCREATEFROMDATAOBJECT)-1;

    if (pfn == (PFNSCRAPCREATEFROMDATAOBJECT)-1)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("shscrap.dll"));

        if (hinst)
        {
            pfn = (PFNSCRAPCREATEFROMDATAOBJECT)GetProcAddress(hinst, "Scrap_CreateFromDataObject");
        }
        else
        {
            pfn = NULL;
        }
    }

    if (pfn)
    {
        // BUGBUG: bummer, TCHAR used on export.
        return pfn(pszPath, pDataObj, fLink, pszNewFile);
    }

    // for failure cases just return E_UNEXPECTED;
    return E_UNEXPECTED;
}


//
// Parameters:
//  pDataObj    -- The data object passed from the drag source.
//  pt          -- Dropped position (in screen coordinate).
//  pdwEffect   -- Pointer to dwEffect variable to be returned to the drag source.
//
STDAPI FS_CreateBookMark(HWND hwnd, LPCTSTR pszPath, IDataObject *pDataObj, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres;
    TCHAR szNewFile[MAX_PATH];
    DECLAREWAITCURSOR;

    // We should have only one bit set.
    ASSERT(*pdwEffect==DROPEFFECT_COPY || *pdwEffect==DROPEFFECT_LINK || *pdwEffect==DROPEFFECT_MOVE);

    SetWaitCursor();
    hres = Scrap_CreateFromDataObject(pszPath, pDataObj, *pdwEffect == DROPEFFECT_LINK, szNewFile);
    ResetWaitCursor();

    if (SUCCEEDED(hres)) 
    {
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szNewFile, NULL);
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szNewFile, NULL);
        FS_PositionFileFromDrop(hwnd, szNewFile, NULL);
    } 
    else 
    {
        *pdwEffect = 0;
    }

    return hres;
}


#define MAX_FORMATS     20

typedef struct
{
    IEnumFORMATETC efmt;
    LONG         cRef;
    UINT         ifmt;
    UINT         cfmt;
    FORMATETC    afmt[1];
} CStdEnumFmt;

// forward
extern const IEnumFORMATETCVtbl c_CStdEnumFmtVtbl;

//===========================================================================
// CStdEnumFmt : Constructor
//===========================================================================
STDAPI SHCreateStdEnumFmtEtc(UINT cfmt, const FORMATETC afmt[], IEnumFORMATETC **ppenumFormatEtc)
{
    CStdEnumFmt * this = (CStdEnumFmt*)LocalAlloc( LPTR, SIZEOF(CStdEnumFmt) + (cfmt-1)*SIZEOF(FORMATETC));
    if (this)
    {
        this->efmt.lpVtbl = &c_CStdEnumFmtVtbl;
        this->cRef = 1;
        this->cfmt = cfmt;
        memcpy(this->afmt, afmt, cfmt * SIZEOF(FORMATETC));
        *ppenumFormatEtc = &this->efmt;
        return S_OK;
    }
    *ppenumFormatEtc = NULL;
    return E_OUTOFMEMORY;
}

STDAPI SHCreateStdEnumFmtEtcEx(UINT cfmt, const FORMATETC afmt[],
                               IDataObject *pdtInner, IEnumFORMATETC **ppenumFormatEtc)
{
    HRESULT hres;
    FORMATETC *pfmt;
    UINT cfmtTotal;

    if (pdtInner)
    {
        IEnumFORMATETC *penum;
        hres = pdtInner->lpVtbl->EnumFormatEtc(pdtInner, DATADIR_GET, &penum);
        if (SUCCEEDED(hres))
        {
            UINT cfmt2, cGot;
            FORMATETC fmte;

            for (cfmt2 = 0; penum->lpVtbl->Next(penum, 1, &fmte, &cGot) == S_OK; cfmt2++) 
            {
                // count up the number of FormatEnum in cfmt2
            }

            penum->lpVtbl->Reset(penum);
            cfmtTotal = cfmt + cfmt2;

            // Allocate the buffer for total
            pfmt = (FORMATETC *)LocalAlloc(LPTR, SIZEOF(FORMATETC) * cfmtTotal);
            if (pfmt)
            {
                UINT i;
                // Get formatetcs from the inner object
                for (i = 0; i < cfmt2; i++) 
                {
                    penum->lpVtbl->Next(penum, 1, &pfmt[i], &cGot);
                }

                // Copy the rest
                if (cfmt)
                    memcpy(&pfmt[cfmt2], afmt, SIZEOF(FORMATETC) * cfmt);
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }

            penum->lpVtbl->Release(penum);
        }
    }
    else
    {
        hres = E_FAIL;  // ptInner == NULL
    }

    if (FAILED(hres) && hres != E_OUTOFMEMORY)
    {
        //
        // Ignore none fatal error from pdtInner::EnumFormatEtc
        // We'll come here if
        //  1. pdtInner == NULL or
        //  2. pdtInner->EnumFormatEtc failed (except E_OUTOFMEMORY)
        //
        hres = NOERROR;
        pfmt = (FORMATETC *)afmt;       // safe const -> non const cast
        cfmtTotal = cfmt;
    }

    if (SUCCEEDED(hres)) 
    {
        hres = SHCreateStdEnumFmtEtc(cfmtTotal, pfmt, ppenumFormatEtc);
        if (pfmt != afmt)
            LocalFree((HLOCAL)pfmt);
    }

    return hres;
}

STDMETHODIMP CStdEnumFmt_QueryInterface(IEnumFORMATETC *pefmt, REFIID riid, void **ppvObj)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);

    if (IsEqualIID(riid, &IID_IEnumFORMATETC) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->efmt;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    this->cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CStdEnumFmt_AddRef(IEnumFORMATETC *pefmt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    return ++this->cRef;
}

STDMETHODIMP_(ULONG) CStdEnumFmt_Release(IEnumFORMATETC *pefmt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    this->cRef--;
    if (this->cRef > 0)
        return this->cRef;

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CStdEnumFmt_Next(IEnumFORMATETC *pefmt, ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    UINT cfetch;
    HRESULT hres = S_FALSE;     // assume less numbers

    if (this->ifmt < this->cfmt)
    {
        cfetch = this->cfmt - this->ifmt;
        if (cfetch>=celt) 
        {
            cfetch = celt;
            hres = S_OK;
        }

        memcpy(rgelt, &this->afmt[this->ifmt], cfetch*SIZEOF(FORMATETC));
        this->ifmt += cfetch;
    }
    else
    {
        cfetch = 0;
    }

    if (pceltFethed)
        *pceltFethed = cfetch;

    return hres;
}

STDMETHODIMP CStdEnumFmt_Skip(IEnumFORMATETC *pefmt, ULONG celt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    this->ifmt += celt;
    if (this->ifmt > this->cfmt) {
        this->ifmt = this->cfmt;
        return S_FALSE;
    }
    return S_OK;
}

STDMETHODIMP CStdEnumFmt_Reset(IEnumFORMATETC *pefmt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    this->ifmt = 0;
    return S_OK;
}

STDMETHODIMP CStdEnumFmt_Clone(IEnumFORMATETC *pefmt, IEnumFORMATETC ** ppenum)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    return SHCreateStdEnumFmtEtc(this->cfmt, this->afmt, ppenum);
}

const IEnumFORMATETCVtbl c_CStdEnumFmtVtbl = {
    CStdEnumFmt_QueryInterface, CStdEnumFmt_AddRef, CStdEnumFmt_Release,
    CStdEnumFmt_Next,
    CStdEnumFmt_Skip,
    CStdEnumFmt_Reset,
    CStdEnumFmt_Clone,
};

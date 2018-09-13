#include "shellprv.h"
#pragma  hdrstop

typedef struct _DefEnum         // deunk
{
    IEnumIDList         eunk;
    int                 cRef;
    int                 iCur;
    LPITEMIDLIST        pidlReturn;
    LPVOID              lpData;
    LPFNENUMCALLBACK    lpfn;
} CDefEnum;

extern IEnumIDListVtbl c_DefEnumVtbl;   // forward

STDAPI SHCreateEnumObjects(HWND hwndOwner, LPVOID lpData, LPFNENUMCALLBACK lpfn, IEnumIDList **ppeunk)
{
    CDefEnum * pdeunk = (CDefEnum *)LocalAlloc( LPTR, SIZEOF(CDefEnum));
    if (pdeunk)
    {
        pdeunk->eunk.lpVtbl = &c_DefEnumVtbl;
        pdeunk->cRef = 1;

        pdeunk->lpData = lpData;
        pdeunk->lpfn   = lpfn;

        ASSERT(pdeunk->iCur == 0);
        ASSERT(pdeunk->pidlReturn == NULL);

        *ppeunk = &pdeunk->eunk;

        return NOERROR;
    }
    *ppeunk = NULL;
    return E_OUTOFMEMORY;
}


void CDefEnum_SetReturn(LPARAM lParam, LPITEMIDLIST pidl)
{
    CDefEnum *pdeunk = (CDefEnum *)lParam;
    pdeunk->pidlReturn = pidl;
}

STDMETHODIMP CDefEnum_QueryInterface(IEnumIDList *peunk, REFIID riid, LPVOID *ppvObj)
{
    CDefEnum * this = IToClass(CDefEnum, eunk, peunk);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IEnumIDList))
    {
        *ppvObj = &this->eunk;
        InterlockedIncrement(&this->cRef);
        return NOERROR;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDefEnum_AddRef(IEnumIDList *peunk)
{
    CDefEnum * this = IToClass(CDefEnum, eunk, peunk);
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CDefEnum_Release(IEnumIDList *peunk)
{
    CDefEnum * this = IToClass(CDefEnum, eunk, peunk);

    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    this->lpfn((LPARAM)this, this->lpData, ECID_RELEASE, 0);
    LocalFree((HLOCAL)this);

    return 0;
}

STDMETHODIMP CDefEnum_Next(IEnumIDList *peunk, ULONG celt, LPITEMIDLIST *ppidlOut, ULONG *pceltFetched)
{
    CDefEnum * this = IToClass(CDefEnum, eunk, peunk);
    HRESULT hres;

    this->pidlReturn = NULL;

    hres = this->lpfn((LPARAM)this, this->lpData, ECID_SETNEXTID, this->iCur);
    if (hres == NOERROR)
    {
        this->iCur++;

        ASSERT(this->pidlReturn);

        *ppidlOut = this->pidlReturn;

        if (pceltFetched)
            *pceltFetched = 1;
    }
    else
    {
        ASSERT(this->pidlReturn == NULL);

        *ppidlOut = NULL;
        if (pceltFetched)
            *pceltFetched = 0;
    }

    return hres;
}


///// These will be used by all enums that aren't yet implemented

STDMETHODIMP CDefEnum_Skip(IEnumIDList *peunk, ULONG celt)
{
    // REVIEW: Implement it later.
    return E_NOTIMPL;
}

STDMETHODIMP CDefEnum_Reset(IEnumIDList *peunk)
{
    // REVIEW: Implement it later.
    return E_NOTIMPL;
}

STDMETHODIMP CDefEnum_Clone(IEnumIDList *peunk, IEnumIDList ** ppenm)
{
    // We'll never support this function.
    return E_FAIL;
}

IEnumIDListVtbl c_DefEnumVtbl =
{
    CDefEnum_QueryInterface, CDefEnum_AddRef, CDefEnum_Release,
    CDefEnum_Next,
    CDefEnum_Skip,
    CDefEnum_Reset,
    CDefEnum_Clone
};



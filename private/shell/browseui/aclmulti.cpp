/* Copyright 1996 Microsoft */

#include <priv.h>
#include "sccls.h"
#include "dbgmem.h"
#include "aclmulti.h"

//
// CACLMulti -- An AutoComplete List COM object that
//                  contains other AutoComplete Lists and
//                  has them do all the work.
//

struct _tagListItem
{
    IUnknown        *punk;
    IEnumString     *pes;
    IACList         *pacl;
};
typedef struct _tagListItem LISTITEM;

#define MULTILIST_GROWTH_CONST 8

/* IUnknown methods */

HRESULT CACLMulti::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumString))
    {
        *ppvObj = SAFECAST(this, IEnumString*);
    }
    else if (IsEqualIID(riid, IID_IObjMgr))
    {
        *ppvObj = SAFECAST(this, IObjMgr*);
    }
    else if (IsEqualIID(riid, IID_IACList))
    {
        *ppvObj = SAFECAST(this, IACList*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CACLMulti::AddRef(void)
{
    _cRef++;
    return _cRef;
}

ULONG CACLMulti::Release(void)
{
    ASSERT(_cRef > 0);

    _cRef--;

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}

/* IEnumString methods */

HRESULT CACLMulti::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;    // nothing found... stop

    *pceltFetched = 0;

    if (celt == 0)
    {
        return S_OK;
    }

    if (!rgelt)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr) && _hdsa)
    {
        //
        // Keep calling Next() starting with the current list
        // until somebody returns something.
        //
        for( ; _iSubList < DSA_GetItemCount(_hdsa); _iSubList++)
        {
            LISTITEM li;

            if ((DSA_GetItem(_hdsa, _iSubList, &li) != -1) && (li.pes != NULL))
            {
                hr = li.pes->Next(1, rgelt, pceltFetched);
                if (hr == S_OK)
                    break;

                if (FAILED(hr))  // Why is the caller failing? 
                    hr = S_FALSE;   // Probably because it failed to conntect to the source (ftp)
            }
        }
    }
    ASSERT(SUCCEEDED(hr));

    return hr;
}

HRESULT CACLMulti::Skip(ULONG)
{
    return E_NOTIMPL;
}

HRESULT CACLMulti::Reset(void)
{
    HRESULT hr = S_OK;
    TraceMsg(TF_BAND|TF_GENERAL, "ACLMulti::Reset() Beginning");

    if (_hdsa)
    {
        // Call Reset() on each sublist.
        for (_iSubList=0; _iSubList < DSA_GetItemCount(_hdsa); _iSubList++)
        {
            LISTITEM li;
            
            if ((DSA_GetItem(_hdsa, _iSubList, &li) != -1) && (li.pes != NULL))
            {
                hr = li.pes->Reset();
                ASSERT(SUCCEEDED(hr));
                if (FAILED(hr))
                    break;
            }
        }
    }

    // Reset ourselves to point to the first list.
    _iSubList = 0;

    return hr;
}

HRESULT CACLMulti::Clone(IEnumString **ppenum)
{
    return CACLMulti_Create(ppenum, this);
}

/* IObjMgr methods */

HRESULT CACLMulti::Append(IUnknown *punk)
{
    HRESULT hr = E_FAIL;

    if (punk)
    {
        if (!_hdsa)
        {
            _hdsa = DSA_Create(SIZEOF(LISTITEM), MULTILIST_GROWTH_CONST);
        }

        if (_hdsa)
        {
            LISTITEM li = { 0 };

            //
            // Call QI to get the necessary interfaces,
            // and append the interfaces to the internal list.
            //
            li.punk = punk;
            li.punk->AddRef();

            li.punk->QueryInterface(IID_IEnumString, (LPVOID *)&li.pes);
            li.punk->QueryInterface(IID_IACList, (LPVOID *)&li.pacl);

            if (DSA_AppendItem(_hdsa, &li) != -1)
            {
                hr = S_OK;
            }
            else
            {
                _FreeListItem(&li, 0);
                hr = E_FAIL;
            }
        }
    }

    return hr;
}

HRESULT CACLMulti::Remove(IUnknown *punk)
{
    HRESULT hr = E_FAIL;
    int i;

    if (punk && _hdsa)
    {
        for(i=DPA_GetPtrCount(_hdsa); i>=0; i--)
        {
            LISTITEM li;

            if (DSA_GetItem(_hdsa, i, &li) != -1)
            {
                if (punk == li.punk)
                {
                    _FreeListItem(&li, 0);
                    if (DSA_DeleteItem(_hdsa, i))
                    {
                        hr = S_OK;
                    }
                    break;
                }
            }
        }
    }

    return hr;
}

/* IACList methods */

HRESULT CACLMulti::Expand(LPCOLESTR pszExpand)
{
    HRESULT hr = S_OK;
    int i;

    if (_hdsa)
    {
        // Call Expand() on each sublist.
        for (i=0; i < DSA_GetItemCount(_hdsa); i++)
        {
            LISTITEM li;
            
            if ((DSA_GetItem(_hdsa, i, &li) != -1) && (li.pacl != NULL))
            {
                hr = li.pacl->Expand(pszExpand);
                if (hr == S_OK)
                    break;
            }
        }
    }
    
    if (E_NOTIMPL == hr)
        hr = S_OK;

    return hr;
}

/* Constructor / Destructor / CreateInstance */

CACLMulti::CACLMulti()
{
    DllAddRef();
    ASSERT(!_hdsa);
    ASSERT(!_iSubList);
    _cRef = 1;
}

CACLMulti::~CACLMulti()
{
    DllRelease();
    DSA_DestroyCallback(_hdsa, _FreeListItem, 0);
}

HRESULT CACLMulti_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory


    *ppunk = NULL;
    CACLMulti * p = new CACLMulti();
    if (p) 
    {
        *ppunk = SAFECAST(p, IEnumString *);
        return NOERROR;
    }

    return E_OUTOFMEMORY;
}

HRESULT CACLMulti_Create(IEnumString **ppenum, CACLMulti * paclMultiToCopy)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppenum = NULL;
    CACLMulti * p = new CACLMulti();
    if (p && paclMultiToCopy->_hdsa) 
    {
        // Clone data
        int iSize = DSA_GetItemCount(paclMultiToCopy->_hdsa);
        int iIndex;
        LISTITEM li;

        hr = S_OK;
        p->_hdsa = DSA_Create(SIZEOF(LISTITEM), MULTILIST_GROWTH_CONST);

        // We need to copy the source HDSA
        for (iIndex = 0; (iIndex < iSize) && (S_OK == hr); iIndex++)
        {
            if (DSA_GetItem(paclMultiToCopy->_hdsa, iIndex, &li) != -1)
                hr = p->Append(li.punk);
            else
                hr = E_FAIL;
        }
        p->_iSubList = paclMultiToCopy->_iSubList;

        if (SUCCEEDED(hr))
            *ppenum = SAFECAST(p, IEnumString *);
        else
            p->Release();
    }

    return hr;
}

//
// Frees all the contents of one list item.
//
int CACLMulti::_FreeListItem(LPVOID p, LPVOID d)
{
    LISTITEM *pli = (LISTITEM *)p;

    SAFERELEASE(pli->pacl);
    SAFERELEASE(pli->pes);
    SAFERELEASE(pli->punk);

    return 1;
}

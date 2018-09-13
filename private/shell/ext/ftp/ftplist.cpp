/*****************************************************************************\
    CFtpList.cpp - Internal list manager

    It is the caller's responsibility to manage any needed serialization.
\*****************************************************************************/

#include "priv.h"
#include "ftplist.h"


/*****************************************************************************\
    FUNCTION: CFtpList::DeleteItemPtr

    Remove the element "pv" from the array.
\*****************************************************************************/

void CFtpList::DeleteItemPtr(LPVOID pv)
{
    int nIndex;

    ASSERT(m_hdpa);
    nIndex = DPA_GetPtrIndex(m_hdpa, pv);
    TraceMsg(TF_FTPLIST, "CFtpList::DeleteItemPtr(pv=%#08lx) this=%#08lx, nIndex=%d", pv, this, nIndex);

    if (-1 != nIndex)
        DPA_DeletePtr(m_hdpa, nIndex);
}


/*****************************************************************************\
    FUNCTION: AppendItem

    Add a new pv to the growing array.
\*****************************************************************************/
HRESULT CFtpList::AppendItem(LPVOID pv)
{
    ASSERT(m_hdpa);
    DPA_AppendPtr(m_hdpa, pv);
    //TraceMsg(TF_FTPLIST, "CFtpList::AppendItem(pv=%#08lx) this=%#08lx", pv, this);

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: InsertSorted

    Add a new pv to the growing array.
\*****************************************************************************/
HRESULT CFtpList::InsertSorted(LPVOID pv, PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    ASSERT(m_hdpa);
    DPA_SortedInsertPtr(m_hdpa, pv, 0, pfnCompare, lParam, DPAS_INSERTBEFORE, pv);

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: SortedSearch

    DESCRIPTION:
        Search thru the list for the item.
\*****************************************************************************/
int CFtpList::SortedSearch(LPVOID pv, PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT options)
{
    return DPA_Search(m_hdpa, pv, 0, pfnCompare, lParam, options);
}


/*****************************************************************************\
    FUNCTION: Find

    Call back once for each item in the pv list.  Stops when the
    callback returns 0, returning the item that triggered the match.

    The callback typically returns the result of a comparison function.
\*****************************************************************************/
LPVOID CFtpList::Find(PFNDPACOMPARE pfn, LPCVOID pv)
{
    LPVOID pvoid = NULL;
    int nIndex;

    nIndex = DPA_Search(m_hdpa, (LPVOID) pv, 0, pfn, NULL, 0);

    if (-1 != nIndex)
        pvoid = DPA_GetPtr(m_hdpa, nIndex);

    //TraceMsg(TF_FTPLIST, "CFtpList::Find(pfn=%#08lx; pv=%#08lx) this=%#08lx, nIndex=%d, result=%#08lx", pfn, pv, this, nIndex, pvoid);

    return pvoid;
}

/*****************************************************************************\
    CFtpList_Create

    Start up a new pv list, with a recommended initial size and other
    callback info.
\*****************************************************************************/
HRESULT CFtpList_Create(int cpvInit, PFNDPAENUMCALLBACK pfn, UINT nGrow, CFtpList ** ppfl)
{
    HRESULT hres = E_OUTOFMEMORY;
    CFtpList * pfl = new CFtpList(pfn);
    *ppfl = pfl;

    if (pfl)
    {
        pfl->m_hdpa = DPA_Create(nGrow);
       //CFtpList_Create(pfn=%#08lx) this=%#08lx, cpvInit=%d, nGrow=%d", pfn, pfl, cpvInit, nGrow);

        if (EVAL(pfl->m_hdpa))
            hres = S_OK;
        else
        {
            pfl->Release();
            *ppfl = NULL;
        }
    }

    return hres;
}


/****************************************************\
    Constructor
\****************************************************/
CFtpList::CFtpList(PFNDPAENUMCALLBACK pfnDestroy) : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_hdpa);

    m_pfnDestroy = pfnDestroy;
    LEAK_ADDREF(LEAK_CFtpList);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpList::~CFtpList()
{
    //TraceMsg(TF_FTPLIST, "CFtpList::~CFtpList() this=%#08lx", this);
    if (m_pfnDestroy)
        DPA_DestroyCallback(m_hdpa, m_pfnDestroy, NULL);
    else
        DPA_Destroy(m_hdpa);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpList);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpList::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpList::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown *);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpList::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

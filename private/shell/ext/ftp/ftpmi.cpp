/*
 *    ftpmi.cpp - IMalloc interface for allocing pidls
*/

#include "priv.h"
#include "ftpmi.h"


LPVOID CMallocItem::Alloc(ULONG cbSize)
{
    WORD cbActualSize = sizeof(DELEGATEITEMID) - 1 + cbSize;
    PDELEGATEITEMID pidl = (PDELEGATEITEMID)SHAlloc(cbActualSize + 2);

    if (pidl)
    {
        pidl->cbSize = cbActualSize;
        pidl->wOuter = 0x6646;          // "Ff"
        pidl->cbInner = (WORD)cbSize;
        *(WORD *)&(((BYTE *)pidl)[cbActualSize]) = 0;
    }

    return pidl;
}


LPVOID CMallocItem::Realloc(LPVOID pv, ULONG cb)
{
    return NULL;
}


void CMallocItem::Free(LPVOID pv)
{
    SHFree(pv);
}


ULONG CMallocItem::GetSize(LPVOID pv)
{
    return (ULONG)-1;
}


int CMallocItem::DidAlloc(LPVOID pv)
{
    return -1;
}


void CMallocItem::HeapMinimize(void)
{
    NULL;
}


HRESULT CMallocItem_Create(IMalloc ** ppm)
{
    HRESULT hres = E_OUTOFMEMORY;
    CMallocItem * pmi = new CMallocItem();

    if (pmi)
    {
        hres = pmi->QueryInterface(IID_IMalloc, (LPVOID *) ppm);
        pmi->Release();
    }

    return hres;
}


CMallocItem::CMallocItem() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    LEAK_ADDREF(LEAK_CMallocItem);
}


CMallocItem::~CMallocItem()
{
    DllRelease();
    LEAK_DELREF(LEAK_CMallocItem);
}


// *** IUnknown Interface ***

ULONG CMallocItem::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CMallocItem::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CMallocItem::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IMalloc))
    {
        *ppvObj = SAFECAST(this, IMalloc *);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CMallocItem::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
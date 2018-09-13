#include "precomp.hxx"
#pragma hdrstop

STDMETHODIMP 
ComObject::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) 
ComObject::AddRef(
   VOID
   )
{
    ULONG ulReturn = m_cRef + 1;

    DebugMsg(DM_OLE | DM_NOPREFIX, TEXT("::AddRef,  0x%08X  %d -> %d"), this, ulReturn - 1, ulReturn);
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}

STDMETHODIMP_(ULONG) 
ComObject::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;
    DebugMsg(DM_OLE | DM_NOPREFIX, TEXT("::Release, 0x%08X  %d -> %d"), this, ulReturn + 1, ulReturn);
    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}

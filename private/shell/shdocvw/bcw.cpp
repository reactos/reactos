/****************************************************************************
bcw.cpp

  Owner: Srinik
  Copyright (c) 1995 Microsoft Corporation
  
    This file contains code for BCW class which implements wrappers for IBindCtx
    and IRunningObjectTable. We use this object to trick the moniker binding
    code to create a new instance of the object (that the moniker is
    referring to) instead connecting to already running instance.
****************************************************************************/

// #include "hlmain.h"
#include "bcw.h"

ASSERTDATA

/****************************************************************************
Implementation of BCW methods.
****************************************************************************/

BCW::BCW(IBindCtx * pibc)
{
    m_pibc = pibc;
    pibc->AddRef();
    m_cObjRef = 1;
    DllAddRef();
}

BCW::~BCW()
{
    m_pibc->Release();
    DllRelease();
}

IBindCtx * BCW::Create(IBindCtx * pibc)
{
    BCW * pbcw = new BCW(pibc);
    
    if (pbcw == NULL)
        return NULL;
    
    if (! pbcw->m_ROT.FInitROTPointer())
    {
        delete pbcw;
        return NULL;
    }
    
    return pbcw;
}

STDMETHODIMP BCW::QueryInterface(REFIID riid, void **ppvObj)
{   
    if (ppvObj == NULL)
        return E_INVALIDARG;
    
    if (riid == IID_IUnknown || riid == IID_IBindCtx)
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    ((IUnknown *) *ppvObj)->AddRef();
    return NOERROR; 
}

STDMETHODIMP_(ULONG) BCW::AddRef(void)
{   
    return ++m_cObjRef;
}


STDMETHODIMP_(ULONG) BCW::Release(void)
{
/* Decrement refcount, destroy object if refcount goes to zero.
    Return the new refcount. */
    if (!(--m_cObjRef))
    {
        delete this;
        return 0;
    }
    
    return m_cObjRef;
}


/****************************************************************************
Implementation of BCW_ROT methods.
****************************************************************************/

/****************************************************************************
BCW_ROT is the IRunningObjectTable imlementation of BCW_ROT.
****************************************************************************/

BCW_ROT::BCW_ROT()
{
    Debug(m_cRef = 0);
    m_piROT = NULL; 
}

BCW_ROT::~BCW_ROT()
{   
    if (m_piROT)
        m_piROT->Release();
}

BOOL_PTR BCW_ROT::FInitROTPointer(void)
{
    if (m_piROT == NULL)
    {
        if (GetRunningObjectTable(NULL/*reserved*/, &m_piROT) == NOERROR)
            m_piROT->AddRef();
    }
    
    return (BOOL_PTR) (m_piROT);
}


inline BCW * BCW_ROT::PBCW()
{
    return BACK_POINTER(this, m_ROT, BCW);
}

STDMETHODIMP BCW_ROT::QueryInterface(REFIID riid, void **ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IRunningObjectTable)
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    ((IUnknown *) *ppvObj)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) BCW_ROT::AddRef(void)
{
    return PBCW()->AddRef();
}

STDMETHODIMP_(ULONG) BCW_ROT::Release(void)
{
    return PBCW()->Release();
}


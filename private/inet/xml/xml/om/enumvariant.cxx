/*
 * @(#)EnumVariant.cxx 1.0 8/28/98
 * 
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "enumvariant.hxx"

#ifndef _XML_OM_MSXMLCOM
#include "msxmlcom.hxx"
#endif

IEnumVARIANTWrapper * 
IEnumVARIANTWrapper::newIEnumVARIANTWrapper(IUnknown * pUnk, EnumVariant * pEnumVariant, Mutex * pMutex)
{
    Assert(pEnumVariant);
    Assert(pUnk);
    Assert(pMutex);

    IEnumVARIANTWrapper * pEnum = new IEnumVARIANTWrapper();
    pEnum->_pUnk = pUnk;
    pEnum->_pEnumVariant = pEnumVariant;
    pEnum->_pMutex = pMutex;

    return pEnum;
}


IEnumVARIANTWrapper::~IEnumVARIANTWrapper() 
{
    _pUnk = NULL;
    _pMutex = NULL;
};


Node * 
NodeIteratorState::getNext(EnumVariant * pEnum)
{
    Node * pNode = null;
    if (!_pvLastPos || pEnum->enumValidate(_pvLastPos))
    {
        // just step on to the next sibling
        pNode = pEnum->enumGetNext(&_pvLastPos);
    }
    // since the current pos is bad, try using the saved 'next'
    else if (_pNextNode && pEnum->enumValidate(_pvPos))
    {
        _pvLastPos = _pvPos;
        pNode = _pNextNode;
    }
    else if (_pNextNode)
    {
        // don't throw an error if the list is completely empty
        void * ppv = null;
        if (pEnum->enumGetNext(&ppv))
            Exception::throwE(E_FAIL, XMLOM_NEXTNODEABORT, null);
    }

    if ( pNode)
    {
        // sneak a peak ahead, in case we loose our 'current' position
        _pLastNode = pNode;
        _pvPos = _pvLastPos;
        _pNextNode = pEnum->enumGetNext(&_pvPos);
    }
    else
    {
        _pvPos = null;
        _pNextNode = null;
    }
    return pNode;
}


IDispatch *
NodeIteratorState::enumGetNext(EnumVariant * pEnum)
{
    return pEnum->enumGetIDispatch(getNext(pEnum));
}


HRESULT STDMETHODCALLTYPE 
IEnumVARIANTWrapper::QueryInterface(REFIID riid, void ** ppv)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;

    if (riid == IID_IEnumVARIANT)
    {
        *ppv = this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        // delegate to the IUnknown object that implements the EnumVariant interface 
        hr = _pUnk->QueryInterface(riid, ppv);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE 
IEnumVARIANTWrapper::Next( 
    /* [in] */ ULONG cnodes,
    /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
    /* [out] */ ULONG __RPC_FAR *pCnodesFetched)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;
    ULONG ulFetched = 0;
    MutexReadLock lock(_pMutex);

    if (!rgVar)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    TRY
    {
        while (cnodes > 0)
        {
            IDispatch * pDisp = _State.enumGetNext(_pEnumVariant);
            if (pDisp)
            {
                VariantInit(rgVar);
                rgVar->vt = VT_DISPATCH;
                V_DISPATCH(rgVar) = pDisp;
                rgVar++;
                ulFetched++;
                cnodes--;
            }
            else
            {
                VariantInit(rgVar);
                hr = S_FALSE;
                goto Cleanup;
            }
        }
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:

    if (pCnodesFetched)
        *pCnodesFetched = ulFetched;
    return hr;
}


HRESULT STDMETHODCALLTYPE 
IEnumVARIANTWrapper::Skip( 
    /* [in] */ ULONG cnodes)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;
    MutexReadLock lock(_pMutex);

    TRY
    {

        while (cnodes > 0)
        {
            if (!_State.enumGetNext(_pEnumVariant))
            {
                hr = S_FALSE;
                goto Cleanup;
            }
            cnodes--;
        }
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
   
Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
IEnumVARIANTWrapper::Reset( void)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr = S_OK;

    TRY
    {
        _State.init();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
   
Cleanup:
    return hr;
}
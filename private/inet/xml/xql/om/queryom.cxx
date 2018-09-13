/*
 * @(#)XQLOM.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * Implementation of XQL object model
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "queryom.hxx"

DISPATCHINFO _dispatchexport<Query, IXQLQuery, &LIBID_MSXQL, ID_MSXQL_TLB, &IID_IXQLQuery>::s_dispatchinfo =
{
    NULL, &IID_IXQLQuery, &LIBID_MSXQL, ID_MSXQL_TLB
};

HRESULT STDMETHODCALLTYPE 
IQueryWrapper::get_dataSrc( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *pp)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        hr = getIXMLElement(getWrapped()->getContext(), pp);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::put_dataSrc( 
        /* [in] */ IXMLElement2 __RPC_FAR *p)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        getWrapped()->setContext(null, ::GetElement(p));
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::reset()
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        getWrapped()->reset();
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::get_peek( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *pp)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        hr = getIXMLElement((Element *) getWrapped()->peekElement(), pp);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::get_next( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *pp)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        hr = getIXMLElement((Element *) getWrapped()->nextElement(), pp);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::get_index( 
        /* [out][retval] */ long __RPC_FAR *p)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        *p = getWrapped()->getIndex(null);
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::contains( 
        /* [in] */ IXMLElement2 __RPC_FAR *p,
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pb)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        *pb = contains(p);
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::get_dataSrcDOM( 
        /* [out][retval] */ IDOMNode __RPC_FAR *__RPC_FAR *pp)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        hr = getIDOMNode(getWrapped()->getContext(), pp);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::put_dataSrcDOM( 
        /* [in] */ IDOMNode __RPC_FAR *p)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {   
        getWrapped()->setContext(null, ::GetElement(p));
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::get_peekDOM( 
        /* [out][retval] */ IDOMNode __RPC_FAR *__RPC_FAR *pp)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        hr = getIDOMNode((Element *) getWrapped()->peekElement(), pp);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::get_nextDOM( 
        /* [out][retval] */ IDOMNode __RPC_FAR *__RPC_FAR *pp)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        hr = getIDOMNode((Element *) getWrapped()->nextElement(), pp);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IQueryWrapper::containsDOM( 
        /* [in] */ IDOMNode __RPC_FAR *p,
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pb)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        *pb = contains(p);
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


VARIANT_BOOL 
IQueryWrapper::contains(IDispatch *p)
{
    if (getWrapped()->contains(null, ::GetElement(p)) != null)
    {
        return VARIANT_TRUE;
    }

    return VARIANT_FALSE;
}


HRESULT
IQueryWrapper::getIXMLElement(Element * e, IXMLElement2 ** pp)
{
    HRESULT hr;

    if (e != null)
    {
        checkhr(e->QueryInterface(IID_IXMLElement2, (void **) pp));
        hr = S_OK;
    }
    else
    {
        *pp = null;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT
IQueryWrapper::getIDOMNode(Element * e, IDOMNode ** pp)
{
    HRESULT hr;

    if (e != null)
    {
        checkhr(e->QueryInterface(IID_IDOMNode, (void **) pp));
        hr = S_OK;
    }
    else
    {
        *pp = null;
        hr = S_FALSE;
    }

    return hr;
}

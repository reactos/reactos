/*
 * @(#)ParseOM.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * Implementation of xql parser object model
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xql/parser/xqlparser.hxx"
#include "queryom.hxx"
#include "parserom.hxx"

DISPATCHINFO _dispatchexport<XQLParser, IXQLParser, &LIBID_MSXQL, ID_MSXQL_TLB, &IID_IXQLParser>::s_dispatchinfo =
{
    NULL, &IID_IXQLParser, &LIBID_MSXQL, ID_MSXQL_TLB
};

HRESULT STDMETHODCALLTYPE 
IXQLParserWrapper::getQuery( 
        /* [in] */ BSTR bstrQuery,
        /* [out][retval] */ IXQLQuery __RPC_FAR *__RPC_FAR *pp)
{
    HRESULT hr;
    STACK_ENTRY;

    TRY
    {
        Query * qry = getWrapped()->parse(bstrQuery);
        *pp = new IQueryWrapper(qry);
        hr = S_OK;
    }
    CATCH
    {
        *pp = null;
        _dispatchImpl::setErrorInfo(GETEXCEPTION());
        hr = ERESULT;
    }
    ENDTRY
    return hr;
}


HRESULT STDMETHODCALLTYPE 
IXQLParserWrapper::getSortedQuery( 
        /* [in] */ BSTR bstrQuery,
        /* [in] */ BSTR bstrOrderBy,
        /* [out][retval] */ IXQLQuery __RPC_FAR *__RPC_FAR *pp)
{
    HRESULT hr;
    STACK_ENTRY;

    TRY
    {
        Query * qry = getWrapped()->parse(bstrQuery);
        qry = getWrapped()->parseOrderBy(qry, bstrOrderBy);
        *pp = new IQueryWrapper(qry);
        hr = S_OK;
    }
    CATCH
    {
        *pp = null;
        _dispatchImpl::setErrorInfo(GETEXCEPTION());
        hr = ERESULT;
    }
    ENDTRY
    return hr;
}



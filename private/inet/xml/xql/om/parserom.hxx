/*
 * @(#)XQLOM.hxx 1.0 6/3/97
 * 
 * Implementation of xql object model interfaces.
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_OM_PARSEROM
#define _XQL_OM_PARSEROM

#ifndef _XQL_PARSE_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif


class IXQLParserWrapper : public _dispatchexport<XQLParser, IXQLParser, &LIBID_MSXQL, ID_MSXQL_TLB, &IID_IXQLParser>
{
public:
    IXQLParserWrapper(XQLParser * p) :
        _dispatchexport<XQLParser, IXQLParser, &LIBID_MSXQL, ID_MSXQL_TLB, &IID_IXQLParser>(p)
        {}

    // IXQLParser

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getQuery( 
        /* [in] */ BSTR bstrQuery,
        /* [out][retval] */ IXQLQuery __RPC_FAR *__RPC_FAR *pp);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getSortedQuery( 
        /* [in] */ BSTR bstrQuery,
        /* [in] */ BSTR bstrOrderBy,
        /* [out][retval] */ IXQLQuery __RPC_FAR *__RPC_FAR *pp);
};

#endif _XQL_OM_PARSEROM


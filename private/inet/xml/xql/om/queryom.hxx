/*
 * @(#)QueryOm.hxx 1.0 6/3/97
 * 
 * Implementation of xql object model interfaces.
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_OM_QUERYOM
#define _XQL_OM_QUERYOM

#ifndef _XQL_QUERY_QUERY
#include "xql/query/query.hxx"
#endif

#include "xql/dll/resource.h"

class IQueryWrapper : public _dispatchexport<Query, IXQLQuery, &LIBID_MSXQL, ID_MSXQL_TLB, &IID_IXQLQuery>
{
public:
    IQueryWrapper(Query * p) :
        _dispatchexport<Query, IXQLQuery, &LIBID_MSXQL, ID_MSXQL_TLB, &IID_IXQLQuery>(p)
        {}

    // IXQLQuery Interface

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_dataSrc( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *pp);

    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_dataSrc( 
        /* [in] */ IXMLElement2 __RPC_FAR *p);

    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE reset();

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_peek( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *pp);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_next( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *pp);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_index( 
        /* [out][retval] */ long __RPC_FAR *p);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE contains( 
        /* [in] */ IXMLElement2 __RPC_FAR *p,
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pb);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_dataSrcDOM( 
        /* [out][retval] */ IDOMNode __RPC_FAR *__RPC_FAR *pp);

    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_dataSrcDOM( 
        /* [in] */ IDOMNode __RPC_FAR *p);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_peekDOM( 
        /* [out][retval] */ IDOMNode __RPC_FAR *__RPC_FAR *pp);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_nextDOM( 
        /* [out][retval] */ IDOMNode __RPC_FAR *__RPC_FAR *pp);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE containsDOM( 
        /* [in] */ IDOMNode __RPC_FAR *p,
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pb);

private:
    VARIANT_BOOL contains(IDispatch *p);
    HRESULT getIXMLElement(Element * e, IXMLElement2 ** pp);
    HRESULT getIDOMNode(Element * e, IDOMNode ** pp);
};


#endif _XQL_OM_QUERYOM


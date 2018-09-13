/*
 * @(#)XQLNodeList.hxx 1.0 7/6/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _XML_OM_XQLNODELIST
#define _XML_OM_XQLNODELIST

#ifndef _XML_OM_NODE_HXX
#include "xml/om/node.hxx"
#endif

#ifndef _DISPATCH_HXX
#include "core/com/_dispatch.hxx"
#endif

#ifndef _XQL_PARSER_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

#ifndef _XQL_QUERY_QUERY
#include "xql/query/query.hxx"
#endif

#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

#ifndef _MT_HXX
#include "mt.hxx"
#endif

#ifndef _ENUMVARIANT_HXX
#include "enumvariant.hxx"
#endif

#ifdef UNIX
// This is a Apogee (HP and Solaris Unix) compiler bug workaround!
// If you change it you may totally break XLM under Unix
class XQLNodeList : public _DOMList, public EnumVariant                
#else
class XQLNodeList : public _dispatchEx<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList, true>, public EnumVariant                
#endif
{
public:
            XQLNodeList();
    virtual ~XQLNodeList();

    // internal method for setting up node list.  Throws exceptions.
    void setQuery(const String* xql, Element* pContext);

    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    ////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMNodeList Interface
    //
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_item( 
        /* [in] */ long index,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [retval][out] */ long __RPC_FAR *pLength);

    virtual /* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
        /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);

    ////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMNodeList Interface

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE nextNode( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE reset( void);

    ////////////////////////////////////////////////////////////////////////////////
    // EnumVariant Interface

    virtual IDispatch * enumGetIDispatch(Node *);
    virtual Node * enumGetNext(void **);

private:
    HRESULT moveTo( 
        /* [in] */ long lIndex,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);

    Node * moveToNode(long lIndex);

    Element*            _next();
//    Element*            _previous();
    HRESULT             getIDOMNode(Element * e, IXMLDOMNode ** pp);

    _reference<Query>       _pQuery;
    RVector                 _pResults; // cached results so we can do previousNode.
    bool                    _fCacheComplete;
    long                    _lIndex;

    _reference<Mutex>       _pMutex;
};

#endif _XML_OM_XQLNODELIST

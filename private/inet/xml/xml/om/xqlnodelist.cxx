/*
 * @(#)XQLNodeList.cxx 1.0 7/6/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#ifndef _XML_OM_IXMLDOMNODE
#include "domnode.hxx"
#endif

#ifndef _XML_OM_XQLNODELIST
#include "xqlnodelist.hxx"
#endif

#ifndef _XQL_PARSER_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

#define CleanupTEST(t)  if (!t) { hr=S_FALSE; goto Cleanup; }
#define ARGTEST(t)  if (!t) { hr=E_INVALIDARG; goto Cleanup; }

//=======================================================================
/*  NOTE: THIS CONSTRUCTOR HAS BEEN MOVED
**  to domnode.cxx, because to not do so breaks all of XQL under Unix
**  due to a unix compiler bug which has to do with statics and templates
**
XQLNodeList::XQLNodeList()
{
    _pResults = new Vector(5);
    _lIndex = -1;
    _fCacheComplete = false;
}
*
*/

XQLNodeList::~XQLNodeList()
{
}

extern CSMutex * g_pMutex;

void
XQLNodeList::setQuery(const String* xql, Element* pContext)
{
    //long __RPC_FAR pLength;
    //HRESULT hr;


    XQLParser* parser = new XQLParser(true); // GC'd object.
    _pQuery = parser->parse(xql);
    _pQuery->setContext(null, pContext);

    Assert(pContext);
    Document * pDoc = pContext->getDocument();
    if (pDoc)
        _pMutex = pDoc->getMutex();
    else
        _pMutex = g_pMutex;

    //hr = get_length(&pLength);
}

////////////////////////////////////////////////////////////////////////////////
// IXMLDOMNodeList Interface
//

HRESULT STDMETHODCALLTYPE 
XQLNodeList::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr = S_OK;

    TRY
    {
        if (iid == IID_IUnknown || iid == IID_IDispatch ||
            iid == IID_IXMLDOMNodeList)
        {
            AddRef();
            *ppv = SAFE_CAST(IXMLDOMNodeList *,this);
        }
        else if (iid == IID_IEnumVARIANT)
        {
            hr = get__newEnum((IUnknown**)ppv);
        }
        else if (iid == IID_IDispatchEx)
        {
            AddRef();
            *ppv = SAFE_CAST(IDispatchEx *, this);
        }
        else
        {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE 
XQLNodeList::get_item( 
    /* [in][optional] */ long index,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY_IUNKNOWN(this);

    HRESULT hr = moveTo(index, (IXMLDOMNode**)ppNode);
    return hr;
}

HRESULT STDMETHODCALLTYPE 
XQLNodeList::get_length( 
    /* [out][retval] */ long __RPC_FAR *pl)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;

    ARGTEST(pl);
    *pl = 0;

    TRY
    {
        // Fill the cache then return the cache size.
        while (!_fCacheComplete)
            _next();
        *pl = _pResults->size();
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
XQLNodeList::nextNode( 
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr = moveTo(_lIndex+1, ppNode);
    return hr; 
}

HRESULT STDMETHODCALLTYPE 
XQLNodeList::reset( void)
{
    _lIndex = -1;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
XQLNodeList::get__newEnum( 
    /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{
    STACK_ENTRY_IUNKNOWN(this);
    TraceTag((tagDOMOM, "XQLNodeList::get__newEnum"));

    HRESULT hr = S_OK;

    ARGTEST( ppUnk);

    TRY
    {
        // Create a new enumeration that enumerates through the same
        // set of children as this XQLNodeList.
        *ppUnk = IEnumVARIANTWrapper::newIEnumVARIANTWrapper((IXMLDOMNodeList*)this, (EnumVariant*)this, _pMutex);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// EnumVariant Interface
/////////////////////////////////////////////////////////////////////////////
// This method is called by the IEnumVARIANTWrapper, so no lock or try/catch 
// is needed here.
IDispatch *
XQLNodeList::enumGetIDispatch(Node * pNode)
{
    if (pNode)
    {
        IXMLDOMNode * pDisp;
        HRESULT hr;
        hr = getIDOMNode(pNode, &pDisp);
        if (!hr)
            return (IDispatch *)pDisp;
    }
    return null;
}

Node *
XQLNodeList::enumGetNext(void ** ppv)
{
    long * pl = (long *)ppv;
    Node * pNode = moveToNode(*pl);
    if (pNode)
        ++(*pl);
    return pNode;
}

    
// -------------- PRIVATE METHODS --------------------------------------
Node *
XQLNodeList::moveToNode(long lIndex)
{
    if (lIndex >= 0)
    {
        // We now prefect all nodes in setQuery(), so we should not
        // be doing any more fetching in here.  So don't uncomment
        // the following couple lines of code.  And don't try to get
        // around the prefetching behavior because the XQL performance
        // is assuming all results are prefetched. Jan 14, 1999 (wiladams)
        // Fill the cache up to the index required.
        while (!_fCacheComplete && (lIndex+1) >= _pResults->size())
            _next();

        if (lIndex < _pResults->size())
        {
            Element* e = (Element*)_pResults->elementAt(lIndex);
            _lIndex = lIndex;
            return CAST_TO(ElementNode*, e);
        }
    }
    return null;
}

HRESULT  
XQLNodeList::moveTo( 
    /* [in] */ long lIndex,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;

    ARGTEST(ppNode);
    *ppNode = 0;

    TRY
    {
        ElementNode* e = moveToNode(lIndex);
        hr = getIDOMNode(e, ppNode);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
Cleanup:
    return hr;
}

Element*
XQLNodeList::_next()
{
    Element* e = null;
    long size = _pResults->size();
    if (! _fCacheComplete)
    {
        e = (Element *) _pQuery->nextElement();
        if (e)
        {
            _pResults->addElement(e);
        }
        else
        {
            _fCacheComplete = true;
        }
    }
    return e;
}


HRESULT
XQLNodeList::getIDOMNode(Element * e, IXMLDOMNode ** pp)
{
    HRESULT hr;

    if (e != null)
    {
        checkhr(e->QueryInterface(IID_IXMLDOMNode, (void **) pp));
        hr = S_OK;
    }
    else
    {
        *pp = null;
        hr = S_FALSE;
    }

    return hr;
}

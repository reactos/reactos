/*
 * @(#)w3cdom.cxx 1.0 3/13/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#undef SINGLE_THREAD_OM

#ifndef _XML_OM_IDOMNODE
#include "xml/om/domnode.hxx"
#endif

#ifndef _XML_OM_NODE_HXX
#include "xml/om/node.hxx"
#endif

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _XML_OM_XQLNODELIST
#include "xqlnodelist.hxx"
#endif

#ifndef _XMLNAMES_HXX
#include "xmlnames.hxx"
#endif

#ifndef _CORE_UTIL_CHARTYPE_HXX
#include "core/util/chartype.hxx"
#endif

#ifndef _XML_OM_OMLOCK
#include "xml/om/omlock.hxx"
#endif

#include <xmldomdid.h>

#define CleanupTEST(t)  if (!t) { hr=S_FALSE; goto Cleanup; }
#define ARGTEST(t)  if (!t) { hr=E_INVALIDARG; goto Cleanup; }

extern TAG tagDOMOM;

W3CDOMWrapper::W3CDOMWrapper( DOMNode * pDOMNode)
{
    _lRefCount = 1;
    _pDOMNode = pDOMNode;
}

W3CDOMWrapper::~W3CDOMWrapper()
{
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::QueryInterface(REFIID iid, void ** ppv)
{
    return _pDOMNode->QueryInterface(iid, ppv);
}

ULONG STDMETHODCALLTYPE 
W3CDOMWrapper::AddRef()
{
    STACK_ENTRY_IUNKNOWN(_pDOMNode);

    TraceTag((tagDOMOM, "AddRef W3CDOMWrapper for %X = %d", _pDOMNode ? getNodeData() : 0, _lRefCount+1));

    ULONG ulRval;
    ulRval = (ULONG)InterlockedIncrement(&_lRefCount);

    _pDOMNode->AddRef();

    return ulRval;
};

ULONG STDMETHODCALLTYPE 
W3CDOMWrapper::Release()
{
    STACK_ENTRY_IUNKNOWN(_pDOMNode);

    TraceTag((tagDOMOM, "Release W3CDOMWrapper for %X = %d", _pDOMNode ? getNodeData() : 0, _lRefCount+1));

    ULONG ulRval;
    if ( (ulRval = (ULONG)InterlockedDecrement(&_lRefCount)) == 0) 
    {
        delete this;
    }
    else
    {
        _pDOMNode->Release();
    }
    return ulRval;

};


HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::GetTypeInfoCount( 
    /* [out] */ UINT __RPC_FAR *pctinfo)
{
    return _pDOMNode->GetTypeInfoCount(pctinfo);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::GetTypeInfo( 
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return _pDOMNode->GetTypeInfo(iTInfo, lcid, ppTInfo);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::GetIDsOfNames( 
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    return _pDOMNode->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::Invoke( 
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    return _pDOMNode->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}


//////////////////////////////////////////////
//// IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_nodeType( 
    /* [out][retval] */ DOMNodeType __RPC_FAR *plType)
{
    return _pDOMNode->get_nodeType(plType);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_nodeName( 
    /* [out][retval] */ BSTR __RPC_FAR *pName)
{
    return _pDOMNode->get_nodeName(pName);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_nodeValue( 
    /* [out][retval] */ VARIANT __RPC_FAR *pVal)
{
    return _pDOMNode->get_nodeValue(pVal);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::put_nodeValue( 
    /* [in] */ VARIANT vVal)
{
    return _pDOMNode->put_nodeValue(vVal);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_parentNode( 
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    return _pDOMNode->get_parentNode(ppNode);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_childNodes( 
    /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
{
    return _pDOMNode->get_childNodes(ppNodeList);
}
HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_firstChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppFirstChild)
{
    return _pDOMNode->get_firstChild(ppFirstChild);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_lastChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppLastChild)
{
    return _pDOMNode->get_lastChild(ppLastChild);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_previousSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppPrevSib)
{
    return _pDOMNode->get_previousSibling(ppPrevSib);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_nextSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNextSib)
{
    return _pDOMNode->get_nextSibling(ppNextSib);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::insertBefore( 
    /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
    /* [in] */ VARIANT refChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
{
    return _pDOMNode->insertBefore(newChild, refChild, ppNewChild);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::replaceChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *pNew,
    /* [in] */ IXMLDOMNode __RPC_FAR *pOld,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    return _pDOMNode->replaceChild(pNew, pOld, ppNode);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::removeChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppOldChild)
{
    return _pDOMNode->removeChild(oldChild, ppOldChild);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::appendChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
{
    return _pDOMNode->appendChild(newChild, ppNewChild);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_attributes( 
    /* [out][retval] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppAttributes)
{
    return _pDOMNode->get_attributes(ppAttributes);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::hasChildNodes( 
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBool)
{
    return _pDOMNode->hasChildNodes(pBool);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::cloneNode( 
    /* [in] */ VARIANT_BOOL deep,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot)
{
    return _pDOMNode->cloneNode(deep, ppCloneRoot);
}


HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_ownerDocument( 
    IXMLDOMDocument __RPC_FAR *__RPC_FAR * ppDOMDocument)
{
    STACK_ENTRY;
    OMREADLOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::get_ownerDocument()"));

    HRESULT hr;

    ARGTEST( ppDOMDocument);

    TRY
    {
        hr = getNodeData()->getDocument()->QueryInterface(IID_IXMLDOMDocument, (void**)ppDOMDocument);
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
W3CDOMWrapper::get_nodeTypeString( 
    /* [out][retval] */ BSTR __RPC_FAR *nodeType)
{
    return _pDOMNode->get_nodeTypeString(nodeType);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_text( 
    /* [out][retval] */ BSTR __RPC_FAR *text)
{
    return _pDOMNode->get_text(text);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::put_text( 
    /* [in] */ BSTR text)
{
    return _pDOMNode->put_text(text);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_definition( 
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    return _pDOMNode->get_definition(ppNode);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_nodeTypedValue( 
    /* [out][retval] */ VARIANT __RPC_FAR *TypedValue)
{
    return _pDOMNode->get_nodeTypedValue(TypedValue);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::put_nodeTypedValue( 
    /* [in] */ VARIANT TypedValue)
{
    return _pDOMNode->put_nodeTypedValue(TypedValue);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_dataType( 
    /* [out][retval] */ VARIANT __RPC_FAR *p)
{
    return _pDOMNode->get_dataType(p);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::put_dataType( 
    /* [in] */ BSTR p)
{
    return _pDOMNode->put_dataType(p);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_xml(
    /* [out][retval] */ BSTR __RPC_FAR *pbstrXml)
{
    return _pDOMNode->get_xml(pbstrXml);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::transformNode( 
    /* [in] */ IXMLDOMNode __RPC_FAR *pStyleSheet,
    /* [out][retval] */ BSTR __RPC_FAR *pbstr)
{
    return _pDOMNode->transformNode(pStyleSheet, pbstr);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::selectNodes( 
    /* [in] */ BSTR bstrXQL,
    /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppResult)
{
    return _pDOMNode->selectNodes(bstrXQL, ppResult);
}
    
HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::selectSingleNode( 
    /* [in] */ BSTR queryString,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *resultNode)
{
    return _pDOMNode->selectSingleNode(queryString, resultNode);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_parsed( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isParsed)
{
    return _pDOMNode->get_parsed(isParsed);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_namespaceURI( 
    /* [out][retval] */ BSTR __RPC_FAR *pURI)
{
    return _pDOMNode->get_namespaceURI(pURI);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_prefix( 
    /* [out][retval] */ BSTR __RPC_FAR *pPrefix)
{
    return _pDOMNode->get_prefix(pPrefix);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_baseName( 
    /* [out][retval] */ BSTR __RPC_FAR *pBaseName)
{
    return _pDOMNode->get_baseName(pBaseName);
}


HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::transformNodeToObject( 
    /* [in] */ IXMLDOMNode __RPC_FAR *stylesheet,
    /* [in] */ VARIANT outputObject)
{
    return _pDOMNode->transformNodeToObject(stylesheet, outputObject);
}


//////////////////////////////////////////////
//// IXMLDOMElement : IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_tagName( 
    /* [retval][out] */ BSTR __RPC_FAR *pTagName)
{
    return _pDOMNode->get_nodeName( pTagName);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::getAttribute( 
    /* [in] */ BSTR name,
    /* [retval][out] */ VARIANT __RPC_FAR *pValue)
{
    STACK_ENTRY;
    OMREADLOCK(this->_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::getAttribute()"));

    HRESULT hr = S_OK;

    ARGTEST( name);
    ARGTEST( pValue);
    VariantInit(pValue);

    TRY
    {
        Node * pNode = getNodeData();
        Document * pDoc = pNode->getDocument();
        Node * pAttr = pNode->findByNodeName( name, Element::ATTRIBUTE, pDoc);
        Object * pObj = null;
        if (pAttr && (pObj = pAttr->getNodeValue()))
        {
            String * pStr = pObj->toString();
            pValue->vt = VT_BSTR;
            V_BSTR(pValue) = pStr->getBSTR();
        }
        else
        {
            pValue->vt = VT_NULL;
            V_BSTR(pValue) = null;
            hr = S_FALSE;
        }
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
W3CDOMWrapper::setAttribute( 
    /* [in] */ BSTR name,
    /* [in] */ VARIANT value)
{
    STACK_ENTRY;
    OMWRITELOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::setAttribute()"));

    HRESULT hr;
    VARIANT vBstrVal; vBstrVal.vt = VT_NULL;
    BSTR bstrVal;
    hr = VariantChangeTypeEx(&vBstrVal, &value, GetThreadLocale(), NULL, VT_BSTR);
    if (hr)
        goto Cleanup;
    bstrVal = V_BSTR(&vBstrVal);

    ARGTEST( name);

    TRY
    {
        Node * pNode = getNodeData();
        Document * pDoc = pNode->getDocument();
        String * pValue;

        pNode->checkReadOnly();

        if (bstrVal)
            pValue = String::newString( bstrVal);
        else
            pValue = String::emptyString();

        pNode->setNodeAttribute(null, name, pValue, null);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY


Cleanup:
    VariantClear(&vBstrVal);
    return hr;
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::removeAttribute( 
    /* [in] */ BSTR name)
{
    STACK_ENTRY;
    OMWRITELOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::removeAttribute()"));

    HRESULT hr = S_OK;

    ARGTEST( name);

    TRY
    {
        Node * pNode = getNodeData();
        Document * pDoc = pNode->getDocument();
        pNode->checkReadOnly();

        Node * pAttr = pNode->findByNodeName( name, Element::ATTRIBUTE);
        if (pAttr)
            pNode->removeNode(pAttr, true);
        else
            hr = S_FALSE;
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
W3CDOMWrapper::getAttributeNode( 
    /* [in] */ BSTR name,
    /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *ppAttr)
{
    STACK_ENTRY;
    OMREADLOCK(this->_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::getAttributeNode()"));

    HRESULT hr = S_OK;

    ARGTEST( name);
    if (ppAttr)
        *ppAttr = NULL;

    TRY
    {
        Node * pNode = getNodeData();
        Document * pDoc = pNode->getDocument();
        Node * pAttr = pNode->findByNodeName( name, Element::ATTRIBUTE, pDoc);
        if (pAttr && ppAttr)
        {
            hr = pAttr->QueryInterface(IID_IXMLDOMAttribute, (void **)ppAttr);
        }
        else
            hr = S_FALSE;
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
W3CDOMWrapper::setAttributeNode( 
        /* [in] */ IXMLDOMAttribute __RPC_FAR * pDOMAttribute,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR * ppAttributeNode)
{
    STACK_ENTRY;
    OMWRITELOCK(this->_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::setAttributeNode()"));

    HRESULT hr = S_OK;

    ARGTEST( pDOMAttribute);

    TRY
    {
        Node * pNode = getNodeData();
        Node * pNewAttr = null;
        pNode->checkReadOnly();

        hr = pDOMAttribute->QueryInterface( Node::s_IID, (void **)&pNewAttr);
        if (hr != S_OK || !pNewAttr || pNewAttr->getNodeType() != Element::ATTRIBUTE)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        Node * pAttr = pNode->findByNameDef( pNewAttr->getNameDef(), Element::ATTRIBUTE, null); // null doc so we don't look up defaults

        if (pAttr)
        {
            pNode->replaceAttr( pNewAttr, pAttr);
            if (ppAttributeNode)
            {
                hr = pAttr->QueryInterface(IID_IXMLDOMAttribute, (void**)ppAttributeNode);
            }
        }
        else
        {
            pNode->insertAttr( pNewAttr);
            if (ppAttributeNode)
                *ppAttributeNode = null;
        }
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
W3CDOMWrapper::removeAttributeNode( 
        /* [in] */ IXMLDOMAttribute __RPC_FAR * pDOMAttribute,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR * ppAttributeNode)
{
    STACK_ENTRY;
    OMWRITELOCK(this->_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::removeAttributeNode()"));

    HRESULT hr = S_OK;

    ARGTEST( pDOMAttribute);

    TRY
    {
        Node * pNode = getNodeData();
        Node * pAttr = null;
        pNode->checkReadOnly();

        hr = pDOMAttribute->QueryInterface( Node::s_IID, (void **)&pAttr);
        if (hr != S_OK || !pAttr || pAttr->getNodeType() != Element::ATTRIBUTE ||
            (pAttr != pNode->findByNameDef( pAttr->getNameDef(), Element::ATTRIBUTE, null))) // null doc so we don't look up defaults
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        
        pNode->removeNode( pAttr);
        
        if (ppAttributeNode)
        {
            pDOMAttribute->AddRef();
            *ppAttributeNode = pDOMAttribute;
        }
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
W3CDOMWrapper::getElementsByTagName( 
    /* [in] */ BSTR tagname,
    /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
{
    HRESULT hr = _pDOMNode->getElementsByTagName(tagname, ppNodeList);

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::normalize( void)
{
    STACK_ENTRY;
    OMWRITELOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::normalize()"));

    HRESULT hr;

    TRY
    {
        Node * pNode = getNodeData();
        pNode->checkReadOnly();
        pNode->normalize();
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

//////////////////////////////////////////////
//// IXMLDOMAttribute : IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_specified( 
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pFlag)
{
    return _pDOMNode->get_specified( pFlag);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_value( 
    /* [retval][out] */ VARIANT __RPC_FAR *pVal)
{
    return _pDOMNode->get_nodeValue(pVal);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::put_value( 
    /* [retval][out] */ VARIANT val)
{
    return _pDOMNode->put_nodeValue(val);
}

//////////////////////////////////////////////
//// IXMLDOMProcessingInstruction : IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_target( 
    /* [retval][out] */ BSTR __RPC_FAR *pStr)
{
    return _pDOMNode->get_nodeName(pStr);
}

//////////////////////////////////////////////
//// IXMLDOMData : IXMLDOMNode
HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_data( 
    /* [retval][out] */ BSTR __RPC_FAR *pData)
{
    if (!pData)
        return E_INVALIDARG;
    VARIANT vData;
    VariantInit(&vData);
    HRESULT hr = _pDOMNode->get_nodeValue(&vData);
    if (S_OK == hr)
    {
        Assert(VT_BSTR == vData.vt);
        *pData = V_BSTR(&vData);
    }
    else
        *pData = NULL;
    return hr;
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::put_data( 
    /* [in] */ BSTR data)
{
    return _pDOMNode->put_text(data);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_length( 
    /* [retval][out] */ long __RPC_FAR * pLength)
{
    STACK_ENTRY;
    OMREADLOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::get_length()"));

    HRESULT hr = S_OK;

    ARGTEST( pLength);

    TRY
    {
        Node * pNode = getNodeData();
        String *pString = pNode->getDOMNodeValue();
        if (pString)
        {
            *pLength = pString->length();
        }
        else
        {
            *pLength = 0;
        }
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
W3CDOMWrapper::substringData( 
    /* [in] */ long start,
    /* [in] */ long count,
    /* [retval][out] */ BSTR __RPC_FAR *pData)
{
    STACK_ENTRY;
    OMREADLOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::substring()"));

    HRESULT hr = S_FALSE;

    ARGTEST( pData);
    *pData = NULL;

    if (start < 0 || count < 0)
    {
        _dispatchImpl::setErrorInfo(XMLOM_INVALID_INDEX);
        hr = E_INVALIDARG;
        goto Cleanup;     
    }

    if (count == 0)
    {
        goto Cleanup;     
    }

    TRY
    {
        const ATCHAR * pText = getNodeData()->getDOMNodeValue()->getCharArray();
        if (pText)
        {
            int len = pText->length();
            if (start <= len)
            {
                if (len)
                {
                    if (start + count > len)
                        count = len - start;
                    *pData = String::newString(pText)->substring(start, start + count)->getBSTR();
                    hr = S_OK;
                }
            }
            else
            {
                _dispatchImpl::setErrorInfo(XMLOM_INVALID_INDEX);
                hr = E_INVALIDARG;
            }
        }
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
W3CDOMWrapper::appendData( 
    /* [in] */ BSTR arg)
{
    STACK_ENTRY;

    TraceTag((tagDOMOM, "W3CDOMWrapper::appendData()"));

    HRESULT hr = S_OK;
    long len;

    if (arg)
        len = _tcslen(arg);
    else 
        len = 0;

    if (len)
    {
        {
            OMREADLOCK(_pDOMNode); // read lock so we can get the current text value.
            TRY
            {
                Node * pNode = getNodeData();
                pNode->checkReadOnly();
                const ATCHAR * pText = pNode->getDOMNodeValue()->getCharArray();
                if (pText)
                    len = pText->length();
                else
                    len = 0;
            }
            CATCH
            {
                hr = ERESULTINFO;
            }
            ENDTRY
        }

        if (!hr)
            hr = replaceData(len, 0, arg);
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::insertData( 
    /* [in] */ long offset,
    /* [in] */ BSTR arg)
{
    HRESULT hr;

    if (!arg || (0 == _tcslen(arg)))
        hr = S_OK;
    else 
        hr = replaceData(offset, 0, arg);

    return hr;
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::deleteData( 
    /* [in] */ long offset,
    /* [in] */ long count)
{
    return replaceData(offset, count, NULL);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::replaceData( 
    /* [in] */ long offset,
    /* [in] */ long count,
    /* [in] */ BSTR arg)
{
    STACK_ENTRY;
    OMWRITELOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::replace()"));

    HRESULT hr = S_FALSE;

    if ((offset < 0) || (count < 0))
    {
        _dispatchImpl::setErrorInfo(XMLOM_INVALID_INDEX);
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    TRY
    {
        int len = 0;
        const ATCHAR * pText;
        Node * pNode = getNodeData();
        pNode->checkReadOnly();

        pText = pNode->getInnerText(true, true, false)->getCharArray();

        if (pText)
        {
            // get normalized text length
            int l = pText->length();
            len = _getNormalizedLength(const_cast<TCHAR *>(pText->getData()), l, l);
        }

        if (offset <= len)
        {
            ATCHAR * pS = NULL;
            int length;

            if (arg)
                length = _tcslen(arg);
            else
                length = 0;

            if (count + offset > len)
            {
                count = len - offset;
            }

            if (length + len - count > 0)
            {
                // Count the offset and count in the actual text
                TCHAR *pch = const_cast<TCHAR *>(pText->getData());
                int len1 = pText->length();
                int offset1 = _getActualLength(pch, offset, len1);
                int count1 = _getActualLength(pch + offset1, count, len1 - offset1);

                pS = new (length + len1 - count1) ATCHAR; 

                if (offset1 > 0)
                {
                    pS->copy(0, offset1, pText, 0);
                }

                if (length > 0)
                {
                    pS->simpleCopy(offset1, length, arg);
                }

                if (offset1 + count1 < len1)
                {
                    pS->copy(offset1 + length, len1 - offset1 - count1, pText, offset1 + count1);
                }
            }

            pNode->setInnerText(pS);

            hr = S_OK;
        }
        else
        {
            _dispatchImpl::setErrorInfo(XMLOM_INVALID_INDEX);
            hr = E_INVALIDARG;
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}

//////////////////////////////////////////////
//// IXMLDOMComment : IXMLDOMData

//////////////////////////////////////////////
//// IXMLDOMText : IXMLDOMData

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::splitText( 
    /* [in] */ long offset,
    /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *ppNewTextNode)
{
    STACK_ENTRY;
    OMWRITELOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::splitText()"));

    HRESULT hr = S_FALSE;

    if (offset < 0)
    {
        _dispatchImpl::setErrorInfo(XMLOM_INVALID_INDEX);
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    ARGTEST( ppNewTextNode);
    *ppNewTextNode = NULL;

    TRY
    {
        Node * pNode = getNodeData();
        pNode->checkReadOnly();
        const ATCHAR * pText = pNode->getInnerText(true, true, false)->getCharArray();
        TCHAR * pch = const_cast<TCHAR *>(pText->getData());
        int len = 0, len1 = 0;

        if (pText)
        {
            len1 = pText->length();
            // get normalized text length
            len = _getNormalizedLength(pch, len1, len1);
        }

        if (offset > len)
        {
            _dispatchImpl::setErrorInfo(XMLOM_INVALID_INDEX);
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (0 == len || offset == len)
        {
            goto Cleanup;
        }

        // The original node contains all the content up to the offset point
        int offset1 = _getActualLength(pch, offset, len1);
        ATCHAR * pS1 = new (offset1) ATCHAR;
        pS1->copy(0, offset1, pText, 0);

        // create a new text node
        Document * pDoc = pNode->getDocument();
        Node * pNodeNew = Node::newNode(pNode->getNodeType(), null, pDoc, pDoc->getNodeMgr());
        DOMNode * pDOMNode = pNodeNew->getDOMNodeWrapper();
        Assert(pDOMNode);

        hr = pDOMNode->QueryInterface(IID_IXMLDOMText, (void**)ppNewTextNode);
        Assert(S_OK == hr);
        pDOMNode->Release();

        // The new text node contains all the content at and after the offset point
        ATCHAR * pS2 = new (len1 - offset1) ATCHAR; 
        pS2->copy(0, len1 - offset1, pText, offset1); 
        pNodeNew->setInnerText(pS2);

        // get parent, and insert if parent exists
        Node * pParent = (Node*)pNode->getNodeParent();
        if (pParent)
        {
            pParent->_insert(pNodeNew, pNode->getNextSibling());
        }

        pNode->setInnerText(pS1);

        goto Cleanup;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Error:
    if (*ppNewTextNode)
    {
        (*ppNewTextNode)->Release();
        *ppNewTextNode = NULL;
    }

Cleanup:
    return hr;
}

//////////////////////////////////////////////
//// IXMLDOMCDATASection : IXMLDOMText

//////////////////////////////////////////////
//// IXMLDOMDocumentFragment

//////////////////////////////////////////////
//// IXMLDOMEntity : IXMLDOMNode

HRESULT
W3CDOMWrapper::_getAttrValue(Name * pName, BSTR * pbstrVal)
{
    STACK_ENTRY;
    OMREADLOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::_getAttrValue()"));

    HRESULT hr;

    ARGTEST(pbstrVal);

    TRY
    {
        Node * pAttr = getNodeData()->find(pName, Element::ATTRIBUTE, NULL);

        if (pAttr)
        {
            Object * pObj = pAttr->getNodeValue();
            if (pObj)
            {
                String * pstr = pObj->toString();
                if (pstr)
                {
                    *pbstrVal = pstr->getBSTR();
                    hr = S_OK;
                    goto Cleanup;
                }
            }
        }
   
        *pbstrVal = NULL;
        hr = S_FALSE;
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
W3CDOMWrapper::get_publicId( 
    /* [retval][out] */ VARIANT __RPC_FAR *pVal)
{
    BSTR bstr = NULL;
    if (!pVal)
        return E_INVALIDARG;
    HRESULT hr = _getAttrValue(XMLNames::name(NAME_PUBLIC), &bstr);
    pVal->vt = VT_BSTR;
    V_BSTR(pVal) = bstr;
    return hr;
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_systemId( 
    /* [retval][out] */ VARIANT __RPC_FAR *pVal)
{
    BSTR bstr = NULL;
    if (!pVal)
        return E_INVALIDARG;
    HRESULT hr = _getAttrValue(XMLNames::name(NAME_SYSTEM), &bstr);
    pVal->vt = VT_BSTR;
    V_BSTR(pVal) = bstr;
    return hr;
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_notationName( 
    /* [retval][out] */ BSTR __RPC_FAR *pStr)
{
    return _getAttrValue(Name::create(_T("NDATA")), pStr);
}

//////////////////////////////////////////////
//// IXMLDOMEntityReference : IXMLDOMNode

//////////////////////////////////////////////
//// IXMLDOMDocumentType

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_name( 
    /* [retval][out] */ BSTR __RPC_FAR *pName)
{
    return _pDOMNode->get_nodeName(pName);
}

HRESULT STDMETHODCALLTYPE 
W3CDOMWrapper::get_entities( 
    /* [retval][out] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppMap)
{
    STACK_ENTRY;
    OMREADLOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::get_entities()"));

    HRESULT hr = S_OK;

    ARGTEST( ppMap);

    TRY
    {
        *ppMap = new DOMNamedNodeMapList(getNodeData(), Element::ENTITY);
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
W3CDOMWrapper::get_notations( 
    /* [retval][out] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppMap)
{
    STACK_ENTRY;
    OMREADLOCK(_pDOMNode);

    TraceTag((tagDOMOM, "W3CDOMWrapper::get_notations()"));

    HRESULT hr = S_OK;

    ARGTEST( ppMap);

    TRY
    {
        *ppMap = new DOMNamedNodeMapList(getNodeData(), Element::NOTATION);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}


Node * 
W3CDOMWrapper::getNodeData()
{
    return _pDOMNode->getNodeData();
}

// Pass-in the normalized length len, and the maximum length in pch
// Return the actual length matches the normalized length in pch
int 
W3CDOMWrapper::_getActualLength(TCHAR * pch, int len, int iMax)
{
    int i = 0, l = 0;
    iMax--;
    while (i < len)
    {
        if (*pch != 0xD || (l == iMax) || *(pch+1) != 0xA)
            i++;
        l++;
        pch++;
    }
    return l;
}


// get normalized text length
int 
W3CDOMWrapper::_getNormalizedLength(TCHAR *pch, int len, int iMax)
{
    int i = 0, l = 0;
    while (i++ < len)
    {
        if (*pch != 0xD || (i == iMax ) || *(pch+1) != 0xA)
            l++;
        pch++;
    }
    return l;
}

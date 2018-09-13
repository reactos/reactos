/*
 * @(#)XMLDOM.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef __XMLDOM_HXX
#define __XMLDOM_HXX

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif



#define CHECK_ARG_INIT(p) if (!p) return E_INVALIDARG; *p = null;
#define CHECK_ARG(p) if (!p) return E_INVALIDARG;

extern int DOMtoInternalType(DOMNodeType type);
extern DOMNodeType InternalToDOMType(int i);

extern TAG tagDOMOM;

class DOMDocumentWrapper : public _dispatchexport<Document, IXMLDOMDocument, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDOMDocument>, public DOMNode 
{
public: 
    
    DOMDocumentWrapper(Document * p);

    ~DOMDocumentWrapper() 
        {}

    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    {
        return DOMNode::AddRef();
    }

    virtual ULONG STDMETHODCALLTYPE Release( void)
    {
        return DOMNode::Release();
    }

    // IDispatch methods

    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    ///////////////////////////////////////////////////////////////////////////////
    // IDispatchEx methods
    //

    HRESULT STDMETHODCALLTYPE GetDispID( 
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */ DISPID __RPC_FAR *pid);
    

    ///////////////////////////////////////////////////////////////////////////////
    // IXMLDOMNode Interface
    //

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeType( 
        /* [out][retval] */ DOMNodeType __RPC_FAR *plType)
        { return DOMNode::get_nodeType(plType); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeName( 
        /* [out][retval] */ BSTR __RPC_FAR *pName)
        { return DOMNode::get_nodeName(pName); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *pVal)
        { return DOMNode::get_nodeValue(pVal); }
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeValue( 
        /* [in] */ VARIANT bstrVal)
        { return DOMNode::put_nodeValue(bstrVal); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parentNode( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
        { return DOMNode::get_parentNode(ppNode); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_childNodes( 
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
        { return DOMNode::get_childNodes(ppNodeList); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_firstChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppFirstChild)
        { return DOMNode::get_firstChild(ppFirstChild); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_lastChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppLastChild)
        { return DOMNode::get_lastChild(ppLastChild); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_previousSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppPrevSib)
        { return DOMNode::get_previousSibling(ppPrevSib); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nextSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNextSib)
        { return DOMNode::get_nextSibling(ppNextSib); }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [in] */ VARIANT refChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outNewChild)
        { return DOMNode::insertBefore(newChild, refChild, outNewChild); }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pNew,
        /* [in] */ IXMLDOMNode __RPC_FAR *pOld,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
        { return DOMNode::replaceChild(pNew, pOld, ppNode); }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppOldChild)
        { return DOMNode::removeChild(oldChild, ppOldChild); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
        { return DOMNode::appendChild(newChild, ppNewChild); }
    
    virtual HRESULT STDMETHODCALLTYPE get_attributes( 
        /* [out][retval] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppAttributes)
        { return DOMNode::get_attributes(ppAttributes); }

    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBool)
        { return DOMNode::hasChildNodes(pBool); }
    
    virtual  HRESULT STDMETHODCALLTYPE get_ownerDocument( 
        /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR *DOMDocument)
        { return DOMNode::get_ownerDocument(DOMDocument); }

    // Document has it's own special implementation of this...
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ VARIANT_BOOL deep,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot);    

    ////////////////////////////////////////////////
    // IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypeString( 
        /* [out][retval] */ BSTR __RPC_FAR *nodeType)
        { return DOMNode::get_nodeTypeString(nodeType); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
        /* [out][retval] */ BSTR __RPC_FAR *text)
        { return DOMNode::get_text(text); }
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_text( 
        /* [in] */ BSTR text)
        { return DOMNode::put_text(text); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_specified( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool)
        { return DOMNode::get_specified(pbool); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_definition( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
        { return DOMNode::get_definition(ppNode); }
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypedValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *typedValue)
        { return DOMNode::get_nodeTypedValue(typedValue); }
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeTypedValue( 
        /* [in] */ VARIANT typedValue)
        { return DOMNode::put_nodeTypedValue(typedValue); }
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dataType( 
        /* [out][retval] */ VARIANT __RPC_FAR *p)
        { return DOMNode::get_dataType(p); }
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_dataType( 
        /* [in] */ BSTR p)
        { return DOMNode::put_dataType(p); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_xml(
        /* [out][retval] */ BSTR __RPC_FAR *pbstrXml)
        { return DOMNode::get_xml(pbstrXml); }

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE transformNode( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pStyleSheet,
        /* [out][retval] */ BSTR __RPC_FAR *pbstr)
        { return DOMNode::transformNode(pStyleSheet, pbstr); }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectNodes( 
        /* [in] */ BSTR bstrXQL,
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppResult)
        { return DOMNode::selectNodes(bstrXQL, ppResult); }
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectSingleNode( 
        /* [in] */ BSTR queryString,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *resultNode)
        { return DOMNode::selectSingleNode(queryString, resultNode); }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parsed( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isParsed)
        { return DOMNode::get_parsed(isParsed); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_namespaceURI( 
        /* [out][retval] */ BSTR __RPC_FAR *pURI)
        { return DOMNode::get_namespaceURI(pURI); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_prefix( 
        /* [out][retval] */ BSTR __RPC_FAR *pPrefix)
        { return DOMNode::get_prefix(pPrefix); }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_baseName( 
        /* [out][retval] */ BSTR __RPC_FAR *pBaseName)
        { return DOMNode::get_baseName(pBaseName); }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE transformNodeToObject( 
        /* [in] */ IXMLDOMNode __RPC_FAR *stylesheet,
        /* [in] */ VARIANT outputObject)
    { return DOMNode::transformNodeToObject(stylesheet, outputObject); }



    //////////////////////////////////
    // IXMLDOMDocument interface

    virtual HRESULT STDMETHODCALLTYPE get_doctype( 
        /* [retval][out] */ IXMLDOMDocumentType __RPC_FAR *__RPC_FAR * ppDocumentType);
    
    virtual HRESULT STDMETHODCALLTYPE get_implementation( 
        /* [retval][out] */ IXMLDOMImplementation __RPC_FAR *__RPC_FAR *ppImpl);
    
    virtual HRESULT STDMETHODCALLTYPE get_documentElement( 
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR * ppDOMElement);

    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE putref_documentElement( 
        /* [in] */ IXMLDOMElement __RPC_FAR * pDOMElement);
    
    virtual HRESULT STDMETHODCALLTYPE createElement( 
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR *ppNewNode);
    
    virtual HRESULT STDMETHODCALLTYPE createDocumentFragment( 
        /* [retval][out] */ IXMLDOMDocumentFragment __RPC_FAR *__RPC_FAR *ppNewNode);
    
    virtual HRESULT STDMETHODCALLTYPE createTextNode( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *ppNewNode);
    
    virtual HRESULT STDMETHODCALLTYPE createAttribute( 
        /* [in] */ BSTR attrName,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *ppNewNode);

    virtual HRESULT STDMETHODCALLTYPE createComment( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMComment __RPC_FAR *__RPC_FAR *ppNewNode);
    
    virtual HRESULT STDMETHODCALLTYPE createCDATASection( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMCDATASection __RPC_FAR *__RPC_FAR *ppNewNode);
    
    virtual HRESULT STDMETHODCALLTYPE createProcessingInstruction( 
        /* [in] */ BSTR target,
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMProcessingInstruction __RPC_FAR *__RPC_FAR *ppNewNode);
    
    virtual HRESULT STDMETHODCALLTYPE createEntityReference( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMEntityReference __RPC_FAR *__RPC_FAR *ppNewNode);
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR tagname,
        /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
        { return DOMNode::getElementsByTagName(tagname, ppNodeList); }
    
    /////////////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMDocument interface.

    virtual HRESULT STDMETHODCALLTYPE createNode( 
        /* [in] */ VARIANT Type,
        /* [in] */ BSTR Text,
        /* [in] */ BSTR NameSpaceURI,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);

    virtual HRESULT STDMETHODCALLTYPE nodeFromID( 
        /* [in] */ BSTR pbstrID,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual HRESULT STDMETHODCALLTYPE load( 
        /* [in] */ VARIANT vTarget,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful);
    
    virtual HRESULT STDMETHODCALLTYPE get_readyState( 
        /* [out][retval] */ long __RPC_FAR *plState);
    
    virtual HRESULT STDMETHODCALLTYPE get_parseError( 
        /* [out][retval] */ IXMLDOMParseError __RPC_FAR *__RPC_FAR *pError);
    
    virtual HRESULT STDMETHODCALLTYPE get_url( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrUrl);
    
    virtual HRESULT STDMETHODCALLTYPE get_async( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pf);
    
    virtual HRESULT STDMETHODCALLTYPE put_async( 
        /* [in] */ VARIANT_BOOL f);
    
    virtual HRESULT STDMETHODCALLTYPE abort( void);
    
    virtual HRESULT STDMETHODCALLTYPE loadXML( 
        /* [in] */ BSTR bstrXML,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful);

    virtual HRESULT STDMETHODCALLTYPE save( 
        /* [in] */ VARIANT vTarget); 

    virtual HRESULT STDMETHODCALLTYPE get_validateOnParse( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isValidating);
    
    virtual HRESULT STDMETHODCALLTYPE put_validateOnParse( 
        /* [in] */ VARIANT_BOOL isValidating);        

    virtual HRESULT STDMETHODCALLTYPE get_resolveExternals( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfResolve);
    
    virtual HRESULT STDMETHODCALLTYPE put_resolveExternals( 
        /* [in] */ VARIANT_BOOL fResolve);        

    virtual HRESULT STDMETHODCALLTYPE get_preserveWhiteSpace( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfPreserve);
    
    virtual HRESULT STDMETHODCALLTYPE put_preserveWhiteSpace( 
        /* [in] */ VARIANT_BOOL fPreserve);        

    virtual HRESULT STDMETHODCALLTYPE put_onreadystatechange( 
        /* [in] */ VARIANT varF);
    
    virtual HRESULT STDMETHODCALLTYPE put_ondataavailable( 
        /* [in] */ VARIANT varF);

    virtual HRESULT STDMETHODCALLTYPE put_ontransformnode( 
        /* [in] */ VARIANT varF);

    // helper functions for Invoke()
    static INVOKEFUNC _invoke;
};

#endif __XMLDOM_HXX
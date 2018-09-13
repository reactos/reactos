/*
 * @(#)w3cdom.hxx 1.0 2/25/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_W3CDOM_HXX
#define _XML_OM_W3CDOM_HXX

#ifndef _XML_OM_NODE_HXX
#include "xml/om/node.hxx"
#endif

class DOMNode;

class W3CDOMWrapper : public IXMLDOMElement,
                      public IXMLDOMAttribute,
                      public IXMLDOMProcessingInstruction,
                      public IXMLDOMComment,
//                      public IXMLDOMText, // we get this via IXMLDOMCDATASection
                      public IXMLDOMCDATASection,
                      public IXMLDOMDocumentFragment,
                      public IXMLDOMEntity,
                      public IXMLDOMNotation,
                      public IXMLDOMEntityReference,
                      public IXMLDOMDocumentType
{
public:
    W3CDOMWrapper( DOMNode *);

protected:
    ~W3CDOMWrapper();

#if DBG==1
    W3CDOMWrapper() { Assert(0 && "This should never get called"); }
    W3CDOMWrapper(const W3CDOMWrapper&) { Assert(0 && "This should never get called"); }
#endif

public:
    // IUnknown methods

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    // IDispatch methods

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ UINT __RPC_FAR *pctinfo);

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    
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

    //////////////////////////////////////////////
    //// IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeType( 
        /* [out][retval] */ DOMNodeType __RPC_FAR *plType);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeName( 
        /* [out][retval] */ BSTR __RPC_FAR *pName);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *pVal);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeValue( 
        /* [in] */ VARIANT vVal);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parentNode( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_childNodes( 
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_firstChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppFirstChild);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_lastChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppLastChild);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_previousSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppPrevSib);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nextSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNextSib);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [in] */ VARIANT refChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pNew,
        /* [in] */ IXMLDOMNode __RPC_FAR *pOld,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppOldChild);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_attributes( 
        /* [out][retval] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppAttributes);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBool);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ VARIANT_BOOL deep,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot);

    virtual  HRESULT STDMETHODCALLTYPE get_ownerDocument( 
        /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR * ppDOMDocument);

    ////////////////////////////////////////////////
    // IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypeString( 
        /* [out][retval] */ BSTR __RPC_FAR *nodeType);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
        /* [out][retval] */ BSTR __RPC_FAR *text);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_text( 
        /* [in] */ BSTR text);
    
//    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_specified( 
//        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_definition( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypedValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *TypedValue);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeTypedValue( 
        /* [in] */ VARIANT TypedValue);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dataType( 
        /* [out][retval] */ VARIANT __RPC_FAR *p);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_dataType( 
        /* [in] */ BSTR p);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_xml(
        /* [out][retval] */ BSTR __RPC_FAR *pbstrXml);

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE transformNode( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pStyleSheet,
        /* [out][retval] */ BSTR __RPC_FAR *pbstr);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectNodes( 
        /* [in] */ BSTR bstrXQL,
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppResult);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectSingleNode( 
        /* [in] */ BSTR queryString,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *resultNode);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parsed( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isParsed);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_namespaceURI( 
        /* [out][retval] */ BSTR __RPC_FAR *pURI);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_prefix( 
        /* [out][retval] */ BSTR __RPC_FAR *pPrefix);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_baseName( 
        /* [out][retval] */ BSTR __RPC_FAR *pBaseName);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE transformNodeToObject( 
        /* [in] */ IXMLDOMNode __RPC_FAR *stylesheet,
        /* [in] */ VARIANT outputObject);


    //////////////////////////////////////////////
    //// IXMLDOMElement : IXMLDOMNode

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_tagName( 
        /* [retval][out] */ BSTR __RPC_FAR *pTagName);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ VARIANT __RPC_FAR *pValue);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ VARIANT value);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *ppAttr);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IXMLDOMAttribute __RPC_FAR * pDOMAttribute,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR * ppAttributeNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IXMLDOMAttribute __RPC_FAR * pDOMAttribute,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR * ppAttributeNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR tagname,
        /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE normalize( void);

    //////////////////////////////////////////////
    //// IXMLDOMAttribute : IXMLDOMNode

//  defined on IXMLDOMDocumentType
//    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_name( 
//        /* [retval][out] */ BSTR __RPC_FAR *attributeName);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_specified( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pFlag);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_value( 
        /* [retval][out] */ VARIANT __RPC_FAR *attributeValue);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_value( 
        /* [in] */ VARIANT attributeValue);

    //////////////////////////////////////////////
    //// IXMLDOMProcessingInstruction : IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_target( 
        /* [retval][out] */ BSTR __RPC_FAR *pStr);

// defined on IXMLDOMData    
//    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_data( 
//        /* [retval][out] */ BSTR __RPC_FAR *pStr);
//    
//    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_data( 
//        /* [in] */ BSTR data);


    //////////////////////////////////////////////
    //// IXMLDOMCharacterData : IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_data( 
        /* [retval][out] */ BSTR __RPC_FAR *data);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_data( 
        /* [in] */ BSTR data);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [retval][out] */ long __RPC_FAR *dataLength);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE substringData( 
        /* [in] */ long offset,
        /* [in] */ long count,
        /* [retval][out] */ BSTR __RPC_FAR *data);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE appendData( 
        /* [in] */ BSTR data);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE insertData( 
        /* [in] */ long offset,
        /* [in] */ BSTR data);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE deleteData( 
        /* [in] */ long offset,
        /* [in] */ long count);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE replaceData( 
        /* [in] */ long offset,
        /* [in] */ long count,
        /* [in] */ BSTR data);

    //////////////////////////////////////////////
    //// IXMLDOMComment : IXMLDOMCharacterData

    //////////////////////////////////////////////
    //// IXMLDOMText : IXMLDOMCharacterData

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE splitText( 
        /* [in] */ long offset,
        /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *ppNewTextNode);

    //////////////////////////////////////////////
    //// IXMLDOMCDATASection : IXMLDOMText

    //////////////////////////////////////////////
    //// IXMLDOMDocumentFragment : IXMLDOMNode

    //////////////////////////////////////////////
    //// IXMLDOMEntity : IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_publicId( 
        /* [retval][out] */ VARIANT __RPC_FAR *pStr);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_systemId( 
        /* [retval][out] */ VARIANT __RPC_FAR *pStr);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_notationName( 
        /* [retval][out] */ BSTR __RPC_FAR *pStr);

    //////////////////////////////////////////////
    //// IXMLDOMEntityReference : IXMLDOMNode

    //////////////////////////////////////////////
    //// IXMLDOMDocumentType

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_name( 
        /* [retval][out] */ BSTR __RPC_FAR *pName);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_entities( 
        /* [retval][out] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppMap);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_notations( 
        /* [retval][out] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppMap);

private:

    HRESULT _getAttrValue(Name * pName, BSTR * pbstrVal);
    int _getActualLength(TCHAR * pch, int len, int iMax);
    int _getNormalizedLength(TCHAR *pch, int len, int iMax);

private:
    Node * getNodeData();

    LONG                                _lRefCount;
    DOMNode *                           _pDOMNode; // DOMNode AddRef's W3CDOMWrapper
};

#endif // _XML_OM_W3CDOM_HXX

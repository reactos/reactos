/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _BEHAVIOUR_HXX
#define _BEHAVIOUR_HXX

#include <mshtml.h>
#include <mshtmhst.h>
#include <msxml.h>

#define NODE_UNKNOWN NODE_INVALID 

///////////////////////////////////////////////////////////////////////////////
//
//                        DOMNodeWrapper class                               
//
///////////////////////////////////////////////////////////////////////////////
class DOMNodeWrapper: public IXMLDOMElement,
                      public IXMLDOMProcessingInstruction,
                      public IXMLDOMComment,
                      public IXMLDOMCDATASection,
                      public IXMLDOMEntityReference,
                      public IXMLDOMAttribute,
                      public ISupportErrorInfo,
                      public IElementBehavior
{
protected:
    DOMNodeType _nodeType;
    IUnknown *_pWrapped;
    IElementBehaviorSite *_pPeerSite;      // IElementBehaviorSite

private:
    long _refcount;

protected:    
    static void setErrorInfo(WCHAR * szDescription);

    typedef IUnknown * (WrapperCastFunc)(DOMNodeWrapper *);    
    
    struct DispInfo
    {
        DISPATCHINFO dispatchinfo;
        WrapperCastFunc *func;
    };

    static DispInfo aDispInfo[NODE_NOTATION+1];

    static DISPATCHINFO s_dispatchinfoDOM;

private:
    HRESULT sniffType(void);
    HRESULT InitPeer(void);

public:
    static HRESULT wrapNode(IHTMLDOMNode *pDOMNode, IXMLDOMNode **ppNode);
    static HRESULT getWrapper(IHTMLDOMNode *pDOMNode, IXMLDOMNode **ppNode);
public:
    DOMNodeWrapper();
    DOMNodeWrapper(IUnknown *punkWrapped, DOMNodeType nodeType = NODE_UNKNOWN);
    virtual ~DOMNodeWrapper();

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
        /* [in] */ VARIANT bstrVal);
    
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

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypeString( 
        /* [out][retval] */ BSTR __RPC_FAR *nodeType);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
        /* [out][retval] */ BSTR __RPC_FAR *text);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_text( 
        /* [in] */ BSTR text);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_specified( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);

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

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_name( 
        /* [retval][out] */ BSTR __RPC_FAR *attributeName);
    
//    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_specified( 
//        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pFlag);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_value( 
        /* [retval][out] */ VARIANT __RPC_FAR *attributeValue);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_value( 
        /* [in] */ VARIANT attributeValue);

    //////////////////////////////////////////////
    //// IXMLDOMProcessingInstruction : IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_target( 
        /* [retval][out] */ BSTR __RPC_FAR *pStr);

// defined on IXMLDOMCharacterData    
//  virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_data( 
//      /* [retval][out] */ BSTR __RPC_FAR *pStr);
//    
//  virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_data( 
//      /* [in] */ BSTR data);


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
    //// IXMLDOMEntityReference : IXMLDOMNode

    //////////////////////////////////////////////
    //// ISupportErrorInfo

    virtual HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo(REFIID riid);

    //////////////////////////////////////////////
    //// IElementBehavior
    
    virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ IElementBehaviorSite __RPC_FAR *pBehaviorSite);
    
    virtual HRESULT STDMETHODCALLTYPE Notify( 
        /* [in] */ LONG event,
        /* [out][in] */ VARIANT __RPC_FAR *pVar);

        virtual HRESULT STDMETHODCALLTYPE Detach() { return S_OK; };
};


///////////////////////////////////////////////////////////////////////////////
//
//                        DOMGhostNodeWrapper class
//
///////////////////////////////////////////////////////////////////////////////

// For attribute nodes, which are treated separate in the HTML DOM and do 
// not inherit from the base dom.  No parent, so we must support it manually.
class DOMGhostNodeWrapper: public DOMNodeWrapper
{
public:
    DOMGhostNodeWrapper(IDispatch *pDisp, IDispatch *pParentDisp);
    virtual ~DOMGhostNodeWrapper();
public:
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeType( 
        /* [out][retval] */ DOMNodeType __RPC_FAR *plType);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parentNode( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
private:
    IDispatch *_pParentWrapped;
};



///////////////////////////////////////////////////////////////////////////////
//
//                        DOMNodeListWrapper class
//
///////////////////////////////////////////////////////////////////////////////

// Node List wrapper
class DOMNodeListWrapper : public _dispatch<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList>
{
public:
    virtual ~DOMNodeListWrapper();
public:
    static HRESULT wrapList(IDispatch *pDisp, IXMLDOMNodeList **ppList);
public:
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_item( 
        /* [in] */ long index,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *listItem);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [retval][out] */ long __RPC_FAR *listLength);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE nextNode(
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE reset( void);

    virtual /* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
        /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);

public:
    void SetWrapped(IDispatch *pIDisp);
private:
    IDispatch *_pWrapped;
};


///////////////////////////////////////////////////////////////////////////////
//
//                        DOMNamedMapWrapper class
//
///////////////////////////////////////////////////////////////////////////////

// Named node map wrapper
// Node List wrapper
class DOMNamedMapWrapper : public _dispatch<IXMLDOMNamedNodeMap, &LIBID_MSXML, &IID_IXMLDOMNamedNodeMap>
{
public:
    virtual ~DOMNamedMapWrapper();
public:
    static HRESULT wrapNodeMap(IDispatch *pDisp, IXMLDOMNamedNodeMap **ppNodeMap);
public:
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getNamedItem( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *namedItem);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setNamedItem( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newItem,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *namedItem);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeNamedItem( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *namedItem);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_item( 
        /* [in] */ long index,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *listItem);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [retval][out] */ long __RPC_FAR *listLength);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getQualifiedItem( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR nameSpaceURN,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeQualifiedItem( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR NameSpace,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE nextNode(
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE reset( void);

    virtual /* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
        /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);

public:
    void SetWrapped(IDispatch *pIDisp);
private:
    IDispatch *_pWrapped;
};




///////////////////////////////////////////////////////////////////////////////
//
//                        DOMDocViewerWrapper class
//
///////////////////////////////////////////////////////////////////////////////
class NOVTABLE DOMDocViewerWrapper : public _dispatch<IXMLDOMDocument, &LIBID_MSXML, &IID_IXMLDOMDocument>
{
public:
    virtual ~DOMDocViewerWrapper();
public:
    // IXMLDOMNode interfaces
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeName( 
        /* [retval][out] */ BSTR __RPC_FAR *name);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeValue( 
        /* [retval][out] */ VARIANT __RPC_FAR *value);
        
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeValue( 
            /* [in] */ VARIANT value);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeType( 
        /* [retval][out] */ DOMNodeType __RPC_FAR *type);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parentNode( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *parent);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_childNodes( 
        /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *childList);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_firstChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *firstChild);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_lastChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *lastChild);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_previousSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *previousSibling);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nextSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *nextSibling);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_attributes( 
        /* [retval][out] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *atrributeMap);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [in] */ VARIANT refChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outNewChild);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outOldChild);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *childNode,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *oldChild);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outNewChild);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *hasChild);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ownerDocument( 
        /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR *DOMDocument);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ VARIANT_BOOL deep,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *cloneRoot);

    // IXMLDOMDocument interfaces
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_doctype( 
        /* [retval][out] */ IXMLDOMDocumentType __RPC_FAR *__RPC_FAR *documentType);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_implementation( 
        /* [retval][out] */ IXMLDOMImplementation __RPC_FAR *__RPC_FAR *impl);
        
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_documentElement( 
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR *DOMElement);
        
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE putref_documentElement( 
        /* [in] */ IXMLDOMElement __RPC_FAR *DOMElement);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createElement( 
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR *element);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createDocumentFragment( 
        /* [retval][out] */ IXMLDOMDocumentFragment __RPC_FAR *__RPC_FAR *docFrag);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createTextNode( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *text);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createComment( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMComment __RPC_FAR *__RPC_FAR *comment);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createCDATASection( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMCDATASection __RPC_FAR *__RPC_FAR *cdata);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createProcessingInstruction( 
        /* [in] */ BSTR target,
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMProcessingInstruction __RPC_FAR *__RPC_FAR *pi);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *attribute);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createEntityReference( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMEntityReference __RPC_FAR *__RPC_FAR *entityRef);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *resultList);

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

    virtual HRESULT STDMETHODCALLTYPE saveXML( 
        /* [in] */ VARIANT vTargert); 

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

public:
    void SetWrapped(IDispatch *pIDisp);
private:
    IDispatch *_pWrapped;
};

#endif
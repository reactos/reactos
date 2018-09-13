/*
 * @(#)EXPANDO.hxx 1.0 3/24/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * Common code for MSXML Data Islands
 * 
 */

#ifndef __EXPANDO_HXX__
#define __EXPANDO_HXX__

#include <oaidl.h>
#include <mshtml.h>
#include <msxml.h>



///////////////////////////////////
// Constants

// Define the names of the properties
#define XMLTREE        L"XMLDocument"
#define XSLTREE        L"XSLDocument"

///////////////////////////////////
// Function prototypes

HRESULT 
AddDOCExpandoProperty(
    BSTR bstrAttribName,
    IHTMLDocument2 *pDoc,
    IDispatch *pDispToAdd
    );

// Fake document for expando security
// only implements getURL
class ExpandoDocument : public IXMLDOMDocument, public ISupportErrorInfo
{
public:
    ExpandoDocument(BSTR bURL);
    virtual ~ExpandoDocument();

private:
    long _refcount;

protected:    
    static void setErrorInfo(WCHAR * szDescription);

    static DISPATCHINFO s_dispatchinfoEXP;

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

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeType( 
        /* [out][retval] */ DOMNodeType __RPC_FAR *plType)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeName( 
        /* [out][retval] */ BSTR __RPC_FAR *pName)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *pVal)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeValue( 
        /* [in] */ VARIANT bstrVal)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parentNode( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_childNodes( 
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_firstChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppFirstChild)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_lastChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppLastChild)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_previousSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppPrevSib)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nextSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNextSib)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [in] */ VARIANT refChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outNewChild)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pNew,
        /* [in] */ IXMLDOMNode __RPC_FAR *pOld,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppOldChild)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE get_attributes( 
        /* [out][retval] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppAttributes)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBool)
    { return E_NOTIMPL; }
    
    virtual  HRESULT STDMETHODCALLTYPE get_ownerDocument( 
        /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR *DOMDocument)
    { return E_NOTIMPL; }

    // Document has it's own special implementation of this...
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ VARIANT_BOOL deep,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot)
    { return E_NOTIMPL; }

    ////////////////////////////////////////////////
    // IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypeString( 
        /* [out][retval] */ BSTR __RPC_FAR *nodeType)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
        /* [out][retval] */ BSTR __RPC_FAR *text)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_text( 
        /* [in] */ BSTR text)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_specified( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_definition( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    { return E_NOTIMPL; }
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypedValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *typedValue)
    { return E_NOTIMPL; }
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeTypedValue( 
        /* [in] */ VARIANT typedValue)
    { return E_NOTIMPL; }
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dataType( 
        /* [out][retval] */ VARIANT __RPC_FAR *p)
    { return E_NOTIMPL; }
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_dataType( 
        /* [in] */ BSTR p)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_xml(
        /* [out][retval] */ BSTR __RPC_FAR *pbstrXml)
    { return E_NOTIMPL; }

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE transformNode( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pStyleSheet,
        /* [out][retval] */ BSTR __RPC_FAR *pbstr)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectNodes( 
        /* [in] */ BSTR bstrXQL,
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppResult)
    { return E_NOTIMPL; }
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectSingleNode( 
        /* [in] */ BSTR queryString,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *resultNode)
    { return E_NOTIMPL; }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parsed( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isParsed)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_namespaceURI( 
        /* [out][retval] */ BSTR __RPC_FAR *pURI)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_prefix( 
        /* [out][retval] */ BSTR __RPC_FAR *pPrefix)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_baseName( 
        /* [out][retval] */ BSTR __RPC_FAR *pBaseName)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE transformNodeToObject(
         /* [in] */ IXMLDOMNode __RPC_FAR *stylesheet,
         /* [in] */ VARIANT outputObject)
    { return E_NOTIMPL; }

    //////////////////////////////////
    // IXMLDOMDocument interface

    virtual HRESULT STDMETHODCALLTYPE get_doctype( 
        /* [retval][out] */ IXMLDOMDocumentType __RPC_FAR *__RPC_FAR * ppDocumentType)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE get_implementation( 
        /* [retval][out] */ IXMLDOMImplementation __RPC_FAR *__RPC_FAR *ppImpl)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE get_documentElement( 
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR * ppDOMElement)
    { return E_NOTIMPL; }

    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE putref_documentElement( 
        /* [in] */ IXMLDOMElement __RPC_FAR * pDOMElement)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE createElement( 
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE createDocumentFragment( 
        /* [retval][out] */ IXMLDOMDocumentFragment __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE createTextNode( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE createAttribute( 
        /* [in] */ BSTR attrName,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE createComment( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMComment __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE createCDATASection( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMCDATASection __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE createProcessingInstruction( 
        /* [in] */ BSTR target,
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMProcessingInstruction __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE createEntityReference( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMEntityReference __RPC_FAR *__RPC_FAR *ppNewNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR tagname,
        /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
    { return E_NOTIMPL; }
    
    /////////////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMDocument interface.

    virtual HRESULT STDMETHODCALLTYPE createNode( 
        /* [in] */ VARIANT Type,
        /* [in] */ BSTR Text,
        /* [in] */ BSTR NameSpaceURI,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE nodeFromID( 
        /* [in] */ BSTR pbstrID,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE load( 
        /* [in] */ VARIANT vTarget,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE get_readyState( 
        /* [out][retval] */ long __RPC_FAR *plState)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE get_parseError( 
        /* [out][retval] */ IXMLDOMParseError __RPC_FAR *__RPC_FAR *pError)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE get_url( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrUrl);
    
    virtual HRESULT STDMETHODCALLTYPE get_async( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pf)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE put_async( 
        /* [in] */ VARIANT_BOOL f)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE abort( void)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE loadXML( 
        /* [in] */ BSTR bstrSrc,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE save( 
        /* [in] */ VARIANT vTargert) 
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE get_validateOnParse( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isValidating)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE put_validateOnParse( 
        /* [in] */ VARIANT_BOOL isValidating)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE get_resolveExternals( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfResolve)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE put_resolveExternals( 
        /* [in] */ VARIANT_BOOL fResolve)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE get_preserveWhiteSpace( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfPreserve)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE put_preserveWhiteSpace( 
        /* [in] */ VARIANT_BOOL fPreserve)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE put_onreadystatechange( 
        /* [in] */ VARIANT varF)
    { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE put_ondataavailable( 
        /* [in] */ VARIANT varF)
    { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE put_ontransformnode( 
        /* [in] */ VARIANT varF)
    { return E_NOTIMPL; }

    //////////////////////////////////////////////
    //// ISupportErrorInfo
    virtual HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo(REFIID riid);

private:
    BSTR _bURL;
};
#endif // __EXPANDO_HXX__
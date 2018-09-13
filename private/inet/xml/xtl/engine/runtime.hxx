/*
 * @(#)runtime.hxx 1.0 07/09/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * XTL runtime object
 * 
 */

#ifndef _XTL_ENGINE_RUNTIME
#define _XTL_ENGINE_RUNTIME

#define ISEMPTYARG(x) ((VT_ERROR == V_VT(&x)) && (DISP_E_PARAMNOTFOUND == V_ERROR(&x)))

class CXTLRuntimeObject;
typedef _reference<CXTLRuntimeObject> RCXTLRuntimeObject;

class CXTLRuntimeObject : 
    public _dispatchEx<IXTLRuntime, &LIBID_MSXML, &IID_IXTLRuntime, false>
{
typedef _dispatch<IXTLRuntime, &LIBID_MSXML, &IID_IXTLRuntime> super;

public:
    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

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

    _reference<IXMLDOMNode> _rXMLDOMNode;

// Private helpers
private:
    Element *ancestor(String * s, Element * node);
    Element *ancestor(Name * tag, Element * node);
    
    HRESULT VariantDateToDateTime(
        VARIANT varDate, 
        LPWSTR szFormat, 
        LCID lcid,
        BSTR *pbstrFormattedString, 
        BOOL fDate);

    BOOL IntToRoman(
        UINT iNumber, 
        LPTSTR szRoman, 
        UINT cchRoman, 
        BOOL fUpperCase);

    LCID LcidFromVariant(
        VARIANT varLocale, 
        BOOL *pfSuccess = NULL);

    UINT GetNumDigits(long i);
    
    static TCHAR s_achRomanUpper[]; 
    static TCHAR s_achRomanLower[]; 

#define ROMAN_1000 0
#define ROMAN_100  2


public:
    CXTLRuntimeObject();
    virtual ~CXTLRuntimeObject();

    // IXTLRuntime methods

    HRESULT STDMETHODCALLTYPE uniqueID( 
        IXMLDOMNode *pNode,
        long *pID);

    HRESULT STDMETHODCALLTYPE depth( 
        IXMLDOMNode *pNode,
        long *pDepth);
    
    HRESULT STDMETHODCALLTYPE childNumber( 
        IXMLDOMNode *pNode,
        long *pNumber);
    
    HRESULT STDMETHODCALLTYPE ancestorChildNumber( 
        BSTR bstrNodeName,
        IXMLDOMNode *pNode,
        long *pNumber);
    
    HRESULT STDMETHODCALLTYPE absoluteChildNumber( 
        IXMLDOMNode *pNode,
        long *pNumber);

    HRESULT STDMETHODCALLTYPE formatIndex( 
        long lIndex,
        BSTR bstrFormat,
        BSTR *pbstrFormattedString);
    
    HRESULT STDMETHODCALLTYPE formatNumber( 
        double dblNumber,
        BSTR bstrFormat,
        BSTR *pbstrFormattedString);
    
    HRESULT STDMETHODCALLTYPE formatDate( 
        VARIANT varDate,
        BSTR bstrFormat,
        VARIANT varDestLocale,
        BSTR *pbstrFormattedString);
    
    HRESULT STDMETHODCALLTYPE formatTime( 
        VARIANT varTime,
        BSTR bstrFormat,
        VARIANT varDestLocale,
        BSTR *pbstrFormattedString);
    
    
    // IXMLDOMNode methods

    // Dummy implementation.  Note: this object is only used from script and the IDispatch::Invoke directs the
    // IXMLDOMNode calls directly to the DOMNode.

    HRESULT STDMETHODCALLTYPE get_nodeType(DOMNodeType *plType) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_nodeName(BSTR *pbstrName) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_nodeValue(VARIANT *pVal) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE put_nodeValue(VARIANT vVal) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_parentNode(IXMLDOMNode **ppNode) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_childNodes(IXMLDOMNodeList **ppNodeList) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_firstChild(IXMLDOMNode **ppFirstChild) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_lastChild(IXMLDOMNode **ppLastChild) {return E_NOTIMPL;}    
    HRESULT STDMETHODCALLTYPE get_previousSibling(IXMLDOMNode  **ppPrevSib) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_nextSibling(IXMLDOMNode  **ppNextSib) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE insertBefore( 
        IXMLDOMNode * newChild,
        VARIANT refChild,
        IXMLDOMNode **outNewChild) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE replaceChild( 
        IXMLDOMNode *pNew,
        IXMLDOMNode *pOld,
        IXMLDOMNode **ppNode) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE removeChild( 
        IXMLDOMNode *pChild,
        IXMLDOMNode **ppNode) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE appendChild( 
        IXMLDOMNode *newChild,
        IXMLDOMNode **ppNewChild) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_attributes(IXMLDOMNamedNodeMap **ppNodeList) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE hasChildNodes(VARIANT_BOOL *pBool) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE cloneNode(VARIANT_BOOL deep,IXMLDOMNode  **ppCloneRoot) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_ownerDocument(IXMLDOMDocument **ppDOMDocument) {return E_NOTIMPL;}

    // IXMLDOMNode methods
    HRESULT STDMETHODCALLTYPE get_nodeTypeString(BSTR *nodeType) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_specified( VARIANT_BOOL  *pbool) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_text(BSTR  *pstr) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE put_text(BSTR text) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_definition(IXMLDOMNode **ppNode) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_nodeTypedValue(VARIANT *TypedValue) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE put_nodeTypedValue(VARIANT TypedValue) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_dataType(VARIANT *p) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE put_dataType(BSTR p) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE transformNode( 
        IXMLDOMNode *stylesheet,
        BSTR *p) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE selectNodes(
        BSTR bstrXQL,
        IXMLDOMNodeList **ppResult) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_xml(BSTR  *pbstrXml) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE selectSingleNode( 
        BSTR queryString,
        IXMLDOMNode **resultNode) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_parsed(VARIANT_BOOL *isParsed) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_namespaceURI(BSTR *pURI) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_prefix(BSTR *pPrefix) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_baseName(BSTR *pBaseName) {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE transformNodeToObject(
        IXMLDOMNode *stylesheet,
        VARIANT outputObject) { return E_NOTIMPL; }

private:
    inline IXMLDOMNode* getDOMNode() { 
        Assert(_rXMLDOMNode && "Need to have a node to operate on");
        return (IXMLDOMNode*)_rXMLDOMNode;
    }
};

#endif // _XTL_ENGINE_RUNTIME

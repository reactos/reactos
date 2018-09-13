/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include <tchar.h>
#include <xmldomdid.h>
#include "utils.hxx"
#include "behaviour.hxx"

#define NOWRAPPER 1

// did we inject?

// macros to get the appropriate trident interfaces
#define QIHTMLDOM(pp) \
    _pWrapped->QueryInterface(IID_IHTMLDOMNode, (void **)(pp))
#define QIATTRDOM(pp) \
    _pWrapped->QueryInterface(IID_IHTMLDOMAttribute, (void **)(pp))
#define QITEXTDOM(pp) \
    _pWrapped->QueryInterface(IID_IHTMLDOMTextNode, (void **)(pp))
#define QICOMMENTELEM(pp) \
    _pWrapped->QueryInterface(IID_IHTMLCommentElement, (void **)(pp))
#define QIHTMLELEM(pp) \
    _pWrapped->QueryInterface(IID_IHTMLElement, (void **)(pp))

///////////////////////////////////////////////////////////////////////////////
//
//                        DOMNodeWrapper class                               
//
///////////////////////////////////////////////////////////////////////////////

#define WrapperCastTo(n,t) IUnknown * n(DOMNodeWrapper *p) { return (t *)p; }
WrapperCastTo( CastElement, IXMLDOMElement)
WrapperCastTo( CastAttribute, IXMLDOMAttribute)
WrapperCastTo( CastPI, IXMLDOMProcessingInstruction)
WrapperCastTo( CastComment, IXMLDOMComment)
WrapperCastTo( CastText, IXMLDOMText)
WrapperCastTo( CastCDATA, IXMLDOMCDATASection)
WrapperCastTo( CastEntityRef, IXMLDOMEntityReference)

extern INVOKE_METHOD s_rgDOMNodeMethods[];
extern DISPIDTOINDEX s_DOMNode_DispIdMap[];
extern INVOKE_METHOD s_rgIXMLDOMElementMethods[];
extern DISPIDTOINDEX s_IXMLDOMElement_DispIdMap[];

extern BYTE s_cDOMNodeMethodLen;
extern BYTE s_cDOMNode_DispIdMap;
extern BYTE s_cIXMLDOMElementMethodLen;
extern BYTE s_cIXMLDOMElement_DispIdMap;

DISPATCHINFO DOMNodeWrapper::s_dispatchinfoDOM =
{
    NULL, &IID_IXMLDOMNode, &LIBID_MSXML, ORD_MSXML, s_rgDOMNodeMethods, s_cDOMNodeMethodLen, s_DOMNode_DispIdMap, s_cDOMNode_DispIdMap, DOMNode::_invokeDOMNode
};


DOMNodeWrapper::DispInfo DOMNodeWrapper::aDispInfo[] = 
{
    // UNKNOWN/INVALID = 0,
    { NULL, NULL, NULL },
    // ELEMENT = 1,
    { NULL, &IID_IXMLDOMElement, &LIBID_MSXML, ORD_MSXML, s_rgIXMLDOMElementMethods, s_cIXMLDOMElementMethodLen, s_IXMLDOMElement_DispIdMap, s_cIXMLDOMElement_DispIdMap, DOMNode::_invokeDOMElement, CastElement},
    // ATTRIBUTE = 2,
    { NULL, &IID_IXMLDOMAttribute, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, CastAttribute},
    // PCDATA = 3,
    { NULL, &IID_IXMLDOMText, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, CastText},
    // CDATA = 4,     
    { NULL, &IID_IXMLDOMCDATASection, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, CastCDATA},
    // ENTITYREF = 5,
    { NULL, &IID_IXMLDOMEntityReference, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, CastEntityRef},
    // ENTITY = 6,     
    { NULL, NULL, NULL },
    // PI = 7,
    { NULL, &IID_IXMLDOMProcessingInstruction, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, CastPI},
    // COMMENT = 8,
    { NULL, &IID_IXMLDOMComment, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, CastComment},
    // DOCUMENT = 9,
    { NULL, NULL, NULL },
    // DOCTYPE = 10,
    { NULL, NULL, NULL },
    // DOCFRAG = 11,
    { NULL, NULL, NULL },
    // NOTATION = 12,     
    { NULL, NULL, NULL },
};


DOMNodeWrapper::DOMNodeWrapper()
{   
    _nodeType = NODE_UNKNOWN;
    _pWrapped = NULL;
    _pPeerSite = NULL;
    _refcount = 1;
    ::IncrementComponents();
}

DOMNodeWrapper::DOMNodeWrapper(IUnknown *punkWrapped, DOMNodeType nodeType)
{
    _nodeType = nodeType;
    _pWrapped = punkWrapped;
    _pPeerSite = NULL;

    if (_pWrapped)
    {
        _pWrapped->AddRef();
        sniffType();  // die gracefully if no go
    } 
       
    _refcount = 1;
    ::IncrementComponents();
}

DOMNodeWrapper::~DOMNodeWrapper()
{
    SafeRelease(_pWrapped);
    _pPeerSite = NULL;
    ::DecrementComponents();
}

//////////////////////////////////////////////
////  IUnknown methods

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (riid == IID_IUnknown)
    {
        return _pWrapped->QueryInterface(riid, ppv);
    }    
    else if (riid == IID_IDispatch || riid == IID_IXMLDOMNode)
    {
        *ppv = this; // assumes vtable layout is definition order
    }
    else if (riid == IID_IXMLDOMElement && _nodeType == NODE_ELEMENT)
    {
        *ppv = static_cast<IXMLDOMElement *>(this);
    }
    else if (riid == IID_IXMLDOMAttribute && _nodeType == NODE_ATTRIBUTE)
    {
        *ppv = static_cast<IXMLDOMAttribute *>(this);
    }
    else if (riid == IID_IXMLDOMProcessingInstruction && _nodeType == NODE_PROCESSING_INSTRUCTION)
    {
         *ppv = static_cast<IXMLDOMProcessingInstruction *>(this);
    }
    else if (riid == IID_IXMLDOMEntityReference && _nodeType == NODE_ENTITY_REFERENCE)
    {
         *ppv = static_cast<IXMLDOMEntityReference *>(this);
    } 
    else if (riid == IID_IXMLDOMCharacterData && (_nodeType == NODE_TEXT || _nodeType == NODE_CDATA_SECTION || _nodeType == NODE_COMMENT))
    {
         *ppv = static_cast<IXMLDOMComment *>(this);
    }
    else if (riid == IID_IXMLDOMComment && _nodeType == NODE_COMMENT)
    {
        *ppv = static_cast<IXMLDOMComment *>(this);
    }
    else if (riid == IID_IXMLDOMText && (_nodeType == NODE_CDATA_SECTION || _nodeType == NODE_TEXT))
    {
        *ppv = static_cast<IXMLDOMText *>(this);
    }
    else if (riid == IID_IXMLDOMCDATASection && _nodeType == NODE_CDATA_SECTION)
    {
        *ppv = static_cast<IXMLDOMCDATASection *>(this);
    }
    else if (riid == IID_ISupportErrorInfo)
    {
        *ppv = static_cast<ISupportErrorInfo *>(this);
    }
    else if (riid == IID_IElementBehavior)
    {
        *ppv = static_cast<IElementBehavior *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE 
DOMNodeWrapper::AddRef()
{
    return InterlockedIncrement(&_refcount);
}

ULONG STDMETHODCALLTYPE 
DOMNodeWrapper::Release()
{
    if (InterlockedDecrement(&_refcount) == 0)
    {
        delete this;
        return 0;
    }
    return _refcount;
}

//////////////////////////////////////////////
//// IDispatch methods

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::GetTypeInfoCount( 
    /* [out] */ UINT __RPC_FAR *pctinfo)
{
    return _dispatchImpl::GetTypeInfoCount(&s_dispatchinfoDOM, pctinfo);
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::GetTypeInfo( 
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return _dispatchImpl::GetTypeInfo(&s_dispatchinfoDOM, iTInfo, lcid, ppTInfo); 
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::GetIDsOfNames( 
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    HRESULT hr = _dispatchImpl::GetIDsOfNames( &s_dispatchinfoDOM,
                                               riid, rgszNames, cNames, lcid, rgDispId);
    if (S_OK != hr)
    {
        DispInfo * pInfo = &aDispInfo[_nodeType];
        if (pInfo->dispatchinfo._puuid)
            hr = _dispatchImpl::GetIDsOfNames( &pInfo->dispatchinfo,
                                               riid, rgszNames, cNames, lcid, rgDispId);
    } // if (!XMLDOMNode_method)

    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::Invoke( 
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    HRESULT hr;
    if (DISPID_DOM_W3CWRAPPERS < dispIdMember && dispIdMember < DISPID_DOM_W3CWRAPPERS_TOP)
    {
        DispInfo * pInfo = &aDispInfo[_nodeType];
        if (pInfo->dispatchinfo._puuid)
            hr = _dispatchImpl::Invoke( &pInfo->dispatchinfo, (pInfo->func)(this),
                                          dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    else
        hr = _dispatchImpl::Invoke( &s_dispatchinfoDOM, this,
                                      dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

//////////////////////////////////////////////
//// ISupportErrorInfo

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::InterfaceSupportsErrorInfo(REFIID riid)
{
    if ((riid == IID_IXMLDOMNode) ||
        (riid == IID_IXMLDOMElement) ||
        (riid == IID_IXMLDOMAttribute) ||
        (riid == IID_IXMLDOMText) ||
        (riid == IID_IXMLDOMCDATASection) ||
        (riid == IID_IXMLDOMEntityReference) ||
        (riid == IID_IXMLDOMProcessingInstruction) ||
        (riid == IID_IXMLDOMComment) ||
        (riid == IID_IXMLDOMCharacterData) )
            return S_OK;
    return S_FALSE;    

}       

void 
DOMNodeWrapper::setErrorInfo(WCHAR * szDescription)
{
    _dispatchImpl::setErrorInfo(szDescription);
}
    

//////////////////////////////////////////////
//// IElementBehavior
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::Init(
    IElementBehaviorSite *pPeerSite)
{
    HRESULT hr;
    IHTMLElement *pIElement = NULL;
    IUnknown *pIUnk = NULL;

    // NOTE no need to addref - the reference does it for us
    _pPeerSite = pPeerSite; 

    hr = _pPeerSite->GetElement(&pIElement);
    CHECKHR(hr);
    hr = pIElement->QueryInterface(IID_IUnknown, (void **)&pIUnk);
    CHECKHR(hr);
    _pWrapped = pIUnk;
    _pWrapped->AddRef();
    sniffType();  // die gracefully if you don't get it
CleanUp:
    SafeRelease(pIElement);
    SafeRelease(pIUnk);
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::Notify(
    LONG event, 
    VARIANT * pVar)
{
    HRESULT hr = S_OK;

    switch (event)
    {
        // BEHAVIOREVENT_CONTENTREADY is sent immediately after the peer's end tag is parsed
        // and at any subsequent time that the content of the tag is modified
        case BEHAVIOREVENT_CONTENTREADY:
        {
            hr = InitPeer();
            break;
        }

        // BEHAVIOREVENT_DOCUMENTREADY is sent when the entire document has been parsed
        case BEHAVIOREVENT_DOCUMENTREADY:
        {
            break;
        }

    }
    return hr;
}


HRESULT DOMNodeWrapper::InitPeer(void)
{
    return S_OK;
}



HRESULT DOMNodeWrapper::sniffType(void)
{
    HRESULT hr = S_OK;
    IHTMLCommentElement *pIComment = NULL;
    IHTMLDOMTextNode *pIText = NULL;
    IHTMLElement *pIElem = NULL;
    BSTR bName = NULL;

    if (_nodeType == NODE_UNKNOWN && _pWrapped)
    {
        // first look for comments by checking for the comment interface
        hr = QICOMMENTELEM(&pIComment);
        if (SUCCEEDED(hr))
        {
            _nodeType = NODE_COMMENT;
            goto CleanUp;
        }
        if (hr != E_NOINTERFACE)  // some other error
            goto CleanUp;
        // look for text nodes by checking for the text dom node interface
        hr = QITEXTDOM(&pIText);
        if (SUCCEEDED(hr))
        {
            _nodeType = NODE_TEXT;
            goto CleanUp;
        }
        if (hr != E_NOINTERFACE)  // some other error
            goto CleanUp;
        // now check for the special element names for pi, cdata, entityref
        hr = QIHTMLELEM(&pIElem);
        CHECKHR(hr);
        hr = pIElem->get_tagName(&bName);
        CHECKHR(hr);
        // BUGBUG need to check for length
        if (::memcmp(bName, L"XMVPI", 5 * sizeof(WCHAR)) == 0)
            _nodeType = NODE_PROCESSING_INSTRUCTION;
        else if (::memcmp(bName, L"XMVER", 5 * sizeof(WCHAR)) == 0)
            _nodeType = NODE_ENTITY_REFERENCE;
        else if (::memcmp(bName, L"XMVCD", 5 * sizeof(WCHAR)) == 0)
            _nodeType = NODE_CDATA_SECTION;
        else 
            _nodeType = NODE_ELEMENT;
    }

CleanUp:
    SafeRelease(pIComment);
    SafeRelease(pIText);
    SafeRelease(pIElem);
    SafeFreeString(bName);
    return hr;
}


HRESULT DOMNodeWrapper::wrapNode(IHTMLDOMNode *pDOMNode, IXMLDOMNode **ppNode)
{
    HRESULT hr;
    IUnknown *punkElem = NULL;
    DOMNodeWrapper *pWrapper = NULL;

    hr = pDOMNode->QueryInterface(IID_IUnknown, (void **)&punkElem);
    CHECKHR(hr);

    pWrapper = new DOMNodeWrapper(punkElem);
    if (!pWrapper) {
        hr  = E_OUTOFMEMORY;
        goto CleanUp;
    }
    hr = pWrapper->QueryInterface(IID_IXMLDOMNode, (void **)ppNode);
    if (hr) {
        delete pWrapper;
        goto CleanUp;
    }
    SafeRelease(pWrapper);

CleanUp:
    SafeRelease(punkElem);
    return hr;
}

HRESULT DOMNodeWrapper::getWrapper(IHTMLDOMNode *pDOMNode, IXMLDOMNode **ppNode)
{
    HRESULT hr;
    IUnknown *punkElem = NULL;
    IDispatch *pdispPeer = NULL;

    // get the raw element
    hr = pDOMNode->QueryInterface(IID_IUnknown, (void **)&punkElem);
    CHECKHR(hr);
    // now get IDispatch which should delegate to our peer
    hr = punkElem->QueryInterface(IID_IDispatch, (void **)&pdispPeer);
    CHECKHR(hr);
    // finally get the dom interface that we want
    hr = pdispPeer->QueryInterface(IID_IXMLDOMNode, (void **)ppNode);
CleanUp:
    SafeRelease(punkElem);
    SafeRelease(pdispPeer);
    return hr;
}



//////////////////////////////////////////////
//// IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_nodeType( 
    /* [out][retval] */ DOMNodeType __RPC_FAR *plType)
{
    *plType = NODE_UNKNOWN;
    HRESULT hr = sniffType();
    if (SUCCEEDED(hr))
    {
        if (_nodeType == NODE_UNKNOWN)
            hr = E_FAIL;
        else
            *plType = _nodeType;
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_nodeName( 
    /* [out][retval] */ BSTR __RPC_FAR *pName)
{
#if NOWRAPPER
    *pName = NULL;
    return E_FAIL;
#else
    // BUGBUG this should change when TRIDENT implements this feature
    HRESULT hr;
    DOMNodeType type;
    IHTMLElement *pIElem = NULL;
    IHTMLDOMAttribute *pIAttr = NULL;
    BSTR bstr = NULL;
    TCHAR * pwc = NULL;

    *pName = NULL;
    hr = get_nodeType(&type);
    CHECKHR(hr);

    switch (type)
    {
    case NODE_ELEMENT:
        hr = QIHTMLELEM(&pIElem);
        CHECKHR(hr);
        hr = pIElem->get_tagName(pName);
        break;
    case NODE_ATTRIBUTE:
        hr = QIATTRDOM(&pIAttr);
        CHECKHR(hr);
        hr = pIAttr->get_nodeName(pName);
        break;
    case NODE_PROCESSING_INSTRUCTION:
    case NODE_ENTITY_REFERENCE:
        {
        VARIANT vAttr;
        // look for the special attribute which stores the name
        hr = QIHTMLELEM(&pIElem);
        CHECKHR(hr);
        VariantInit(&vAttr);
        bstr = ::SysAllocString(L"omn");
        hr = pIElem->getAttribute(bstr, 0, &vAttr);
        CHECKHR(hr);
        if (vAttr.vt == VT_BSTR)
            *pName = V_BSTR(&vAttr);
        else 
        {
            VariantClear(&vAttr);
            hr = E_FAIL;
        }
        break;
        }
    case NODE_CDATA_SECTION:
        pwc = _T("#cdata-section"); 
        break;
    case NODE_TEXT:
        pwc = _T("#text");
        break;
    case NODE_COMMENT:
        pwc = _T("#comment");
        break;
    default:
        hr = E_FAIL;
    }

    if (pwc)
        *pName = ::SysAllocString(pwc);
        
CleanUp:
    SafeRelease(pIElem);
    SafeRelease(pIAttr);
    SafeFreeString(bstr);
    return hr;
#endif
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_nodeValue( 
    /* [out][retval] */ VARIANT __RPC_FAR *pVal)
{
#if NOWRAPPER
    VariantClear(pVal);
    return E_FAIL;
#else
    HRESULT hr;
    DOMNodeType type;
    IHTMLElement *pIElem = NULL;
    IHTMLDOMAttribute *pIAttr = NULL;
    IHTMLDOMTextNode *pIText = NULL;
    IHTMLCommentElement *pIComment = NULL;
    BSTR bstr = NULL;
    BSTR bstrFetch = NULL;

    // BUGBUG this should change when TRIDENT implements this feature    
    VariantClear(pVal);

    hr = get_nodeType(&type);
    CHECKHR(hr);

    switch (type)
    {
    case NODE_CDATA_SECTION:
        hr = QIHTMLELEM(&pIElem);
        CHECKHR(hr);
        hr = pIElem->get_innerText(&bstr);
        break;
    case NODE_ATTRIBUTE:
        {
        VARIANT vAttr;
        VariantInit(&vAttr);
        hr = QIATTRDOM(&pIAttr);
        CHECKHR(hr);
        hr = pIAttr->get_nodeValue(&vAttr);
        CHECKHR(hr);
        if (vAttr.vt == VT_BSTR)
            bstr = V_BSTR(&vAttr);
        else
        {
            VariantClear(&vAttr);
            hr = E_FAIL;
        }
        break;
        }
    case NODE_TEXT:
        hr = QITEXTDOM(&pIText);
        CHECKHR(hr);
        hr = pIText->get_data(&bstr);
        break;
    case NODE_COMMENT:
        hr = QICOMMENTELEM(&pIComment);
        CHECKHR(hr);
        hr = pIComment->get_text(&bstr);
        break;
    case NODE_PROCESSING_INSTRUCTION:
        {
        VARIANT vAttr;
        // look for the special attribute which stores the value
        hr = QIHTMLELEM(&pIElem);
        CHECKHR(hr);
        VariantInit(&vAttr);
        bstrFetch = ::SysAllocString(L"omv");
        hr = pIElem->getAttribute(bstrFetch, 1, &vAttr);
        CHECKHR(hr);
        if (vAttr.vt == VT_BSTR)
            bstr = V_BSTR(&vAttr);
        else
        {
            VariantClear(&vAttr);
            hr = E_FAIL;
        }
        break;
        }
    case NODE_ELEMENT:
    case NODE_ENTITY_REFERENCE:
        pVal->vt = VT_NULL;
        break;
    default:
        hr = E_FAIL;
        break;
    }

    if (bstr) {
        pVal->vt = VT_BSTR;
        V_BSTR(pVal) = bstr;
    }

CleanUp:
    SafeRelease(pIElem);
    SafeRelease(pIAttr);
    SafeRelease(pIText);
    SafeRelease(pIComment);
    SafeFreeString(bstrFetch);
    return hr;
#endif
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::put_nodeValue( 
    /* [in] */ VARIANT vVal)
{
#if NOWRAPPER
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    DOMNodeType type;
    IHTMLElement *pIElem = NULL;
    IHTMLDOMAttribute *pIAttr = NULL;
    IHTMLDOMTextNode *pIText = NULL;
    IHTMLCommentElement *pIComment = NULL;
    BSTR bstrName = NULL;
    VARIANT vBstrVal; vBstrVal.vt = VT_NULL;
    BSTR bstrVal;
    // BUGBUG this should change when TRIDENT implements this feature
    hr = get_nodeType(&type);
    CHECKHR(hr);
    hr = VariantChangeTypeEx(&vBstrVal, &vVal, GetThreadLocale(), NULL, VT_BSTR);
    CHECKHR(hr);
    bstrVal = V_BSTR(&vBstrVal);
    switch (type)
    {
    case NODE_ELEMENT:
    case NODE_CDATA_SECTION:
        hr = QIHTMLELEM(&pIElem);
        CHECKHR(hr);
        hr = pIElem->put_innerText(bstrVal);
        break;
    case NODE_ATTRIBUTE:
        {
        hr = QIATTRDOM(&pIAttr);
        CHECKHR(hr);
        hr = pIAttr->put_nodeValue(vBstrVal);
        break;
        }
    case NODE_TEXT:
        hr = QITEXTDOM(&pIText);
        CHECKHR(hr);
        hr = pIText->put_data(bstrVal);
        break;
    case NODE_PROCESSING_INSTRUCTION:
        {
        // set the special attribute    
        hr = QIHTMLELEM(&pIElem);
        CHECKHR(hr);
        bstrName = ::SysAllocString(L"omv");
        hr = pIElem->setAttribute(bstrName, vBstrVal, 1);
        break;
        }
    case NODE_COMMENT:
        hr = QICOMMENTELEM(&pIComment);
        CHECKHR(hr);
        hr = pIComment->put_text(bstrVal);
        break;
    default:
        // not allowed for other types
        // BUGBUG entity reference should raise an exception as per DOM spec
        hr = E_FAIL;
        break;
    }

CleanUp:
    SafeRelease(pIElem);
    SafeRelease(pIAttr);
    SafeRelease(pIText);
    SafeRelease(pIComment);
    SafeFreeString(bstrName);
    VariantClear(&vBstrVal);
    return hr;
#endif
}


    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_parentNode( 
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
#if NOWRAPPER
    *ppNode = NULL;
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    DOMNodeType type;
    IHTMLDOMNode *pIXMLDOMNode = NULL;
    IHTMLDOMNode *pIXMLDOMChild = NULL;

    *ppNode = NULL;
    hr = get_nodeType(&type);
    CHECKHR(hr);
    
    if (type==NODE_ATTRIBUTE) {
        hr = E_FAIL;
        goto CleanUp;
    }
    
    hr = QIHTMLDOM(&pIXMLDOMNode);
    CHECKHR(hr);    
    hr = pIXMLDOMNode->get_parentNode(&pIXMLDOMChild);
    CHECKHR(hr);

    // BUGBUG: check for the injection and skip the body and html tags

    
    // prepare the wrapper 
    hr = wrapNode(pIXMLDOMChild, ppNode);

CleanUp:
    SafeRelease(pIXMLDOMNode);
    SafeRelease(pIXMLDOMChild);
    return hr;
#endif
}
     
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_childNodes( 
    /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
{
#if NOWRAPPER
    *ppNodeList = NULL;
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    DOMNodeType type;
    IHTMLDOMNode *pIDOMNode = NULL;
    IDispatch *pIDisp = NULL;

    *ppNodeList = NULL;

    hr = get_nodeType(&type);
    CHECKHR(hr);
    if (type!=NODE_ELEMENT) {
        hr = E_FAIL;
        goto CleanUp;
    }

    hr = QIHTMLDOM(&pIDOMNode);
    CHECKHR(hr);    
    hr = pIDOMNode->get_childNodes(&pIDisp);
    CHECKHR(hr);
    hr = DOMNodeListWrapper::wrapList(pIDisp, ppNodeList);
        
CleanUp:
    SafeRelease(pIDisp);
    SafeRelease(pIDOMNode);
    return hr;
#endif
}
     
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_firstChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppFirstChild)
{
    // BUGBUGTODO 
    return E_FAIL;
}
 
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_lastChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppLastChild)
{
    // BUGBUGTODO 
    return E_FAIL;
}
     
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_previousSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppPrevSib)
{
    *ppPrevSib = NULL;
    return S_FALSE;
}
     
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_nextSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNextSib)
{
    *ppNextSib = NULL;
    return S_FALSE;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::insertBefore( 
    /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
    /* [in] */ VARIANT refChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
{
#if NOWRAPPER
    *ppNewChild = NULL;
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    DOMNodeType type;
    IHTMLDOMNode *pIDOMNode = NULL;
    IHTMLDOMNode *pIDOMChild = NULL;
    IHTMLDOMNode *pIDOMChildRet = NULL;

    *ppNewChild = NULL;
    hr = get_nodeType(&type);
    CHECKHR(hr);
    
    if (type!=NODE_ELEMENT) {
        hr = E_FAIL;
        goto CleanUp;
    }
    
    hr = QIHTMLDOM(&pIDOMNode);
    CHECKHR(hr);

    // BUGBUG how to get IHTMLDOM from IDOMNode interface
//  hr = newChild->GetHTMLDOMPOINTER(&pIDOMChild);
//  CHECKHR(hr);

    // BUGBUG HAVE TO CRACK THE VARIANT



    hr = pIDOMNode->insertBefore(pIDOMChild, refChild, &pIDOMChildRet);
    CHECKHR(hr);

    // prepare the wrapper 
    hr = wrapNode(pIDOMChildRet, ppNewChild);

CleanUp:
    SafeRelease(pIDOMNode);
    SafeRelease(pIDOMChild);
    SafeRelease(pIDOMChildRet);

    return hr;
#endif
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::replaceChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *pNew,
    /* [in] */ IXMLDOMNode __RPC_FAR *pOld,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
#if NOWRAPPER
    *ppNode = NULL;
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    DOMNodeType type;
    IHTMLDOMNode *pIDOMNode = NULL;
    IHTMLDOMNode *pIDOMOldChild = NULL;
    IHTMLDOMNode *pIDOMNewChild = NULL;
    IHTMLDOMNode *pIDOMChildRet = NULL;

    *ppNode = NULL;
    hr = get_nodeType(&type);
    CHECKHR(hr);
    
    if (type!=NODE_ELEMENT) {
        hr = E_FAIL;
        goto CleanUp;
    }
    
    hr = QIHTMLDOM(&pIDOMNode);
    CHECKHR(hr);
    // BUGBUG how to get IHTMLDOM from IDOMNode interface
//  hr = pNew->GetHTMLDOMPOINTER(&pIDOMNewChild);
//  CHECKHR(hr);
//  hr = pOld->GetHTMLDOMPOINTER(&pIDOMOldChild);
//  CHECKHR(hr);
    hr = pIDOMNode->replaceChild(pIDOMNewChild, pIDOMOldChild, &pIDOMChildRet);
    CHECKHR(hr);

    // prepare the wrapper 
    hr = wrapNode(pIDOMChildRet, ppNode);

CleanUp:
    SafeRelease(pIDOMNode);
    SafeRelease(pIDOMOldChild);
    SafeRelease(pIDOMNewChild);
    SafeRelease(pIDOMChildRet);
    return hr;
#endif
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::removeChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppOldChild)
{
#if NOWRAPPER
    *ppOldChild = NULL;
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    DOMNodeType type;
    IHTMLDOMNode *pIDOMNode = NULL;
    IHTMLDOMNode *pIDOMOldChild = NULL;
    IHTMLDOMNode *pIDOMChildRet = NULL;

    *ppOldChild = NULL;
    hr = get_nodeType(&type);
    CHECKHR(hr);
    
    if (type!=NODE_ELEMENT) {
        hr = E_FAIL;
        goto CleanUp;
    }
    
    hr = QIHTMLDOM(&pIDOMNode);
    CHECKHR(hr);
    // BUGBUG how to get IHTMLDOM from IDOMNode interface
//  hr = oldChild->GetHTMLDOMPOINTER(&pIDOMOldChild);
//  CHECKHR(hr);
    hr = pIDOMNode->removeChild(pIDOMOldChild, &pIDOMChildRet);
    CHECKHR(hr);

    // prepare the wrapper 
    hr = wrapNode(pIDOMChildRet, ppOldChild);

CleanUp:
    SafeRelease(pIDOMNode);
    SafeRelease(pIDOMOldChild);
    SafeRelease(pIDOMChildRet);
    return hr;
#endif
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::appendChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
{
#if NOWRAPPER
    *ppNewChild = NULL;
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    DOMNodeType type;
    IHTMLDOMNode *pIDOMNode = NULL;
    IHTMLDOMNode *pIDOMNewChild = NULL;
    IHTMLDOMNode *pIDOMChildRet = NULL;
    VARIANT vInsert;

    *ppNewChild = NULL;
    hr = get_nodeType(&type);
    CHECKHR(hr);
    
    if (type!=NODE_ELEMENT) {
        hr = E_FAIL;
        goto CleanUp;
    }
    
    hr = QIHTMLDOM(&pIDOMNode);
    CHECKHR(hr);
    // BUGBUG how to get IHTMLDOM from IDOMNode interface
//  hr = newChild->GetHTMLDOMPOINTER(&pIDOMNewChild);
//  CHECKHR(hr);
    vInsert.vt = VT_NULL;
    hr = pIDOMNode->insertBefore(pIDOMNewChild, vInsert, &pIDOMChildRet);
    CHECKHR(hr);

    // prepare the wrapper 
    hr = wrapNode(pIDOMChildRet, ppNewChild);

CleanUp:
    SafeRelease(pIDOMNode);
    SafeRelease(pIDOMNewChild);
    SafeRelease(pIDOMChildRet);

    return hr;
#endif
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_attributes( 
    /* [out][retval] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppAttributes)
{
#if NOWRAPPER
    *ppAttributes = NULL;
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    IHTMLDOMNode *pIDOMNode = NULL;
    IDispatch *pIDisp = NULL;

    *ppAttributes = NULL;
    hr = QIHTMLDOM(&pIDOMNode);
    CHECKHR(hr);    
    hr = pIDOMNode->get_attributes(&pIDisp);
    CHECKHR(hr);
    hr = DOMNamedMapWrapper::wrapNodeMap(pIDisp, ppAttributes);
        
CleanUp:
    SafeRelease(pIDisp);
    SafeRelease(pIDOMNode);
    return hr;
#endif
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::hasChildNodes( 
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBool)
{
    HRESULT hr;
    DOMNodeType type;
    
    hr = get_nodeType(&type);
    if (!SUCCEEDED(hr))
        return hr;
    
    if (type == NODE_ELEMENT)
    {
        *pBool = VARIANT_TRUE;
        hr = S_OK;
    }
    else
    {
        *pBool = VARIANT_FALSE;
        hr = S_FALSE;
    }
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::cloneNode( 
    /* [in] */ VARIANT_BOOL deep,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot)
{
    // BUGBUGTODO 
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_ownerDocument( 
    /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR * ppDOMDocument)
{
    // BUGBUGTODO 
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_nodeTypeString( 
    /* [out][retval] */ BSTR __RPC_FAR *nodeType)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_text( 
    /* [out][retval] */ BSTR __RPC_FAR *text)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::put_text( 
    /* [in] */ BSTR text)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_specified( 
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_definition( 
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_nodeTypedValue( 
    /* [out][retval] */ VARIANT __RPC_FAR *TypedValue)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::put_nodeTypedValue( 
    /* [in] */ VARIANT TypedValue)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_dataType( 
    /* [out][retval] */ VARIANT __RPC_FAR *p)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::put_dataType( 
    /* [in] */ BSTR p)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_xml(
    /* [out][retval] */ BSTR __RPC_FAR *pbstrXml)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::transformNode( 
    /* [in] */ IXMLDOMNode __RPC_FAR *pStyleSheet,
    /* [out][retval] */ BSTR __RPC_FAR *pbstr)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::selectNodes( 
    /* [in] */ BSTR bstrXQL,
    /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppResult)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::selectSingleNode( 
    /* [in] */ BSTR queryString,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *resultNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_parsed( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isParsed)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_namespaceURI( 
    /* [out][retval] */ BSTR __RPC_FAR *pURI)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_prefix( 
    /* [out][retval] */ BSTR __RPC_FAR *pPrefix)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_baseName( 
    /* [out][retval] */ BSTR __RPC_FAR *pBaseName)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::transformNodeToObject( 
    /* [in] */ IXMLDOMNode __RPC_FAR *stylesheet,
    /* [in] */ VARIANT outputObject)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}


//////////////////////////////////////////////
//// IXMLDOMElement : IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_tagName( 
    /* [retval][out] */ BSTR __RPC_FAR *pTagName)
{
    HRESULT hr = S_OK;
    IHTMLElement *pIElem = NULL;
    hr = QIHTMLELEM(&pIElem);
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::getAttribute( 
    /* [in] */ BSTR name,
    /* [retval][out] */ VARIANT __RPC_FAR *pValue)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::setAttribute( 
    /* [in] */ BSTR name,
    /* [in] */ VARIANT value)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::removeAttribute( 
    /* [in] */ BSTR name)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::getAttributeNode( 
    /* [in] */ BSTR name,
    /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *ppAttr)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::setAttributeNode( 
    /* [in] */ IXMLDOMAttribute __RPC_FAR * pDOMAttribute,
    /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR * ppAttributeNode)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::removeAttributeNode( 
    /* [in] */ IXMLDOMAttribute __RPC_FAR * pDOMAttribute,
    /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR * ppAttributeNode)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::getElementsByTagName( 
    /* [in] */ BSTR tagname,
    /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE DOMNodeWrapper::normalize(void)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////
//// IXMLDOMAttribute : IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_name( 
    /* [retval][out] */ BSTR __RPC_FAR *attributeName)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_value( 
    /* [retval][out] */ VARIANT __RPC_FAR *attributeValue)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::put_value( 
    /* [in] */ VARIANT attributeValue)
{
    return E_FAIL;
}

//////////////////////////////////////////////
//// IXMLDOMProcessingInstruction : IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_target( 
    /* [retval][out] */ BSTR __RPC_FAR *pStr)
{
    return E_FAIL;
}

//////////////////////////////////////////////
//// IXMLDOMCharacterData : IXMLDOMNode

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_data( 
    /* [retval][out] */ BSTR __RPC_FAR *data)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::put_data( 
    /* [in] */ BSTR data)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::get_length( 
    /* [retval][out] */ long __RPC_FAR *dataLength)
{
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::substringData( 
    /* [in] */ long offset,
    /* [in] */ long count,
    /* [retval][out] */ BSTR __RPC_FAR *data)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::appendData( 
    /* [in] */ BSTR data)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::insertData( 
    /* [in] */ long offset,
    /* [in] */ BSTR data)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::deleteData( 
    /* [in] */ long offset,
    /* [in] */ long count)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::replaceData( 
    /* [in] */ long offset,
    /* [in] */ long count,
    /* [in] */ BSTR data)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////
//// IXMLDOMText : IXMLDOMCharacterData

HRESULT STDMETHODCALLTYPE 
DOMNodeWrapper::splitText( 
    /* [in] */ long offset,
    /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *ppNewTextNode)
{
    return E_FAIL;
}

    
///////////////////////////////////////////////////////////////////////////////
//
//                        DOMGhostNodeWrapper class
//
///////////////////////////////////////////////////////////////////////////////

// DOMGhostNodeWrapper class 
DOMGhostNodeWrapper::DOMGhostNodeWrapper(IDispatch *pDisp, IDispatch *pParentDisp)
    : DOMNodeWrapper(pDisp, NODE_ATTRIBUTE)
{
    _pParentWrapped = pParentDisp;
    if (_pParentWrapped)
        _pParentWrapped->AddRef();
}

DOMGhostNodeWrapper::~DOMGhostNodeWrapper()
{
    SafeRelease(_pParentWrapped);
}

// IXMLDOMNode Methods
STDMETHODIMP
DOMGhostNodeWrapper::get_nodeType( 
    /* [out][retval] */ DOMNodeType FAR *plType)
{
    return _nodeType;
}

STDMETHODIMP
DOMGhostNodeWrapper::get_parentNode( 
    /* [out][retval] */ IXMLDOMNode FAR *FAR *ppNode)
{
    // BUGBUGTODO 
    return E_FAIL;
}


///////////////////////////////////////////////////////////////////////////////
//
//                        DOMNodeListWrapper class
//
///////////////////////////////////////////////////////////////////////////////

//DISPATCHINFO _dispatch<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList>::s_dispatchinfo = 
//{
//    NULL, &IID_IXMLDOMNodeList, &LIBID_MSXML, ORD_MSXML
//};

DOMNodeListWrapper::~DOMNodeListWrapper()
{
    SafeRelease(_pWrapped);
}

void DOMNodeListWrapper::SetWrapped(IDispatch *pIDisp)
{   
    _pWrapped = pIDisp;
    if (_pWrapped)
        _pWrapped->AddRef();
}

HRESULT 
DOMNodeListWrapper::wrapList(IDispatch *pDisp, IXMLDOMNodeList **ppListRet)
{
    HRESULT hr = E_FAIL;
    DOMNodeListWrapper *pWrapper;
    pWrapper = new DOMNodeListWrapper();
    if (pWrapper)
    {
        pWrapper->SetWrapped(pDisp);
        hr = pWrapper->QueryInterface(IID_IXMLDOMNodeList, (void **)ppListRet);
        if (!SUCCEEDED(hr))
            delete pWrapper;
    }
    return hr;
}

// IXMLDOMNodeList Methods
HRESULT STDMETHODCALLTYPE 
DOMNodeListWrapper::get_item( 
    /* [in] */ long index,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *listItem)
{
    // BUGBUGTODO 
    return E_FAIL;
}
        
HRESULT STDMETHODCALLTYPE 
DOMNodeListWrapper::get_length( 
    /* [retval][out] */ long __RPC_FAR *listLength)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeListWrapper::nextNode(
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeListWrapper::reset( void)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNodeListWrapper::get__newEnum( 
    /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//
//                        DOMNamedMapWrapper class
//
///////////////////////////////////////////////////////////////////////////////

//DISPATCHINFO _dispatch<IXMLDOMNamedNodeMap, &LIBID_MSXML, &IID_IXMLDOMNamedNodeMap>::s_dispatchinfo = 
//{
//    NULL, &IID_IXMLDOMNamedNodeMap, &LIBID_MSXML, ORD_MSXML
//};

DOMNamedMapWrapper::~DOMNamedMapWrapper()
{
    SafeRelease(_pWrapped);
}

void DOMNamedMapWrapper::SetWrapped(IDispatch *pIDisp)
{   
    _pWrapped = pIDisp;
    if (_pWrapped)
        _pWrapped->AddRef();
}


HRESULT DOMNamedMapWrapper::wrapNodeMap(IDispatch *pDisp, IXMLDOMNamedNodeMap **ppNodeMap)
{
    HRESULT hr = E_FAIL;
    DOMNamedMapWrapper *pWrapper;
    pWrapper = new DOMNamedMapWrapper();
    if (pWrapper)
    {
        pWrapper->SetWrapped(pDisp);
        hr = pWrapper->QueryInterface(IID_IXMLDOMNamedNodeMap, (void **)ppNodeMap);
        if (!SUCCEEDED(hr))
            delete pWrapper;
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::getNamedItem( 
    /* [in] */ BSTR name,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *namedItem)
{
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::setNamedItem(
    /* [in] */ IXMLDOMNode __RPC_FAR *newItem,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *namedItem)
{
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::removeNamedItem( 
    /* [in] */ BSTR name,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *namedItem)
{
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::get_item( 
    /* [in] */ long index,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *listItem)
{
    // BUGBUGTODO 
    return E_FAIL;
}
        
HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::get_length( 
    /* [retval][out] */ long __RPC_FAR *listLength)
{
    // BUGBUGTODO 
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::nextNode(
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::reset( void)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::get__newEnum( 
    /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::getQualifiedItem( 
    /* [in] */ BSTR name,
    /* [in] */ BSTR nameSpaceURN,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
DOMNamedMapWrapper::removeQualifiedItem( 
    /* [in] */ BSTR name,
    /* [in] */ BSTR NameSpace,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
//
//                        DOMDocViewerWrapper class
//
///////////////////////////////////////////////////////////////////////////////

// cache the logical root of the XML tree
// BUGBUG QueryInterface template is bad, won't return valid pointer for IXMLDOMNode base class

DISPATCHINFO _dispatch<IXMLDOMDocument, &LIBID_MSXML, &IID_IXMLDOMDocument>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDOMDocument, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL
};


DOMDocViewerWrapper::~DOMDocViewerWrapper()
{
    SafeRelease(_pWrapped);
}

void DOMDocViewerWrapper::SetWrapped(IDispatch *pIDisp)
{
    _pWrapped = pIDisp;
    if (_pWrapped)
        _pWrapped->AddRef();
}

// IXMLDOMNode interfaces
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_nodeName( 
    /* [retval][out] */ BSTR __RPC_FAR *name)
{
    *name = ::SysAllocString(_T("#document"));
    return (*name) ? S_OK : E_OUTOFMEMORY;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_nodeValue( 
    /* [retval][out] */ VARIANT __RPC_FAR *value)
{
    VariantClear(value);
    value->vt = VT_NULL;
    return S_OK;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::put_nodeValue( 
        /* [in] */ VARIANT value)
{
    // BUGBUG entity reference should raise an exception as per DOM spec
    return E_FAIL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_nodeType( 
    /* [retval][out] */ DOMNodeType __RPC_FAR *type)
{
    *type = NODE_DOCUMENT;
    return S_OK;
}        
    
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_parentNode( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *parent)
{
    *parent = NULL;
    return S_FALSE;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_childNodes( 
    /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *childList)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_firstChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *firstChild)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_lastChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *lastChild)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_previousSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *previousSibling)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_nextSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *nextSibling)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_attributes( 
    /* [retval][out] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *atrributeMap)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::insertBefore( 
    /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
    /* [in] */ VARIANT refChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outNewChild)
{
    return E_NOTIMPL;
}        
    
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::replaceChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
    /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outOldChild)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::removeChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *childNode,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *oldChild)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::appendChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outNewChild)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::hasChildNodes( 
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *hasChild)
{
    *hasChild = VARIANT_TRUE;
    return S_OK;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_ownerDocument( 
    /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR *DOMDocument)
{
    *DOMDocument = NULL;
    return S_FALSE;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::cloneNode( 
    /* [in] */ VARIANT_BOOL deep,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *cloneRoot)
{
    return E_FAIL;
}        

// IXMLDOMDocument interfaces
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_doctype( 
    /* [retval][out] */ IXMLDOMDocumentType __RPC_FAR *__RPC_FAR *documentType)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_implementation( 
    /* [retval][out] */ IXMLDOMImplementation __RPC_FAR *__RPC_FAR *impl)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::get_documentElement( 
    /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR *DOMElement)
{
    // BUGBUGTODO 
    return E_FAIL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::putref_documentElement( 
    /* [in] */ IXMLDOMElement __RPC_FAR *DOMElement)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createElement( 
    /* [in] */ BSTR tagName,
    /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR *element)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createDocumentFragment( 
    /* [retval][out] */ IXMLDOMDocumentFragment __RPC_FAR *__RPC_FAR *docFrag)
{
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createTextNode( 
    /* [in] */ BSTR data,
    /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *text)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createComment( 
    /* [in] */ BSTR data,
    /* [retval][out] */ IXMLDOMComment __RPC_FAR *__RPC_FAR *comment)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createCDATASection( 
    /* [in] */ BSTR data,
    /* [retval][out] */ IXMLDOMCDATASection __RPC_FAR *__RPC_FAR *cdata)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        
    
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createProcessingInstruction( 
    /* [in] */ BSTR target,
    /* [in] */ BSTR data,
    /* [retval][out] */ IXMLDOMProcessingInstruction __RPC_FAR *__RPC_FAR *pi)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createAttribute( 
    /* [in] */ BSTR name,
    /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *attribute)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createEntityReference( 
    /* [in] */ BSTR name,
    /* [retval][out] */ IXMLDOMEntityReference __RPC_FAR *__RPC_FAR *entityRef)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        
        
HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::getElementsByTagName( 
    /* [in] */ BSTR tagName,
    /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *resultList)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE 
DOMDocViewerWrapper::createNode( 
    /* [in] */ VARIANT Type,
    /* [in] */ BSTR Text,
    /* [in] */ BSTR NameSpaceURI,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::nodeFromID( 
    /* [in] */ BSTR pbstrID,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::load( 
    /* [in] */ VARIANT vTarget,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::get_readyState( 
    /* [out][retval] */ long __RPC_FAR *plState)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::get_parseError( 
    /* [out][retval] */ IXMLDOMParseError __RPC_FAR *__RPC_FAR *pError)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::get_url( 
    /* [out][retval] */ BSTR __RPC_FAR *pbstrUrl)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::get_async( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pf)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::put_async( 
    /* [in] */ VARIANT_BOOL f)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::abort( void)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::loadXML( 
    /* [in] */ BSTR bstrXML,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::saveXML( 
    /* [retval][out] */ VARIANT vTarget) 
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::get_validateOnParse( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isValidating)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::put_validateOnParse( 
    /* [in] */ VARIANT_BOOL isValidating)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::get_resolveExternals( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfResolve)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::put_resolveExternals( 
    /* [in] */ VARIANT_BOOL fResolve)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::get_preserveWhiteSpace( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfPreserve)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::put_preserveWhiteSpace( 
    /* [in] */ VARIANT_BOOL fPreserve)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::put_onreadystatechange( 
    /* [in] */ VARIANT varF)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::put_ondataavailable( 
    /* [in] */ VARIANT varF)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

HRESULT STDMETHODCALLTYPE  
DOMDocViewerWrapper::put_ontransformnode( 
    /* [in] */ VARIANT varF)
{
    // BUGBUGTODO 
    return E_NOTIMPL;
}        

/*
 * @(#)XMLDOM.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _PARSER_OM_ELEMENT
#include "element.hxx"
#endif

#ifndef __XMLDOM_HXX
#include "xml/om/xmldom.hxx"
#endif

#ifndef _XML_OM_DOMERROR_HXX
#include "xml/om/idomerror.hxx"
#endif

#ifndef _XML_OM_XQLNODELIST
#include "xqlnodelist.hxx"
#endif

#ifndef _XQL_PARSER_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

#ifndef _XTL_ENGINE_PROCESSOR
#include "xtl/engine/processor.hxx"
#endif

#ifndef _XML_OM_DOMIMPLEMENTATION_HXX
#include "domimplementation.hxx"
#endif

#ifndef _XML_OM_OMLOCK
#include "xml/om/omlock.hxx"
#endif

#ifndef _CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif

#ifndef _XML_OM_DOCSTREAM
#include "xml/om/docstream.hxx"
#endif

#ifndef _VARIANT_HXX
#include "core/com/variant.hxx"
#endif

#include <xmldomdid.h>

#define ARGTEST(t)  if (!t) { hr=E_INVALIDARG; goto Cleanup; }
#define SAFERELEASE(p) if (p) {(p)->Release(); p = NULL;} else ;

const IID IID_IResponse = {0xD97A6DA0,0xA864,0x11cf,{0x83,0xBE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8}};
const IID IID_IRequest = {0xD97A6DA0,0xA861,0x11cf,{0x93,0xAE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8}};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data structures for customized idispatch implementation for DOMDocumentWrapper
//

extern INVOKE_METHOD s_rgDOMNodeMethods[];
extern BYTE s_cDOMNodeMethodLen;

static VARTYPE s_argTypes_DOMDocument_documentElement[] =       { VT_DISPATCH };
static VARTYPE s_argTypes_DOMDocument_createElement[] =         { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_createTextNode[] =        { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_createComment[] =         { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_createCDATASection[] =    { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_createProcessingInstruction[] =   { VT_BSTR, VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_createAttribute[] =       { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_createEntityReference[] = { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_getByTagName[] =          { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_createNode[] =            { VT_VARIANT, VT_BSTR, VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_nodeFromID[] =            { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_load[] =                  { VT_VARIANT };
static VARTYPE s_argTypes_DOMDocument_async[] =                 { VT_BOOL };
static VARTYPE s_argTypes_DOMDocument_loadXML[] =               { VT_BSTR };
static VARTYPE s_argTypes_DOMDocument_save[] =                  { VT_VARIANT };
static VARTYPE s_argTypes_DOMDocument_validateOnParse[] =       { VT_BOOL };
static VARTYPE s_argTypes_DOMDocument_resolveExternals[] =      { VT_BOOL };
static VARTYPE s_argTypes_DOMDocument_preserveWhiteSpace[] =    { VT_BOOL };
static VARTYPE s_argTypes_DOMDocument_onreadystatechange[] =    { VT_VARIANT };
static VARTYPE s_argTypes_DOMDocument_ondataavailable[] =       { VT_VARIANT };
static VARTYPE s_argTypes_DOMDocument_ontransformnode[] =       { VT_VARIANT };

static const IID*   s_argTypeIds_DOMDocument_documentElement[] = { &IID_IXMLDOMElement };

static INVOKE_METHOD 
s_rgDOMDocumentMethods[] =
{
    // name                 dispid                       argument number  argument table          argument guid id          return type    invoke kind
    {L"abort",              DISPID_XMLDOM_DOCUMENT_ABORT,            0,      NULL,                NULL,                     VT_ERROR,      DISPATCH_PROPERTYGET | DISPATCH_METHOD },
    {L"async",              DISPID_XMLDOM_DOCUMENT_ASYNC,            1,      s_argTypes_DOMDocument_async,      NULL,       VT_BOOL,       DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },

    {L"createAttribute",    DISPID_DOM_DOCUMENT_CREATEATTRIBUTE,     1,      s_argTypes_DOMDocument_createAttribute, NULL,  VT_DISPATCH,   DISPATCH_METHOD },
    {L"createCDATASection", DISPID_DOM_DOCUMENT_CREATECDATASECTION,  1,      s_argTypes_DOMDocument_createCDATASection, NULL, VT_DISPATCH, DISPATCH_METHOD },
    {L"createComment",      DISPID_DOM_DOCUMENT_CREATECOMMENT,       1,      s_argTypes_DOMDocument_createComment,  NULL,   VT_DISPATCH,   DISPATCH_METHOD },
    {L"createDocumentFragment", DISPID_DOM_DOCUMENT_CREATEDOCUMENTFRAGMENT, 0,      NULL,         NULL,                     VT_DISPATCH,   DISPATCH_PROPERTYGET | DISPATCH_METHOD },
    {L"createElement",      DISPID_DOM_DOCUMENT_CREATEELEMENT,       1,      s_argTypes_DOMDocument_createElement, NULL,    VT_DISPATCH,   DISPATCH_METHOD },
    {L"createEntityReference", DISPID_DOM_DOCUMENT_CREATEENTITYREFERENCE, 1, s_argTypes_DOMDocument_createEntityReference, NULL,    VT_DISPATCH,   DISPATCH_METHOD },
    {L"createNode",         DISPID_XMLDOM_DOCUMENT_CREATENODE,       3,      s_argTypes_DOMDocument_createNode, NULL,       VT_DISPATCH,   DISPATCH_METHOD },
    {L"createProcessingInstruction", DISPID_DOM_DOCUMENT_CREATEPROCESSINGINSTRUCTION, 2, s_argTypes_DOMDocument_createProcessingInstruction, NULL,    VT_DISPATCH,   DISPATCH_METHOD },
    {L"createTextNode",     DISPID_DOM_DOCUMENT_CREATETEXTNODE,      1,      s_argTypes_DOMDocument_createTextNode, NULL,   VT_DISPATCH,   DISPATCH_METHOD },
     
    {L"doctype",            DISPID_DOM_DOCUMENT_DOCTYPE,             0,      NULL,                NULL,                     VT_DISPATCH,   DISPATCH_PROPERTYGET },
    {L"documentElement",    DISPID_DOM_DOCUMENT_DOCUMENTELEMENT,     1,      s_argTypes_DOMDocument_documentElement,  s_argTypeIds_DOMDocument_documentElement,  VT_DISPATCH,   DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUTREF },

    {L"getElementsByTagName", DISPID_DOM_DOCUMENT_GETELEMENTSBYTAGNAME,    1, s_argTypes_DOMDocument_getByTagName,    NULL,          VT_DISPATCH,   DISPATCH_METHOD },

    {L"implementation",     DISPID_DOM_DOCUMENT_IMPLEMENTATION,      0,      NULL,                NULL,                     VT_DISPATCH,   DISPATCH_PROPERTYGET },

    {L"load",               DISPID_XMLDOM_DOCUMENT_LOAD,             1,      s_argTypes_DOMDocument_load,       NULL,       VT_BOOL,       DISPATCH_METHOD },
    {L"loadXML",            DISPID_XMLDOM_DOCUMENT_LOADXML,          1,      s_argTypes_DOMDocument_loadXML,    NULL,       VT_BOOL,       DISPATCH_METHOD },
    
    {L"nodeFromID",         DISPID_XMLDOM_DOCUMENT_NODEFROMID,       1,      s_argTypes_DOMDocument_nodeFromID, NULL,       VT_DISPATCH,   DISPATCH_METHOD },

    {L"ondataavailable",    DISPID_XMLDOM_DOCUMENT_ONDATAAVAILABLE,  1,      s_argTypes_DOMDocument_ondataavailable, NULL, VT_ERROR,    DISPATCH_PROPERTYPUT },
    {L"onreadystatechange", DISPID_XMLDOM_DOCUMENT_ONREADYSTATECHANGE, 1,    s_argTypes_DOMDocument_onreadystatechange, NULL, VT_ERROR,    DISPATCH_PROPERTYPUT },
    {L"ontransformnode",    DISPID_XMLDOM_DOCUMENT_ONTRANSFORMNODE,  1,      s_argTypes_DOMDocument_ontransformnode, NULL, VT_ERROR,    DISPATCH_PROPERTYPUT },
        
    {L"parseError",         DISPID_XMLDOM_DOCUMENT_PARSEERROR,       0,      NULL,                NULL,                     VT_DISPATCH,   DISPATCH_PROPERTYGET },
    {L"preserveWhiteSpace", DISPID_XMLDOM_DOCUMENT_PRESERVEWHITESPACE, 1,    s_argTypes_DOMDocument_preserveWhiteSpace, NULL, VT_BOOL,     DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },

    {L"readyState",         DISPID_READYSTATE,                       0,      NULL,                              NULL,       VT_I4,         DISPATCH_PROPERTYGET }, 
    {L"resolveExternals",   DISPID_XMLDOM_DOCUMENT_RESOLVENAMESPACE, 1,      s_argTypes_DOMDocument_resolveExternals, NULL, VT_BOOL,       DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },

    {L"save",               DISPID_XMLDOM_DOCUMENT_SAVE,             1,      s_argTypes_DOMDocument_save,       NULL,       VT_ERROR,      DISPATCH_METHOD },

    {L"url",                DISPID_XMLDOM_DOCUMENT_URL,              0,      NULL,                NULL,                     VT_BSTR,       DISPATCH_PROPERTYGET },

    {L"validateOnParse",    DISPID_XMLDOM_DOCUMENT_VALIDATE,         1,      s_argTypes_DOMDocument_validateOnParse,  NULL,  VT_BOOL,       DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },
};


//
// Map dispid to corresponding index in s_rgDOMDocumentMethods[]
//
static DISPIDTOINDEX 
s_DOMDocument_DispIdMap[] =
{
    { DISPID_READYSTATE, 23},
    { DISPID_DOM_DOCUMENT_DOCTYPE, 11 },
    { DISPID_DOM_DOCUMENT_IMPLEMENTATION, 14 },
    { DISPID_DOM_DOCUMENT_DOCUMENTELEMENT, 12 },
    { DISPID_DOM_DOCUMENT_CREATEELEMENT, 6 },
    { DISPID_DOM_DOCUMENT_CREATEDOCUMENTFRAGMENT, 5 }, 
    { DISPID_DOM_DOCUMENT_CREATETEXTNODE, 10 },
    { DISPID_DOM_DOCUMENT_CREATECOMMENT, 4 },
    { DISPID_DOM_DOCUMENT_CREATECDATASECTION, 3 },
    { DISPID_DOM_DOCUMENT_CREATEPROCESSINGINSTRUCTION, 9 },
    { DISPID_DOM_DOCUMENT_CREATEATTRIBUTE, 2 },
    { DISPID_DOM_DOCUMENT_CREATEENTITYREFERENCE, 7 },
    { DISPID_DOM_DOCUMENT_GETELEMENTSBYTAGNAME, 13 },
    { DISPID_XMLDOM_DOCUMENT_CREATENODE, 8 },
    { DISPID_XMLDOM_DOCUMENT_NODEFROMID, 17 },
    { DISPID_XMLDOM_DOCUMENT_LOAD, 15 },
    { DISPID_XMLDOM_DOCUMENT_PARSEERROR, 21 },
    { DISPID_XMLDOM_DOCUMENT_URL, 26 },
    { DISPID_XMLDOM_DOCUMENT_ASYNC, 1 },
    { DISPID_XMLDOM_DOCUMENT_ABORT, 0 },
    { DISPID_XMLDOM_DOCUMENT_LOADXML, 16 },
    { DISPID_XMLDOM_DOCUMENT_SAVE, 25 },
    { DISPID_XMLDOM_DOCUMENT_VALIDATE, 27 },
    { DISPID_XMLDOM_DOCUMENT_RESOLVENAMESPACE, 24 },
    { DISPID_XMLDOM_DOCUMENT_PRESERVEWHITESPACE, 22 },
    { DISPID_XMLDOM_DOCUMENT_ONREADYSTATECHANGE, 19 },
    { DISPID_XMLDOM_DOCUMENT_ONDATAAVAILABLE, 18 },
    { DISPID_XMLDOM_DOCUMENT_ONTRANSFORMNODE, 20 }
};


DISPATCHINFO _dispatchexport<Document, IXMLDOMDocument, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDOMDocument>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDOMDocument, &LIBID_MSXML, ORD_MSXML, s_rgDOMDocumentMethods, NUMELEM(s_rgDOMDocumentMethods), s_DOMDocument_DispIdMap, NUMELEM(s_DOMDocument_DispIdMap),  DOMDocumentWrapper::_invoke
};

///////////////////////////////////////////////////////////////////////////////
// DOMDocumentWrapper
//

DOMDocumentWrapper::DOMDocumentWrapper(Document * p) : _dispatchexport<Document, IXMLDOMDocument, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDOMDocument>(p), DOMNode(p->getDocNode())
{
    // This must be here to correctly initialize s_dispatchinfo under Unix
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::QueryInterface(REFIID iid, void ** ppvObject)
{
    STACK_ENTRY_WRAPPED;

    HRESULT hr = S_OK;
    TraceTag((tagDOMOM, "DOMDocumentWrapper::QueryInterface"));

    hr = getNodeData()->QIHelper( this, (DOMNode *)this, NULL, NULL, iid, ppvObject);

    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::GetIDsOfNames( 
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;
    Assert(GetComponentCount());

    // document methods?
    hr = _dispatchImpl::FindIdsOfNames( rgszNames, cNames, s_rgDOMDocumentMethods,
                                        NUMELEM(s_rgDOMDocumentMethods), lcid, rgDispId, false);
    // IXMLDOMNode methods?
    if (DISP_E_UNKNOWNNAME == hr)
    {
        hr = _dispatchImpl::FindIdsOfNames( rgszNames, cNames, s_rgDOMNodeMethods, 
                                            s_cDOMNodeMethodLen, lcid, rgDispId, false);
    }
    
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::Invoke( 
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;
    Assert(GetComponentCount());

    if (dispIdMember > DISPID_DOM_NODE && dispIdMember < DISPID_XMLDOM_NODE__TOP) // IXMLDOMNode method?
    {
        hr = DOMNode::Invoke(dispIdMember, riid, lcid, wFlags, 
                             pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    else 
    {
        hr = _dispatchImpl::Invoke(&_dispatchexport<Document, IXMLDOMDocument, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDOMDocument>::s_dispatchinfo,
                                   (IXMLDOMDocument*)this, dispIdMember, riid, lcid, wFlags, pDispParams,
                                   pVarResult, pExcepInfo, puArgErr);
    }
    return hr;
}


HRESULT
DOMDocumentWrapper::_invoke(
    void * pTarget,
    DISPID dispIdMember,
    INVOKE_ARG *rgArgs, 
    WORD wInvokeType,
    VARIANT *pVarResult,
    UINT cArgs)
{
    HRESULT hr;
    IXMLDOMDocument * pObj = (IXMLDOMDocument*)pTarget;

    TEST_METHOD_TABLE(s_rgDOMDocumentMethods, NUMELEM(s_rgDOMDocumentMethods), s_DOMDocument_DispIdMap, NUMELEM(s_DOMDocument_DispIdMap));

    switch( dispIdMember )
    {
        case DISPID_READYSTATE:
            hr = pObj->get_readyState((long*)&pVarResult->lVal);
            break;

        case DISPID_DOM_DOCUMENT_DOCTYPE:
            hr = pObj->get_doctype((IXMLDOMDocumentType**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_DOCUMENT_IMPLEMENTATION:
            hr = pObj->get_implementation((IXMLDOMImplementation**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_DOCUMENT_DOCUMENTELEMENT:
            if ( DISPATCH_PROPERTYGET == wInvokeType )
                hr = pObj->get_documentElement( (IXMLDOMElement**) &pVarResult->pdispVal );
            else
                hr = pObj->putref_documentElement( (IXMLDOMElement*) VARMEMBER(&rgArgs[0].vArg, pdispVal) );
            break;

        case DISPID_DOM_DOCUMENT_CREATEELEMENT:
            METHOD_INVOKE_1( pObj, createElement, bstrVal, BSTR, IXMLDOMElement*);
            break;

        case DISPID_DOM_DOCUMENT_CREATEDOCUMENTFRAGMENT:
            hr = pObj->createDocumentFragment((IXMLDOMDocumentFragment**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_DOCUMENT_CREATETEXTNODE:
            METHOD_INVOKE_1( pObj, createTextNode, bstrVal, BSTR, IXMLDOMText*);
            break;

        case DISPID_DOM_DOCUMENT_CREATECOMMENT:
            METHOD_INVOKE_1( pObj, createComment, bstrVal, BSTR, IXMLDOMComment*);
            break;

        case DISPID_DOM_DOCUMENT_CREATECDATASECTION:
            METHOD_INVOKE_1( pObj, createCDATASection, bstrVal, BSTR, IXMLDOMCDATASection*);
            break;

        case DISPID_DOM_DOCUMENT_CREATEPROCESSINGINSTRUCTION:
            hr = pObj->createProcessingInstruction((BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                                             (BSTR) VARMEMBER( &rgArgs[1].vArg, bstrVal),
                                             (IXMLDOMProcessingInstruction**) &pVarResult->pdispVal);
            break;
            
        case DISPID_DOM_DOCUMENT_CREATEATTRIBUTE:
            METHOD_INVOKE_1( pObj, createAttribute, bstrVal, BSTR, IXMLDOMAttribute*);
            break;

        case DISPID_DOM_DOCUMENT_CREATEENTITYREFERENCE:
            METHOD_INVOKE_1( pObj, createEntityReference, bstrVal, BSTR, IXMLDOMEntityReference*);
            break;

        case DISPID_DOM_DOCUMENT_GETELEMENTSBYTAGNAME:
            hr = pObj->getElementsByTagName((BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                                     (IXMLDOMNodeList**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_DOCUMENT_CREATENODE:
            hr = pObj->createNode( rgArgs[0].vArg,
                            (BSTR) VARMEMBER( &rgArgs[1].vArg, bstrVal),
                            (BSTR) VARMEMBER( &rgArgs[2].vArg, bstrVal),
                            (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_DOCUMENT_NODEFROMID:
            hr = pObj->nodeFromID((BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                            (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_DOCUMENT_LOAD:
            hr = pObj->load( rgArgs[0].vArg, (VARIANT_BOOL*) &pVarResult->boolVal );
            break;

        case DISPID_XMLDOM_DOCUMENT_PARSEERROR:
            hr = pObj->get_parseError((IXMLDOMParseError**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_DOCUMENT_URL:
            hr = pObj->get_url(&pVarResult->bstrVal);
            break;

        case DISPID_XMLDOM_DOCUMENT_ASYNC:
            PROPERTY_INVOKE_READWRITE( pObj, async, boolVal, VARIANT_BOOL);
            break;

        case DISPID_XMLDOM_DOCUMENT_ABORT:
            hr = pObj->abort();
            break;

        case DISPID_XMLDOM_DOCUMENT_LOADXML:
            hr = pObj->loadXML( (BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                          &pVarResult->boolVal);
            break;

        case DISPID_XMLDOM_DOCUMENT_SAVE:
            hr = pObj->save(rgArgs[0].vArg);
            break;

        case DISPID_XMLDOM_DOCUMENT_VALIDATE:
            PROPERTY_INVOKE_READWRITE( pObj, validateOnParse, boolVal, VARIANT_BOOL);
            break;

        case DISPID_XMLDOM_DOCUMENT_RESOLVENAMESPACE:
            PROPERTY_INVOKE_READWRITE( pObj, resolveExternals, boolVal, VARIANT_BOOL);
            break;

        case DISPID_XMLDOM_DOCUMENT_PRESERVEWHITESPACE:
            PROPERTY_INVOKE_READWRITE( pObj, preserveWhiteSpace, boolVal, VARIANT_BOOL);
            break;

        case DISPID_XMLDOM_DOCUMENT_ONREADYSTATECHANGE:
            hr = pObj->put_onreadystatechange(rgArgs[0].vArg);
            break;

        case DISPID_XMLDOM_DOCUMENT_ONDATAAVAILABLE:
            hr = pObj->put_ondataavailable(rgArgs[0].vArg);
            break;

        case DISPID_XMLDOM_DOCUMENT_ONTRANSFORMNODE:
            hr = pObj->put_ontransformnode(rgArgs[0].vArg);
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }
    
    return hr;
}

HRESULT STDMETHODCALLTYPE
DOMDocumentWrapper::GetDispID( 
    /* [in] */ BSTR bstrName,
    /* [in] */ DWORD grfdex,
    /* [out] */ DISPID __RPC_FAR *pid)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;
    Assert(GetComponentCount());
     
    // Our typelibs should never be localized, so it's safe to assume a LCID of 0x409

    // IXMLDOMDocument method?
    hr = _dispatchImpl::FindIdsOfNames( &bstrName, 1, s_rgDOMDocumentMethods, NUMELEM(s_rgDOMDocumentMethods), 
                                        0x409, pid, grfdex & fdexNameCaseSensitive);
    // IXMLDOMNode method?
    if ( DISP_E_UNKNOWNNAME == hr )
    {
        hr = _dispatchImpl::FindIdsOfNames( &bstrName, 1, s_rgDOMNodeMethods, s_cDOMNodeMethodLen, 
                                            0x409, pid, grfdex & fdexNameCaseSensitive);
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::cloneNode( 
    /* [in] */ VARIANT_BOOL deep,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapper::cloneNode()"));
    HRESULT hr = S_OK;

    if (!ppCloneRoot)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    TRY
    {
        // clone whole document.
        Document * doc = getWrapped()->clone(deep == VARIANT_TRUE);
        *ppCloneRoot = doc->getDocNode()->getDOMNodeWrapper();
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
DOMDocumentWrapper::get_doctype( 
    /* [retval][out] */ IXMLDOMDocumentType __RPC_FAR *__RPC_FAR * ppDocumentType)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::get_doctype()"));
    HRESULT hr = S_OK;

    CHECK_ARG( ppDocumentType);
    *ppDocumentType = null;

    TRY
    {
        Node * docNode = getNodeData();
        Node * pDocTypeNode = docNode->find(null, Node::DOCTYPE);
        if ( pDocTypeNode)
        {
            hr = pDocTypeNode->QueryInterface(IID_IXMLDOMDocumentType, (void**)ppDocumentType);
            goto Cleanup;
        }

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
DOMDocumentWrapper::get_implementation( 
        /* [retval][out] */ IXMLDOMImplementation __RPC_FAR *__RPC_FAR *ppImpl)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::get_implementation()"));

    HRESULT hr;
    ARGTEST(ppImpl);

    TRY 
    {
        *ppImpl = (IXMLDOMImplementation*)new DOMImplementation();
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
DOMDocumentWrapper::putref_documentElement( 
        /* [in] */ IXMLDOMElement __RPC_FAR * pDOMElement)
{
    STACK_ENTRY;
    OMWRITELOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::putref_documentElement()"));

    HRESULT hr;

    ARGTEST(pDOMElement);

    TRY
    {
        Node * pNode = NULL;
        if (!pDOMElement->QueryInterface(Node::s_IID, (void**)&pNode) && pNode)
        {
            getWrapped()->setRoot(pNode);
            hr = S_OK;
        }
        else
        {
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

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::get_documentElement( 
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR * ppDOMElement)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::get_documentElement()"));
    HRESULT hr;

    TRY
    {
        Node * docNode = getNodeData();
        Node * pDocElem = docNode->find(null, Node::ELEMENT);
        if (pDocElem)
        {
            hr = pDocElem->QueryInterface(IID_IXMLDOMElement, (void**)ppDOMElement);
        }
        else
        {
            *ppDOMElement = null;
            hr = S_FALSE;
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

static
HRESULT
CreateDOMNode( Document * pDoc, Element::NodeType eType, BSTR name, BSTR data, const IID * piid, void ** ppv)
{ 
    HRESULT hr;

    TRY
    {
        Node * pNode = pDoc->createNode( eType, name, null, false);
        if (data)
            pNode->setInnerText(data);
        hr = pNode->QueryInterface(*piid, ppv);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

    
HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createElement( 
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IXMLDOMElement __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createElement()"));

    HRESULT hr;

    CHECK_ARG( ppNewNode);
    CHECK_ARG( tagName);

    hr = CreateDOMNode( getWrapped(), Node::ELEMENT, tagName, NULL, &IID_IXMLDOMElement, (void **)ppNewNode);

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createAttribute( 
        /* [in] */ BSTR attrName,
        /* [retval][out] */ IXMLDOMAttribute __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createAttribute()"));

    HRESULT hr;
    DOMNode * pDOMNode = null;

    CHECK_ARG( ppNewNode);
    CHECK_ARG( attrName);

    hr = CreateDOMNode( getWrapped(), Node::ATTRIBUTE, attrName, NULL, &IID_IXMLDOMAttribute, (void **)ppNewNode);

Cleanup:
    return hr;
}

    
HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createDocumentFragment( 
        /* [retval][out] */ IXMLDOMDocumentFragment __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createDocumentFragment()"));

    HRESULT hr;
    DOMNode * pDOMNode = null;

    CHECK_ARG( ppNewNode);

    hr = CreateDOMNode( getWrapped(), Node::DOCFRAG, NULL, NULL, &IID_IXMLDOMDocumentFragment, (void **)ppNewNode);

Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createTextNode( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMText __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createTextNode()"));

    HRESULT hr;
    DOMNode * pDOMNode = null;

    CHECK_ARG( ppNewNode);

    hr = CreateDOMNode( getWrapped(), Node::PCDATA, NULL, data, &IID_IXMLDOMText, (void **)ppNewNode);

Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createComment( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMComment __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createComment()"));

    HRESULT hr;
    DOMNode * pDOMNode = null;

    CHECK_ARG( ppNewNode);

    hr = CreateDOMNode( getWrapped(), Node::COMMENT, NULL, data, &IID_IXMLDOMComment, (void **)ppNewNode);


Cleanup:
    return hr;

}
    
HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createCDATASection( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMCDATASection __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createCDATASection()"));

    HRESULT hr;
    DOMNode * pDOMNode = null;

    CHECK_ARG( ppNewNode);

    hr = CreateDOMNode( getWrapped(), Node::CDATA, NULL, data, &IID_IXMLDOMCDATASection, (void **)ppNewNode);

Cleanup:
    return hr;
}
 
HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createProcessingInstruction( 
        /* [in] */ BSTR target,
        /* [in] */ BSTR data,
        /* [retval][out] */ IXMLDOMProcessingInstruction __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createProcessingInstruction()"));

    HRESULT hr;
    DOMNode * pDOMNode = null;

    CHECK_ARG( ppNewNode);

    TRY
    {
        if (StrCmpW(target,L"xml") == 0)
        {
            Node* xmldecl = getWrapped()->parseXMLDecl(data);
            hr = xmldecl->QueryInterface(IID_IXMLDOMProcessingInstruction, (void**)ppNewNode);
        }
        else
            hr = CreateDOMNode( getWrapped(), Node::PI, target, data, &IID_IXMLDOMProcessingInstruction, (void **)ppNewNode);
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
DOMDocumentWrapper::createEntityReference( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMEntityReference __RPC_FAR *__RPC_FAR *ppNewNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createEntityReference()"));

    HRESULT hr;
    DOMNode * pDOMNode = null;

    CHECK_ARG( ppNewNode);

    hr = CreateDOMNode( getWrapped(), Node::ENTITYREF, name, NULL, &IID_IXMLDOMEntityReference, (void **)ppNewNode);

Cleanup:
    return hr;
}
    

/////////////////////////////////////////////////////////////////////////////////////////
// IXMLDOMDocument methods

static 
bool
mywcharcmp( const WCHAR * a, const WCHAR * b)
{
    while (*a && *b)
    {
        if (Character::toLowerCase((TCHAR)*a) != (TCHAR)*b)
            return false;
        a++;
        b++;
    }
    return (*a == *b);
}

struct CreateNodeTypeNamesInfo
{
    WCHAR * pwc;
    DOMNodeType eType;
} createNodeTypeNames[] = {
    L"document", NODE_DOCUMENT,
    L"element", NODE_ELEMENT,
    L"attribute", NODE_ATTRIBUTE,
    L"pi", NODE_PROCESSING_INSTRUCTION,
    L"processinginstruction", NODE_PROCESSING_INSTRUCTION,
    L"comment", NODE_COMMENT,
    L"text", NODE_TEXT,
    L"cdata", NODE_CDATA_SECTION,
    L"cdatasection", NODE_CDATA_SECTION,
    L"documentfragment", NODE_DOCUMENT_FRAGMENT,
    L"entity", NODE_ENTITY,
    L"entityreference", NODE_ENTITY_REFERENCE,
    L"documenttype", NODE_DOCUMENT_TYPE,
    L"notation", NODE_NOTATION,
    NULL, NODE_INVALID
};

Node::NodeType aXMLType2NT[] =
{
    // NODE_INVALID, // = 0
    Element::ANY,
    // NODE_ELEMENT, // = 1
    Element::ELEMENT,
    // NODE_ATTRIBUTE, // = 2
    Element::ATTRIBUTE,
    // NODE_TEXT, // = 3
    Element::PCDATA,
    // NODE_CDATA_SECTION, // = 4
    Element::CDATA,
    // NODE_ENTITY_REFERENCE, // = 5
    Element::ENTITYREF,
    // NODE_ENTITY, // = 6
    Element::ENTITY,
    // NODE_PROCESSING_INSTRUCTION, // = 7
    Element::PI,
    // NODE_COMMENT, // = 8
    Element::COMMENT,
    // NODE_DOCUMENT, // = 9
    Element::DOCUMENT,
    // NODE_DOCUMENT_TYPE, // = 10
    Element::DOCTYPE,
    // NODE_DOCUMENT_FRAGMENT, // = 11
    Element::DOCFRAG,
    // NODE_NOTATION // = 12
    Element::NOTATION,
};

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::createNode( 
    /* [in] */ VARIANT Type,
    /* [in] */ BSTR Text,
    /* [in] */ BSTR NameSpaceURI,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::createNode()"));

    VARIANT vType; vType.vt = VT_NULL;
    HRESULT hr = S_OK;
    DOMNodeType eType = NODE_INVALID;

    CHECK_ARG( ppNode);

    if (S_OK != VariantChangeType( &vType, &Type, VARIANT_NOVALUEPROP, VT_I4))
    {
        if (S_OK != VariantChangeTypeEx( &vType, &Type, GetThreadLocale(), VARIANT_NOVALUEPROP, VT_BSTR))
        {
            hr =  E_INVALIDARG;
            goto Cleanup;
        }
        // match text
        CreateNodeTypeNamesInfo * pInfo = createNodeTypeNames;
        while (pInfo->pwc)
        {
            if (mywcharcmp(V_BSTR(&vType), pInfo->pwc))
            {
                eType = pInfo->eType;
                break;
            }
            pInfo++;
        }
    }
    else
    {
        eType = (DOMNodeType)V_I4(&vType);
    }

    if ((NODE_INVALID < eType) && (eType <= NODE_NOTATION))
    {
        TRY
        {
            Node * pNode = getWrapped()->createNode( aXMLType2NT[eType], Text, NameSpaceURI, true);
            *ppNode = pNode->getDOMNodeWrapper();
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY
    }
    else
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:
    VariantClear(&vType);
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::nodeFromID( 
    /* [in] */ BSTR pbstrID,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    TraceTag((tagDOMOM, "DOMDocumentWrapped::nodeFromID()"));

    CHECK_ARG(pbstrID);
    CHECK_ARG(ppNode);
    HRESULT hr = S_OK;

    TRY 
    {
        Node* node = getWrapped()->nodeFromID(Name::create(String::newString(pbstrID)));
        IXMLDOMNode * pIDOMNode = null;
        if (node != NULL)
        {
            pIDOMNode = (IXMLDOMNode *)node->getDOMNodeWrapper();
        }
        *ppNode = pIDOMNode;
        if (pIDOMNode == null)
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
DOMDocumentWrapper::load( 
    /* [in] */ VARIANT vTarget,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful)
{
    // since there is no standard lock here we have to set up the model
    STACK_ENTRY_WRAPPED;

    TraceTag((tagDOMOM, "DOMDocumentWrapped::load(VARIANT)"));

//  Locking is taken care of in Document for this case.
//    OMREADLOCK(getWrapped());

    HRESULT hr = S_OK;
    VARIANT vURL; vURL.vt = VT_NULL;
    IStream* pStm = NULL;
    IPersistStream* pPS = NULL;
    IRequest* pRequest = NULL;
    IUnknown* pUnk = NULL;
    bool fLocked = false;

    ARGTEST(isSuccessful);

    TRY 
    { 
        if (vTarget.vt & VT_ARRAY)
        {
            if (vTarget.vt != (VT_ARRAY | VT_UI1))
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
            else
            {
                // wrap the safe array in an IStream and load the document from it.
                TraceTag((tagDOMOM, "   VARIANT=SAFEARRAY)"));
                pStm = (IStream *) new DocStream(V_ARRAY(&vTarget));
                getWrapped()->Load(pStm);
            }
        }
        else if (NULL != (pUnk = Variant::getUnknown(&vTarget)))
        {
            // Perhaps it is some object that supports IStream and we need to load the document
            // from that stream (like, say the ASP Request object :-)
            if (S_OK == pUnk->QueryInterface(IID_IStream, (void**)&pStm))
            {
                TraceTag((tagDOMOM, "   VARIANT=IStream)"));
                getWrapped()->Load(pStm);
            }
            else if ((S_OK == pUnk->QueryInterface(IID_IPersistStream, (void**)&pPS)) ||
                (S_OK == pUnk->QueryInterface(IID_IPersistStreamInit, (void**)&pPS)))
            {
                // IPersistStream identical to IPersistStreamInit except it does
                // not have the InitNew method at the end - to this is vtable compatible
                // which means it is ok to cast an IPersistStreamInit interface to
                // IPersistStream since we don't use the InitNew method.
                TraceTag((tagDOMOM, "   VARIANT=IPersistStream)"));
                pStm = (IStream *) new DocStream(getWrapped());
                // Save the persistable object into this document (hence loading this document)
                // The persistable object will call Write on our docstream.
                pPS->Save(pStm,FALSE); 
            }
            else if (S_OK == pUnk->QueryInterface(IID_IRequest, (void**)&pRequest))
            {
                // Create an IStream wrapper for the pRequest object so that
                // we can read from it.
                TraceTag((tagDOMOM, "   VARIANT=IRequest)"));
                pStm = (IStream *) new DocStream(pRequest);
                getWrapped()->Load(pStm);
            }
            else
            {
                hr =  E_INVALIDARG;
                goto Cleanup;
            }
        }
        else
        {
            // Try and convert it to a BSTR then and treat it like a URL.
            if (S_OK != VariantChangeTypeEx( &vURL, &vTarget, GetThreadLocale(), VARIANT_NOVALUEPROP, VT_BSTR)
                || NULL == V_BSTR(&vURL))
            {
                hr =  E_INVALIDARG;
                goto Cleanup;
            }

            TraceTag((tagDOMOM, "   VARIANT=BSTR)"));

            // enterDOMLock aborts the current loading if any;
            // and then grab a readlock when it is available.
            // We don't do this for the above IStream cases because
            // the document::Load(IStream) does it already.
            getWrapped()->enterDOMLoadLock();
            fLocked = true;
            String * url = String::newString(V_BSTR(&vURL));
            getWrapped()->load(url, getWrapped()->isAsync());
        }

        // garbage collecting
//        Base::checkZeroCountList();

        hr = getWrapped()->GetLastError();
    } 
    CATCH 
    { 
        hr = ERESULTINFO;
    }
    ENDTRY


    TRY
    {
        getWrapped()->setExitedLoad(true);
        if (fLocked) getWrapped()->leaveDOMLoadLock(hr);
    }
    CATCH
    {
        if (hr == S_OK)
        {
            hr = ERESULTINFO;
        }
    }
    ENDTRY

    // Only return errors if you want a JavaScript client to get a 
    // scripting error.
    if (FAILED(hr) && hr != E_ACCESSDENIED)
    {
        hr = S_FALSE;
    }
    
Cleanup:
    if (S_OK == hr)
        *isSuccessful = VARIANT_TRUE;
    else
        *isSuccessful = VARIANT_FALSE;
    VariantClear(&vURL);
    if (pStm)
        pStm->Release();
    if (pRequest)
        pRequest->Release();
    if (pPS)
        pPS->Release();
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::get_readyState( 
    /* [out][retval] */ long __RPC_FAR *plState)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::get_readyState()"));

    CHECK_ARG( plState);

    HRESULT hr = S_OK;

    TRY {         
        *plState = getWrapped()->getReadyStatus();
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY

CleanUp:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::get_parseError( 
    /* [out][retval] */ IXMLDOMParseError __RPC_FAR *__RPC_FAR *pError)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::get_parseError()"));

    CHECK_ARG(pError);
    HRESULT hr = S_OK;

    TRY 
    {
        *pError = (IXMLDOMParseError*)new DOMError(getWrapped()->getErrorMsg());
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
DOMDocumentWrapper::get_url( 
    /* [out][retval] */ BSTR __RPC_FAR *pbstrUrl)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::get_url()"));

    CHECK_ARG( pbstrUrl);
    HRESULT hr = S_OK;

    TRY 
    { 
        String * pUrl = getWrapped()->getURL();
        if (pUrl != null)
            *pbstrUrl = pUrl->getBSTR(); 
        else 
        {
            *pbstrUrl = null;
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
DOMDocumentWrapper::get_async( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pf)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::get_async()"));

    CHECK_ARG( pf);

    if (getWrapped()->isAsync())
        *pf = VARIANT_TRUE;
    else
        *pf = VARIANT_FALSE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::put_async( 
    /* [in] */ VARIANT_BOOL f)
{
    STACK_ENTRY;
    OMWRITELOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::put_async()"));

    getWrapped()->setAsync(f != VARIANT_FALSE);

Cleanup:
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::abort( void)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    TraceTag((tagDOMOM, "DOMDocumentWrapped::abort()"));

    // Don't grab any lock here, it's handled in Document
    TRY
    {
        getWrapped()->abort(Exception::newException(XMLOM_USERABORT,
                            XMLOM_USERABORT, null));
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
DOMDocumentWrapper::loadXML( 
        /* [in] */ BSTR bstrXML,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *isSuccessful)
{
    // since there is no standard lock here we have to set up the model
    STACK_ENTRY_WRAPPED;

//  Locking is taken care of in Document for this case.
//    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapped::loadXML()"));

    HRESULT hr = S_OK;
    ARGTEST(isSuccessful);

    TRY 
    {
        // enterDOMLock aborts the current loading if any;
        // and then grab a readlock when it is available
        getWrapped()->enterDOMLoadLock();

        String * s = String::newString(bstrXML); // newString now handles NULL.
        getWrapped()->loadXML(s);
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY

    TRY
    {
        getWrapped()->setExitedLoad(true);     
        getWrapped()->leaveDOMLoadLock(hr);
    }
    CATCH
    {
        if (hr == S_OK)
        {
            hr = ERESULTINFO;
        }
    }
    ENDTRY

    // Only return errors if you want a JavaScript client to get a 
    // scripting error.
    if (FAILED(hr) && hr != E_ACCESSDENIED)
        hr = S_FALSE;

Cleanup:
    if (S_OK == hr)
        *isSuccessful = VARIANT_TRUE;
    else
        *isSuccessful = VARIANT_FALSE;
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::save( 
        /* [in] */ VARIANT vTarget)
{
    STACK_ENTRY_WRAPPED;
    VARIANT vSrc; vSrc.vt = VT_NULL;
    HRESULT hr = S_OK;
    IStream* pStm = NULL;
    IResponse* pResponse = NULL;
    IPersistStream* pPS = NULL;
    DWORD dwOptions, dwEnabled;

    TRY
    {
        IUnknown* pUnk = Variant::getUnknown(&vTarget);
        if (NULL != pUnk)
        {
            // Perhaps it is some object that supports IStream and we need to save the document
            // to that stream (like, say the ASP Response object :-)
            // NOTE: no security needed here since you cannot script into this code path
            // without the help of some other ActiveX control.
            if (S_OK == pUnk->QueryInterface(IID_IStream, (void**)&pStm))
            {
                getWrapped()->Save(pStm);
            }
            else if ((S_OK == pUnk->QueryInterface(IID_IPersistStream, (void**)&pPS)) ||
                (S_OK == pUnk->QueryInterface(IID_IPersistStreamInit, (void**)&pPS)))
            {
                // IPersistStream identical to IPersistStreamInit except it does
                // not have the InitNew method at the end - to this is vtable compatible
                // which means it is ok to cast an IPersistStreamInit interface to
                // IPersistStream since we don't use the InitNew method.
                TraceTag((tagDOMOM, "   VARIANT=IPersistStream)"));
                pStm = (IStream *) new DocStream(getWrapped());
                // load the persistable object from this document (hence saving this document)
                // the persistable object will call Read on our docstream.
                pPS->Load(pStm); 
            }
            else if (S_OK == pUnk->QueryInterface(IID_IResponse, (void**)&pResponse))
            {
                // Create an IStream wrapper for the pResponse object so that
                // every buffer full of XML that we generate we send directly
                // down the wire !!
                pStm = (IStream *) new DocStream(pResponse);
                getWrapped()->Save(pStm);
            }
            else
            {
                hr = E_INVALIDARG;
                goto CleanUp;
            }
        }
        else 
        {
            if (S_OK != VariantChangeTypeEx( &vSrc, &vTarget, GetThreadLocale(), VARIANT_NOVALUEPROP, VT_BSTR)
                || NULL == V_BSTR(&vSrc))
            {
                hr =  E_INVALIDARG;
                goto CleanUp;
            }
            if (getWrapped()->isSecure())
            {
                hr = E_ACCESSDENIED;        // cannot write to disk from inside browser.
                goto CleanUp;
            }
            getWrapped()->save(String::newString(V_BSTR(&vSrc)),null);
        }
        // Force stream to cleanup and catch any last buffer errors (like the empty document case).
        if (pStm)
        {
            pStm->Release();
            pStm = NULL;
            Exception* e = getWrapped()->getErrorMsg();
            if (e) e->throwE(); // so we set the error info properly.
        }
    }
    CATCH
    {
        hr = ERESULTINFO; 
    }
    ENDTRY

CleanUp:
    VariantClear(&vSrc);
    if (pStm)
        pStm->Release();
    if (pPS)
        pPS->Release();
    if (pResponse)
        pResponse->Release();
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::get_validateOnParse( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isValidating)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapper::get_validateOnParse()"));

    HRESULT hr = S_OK;

    TRY
    {
        if ( getWrapped()->getValidateOnParse())
            *isValidating = VARIANT_TRUE;
        else
            *isValidating = VARIANT_FALSE;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY;    

Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::put_validateOnParse( 
        /* [in] */ VARIANT_BOOL isValidating)
{
    STACK_ENTRY;
    OMWRITELOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapper::put_validateOnParse()"));

    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->setValidateOnParse(isValidating == VARIANT_TRUE);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY;    

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::get_resolveExternals( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfResolve)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapper::get_resolveExternals()"));

    HRESULT hr = S_OK;

    TRY
    {
        if ( getWrapped()->getResolveExternals())
            *pfResolve = VARIANT_TRUE;
        else
            *pfResolve = VARIANT_FALSE;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY;    

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::put_resolveExternals( 
    /* [in] */ VARIANT_BOOL fResolve)
{
    STACK_ENTRY;
    OMWRITELOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapper::put_resolveExternals"));

    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->setResolveExternals(fResolve == VARIANT_TRUE);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY;    

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::get_preserveWhiteSpace( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pfPreserve)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapper::get_preserveWhiteSpace()"));

    HRESULT hr = S_OK;

    TRY
    {
        if (getWrapped()->getPreserveWhiteSpace())
            *pfPreserve = VARIANT_TRUE;
        else
            *pfPreserve = VARIANT_FALSE;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY;    

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::put_preserveWhiteSpace( 
    /* [in] */ VARIANT_BOOL fPreserve)
{
    STACK_ENTRY;
    OMWRITELOCK(getWrapped());

    TraceTag((tagDOMOM, "DOMDocumentWrapper::get_preserveWhiteSpace()"));

    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->setPreserveWhiteSpace(fPreserve == VARIANT_TRUE);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY;    

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::put_onreadystatechange( 
    /* [in] */ VARIANT varFn)
{
    STACK_ENTRY_WRAPPED;
    TraceTag((tagDOMOM, "DOMDocumentWrapped::put_onreadystatechange()"));

    VARIANT varTemp;
    HRESULT hr;

    VariantInit(&varTemp);

    if (SUCCEEDED(hr = VariantChangeType(&varTemp, &varFn, VARIANT_NOVALUEPROP, VT_DISPATCH)))
    {
        hr = getWrapped()->putOnReadyStateChange(varTemp.pdispVal);
    }

Cleanup:
    VariantClear(&varTemp);
    return hr;
}
  

HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::put_ondataavailable( 
    /* [in] */ VARIANT varFn)
{
    STACK_ENTRY_WRAPPED;
    TraceTag((tagDOMOM, "DOMDocumentWrapped::put_ondataavailable()"));

    VARIANT varTemp;
    HRESULT hr;

    VariantInit(&varTemp);

    if (SUCCEEDED(hr = VariantChangeType(&varTemp, &varFn, VARIANT_NOVALUEPROP, VT_DISPATCH)))
    {
        hr = getWrapped()->putOnDataAvailable(varFn.pdispVal);
    }

Cleanup:
    VariantClear(&varTemp);
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMDocumentWrapper::put_ontransformnode( 
    /* [in] */ VARIANT varFn)
{
    STACK_ENTRY_WRAPPED;
    TraceTag((tagDOMOM, "DOMDocumentWrapped::put_ontransformnode()"));

    VARIANT varTemp;
    HRESULT hr;

    VariantInit(&varTemp);

    if (SUCCEEDED(hr = VariantChangeType(&varTemp, &varFn, VARIANT_NOVALUEPROP, VT_DISPATCH)))
    {
        hr = getWrapped()->putOnTransformNode(varFn.pdispVal);
    }

Cleanup:
    VariantClear(&varTemp);
    return hr;
}


/*****************************************************************
// SAFEARRAY stuff, just in case we need to resurrect it one day.

    HRESULT hr = S_OK;
	IPersistStreamInit* pPSI = NULL;
    HGLOBAL hGlobal;
    DWORD dwSize;
    SAFEARRAY * psa = NULL;
    SAFEARRAYBOUND rgsabound[1];	
    BYTE* pBytes;
    IStream* pStm = NULL;
    STATSTG statStg;

    if (!pvData)
        return E_INVALIDARG;
    else
        VariantInit(pvData);

    TRY
    {
        checkhr(getWrapped()->QueryInterface(IID_IPersistStreamInit, (void **)&pPSI) );

        checkhr(CreateStreamOnHGlobal(NULL, TRUE, &pStm));
        checkhr(pPSI->Save(pStm, TRUE));

        checkhr(GetHGlobalFromStream(pStm, &hGlobal));

        pStm->Stat(&statStg, STATFLAG_NONAME);
        dwSize = statStg.cbSize.QuadPart;

        if (dwSize == 0)
        {
            VariantClear(pvData);
        }
        else
        {
            const BYTE* pStmData = *((const BYTE**)hGlobal);
            // use our CreateVectory method (in core\util\datatypes.cxx)
            checkhr(CreateVector(pvData, pStmData, (LONG)dwSize));
        }
    }
    CATCH
    {
        VariantClear(pvData);
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    if (pPSI)
        pPSI->Release();
	if (pStm)
        pStm->Release();

    return hr;
}
****************************************************************/

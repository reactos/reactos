/*
 * @(#)msxmlCOM.cxx 1.0 6/3/97
 * 
 * Implementation of msxml2.idl interfaces.
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#ifndef _XML_OM_MSXMLCOM
#include "msxmlcom.hxx"
#endif

#ifndef __XMLDOM_HXX
#include "xmldom.hxx"
#endif

#ifndef _XML_OM_OMLOCK
#include "xml/om/omlock.hxx"
#endif

#include "xml/om/eventhelp.hxx"
#include "xml/om/provideclassinfo.hxx"
#include "xml/om/docstream.hxx"

class OutputHelper; 

#define CHECKHR(x) {hr = x; goto Cleanup;}

DeclareTag(tagIE4OM, "MSXMLCOM", "IE4 Object Model");


// #include "msxml2_i.c" -- this is now in uuid.lib

DISPATCHINFO _dispatchexport<Document, IXMLDocument2, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDocument2>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDocument2, &LIBID_MSXML, ORD_MSXML
};

// this lock is used for elements outside of a document...
static SRCSMutex s_pMutex;

ElementLock::ElementLock(Element * pElem) : MutexLock(pElem->getDocument() ? 
                                                      pElem->getDocument()->getMutex() : 
                                                      s_pMutex)
{
}

void OMInit()
{
    s_pMutex = CSMutex::newCSMutex();
    // starts with refcount 1
    s_pMutex->Release();
}


IDocumentWrapper::IDocumentWrapper(Document * p):_dispatchexport<Document, IXMLDocument2, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDocument2>(p)
{
    // This must be here to correctly initialize s_dispatchinfo under Unix
}


/**
 * Document::QueryInterface
 */
HRESULT Document::QueryInterface(REFIID iid, void ** ppvObject)
{
    STACK_ENTRY_OBJECT(this);
    HRESULT hr = S_OK;
    TraceTag((tagIE4OM, "Document::QueryInterface"));

    Assert(getDocNode());
    hr = getDocNode()->QIHelper( NULL, NULL, NULL, NULL, iid, ppvObject);

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IDocumentWrapper::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    HRESULT hr = S_OK;
    TraceTag((tagIE4OM, "IDocumentWrapper::QueryInterface"));

    Assert(getWrapped()->getDocNode());
    hr = getWrapped()->getDocNode()->QIHelper( NULL, NULL, this, NULL, iid, ppv);

    return hr;
}

ULONG STDMETHODCALLTYPE IDocumentWrapper::Release()
{
    STACK_ENTRY;
    OMREADLOCK(getWrapped());

    ULONG r = _dispatchexport<Document, IXMLDocument2, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDocument2>::Release();
    if (r == 0)
    {
        // This is a good time to do some garbage collecting because
        // we probably just released a big tree of nodes.
//        Base::checkZeroCountList();
    }

    return r;
}

/**
 * COM object model implementation.
 */

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_root( 
    /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *p)
{
    CHECK_ARG_INIT(p);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
// ACTUALLY - CDFVIEW depends on us returning the root node before we're
// finished.  This is a compatibility bug fix.  It is safe because
// they call us on the same thread.
//        if (getWrapped()->getReadyStatus() == READYSTATE_LOADING)
//        {
//            TraceTag((tagIE4OM, "IDocumentWrapper::get_root - failed"));
//            return E_PENDING;
//        }
//        else 

        if (getWrapped()->getErrorMsg() != null)
        {
            TraceTag((tagIE4OM, "IDocumentWrapper::get_root - failed"));
            hr = getWrapped()->getErrorMsg()->getHRESULT();
            if (!FAILED(hr)) 
                hr = E_FAIL;
            goto Cleanup;
        }

        // This is a good time to do some garbage collecting because
        // we probably just finished downloading and there is a bunch
        // of stuff in the zero count list - including the parser, encoder, etc.
//        Base::checkZeroCountList();

        Element* root = getWrapped()->getRoot();
        if (!root)
        {
            // unknown reason.  perhaps put_url hasn't been called yet.
            TraceTag((tagIE4OM, "IDocumentWrapper::get_root - failed"));
            *p = null;
            // NOTE: MUST RETURN E_FAIL HERE BECAUSE SOME CLIENT CODE
            // BLINDLY DEREFERENCES THE REUTRNED POINTER IF HR == S_OK.
            hr = E_FAIL;
            goto Cleanup;
        }

        TraceTag((tagIE4OM, "IDocumentWrapper::get_root - ok"));
        Assert( CHECKTYPEID(*root, ElementNode));
        *p = new IElementWrapper(CAST_TO(ElementNode *, root));
    }
    CATCH
    {
        hr = S_FALSE; // so script can code around it.
        goto Cleanup;
    }
    ENDTRY

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE IDocumentWrapper::put_root( 
    /* [in] */ IXMLElement2 __RPC_FAR *p)
{
    if (!p)
        return E_INVALIDARG;

    STACK_ENTRY;
    OMWRITELOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        Node * e = NULL;
        if (!p->QueryInterface(Node::s_IID, (void**)&e) && e)
        {
            Document* doc = getWrapped();
            doc->setRoot(e);
            TraceTag((tagIE4OM, "IDocumentWrapper::put_root - ok"));
        }
        else
        {
            TraceTag((tagIE4OM, "IDocumentWrapper::put_root - failed"));
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


HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_fileSize( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    TraceTag((tagIE4OM, "IDocumentWrapper::get_fileSize: NOTIMPL"));
    // BUGBUG later... (not implemented in old msxml)
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_fileModifiedDate( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    TraceTag((tagIE4OM, "IDocumentWrapper::get_fileModifiedDate: NOTIMPL"));
    // BUGBUG later... (not implemented in old msxml)
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_fileUpdatedDate( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    TraceTag((tagIE4OM, "IDocumentWrapper::get_fileUpdatedDate: NOTIMPL"));
    // BUGBUG later... (not implemented in old msxml)
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_URL( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    CHECK_ARG_INIT(p);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        String* s = getWrapped()->getURL();
        if (s == null) s = String::emptyString();
        TraceTag((tagIE4OM, "IDocumentWrapper::get_URL: %s",  (char *)AsciiText(s) ));
        *p = s->getBSTR();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::put_URL( 
    /* [in] */ BSTR p)
{
    STACK_ENTRY;
    OMWRITELOCK(getWrapped());

    HRESULT hr = S_OK;
    TRY
    {
        String * s = String::newString(p);
        TraceTag((tagIE4OM, "IDocumentWrapper::put_URL: %s",  (char *)AsciiText(s) ));
        getWrapped()->load(s, getWrapped()->isAsync());

//      garbage collecting
//      Base::checkZeroCountList();       // moved to Document::run

        hr = getWrapped()->GetLastError();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

// IE4 doesn't do this
    // Only return errors if you want a JavaScript client to get a 
    // scripting error.
//    if (FAILED(hr) && hr != E_ACCESSDENIED)
//        hr = S_FALSE;

    return hr;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_mimeType( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    TraceTag((tagIE4OM, "IDocumentWrapper::get_mimeType: NOTIMPL"));
    // BUGBUG later... (not implemented in old msxml)
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_readyState( 
    /* [out][retval] */ long __RPC_FAR *pl)
{
    CHECK_ARG_INIT(pl);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    long i = getWrapped()->getReadyStatus();
    *pl = i;
    TraceTag((tagIE4OM, "IDocumentWrapper::get_readyState: %i", i));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_charset( 
    /* [out][retval] */ BSTR __RPC_FAR *pEncoding)
{
    CHECK_ARG_INIT(pEncoding);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        String* encoding = getWrapped()->getEncoding();
        if (!encoding)
            encoding = String::newString(_T("UTF-8"));

        TraceTag((tagIE4OM, "IDocumentWrapper::get_charset: %s", (char *)AsciiText(encoding)));
        *pEncoding = encoding->getBSTR();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::put_charset( 
    /* [in] */ BSTR pEncoding)
{
    if (!pEncoding)
        return E_INVALIDARG;

    STACK_ENTRY;
    OMWRITELOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        String* encoding = String::newString(pEncoding);
        TraceTag((tagIE4OM, "IDocumentWrapper::put_charset: %s", (char *)AsciiText(encoding)));
        getWrapped()->setEncoding(encoding);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_version( 
    /* [out][retval] */ BSTR __RPC_FAR *pVersion)
{
    CHECK_ARG_INIT(pVersion);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        String* version = getWrapped()->getVersion();
        TraceTag((tagIE4OM, "IDocumentWrapper::get_version: %s", (char *)AsciiText(version)));
        *pVersion = version->getBSTR();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_doctype( 
    /* [out][retval] */ BSTR __RPC_FAR *pDoctype)
{
    CHECK_ARG_INIT(pDoctype);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        NameDef* doctype = getWrapped()->getDocType();
        TraceTag((tagIE4OM, "IDocumentWrapper::get_doctype: %s", (char *)AsciiText(doctype)));

        if (doctype)
        {
            *pDoctype = doctype->toString()->getBSTR();
        }
        else
        {
            *pDoctype = NULL;
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

HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_dtdURL( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    TraceTag((tagIE4OM, "IDocumentWrapper::get_dtdURL: NOTIMPL"));

    // BUGBUG later... (not implemented in old msxml)
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDocumentWrapper::createElement( 
    /* [in] */ VARIANT vType,
    /* [in][optional] */ VARIANT var1,
    /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *ppElem)
{
    CHECK_ARG_INIT(ppElem);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    HRESULT hr;
    TraceTag((tagIE4OM, "IDocumentWrapper::createElement"));

    TRY
    {
        VARIANT v;
        VariantInit(&v);
        hr = VariantChangeType(&v, &vType, 0, VT_I4);
        if (hr != S_OK)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        int t = V_I4(&v);
        if (t >= XMLELEMTYPE_OTHER) // what does it mean to create an element of type OTHER ???
        {
            hr = E_NOTIMPL;
            goto Cleanup;
        }
        if (t == XMLELEMTYPE_DOCUMENT) // this should never be done (it results in an inconsisten tree)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        NameDef * n = null;
        if (var1.vt == VT_BSTR) { // see if optional name is specified.
            // Sanity check - cannot set tag name on text-only nodes.
            TCHAR* tagname = V_BSTR(&var1);
            if (tagname != NULL)
            {
                if (t == XMLELEMTYPE_TEXT || t == XMLELEMTYPE_COMMENT)
                    return E_INVALIDARG;
                String* s = String::newString(tagname);
                if (getWrapped()->isCaseInsensitive())
                    n = getWrapped()->getNamespaceMgr()->createNameDef(s->toUpperCase());
                else
                    n = getWrapped()->getNamespaceMgr()->createNameDef(s);
            }
        }
        TraceTag((tagIE4OM, "IDocumentWrapper::createElement %i, %s", t, (char *)AsciiText(n)));
        // XMLELEM_TYPE and the Element::types are aligned to match up.
        Element * e = getWrapped()->createElement(null, t, n, null);
        *ppElem = new IElementWrapper(CAST_TO(ElementNode *, e));
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE IDocumentWrapper::get_async( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *p)
{
    CHECK_ARG_INIT(p);
    STACK_ENTRY;
    OMREADLOCK(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        // delegate to DOM interface.
        if (getWrapped()->isAsync())
            *p = VARIANT_TRUE;
        else
            *p = VARIANT_FALSE;
        TraceTag((tagIE4OM, "IDocumentWrapper::get_async %i", (*p != VARIANT_FALSE)));
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE IDocumentWrapper::put_async( 
    /* [in] */ VARIANT_BOOL p)
{
    STACK_ENTRY;
    OMWRITELOCK(getWrapped());
    HRESULT hr = S_OK;
    
    TRY
    {
        TraceTag((tagIE4OM, "IDocumentWrapper::put_async %i", (p != VARIANT_FALSE)));
        // delegate to DOM interface.
        getWrapped()->setAsync(p == VARIANT_TRUE);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


//==========================================================================================
DISPATCHINFO _dispatchexport<ElementNode, IXMLElement2, &LIBID_MSXML, ORD_MSXML, &IID_IXMLElement2>::s_dispatchinfo =
{
    NULL, &IID_IXMLElement2, &LIBID_MSXML, ORD_MSXML
};


IElementWrapper::IElementWrapper(ElementNode * p):_dispatchexport<ElementNode, IXMLElement2, &LIBID_MSXML, ORD_MSXML, &IID_IXMLElement2>(p)
{
    p->_addRef(); // extra addref to make sure that the refcount > 1
    // This must be here to correctly initialize s_dispatchinfo under Unix
}

IElementWrapper::~IElementWrapper()
{
    getWrapped()->_release();
}


static
W3CDOMWrapper * GetW3CDOMWrapper(Node * pNode, DOMNode * pDOMNodeParam)
{
    DOMNode * pDOMNode = pDOMNodeParam;
    if (!pDOMNodeParam)
        pDOMNode = pNode->getDOMNodeWrapper();
    W3CDOMWrapper * pW3CWrapper = pDOMNode->getW3CWrapper();
    pW3CWrapper->AddRef();
    if (!pDOMNodeParam)
        pDOMNode->Release();
    return pW3CWrapper;
}

class DOMSupportErrorInfoTear : public _unknown<ISupportErrorInfo, &IID_ISupportErrorInfo>
{
private:
    _reference<Document>    _pDocument;
    _reference<Node>        _pNode;

public:
    DOMSupportErrorInfoTear(Document * pDoc, Node * pNode) 
        :_pDocument(pDoc), _pNode(pNode)
    {}

    // ISupportErrorInfo :
    HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo(REFIID riid)
    {
        HRESULT hr = S_FALSE;
        Element::NodeType eType = _pNode->getNodeType();
        if (_pDocument->isDOM())
        {
            if ((riid == IID_IXMLDOMNode) ||
                (riid == IID_IXMLDOMElement && eType == Element::ELEMENT) ||
                (riid == IID_IXMLDOMAttribute && eType == Element::ATTRIBUTE) ||
                (riid == IID_IXMLDOMProcessingInstruction && (eType == Element::PI || eType == Element::XMLDECL)) ||
                (riid == IID_IXMLDOMComment && eType == Element::COMMENT) ||
                (riid == IID_IXMLDOMCharacterData && (eType == Element::PCDATA || eType == Element::CDATA || eType == Element::COMMENT)) ||
                (riid == IID_IXMLDOMText && (eType == Element::PCDATA || eType == Element::CDATA)) ||
                (riid == IID_IXMLDOMCDATASection && eType == Element::CDATA) ||
                (riid == IID_IXMLDOMDocumentFragment && eType == Element::DOCFRAG) ||
                (riid == IID_IXMLDOMEntity && eType == Element::ENTITY) ||
                (riid == IID_IXMLDOMNotation && eType == Element::NOTATION) ||
                (riid == IID_IXMLDOMEntityReference && eType == Element::ENTITYREF) ||
                (riid == IID_IXMLDOMDocumentType && eType == Element::DOCTYPE) ||
                (riid == IID_IXMLDOMDocument && eType == Element::DOCUMENT))
            {
                hr = S_OK;
            }
        }
        return hr;
    }
};

// Just compare the last 3 elements ...
BOOL ShortIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
	return (
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

//
// We do not allow switching between DOM and IE4 object model
// so we have to check whether it is DOM or not in query interfaces
//

#define IID_IUnknownDATA1           0x00000000
#define IID_IMarshalDATA1           0x00000003
#define IID_IStreamDATA1            0x0000000c
#define IID_IPersistStreamDATA1     0x00000109
#define IID_IDispatchDATA1          0x00020400
#define IID_IXMLDOMDocumentDATA1    0x2933BF81
#define IID_IXMLDocument2DATA1      0x2B8DE2FE
#define IID_IXMLDOMNodeDATA1        0x2933BF80
#define IID_IXMLDOMCharacterDataDATA1   0x2933BF84
#define IID_IXMLDOMAttributeDATA1   0x2933BF85
#define IID_IXMLDOMElementDATA1     0x2933BF86
#define IID_IXMLDOMCommentDATA1     0x2933BF88
#define IID_IXMLDOMTextDATA1        0x2933BF87
#define IID_IXMLDOMProcessingInstructionDATA1 0x2933BF89
#define IID_IXMLDOMCDATASectionDATA1    0x2933BF8A
#define IID_IXMLDOMDocumentTypeDATA1    0x2933BF8B
#define IID_IXMLDOMNotationDATA1        0x2933BF8C
#define IID_IXMLDOMEntityDATA1          0x2933BF8D
#define IID_IXMLDOMEntityReferenceDATA1 0x2933BF8E
#define IID_IXMLElement2DATA1           0x2B8DE2FF
#define IID_IXMLParserDATA1         0xd242361e
#define IID_IXMLDOMDocumentFragmentDATA1     0x3efaa413
#define IID_IXMLElementDATA1        0x3F7F31AC
#define IID_ElementDATA1            0x4014F154
#define IID_DOMNodeDATA1            0x6d7fc382
#define IID_IPersistMonikerDATA1    0x79eac9c9
#define IID_IPersistStreamInitDATA1 0x7FD52380
#define IID_ObjectDATA1             0x82ef0880
#define IID_IXMLErrorDATA1          0x948C5AD3
#define IID_IDispatchExDATA1        0xa6ef9860
#define IID_IProvideClassInfoDATA1  0xB196B283
#define IID_IConnectionPointContainerDATA1 0xB196B284
#define IID_DocumentDATA1           0xB5B359F0
#define IID_IOleCommandTargetDATA1  0xb722bccb
#define IID_IObjectSafetyDATA1      0xcb5bdc81 
#define IID_IXMLAttributeDATA1      0xD4D4A0FC
#define IID_ISupportErrorInfoDATA1  0xDF0B3D60
#define IID_NodeDATA1               0xeea82e63
#define IID_IObjectWithSiteDATA1    0xFC4801A3
#define IID_IXMLDocumentDATA1       0xF52E2B61


HRESULT
Node::QIHelper(DOMDocumentWrapper * pDOMDoc, DOMNode * pDOMNode, 
                IDocumentWrapper * pIE4Doc, IDispatch * pIE4Node, 
                REFIID iid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
// Assert(0);
    TRY
    {
        *ppv = NULL;
        Document *pDoc = getNodeDocument();
        Element::NodeType eType = getNodeType();

        switch (iid.Data1)
        {

            case IID_IUnknownDATA1:
            {
                if (ShortIsEqualGUID(iid, IID_IUnknown))
                {
                    if (eType == DOCUMENT)
                        assign(ppv, (GenericBase *)pDoc);
                    else
                        assign(ppv, this);
                    hr = S_OK;
                }
            }
            break;
            
            case IID_IDispatchDATA1:
            {
                if (ShortIsEqualGUID(iid, IID_IDispatch))
                {
                    hr = S_OK;
                    if (eType == DOCUMENT)
                    {
                        if (pDoc->isDOM())
                        {
                            if (pDOMDoc)
                                assign(ppv, (IXMLDOMDocument *)pDOMDoc);
                            else
                                *ppv = (IXMLDOMDocument *)new DOMDocumentWrapper(pDoc);
                        }
                        else
                        {
                            if (pIE4Doc)
                                assign(ppv, (IXMLDocument2 *)pIE4Doc);
                            else
                                *ppv = (IXMLDocument2 *)new IDocumentWrapper(pDoc);
                        }
                    }
                    else // not (getNodeType() == DOCUMENT)
                    {
                        if (pDoc->isDOM())
                            if (pDOMNode)
                                assign(ppv, (IXMLDOMNode *)pDOMNode);
                            else
                                *ppv = (IXMLDOMNode *)getDOMNode();
                        else
                            if (pIE4Node)
                                assign(ppv, pIE4Node);
                            else
                                if( _fAttribute)
                                    *ppv = (IXMLAttribute *)new IAttributeWrapper(this);
                                else
                                    *ppv = (IXMLElement *)new IElementWrapper(this);
                    }
                }
            }
            break;

            case IID_IDispatchExDATA1:
            {
                if (ShortIsEqualGUID(iid, IID_IDispatchEx))
                {
                    if (pDoc->isDOM())
                    {
                        if (eType == DOCUMENT)
                        {
                            if (pDOMDoc)
                                assign(ppv, static_cast<IDispatchEx *>(pDOMDoc));
                            else
                                *ppv = static_cast<IDispatchEx *>(new DOMDocumentWrapper(pDoc));
                        }
                        else // not (getNodeType() == DOCUMENT)
                        {
                            if (pDOMNode)
                                assign(ppv, static_cast<IDispatchEx *>(pDOMNode));
                            else
                                *ppv = static_cast<IDispatchEx *>(getDOMNode());
                        }
                        hr = S_OK;
                    }
                }
            }
            break;

            case IID_ISupportErrorInfoDATA1:
            {
                if (ShortIsEqualGUID(iid, IID_ISupportErrorInfo))
                {
                    if (pDoc->isDOM())
                    {
                        assign(ppv, static_cast<ISupportErrorInfo *>(new DOMSupportErrorInfoTear(pDoc, this)));
                        hr = S_OK;
                    }
                }
            }
            break;

            case IID_NodeDATA1:
            {
                if (ShortIsEqualGUID(iid, Node::s_IID))
                {
                    *ppv = getNodeData();
                    hr = S_OK;
                }
            }
            break;

            case IID_ObjectDATA1:
            {
                if (ShortIsEqualGUID(iid, IID_Object))
                {
                    *ppv = (Object *)this;
                    hr = S_OK;
                }
            }
            break;

            case IID_ElementDATA1:
            {
                if (ShortIsEqualGUID(iid, IID_Element))
                {
                    // no refcount !
                    *ppv = (Element *)this;
                    hr = S_OK;
                }
            }
            break;

            default:
            {
                if (pDoc->isDOM())
                {

                    switch (iid.Data1)
                    {
                    
                        case IID_IXMLDOMNodeDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMNode))
                            {
                                hr = S_OK;
                                if (pDOMNode)
                                    assign(ppv, (IXMLDOMNode *)pDOMNode);
                                else
                                    *ppv = (IXMLDOMNode *)getDOMNode();
                            }
                        }
                        break;

                        case IID_DOMNodeDATA1:
                        {
                            if (ShortIsEqualGUID(iid, DOMNode::s_IID))
                            {
                                hr = S_OK;
                                if (pDOMNode)
                                    assign(ppv, (IXMLDOMNode *)pDOMNode);
                                else
                                    *ppv = (IXMLDOMNode *)getDOMNode();
                            }
                        }
                        break;

                        case IID_IXMLDOMElementDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMElement))
                            {
                                if (eType == Node::ELEMENT)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMElement *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMAttributeDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMAttribute))
                            {
                                if (eType == Node::ATTRIBUTE)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMAttribute *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMProcessingInstructionDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMProcessingInstruction))
                            {
                                if (eType == Element::PI || eType == Element::XMLDECL)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMProcessingInstruction *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMCommentDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMComment))
                            {
                                if (eType == Node::COMMENT)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMComment *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMCharacterDataDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMCharacterData))
                            {
                                if (eType == Node::PCDATA || eType == Node::CDATA || eType == Node::COMMENT)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMText *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMTextDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMText))
                            {
                                if (eType == Node::PCDATA || eType == Node::CDATA)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMText *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMCDATASectionDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMCDATASection))
                            {
                                if (eType == Node::CDATA)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMCDATASection *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMDocumentFragmentDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMDocumentFragment))
                            {
                                if (eType == Node::DOCFRAG)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMDocumentFragment *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMEntityDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMEntity))
                            {
                                if (eType == Node::ENTITY)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMEntity *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMNotationDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMNotation))
                            {
                                if (eType == Node::NOTATION)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMNotation *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMEntityReferenceDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMEntityReference))
                            {
                                if (eType == Node::ENTITYREF)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMEntityReference *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        case IID_IXMLDOMDocumentTypeDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLDOMDocumentType))
                            {
                                if (eType == Node::DOCTYPE)
                                {
                                    W3CDOMWrapper * pWrapper = GetW3CDOMWrapper(this, pDOMNode);
                                    *ppv = (IXMLDOMDocumentType *)pWrapper;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                        default:
                        {
                            if (eType == DOCUMENT)
                            {
                                hr = pDoc->QIHelper(pDOMDoc, pIE4Doc, iid, ppv);
                            }
                        }
                        break;
                    } // switch (iid.Data1)
                }
                else
                { 
                    switch (iid.Data1)
                    {
                        case IID_IXMLElementDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLElement))
                            {
                                if (!_fAttribute)
                                {
                                    hr = S_OK;
                                    if (pIE4Node)
                                        assign(ppv, (IXMLElement2 *)pIE4Node);
                                    else
                                        *ppv = (IXMLElement2 *)new IElementWrapper(this);
                                }
                            }
                        }
                        break;

                        case IID_IXMLElement2DATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLElement2))
                            {
                                if (!_fAttribute)
                                {
                                    hr = S_OK;
                                    if (pIE4Node)
                                        assign(ppv, (IXMLElement2 *)pIE4Node);
                                    else
                                        *ppv = (IXMLElement2 *)new IElementWrapper(this);
                                }
                            }
                        }
                        break;

                        case IID_IXMLAttributeDATA1:
                        {
                            if (ShortIsEqualGUID(iid, IID_IXMLAttribute))
                            {
                                if (_fAttribute)
                                {
                                    hr = S_OK;
                                    if (pIE4Node)
                                        assign(ppv, (IXMLAttribute *)pIE4Node);
                                    else
                                        *ppv = (IXMLAttribute *)new IAttributeWrapper(this);
                                }
                            }
                        }
                        break;

                        default:
                        {
                            if (eType == DOCUMENT)
                            {
                                hr = pDoc->QIHelper(pDOMDoc, pIE4Doc, iid, ppv);
                            }
                        }
                        break;
                    }
                }  // if (pDoc->isDOM())

            } // default
        } // switch(iid.Data1)
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT
Document::QIHelper(DOMDocumentWrapper * pDOMDoc, IDocumentWrapper * pIE4Doc,
                   REFIID iid, void **ppv)
{
    HRESULT  hr = E_NOINTERFACE;

    switch (iid.Data1)
    {
        case IID_IPersistStreamInitDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IPersistStreamInit))
            {
                *ppv = (IPersistStreamInit *)new IPersistStreamWrapper(this, _pMutex);
                hr = S_OK;
            }
        }
        break;

        case IID_IPersistStreamDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IPersistStream))
            {
                // IPersistStream identical to IPersistStreamInit except it does
                // not have the InitNew method at the end - to this is vtable compatible.
                *ppv = (IPersistStream *)new IPersistStreamWrapper(this, _pMutex);
                hr = S_OK;
            }
        }
        break;

        case IID_IStreamDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IStream))
            {
                // Someone wants to read or write directly to this document object.
                // Ok, then wrap the document a special IStream that knows how to 
                // build the document upon Write, or save the document upon Read.
                *ppv = (IStream *) new DocStream(this);
                hr = S_OK;
            }
        }
        break;

        
        case IID_IPersistMonikerDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IPersistMoniker))
            {
                *ppv = (IPersistMoniker *)new IPersistMonikerWrapper(this, _pMutex);
                hr = S_OK;
            }
        }
        break;

        
        case IID_IXMLErrorDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IXMLError))
            {
                if (!isDOM())
                {
                    *ppv = (IXMLError *)new IXMLErrorWrapper(this);
                    hr = S_OK;
                }
            }
        }
        break;
        
        
        case IID_IObjectWithSiteDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IObjectWithSite))
            {
                *ppv = (IObjectWithSite *)new IObjectWithSiteWrapper(this, _pMutex);
                hr = S_OK;
            }
        }
        break;

        case IID_IObjectSafetyDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IObjectSafety))
            {
                *ppv = (IObjectSafety *)new IObjectSafetyWrapper(this, _pMutex);
                hr = S_OK;
            }
        }
        break;

        case IID_IOleCommandTargetDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IOleCommandTarget))
            {
                *ppv = (IOleCommandTarget *)new IOleCommandTargetWrapper(this, _pMutex);
                hr = S_OK;
            }
        }
        break;

        case IID_IXMLParserDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IXMLParser))
            {
                STACK_ENTRY;
                OMWRITELOCK( this);

                // This is a back-door hand shake for internal clients who want to plug in 
                // their own node factories.
                IXMLParser* pParser;
                 getParser(&pParser);
                *ppv = pParser;
                hr = S_OK;
            }
        }
        break;
            
        case IID_DocumentDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_Document))
            {
                // no refcount !
                *ppv = (Document *)this;
                hr = S_OK;
            }
        }
        break;

        case IID_ObjectDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_Object))
            {
                *ppv = (GenericBase *)this;
                hr = S_OK;
            }
        }
        break;

        case IID_IMarshalDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IMarshal))
            {
#ifdef RENTAL_MODEL
                if (model() == Rental)
                {
                    goto Cleanup;
                }
#endif
                if (_pFreeThreadedMarshaller == null)
                {
                    // We have to aggregate the IMarshal interface for IIS 4 to
                    // accept us in free threading mode.
                    IUnknown* outerUnknown;

                    hr = super::QueryInterface(IID_IUnknown, (void**)&outerUnknown);
                    if (hr)
                        goto Cleanup;

                    hr = ::CoCreateFreeThreadedMarshaler( 
                        outerUnknown, &_pFreeThreadedMarshaller);

                    outerUnknown->Release();

                    if (hr) 
                        goto Cleanup;
                }
                return _pFreeThreadedMarshaller->QueryInterface(iid, ppv);
            }
        }
        break;

        case IID_IConnectionPointContainerDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IConnectionPointContainer))
            {
                CXMLConnectionPtContainer *pXCPC = new CXMLConnectionPtContainer(
                        DIID_XMLDOMDocumentEvents,
                        (IUnknown *)(IOleCommandTarget *)this, 
                        &_pCPListRoot,
                        &_lSpinLockCPC);
    
                if (pXCPC)
                {
                    *ppv = (LPVOID *)(IUnknown *)(IConnectionPointContainer *)pXCPC;
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        break;

        case IID_IProvideClassInfoDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IProvideClassInfo))
            {
                ProvideClassInfo *pPCI = new ProvideClassInfo(
                    (IUnknown *)(IOleCommandTarget *)this,
                    LIBID_MSXML,
                    CLSID_DOMDocument);

                if (NULL != pPCI)
                {
                    *ppv = (LPVOID *)(IProvideClassInfo *)pPCI;
                    hr = S_OK;
                }
                else
                {
                    return E_OUTOFMEMORY;
                }
            }
        }
        break;
        
        case IID_IXMLDocumentDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IXMLDocument))
            {
                if (!isDOM())
                {
                    if (pIE4Doc)
                        assign(ppv, (IXMLDocument2 *)pIE4Doc);
                    else
                        *ppv = (IXMLDocument2 *)new IDocumentWrapper(this);
                    hr = S_OK;
                }
            }
        }
        break;

        case IID_IXMLDocument2DATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IXMLDocument2))
            {
                if (!isDOM())
                {
                    if (pIE4Doc)
                        assign(ppv, (IXMLDocument2 *)pIE4Doc);
                    else
                        *ppv = (IXMLDocument2 *)new IDocumentWrapper(this);
                    hr = S_OK;
                }
            }
        }
        break;

        case IID_IXMLDOMDocumentDATA1:
        {
            if (ShortIsEqualGUID(iid, IID_IXMLDOMDocument))
            {
                if (isDOM())
                {
                    if (pDOMDoc)
                        assign(ppv, (IXMLDOMDocument *)pDOMDoc);
                    else
                        *ppv = (IXMLDOMDocument *)new DOMDocumentWrapper(this);
                    hr = S_OK;
                }
            }
        }
        break;

        default:
            hr = E_NOINTERFACE;
    }

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
IElementWrapper::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    TraceTag((tagIE4OM, "IElementWrapper::QueryInterface"));

    hr = getWrapped()->getNodeData()->QIHelper( NULL, NULL, NULL, this, iid, ppv);

    return hr;
}

/**
 * COM object model implementation.
 */
HRESULT STDMETHODCALLTYPE IElementWrapper::get_tagName( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    CHECK_ARG_INIT(p);
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        // compatibility code
        // BUGBUG - we could skip this in the normal case if we did
        // this check after getTagName returned null.  First we need
        // to make comments return null.  Currently they return "--".
        switch (getWrapped()->getType())
        {
        case Element::PCDATA:   
        case Element::CDATA:
        case Element::COMMENT:
            hr = E_NOTIMPL;
            goto Cleanup;
        }

        Name * n = getWrapped()->getTagName();
        if (n != null)
            *p = n->toString()->getBSTR();
        TraceTag((tagIE4OM, "IElementWrapper::get_tagName: %s", (char *)AsciiText(n)));
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::put_tagName( 
    /* [in] */ BSTR p)
{
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        String * name = String::newString(p);
        TraceTag((tagIE4OM, "IElementWrapper::put_tagName: %s", (char *)AsciiText(name)));

        switch (getWrapped()->getType())
        {
        case Element::ELEMENT:
        case Element::PI:
        case Element::ENTITY:
            getWrapped()->setTagName(name);
            break;

        default:
            hr = S_FALSE; // fix bug 251
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::get_parent( 
    /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *ppParent)
{
    CHECK_ARG_INIT(ppParent);
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    TraceTag((tagIE4OM, "IElementWrapper::get_parent"));

    TRY
    {
        Element* parent = getWrapped()->getParent();
        // For compatibility we do NOT return the Document as an Element.
        if (parent == null || parent->getType() == Element::DOCUMENT)
            hr = S_FALSE;
        else
            *ppParent = new IElementWrapper(CAST_TO(ElementNode *, parent));
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::setAttribute( 
    /* [in] */ BSTR strPropertyName,
    /* [in] */ VARIANT PropertyValue)
{
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        // strict compatibility means cannot set attributes on text nodes.

        switch (getWrapped()->getType())
        {
        case Element::PCDATA:   
        case Element::CDATA:
        case Element::COMMENT:
//        case Element::PI:  // XML declaration has attributes...
            hr = E_NOTIMPL;
            goto Cleanup;

        default:
            // fall through
            break;
        }
        String * n = String::newString(strPropertyName)->toUpperCase();
        if (PropertyValue.vt == VT_BSTR)
        {
            String * v = String::newString(V_BSTR(&PropertyValue));
            TraceTag((tagIE4OM, "IElementWrapper::setAttribute: %s, %s", (char *)AsciiText(n),  (char *)AsciiText(v)));
            getWrapped()->setAttribute(Name::create(n), v);
        }
        else
            hr = E_INVALIDARG;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::getAttribute( 
    /* [in] */ BSTR strPropertyName,
    /* [out][retval] */ VARIANT __RPC_FAR *PropertyValue)
{
    CHECK_ARG(PropertyValue);
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    PropertyValue->vt = VT_EMPTY;
    PropertyValue->bstrVal = NULL;

    TRY
    {
        String * n = String::newString(strPropertyName)->toUpperCase();
        Object * o = getWrapped()->getAttribute(Name::create(n));
        if (o == NULL)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        String * v = o->toString();
        TraceTag((tagIE4OM, "IElementWrapper::getAttribute: %s, %s", (char *)AsciiText(n),  (char *)AsciiText(v)));
        PropertyValue->vt = VT_BSTR;
        V_BSTR(PropertyValue) = v->getBSTR();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::removeAttribute( 
    /* [in] */ BSTR strPropertyName)
{
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    TRY
    {
        // strict compatibility means cannot remove attributes on text nodes.
        switch (getWrapped()->getType())
        {
        case Element::PCDATA:   
        case Element::CDATA:
        case Element::COMMENT:
//        case Element::PI:  // XML declaration has attributes...
            hr = E_NOTIMPL;
            goto Cleanup;

        default:
            // fall through
            break;
        }

        Name * n = Name::create(String::newString(strPropertyName)->toUpperCase());
        TraceTag((tagIE4OM, "IElementWrapper::removeAttribute: %s", (char *)AsciiText(n)));

        if (getWrapped()->getAttribute(n) == null)    //  Actually the old IE4 object model did NOT fail this !!
            hr = S_FALSE;                       //  Bsides, this may be useful from script.
        else
            getWrapped()->removeAttribute(n);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::get_children( 
    /* [out][retval] */ IXMLElementCollection __RPC_FAR *__RPC_FAR *pp)
{
    CHECK_ARG_INIT(pp);
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    ElementLock lock(getWrapped());

    TRY
    {
        // old object model returns no collection if no children.
        TraceTag((tagIE4OM, "IElementWrapper::get_children"));
        HANDLE h;
        Element * eFirst = getWrapped()->getFirstChild(&h);
        if (eFirst)
        {
            *pp = ElementCollection::newElementCollection(getWrapped());
        }
        else
        {
            *pp = NULL;
            hr = S_FALSE; // No children 
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    if (E_PENDING == hr)
    {
        hr = S_FALSE;
        *pp = NULL;
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::get_type( 
    /* [out][retval] */ long __RPC_FAR *plType)
{
    CHECK_ARG_INIT(plType);
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    ElementLock lock(getWrapped());

    TRY
    {
        long type = XMLELEMTYPE_OTHER;
        switch (getWrapped()->getType())
        {
        case Element::ELEMENT:  type = XMLELEMTYPE_ELEMENT;  break;
        case Element::PCDATA:   
        case Element::CDATA:    type = XMLELEMTYPE_TEXT;     break;
        case Element::COMMENT:  type = XMLELEMTYPE_COMMENT;  break;
        case Element::DOCUMENT: type = XMLELEMTYPE_DOCUMENT; break;
        case Element::DOCTYPE:  type = XMLELEMTYPE_DTD;      break;
        case Element::PI:       type = XMLELEMTYPE_PI;       break;
        default:                type = XMLELEMTYPE_OTHER;
        }
        *plType = type;
        TraceTag((tagIE4OM, "IElementWrapper::get_type: %i", (int)type));
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::get_text( 
    /* [out][retval] */ BSTR __RPC_FAR *p)
{
    CHECK_ARG_INIT(p);
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    TraceTag((tagIE4OM, "IElementWrapper::get_text"));

    TRY
    {
        Element::NodeType eType = getWrapped()->getNodeType();
        if (eType == Element::DOCTYPE)
        {
            hr = E_NOTIMPL;
        }
        else if (eType == Element::XMLDECL)
        {
            *p = NULL;
        }
        else
        {
            Node* node = getWrapped()->getNodeData();
            String* s = node->getInnerText(node->xmlSpacePreserve(), false, false);
            *p = s->getBSTR();
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::put_text( 
    /* [in] */ BSTR p)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    ElementLock lock(getWrapped());

    TraceTag((tagIE4OM, "IElementWrapper::put_text"));

    TRY
    {
        // Only allow text to be set on those nodes that 
        // actually will return the text.
        switch (getWrapped()->getType())
        {
        case Element::PCDATA:   
        case Element::CDATA:
        case Element::COMMENT:
        case Element::PI:
            {
                String* s = String::newString(p);
                getWrapped()->setText(s);
            }
            break;
        default:
            hr = E_NOTIMPL;
            break;
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::addChild( 
    /* [in] */ IXMLElement2 __RPC_FAR *pChildElem,
    long lIndex,
    long lReserved)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    ElementLock lock(getWrapped());

    TraceTag((tagIE4OM, "IElementWrapper::addChild"));

    if (!pChildElem)
        return E_INVALIDARG;

    TRY
    {
        // BUGBUG for now we only add our own objects...
        Element * e = _comexport<ElementNode, IXMLElement2, &IID_IXMLElement2>::getExported(pChildElem, IID_Element);
        getWrapped()->addChildAt(e, lIndex);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IElementWrapper::removeChild( 
    /* [in] */ IXMLElement2 __RPC_FAR *pChildElem)
{
    STACK_ENTRY_WRAPPED;
    ElementLock lock(getWrapped());
    HRESULT hr = S_OK;

    TraceTag((tagIE4OM, "IElementWrapper::removeChild"));

    if (!pChildElem)
        return E_INVALIDARG;

    TRY
    {
        // BUGBUG for now we only add our own objects...
        Element * e = _comexport<ElementNode, IXMLElement2, &IID_IXMLElement2>::getExported(pChildElem, IID_Element);
        getWrapped()->removeChild(e);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


//========== IXMLElement2 =================================================

HRESULT STDMETHODCALLTYPE IElementWrapper::get_attributes( 
    /* [out][retval] */ IXMLElementCollection __RPC_FAR *__RPC_FAR *pp)
{
    CHECK_ARG_INIT(pp);
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    ElementLock lock(getWrapped());
    TRY
    {
        // old object model returns no collection if no children.
        TraceTag((tagIE4OM, "IElementWrapper::get_attributes"));
        HANDLE h;
        Element * eFirst = getWrapped()->getFirstAttribute(&h);
        if (eFirst)
        {
            *pp = ElementCollection::newElementCollection(getWrapped(), null, true);
        }
        else
        {
            *pp = NULL;
            hr = S_FALSE; // No children 
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

    
//====================================================================================
// Class ElementCollection
//

DISPATCHINFO _dispatch<IXMLElementCollection, &LIBID_MSXML, &IID_IXMLElementCollection>::s_dispatchinfo = 
{
    NULL, &IID_IXMLElementCollection, &LIBID_MSXML, ORD_MSXML
};


ElementCollection::ElementCollection()
{
    // This must be here to correctly initialize s_dispatchinfo under Unix
};

ElementCollection * 
ElementCollection::newElementCollection(Element * root, Name * tag, bool fEnumAttributes)
{
    Assert(root);

    ElementCollection * en = new ElementCollection();
    en->_pRoot = root;
    en->_pName = tag;
    en->_fAttributes = fEnumAttributes;
	return en;
}


HRESULT STDMETHODCALLTYPE 
ElementCollection::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_OBJECT(_pRoot);
    HRESULT hr;

    TRY
    {
        if (iid == IID_IUnknown || iid == IID_IDispatch ||
            iid == IID_IXMLElementCollection)
        {
            AddRef();
            *ppv =(IXMLElementCollection *)this;
            hr = S_OK;
        }
        else if (iid == IID_IEnumVARIANT)
        {
            hr = get__newEnum((IUnknown**)ppv);
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
ElementCollection::put_length( 
    /* [in] */ long v)
{
    TraceTag((tagIE4OM, "ElementCollection::put_length: E_FAIL"));

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE 
ElementCollection::get_length( 
    /* [out][retval] */ long __RPC_FAR *p)
{
    CHECK_ARG_INIT(p);
    STACK_ENTRY_OBJECT(_pRoot);;
    HRESULT hr = S_OK;
    long count = 0;

    TraceTag((tagIE4OM, "ElementCollection::get_length"));
    
    ElementLock lock(_pRoot);

    TRY
    {
        // Compatibility - comment and PI elements didn't used to
        // have children. (Now they have a text child)  Return
        // zero for those component types.
        //
        switch (_pRoot->getType())
        {
        case Element::COMMENT:
        case Element::PI:
            break;
        default:
            // Count how many children match this collection's
            // tagname constraint.
            HANDLE handle = 0;
            Node* pNode = CAST_TO(ElementNode *, _pRoot)->getNodeData();
            while (_next(pNode, &handle, _pName)) 
                count++;
            break;
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    if (E_PENDING == hr)
        hr = S_FALSE;
    *p = count;
    return hr;
}

Node *
ElementCollection::_next(Node * pNode, void ** ppv, Name * tag)
{
    if (_fAttributes)
    {
        return pNode->getNextMatchingAttribute(ppv, tag);
    }
    else
    {
        return pNode->getNextMatchingChild(ppv, tag);
    }
}


HRESULT STDMETHODCALLTYPE 
ElementCollection::get__newEnum( 
    /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{
    CHECK_ARG_INIT(ppUnk);
    STACK_ENTRY_OBJECT(_pRoot);;
    HRESULT hr = S_OK;

    TraceTag((tagIE4OM, "ElementCollection::get__newEnum"));

    ElementLock lock(_pRoot);

    TRY
    {
        // Create a new enumeration that enumerates through the same
        // set of children as this collection.
        *ppUnk = IEnumVARIANTWrapper::newIEnumVARIANTWrapper((IXMLElementCollection*)this, (EnumVariant*)this, 
                            _pRoot->getDocument() ? _pRoot->getDocument()->getMutex() : s_pMutex);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}

void GetNameAndIndexFromVariants(
    VARIANT var1,
    VARIANT var2,
    LONG *plIndex,
    BSTR *ppName)
{
    VARIANT *pvarOne  = NULL;
    VARIANT *pvarName = NULL;   
    VARIANT *pvarIndex = NULL;   

    pvarOne = (V_VT(&var1) == (VT_BYREF | VT_VARIANT)) ? V_VARIANTREF(&var1) : &var1;

    if ((V_VT(pvarOne)==VT_BSTR) || V_VT(pvarOne)==(VT_BYREF|VT_BSTR))
    {
        pvarName = (V_VT(pvarOne) & VT_BYREF) ?
            V_VARIANTREF(pvarOne) : pvarOne;

        if ((V_VT(&var2) != VT_ERROR ) &&
            (V_VT(&var2) != VT_EMPTY ))
        {
            pvarIndex = &var2;
        }
    }
    else if ((V_VT(&var1) != VT_ERROR )&&
             (V_VT(&var1) != VT_EMPTY ))
    {
        pvarIndex = &var1;
    }

    if (pvarIndex)
    {
        VARIANT varNum;

         VariantInit(&varNum);
         checkhr(VariantChangeTypeEx(&varNum,
                                      pvarIndex,
                                      LOCALE_USER_DEFAULT,
                                      0,
                                      VT_I4));
         *plIndex = V_I4(&varNum);
    }

    if (pvarName)
        *ppName = V_BSTR(pvarName);
}

HRESULT STDMETHODCALLTYPE 
ElementCollection::item( 
    /* [in][optional] */ VARIANT var1,
    /* [in][optional] */ VARIANT var2,
    /* [out][retval] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
    CHECK_ARG_INIT(ppDisp);
    STACK_ENTRY_OBJECT(_pRoot);;
    HRESULT hr = S_OK;
    ElementLock lock(_pRoot);

    TraceTag((tagIE4OM, "ElementCollection::item"));

    TRY
    {
        BSTR        bstrName = NULL;
        long        index = -1;
        Element * e = null;
        Name * tag;
        HANDLE handle = 0;
        Node * pNode;
        long count = 0;
        Node * pRootNode = CAST_TO(ElementNode *, _pRoot)->getNodeData();

        GetNameAndIndexFromVariants(var1, var2, &index, &bstrName);

        if (bstrName)
        {
            Document* doc = _pRoot->getDocument();
            if (doc != null && doc->isCaseInsensitive())
            {
                tag = Name::create(String::newString(bstrName)->toUpperCase());
            } 
            else
            {
                tag = Name::create(bstrName);
            }

            TraceTag((tagIE4OM, "\tname=%s, index=%i", (char *)AsciiText(tag), index));

            if (index < 0)
            {
                pNode = _next(pRootNode, &handle, tag);
                if (pNode)
                {
                    if (_next(pRootNode, &handle, tag))
                    {
                        // when there are more than one element
                        // returns a collection object representing 
                        // the collection of all children with matching name.
                        *ppDisp = (IXMLElementCollection*) ElementCollection::newElementCollection(_pRoot, tag, _fAttributes);
                        goto Cleanup;
                    }
                }
            }
            else
            {
                while ((pNode = _next(pRootNode, &handle, tag)) && index != count++);
            }
        }
        else if (index < 0)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            while ((pNode = _next(pRootNode, &handle, _pName)) && index != count++);
            // IE4 compatibility requires that we return E_FAIL in this case.
            if (pNode == null)
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }

        if (pNode)
        {
            e = pNode->getElementWrapper();
            if (_fAttributes)
                *ppDisp = new IAttributeWrapper(CAST_TO(ElementNode *, e));
            else
                *ppDisp = new IElementWrapper(CAST_TO(ElementNode *, e));
        }
        else
        {
            hr = S_FALSE;
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    if (E_PENDING == hr)
    {
        hr = S_FALSE;
        *ppDisp = NULL;
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// EnumVariant Interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// This method is called by the IEnumVARIANTWrapper, so no lock or try/catch 
// is needed here.
IDispatch * 
ElementCollection::enumGetIDispatch(Node * pNode)
{
    IDispatch * pDisp;
    if (pNode)
    {
        Element * e = pNode->getElementWrapper();
        Assert( CHECKTYPEID(*e, ElementNode));
        if (_fAttributes)
        {
            pDisp = new IAttributeWrapper(CAST_TO(ElementNode *, e));
        }
        else
        {
            pDisp = new IElementWrapper(CAST_TO(ElementNode *, e));
        }
    }
    else
    {
        pDisp = null;
    }
    return pDisp;
}

Node * 
ElementCollection::nextNode(void ** ppv)
{
    Node * pNode;
    TRY
    {
        pNode = _next(CAST_TO(ElementNode*, _pRoot)->getNodeData(), ppv, _pName);
    }
    CATCH
    {
        HRESULT hr = ERESULT;
        if (E_PENDING != hr)
            Exception::throwAgain();
        pNode = null;
    }
    ENDTRY
    return pNode;
}

//====================================================================================
DISPATCHINFO _dispatchexport<ElementNode, IXMLAttribute, &LIBID_MSXML, ORD_MSXML, &IID_IXMLAttribute>::s_dispatchinfo = 
{
    NULL, &IID_IXMLAttribute, &LIBID_MSXML, ORD_MSXML
};

IAttributeWrapper::IAttributeWrapper(ElementNode * p) : _dispatchexport<ElementNode, IXMLAttribute, &LIBID_MSXML, ORD_MSXML, &IID_IXMLAttribute>(p)
{
    p->_addRef(); // extra addref to make sure that the refcount > 1
    // This must be here to correctly initialize s_dispatchinfo under Unix
}

IAttributeWrapper::~IAttributeWrapper()
{
    getWrapped()->_release();
}


HRESULT STDMETHODCALLTYPE 
IAttributeWrapper::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    TraceTag((tagIE4OM, "IAttributeWrapper::QueryInterface"));

    hr = getWrapped()->getNodeData()->QIHelper( NULL, NULL, NULL, this, iid, ppv);

    return hr;
}

/* [id][propget] */ HRESULT STDMETHODCALLTYPE 
IAttributeWrapper::get_name( 
        /* [out][retval] */ BSTR __RPC_FAR *p)
{
    CHECK_ARG_INIT(p);
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    // BUGBUG have to use global mutex for now because cannot get to element...
    MutexLock lock(s_pMutex);
    TRY
    {
        Name * n = getWrapped()->getTagName();
        *p = n->toString()->getBSTR();
        TraceTag((tagIE4OM, "IAttributeWrapper::get_name: %s", (char *)AsciiText(n)));
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}
    
/* [id][propget] */ HRESULT STDMETHODCALLTYPE 
IAttributeWrapper::get_value( 
        /* [out][retval] */ BSTR __RPC_FAR *v)
{
    CHECK_ARG_INIT(v);
    STACK_ENTRY_WRAPPED;
    HRESULT hr = S_OK;
    // BUGBUG have to use global mutex for now because cannot get to element...
    MutexLock lock(s_pMutex);

    TRY
    {
        Node* node = getWrapped()->getNodeData();
        String* s = node->getInnerText(node->xmlSpacePreserve(), false, true);
        *v = s->getBSTR();
        TraceTag((tagIE4OM, "IAttributeWrapper::get_value: %s", (char *)AsciiText(s)));
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}


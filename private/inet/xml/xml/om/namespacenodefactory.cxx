/*
 * @(#)NameSpaceNodeFactory.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "document.hxx"
#include "namespacenodefactory.hxx"
#include "xmlnames.hxx"
#include "validationfactory.hxx"
#include "dtdnodefactory.hxx"
#include "xml/schema/schemanodefactory.hxx"
#include "xml/om/namespacemgr.hxx"

#ifndef _CORE_UTIL_WSSTRINGBUFFER
#include "core/util/wsstringbuffer.hxx"
#endif

extern void GetBaseURL(IXMLNodeSource * pSource, Document * pDocument, const WCHAR **ppBaseURL, WCHAR **pBuffer);

#if PRODUCT_PROF
#include "core/base/icapexp.h"
class IceCAP
{
public:
    IceCAP()
    {
        ResumeCAP();
    }
    ~IceCAP()
    {
        SuspendCAP();
    }
};
#endif

NameSpaceNodeFactory::NameSpaceNodeFactory(Document* doc, IXMLNodeFactory * f, DTD* dtd, NamespaceMgr * mgr, 
                                           bool fIgnoreDTD, bool fProcessNamespaces)
{
    Assert(doc != null && f != null && dtd != null && mgr != null);


    _pFactory = null;
    _pMgr = mgr;
    _fHasDTD = false;
    _fEndProlog = false;
    _fLoadingNamespace = false;
    _fInAttribute = false;
    _lInDTD = 0;
    _fIgnoreDTD = fIgnoreDTD;
    _pDTD = dtd;
    _pDoc = doc;
    _cSchemaPrefixLen = _tcslen(XMLNames::pszSchemaURLPrefix);
    _pNodePending = NULL;

    TRY 
    {
        _pFactoryDefault = f;
        if (! fIgnoreDTD)
        {
            // If ValidateOnParse then we need to load the DTD.
            // But we still need the ValidationFactory even when the DTD
            // is empty so we can generate errors for undefined entities.
            if (doc->getValidateOnParse() && ! _pDTD->isSchema())
            {
                // Then we will start with default node factory,
                // switch to the DTD node factory if we find a DOCTYPE decl
                // and then switch to validation node factory.
                _pValidationFactory = new ValidationFactory(_pFactoryDefault, _pDTD, _pDoc);
                _pValidationFactory->Release(); // smart pointer addref'd it.
            }
        }
    }
    CATCH
    {
        _pFactoryDefault = null;
        _pValidationFactory = null;
        Exception::throwAgain();
    }
    ENDTRY
}

NameSpaceNodeFactory::~NameSpaceNodeFactory()
{
}

///////////////////////////////////////////////////////////////////////////////////
// IXMLNodeFactory Interface Implementation
//

HRESULT STDMETHODCALLTYPE 
NameSpaceNodeFactory::NotifyEvent( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
    /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
    HRESULT hr;

#if PRODUCT_PROF
    IceCAP cap;
#endif
    TRY
    {
        switch (iEvt)
        {
        case XMLNF_STARTDOCUMENT:
            _pFactory = _pFactoryDefault;
            _fHasDTD = false;
            break;

        case XMLNF_STARTDTD:
        case XMLNF_STARTDTDSUBSET:
            _fHasDTD = true;
            _lInDTD++;
            break;
        case XMLNF_ENDDTD:
            _lInDTD--;
            break;
        case XMLNF_ENDDTDSUBSET:
            _lInDTD--;
            // BUGBUG -- popscope here too ??
            break;
        case XMLNF_ENDPROLOG:
            // now we can switch to the validation node factory.
            _fEndProlog = true;
            // fall through
        case XMLNF_ENDENTITY:
            hr = _pFactory->NotifyEvent(pNodeSource, iEvt);
            if (hr == S_OK)
            {
                // If we have a DTD that requires validation & validation is turned on, 
                // then switch to validation factory.
                if (_pValidationFactory && _pDTD->validate() && _pDoc->getValidateOnParse())
                {
                    if (_pFactory != _pValidationFactory)
                    {
                        _pFactory = _pValidationFactory;
                        _pFactory->NotifyEvent(pNodeSource, XMLNF_STARTDTD);
                    }
                }
                else
                {
                    // switch back to default factory.
                    _pFactory = _pFactoryDefault;
                }
            }
            else if (hr == S_FALSE)
            {
                // S_FALSE is a sneaky way for DTDNodeFactory to tell
                // the NamespaceNodeFactory that we are now parsing
                // entities.
                hr = S_OK;
            }
            goto CleanUp;
            break;
        case XMLNF_ENDDOCUMENT:
            break;

        case XMLNF_ENDSCHEMA:
            if (_pNodePending && !_pDoc->getValidateOnParse())
            {
                hr = HandlePending();
                if (FAILED(hr)) goto CleanUp;
            }
            break;
        }

        hr = _pFactory->NotifyEvent(pNodeSource, iEvt);

    }
    CATCH
    {
//        hr = ERESULT;
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDoc);
    }
    ENDTRY

CleanUp:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
NameSpaceNodeFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
    /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
    HRESULT hr = S_OK;

#if PRODUCT_PROF
    IceCAP cap;
#endif
    TRY {
        hr = _pFactory->BeginChildren(pNodeSource, pNodeInfo);
    }
    CATCH
    {
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDoc);
    }
    ENDTRY


CleanUp:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
NameSpaceNodeFactory::Error( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
    /* [in] */ HRESULT hrErrorCode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
#if PRODUCT_PROF
    IceCAP cap;
#endif
    HRESULT hr = S_OK;

    TRY {
        return _pFactory->Error(pNodeSource, hrErrorCode, cNumRecs, apNodeInfo);
    }
    CATCH
    {
        // We lose the exception, because we are already
        // in an error condition, and the first error wins !!
    }
    ENDTRY

CleanUp:
    return hr;

}

HRESULT STDMETHODCALLTYPE NameSpaceNodeFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
#if PRODUCT_PROF
    IceCAP cap;
#endif
    HRESULT hr = S_OK;

    TRY 
    {
        hr = _pFactory->EndChildren(pNodeSource, fEmptyNode, pNodeInfo); 
        _pMgr->popScope(pNodeInfo->pNode);
    }
    CATCH
    {
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDoc);
    }
    ENDTRY


CleanUp:
    return hr;
}

HRESULT STDMETHODCALLTYPE NameSpaceNodeFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
#if PRODUCT_PROF
    IceCAP cap;
#endif
    HRESULT hr = S_OK;
    NameSpace* ns = null;
    bool isName = false;
    bool fScoping = false;
    bool fDefaultNS = false;
    bool fLoadedSchema = false;
    XML_NODE_INFO* pNodeInfo = *apNodeInfo;

    Atom *pAtomURN = null;
    Atom *pSrcURN = null;

    TRY 
    {
        // NOTE we no longer blindly set pNodeInfo->pReserved to the namedef
        // because the pNodeInfo->pReserved is now used for a different reason
        // in XML_PCDATA nodes to communicate the attribute value quote character.
        // We could eventually remember this in a flag on the Node so we can 
        // persist the attribute value quotes correctly.
        
        switch (pNodeInfo->dwType)
        {                    
            case XML_ELEMENT:        // <element
                fDefaultNS = true;
                //
                // Search for namespace attributes
                // NOTE: we are using special HACK from parser that tells us that
                // there are some XML_NS attributes when the pReserved field is not zero.
                //
                if (cNumRecs > 1 && pNodeInfo->pReserved != 0)
                {
                    fScoping = ProcessXMLNSAttributes(pNodeSource,
                                    cNumRecs - 1, &apNodeInfo[1], fLoadedSchema);
                }
                goto atomize;

            case XML_ENTITYREF:
                // Special check in case we have optimized the ValidationFactory away.
                // This will catch the use of undefined entity references in the document.
                if (_pFactory != _pValidationFactory && _pFactory != _pDTDNodeFactory)
                {
                    // These names cannot have namespace (or a colon in their names) 
                    if (pNodeInfo->ulNsPrefixLen)
                    {
                        Exception::throwE(XML_NAME_COLON, XML_NAME_COLON, null);
                    }
                    NameDef* namedef = _pMgr->createNameDef(pNodeInfo->pwcText, pNodeInfo->ulLen, 
                        pNodeInfo->ulNsPrefixLen, fDefaultNS, null, null);
                    pNodeInfo->pReserved = namedef;
                    _pDTD->checkEntityRef(namedef->getName(), null, _fInAttribute);
                    break;
                }
                // fall through.
            case XML_ENTITYDECL:     // <!ENTITY ...
            case XML_PI:              // <?foo ...?>
            case XML_PEREF:          // %foo;
            case XML_NOTATION:       // <!NOTATION ...
            case XML_NMTOKEN:
                // These names cannot have namespace (or a colon in their names) 
                if (pNodeInfo->ulNsPrefixLen)
                {
                    Exception::throwE(XML_NAME_COLON, XML_NAME_COLON, null);
                }
                goto atomize;

            case XML_ATTDEF:
                // Is this a default xmlns declaration?
                if (pNodeInfo->ulLen == 5 && !StrCmpN(XMLNames::pszXMLNS, pNodeInfo->pwcText, 5))
                {
                    pNodeInfo->pReserved = _pMgr->createNameDef(XMLNames::pszXMLNSCOLON, 6, 5, null, XMLNames::atomURNXMLNS, XMLNames::atomURNXMLNS);
                    _pDTD->setNeedFixNames(true);
                    break;
                }
                goto createurn;

            case XML_NAME:
                if (!_pDTD->isInElementdeclContent())
                {
                    goto atomize;
                }
                // else fall through

            case XML_ATTLISTDECL:    // <!ATTLIST ...
            case XML_ELEMENTDECL:    // <!ELEMENT ...
            case XML_ATTTYPE:
createurn:
                if (pNodeInfo->ulNsPrefixLen)
                {
                    // BUGBUG - have to match the NameSpaceMgr findURN function
                    // otherwise we build a name that doesn't match during
                    // validation.
                    if (pNodeInfo->ulNsPrefixLen == 3 && !StrCmpN(XMLNames::pszXML, pNodeInfo->pwcText, 3))
                        pSrcURN = pAtomURN = XMLNames::atomURNXML;
                    else if (pNodeInfo->ulNsPrefixLen == 5 && !StrCmpN(XMLNames::pszXMLNS, pNodeInfo->pwcText, 5))
                        pSrcURN = pAtomURN = XMLNames::atomURNXMLNS;
                    else
                    {
                        pSrcURN = Atom::create(pNodeInfo->pwcText, pNodeInfo->ulNsPrefixLen);
                        pAtomURN = NamespaceMgr::CanonicalURN(pSrcURN);
                    }
                    _pDTD->setNeedFixNames(true);
                }
                // fall through

            case XML_DTDATTRIBUTE:
            case XML_XMLDECL:
atomize:
                pNodeInfo->pReserved = _pMgr->createNameDef(pNodeInfo->pwcText, pNodeInfo->ulLen, 
                    pNodeInfo->ulNsPrefixLen, fDefaultNS, pAtomURN, pSrcURN);

                break;

            case XML_ATTRIBUTE:      // <foo bar=...>
                if (pNodeInfo->dwSubType == XML_NS)
                {
                    if (0 == pNodeInfo->ulNsPrefixLen)
                    {
                        pNodeInfo->pReserved = _pMgr->createNameDef(XMLNames::pszXMLNSCOLON, 6, 5, null, XMLNames::atomURNXMLNS, XMLNames::atomURNXMLNS);
                        break;
                    }
                    pSrcURN = pAtomURN = XMLNames::atomURNXMLNS;
                }
                goto atomize;
                break;


            case XML_MODEL:
                if (pNodeInfo->dwSubType == XML_MIXED)   // (#pcdata).
                    pNodeInfo->pReserved = _pMgr->createNameDef(XMLNames::name(NAME_PCDATA));
                else
                    goto atomize;
                break;

            case XML_DOCTYPE:        // <!DOCTYPE  
                if (!_pDTD->isSchema())
                {
                    if (! _fIgnoreDTD)
                    {
                        // Then we will start with DTD node factory and then switch to 
                        // validation node factory.
                        _pFactory = _pDTDNodeFactory = new DTDNodeFactory(_pFactoryDefault, _pDTD, _pDoc);
                        _pFactory->Release(); // smart pointer addref'd it.
                    }
                }
                else
                {
                    hr = SCHEMA_DOCTYPE_INVALID;
                    goto CleanUp;
                }
                goto createurn;
        }

        hr = _pFactory->CreateNode(pNodeSource, pNodeParent, cNumRecs, apNodeInfo);
        if (FAILED(hr)) goto CleanUp;

        // Now that we have actually created the Node, we can fix the namespace scope
        // stack to use this node as the context so this namespace scope gets popped on 
        // EndChildren.
        if (fScoping && SUCCEEDED(hr) && pNodeInfo->pNode != null)
        {
            _pNodePending = pNodeInfo->pNode;
            _pMgr->changeContext(null, pNodeInfo->pNode);
        }

        // Process the attributes by calling createNode recurrsively as if
        // it was the old node factory interface.
        // BUGBUG -- this is the slow but mimimum change approach...
        if (cNumRecs > 1)
        {
            _fInAttribute = false;
            XML_NODE_INFO* pLastAttr = null;
            Node* pNode = (Node*)pNodeInfo->pNode;
            Node* pParent = pNode;
            apNodeInfo++;

            if (pNodeInfo->dwType == XML_PI)
            {
                // PI has PI + PCDATA in one createnode call, and so we
                // need to make sure we do the BeginChildren call for the
                // nodedatanodefactory.
                hr = BeginChildren(pNodeSource, pNodeInfo);
                if (FAILED(hr)) goto CleanUp;
            }

            for (long i = cNumRecs-1; i > 0; i--)
            {
                XML_NODE_INFO* pNodeInfo = *apNodeInfo;
                if (pNodeInfo->dwType == XML_ATTRIBUTE)
                {
                    if (_fInAttribute)
                    {
                        hr = EndChildren(pNodeSource, FALSE, pLastAttr);
                        if (FAILED(hr)) goto CleanUp;
                    }
                    pLastAttr = pNodeInfo;
                    _fInAttribute = true;
                    pParent = pNode; // reset parent back to element node

                    if (pNodeInfo->dwSubType == XML_XMLLANG)
                    {
                        bool valid = false;
                        String * pstr;
                        if (i > 1)
                        {
                            XML_NODE_INFO * pInfo = pNodeInfo;
                            int count = i - 1;
                            pstr = GetAttributeValue(&apNodeInfo[1], &count, _pDTD);
                            const WCHAR * pText = pstr->getWCHARPtr();
                            if (pText && isValidLanguageID(pText, pstr->length()))
                            {
                                valid = true;
                            }
                        }
                        else
                        {
                            pstr = String::emptyString();
                        }
                        if (! valid)
                        {
                            Exception::throwE(XML_XMLLANG_INVALIDID, XML_XMLLANG_INVALIDID, pstr, null);
                        }
                    }
                }
                hr = CreateNode(pNodeSource, pParent, 1, apNodeInfo);

                if (pNodeInfo->dwType == XML_ATTRIBUTE)
                {
                    // attributes are also parents of their child PCDATA nodes.
                    pParent = (Node*)pNodeInfo->pNode;
                }
                if (FAILED(hr)) goto CleanUp;
                apNodeInfo++;
            }
            if (pLastAttr) // set if we found an XML_ATTRIBUTE or an XML_PI
            {
                hr = EndChildren(pNodeSource, FALSE, pLastAttr);
            }
            _fInAttribute = false;
        }

        if (fLoadedSchema && _pDoc->hasPendingChildDocs())
        {
            // The child document will wake up this document
            // when it's done.
            hr = _pDoc->startNextPending() ? E_PENDING : S_OK;
            if (_pDoc->GetLastError() != S_OK) // unless we hit an error already !
                hr = _pDoc->GetLastError(); // in which case stop right away.
        }

    }
    CATCH
    {
        pNodeInfo->pNode = NULL; // don't return anything then !!
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDoc);
    }
    ENDTRY

CleanUp:
    return hr;
}

HRESULT
AbortParse(IXMLNodeSource * pNodeSource, Exception * e, Document * pDoc)
{
    String* msg = e->getMessage();
    if (msg != null)
    {
        BSTR bstr = msg->getBSTR();
        pNodeSource->Abort(bstr);
        ::SysFreeString(bstr);
        pDoc->setLastError(e);
    }    
    return e->getHRESULT();
}

bool
NameSpaceNodeFactory::ProcessXMLNSAttributes(
    IXMLNodeSource* pNodeSource, 
    USHORT count,
    XML_NODE_INFO** apNodeInfo,
    bool& fLoadedSchema)
{
    bool fScoping = false;
    int i = count;
    while (i > 0)
    {
        XML_NODE_INFO* pInfo = *apNodeInfo++;
        // check xmlns attributes ( PrefixDef ::= 'xmlns' (':' NCName)? )
        if (XML_ATTRIBUTE == pInfo->dwType && 
            XML_NS == pInfo->dwSubType && 
            0 != pInfo->ulLen)
        {
            Atom *pPrefix, *pURN;
        
            //
            // get prefix
            //
            if (0 == pInfo->ulNsPrefixLen)
            {
                // default namespace: xmlns="urn..."
                pPrefix = null;
            }
            else
            {
                pPrefix = Atom::create(pInfo->pwcText + 6, pInfo->ulLen - 6);
            }

            //
            // get urn
            //

            i--;
            String* s;
            if (i > 0)  // in case it is was an empty attribute: xmlns=""
            {
                // BUGBUG - need to resolve the url to a full 
                // path so that the URN is the same no matter how
                // we get there.  But this is very difficult to do
                // because of HTTP redirection.

                // normalization, and handle entity references
                int j = i;
                s = GetAttributeValue(apNodeInfo, &i, _pDTD);
                apNodeInfo += (j-i);
                i++;
            }
            else
            {
                s = String::emptyString();
            }

#if DBG == 1
            char * pchURN = (char*)AsciiText(s);
#endif

            // AttValue may be empty only if the PrefixDef is simply "xmlns", 
            // i.e. is declaring a default namespace
            if (String::emptyString() == s && pPrefix)
            {
                Exception::throwE(XML_NAMESPACE_URI_EMPTY, XML_NAMESPACE_URI_EMPTY, null);
            }

            Atom * pSrcURN = Atom::create(s);
            pURN = NamespaceMgr::CanonicalURN(pSrcURN);
            _pMgr->pushScope(pPrefix, pURN, pSrcURN, null);

            fScoping = true;
            const WCHAR* pwcText = s->getWCHARPtr();

            // we only load the schema iff the URL starts with the string "x-schema:"
            if (!_fIgnoreDTD && !_fHasDTD && _pDoc->getResolveExternals() &&
                    s->length() > _cSchemaPrefixLen &&
                    (0 == ::memcmp(pwcText, XMLNames::pszSchemaURLPrefix, 
                                sizeof(TCHAR)*_cSchemaPrefixLen)))
            {
                LoadSchema(pNodeSource, pSrcURN);
                fLoadedSchema = true;
            }
        }
        i--;
    }
    return fScoping;
}


void
NameSpaceNodeFactory::LoadSchema(IXMLNodeSource* pNodeSource, Atom* pURN)
{
    if (! _pDTD->hasSchema(pURN))
    {
        const WCHAR* pwcBaseURL;
        WCHAR* pwcBuffer = NULL;
        Document * pDoc;

        GetBaseURL(pNodeSource, _pDoc, &pwcBaseURL, &pwcBuffer);

        TRY
        {
            ParserFlags flags;
            _pDTD->setSchema(true); // mark the DTD as being for a schema.
                // so that we don't try and "validate" the schema itself.

            pDoc = _pDoc->createChildDoc(pwcBaseURL,pURN,flags);
            pDoc->setURN(pURN);
            pDoc->setReadOnly(true);
        }
        CATCH
        {
            delete [] pwcBuffer;
            Exception::throwAgain();
        }
        ENDTRY

        delete [] pwcBuffer;

        // Now replace entry in hashtable with actual schema document so we can
        // find the schema information for the definition method later when we need it.
        _pDTD->addSchemaURN(pURN, (Object*)((GenericBase*)pDoc));

        Exception *e = pDoc->getErrorMsg();
        if (e)
            e->throwE();

        // We are now loading a schema, so let's get ready for validation !!
        // (if we are not already).
        if (_pValidationFactory && _pFactory != _pValidationFactory)
        {
            // now we can switch to the validation node factory.
            _pFactory = _pValidationFactory;
            _pFactory->NotifyEvent(pNodeSource, XMLNF_STARTDTD);
        }

        // notify _pFactory that a schema is found
        _pFactory->NotifyEvent(pNodeSource, (XML_NODEFACTORY_EVENT)XMLNF_STARTSCHEMA);
    }
    else
    {
        // if this schema is already pending for a parent document, then
        // move it to this document so we can process it now !
        Document* schema = _pDTD->getSchema(pURN);
        if (schema && schema->isPending())
        {
            _pDoc->adoptChild(schema);
        }
    }
}

String*
GetAttributeValue(XML_NODE_INFO** apNodeInfo, int * count, DTD * pDTD)
{
    bool fInAttrValue = true;
    int i = *count;
    WSStringBuffer * psb = WSStringBuffer::newWSStringBuffer(32);

    do
    {
        XML_NODE_INFO* pInfo = *apNodeInfo++;
        switch (pInfo->dwType)
        {
        case XML_PCDATA:
            psb->append(pInfo->pwcText, pInfo->ulLen, WSStringBuffer::WS_COLLAPSE);
            i--;
            break;
        case XML_ENTITYREF:
            {
                Name * pName = Name::create(pInfo->pwcText, pInfo->ulLen);
                pDTD->checkEntityRef(pName, null, true);
                Entity * pEnt = pDTD->findEntity( pName, false);
                Assert(pEnt);
                Node * pNode = (Node*)((Object*)(pEnt->_pNode));
                Assert(pNode);
                psb->append(pNode->getInnerText(), WSStringBuffer::WS_COLLAPSE);
            }
            i--;
            break;
        default:
            fInAttrValue = false;
            break;
        }
    } while (fInAttrValue && i > 0);

    *count = i; 
    return psb->toString();
}


HRESULT
NameSpaceNodeFactory::HandlePending()
{
    HRESULT hr = S_OK;
    void* pTag;
    Node* attr;

    attr = ((Node*)_pNodePending)->getNodeFirstAttribute(&pTag);
    while (attr)
    {
        // Call notifyNew on node to set attribute type
        attr->notifyNew(NULL, true, true);
        attr = ((Node*)_pNodePending)->getNodeNextAttribute(&pTag);
    }

    _pNodePending = null;

Cleanup:
    return hr;
}

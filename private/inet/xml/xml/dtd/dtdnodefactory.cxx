/*
 * @(#)DTDNodeFactory.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "dtdnodefactory.hxx"
#include "domnode.hxx"
#include "document.hxx"
#include "xmlnames.hxx"
#include "chartype.hxx"
#include "xml/tokenizer/parser/xmlparser.hxx"


void GetBaseURL(IXMLNodeSource * pSource, Document * pDocument, const WCHAR **ppBaseURL, WCHAR ** pBuffer)
{
    pSource->GetURL(ppBaseURL);
    if (*ppBaseURL == NULL || **ppBaseURL == 0)
    {
        // Then use the document's base URL instead.
        String* base = pDocument->GetBaseURL();
        if (base)
        {
            *ppBaseURL = *pBuffer = base->getWCHARBuffer();
        }
    }
}

XMLParser* _getParser(IXMLNodeSource* pSource)
{
    XMLParser* pParser = NULL;
    HRESULT hr = pSource->QueryInterface(IID_Parser, (void**)&pParser);
    if (hr)
        Exception::throwE(hr);
    Assert(pParser != null);
    return pParser;
}

// ###
// ### THIS NODE FACTORY MUST NOW BE WRAPPED BY THE NAMESPACE NODE FACTORY BECAUSE
// ### IT NO LONGER DOES TRY/CATCH OR STACK_ENTRY.
// ###

//=====================================================================================
DTDNodeFactory::DTDNodeFactory(IXMLNodeFactory * fc, DTD* dtd, Document * document)
{
    init();
    _pFactory = fc;
    _pDTD = dtd;
    _pDocument = document;
    _pNamespaceMgr = document->getNamespaceMgr();
    _cDepth = 0;
}

DTDNodeFactory::~DTDNodeFactory()
{
    _pFactory = null;
    _pDTD = null;
//    _pContexts = null;
//    _pCurrent = null;
    _pAttDef = null;
    _pEntity = null;
    _pElementDecl = null;
    _pNotation = null;
    _pDocTypeNode = null;
    _pEntityEnumeration = null;
    _pCurrentElement = null;
    _pDeclNamedef = null;
    _pDocument = null;
    _pNamespaceMgr = null;
}

void DTDNodeFactory::init()
{
//    _pContexts = new Stack();
//    _pCurrent = null;
    _pAttDef = null;

    // DTD building state.
    _lInDTD = 0;
    _pfn = &DTDNodeFactory::BuildDecl;
    _pEntity = null;
    _pElementDecl = null;
    _pNotation = null;

    _fInAttribute = false;
    _fParsingEntity = false;
    _pDocTypeNode = null;
    _pEntityEnumeration = null;
    _pCurrentElement = null;
    _fInElementDecl = false;
    _fBuildDefNode = false;
    _nState = eProlog;
    _dwAttrType = 0;
    _fBuildNodes = false;

    _pDeclNamedef = null;
}

///////////////////////////////////////////////////////////////////////////////////
// IXMLNodeFactory Interface Implementation
//
HRESULT STDMETHODCALLTYPE 
DTDNodeFactory::NotifyEvent( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
    HRESULT hr = S_OK;

    switch (iEvt)
    {
    case XMLNF_STARTDOCUMENT:
        init();
        break;
    case XMLNF_STARTDTD:
    case XMLNF_STARTDTDSUBSET:
        _lInDTD++;
        _nState = eDTD;
        break;
    case XMLNF_ENDDTD:
    case XMLNF_ENDDTDSUBSET:
        _lInDTD--;
        if (_lInDTD == 0) _nState = eValidating;
        break;
    case XMLNF_ENDPROLOG:
        _pDTD->checkForwardRefs(); // for any forward references within the DTD itself.

        //BUGBUG: We do not call EndChildren and EndAttributes on ElementDecl->_pNode for now
        //        We may have to do that in the future when those calls do more

        // No we've finished both the internal subset and external DTD's, so
        // ow is the right time to go back and parse the entities.
        if (hr == S_OK && _lInDTD == 0 && ! _fParsingEntity)
        {
            _pEntityEnumeration = _pDTD->entityDeclarations(false);
            _fParsingEntity = false;
            hr = _parseEntities(pSource);
            if (FAILED(hr)) goto CleanUp;        
        }

        // If there was no DTD then the state will still be eProlog,
        // in which case we can now switch to the super fast eNone state.
        // Actually we could even remove the DTD node factory at this point.
        if (_nState == eProlog)
        {
            _nState = eNone;
        }
        if (_nState == eEntities)
        {
            // we are loading entities
            hr = S_FALSE;
            goto CleanUp;
        }
        goto fixstuff;
        break;

    case XMLNF_ENDENTITY:
        _fParsingEntity = false;
        _nState = eValidating;

        if (_pEntity != NULL && _pEntity->_pNode != null)
        {
            Node* pNode = ((Node *)(Object *)(_pEntity->_pNode));
            pNode->setFinished(true);
            pNode->setReadOnly(true, true);
            _pEntity = null;
            hr = _parseEntities(pSource);
        }
        if (FAILED(hr)) goto CleanUp;

        if (_nState == eValidating)
        {
            iEvt = XMLNF_ENDPROLOG;
fixstuff:
            // Check infinite loops in entity references
            checkEntityRefLoop();

            // Now that we've finished parsing the entities we can fix the attdefs
            hr = FinishAttDefs();
            if (FAILED(hr)) goto CleanUp;

            // And now we need to fix the namedef-s in the DTD
            if (_pDTD->needFixNames())
            {
                FixNamesInDTD();
            }

            Assert(_pDocTypeNode != NULL);
            CAST_TO(Node*, (Object*)_pDocTypeNode)->setFinished(true);
        }
        break;
    }

    hr = _pFactory->NotifyEvent(pSource, iEvt);

    if (iEvt == XMLNF_ENDENTITY && _nState == eEntities)
    {
        hr = S_FALSE;
    }

CleanUp:
    return hr;
}

HRESULT STDMETHODCALLTYPE DTDNodeFactory::Error( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ HRESULT hrErrorCode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
    if (_fParsingEntity && _pEntity)
    {
        _fParsingEntity = false;
        BSTR bstr = null;
        pSource->GetErrorInfo(&bstr);
        String* msg = Resources::FormatMessage(
                    XML_PARSING_ENTITY, _pEntity->getName()->toString(), String::emptyString(), null);
        if (bstr != null)
        {
            msg = String::add(msg, String::newString(bstr), null);
            ::SysFreeString(bstr);
        }
        bstr = msg->getBSTR();
        pSource->Abort(bstr);
        ::SysFreeString(bstr);
    }
    return _pFactory->Error(pSource, hrErrorCode, cNumRecs, apNodeInfo);
}


HRESULT STDMETHODCALLTYPE DTDNodeFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
    HRESULT hr = S_OK;

    // This ocde is organized to maximize speed for the eNone case.

    switch (_nState)
    {
    case eDTD:
//            _pAttDef = null; // cannot be nulled out until endchildren.
        _fInAttribute = false;
        // no delegation to sub node factory required in this case.
        break;

    case eAttribute:
        _nState = eValidating;
        // fall through
    case eValidating:
        _pAttDef = null;
//        _pCurrentElement = null;
        _fInAttribute = false;
        // fall through
    case eEntities:
    case eDocType:
    case eNone:
    case eProlog:
        hr =  _pFactory->BeginChildren(pSource, pNodeInfo);
        break;
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE DTDNodeFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
    HRESULT hr = S_OK;

    _cDepth--;
    DWORD dwType = pNodeInfo->dwType;
    
    // This ocde is organized to maximize speed for the eNone case.

    switch (_nState)
    {
    case eDTD:
        if (dwType == XML_DTDATTRIBUTE)
        {
            _fInAttribute = false;
            if (_fBuildNodes)
            {
                goto echild;
            }
        }
        else if (dwType == XML_GROUP)
        {
            if (_fInElementDecl)
                _pElementDecl->_pContent->closeGroup();
            else
                _nAttDefState = 1;
        }
        else if (dwType == XML_DOCTYPE)
        {
            goto doctype;
        }
        else 
        {                
            if (XML_ELEMENTDECL == dwType)
            {
                _pElementDecl->_pContent->finish();
                _fInElementDecl = false;
                _pElementDecl = NULL;
                _pDTD->setInElementdeclContent(false);
            }
            else if (XML_ATTLISTDECL == dwType)
            {
                if (null != (void*)_pAttDef)
                    _pAttDef->checkComplete(_pElementDecl);
            }

            if (_fBuildDefNode)
            {
                _fBuildDefNode = false;
                _endChildren(_pFactory, pSource, (Object*)(_pAttDef->_pDefNode),
                    XML_ATTRIBUTE, FALSE, NULL);
                _pAttDef = null;
            }
            else if (_fBuildNodes)
            {
                _fBuildNodes = false;
                _pfn = &DTDNodeFactory::BuildDecl;
                goto echild;
            } 

            // Ok, we finished the current declaration.
            _pfn = &DTDNodeFactory::BuildDecl;
            _dwAttrType = 0;
        }
        break;

    case eAttribute:
        _nState = eValidating;
        // fall through
    case eValidating:
        switch (dwType)
        {
//        case XML_ATTRIBUTE:
        case XML_DTDATTRIBUTE:
            _fInAttribute = false;
            break;
        case XML_PI:
            break;
        case XML_DOCTYPE:
            goto doctype;
        }

        goto echild;
        
    case eEntities:
        goto echild;

    case eProlog:
    case eDocType:
        if (dwType == XML_DOCTYPE)
        {
doctype:
            _nState = eValidating;
            _dwAttrType = 0;

            if (_pDTDHref != null && _pDocument->getResolveExternals())
            {
                // According to the XML Language Spec, the external DTD must
                // come AFTER the internal subset so that things in the
                // internal subset take precidence.
                const WCHAR* baseURL;
                WCHAR* buffer = NULL;
                GetBaseURL(pSource, _pDocument, &baseURL, &buffer);
                ATCHAR* aszURL = _pDTDHref->toCharArrayZ();
                XMLParser* pParser = _getParser(pSource);
                hr = pParser->LoadDTD(baseURL, aszURL->getData());
                pParser->Release();
                delete [] buffer;
                if (FAILED(hr)) goto CleanUp;
            }
            _pDTDHref = null;
        }
        // fall through.

    case eNone:
echild:
        hr = _pFactory->EndChildren(pSource, fEmptyNode, pNodeInfo);
        break;
    }

CleanUp:

    return hr;
}

HRESULT STDMETHODCALLTYPE DTDNodeFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
    HRESULT hr = S_OK;

    XML_NODE_INFO* pNodeInfo = *apNodeInfo; // there's always at least one record.

    if (! pNodeInfo->fTerminal) _cDepth++;

    // this code is optimized for the eNone case.

    switch (_nState)
    {
    case eEntities:
        // Fix up the parent

        if ( (!pNodeInfo->fTerminal && _cDepth == _s_cDepth + 1) || (pNodeInfo->fTerminal && _cDepth == _s_cDepth))
            pNodeParent = _pEntity->_pNode;

        if (pNodeInfo->dwType == XML_ENTITYREF)
        {
            NameDef* name = (NameDef*)pNodeInfo->pReserved;
            Assert(name);
            Name* n = name->getName();
            Entity* en = _pDTD->findEntity(n, false);
            _pDTD->checkEntity(en, n, _fInAttribute);
        }
        hr = _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
        break;

    case eProlog:
        // Currently no validation required, but we still need to remember the doctype name,
        // and check the XML version number, and check to make sure we don't get undefined
        // entity references.
        switch (pNodeInfo->dwType)
        {
        case XML_DOCTYPE:
            _pDTD->enableValidation();  // from here on we require strict DTD validation
            _pDTD->setDocType((NameDef*)pNodeInfo->pReserved);
            _nState = eDocType;
            break;
        case XML_ENTITYREF:
basicent:
            // There is no element declaration, but still need to check that
            // the entityref exists, and that it is not a parameter entity.
            // (and don't do this for built in entities (ie non-terminal entities).
            if (pNodeInfo->fTerminal)
            {
                NameDef* name = (NameDef*)pNodeInfo->pReserved;
                Assert(name);
                // BUGBUG -- we were doing some validation DURING DTD BUILDING so
                // we don't have a DTDState yet.
                _pDTD->checkEntityRef(name->getName(), null, _fInAttribute);
            }
            break;
        }
        hr = _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);

        if (SUCCEEDED(hr))
        {
            if (pNodeInfo->dwType == XML_DOCTYPE)
            {
                _pDocTypeNode = (Node*)pNodeInfo->pNode;
            }
        }
        break;

    case eDocType:
        // We are inside the <!DOCTYPE tag, so we need to simulate attributes on the doctype
        // node so this information is available through the object model.
        if (pNodeInfo->dwType == XML_DTDATTRIBUTE)
        {
            _dwAttrType = pNodeInfo->dwSubType;
        }
        else if (pNodeInfo->dwType == XML_PCDATA)
        {
            if (_dwAttrType == XML_SYSTEM)
            {
                _pDTDHref = String::newString(pNodeInfo->pwcText, 0, pNodeInfo->ulLen); // save this for EndChildren.
            }
            else // need to check PublicID
            {
                if (!isValidPublicID(pNodeInfo->pwcText, pNodeInfo->ulLen))
                {
                    Exception::throwE(XML_PUBLICID_INVALID, XML_PUBLICID_INVALID, 
                                      String::newString(pNodeInfo->pwcText, 0, pNodeInfo->ulLen), null);
                }
            }
        }
        goto createnode;
        break;

    case eValidating:
        if (pNodeInfo->dwType == XML_ATTRIBUTE)    // need to remember when we're inside an attribute or not.
        {
            _fInAttribute = true;
            _nState = eAttribute;
        }

createnode:
        hr = _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
        break;

    case eAttribute:
        if (pNodeInfo->dwType == XML_ENTITYREF)
        {
            goto basicent;
        }
        goto createnode;
        break;

    case eDTD:
        if (pNodeInfo->dwType == XML_DTDSUBSET)
            goto createnode;

        if (pNodeInfo->dwType == XML_ATTRIBUTE || 
            pNodeInfo->dwType == XML_DTDATTRIBUTE )    // need to remember when we're inside an attribute or not.
        {
            _fInAttribute = true;
            // but we don't switch to eAttribute state because that is only for validation
            // which we are not doing yet -- we are currently still parsing the DTD.
        }

        if (_cDepth == 0)
        {
            // Since the DocType node ended before we called LoadDTD we have to
            // fix up the parent here so that we add ENTITY element to the doctype
            // element and not to the document root element.
            pNodeParent = _pDocTypeNode;
        }

        // Parameter entities are handled separately.
        if (pNodeInfo->dwType == XML_PEREF)
        {
            NameDef* name = (NameDef*)pNodeInfo->pReserved;
            hr = HandlePERef(pSource, name->getName());
        }
        else
        {
            NameDef* name = (NameDef*)pNodeInfo->pReserved;
            String* text = String::newString(pNodeInfo->pwcText, 0, pNodeInfo->ulLen);
            hr = (this->*_pfn)(pSource, pNodeParent, pNodeInfo->dwType, pNodeInfo->dwSubType, 
                text, name, &(pNodeInfo->pNode));
            if (SUCCEEDED(hr))
            {
                if (_fBuildDefNode)
                {
                    // must be a XML_NAME, XML_NMTOKEN, XML_PCDATA or XML_ENTITYREF node.
                    hr = _pFactory->CreateNode(pSource, _pAttDef->_pDefNode, cNumRecs, apNodeInfo);
                }
                else if (_fBuildNodes)
                {
                    if (pNodeInfo->pNode == NULL)
                    {
                        hr = _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
                    }
                }
            }
        }
        break;
    }

CleanUp:

    return hr;
}

#if 0
/********* not any more :-)
HRESULT
DTDNodeFactory::ExpandEntity(IXMLNodeSource* pSource, Name* name)
{
    HRESULT hr;

    // This needs to be expanded so that we can parse the names
    // in the expanded entity text.
    Entity* en = _pDTD->findEntity(name, false);
    hr = _pDTD->checkEntity(en, name);
    if (hr == S_OK)
    {
    // If the entity is parsed already we can avoid reparsing it by putting
    // the text back together.  The problem with this is it makes the simple
    // case slower
    //    String* text = en->_pNode->getInnerText(true, true);
        String* text = en->getText();
        ATCHAR* atext = text->toCharArray();
        
        XMLParser* pParser = _getParser(pSource);
        hr = pParser->ExpandEntity(atext->getData(), text->length());
        pParser->Release();
    }
    return hr;
}
*********/
#endif

HRESULT 
DTDNodeFactory::HandlePERef(IXMLNodeSource* pSource, Name* name)
{
    HRESULT hr = S_OK;
    XMLParser* pParser = NULL;

    Entity* en = _pDTD->findEntity(name, true);
    if (! en)
    {
        hr = XML_E_MISSING_PE_ENTITY;
    }
    else
    {
        pParser = _getParser(pSource);

        // Now parse the parameter entity in place.
        String* text = en->getText();
        if (text != null)
        {
            hr = pParser->ParseEntity(text->getWCHARPtr(), text->length(), TRUE);
        }
        else if (_pDocument->getResolveExternals() && (text = en->getURL()) != null)
        {
            const WCHAR* baseURL;
            pSource->GetURL(&baseURL);
            RATCHAR aszURL = text->toCharArrayZ();
            hr = pParser->LoadEntity(baseURL, aszURL->getData(), TRUE);
        }
    }
CleanUp:
    if (pParser)
        pParser->Release();
    return hr;
}

HRESULT  DTDNodeFactory::BuildDecl( 
    /* [in] */ IXMLNodeSource* pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD dwSubType,
    /* [in] */ String* str,
    /* [in] */ NameDef* namedef,
    /* [out] */ PVOID* ppNodeChild)
{
    HRESULT hr = S_OK;
    bool pe = false;
    Name * name = null;

    switch (dwType)
    {
    case XML_ENTITYDECL:
        name = namedef->getName();
        pe = (dwSubType == XML_PENTITYDECL);
        _pEntity = new Entity(name, pe); 
        _pEntity->setPosition(pSource->GetLineNumber(), pSource->GetLinePosition());
        _pDeclNamedef = namedef;
        _pfn = &DTDNodeFactory::BuildEntity;
        // If the entity is not declared, we add the declaration to DTD.
        // Otherwise, we parse and then discard the later declarations
        if (_pDTD->findEntity(name, pe) == null)
        {
            _pDTD->addEntity(_pEntity);
        }

        if (!pe)
        {
            // Create an empty ENTITY node as a place holder for the
            // parsed entity value.  We can't populate it till later.
            _createDTDNode(_pFactory, pSource, _pDocTypeNode, XML_ENTITYDECL, 
                        true, false, NULL, 0, 0, namedef, (PVOID*)ppNodeChild);

            // so that it gets the SYSTEM and PUBLIC attributes also.
            _pEntity->_pNode = (Node*)(*ppNodeChild);
            _fBuildNodes = true;
        }
        break;
    case XML_ELEMENTDECL:
        name = namedef->getName();
        _pElementDecl = _pDTD->findElementDecl(name);
        if (_pElementDecl != null)
        {
            Exception::throwE(XML_ELEMENT_DEFINED, 
                XML_ELEMENT_DEFINED, namedef->toString(), null);
        }
        else
        {
            _pElementDecl = _pDTD->createDeclaredElementDecl(namedef);
        }
        _pElementDecl->_pContent = ContentModel::newContentModel();
        _pElementDecl->_pContent->start();
        _pElementDecl->_pContent->setType(ContentModel::ELEMENTS); // default
        _fInElementDecl = true;
        _pDTD->addElementDecl(_pElementDecl);
        _pDTD->setInElementdeclContent(true);
        _pfn = &DTDNodeFactory::BuildElementDecl;
        break;
    case XML_ATTLISTDECL:
        name = namedef->getName();
        _pElementDecl = _pDTD->findElementDecl(name);
        if (_pElementDecl == NULL)
        {
            _pElementDecl = _pDTD->createUndeclaredElementDecl(namedef);
        }
        _pfn = &DTDNodeFactory::BuildAttDefs;
        break;
    case XML_NOTATION:
        name = namedef->getName();
        _pNotation = _pDTD->findNotation(name);
        if (_pNotation != null)
        {
            Exception::throwE(XML_NOTATION_DEFINED, 
                XML_NOTATION_DEFINED, namedef->toString(), null);
        }
        _pNotation = new Notation(name);
        _pDTD->addNotation(_pNotation);

        _createDTDNode(_pFactory, pSource, _pDocTypeNode, XML_NOTATION, 
                    false, false, NULL, 0, 0, namedef, ppNodeChild);

        // leave _pNotation->_pNode open until EndChildren.            
        _fBuildNodes = true;
        _pNotation->_pNode = (Node*)(*ppNodeChild);
        _pfn = &DTDNodeFactory::BuildNotation;
        break;
    case XML_IGNORESECT:
    case XML_INCLUDESECT:
        break;
    default:
        Assert("Error");
        break;
    }

CleanUp:
    return hr;
}


HRESULT  DTDNodeFactory::BuildEntity( 
    /* [in] */ IXMLNodeSource* pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD dwSubType,
    /* [in] */ String* str,
    /* [in] */ NameDef* namedef,
    /* [out] */ PVOID* ppNodeChild)
{
    HRESULT hr = S_OK;

    switch (dwType)
    {
    case XML_STRING: 
        _pEntity->setText(str);
        break;
    case XML_DTDATTRIBUTE:
        _dwAttrType = dwSubType;
        break;
    case XML_PCDATA:
        switch (_dwAttrType)
        {
        case XML_PUBLIC:
            _pEntity->pubid = str;
            break;
        case XML_SYSTEM:
            _pEntity->setURL(str);
            break;
        default:
            Assert("Error");
            break;
        }
        _dwAttrType = 0;
        break;
    case XML_NAME:
        if (_dwAttrType == XML_NDATA)
        {
            if (_pEntity->par)
                return XML_NDATA_INVALID_PE;
            Name* name = namedef->getName();
            if (! _pDTD->findNotation(name))
            {
                _pDTD->addForwardRef(_pDeclNamedef, name, 
                    pSource->GetLineNumber(), pSource->GetLinePosition(),
                    false, DTD::REF_NOTATION);
            }
            _pEntity->setNDATA(namedef->getName());
            break;
        }
        _dwAttrType = 0;
        break;
    default:
        Assert("Error");
        break;

    }
    return hr;
}



HRESULT  DTDNodeFactory::BuildElementDecl( 
    /* [in] */ IXMLNodeSource* pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD dwSubType,
    /* [in] */ String* str,
    /* [in] */ NameDef* namedef,
    /* [out] */ PVOID* ppNodeChild)
{
    switch (dwType)
    {
    case XML_GROUP:
        _pElementDecl->_pContent->openGroup();
        break;
    case XML_NAME:
        _pElementDecl->_pContent->addTerminal(namedef->getName());
        break;
    case XML_MODEL:
        switch (dwSubType)
        {
        case XML_EMPTY: 
            _pElementDecl->_pContent->setType(ContentModel::EMPTY);
            break;
        case XML_ANY:
            _pElementDecl->_pContent->setType(ContentModel::ANY);
            break;
        case XML_MIXED:
            _pElementDecl->_pContent->addTerminal(namedef->getName());
            break;
        case XML_SEQUENCE:
            _pElementDecl->_pContent->addSequence();
            break;
        case XML_CHOICE:
            _pElementDecl->_pContent->addChoice();
            break;
        case XML_STAR:
            _pElementDecl->_pContent->star();
            break;
        case XML_PLUS:
            _pElementDecl->_pContent->plus();
            break;
        case XML_QUESTIONMARK:
            _pElementDecl->_pContent->questionMark();
            break;
        }
        break;
    }

    return S_OK;
}

HRESULT  DTDNodeFactory::BuildNotation( 
    /* [in] */ IXMLNodeSource* pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD dwSubType,
    /* [in] */ String* str,
    /* [in] */ NameDef* namedef,
    /* [out] */ PVOID* ppNodeChild)
{
    switch (dwType)
    {
    case XML_DTDATTRIBUTE:
        _dwAttrType = dwSubType;
        break;
    case XML_PCDATA:
        switch (_dwAttrType)
        {
        case XML_PUBLIC:
            _pNotation->pubid = str;
            break;
        case XML_SYSTEM:
            _pNotation->url = str;
            break;
        }
        _dwAttrType = 0;
        break;
    }
    return S_OK;
}

HRESULT  DTDNodeFactory::BuildAttDefs( 
    /* [in] */ IXMLNodeSource* pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD dwSubType,
    /* [in] */ String* str,
    /* [in] */ NameDef* namedef,
    /* [out] */ PVOID* ppNodeChild)
{
    HRESULT hr = S_OK;
    Name * name = namedef ? namedef->getName() : null;

    switch (dwType)
    {
    case XML_ATTDEF:
    {
        _fBuildDefNode = false;
        if (_pAttDef)
        {
            Node* node = (Node*)(Object*)_pAttDef->_pDefNode;
            if (node != null)
            {
                // finish previous defnode.
                _endChildren(_pFactory, pSource, node, XML_ATTRIBUTE, FALSE, NULL);
            }
            _pAttDef->checkComplete(_pElementDecl);
        }
        _pAttDef = _pElementDecl->getAttDef(name);
        bool exists = (_pAttDef != null);
        _pAttDef = AttDef::newAttDef(name, DT_NONE);
        _pDeclNamedef = namedef;
        if (! exists)
        {
            _pElementDecl->addAttDef(_pAttDef);
        }
        _nAttDefState = 0;
        break;
    }
    case XML_NAME:
    case XML_NMTOKEN:
        // The parser still parses the ATTLIST enumeration and returns
        // XML_NAME or XML_NMTOKENs.  And if this is a NOTATION enumeration 
        // the notations must be defined.
        if (_nAttDefState == 0)
        {
            if (_pAttDef->_datatype == DT_AV_NOTATION)
            {
                if (_pDTD->findNotation(name) == null)
                {
                    _pDTD->addForwardRef(_pDeclNamedef, name, 
                        pSource->GetLineNumber(), pSource->GetLinePosition(),
                        false, DTD::REF_NOTATION);
                }
            }
            _pAttDef->addValue(name);
        }
#if 0
/*************** not used any more since tokenizer no longer processes attribute values
                 differently based on DTD information.  This code is moved to
                 AttDef::checkAttributeType.
        else
        {

            _fBuildDefNode = true;
            switch (_pAttDef->type)
            {
            case Element::AV_IDREFS:
            case Element::AV_ENTITIES:
            case Element::AV_NMTOKENS:
            {
                if (_pAttDef->def == null)
                {
                    _pAttDef->def = Vector::newVector();
                }
                Vector* v = (Vector *)(Object *)(_pAttDef->def);
                v->addElement(namedef->getName());
                break;
            }
            default:
                Assert(_pAttDef->def == null);
                _pAttDef->def = namedef->getName();
                _pAttDef->checkEnumeration(namedef->getName());
            };
        }
****************/
#endif
        break;
       
    case XML_PCDATA:    // for CDATA attributes...
        // _pAttDef->def is now initialized after we finish parsing the default string.
        if (_pAttDef->getType() == DT_AV_ID)
        {
            hr = XML_ATTLIST_ID_PRESENCE;   // ID attributes cannot have default values.
            goto CleanUp;
        }
        _fBuildDefNode = true;
        break;

    case XML_ATTTYPE:
        switch (dwSubType)
        {
        case XML_AT_ID:
            if (_pElementDecl->isIDDeclared())
            {
                hr = XML_ATTLIST_DUPLICATED_ID;
                goto CleanUp;
            }
            _pElementDecl->setIDDeclared(true);
            //fall through
        case XML_AT_CDATA:
        case XML_AT_IDREF:
        case XML_AT_IDREFS:
        case XML_AT_ENTITY:
        case XML_AT_ENTITIES:
        case XML_AT_NMTOKEN:
        case XML_AT_NMTOKENS:
            _nAttDefState = 1;    // now we expect the default value.
        case XML_AT_NOTATION:
            // Both sets of types are contiguous.
            _pAttDef->_datatype = (DataType)(dwSubType - XML_AT_CDATA + DT_AV_CDATA);
            break;
        }
        break;

    case XML_GROUP:
        if (_pAttDef->getType() != DT_AV_NOTATION)
            _pAttDef->_datatype = DT_AV_ENUMERATION;
        break;
    case XML_ATTPRESENCE:
        switch (dwSubType)
        {
        case XML_AT_REQUIRED:
            _pAttDef->_presence = AttDef::REQUIRED;
            _nAttDefState = 1;    // now we expect the default value.
            break;
        case XML_AT_IMPLIED:
            _pAttDef->_presence = AttDef::IMPLIED;
            _nAttDefState = 1;    // now we expect the default value.
            break;
        case XML_AT_FIXED:
            if (_pAttDef->getType() ==  DT_AV_ID)
            {
                hr = XML_ATTLIST_ID_PRESENCE;
                goto CleanUp;
            }
            _pAttDef->_presence = AttDef::FIXED;
            _nAttDefState = 1;    // now we expect the default value.
            break;
        }
        break;
    case XML_ENTITYREF: 
        // The XML Language spec says that general entity references in a default
        // value MUST be defined BEFORE the ATTLIST - so we can check this now.
        // Section 4.1 "Well-formedness Constraint: Entity Declared"        
        _fBuildDefNode = true;        
        {
            Entity* en = _pDTD->findEntity(name, false);
            _pDTD->checkEntity(en, name, true);
        }
        break;
    }

    if (_fBuildDefNode && _pAttDef->_pDefNode == NULL)
    {
        PVOID pNode;
        if (_pElementDecl->_pNode == null)
        {
             _createDTDNode(_pFactory, pSource, NULL, XML_ELEMENT, false, false, NULL, 0, 0, 
                     _pElementDecl->getNameDef(), &pNode);
             _pElementDecl->_pNode = (Node*)pNode;

             // Close it right away so that createNode/endChildren calls are balanced.
             // We have to do this here because we don't know how many <!ATTLISTs
             // we are going to get for this _pElementDecl.
             _endChildren(_pFactory, pSource, (Object*)(_pElementDecl->_pNode),
                            XML_ELEMENT, TRUE, NULL);
        }
        // Create a node to hold the default value nodes
        _createDTDNode(_pFactory, pSource, _pElementDecl->_pNode, XML_ATTRIBUTE, true, false, NULL, 0, 0, 
                    _pDeclNamedef, &pNode);
        _pAttDef->_pDefNode = (Node*)pNode;
        ((Node*)(Object*)_pAttDef->_pDefNode)->setSpecified(false);
    }

CleanUp:
    return hr;
}


HRESULT
DTDNodeFactory::_parseEntities(IXMLNodeSource* pSource)
{
    HRESULT hr = S_OK;

    if (_pEntityEnumeration == NULL)
        goto CleanUp;

    // Search for parsed non-parameter entities
    while (_pEntityEnumeration->hasMoreElements()) 
    {
        _pEntity = ICAST_TO(Entity *, _pEntityEnumeration->nextElement());
         
        if (!_pEntity->ndata && !_pEntity->isParsed())
        {
            hr = _parseEntity(pSource);
            if (FAILED(hr))
            {
                // return error !!
                goto CleanUp;
            }
            if (_fParsingEntity)
            {
                // we found one !!
                goto CleanUp;
            }
        }

        CAST_TO(Node *, (Object*)_pEntity->_pNode)->setFinished(true);
    }
    _pEntity = null;

CleanUp:

    return hr;
}


void
DTDNodeFactory::checkEntityRefLoop()
{
    if (_pEntityEnumeration)
    {
        _pEntityEnumeration->reset();

        // Search for parsed non-parameter entities
        while (_pEntityEnumeration->hasMoreElements()) 
            _pDTD->checkEntityRefLoop(ICAST_TO(Entity *, _pEntityEnumeration->nextElement()));
    }
}


HRESULT
DTDNodeFactory::_parseEntity(IXMLNodeSource* pSource)
{
    HRESULT hr = S_OK;
    bool external = false;
    ATCHAR* atext;
    const WCHAR* baseURL;
    
    String* text = _pEntity->getText();
    if (text != null)
    {
        atext = text->toCharArray();
        _fParsingEntity = true;
        _nState = eEntities;
    }
    else if ((text = _pEntity->getURL()) != null && _pDocument->getResolveExternals())
    {
        pSource->GetURL(&baseURL);
        atext = text->toCharArrayZ();
        _fParsingEntity = true;
        _nState = eEntities;
        external = true;
    }
    else
    {
        _fParsingEntity = false;
        _nState = eValidating;
    }
    if (_fParsingEntity)
    {
        _pEntity->setParsed(true);
        _s_cDepth = _cDepth;
        XMLParser* pParser = _getParser(pSource);
        atext->AddRef();
        if (external)
        {
            hr = pParser->LoadEntity(baseURL, atext->getData(), FALSE);
        }
        else
            hr = pParser->ParseEntity(atext->getData(), text->length(), FALSE);
        atext->Release();
        pParser->Release();

        if (FAILED(hr))
        {
            _fParsingEntity = false;
            _nState = eValidating;
            String * msgError;
            if (external)
                msgError = Resources::FormatMessage(XML_LOAD_EXTERNALENTITY, text, null);
            else
                msgError = String::emptyString();
            String* msg = AddError(hr, 
                Resources::FormatMessage(
				    XML_PARSING_ENTITY, _pEntity->getName()->toString(), msgError, null));

            Exception::throwE(msg, hr);
        }
    }

    return hr;
}

String*         
DTDNodeFactory::AddError(HRESULT hr, String* error)
{
    String* msg = null;
    TRY 
    {
        msg = Resources::FormatMessage(hr, null);
    }
    CATCH
    {
        TRY 
        {
            msg = Resources::FormatSystemMessage(hr);
        }
        CATCH
        {
        }
        ENDTRY
    }
    ENDTRY
    return String::add(error, msg, null);
}

void
_createDTDNode(IXMLNodeFactory* pFactory, IXMLNodeSource* pSource, PVOID parent, DWORD nodetype, 
               bool beginChildren, bool terminal, const WCHAR* pwcText, ULONG ulLen, ULONG usNSLen, NameDef* name, PVOID* ppNode)
{
    HRESULT hr = S_OK;
    DWORD subtype = 0;

    Assert(pFactory);

    XML_NODE_INFO info = { sizeof(XML_NODE_INFO),
        nodetype,                     // dwType
        0,                            // dwSubType
        terminal ? TRUE : FALSE,      // fTerminal
        pwcText,                      // pwcText
        ulLen,                        //ulLen
        usNSLen,                      // ulNsPrefixLen
        NULL,                         // pNode
        name };                       // pReserved

    XML_NODE_INFO* aNodeInfo[1];
    aNodeInfo[0] = &info;

    hr = pFactory->CreateNode(pSource, parent, 1, aNodeInfo);
    if (hr == S_OK && beginChildren && info.pNode != null)
    {
        hr = pFactory->BeginChildren(pSource, &info);
    }
    if (ppNode)
        *ppNode = info.pNode;

    if (hr)
        Exception::throwE(hr, hr, null);

}

void
_endChildren(IXMLNodeFactory* pFactory, IXMLNodeSource* pSource, IUnknown* node, DWORD nodetype, 
                             BOOL empty, NameDef* name)
{
    HRESULT hr = S_OK;
    DWORD subtype = 0;

    Assert(pFactory);

    XML_NODE_INFO info = { sizeof(XML_NODE_INFO),
        nodetype,   // dwType
        0,          // dwSubType
        FALSE,      // fTerminal
        NULL,       // pwcText
        0,          //ulLen
        0,          // ulNsPrefixLen
        node,       // pNode
        name };     // pReserved

    hr = pFactory->EndChildren(pSource, empty, &info);

    if (hr)
        Exception::throwE(hr, hr, null);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Argorithm of namespace handling in DTD:
//
// 1) All DTD classes can continue using the Name,
//    and do not need to be converted over to use the NameDef 
// 2) While parsing DTD's the namespace node factory builds weird Names with the URN = PREFIX.  
//    So for example "x:y" builds a NameDef with "x" being the prefix AND a Name with "x" being the urn.  
//    This means AttDefs with the same prefix will actually be using the same Name 
//    even if the real URN ends up being different.  This is ok, and will be fixed in step 3. 
// 3) At EndProlog time (before or after parsing entities) you walk the ElementDecls and 
//    for each element decl you find all the xmlns attributes and push them into
//    the namespace scope, then you check the ElementDecl name and all the non xmlns 
//    attributes for NameDefs that need to be "fixed".  For each NameDef that needs 
//    to be fixed, you build a new Name and replace it in the NameDef (using the current
//    namespace manager scope).  Then you add the ElementDecl to a NEW HashTable.  
//    And if there are any NameDefs that cannot be fixed you throw an error.  
//    (Not including the content model yet).  
//    Then you pop this scope and continue to next ElementDecl and so on. 
// 4) At the end of all this, you go back and fix all the names in all the ContentModels by 
//    looking them up in the old ElementDecl HashTable and using the ElementDecl to get 
//    the new "fixed" Name. 
// 5) Then replace the old ElementDecl hashtable with the new ElementDecl hashtable and you're done ! 
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void
DTDNodeFactory::FixNamesInDTD()
{
    ElementDecl * pDecl;
    Atom * pAtomURN, * pAtomPrefix;
    Hashtable   * pTempElementDecls = Hashtable::newHashtable();
    Enumeration * pElementDecls = _pDTD->elementDeclarations();
    Enumeration * pAttdefs;
    AttDef * pAttDef;
    Integer * lookup;
    ATCHAR *pPrefix;

    //
    // Fix names for ElementDecl-s and Attributes
    //
    while (pDecl = (ElementDecl*)pElementDecls->nextElement())
    {
        bool fScope = false;

        pAttdefs = pDecl->getAttDefs();

        if (pAttdefs)
        {
            //
            // Check && Push scopes
            //
            for (;pAttdefs->hasMoreElements();)
            {
                pAttDef = (AttDef*)pAttdefs->nextElement();

                // See if it matches the magic URN returned from findGlobalURN.
                if ((Atom*)XMLNames::atomURNXMLNS == pAttDef->getName()->getNameSpace())
                {               
                    fScope = true;
                    Atom *pSrcURN;
                    Object * pObj = pAttDef->getDefault();
                    if (pObj)
                        pSrcURN = Atom::create(pObj->toString());
                    else
                        pSrcURN = Atom::create(String::emptyString());
                    pAtomPrefix = pAttDef->getName()->getName();
                    if (pAtomPrefix && ((Atom*)XMLNames::atomEmpty != pAtomPrefix))
                    {
                        // Check for reserved namespace
                        if (NamespaceMgr::isReservedNameSpace(pAtomPrefix))
                        {
                            Exception::throwE(XML_XMLNS_RESERVED, 
                                XML_XMLNS_RESERVED, pAttDef->getName()->toString(), null);
                        }
                    }
                    else 
                    {
                        pAtomPrefix = null;
                    }

                    _pNamespaceMgr->pushScope(pAtomPrefix,
                                    null, pSrcURN, pDecl);
                }
            }
        }

        //
        // Fix up name for ElementDecl
        //
#if 0
        Atom * pNodeURN = null;
#endif
        bool fReserved = false;

        pAtomPrefix = pDecl->getName()->getNameSpace();
        if (pAtomPrefix)
        {
            fReserved = _pNamespaceMgr->isReservedNameSpace(pAtomPrefix);
        }

        if (fScope)
        {
            if (!fReserved)
            {
                Atom * pSrcURN = null;
                pAtomURN = _pNamespaceMgr->findURN(pAtomPrefix, null, &pSrcURN);
                if (pAtomURN)
                {
#if 0
                    pNodeURN = pAtomURN;
#endif
                    pDecl->setNameDef(_pNamespaceMgr->createNameDef(pDecl->getNameDef(), pAtomURN, pSrcURN));
                }
                else if (pAtomPrefix)
                {
                    goto Error;
                }
            }
        }
        else if (pAtomPrefix && !fReserved)
        {
            goto Error;
        }

        //
        // Fix up names for attributes
        //
        if (pAttdefs)
        {
            pAttdefs->reset();
            for (;pAttdefs->hasMoreElements();)
            {
                pAttDef = (AttDef*)pAttdefs->nextElement();
                pAtomPrefix = pAttDef->getName()->getNameSpace();

                //
                // Notice: Do not need to fix namespace declaration or xml:* attdef names
                //
                if (pAtomPrefix && pAtomPrefix != XMLNames::atomURNXMLNS && pAtomPrefix != XMLNames::atomURNXML)
                {
                    pAtomURN = null;
                    Atom * pSrcURN = null;
                    if (fScope)
                        pAtomURN = _pNamespaceMgr->findURN(pAtomPrefix, null, &pSrcURN);
                    else
                        pSrcURN = pAtomURN = _pNamespaceMgr->findGlobalURN(pAtomPrefix);

                    if (pAtomURN)
                    {
                        pAttDef->setName(Name::create(pAttDef->getName()->toString(), pAtomURN));
                    }
                    else if (pAtomPrefix != XMLNames::atomURNXML)
                    {
                        goto Error;
                    }
              
                    Node * pNode = pAttDef->getDefaultNode();
                    if (pNode)
                    {
                        pNode->setName(_pNamespaceMgr->createNameDef(pAttDef->getName(), pSrcURN, pAtomPrefix));

                        if (XMLNames::name(NAME_DTDT) == pAttDef->getName())
                        {
                            // HACKHACK?
                            pNode->notifyNew();
                        }
                    }
                }
            }
        }

        // Pop scopes
        if (fScope)
        {
            _pNamespaceMgr->popScope(pDecl);
        }

        // Add elementdecl to a temp hashtable
        pTempElementDecls->put(pDecl->getName(), pDecl);
    }

    //
    // Fix up doctype name
    //
    {
        NameDef * pDocType = _pDTD->getDocType();
        if (pDocType->getPrefix())
        {
            Hashtable* elementDecls = _pDTD->getElementDecls();
            if (elementDecls)
            {
                ElementDecl * pRoot = (ElementDecl*)elementDecls->get(pDocType->getName());
                if (pRoot)
                {
                    _pDTD->setDocType(pRoot->getNameDef());

                    // update namedef on DOCTYPE node
                    Node * pDocTypeNode = _pDocument->getDocNode()->find(null, Node::DOCTYPE);
                    Assert(pDocTypeNode);
                    pDocTypeNode->setName(pRoot->getNameDef());
                }
            }

            //
            // Do not validate here!!
            // We do all validation in validation factory
        }
    }

    //
    // Fix names in ElementDecl Content models
    //
    pElementDecls->reset();
    while (pDecl = (ElementDecl*)pElementDecls->nextElement())
    {
        Vector * pSymbols = pDecl->_pContent->_pSymbols;
        Hashtable * pSymboltable = pDecl->_pContent->_pSymbolTable;

        Assert(pSymbols);
        Assert(pSymboltable);

        for (int i = pSymbols->size() - 1; i >= 0; i--)
        {
            Name * pName = (Name*)pSymbols->elementAt(i);
      
            if (pName != XMLNames::name(NAME_PCDATA))
            {
                ElementDecl *pDecl1 = null;

                Hashtable* elementDecls = _pDTD->getElementDecls();
                if (elementDecls)
                {
                    pDecl1 = (ElementDecl*)elementDecls->get(pName);
                }
                if (pDecl1)
                {
                    lookup = (Integer *)pSymboltable->get(pName);
                    pSymboltable->remove(pName);
                    pName = pDecl1->getName();
                    pSymbols->setElementAt(pName, i);
                    pSymboltable->put(pName, lookup);
                }
                else
                {
                    Exception::throwE(XML_ELEMENT_UNDEFINED, 
                        XML_ELEMENT_UNDEFINED, pName->toString(), null);
                }

            }
        }
    }

    //
    // Replace the old elementdecl hashtable
    //
    _pDTD->setElementDecls(pTempElementDecls);
    goto Cleanup;

Error:
    Exception::throwE(XML_XMLNS_UNDEFINED, 
        XML_XMLNS_UNDEFINED, pAtomPrefix->toString(), null);

Cleanup:    
    return;
}

HRESULT
DTDNodeFactory::FinishAttDefs()
{
    // We cannot parse the default attribute values until we are at the end of
    // the DTD because we need to expand the entity references in the default
    // value and the entities are not parsed until the end of the DTD because
    // entities CAN have forward references to other entities.  This mess is all 
    // due to the conflict between the XML spec which thinks of entities as macros 
    // that always get expanded when they are used and our Object Model which likes
    // to actually "preserve" these entity references.  So in order to satisfy both, 
    // we have to loosen our XML Language spec compliance a bit and actually allow
    // forward references inside an entity used inside an attribute default value.

    HRESULT hr = S_OK;
    ElementDecl* pDecl;
    Enumeration * pElementDecls = _pDTD->elementDeclarations();
    while (pDecl = (ElementDecl*)pElementDecls->nextElement())
    {
        Enumeration* pAttdefs = pDecl->getAttDefs();
        if (pAttdefs)
        {
            for (;pAttdefs->hasMoreElements();)
            {
                AttDef* pAttDef = (AttDef*)pAttdefs->nextElement();
                // Now set and validate the value of the default node according to 
                // the pAttDef itself.  This is done at the end of the DTD so that
                // we can support forward references to entities in the default
                // value.
                Node* node = (Node*)(Object*)pAttDef->_pDefNode;
                if (node)
                {
                    pAttDef->checkAttributeType(NULL, node, _pDocument, true);
                }
            }
        }
    }
Cleanup:
    return hr;
}

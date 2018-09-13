/*
 * @(#)ValidationFactory.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "validationfactory.hxx"
#include "domnode.hxx"
#include "xmlnames.hxx"
#include "chartype.hxx"
#include "entity.hxx"
#include "notation.hxx"
#include "attdef.hxx"
#include "namespacemgr.hxx"
#include "xmlnames.hxx"

//=====================================================================================
ValidationFactory::ValidationFactory(IXMLNodeFactory * fc, DTD* dtd, Document* doc)
{
    _pFactory = fc;
    _pDTD = dtd;
    _pDoc = doc;
}

ValidationFactory::~ValidationFactory()
{
    _pFactory = null;
    _pDTD = null;
    _pContexts = null;
    _pCurrent = null;
    _pAttDef = null;
    _pCurrentElement = null;
    _pDoc = null;
}

void ValidationFactory::init()
{
    _pContexts = Stack::newStack();
    _pCurrent = null;
    _pCurrentElement = null;
    _pAttDef = null;
    _nState = _pDTD->isSchema() ? eSchema : eValidating; // schema is looser
    _fInPI = false;
    _fInAttribute = false;
    _fValidateRoot = true;
    _fNotifiedError = false;
    _fForcePreserveWhiteSpace = _pDoc->getPreserveWhiteSpace();
}

///////////////////////////////////////////////////////////////////////////////////
// IXMLNodeFactory Interface Implementation
//
HRESULT STDMETHODCALLTYPE 
ValidationFactory::NotifyEvent( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
    HRESULT hr = S_OK;

    switch (iEvt)
    {
    case XMLNF_STARTDTD:
        init();
        break;

    case XMLNF_ENDDOCUMENT:
        if (!_fNotifiedError)
            _pDTD->checkForwardRefs(); // forward references to ID's in the document.
        break;
    }

    hr = _pFactory->NotifyEvent(pSource, iEvt);

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE ValidationFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
    HRESULT hr = S_OK;

    switch (_nState)
    {
    case ePending:
        if (pNodeInfo->pNode == _pNodePending)
        {
            hr = CheckPending(pSource);
            if (FAILED(hr)) goto Cleanup;
            _nState = eValidating;
        }
        else
            goto none;
        // fall through - but only because eAttribute is already doing the right thing.

    case eAttribute:
        _nState = eValidating;
        // fall through

    case eValidating:
        _pAttDef = null;
        _pCurrentElement = null;
        _fInAttribute = false;

        if (_pCurrent && _pCurrent->ed && pNodeInfo->dwType == XML_ELEMENT)
        {
            _pCurrent->ed->checkRequiredAttributes((Node*)pNodeInfo->pNode,
                _pDTD, pSource);
            if (!_fForcePreserveWhiteSpace)
            {
                Assert(pNodeInfo->pReserved == null);
                pNodeInfo->pReserved = (PVOID)_pCurrent->ed->getXmlSpace();
            }
        }
        // fall through
    case eSchema:
none:
        hr =  _pFactory->BeginChildren(pSource, pNodeInfo);
        break;
    }
Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE ValidationFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
    HRESULT hr = S_OK;

    DWORD dwType = pNodeInfo->dwType;

    hr = _pFactory->EndChildren(pSource, fEmptyNode, pNodeInfo);
    if (FAILED(hr)) goto Cleanup;

    switch (_nState)
    {
    case ePending:
        if (pNodeInfo->pNode == _pNodePending)
        {
            hr = CheckPending(pSource);
            if (FAILED(hr)) goto Cleanup;
        }
        else
            break;
        // fall through - but only because eAttribute is already doing the right thing.

    case eAttribute:
        _nState = eValidating;
        // fall through

    case eValidating:
        switch (dwType)
        {
        case XML_ATTRIBUTE:
            _fInAttribute = false;

            if (pNodeInfo->pNode && _pCurrent && _pCurrent->ed && _pAttDef)
            {
                // need to check the contents of this attribute to make sure
                // it is valid according to the specified attribute type.
                _pAttDef->checkAttributeType(pSource, (Node*)pNodeInfo->pNode, _pDoc);
            }
            break;

        case XML_PI:
            _fInPI = false;
            break;
        }

    case eSchema:
        if (_pCurrent && (_pCurrent->pt == pNodeInfo->pNode))
        {
            if (_pCurrent->ed)
            {
                if (fEmptyNode && pNodeInfo->dwType == XML_ELEMENT)
                {
                    _pCurrent->ed->checkRequiredAttributes((Node*)pNodeInfo->pNode,
                        _pDTD, pSource);
                }
                if (fEmptyNode && !_pCurrent->ed->acceptEmpty())
                {
                    hr = XML_EMPTY_NOT_ALLOWED;
                    goto Cleanup;
                }
                else if (!_pCurrent->matched)
                {
                    hr = XML_ELEMENT_NOT_COMPLETE;
                    goto Cleanup;
                }
            }
            _pContexts->pop();
            if (_pContexts->empty())
            {
                _pCurrent = NULL;
                _nState = eSchema;
            }
            else
            {
                _pCurrent = (DTDState*)_pContexts->peek();
                _nState = (EState)_pCurrent->nState;
            }
        }
    }

Cleanup:

    return hr;
}

HRESULT STDMETHODCALLTYPE ValidationFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    HRESULT hr = S_OK;

    XML_NODE_INFO* pNodeInfo = *apNodeInfo;
    NameDef* namedef = (NameDef*)pNodeInfo->pReserved;

    switch (_nState)
    {
    case eValidating:

        //
        // check that we're inside an attribute or not.
        //
        if (pNodeInfo->dwType == XML_ATTRIBUTE)    
        {
            _fInAttribute = true;
            _nState = eAttribute;
        }

        //
        // validate if (_pCurrent != null)
        //
        if (!_pCurrent)
        {
            //
            // check root name matches doctype name
            //
            if (_fValidateRoot)
            {
                _fValidateRoot = false;
                if (! _pDTD->isSchema()) // it is ok to have undefined root in schema doc.
                {
                    // Now it is possible at this point to have the urn's be different
                    // because of namespace fixup's that happen when we discover the
                    // xmlns attribute.  So we only compare the gi and prefix atoms.
//                    NameDef* namedef = (NameDef*)pNodeInfo->pReserved;
                    NameDef * docType = _pDTD->getDocType();
                    if (docType != null && 
                        (docType->getName()->getName() != namedef->getName()->getName() || 
                            docType->getPrefix() != namedef->getPrefix()))
                    {
                        hr = XML_ROOT_NAME_MISMATCH;
                        goto Cleanup;
                    }
                }
            }
        }
        else if (_pCurrent->ed)
        {
//            NameDef* namedef = (NameDef*)pNodeInfo->pReserved;

            switch (pNodeInfo->dwType)
            {
            case XML_ELEMENT:       // <foo ... >
                _pCurrent->ed->checkContent(_pCurrent, namedef->getName(), pNodeInfo->dwType); // // throws exception 
                break;

            case XML_ATTRIBUTE:     // <foo bar=...>
                CheckAttribute(namedef);
                break;

            case XML_WHITESPACE:
                // make sure content is allowed to contain whitespace !
                if (_pCurrent->ed->getContent()->getType() == ContentModel::EMPTY)
                    _pCurrent->ed->checkContent(_pCurrent, NULL, XML_PCDATA); // throws exception
                break; 

                // fall through
            case XML_PCDATA:        // text inside a node
            case XML_CDATA:         // <![CDATA[...]]>
                if (!_fInPI)
                {
                    _pCurrent->ed->checkContent(_pCurrent, null, pNodeInfo->dwType); // throws exception
                }
                break;

            case XML_ENTITYREF:
                if (pNodeInfo->fTerminal)
                {
                    goto basicent;
                }
                else
                {
                    // this is a built in entity - so treat it like PCDATA.
                    _pCurrent->ed->checkContent(_pCurrent, NULL, XML_PCDATA); // throws exception
                }
                break;

            case XML_PI:
                _fInPI = true;
                break;
            }
        }

createnode:
        hr = _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);

        if (SUCCEEDED(hr))
        {
            switch (pNodeInfo->dwType)
            {
            case XML_ELEMENT:
                {
//                    NameDef* namedef = (NameDef*)pNodeInfo->pReserved;
                    Name* name = namedef->getName();
                    ElementDecl* ed = _pDTD->findElementDecl(name);

                    // This is going to push the parent's _nState to the stack
                    _pCurrent = DTDState::newDTDState(name, pNodeInfo->dwType, ed, pNodeInfo->pNode, _nState);
                    _pContexts->push((Object *)_pCurrent);

                    if (ed)
                    {
                        ed->initContent(_pCurrent);
                        _pCurrentElement = (Node*)pNodeInfo->pNode;

                       // have to do this for the eSchema goto case
                       _nState = eValidating;
                    }
                    else
                    {
                        if (_pDTD->validate() && ! _pDTD->isSchema())
                        {
                            _pDTD->reportUndeclaredElement(namedef);
                        }
                        else if (_pDTD->isLoadingSchema(name->getNameSpace()))
                        {
                            _pNodePending = (Node*)pNodeInfo->pNode;
                            _nState = ePending;
                        }
                        else
                        {
                            // it is ok to have undefined elements in Schema content
                            // but the element must not be in a Schema namespace
                            if (_pDTD->hasSchema(name->getNameSpace()))
                            {
                                Exception::throwE(XML_ELEMENT_UNDECLARED,  
                                                  XML_ELEMENT_UNDECLARED, namedef->toString(), null);
                            }
                            _nState = eSchema;
                        }
                    }
                }
                break;
            }
        }
        break;


    case eSchema:
        // In the schema case only part of the document may actually point to a schema
        // so we have to watch for this and then switch to eValidating.
        if (pNodeInfo->dwType == XML_ELEMENT)
        {
//            NameDef* namedef = (NameDef*)pNodeInfo->pReserved;

            if (_pDTD->hasSchema(namedef->getName()->getNameSpace()))
            {
                if (!_pCurrent)
                    _nState = eValidating;
                goto createnode;
            }
        }
        else if (pNodeInfo->dwType == XML_ATTRIBUTE) // check global attribute
        {
            CheckGlobalAttribute(namedef);
        }
        // fall through

    case eAttribute:
        if (pNodeInfo->dwType == XML_ENTITYREF && pNodeInfo->fTerminal)
        {
basicent:
//            NameDef* namedef = (NameDef*)pNodeInfo->pReserved;
            Assert(namedef);
            _pCurrent = _pDTD->checkEntityRef(namedef->getName(), _pCurrent, _fInAttribute);
        }
        // fall through

    case ePending:
        // BUGBUG: what about global attributes?
justcreate:
        hr = _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
        break;

    }

Cleanup:

    return hr;
}

HRESULT
ValidationFactory::CheckPending(IXMLNodeSource  *pSource)
{
    HRESULT hr = S_OK;

    NameDef* namedef = _pNodePending->getNameDef();
    Name* name = namedef->getName();
    void* pTag;
    Node* attr;
    bool  fEd = true;

    ElementDecl* ed = _pDTD->findElementDecl(name);

    if (ed == null)
    {
        // it is ok to have undefined elements in Schema content
        // but the element must not be in a Schema namespace
        if (_pDTD->hasSchema(name->getNameSpace()))
        {
            Exception::throwE(XML_ELEMENT_UNDECLARED,  
                              XML_ELEMENT_UNDECLARED, namedef->toString(), null);
        }
        _nState = eSchema;
        fEd = false;
    }
    else
    {
        _pCurrent->ed = ed;
    }

    // validate the node's attributes since we could't do it during CreateNode.
    attr = _pNodePending->getNodeFirstAttribute(&pTag);
    while (attr)
    {
        if (fEd)
            CheckAttribute( attr->getNameDef() );
        else
            CheckGlobalAttribute( attr->getNameDef() );

        if (_pAttDef)
        {
            // Fix up the attribute type - since we didn't
            // do it during CreateNode.
            attr->setNodeAttributeType(_pAttDef->getType());

            // This makes sure that id is added to the hashtable if it is a id
            attr->notifyNew(NULL, true, false, null, _pAttDef);

            _pAttDef->checkAttributeType(pSource, (Node*)attr, _pDoc);
        }
        attr = _pNodePending->getNodeNextAttribute(&pTag);
    }

    if (fEd)
    {
        // Now do what CreateNode would have done to prepare for validation.
        ed->initContent(_pCurrent);
        _pCurrentElement = (Node*)_pNodePending;
    }

Cleanup:
    _pNodePending = null;
    return hr;
}

void
ValidationFactory::CheckAttribute(NameDef* namedef)
{
    Name* name = namedef->getName();
    _pAttDef = _pCurrent->ed->getAttDef(name);

    // In DTD, every attribute must be declared.
    // In Schema, a. if it is open, we allow undeclared attributes if they are global or no namespace
    //   b. or if it is closed, we have to allow undeclared xmlns[:prefix] attribute on an element
    if (! _pAttDef &&
        !(_pCurrent->ed->isOpen() && ((!_pDTD->hasSchema(name->getNameSpace()) && name->getNameSpace()) || _pDTD->getGAttributeType(name) != NULL)) && 
        !(_pDTD->isSchema() && name->getNameSpace() == XMLNames::atomURNXMLNS))
    {
        Exception::throwE(XML_ATTRIBUTE_NOT_DEFINED, 
                          XML_ATTRIBUTE_NOT_DEFINED, namedef->toString(), null);
    }
}


void
ValidationFactory::CheckGlobalAttribute(NameDef* namedef)
{
    Name* name = namedef->getName();
    _pAttDef = _pDTD->getGAttributeType(name);
    if ((_pAttDef == null) && _pDTD->hasSchema(name->getNameSpace()))
    {
        Exception::throwE(XML_ATTRIBUTE_NOT_DEFINED, 
                          XML_ATTRIBUTE_NOT_DEFINED, namedef->toString(), null);
    }
}

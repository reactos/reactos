/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#include "node.hxx"
#include <xmlparser.h>
#include "nodedatanodefactory.hxx"
#include "nodeproperty.hxx"

#ifndef _XML_OM_DOMNODE
#include "xml/om/domnode.hxx"
#endif

#ifndef _XMLNAMES_HXX
#include "xmlnames.hxx"
#endif

#ifndef _XML_PARSER_DTD
#include "xml/dtd/dtd.hxx"
#endif

#ifndef _NAMESPACENODEFACTORY_HXX
#include "xml/om/namespacenodefactory.hxx"
#endif

#ifndef _VALIDATIONFACTORY_HXX
#include "xml/dtd/validationfactory.hxx"
#endif

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

// ###
// ### THIS NODE FACTORY MUST NOW BE WRAPPED BY THE NAMESPACE NODE FACTORY BECAUSE
// ### IT NO LONGER DOES TRY/CATCH OR STACK_ENTRY.
// ###

#if DBG==1
char * 
nodeTypeDebug( DWORD dwType)
{
    char * pstr = null;
    switch (dwType)
    {
    case XML_ATTRIBUTE:
        pstr = "XML_ATTRIBUTE";
        break;
    case XML_DTDATTRIBUTE:
        pstr = "XML_DTDATTRIBUTE";
        break;
    case XML_ELEMENT:
        pstr = "XML_ELEMENT";
        break;
    case XML_PCDATA:
        pstr = "XML_PCDATA";
        break;
    case XML_CDATA:
        pstr = "XML_CDATA";
        break;
    case XML_COMMENT:
        pstr = "XML_COMMENT";
        break;
    case XML_ENTITYREF:
        pstr = "XML_ENTITYREF";
        break;
    case XML_XMLDECL:                        
        pstr = "XML_XMLDECL";
        break;
    case XML_PI:
        pstr = "XML_PI";
        break;
    case XML_DOCTYPE:
        pstr = "XML_DOCTYPE";
        break;
    case XML_DTDSUBSET:
        pstr = "XML_DTDSUBSET";
        break;
    case XML_WHITESPACE:
        pstr = "XML_WHITESPACE";
        break;
    case XML_ENTITYDECL:
        pstr = "XML_ENTITYDEC;";
        break;
    case XML_ELEMENTDECL:
        pstr = "XML_ELEMENTDECL";
        break;
    case XML_ATTLISTDECL:
        pstr = "XML_ATTLISTDECL";
        break;
    case XML_NOTATION:
        pstr = "XML_NOTATION";
        break;
    case XML_GROUP:
        pstr = "XML_GROUP";
        break;
    case XML_IGNORESECT:
        pstr = "XML_IGNORESECT";
        break;
    case XML_INCLUDESECT:
        pstr = "XML_INCLUDESECT";
        break;
    case XML_PEREF:
        pstr = "XML_PEREF";
        break;
    case XML_NAME:
        pstr = "XML_NAME";
        break;
    case XML_STRING:
        pstr = "XML_STRING";
        break;
    case XML_ATTTYPE:
        pstr = "XML_ATTTYPE";
        break;
    case XML_ATTPRESENCE:
        pstr = "XML_ATTPRESENCE";
        break;
    case XML_MODEL:
        pstr = "XML_MODEL";
        break;
    case XML_XMLSPACE:
        pstr = "XML_XMLSPACE";
        break;
    case XML_XMLLANG:
        pstr = "XML_XMLLANG";
        break;
    default:
        pstr = "??";
        break;
    }
    return pstr;
}
#endif


//////////////////////////////////////////////////////////////////////////////////////
// class BaseNDNodeFactory

_NDNodeFactory::_NDNodeFactory( Document * pDocument)
{
    Assert( pDocument);
    _pNodeMgr = pDocument->getNodeMgr();
    _pLastTextNode = null;
    _pDocument = pDocument;
    _fTextInBuffer = false;
    _fCollapsed = false;
    _ulBufUsed = 0;
    _pBuffer = null;
    _fPreserveWhiteSpace = _fForcePreserveWhiteSpace = pDocument->getPreserveWhiteSpace();
}

_NDNodeFactory::~_NDNodeFactory()
{
    _pNodeMgr = null;
    _pDocument = null;
    Assert(0 == _ulBufUsed);
    _pBuffer = null;
}


bool
_NDNodeFactory::bufferAppend(Node* pParent, const WCHAR *pwc, ULONG ulLen)
{
    bool result = true;
    Node * pDataNode = pParent->_pLast;
    if (!pDataNode)
    {
        AWCHAR * pText;
        pText = new (ulLen) AWCHAR;
        pText->simpleCopy(0, ulLen, pwc);
        pParent->setCollapsedText(pText);
        _fCollapsed = true;
        _pLastTextNode = pParent;
        _fTextInBuffer = false;
        goto CleanUp;
    }
    else
    {
        if (_fTextInBuffer)
        {
            bufferAppend(pwc, ulLen);
            goto CleanUp;
        }
        if (_fCollapsed)
        {
            Assert(!_fTextInBuffer && _ulBufUsed == 0);
            AWCHAR * pText = pParent->orphanText();
            Assert(pText);
            bufferAppend(pText->getData(), pText->length());
            pText->Release();
            _fTextInBuffer = true;
            bufferAppend(pwc, ulLen);
            goto CleanUp;
        }
    }

    if (pDataNode && pDataNode == _pLastTextNode)
    {
        Assert(!_fTextInBuffer && _ulBufUsed == 0);
        AWCHAR * pText = (AWCHAR*)pDataNode->getNodeText();
        bufferAppend(pText->getData(), pText->length());
        bufferAppend(pwc, ulLen);
        _fTextInBuffer = true;
        goto CleanUp;
    }

    _fCollapsed = false;
    result = false;
CleanUp:
    return result;
}

void
_NDNodeFactory::bufferAppend(const WCHAR *pwc, ULONG ulLen)
{
// make sure buffer is large enough
    ULONG ulNeedLen = ulLen + _ulBufUsed;
    ULONG ulNewLen = _pBuffer ? _pBuffer->length() : 0;
    if (ulNeedLen > ulNewLen)
    {
        // start with 1k buffer
        if (!ulNewLen)
            ulNewLen = 1024;

        // is this a good alogrithm for this use?
        while (ulNeedLen > ulNewLen)
            ulNewLen *= 2;

        _pBuffer = _pBuffer ? _pBuffer->resize(ulNewLen) : new (ulNewLen) AWCHAR;
    }

    _pBuffer->simpleCopy(_ulBufUsed, ulLen, pwc);
    _ulBufUsed += ulLen;
}

void
_NDNodeFactory::bufferAttach()
{
    AWCHAR * pText;
    pText = new (_ulBufUsed) AWCHAR;
    pText->simpleCopy(0, _ulBufUsed, _pBuffer->getData());
    if (_fCollapsed)
    {
        _pLastTextNode->setCollapsedText(pText);
    }
    else
    {
        _pLastTextNode->_setText(pText);
    }
    _pLastTextNode = null;
    _fTextInBuffer = false;
    _fCollapsed = false;
    _ulBufUsed = 0;
}

//////////////////////////////////////////////////////////////////////////////////////
// class NodeDataNodeFactory

HRESULT STDMETHODCALLTYPE 
NodeDataNodeFactory::NotifyEvent( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
    /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
#if PRODUCT_PROF
    IceCAP cap;
#endif
    switch (iEvt)
    {
    case XMLNF_STARTDOCUMENT:
        _fXMLSpaceAttr = false;
        _pLastTextNode = null;
        _pRoot = _pDocument->getDocNode();

        // _fInsideEntity flag is not meant to imply 
        // that all nodes created are necessarily 'inside' 
        // an entity, but rather that any ELEMENT/ATTRIBUTE 
        // that is created is part of an entity... 
        // (we need to know this because IDs are not registered 
        //  for nodes inside and entity)
        _fInsideEntity = true;

        // if the NodeDateNF is being used, the document's primary
        // factory _must_ be the NSNF!
        {
            NameSpaceNodeFactory * pNSNF = (NameSpaceNodeFactory*)_pDocument->getFactory();
            _pValidationNF = (ValidationFactory*)pNSNF->getValidationNF();
        }

        // This must be last, since it will cause onReadyStateChagne to fire
        // which may invoke code/script with changes/resets the state of the
        // document or parser.
        _pDocument->onStartDocument();
#if DBG == 1
        _cDepth = 0;
#endif
        break;
    case XMLNF_STARTDTD:
        break;
    case XMLNF_ENDDTD:
        break;
    case XMLNF_STARTDTDSUBSET:
        break;
    case XMLNF_ENDDTDSUBSET:
        break;
    case XMLNF_ENDPROLOG:
        _pDocument->onEndProlog();
        _fInsideEntity = false;
        break;
    case XMLNF_ENDENTITY:
        if (_fTextInBuffer)
            bufferAttach();
        _pLastTextNode = null;
        _fCollapsed = false;
        break;
    case XMLNF_ENDDOCUMENT:
#if DBG == 1
        Assert(_cDepth == 0);
#endif
        _pDocument->HandleEndDocument();  // tell document to find out that we've finished.
        _pLastTextNode = null;
        _ulBufUsed = 0;    
        _pBuffer = null;
        _fPreserveWhiteSpace = false;
        _fXMLSpaceAttr = false;
        _pValidationNF = null;
        break;
    case XMLNF_DATAAVAILABLE:
        _pDocument->onDataAvailable();
        break;
    case XMLNF_STARTSCHEMA:
        break;
    }
    return S_OK;
}    

HRESULT STDMETHODCALLTYPE 
NodeDataNodeFactory::BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
    DWORD_PTR dwInfo = (DWORD_PTR)pNodeInfo->pReserved;
    _pLastTextNode = null;
    _fCollapsed = false;
    if (_fPreserveWhiteSpace)
        pNodeInfo->pReserved = (PVOID)(dwInfo | XMLSPACE_PARENT_FLAG);
    if (_fXMLSpaceAttr)
        _fPreserveWhiteSpace = _fXMLSpaceAttrValue;
    else if (0 != (dwInfo & XMLSPACE_DEFAULT_DEFINED))
    {
        Assert(pNodeInfo->dwType == XML_ELEMENT);
        Assert(!_fForcePreserveWhiteSpace);
        _fPreserveWhiteSpace = 0 != (dwInfo & XMLSPACE_DEFAULT_PRESERVE);
    }
    _fXMLSpaceAttr = false;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
NodeDataNodeFactory::EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo) 
{
#if PRODUCT_PROF
    IceCAP cap;
#endif
#if DBG==1
        // This is to help devs figure out the event type... 
        // by translating the DWORD into a it's textual name....
        const char * pstrType = nodeTypeDebug( pNodeInfo->dwType);
#endif

#if DBG == 1
    _cDepth--;
#endif
    bool isAttribute = (XML_ATTRIBUTE == pNodeInfo->dwType);

    if (pNodeInfo->pNode)
    {
        Node * pNode = (Node*)(pNodeInfo->pNode);
        Element::NodeType nodeType = pNode->getNodeType();

        // If the nodetype is ENTITY (or DOCTYPE), we will set finished to true
        // after we have parsed the entity (or all entities)  
        if (Element::ENTITY != nodeType && Element::DOCTYPE != nodeType)
            pNode->setFinished(true);

        if (_fTextInBuffer)
            bufferAttach();
        _pLastTextNode = null;
        _fCollapsed = false;
        ValidationFactory * pVNF = _pValidationNF;
        pNode->notifyNew(null, true, _fInsideEntity, 
                         pVNF ? pVNF->getElemDecl() : null, 
                         pVNF ? pVNF->getAttDef() :  null);
        if (!_fForcePreserveWhiteSpace && pNode->_fAttribute && pNode->getName() == XMLNames::name(NAME_XMLSpace2))
        {
            // watch out.. this throws an exception if value is not valid
            bool fPreserve = ProcessXmlSpace(pNode, false);
            _fXMLSpaceAttr = true;
            _fXMLSpaceAttrValue = fPreserve;
        }
        else 
        {
            if (Element::ELEMENT == pNode->getNodeType() && !fEmptyNode
                && ! pNode->isCollapsedText() 
                 && (!pNode->_pLast || pNode->_pLast->_fAttribute))
            {
                Assert(pNode->isParent());
                pNode->setNotQuiteEmpty(true);
            }
        }
    }

    if (!isAttribute)
    {
        if (!fEmptyNode)
            _fPreserveWhiteSpace = ((DWORD_PTR)pNodeInfo->pReserved & XMLSPACE_PARENT_FLAG) != 0; 
        else
            _fXMLSpaceAttr = false; // then we've already ended the node that had a xml:space attr !
    }

Cleanup:
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
NodeDataNodeFactory::Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
#if PRODUCT_PROF
    IceCAP cap;
#endif
#if DBG == 1
    ULONG lLine = pNodeSource->GetLineNumber();
    ULONG lPos = pNodeSource->GetLinePosition();
#endif

    _pDocument->_clearDocNode();

    _pDocument->HandleEndDocument();
    _pLastTextNode = null;
    _ulBufUsed = 0;    
    _pBuffer = null;
    _pValidationNF = null;
#if DBG == 1
    _cDepth = 0;
#endif
    return hrErrorCode;
}

HRESULT STDMETHODCALLTYPE 
NodeDataNodeFactory::CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ PVOID pNodeParent, 
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
#if PRODUCT_PROF
    IceCAP cap;
#endif
#if DBG==1
    const char * pstrType = nodeTypeDebug( apNodeInfo[0]->dwType);
#endif

    Element::NodeType eNodeType;
    Node * pNode = null;
    Node * pParent = (Node*)pNodeParent;
    XML_NODE_INFO* pNodeInfo = *apNodeInfo;
    bool fTerminal = (pNodeInfo->fTerminal == TRUE);
#if DBG == 1
    if (! fTerminal) 
        _cDepth++;
#endif
    NodeProperties p = nodeProperties[pNodeInfo->dwType];

    if (_fTextInBuffer)
    {
        Assert(_pLastTextNode);
        if (p.HasName() || (pNodeInfo->dwType != XML_PCDATA && pNodeInfo->dwType != XML_WHITESPACE))
        {
            // must make a text node now since we have a non-text sibling !
            bufferAttach(); 
        }
        else
        {
            // append text to buffer
            Assert(!p.HasName() && p.IsData() 
                   && (XML_PCDATA == pNodeInfo->dwType || XML_WHITESPACE == pNodeInfo->dwType));
            bufferAppend(pNodeInfo->pwcText, pNodeInfo->ulLen);
            goto NoNode;
        }
    }

    if (p.HasName())
    {
        pNode = Node::newNodeFast((Element::NodeType)p.iNodeType, (NameDef*)pNodeInfo->pReserved, pParent,
                                  null, null, 0, 
                                  _pDocument, _pNodeMgr);
#if DBG == 1
        _pDocument->_cNodes++;
        _pDocument->_cNamedNodes++;
#endif
    }
    else
    {
        if (p.IsData())
        {
            Assert(pParent);

            switch (pNodeInfo->dwType)
            {
            case XML_WHITESPACE:
                if (pParent == _pRoot) // no whitespace nodes allowed at root level
                    goto NoNode;
                if (!_fPreserveWhiteSpace && ! _fCollapsed)
                {
                    Node * pDataNode = pParent->_pLast;
                    if (!pDataNode || pDataNode != _pLastTextNode)
                    {
                        Node * pPrevSib = pParent->getNodeLastChild();
                        if ( pPrevSib)
                            pPrevSib->setWSFollow(true);
                        else if ( pParent)
                            pParent->setWSInner(true);
                        goto NoNode;
                    }
                }
                // fall through

            case XML_PCDATA:
                if (bufferAppend(pParent, pNodeInfo->pwcText, pNodeInfo->ulLen))
                    goto NoNode;
                break;

            case XML_CDATA:
                if (pParent->getNodeType() == Element::PI)
                {
                    // if we are tyring to attach a text node to a node which doesn't have children, 
                    //  just stick the text on the parent
                    // for now, the only case should be PIs
                    pParent->_appendText( pNodeInfo->pwcText, pNodeInfo->ulLen);
#if DBG == 1
                    _pDocument->_cbText += pNodeInfo->ulLen * sizeof(TCHAR);
#endif
                    goto NoNode;
                }
                break;
            }
        }
        else if (pNodeInfo->dwType == XML_DTDSUBSET)
        {
            String * pSubset = String::newString(pNodeInfo->pwcText, 0, pNodeInfo->ulLen);
            DTD * pDTD = _pDocument->getDTD();
            pDTD->setSubsetText( pSubset);
            goto NoNode;
        }
        else if (p.iNodeType > Element::XMLDECL)
            goto NoNode;
        pNode = Node::newNodeFast((Element::NodeType)p.iNodeType, null, pParent,
                                  null, pNodeInfo->pwcText, pNodeInfo->ulLen, 
                                  _pDocument, _pNodeMgr);

        if ((Element::NodeType)p.iNodeType == Element::PCDATA)
        {
            _pLastTextNode = pNode;
#if DBG == 1
            _pDocument->_cTextNodes++;
#endif
        }
#if DBG == 1
        _pDocument->_cNodes++;
        _pDocument->_cbText += pNodeInfo->ulLen * sizeof(TCHAR);
#endif
    }

    _fCollapsed = false;
    pNode->setFinished(fTerminal);
    pNodeInfo->pNode = pNode;
    pNodeInfo->pReserved = NULL;
    
NoNode:
    return S_OK;
}

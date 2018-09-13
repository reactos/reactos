/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#include "node.hxx"
#include "ie4nodefactory.hxx"

#ifndef _XML_OM_DOMNODE
#include "xml/om/domnode.hxx"
#endif

#ifndef _XMLNAMES_HXX
#include "xmlnames.hxx"
#endif

#ifndef _NODEPROPERITY_HXX
#include "nodeproperty.hxx"
#endif

NodeProperties nodeProperties[] = 
{
    // -------------- Container Nodes -----------------
    {Node::ANY, 0, 0, 0, 0, 0},           // ANY
    {Node::ELEMENT, 1, 1, 1, 0, 0},       // XML_ELEMENT = 1,    // <foo ... >
    {Node::ATTRIBUTE, 1, 1, 0, 0, 0},     // XML_ATTRIBUTE,      // <foo bar=...> 
    {Node::PI, 1, 0, 0, 0, 0},            // XML_PI,             // <?foo ...?>   
    {Node::XMLDECL, 1, 0, 1, 0, 0},       // XML_XMLDECL,        // <?xml version=...
    {Node::DOCTYPE, 1, 1, 1, 0, 0},       // XML_DOCTYPE,        // <!DOCTYPE          
    {Node::ATTRIBUTE, 1, 0, 0, 0, 0},     // XML_DTDATTRIBUTE,   // properties of DTD declarations (like XML_SYSTEM)
    {Node::ENTITY, 1, 0, 0, 0, 0},        // XML_ENTITYDECL,     // <!ENTITY ...       
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_ELEMENTDECL,    // <!ELEMENT ...      
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_ATTLISTDECL,    // <!ATTLIST ...        
    {Node::NOTATION, 1, 0, 0, 0, 0},      // XML_NOTATION,       // <!NOTATION ...
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_GROUP,          // The ( ... ) grouping in content models.
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_INCLUDESECT,    // <![ INCLUDE [... ]]>  

    // -------------- Terminal Nodes -------------------
    {Node::PCDATA, 0, 0, 0, 1, 0},        // XML_PCDATA,         // text inside a node or an attribute.
    {Node::CDATA, 0, 0, 0, 1, 0},         // XML_CDATA,          // <![CDATA[...]]>    
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_IGNORESECT,     // <![ IGNORE [...]]>
    {Node::COMMENT, 0, 0, 0, 0, 0},       // XML_COMMENT,        // <!--' and '-->
    {Node::ENTITYREF, 1, 0, 0, 0, 0},     // XML_ENTITYREF,      // &foo;
    {Node::PCDATA, 0, 0, 0, 1, 0},        // XML_WHITESPACE,     // white space between elements 
    {Node::PCDATA, 0, 0, 0, 0, 0},        // XML_NAME,           // general NAME token for typed attribute values or DTD declarations
    {Node::PCDATA, 0, 0, 0, 0},           // XML_NMTOKEN,        // general NMTOKEN for typed attribute values or DTD declarations
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_STRING,         // general quoted string literal in DTD declarations.
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_PEREF,          // %foo;        
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_MODEL,          // EMPTY, ANY of MIXED.
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_ATTDEF,         // Name of attribute being defined.
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_ATTTYPE,
    {Node::ANY, 0, 0, 0, 0, 0},           // XML_ATTPRESENCE,

    {Node::CDATA, 0, 0, 0, 0},         // XML_DTDSUBSET,      // entire DTD subset as a string. 
};


//////////////////////////////////////////////////////////////////////////////////////
// class IE4NodeFactory

IE4NodeFactory::IE4NodeFactory( Document * pDocument)
: _NDNodeFactory(pDocument)
{
    Assert(pDocument);
    _pNamespaceMgr = pDocument->getNamespaceMgr();
}

IE4NodeFactory::~IE4NodeFactory()
{
    _pNamespaceMgr = NULL;
}

HRESULT STDMETHODCALLTYPE 
IE4NodeFactory::NotifyEvent( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
    /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
    HRESULT hr = S_OK;

    TRY
    {
        switch (iEvt)
        {
        case XMLNF_STARTDOCUMENT:
            _pLastTextNode = null;
            break;
        case XMLNF_ENDPROLOG:
            _pDocument->onEndProlog();
            break;
        case XMLNF_ENDDOCUMENT:
            _pDocument->HandleEndDocument();  // tell document to find out that we've finished.
            _pLastTextNode = null;
            _ulBufUsed = 0;    
            _pBuffer = null;
            break;
        case XMLNF_DATAAVAILABLE:
            _pDocument->onDataAvailable();
            break;
        }
    }
    CATCH
    {
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDocument);
    }
    ENDTRY

    return hr;
}    


HRESULT STDMETHODCALLTYPE 
IE4NodeFactory::BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
{
    _pLastTextNode = null;
    _fCollapsed = false;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE 
IE4NodeFactory::EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo) 
{
    HRESULT hr = S_OK;

    TRY
    {
        Assert(pNodeInfo->pNode);
        
        Node * pNode = ((Node*)pNodeInfo->pNode);
        pNode->setFinished(true);
        if (_fTextInBuffer)
            bufferAttach();
        _pLastTextNode = null;
        _fCollapsed = false;
    }
    CATCH
    {
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDocument);
    }
    ENDTRY

    return hr;
}


HRESULT STDMETHODCALLTYPE 
IE4NodeFactory::Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    HRESULT hr;

    TRY
    {
        _pDocument->_clearDocNode();
        _pDocument->HandleEndDocument();
        _pLastTextNode = null;
        _ulBufUsed = 0;    
        _pBuffer = null;
        hr = hrErrorCode;
    }
    CATCH
    {
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDocument);
    }
    ENDTRY

    return hr;
}


bool
ProcessXmlSpace(Node * pNode, bool fIE4)
{
    Assert(pNode);
    Assert(Element::ATTRIBUTE == pNode->getNodeType());
    String * pS = pNode->getInnerText(false, true, false);
    if (pS)
    {
        if (fIE4)
        {
            if (pS->equalsIgnoreCase( const_cast<TCHAR *>(XMLNames::pszPreserve)))
                return true;
            else if (pS->equalsIgnoreCase( const_cast<TCHAR *>(XMLNames::pszDefault)))
                return false;
        }
        else
        {
            if (pS->equals( const_cast<TCHAR *>(XMLNames::pszPreserve)))
                return true;
            else if (pS->equals( const_cast<TCHAR *>(XMLNames::pszDefault)))
                return false;
        }
    }
    Exception::throwE(MSG_E_INVALIDXMLSPACE, MSG_E_INVALIDXMLSPACE, null);
    return false;
}


HRESULT STDMETHODCALLTYPE 
IE4NodeFactory::CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ PVOID pNodeParent, 
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    HRESULT hr = S_OK;
    Node * pParent;
    Element::NodeType eNodeType;
    NameDef * pName;
    Node * pNode, *pElementNode;
    bool fInAttribute;
    bool fXmlSpace, fPreserve;
    XML_NODE_INFO* pLastAttr;
    XML_NODE_INFO* pPrimaryNodeInfo;

    TRY {
        pParent = (Node*)pNodeParent;
        fInAttribute = false;
        fXmlSpace = fPreserve = false;
        pPrimaryNodeInfo = *apNodeInfo;

        for (int i = cNumRecs; i > 0; i--)
        {
            XML_NODE_INFO* pNodeInfo = *apNodeInfo++;
            Assert(pNodeInfo->dwType >= 0 && pNodeInfo->dwType < XML_LASTNODETYPE);

            pNode = null;
            NodeProperties p = nodeProperties[pNodeInfo->dwType];

            if (_fTextInBuffer)
            {
                Assert(_pLastTextNode);
                if (p.HasName() || (pNodeInfo->dwType != XML_PCDATA && pNodeInfo->dwType != XML_WHITESPACE))
                {
                    // copy text out of buffer
                    bufferAttach();
                }
                else
                {
                    // append text to buffer
                    Assert(!p.HasName() && p.IsData() 
                           && (XML_PCDATA == pNodeInfo->dwType || XML_WHITESPACE == pNodeInfo->dwType));
                    bufferAppend(pNodeInfo->pwcText, pNodeInfo->ulLen);
                    goto NextNode;
                }
            }

            if (p.HasName())
            {
                pName = _pNamespaceMgr->createNameDef(pNodeInfo->pwcText, pNodeInfo->ulLen);
                pNode = Node::newNodeFast((Element::NodeType)p.iNodeType, pName, pParent,
                                          null, null, 0, 
                                          _pDocument, _pNodeMgr);
            }
            else
            {
                Assert(pParent);
                
                if (p.IsData())
                {
                    switch (pNodeInfo->dwType)
                    {
                    case XML_WHITESPACE:
                        if (! _fCollapsed)
                        {
                            Node * pDataNode = pParent->_pLast;
                            if (!pDataNode || pDataNode != _pLastTextNode)
                            {
                                Node * pPrevSib = pParent->getNodeLastChild();
                                if ( pPrevSib)
                                    pPrevSib->setWSFollow( true);
                                else if ( pParent)
                                    pParent->setWSInner( true);          
                                goto NextNode; 
                            }
                        }
                        // fall through

                    case XML_PCDATA:
                        if (bufferAppend(pParent, pNodeInfo->pwcText, pNodeInfo->ulLen))
                            goto NextNode;
                        break;

                    case XML_CDATA:
                        if (pParent->getNodeType() == Element::PI)
                        {
                            // if we are tyring to attach a text node to a node which doesn't have children, 
                            //  just stick the text on the parent
                            // for now, the only case should be PIs
                            pParent->_appendText( pNodeInfo->pwcText, pNodeInfo->ulLen);
                            goto NextNode;
                        }
                        break;
                    }
                }
                else if (p.iNodeType > Element::XMLDECL)
                    goto NextNode;                


                pNode = Node::newNodeFast((Element::NodeType)p.iNodeType, null, pParent,
                                          null, pNodeInfo->pwcText, pNodeInfo->ulLen, 
                                          _pDocument, _pNodeMgr);
                if ((Node::NodeType)p.iNodeType == Element::PCDATA)
                    _pLastTextNode = pNode;
            }

            _fCollapsed = false;
            pNode->setFinished(pNodeInfo->fTerminal == TRUE);
            pNodeInfo->pNode = pNode;

NextNode:
            // Process the attributes
            if (i > 1)
            {
                if (p.IsContainer())
                {
                    pParent = pNode;
                    pElementNode = pNode;
                }
                else if (pNodeInfo->dwType == XML_ATTRIBUTE)
                {
                    pParent = pNode;
                    if (pNode->getName()->toString()->equalsIgnoreCase(XMLNames::pszXMLSpace))
                        fXmlSpace = true;
                }

                // see if next node is an attribute...
                pNodeInfo = *apNodeInfo;
                if (pNodeInfo->dwType == XML_ATTRIBUTE)
                {
                    if (_fTextInBuffer)
                        bufferAttach();
                    _pLastTextNode = null;
                    _fCollapsed = false;
                    if (fInAttribute)
                    {
                        Node * pAttrNode = (Node*)pLastAttr->pNode;
                        if (fXmlSpace)
                            fPreserve = ProcessXmlSpace(pAttrNode, true);
                        pAttrNode->setFinished(true);
                    }
                    fXmlSpace = false;
                    pLastAttr = pNodeInfo;
                    fInAttribute = true;
                    pParent = pElementNode; // reset parent back to element node
                }
            }

        } // for 

        if (fInAttribute)
        {
            if (_fTextInBuffer)
            {
                Assert(_pLastTextNode);
                // copy text out of buffer
                bufferAttach();
            }
            Node * pAttrNode = (Node*)pLastAttr->pNode;
            if (fXmlSpace)
                fPreserve = ProcessXmlSpace(pAttrNode, true);
            pAttrNode->setFinished(true);
            _pLastTextNode = null;
            _fCollapsed = false;
        }   

    }
    CATCH
    {
        hr = AbortParse(pNodeSource, GETEXCEPTION(), _pDocument);
    }
    ENDTRY

Cleanup:
    return hr;
}

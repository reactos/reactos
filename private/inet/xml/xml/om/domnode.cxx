/*
 * @(#)IXMLDOMNode.cxx 1.0 3/13/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#undef SINGLE_THREAD_OM

#include "domnode.hxx"

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#ifndef _XML_PARSER_XMLNAMES
#include "xmlnames.hxx"
#endif

#ifndef _XML_PARSER_DTD
#include "dtd.hxx"
#endif

#ifndef _XML_PARSER_ELEMENTDECL
#include "elementdecl.hxx"
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

#ifndef _BSTR_HXX
#include "core/com/bstr.hxx"
#endif

#ifndef _VARIANT_HXX
#include "core/com/variant.hxx"
#endif

#ifndef _CORE_UTIL_CHARTYPE_HXX
#include "core/util/chartype.hxx"
#endif

#ifndef _XML_OM_OMLOCK
#include "xml/om/omlock.hxx"
#endif

#ifndef _XML_OM_DOCSTREAM
#include "xml/om/docstream.hxx"
#endif

#include <xmldomdid.h>


#define CleanupTEST(t)  if (!t) { hr=S_FALSE; goto Cleanup; }
#define ARGTEST(t)  if (!t) { hr=E_INVALIDARG; goto Cleanup; }


DeclareTag(tagDOMNode, "DOMNODE", "DOMNODE");
#if DBG == 1
DOMNode * s_pDOMNodeDebug = 0;
#endif


/*********************** DOMNode ***********************/

const IID DOMNode::s_IID = { 0x6d7fc382, 0x20bf, 0x11d2, { 0x83, 0x6e, 0x0, 0x0, 0xf8, 0x7a, 0x77, 0x82 } };


int aNodeType2DOMNodeType[] =
{
    NODE_ELEMENT,                  // ELEMENT = 0,
    NODE_TEXT,                     // PCDATA = 1,
    NODE_COMMENT,                  // COMMENT = 2,
    NODE_DOCUMENT,                 // DOCUMENT = 3,
    NODE_DOCUMENT_TYPE,            // DOCTYPE = 4,
    NODE_PROCESSING_INSTRUCTION,   // PI = 5,
    NODE_CDATA_SECTION,            // CDATA = 6,     
    NODE_ENTITY,                  // ENTITY = 7,     
    NODE_NOTATION,                 // NOTATION = 8,     
    -1,                            // ELEMENTDECL = 9,     
    NODE_PROCESSING_INSTRUCTION,   // NAMESPACE = 10,
    NODE_ENTITY_REFERENCE,         // ENTITYREF = 11,
    NODE_TEXT,                     // WHITESPACE = 12,
    -1,                            // INCLUDESECTION = 13,
    -1,                            // IGNORESECTION = 14,
    NODE_ATTRIBUTE,                // ATTRIBUTE = 15,
    -1,                            // TYPEDVALUE = 16,
    NODE_DOCUMENT_FRAGMENT,        // DOCFRAG = 17,
    NODE_PROCESSING_INSTRUCTION,   // XMLDECL = 18,
};


DOMNode *
DOMNode::Variant2DOMNode( VARIANT * pVar)
{
    return reinterpret_cast<DOMNode *>(Variant::QIForIID(pVar, &DOMNode::s_IID));
}


int
_NodeType2DOMNodeType(Element::NodeType eNodeType)
{
    Assert( eNodeType < LENGTH(aNodeType2DOMNodeType) );
    return aNodeType2DOMNodeType[(int)eNodeType];
}


DeclareTag(tagDOMOM, "DOMNode", "COM DOM");

#define ASSERTDOMNODE(x) Assert( x == null || CHECKTYPEID(*x, DOMNode))


#if DBG == 1
LONG _domnodeCount;
#endif

DOMNode::DOMNode(Node * pNode) 
#ifdef RENTAL_MODEL
    : _dispatchEx<IXMLDOMNode, &LIBID_MSXML, &IID_IXMLDOMNode, false>(pNode->model())
#else
    : _dispatchEx<IXMLDOMNode, &LIBID_MSXML, &IID_IXMLDOMNode, false>()
#endif
{ 
    _pNode = null;
    _pW3CWrapper = null;
    init(pNode); 
#if DBG == 1
    InterlockedIncrement(&_domnodeCount);
#endif
}


DOMNode::~DOMNode()
{
    reset();
    release(&_pW3CWrapper);
#if DBG == 1
    InterlockedDecrement(&_domnodeCount);
#endif
}


void DOMNode::init(Node * pNode)
{
    TraceTag((tagDOMNode, "Created DOMNode for %X", getNodeData()));
#if DBG == 1
    if (s_pDOMNodeDebug == this)
        DebugBreak();
#endif
    Assert(_pNode == 0);
    _pNode = pNode;
    pNode->_addRef();
    pNode->_addRef(); // double addref to guarantee that it's refCoutn >= 2
}


void DOMNode::reset()
{
    TraceTag((tagDOMNode, "Deleted DOMNode for %X", getNodeData()));
#if DBG == 1
    if (s_pDOMNodeDebug == this)
        DebugBreak();
#endif
    if (_pNode)
    {
        _pNode->_release();
        _pNode->_release(); // see init() above for why we do this
        _pNode = null;
    }
}


ULONG STDMETHODCALLTYPE 
DOMNode::Release()
{
    ULONG ul = Decrement();
    if (ul == 0)
    {
        STACK_ENTRY_IUNKNOWN(this);
        TRY
        {
            int i;
            ULONG refs = Refs();
            reset();
            if (refs & REF_MARKED)
            {
                Assert(CHECKTYPEID(*this, DOMNode));
                DOMNode ** ppDOMNodes;
#ifdef RENTAL_MODEL
                if (refs & REF_RENTAL)
                    ppDOMNodes = _apDOMNodesRental;
                else
#endif
                    ppDOMNodes = _apDOMNodes;
                for (i = 0; i < DOMNODECACHESIZE; i++)
                {
                    if (null == *ppDOMNodes &&
                        InterlockedCompareExchange((void **)ppDOMNodes, this, null) == null)
                    {
                        TraceTag((tagRefCount, "DecrementComponents - DOMNode::Release()"));
#if DBG == 1
                        InterlockedDecrement(&_domnodeCount);
#endif
                        ::DecrementComponents();
                        break;
                    }
                    ppDOMNodes++;
                }
                if (i == DOMNODECACHESIZE)
                    delete this;
            }
            else
            {
                Assert(CHECKTYPEID(*this, DOMDocumentWrapper));
                delete this;
            }
        }
        CATCH
        {
        }
        ENDTRY
    }
    return ul;
}


DOMNode *
DOMNode::newDOMNode(Node * pNode)
{
    int i;
    DOMNode * pDOMNode = NULL;
    DOMNode ** ppDOMNodes;
    ULONG rental = pNode->isRental() ? (ULONG)REF_RENTAL : 0;
#ifdef RENTAL_MODEL
    if (0 != rental)
        ppDOMNodes = _apDOMNodesRental;
    else
#endif
        ppDOMNodes = _apDOMNodes;

    for (i = 0; i < DOMNODECACHESIZE; i++)
    {
        // only try an interlocked exchange if it looks like it might be successful
        if (null != *ppDOMNodes &&
            null != (pDOMNode = (DOMNode *)INTERLOCKEDEXCHANGE_PTR(ppDOMNodes, null)))
        {
            pDOMNode->AddRef();
            pDOMNode->init(pNode);
            TraceTag((tagRefCount, "IncrementComponents - DOMNode::newDOMNode(Node * pNode)"));
#if DBG == 1
            InterlockedIncrement(&_domnodeCount);
#endif
            ::IncrementComponents(); 
            break;
        }
        ppDOMNodes++;
    }
    if (!pDOMNode)
    {
        pDOMNode = new DOMNode(pNode);
        // mark it so we now when released that it is safe to cache
        if (pDOMNode)
            pDOMNode->SetBits((ULONG)REF_MARKED | rental);
    }

    return pDOMNode;
}


DOMNode * DOMNode::_apDOMNodes[DOMNODECACHESIZE];
#ifdef RENTAL_MODEL
DOMNode * DOMNode::_apDOMNodesRental[DOMNODECACHESIZE];
#endif

void
DOMNode::classExit()
{
    DOMNode ** apDOMNodes = _apDOMNodes;
#ifdef RENTAL_MODEL
    while (1)
#endif
    {
        for (int i = 0; i < DOMNODECACHESIZE; i++)
        {
            DOMNode * pDOMNode = apDOMNodes[i];
            if (pDOMNode)
            {
                TraceTag((tagRefCount, "IncrementComponents - DOMNode::classExit()"));
#if DBG == 1
                InterlockedIncrement(&_domnodeCount);
#endif
                // increment component count so that it doesn't go negative
                // when we decrement it in the destructor
                ::IncrementComponents(); 
                DOMNode::deleteDOMNode(pDOMNode);
            }
        }

#ifdef RENTAL_MODEL
        if (apDOMNodes == _apDOMNodesRental)
            break;
        // now free the rental threaded wrappers
        apDOMNodes = _apDOMNodesRental;
#endif
    }
}


void 
DOMNode::deleteDOMNode(DOMNode * pDOMNode)
{
    delete pDOMNode;
}


Document * 
DOMNode::getDocument()
{
    Assert(getNodeData()->getDocument() != null);
    return getNodeData()->getDocument();
}


W3CDOMWrapper * 
DOMNode::getW3CWrapper()
{
    if (!_pW3CWrapper)
    {
        W3CDOMWrapper * p = new W3CDOMWrapper(this);
        if (InterlockedCompareExchange((void **)&_pW3CWrapper, p, null) != null)
        {
            p->Release();
        }
    }

    return _pW3CWrapper;
}

HRESULT STDMETHODCALLTYPE 
DOMNode::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr = S_OK;

    TraceTag((tagDOMNode, "DOMNode::QueryInterface"));

    hr = getNodeData()->QIHelper( NULL, this, NULL, NULL, iid, ppv);

    return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data structures for customized idispatch implementation for IXMLDOMElement
//

static VARTYPE s_argTypes_IXMLDOMElement_getAttribute[] =           {VT_BSTR};
static VARTYPE s_argTypes_IXMLDOMElement_setAttribute[] =           {VT_BSTR, VT_VARIANT};
static VARTYPE s_argTypes_IXMLDOMElement_removeAttribute[] =        {VT_BSTR};
static VARTYPE s_argTypes_IXMLDOMElement_getAttributeNode[] =       {VT_BSTR};
static VARTYPE s_argTypes_IXMLDOMElement_setAttributeNode[] =       {VT_DISPATCH};
static VARTYPE s_argTypes_IXMLDOMElement_removeAttributeNode[] =    {VT_DISPATCH};
static VARTYPE s_argTypes_IXMLDOMElement_getElementsByTagName[] =   {VT_BSTR};

static const IID * s_argTypeIIDs_IXMLDOMElement_setAttributeNode[] = {&IID_IXMLDOMAttribute};
static const IID * s_argTypeIIDs_IXMLDOMElement_removeAttributeNode[] = {&IID_IXMLDOMAttribute};


INVOKE_METHOD 
s_rgIXMLDOMElementMethods[] =
{
    { L"getAttribute",           99, 1, s_argTypes_IXMLDOMElement_getAttribute,         NULL, VT_VARIANT,  DISPATCH_METHOD },
    { L"getAttributeNode",      102, 1, s_argTypes_IXMLDOMElement_getAttributeNode,     NULL, VT_DISPATCH, DISPATCH_METHOD },
    { L"getElementsByTagName",  105, 1, s_argTypes_IXMLDOMElement_getElementsByTagName, NULL, VT_DISPATCH, DISPATCH_METHOD },
    { L"normalize",             106, 0, NULL,                                           NULL, VT_ERROR,    DISPATCH_PROPERTYGET | DISPATCH_METHOD },
    { L"removeAttribute",       101, 1, s_argTypes_IXMLDOMElement_removeAttribute,      NULL, VT_ERROR,    DISPATCH_METHOD },
    { L"removeAttributeNode",   104, 1, s_argTypes_IXMLDOMElement_removeAttributeNode, s_argTypeIIDs_IXMLDOMElement_removeAttributeNode, VT_DISPATCH, DISPATCH_METHOD },
    { L"setAttribute",          100, 2, s_argTypes_IXMLDOMElement_setAttribute,         NULL, VT_ERROR,    DISPATCH_METHOD },
    { L"setAttributeNode",      103, 1, s_argTypes_IXMLDOMElement_setAttributeNode,     s_argTypeIIDs_IXMLDOMElement_setAttributeNode,   VT_DISPATCH, DISPATCH_METHOD },
    { L"tagName",               97,  0, NULL,                                           NULL, VT_BSTR,      DISPATCH_PROPERTYGET}
};


DISPIDTOINDEX 
s_IXMLDOMElement_DispIdMap[] =
{
  { 97, 8}, // IXMLDOMElement::tagName
  { 99, 0}, // IXMLDOMElement::getAttribute
  {100, 6}, // IXMLDOMElement::setAttribute
  {101, 4}, // IXMLDOMElement::removeAttribute
  {102, 1}, // IXMLDOMElement::getAttributeNode
  {103, 7}, // IXMLDOMElement::setAttributeNode
  {104, 5}, // IXMLDOMElement::removeAttributeNode
  {105, 2}, // IXMLDOMElement::getElementsByTagName
  {106, 3} // IXMLDOMElement::normalize
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data structures for customized idispatch implementation for DOMNode
//

static VARTYPE s_argTypes_DOMNode_appendChild[] =   { VT_DISPATCH };
static VARTYPE s_argTypes_DOMNode_attributes[] =    { VT_I4 };
static VARTYPE s_argTypes_DOMNode_childNodes[] =    { VT_I4 };
static VARTYPE s_argTypes_DOMNode_cloneNode[] =     { VT_BOOL };
static VARTYPE s_argTypes_DOMNode_dataType[] =      { VT_BSTR };
static VARTYPE s_argTypes_DOMNode_insertBefore[] =  { VT_DISPATCH, VT_VARIANT };
static VARTYPE s_argTypes_DOMNode_nodeTypedValue[] = { VT_VARIANT };
static VARTYPE s_argTypes_DOMNode_nodeValue[] =     { VT_VARIANT };
static VARTYPE s_argTypes_DOMNode_replaceChild[] =  { VT_DISPATCH, VT_DISPATCH };
static VARTYPE s_argTypes_DOMNode_removeChild[] =   { VT_DISPATCH };
static VARTYPE s_argTypes_DOMNode_selectNodes[] =   { VT_BSTR };
static VARTYPE s_argTypes_DOMNode_selectSingleNode[] = { VT_BSTR };
static VARTYPE s_argTypes_DOMNode_text[] =          { VT_BSTR };
static VARTYPE s_argTypes_DOMNode_transformNode[] = { VT_DISPATCH };
static VARTYPE s_argTypes_DOMNode_transformNodeToObject[] = { VT_DISPATCH, VT_VARIANT };

static const IID*   s_argTypeIds_DOMNode_appendChild[] = { &IID_IXMLDOMNode };
static const IID*   s_argTypeIds_DOMNode_insertBefore[] = { &IID_IXMLDOMNode, NULL };
static const IID*   s_argTypeIds_DOMNode_replaceChild[] = { &IID_IXMLDOMNode, &IID_IXMLDOMNode };
static const IID*   s_argTypeIds_DOMNode_removeChild[] = { &IID_IXMLDOMNode };
static const IID*   s_argTypeIds_DOMNode_transformNode[] = { &IID_IXMLDOMNode };
static const IID*   s_argTypeIds_DOMNode_transformNodeToObject[] = { &IID_IXMLDOMNode, NULL };


INVOKE_METHOD 
s_rgDOMNodeMethods[] =
{
    // name                dispid                       argument number  argument table                  argument guid id                  return type    invoke kind
    { L"appendChild",     DISPID_DOM_NODE_APPENDCHILD,  1,  s_argTypes_DOMNode_appendChild, s_argTypeIds_DOMNode_appendChild, VT_DISPATCH, DISPATCH_METHOD },
    { L"attributes",      DISPID_DOM_NODE_ATTRIBUTES,   1,  s_argTypes_DOMNode_attributes,  NULL,                             VT_DISPATCH, DISPATCH_PROPERTYGET | DISPATCH_METHOD },

    { L"baseName",        DISPID_XMLDOM_NODE_BASENAME,  0,  NULL,    NULL,         VT_BSTR,     DISPATCH_PROPERTYGET },

    { L"childNodes",      DISPID_DOM_NODE_CHILDNODES,   1,  s_argTypes_DOMNode_childNodes,  NULL,                             VT_DISPATCH, DISPATCH_PROPERTYGET | DISPATCH_METHOD }, 
    { L"cloneNode",       DISPID_DOM_NODE_CLONENODE,    1,  s_argTypes_DOMNode_cloneNode,   NULL,                             VT_DISPATCH, DISPATCH_METHOD },

    { L"dataType",        DISPID_XMLDOM_NODE_DATATYPE,  1,  s_argTypes_DOMNode_dataType,    NULL,                             VT_VARIANT,    DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },
    { L"definition",      DISPID_XMLDOM_NODE_DEFINITION,0,  NULL,    NULL,         VT_DISPATCH,     DISPATCH_PROPERTYGET },

    { L"firstChild",      DISPID_DOM_NODE_FIRSTCHILD,   0,  NULL,    NULL,         VT_DISPATCH, DISPATCH_PROPERTYGET },

    { L"hasChildNodes",   DISPID_DOM_NODE_HASCHILDNODES,0,  NULL,    NULL,         VT_BOOL,     DISPATCH_PROPERTYGET | DISPATCH_METHOD },

    { L"insertBefore",    DISPID_DOM_NODE_INSERTBEFORE, 2,  s_argTypes_DOMNode_insertBefore, s_argTypeIds_DOMNode_insertBefore, VT_DISPATCH, DISPATCH_METHOD },

    { L"lastChild",       DISPID_DOM_NODE_LASTCHILD,    0,  NULL,    NULL,         VT_DISPATCH, DISPATCH_PROPERTYGET },

    { L"namespaceURI",    DISPID_XMLDOM_NODE_NAMESPACE, 0,  NULL,    NULL,         VT_BSTR,     DISPATCH_PROPERTYGET },
    { L"nextSibling",     DISPID_DOM_NODE_NEXTSIBLING,  0,  NULL,    NULL,         VT_DISPATCH, DISPATCH_PROPERTYGET },
    { L"nodeName",        DISPID_DOM_NODE_NODENAME,     0,  NULL,    NULL,         VT_BSTR,     DISPATCH_PROPERTYGET },
    { L"nodeType",        DISPID_DOM_NODE_NODETYPE,     0,  NULL,    NULL,         VT_I4,       DISPATCH_PROPERTYGET },
    { L"nodeTypedValue",  DISPID_XMLDOM_NODE_NODETYPEDVALUE, 1,s_argTypes_DOMNode_nodeTypedValue, NULL, VT_VARIANT, DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },
    { L"nodeTypeString",  DISPID_XMLDOM_NODE_STRINGTYPE,0,  NULL,    NULL,         VT_BSTR,     DISPATCH_PROPERTYGET },
    { L"nodeValue",       DISPID_DOM_NODE_NODEVALUE,    1,  s_argTypes_DOMNode_nodeValue,         NULL,  VT_VARIANT,    DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },

    { L"ownerDocument",   DISPID_DOM_NODE_OWNERDOC,     0,  NULL,    NULL,         VT_DISPATCH, DISPATCH_PROPERTYGET },

    { L"parentNode",      DISPID_DOM_NODE_PARENTNODE,   0,  NULL,    NULL,         VT_DISPATCH, DISPATCH_PROPERTYGET },
    { L"parsed",          DISPID_XMLDOM_NODE_PARSED,    0,  NULL,    NULL,         VT_BOOL,     DISPATCH_PROPERTYGET },
    { L"prefix",          DISPID_XMLDOM_NODE_PREFIX,    0,  NULL,    NULL,         VT_BSTR,     DISPATCH_PROPERTYGET },
    { L"previousSibling", DISPID_DOM_NODE_PREVIOUSSIBLING, 0, NULL,  NULL,         VT_DISPATCH, DISPATCH_PROPERTYGET },

    { L"removeChild",     DISPID_DOM_NODE_REMOVECHILD,  1,  s_argTypes_DOMNode_removeChild, s_argTypeIds_DOMNode_removeChild, VT_DISPATCH, DISPATCH_METHOD },
    { L"replaceChild",    DISPID_DOM_NODE_REPLACECHILD, 2,  s_argTypes_DOMNode_replaceChild, s_argTypeIds_DOMNode_replaceChild, VT_DISPATCH, DISPATCH_METHOD },

    { L"selectNodes",     DISPID_XMLDOM_NODE_SELECTNODES,1, s_argTypes_DOMNode_selectNodes, NULL, VT_DISPATCH, DISPATCH_METHOD },
    { L"selectSingleNode",DISPID_XMLDOM_NODE_SELECTSINGLENODE, 1, s_argTypes_DOMNode_selectSingleNode, NULL, VT_DISPATCH, DISPATCH_METHOD },
    { L"specified",       DISPID_XMLDOM_NODE_SPECIFIED, 0,  NULL,    NULL,         VT_BOOL,     DISPATCH_PROPERTYGET },

    { L"text",            DISPID_XMLDOM_NODE_TEXT,      1,  s_argTypes_DOMNode_text,  NULL,      VT_BSTR,     DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT },
    { L"transformNode",   DISPID_XMLDOM_NODE_TRANSFORMNODE, 1, s_argTypes_DOMNode_transformNode, s_argTypeIds_DOMNode_transformNode, VT_BSTR, DISPATCH_METHOD },
    { L"transformNodeToObject", DISPID_XMLDOM_NODE_TRANSFORMNODETOOBJECT, 2, s_argTypes_DOMNode_transformNodeToObject, s_argTypeIds_DOMNode_transformNodeToObject, VT_ERROR, DISPATCH_METHOD },

    { L"xml",             DISPID_XMLDOM_NODE_XML,       0,  NULL,    NULL,         VT_BSTR,     DISPATCH_PROPERTYGET },
};


//
// Map dispid to corresponding index in s_rgDOMNodeMethods[]
DISPIDTOINDEX 
s_DOMNode_DispIdMap[] =
{
    {DISPID_DOM_NODE_NODENAME, 13 },
    {DISPID_DOM_NODE_NODEVALUE, 17 },
    {DISPID_DOM_NODE_NODETYPE, 14 },
    {DISPID_DOM_NODE_PARENTNODE, 19 },
    {DISPID_DOM_NODE_CHILDNODES, 3 },
    {DISPID_DOM_NODE_FIRSTCHILD, 7 },
    {DISPID_DOM_NODE_LASTCHILD, 10 },
    {DISPID_DOM_NODE_PREVIOUSSIBLING, 22 },
    {DISPID_DOM_NODE_NEXTSIBLING, 12 },
    {DISPID_DOM_NODE_ATTRIBUTES, 1 },
    {DISPID_DOM_NODE_INSERTBEFORE, 9 },
    {DISPID_DOM_NODE_REPLACECHILD, 24 },
    {DISPID_DOM_NODE_REMOVECHILD, 23 },
    {DISPID_DOM_NODE_APPENDCHILD, 0 },
    {DISPID_DOM_NODE_HASCHILDNODES, 8 },
    {DISPID_DOM_NODE_OWNERDOC, 18 },
    {DISPID_DOM_NODE_CLONENODE, 4 },
    {DISPID_XMLDOM_NODE_STRINGTYPE, 16 },
    {DISPID_XMLDOM_NODE_SPECIFIED, 27  },
    {DISPID_XMLDOM_NODE_DEFINITION, 6 },
    {DISPID_XMLDOM_NODE_TEXT, 28 },
    {DISPID_XMLDOM_NODE_NODETYPEDVALUE, 15 },
    {DISPID_XMLDOM_NODE_DATATYPE, 5   },
    {DISPID_XMLDOM_NODE_XML, 31 },
    {DISPID_XMLDOM_NODE_TRANSFORMNODE, 29 },
    {DISPID_XMLDOM_NODE_SELECTNODES, 25},
    {DISPID_XMLDOM_NODE_SELECTSINGLENODE, 26 },
    {DISPID_XMLDOM_NODE_PARSED, 20 },
    {DISPID_XMLDOM_NODE_NAMESPACE, 11 },
    {DISPID_XMLDOM_NODE_PREFIX, 21 },
    {DISPID_XMLDOM_NODE_BASENAME, 2 },
    {DISPID_XMLDOM_NODE_TRANSFORMNODETOOBJECT, 30}
};


BYTE s_cDOMNodeMethodLen = NUMELEM(s_rgDOMNodeMethods);
BYTE s_cIXMLDOMElementMethodLen = NUMELEM(s_rgIXMLDOMElementMethods);
BYTE s_cDOMNode_DispIdMap = NUMELEM(s_DOMNode_DispIdMap);
BYTE s_cIXMLDOMElement_DispIdMap = NUMELEM(s_IXMLDOMElement_DispIdMap);


DISPATCHINFO _dispatch<IXMLDOMNode, &LIBID_MSXML, &IID_IXMLDOMNode>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDOMNode, &LIBID_MSXML, ORD_MSXML, s_rgDOMNodeMethods, NUMELEM(s_rgDOMNodeMethods), s_DOMNode_DispIdMap, NUMELEM(s_DOMNode_DispIdMap), DOMNode::_invokeDOMNode
};

#define W3CCastTo(n,t) IUnknown * n(W3CDOMWrapper * p) { return (t *)p; }
W3CCastTo( W3CCastElement, IXMLDOMElement)
W3CCastTo( W3CCastText, IXMLDOMText)
W3CCastTo( W3CCastComment, IXMLDOMComment)
W3CCastTo( W3CCastDocType, IXMLDOMDocumentType)
W3CCastTo( W3CCastPI, IXMLDOMProcessingInstruction)
W3CCastTo( W3CCastCDATA, IXMLDOMCDATASection)
W3CCastTo( W3CCastEntity, IXMLDOMEntity)
W3CCastTo( W3CCastEntityRef, IXMLDOMEntityReference)
W3CCastTo( W3CCastNotation, IXMLDOMNotation)
W3CCastTo( W3CCastAttribute, IXMLDOMAttribute)
W3CCastTo( W3CCastDocFrag, IXMLDOMDocumentFragment)

DOMNode::DispInfo DOMNode::aDispInfo[] = 
{
    // ELEMENT = 0,
    { NULL, &IID_IXMLDOMElement, &LIBID_MSXML, ORD_MSXML, s_rgIXMLDOMElementMethods, NUMELEM(s_rgIXMLDOMElementMethods), s_IXMLDOMElement_DispIdMap, NUMELEM(s_IXMLDOMElement_DispIdMap), DOMNode::_invokeDOMElement, W3CCastElement},
    // PCDATA = 1,
    { NULL, &IID_IXMLDOMText, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastText},
    // COMMENT = 2,
    { NULL, &IID_IXMLDOMComment, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastComment},
    // DOCUMENT = 3,
    { NULL, NULL, NULL},
    // DOCTYPE = 4,
    { NULL, &IID_IXMLDOMDocumentType, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastDocType},
    // PI = 5,
    { NULL, &IID_IXMLDOMProcessingInstruction, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastPI},
    // CDATA = 6,     
    { NULL, &IID_IXMLDOMCDATASection, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastCDATA},
    // ENTITY = 7,     
    { NULL, &IID_IXMLDOMEntity, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastEntity},
    // NOTATION = 8,     
    { NULL, &IID_IXMLDOMNotation, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastNotation},
    // _ELEMENTDECL = 9,     
    { NULL, NULL, NULL },
    // NAMESPACE = 10,
    { NULL, &IID_IXMLDOMProcessingInstruction, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastPI},
    // ENTITYREF = 11,
    { NULL, &IID_IXMLDOMEntityReference, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastEntityRef},
    // _WHITESPACE = 12,
    { NULL, NULL, NULL },
    // _INCLUDESECTION = 13,
    { NULL, NULL, NULL },
    // _IGNORESECTION = 14,
    { NULL, NULL, NULL },
    // ATTRIBUTE = 15,
    { NULL, &IID_IXMLDOMAttribute, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastAttribute},
    // TYPEDVALUE = 16,
    { NULL, NULL, NULL},
    // DOCFRAG = 17,
    { NULL, &IID_IXMLDOMDocumentFragment, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastDocFrag},
    // XMLDECL = 18,
    { NULL, &IID_IXMLDOMProcessingInstruction, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL, W3CCastPI},
};




HRESULT STDMETHODCALLTYPE 
DOMNode::GetIDsOfNames( 
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    STACK_ENTRY;
    HRESULT hr;
    Assert(GetComponentCount());

    // IXMLDOMNode method/property?
    hr = _dispatchImpl::FindIdsOfNames( rgszNames, cNames, s_rgDOMNodeMethods, 
                         NUMELEM(s_rgDOMNodeMethods), lcid, rgDispId, false);

    // one not found?
    if( hr == DISP_E_UNKNOWNNAME )
    {
        DispInfo * pInfo = &aDispInfo[getNodeData()->getNodeType()];
        if (pInfo->dispatchinfo._puuid)
            hr = _dispatchImpl::GetIDsOfNames( &pInfo->dispatchinfo,
                                               riid, rgszNames, cNames, lcid, rgDispId);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNode::GetDispID( 
    /* [in] */ BSTR bstrName,
    /* [in] */ DWORD grfdex,
    /* [out] */ DISPID __RPC_FAR *pid)
{
    STACK_ENTRY;
    HRESULT hr;
    Assert(GetComponentCount());

    // IXMLDOMNode method/property?
    // Our typelibs should never be localized, so it's safe to assume a LCID of 0x409
    hr = _dispatchImpl::FindIdsOfNames( &bstrName, 1, s_rgDOMNodeMethods, 
                                        NUMELEM(s_rgDOMNodeMethods), 0x409, pid, grfdex & fdexNameCaseSensitive);

    // one not found?
    if( hr == DISP_E_UNKNOWNNAME )
    {
        DispInfo * pInfo = &aDispInfo[getNodeData()->getNodeType()];
        if (pInfo->dispatchinfo._puuid)
            hr = _dispatchImpl::GetDispID(&pInfo->dispatchinfo, false,
                                          bstrName, grfdex, pid);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
DOMNode::Invoke(
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    STACK_ENTRY;
    Assert(GetComponentCount());
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    if ( DISPID_DOM_W3CWRAPPERS < dispIdMember && dispIdMember < DISPID_DOM_W3CWRAPPERS_TOP)
    {
        W3CDOMWrapper * pWrapper = getW3CWrapper();
        DispInfo * pInfo = &aDispInfo[getNodeData()->getNodeType()];
        if (pInfo->dispatchinfo._puuid)
        {
            hr = _dispatchImpl::Invoke(&pInfo->dispatchinfo, (pInfo->func)(pWrapper),
                                       dispIdMember, riid, lcid, wFlags, pDispParams, 
                                       pVarResult, pExcepInfo, puArgErr);
        }
    }
    else
    {
        hr = _dispatchImpl::Invoke(&s_dispatchinfo, static_cast<IXMLDOMNode*>(this),
                                   dispIdMember, riid, lcid, wFlags, pDispParams, 
                                   pVarResult, pExcepInfo, puArgErr);
    }

    return hr;
}


HRESULT
DOMNode::_invokeDOMElement(
    void * pTarget,
    DISPID dispIdMember,
    INVOKE_ARG *rgArgs, 
    WORD wInvokeType,
    VARIANT *pVarResult,
    UINT cArgs)
{
    HRESULT hr;

    W3CDOMWrapper * pWrapper = (W3CDOMWrapper*)pTarget;

    TEST_METHOD_TABLE(s_rgIXMLDOMElementMethods, NUMELEM(s_rgIXMLDOMElementMethods), s_IXMLDOMElement_DispIdMap, NUMELEM(s_IXMLDOMElement_DispIdMap));

    switch( dispIdMember )
    {
        case DISPID_DOM_ELEMENT_GETTAGNAME:
            hr = pWrapper->get_tagName( &pVarResult->bstrVal );
            break;

        case DISPID_DOM_ELEMENT_GETATTRIBUTE:
            hr = pWrapper->getAttribute( ( BSTR ) VARMEMBER(&rgArgs[0].vArg,bstrVal), pVarResult);
            break;

        case DISPID_DOM_ELEMENT_SETATTRIBUTE:
            hr = pWrapper->setAttribute( ( BSTR ) VARMEMBER(&rgArgs[0].vArg,bstrVal), rgArgs[1].vArg);
            break;

        case DISPID_DOM_ELEMENT_REMOVEATTRIBUTE:
            hr = pWrapper->removeAttribute( ( BSTR ) VARMEMBER(&rgArgs[0].vArg,bstrVal));
            break;

        case DISPID_DOM_ELEMENT_GETATTRIBUTENODE:
            hr = pWrapper->getAttributeNode( ( BSTR ) VARMEMBER(&rgArgs[0].vArg,bstrVal), 
                                             (IXMLDOMAttribute**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_ELEMENT_SETATTRIBUTENODE:
            hr = pWrapper->setAttributeNode( (IXMLDOMAttribute*) V_DISPATCH(&rgArgs[0].vArg), 
                                             (IXMLDOMAttribute**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_ELEMENT_REMOVEATTRIBUTENODE:
            hr = pWrapper->removeAttributeNode( (IXMLDOMAttribute*) V_DISPATCH(&rgArgs[0].vArg), 
                                                (IXMLDOMAttribute**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_ELEMENT_GETELEMENTSBYTAGNAME:
            hr = pWrapper->getElementsByTagName( ( BSTR ) VARMEMBER(&rgArgs[0].vArg,bstrVal), 
                                                 (IXMLDOMNodeList**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_ELEMENT_NORMALIZE:
            hr = pWrapper->normalize(); 
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }

    return hr;
}


#define PROPERTY_INVOKE_READ_NODELIST( pObj, _property, _nodetype)\
    if( DISPATCH_PROPERTYGET & wInvokeType )\
        hr = pObj->get_##_property( (_nodetype**)&pVarResult->pdispVal );\
    else \
    {\
        _nodetype * pNode = NULL;\
        hr = pObj->get_##_property( (_nodetype**)&pNode );\
        if (S_OK == hr)\
        {\
            hr = pNode->get_item( (long)VARMEMBER(&rgArgs[0].vArg, lVal), (IXMLDOMNode**) &pVarResult->pdispVal );\
            pNode->Release();\
        }\
    }\


HRESULT
DOMNode::_invokeDOMNode(
    void * pTarget,
    DISPID dispIdMember,
    INVOKE_ARG *rgArgs, 
    WORD wInvokeType,
    VARIANT *pVarResult,
    UINT cArgs)
{
    HRESULT hr;
    IXMLDOMNode * pObj = (IXMLDOMNode*)pTarget;

    TEST_METHOD_TABLE(s_rgDOMNodeMethods, NUMELEM(s_rgDOMNodeMethods), s_DOMNode_DispIdMap, NUMELEM(s_DOMNode_DispIdMap));

    switch( dispIdMember )
    {
        case DISPID_DOM_NODE_NODENAME:
            hr = pObj->get_nodeName( &pVarResult->bstrVal );
            break;

        case DISPID_DOM_NODE_NODEVALUE:
            PROPERTY_INVOKE_READWRITE_VARIANT( pObj, nodeValue );
            break;

        case DISPID_DOM_NODE_NODETYPE: 
            hr = pObj->get_nodeType( (tagDOMNodeType*) &pVarResult->lVal );
            break;

        case DISPID_DOM_NODE_PARENTNODE:
            hr = pObj->get_parentNode( (IXMLDOMNode**) &pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_CHILDNODES:
            PROPERTY_INVOKE_READ_NODELIST( pObj, childNodes, IXMLDOMNodeList );
            break;

        case DISPID_DOM_NODE_FIRSTCHILD:
            hr = pObj->get_firstChild( (IXMLDOMNode**) &pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_LASTCHILD:
            hr = pObj->get_lastChild( (IXMLDOMNode**) &pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_PREVIOUSSIBLING:
            hr = pObj->get_previousSibling( (IXMLDOMNode**) &pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_NEXTSIBLING:
            hr = pObj->get_nextSibling( (IXMLDOMNode**) &pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_ATTRIBUTES:
            PROPERTY_INVOKE_READ_NODELIST( pObj, attributes, IXMLDOMNamedNodeMap )
            break;

        case DISPID_DOM_NODE_INSERTBEFORE:
            hr = pObj->insertBefore( (IXMLDOMNode*) V_DISPATCH(&rgArgs[0].vArg),
                                rgArgs[1].vArg, (IXMLDOMNode**)&pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_REPLACECHILD:
            hr = pObj->replaceChild( (IXMLDOMNode*) V_DISPATCH(&rgArgs[0].vArg), 
                               (IXMLDOMNode*) V_DISPATCH(&rgArgs[1].vArg), 
                               (IXMLDOMNode**)&pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_REMOVECHILD:
            hr = pObj->removeChild( (IXMLDOMNode*) V_DISPATCH(&rgArgs[0].vArg),
                          (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_NODE_APPENDCHILD:
            hr = pObj->appendChild( (IXMLDOMNode*) V_DISPATCH(&rgArgs[0].vArg),
                          (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_NODE_HASCHILDNODES:
            hr = pObj->hasChildNodes( (VARIANT_BOOL*) &pVarResult->boolVal );
            break;

        case DISPID_DOM_NODE_OWNERDOC:
            hr = pObj->get_ownerDocument( (IXMLDOMDocument**) &pVarResult->pdispVal );
            break;

        case DISPID_DOM_NODE_CLONENODE:
            METHOD_INVOKE_1( pObj, cloneNode, boolVal, VARIANT_BOOL, IXMLDOMNode* );
            break;

        case DISPID_XMLDOM_NODE_STRINGTYPE:
            hr = pObj->get_nodeTypeString( (BSTR*) &pVarResult->bstrVal );
            break;

        case DISPID_XMLDOM_NODE_TEXT:
            PROPERTY_INVOKE_READWRITE( pObj, text, bstrVal, BSTR );
            break;

        case DISPID_XMLDOM_NODE_SPECIFIED:
            hr = pObj->get_specified( &pVarResult->boolVal );
            break;

        case DISPID_XMLDOM_NODE_DEFINITION:
            hr = pObj->get_definition( (IXMLDOMNode**) &pVarResult->pdispVal );
            break;

        case DISPID_XMLDOM_NODE_NODETYPEDVALUE:
            PROPERTY_INVOKE_READWRITE_VARIANT( pObj, nodeTypedValue );
            break;

        case DISPID_XMLDOM_NODE_DATATYPE:
            if( DISPATCH_PROPERTYGET == wInvokeType )
                hr = pObj->get_dataType( pVarResult );
            else
                hr = pObj->put_dataType( ( BSTR ) VARMEMBER(&rgArgs[0].vArg,bstrVal) );
            break;

        case DISPID_XMLDOM_NODE_XML:
            hr = pObj->get_xml( &pVarResult->bstrVal );
            break;

        case DISPID_XMLDOM_NODE_TRANSFORMNODE:
            METHOD_INVOKE_1( pObj, transformNode, pdispVal, IXMLDOMNode*, BSTR );
            break;

        case DISPID_XMLDOM_NODE_SELECTNODES:
            METHOD_INVOKE_1( pObj, selectNodes, bstrVal, BSTR, IXMLDOMNodeList* );
            break;

        case DISPID_XMLDOM_NODE_SELECTSINGLENODE:
            METHOD_INVOKE_1( pObj, selectSingleNode, bstrVal, BSTR, IXMLDOMNode* );
            break;

        case DISPID_XMLDOM_NODE_PARSED:
            hr = pObj->get_parsed( &pVarResult->boolVal );
            break;

        case DISPID_XMLDOM_NODE_NAMESPACE:
            hr = pObj->get_namespaceURI( &pVarResult->bstrVal );
            break;

        case DISPID_XMLDOM_NODE_PREFIX:
            hr = pObj->get_prefix( &pVarResult->bstrVal );
            break;

        case DISPID_XMLDOM_NODE_BASENAME:
            hr = pObj->get_baseName( &pVarResult->bstrVal );
            break;

        case DISPID_XMLDOM_NODE_TRANSFORMNODETOOBJECT:
            hr = pObj->transformNodeToObject( (IXMLDOMNode*) V_DISPATCH(&rgArgs[0].vArg), 
                                         rgArgs[1].vArg ); 
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNode::get_nodeType( 
    /* [out][retval] */ DOMNodeType __RPC_FAR *plType)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNode::get_nodeType()"));

    int eType = _NodeType2DOMNodeType( getNodeData()->getNodeType());
    if ( eType >= 0)
    {
        *plType = (DOMNodeType)eType;
        hr = S_OK;
    }
    else
    {
        *plType = (DOMNodeType)0;
        hr = S_FALSE;
    }

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMNode::get_nodeName( 
    /* [out][retval] */ BSTR __RPC_FAR *pOut)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNode::get_nodeName()"));

    ARGTEST( pOut);

    TRY
    {
        Node * pNode = getNodeData();

        TCHAR * pwc = null;
        String * pstr = null;

        // BUGBUG NEWDOM: implement all types
        switch(pNode->getNodeType())
        {
        case Element::DOCUMENT:
            pwc = _T("#document");
            break;
        case Element::DOCFRAG:
            pwc = _T("#document-fragment");
            break;
        case Element::COMMENT:
            pwc = _T("#comment");
            break;
        case Element::PCDATA:
            pwc = _T("#text");
            break;
        case Element::CDATA:
            pwc = _T("#cdata-section");
            break;

        default:
            {
                NameDef * pName = pNode->getNameDef();
                Assert( pName);
                pstr = pName->toString();
            }
        }

        if (pwc)
        {
            Assert(!pstr);
            *pOut = ::SysAllocString( pwc);
        }
        else
        {
            Assert(pstr);
            *pOut = pstr->getBSTR();
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
DOMNode::get_nodeValue( 
    /* [out][retval] */ VARIANT __RPC_FAR *pVal)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_nodeValue()"));

    HRESULT hr = S_OK;

    ARGTEST( pVal);
    pVal->vt = VT_NULL;
    V_BSTR(pVal) = null;

    TRY
    {
        Node * pNode = getNodeData();
        String * pString = null;
        switch (pNode->getNodeType())
        {
        case Element::DOCUMENT:
        case Element::ELEMENT:
        case Element::DOCTYPE:
        case Element::ENTITY:
        case Element::NOTATION:
        case Element::TYPEDVALUE:
        case Element::DOCFRAG:
        case Element::ENTITYREF:
            pVal->vt = VT_NULL;
            break;

        case Element::XMLDECL:
        case Element::PI:
        case Element::ATTRIBUTE:
        case Element::COMMENT:
        case Element::PCDATA:
        case Element::CDATA:
            // getInnerText params must match Node::getNodeTypedValue()
            pString = pNode->getDOMNodeValue(); // ALWAYS PRESERVE NOW !!!
            break;

        default:
            break;
        }

        if ( pString)
        {
            pVal->vt = VT_BSTR;
            V_BSTR(pVal) = pString->getBSTR();
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
    return hr;
}


const static long s_lAllowPutNodeValue = (1 << (int)Node::ATTRIBUTE) | 
                                   (1 << (int)Node::PCDATA) | (1 << (int)Node::CDATA) | 
                                   (1 << (int)Node::PI) | (1 << (int)Node::COMMENT);

HRESULT STDMETHODCALLTYPE 
DOMNode::put_nodeValue( 
    /* [in] */ VARIANT vVal)
{
    STACK_ENTRY;
    OMWRITELOCK(this);

    TraceTag((tagDOMOM, "DOMNode::set_nodeValue()"));

    HRESULT hr;
    VARIANT vBstrVal; vBstrVal.vt = VT_NULL;
    hr = VariantChangeTypeEx(&vBstrVal, &vVal, GetThreadLocale(), NULL, VT_BSTR);
    if (hr)
        goto Cleanup;

    TRY
    {
        Node * pNode = getNodeData();
        if (s_lAllowPutNodeValue & (1 << (int)pNode->getNodeType())) 
        {
            pNode->checkReadOnly();

            BSTR bstr = V_BSTR(&vBstrVal);
            getNodeData()->setInnerText(bstr, _tcslen(bstr));
        }
        else
        {
            Exception::throwE(E_FAIL, 
                              XMLOM_INVALIDTYPE, 
                              pNode->getNodeTypeAsString(), 
                              null);
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY


Cleanup:
    VariantClear(&vBstrVal);
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMNode::get_parentNode( 
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;
    TraceTag((tagDOMOM, "DOMNode::get_parentNode()"));

    HRESULT hr = S_OK;

    Node * pNode = getNodeData();
    DOMNode * pReturn = null;
    Element::NodeType eNodeType = pNode->getNodeType();

    ARGTEST( ppNode);

    if (eNodeType != Element::ATTRIBUTE && 
        eNodeType != Element::DOCUMENT && 
        eNodeType != Element::DOCFRAG)
    {
        OMREADLOCK(this);

        TRY
        {
            Node * pParent = pNode->_pParent;
            if (pParent)
                pReturn = pParent->getDOMNodeWrapper();
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY
    }

Cleanup:
    if (SUCCEEDED(hr))
    {
        if (!pReturn)
            hr = S_FALSE;
        *ppNode = (IXMLDOMNode *)pReturn;
    }
    return hr;
}


const static long s_lAllowAttributes = (1 << (int)Element::ELEMENT)| (1 << (int)Element::XMLDECL) 
                                       | (1 << (int)Element::DOCTYPE) | (1 << (int)Element::ENTITY)
                                       | (1 << (int)Element::NOTATION);

HRESULT STDMETHODCALLTYPE 
DOMNode::get_specified( 
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pbool)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_specified()"));

    HRESULT hr;

    ARGTEST( pbool);

    TRY
    {
        Node * pNode = getNodeData();
        if (pNode->getNodeType() != Element::ATTRIBUTE || pNode->isSpecified())
        {
            *pbool = VARIANT_TRUE;
        }
        else
        {
            *pbool = VARIANT_FALSE;
        }
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
DOMNode::get_firstChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppFirstChild)
{
    STACK_ENTRY;

    TraceTag((tagDOMOM, "DOMNode::get_firstChild()"));
    HRESULT hr = S_OK;
    Node * pNode = getNodeData();

    ARGTEST(ppFirstChild);

    {
        OMREADLOCK(this);

        TRY
        {
            void * pv;
            Node * pChildNode = pNode->getNodeFirstChild(&pv);

            if (pChildNode)
            {
                *ppFirstChild = pChildNode->getDOMNodeWrapper();
            }
            else
            {
                *ppFirstChild = null;
                hr = S_FALSE;
            }
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY
    }
    
Cleanup:
    return hr;
}
 
   
HRESULT STDMETHODCALLTYPE 
DOMNode::get_lastChild( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppLastChild)
{
    STACK_ENTRY;
    TraceTag((tagDOMOM, "DOMNode::get_lastChild()"));
    HRESULT hr = S_OK;
    Node * pNode = getNodeData();

    ARGTEST(ppLastChild);

    {
        OMREADLOCK(this);

        TRY
        {
            Node * pChildNode = pNode->getNodeLastChild();

            if (pChildNode)
            {
                *ppLastChild = pChildNode->getDOMNodeWrapper();
            }
            else
            {
                *ppLastChild = null;
                hr = S_FALSE;
            }
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY
    }
    
Cleanup:
    return hr;
}
 
   
HRESULT STDMETHODCALLTYPE 
DOMNode::get_previousSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppPrevSib)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_previousSibling()"));

    HRESULT hr = S_OK;

    ARGTEST(ppPrevSib);

    TRY
    {
        Node * pNode = getNodeData();
        Node * pPrevSib;
        if (pNode->getNodeType() != Element::ATTRIBUTE &&
            (pPrevSib = pNode->getPrevSibling()) != null)
        {
            *ppPrevSib = (IXMLDOMNode *)pPrevSib->getDOMNodeWrapper();
        }
        else
        {
            *ppPrevSib = null;
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
DOMNode::get_nextSibling( 
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNextSib)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_nextSibling()"));

    HRESULT hr = S_OK;

    ARGTEST(ppNextSib);

    TRY
    {
        Node * pNode = getNodeData();
        Node * pSib;
        if (pNode->getNodeType() != Element::ATTRIBUTE &&
            (pSib = pNode->getNextSibling()) != null)
        {
            *ppNextSib = (IXMLDOMNode *)pSib->getDOMNodeWrapper();
        }
        else
        {
            *ppNextSib = null;
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
DOMNode::hasChildNodes( 
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBool)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::hasChildNodes()"));

    HRESULT hr = S_OK;
    Node * pNode = getNodeData();

    ARGTEST(pBool);

    TRY
    {
        if (! pNode->isEmpty())
        {
            *pBool = VARIANT_TRUE;
        }
        else
        {
            *pBool = VARIANT_FALSE;
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
DOMNode::insertBefore(
    /* [in] */ IXMLDOMNode __RPC_FAR *pNewChild,
    /* [in] */ VARIANT refChild,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
{
    STACK_ENTRY;
    TraceTag((tagDOMOM, "DOMNode::insertBefore()"));
    HRESULT hr;
    OMWRITELOCK(this);
    OMWriteLock * pOldDocLock = null;
    Node * pNode = getNodeData();

    ARGTEST( pNewChild);
    if (ppNewChild)
        *ppNewChild = null;

    TRY
    {
        Node * pRefNode = Node::Variant2Node(&refChild);
        Node * pChildNode;

        if (S_OK != pNewChild->QueryInterface(Node::s_IID, (void **)&pChildNode))
        {
            Exception::throwE((HRESULT)E_INVALIDARG);
        }

        Node * pOldParent = pChildNode->getNodeParent();
        if (pOldParent && pOldParent->getDocument() != pNode->getDocument())
        {
            pOldDocLock = new OMWriteLock(_EnsureTls.getTlsData(), pOldParent);
            if (!pOldDocLock->Locked())
            {
                hr = E_FAIL;
                _dispatchImpl::setErrorInfo(XMLOM_READONLY);
                goto Cleanup;
            }
        }

        pNode->insertNode(pChildNode, pRefNode);

        if ( ppNewChild)
        {
            *ppNewChild = pNewChild;
            pNewChild->AddRef();
        }
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    if (pOldDocLock)
        delete pOldDocLock;
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNode::appendChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild)
{
    VARIANT varNULL;
    varNULL.vt = VT_NULL;
    return insertBefore( newChild, varNULL, ppNewChild);
}



HRESULT STDMETHODCALLTYPE 
DOMNode::removeChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *pChild,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;
    OMWRITELOCK(this);

    TraceTag((tagDOMOM, "DOMNode::removeChild()"));

    HRESULT hr = S_OK;
    ARGTEST( pChild);

    TRY
    {
        Node * pChildNode;
        Node * pNode = getNodeData();

        if (S_OK != pChild->QueryInterface(Node::s_IID, (void **)&pChildNode))
        {
            Exception::throwE((HRESULT)E_INVALIDARG);
        }

        if (pChildNode->getNodeType() == Element::ATTRIBUTE)
            Exception::throwE(E_FAIL, 
                              XMLOM_INVALIDTYPE,
                              pChildNode->getNodeTypeAsString(),
                              null);

        pNode->removeNode(pChildNode, false);

        if ( ppNode)
        {
            *ppNode = pChild;
            pChild->AddRef();
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
DOMNode::replaceChild( 
    /* [in] */ IXMLDOMNode __RPC_FAR *pNew,
    /* [in] */ IXMLDOMNode __RPC_FAR *pOld,
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;
    OMWRITELOCK(this);
    OMWriteLock * pOldDocLock = null;

    TraceTag((tagDOMOM, "DOMNode::replaceNode()"));

    HRESULT hr = S_OK;
    Node * pNode = getNodeData();
    ARGTEST( pNew);
    ARGTEST( pOld);

    TRY
    {
        Node * pOldNode;
        Node * pNewNode;

        if (S_OK != pNew->QueryInterface(Node::s_IID, (void **)&pNewNode))
        {
            Exception::throwE((HRESULT)E_INVALIDARG);
        }
        if (S_OK != pOld->QueryInterface(Node::s_IID, (void **)&pOldNode))
        {
            Exception::throwE((HRESULT)E_INVALIDARG);
        }

        Node * pOldParent = pNewNode->getNodeParent();
        if (pOldParent && pOldParent->getDocument() != pNode->getDocument())
        {
            pOldDocLock = new OMWriteLock(_EnsureTls.getTlsData(), pOldParent);
            if (!pOldDocLock->Locked())
            {
                hr = E_FAIL;
                _dispatchImpl::setErrorInfo(XMLOM_READONLY);
                goto Cleanup;
            }
        }
        pNode->replaceNode( pNewNode, pOldNode);

        if (ppNode)
        {
            *ppNode = pOld;
            pOld->AddRef();
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    if (pOldDocLock)
        delete pOldDocLock;
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMNode::get_attributes( 
    /* [out][retval] */ IXMLDOMNamedNodeMap  __RPC_FAR *__RPC_FAR *ppNodeList)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_attributes()"));

    HRESULT hr = S_OK;

    ARGTEST( ppNodeList);

    TRY
    {
        Node *pNode = getNodeData();

        if (s_lAllowAttributes & (1 << (int)pNode->getNodeType())) 
        {
            *ppNodeList = new DOMNamedNodeMapList( pNode, Node::ATTRIBUTE);
        }
        else
        {
            *ppNodeList = null;
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
DOMNode::get_childNodes( 
    /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
{
    STACK_ENTRY;
    TraceTag((tagDOMOM, "DOMNode::get_childNodes()"));
    OMREADLOCK(this);

    HRESULT hr;
    Node * pNode = getNodeData();
    Element::NodeType eNodeType = pNode->getNodeType();
    Element::NodeType eFilter = Node::ANY;

    ARGTEST( ppNodeList);

    TRY
    {
        if (!pNode->isParent())
            pNode = null;
        else if (eNodeType == Element::DOCTYPE)
                 eFilter = Element::CDATA;

        *ppNodeList = new DOMChildList( pNode, eFilter);
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
DOMNode::cloneNode( 
    /* [in] */ VARIANT_BOOL deep,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::cloneNode()"));
    HRESULT hr = S_OK;

    if (!ppCloneRoot)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    {
        Node * pNode = getNodeData();
        Document * pDoc = pNode->getDocument();

        // clone allocated in same document's slot memory, 
        // but make sure we protect ourselves
        NodeManager * pNodeMgr = pDoc->getAltNodeMgr();

#ifdef RENTAL_MODEL
        if (MultiThread==model())
#endif
            pNodeMgr->Lock();

        TRY
        {
            Node * pClone = pNode->clone( deep == VARIANT_TRUE, false, pDoc, pDoc->getNodeMgr());    
            *ppCloneRoot = pClone->getDOMNodeWrapper();
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY

#ifdef RENTAL_MODEL
        if (MultiThread==model())
#endif
            pNodeMgr->Unlock();
    }

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMNode::get_ownerDocument( 
    IXMLDOMDocument __RPC_FAR *__RPC_FAR * ppDOMDocument)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_ownerDocument()"));

    HRESULT hr;

    ARGTEST( ppDOMDocument);

    TRY
    {
        Node * pNode = getNodeData();
        if (pNode->getNodeType() == Element::DOCUMENT)
        {
            hr = S_OK;
            *ppDOMDocument = null;
        }
        else
        {
            hr = pNode->getDocument()->QueryInterface(IID_IXMLDOMDocument, (void**)ppDOMDocument);
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
DOMNode::get_definition( 
    /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_definition()"));

    HRESULT hr = S_OK;

    ARGTEST( ppNode);

    TRY
    {
        Node * pNode = getNodeData();
        Node * pDefNode = pNode->getDefinition();
        if (pDefNode != null)
        {
            *ppNode = (IXMLDOMNode *)pDefNode->getDOMNodeWrapper();
        }
        else
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
DOMNode::get_text( 
    /* [out][retval] */ BSTR __RPC_FAR *pstr)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_innerText()"));

    HRESULT hr = S_OK;

    ARGTEST( pstr);

    TRY
    {
        Node * pNode = getNodeData();
        String * pString = pNode->getInnerText();
        if ( pString)
        {
            *pstr = pString->getBSTR();
        }
        else
        {
            *pstr = null;
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
DOMNode::put_text( 
    /* [in] */ BSTR text)
{
    STACK_ENTRY;
    OMWRITELOCK(this);

    TraceTag((tagDOMOM, "DOMNode::put_innerText()"));

    HRESULT hr = S_OK;

    TRY
    {
        Node * pNode = getNodeData();
        switch(pNode->getNodeType())
        {
        case Element::ELEMENT:
        case Element::ATTRIBUTE:
        case Element::PI:
        case Element::COMMENT:
        case Element::PCDATA:
        case Element::CDATA:
        case Element::ENTITY:
        case Element::ENTITYREF:
            {
                pNode->checkReadOnly();
                getNodeData()->setInnerText(text, _tcslen(text));
            }
            break;

        case Element::DOCUMENT:
        case Element::DOCFRAG:
        case Element::DOCTYPE:
        default:
            Exception::throwE(E_FAIL, XMLOM_INVALIDTYPE, pNode->getNodeTypeAsString(), null);
            break;

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
DOMNode::get_namespaceURI(
    /* [out][retval] */ BSTR __RPC_FAR *pURI)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_namespaceURI()"));

    HRESULT hr = S_OK;
    ARGTEST(pURI);

    TRY
    {
        Node * pNode = getNodeData();
        NameDef * pNameDef = pNode->getNameDef();
        Atom * pURN = pNameDef ? pNameDef->getSrcURN() : null;
        if ( pURN)
        {
            *pURI = pURN->toString()->getBSTR();
        }
        else
        {
            *pURI = null;
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
DOMNode::get_prefix( 
    /* [out][retval] */ BSTR __RPC_FAR *pPrefix)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_prefix()"));

    HRESULT hr = S_OK;
    ARGTEST(pPrefix);

    TRY
    {
        Node * pNode = getNodeData();
        NameDef * pNameDef = pNode->getNameDef();
        Atom * pAtomPrefix = pNameDef ? pNameDef->getPrefix() : null;
        if ( pAtomPrefix)
        {
            *pPrefix = (pAtomPrefix->toString())->getBSTR();
        }
        else
        {
            *pPrefix = null;
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
DOMNode::get_baseName( 
    /* [out][retval] */ BSTR __RPC_FAR *pBaseName)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_baseName()"));

    HRESULT hr = S_OK;
    ARGTEST(pBaseName);

    TRY
    {
        Node * pNode = getNodeData();
        Name * pName = pNode->getName();
        if ( pName)
        {
            *pBaseName = pName->getName()->toString()->getBSTR();
        }
        else
        {
            *pBaseName = null;
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
DOMNode::get_nodeTypedValue( 
    /* [out][retval] */ VARIANT __RPC_FAR *TypedValue)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_nodeTypedValue()"));

    HRESULT hr = S_OK;

    ARGTEST(TypedValue);
    TypedValue->vt = VT_NULL;

    TRY
    {
        Node * pNode = getNodeData();

        if (pNode->isTyped())
        {
            pNode->testDataType();
            pNode->getNodeTypedValue(TypedValue);
        }
        else
        {
            if (pNode->getType() == Element::ELEMENT ||
                pNode->getType() == Element::ATTRIBUTE)
            {
                BSTR bstr = NULL;
                hr = get_text(&bstr);
                if (!hr)
                {
                    TypedValue->vt = VT_BSTR;
                    V_BSTR(TypedValue) = bstr;
                }
            }
            else
                hr = get_nodeValue(TypedValue);
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
        goto Cleanup;
    }
    ENDTRY


Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNode::put_nodeTypedValue( 
    /* [in] */ VARIANT TypedValue)
{
    STACK_ENTRY;

    TraceTag((tagDOMOM, "DOMNode::put_nodeTypedValue()"));

    HRESULT hr = S_OK;

    TRY
    {
        Node * pNode = getNodeData();
        if (pNode->isTyped())
        {
            OMWRITELOCK(this);
            pNode->checkReadOnly();
            pNode->setNodeTypedValue(&TypedValue);
        }
        else
        {
            hr = put_nodeValue(TypedValue);
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
        goto Cleanup;
    }
    ENDTRY


Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNode::get_dataType( 
    /* [out][retval] */ VARIANT __RPC_FAR *pVar)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_dataType()"));

    HRESULT hr = S_OK;
    ARGTEST(pVar);

    VariantInit( pVar);
    TRY
    {
        String * pS = getNodeData()->getDataTypeString();
        if (pS)
        {
            pVar->vt = VT_BSTR;
            V_BSTR(pVar) = pS->getBSTR();
        }
        else
        {
            pVar->vt = VT_NULL;
            V_BSTR(pVar) = null;
            hr = S_FALSE;
            goto Cleanup;
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
        goto Cleanup;
    }
    ENDTRY


Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNode::put_dataType( 
    /* [in] */ BSTR p)
{
    STACK_ENTRY;
    OMWRITELOCK(this);

    TraceTag((tagDOMOM, "DOMNode::put_dataType()"));

    HRESULT hr = S_OK;
    ARGTEST(p);

    TRY
    {
        String * pS = String::newString(p);
        Node * pNode = getNodeData();
        pNode->checkReadOnly();
        if (pNode->getNodeType() != Element::ELEMENT &&
            pNode->getNodeType() != Element::ATTRIBUTE)
            Exception::throwE(E_FAIL, XMLOM_INVALIDTYPE, pNode->getNodeTypeAsString(), null);
        pNode->setDataTypeString(pS);
    }
    CATCH
    {
        hr = ERESULTINFO;
        goto Cleanup;
    }
    ENDTRY


Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNode::transformNode(
        /* [in] */ IXMLDOMNode __RPC_FAR *pStyleSheet,
        /* [out][retval] */ BSTR __RPC_FAR *pbstr)
{
    STACK_ENTRY;
    OMREADLOCK(this);
    HRESULT hr;
    IStream * pstm = null;
    Document *pDoc = getDocument();

    TraceTag((tagDOMOM, "DOMNode::transformNode()"));

    ARGTEST(pStyleSheet);
    ARGTEST(pbstr);

    TRY
    {
        StringBuffer * strbuf = StringBuffer::newStringBuffer();
        StringStream * strstm = StringStream::newStringStream(strbuf);

        strstm->getIStream(&pstm);

        ::TransformNode(pStyleSheet, (IXMLDOMNode *)this, pstm);

        *pbstr = strbuf->toString()->getBSTR();

        hr = S_OK;
    }
    CATCH
    {
        *pbstr = null;
        hr = ERESULTINFO;
    }
    ENDTRY;

    if (pstm)
        pstm->Release();

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMNode::selectNodes( 
    /* [in] */ BSTR bstrXQL,
    /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppResult)
{
    STACK_ENTRY;
    OMREADLOCK(this);
    HRESULT hr;
    XQLNodeList* list = null;


    ARGTEST(bstrXQL);
    ARGTEST(ppResult);

    TraceTag((tagDOMOM, "DOMNode::selectNodes()"));

    TRY
    {
        list = new XQLNodeList();
        list->setQuery(String::newString(bstrXQL), ::GetElement((IXMLDOMNode *)this));
        *ppResult = list;
        hr = S_OK;
    }
    CATCH
    {
        delete list;
        hr = ERESULTINFO;
    }
    ENDTRY;

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNode::get_xml(
    /* [out][retval] */ BSTR __RPC_FAR *pbstrXml)
{
    STACK_ENTRY;
    OMREADLOCK(this);
    HRESULT hr = S_OK;

    ARGTEST(pbstrXml);

    TraceTag((tagDOMOM, "DOMNode::get_xml()"));

    TRY
    {
        Node * pNode = getNodeData();
        String * pString = pNode->getXML();
        *pbstrXml = pString->getBSTR();
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
DOMNode::get_parsed( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR * isParsed)
{
    STACK_ENTRY;
    OMREADLOCK(this);

    TraceTag((tagDOMOM, "DOMNode::get_parsed()"));

    HRESULT hr = S_OK;

    ARGTEST(isParsed);

    TRY
    {
        if (getNodeData()->isFinished())
            *isParsed = VARIANT_TRUE;
        else
            *isParsed = VARIANT_FALSE;
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
DOMNode::selectSingleNode( 
        /* [in] */ BSTR bstrQueryString,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR * ppResultNode)
{
    STACK_ENTRY;
    OMREADLOCK(this);
    HRESULT hr;

    ARGTEST(bstrQueryString);
    ARGTEST(ppResultNode);
    *ppResultNode = NULL;

    TraceTag((tagDOMOM, "DOMNode::selectSingleNode()"));

    TRY
    {
        Node* pNode = getNodeData()->selectSingleNode(String::newString(bstrQueryString));
        if (pNode)
        {
            *ppResultNode = pNode->getDOMNodeWrapper();
            hr = S_OK;
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
    ENDTRY;

Cleanup:
    return hr;
}

static 
const TCHAR * TypeString[] =
{
    _T("_unknownType"),    
    _T("element"),
    _T("attribute"),    
    _T("text"),    
    _T("cdatasection"),    
    _T("entityreference"),    
    _T("entity"),    
    _T("processinginstruction"),    
    _T("comment"),    
    _T("document"),    
    _T("documenttype"),    
    _T("documentfragment"),    
    _T("notation")                    
};

HRESULT STDMETHODCALLTYPE 
DOMNode::get_nodeTypeString( 
        /* [out][retval] */ BSTR __RPC_FAR * pbstrNodeType)
{
    STACK_ENTRY;
    TraceTag((tagDOMOM, "DOMNode::get_nodeTypeString()"));
    OMREADLOCK(this);

    HRESULT hr;
    ARGTEST( pbstrNodeType);

    TRY
    {
        int type = _NodeType2DOMNodeType( getNodeData()->getNodeType());
        *pbstrNodeType = ::SysAllocString(TypeString[type]);
        hr = S_OK;
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
DOMNode::getElementsByTagName( 
    /* [in] */ BSTR tagname,
    /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList)
{
    STACK_ENTRY;
    TraceTag((tagDOMOM, "DOMNode::getElementsByTagName()"));

    OMREADLOCK(this);
    HRESULT hr;
    bstr b;

    //
    // We do not need a lock here because DOMNode::selectNodes will 
    //

    ARGTEST(tagname);
    ARGTEST(ppNodeList);

    TRY
    {
        // BUGBUG - Perf - This could be done much more effeciently by directly 
        // creating a TreeQuery object.

        IXMLDOMNodeList * pXMLDOMNodeList = NULL;
        String * pStr = String::add(String::newString(_T(".//")), 
                                    String::newString(tagname), null);
        b = pStr;
        hr = selectNodes(b, &pXMLDOMNodeList);
        *ppNodeList = pXMLDOMNodeList;
    }
    CATCH
    {
        *ppNodeList = NULL;
        hr = ERESULTINFO;
    }
    ENDTRY;

Cleanup:
    return hr;
}


// Transform the node using the specified stylesheet.  The form of output is determined by 
// the contents of the VARIANT.  It has to contain either a VT_DISPATCH or a VT_UNKNOWN.  We
// first QI to see if the object is a Document.  If so, we construct a simple stream object, 
// tell the document to load from that stream, and then do the transform to that stream.  If 
// the object is not a Document, we QI for IStream.  If that succeeds, we pass the stream along, 
// and the transform is done into that stream

HRESULT STDMETHODCALLTYPE 
DOMNode::transformNodeToObject( 
    /* [in] */ IXMLDOMNode __RPC_FAR *pXDNStylesheet,
    /* [in] */ VARIANT varOutputObject)
{
    STACK_ENTRY;
    OMREADLOCK(this);
    HRESULT hr;
    Document *pDoc, *pDocDest = NULL;
    IStream *pstm = NULL;
    IResponse* pResponse = NULL;
    IHTMLObjectElement * pobj = null;
    IDispatch * pdisp = null;

    // This IUnknown pointer should not be Released - it holds the same value as 
    // the pointer in the VARIANT, either IDispatch or IUnknown, and is therefore
    // released when the VARIANT is cleaned up
    IUnknown *punk;


    TraceTag((tagDOMOM, "DOMNode::transformNodeToObject()"));

    ARGTEST(pXDNStylesheet);

    //
    // We must be passed either an IDispatch or an IUnknown in the VARIANT
    //

    punk = Variant::getUnknown(&varOutputObject);
    if (NULL == punk)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Now we try to determine what we have been given
    // 

    if (NULL != punk)
    {
        // 
        // First we try to get a Document
        //

        hr = punk->QueryInterface(IID_IHTMLObjectElement, (void **) &pobj);
        if (SUCCEEDED(hr) && pobj)
        {
            hr = pobj->get_object(&pdisp);
            if (!SUCCEEDED(hr) || !pdisp)
            {
                goto Cleanup;
            }
            punk = pdisp;
        }

        if (SUCCEEDED(punk->QueryInterface(IID_Document, (LPVOID *)&pDocDest)))
        {
            pstm = (IStream *) new_ne DocStream(pDocDest);
            if (!pstm)
            {
                // 
                // Couldn't allocate memory for the stream class
                // 
                
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            } 
        }
        else if (SUCCEEDED(punk->QueryInterface(IID_IStream, (LPVOID *)&pstm)))
        {
            // great, then we'll just write straight to the stream !!;
        }
        else if (SUCCEEDED(punk->QueryInterface(IID_IResponse, (void**)&pResponse)))
        {
            // Create an IStream wrapper for the pResponse object so that
            // every buffer full of XML that we generate we send directly
            // down the wire !!
            pstm = (IStream *) new_ne DocStream(pResponse);
            pResponse->Release();
            if (!pstm)
            {
                // 
                // Couldn't allocate memory for the stream class
                //   
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            } 
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else
    {
        //
        // NULL interface pointer is not valid
        //

        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert (pstm && "At this point, we should definitely have a valid IStream interface pointer");

    pDoc = getDocument();
    TRY
    {
        // if the destination is a document check the models
        // the XSL and XML document model check happens in ::TransformNode
        if (pDocDest && pDoc->model() != pDocDest->model())
            Exception::throwE(E_FAIL, XMLOM_INVALID_MODEL, null);

        // Stream has to start with unicode byte order mark because
        // that is the only encoding TransformNode generates at this point.
        ULONG ulWritten; // IIS5 crashes if we pass NULL for this argument.
        hr = pstm->Write(s_ByteOrderMark, sizeof(WCHAR), &ulWritten);
        if (hr)
            goto Cleanup;

        ::TransformNode(pXDNStylesheet, (IXMLDOMNode *)this, pstm );

        hr = S_OK;
    }
    CATCH
    {
        // BUGBUG: We'll need to do something more elaborate here, to flush the 
        // stream etc

        hr = ERESULTINFO;
    }
    ENDTRY;
    
Cleanup:

    if (pstm)
        pstm->Release();
    if (pdisp)
        pdisp->Release();
    if (pobj)
        pobj->Release();

    return hr;
}


/***********************************************************/
/*********************** DOMNodeList ***********************/

DOMNodeList::DOMNodeList(Node* node)
{
    _pParent = node;
    _State.init();
}


DOMNodeList::~DOMNodeList()
{
}


#ifdef UNIX
// This is a Apogee (HP and Solaris Unix) compiler bug workaround!
// If you change it you may totally break XLM under Unix
_DOMList::_DOMList()
{
}

HRESULT _DOMList::GetDispID( 
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */ DISPID __RPC_FAR *pid)
{
        TraceTag((tagDispatch, "GetDispID"));
        return _dispatchImpl::GetDispID(&_dispatch<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList>::s_dispatchinfo, true, bstrName, grfdex, pid);
}
#endif // UNIX


/*  
*** NOTE: This constructor has been moved here from
**  xqlnodelist.cxx, because to not do so breaks all of XQL under Unix,
**  due to a unix compiler bug which has to do with statics and templates
**
**/
XQLNodeList::XQLNodeList()
{
    _pResults = new Vector(5);
    _lIndex = -1;
    _fCacheComplete = false;
}

DOMChildList::DOMChildList(Node* node, Element::NodeType eFilterType)
: DOMNodeList( node)
{
    if (eFilterType != Node::ANY)
        _fFilter = true;
    else
        _fFilter = false;
    _eNodeTypeFilter = eFilterType;
}


DOMNamedNodeMapList::DOMNamedNodeMapList(Node* node, Element::NodeType eType)
: DOMNodeList( node), _eNodeType(eType)
{ 
}



HRESULT STDMETHODCALLTYPE 
DOMChildList::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;

    TRY
    {
        if (iid == IID_IXMLDOMNodeList)
        {
            AddRef();
            *ppv = (IXMLDOMNodeList *)this;
            hr = S_OK;
        }
        else if (iid == IID_IEnumVARIANT)
        {
            hr = get__newEnum((IUnknown**)ppv);
        }
        else
        {
            return _dispatchEx<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList, true>::QueryInterface(iid, ppv);
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Customized IDispatch implementation for DOMChildList
//

static VARTYPE s_argTypes_DOMNodeList_item[]        = {VT_I4};

static INVOKE_METHOD 
s_rgDOMNodeListMethods[] =
{
    // name         dispid                 argument number  argument table   argument guid id   return type    invoke kind
    { L"item",      DISPID_VALUE,                      1,   s_argTypes_DOMNodeList_item,       NULL,           VT_DISPATCH,    DISPATCH_PROPERTYGET | DISPATCH_METHOD},
    { L"length",    DISPID_DOM_NODELIST_LENGTH,        0,   NULL,              NULL,           VT_I4,          DISPATCH_PROPERTYGET},
    { L"nextNode",  DISPID_XMLDOM_NODELIST_NEXTNODE,   0,   NULL,              NULL,           VT_DISPATCH,    DISPATCH_PROPERTYGET | DISPATCH_METHOD},
    { L"reset",     DISPID_XMLDOM_NODELIST_RESET,      0,   NULL,              NULL,           VT_ERROR,       DISPATCH_PROPERTYGET | DISPATCH_METHOD },

    // BUGBUG: a hack to deal with hidden property/method
    { L"_newEnum",  DISPID_NEWENUM, 0, NULL, NULL, VT_UNKNOWN, DISPATCH_PROPERTYGET | DISPATCH_METHOD}
};


static DISPIDTOINDEX 
s_DOMNodeList_DispIdMap[] =
{
  { DISPID_NEWENUM,                    4}, // IXMLDOMNodeList::_newEnum
  { DISPID_VALUE,                      0}, // IXMLDOMNodeList::item
  { DISPID_DOM_NODELIST_LENGTH,        1}, // IXMLDOMNodeList::length
  { DISPID_XMLDOM_NODELIST_NEXTNODE,   2}, // IXMLDOMNodeList::nextNode
  { DISPID_XMLDOM_NODELIST_RESET,      3}, // IXMLDOMNodeList::reset
};



DISPATCHINFO _dispatch<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDOMNodeList, &LIBID_MSXML, ORD_MSXML, s_rgDOMNodeListMethods, NUMELEM(s_rgDOMNodeListMethods), s_DOMNodeList_DispIdMap, NUMELEM(s_DOMNodeList_DispIdMap), DOMChildList::_invoke
};


HRESULT
DOMChildList::_invoke(
    void * pTarget,
    DISPID dispIdMember,
    INVOKE_ARG *rgArgs, 
    WORD wInvokeType,
    VARIANT *pVarResult,
    UINT cArgs)
{
    HRESULT hr;
    IXMLDOMNodeList * pObj = (IXMLDOMNodeList*)pTarget;

    TEST_METHOD_TABLE(s_rgDOMNodeListMethods, NUMELEM(s_rgDOMNodeListMethods) - 1, s_DOMNodeList_DispIdMap, NUMELEM(s_DOMNodeList_DispIdMap));

    switch( dispIdMember )
    {
        case DISPID_NEWENUM:
            hr = pObj->get__newEnum((IUnknown **) &pVarResult->pdispVal);
            break;

        case DISPID_VALUE:
            if (cArgs != 1)
                hr = DISP_E_BADPARAMCOUNT;
            else
                hr = pObj->get_item((long) VARMEMBER( &rgArgs[0].vArg, lVal),
                          (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_NODELIST_LENGTH:
            hr = pObj->get_length((long*) &pVarResult->lVal);
            break;

        case DISPID_XMLDOM_NODELIST_NEXTNODE:
            hr = pObj->nextNode((IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_NODELIST_RESET:
            hr = pObj->reset();
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Customized IDispatch implementation for DOMNamedNodeMapList
//

static VARTYPE s_argTypes_DOMNamedNodeMap_getNamedItem[]        = {VT_BSTR};
static VARTYPE s_argTypes_DOMNamedNodeMap_setNamedItem[]        = {VT_DISPATCH};
static VARTYPE s_argTypes_DOMNamedNodeMap_removeNamedItem[]     = {VT_BSTR};
static VARTYPE s_argTypes_DOMNamedNodeMap_getQualifiedItem[]    = {VT_BSTR, VT_BSTR};
static VARTYPE s_argTypes_DOMNamedNodeMap_removeQualifiedItem[] = {VT_BSTR, VT_BSTR};
static VARTYPE s_argTypes_DOMNamedNodeMap_item[]                = {VT_I4};

static const IID * s_argTypesIIDs_DOMNamedNodeMap_setNamedItem[] = {&IID_IXMLDOMNode};


static INVOKE_METHOD 
s_rgDOMNamedNodeMapMethods[] =
{
    { L"getNamedItem",     83, 1, s_argTypes_DOMNamedNodeMap_getNamedItem,      NULL, VT_DISPATCH, DISPATCH_METHOD},
    { L"getQualifiedItem", 87, 2, s_argTypes_DOMNamedNodeMap_getQualifiedItem,  NULL, VT_DISPATCH, DISPATCH_METHOD},
    { L"item",              0, 1, s_argTypes_DOMNamedNodeMap_item,              NULL, VT_DISPATCH, DISPATCH_PROPERTYGET | DISPATCH_METHOD},
    { L"length",           74, 0, NULL,                                         NULL, VT_I4,       DISPATCH_PROPERTYGET},
    { L"nextNode",         89, 0, NULL,                                         NULL, VT_DISPATCH, DISPATCH_PROPERTYGET | DISPATCH_METHOD},
    { L"removeNamedItem",  85, 1, s_argTypes_DOMNamedNodeMap_removeNamedItem,   NULL, VT_DISPATCH, DISPATCH_METHOD},
    { L"removeQualifiedItem", 88, 2, s_argTypes_DOMNamedNodeMap_removeQualifiedItem, NULL, VT_DISPATCH, DISPATCH_METHOD},
    { L"reset",            90, 0, NULL,                                         NULL, VT_ERROR, DISPATCH_PROPERTYGET | DISPATCH_METHOD},
    { L"setNamedItem",     84, 1, s_argTypes_DOMNamedNodeMap_setNamedItem, s_argTypesIIDs_DOMNamedNodeMap_setNamedItem, VT_DISPATCH, DISPATCH_METHOD},

    // BUGBUG: a hack to deal with hidden property/method
    { L"_newEnum",         -4, 0, NULL, NULL, VT_UNKNOWN, DISPATCH_PROPERTYGET | DISPATCH_METHOD}
};


static DISPIDTOINDEX 
s_DOMNamedNodeMap_DispIdMap[] =
{
  { DISPID_NEWENUM,                              9}, // IXMLDOMNamedNodeMap::_newEnum
  { DISPID_VALUE,                                2}, // IXMLDOMNamedNodeMap::item
  { DISPID_DOM_NODELIST_LENGTH,                  3}, // IXMLDOMNamedNodeMap::length
  { DISPID_DOM_NAMEDNODEMAP_GETNAMEDITEM,        0}, // IXMLDOMNamedNodeMap::getNamedItem
  { DISPID_DOM_NAMEDNODEMAP_SETNAMEDITEM,        8}, // IXMLDOMNamedNodeMap::setNamedItem
  { DISPID_DOM_NAMEDNODEMAP_REMOVENAMEDITEM,     5}, // IXMLDOMNamedNodeMap::removeNamedItem
  { DISPID_XMLDOM_NAMEDNODEMAP_GETQUALIFIEDITEM,    1}, // IXMLDOMNamedNodeMap::getQualifiedItem
  { DISPID_XMLDOM_NAMEDNODEMAP_REMOVEQUALIFIEDITEM, 6}, // IXMLDOMNamedNodeMap::removeQualifiedItem
  { DISPID_XMLDOM_NAMEDNODEMAP_NEXTNODE,            4}, // IXMLDOMNamedNodeMap::nextNode
  { DISPID_XMLDOM_NAMEDNODEMAP_RESET,               7}  // IXMLDOMNamedNodeMap::reset
};


DISPATCHINFO _dispatch<IXMLDOMNamedNodeMap, &LIBID_MSXML, &IID_IXMLDOMNamedNodeMap>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDOMNamedNodeMap, &LIBID_MSXML, ORD_MSXML, s_rgDOMNamedNodeMapMethods, NUMELEM(s_rgDOMNamedNodeMapMethods), s_DOMNamedNodeMap_DispIdMap, NUMELEM(s_DOMNamedNodeMap_DispIdMap), DOMNamedNodeMapList::_invoke
};


HRESULT
DOMNamedNodeMapList::_invoke(
    void * pTarget,
    DISPID dispIdMember,
    INVOKE_ARG *rgArgs, 
    WORD wInvokeType,
    VARIANT *pVarResult,
    UINT cArgs)
{
    HRESULT hr;
    IXMLDOMNamedNodeMap * pObj = (IXMLDOMNamedNodeMap*)pTarget;

    TEST_METHOD_TABLE(s_rgDOMNamedNodeMapMethods, NUMELEM(s_rgDOMNamedNodeMapMethods) - 1, s_DOMNamedNodeMap_DispIdMap, NUMELEM(s_DOMNamedNodeMap_DispIdMap));

    switch( dispIdMember )
    {
        case DISPID_NEWENUM:
            hr = pObj->get__newEnum((IUnknown **) &pVarResult->pdispVal);
            break;

        case DISPID_VALUE:                                // IXMLDOMNamedNodeMap::item
            if (cArgs != 1)
                hr = DISP_E_BADPARAMCOUNT;
            else
                hr = pObj->get_item((long) VARMEMBER( &rgArgs[0].vArg, lVal),
                          (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_NODELIST_LENGTH:                  // IXMLDOMNamedNodeMap::length
            hr = pObj->get_length((long*) &pVarResult->lVal);
            break;

        case DISPID_DOM_NAMEDNODEMAP_GETNAMEDITEM:        // IXMLDOMNamedNodeMap::getNamedItem
            hr = pObj->getNamedItem( (BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                               (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_NAMEDNODEMAP_SETNAMEDITEM:        // IXMLDOMNamedNodeMap::setNamedItem
            hr = pObj->setNamedItem( (IXMLDOMNode*) V_DISPATCH(&rgArgs[0].vArg),
                               (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_DOM_NAMEDNODEMAP_REMOVENAMEDITEM:     // IXMLDOMNamedNodeMap::removeNamedItem
            hr = pObj->removeNamedItem( (BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                               (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_NAMEDNODEMAP_GETQUALIFIEDITEM:    // IXMLDOMNamedNodeMap::getQualifiedItem
            hr = pObj->getQualifiedItem( (BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                                   (BSTR) VARMEMBER( &rgArgs[1].vArg, bstrVal),
                                   (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_NAMEDNODEMAP_REMOVEQUALIFIEDITEM: // IXMLDOMNamedNodeMap::removeQualifiedItem
            hr = pObj->removeQualifiedItem( (BSTR) VARMEMBER( &rgArgs[0].vArg, bstrVal),
                                   (BSTR) VARMEMBER( &rgArgs[1].vArg, bstrVal),
                                   (IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_NAMEDNODEMAP_NEXTNODE:            // IXMLDOMNamedNodeMap::nextNode
            hr = pObj->nextNode((IXMLDOMNode**) &pVarResult->pdispVal);
            break;

        case DISPID_XMLDOM_NAMEDNODEMAP_RESET:               // IXMLDOMNamedNodeMap::reset
            hr = pObj->reset();
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNamedNodeMapList::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;

    TRY
    {
        if (iid == IID_IXMLDOMNamedNodeMap)
        {
            AddRef();
            *ppv =(IXMLDOMNamedNodeMap *)this;
            hr = S_OK;
        }
        else if (iid == IID_IEnumVARIANT)
        {
            hr = get__newEnum((IUnknown**)ppv);
        }
        else
        {
            return _dispatchEx<IXMLDOMNamedNodeMap, &LIBID_MSXML, &IID_IXMLDOMNamedNodeMap, true>::QueryInterface(iid, ppv);
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


Node *
DOMChildList::_next(Node * pParentNode, void ** ppv)
{
    Node * pNode;

    if ( *ppv == null)
        pNode = pParentNode->getNodeFirstChild( ppv);
    else
        pNode = pParentNode->getNodeNextChild( ppv);

    if ( _fFilter)
        while ( pNode && (pNode->getNodeType() == _eNodeTypeFilter))
            pNode = pParentNode->getNodeNextChild( ppv);

    return pNode;
}


Node *
DOMNamedNodeMapList::_next(Node * pParentNode, void ** ppv)
{
    Node * pNode;

    if (Element::ATTRIBUTE == _eNodeType)
    {
        if ( *ppv == null)
            pNode = pParentNode->getNodeFirstAttributeWithDefault( ppv);
        else
            pNode = pParentNode->getNodeNextAttributeWithDefault( ppv);
    }
    else
    {
        if ( *ppv == null)
            pNode = pParentNode->getNodeFirstChild( ppv);
        else
            pNode = pParentNode->getNodeNextChild( ppv);

        while (pNode && pNode->getNodeType() != _eNodeType)
            pNode = pParentNode->getNodeNextChild( ppv);
    }

    return pNode;
}


HRESULT STDMETHODCALLTYPE 
DOMNodeList::get_length( 
        /* [out][retval] */ long __RPC_FAR *pl)
{
    STACK_ENTRY;

    TraceTag((tagDOMOM, "DOMNodeList::get_length()"));

    HRESULT hr = S_OK;

    ARGTEST( pl);

    // watch for empty node list
    if ( _pParent)
    {
        OMREADLOCK(this->_pParent);

        void * pv = null;
        long lCount = 0;
        Node * pParentNode = _pParent->getNodeData();
        while (_next( pParentNode, &pv) != null)
            lCount++;

        *pl = lCount;
    }
    else
    {
        *pl = 0;
    }

Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeList::get_item( 
        /* [in][optional] */ long index,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNodeList::item()"));

    if ( index < 0)
    {
        *ppNode = null; // bug 33780.
        hr = S_FALSE;
        goto Cleanup;
    }

    ARGTEST( ppNode);
    *ppNode = null;

    // watch for empty node list
    if ( !_pParent)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        OMREADLOCK(this->_pParent);

        TRY
        {
            void * pv = null;
            long lCount = 0;
            Node * pNode;
            Node * pParentNode = _pParent->getNodeData();

            while ((pNode = _next( pParentNode, &pv)) != null &&
                   lCount < index)
                lCount++;

            if ( pNode)
            {
                *ppNode = (IXMLDOMNode *)pNode->getDOMNodeWrapper();
            }
            else
            {
                *ppNode = null;
                hr = S_FALSE;
            }
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY

    }

Cleanup:
    return hr;
}


                       
HRESULT STDMETHODCALLTYPE 
DOMNodeList::nextNode( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;

    TraceTag((tagDOMOM, "DOMNodeList::get_nextNode()"));

    HRESULT hr = S_OK;

    ARGTEST( ppNode);
    *ppNode = null;

    // watch for empty node list
    // watch for current position being moved from parent...
    if ( !_pParent )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        OMREADLOCK(this->_pParent);

        TRY
        {
            Node * pNode = _State.getNext((EnumVariant *)this);
            if (pNode)
            {
                *ppNode = (IXMLDOMNode *)pNode->getDOMNodeWrapper();
            }
            else
            {
                *ppNode = null;
                hr = S_FALSE;
            }
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY
    }

Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMNodeList::reset()
{
    STACK_ENTRY;

    TraceTag((tagDOMOM, "DOMNodeList::reset()"));

    HRESULT hr = S_OK;
    OMREADLOCK(this->_pParent);

    TRY
    {
        _State.init();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Cleanup:
    return hr;
}
    

HRESULT
DOMNodeList::get__newEnum(IUnknown *pUnk, IUnknown **ppUnk)
{
    STACK_ENTRY;
    TraceTag((tagDOMOM, "DOMNodeList::get__newEnum"));

    HRESULT hr = S_OK;

    ARGTEST( ppUnk);

    if ( _pParent)
    {
        OMREADLOCK(_pParent);

        TRY
        {
            // Create a new enumeration that enumerates through the same
            // set of children as this DOMNodeList.
            *ppUnk = IEnumVARIANTWrapper::newIEnumVARIANTWrapper(pUnk, (EnumVariant*)this, _pParent->getDocument()->getMutex());
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY
    }
    else
    {
        *ppUnk = null;
        hr = S_FALSE;
    }

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNamedNodeMapList::getNamedItem( 
        /* [in] */ BSTR pbstrName,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNodeList::getNamedItem()"));

    ARGTEST(pbstrName);
    ARGTEST(ppNode);
    *ppNode = null;

    // watch for empty node list
    if ( !_pParent)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        OMREADLOCK(this->_pParent);

        TRY
        {
            Node * pParentNode = _pParent->getNodeData();
            Node * pNode = pParentNode->findByNodeName(pbstrName, _eNodeType, pParentNode->getDocument());

            if ( pNode)
            {
                *ppNode = (IXMLDOMNode *)pNode->getDOMNodeWrapper();
            }
            else
            {
                *ppNode = null;
                hr = S_FALSE;
            }
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY

    }

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNamedNodeMapList::setNamedItem( 
    /* [in] */ IXMLDOMNode __RPC_FAR *arg,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNodeList::setNamedItem()"));
    Node * pNewNode = null;
    
    if (!arg ||
        S_OK != arg->QueryInterface(Node::s_IID, (void **)&pNewNode) ||
        pNewNode == null ||
        pNewNode->getNodeType() != _eNodeType ||
        _eNodeType != Element::ATTRIBUTE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // watch for empty node list
    if ( !_pParent)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        OMWRITELOCK(this->_pParent);

        TRY
        {
            Node * pNode;
            Node * pParentNode = _pParent->getNodeData();

            pNode = pParentNode->findByNameDef(pNewNode->getNameDef(), _eNodeType);

            if ( pNode)
                pParentNode->replaceNode(pNewNode, pNode, Element::ATTRIBUTE == _eNodeType);
            else
                pParentNode->insertNode(pNewNode, null, Element::ATTRIBUTE == _eNodeType);

            if (ppNode)
                *ppNode = pNewNode->getDOMNodeWrapper();
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY
    }

Cleanup:
    if (hr && ppNode)
        *ppNode = NULL;
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNamedNodeMapList::removeNamedItem( 
    /* [in] */ BSTR pbstrName,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNodeList::removeNamedItem()"));

    ARGTEST(pbstrName);
    if (ppNode)
        *ppNode = null;

    // watch for empty node list
    if ( !_pParent)
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    if (_eNodeType != Element::ATTRIBUTE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    {
        OMREADLOCK(this->_pParent);

        TRY
        {
            Node * pParentNode = _pParent->getNodeData();
            Node * pNode = pParentNode->findByNodeName(pbstrName, _eNodeType, pParentNode->getDocument());

            if ( pNode)
            {
                if (ppNode)
                    *ppNode = (IXMLDOMNode *)pNode->getDOMNodeWrapper();
                pParentNode->removeNode( pNode, true);
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

    }

Cleanup:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// IXMLDOMNamedNodeMap Interface

static
Node *
FindQualifiedNode(BSTR bstrName, BSTR bstrNameSpaceURN, Node * pNode, Element::NodeType eNodeType)
{
    Node * pFoundNode;
    Document * pDoc = pNode->getDocument();
    Atom * pURN = (bstrNameSpaceURN && *bstrNameSpaceURN) ? Atom::create(bstrNameSpaceURN) : null;
    Name * pName = Name::create(bstrName, lstrlenW(bstrName), pURN);
    return pNode->find(pName, eNodeType, pDoc);
}

HRESULT STDMETHODCALLTYPE 
DOMNamedNodeMapList::getQualifiedItem(
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrNameSpaceURN,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNodeList::getQualifiedItem()"));

    ARGTEST(bstrName);
    ARGTEST(ppNode);
    *ppNode = null;

    // watch for empty node list
    if ( !_pParent)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        OMREADLOCK(this->_pParent);

        TRY
        {
            Node * pParentNode = _pParent->getNodeData();
            Node * pNode = FindQualifiedNode(bstrName, bstrNameSpaceURN, pParentNode, _eNodeType);

            if ( pNode)
            {
                *ppNode = (IXMLDOMNode *)pNode->getDOMNodeWrapper();
            }
            else
            {
                //*ppNode = null;
                hr = S_FALSE;
            }
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY

    }

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMNamedNodeMapList::removeQualifiedItem(
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrNameSpaceURN,
    /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "DOMNodeList::removeQualifiedItem()"));

    ARGTEST(bstrName);
    ARGTEST(ppNode);
    *ppNode = null;

    // watch for empty node list
    if ( !_pParent)
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    if (_eNodeType != Element::ATTRIBUTE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    {
        OMREADLOCK(this->_pParent);

        TRY
        {
            Node * pParentNode = _pParent->getNodeData();
            Node * pNode = FindQualifiedNode(bstrName, bstrNameSpaceURN, pParentNode, _eNodeType);

            if ( pNode)
            {
                if (ppNode)
                    *ppNode = (IXMLDOMNode *)pNode->getDOMNodeWrapper();
                // makes sure that all wrapper Document * cached ptrs are null-d and removed IDs form ID hash
                pParentNode->removeNode(pNode, true);
            }
            else
            {
                if (ppNode)
                    *ppNode = null;
                hr = S_FALSE;
            }
        }
        CATCH
        {
            hr = ERESULTINFO;
        }
        ENDTRY

    }

Cleanup:
    return hr;
}




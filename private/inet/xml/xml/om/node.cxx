/*
 * @(#)Node.cxx 1.0 2/25/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "node.hxx"

#include "domnode.hxx"
// BUGBUG until entities handled differently...
#ifndef _XMLNAMES_HXX
#include "xmlnames.hxx"
#endif

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#ifndef _CORE_UTIL_WSSTRINGBUFFER
#include "core/util/wsstringbuffer.hxx"
#endif

#ifndef _XML_PARSER_DTD
#include "dtd.hxx"
#endif

#ifndef _XML_PARSER_ELEMENTDECL
#include "elementdecl.hxx"
#endif

#ifndef _XML_PARSER_NOTATION
#include "notation.hxx"
#endif

#ifndef _XML_PARSER_ATTDEF
#include "attdef.hxx"
#endif

#ifndef _VARIANT_HXX
#include "core/com/variant.hxx"
#endif

#ifndef _CORE_UTIL_CHARTYPE_HXX
#include "core/util/chartype.hxx"
#endif

#ifndef _XML_DOM_NAMESPACEMGR
#include "xml/om/namespacemgr.hxx"
#endif

#ifndef _XQL_PARSER_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

#ifndef _SCHEMANAMES_HXX
#include "xml/schema/schemanames.hxx"
#endif

DeclareTag(tagNode, "Node", "Node data tree");

#define PMATCH(o,c) (!o || CHECKTYPEID(*o, c))


// {EEA82E63-1111-11d2-84EA-0000F87A7782}
const IID Node::s_IID = 
{ 0xeea82e63, 0x1111, 0x11d2, { 0x84, 0xea, 0x0, 0x0, 0xf8, 0x7a, 0x77, 0x82 } };


void * 
Node::operator new (size_t size, NodeManager * pManager)
{
    Assert(pManager);
    void * p = pManager->Alloc();
    if (p)
    {
        // start refcount locked !
        TLSDATA * ptlsdata = GetTlsData();
        ((Node *)p)->_refs = (ptlsdata->_reModel == Rental) ? REF_RENTAL
                                                            : REF_LOCKED;
#if DBG == 1
        ((Node *)p)->_dwTID = GetTlsData()->_dwTID;
#endif
    }
    else
        Exception::throwEOutOfMemory();
    return p;
}


void 
Node::operator delete (void * p)
{
    SlotPage::PAGE(p)->getSlotManager()->Free(p);
}


Node * 
Node::newNode(Element::NodeType type, NameDef * pName, Document * pDoc, NodeManager * pManager)
{
    Assert( pDoc);
    Assert(pManager);
    return new (pManager) Node(type, pName, pDoc);
}


Node * 
Node::newNodeFast(Element::NodeType type, NameDef * pName, Node * pParent,
                  AWCHAR * pText, const TCHAR *pwcContext, ULONG ulContentLen, 
                  Document * pDoc, NodeManager * pManager)
{
    Assert( pDoc);
    Assert(pManager);
    Node * pNode = new (pManager) Node(type, pName, pDoc);

    if (ulContentLen)
    {
        Assert(pwcContext);
        AWCHAR * chars = new (ulContentLen) AWCHAR;
        chars->simpleCopy(0, ulContentLen, pwcContext);
        // this is safe because we just created the object, thus there is nothing to clear up after
        pNode->_valuetype = VAL_STR;
        assign(&pNode->_pText, chars);
    }
    else if (pText)
    {
        pNode->_valuetype = VAL_STR;
        assign(&pNode->_pText, pText);
    }

    if (pParent)
    {
        if (pParent->isCollapsedText())
            pParent->uncollapse();
        pParent->_append(pNode, &pParent->_pLast);
    }

    return pNode;
}


Node * 
Node::newDocumentNode(Document * pDoc, NodeManager * pManager)
{
    Assert(pManager);
    Node * pNode = new (pManager) Node(Node::DOCUMENT, null, pDoc);
    if ( pNode)
    {
        pDoc->weakAddRef();
    }
    else
        Exception::throwE((HRESULT)E_OUTOFMEMORY);

    return pNode;
}


const char * s_achTypes[] =
{
    "ELEMENT",
    "PCDATA",
    "COMMENT",
    "DOCUMENT",
    "DTD",
    "PI",
    "CDATA",
    "ENTITY",
    "NOTATION",
    "ELEMENTDECL",
    "NAMESPACE",
    "ENTITYREF",
    "WHITESPACE",
    "INCLUDESECTION",
    "IGNORESECTION",
    "ATTRIBUTE",
    "TYPEDVALUE",
    "DOCFRAG",
    "XMLDECL",
};

// static
String * 
Node::NodeTypeAsString(Element::NodeType eType)
{
    const char * pc;
    if (eType < Element::ELEMENT || Element::XMLDECL < eType)
        pc = "[UNKNOWN]";
    else
        pc = s_achTypes[eType];
    return String::newString(pc);
}

String * 
Node::getNodeTypeAsString()
{
    return NodeTypeAsString(getNodeType());
}

#if DBG == 1

const char * _type(Node * pN)
{
    Assert(pN->getNodeType() <= Element::XMLDECL);
    return s_achTypes[pN->getNodeType()];
}

String * _name(Node * pN) 
{ 
    TRY
    {
        Name * pName = pN->getName();
        return pName ? pName->toString() : null;
    }
    CATCH
    {
    }
    ENDTRY
    return null;
}

String * _text(Node * pN)
{
    TRY
    {
        const AWCHAR * pText = pN->getNodeType() != Element::ELEMENT ? pN->getNodeText() : null;
        return pText ? String::newString(pText) : null;
    }
    CATCH
    {
    }
    ENDTRY
    return null;
}

void TraceNode(Node * pNode, char * pText)
{
    TraceTag((tagNode, "%s node %X:%s:<%s>:[%s]", pText, pNode, _type(pNode), (char *)AsciiText(_name(pNode)), (char *)AsciiText(_text(pNode))));
}

#endif

#if defined(UNIX) || DBG == 1
const
Node::ValueType s_aNodeType2ValueType[] =
{
    /**
     * A general container element having optional attributes
     * and optional child elements.
     */
Node::VAL_PARENT,//    ELEMENT = 0,
    /**
     * A text element that has no children or attributes and that
     * contains parsed character data.
     */
Node::VAL_STR,//    PCDATA = 1,
    /**
     * An XML comment ( &lt;!-- ... --&gt; ).
     */
Node::VAL_STR,//    COMMENT = 2,
    /**
     * Reserved for use by the Document node only.
     */
Node::VAL_PARENT,//    DOCUMENT = 3,
    /**
     * Reserved for use by the DTD node only.
     */
Node::VAL_PARENT,//    DOCTYPE = 4,
    /**
     * A processing instruction node ( &lt;? ... ?&gt; ).
     */
Node::VAL_STR,//    PI = 5,
    /**
     * Raw character data specified with special CDATA construct:
     * &lt;![CDATA[...]]&gt;
     * where ... can be anything except ]]&gt; including HTML tags. 
     */
Node::VAL_STR,//    CDATA = 6,     
    /**
     * An entity nodes. 
     */ 
// for compatibility..
Node::VAL_PARENT,//    ENTITY = 7, 
    /**
     * A notation nodes. 
     */
Node::VAL_PARENT,//    NOTATION = 8,     
    /**
     * An element * declaration nodes. 
     */
Node::VAL_PARENT,//    ELEMENTDECL = 9,     
    /**
     * A name space node that declares new name spaces in the element tree
     */
Node::VAL_PARENT,//    NAMESPACE = 10,
    /**
     * Entity reference nodes
     */
Node::VAL_PARENT,//    ENTITYREF = 11,
    /**
     * Ignorable white space between elements.
     */
Node::VAL_STR,//    WHITESPACE = 12,
     /**
     * INCLUDE conditional section
     */
Node::VAL_PARENT,//    INCLUDESECTION = 13,
    /**
     * IGNORE conditional section
     */
Node::VAL_PARENT,//    IGNORESECTION = 14
    /**
     * ATTRIBUTE node
     */
Node::VAL_PARENT,//    ATTRIBUTE = 15,
    /**
     *  node storing typed value
     */
Node::VAL_TYPED,//    TYPEDVALUE = 16,
    /**
     * Document Fragment
     */
Node::VAL_PARENT,//    DOCFRAG = 17,
    /**
     * XML Decl PI
     */
Node::VAL_PARENT,//    XMLDECL = 18,
};

#endif

#ifndef UNIX
const
DWORD s_aNodeType2Flags[] =
{
    /**
     * A general container element having optional attributes
     * and optional child elements.
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::ELEMENT << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * A text element that has no children or attributes and that
     * contains parsed character data.
     */
    (Node::VAL_STR << SHIFT_valuetype) | (Node::PCDATA << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * An XML comment ( &lt;!-- ... --&gt; ).
     */
    (Node::VAL_STR << SHIFT_valuetype) | (Node::COMMENT << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * Reserved for use by the Document node only.
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::DOCUMENT << SHIFT_nodetype) | (1 << SHIFT_fDocument) | (1 << SHIFT_fFinished),
    /**
     * Reserved for use by the DTD node only.
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::DOCTYPE << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * A processing instruction node ( &lt;? ... ?&gt; ).
     */
    (Node::VAL_STR << SHIFT_valuetype) | (Node::PI << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * Raw character data specified with special CDATA construct:
     * &lt;![CDATA[...]]&gt;
     * where ... can be anything except ]]&gt; including HTML tags. 
     */
    (Node::VAL_STR << SHIFT_valuetype) | (Node::CDATA << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),     
    /**
     * An entity nodes. 
     */ 
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::ENTITY << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished), 
    /**
     * A notation nodes. 
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::NOTATION << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),     
    /**
     * An element * declaration nodes. 
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::ELEMENTDECL << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),     
    /**
     * A name space node that declares new name spaces in the element tree
     */
     0, // (Node::VAL_PARENT << SHIFT_valuetype) | (Node::NAMESPACE << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * Entity reference nodes
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::ENTITYREF << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * Ignorable white space between elements.
     */
     0, // (Node::VAL_STR << SHIFT_valuetype) | (Node::WHITESPACE << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
     /**
     * INCLUDE conditional section
     */
    0, // (Node::VAL_PARENT << SHIFT_valuetype) | (Node::INCLUDESECTION << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * IGNORE conditional section
     */
     0, // (Node::VAL_PARENT << SHIFT_valuetype) | (Node::IGNORESECTION << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * ATTRIBUTE node
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::ATTRIBUTE << SHIFT_nodetype) | (1 << SHIFT_fAttribute) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     *  node storing typed value
     */
    (Node::VAL_TYPED << SHIFT_valuetype) | (Node::TYPEDVALUE << SHIFT_nodetype) | (1 << SHIFT_fVariant) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * Document Fragment
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::DOCFRAG << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
    /**
     * XML Decl PI
     */
    (Node::VAL_PARENT << SHIFT_valuetype) | (Node::XMLDECL << SHIFT_nodetype) | (1 << SHIFT_fFloating) | (1 << SHIFT_fFinished),
};
#endif

// maps the allowable ValueTypes for a given NodeType
const
unsigned s_aNodeType2PermitedValueTypes[] =
{
    //    ELEMENT = 0,
    (1 << Node::VAL_PARENT),

    //    PCDATA = 1,
    (1 << Node::VAL_STR),

    //    COMMENT = 2,
    (1 << Node::VAL_STR),

    //    DOCUMENT = 3,
    (1 << Node::VAL_PARENT),

    //    DOCTYPE = 4,
    (1 << Node::VAL_PARENT),

    //    PI = 5,
    (1 << Node::VAL_STR),

    //    CDATA = 6,     
    (1 << Node::VAL_STR),

    //    ENTITY = 7, 
    (1 << Node::VAL_PARENT),

    //    NOTATION = 8,     
    (1 << Node::VAL_PARENT),

    //    ELEMENTDECL = 9,
    (1 << Node::VAL_PARENT),

    //    NAMESPACE = 10,
    0,

    //    ENTITYREF = 11,
    (1 << Node::VAL_PARENT),

    //    WHITESPACE = 12,
    0,

    //    INCLUDESECTION = 13,
    0,

    //    IGNORESECTION = 14
    0,

    //    ATTRIBUTE = 15,
    ((1 << Node::VAL_PARENT) | (1 << Node::VAL_OBJ)),

    //    TYPEDVALUE = 16,
    (1 << Node::VAL_TYPED),

    //    DOCFRAG = 17
    (1 << Node::VAL_PARENT),

    //    XMLDECL = 18
    (1 << Node::VAL_PARENT),
};

// maps the allowable types of sub-nodes
const
bool s_aNodeTypeAllowsAttributes[] =
{
    //    ELEMENT = 0,
    true,

    //    PCDATA = 1,
    false,

    //    COMMENT = 2,
    false,

    //    DOCUMENT = 3,
    false,

    //    DOCTYPE = 4,
    true,

    //    PI = 5,
    false,

    //    CDATA = 6,     
    false,

    //    ENTITY = 7, 
    true,

    //    NOTATION = 8,     
    true,

    //    ELEMENTDECL = 9,     
    true,

    //    NAMESPACE = 10,
    true,

    //    ENTITYREF = 11,
    true,

    //    WHITESPACE = 12,
    false,

    //    INCLUDESECTION = 13,
    true,

    //    IGNORESECTION = 14
    true,

    //    ATTRIBUTE = 15,
    false,

    //    TYPEDVALUE = 16,
    false,

    //    DOCFRAG = 17
    false,

    //    XMLDECL = 18
    true,
};


// maps the allowable types of sub-nodes
const
bool s_aNodeTypeAllowsChildren[] =
{
    //    ELEMENT = 0,
    true,

    //    PCDATA = 1,
    false,

    //    COMMENT = 2,
    false,

    //    DOCUMENT = 3,
    true,

    //    DOCTYPE = 4,
    true,

    //    PI = 5,
    false,

    //    CDATA = 6,     
    false,

    //    ENTITY = 7, 
    true,

    //    NOTATION = 8,     
    true,

    //    ELEMENTDECL = 9,     
    true,

    //    NAMESPACE = 10,
    true,

    //    ENTITYREF = 11,
    true,

    //    WHITESPACE = 12,
    false,

    //    INCLUDESECTION = 13,
    true,

    //    IGNORESECTION = 14
    true,

    //    ATTRIBUTE = 15,
    true,

    //    TYPEDVALUE = 16,
    false,

    //    DOCFRAG = 17
    true,

    //    XMLDECL = 17
    false,
};

// maps the allowable types of sub-nodes
const
unsigned uNodeTypeRequireName = (1 << (int)Element::ELEMENT)  | (1 << (int)Element::DOCTYPE)   | 
                                (1 << (int)Element::PI)       | (1 << (int)Element::ENTITY)    | 
                                (1 << (int)Element::NOTATION) | (1 << (int)Element::ENTITYREF) | 
                                (1 << (int)Element::ATTRIBUTE);

Node::Node(Element::NodeType type, NameDef * pName, Document * pDoc) : _pDocument(pDoc)
{
    Assert(type >= ELEMENT && type <= XMLDECL);
#ifdef UNIX
    // do it the old way...
    _nodetype = type;
    _valuetype = s_aNodeType2ValueType[type];
    _fAttribute = _nodetype == ATTRIBUTE;
    _fVariant = _nodetype == TYPEDVALUE;
    _fDocument = _nodetype == DOCUMENT;
    _fFloating = !_fDocument;
    _fFinished = 1; // assume Document::createNode schenario.
#else
    _flags = s_aNodeType2Flags[type];
    Assert(_nodetype == (unsigned)type);
    Assert(_valuetype == (unsigned)s_aNodeType2ValueType[type]);
    Assert(_fAttribute == (unsigned)(_nodetype == ATTRIBUTE));
    Assert(_fVariant == (unsigned)(_nodetype == TYPEDVALUE));
    Assert(_fDocument == (unsigned)(_nodetype == DOCUMENT));
    Assert(_fFloating == (unsigned)!_fDocument);
    Assert(_fFinished == 1); // assume Document::createNode schenario.
#endif
    // check that we have a namedef (and a name if we except it)
    Assert(!(uNodeTypeRequireName & (1 << (int)type)) || (pName && pName->getName()));
//    assign(&_pName, pName);
    if (pName)
    {
        pName->_addRef();
        _pName = pName;
        if (type == ELEMENT && pName->getName()->getNameSpace() == XMLNames::atomDTTYPENS)
        {
            _fDTName = true;
            _datatype = LookupDataType(pName->getName()->toString(), false);
            if (DT_USER_DEFINED == _datatype)
                _datatype = DT_NONE;
            _fDT = (DT_NONE != _datatype);
        }
    }

    TraceTag((tagNode, "create node %X:%s:<%s>", this, _type(this), (char *)AsciiText(_name(this))));
}

Node::~Node()
{
}

void
Node::finalize()
{
/*	
    BUGBUG: AsciiText() allocates objects, which could cause a rental object 
	        be assigned to an non-rental object during 
    TraceTag((tagNode, "delete node %X:%s:<%s>", this, _type(this), (char *)AsciiText(_name(this))));
*/

    if ( _fDocument && _pDocument)
        _pDocument->weakRelease();

    _fReadOnly = 0;

    if (_fVariant)
    {
        Assert(getValueType() == VAL_TYPED);
        clearVariantData();
    }
    else
    {
        switch( getValueType())
        {
        case VAL_PARENT:
            {
                // remove all children and delete if no wrappers exists
                Assert(_pLast != (Node *)REF_LOCKED);
                AWCHAR* pText = getCollapsedText();
                if (pText)
                {
                    pText->Release();
                }
                else
                {
                    while (_pLast)
                    {
                        Node * pNode = _pLast->getSibling();
                        _remove(pNode);
                    }
                }
            }
            break;

        case VAL_STR:
        case VAL_OBJ:
            if (_pValue)
                _pValue->Release();
            break;
        }
    }

    if (_fTyped)
    {
        release(&_pTypedValue);
    }
    else
    {
        assign(&_pName, null);
    }
}


void
Node::removeID(DTD * pDTD)
{
    Assert(_fID);
    Assert(DT_AV_ID == _datatype);
    Name * pIDName = getContentAsName();
    if ( pIDName)
    {
        Node* pEnteredNode = (Node*)pDTD->findID(pIDName);
        if (_pParent && pEnteredNode == _pParent)
        {
            bool fRemoved = pDTD->removeID(pIDName);
            Assert( fRemoved);
            _fID = false;
            _pParent->_fIDed = false;
        }
    }
}

void
Node::setFloatingRec( bool fFloating)
{
    _fFloating = fFloating;
    if (fFloating && _fID && _fAttribute)
        removeID(_pDocument->getDTD());

    void * pv;
    Node * pNode;
    for(pNode = getFirstNodeNoExpand(&pv); pNode; pNode = getNextNode(&pv))
        pNode->setFloatingRec( fFloating);
}


const static long s_lReadOnlyNodeType = ((1 << (int)Element::DOCTYPE) | 
    (1 << (int)Element::ENTITY) | (1 << (int)Element::NOTATION) | (1 << (int)Element::ENTITYREF));

bool
Node::checkReadOnly()
{
    if (isReadOnly())
        goto ReadOnly;

    if (0 != (s_lReadOnlyNodeType & (1 << (int)getNodeType())))
    {
        goto ReadOnly;
    }

    goto Done;

ReadOnly:
    Exception::throwE(E_FAIL,
                      XMLOM_READONLY, 
                      null);
Done:
    return false;
}   


void
Node::setReadOnly( bool fReadOnly, bool fDeep)
{
    _fReadOnly = fReadOnly;

    if (fDeep)
    {
        void * pv;
        Node * pNode;
        for (pNode = getFirstNodeNoExpand(&pv); pNode; pNode = getNextNode(&pv))
        {   
            pNode->setReadOnly(fReadOnly, true);
        }
    }
}

bool
Node::testNotify() const
{ 
    return (_fDT || XMLNames::name(NAME_DTDT) == getName());
}

void
Node::parseAndSetTypedData(DataType eDataType, bool fParsing, bool fInsideEntity,
                           String * psNewContent,
                           Node * pParent)
{
    VARIANT vDT; vDT.vt = VT_NULL;
    if (fParsing && DT_USER_DEFINED == eDataType)
    {
        // BUGBUG: this is ugly, but we need some way to remember that we had a user-defined DT so that
        // we can throw an error when they call get_nodeTypedValue
        vDT.vt = VT_ERROR;
        V_BSTR(&vDT) = NULL;
        // setNodeDataType needs a non DT_NONE DataType, so we give it DT_USER_DEFINED,
        // and then immidiately _manually_ set the _datatype field to DT_NONE.
        setNodeDataType(DT_USER_DEFINED, &vDT);
        _datatype = DT_NONE;
    }
    else
    {
        if (!pParent)
            pParent = _pParent;

        if ((DT_AV_ID == eDataType || _fID) && 
            !isFloating() && 
            null != pParent && 
            !fInsideEntity)
        {
            // Don't do anything for floating nodes, nor nodes with no parent...
            // Change ID attribute's contents
            DTD * pDTD = _pDocument->getDTD();
            Assert(pDTD); // BUGBUG: should we assert or return?
            bool fThrowErrors = _pDocument->getValidateOnParse();
            if (_fID) // are we currently an ID?
            {
                Name * pOldID = getContentAsName(!fParsing);
                if (pOldID)
                {
                    pDTD->removeID(pOldID);
                    pParent->_fIDed = false;
                }
                _fID = false;
            }
            if (DT_AV_ID == eDataType)
            {
                fThrowErrors = fThrowErrors || !fParsing;
                if (!psNewContent)
                    psNewContent = _dtText(null);
                Name * pNewID = getContentAsName(fThrowErrors, psNewContent);
                if (pNewID)
                    addNewID(pDTD, pNewID, pParent, fThrowErrors);
            }
        }

        if (DT__NON_AV <= eDataType)
            parseTypedContent(_pDocument, psNewContent, eDataType, &vDT, fParsing);

        setNodeDataType(eDataType, &vDT);
    }
    // we don't do anything for AV attribute types (or DT_STRING)
}

void
Node::addNewID(DTD * pDTD, Name * pID, Node * pParent, bool fThrowError)
{
    Assert(pParent);

    if (fThrowError && pParent->_fIDed)
        Exception::throwE(E_FAIL, XML_ATTLIST_DUPLICATED_ID, null);

    if (pDTD->findID(pID))
    {
        if (fThrowError)
            Exception::throwE(E_FAIL, XMLOM_DUPLICATE_ID, pID->toString(), null);
    }
    else
    {
        pDTD->addID(pID, pParent);
        _fID = true;
        pParent->_fIDed = true;
    }
}


void
Node::notifyNew(Node * pParent /* = null */,
                bool fParsing /* = false */, bool fInsideEntity /* = false */,
                ElementDecl * pED /* = null */, AttDef * pAD /* = null */)
{
    Element::NodeType nodetype = getNodeType();
    if (0 == ((1 << (int)nodetype) & ((1 << (int)Element::ATTRIBUTE) | (1 << (int)Element::ELEMENT) | (1 << (int)Element::DOCTYPE))))
        return;

    if (!pParent)
        pParent = _pParent;

    if (nodetype == Node::ATTRIBUTE)
    {
        Name * pName = getName();

        // if this is a dt:dt attribute, apply the datatype
        if (XMLNames::name(NAME_DTDT) == pName)
        {
            String *psType = getInnerText(false, true, false);
            // if we are parsing, then we just mark the (parent) element
            // as typed and it will apply the datatype when it has
            // finish it's content.  Otherwise we apply it now.
            // If parsing and !validateOnParse, then don't throw
            // errors, instead, mark the node has havng had an error
            // and leave it at that.
            DataType eDT = LookupDataType(psType, (!fParsing || _pDocument->getValidateOnParse()));
            if (pParent)
            {
                if (pParent->_fDT)
                    Exception::throwE(E_FAIL, XMLOM_DTDT_DUP, null);
                if (!fParsing)
                {
                    // if this is not really a change, then don't need to do anything
                    // except when it is an ID, then we need to remove the old parent
                    //  and add the new parent to the appropriate hashtables
                    if (eDT != (DataType)pParent->_datatype || DT_AV_ID == eDT)
                        pParent->parseAndSetTypedData(eDT, false /* == fParsing */);
                }
                else // will be done when the Element is finished being parsed
                {
                    pParent->_fDT = eDT != DT_NONE;
                    pParent->_datatype = eDT;
                }
            }
        } else
        // only add ID if parsing
            if (pParent && fParsing)
        {
            if (!pAD && Element::ELEMENT == pParent->getNodeType())
                pAD = getAttDef();

            if (pAD)
            {
                // BUGBUG: can we make this faster for the typical
                // case where we really don't need to do any special
                // datatype handling?
                DataType dt = pAD->getType();
                if (dt != DT_NONE)
                {
                    _fDT = true;
                    _datatype = dt;
                    goto ApplyDataType;
                }
            }

        }
    }
    else if (nodetype == Element::ELEMENT)
    {
        // if it did not have an explicit dt:dt, 
        // then check the DTD for a default one
        if (!_fDT && fParsing)
        {
            Name * pName = getName();
            if (pName->getNameSpace() != XMLNames::atomDTTYPENS)
            {
                if (!pED)
                    pED = getElementDecl();
                if (pED)
                {
                    DataType dt = pED->getDataType();
                    if (dt != DT_NONE)
                    {
                        if (_fDT)
                            Exception::throwE(E_FAIL, XMLOM_DTDT_DUP, null);
                        _fDT = true;
                        _datatype = dt;
                        // fall through to next test
                    }
                }
            }
        }

ApplyDataType:
        if (_fDT)
            parseAndSetTypedData((DataType)_datatype, fParsing, fInsideEntity);
    }
    else // if (nodetype == Node::DOCTYPE)
    {
        Assert(nodetype == Node::DOCTYPE);
        if (fParsing)
            setReadOnly(true, true);
    }
}

void
Node::notifyRemove(Node * pParent)
{
    if (!_fAttribute || (XMLNames::name(NAME_DTDT) != getName()))
        return;
    if (!pParent)
        pParent = _pParent;
    if (!pParent)
        return;
    pParent->removeDataType();
}

void
Node::notifyChangeContent(String * psNewContent)
{
    switch (getNodeType())
    {
    case Element::ATTRIBUTE:
    case Element::ELEMENT:
        if (_fDT)
        {
            parseAndSetTypedData((DataType)_datatype, false, false, psNewContent);
        }
        else if (XMLNames::name(NAME_DTDT) == getName())
        {
            // change DT attribute's contents
            String *psType = psNewContent->trim();
            DataType eNewDT = LookupDataType(psType, true);
            if (null != _pParent)
                _pParent->parseAndSetTypedData(eNewDT, false);
        }
        break;

    case Element::PCDATA:
    case Element::CDATA:
//    case Node::ENTITYREF: // these are const in our impl, so you should never get a changeContent on one !!
        if (!_pParent)
            return;
        if (_pParent->testNotify())
        {
            const WCHAR * pwchars = psNewContent->getWCHARPtr();
            String * ps = _pParent->_dtText(pwchars, psNewContent->length(), null, this, this, null);
            _pParent->notifyChangeContent(ps);
        }
        break;
    }
}

void
Node::deleteChildren(bool fExtNotifications, bool fIntNotifications)
{
    if (isParent())
    {
        if (!fExtNotifications && fIntNotifications)
        {
            notifyChangeContent(String::emptyString());
        }

        Assert(_pLast != (Node *)REF_LOCKED);
        AWCHAR* pText = getCollapsedText();
        if (pText)
        {
            pText->Release();
            _pText = null;
        }
        else
        {
            while (_pLast && !_pLast->_fAttribute)
            {
                Node * pNode = _pLast;
                if (fExtNotifications)
                    moveNode(null, null, pNode, false, false);
                else
                {
                    _remove(pNode);
                    pNode->setFloatingRec(true);
                }
            }
        }
    }
}


unsigned
Node::allowChildren() const
{
    return s_aNodeTypeAllowsChildren[ getNodeType()];
}


unsigned
Node::allowAttributes() const
{
    return s_aNodeTypeAllowsAttributes[ getNodeType()];
}


bool
Node::hasAncestor(const Node * pFindNode)
{
    Node * pNode = this;
    Node * pParent;
    while ( pNode->_pParent != pFindNode &&
            pNode->getNodeType() != Element::DOCUMENT)
    {
        pParent = pNode->_pParent;
        if (!pParent)
            break;
        Assert( CHECKTYPEID(*pParent, Node));
        pNode = pParent;
    }
Cleanup:
    return pNode->_pParent == pFindNode;
}


Node *
Node::getAncestorWithNodeType(Element::NodeType nodeType, bool fNot )
{
    Node *pNode = this;

    while (pNode && (!fNot == (pNode->getNodeType() != nodeType)))
    {
        pNode = pNode->getNodeParent();
    }

    return pNode;
}


bool
Node::setValueType( ValueType eValueType)
{
    if ( eValueType == getValueType())
        return true;

    // check if this nodeType can become the given ValueType
    unsigned uValueTypeToBe = 1 << (unsigned)eValueType;
    unsigned uValueTypeAllowed = s_aNodeType2PermitedValueTypes[ getNodeType()];
    if (uValueTypeToBe & uValueTypeAllowed)
    {
        switch (getValueType())
        {
        case VAL_PARENT:
            deleteChildren(true);
            break;

        case VAL_STR:
        case VAL_OBJ:
            assign(&_pValue, null);
            break;
#if DBG == 1
        case VAL_TYPED:
            Assert(0 && "Should never happen!");
#endif
        }
        _valuetype = eValueType;
        return true;
    }
    return false;
}


Name *
Node::getName() const
{
    NameDef * namedef = _getNameDef();
    if (namedef)
        return namedef->getName();
    else 
        return null;
}

NameDef *
Node::_getNameDef() const
{
    if (_fTyped)
    {
        Assert(PMATCH(_pTypedValue, Node));
        return _pTypedValue->_pName;
    }
    else
    {
        Assert(PMATCH(_pName, NameDef));
        return _pName;
    }
}


void
Node::setName(NameDef * pName)
{
    Assert(pName && pName->getName());

    if (_fTyped)
    {
        Assert(PMATCH(_pTypedValue, Node));
        assign(&_pTypedValue->_pName, pName);
    }
    else
    {
        assign(&_pName, pName);
    }
}


static const long aNodeType2AllowedChildTypes[] =
{
    // ELEMENT = 0, allow ELEMENT, PCDATA, COMMENT, PI, CDATA, ENTITYREF, WHITESPACE, 
    (1 << (int)Element::ELEMENT) | (1 << (int)Element::PCDATA) | (1 << (int)Element::COMMENT) | (1 << (int)Element::PI) | 
        (1 << (int)Element::CDATA) | (1 << (int)Element::ENTITYREF),

    // PCDATA = 1, no children
    0,

    // COMMENT = 2, no children
    0,

    // DOCUMENT = 3, allow ELEMENT (Document Element), COMMENT, PI, DOCTYPE, XMLDECL, WHITESPACE, NAMESPACE
    (1 << (int)Element::ELEMENT) | (1 << (int)Element::COMMENT) | (1 << (int)Element::PI)
        | (1 << (int)Element::DOCTYPE) | (1 << (int)Element::XMLDECL),

    // DOCTYPE = 4, allow CDATA (so internal subset can be updated).
    (1 << (int)Element::CDATA),

    // PI = 5,
    0,

    // CDATA = 6,
    0,

    // ENTITY = 7, (same as ELEMENT)
    (1 << (int)Element::ELEMENT)  | (1 << (int)Element::PCDATA) | (1 << (int)Element::COMMENT)
        | (1 << (int)Element::PI) | (1 << (int)Element::CDATA)  | (1 << (int)Element::ENTITYREF),

    // NOTATION = 8,
    0,

    // ELEMENTDECL = 9,     
    0,

    // NAMESPACE = 10,
    0,

    // ENTITYREF = 11, (character entities have a single PCDATA node under them, but you can never set it via insertNode)
    0,

    // WHITESPACE = 12,
    0,

    // INCLUDESECTION = 13,
    0,

    // IGNORESECTION = 14,
    0,

    // ATTRIBUTE = 15,
    ((1 << (int)Element::PCDATA) | (1 << (int)Element::ENTITYREF)),

    // TYPEDVALUE = 16,
    0,

    // DOCFRAG = 17, 
    ((1 << (int)Element::ELEMENT)  | (1 << (int)Element::PCDATA)  | (1 << (int)Element::COMMENT)     |
     (1 << (int)Element::PI)       | (1 << (int)Element::CDATA)   | (1 << (int)Element::ENTITYREF)   |
     (1 << (int)Element::XMLDECL)),

    // XMLDECL = 18,
    0,
};


void
ValidateInsertNodeTypes(Node * pPotentialParent, Node * pInsert)
{
    long lAllowedTypes = aNodeType2AllowedChildTypes[pPotentialParent->getNodeType()];
    Element::NodeType eNodeType = pInsert->getNodeType();

    if ( eNodeType == Element::DOCUMENT ||
         ! (lAllowedTypes & (1 << (int)pInsert->getNodeType())))
    {
        Exception::throwE(E_FAIL, 
                          XMLOM_INVALIDTYPE,
                          String::newString(s_achTypes[eNodeType]),
                          null);
    }
}



struct NodeInsertInfo
{
    Document * pNewDoc;
    DTD * pNewDTD;
    DTD * pOldDTD;
    NamespaceMgr * pNewNSMgr;
    NamespaceMgr * pOldNSMgr;
    Node * pNewRootParent;
    Node * pRoot;
    bool fFloating;
    bool fLock;
    
    // RNameSpaceStack pNameSpaceStack;
    RHashtable pNewNameSpaces;
};


// Validate:
//  id's are not duplicated
//  !! namespaces do not conflict (list of new NameSpaces)
//  default dt:dt attrs applied
// Update:
//  rebuild NameDef
//  update _pDocument
//  update _fFloating
//  add to new ID table
//  remove IDs from old
void
Node::validateAndUpdateRec( NodeInsertInfo * pInfo, ElementDecl* pED)
{
    bool fValid = true;
    ElementDecl * pThisED = null;
    NameDef * pNameDef = _getNameDef();
    Name * pName = pNameDef ? pNameDef->getName() : null;
    String * pError;
    bool fDefaultDT = _fDT && !_fDTName;
    HRESULT hrError = E_FAIL;
    bool fRoot = (pInfo->pNewRootParent && pInfo->pRoot == this);

    bool fOldFloating = isFloating();
    Document * pOldDoc = _pDocument;

    floating( pInfo->fFloating);

    if (pInfo->pNewDoc != pOldDoc)
    {
        _pDocument = pInfo->pNewDoc;

        // check if we have to release the document because there are
        // outstanding references on this node...
        LONG_PTR lRefs = _refs;
        if (lRefs == REF_LOCKED || !(lRefs & REF_RENTAL))
        {
            lRefs = spinLock();
        }
        if ((lRefs >> REF_SHIFT) > 1)
        {
            pOldDoc->_release();
            _pDocument->_addRef();
        }
        if (!(lRefs & REF_RENTAL))
            unLock(lRefs);

        if (pNameDef)
        {
            Atom * pPrefix = pNameDef->getPrefix();
            NameDef * pNewName = pInfo->pNewNSMgr->createNameDef(pName, null, pPrefix);
            setName( pNewName);
        }
    }

    // don't need to worry about this funky stuff if we don't have a name
    if (pNameDef)
    {
        if (!fOldFloating && _datatype == DT_AV_ID && _fID)
            removeID(pOldDoc->getDTD());

        switch (getNodeType())
        {
        case Element::ELEMENT:
            if (pInfo->pNewDTD)
                pThisED = getElementDecl(pInfo->pNewDTD);
            break;

        case Element::ENTITYREF:
            if (pInfo->pNewDoc != pOldDoc)
            {
                // remove the children of an entity-ref
                deleteChildren(false);
            }
        }
    }

    void * pv;
    Node * pNode;
    for (pNode = getFirstNodeNoExpand(&pv); pNode; pNode = getNextNode(&pv))
    {
        if (pNode->_fAttribute)
        {
            if (XMLNames::name(NAME_DTDT) == pNode->getName())
                fDefaultDT = false;
            pNode->validateAndUpdateRec(pInfo, pThisED);
        }
        else
        {
            pNode->validateAndUpdateRec(pInfo, null);
        }
    }

    // if there was no explicit dt:dt attribute, check new document for a default dt:dt
    if (pNameDef)
    {
        Node * pParent;
        if (fRoot)
            pParent = pInfo->pNewRootParent;
        else
            pParent = null;

        if (!_fDT || fDefaultDT)
        {
            // BUGBUG: Does this properly handle Attributes?
            DTD * pDTD = pInfo->pNewDTD;
            DataType dtNew = DT_NONE;
            VARIANT var; 
            if (pDTD)
            {
                switch (getNodeType())
                {
                case Element::ELEMENT:
                    {
                        pED = getElementDecl(pDTD);
                        if (pED)
                            dtNew = pED->getDataType();
                    }
                    break;

                case Element::ATTRIBUTE:
                    {
                        if (pED || (fRoot && null != (pED = pParent->getElementDecl(pDTD))))
                        {
                            AttDef * pAD = getAttDef( pED, pDTD, 
                                                     (fRoot ? pInfo->pNewRootParent : null));
                            if (pAD)
                                dtNew = pAD->getType();
                        }
                    }
                    break;
                }

                if (dtNew != DT_NONE)
                {
                    // apply new DataType
                    TRY
                    {
                        parseAndSetTypedData(dtNew, false, false, null, pParent);
                    }
                    CATCH
                    {
                        if (pInfo->pOldDTD != pInfo->pNewDTD)
                        {  
                            pError = GETEXCEPTION()->getMessage();
                            hrError = GETEXCEPTION()->getHRESULT();
                            goto Invalid;
                        }
                        else
                        {    // no way (or no need) to restore the previous data
                            Exception::throwAgain();
                        }
                    }
                    ENDTRY
                }
            }
            else if (fDefaultDT)
            {
                // remove previous default datatype
                removeDataType();
            }
        }
        else if (_fDT && !_fFloating &&
                 DT_AV_ID == _datatype && 
                 fRoot)
        {
            if (pParent)
                parseAndSetTypedData(DT_AV_ID, false, false, null, pParent);
        }
    }

    return;

Invalid:
    Node * pRoot = pInfo->pRoot;
    ElementDecl * pRootED = null;
    Node * pRootParent = pRoot->_pParent;
    if (pInfo->pOldDTD && pRoot && pRoot->getNodeType() == Element::ATTRIBUTE &&
        pRootParent)
    {
        pRootED = pRootParent->getElementDecl(pInfo->pOldDTD);
    }
    DTD * pDTDTemp = pInfo->pOldDTD; 
    pInfo->pOldDTD = pInfo->pNewDTD; 
    pInfo->pNewDTD = pDTDTemp;
    pInfo->fFloating = pRootParent ? pRootParent->isFloating() : true;
    pRoot->validateAndUpdateRec( pInfo, pRootED);
    Exception::throwE(pError, hrError);
    // return;
}

void
Node::ValidateAndUpdate(Node * pNewParent, Node * pNewChild)
{
    if ( pNewParent && 
         (pNewChild->getNodeType() == Element::ATTRIBUTE))
    {
        NameDef * pndNewChild = pNewChild->_getNameDef();
        Name * pnNewChild = pndNewChild->getName();
        Atom * pNS = pnNewChild->getNameSpace();
        if (pNS)
        {
            Node * pPotentialConflict = null;
            Atom * pPrefix = pndNewChild->getPrefix();

            if (pNewParent->_getNameDef()->getPrefix() == pPrefix
                && pNewParent->getName()->getNameSpace() != null)
                pPotentialConflict = pNewParent;
            else
            {
                // check if another attribute uses the prefix
                void * pv;
                Node * pAttr;
                for (pAttr = pNewParent->getNodeFirstAttribute(&pv);
                     pAttr != null;
                     pAttr = pNewParent->getNodeNextAttribute(&pv))
                {
                    NameDef * pnd = pAttr->_getNameDef();
                    if (pnd->getPrefix() == XMLNames::atomXMLNS)
                    {
                        if (pnd->getName()->getName() == pPrefix)
                        {
                            if (NamespaceMgr::CanonicalURN(Atom::create(pAttr->getInnerText(false,true, true))) == pNS)
                                goto ThrowNameSpaceConflict;
                        }
                    }
                    else if (pnd->getPrefix() == pPrefix
                             && pnd->getName()->getNameSpace() != null)
                    {
                        pPotentialConflict = pAttr;
                        break;
                    }
                }
            }
            if (pPotentialConflict 
                && pPotentialConflict->getName()->getNameSpace() != pNS)
            {
ThrowNameSpaceConflict:
                Exception::throwE((HRESULT)E_INVALIDARG,  
                                  XMLOM_NAMESPACE_CONFLICT, pPrefix ? pPrefix->toString() : String::emptyString(), null);
            }
        }
    }

    if ( !pNewParent ||
         pNewChild->getNodeDocument() != pNewParent->getNodeDocument() ||
         pNewChild->isFloating() != pNewParent->isFloating())
    {
        NodeInsertInfo Info;
        Document * pOldDoc = pNewChild->getNodeDocument();
        Document * pNewDoc = pNewParent ? pNewParent->getNodeDocument() : pOldDoc;
        Info.pNewDoc = pNewDoc;
        Info.pNewDTD = pNewDoc->getDTD();
        if (Info.pNewDTD && !Info.pNewDTD->validate())
            Info.pNewDTD = null;
        Info.pOldDTD = pOldDoc->getDTD();
        if (Info.pOldDTD && !Info.pOldDTD->validate())
            Info.pOldDTD = null;
        Info.pNewNSMgr = pNewDoc->getNamespaceMgr();
        Info.pOldNSMgr = pOldDoc->getNamespaceMgr();
        Info.pNewRootParent = pNewParent;
        Info.pRoot = pNewChild;
        Info.fFloating = pNewParent ? pNewParent->isFloating() : true;
        Info.fLock = (pOldDoc->model() == MultiThread);

        TRY
        {
            pNewChild->validateAndUpdateRec( &Info, null);
        }
        CATCH
        {
            Info.pNewNameSpaces = null;
            Exception::throwAgain();
        }
        ENDTRY
    }
}


void
Node::_append(Node * pChild, Node ** ppLast)
{
    pChild->attach();
    pChild->_fFloating = _fFloating;
    Assert(pChild->_pDocument == _pDocument);
    Node * pLast = *ppLast;
    if (!pLast)
    {
        Assert(setValueType(VAL_PARENT));
        _valuetype = VAL_PARENT;
        pChild->setSibling( pChild);
    }
    else
    {
        Assert(_valuetype == VAL_PARENT);
        Assert(ATTRIBUTE != pChild->getNodeType() || ATTRIBUTE == pLast->getNodeType());
        pChild->setSibling( pLast->getSibling());
        pLast->setSibling( pChild);
    }
    pChild->_pParent = this;
    *ppLast = pChild;
}


void
Node::_insert(Node * pChild, Node * pBefore)
{

    TraceTag((tagNode, "insert node %X:%s:<%s>:[%s] into %X:%s:<%s>:[%s] before %X:%s:<%s>:[%s]", 
        pChild, _type(pChild), (char *)AsciiText(_name(pChild)), (char *)AsciiText(_text(pChild)),
        this, _type(this), (char *)AsciiText(_name(this)), (char *)AsciiText(_text(this)),
        pBefore, ( pBefore ? _type(pBefore) : "" ), ( pBefore ? (char *)AsciiText(_name(pBefore)) : ""), ( pBefore ? (char *)AsciiText(_text(pBefore)) : "" )
        ));

    // either we are a parent or
    //  if we are inserting an attribute. we can become a parent holding attributes
    //  if we are not inserting, we can become a parent holding children
    if ( !isParent() && !setValueType(VAL_PARENT))
    {
        Assert(0 && "Should never happen");
        Exception::throwE(E_UNEXPECTED); // should never happen
    }
    
    // remove the node being inserted from it's old tree
    if (pChild->_pParent)
        pChild->_pParent->_remove( pChild);

    if (isCollapsedText())
        uncollapse();

    Assert(_pLast != (Node *)REF_LOCKED);

    if (pChild->_fAttribute)
    {
#if DBG == 1
        if ( !allowAttributes())
        {
            Assert(0 && "Should never happen");
            Exception::throwE(E_UNEXPECTED); // should never happen
        }
#endif
        Assert(pBefore == null);
        // shortcut for first child...
        if (!_pLast)
        {
            _pLast = pChild;
            pChild->setSibling( pChild);
        }
        else
        {
            // insert always as last attribute...
            Node * pNode = _pLast;
            for (;;)
            {
                if (!pNode->getSibling()->_fAttribute)
                    break;
                pNode = pNode->getSibling();
                if (pNode == _pLast)
                    break;
            }
            // we are either at _pLast or at the last attribute, insert after pNode
            pChild->setSibling( pNode->getSibling());
            pNode->setSibling( pChild);
            if (_pLast == pNode && pNode->_fAttribute)
            {
                _pLast = pChild;
            }
        }
    }
    else    // pChild->_fAttribute
    {
#if DBG == 1
        if ( !allowChildren())
        {
            Assert( 0 && "Should never happen");
            Exception::throwE((HRESULT)E_UNEXPECTED);
        }
#endif
        // shortcut for first child...
        if (!_pLast || !pBefore)
        {
            Assert(pBefore == null);
            Assert(!pChild->_fDocument);
            _append(pChild, &_pLast);
            return;
        }
        else
        {
            // here we assume inserting a non-attribute node since it is positioned...
            if (pBefore->_nodetype == ATTRIBUTE)
            {
                Assert( 0 && "insert position node can not be an attribute");
                Exception::throwE((HRESULT)E_INVALIDARG);
            }

            Assert(pChild->_nodetype != ATTRIBUTE);

            Node * pNode = _pLast;
            for (;;)
            {
                if (pNode->getSibling() == pBefore)
                    break;
                pNode = pNode->getSibling();
                if (pNode == _pLast)
                {
                    // didn't find pBefore
                    Exception::throwE(E_INVALIDARG, XMLOM_INSERTPOS_NOTFOUND, null);
                }
            }
            pChild->setSibling( pBefore);
            pNode->setSibling( pChild);
            if (_pLast->getSibling() == pBefore)
            {
                _pLast->setSibling( pChild);
            }
        }
    }
    pChild->attach();
    pChild->_fFloating = this->_fFloating;
    pChild->_pParent = this;
    Assert(pChild->_pDocument == _pDocument);
}


void
Node::_remove(Node * pChild)
{
    Assert(isParent());
/*
    BUGBUG: AsciiText() allocates objects, which causes a rental object be assigned to an non-rental object during 
            finalize(). 
    TraceTag((tagNode, "remove node %X:%s:<%s>:[%s] from %X:%s:<%s>:[%s]", 
        pChild, _type(pChild), (char *)AsciiText(_name(pChild)), (char *)AsciiText(_text(pChild)),
        this, _type(this), (char *)AsciiText(_name(this)), (char *)AsciiText(_text(this))
        ));
*/
    Assert(! isCollapsedText());
    Assert(_pLast != (Node *)REF_LOCKED);
    Node * pNode = _pLast;
    if (_pLast)
    {
        for (;;)
        {
            if (pNode->getSibling() == pChild)
                break;
            pNode = pNode->getSibling();
            if (pNode == _pLast)
            {
                pNode = null;
                break;
            }
        }
    }

    if (pNode)
    {
        pNode->setSibling( pChild->getSibling());
        pChild->setSibling( null);
        if (_pLast == pChild)
        {
            if (_pLast == pNode)
                _pLast = null;
            else
                _pLast = pNode;
        }
        
        pChild->_fFloating = TRUE;
        pChild->_pParent = null;
        pChild->detach();
    }
    else
    {
        Exception::throwE(E_FAIL, XMLOM_NODE_NOTFOUND, null);
    }
}

void 
Node::moveNode(Node * pChildNode, Node * pRefNode, Node * pRemoveNode, bool fAttr, bool fNotify)
{
    Element::NodeType eChildType = ANY;
    bool fEvent = false;

    checkReadOnly();

    if (isCollapsedText())
        uncollapse();

    if (pChildNode)
    {
#ifdef RENTAL_MODEL
        if (model() != pChildNode->model())
            Exception::throwE(E_FAIL, XMLOM_INVALID_MODEL, null);
#endif
        eChildType = pChildNode->getNodeType();

        if (pRefNode && pRefNode->_pParent != this)
        {
            Exception::throwE(E_INVALIDARG, 
                              fAttr ? XMLOM_INVALID_ATTR : XMLOM_INVALID_INSERT_POS, 
                              null);
        }

        if (Element::DOCTYPE == eChildType)
            goto ErrReadOnly;

        Node * pOldParent = pChildNode->getNodeParent();
        if (pOldParent)
            pOldParent->checkReadOnly();

        // check ancestor chain to avoid loops
        if (pChildNode == this || hasAncestor( pChildNode))
            Exception::throwE(E_FAIL, XMLOM_INVALID_INSERT_PARENT, null);

        if (_fDocument)
        {
            int count = 0;
            if (Element::DOCFRAG == eChildType)
            {
                void * pv;
                Node * pNode;
                for (pNode = pChildNode->getNodeFirstChild(&pv); pNode; pNode = pChildNode->getNodeNextChild(&pv))
                    if (pNode->getNodeType() == Element::ELEMENT)
                    {
                        count++;
                        if (count > 1)
                            goto ErrMultiRoots;
                    }
            }
            if (Element::ELEMENT == eChildType || count)
            {
                Node * pDocElem = find(null, Element::ELEMENT, null);
                if (pDocElem && pDocElem != pRemoveNode)
                    goto ErrMultiRoots;
            }
        }

        if (Element::DOCFRAG != eChildType)
        {
            // check that pInsert can be inserted (based on NodeType)
            if (fAttr)
            {
                if (!pChildNode->_fAttribute)
                    Exception::throwE(E_FAIL, XMLOM_INVALIDTYPE,
                                      pChildNode->getNodeTypeAsString(),
                                      null);
                if (pChildNode->_pParent)
                    Exception::throwE(E_FAIL, XMLOM_ATTRMOVE,
                                      null);
            }
            else 
                ValidateInsertNodeTypes(this, pChildNode);
        }
        else
        {
            // docfrag acts as a container...
            void * pv;
            Node * pInsertNode, *pNextSib;
            for (pInsertNode = pChildNode->getNodeFirstChild(&pv); pInsertNode; pInsertNode = pChildNode->getNodeNextChild(&pv))
            {
                ValidateInsertNodeTypes(this, pInsertNode);
            }
        }

        if ( pChildNode == pRefNode)
        {
            Assert(null == pRemoveNode || pChildNode == pRemoveNode);
            return;
        }

        if (pRefNode && pRefNode->_fAttribute)
            pRefNode = null;
    }

    if (pRemoveNode)
    {
        Assert(pRemoveNode != pChildNode);
        if (pRemoveNode->getNodeType() == Element::DOCTYPE)
            goto ErrReadOnly;

        // make sure that it's parent is correct
        if (pRemoveNode->_pParent != this)
            Exception::throwE(E_INVALIDARG,
                              fAttr ? XMLOM_INVALID_ATTR : XMLOM_NOTCHILD,
                              null);

        if (pRemoveNode->getNodeType() == Element::ATTRIBUTE &&
            pRemoveNode->getName()->getNameSpace() == XMLNames::atomURNXMLNS)
            goto ErrReadOnly;
    }

    if (fNotify)
    {
        NodeType eParentType = getNodeType();
        if (eParentType == ELEMENT || eParentType == ATTRIBUTE)
        {
            if (pRemoveNode)
            {
                pRemoveNode->notifyRemove();
                fEvent = true;
            }

            if (pChildNode)
            {
                fEvent = true;
                if (Element::DOCFRAG != eChildType)
                {
                    if (!(Element::ELEMENT == eChildType && pChildNode->_fDT))
                        pChildNode->notifyNew(this);
                }
                else
                {
                    // docfrag acts as a container...
                    void * pv;
                    Node * pInsertNode;
                    for (pInsertNode = pChildNode->getNodeFirstChild(&pv); 
                         pInsertNode; 
                         pInsertNode = pChildNode->getNodeNextChild(&pv))
                    {
                        pInsertNode->notifyNew(this);
                    }
                }
            }

            // if this is an attribute which is an ID or DT:DT attribute, 
            //  or this is an element and is datatyped
            if (((pChildNode && !pChildNode->_fAttribute) ||
                 (pRemoveNode && !pRemoveNode->_fAttribute))
                && testNotify())
            {
                String * ps = this->_dtText(null, 0, pChildNode, pRefNode, pRemoveNode, null);
                this->notifyChangeContent(ps);
                fEvent = true;
            }
        }
    }

    TRY
    {
        if (pRemoveNode)
        {
            // remember were to insert...
            if (pRefNode == pRemoveNode)
                pRefNode = pRemoveNode->getNextSibling();

            // notify the listener that we're about to remove a node
            Node * pNodeBefore = (_pLast == pRemoveNode) ? NULL : pRemoveNode->getSibling();
            _pDocument->NotifyListener(XML_REASON_NodeRemoved, XML_PHASE_AboutToDo,
                                        pRemoveNode, this, pNodeBefore);

            TRY
            {
                // makes sure that all wrapper Document * cached ptrs are null-d and removed IDs form ID hash
                ValidateAndUpdate( null, pRemoveNode);

                _remove( pRemoveNode);
        
                // notify the listener that we removed the node
                _pDocument->NotifyListener(XML_REASON_NodeRemoved, XML_PHASE_DidEvent,
                                            pRemoveNode, this, pNodeBefore);
            }
            CATCH
            {
                // notify the listener that we failed to remove the node
                _pDocument->NotifyListener(XML_REASON_NodeRemoved, XML_PHASE_FailedToDo,
                                            pRemoveNode, this, pNodeBefore);
                Exception::throwAgain();
            }
            ENDTRY
        }


        if (pChildNode)
        {
            bool fOKdAdd = false;
            Node *pNodeOldParent = pChildNode->getNodeParent();

            // first, remove the node from its old position.  Do this via
            // a recursive call, so that the right notifications happen.
            if (pNodeOldParent)
            {
                Assert(!fAttr && "Can't move an attribute node");
                pNodeOldParent->removeNode(pChildNode, false);
            }
            
            TRY
            {
                // notify the listener that we're about to add a node
                _pDocument->NotifyListener(XML_REASON_NodeAdded, XML_PHASE_AboutToDo,
                                            pChildNode, this, pRefNode);
                fOKdAdd = true;

                ValidateAndUpdate( this, pChildNode);

                if (Node::DOCFRAG != eChildType)
                {
                    _insert(pChildNode, pRefNode);

//                    if (fAttr && fEvent && fNotify)
//                        pChildNode->notifyNew();
                }
                else
                {
                    // docfrag acts as a container...
                    void * pv;
                    Node * pInsertNode, *pNextSib;
                    for (pInsertNode = pChildNode->getNodeFirstChild(&pv); pInsertNode; pInsertNode = pNextSib)
                    {
                        pNextSib =pChildNode->getNodeNextChild(&pv);
                        _insert(pInsertNode, pRefNode);
                    }
                }

                // notify the listener that we added a node
                _pDocument->NotifyListener(XML_REASON_NodeAdded, XML_PHASE_DidEvent,
                                            pChildNode, this, pRefNode);
            }
            CATCH
            {
                if (fOKdAdd)
                {
                    // notify the listener that we failed to add the node
                    _pDocument->NotifyListener(XML_REASON_NodeAdded, XML_PHASE_FailedToDo,
                                                pChildNode, this, pRefNode);
                }

                if (pRemoveNode)
                {
                    // put node back in...
                    moveNode(pRemoveNode, pRefNode, null, fAttr);
                }

                Exception::throwAgain();
            }
            ENDTRY
        } // inserting new node

        return;
    }
    CATCH
    {
        if (fEvent && fNotify)
        {
            if (pRemoveNode && this == pRemoveNode->_pParent)
                pRemoveNode->notifyNew();
            if (Element::DOCFRAG != eChildType)
            {
                if (!(Element::ELEMENT == eChildType && pChildNode->_fDT))
                    pChildNode->notifyRemove(this);
            }
            else
            {
                // docfrag acts as a container...
                void * pv;
                Node * pInsertNode;
                for (pInsertNode = pChildNode->getNodeFirstChild(&pv); 
                     pInsertNode; 
                     pInsertNode = pChildNode->getNodeNextChild(&pv))
                {
                    pInsertNode->notifyRemove(this);
                }
            }
        }
        Exception::throwAgain();
    }
    ENDTRY

ErrReadOnly:
    Exception::throwE(E_FAIL, XMLOM_READONLY, null);
ErrMultiRoots:
    Exception::throwE(E_FAIL, MSG_E_MULTIPLEROOTS, null);
}


Node *
Node::Variant2Node( VARIANT * pVar)
{
    return (Node *)Variant::QIForIID(pVar, &Node::s_IID);
}


Node * 
Node::getFirstNodeNoExpand(void ** ppv) const
{
    Node * pNode = null;
    if ( isParent())
    {
        if (isCollapsedText())
        {
            // we deliberately do NOT expand the single text node optimization
            // here because in all usages of this method, the client code does
            // NOT need text nodes.  This saves everyone having to test 
            // isCollapsedText() before calling this method.
            pNode = null;
        }
        else if (_pLast && _pLast != (Node *)REF_LOCKED)
            pNode = _pLast->getSibling();
    }
    if (pNode)
        *ppv = (void *)pNode;
    return pNode;
}


Node * 
Node::getFirstNode(void ** ppv)
{
    Node * pNode = null;
    if (isParent())
    {
        if (isCollapsedText())
            uncollapse();
        if (_pLast && _pLast != (Node *)REF_LOCKED)
            pNode = _pLast->getSibling();
        else if (ENTITYREF == _nodetype)
        {
            if (isSpecified())
                _expandEntityRef();
            if (_pLast)
                pNode = _pLast->getSibling();
        }
    }
    if (pNode)
        *ppv = (void *)pNode;
    return pNode;
}


Node *
Node::getNextNode(void ** ppv) const
{
    Node * pNode = (Node *)*ppv;
    Assert(this == pNode->_pParent);
    if (pNode == _pLast)
        pNode = null;
    else
        pNode = pNode->getSibling();
    if (pNode)
        *ppv = (void *)pNode;
    return pNode;
}


Node * 
Node::find(Name * pName, Element::NodeType nt, Document * pDoc)
{
    void * pv;
    Node * pNode = null;
    if (! isCollapsedText())
    {
        for (pNode = getFirstNode(&pv); pNode; pNode = getNextNode(&pv))
        {
            if ( ( pName == null || pName == pNode->getName()) && 
                 ( pNode->_nodetype == (unsigned)nt) )
                break;
        }
    }
    if (!pNode && ATTRIBUTE == nt && pDoc)
    {
        Assert(pDoc->getDTD());
        DTD * pDTD = pDoc->getDTD();
        if (pDTD)
        {
            pNode = pDTD->getDefaultAttrNode(getName(), pName);
        }
    }

    return pNode;
}


// if pPrefix is NULL, then node to find must have a NULL prefix ptr
// if pBaseName is NULL, then we match ANY basename
Node * 
Node::find(Atom * pBaseName, Atom * pPrefix, Element::NodeType nt, Document * pDoc)
{
    Assert( pBaseName || pPrefix);
    void * pv;
    Node * pNode = null;
    if (! isCollapsedText())
    {
        for (pNode = getFirstNode(&pv); pNode; pNode = getNextNode(&pv))
        {
            if ( pNode->getName() &&
                 pNode->_nodetype == (unsigned)nt &&
                 (!pBaseName || pBaseName == pNode->getName()->getName()) &&
                 pPrefix == pNode->_getNameDef()->getPrefix() )
                break;
        }
    }
    if (!pNode && ATTRIBUTE == nt && pDoc)
    {
        Assert(pDoc->getDTD());
        DTD * pDTD = pDoc->getDTD();
        if (pDTD)
        {
            pNode = pDTD->getDefaultAttrNode(getName(), pBaseName, pPrefix);
        }
    }

    return pNode;
}


Node *
Node::findByNodeName(const WCHAR * pwcName, Element::NodeType eNodeType, Document * pDoc)
{
    Node * pNode;
    // parse name into prefix and baseName
    ULONG nPos;

    if (!isValidName(pwcName, &nPos))
        Exception::throwE(E_INVALIDARG, 
                          MSG_E_BADNAMECHAR,
                          null);
    Atom * pPrefix = null;
    Atom * pName;
    if (nPos)
    {
        pPrefix = Atom::create(pwcName, nPos);
        pName = Atom::create(pwcName+nPos+1);
    }
    else
    {
        pName = Atom::create(pwcName);
    }

    pNode = find(pName, pPrefix, eNodeType, pDoc);

    return pNode;
}

Node *
Node::findByNameDef(NameDef * pNameDef, Element::NodeType eNodeType, Document * pDoc)
{
    Name * pName = pNameDef->getName();
    Node * pNode = find(pName, eNodeType, pDoc);
    if (!pNode && pNameDef->getPrefix())
    {
        // look for 'foo':'bar' w/o namespace
        pNode = find(pName->getName(), pNameDef->getPrefix(), eNodeType, pDoc);
    }

    return pNode;
}

Node * 
Node::findChild(int index)
{
    void * pTag;
    for (Node * pNode = getNodeFirstChild(&pTag); pNode; pNode = getNodeNextChild(&pTag), index--)
    {
        if (!index)
            return pNode;
    }
    return null;
}


// return the index of pNodeChild in my child list.
// if pNodeChild == null, return count of all children.
// if pName != null, only count children with that name.
long 
Node::getChildIndex(Node *pNodeChild, Name * pName, NodeType type)
{
    long index;
    void * pTag;

    if (isParent() && (pNodeChild == null || pNodeChild->getNodeParent() == this))
    {
        index = 0;
        for (Node * pNode = getNodeFirstChild(&pTag); pNode; pNode = getNodeNextChild(&pTag))
        {
            if (type == ANY || type == pNode->getNodeType())
            {
                if (!pName || pName == pNode->getName())
                {
                    if (pNode == pNodeChild)
                    {
                        return index;
                    }

                    ++ index;
                }
            }
        }

        // we get here only if pNodeChild is null (otherwise we should have
        // found pNodeChild among the children of its parent).  In this case,
        // we're supposed to find out how many children there
        // are with the given name.  index is now set to that number.
        Assert(!pNodeChild);
        return index;
    }

    return -1;
}


long 
Node::getIndex(bool fRelative) 
{ 
    Name * nm;
    NodeType type;

    if (fRelative)
    {
        nm = getName();
        type = getNodeType();
    }
    else
    {
        nm = null;
        type = ANY;
    }

    return _pParent ? _pParent->getChildIndex(this, nm, type) : NULL;
}


Node * 
Node::getNodeLastChild()
{
    if (isParent())
    {
        if (ENTITYREF == _nodetype && (!_pLast || _pLast == (Node*)REF_LOCKED))
        {
            void *pv;
            getFirstNode(&pv);
        }
        if (isCollapsedText())
            uncollapse();
        if (_pLast && !_pLast->_fAttribute)
            return _pLast;
    }
    return null;
}


Node * 
Node::getNextSibling()
{
    if (!_pParent || 
        this == _pParent->_pLast)
        return null;
    Node * pNode = getSibling();
    if (pNode->_fAttribute)
        return null;
    return pNode;
}


Node * 
Node::getPrevSibling()
{
    Node * pPrev = null;
    Node * pNode;
    void * pv;
    Node * pParent = _pParent;
    if (!pParent)
        return null;
    pNode = pParent->getFirstNodeNoExpand(&pv);
    while(pNode)
    {
        if (pNode == this)
        {
            if (pPrev && pPrev->_fAttribute)
                return null;
            else
                return pPrev;
        }
        pPrev = pNode;
        pNode = pParent->getNextNode(&pv);
    }
    return null;
}


Node * 
Node::getNodeFirstChild(void ** ppTag)
{
    Assert(ppTag);
    Node * pNode;
    for (pNode = getFirstNode(ppTag); pNode; pNode = getNextNode(ppTag))
        if (!pNode->_fAttribute)
            break;
    if (pNode)
        *ppTag = pNode;
    return pNode;
}


Node * 
Node::getNodeNextChild(void ** ppTag) const
{
    Assert(ppTag);

    Node * pNode = (Node *)*ppTag;
    if (pNode)
    {
        Assert(CHECKTYPEID(*pNode, Node));
        if (pNode->_pParent != this)
        {
            pNode = null;
        }
        else
        {
            pNode = getNextNode(ppTag);
        }
    }
    return pNode;
}


void
Node::setNodeValue(Object * pValue)
{
    // for now only attributes can do this
    Assert(_fAttribute);

    checkReadOnly();

    if (String::_getClass()->isInstance( pValue))
    {
        setInnerText(pValue->toString());
    }
    else
    {
        bool f = setValueType(VAL_OBJ);
        if (!f)
            Exception::throwE((HRESULT)E_UNEXPECTED);
        assign(&_pValue, pValue);
    }

    TraceTag((tagNode, "set value (%s) on node %X:%s:<%s>:[%s]",
        (char *)AsciiText(pValue->toString()),
        this, _type(this), (char *)AsciiText(_name(this)), (char *)AsciiText(_text(this))
        ));
}


Object *
Node::getNodeValue()
{
    // for now only attributes can do this
    Assert(_fAttribute);

    Object * pObj;
    AWCHAR* pText;

    if ( getValueType() == VAL_OBJ)
    {
        TraceTag((tagNode, "get value (%s) on node %X:%s:<%s>:[%s]",
            _pValue ? (char *)AsciiText(_pValue->toString()) : "",
            this, _type(this), (char *)AsciiText(_name(this)), (char *)AsciiText(_text(this))
            ));

        pObj = _pValue ? _pValue : String::emptyString();
    }
/*
    else if ( getValueType() == VAL_STR)
    {
        TraceTag((tagNode, "get (text) value (%s) on node %X:%s:<%s>:[%s]",
            _pValue ? (char *)AsciiText(_text(this)) : "",
            this, _type(this), (char *)AsciiText(_name(this)), (char *)AsciiText(_text(this))
            ));

        if (_pText)
            pObj = String::newString(_pText);
        else
            pObj = String::emptyString();
    }
    else if (null != (pText = getCollapsedText()))
    {
        pObj = String::newString(pText);
    }
*/
    else
    {
        String * ps = getInnerText(true, true, true);
        pObj = ps;

        TraceTag((tagNode, "get (derived) value (%s) on node %X:%s:<%s>:[%s]",
            ps ? (char *)AsciiText(ps) : "",
            this, _type(this), (char *)AsciiText(_name(this)), (char *)AsciiText(_text(this))
            ));

    }

    return pObj;
}

/*
 * Schema Information
 */
Node* Node::getDefinition()
{
    Name * pName = getName();
    Document * pDoc = getNodeDocument();
    DTD * pDTD = pDoc->getDTD();
    Node * pNodeDefinition = null;

    if (pName == null || pDTD == null)
        goto CleanUp;

    switch (getNodeType())
    {
    case Element::ENTITYREF:
        {
            Entity * pEnt = pDTD->findEntity( pName, false);
            if (pEnt != null)
            {
                pNodeDefinition = (Node*)(Object*)(pEnt->_pNode);
            }
        }
        break;
#if NEVER
// this capability is going to eventually be exposed through the datatype mechanism... eventually
            else if (pNode->getNodeAttributeType() == DT_AV_ENTITY)
            {
                // BUGBUG - this doesn't handle attributes of type
                // ENTITIES (plural).
                // BUGBUG - this is broken for qualified names.
                Object *value = pNode->getNodeValue();
                if (value != null)
                {
                    Name* name = Name::create(value->toString());
                    Entity * pEnt = pDTD->findEntity( name, false);
                    if (pEnt != null)
                    {
                        pRefNode = (Node*)(Object*)(pEnt->_pNode);
                    }
                }
            }
#endif
    case Element::ENTITY:
    {
        Name* ndata = XMLNames::name(NAME_NDATA);
        Object* attr = getNodeAttribute(ndata);
        if (attr != null)
        {
            Name* name = Name::create(attr->toString());
            Notation * pNotation = pDTD->findNotation(name);
            if (pNotation != null)
            {
                pNodeDefinition = (Node*)(Object*)(pNotation->_pNode);
            }
        }
        break;
    }
    case Element::ATTRIBUTE:
        {
            // Look up the AttDef.
            AttDef* pAttDef = getAttDef();
            if (pAttDef)
            {
                pNodeDefinition = pAttDef->getSchemaNode();
            }
        }
        break;

    case Element::ELEMENT:
        {
            ElementDecl* pElementDecl = getElementDecl();
            if (pElementDecl)
            {
                pNodeDefinition = pElementDecl->getSchemaNode();
            }
        }
        break;
    }
CleanUp:
    return pNodeDefinition;
}

AttDef* 
Node::getAttDef( ElementDecl * pED, DTD * pDTD, Node * pParent)
{
    AttDef * pAttDef = NULL;
    Name * pName = getName();
    Assert(pName);
    Assert(_fAttribute);

    // allow pParent attribute to override _pParent
    if (!pParent)
        pParent = _pParent;

    // look up ElementDecl if we need to (and can)
    if (!pED && pParent)
        pED = pParent->getElementDecl(pDTD);
    // look up AttDef on ElementDecl
    if (pED)
        pAttDef = pED->getAttDef(pName);

    // if we still haven't found it, try global attributes 
    //  if attribute have a prefix
    if (!pAttDef && _getNameDef()->getPrefix())
    {
        if (!pDTD)
            pDTD = _pDocument->getDTD();
        pAttDef = pDTD->getGAttributeType(pName);
    }

    return pAttDef;
}

ElementDecl* 
Node::getElementDecl(DTD * pDTD)
{
    // this only works for elements
    if (Element::ELEMENT != getNodeType())
        return null;

    // allow DTD param to override
    if (!pDTD)
        pDTD = getNodeDocument()->getDTD();

    ElementDecl* pElementDecl = null;
    if (pDTD)
    {
        Name* pName = getName();
        if (pName)
        {
            pElementDecl = pDTD->findElementDecl(pName);
        }
    }
    return pElementDecl;


#ifdef NEVER
/*************************
/* Well it would be nice to also be able to navigate through the definition method
/* from <element type="..."> or <attribute type="..."/> nodes to the associated
/* <ElementType> and <AttributeType> and the following almost works.  The problem is
/* that the Schema Documents CANNOT currently point to the main DTD because of 
/* circular reference problems, and therefore even if we do find the right pName
/* to lookup the pDTD->findElementDecl will fail because we have the wrong DTD object.

        Atom* pNS = pName->getNameSpace();
        if (pNS == SchemaNames::atomSCHEMA || pNS == SchemaNames::atomSCHEMAAlias)
        {
            // we are inside the schema, so now we have to find the
            // definition for an <element type="..."> or <attribute type="..."/>
            Atom* pNameGI = pName->getName();
            pName = null;
            if (pNameGI == SchemaNames::name(SCHEMA_ELEMENT)->getName())
            {
                Object* pValue = getAttribute(SchemaNames::name(SCHEMA_TYPE));
                if (pValue)
                {
                    pName = qualifyName(pValue->toString());
                }
            }
            else if (pNameGI == SchemaNames::name(SCHEMA_ELEMENTTYPE)->getName())
            {
                Object* pValue = getAttribute(SchemaNames::name(SCHEMA_NAME));
                if (pValue)
                {
                    pName = qualifyName(pValue->toString());
                }
            }
            else (pNameGI == SchemaNames::name(SCHEMA_ATTRIBUTE)->getName())
            {
                // We are on an <attribute type="..."> so get the ElementDecl for the
                // parent <ElementType> then lookup the attdef.
                pElementDecl = getNodeParent()->getElementDecl();
                if (pElementDecl)
                {
                    Object* pValue = getAttribute(SchemaNames::name(SCHEMA_TYPE));
                    if (pValue)
                    {
                        pName = qualifyName(pValue->toString());
                        AttDef* pAttDef = pElementDecl->getAttDef(pName);
                        break;
                    }
                }
            }
        }
*************************/
#endif
}

Node* 
Node::selectSingleNode(String* query)
{
    XQLParser* parser = XQLParser::newXQLParser(); // GC'd object.
    Query* pQuery = parser->parse(query);
    pQuery->setContext(null, this);
    Node* result = (Node *) pQuery->nextElement();
    return result;
}


/**
 * Helpers to clear, copy in/out of a variant.
 */

void
Node::clearVariantData()
{
    Assert(_fVariant);
    if (_vt != VT_NULL)
    {
        VARIANT var;
        toVariant(&var);
        VariantClear(&var);
        _vt = VT_NULL;
    }
}


void 
Node::toVariant(VARIANT * pVar)
{
    Assert(_fVariant);
    pVar->vt = _vt;
    Assert(sizeof(_variantData) == sizeof(__int64));
    Assert(sizeof(double) == sizeof(__int64));
    *(__int64 *)&(V_R8(pVar)) = *(__int64 *)&_variantData;
}


void 
Node::fromVariant(VARIANT * pVar)
{
    Assert(_fVariant);
    if (_vt != VT_NULL)
        clearVariantData();
    // NUMBERS we just parse .. we dont store it as a seperate node...
    if (_datatype != DT_STRING && _datatype != DT_URI && _datatype != DT_UUID && _datatype != DT_NUMBER)
    {
        _vt = pVar->vt;
        Assert(sizeof(_variantData) == sizeof(__int64));
        Assert(sizeof(double) == sizeof(__int64));
        *(__int64 *)&_variantData = *(__int64 *)&(V_R8(pVar));
    }
    else
    {
        VariantClear( pVar);
    }
}


void
Node::createContent(VARIANT * pVar)
{
    HRESULT hr = S_OK;
    Node * pNode = null;
    String * pString = null;
    AWCHAR * pS;

    if (!_fTyped)
    {
        Assert( 0 && "Should never happen!");
        return;
    }

    hr = UnparseDatatype( &pString, pVar, (DataType)_datatype);
    if (hr)
        Exception::throwE(hr);

    long length = pString->length();
    ATCHAR * pachars = pString->toCharArrayZ();
    pachars->AddRef();
    pS = new (length) AWCHAR;
    pS->simpleCopy(0, length, (WCHAR *)pachars->getData()); 
    pachars->Release();

    deleteChildren(true);

    pNode = Node::newNodeFast(PCDATA, null, this, pS, null, 0,
                              _pDocument,
                              _pDocument->getNodeMgr());
}


/**
 * Helper to return locale to be used for this node.
 */
LCID
Node::getLocale()
{
    // BUGBUG should look at xml:lang !
    return GetSystemDefaultLCID();
}


/**
 * Add typed value node by using the _pName/_pTypedValue union.
 * The name is stored in the new typed value node.
 */
void
Node::setNodeDataType(DataType dt, VARIANT * pVar)
{
    _datatype = dt;
    if (DT__NON_AV > dt)
    {
        if (_fTyped)
        {
            // can't use assign() since _pName overlaps with _pTypedValue
            Node * pTypedNode = _pTypedValue;
            _pName = _pTypedValue->_pName;
            _pName->AddRef();

            pTypedNode->Release();
            _fTyped = 0;
        }
        _fDT = (DT_NONE != dt);
    }
    else
    {
        NameDef * pNameDef = _getNameDef();
        TRY
        {
            if (!_fTyped)
                assign(&_pName, null);
            if (!_pTypedValue)
            {
                // Note: need to keep this ptr on the stack until we AddRef it...
                Node * pDTTypeDataNode = Node::newNode(TYPEDVALUE, pNameDef, _pDocument, _pDocument->getNodeMgr());
                pDTTypeDataNode->AddRef();
                pDTTypeDataNode->_vt = VT_NULL;
                _pTypedValue = pDTTypeDataNode;
            }
            // Note: fromVariant will clear old variant data
        }
        CATCH
        {
            if (!_fTyped)
                _pName = pNameDef;

            Exception::throwAgain();
        }
        ENDTRY

        _fTyped = 1;
        _fDT = 1;
        _pTypedValue->fromVariant( pVar);
    }
}


void
Node::removeDataType()
{
    if (_fDT)
        parseAndSetTypedData(DT_NONE, false);
    _fDT = 0;
    _datatype = DT_NONE;
}

void
ThrowBadDatatypeError(String * pContent, DataType dt)
{
    // BUGBUG: there must be a way to remember the error...
    Assert(pContent);
    const TCHAR * pwc = LookupDataTypeName(dt);
    String * ps;
    if (pwc)
        ps = String::newString(pwc);
    else
        ps = String::newString(_T("[USER DEFINED]"));

    Exception::throwE(E_FAIL, XMLOM_DATATYPE_PARSE_ERROR,
                                               pContent,
                                               ps, 
                                               null); 
}

Node * FindDTAttribute(Node * pElem, Document * pDoc)
{
    Name * pName;
    Node * pDTAttr;
    pName = XMLNames::name(NAME_DTDT);
    pDTAttr = pElem->find(pName, Element::ATTRIBUTE, pDoc);
    return pDTAttr;
}

void
Node::testDataType()
{
    if (_fTyped && _pTypedValue->_vt == VT_ERROR)
    {
        if (DT_NONE != _datatype)
        {
            // try and reparse it, expecting to throw an error
            VARIANT v;
            v.vt = VT_NULL;
            parseTypedContent(getNodeDocument(), null, getNodeDataType(),  &v, false);
            Assert(0 && "Expected parseTypedContent to throw error!");
            // in case it actually worked (this should not happen)
            VariantClear(&v);
        }
        else
        {
            Node * pDTAtt = FindDTAttribute(this, getNodeDocument());
            String * pDTName;
            if (pDTAtt)
                pDTName = pDTAtt->getInnerText(true,true,true);
            else // try and find it in the dtd.
            {
                pDTName = NULL;
                if (_fAttribute)
                {
                    AttDef* pAttDef = getAttDef();
                    if (pAttDef)
                        pDTName = pAttDef->getTypeName();
                }
                else
                {
                    Assert(_nodetype == Element::ELEMENT);
                    ElementDecl * pElemDecl = getElementDecl();
                    if (pElemDecl)
                        pDTName = pElemDecl->getDataTypeName();
                }
            }

            if (!pDTName)
                pDTName = String::newString(L"?? Unknown Datatype ??");

            Exception::throwE(E_FAIL, XMLOM_INVALID_DATATYPE, pDTName, NULL);
        }
    }
}

void 
Node::parseTypedContent(Document * pDoc, String * pString, DataType dt, VARIANT * pVar, bool fParsing)
{
    if (!isEmpty() || pString)
    {
        HRESULT hr;
        String * pText;
        const ATCHAR * pachars;
        bool fThrowErrors = !fParsing || pDoc->getValidateOnParse();

        TRY
        {
            pText = pString ? pString : _dtText(null);

            // make sure the chars don't go away when we get the internal data...
            pText->AddRef();
            hr = ParseDatatype(pText->getWCHARPtr(), pText->length(), dt, pVar, null);
            pText->Release();
        }
        CATCH
        {
            if (fThrowErrors)
                Exception::throwAgain();
            goto MarkAsError;
        }
        ENDTRY

        if (hr)
        {
            if (fThrowErrors)
            {
                ThrowBadDatatypeError(pText, dt);
            }
            else
            {
MarkAsError:
                pVar->vt = VT_ERROR;
                V_BSTR(pVar) = NULL;
            }
        }
    }
}


void
Node::getNodeTypedValue(VARIANT * pVar)
{
    HRESULT hr;

    // special case string value which is stored natively (or UUID, which we just return as a string)
    if (_fTyped && !(_datatype == DT_STRING || _datatype == DT_URI || _datatype == DT_UUID || _datatype == DT_NUMBER)
        && _pTypedValue->_vt != VT_ERROR)
    {
        VARIANT var;
        _pTypedValue->toVariant(&var);
        hr = VariantCopy(pVar, &var);
        if (hr)
            Exception::throwE( hr);
    }
    else
    {
        // note this must be the same as DOMNode::get_nodeValue
        BSTR bstr = getDOMNodeValue()->getBSTR();
        pVar->vt = VT_BSTR;
        V_BSTR(pVar) = bstr;
        hr = S_OK;
    }
}


void
Node::getNodeTypedValue(DataType dt, VARIANT * pVar)
{
    HRESULT hr;
    VARTYPE vt;

    getNodeTypedValue(pVar);
    vt = VariantTypeOfDataType(dt);
    if (vt != V_VT(pVar))
    {
        VARIANT var;
        ::VariantInit(&var);
        ::VariantCopy(&var,pVar);
        ::VariantClear(pVar);
        hr = ::VariantChangeTypeEx(pVar, &var, GetSystemDefaultLCID(), VARIANT_NOVALUEPROP, vt);
        // Don't throw an exception is an error occurs.  Just return a variant with type VT_ERROR.
        // BUGBUG - Does VariantChangeTypeEx set the VT to VT_ERROR?
        ::VariantClear(&var);
    }
}


void 
Node::setNodeTypedValue(VARIANT * pVar)
{
    HRESULT hr = S_OK;
    VARIANT varNew;
    varNew.vt = VT_NULL;
    
    checkReadOnly();

    bool fAttached = (getNodeParent() && getNodeParent()->getNodeParent());
    Node * pNodeParent = getNodeParent();

    // notify listener that we're about to change a value
    if (fAttached)
        _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_AboutToDo,
                                pNodeParent, pNodeParent->getNodeParent());

    TRY
    {
        if ( pVar->vt == VT_BSTR)
        {
            setInnerText(V_BSTR(pVar));
        }
        else if ( pVar->vt == VT_NULL)
        { // special case: null out the type info
            setInnerText(String::emptyString());
            removeDataType();
        }
        else // try to convert
        {
            ConvertVariant2DataType((DataType)_datatype, pVar, &varNew);
            if (_fDT && _datatype >= DT__NON_AV)
            {
                setNodeDataType((DataType)_datatype, &varNew);

                // try to set tree structure
                createContent(&varNew);

                // setNodeDataType() takes ownership of variant data,
                // we need to mark this as VT_NULL, so that VariantClear will do nothing
                varNew.vt = VT_NULL;
            }
            else
            {
                Assert(VT_BSTR == varNew.vt);
                setInnerText(V_BSTR(pVar));
            }
        }

        // notify listener that we changed a value
        if (fAttached)
            _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_DidEvent,
                                    pNodeParent, pNodeParent->getNodeParent());
    }
    CATCH
    {
        // notify listener that we failed to change a value
        if (fAttached)
            _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_FailedToDo,
                                    pNodeParent, pNodeParent->getNodeParent());

        VariantClear(&varNew);
        Exception::throwAgain();
    }
    ENDTRY

Cleanup:
    VariantClear(&varNew);
    if (hr)
        Exception::throwE(hr);
}


void
Node::setNodeAttribute(Name * pName, const BSTR bstrName, Object * pValue, Atom * pPrefix, Node ** ppNode)
{
    Assert(pName || bstrName);

    Document * pDoc = getNodeDocument();
    checkReadOnly();

    if ( !isParent() || !allowAttributes())
    {
        Assert(0 && "Node::setAttribute called on a node which does not allow Attributes!");
        Exception::throwE((HRESULT)E_UNEXPECTED);
    }

    if (isParent())
    {
        NameDef * pNameDef;
        Node * pNode;
        if (pName)
        {
            pNode = find(pName, ATTRIBUTE);
            if (!pNode)
                pNameDef = pDoc->getNamespaceMgr()->createNameDef(pName, null, pPrefix);
        }
        else
        {
            pNode = findByNodeName(bstrName, Node::ATTRIBUTE, pDoc);
            if (!pNode)
            {
                pNameDef = pDoc->getNamespaceMgr()->createNameDefOM(bstrName);
                pName = pNameDef->getName();
            }
        }

        bool fNewNode = (pNode == NULL);
        
        if (fNewNode)
        {
            if (pNameDef->getPrefix() == XMLNames::atomXMLNS &&
                pNameDef->getName()->getName() == XMLNames::atomXML)
            {
                Exception::throwE(E_INVALIDARG, 
                                  XML_XMLNS_RESERVED,
                                  pNameDef->getName()->getName()->toString(),
                                  null);
            }
            pNode = Node::newNode(ATTRIBUTE, pNameDef,
                                  pDoc,
                                  pDoc->getNodeMgr());
        }
        else
        {
            if (!pNode->isSpecified())
            {
                pNode = pNode->clone( true, false, pDoc, pDoc->getNodeMgr());
                pNode->setSpecified(true);
                fNewNode = true;
            }
            else
            {
                pNode->checkReadOnly();
            }
        }

        // notify listener we are about to change an attribute
        _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_AboutToDo,
                                pNode, this);

        TRY
        {
            pNode->setNodeValue(pValue);

            if (fNewNode)
            {
                insertAttr(pNode);
            }

            if ( ppNode != null)
                *ppNode = pNode;

            // notify listener we changed an attribute
            _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_DidEvent,
                                    pNode, this);
        }
        CATCH
        {
            // notify listener we failed to change an attribute
            _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_FailedToDo,
                                    pNode, this);
            Exception::throwAgain();
        }
        ENDTRY
    }
}


Object * 
Node::getNodeAttribute(Name * pName)
{
    Assert(pName);
    Assert(isParent());

    Object * pValue;

    Node * pNode = find(pName, ATTRIBUTE, getNodeDocument());

    if (pNode)
    {
        pValue = pNode->getNodeValue();
    }
    else
    {
        pValue = null;
    }

    TraceTag((tagNode, "get attribute (%s) on node %X:%s:<%s>:[%s]",
        (char *)AsciiText(pName->toString()),
        (char *)AsciiText(pValue ? pValue->toString() : String::nullString()),
        this, _type(this), (char *)AsciiText(_name(this)), (char *)AsciiText(_text(this))
        ));

    return pValue;
}


void 
Node::removeAttribute(Name * pName, Node ** ppNode)
{
    Assert(pName);
    Assert(isParent());
    Document * pDoc = getNodeDocument();

    Node * pNode = find(pName, ATTRIBUTE);
    if (ppNode)
        *ppNode = pNode;

    if (pNode)
    {
        removeNode(pNode, true);
    }
    else
    {
        Exception::throwE(E_FAIL, XMLOM_NODE_NOTFOUND, null);
    }
}


void
Node::setNodeAttributeType(DataType attributetype)
{
    // This should only be used by the validationfactory (and only for right now)
    Assert(_nodetype == ATTRIBUTE);
    Assert(attributetype >= DT_NONE && attributetype < DT_USER_DEFINED);
    _datatype = attributetype;
}


Node * 
Node::getNodeFirstAttribute(void ** ppTag)
{
    Assert(ppTag);

    Node * pNode = getFirstNodeNoExpand(ppTag);
    if (pNode && !pNode->_fAttribute)
        pNode = null;
    return pNode;
}

    
Node * 
Node::getNodeNextAttribute(void ** ppTag)
{
    Assert(ppTag);
    Node * pNode = (Node *)*ppTag;
    Assert(pNode && "Shouldn't be called if getFirstChild returned null!");
    Assert(CHECKTYPEID(*pNode, Node));
    pNode = getNextNode(ppTag);
    if (pNode && !pNode->_fAttribute)
        pNode = null;
    return pNode;
}

Node * 
Node::getNodeFirstAttributeWithDefault(void ** ppTag)
{
    Node * pNode = getNodeFirstAttribute(ppTag);
    if (!pNode && allowAttributes())
        pNode = getFirstDefaultAttribute(ppTag);
    return pNode;
}


Node *
Node::getFirstDefaultAttribute(void ** ppTag)
{
    Node * pNode;
    NameDef * namedef = _getNameDef();
    Assert(namedef);

    Document * pDoc = getNodeDocument();
    Assert(pDoc);
    Assert(pDoc->getDTD());

    if (pDoc->getIgnoreDTD())
        return null;

    Node * pDefAttrNode = pDoc->getDTD()->getDefaultAttributes(namedef->getName());
    if (pDefAttrNode)
    {
        pNode = pDefAttrNode->getNodeFirstAttribute(ppTag);
        do
        {
            if (pNode && !find(pNode->getName(), Node::ATTRIBUTE, null))
            {
                // we use this trick to mark that the default attribute is used 
                *ppTag = (void *)((DWORD_PTR)*ppTag | 1);
                goto Cleanup;
            }
            pNode = pDefAttrNode->getNodeNextAttribute(ppTag);
        }
        while (pNode);
    }

    pNode = null;
    *ppTag = null;

Cleanup:
    return pNode;
}


Node * 
Node::getNodeNextAttributeWithDefault(void ** ppTag)
{
    Node * pNode = null;

    bool fDefault = ((DWORD_PTR)*ppTag & 1) != 0; 

    if (!fDefault)
        pNode = getNodeNextAttribute(ppTag);

    if (!pNode)
    {
        if (fDefault)
        {
            Node * pv = (Node*)((DWORD_PTR)*ppTag ^ 1);
            Node * pAttrNode = pv->_pParent;
            do
            {
                pNode = pAttrNode->getNodeNextAttribute((void **)&pv);
                if (pNode && !find(pNode->getName(), Node::ATTRIBUTE, null))
                {
                    *ppTag = (void *)((DWORD_PTR)pv | 1);
                    goto Cleanup;
                }
            }
            while (pNode);
            *ppTag = null;
        }
        else
        {
            pNode = getFirstDefaultAttribute(ppTag); 
        }
    }

Cleanup:
    return pNode;
}


Node * 
Node::resolveEntityRef()
{
    Name * pName;
    Entity * pEnt;
    DTD * pDTD;
    Node * pNode = null;

    if (getNodeType() != ENTITYREF)
    {
        goto Cleanup;
    }

    pName = getName();
    pDTD = _pDocument->getDTD();

    if (!pName || !pDTD || !(pEnt = pDTD->findEntity( pName, false)))
    {
        goto Cleanup;
    }

    pNode = (Node*)((Object*)(pEnt->_pNode));

Cleanup:
    return pNode;
}


/**
 * This function should add only text to the StringBuffer for
 * ELEMENT, PCDATA, CDATA, ENTITYREF
 */
String * 
Node::_dtText(WSStringBuffer * psb)
{
    return _dtText(null, 0 , null, null, null, psb);
}

String * 
Node::_dtText(const TCHAR * text, int len, Node * pNewNode, Node * pInsertBefore, Node * pSkip, WSStringBuffer * psb_)
{
    // BUGBUG: tune the size of the buffer
    WSStringBuffer * psb = psb_ ? psb_ : WSStringBuffer::newWSStringBuffer(32);

    if (isParent())
    {
        void * pTag;
        Node * pNode = getFirstNodeNoExpand(&pTag);
        while (1)
        {
            // is this where we insert the alternate/new text?
            if (pNode == pInsertBefore)
            {
                if (pNewNode)
                {
                    if (pNewNode->getNodeType() == DOCFRAG)
                        pNewNode->_dtText(psb);
                    else
                        pNode = pNewNode;
                }
                else if (len)
                {
                    psb->append(text, len, WSStringBuffer::WS_TRIM);
                }
            }

Loop:
            if (!pNode)
            {
                AWCHAR* pText = getCollapsedText();
                if (pText)
                {
                    psb->append(pText, WSStringBuffer::WS_TRIM);
                }
                break;
            }

            // is this the node to skip?
            if (pNode == pSkip)
                goto Continue;

            // do somethign appropriate
            switch (pNode->_nodetype)
            {
            case ATTRIBUTE:
                goto Continue;

            case ENTITYREF:
                if ( _pDocument)
                {
                    Node * pEntity = pNode->resolveEntityRef();
                    if ( pEntity)
                        pEntity->_dtText(psb);
                }
                break;

            case ELEMENT:
                // throw error
                Exception::throwE(E_FAIL, SCHEMA_ELEMENT_NOSUPPORT, pNode->_getNameDef()->toString(), null);
                break;

            case CDATA:
                if (pNode->_pText)
                    psb->append(pNode->_pText, WSStringBuffer::WS_PRESERVE); // always preserve whitespace in CDATA
                break;

            case PCDATA:
                if (pNode->_pText)
                    psb->append(pNode->_pText, WSStringBuffer::WS_TRIM);
                break;

            default:
                break;
            }
            if (pNode->getWSFollow())
                psb->append(L' ', WSStringBuffer::WS_COLLAPSE);

            if (pNode == pNewNode)
            {
                pNode = pInsertBefore;
                goto Loop;
            }
Continue:
            pNode = getNodeNextChild(&pTag);
        }
    }
    else
    {
        if (this->_pText)
        {
            Assert(getValueType() == VAL_STR);
            psb->append(this->_pText, WSStringBuffer::WS_TRIM);
        }
    }

    return psb_ ? null : psb->toString();
}

/**
 * This function should add only text to the StringBuffer for
 * ELEMENT, PCDATA, CDATA, ENTITYREF
 */
void 
Node::addText(WSStringBuffer * psb, bool fPreserve, bool fIgnoreXmlSpace, bool fIE4)
{
    if (getNodeType() == ELEMENT)
    {
        if (!fIgnoreXmlSpace)
            getXmlSpace( &fPreserve);

        if (getWSInner())
            goto AddInnerWSText;
    }
    else if (getNodeType() == ENTITY && getWSInner())
    {
AddInnerWSText:
        psb->append( L' ', WSStringBuffer::WS_COLLAPSE);
    }

    WSStringBuffer::WSMODE eWSMode = fPreserve ? WSStringBuffer::WS_PRESERVE : (fIE4 ? WSStringBuffer::WS_COLLAPSE : WSStringBuffer::WS_TRIM);

    if (isParent())
    {
        AWCHAR* pText = getCollapsedText();
        if (pText)
        {
            psb->append(pText, eWSMode);
        }
        else
        {
            void * pTag;
            for (Node * pNode = getFirstNodeNoExpand(&pTag); pNode; pNode = getNodeNextChild(&pTag))
            {
                switch (pNode->_nodetype)
                {
                case ATTRIBUTE:
                    continue;

                case ENTITYREF:
                    if ( _pDocument)
                    {
                        Node * pEntity = pNode->resolveEntityRef();
                        if ( pEntity)
                            pEntity->addText( psb, fPreserve, fIgnoreXmlSpace, fIE4);
                    }
                    break;

                case ELEMENT:
                    if ( pNode->isParent())
                        pNode->addText(psb, fPreserve, fIgnoreXmlSpace, fIE4);
                    else
                        psb->append( pNode->getNodeText(), eWSMode);
                    break;

                case CDATA:
                    if (pNode->_pText)
                    {
                        psb->append(pNode->_pText, WSStringBuffer::WS_PRESERVE); // always preserve whitespace in CDATA
                    }
                    break;

                case PCDATA:
                    Assert( pNode->getValueType() == VAL_STR);
                    if (pNode->_pText)
                    {
                        psb->append(pNode->_pText, eWSMode);
                    }
                    break;

                default:
                    break;
                }
                if (pNode->getWSFollow())
                    psb->append( L' ', WSStringBuffer::WS_COLLAPSE);
            }
        }
    }
}


bool
Node::getXmlSpace( bool * pfPreserve)
{
    Node * pNode = null;
    Document * pDoc = getNodeDocument();
    if (pDoc && ! pDoc->isDOM())
        pNode = find( XMLNames::name(NAME_XMLSpace), Node::ATTRIBUTE);
    else
        pNode = find( XMLNames::name(NAME_XMLSpace2), Node::ATTRIBUTE, getNodeDocument());

    if (pNode)
    {
        *pfPreserve = false;

        String * pS = pNode->getInnerText(false, true, false);
        if (pS)
        {
            if (pDoc->isDOM()) // case sensitive
            {
                if (pS->equals( const_cast<TCHAR *>(XMLNames::pszPreserve)))
                    *pfPreserve = true;
            }
            else
            {
                if (pS->equalsIgnoreCase( const_cast<TCHAR *>(XMLNames::pszPreserve)))
                    *pfPreserve = true;
            }
        }
        return true;
    }
    return false;
}   

bool 
Node::ignoreXMLSpace() const
{
    bool fIgnore = false;
    Document* pDoc = (const_cast<Node* const>(this))->getNodeDocument();
    if (pDoc->getPreserveWhiteSpace())
    {
        fIgnore = true;
    }
    return fIgnore;
}


/**
 * Starting at this node look for an xml:space attribute, 
 * walking up the parent chain until either the root, or
 * a specified xml:space attriubute is found
 */
bool
Node::xmlSpacePreserve() const
{
    bool fPreserve = false;
    Node * pNode = const_cast<Node *>(this);

    if (ignoreXMLSpace())
    {
        fPreserve = true;
        goto Done;
    }

    while (pNode)
    {
        NodeType type = pNode->getNodeType();
        switch(type)
        {
        case Element::ELEMENT:
            if (pNode->getXmlSpace(&fPreserve))
                goto Done;
            break;

        case Element::CDATA:
        case Element::PI:
        case Element::COMMENT:
            fPreserve = true;
            goto Done;

        case Element::ATTRIBUTE:
            // attributes do NOT inherit xml:space
            goto Done;
        }

        pNode = pNode->_pParent;
    }

Done:
    return fPreserve;
}


String * 
Node::getInnerText( bool fPreserve, bool fIgnoreXmlSpace, bool fNormalize)
{
    String * pText;
    NodeType eNodeType = getNodeType();

    if (DOCTYPE == eNodeType)
        return String::emptyString();
    else if (DOCUMENT == eNodeType)
    {
        Node * pDocElem = find(null, ELEMENT, null);
        return (null == pDocElem) ? String::emptyString() : pDocElem->getInnerText(fPreserve, fIgnoreXmlSpace, fNormalize);
    }
    else if (XMLDECL == eNodeType)
    {
        pText = getAttributesXML()->trim();
        return pText;
    }

    if (!fIgnoreXmlSpace && eNodeType == ELEMENT)
        getXmlSpace( &fPreserve);

    // BUGBUG: we should do some profiling to figure out a good value for
    //         the start size of this buffer...
    WSStringBuffer * psb = WSStringBuffer::newWSStringBuffer(32);
    bool fIE4 = !getNodeDocument()->isDOM();

    if ( eNodeType == ENTITYREF)
    {
        Node * pEntity = resolveEntityRef();
        if (pEntity)
            pEntity->addText( psb, fPreserve, fIgnoreXmlSpace, fIE4);
    }
    else if (isParent())
        addText(psb, fPreserve, fIgnoreXmlSpace, fIE4);
    else
        psb->append( getNodeText(), fPreserve ? WSStringBuffer::WS_PRESERVE : (fIE4 ? WSStringBuffer::WS_COLLAPSE : WSStringBuffer::WS_TRIM));

    if (fNormalize)
    {
        psb->normalize(fIE4);
    }
    pText = psb->toString();

    if (pText)
        return pText;
    return String::emptyString();
}


Name * 
Node::getContentAsName(bool fValidate, String * pContent)
{
    String * pIDStr = pContent ? pContent : getInnerText(false, true, false);
    if (!pIDStr->isWhitespace())
    {
        if (fValidate)
            return (Name *)_pDocument->getNamespaceMgr()->parseNames(DT_AV_ID, pIDStr);
        else
            return Name::create(pIDStr);
    }
    return null;
}


void
Node::setInnerText(const WCHAR * pch, int len, bool fNoChild)
{
    AWCHAR * pS;
    if (pch)
    {
        pS = new (len) AWCHAR;
        pS->simpleCopy(0, len, pch);
    }
    else
        pS = null;
    setInnerText(pS, fNoChild);
}

void
Node::setInnerText(String * pS, bool fNoChild)
{
    pS->AddRef();
    TRY
    {
        setInnerText(pS->toCharArray(), fNoChild);
    }
    CATCH
    {
        pS->Release();
        Exception::throwAgain();
    }
    ENDTRY
    pS->Release();
}


const WCHAR * s_pwcScanTargets[] =
{
    null, // ELEMENT
    null, // PCDATA
    L"--", // COMMENT
    null, // DOCUMENT
    null, // DTD
    L"?>", // PI
    L"]]>", // CDATA
    null, // ENTITY
    null, // NOTATION
    null, // ELEMENTDECL
    null, // NAMESPACE
    null, // ENTITYREF
    null, // WHITESPACE
    null, // INCLUDESECTION
    null, // IGNORESECTION
    null, // ATTRIBUTE
    null, // TYPEDVALUE
    null, // DOCFRAG
    null, // XMLDECL
};

// this will replace whatever our old content was with the new string
void
Node::setInnerText(const AWCHAR * pS, bool fNoChild)
{
    checkReadOnly();

    if (pS)
    {
        const WCHAR * scanTarget = s_pwcScanTargets[(int)getNodeType()];
        if (scanTarget && pS)
        {
            int len = pS->length();
            int lenTarget = _tcslen(scanTarget);
            const WCHAR * pwc = pS->getData();
            const WCHAR * match = scanTarget;
            WCHAR wc;

            if (getNodeType() == COMMENT && pwc[len-1] == '-')
                goto ErrorBadValue;

            while ((wc = *pwc))
            {
                if (wc == *match && len >= lenTarget &&
                    !memcmp(pwc, scanTarget, sizeof(WCHAR)*lenTarget))
                    goto ErrorBadValue;
                len--;
                pwc++;
            }
            if (getNodeType() == COMMENT && 
                pS->length() && 
                pS->item(pS->length()-1) == '-')
            {
    ErrorBadValue:
                Exception::throwE(E_INVALIDARG, 
                                  XMLOM_BADVALUE, 
                                  String::newString(s_achTypes[getNodeType()]), 
                                  null);
            }
        }
    }

    // notify the listener that we're about to change text
    Node *pNodeChange = getAncestorWithNodeType(PCDATA, /* fNot */ TRUE);
    Node *pNodeParent = pNodeChange ? pNodeChange->getNodeParent() : null;

    if (pNodeChange)
    {
        _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_AboutToDo,
                                pNodeChange, pNodeParent);
    }
    
    TRY
    {
        if (setValueType( VAL_STR))
        {
            notifyChangeContent(pS ? String::newString(const_cast<AWCHAR *>(pS)) : String::emptyString());

            assign(&_pText, const_cast<AWCHAR *>(pS));
        }
        else if (!fNoChild && setValueType(VAL_PARENT))
        {
            notifyChangeContent(pS ? String::newString(const_cast<AWCHAR *>(pS)) : String::emptyString());

            // only send notification if we aren't sending TextChanged notification
            deleteChildren((NULL==pNodeChange)?true:false, false);

            // create new PCDATA node
            Node * pNewNode = Node::newNodeFast(Node::PCDATA, null, this, const_cast<AWCHAR *>(pS), null, 0,
                                                _pDocument,
                                                _pDocument->getNodeMgr());
        }
        else if (!fNoChild)
        {
            Assert( 0 && "setInnerText on called on invalid NodeType!");
            Exception::throwE((HRESULT)E_UNEXPECTED);
        }

        // notify the listener that we changed the text
        if (pNodeChange)
        {
            _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_DidEvent,
                                    pNodeChange, pNodeParent);
        }
    }
    CATCH
    {
        // notify the listener that we failed to change the text
        if (pNodeChange)
        {
            _pDocument->NotifyListener(XML_REASON_TextChanged, XML_PHASE_FailedToDo,
                                    pNodeChange, pNodeParent);
        }

        Exception::throwAgain();
    }
    ENDTRY

    return;
}


const AWCHAR * 
Node::getNodeText()
{
    AWCHAR * pTemp;
    const AWCHAR * pText;

    switch(getValueType())
    {
    case VAL_OBJ:
        pText = _pValue ? _pValue->toString()->getCharArray() : null;
        break;
    case VAL_STR:
        pText = _pText;
        break;
    default:
        pText = getCollapsedText();
        break;
    }

    return pText;
}

String * 
Node::getTextString()
{
    String * pText = null;
    switch(getValueType())
    {
    case VAL_OBJ:
        pText = _pValue ? _pValue->toString() : null;
        break;
    case VAL_STR:
        pText = _pText ? String::newString(_pText) : null;
        break;
    default:
        {
            AWCHAR* pCollapsedText = getCollapsedText();
            if (pCollapsedText)
                pText = String::newString(pCollapsedText);
        }
    }

    if (!pText)
        pText = String::emptyString();
    return pText;
}


void
Node::_appendText(const WCHAR * pText, int length)
{
    Assert((getValueType() == VAL_STR) || !_pText);
    if (!_pText)
        _valuetype = VAL_STR;

    const AWCHAR * oldText = _pText;
    int oldLen = oldText ? oldText->length() : 0;
    AWCHAR * newText = new(oldLen + length) AWCHAR;
    if (oldLen)
        newText->simpleCopy(0, oldLen, oldText->getData());
    newText->simpleCopy(oldLen, length, pText);
    assign(&_pText, const_cast<AWCHAR *>(newText));
}


void Node::uncollapse()
{
    AWCHAR* pText = getCollapsedText();
    if (pText)
    {
        NodeManager * pNodeMgr = getNodeDocument()->getAltNodeMgr();
        pNodeMgr->Lock();
        TRY
        {
            // Now that we have the node manager exclusive lock, we
            // can see if we still need to uncollapse - another reader
            // may have already done this.
            if (isCollapsedText())
            {
                _pText = null; // stop newNodeFast recurrsing back into uncollapse.
                // create new node - but no parent yet (on purpose).
                Node* pNode = Node::newNodeFast(Node::PCDATA, null, this,
                                  pText, null, 0, _pDocument, pNodeMgr);
                pNode->setFinished(true);
                pNode->setReadOnly(isReadOnly(),true);
                pText->Release();
            }
/************ if we had a multithread safe nodeManager we could do the following 
              to avoid contention between multiple readers ************************
            // create new node - but no parent yet (on purpose).
            Node* pNode = Node::newNodeFast(Node::PCDATA, null, null,
                              pText, null, 0, _pDocument, pNodeMgr);
            pNode->setFinished(true);
            // Now hook it up (in a multi-reader thread safe manner) so that
            // if another thread is doing an uncollapse at exactly the same time
            // we don't clobber the other thread.
            if (pCollapsedText == InterlockedCompareExchange((void**)&_pLast,pNode,pCollapsedText))
            {
                // we won !
                _append(pNode, &_pLast);
                pText->Release();
            }
*************************/
            else
            {
                // another thread won, so we need to throw away the node we just created.
                // The GC will clean it up.
                pText = null;  // this allows us to put a breakpoint on this interesting case.
            }
            pNodeMgr->Unlock();
        }
        CATCH
        {
            pNodeMgr->Unlock();
            Exception::throwAgain();
        }
        ENDTRY
    }
}

AWCHAR* Node::orphanText()
{
    // This is NOT multi-reader safe - it is only used at load time
    // before the NODE is marked "finished".
    AWCHAR* pText = getCollapsedText();
    if (pText)
    {
        _pText = null;
        _valuetype = VAL_STR;
    }
    return pText;
}

String*
Node::getXML()
{
    StringBuffer* buf = StringBuffer::newStringBuffer();
    Stream* out = StringStream::newStringStream(buf);
    IStream * pIStream;
    OutputHelper* h = null;
    String* xml = null;

    out->getIStream(&pIStream);
    TRY
    {

        // String is in UCS-2 format...
        // because StringStream accepts UNICODE in Write(), we can directly use OutputHelper
        // instead of using EncodingStream to wrap out first
        h = OutputHelper::newOutputHelper(pIStream, OutputHelper::PRETTY);
        h->_fEncoding = false; // just write unicode string...
        // Save document to this string buffer.
        save(getNodeDocument(), h);
        h->close();
        xml = String::newString(buf);
    }
    CATCH
    {
        if (h)
            h->close();
        pIStream->Release();
        Exception::throwAgain();
    }
    ENDTRY

    pIStream->Release();

    return xml;
}


String*
Node::getAttributesXML()
{
    StringBuffer* buf = StringBuffer::newStringBuffer();
    Stream* out = StringStream::newStringStream(buf);
    IStream * pIStream;
    OutputHelper* h = null;

    out->getIStream(&pIStream);
    TRY
    {

        // String is in UCS-2 format...
        // because StringStream accepts UNICODE in Write(), we can directly use OutputHelper
        // instead of using EncodingStream to wrap out first
        h = OutputHelper::newOutputHelper(pIStream, OutputHelper::PRETTY);

        // Save attributes to this string buffer.
        saveAttributes(getNodeDocument(), h);
        h->close();

    }
    CATCH
    {
        if (h)
            h->close();
        pIStream->Release();
        Exception::throwAgain();
    }
    ENDTRY

    pIStream->Release();

    return String::newString(buf);
}


// Ensure that there is a NameSpace decl attribute for the given namedef
void
EnsureNSDecl(OutputHelper * o, NamespaceMgr * pNSStack, Node * pNode, NameDef * pNameDef, bool fAttr)
{
    Assert(pNSStack); 
    Assert(pNode); 
    Assert(pNameDef);

    Atom * pPrefix = pNameDef->getPrefix();
    Name * pName = pNameDef->getName();
    Atom * pURN = pName->getNameSpace();
    PVOID pContext = null;

    // don't worry about attribute's with no prefix, or prefixed name with no Namespace
    if ((!pPrefix && fAttr) || (pPrefix && !pURN))
        return;

    // BUGBUG: what do we do if there is a conflict? right now we assume that whatever was pushed first is correct
    if (pNSStack->findURN(pPrefix, &pContext) != pURN
        && pContext != (void *)pNode)
    {
        pNSStack->pushScope( pPrefix, pURN, null, (void *)pNode);
        o->write((TCHAR*)XMLNames::pszSpace);
        o->write((TCHAR*)XMLNames::pszXMLNS);
        if (pPrefix)
        {
            o->write(':');
            o->write(pPrefix->toString());
        }
        o->write('=');
        o->write('\"');
        Atom * pSrcURN = pNameDef->getSrcURN();
        if (pSrcURN)
            o->write(pSrcURN->toString());
        o->write('\"');
    }
}

/**
 * Save the attributes in XML format
 */
void 
Node::saveAttributes(Document * pDoc, OutputHelper * o, NamespaceMgr * pNSStack)
{
    HANDLE  h;

    Node * a = getNodeFirstAttribute(&h);
    while (a)
    {
        if (pNSStack)
            EnsureNSDecl( o, pNSStack, this, a->_getNameDef(), true);

        o->write((TCHAR*)XMLNames::pszSpace);
        a->save(pDoc, o);
        a = getNodeNextAttribute(&h);
    }
}

void WriteAttribute(OutputHelper* o, TCHAR* pszName, String* value)
{
    o->write((TCHAR*)XMLNames::pszSpace);
    o->write(pszName);
    o->write((TCHAR*)XMLNames::pszEquals);
    o->write((TCHAR*)XMLNames::pszQUOTE);
    o->write(value); 
    o->write((TCHAR*)XMLNames::pszQUOTE);
}

void
Node::saveQuotedValue(OutputHelper* o)
{
    Object * pObj;
    String * pText;
    AWCHAR* pCollapsedText;

    o->write(L'"');
    switch (getValueType())
    {
    case VAL_OBJ:
        if (_pValue)
            o->write(_pValue->toString());
        break;

    case VAL_STR:
        if (_pText)
            o->writeString((TCHAR*)_pText->getData(), _pText->length());
        break;

    case VAL_TYPED:
        Assert(0 && "Shoult Never Happen!");
        break;
 
    case VAL_PARENT:
        if (null != (pCollapsedText = getCollapsedText()))
        {
            o->writeString((TCHAR*)pCollapsedText->getData(), pCollapsedText->length());
        }
        else
        {
            bool fLastWasText = false;
            void * pTag;
            for (Node * pNode = getNodeFirstChild(&pTag); pNode; pNode = getNodeNextChild(&pTag))
            {
                switch (pNode->_nodetype)
                {
                case ENTITYREF:
                    o->write(L'&');
                    o->write(pNode->_getNameDef()->toString());
                    o->write(L';');
                    break;

                case PCDATA:
                    Assert( pNode->getValueType() == VAL_STR);
                    if (pNode->_pText)
                        o->writeString((TCHAR*) pNode->_pText->getData(), pNode->_pText->length());
                    break;
                }
                if (pNode->getWSFollow())
                    o->write( L' ');
            }
        }
        break;
    }
    o->write(L'"');
}

void
Node::save(Document * pDoc, OutputHelper * o, NamespaceMgr * pNSStack)
{
    NameDef * tag = _getNameDef();
    if (!pNSStack)
        pNSStack = NamespaceMgr::newNamespaceMgr(false);

    switch (_nodetype)
    {
        case DOCFRAG:
            {
                void * pv;
                Node * pChildNode = getNodeFirstChild(&pv);
                for (; pChildNode != null; pChildNode = getNodeNextChild( &pv))
                {
                    pChildNode->save(pDoc, o, pNSStack);
                    if (pChildNode->getNodeType() != Element::PCDATA &&
                        pChildNode->getWSFollow())
                         o->writeNewLine();
                }
            }
            break;

        case DOCUMENT:
        case ELEMENT:
            {
                // find out if this is a mixed element.
                bool fPrevWS = getWSInner();
                bool fHasChildren = ! isEmpty();
                bool fClose = false;

                if (tag != null) 
                {
                    o->write((TCHAR*)XMLNames::pszLessThan);
                    // Write out the tag name
                    o->write(tag->toString());

                    if (! isCollapsedText()) 
                    {
                        pNSStack->pushScope(this);

                        EnsureNSDecl( o, pNSStack, this, _getNameDef(), false);

                        // Write out the attributes->
                        saveAttributes(pDoc, o, pNSStack);
                    }
                    else
                        EnsureNSDecl( o, pNSStack, this, _getNameDef(), false);

                    // Write the close start tag.
                    if (fHasChildren || fPrevWS || _fNotQuiteEmpty)
                    {
                        fClose = true;
                        o->write((TCHAR*)XMLNames::pszGreaterThan);                            
                    }
                    else
                    {
                        o->write((TCHAR*)XMLNames::pszEMPTYTAGEND);
                    }                            
                }

                if (isCollapsedText())
                {
                    o->writeString(getCollapsedText());
                }
                else
                {
                    void * pv;
                    Node * pChildNode;

                    pChildNode = getNodeFirstChild( &pv);

                    if (fPrevWS && (!pChildNode || pChildNode->getNodeType() != Element::PCDATA))
                        o->writeNewLine();
            
                    if (pChildNode)
                    {
                        if (!_fDocument)
                            o->addIndent(1);

                        while (pChildNode != null)
                        {
                            pChildNode->save(pDoc, o, pNSStack);
                            Node * pNextChild = getNodeNextChild( &pv);
                            if ( (_fDocument) ||
                                 ((!pNextChild ||
                                   pNextChild->getNodeType() != Element::PCDATA)
                                  && pChildNode->getWSFollow()))
                                 o->writeNewLine();
                            pChildNode = pNextChild;
                        }

                        if (!_fDocument)
                            o->addIndent(-1);
                    }
                }

                if (fClose)
                {                        
                    o->write((TCHAR*)XMLNames::pszCLOSETAGSTART);
                    o->write(tag->toString());
                    o->write((TCHAR*)XMLNames::pszGreaterThan); 
                }
                pNSStack->popScope((void*)this);
            }
            break;
        case PI:
            o->write((TCHAR*)XMLNames::pszPITAGSTART);
            o->write(tag->toString());
            saveAttributes(pDoc, o);
            o->write((TCHAR*)XMLNames::pszSpace);
            o->write(_pText);
            o->write((TCHAR*)XMLNames::pszPITAGEND);
            break;
        case CDATA:
            o->write((TCHAR*)XMLNames::pszDECLTAGSTART);
            o->write((TCHAR*)XMLNames::pszCDATA2);
            o->write(_pText);
            o->write((TCHAR*)XMLNames::pszCDEND);
            break;
        case PCDATA:
            // This is a special version that escapes special characters.
            {
                // BUGBUG - now that we support XML-SPACE in IE4 documents
                // we may need to strip whitespace here on Save in order
                // to be fully compatible with old IE4 behavior.
                o->writeString(_pText);
            }
            break;
        case ENTITYREF:
            o->write((TCHAR*)XMLNames::pszAMP);
            o->write(tag->toString());
            o->write((TCHAR*)XMLNames::pszSEMICOLON);
            break;
        case COMMENT:
            o->write((TCHAR*)XMLNames::pszCOMMENT);
            o->write(_pText);
            o->write((TCHAR*)XMLNames::pszENDCOMMENT);
            break;
        case ATTRIBUTE:
            {
                if (tag->getName() == XMLNames::name(NAME_XMLSpace))
                    o->write(tag->getName()->getName()->toString());
                else if (tag->getName() == XMLNames::name(NAME_XMLNS))
                    o->write(XMLNames::atomXMLNS->toString());
                else
                    o->write(tag->toString());
                o->write('=');
                saveQuotedValue(o);
            }
            break;
        case XMLDECL:
            {
                // We must write out the attributes in the right order
                // to comply with the XML spec.
                // (version, encoding, standalone).
                o->write((TCHAR*)XMLNames::pszPITAGSTART);
                o->write((TCHAR*)XMLNames::pszXML);

                String * version = pDoc->getAttributeIgnoreCase(this, XMLNames::name(NAME_VERSION));
                if (version == null)
                {
                    version = String::newString(XMLNames::pszDefaultVersion);
                }
                WriteAttribute(o, (TCHAR*)XMLNames::pszVersion, version);

                if (o->_fEncoding)
                {
                    String* encoding = pDoc->getAttributeIgnoreCase(this, XMLNames::name(NAME_encoding));
                    if (encoding)
                    {
                        WriteAttribute(o, (TCHAR*)XMLNames::pszEncoding, encoding);
                    }
                }

                String* standalone = pDoc->getAttributeIgnoreCase(this, XMLNames::name(NAME_Standalone));
                if (standalone)
                {
                    WriteAttribute(o, (TCHAR*)XMLNames::pszStandalone, standalone);
                }
            
                o->write((TCHAR*)XMLNames::pszPITAGEND);
            }
            break;
        case DOCTYPE:
        case ENTITY:
        case NOTATION:
            {
                o->write((TCHAR*)XMLNames::pszLessThan);
                o->write((TCHAR*)XMLNames::pszBANG);
                if (DOCTYPE == _nodetype)
                    o->write((TCHAR*)XMLNames::pszDOCTYPE);
                else if (ENTITY == _nodetype)
                    o->write((TCHAR*)XMLNames::pszENTITY);
                else if (NOTATION == _nodetype)
                    o->write((TCHAR*)XMLNames::pszNOTATION);
                else
                    Assert(0 && "Unexpected Type!");

                Assert(tag);
                o->write((TCHAR*)XMLNames::pszSpace);
                o->write(tag);

                Object * sysId = getAttribute(XMLNames::name(NAME_SYSTEM));
                // notations can have just a public ID
                if (sysId != null || (NOTATION == _nodetype)) 
                {
                    Object * pubId = getAttribute(XMLNames::name(NAME_PUBLIC));
                    if (pubId != null) 
                    {
                        o->write((TCHAR*)XMLNames::pszSpace);
                        o->write((TCHAR*)XMLNames::pszPUBLIC);
                        o->write((TCHAR*)XMLNames::pszSpace);
                        o->write(L'"');
                        o->writeString(pubId->toString());
                        o->write(L'"');
                    } 
                    else if (sysId)
                    {
                        o->write((TCHAR*)XMLNames::pszSpace);
                        o->write((TCHAR*)XMLNames::pszSYSTEM);
                    }
                    if (sysId)
                    {
                        o->write((TCHAR*)XMLNames::pszSpace);
                        o->write(L'"');
                        o->writeString(sysId->toString());
                        o->write(L'"');
                    }

                    if (ENTITY == _nodetype)
                    {
                        Object * ndata = getAttribute(XMLNames::name(NAME_NDATA));
                        if (ndata != null) 
                        {
                            o->write((TCHAR*)XMLNames::pszSpace);
                            o->write((TCHAR*)XMLNames::pszNDATA);
                            o->write((TCHAR*)XMLNames::pszSpace);
                            o->writeString(ndata->toString());
                        }
                    }
                }
                else if (ENTITY == _nodetype) // entity has value
                {
                    o->write((TCHAR*)XMLNames::pszSpace);
                    saveQuotedValue(o);
                }

                if (DOCTYPE == _nodetype) // entity has value
                {
                    // check if there was an internal
                    // subset that also needs to be written out.
                    DTD * pDTD = getNodeDocument()->getDTD();
                    String * pSubset = pDTD ? pDTD->getSubsetText() : null;
                    if (pSubset)
                    {
                        o->write((TCHAR*)XMLNames::pszSpace);
                        o->write((TCHAR*)XMLNames::pszLEFTSQB);                
                        o->write(pSubset);
                        o->write((TCHAR*)XMLNames::pszRIGHTSQB);                
                    }
                }

                o->write((TCHAR*)XMLNames::pszGreaterThan);
            }

    } // end case
}


///////////////////////////////////////////////////////////////////////////////
// clone part of the node 
///////////////////////////////////////////////////////////////////////////////
Node* 
Node::_clone(bool fReadOnly, Document* pDoc, NodeManager* pManager)
{
    Node * clonedNode = new (pManager) Node();
    if (!clonedNode)
    {
        Exception::throwE((HRESULT)E_OUTOFMEMORY);
    }

    clonedNode->_flags = _flags;

    clonedNode->_fFloating = !_fDocument;
    clonedNode->_fFinished = 1;

    clonedNode->_pDocument = pDoc;
    clonedNode->_fReadOnly = fReadOnly;

    if (_fDocument)
    {
        Assert(DOCUMENT == _nodetype);
        Assert(VAL_PARENT == _valuetype);
        _pDocument->weakAddRef();
    }
    else
    {
        if (_fTyped)
        {
            Assert(_pTypedValue);
            clonedNode->_pTypedValue = _pTypedValue->_clone(fReadOnly, pDoc, pManager);
            clonedNode->_pTypedValue->attach();
        }
        else
        {
            if (_pName)
            {
                if (pDoc != _pDocument)
                {
                    // create new NameDef using new document's namespace mgr.
                    Atom * pAtomURN = _pName->getName()->getNameSpace();
                    Atom * pAtomPrefix = _pName->getPrefix();
                    NameDef * pNameDef = pDoc->getNamespaceMgr()->createNameDef(_pName->getName()->getName()->toString(), pAtomURN, _pName->getSrcURN(), pAtomPrefix);
                    assign(&(clonedNode->_pName), pNameDef);
                }
                else
                {   
                    assign(&(clonedNode->_pName), _pName);
                }
            }

            switch (_valuetype)
            {
            case VAL_PARENT:
                break;

            case VAL_STR:
                assign(&clonedNode->_pText, _pText);
                break;

            case VAL_OBJ:
                // BUGBUG -- should we really use Object::clone here ???
                assign(&clonedNode->_pValue, _pValue);
                break;

            case VAL_TYPED:
                {
                    VARIANT var;
                    VARIANT varNew;
                    TRY
                    {
                        VariantInit(&var);
                        VariantInit(&varNew);
                        toVariant(&var); // get this node's variant
                        HRESULT hr = VariantCopy(&varNew, &var); // copy it
                        if (hr)
                            Exception::throwE(hr);
                        // fromVariant takes over ownership of varNew.
                        clonedNode->fromVariant(&varNew);
                    }
                    CATCH
                    {
                        VariantClear(&varNew);
                        Exception::throwAgain();
                    }
                    ENDTRY
                }
                break;
            }
        }
    }
Cleanup:
    return clonedNode;
}

///////////////////////////////////////////////////////////////////////////////
// if deep is true, clone node, attributes and children
// otherwise, just copy the node and its attributes
///////////////////////////////////////////////////////////////////////////////
Node* 
Node::clone(bool fDeep, bool fReadOnly, Document* pDoc, NodeManager* pManager, bool fWholeDoc)
{
    Node * clonedNode = null;

    clonedNode = _clone(fReadOnly, pDoc, pManager); 

    if (isCollapsedText())
    {
        if (fDeep)
        {
            AWCHAR* pText = getCollapsedText();
            if (pText)
            {
                assign(&clonedNode->_pText, pText);
                clonedNode->_pText = BitSetPointer(clonedNode->_pText,COLLAPSED_TEXT_BIT);
            }
        }
    }
    else if (VAL_PARENT == clonedNode->getValueType())
    {
        // clone attributes (and children if fDeep == true) if there is any
        clonedNode->_pLast = cloneChildren( fDeep, true, fReadOnly, pDoc, pManager, clonedNode, fWholeDoc);
    }

Cleanup:
    return clonedNode;
}


///////////////////////////////////////////////////////////////////////////////
// clone the children of the node
///////////////////////////////////////////////////////////////////////////////
Node * 
Node::cloneChildren(bool fDeep, bool fAttrs, bool fReadOnly, Document* pDoc, NodeManager* pManager, Node * pParent, bool fWholeDoc)
{
    // clone attributes (and children if fDeep == true) if there is any
    void * pv;
    Node * pNode, * pClone;
    Node * pLast = null;
    bool fRegisterID = fWholeDoc && !_fAttribute;
    for (pNode = getFirstNodeNoExpand(&pv); pNode; pNode = getNextNode(&pv))
    {
        // if it's an attribute, do a deep clone of it
        if (pNode->_fAttribute && fAttrs || fDeep)
        {
            pClone = pNode->clone(pNode->_fAttribute || fDeep, fReadOnly, pDoc, pManager, fWholeDoc);
            pParent->_append(pClone, &pLast);
            if (fRegisterID && pClone->_fAttribute && pClone->_fID)
            {
                DTD * pDTD = pDoc->getDTD();
                Name * pID = pClone->getContentAsName();
                pDTD->addID(pID, pParent);
            }
        }
    }
    return pLast;
}


///////////////////////////////////////////////////////////////////////////////
// clone the children of the entity node this entity reference refers to
///////////////////////////////////////////////////////////////////////////////
void 
Node::_expandEntityRef(void)
{
    Document * pDoc = getNodeDocument();
    Node * pEntityNode = resolveEntityRef();

    if (pEntityNode)
    {
        if (pEntityNode->isCollapsedText())
            pEntityNode->uncollapse();

        NodeManager * pNodeMgr = pDoc->getAltNodeMgr();
        pNodeMgr->Lock();
        if (!_pLast)
        {
            TRY
            {
                _pLast = pEntityNode->cloneChildren(true, false, true, pDoc, pNodeMgr, this);
                setSpecified(false);
                pNodeMgr->Unlock();
            }
            CATCH
            {
                pNodeMgr->Unlock();
                Exception::throwAgain();
            }
            ENDTRY
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// merge adjacent text nodes that have the same parent into one node, 
// but only untyped PCDATA nodes, and NOT attribute values.
///////////////////////////////////////////////////////////////////////////////
void 
Node::_normalize(void)
{
    // Can never be called on an ENTITYREF
    Assert(isParent() && _pLast);
    Assert(!_fTyped);
    Assert(ELEMENT == _nodetype || DOCUMENT == _nodetype || DOCFRAG == _nodetype);

    HANDLE handle;

    if (isCollapsedText())
        return; // nothing to do.

    Node * pFirst = getNodeFirstChild(&handle);
    if (!pFirst)
        return;

    Node * pNode = pFirst;
    Node * pFirstText = null, * pLastText = null;
    int size  = 0;

    for (;;)
    {
        if (pNode && PCDATA == pNode->_nodetype)  // Found a PCDATA node
        {
            if (!pFirstText)
            {
                pFirstText = pNode;
            }
            pLastText = pNode;
            if (pNode->_pText)
            {
                size += pLastText->_pText->length();
            }
        }
        else
        {
            if (pFirstText)
            {
                if (pFirstText != pLastText)  // Need to normalize
                {
                    AWCHAR * buf = null;
                    if (size)
                    {
                         buf = new (size) AWCHAR;
                    }

                    // Normalize text nodes
                    Node * pNodeNew = Node::newNodeFast(PCDATA, null, null, buf, null, 0, 
                                                        _pDocument,
                                                        _pDocument->getNodeMgr());
                    _insert(pNodeNew, pFirstText); // insert before pFirstText
                    int  offset = 0;
                    Node * p1;
                    Node * p = pFirstText;
                    do {
                        if (p->_pText && pNodeNew->_pText)
                        {
                            pNodeNew->_pText->simpleCopy(offset, p->_pText->length(), p->_pText->getData());
                            offset += p->_pText->length();
                        }
                        p1 = p;
                        p = p->getNextSibling();
                        _remove(p1);
                    }
                    while (p1 != pLastText);
                }

                // null out 
                pFirstText = pLastText = null;
                size = 0;
            }

            if (pNode && !pNode->_fTyped && (ELEMENT == pNode->_nodetype)
                && pNode->isParent() && pNode->_pLast)
            {
                pNode->_normalize();
            }
        }

        if (pNode)
        {
            pNode = getNodeNextChild(&handle);
        }
        else
        {
            break;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// check conditon and calls _normalize() to normalize untyped PCDATA nodes 
///////////////////////////////////////////////////////////////////////////////
void 
Node::normalize(void)
{

    if (((!_fTyped && (ELEMENT == _nodetype)) || (DOCUMENT == _nodetype) || (DOCFRAG == _nodetype))
        && isParent() && _pLast)
    {
        _normalize();
    }
}


Node *
Node::getNextMatchingAttribute(void ** ppv, Name * tag)
{
    Node * pNode;

    if (*ppv == null)
        pNode = getNodeFirstAttributeWithDefault(ppv);
    else
        pNode = getNodeNextAttributeWithDefault(ppv);

    while (pNode && tag && (pNode->getName() != tag))
        pNode = getNodeNextAttribute(ppv);

    return pNode;
}


Node *
Node::getNextMatchingChild(void ** ppv, Name * tag)
{
    Node * pNode;

    if (*ppv == null)
        pNode = getNodeFirstChild(ppv);
    else        
        pNode = getNodeNextChild(ppv);

    while (pNode && tag && (pNode->getName() != tag))
        pNode = getNodeNextChild(ppv);

    return pNode;
}


/************************************************************************* 
 * Element methods and members
 *************************************************************************/

DEFINE_CLASS_MEMBERS_CLASS(ElementNode, _T("ElementNode"), Element);

#define AssertElementNode(x) AssertPMATCH(x, ElementNode)

/** 
* Retrieves the parent of this element. 
* Every element in the tree except the Document itself, has
* a parent.  
*
* @return the parent element or null if at the root of 
* the tree.
*/
Element * 
ElementNode::getParent()
{
    Node * pnode = getNodeParent();
    while (pnode && pnode->getType() == Node::ENTITYREF)
    {
        pnode = pnode->getNodeParent();
    }
    return pnode;
}


Name *
ElementNode::getTagName()
{
    NameDef * namedef = _getNameDef();
    if (namedef)
        return namedef->getName();
    else
        return null;
}


void
ElementNode::setTagName(String * name)
{
    Document * pDoc = getNodeDocument();
    NameDef * pNameDef = pDoc->getNamespaceMgr()->createNameDef(name->getWCHARPtr(), name->length());
    setName(pNameDef);
}


int s_aNodeType2ElementType[] =
{
    0,  //    ELEMENT = 0,
    1,  //    PCDATA = 1,
    2,  //    COMMENT = 2,
    3,  //    DOCUMENT = 3,
    4,  //    DOCTYPE = 4,
    5,  //    PI = 5,
    6,  //    CDATA = 6,     
    7,  //    ENTITY = 7, 
    8,  //    NOTATION = 8,     
    9, //    ELEMENTDECL = 9,     
    10, //    NAMESPACE = 10,
    11, //    ENTITYREF = 11,
    12, //    WHITESPACE = 12,
    13, //    INCLUDESECTION = 13,
    14, //    IGNORESECTION = 14
    15, //    ATTRIBUTE = 15,
    -1, //    TYPEDVALUE = 16,
    3,  //    DOCFRAG = 17
    5   //    XMLDECL = 18
};

/**
* Retrieves the type of the element.
* This is always one of the following values:
* <code>DOCUMENT</code>, <code>ELEMENT</code>, <code>PCDATA</code>, <code>PI</code>,
* <code>META</code>, <code>COMMENT</code>, or <code>CDATA</code>.
* 
* @return element type.
*/    
int 
ElementNode::getType()
{
    // BUGBUG - Why is getType mapping the type now that Element::NodeType and Node::NodeType are the same?
    Element::NodeType type = getNodeData()->getNodeType();
    int r = s_aNodeType2ElementType[type];
    Assert(r >= 0);
    return r;
}   

/** 
* Returns the non-marked-up text contained by this element.
* For text elements, this is the raw data.  For elements
* with child nodes, this traverses the entire subtree and
* appends the text for each terminal text element, effectively
* stripping out the XML markup for the subtree. For example,
* if the XML document contains the following:
* <xmp>
*      <AUTHOR>
*          <FIRST>William</FIRST>
*          <LAST>Shakespeare</LAST>
*      </AUTHOR>
* </xmp>
* <p><code>Document.getText</code> returns "William Shakespeare".
*/
String * 
ElementNode::getText(bool fForceCollapse, bool fNormalize)
{
    Node * pNode = getNodeData();

    checkFinished();

    bool fPreserve = !(isParent() || fForceCollapse);
    // get inner text, but collapse whitespace (ignoring XML:Space)
    return pNode->getInnerText(fPreserve, true, fNormalize);
}

/**
* Sets the text for this element. Only meaningful in 
* <code>CDATA</code>, <code>PCDATA</code>, and <code>COMMENT</code> nodes.
*
* @param text The text to set.
* @return No return value.
*/    
void 
ElementNode::setText(String * text)
{
    Node * pNode = getNodeData();
    if (!pNode->isParent())
        pNode->setInnerText(text, true);
}


/**
 * Returns the first child of this element. The <code>HANDLE</code> must be
 * passed to <code>getNextChild</code>.
 */
Element * 
ElementNode::getFirstChild(HANDLE * ph)
{
    Node * pnode = getNodeData();
    Element * e;
    
    while (true)
    {
        pnode = pnode->getNodeFirstChild(ph);
        if (!pnode)
        {
            checkFinished();
            e = null;
            break;
        }

        if (pnode->getNodeType() != ENTITYREF)
        {
            e = (Element *) pnode->getElementWrapper();
            break;
        }
    }

    return e;
}

/**
 * Returns the next child of this element. The <code>HANDLE</code> must be
 * passed to <code>getNextChild</code>.
 */
Element * 
ElementNode::getNextChild(HANDLE * ph)
{
    Node * pRoot = getNodeData();
    Node * pParent = ((Node *) *ph)->_pParent;
    Node * pnode;
    Element * e = null;

    while (pParent)
    {
        pnode = pParent->getNextNode(ph);
        if (!pnode)
        {
            if (pParent != pRoot)
            {
                *ph = pParent;
                pParent = pParent->_pParent;
            }
            else
            {
                checkFinished();
                e = null;
                break;
            }
        }
        else  if (pnode->getNodeType() != ENTITYREF)
        {
            e = (Element *) pnode->getElementWrapper();
            break;
        }
        else
        {
            e = pnode->getFirstChild(ph);
            break;
        }
    }

    return e;
}


/**
* Retrieves the number of child elements.
* @return the number of child elements.
*/
int 
ElementNode::numElements()
{
    void * pTag;
    int n = 0;

    Node * pNode = getNodeData();
    if (pNode->isParent())
    {
        for (Element * pe = pNode->getFirstChild(&pTag); pe; pe = pNode->getNextChild(&pTag))
            n++;
    }
    return n;
}


/**
* Retrieves nonzero if element has children
*/

int
ElementNode::hasChildren()
{
    return ! isEmpty();
}


/**
* Adds a child to this element. Any element can only
* have one parent element and so the previous parent will lose this child
* from its subtree.  
*
* @param elem  The element to add.      
* The child element becomes the last element if <i>after</i> is null.
* The child is added to the beginning of the list if <i>after</i> is this object.
* @param after The element after which to add it. 
* @return No return value.
*/
void 
ElementNode::addChild(Element * elem, Element * eBefore)
{
    void * pTag;

    AssertElementNode(elem);
    ElementNode * penElem = CAST_TO(ElementNode *, elem);
    Node * pNode = getNodeData();
    if (pNode->isParent())
    {
        Node * pndBefore;
        if (eBefore == null)
        {
            pndBefore = null;
        }
        else if (eBefore == this)
        {
            pndBefore = pNode->getNodeFirstChild(&pTag);
        }
        else
        {
            pndBefore = CAST_TO(ElementNode *, eBefore)->getNodeData();
        }

        Node * pndElem = penElem->getNodeData();
        pNode->insertNode(pndElem, pndBefore);
    }
}

/**
* Adds a child to this element. 
* @param elem The element to add.
* @param pos  The position to add this element (calling <code>getChild(pos)</code> 
* will return this element). If <i>pos</i> is less than 0, <i>elem</i> becomes 
* the new last element.
* @param reserved The reserved parameter.
* @return No return value.
*/
void 
ElementNode::addChildAt(Element * elem, int pos)
{
    AssertElementNode(elem);
    ElementNode * penElem = CAST_TO(ElementNode *, elem);
    Node * pNode = getNodeData();
    if (pNode->isParent())
    {
        Element * pOldParent = penElem->getNodeParent();
        if (pOldParent)
            pOldParent->removeChild(elem);

        Node * pndElem = penElem->getNodeData();
        Node * pRefNode = pos < 0 ? null : pNode->findChild(pos);
        pNode->insertNode(pndElem, pRefNode);
    }
}


/**
* Removes a child element from the tree.
*
* @param elem  The element to remove.
*/   
void 
ElementNode::removeChild(Element * elem)
{
    AssertElementNode(elem);
    ElementNode * penElem = CAST_TO(ElementNode *, elem);
    Node * pNode = getNodeData();
    if (pNode->isParent())
    {
        if (penElem->getNodeParent() == this)
        {
            Node * pndElem = penElem->getNodeData();
            pNode->removeNode(pndElem);

            pndElem->setFloatingRec(pNode->isFloating());
        }
        else
        {
            Exception::throwE(E_INVALIDARG, XMLOM_NOTCHILD, null);
        }
    }
}


/**
 * Returns the first attribute of this element. The <code>HANDLE</code> must be
 * passed to <code>getNextAttribute</code>.
 */
Element * 
ElementNode::getFirstAttribute(HANDLE * ph)
{
    Node * pnode = getNodeData()->getNodeFirstAttributeWithDefault(ph);
    if (pnode)
    {
        return (Element *) pnode->getElementWrapper();
    }

    return null;
}

/**
 * Returns the next attribute of this element. The <code>HANDLE</code> must be
 * passed to <code>getAttribute</code>.
 */
Element * 
ElementNode::getNextAttribute(HANDLE * ph)
{
    Node * pnode = getNodeData()->getNodeNextAttributeWithDefault(ph);
    if (pnode)
    {
        return (Element *) pnode->getElementWrapper();
    }

    return null;
}

/**
* Retrieves the number of attributes.
* @return the number of attributes.
*/
int 
ElementNode::numAttributes()
{
    void * pTag;
    int n = 0;

    Node * pNode = getNodeData();
    if (pNode->isParent())
    {
        for (Node * pN = pNode->getNodeFirstAttributeWithDefault(&pTag); pN; pN = pNode->getNodeNextAttributeWithDefault(&pTag))
            n++;
    }
    return n;
}

/**
* Retrieves an attribute's value given its name.
* @param name The name of the attribute.
* @return the value of the attribute 
* or null if the attribute is not found.
*/    
Object * 
ElementNode::getAttribute(Name * n)
{
    Node * pNode = getNodeData();
    if (pNode->isParent())
    {
        return pNode->getNodeAttribute(n);
    }

    return null;
}

/**
* Sets the attribute of this element.    
*
* @param name  The attribute name.
* @param value The attribute value.
*/
void 
ElementNode::setAttribute(Name * name, Object * value)
{
    setAttribute(name, value, getNodeDocument(), null);
}

void 
ElementNode::setAttribute(Name * name, Object * value, Document * document, Atom * pPrefix)
{
    Node * pNode = getNodeData();
    if (pNode->isParent())
        pNode->setNodeAttribute(name, value, pPrefix);
}

/**
* Deletes an attribute from an element.
* @param name The attribute to delete.
* @return No return value.
*/    
void 
ElementNode::removeAttribute(Name * name)
{
    Node * pNode = getNodeData();
    if (pNode->isParent())
        pNode->removeAttribute(name, null);
}

/**
 * Gets typed value into the variant.
 */
void
ElementNode::getTypedValue(VARIANT * pVar)
{
    checkFinished();
    getNodeData()->getNodeTypedValue(pVar);
}

/**
 * Gets typed value into the variant.
 */
void
ElementNode::getTypedValue(DataType dt, VARIANT * pVar)
{
    checkFinished();
    getNodeData()->getNodeTypedValue(dt, pVar);
}


/**
 * Sets typed value.
 */
void 
ElementNode::setTypedValue(VARIANT * pVar)
{
    getNodeData()->setNodeTypedValue(pVar);
}

/**
 * Gets data type string.
 */
String * 
Node::getDataTypeString() // virtual function
{
    const TCHAR * pwc;

    // If it is not an attribute, don't return datatype string 
    // for attribute types other than ID
    if (_fDT && (_fAttribute || _datatype <= DT_AV_ID || DT__NON_AV <= _datatype))
        return String::newString(LookupDataTypeName((DataType)_datatype));
    else
        return null;

#if NEVER
    // find an attribute with a name where namespace is 
    // for the datatype URI
    void * pTag;

    for (Node * pA = getNodeFirstAttributeWithDefault(&pTag); pA; pA = getNodeNextAttributeWithDefault(&pTag))
    {
        if (XMLNames::name(NAME_DTDT) == pA->getName())
        {
            Object * pO = pA->getNodeValue();
            if (pO)
                return String::add(String::newString(_T("[USER DEFINED] ")), pO->toString(), null);
        }
    }
    return String::newString(_T("[USER DEFINED]"));
#endif
}

/**
 * Sets data type string.
 */
void 
ElementNode::setDataTypeString(String * pS)
{
    Node * pDTAttr = null;
    if (!_fAttribute)
        pDTAttr = FindDTAttribute(this, getDocument());
    else if (!_appliedDt && DT_NONE != _datatype && DT_STRING != _datatype)
    {
        // can not change datatype applied by schema!
        // BUGBUG: this should be a better error message !!!
        Exception::throwE(E_FAIL,
                          XMLOM_DTDT_DUP, 
                          null);
    }

    if (pS->isWhitespace())
    {
        if (pDTAttr)
            removeNode(pDTAttr, true);
        else if (_fDT)
            removeDataType();        
    }
    else
    {
        if (_fAttribute)
        {
            DataType eDT = LookupDataType(pS->trim(), true);
            parseAndSetTypedData(eDT, false);
            _appliedDt = true;
        }
        else
        {
            // prefix param is only used if no node with name==pName is found
            setNodeAttribute(XMLNames::name(NAME_DTDT), pS, Atom::create(XMLNames::pszDTDT), null);
        }
    }
}


/**
 * Gets typed attribute into the variant.
 */
void 
Node::getTypedAttribute(Name * n, VARIANT * pVar)
{
    Node * pAttr = find(n, Element::ATTRIBUTE, _pDocument);
    if (pAttr)
        pAttr->getNodeTypedValue(pVar);
    else
        ::VariantClear(pVar);
}


/**
 * Sets typed attribute from the variant.
 */
void 
Node::setTypedAttribute(Name * n, VARIANT * pVar)
{
    Node * pAttr = find(n, Element::ATTRIBUTE, _pDocument);
    if (pAttr)
        pAttr->setNodeTypedValue(pVar);
}


/**
 * Whitespace inside of this element, e.g.
 *      <TR>
 *          <TD>XYZ</TD>
 *      </TR>
 *  
 *  hasWSInside is true for the TR because of the WS between it and the first TD
 */

bool 
ElementNode::hasWSInside()
{
    // BUGBUG - need to have a child or I'm finished - Not an issue now because XSL doc is downloaded sync.
    return getNodeData()->getWSInner();
}

/**
 * Whitespace inside of this element, e.g.
 *      <TR>
 *          <TD>XYZ</TD>
 *      </TR>
 *  
 *  hasWSAfter  is true for the TD because of the WS between it and the first </TD>
 */

bool 
ElementNode::hasWSAfter()
{
    // BUGBUG - need to check for sibling or parent is finished - Not an issue now because XSL doc is downloaded sync.
    return getNodeData()->getWSFollow();
}


TriState 
ElementNode::compare(OperandValue::RelOp op, DataType dt, OperandValue * popval)
{
    OperandValue opval;

    if (dt == DT_NONE)
    {
        dt = popval->_dt;
    }

    getValue(dt, &opval);

    return opval.compare(op, popval);
}


int 
ElementNode::compare(DWORD dwCmpFlags, DataType dt, OperandValue * popval, int * presult)
{
    OperandValue opval;

    if (dt == DT_NONE)
    {
        dt = popval->_dt;
    }

    getValue(dt, &opval);

    return opval.compare(dwCmpFlags, popval, presult);

}


void
ElementNode::getValue(DataType dt, OperandValue * popval)
{
    VARIANT var;

    switch (dt)
    {
    case DT_NONE:
    case DT_STRING:
    case DT_URI:
    case DT_UUID:
    // BUGBUG - What should be done with these
    case DT_BASE64:
    case DT_BIN_HEX:
    default:
        popval->initString(getText());
        break;

    case DT_BOOLEAN:
        V_VT(&var) = VT_EMPTY;
        getTypedValue(DT_BOOLEAN, &var);
        if (V_VT(&var) == VT_BOOL)
        {
            popval->initBOOL(V_BOOL(&var) ? true : false);
        }
        break;

    case DT_I1:
    case DT_I2:
    case DT_I4:
    case DT_INT:
    case DT_CHAR:
    case DT_UI1:
    case DT_UI2:
    case DT_UI4:
    case DT_FLOAT:
    case DT_FLOAT_IEEE_754_32:
    case DT_FLOAT_IEEE_754_64:
    case DT_R4:
    case DT_R8:
    // BUGBUG - DT_NUMBER can be converted more effeciently
    case DT_NUMBER:
        V_VT(&var) = VT_EMPTY;
        getTypedValue(DT_R8, &var);
        if (V_VT(&var) == VT_R8)
        {
            popval->initR8(V_R8(&var));
        }
        break;

    case DT_FIXED_14_4:
        V_VT(&var) = VT_EMPTY;
        getTypedValue(DT_FIXED_14_4, &var);
        if (V_VT(&var) == VT_CY)
        {
            popval->initCY(V_CY(&var));
        }
        break;
        
    case DT_DATE_ISO8601:
    case DT_DATETIME_ISO8601: 
    case DT_DATETIME_ISO8601TZ:
    case DT_TIME_ISO8601:
    case DT_TIME_ISO8601TZ:
        V_VT(&var) = VT_EMPTY;
        getTypedValue(DT_DATE_ISO8601, &var);
        if (V_VT(&var) == VT_DATE)
        {
            popval->initDATE(V_DATE(&var));
        }
        break;
    }
}


void
ElementNode::checkFinished()
{
    if (!isFinished())
    {
        Assert(!_pDocument->getDocNode()->isFinished() && "Node is not marked finished but Document is completely downloaded - Fix NodeFactory");

        Exception::throwE(E_PENDING);
    }
}


Element * 
GetElement(IDispatch * p)
{
    Element * e = null;
    IHTMLObjectElement * pobj = null;
    IDispatch * pdisp = null;

    // If this object is a document cocreated on an HTML page then MSHTML wraps the document
    // and returns an invalid Element.  Query for MSHTML's interface first to catch this.

    HRESULT hr;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = p->QueryInterface(IID_IHTMLObjectElement, (void **) &pobj);
    if (SUCCEEDED(hr) && pobj)
    {
        hr = pobj->get_object(&pdisp);
        if (!SUCCEEDED(hr) || !pdisp)
        {
            goto Cleanup;
        }
        p = pdisp;
    }

    hr = p->QueryInterface(IID_Element, (void **) &e);

Cleanup:
    release(&pobj);
    release(&pdisp);
    checkhr(hr);
    return e;
}


DOMNode * 
Node::getDOMNodeWrapper()
{
    if (_fDocument)
        return (DOMNode *)new DOMDocumentWrapper(getNodeDocument());
    else
        return DOMNode::newDOMNode(this);
}


HRESULT 
ElementNode::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_OBJECT(this);

    return QIHelper( NULL, NULL, NULL, NULL, iid, ppv);
}


ULONG
ElementNode::AddRef()
{
    STACK_ENTRY_OBJECT(this);

    return _addRef();
}


ULONG
ElementNode::_addRef()
{
    ULONG ul = super::_addRef();
    if (ul == 2)
        getNodeDocument()->_addRef();
    return ul;
}


ULONG 
ElementNode::Release()
{
    STACK_ENTRY_OBJECT(this);

    return _release();
}


ULONG 
ElementNode::_release()
{
    Document * pDocument = getNodeDocument();
    ULONG ul = super::_release();
    if (ul == 1)
        pDocument->_release();
    return ul;
}

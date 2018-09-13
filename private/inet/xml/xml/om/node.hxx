/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _XML_OM_NODE_HXX
#define _XML_OM_NODE_HXX

#ifndef _XML_CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif

class OutputHelper;

class Node;
DEFINE_CLASS(Document);
DEFINE_CLASS(Element);

#define ElementNode Node

DEFINE_CLASS(DTD);
DEFINE_CLASS(ElementDecl);
DEFINE_CLASS(AttDef);
DEFINE_CLASS(WSStringBuffer);
class DOMNode;
class DOMDocumentWrapper;
class IDocumentWrapper;
struct NodeInsertInfo;
class NamespaceMgr;

#include "element.hxx"

// just use the slotallocator for the nodes...
typedef SlotAllocator NodeManager;
typedef _reference<NodeManager> RNodeManager;

class Node : public Element
{
friend class Document;
friend class DOMNode;
friend class _NDNodeFactory;
friend class NodeDataNodeFactory;
friend class IE4NodeFactory;

    // Used to QI ElementNode-s and DOMNode-s for their NodeData
    public: static const IID s_IID;

    /*
     * valid "value" types
     */    
    enum ValueType
    {
        VAL_PARENT      = 0,
        VAL_TYPED       = 1,
        VAL_STR         = 2,
        VAL_OBJ         = 3,
    };

    // use static newNode to allocate a node
    protected: Node(Element::NodeType type, NameDef * pName, Document * pDoc);

    protected: ~Node();
         
    public:
    void notifyNew(Node * pParent = null, 
                   bool fParsing = false, bool fInsideEntity = false,
                   ElementDecl * = null, AttDef * = null);
    void notifyRemove(Node * pParent = null);
    void notifyChangeContent(String * psNewContent);

    // test if we need to send notification msgs
    bool testNotify() const;

    // (assuming this is an ID attribute), remove the corresponding
    // entry from the ID loojup table
    void removeID(DTD *);
    // try and add a new ID to the ID lookup table
    void addNewID(DTD * pDTD, Name * pID, Node * pParent, bool fThrowError);

    // takes a VARIANT and resolves IDispatch and IUnk's to a Node via QueryInterface(Node::s_IID...)
    public: static Node * Variant2Node( VARIANT * pVar);

    // generic constructor
    public: static Node * newNode(Element::NodeType type, NameDef * pName, Document * pDoc, NodeManager * pManager);
    public: static Node * newDocumentNode( Document *, NodeManager * pManager);
    // custom constructor of parser/treebuilder (less error checking)
    public: static Node * newNodeFast(Element::NodeType type, NameDef * pName, Node * pParent,
                                      AWCHAR * pText, const TCHAR *pwcContext, ULONG ulContentLen,
                                      Document * pDoc, NodeManager * pManager);

    public: void * operator new (size_t size, NodeManager * pManager);
    public: void operator delete (void * p);

    // never allocate with normal new
    private: void * operator new (size_t size) { Assert("shouldn't be used"); return (void *)null; }

    // Empty constructor is only used in _clone()
    private: Node() { }

    // the Node structure is the same size as the Variant structure which is used 
    // by storing the typed value as a variant node type !
    protected: union
            {
                struct
                {
                    unsigned    _vt                 : 16;   // used when _fVariant is set
                };

#define SHIFT__fill             0
#define SHIFT_fIDed             2
#define SHIFT_appliedDt         3
#define SHIFT_datatype          4
#define SHIFT_fFloating         10
#define SHIFT_fWSInner          11
#define SHIFT_fWSFollow         12
#define SHIFT_fNotSpecified     13
#define SHIFT_fDT               14
#define SHIFT_fID               15
#define SHIFT_fFinished         16
#define SHIFT_fTyped            17
#define SHIFT_valuetype         18
#define SHIFT_fAttribute        20
#define SHIFT_fNotQuiteEmpty    21
#define SHIFT_filler            22
#define SHIFT_nodetype          24
#define SHIFT_fReadOnly         29
#define SHIFT_fVariant          30
#define SHIFT_fDocument         31

                struct
                {
                    // flags, the first 16 bits is conflicting with the VARTYPE vt field in 
                    // the variant structure, add bits here which cannot be accessed in the
                    // typed value node 
                    unsigned    __fill              : 1;    // was attribute type
                    unsigned    _fDTName            : 1;    // element whose name is dt:*
                    unsigned    _fIDed              : 1;    // this node has an ID
                    unsigned    _appliedDt          : 1;    // datatype was applied via OM (not via DTD/Schema)
                    unsigned    _datatype           : 6;    // 'type' of datatype (stored on element/attribute which is typed)
                    unsigned    _fFloating          : 1;    // true if not part of the document
                    unsigned    _fWSInner           : 1;    // WhiteSpace hint
                    unsigned    _fWSFollow          : 1;    // WhiteSpace hint
                    unsigned    _fNotSpecified      : 1;    // wheather (attribute) ws specified (in the instance)
                                                            // also used to indicate that and entityref has been expanded
                    unsigned    _fDT                : 1;    // _fDT implies that the node has a datatype applied.
                    unsigned    _fID                : 1;    // this is of type ID

                    unsigned    _fFinished          : 1;    // true if completely parsed
                    unsigned    _fTyped             : 1;    // true if this (element/attr) has datatype data attached
                    unsigned    _valuetype          : 2;    // Node::ValueType
                    unsigned    _fAttribute         : 1;    // true iff nodeType == ATTRIBUTE (perf)
                    unsigned    _fNotQuiteEmpty     : 1;    // true if no content, but not an 'empty' tag.
                    unsigned    _filler             : 2;    // usused bits

                    // this must start above 16 bits !
                    unsigned    _nodetype           : 5;    // type of node (Node::NodeType)    
                    unsigned    _fReadOnly          : 1;    // true if node should be used read-only
                    unsigned    _fVariant           : 1;    // true if this holds the datatype info
                    unsigned    _fDocument          : 1;    // true iff nodeType == DOCUMENT (perf)

                };
                DWORD       _flags;
            };
    protected: union
            {
                // pointer to namedef object or typed value node
                NameDef *      _pName;
                Node *      _pTypedValue;
            };
    protected: union
            {
                struct
                {
                    // pointer to sibling, last points to first 
                    // or pointer to Document (iff _nodetype == DOCUMENT)
                    union {
                        Node *      _pSibling;
                    };

                    // pointer to data (last child, text, entity, etc.)
                    union
                    {
                        Node *      _pLast;     // last child/attr
                        AWCHAR *    _pText;     // text
                        Object *    _pValue;    // object attribute value
                    };
                };
                struct
                {
                    // contains variant data for typed valued nodes
                    union 
                    {
                        LONG lVal;
                        BYTE bVal;
                        SHORT iVal;
                        FLOAT fltVal;
                        DOUBLE dblVal;
                        VARIANT_BOOL boolVal;
                        _VARIANT_BOOL boolV;
                        SCODE scode;
                        CY cyVal;
                        DATE date;
                        BSTR bstrVal;
                        IUnknown __RPC_FAR *punkVal;
                        IDispatch __RPC_FAR *pdispVal;
                        SAFEARRAY __RPC_FAR *parray;
                        BYTE __RPC_FAR *pbVal;
                        SHORT __RPC_FAR *piVal;
                        LONG __RPC_FAR *plVal;
                        FLOAT __RPC_FAR *pfltVal;
                        DOUBLE __RPC_FAR *pdblVal;
                        VARIANT_BOOL __RPC_FAR *pboolVal;
                        _VARIANT_BOOL __RPC_FAR *pbool;
                        SCODE __RPC_FAR *pscode;
                        CY __RPC_FAR *pcyVal;
                        DATE __RPC_FAR *pdate;
                        BSTR __RPC_FAR *pbstrVal;
                        IUnknown __RPC_FAR *__RPC_FAR *ppunkVal;
                        IDispatch __RPC_FAR *__RPC_FAR *ppdispVal;
                        SAFEARRAY __RPC_FAR *__RPC_FAR *pparray;
                        VARIANT __RPC_FAR *pvarVal;
                        PVOID byref;
                        CHAR cVal;
                        USHORT uiVal;
                        ULONG ulVal;
                        INT intVal;
                        UINT uintVal;
                        DECIMAL __RPC_FAR *pdecVal;
                        CHAR __RPC_FAR *pcVal;
                        USHORT __RPC_FAR *puiVal;
                        ULONG __RPC_FAR *pulVal;
                        INT __RPC_FAR *pintVal;
                        UINT __RPC_FAR *puintVal;
                    } _variantData;
                };
            };


    public: ElementNode * getElementWrapper()
            {
                return this;
            }


    public: DOMNode * getDOMNodeWrapper(DOMNode * pParent, Document * pDoc = null)
            {
                return getDOMNodeWrapper();
            }

    public: DOMNode * getDOMNodeWrapper();


    public: bool isFinished()
            {
                return _fFinished;
            }
    public: void setFinished(bool flag)
            {
                _fFinished = flag;
            }

    public: void attach()
            {
                _addRef();
            }

    public: void detach()
            {
                _release();
            }

    public: bool isSpecified()
            {
                return !_fNotSpecified;                
            }

    public: void setSpecified(bool fSpecified)
            {
                _fNotSpecified = !fSpecified;
            }

    public: bool isFloating()
            {
                return (_fFloating != 0);
            }
    public: void floating( bool fFloat = true)
            {
                _fFloating = fFloat;
            }
    public: void notFloating()
            {
                _fFloating = false;
            }
    // walk the subtree (under the current node) and update the floating bit
    public: void setFloatingRec( bool fFloating);

    public: bool isReadOnly()
            {
                return (_fReadOnly != 0);
            }
    public: bool checkReadOnly();

    // Set Read-Only flag (recursively if fDeep == true)
    public: void setReadOnly(bool fRO, bool fDeep);

    public: unsigned isParent() const
            {
                return (getValueType() == VAL_PARENT);
            }

    public: unsigned allowChildren() const;
    public: unsigned allowAttributes() const;

    public: ValueType getValueType() const
            {
                return (ValueType) _valuetype;
            }

    private: bool setValueType( ValueType eValueType);

    public: unsigned isEmpty()
            {
                return !isCollapsedText() && !getNodeLastChild();
            }

    public: unsigned isTyped()
            {
                return _fDT;
            }
    public: DataType getNodeDataType()
            { 
                return (DataType) _datatype;
            }

    public: Element::NodeType getNodeType() const
            { 
                return (Element::NodeType) _nodetype; 
            }
    public: String * getNodeTypeAsString();
    public: static String * NodeTypeAsString(NodeType eType);


    public: Name * getName() const;

    public: NameDef * _getNameDef() const;

    // inherited from Element
    public: virtual NameDef * getNameDef() { return _getNameDef(); }

    public: void setName(NameDef *);

    public: Node * getSibling() const
            {
                Assert( !_fDocument);
                return _pSibling;
            }
    public: Node * getNextSibling();
    public: Node * getPrevSibling();

    public: void setSibling( Node * pNode)
            {
                Assert( !_fDocument);
                _pSibling = pNode;
            }

    public: bool getWSInner()
            {
                return _fWSInner;
            }

    public: void setWSInner( bool f = true)
            {
                _fWSInner = f;
            }

    public: bool getWSFollow()
            {
                return _fWSFollow;
            }

    public: void setWSFollow( bool f = true)
            {
                _fWSFollow = f;
            }

    public: bool getNotQuiteEmpty()
            {
                return _fNotQuiteEmpty;
            }

    public: void setNotQuiteEmpty(bool f = true)
            {
                _fNotQuiteEmpty = f;
            }

    public: Node * find(Name * pName, Element::NodeType nt, Document * pDoc = null);
    public: Node * find(Atom * pBaseName, Atom * pPrefix, Element::NodeType nt, Document * pDoc = null);
    public: Node * findByNodeName(const WCHAR * pName, Element::NodeType nt, Document * pDoc = null);
    public: Node * findByNameDef(NameDef * pNameDef, Element::NodeType nt, Document * pDoc = null);

    public: Node * findChild(int index);

    /**
     * Add to end if pBefore is null, insert before otherwise.
     */
    public: void _insert(Node * pChild, Node * pBefore);
    public: void _append(Node * pChild, Node **ppLast); // pChild must be already removed
    public: void _remove(Node * pChild);

    // used to update tree during moveNode calls
    private: static void ValidateAndUpdate(Node * pNewParent, Node * pNewChild);
    private: void validateAndUpdateRec(NodeInsertInfo * pInfo, ElementDecl* pED);
    private: void insertUpdateAbort(Node * pErrorNode, NodeInsertInfo * pInfo, ElementDecl* pED);

    public:
    void moveNode(Node * pChild, Node * pRef, Node * pRemove, bool fAttr = false, bool fNotify = true);
    void insertNode(Node * pChild, Node * pRef, bool fAttr = false) 
    { 
        moveNode(pChild, pRef, null, fAttr); 
    }
    inline void insertAttr( Node * pChild)
    {
        moveNode(pChild, null, null, true); 
    }
    void replaceNode(Node * pNew, Node * pOld, bool fAttr = false) 
    { 
        moveNode(pNew, pOld, pOld, fAttr); 
    }
    inline void replaceAttr(Node * pNew, Node * pOld)
    {
        moveNode(pNew, pOld, pOld, true); 
    }
    void removeNode(Node * pChild, bool fAttr = false) 
    { 
        moveNode(null, null, pChild, fAttr); 
    }

    protected: void deleteChildren(bool fExtNotifications, bool fIntNotifications = true);

    public:
    // returns true if this has pFindNode anywhere in it's ancestor(parent) chain
    bool hasAncestor(const Node * pFindNode);

    Node * getAncestorWithNodeType(Element::NodeType nodeType, bool fNot=FALSE );

    bool validateHandle(void ** ppv) const 
    { 
        Assert(ppv);
        Node * pNode = (Node *)*ppv;
        Assert(!pNode|| CHECKTYPEID(*pNode, Node));
        return (!pNode || this == pNode->_pParent);
    }
    Node * getFirstNode(void ** ppv);
    Node * getFirstNodeNoExpand(void ** ppv) const; // also does NOT expand single PCDATA child nodes
    Node * getNextNode(void ** ppv) const;

    // enumerate children
    Node * getNodeFirstChild(void ** ppTag);
    Node * getNodeNextChild(void ** ppTag) const;
    Node * getNextMatchingChild(void **, Name * tag);
    Node * getNodeLastChild();


    long getChildIndex(Node *pNodeChild, Name *pName, NodeType type);
    long getIndex(bool fRelative); 
    
    // attribute helpers
    void setNodeAttributeType(DataType attributetype);
    DataType getNodeAttributeType()
    {
        return (DataType)_datatype;
    }

    Object * getNodeAttribute(Name * pName);

    void setNodeAttribute(Name * pName, Object * pValue, Atom * pPrefix, Node ** ppNode = null)
    {
        setNodeAttribute(pName, null, pValue, pPrefix, ppNode);
    }
    void setNodeAttribute(Name * pName, const BSTR bstrName, Object * pValue, Atom * pPrefix, Node ** ppNode = null);

    void removeAttribute(Name * pName, Node ** ppNode);

    // enumerate attributes
    Node * getNodeFirstAttribute(void ** ppTag);
    Node * getNodeNextAttribute(void ** ppTag);

    Node * getNodeFirstAttributeWithDefault(void ** ppTag);
    Node * getNodeNextAttributeWithDefault(void ** ppTag);
    Node * getNextMatchingAttribute(void **, Name * tag);

    protected: Node * getFirstDefaultAttribute(void ** ppTag);


    public: // text manipulation

    // was xml:space specified? if so, how so?
    bool getXmlSpace(bool * pfPreserve);
    // is xml:space delcared as 'preserve' on this element?
    bool xmlSpacePreserve() const;
    // whether to ignore xmlspace alltogether (when preserveWhiteSpace is true).
    bool ignoreXMLSpace() const;

    // doesn't decend into children
    String * getTextString();
    const AWCHAR * getNodeText();
    String * getDOMNodeValue() { return getInnerText(true, true, true); }

    // concatinate content of all children
    String * getInnerText(bool fPreserve, bool fIgnoreXmlSpace, bool fNormalize );
    inline String * getInnerText()
    {
        return getInnerText(xmlSpacePreserve(), ignoreXMLSpace(), true);
    };
    Name * getContentAsName(bool fValidate = false, String * pContent = null);

    protected: void addText(WSStringBuffer * psb, bool fPreserve, bool fIgnoreXmlSpace, bool fIE4);
    // private version of addText used internally by notify()
    protected: String * _dtText(WSStringBuffer * psb);
    protected: String * _dtText(const TCHAR *, int len, 
                                Node * pNewNode, 
                                Node * pInsertBefore, Node * pSkip, 
                                WSStringBuffer * psb);

    public: 
    void setInnerText(const WCHAR * pwch, bool fNoChild = false) { setInnerText(pwch, _tcslen(pwch), fNoChild); }
    void setInnerText(const WCHAR * pwch, int cch, bool fNoChild = false);
    void setInnerText(const AWCHAR * pS, bool fNoChild = false);
    void setInnerText(String * pS, bool fNoChild = false);

    // no notifications
    void _setText(AWCHAR * pText) { Assert(_valuetype == VAL_STR); assign(&_pText, pText); }
    // specially optimized function for load/tree-builder 
    void _appendText(const WCHAR * pText, int length);

#define COLLAPSED_TEXT_BIT 0x1

    // single text node optimization (no notification)
    public: void setCollapsedText(AWCHAR* pText) { Assert(_pText == null); _valuetype = VAL_PARENT; assign(&_pText, pText); _pText = BitSetPointer(_pText,COLLAPSED_TEXT_BIT); }
    // used to uncollapse the single text node.
    private: void uncollapse();

    public: bool isCollapsedText() const
            {
                return ((LONG_PTR)_pText & COLLAPSED_TEXT_BIT) != 0;
            }

    public: AWCHAR* getCollapsedText()
            {
                AWCHAR* pText = _pText;
                if (((LONG_PTR)pText & COLLAPSED_TEXT_BIT) != 0)
                {
                    return (AWCHAR*)((LONG_PTR)pText & ~COLLAPSED_TEXT_BIT);
                }
                return null;
            }

    private: inline AWCHAR* BitSetPointer(AWCHAR* pText, int bit) { return (AWCHAR*)((LONG_PTR)pText | bit); }
    private: inline AWCHAR* MaskPointer(AWCHAR* pText, int bit) { return (AWCHAR*)((LONG_PTR)pText & ~bit); }

    public: AWCHAR* orphanText();

    // value manipulation
    Object * getNodeValue();
    void setNodeValue(Object *);

    ////////////
    ///// Schema Information
    public: Node* getDefinition();
    public: ElementDecl* getElementDecl(DTD * pDTD = null);
    public: AttDef* getAttDef(ElementDecl * pED = null, DTD * pDTD = null, Node * pParent = null);

    ////////////
    ///// Simple XQL queries.  (faster than going through IXQLNodeList).
    public: Node* selectSingleNode(String* query);

    ////////////
    ///// Save

    String * getXML();
    String * getAttributesXML();

    void save(Document * pDoc, OutputHelper * o, NamespaceMgr * pNSStack = null);
    void saveAttributes(Document * pDoc, OutputHelper * o, NamespaceMgr * pNSStack = null);
    void saveQuotedValue(OutputHelper* o);

    ////////////
    ///// Clone

    // if deep is true, clone node, attributes and children
    // otherwise, just copy the node and its attributes
    Node* clone(bool deep, bool fReadOnly, Document* pDoc, 
                NodeManager* pManager, bool fWholeDoc = false); 
    Node* cloneChildren(bool deep, bool attrs, bool fReadOnly,
                        Document* pDoc, NodeManager* pManager, 
                        Node *pParent, bool fWholeDoc = false); 
    protected: Node* _clone(bool fReadOnly, Document* pDoc, NodeManager* pManager);

    public:
    // For EntityRef
    Node * resolveEntityRef();
    // clone the children of the entity node this entity reference refers to
    void _expandEntityRef(void);

    // merge adjacent text nodes that have the same parent into one node 
    // but only untyped PCDATA nodes, and NOT attribute values.
    void normalize(void);
    protected: void _normalize(void);

    public:
    // helper returning the local to be used for this node
    LCID getLocale();

    // data type manipulation
    void getNodeTypedValue(VARIANT * pVar);
    void getNodeTypedValue(DataType dt, VARIANT * pVar);

    // converts passed VARIANT to appropriate internal VARIANT datatype storage type
    void setNodeTypedValue(VARIANT * pVar);

    // parse the contents and set the typed value appropriately
    void parseAndSetTypedData(DataType eDataType, bool fParsing, 
                              bool fInsideEntity = false,
                              String * psContent = null,
                              Node * pParent = null);
    // parse/validate text content into VARIANT
    void parseTypedContent(Document *, String * pString, DataType dt, VARIANT * pVar, bool fParsing);
    // create data type node
    void setNodeDataType(DataType dt, VARIANT * pVar);
    // remove data type node (if a Node handle is passed in, it is up to the caller to delete the Node)
    void removeDataType();
    // throw an error if we have a bad datatype
    void testDataType();

    // create content nodes (innerText) from data type node
    protected: void createContent(VARIANT * pVar);
    // copy in and out variant data into a variant
    protected: void clearVariantData();
    protected: void toVariant(/*[OUT]*/ VARIANT * pVar);
    protected: void fromVariant(/*[IN]*/ VARIANT * pVar);

    friend class ElementNodeLock;

    /************************************************************************* 
     * Element methods and members
     *************************************************************************/

    _DECLARE_CLASS_MEMBERS_NOQI(Node, Element);
    DECLARE_CLASS_INSTANCE(Node, Element);

    public: virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
    // the core impl of QI for all OM COM classes
    public: HRESULT QIHelper(DOMDocumentWrapper * pDOMDoc, DOMNode * pDOMNode, 
                             IDocumentWrapper * pIE4Doc, IDispatch * pIE4Node, 
                             REFIID iid, void **ppv);

    public: virtual ULONG STDMETHODCALLTYPE AddRef();
    public: virtual ULONG STDMETHODCALLTYPE Release();
    public: ULONG _addRef();
    public: ULONG _release();

    protected: virtual void finalize();

    public: Node * getNodeData()
            {
               return this;
            }

    /** 
     * Retrieves the parent of this element skipping EntityRef's. 
     * Every element in the tree except the Document itself, has
     * a parent.  
     *
     * @return the parent element or null if at the root of 
     * the tree.
     */
    public: virtual Element * getParent();

    /** 
     * Retrieves the name of the tag as a string. 
     * 
     * @return the tag name or null for DATA and PCDATA elements. 
     */
    public: virtual Name * getTagName();

    /** 
     * Set the name of the tag as a string. 
     */
    public: virtual void setTagName(String* name);

    /**
     * Retrieves the type of the element.
     * This is always one of the following values:
     * <code>DOCUMENT</code>, <code>ELEMENT</code>, <code>PCDATA</code>, <code>PI</code>,
     * <code>META</code>, <code>COMMENT</code>, or <code>CDATA</code>.
     * 
     * @return element type.
     */    
    public: virtual int getType();

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
    public: virtual String * getText(bool fForceCollapse = false, bool fNormalize = false);
   
    /**
     * Sets the text for this element. Only meaningful in 
     * <code>CDATA</code>, <code>PCDATA</code>, and <code>COMMENT</code> nodes.
     *
     * @param text The text to set.
     * @return No return value.
     */    
    public: virtual void setText(String * text);

    /**
     * Returns the first child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getFirstChild(HANDLE * ph);

    /**
     * Returns the next child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getNextChild(HANDLE * ph);

    /**
     * Retrieves the number of child elements.
     * @return the number of child elements.
     */
    public: virtual int numElements();

    public: virtual int hasChildren();

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
    public: virtual void addChild(Element * elem, Element * eBefore);

    /**
     * Adds a child to this element. 
     * @param elem The element to add.
     * @param pos  The position to add this element (calling <code>getChild(pos)</code> 
     * will return this element). If <i>pos</i> is less than 0, <i>elem</i> becomes 
     * the new last element.
     * @param reserved The reserved parameter.
     * @return No return value.
     */
    public: virtual void addChildAt(Element * elem, int pos);

    /**
    /**
     * Removes a child element from the tree.
     *
     * @param elem  The element to remove.
     */   
    public: virtual void removeChild(Element * elem);
    
    /**
     * Returns the first child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getFirstAttribute(HANDLE * ph);

    /**
     * Returns the next child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getNextAttribute(HANDLE * ph);

    /**
     * Retrieves the number of attributes.
     * @return the number of attributes.
     */
    public: virtual int numAttributes();

    /**
     * Retrieves an attribute's value given its name.
     * @param name The name of the attribute.
     * @return the value of the attribute 
     * or null if the attribute is not found.
     */    
    public: virtual Object * getAttribute(Name * n);
    
    /**
     * Sets the attribute of this element.    
     *
     * @param name  The attribute name.
     * @param value The attribute value.
     */
    public: virtual void setAttribute(Name * name, Object * value);

    /**
     * Deletes an attribute from an element.
     * @param name The attribute to delete.
     * @return No return value.
     */    
    public: virtual void removeAttribute(Name * name);

    /**
     * Gets typed value into the variant.
     */
    public: virtual void getTypedValue(VARIANT * pVar);

    public: virtual void getTypedValue(DataType dt, VARIANT * pVar);

    /**
     * Sets typed value.
     */
    public: virtual void setTypedValue(VARIANT * pVar);

    /**
     * Gets typed attribute into the variant.
     */
    public: virtual void getTypedAttribute(Name * n, VARIANT * pVar);

    /**
     * Sets typed attribute from the variant.
     */
    public: virtual void setTypedAttribute(Name * n, VARIANT * pVar);

    /**
     * Gets data type string.
     */
    public: String * getDataTypeString();

    /**
     * Sets data type string.
     */
    public: void setDataTypeString(String * pS);

    /**
     * Return the value.
     */
    public: virtual Object * getValue()
            {
                return getNodeData()->getNodeValue();
            }

    /**
     * Used by Attributes, private to this package
     */
    public: void setValue(Object * o)
            {
                getNodeData()->setNodeValue(o);
            }


    /**
     * Return the type.
     */
    public: virtual DataType getAttributeType()
            {
                return getNodeAttributeType();
            }

    public: virtual bool hasWSInside();

    public: virtual bool hasWSAfter();


    /**
    * Functions for comparing nodes and operands
    */
    public: virtual TriState compare(OperandValue::RelOp op, DataType dt, OperandValue * popval);

    public: virtual int compare(DWORD dwCmpFlags, DataType dt, OperandValue * popval, int * presult);

    public: virtual void getValue(DataType dt, OperandValue * popval);

    public: virtual DataType getDataType() { return getNodeDataType(); }

    protected: void setAttribute(Name * name, Object * value, Document * pDocument, Atom * pPrefix);

    public: inline Document * getNodeDocument() const
    { 
        Assert( ((Document *)_pDocument) != null); 
        return _pDocument; 
    }

    public: virtual Document * getDocument()
    {
        return getNodeDocument();
    }

    private:   DOMNode * getDOMNode() { return getDOMNodeWrapper(); }

    protected: ElementNode * _pParent;
    public:    ElementNode * getNodeParent() { return _pParent; }

    private:   void checkFinished();

    protected: WDocument _pDocument;
};

#ifdef DEBUGRENTAL
#define NODE_SIZE WHEN_NOT_DBG(40) WHEN_DBG(48)
#else
#define NODE_SIZE WHEN_NOT_DBG(32) WHEN_DBG(40)
#endif
COMPILE_TIME_ASSERT(Node, NODE_SIZE);

#endif _XML_OM_NODE_HXX
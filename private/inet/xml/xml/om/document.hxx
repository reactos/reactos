/*
 * @(#)Document.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_DOCUMENT
#define _XML_OM_DOCUMENT

#ifndef _XML_OM_NODE
#include "xml/om/node.hxx"
#endif

#ifndef _XML_OM_DOMNODE
#include "xml/om/domnode.hxx"
#endif

#ifndef _SAFECTRL_HXX
#include "safectrl.hxx"
#endif

#ifndef _XML_OM_IOLECOMMANDTARGET
#include "xml/om/iolecommandtarget.hxx"
#endif

#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

#ifndef _EVENTHELP_HXX_
#include "xml/om/eventhelp.hxx"
#endif

#include "xmldocnf.h"

#define XMLNF_STARTSCHEMA (XMLNF_LASTEVENT + 1)
#define XMLNF_ENDSCHEMA   (XMLNF_LASTEVENT + 2)


DEFINE_CLASS(NameDef);

#if 1
// Note: Keep both implementations of LookasideCache around!!
//  the only reason for this is because the _array<> version is too large.
//  this will be looked into and hopefully resolved.
class NOVTABLE LookasideCache
{
private:
    int _count;
    long _length;
    DWORD * _pCache;
    ULONG_PTR _ul;

public:
    LookasideCache() : _count(0), _length(0), _pCache(null), _ul(0) {}
    ~LookasideCache() { delete [] _pCache; }

    HRESULT add(DWORD dwKey);
    void remove(DWORD dwKey);
    bool search(DWORD dwKey);
    int count() const { return _count; }
};
#else
class NOVTABLE LookasideCache
{
private:
    int _count;
    _reference<_array<DWORD> > _pCache;
    ULONG_PTR _ul;

public:
    HRESULT add(DWORD dwKey);
    void remove(DWORD dwKey);
    bool search(DWORD dwKey);
    int count() const { return _count; }
};
#endif

struct ParserFlags
{
    bool    fCaseInsensitive;            // names are not case sensitive
    bool    fIgnoreDTD;                  // do not process DTD
    bool    fOmitWhiteSpaceElements;     // do not create white space nodes
    bool    fParseNamespaces;           // whether allow namespaces, initalized to true
    bool    fShortEndTags;
    bool    fIE4Compatibility;
    bool    fValidateOnParse;
    bool    fResolveExternals;

    ParserFlags() : fCaseInsensitive(false), fIgnoreDTD(false), 
        fOmitWhiteSpaceElements(false), fParseNamespaces(true),
        fShortEndTags(false), fIE4Compatibility(false), fValidateOnParse(true), fResolveExternals(true) {};
};

#ifndef _ENTITY_HXX
#include "xml/dtd/entity.hxx"
#endif 

#ifndef _CORE_UTIL_TASK
#include "core/util/task.hxx"
#endif

struct IXMLParser;
typedef _reference<IXMLParser> RXMLParser;
typedef _reference<IXMLNodeFactory> RNodeFactory;
typedef _reference<IStream> RStream;

// multithread safe references.
typedef _reference<IMoniker> RMoniker;
typedef _gitpointer<IDispatch,&IID_IDispatch> GITDispatch; 

#include "namespacemgr.hxx"

DEFINE_STRUCT(IMoniker);
DEFINE_STRUCT(IBindCtx);
DEFINE_CLASS(DTD);
DEFINE_CLASS(NameSpaceNodeFactory);
DEFINE_CLASS(SchemaNodeFactory);
class NodeDataNodeFactory;
class OutputHelper;


/**
 * This class implements an XML document, which can be thought of as the root of a tree.
 * Each XML tag can either represent a node or a leaf of this tree.  
 * The <code>Document</code> class allows you to load an XML document, manipulate it, 
 * and then save it back out again. The document can be loaded by specifying 
 * a URL or an input stream. 
 * <P>
 * According to the XML specification, the root of the tree consists of any combination
 * of comments and processing instructions, but only one root element.
 * A helper method <code>getRoot</code> is provided
 * as a short cut to finding the root element.
 *
 * @version 1.0, 6/3/97
 * @see Element
 */


const IID IID_Document = {0xB5B359F0,0x53DC,0x11d1,{0x8C, 0x6A,0x80,0xBA,0xB4,0x00,0x00,0x00}};
class DECLSPEC_UUID("B5B359F0-53DC-11d1-8C6A-80BAB4000000")
Document : public CSafeControl, 
           public PersistStream,
           public PersistMoniker,
           public OleCommandTarget
{
    friend class NodeDataNodeFactory;
    friend class NameSpaceNodeFactory;
    friend class Node;
    friend class DOMNode;

    DECLARE_CLASS_MEMBERS_NOQIADDREF_I3(Document, CSafeControl, PersistStream, PersistMoniker, OleCommandTarget);
    DECLARE_CLASS_INSTANCE(Document, CSafeControl);

    public: HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
    public: ULONG STDMETHODCALLTYPE AddRef();
    public: ULONG STDMETHODCALLTYPE Release();
    public: ULONG _addRef();
    public: ULONG _release();

    // used to implement QueryInterface.  Only called by Node::QIHelper!!!
    public: virtual HRESULT QIHelper(DOMDocumentWrapper * pDOMDoc, IDocumentWrapper * pIE4Doc, REFIID iid, void **ppv);
    
    private:
        HRESULT FireOnReadyStateChange();
        HRESULT FireOnDataAvailable();

    public: ULONG_PTR * getSpinLockCPC() { return &_lSpinLockCPC; }
    
    public: Mutex * getMutex()
            {
                Assert(_pMutex != null);
                return (Mutex*)_pMutex;
            }
    public: Mutex * getMutexNonReentrant(DWORD dwTID);

    // So you can mark the document non-re-entrant for this thread
    // for write operations.
    public: DWORD registerNonReentrant();
    public: void removeNonReentrant(DWORD = 0);

    // You can also makr the document read only so that it cannot be
    // modified through the DOM at all (e.g. schema documents).
    public: void setReadOnly(bool readonly) { _fReadOnly = readonly; }
    public: bool isReadOnly() { return _fReadOnly; }

    public: void enterDOMLoadLock();
    public: void leaveDOMLoadLock(HRESULT hr);

    /**
     * Construct a new empty document and use the default
     * element factory.
     */
    public: Document();
    protected: ~Document();

public:
    // A shallow clone of a document gives you a new empty document in
    // the same state as the original. Same safety options, same site, 
    // same base URLs, same DOM mode, same parser flags.  
    public: virtual Document * clone(bool deep); 

    Node * createNode(Element::NodeType eType, const BSTR name, const BSTR bstrNameSpace, bool fHasNS);

    void loadXML( String * pXML);

    void save( String * Url,
               String * Encoding);

    HRESULT STDMETHODCALLTYPE putOnReadyStateChange(IDispatch *pdisp);
    HRESULT STDMETHODCALLTYPE putOnDataAvailable(IDispatch *pdisp);
    HRESULT STDMETHODCALLTYPE putOnTransformNode(IDispatch *pdisp);
    IDispatch *getTransformNodeSink() { return _pdispOnTransformNode; }


    virtual void onStartDocument();
    virtual void onEndProlog();

    /**
     * Set the given element factory for use in the next load.
     * The old factory is returned.
     */
    public: void setFactory(IXMLNodeFactory * f);
    public: IXMLNodeFactory* getFactory() { return _pFactory; }

    public: void initDefaultFactory(Document * pSchemaDTD = null, Atom * pSchemaURN = null);


    /**  
     * Retrieves the root node of the XML parse tree. This is
     * guaranteed to be of type <code>Element.ELEMENT</code>.
     * @return the root node.
     */
    public: Node * getRoot();

    public: void setRoot(Node* e);


    /**
     * Retrieves the ElementNode which is the root for the tree (type DOCUMENT)
     */
    public: Element * getDocElem()
            {
                return (Element *)_pDocNode;
            }

    public: Node* parseXMLDecl(const WCHAR* attrs);

    /**
     * Returns the information stored in the &lt;?XML ...?&gt; tag
     * as an Element.  Typically this has two attributes
     * named VERSION and ENCODING.
     * It is a private: virtual function because users should not be able to mess around
     * with the XML element.  Set/getVersion and set/getEncoding are available 
     * for those purposes.
     */
    public: Element * getXML( bool fBuildIt = false);

    /**
     * Retrieves the version information.
     * @return the version number stored in the &lt;?XML ...?&gt; tag.
     */
    public: String * getVersion();

    /**
     * Sets the version number stored in the &lt;?XML ...?&gt; tag.      
     * @param version The version information to set.
     * @return No return value.
     */
    public: void setVersion( String * version );

    /**
     * Retrieves the character encoding information.
     * @return the encoding information  stored in the &lt;?XML ...?&gt; tag or
     * the user-defined output encoding if it has been more recently set.
     */
    public: String * getEncoding();

    /**
     * Sets the character encoding for output. Eventually it sets the ENCODING 
     * stored in the &lt;?XML ...?&gt; tag, but not until the document is saved.
     * You should not call this method until the Document has been loaded.
      * @return No return value.
     */
    public: void setEncoding( String * charset );

    /**
     * Retrieves the standalone attribute stored in the &lt;?XML ...?&gt; tag. 
     * @return the standalone attribute value.
     */
    public: String* getStandalone();

    /**
     * Sets the Standalone attribute stored in the &lt;?XML ...?&gt; tag.      
     * @param standalone The value to set.
     * @return No return value.
     */
    public: void setStandalone( String* standalone );

    /**
     * Returns the name specified in the &lt;!DOCTYPE&gt; tag.
     */
    public: NameDef * getDocType();

    /**
     * Retrieves the document type URL.
     * @return the URL specified in the &lt;!DOCTYPE&gt; tag or null 
     * if an internal DTD was specified.
     */
    public: String * getDTDURL();

    /**
     * Retrieves the URL.
     * @return the last URL sent to the load() method  or null 
     * if an input stream was used.
     */
    public: String * getURL();

    // Return the node for the given ID.
    Node* nodeFromID(Name* name);

    /**
     * Retrieves the last modified date on the source of the URL.
     * @return the modified date.
     */
    public: virtual int64 getFileModifiedDate();

    /**
     * Retrieves the external identifier. 
     * @return the external identifier specified in the &lt;!DOCTYPE&gt; tag 
     * or null if no &lt;!DOCTYPE&gt; tag was specified. 
     */
    public: String * getId();

    /**
     * Creates a new element for the given element type and tag name using
     * the <code>ElementFactory</code> for this <code>Document</code>. 
     * This method allows the <code>Document</code> class to be used as an 
     * <code>ElementFactory</code> itself.
     * @param type The element type.
     * @param tag The element tag.
     */    
    public: Element * createElement(Element* parent, int type, NameDef * tag, String* text);

    /** 
     * Switch to determine whether next XML loaded will be treated
     * as case sensitive or not.  If not, names are folded to uppercase.
     */
    public: void setCaseInsensitive(bool yes)
    {
        _uParserFlags.fCaseInsensitive = yes;
    }

    /**
     * Return whether we're case insensitive.
     */
    public: bool isCaseInsensitive()
    {
        return _uParserFlags.fCaseInsensitive;
    }

    /** 
     * Switch to determine whether the DTD (if present) is processed.
     */
    public: void setIgnoreDTD(bool yes)
    {
        _uParserFlags.fIgnoreDTD = yes;
    }

    public: bool getIgnoreDTD()
    {
        return _uParserFlags.fIgnoreDTD;
    }

    public: void setDOM(bool yes);

    public: bool isDOM()
    {
        return _fDOM;
    }

    public: bool setParseNamespaces(bool yes)
    {
        return _uParserFlags.fParseNamespaces = yes;
    }

    public: bool getParseNamespaces()
    {
        return _uParserFlags.fParseNamespaces; 
    }

    /** 
     * Switch to determine whether white space nodes are created.
     */
    public: void setOmitWhiteSpaceElements(bool yes)
    {
        _uParserFlags.fOmitWhiteSpaceElements = yes;
    }

    /**
     * Return whether we're omitting white space nodes.
     */
    public: bool getOmitWhiteSpaceElements()
    {
        return _uParserFlags.fOmitWhiteSpaceElements;
    }

    public: void setShortEndTags(bool yes)
    {
        _uParserFlags.fShortEndTags = yes;
    }

    public: bool getShortEndTags()
    {
        return _uParserFlags.fShortEndTags;
    }

    public: void setIe4Compatibility(bool yes)
    {
        _uParserFlags.fIE4Compatibility = yes;
    }

    public: bool getIe4Compatibility()
    {
        return _uParserFlags.fIE4Compatibility;
    }

    // Validating flag
    public: bool getValidateOnParse() const
    {
        return _uParserFlags.fValidateOnParse;
    }

    public: void setValidateOnParse(bool fValidateOnParse)
    {
        _uParserFlags.fValidateOnParse = fValidateOnParse;
    }

    public: ParserFlags getParserFlags() const
    {
        return _uParserFlags; 
    }

    public: bool getResolveExternals() const
    {
        return _uParserFlags.fResolveExternals;
    }

    public: void setResolveExternals(bool fResolveExternals)
    {
        _uParserFlags.fResolveExternals = fResolveExternals;
    }    

    public: bool getPreserveWhiteSpace()
    {
        return _fPreserveWS;
    }

    public: void setPreserveWhiteSpace(bool fPreserveWhiteSpace)
    {
        _fPreserveWS = fPreserveWhiteSpace;
    }

    /**
     * Loads the document from the given URL string.
     * @param urlstr The URL specifying the address of the document.
     * @exception Exception if the file contains errors.
     * @return No return value.
     */
    public: virtual void load(String * urlstr, bool async = false); //throws Exception

    /**
     * An alias for load and is here for compatibility
     * reasons only. 
     * @param urlstr The URL string.
     * 
     * @return No return value.
     * @exception Exception if an error occurs while parsing the URL string.
     * 
     */
    public: virtual void setURL( String * urlstr, bool async = false ) //throws Exception
    {
        load(urlstr, async);
    }

    /**
     * PersistStream interface
     * @param in The input stream.
     * @return No return value.
     * @exception Exception when a syntax error is found.
     */
    public: virtual void Load(IStream * in);

    public: virtual void load(bool fFullyAvailable, IMoniker *pmk, LPBC pbc, DWORD grfMode);

    private: void _load(bool async, IMoniker *pmk = NULL, LPBC pbc = NULL, IStream* pStm = NULL);

    /**
     * Sets the style for writing to an output stream.
     * Use XMLOutputStream.PRETTY or .COMPACT.
     * 
     * @param int The style to set.
     * @return No return value.
     * @see XMLOutputStream
     */
    public: virtual void setOutputStyle(int style)
    {
        _dwOutputStyle = style;
    }

    /**
     * Retrieves the current output style.  
     * @return the output style. The default style is XMLOutputStream.PRETTY.
     */
    public: virtual int getOutputStyle()
    {
        return _dwOutputStyle;
    }

    /**
     * Creates an IStream wrapper that does the right encoding and provides
     * helper functionality for generating well formed XML. 
     * 
     * @exception IOException if the output stream cannot be created.
     * @return the XML output stream.
     */
    public: OutputHelper* createOutput(IStream * out, String* s = null);

    /**
     * Saves the document to the given output stream.
     * @param o  The output stream.
     * @return No return value.
     * @exception IOException if there is a problem saving the output.
     */
    public: virtual void save(OutputHelper * o); 

    /**
     * PersistStream interface.
     * @param o  The output stream.
     * @return No return value.
     * @exception IOException if there is a problem saving the output.
     */
    public: virtual void Save(IStream * o); //throws IOException * * 

    public: void cleanUp();

    // This method checks to see if a load is aleady happening and if so
    // it resets the parser to make sure the second load succeeds properly.
    public: void reset();

    public: void setLastError(Exception * e);

    public: HRESULT GetLastError();

    public: virtual HRESULT run();

    public: virtual void abort(Exception * e);

    /**
     * Sets the Document back to its initial empty state retaining 
     * only the <code>ElementFactory</code> association.
     * @return No return value.
     */
    public: virtual void clear();

    /**
     * Sets the document load state
     */
    public: virtual void setReadyStatus(int state);

    public: void setAsync(bool async)
    {
        this->_fAsync = async;
    }

    public: bool isAsync()
    {
        return this->_fAsync;
    }

    public: GUID getClassID();
            
    public: virtual void getCurMoniker(IMoniker** ppimkName);

    public: DTD* getDTD();

    public: Element* getDTDNode();

    /**
     *  ------------------ Error info ------------------------------
     *
     * Implementation of the XMLError interface.
    */
    public: Exception * getErrorMsg()
            {
                return _pParseError;
            }

    // Implementation of the OleCommandTarget interface.
    public: virtual void queryStatus(const GUID *pguidCmdGroup,
                                    ULONG cCmds,
                                    OLECMD prgCmds[],
                                    OLECMDTEXT *pCmdText);

    public: virtual void exec(const GUID *pguidCmdGroup,
                                DWORD nCmdID,
                                DWORD nCmdexecopt,
                                VARIANT *pvaIn,
                                VARIANT *pvaOut);

    public: NamespaceMgr* getNamespaceMgr()
            {
                if (!_pNamespaceMgr)
                {
                    _pNamespaceMgr = NamespaceMgr::newNamespaceMgr(true);
                }
                return _pNamespaceMgr;
            }

    // return the ready status
    public: int getReadyStatus()
            {
                return _dwLoadState;
            }

    // Notifications
    public: virtual void onDataAvailable();

    /**
     * Root of element tree.
     */
    public: Node* getDocNode()
            {
                Assert(_pDocNode);
                return _pDocNode;
            }
    public: void _clearDocNode();

    protected: void _createDocNode();

    protected: void reportObjects();
    protected: void newNodeMgr(NodeManager**);
    public: NodeManager * getNodeMgr()
               {
                   Assert( (NodeManager*)_pNodeMgr);
                   return _pNodeMgr;
               }
    public: NodeManager * getAltNodeMgr();

    public: void getParser(IXMLParser** ppParser);    // also assigns parser member variable.
    
    public: void setFilterSchemaDoc(bool FilterSchemaDoc) { _fFilterSchemaDoc=FilterSchemaDoc; }
//    public: bool getFilterSchemaDoc() { return _fFilterSchemaDoc; }     not needed

    static String* getAttributeIgnoreCase(Node* node, Name* name);

    // BUILT IN ENTITIES MOVED TO TOKENIZER.

    public: static void classInit()
            {
            }
    public: static void classExit();

    protected: virtual void finalize();

    private: HRESULT newParser(IXMLParser** ppParser);    

    // Throws exception if not allowed.
    public: void CheckOpenAllowed(String* url);

    public: virtual void NotifyListener(XMLNotifyReason, XMLNotifyPhase,
                                        Node *pNode, Node *pNodeParent,
                                        Node *pNodeBefore=NULL)
            { }
    
    public: void HandleEndDocument(); // XMLNF_ENDDOCUMENT notification.
    private: void HandleParseError(IXMLNodeSource* pParser);
    private: Exception* createException(IXMLNodeSource* pParser, HRESULT hr);

    //---------------- Child Documents --------------------------------------------
    // A child document is a new Document object setup to load the given URL and 
    // added to the _pChildDocs vector for later processing via startNextPending.  
    Document* createChildDoc(const WCHAR* pwcBaseURL, Atom* pURL, ParserFlags flags);

    // Return whether current document has any pending child documents.
    // If so the parent document should return E_PENDING to the parser and
    // should not call Run on the parser until the child documents are ready.
    bool hasPendingChildDocs() const;

    // this method calls "load" on the next pending child document and returns
    // true if the child is still loading asynchronously when it returns.
    // If it returns true the caller should reutrn E_PENDING its the parser.
    // because it means this document is suspended and the child document 
    // will call childFinished when it's done.  (This is used for loading schemas).
    bool startNextPending();

    // This method is called by the child document when it is finished loading.
    // The parent document will then continue on (if it is not waiting for any
    // other child documents to finish).  If the child ran into an error, the
    // parent will adopt the error from the child so it bubbles back up to
    // the Object Model level.
    void childFinished(Document* child);

    // Move a child document from a parent down to this document.
    // This happens if another child document is also dependent on the given
    // child and cannot proceed until it is complete.
    void adoptChild(Document* child);

    bool isPending() { return _fPending; }

    // Schema URN's.  This is so that when you are inside a schema document
    // you can find out what URN it represents (i.e. the URN that was in
    // the original xmlns:foo="..." attribute that caused this schema document
    // to get loaded).
    public: Atom*   getURN() { return _pURN; }
    public: void    setURN(Atom* pURN) { _pURN = pURN; }

    //------------------------ Member variables ------------------------------
    protected:

    RMoniker    _pMoniker;
    bool        _fOnDataAvailableEvents; // whether to fire these events or not.
    RMutex      _pMutex;
    DWORD       _dwLoadState;
    RString     _pURLdoc;
    RString     _pResolvedURLdoc;
    Node *      _pDocNode;
    RNodeManager _pNodeMgr;
    RNodeManager _pAltNodeMgr;
    RString     _pOutputEncoding;
    RNodeFactory _pFactory;
    RXMLParser  _pParser;

    private: 
    RException  _pParseError;
    RNamespaceMgr _pNamespaceMgr;
    RNameDef    _pNameDataType;
    DWORD       _dwOutputStyle;
    ParserFlags _uParserFlags;
    bool        _fInsideRun;
    bool        _fDefaultFactory;
    RDTD        _pDTD;
    bool        _fAsync;
    bool        _fDOM;
    bool        _fPreserveWS;
    bool        _fFilterSchemaDoc;
    LookasideCache _lookaside;
    private: // event handling
    PCPNODE     _pCPListRoot;
    ULONG_PTR	_lSpinLockCPC;
    RAtom       _pURN;
    bool        _fReadOnly;
    bool        _fAborting;
    bool        _fPending;

    // These can be used multi-threaded if someone starts an async download via 
    // XMLHTTP and assigns the XMLHTTP.responseXML.ondataavailable property before
    // doing the send().
    GITDispatch _pdispOnReadyStateChange;
    GITDispatch _pdispOnDataAvailable;
    GITDispatch _pdispOnTransformNode;

    RMutex           _pMutexLoad;
    HANDLE           _hEventLoad;
    bool             _fHasLoader;
    bool             _fExited;
    DWORD            _dwLoadThreadId;

    public:          void setExitedLoad(bool fExited)
                     {
                         _fExited = fExited;
                     }

    // child documents.
    WDocument   _pParent;
    RVector     _pChildDocs; // keep child docs alive.

#if NOT_IN_CURRENT_SPEC
    bool        _fIECompatible;
#endif

    public: 
    _reference<IUnknown>    _pFreeThreadedMarshaller;

#ifdef RENTAL_MODEL
    private:    RentalEnum  _reModel;

    public:     RentalEnum  model() { return _reModel; }
#endif

#if DBG == 1
    public:     unsigned _cNodes;
    public:     unsigned _cNamedNodes;
    public:     unsigned _cTextNodes;
    public:     unsigned _cbText;
#endif

#if DBG == 1
    public:     TLSDATA * ptlsdataRental;   // in DEBUG mode the thread currently executing code
#endif
};  

#include "xmldom.hxx"

#ifndef _XML_OM_XMLERROR
#include "xml/om/xmlerror.hxx"
#endif

#endif _XML_OM_DOCUMENT


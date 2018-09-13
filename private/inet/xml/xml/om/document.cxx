/*
 * @(#)Document.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifdef UNIX
#include <olectl.h>
#endif // UNIX

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#ifndef _XMLNAMES_HXX
#include "xmlnames.hxx" 
#endif

#ifndef _XML_OUTPUTHELPER
#include "xml/util/outputhelper.hxx"
#endif

#ifndef _XML_DTD
#include "dtd.hxx"
#endif

#ifndef _BSTR_HXX
#include "core/com/bstr.hxx"
#endif

#include "ie4nodefactory.hxx"

#ifndef _XML_OM_NODE
#include "xml/om/node.hxx"
#endif

#ifndef _XML_OM_DOMNODE
#include "xml/om/domnode.hxx"
#endif

#ifndef _XML_OM_NODEDATANODEFACTORY
#include "xml/om/nodedatanodefactory.hxx"
#endif

#ifndef _SchemaNodeFactory_HXX
#include "xml/schema/schemanodefactory.hxx"
#endif

#ifndef _CORE_UTIL_HASHTABLE
#include "core/util/hashtable.hxx"
#endif

#ifndef _XML_DTD_ENTITY
#include "xml/dtd/entity.hxx"
#endif

#ifndef _XML_OM_OMLOCK
#include "xml/om/omlock.hxx"
#endif

#include "namespacenodefactory.hxx"
#include "namespacemgr.hxx"
#include "xml/tokenizer/net/urlstream.hxx"
#include "xmldomdid.h"

DeclareTag(tagDocRun, "XML Document", "Doc::Run");
DeclareTag(tagSchemas, "Schemas", "Schemas");

//----------------------------------------------------------------------------------
// Ready states for DOM from events OM spec:
// UNINITIALIZED   The XML object has been created but there is no XML tree and no load 
//                 in progress, i.e. load() has not been called. 
// LOADING         We are boot-strapping our object - that is, reading any persisted 
//                 properties - not parsing data. For purposes of the standard readyState 
//                 definitions, data should be considered equivalent to BLOB properties. 
// LOADED          We have finished boot-strapping our object and are now beginning to 
//                 read and parse data. 
// INTERACTIVE     Some data has been read and parsed and the object model is now  
//                 available on the partially retrieved data set.  
// COMPLETED       Document has been loaded, successfully or unsuccessfully.
//----------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////////
// Implement CreateParser function used by xml\dll\msxml.cxx and used below also.
#include "xml/tokenizer/parser/xmlparser.hxx"

#ifdef RENTAL_MODEL
HRESULT STDMETHODCALLTYPE
_CreateParserHelper(REFIID iid, void **ppvObj, RentalEnum re)
{
    HRESULT hr;
    XMLParser * str = new_ne XMLParser(re);
    if (str == NULL)
        return E_OUTOFMEMORY;
    hr = str->QueryInterface(iid, ppvObj);     
    str->Release();
    return hr;
}

HRESULT STDMETHODCALLTYPE
CreateParser(REFIID iid, void **ppvObj)
{
    return _CreateParserHelper(iid, ppvObj, MultiThread);
}

#else

HRESULT STDMETHODCALLTYPE
CreateParser(REFIID iid, void **ppvObj)
{
    HRESULT hr;
    XMLParser * str = new_ne XMLParser();
    if (str == NULL)
        return E_OUTOFMEMORY;
    hr = str->QueryInterface(iid, ppvObj);     
    str->Release();
    return hr;
}
#endif

////////////////////////////////////////////////////////////////////////////////////


DEFINE_CLASS_MEMBERS_NEWINSTANCE(Document, _T("Document"), CSafeControl);

// QueryInterface moved to msxmlCOM.hxx

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

LONG g_lDocumentCount;

/**
 * Construct a new empty document and use the default
 * element factory.
 */
Document::Document()
{
#ifdef RENTAL_MODEL
    _reModel = Base::model();
#endif

    InterlockedIncrement(&g_lDocumentCount);

    //EnableTag(tagDocRun, TRUE);

    // Objects are ALWAYS initialized to ZERO - so the following
    // initializations that are setting variables to ZERO are commented out
    // for added efficiency and reduced code size.  It is still good to
    // document these initializations.

    // Initialize the Dispatch pointers used to fire events
    // _pdispOnTransformNode = NULL;    
    // _pdispOnDataAvailable = NULL;
    // _pdispOnReadyStateChange = NULL;
    
    // _fDOM = false;
    _dwOutputStyle = OutputHelper::DEFAULT;
    // _fReadOnly = false;

    XMLNames::classInit();
    // _pFreeThreadedMarshaller = null;
    // _fAsync = false;
    _pMutex = ApartmentMutex::newApartmentMutex();
    // starts with refcount 1
    _pMutex->Release();

    _pMutexLoad = ShareMutex::newShareMutex();
    // starts with refcount 1
    _pMutexLoad->Release();

    _fHasLoader = false;
    _dwLoadThreadId = 0;
    _hEventLoad = CreateEvent(NULL, TRUE, TRUE, NULL);
    ResetEvent(_hEventLoad);

    // The DOM object model Load and Abort methods have a special behavior.
    // The Load methods take a read lock and mark the document read-only so
    // that any other write operations (like addChild) will fail -- except
    // for Abort or another Load (which effectively aborts the previos Load).
    // which will succeed by releasing the
    // read lock and grabbing a write lock to do the Abort


    // _fInsideRun = false;
    // _fAborting = false;
    // _fDefaultFactory = false;
    // _fOnDataAvailableEvents = true;
    // _fPreserveWS = false;
    // _fPending = false;

    _pNamespaceMgr = NamespaceMgr::newNamespaceMgr(true);
    newNodeMgr(&_pNodeMgr);
    _createDocNode();
}

Document::~Document()
{
    InterlockedDecrement(&g_lDocumentCount);
}

Document * 
Document::clone(bool deep)
{
    Document* doc = new Document();

    super::CloneBase((super*)doc);
    doc->_fAsync = _fAsync;
    doc->_dwLoadState = _dwLoadState;
    doc->_pURLdoc = (String*)_pURLdoc;
    doc->_pResolvedURLdoc = (String*)_pResolvedURLdoc;
    doc->_fDOM = _fDOM;
    doc->_uParserFlags = _uParserFlags;
    doc->_pURN = _pURN;
    doc->_fReadOnly = _fReadOnly;

    if (deep)
    {
        doc->_pDTD = !_pDTD ? null : (CAST_TO(DTD*, _pDTD->clone()));
        doc->_pNamespaceMgr = CAST_TO(NamespaceMgr*, _pNamespaceMgr->clone());

        Node * pNewDocNode = doc->_pDocNode;
        // and then clone the data also.
        pNewDocNode->_pLast = getDocNode()->cloneChildren(true, false, false, doc, doc->getNodeMgr(), pNewDocNode, true);
    }
    return doc;
}

GUID 
Document::getClassID()
{
    if (isDOM())
    {
#ifdef RENTAL_MODEL
        return model() == Rental ? CLSID_DOMDocument : CLSID_DOMFreeThreadedDocument;
#else
        return CLSID_DOMDocument;
#endif
    }
    else
        return CLSID_XMLDocument;
}

void
Document::setFactory(IXMLNodeFactory * f)
{
    _pFactory = f;
    _fDefaultFactory = false;
}

void 
Document::newNodeMgr(NodeManager ** ppNodeMgr)
{
    VMManager * pVMM = null;
    VMManager::getDefaultVMM( &pVMM);
    *ppNodeMgr = new NodeManager( pVMM, sizeof(Node));
    pVMM->Release();
}

NodeManager * 
Document::getAltNodeMgr()
{
    if (!_pAltNodeMgr)
        newNodeMgr(&_pAltNodeMgr);
    Assert(!!_pAltNodeMgr);
    return _pAltNodeMgr;
}

DTD* 
Document::getDTD()
{
    if (! _pDTD)
    {
         _pDTD = DTD::newDTD();
    }
    return _pDTD; 
}

void 
Document::initDefaultFactory(Document * pSrcDoc, Atom * pSchemaURN)
{
    _fDefaultFactory = (pSrcDoc == null);
    Assert( (NodeManager*)_pNodeMgr);
    // then setFactory was not called, so create our own.
    IXMLNodeFactory* ndnf = null;
    IXMLNodeFactory* vnf = null;
    IXMLNodeFactory* snf = null;
    TRY 
    {
        if (isDOM())
        {
            ndnf = new NodeDataNodeFactory( this);

            if (pSrcDoc)
            {
                _pDTD = pSrcDoc->getDTD();
                setFilterSchemaDoc(true);  // this is used by dso, but all it does is to KEEP the DTD when document is initiated.
                                           // in schema case, we want to set the DTD to pSrcDoc->DTD
                                       

                snf = new SchemaNodeFactory(ndnf, _pDTD, pSrcDoc->getNamespaceMgr(), _pNamespaceMgr, pSchemaURN, this);
            }

            // this will prevent the DTD from getting reset when we open new schema document 
            // using a separate DTD loaded by the DSO.  (This is the first step towards
            // sharing DTD's across documents !!)
            if (!_fFilterSchemaDoc) 
            {
                // Now we reset the DTD by calling clear.  We CANNOT blindly create a new DTD
                // object here because we may have already constructed the node factories
                // at this point - so we need to keep the correct DTD object around -
                // especially if someone QI'd for the IXMLParser before calling load so 
                // that they could insert their own node factory.
                getDTD()->clear();
            }            
    

            // Now we always wrap these in a namespace factory.  This will do all the
            // TRY/CATCH and STACK_ENTRY stuff so the other node factories don't have to.
            _pFactory = new NameSpaceNodeFactory(this, snf ? snf : ndnf, getDTD(), _pNamespaceMgr, 
                                                 _uParserFlags.fIgnoreDTD,
                                                 _uParserFlags.fParseNamespaces);

            if (pSrcDoc == null)
                _pDTD->setNodeFactory(ndnf);
        }
        else
        {
            _pFactory = new IE4NodeFactory(this);
        }
        _pFactory->Release(); // smart pointer is now holding on to it.
    }
    CATCH
    {
        release(&ndnf);
        release(&snf);
        Exception::throwAgain();
    }
    ENDTRY

CleanUp:
    release(&ndnf);
    release(&snf);
}

Node*
Document::parseXMLDecl(const WCHAR* attrs)
{
    String* xml = String::add(String::newString("<?xml "),
                                String::newString(attrs),
                                String::newString("?><xml/>"),
                                null);
    Document* fakeDoc = new Document();
    fakeDoc->setDOM(true);

    IXMLParser* pParser;
    fakeDoc->getParser(&pParser);
    ((XMLParser*)pParser)->setIgnoreEncodingAttr(true);
    pParser->Release();

    fakeDoc->loadXML(xml);
    Node* xmldecl = (Node*)fakeDoc->getXML(false);
    Assert(xmldecl);
    // Clone it so it "belongs" to this Document.
    Node * pClone = xmldecl->clone(true, false, this, getNodeMgr());    
    return pClone;
}

/**
 * Retrieves the  XML decl PI
 */
Element * Document::getXML( bool fBuildIt)
{
    Assert(_pDocNode);

    Node * pNode = _pDocNode->find( null, Node::XMLDECL);
    if ( pNode)
        return (Element *)pNode;

    if ( fBuildIt)
    {   
        
        Element * pElem = createElement(null, Element::PI,
                          _pNamespaceMgr->createNameDef(XMLNames::name(NAME_XML)), null);
        _pDocNode->addChildAt( pElem, 0 );
        return pElem;
    }

    return null;
}

/**
 * Retrieves the version information.
 * @return the version number stored in the &lt;?XML ...?&gt; tag. 
 */
String * Document::getVersion()
{
    Element * eXMLDecl = getXML();
    if (eXMLDecl != null) 
    {
        // Version attribute can be upper case in IE4 mode due
        // to folding to uppercase in nodedatanodefactory.
        // (and there is no namespacefactory in this case).
        return getAttributeIgnoreCase((Node*)eXMLDecl, XMLNames::name(NAME_VERSION));
    }
    return String::newString(XMLNames::pszDefaultVersion);
}

void
Document::reportObjects()
{
#ifdef RENTAL_MODEL
    if (model() != Rental)
    {
#endif
        // give a hint to the GC about the number of pages to be released !
        Base::reportObjects(_pNodeMgr->getPages() * (_VMM_PAGESIZE * 1024 / sizeof(Node)));
#ifdef RENTAL_MODEL
    }
#endif
}

void 
Document::_clearDocNode()
{
    if (_pDocNode)
    {
        _pDocNode->deleteChildren(false); 
        // give a hint to the GC about the number of pages to be released !
        reportObjects();
        Base::testForGC(0);
    }
}

void 
Document::_createDocNode()
{
    if (_pDocNode)
    {
        _pDocNode->detach();
        _pDocNode = null;
    }
    _pDocNode = Node::newNode(Element::DOCUMENT, null, this, getNodeMgr());
    _pDocNode->attach();
    weakAddRef();
}


Node * 
Document::
getRoot()
{
    Assert(_pDocNode);
    Node * pNode = _pDocNode->find( null, Element::ELEMENT);
    return pNode;
}


void 
Document::
setRoot(Node* pNewNode)
{
    Node * pNode;
    
    if ( pNewNode && pNewNode->getNodeType() != Element::ELEMENT)
        Exception::throwE((HRESULT)E_INVALIDARG);

    Assert(_pDocNode);
    pNode = _pDocNode->find( null, Element::ELEMENT);

    if ( pNode)
    {
        if ( pNewNode)
        {
            _pDocNode->replaceNode( pNewNode, pNode);
        }
        else
        {
            _pDocNode->removeNode( pNode);
        }
    }
    else
    {
        _pDocNode->insertNode( pNewNode, null);
    }
}


/**
 * Sets the version number stored in the &lt;?XML ...?&gt; tag.      
 * @param version The version information to set.
 * @return No return value.
 */
void Document::setVersion( String * version )
{
    Element * pXML = getXML( true);
    if ( pXML)
    {
        pXML->setAttribute(XMLNames::name(NAME_VERSION), version );
    }
}

Element * 
Document::createElement(Element* parent, int type, NameDef * tag, String* text)
{
    bool fHasName = false;
    Element::NodeType nodeType;
    switch (type)
    {
    case Element::ELEMENT:
        nodeType = Node::ELEMENT;
        fHasName = true;
        break;
    case Element::PI:
        nodeType = Node::PI;
        fHasName = true;
        break;
    case Element::DOCTYPE:
        nodeType = Node::DOCTYPE;
        fHasName = true;
        break;
    case Element::PCDATA:
        nodeType = Node::PCDATA;
        break;
    case Element::CDATA:
        nodeType = Node::CDATA;
        break;
    case Element::COMMENT:
        nodeType = Node::COMMENT;
        break;
    case Element::ENTITYREF:
        nodeType = Node::ENTITYREF;
        fHasName = true;
        break;
    case Element::DOCUMENT:
        // this couldn't really create a new document, so we should just fail
        return null;

    default:
        Assert( 0 && "Called createElement() with a bogus element type");
        return null;
    }

    if (fHasName && !tag)
        tag = _pNamespaceMgr->createNameDef(String::emptyString());
    Node * pNode = Node::newNode( nodeType, tag, this, getNodeMgr());
    if (pNode == null)
    {
        Exception::throwEOutOfMemory(); // Exception::OutOfMemoryException);
    }

    if (text != null)
    {
        pNode->setInnerText(text, true);
    }

    Object * o = pNode->getElementWrapper();
    if (o == null)
        Exception::throwEOutOfMemory(); // Exception::OutOfMemoryException);
    
    return (CAST_TO(Element *, o));
}

String* 
Document::getAttributeIgnoreCase(Node* node, Name* name)
{
    Object* v = node->getAttribute(name);
    if (v == null)
    {
        String* str = name->getName()->toString();
        Name* name = Name::create(str->toUpperCase());
        v = node->getAttribute(name);
    }
    return (v == null) ? null : v->toString();
}

/**
 * Retrieves the character encoding information.
 * @return the encoding information  stored in the &lt;?XML ...?&gt; tag
 * or the user-defined output encoding if it has been more recently set.
 */
String * Document::getEncoding()
{
    // _pOutputEncoding may have been set manually via setEncoding, but
    // if not we can get the encoding from the XML declaration, if any.
    // otherwise default to UTF-8.
    if( _pOutputEncoding == null )
    {
        Element * eXMLDecl = getXML();
        if (eXMLDecl != null) 
        {
            // Encoding attribute can be upper case in IE4 mode due
            // to folding to uppercase in nodedatanodefactory.
            // (and there is no namespacefactory in this case).
            return getAttributeIgnoreCase((Node*)eXMLDecl, XMLNames::name(NAME_encoding));
        }
    }
    return _pOutputEncoding;
}

/**
 * Sets the character encoding for output. Eventually it sets the ENCODING 
 * stored in the &lt;?XML ...?&gt; tag, but not until the document is saved.
 * You should not call this method until the Document has been loaded.
  * @return No return value.
 */
void Document::setEncoding( String* charset )
{
    _pOutputEncoding = charset;        

// BUGBUG ------ Hmmm -- With the Node, the XML element will never be set.
// so updating the tree is a little more difficult.
/*
    if (XML == null)
    {
        // Create an XML declaration so that we write one out
        // when save is called.
        XML = createElement(this, Element.PI, XMLNames::name(NAME_XML), null);
        XML->setAttribute(XMLNames::name(NAME_ENCODING), charset);
    }
*/
}

/**
 * Retrieves the standalone attribute stored in the &lt;?XML ...?&gt; tag. 
 * This is a new attribute based on Nov17 XML Lang spec that replaces
 * the old RMD attribute.
 * @return the standalone attribute value.
 */
String* Document::getStandalone()
{
    Element * eXMLDecl = getXML();
    if (eXMLDecl != null) 
    {
        // Standalone attribute can be upper case in IE4 mode due
        // to folding to uppercase in nodedatanodefactory.
        // (and there is no namespacefactory in this case).
        return getAttributeIgnoreCase((Node*)eXMLDecl, XMLNames::name(NAME_Standalone));
    }
    return null;
}

/**
 * Sets the Standalone attribute stored in the &lt;?XML ...?&gt; tag.      
 * @param standalone The value to set.
 * @return No return value.
 */
void Document::setStandalone( String* standalone )
{
    Element * pXML = getXML( true);
    if ( pXML)
    {
        pXML->setAttribute(XMLNames::name(NAME_Standalone), standalone );
    }
}

/**
 * Returns the name specified in the &lt;!DOCTYPE&gt; tag.
 */
NameDef * Document::getDocType()
{
    Node * pDTDNode = _pDocNode->find(null, Node::DOCTYPE);
    if (pDTDNode == null)
        return null;
    return pDTDNode->getNameDef();
}

/**
 * Retrieves the document type URL.
 * @return the URL specified in the &lt;!DOCTYPE&gt; tag or null 
 * if an internal DTD was specified.
 */
String * Document::getDTDURL()
{
    Node * pDTDNode = _pDocNode->find(null, Node::DOCTYPE);
    if (pDTDNode == null)
        return null;
    Object * o = pDTDNode->getAttribute(XMLNames::name(NAME_SYSTEM));
    if (o)
        return o->toString();
    return null;
}

/**
 * Retrieves the last modified date on the source of the URL.
 * @return the modified date.
 */
int64 Document::getFileModifiedDate()
{
    // Failed or no URL source available
    return 0;
}

/**
 * Retrieves the external identifier. 
 * @return the external identifier specified in the &lt;!DOCTYPE&gt; tag 
 * or null if no &lt;!DOCTYPE&gt; tag was specified. 
 */
String * Document::getId()
{
    Node * pDTDNode = _pDocNode->find(null, Node::DOCTYPE);
    if (pDTDNode == null)
        return null;
    Object * o = pDTDNode->getAttribute(XMLNames::name(NAME_PUBLIC));
    if (o)
        return o->toString();
    return null;
}


/**
 * Loads the document from the given URL string.
 * @param urlstr The URL specifying the address of the document.
 * @exception Exception if the file contains errors.
 * @return No return value.
 */
void Document::load(String * urlstr, bool async) //throws Exception
{
    reset();
    getBaseURL();
    _pURLdoc = _pResolvedURLdoc = urlstr->trim();
    _load(async);
}

String * 
Document::getURL()
{
    return _pResolvedURLdoc;
}

/**
 * Implementation of the PersistStream interface.
 *
 * @param in The input stream.
 * @return No return value.
 * @exception Exception when a syntax error is found.
 */
void Document::Load(IStream * in) //throws Exception
{
    HRESULT hr = S_OK;

    TRY
    {
        enterDOMLoadLock();

        reset();
        getBaseURL();
        _load(this->_fAsync,NULL,NULL,in);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    TRY
    {
        leaveDOMLoadLock(hr);
        setExitedLoad(true);
    }
    CATCH
    {
        if (hr == S_OK)
        {
            hr = ERESULTINFO;
        }
    }
    ENDTRY

    if (S_OK != hr)
    {
        Exception::throwAgain();
    }
}

// Implementation of the IPersistMoniker interface
void Document::load(
                bool fFullyAvailable,
                IMoniker *pmk,
                LPBC lpbc,
                DWORD grfMode)
{
    HRESULT hr = S_OK;

    TRY
    {
        enterDOMLoadLock();

        reset();
        getBaseURL();
        _load(!fFullyAvailable,pmk,lpbc);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    TRY
    {
        leaveDOMLoadLock(hr);
        setExitedLoad(true);
    }
    CATCH
    {
        if (hr == S_OK)
        {
            hr = ERESULTINFO;
        }
    }
    ENDTRY

    if (S_OK != hr)
    {
        Exception::throwAgain();
    }
}

void Document::_load(bool async, IMoniker *pmk, LPBC pbc, IStream* pStm)
{   
    HRESULT hr = S_OK;
    RXMLParser pParserSecond;

    TRY 
    {
//        TraceTag((0, "Before _load allocated = %d", DbgTotalAllocated()));
        this->_fAsync = async;

        clear();

        // This code is tricky because it has to be multithread safe since we
        // are manipulating the _pParser pointer when we only have a read lock.
        // This boils down to making sure we always keep an extra ref-count on
        // the parser so another load cannot blow it away.
        if (_pParser == null)
        {
            getParser(&pParserSecond); // initializes the parser member.
            _pParser = pParserSecond;
        }
        else
        {
            // And in the synchronous load case, keep the parser alive until we're done.
            pParserSecond = _pParser;
        }

        // We need to go from LOADING to LOADED.  This is somewhat lame, but the 
        // Trident code relies on us transitioning from LOADING.  They are willing to 
        // investigate changing this post-IE 5.0 beta 2 (simonb 09-11-98)
        setReadyStatus(READYSTATE_LOADING);
        setReadyStatus(READYSTATE_LOADED);

        getDocNode()->setFinished(false);

        if (pStm)
        {
            hr = _pParser->SetInput(pStm);
            if (FAILED(hr))
            {
                Exception::throwE(hr);
            }
        }
        else if (pmk != NULL)
        {
            _pMoniker = pmk; // remember it so we can implement getCurMoniker.
            // In this case the parser will pump itself during download.
            hr = _pParser->Load(true, pmk, pbc, 0);
        }
        else
        {
            const TCHAR* base = null;
            if (_pBaseURL != null)
            {
                ATCHAR* astr = _pBaseURL->toCharArrayZ();
                base = astr->getData();
            }
            const TCHAR* url = null;
            if (_pURLdoc != null)
            {
                if (_pURLdoc->length() == 0)
                {
                    Exception* e = Exception::newException((HRESULT)E_INVALIDARG,
                        Resources::FormatSystemMessage(E_INVALIDARG));
                    e->throwE();
                }
                ATCHAR* astr = _pURLdoc->toCharArrayZ();
                url = astr->getData();
            }
            hr = _pParser->SetURL(base, url, async ? TRUE : FALSE);
        }
        // Ok, now get the fully resolved URL back out from the parser
        // ad store it in _pResolvedURLdoc so we can return it from getURL.
        // This is especially important in the IMoniker case where 
        // _pResolvedURLdoc is currently null.
        {
            const WCHAR* pszURL = NULL;
            pParserSecond->GetURL(&pszURL);
            if (pszURL)
            {
                _pResolvedURLdoc = String::newString(pszURL);
                TraceTag((tagSchemas, "Document %X is loading %s", this, (char*)AsciiText(_pResolvedURLdoc)));
            }
        }

        if (hr == E_PENDING)
        {
            // this is normal only for asynchronous download
            return;
        }

        // This is a good time to garbage collect.
//        Base::checkZeroCountList();

        if (hr != S_OK)
        {
            // Error messages still contain initial un-resolved URL
            // so that they are not location dependent and our DRT's
            // continue to run.
            String* msg = Resources::FormatSystemMessage(hr);
            Exception* e = Exception::newException(hr, msg);
            e->setUrl(_pURLdoc);
            e->throwE();
        }

        run();
    }
    CATCH
    {
        abort(GETEXCEPTION());

        // This is a good time to garbage collect.
//        Base::checkZeroCountList();

        // BUGBUG: have to get ride of the parser, because the parser could have been gone 
        //         when the parser dll was unloaded, even before the document->finalize() is called,
        //         If that happens, the document is holding a invalid pointer to the parser.
        //         Before we have a good solution to this, we have to release the parser after loading
        //         and not to optimize this.
        _pParser = null;
        pParserSecond = null;

        Exception::throwAgain();
    }
    ENDTRY

//    TraceTag((0, "After _load allocated = %d _cNodes = %d _cNamedNodes = %d _cTextNodes = %d _cbText = %d", DbgTotalAllocated(),
//        _cNodes, _cNamedNodes, _cTextNodes, _cbText));
}

HRESULT
Document::newParser(IXMLParser** ppParser)
{
#ifdef RENTAL_MODEL
    HRESULT hr = _CreateParserHelper(IID_IXMLParser, (void**)ppParser, model());
#else
    HRESULT hr = CreateParser(IID_IXMLParser, (void**)ppParser);
#endif
    if (hr)
    {
        return hr;
    }

    // make sure base url is initialized, and pass it through to the parser so the
    // parser can resolve relative DTD's even if Load is called with an IStream.
    if (0 != _dwSafetyOptions)
    {
        if (_pSecureBaseURL == null)
        {
            getSecureBaseURL();
        }
        if (_pSecureBaseURL != null)
        {
            // set base URL to use for security checking.
            ATCHAR* astr = _pSecureBaseURL->toCharArrayZ();
            (*ppParser)->SetSecureBaseURL(astr->getData());
        }
        else if (_pBaseURL != null)
        {
            // Trident calls put_baseURL for security...
            ATCHAR* astr = _pBaseURL->toCharArrayZ();
            (*ppParser)->SetSecureBaseURL(astr->getData());
        }
    }
    ((XMLParser*)(*ppParser))->setSafetyOptions(_dwSafetyOptions);

    // In the loadXML case we go through PushData, so in this case
    // we need to copy the base URL into the parser.
    String* baseURL = GetBaseURL();
    if (baseURL)
    {
        ATCHAR* astr = baseURL->toCharArrayZ();
        ((XMLParser*)(*ppParser))->SetBaseURL(astr->getData());
    }

    return hr;
};


void
Document::getParser(IXMLParser** ppParser)
{
    HRESULT hr = newParser(ppParser);
    if (FAILED(hr))
    {
        Exception::throwE(hr);
    }

    _pParser = *ppParser;

    // And initialize the parser with the current settings...
    _pParser->SetRoot(_pDocNode);

    // If we have the default factory, blow it away so that we clear
    // out the current DTD.
    if (_pFactory == NULL || _fDefaultFactory)
    {
        initDefaultFactory();
    }
    // And then reset the namespaces before each reload.
    if (_pNamespaceMgr != null)
    {
        _pNamespaceMgr->reset();
    }
    _pParser->SetFactory(_pFactory);

    ULONG flags = 0;
    if (! _uParserFlags.fParseNamespaces)
        flags |= XMLFLAG_NONAMESPACES;
    if (_uParserFlags.fCaseInsensitive)
        flags |= XMLFLAG_CASEINSENSITIVE;
    if (_uParserFlags.fOmitWhiteSpaceElements)
        flags |= XMLFLAG_NOWHITESPACE;
    if (_uParserFlags.fShortEndTags)
        flags |= XMLFLAG_SHORTENDTAGS;
    if (_uParserFlags.fIgnoreDTD)
        flags |= XMLFLAG_NODTDNODES;
    if (_uParserFlags.fIE4Compatibility)
        flags |= XMLFLAG_IE4COMPATIBILITY;

    _pParser->SetFlags(flags);
}

/**
 *Gets the URL from the specified IMoniker and binding context
 */
void Document::getCurMoniker(IMoniker** ppimkName)
{
    *ppimkName = (IMoniker*)_pMoniker;
}

OutputHelper * 
Document::createOutput(IStream * out, String* s)
{
    OutputHelper* h = null;
    if (s == null)
        s = getEncoding();
    if (s == null)
        s = String::newString(_T("UTF-8"));
    IStream* o;
    ATCHAR* astr = s->toCharArrayZ();
    HRESULT hr = ::CreateEncodingStream(out, astr->getData(), &o);
    if (FAILED(hr))
    {
        Exception::throwE(hr,hr,null);
    }
    h = OutputHelper::newOutputHelper(o, _dwOutputStyle, s);
    o->Release(); // since OutputHelper now holds onto it.
    return h;
}

void Document::reset()
{
    if (_pParser)
    {
        // If this was a newly created parser gotten through QI(IID_IXMLParser)
        // then it will still be in XMLPARSER_IDLE state in which case we DO NOT
        // want to NULL out the _pParser variable because if we do we will BREAK
        // the XML MimeType Viewer !!!
        HRESULT hr = _pParser->GetParserState();
        if (hr != XMLPARSER_IDLE)
        {
            _pParser = null;
        }
    }
    _pParseError = null;
    _pMoniker = null;
    _fInsideRun = false;
    _pURLdoc = null;
    _pResolvedURLdoc = null;
    _fOnDataAvailableEvents = true; // default.
}

void Document::cleanUp()
{
    // BUGBUG: have to get rid of the parser, because the parser could have been gone 
    //         when the parser dll was unloaded, even before the document->finalize() is called,
    //         If that happens, the document is holding a invalid pointer to the parser.
    //         Before we have a good solution to this, we have to release the parser after loading
    //         and not to optimize this.
    if (! _fInsideRun)      // bug 58391
        _pParser = null;

    // _pMoniker = null; // actually, according to IPersistMoniker docs - we need to hang 
                         // on to this in order to implement getCurMoniker correctly.

    // Clear the default node factories - this is required to break circular
    // references in SchemaNodeFactory.
    if (_fDefaultFactory)
    {
        _pFactory = null;
    }

    if (_pParent == NULL)
    {
        getDTD()->setNodeFactory(null);
    }
}

/**
 * Saves the document to the given output stream.
 * @param o  The output stream.
 * @return No return value.
 * @exception IOException if there is a problem saving the output.
 */
void Document::Save(IStream* pStm)
{
    OutputHelper* o = createOutput(pStm);
    TRY
    {
        save(o);
    }
    CATCH
    {
        o->close();
        Exception::throwAgain();
    }
    ENDTRY
    o->close();
}

void Document::save(OutputHelper* o) //throws IOException * * 
{
    if (_pDocNode)
    {
        o->setOutputStyle(OutputHelper::PRETTY);
        _pDocNode->save(this, o);
    }
}


/**
 * Sets the Document back to its initial empty state retaining 
 * only the <code>ElementFactory</code> association.
 * @return No return value.
 */
void Document::clear()
{
#if DBG == 1
    _cNodes = 0;
    _cNamedNodes = 0;
    _cTextNodes = 0;
    _cbText = 0;
#endif
    _clearDocNode();
    _pOutputEncoding = null;
    
    // this will prevent the DTD from getting reset when we open new schema document 
    // using a separate DTD loaded by the DSO.  (This is the first step towards
    // sharing DTD's across documents !!)
    if (!_fFilterSchemaDoc) 
    {
        // Now we reset the DTD by calling clear.  We CANNOT blindly create a new DTD
        // object here because we may have already constructed the node factories
        // at this point - so we need to keep the correct DTD object around -
        // especially if someone QI'd for the IXMLParser before calling load so 
        // that they could insert their own node factory.
        getDTD()->clear();
    }

    _pParseError = null;
    _pChildDocs = null;
}


void 
Document::enterDOMLoadLock()
{
    // set _pLoadMutex to tell us
    // that we have entered the "Load" read lock, and mark the
    // document read-only so any script call backs to modify the
    // (like appendChild) tree will fail - except for abort
    
    if (isDOM())
    {
        // Can only have one thread loading the document at a time.
        MutexLock lock(_pMutexLoad);

        // If there is a thread loading the document, 
        // abort it and wait for its exit
        if (_fHasLoader)
        {
            abort(Exception::newException(XMLOM_USERABORT, XMLOM_USERABORT, null));

            // If it's not the current thread that is loading, then have to wait 
            if (GetTlsData()->_dwTID != _dwLoadThreadId)
            {
                // wait for the loading thread to exit
                WaitForSingleObject(_hEventLoad, INFINITE);
            }
        }

        // Clear the event
        ResetEvent(_hEventLoad);

        _fHasLoader = true;
        _fExited = false;

        // Set readonly lock here
        _pMutex->EnterRead();
        _dwLoadThreadId = registerNonReentrant();
    }
}


void 
Document::leaveDOMLoadLock(HRESULT hr)
{
    // We allow only one loading a time, so no mutex is needed here
    if (isDOM() && _fHasLoader && 
        (FAILED(hr) || _pParseError != NULL || _dwLoadState == READYSTATE_COMPLETE))
    {
        Assert(_pMutex != null);
        _pMutex->LeaveRead();
        if (_dwLoadThreadId)
            removeNonReentrant(_dwLoadThreadId);
        _fHasLoader = false;
        // clear _dwCurrentThreadID
        _dwLoadThreadId = 0;

        // Notify its exit
        SetEvent(_hEventLoad);
    }
}


void Document::abort(Exception * e)
{
    if (_fAborting)
        return;
    _fAborting = true;

    TRY
    {

        // Tell the parser to abort the download. This is needed because just releasing 
        // the parser is no guarentee that the parser refcount will actually hit zero.
        if (_pParser) 
        {
            _pParser->Abort(null);
        }

        if (_pParseError == NULL)
        {
            _pParseError = e;
        }

        if (_pChildDocs)
        {
            // abort any pending child document downloads also.
            while (_pChildDocs->size() > 0)
            {
                // Now when we abort the child document, the child will call
                // setReadyStatus and setReadyStatus will call childFinished on 
                // this document which will in turn remove the child from the _pChildDocs.
                // Hence this loop cannot do a for loop through the _pChildDocs, but 
                // instead it does a while loop for _pChildDocs->size() > 0.
                Document* child = (Document*)(GenericBase*)_pChildDocs->elementAt(0);
                child->abort(e); // this will also remove this child.
            }
        }
    
        // blow away the tree if we hit an error.  (If user hits Stop/ESC/Navigate
        // after we're done, that doesn't count as an error.)
        if (_dwLoadState != READYSTATE_COMPLETE || !getDocNode()->isFinished())
        {
            clear();
            _pParseError = e;
        }

        if (_uParserFlags.fIE4Compatibility)
            setReadyStatus(READYSTATE_UNINITIALIZED);
        else
            setReadyStatus(READYSTATE_COMPLETE);

        getDocNode()->setFinished(true);
    }
    CATCH
    {
        _fAborting = false;
        Exception::throwAgain();
    }
    ENDTRY

    _fAborting = false;
}


//
// implementation if OleCommandTarget interface
//

void Document::queryStatus(const GUID *pguidCmdGroup,
                            ULONG cCmds,
                            OLECMD prgCmds[],
                            OLECMDTEXT *pCmdText)
{
    
}


void Document::exec(const GUID *pguidCmdGroup,
                    DWORD nCmdID,
                    DWORD nCmdexecopt,
                    VARIANT *pvaIn,
                    VARIANT *pvaOut)
{
    bstr b;

    if (pguidCmdGroup != null)
        Exception::throwE(OLECMDERR_E_UNKNOWNGROUP);

    switch (nCmdID)
    {
    case OLECMDID_STOP:
        {
            // This is tricky !!
            // The DOMDocument wrapper does some magic locking stuff
            // to avoid deadlocks -- so let's re-use all that stuff here.
            IXMLDOMDocument* pDoc;
            HRESULT hr = QueryInterface(IID_IXMLDOMDocument, (void**)&pDoc);
            if (SUCCEEDED(hr))
            {
                VARIANT_BOOL result;
                hr = pDoc->abort();
                pDoc->Release();
            }
            if (FAILED(hr)) Exception::throwE(hr);
        }
        break;

    default:
        break;
    }
}

void Document::setDOM(bool yes)
{
    _fDOM = yes;    
    // DOM documents start at ready state COMPLETE.
    if (yes) _dwLoadState = READYSTATE_COMPLETE; 
}

void Document::onDataAvailable()
{
    // only notify if we have children to return (bug 44550).
    void * pTag;
    if (isAsync() && // and only if this is an async download
        _fOnDataAvailableEvents && 
        getDocNode()->getNodeFirstChild(&pTag)) 
        FireOnDataAvailable();
}

void Document::onStartDocument()
{
    if (isDOM())
    {
        setReadyStatus(READYSTATE_INTERACTIVE);
    }
}

void Document::onEndProlog()
{
}

HRESULT Document::run()
{
    HRESULT hr = S_OK;

    if (_fInsideRun)
        return E_PENDING;

    if (_pParser == null || _dwLoadState == READYSTATE_COMPLETE ||
        _dwLoadState == READYSTATE_UNINITIALIZED)
    {
        return S_OK;
    }

    _fInsideRun = true;

    TRY
    {
        // Before calling parser Run method, check the ready state.
        hr = _pParser->GetParserState();

        if (hr == XMLPARSER_STOPPED || hr == XMLPARSER_ERROR)
        {
            // This way we don't call Run when we're already done.
            hr = _pParser->GetLastError();
        }
        else
        {
            Assert(! hasPendingChildDocs());
            hr = _pParser->Run(_fAsync ? 4096 : -1);
        }

        TraceTag((tagDocRun, "Doc::Run parser returned hr %x", hr));
        
        if (hr == E_PENDING)
            goto CleanUp;

        if (hr == XML_E_STOPPED)
            hr = _pParser->GetLastError();

        if (hr == XML_E_SUSPENDED)
        {
            hr = E_PENDING;
            goto CleanUp;
        }

        _fInsideRun = false; // reset before calling handle end document.
        HandleEndDocument();
    }
    CATCH
    {
        _fInsideRun = false;
        HandleParseError(_pParser);
    }
    ENDTRY

CleanUp:
    _fInsideRun = false;
    return hr;
}

void Document::HandleEndDocument()
{
    TRY 
    {
        // Finished means all nodes have been parsed.  It is set by the first nodefactory that
        // calls this function.
        _pFactory = null;

        getDocNode()->setFinished(true);

        if (! _pParseError && _pParser)
        {
            // must be async end document
            HRESULT hr = _pParser->GetParserState();

            if (hr == XMLPARSER_STOPPED || hr == XMLPARSER_ERROR)
            {
                // This way we don't call Run when we're already done.
                hr = _pParser->GetLastError();
            }

            Assert(hr != XMLPARSER_SUSPENDED);
            Assert(hr != E_PENDING);

            if (hr != 0)
            {
                createException(_pParser, hr)->throwE();
            }
        }
        if (_pParseError)           // must now jump to HandleParseError
            _pParseError->throwE(); 

        if (! _fInsideRun)
        {
            cleanUp();
            setReadyStatus(READYSTATE_COMPLETE);
        }

    }
    CATCH
    {
        HandleParseError(_pParser);
    }
    ENDTRY
}

void Document::setLastError(Exception * e)
{
    // The _pParseError is already set if we are inheriting this exception from
    // a child document.  In which case we do not want to message with the
    // information inside the exception, although some day we may want to concatentate
    // more information so you know where the child document was loaded from in
    // the parent document...
    if (_pParseError != e)
    {
        _pParseError = e;
        if (_pParser)
        {
            e->_nFilePosition = _pParser->GetAbsolutePosition();
            WCHAR* buf = null;
            ULONG len = 0, sp = 0;
            HRESULT hr = _pParser->GetLineBuffer((const WCHAR**)&buf, &len, &sp);
            if (buf && len > 0)
            {
                e->setSourceText(String::newString(buf, 0, len));  
            }
            if (e->_nLine == 0)
            {
                e->_nLine = _pParser->GetLineNumber();
                e->_nCol = _pParser->GetLinePosition();
            }
        }
    }
    if (e->getUrl() == null)
    {
        if (_pParser)
        {
            const WCHAR* pwcBaseURL;
            _pParser->GetURL(&pwcBaseURL);
            if (pwcBaseURL)
            {
                e->setUrl(String::newString(pwcBaseURL));
            }
        }
    }
    // if it's still NULL then pick up the doc URL.
    if (e->getUrl() == null)
    {
        // Error messages still contain initial un-resolved URL
        // so that they are not location dependent and our DRT's
        // continue to run.
        e->setUrl(_pURLdoc);
    }
}

void Document::HandleParseError(IXMLNodeSource* pParser)
{   
    Exception * e = GETEXCEPTION();
    setLastError(e);

    _dispatchImpl::setErrorInfo(e);
    HRESULT hr = e->getHRESULT();
    TraceTag((tagDocRun, "Doc::Run exception with hr %x", hr));

    // BUGBUG: have to get rid of the parser, because the parser could have been gone 
    //         when the parser dll was unloaded, even before the document->finalize() is called,
    //         If that happens, the document is holding a invalid pointer to the parser.
    //         Before we have a good solution to this, we have to release the parser after loading
    //         and not to optimize this.
    if (! _fInsideRun)
    {
        abort(e);
        _pParser = null;
    }
}

Exception* 
Document::createException(IXMLNodeSource* pParser, HRESULT hr)
{
    String* description = null;
    BSTR bstr;
    HRESULT hr2 = _pParser->GetErrorInfo(&bstr);
    if (SUCCEEDED(hr2))
    {
        description = String::newString(bstr);
        ::SysFreeString(bstr);
    }
    if (hr2 != S_OK)
    {
        TRY 
        {
            String * msg = Resources::FormatMessage(hr, null);
            if (msg != null)
            {
                description = String::add(msg, description, null);
            }
        }
        CATCH
        {
            // put error code into message then.
            // SHLWAPI function
            ATCHAR * buf = new (100) ATCHAR;
            _ltow(hr, (TCHAR*)buf->getData(), 16);
            String* msg = String::newString(buf);
            String* fmsg = Resources::FormatMessage(XML_E_UNKNOWNERROR, msg, null); 
            description = String::add(fmsg, description, null);
        }
        ENDTRY

    }
    Exception* e = Exception::newException(hr, description);
    return e;
}

void Document::setReadyStatus(int state)
{
    if (_dwLoadState != (DWORD)state)
    {
        _dwLoadState = (DWORD)state;

        if (_fExited && state == READYSTATE_COMPLETE)
        {
            // Release the read lock since we're done so that the script
            // can now safely change the tree.  The DOM document is guarenteed 
            // to always go to READYSTATE_COMPLETE regardless of errors, etc.
            leaveDOMLoadLock(S_OK);
        }

        FireOnReadyStateChange();
        if (_pParent && state == READYSTATE_COMPLETE)
        {
            _pParent->childFinished(this);
            _pParent = null; // don't need the parent pointer any more.
        }
    }
}

void Document::finalize()
{
    // Release any pointers to functions for event firing
    _pdispOnReadyStateChange = null;
    _pdispOnDataAvailable = null;
    _pdispOnTransformNode = null;
    
    // Blow away the list of connections through IConnectionPointContainer
    ReleaseCPNODEList(_pCPListRoot);

    _pURLdoc = null;
    _pResolvedURLdoc = null;
    _pDTD = null; // blow away pointers to nodes in DTD BEFORE killing the tree...
    _pFactory = null;

    if ( _pDocNode != null)
    {
        // give a hint to the GC about the number of pages to be released !
        reportObjects();
        _pDocNode->detach();
        _pDocNode = null;
    }
    _pNodeMgr = null;
    _pOutputEncoding = null;
    _pParser = null;  
    _pMoniker = null;
    _pParseError = null;
    _pFreeThreadedMarshaller = null;
    _pNamespaceMgr = null;
    _pNameDataType = null;
    _pChildDocs = null;
    _pParent = null;
    _pURN = null;

    _pMutex = null;
    _pMutexLoad = null;
    CloseHandle(_hEventLoad);

    super::finalize();
}


void Document::classExit()
{
    DOMNode::classExit();
}


HRESULT Document::GetLastError()
{
    HRESULT hr;
    if (_pParseError)
        hr = _pParseError->getHRESULT();
    else
        hr = S_OK;
    return hr;
}


#if DBG == 1
LONG _documentCount;
#endif

#if 0
#define _MAX_DOCUMENT_COUNT 100000
LONG _documentIndex;
DWORD _documentTID[_MAX_DOCUMENT_COUNT];
Document * _documents[_MAX_DOCUMENT_COUNT];
long _documentRefs[_MAX_DOCUMENT_COUNT];
#endif

ULONG 
Document::AddRef()
{
    STACK_ENTRY_OBJECT(this);

    return _addRef();
}

ULONG 
Document::Release()
{
    STACK_ENTRY_OBJECT(this);

    return _release();
}

ULONG Document::_addRef()
{
    ULONG r = super::_addRef();
    if (r == 1)
    {
        TraceTag((tagRefCount, "IncrementComponents - ULONG Document::AddRef()"));
#if 0
        InterlockedIncrement(&_documentCount);
        ul = InterlockedIncrement(&_documentIndex);
        if (ul < _MAX_DOCUMENT_COUNT)
        {
            _documentTID[ul] = GetCurrentThreadId();
            _documents[ul] = this;
            _documentRefs[ul] = _refs;
        }
#endif
        ::IncrementComponents();
    }
    return r;
}

ULONG Document::_release()
{
    // 6/18/98 - we want to null out the site object when
    // the refcount goes to zero, but the problem is that 
    // when the count goes to zero the document can be
    // deleted (if we are in GC mode) in which case the site
    // object will be 0xadadadad (in debug builds).  Therefore
    // we first addref, the watch for refcount going to 1
    // then clear the site object, and release at the end.
    super::_addRef(); 

    ULONG r = super::_release();

    if (r == 1)
    {
        // Bug 50776 - must do this here an NOT in ~Document() because
        // we need to release EXTERNAL COM components BEFORE RuntimeExit.
        TraceTag((tagRefCount, "DecrementComponents - ULONG Document::_release()"));
#if 0
        InterlockedDecrement(&_documentCount);
        LONG l = InterlockedIncrement(&_documentIndex);
        if (l < _MAX_DOCUMENT_COUNT)
        {
            _documentTID[l] = GetCurrentThreadId();
            _documents[l] = this;
            _documentRefs[l] = _refs;
        }
#endif
        ::DecrementComponents();

        // Bug 21768 - must null out the site object since this is an
        // apartment model object and cannot be cleaned up later by the 
        // GC in some other thread.
        _pSite = null;
        _pMoniker = null;

        // give a hint to the GC about the number of pages to be released !
        reportObjects();
    }

    // Now really release it !
    r = super::_release();

    if (r == 0)
    {
        Base::testForGC(0);
    }

    return r;
}


//
// Implementation of character entity resolution, includes support
//  for the 5 builtin entities to XML as well as the HTML ones defined
//  to support CDF

// ### 3/20/98 - has been moved to xml\tokenizer\xmlstream\HTMLEnt.?xx

// Event firing helpers/properties

HRESULT Document::FireOnReadyStateChange()
{ 
    HRESULT hr;
    
    if (_pdispOnReadyStateChange || _pCPListRoot)
    {
        BOOL f = _dwLoadState != READYSTATE_COMPLETE;
        if (f)
            registerNonReentrant();
        else if (_dwLoadThreadId) // make sure we can change it now
        {
            removeNonReentrant(_dwLoadThreadId);
            _dwLoadThreadId = 0;
        }

        if (_pdispOnReadyStateChange)
        {
            // BUGBUG: Need to get an IDispatch for the node (?) that caused the event to be fired
            //         so we can pass it through as the "this"
            FireEventThroughInvoke0(
                NULL,
                _pdispOnReadyStateChange, 
                NULL, 
                NULL);
        }

        hr = FireEventWithNoArgsThroughCP(
            DISPID_XMLDOMEVENT_ONREADYSTATECHANGE, 
            _pCPListRoot, 
            &_lSpinLockCPC);

        if (f)
            removeNonReentrant();
    }
    
    return hr;
}

HRESULT Document::FireOnDataAvailable()
{ 
    HRESULT hr;

    if (_pdispOnDataAvailable || _pCPListRoot)
    {
        registerNonReentrant();

        if (_pdispOnDataAvailable)
        {
            // BUGBUG: Need to get an IDispatch for the node (?) that caused the event to be fired
            //         so we can pass it through as the "this"
            FireEventThroughInvoke0(
                NULL,
                _pdispOnDataAvailable, 
                NULL, 
                NULL);
        }

        hr = FireEventWithNoArgsThroughCP(
            DISPID_XMLDOMEVENT_ONDATAAVAILABLE, 
            _pCPListRoot,
            &_lSpinLockCPC);

        removeNonReentrant();

        Assert (SUCCEEDED(hr) && "Error firing events through connection point");
    }
    return hr;
}


HRESULT
Document::putOnReadyStateChange(
    IDispatch *pdisp)
{
    HRESULT hr = S_OK;

    _pdispOnReadyStateChange = pdisp;

Error_exit:
    return hr;
}

HRESULT 
Document::putOnDataAvailable(
    IDispatch *pdisp)
{
    HRESULT hr = S_OK;

    _pdispOnDataAvailable = pdisp;

Error_exit:
    return hr;
}

HRESULT 
Document::putOnTransformNode(
    IDispatch *pdisp)
{
    HRESULT hr = S_OK;

    _pdispOnTransformNode = pdisp;

Error_exit:
    return hr;
}


extern TAG tagDOMOM;

Node * 
Document::createNode( 
    Element::NodeType eType,
    const BSTR bstrName,
    const BSTR bstrNameSpace,
    bool fHasNS)
{
    bool fHasName;
    NameDef * pNameDef = null;

    switch (eType)
    {
    case Element::ELEMENT:       // 2
    case Element::ATTRIBUTE:     // 3
    case Element::PI: // 4
    case Element::ENTITYREF:     // 10
        fHasName = true;
        break;

    case Element::COMMENT:       // 5
    case Element::PCDATA:        // 6
    case Element::CDATA:         // 7
    case Element::DOCFRAG:       // 8
        fHasName = false;
        break;

    case Element::DOCTYPE:       // 11
        Exception::throwE(E_INVALIDARG, XMLOM_INVALID_ONDOCTYPE, null);

    default:
        Exception::throwE(E_INVALIDARG, XMLOM_INVALIDTYPE, Node::NodeTypeAsString(eType), null);
    }

    if (fHasName)
    {
        if (!bstrName || !*bstrName)
            Exception::throwE(E_INVALIDARG, XMLOM_CREATENODE_NEEDNAME, null);
        int len = _tcslen(bstrName);
        if (eType == Node::ATTRIBUTE && 
            5 == len && 
            !memcmp(bstrName, XMLNames::pszXMLNS, 5*sizeof(WCHAR)))
            pNameDef = _pNamespaceMgr->createNameDef(XMLNames::name(NAME_XMLNS), XMLNames::atomXMLNS, XMLNames::atomXMLNS);
        else if (!fHasNS || eType == Node::PI)
        {
            pNameDef = _pNamespaceMgr->createNameDefOM(bstrName, eType != Node::PI);
        }
        else
            pNameDef = _pNamespaceMgr->createNameDef(bstrName, bstrNameSpace);
    }

    if ((!fHasName && fHasNS && bstrNameSpace && *bstrNameSpace) 
        || (pNameDef 
            && ((pNameDef->getPrefix() && eType != Element::ELEMENT && eType != Element::ATTRIBUTE && eType != Node::PI)
                || (bstrNameSpace && *bstrNameSpace && eType != Element::ELEMENT && eType != Element::ATTRIBUTE)
               )))
    {
        // NameSpace qualifier only allowed on PIs, ELEMENTs, and ENTITYREFs
        Exception::throwE(E_INVALIDARG, XMLOM_UNEXPECTED_NS, null);
    }

    // since we are not exposing these nodeTypes anymore... we should be able to create them the same way they look...
    if (eType == Node::PI)
    {
        if (pNameDef->getName() == XMLNames::name(NAME_XML))
            eType = Element::XMLDECL;
    }
    if (eType == Node::ATTRIBUTE &&
        pNameDef->getPrefix() == XMLNames::atomXMLNS &&
        pNameDef->getName()->getName() == XMLNames::atomXML)
    {
        Exception::throwE(E_INVALIDARG,
                          XML_XMLNS_RESERVED,
                          pNameDef->getName()->getName()->toString(),
                          null);
    }

    return Node::newNode( eType, pNameDef, this, getNodeMgr());
}


void
Document::loadXML( String * pStrXML)
{
    IStream* pStream = NULL;
    Stream* stream = StringStream::newStringStream(pStrXML);
    stream->getIStream(&pStream);
    bool wasResolveExternals = getResolveExternals();
    bool wasOnDataAvailableEvents = _fOnDataAvailableEvents;
    TRY
    {
        if (_pSite == null && _dwSafetyOptions != 0)
        {
            // BUGBUG - this hack is here to keep the userData.XMLDocument
            // property exposed by Trident's userData persist secure.
            setResolveExternals(false);
        }
        reset();
        TRY 
        {
            getBaseURL(); 
        }
        CATCH
        {
            // Oh well, we'll try without it then !!
        }
        ENDTRY
        _fOnDataAvailableEvents = false; // these events are not wanted in this case.
        _load(this->_fAsync,NULL,NULL,pStream);
        if (_pParseError)
            _pParseError->throwE();
    }
    CATCH
    {
        setResolveExternals(wasResolveExternals);
        _fOnDataAvailableEvents = wasOnDataAvailableEvents;
        release(&pStream);
        Exception::throwAgain();
    }
    ENDTRY

    setResolveExternals(wasResolveExternals);
    _fOnDataAvailableEvents = wasOnDataAvailableEvents;
    release(&pStream);
}


void
Document::save( String * Url,
                String * Encoding)
{
    Assert( Url);
    HRESULT hr = S_OK;

    TraceTag((tagDOMOM, "Document::save()"));

    IStream* pStm = null;

    TRY { 

        if (_dwSafetyOptions != 0)
        {
            hr = E_ACCESSDENIED;
            goto CleanUp;
        }

        BSTR bstrURL = Url->getBSTR();
        String* baseURL = GetBaseURL();
        hr = ::CreateURLStream(bstrURL, baseURL ? baseURL->getWCHARPtr() : NULL, TRUE, &pStm);
        ::SysFreeString( bstrURL);
        if (FAILED(hr))
            goto CleanUp;

        OutputHelper* o = createOutput(pStm, Encoding);
        TRY
        {
            save(o);
        }
        CATCH
        {
            o->close();
            Exception::throwAgain();
        }
        ENDTRY
        o->close(); // flush
    } 
    CATCH 
    { 
        release( &pStm);
        Exception::throwAgain();
    }
    ENDTRY

    release( &pStm);

CleanUp:
    if (hr)
        Exception::throwE(hr);
}


Node *
Document::nodeFromID(Name* name)
{
    if ( _pDTD == null)
        return null;

    Node* node = (Node*)_pDTD->findID(name);

    if (!node)
    {
        getDocNode()->checkFinished();
    }
    return node;
}

//---------------- Child Documents --------------------------------------------
Document* 
Document::createChildDoc(const WCHAR* pwcBaseURL, Atom* pURN, ParserFlags flags)
{
    Document* doc = clone(false); // shallow clone
    doc->_pParent = this;
    doc->_pBaseURL = pwcBaseURL ? String::newString(pwcBaseURL) : String::emptyString();
    doc->_uParserFlags = flags;
    doc->getDTD()->setSchema(true);

    TraceTag((tagSchemas, "Parent %X, createdChildDoc %X for %s", this, doc, (char *)AsciiText(pURN->toString())));

    if (_pChildDocs == null)
    {
        _pChildDocs = new Vector(4);
    }
    _pChildDocs->addElement((GenericBase*)doc);

    doc->initDefaultFactory(this, pURN); 

    // Strip off the schema URL prefix.
    // BUGBUG - This method is ONLY called to load schemas.  
    // If that changes then this code will have to change).
    int cSchemaPrefixLen = _tcslen(XMLNames::pszSchemaURLPrefix);
    String* pURNstr = pURN->toString();
    Assert(cSchemaPrefixLen < pURNstr->length());
    String* url = pURNstr->substring(cSchemaPrefixLen);
    doc->_pURLdoc = url;
    doc->_fPending = true;

    return doc;
}

bool 
Document::startNextPending()
{
    bool suspended = false;
    if (hasPendingChildDocs())
    {
        if (isAsync() && _pParser)
        {
            // have to suspend current parser
            TraceTag((tagSchemas, "Document %X is suspended", this));
            _pParser->Suspend();
            suspended = true;
        }

        // And kick off the child document.
        Document* child = (Document*)(GenericBase*)_pChildDocs->elementAt(0);
        if (child->_fPending)
        {
            child->_fPending = false;
            child->load(child->_pURLdoc, child->isAsync());
        }
        // childFinished() will be called when this child is done.

        if (suspended && (_pChildDocs == NULL || _pChildDocs->size() == 0) &&
            GetLastError() == S_OK)
        {
            // we don't need to be suspended then
            suspended = false;
            _pParser->Run(0); // resume parser.
        }
    }
    return suspended;
}

void 
Document::adoptChild(Document* child)
{
    TraceTag((tagSchemas, "Child document %X (%s) is reparented into document %X (%s)", child, 
        (char*)AsciiText(child->_pURLdoc), this, (char*)AsciiText(_pURLdoc)));

    if (child->_pParent != null && child->_pParent->_pChildDocs != null)
    {
        child->_pParent->_pChildDocs->removeElement((GenericBase*)child);
    }
    child->_pParent = this;

    if (_pChildDocs == null)
    {
        _pChildDocs = new Vector(4);
    }
    _pChildDocs->addElement((GenericBase*)child);
}

bool
Document::hasPendingChildDocs() const
{
    return (_pChildDocs != null && _pChildDocs->size() > 0);
}

void 
Document::childFinished(Document* child)
{
    TraceTag((tagSchemas, "Child document %X (%s) is finished", child, (char*)AsciiText(child->_pURLdoc)));

    Assert(_pChildDocs != null);

    _pChildDocs->removeElement((GenericBase*)child);
    // Must break circular reference to DTD since DTD now references the 
    // child document in the loadedSchemas hashtable.
    child->_pDTD = null;
    child->_pFactory = null;

    HRESULT hr = child->GetLastError();
    if (hr)
    {
        // Add information about which child document failed.
        String* msg = Resources::FormatMessage(XML_IOERROR, child->_pURLdoc, null);
        Exception* e = child->getErrorMsg();
        if (e != null)
        {
            e->addDetail(msg);
        }
        else
        {
            e = Exception::newException(hr, msg);
        }
        if (isAsync() && ! _fInsideRun)
        {
            if (_pParser) _pParser->Abort(null);
            abort(e);
            _pParser = null;
        }
        else
        {
            // We have to be careful in this case not to totally blow away the
            // parser because the parser Run() method is potentially still on the 
            // stack.  So suspending it will case Run() to return, then it
            // will discover the error and bail out.
            if (_pParser) _pParser->Abort(null);
            _pParseError = e;
        }
    }
    else 
    {
        if (_pChildDocs->size() != 0)
        {
            startNextPending();
        }

        if (_pChildDocs->size() == 0)
        {
            // Notify the factory to check pending nodes
            if (_pFactory)
            {
                _pFactory->NotifyEvent(_pParser, (XML_NODEFACTORY_EVENT)XMLNF_ENDSCHEMA);
            }

            TraceTag((tagSchemas, "Document %X (%s) is resumed", this, (char*)AsciiText(_pResolvedURLdoc)));

            // This document is being loaded asynchronously and it has just
            // received notification that the last pending child document is ready
            // continue on.  Of course, if run() is already on the stack then we don't
            // need to call it again.
            if (isAsync() && ! _fInsideRun && _dwLoadState != READYSTATE_COMPLETE)
                run();
        }
    }
}

Mutex * 
Document::getMutexNonReentrant(DWORD dwTID)
{
    if (_lookaside.count() &&
        _lookaside.search(dwTID))
        return null;
    Assert(_pMutex != null);
    return (Mutex*)_pMutex;
}


DWORD
Document::registerNonReentrant()
{
    DWORD dwTID = GetTlsData()->_dwTID;
    HRESULT hr = _lookaside.add(dwTID);
    if (hr)
        Exception::throwE(hr);
    return dwTID;
}

void
Document::removeNonReentrant(DWORD dwTID)
{
    if (0 == dwTID)
        dwTID = GetTlsData()->_dwTID;
    _lookaside.remove(dwTID);
}

static const int nGrowFactor = 8;

#if 1
// Note: Keep both implementations of LookasideCache around!!
//  the only reason for this is because the _array<> version is too large.
//  this will be looked into and hopefully resolved.

HRESULT 
LookasideCache::add(DWORD dw)
{
    HRESULT hr;
    ULONG_PTR ul = ::SpinLock(&_ul);
    int len = _length;
    int n = 0;
    // scan for empty
    while (n < len)
    {
        if (_pCache[n] == null)
            goto Done;
        n++;
    }
    // no space left
    {
        n = len;
        len += nGrowFactor;
        DWORD* _pOldCache = _pCache;
        DWORD* _pNewCache = new_ne DWORD[len];
        if (!_pNewCache)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        memcpy(_pNewCache, _pOldCache, _length*sizeof(DWORD));
        _pCache = _pNewCache;
        _length = len;
        delete [] _pOldCache;
    }
Done:
    _pCache[n] = dw;
    _count++;
    hr = S_OK;
Cleanup:
    ::SpinUnlock(&_ul, ul);
    return hr;
}

void 
LookasideCache::remove(DWORD dw)
{
    ULONG_PTR ul = ::SpinLock(&_ul);
    int n = 0;
    if (!_pCache)
        goto Done;
    // scan for empty
    while (n < _length)
    {
        if (_pCache[n] == dw)
        {
            _pCache[n] = null;
            _count--;
            goto Done;
        }
        n++;
    }
    Assert(0 && "Unable to find key!!!");
Done:
    ::SpinUnlock(&_ul, ul);
}

bool 
LookasideCache::search(DWORD dw)
{
    DWORD* ppv = _pCache;
    if (!_pCache)
        return false;
    int n = _length;
    // scan for empty
    while (n--)
        if (*ppv++ == dw)
            return true;
    return false;
}

#else

HRESULT 
LookasideCache::add(DWORD dw)
{
    HRESULT hr;
    ULONG_PTR ul = ::SpinLock(&_ul);
    int len = _pCache ? _pCache->length() : 0;
    int n = 0;
    // scan for empty
    while (n < len)
    {
        if ((*_pCache)[n] == null)
            goto Done;
        n++;
    }
    // no space left
    TRY
    {
        n = len;
        if (_pCache)
            _pCache = _pCache->resize(n + nGrowFactor);
        else
            _pCache = new (nGrowFactor) _array<void *>;
    }
    CATCH
    {
        hr = ERESULTINFO;
        goto Cleanup;
    }
    ENDTRY
Done:
    (*_pCache)[n] = dw;
    _count++;
    hr = S_OK;
Cleanup:
    ::SpinUnlock(&_ul, ul);
    return hr;
}

void 
LookasideCache::remove(DWORD dw)
{
    ULONG_PTR ul = ::SpinLock(&_ul);
    int n = 0;
    // scan for empty
    while (n < _pCache->length())
    {
        if ((*_pCache)[n] == dw)
        {
            (*_pCache)[n] = null;
            _count--;
            goto Done;
        }
        n++;
    }
    Assert(0 && "Unable to find key!!!");
Done:
    ::SpinUnlock(&_ul, ul);
}

bool 
LookasideCache::search(DWORD dw)
{
    DWORD* ppv = (DWORD*)_pCache->getData();
    int n = _pCache->length();
    // scan for empty
    while (n--)
        if (*ppv++ == dw)
            return true;
    return false;
}
#endif

/*
 * @(#)XMLDSO.cxx 1.0 6/16/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xmldso.hxx"
#include "dsoctrl.hxx"
#include "ospwrap.hxx"
#include "xmlrowset.hxx"
#include "hashtable.hxx"
#include "xml/dtd/xmlnames.hxx"
#include "msxmlcom.hxx"
#include "islandshared.hxx"

#if DBG==1
#include "xml/util/outputhelper.hxx"        //only for testing - dumptree
#include "core/io/filestream.hxx"        //only for testing - dumptree
#endif

#include "dtd.hxx"
#include "elementdecl.hxx"
#include "attdef.hxx"

extern TAG tagDocRun;
extern TAG tagPointerLeaks;
DeclareTag(tagDSONotify, "XML DSO", "trace notifications");

const IID IID_DataSourceListener = {0x7c0ffab2,0xcd84,0x11d0,{0x94,0x9a,0x00,0xa0,0xc9,0x11,0x10,0xed}};
const IID IID_IXMLDocumentNotify = {0x53BE4F42,0x3602,0x11d2,{0x80,0x1B,0x00,0x00,0xF8,0x7A,0x6C,0xDF}};

#if DBG==1
static void dumpTree(Document *pDoc, char *filename)
{
    FileStream *filestr=new FileStream(filename,true);
    OutputHelper *o = pDoc->createOutput(filestr);
    o->setOutputStyle(OutputHelper::PRETTY);
    pDoc->save(o);
    o->close();
}
#endif

// BUGBUG - this is not asynchronous yet, and in fact, making the auto-schema
// generation asynchronous is going to be tricky.

//--------------------------------------------------------------------
// Internal class used during shape generation. 
DEFINE_CLASS(ShapeNode);
DEFINE_CLASS(ShapeNodeEnvironment);

enum SN_FIELDTYPE_ENUM {
    SN_ELEMENT,
    SN_ATTRIBUTE,
    SN_ROOT,
};

class ShapeNodeEnvironment : public Base
{
    friend class ShapeNode;
public:
    DECLARE_CLASS_MEMBERS(ShapeNodeEnvironment, Base);
    
    ShapeNodeEnvironment(Document *pDoc);

private:
    RDocument       _pDoc;
    RNamespaceMgr   _pNamespaceMgr;
    RShapeNode      _pSNBangText;
};

class ShapeNode : public Base
{
    DECLARE_CLASS_MEMBERS(ShapeNode, Base);

    ShapeNode(NameDef* pNameDef)
    {
        Assert(pNameDef && "No field name!");
        _pNameDef = pNameDef;
    }

public:
    // static constructor
    static ShapeNode * newShapeNode(NameDef* pNameDef);

    ShapeNode* SetField(NameDef* pNameDef, SN_FIELDTYPE_ENUM eFieldType)
    {
        // look up the given field name
        ShapeNode* pSN = GetField(pNameDef->getName());
        
        // add new field if new
        if (pSN == null)
        {
            pSN = newShapeNode(pNameDef);
            AddField(pNameDef->getName(), pSN);
        }
        if (!pSN)
            goto Cleanup;

        // mark how we found the field
        pSN->MarkType(eFieldType);

        // see if it's repeating
        if (eFieldType == SN_ELEMENT && pSN->IsSubElement())
        {
            if (pSN->IsInFamily())
                pSN->SetRepeating();
            pSN->SetInFamily(true);
        }

Cleanup:
        return pSN;
    }

    void AddField(Name* pName, ShapeNode* pSN)
    {
        if (_pFields == null) 
            _pFields = Vector::newVector();

        _pFields->addElement(pSN);
    }

    ShapeNode* GetField(Name* pName)
    {
        ShapeNode *pSNreturn = null;
        
        if (_pFields)
        {
            Enumeration *pEnum = VectorEnumerator::newVectorEnumerator(_pFields);

            while (pEnum->hasMoreElements())
            {
                ShapeNode *pSN = (ShapeNode*) pEnum->nextElement();
                if (pSN->_pNameDef->getName() == pName)
                {
                    pSNreturn = pSN;
                    break;
                }
            }
        }
        
        return pSNreturn;
    }

    bool HasFields()
    {
        return (_pFields != null);
    }

    void ClearFamily()
    {
        if (_pFields)
        {
            Enumeration *pEnum = VectorEnumerator::newVectorEnumerator(_pFields);

            while (pEnum->hasMoreElements())
            {
                ShapeNode *pSN = (ShapeNode*) pEnum->nextElement();

                pSN->SetInFamily(false);
            }
        }
    }
    
    Element * GenerateShapeTree(ShapeNodeEnvironment *pEnv, bool fAttrAndElem=false)
    {
        Element *pElemReturn;
        NameDef *pNameDef = _pNameDef;
        Name *pNameType;
        NamespaceMgr *pNamespaceMgr = pEnv->_pNamespaceMgr;
        bool fRowset = (IsSubElement() && (fAttrAndElem == IsAttribute()) &&
                        (HasFields() || IsRepeating()) ) || IsRoot();

        // create an element with a tag indicating my type
        pNameType = fRowset ? XMLNames::name(NAME_ROWSET)
                            : XMLNames::name(NAME_COLUMN);
        pElemReturn = pEnv->_pDoc->createElement(NULL,
                                          Element::ELEMENT,
                                          pNamespaceMgr->createNameDef(pNameType),
                                          NULL);
        ((Node*)pElemReturn)->setWSFollow(true);    // makes dumpTree output look nicer
        
        // set its name attribute to indicate my field name
        if (fAttrAndElem)
        {
            String *pStrName = _pNameDef->getName()->toString();
            String *pStrPrefix = String::newString("!");
            Name * pNewName = Name::create(String::add(pStrPrefix, 
                                                        pStrName,
                                                        null));

            pElemReturn->setAttribute(XMLNames::name(NAME_CHILDNAME), pNameDef);
            pNameDef = pNamespaceMgr->createNameDef(pNewName);
        }
        pElemReturn->setAttribute(XMLNames::name(NAME_NAME), pNameDef);

        // if I'm a rowset, recursively add my subfields
        if (fRowset)
        {
            Element *pSubElem;

            // start with the real fields
            if (HasFields())
            {
                Enumeration *pEnum = VectorEnumerator::newVectorEnumerator(_pFields);

                while (pEnum->hasMoreElements())
                {
                    ShapeNode *pSN = (ShapeNode*) pEnum->nextElement();
                    pSubElem = pSN->GenerateShapeTree(pEnv);

                    pElemReturn->addChild(pSubElem, null);
                    if (pSN->IsAttribute())
                        pSubElem->setAttribute(XMLNames::name(NAME_ATTR),
                                                String::newString(""));

                    // if the field is both subelement and attribute, make a !Name field
                    if (pSN->IsSubElement() && pSN->IsAttribute())
                    {
                        pSubElem = pSN->GenerateShapeTree(pEnv, true);
                        pElemReturn->addChild(pSubElem, null);
                    }
                }
            }

            // then add the special $Text field
            pSubElem = pEnv->_pSNBangText->GenerateShapeTree(pEnv);
            pSubElem->setAttribute(XMLNames::name(NAME_TEXT),
                                    String::newString(""));
            pElemReturn->addChild(pSubElem, null);
        }

        return pElemReturn;
    }

    void DTDtoShape(DTD *pDTD, ShapeNodeEnvironment *pEnv)
    {
        Name * pCurName = _pNameDef->getName(); // name of current shape node
        ElementDecl * pCurElemDecl = pDTD->findElementDecl(pCurName);   //element declaration in dtd of current shape node

        if (pCurElemDecl == null)
            return;                                     //if current element is not defined, return
    
        ContentModel *pContent = pCurElemDecl->getContent();

        Enumeration * pNames = pContent->getSymbols();  //names of all terminal nodes of current element
        while (pNames->hasMoreElements())               //iterate thru all terminal nodes
        {
            Name * name = (Name*)pNames->nextElement();
            if (!name->equals(XMLNames::name(NAME_PCDATA)))  //if we find a non-text node child
            {
                NameDef * namedef = pEnv->_pNamespaceMgr->createNameDef(name);
                ShapeNode* pSN = SetField(namedef, SN_ELEMENT); //make current shape node to a row, add child
                if (pContent->isRepeatable(name))
                    pSN->SetRepeating();
                pSN->DTDtoShape(pDTD, pEnv);   //recurse
            }
        }

        Enumeration * pAttDefs = pCurElemDecl->getAttDefs();
        if (pAttDefs != null)       //if element has attributes, add colums with attribute names
        {
            while (pAttDefs->hasMoreElements())
            {
                AttDef * pAttDef = (AttDef *)pAttDefs->nextElement();
                if (!pAttDef->isHidden())
                {
                    NameDef * namedef = pEnv->_pNamespaceMgr->createNameDef(pAttDef->getName());
                    ShapeNode * pSN = SetField(namedef, SN_ATTRIBUTE);     //'true' means is attribute
                }
            }
        }
    }


    bool IsAttribute()  { return _fAttribute; }
    bool IsSubElement() { return _fSubElement; }
    bool IsRepeating () { return _fRepeating; }
    bool IsRoot()       { return _fRoot; }
    bool IsInFamily()   { return _fInFamily; }
    void SetRepeating() { _fRepeating = true; }
    void SetInFamily(unsigned f) { _fInFamily = f; }
    
    void MarkType(SN_FIELDTYPE_ENUM eFieldType)
    {
        switch (eFieldType)
        {
        case SN_ELEMENT:
            _fSubElement = true;
            break;

        case SN_ATTRIBUTE:
            _fAttribute = true;
            break;

        case SN_ROOT:
            _fRoot = true;
            break;
        }
    }
    
private:
    RNameDef    _pNameDef;          // name of this field
    RVector     _pFields;           // child fields (for rowsets)

    unsigned    _fAttribute:1;      // this field corresponds to an attribute
    unsigned    _fSubElement:1;     // this field corresponds to a sub-element
    unsigned    _fRepeating:1;      // this field occurs more than once
    unsigned    _fRoot:1;           // this field is the root
    unsigned    _fInFamily:1;       // this field appears in the current family

    virtual void finalize()
    {
        _pFields = null;
        _pNameDef = null;
        super::finalize();
    }
};

DEFINE_ABSTRACT_CLASS_MEMBERS(ShapeNode, _T("ShapeNode"), Base);
DEFINE_ABSTRACT_CLASS_MEMBERS(ShapeNodeEnvironment, _T("ShapeNodeEnvironment"), Base);

// static constructor
ShapeNode *
ShapeNode::newShapeNode(NameDef* pNameDef)
{
    return new ShapeNode(pNameDef);
}

ShapeNodeEnvironment::ShapeNodeEnvironment(Document *pDoc)
{
    _pDoc = pDoc;
    _pNamespaceMgr = _pDoc->getNamespaceMgr();

    Name *pName = Name::create(String::newString("$Text"));
    NameDef *pNameDef = _pNamespaceMgr->createNameDef(pName);
    _pSNBangText = ShapeNode::newShapeNode(pNameDef);
}

///////////////////////////////////////////////////////////////////
//  XMLDSO creation/destruction

DEFINE_CLASS_MEMBERS_NEWINSTANCE(XMLDSO, _T("XMLDSO"), CSafeControl);

XMLDSO::XMLDSO()
{
//    EnableTag(tagDocRun, TRUE);
//    EnableTag(tagDSONotify, TRUE);
//    EnableTag(tagPointerLeaks, TRUE);
    TraceTag((tagDocRun, "XMLDSO (%p) created", this));
    
    _fJavaDSOCompatible=false;
    _fShapeFromFilter=false;
    _fDrillIn = false;
    
    _pMutex = ApartmentMutex::newApartmentMutex();
    _pMutex->Release();             // starts with refcount 1
}

XMLDSO::~XMLDSO()
{
}

void
XMLDSO::finalize()
{
    if (_pDoc)
    {
        _pDoc->RemoveListener(&_Listener);
        _pDoc->SetDSO(null);
        _pDoc = null;
    }

    _pDSL = null;
    _pOSP = null;
    _pShape = null;
    _pMutex = null;

    super::finalize();
}


HRESULT
XMLDSO::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_OBJECT(this);
    HRESULT hr = S_OK;

    TRY 
    {
        if (iid == IID_IUnknown)
        {
            assign(ppv, (GenericBase *)this);
        }
        else if (iid == IID_DataSource)
        {
            *ppv = (DataSource *)new DataSourceWrapper(this, _pMutex);
        }
        else if (iid == IID_IObjectWithSite)
        {
            *ppv = (IObjectWithSite *)new IObjectWithSiteWrapper(this, _pMutex);
        }
        else if (iid == IID_IObjectSafety)
        {
            *ppv = (IObjectSafety *)new IObjectSafetyWrapper(this, _pMutex);
        }
        else if (iid == IID_IDispatch || iid == IID_IXMLDSOControl)
        {
            *ppv = (IXMLDSOControl *)new CXMLDSOControl(this);
        }
        else if (iid == IID_ISupportErrorInfo)
        {
            *ppv = (ISupportErrorInfo *)new CXMLDSOControl(this);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


void
XMLDSO::GetAttributesFromElement(IHTMLElement *pElement)
{
    if (pElement)
    {
        VARIANT var;

        // JavaDSOCompatible
        if (SUCCEEDED(GetExpandoProperty(pElement, String::newString("JavaDSOCompatible"), &var)) &&
            SUCCEEDED(VariantChangeTypeEx(&var, &var, 0, 0, VT_BOOL)))
        {
            setJavaDSOCompatible(!!V_BOOL(&var));
        }
        VariantClear(&var);
    }
}
        
            

void
XMLDSO::AddDocument(DSODocument *pDoc)
{
    if (_pDoc)
    {
        _pDoc->RemoveListener(&_Listener);
    }
    
    // for now, we only support one document
    _pDoc = pDoc;

    if (_pDoc)
    {
        _pDoc->AddListener(&_Listener);
        
        if (_pSite)
        {
            _pDoc->setSite(_pSite);
        }
        _pDoc->setInterfaceSafetyOptions(IID_IUnknown,_dwSafetyOptions,_dwSafetyOptions);
        
        // BUGBUG (sambent) this should be removed when we can QI the XMLIslandPeer
        // for IDataSource.  For now, the QI happens on the document, so it needs
        // a way to get back to the DSO.
        _pDoc->SetDSO(this);

        // this tosses the old shape and create a new one,
        // informing the DataSourceListener that there is a new dataset
        FireDataMemberChanged();
    }
}

void XMLDSO::setSite(IUnknown *u)
{
    super::setSite(u);

    // My doc needs to use the same site, so it gets baseURL etc.
    if (_pDoc)
    {
        _pDoc->setSite(u);

        // look for XML inside the <OBJECT> tag, if any
        IOleControlSite *pControlSite = null;
        IDispatch *pDisp = null;
        IHTMLElement *pHTMLElement = null;
        VARIANT varHTML;

        VariantInit(&varHTML);

        if (u &&
            SUCCEEDED(u->QueryInterface(IID_IOleControlSite, (void**)&pControlSite)) &&
            SUCCEEDED(pControlSite->GetExtendedControl(&pDisp)) && pDisp &&
            SUCCEEDED(pDisp->QueryInterface(IID_IHTMLElement, (void**)&pHTMLElement)) &&
            SUCCEEDED(pHTMLElement->getAttribute(_T("altHTML"), 0, &varHTML))
            )
        {
            TRY
            {
                _pDoc->loadXML(String::newString(V_BSTR(&varHTML)));
            }
            CATCH
            {
                // ignore errors - the doc just stays empty
            }
            ENDTRY
        }

        VariantClear(&varHTML);
        if (pHTMLElement)   pHTMLElement->Release();
        if (pDisp)          pDisp->Release();
        if (pControlSite)   pControlSite->Release();
    }
}

void 
XMLDSO::setInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    super::setInterfaceSafetyOptions(riid, dwOptionSetMask, dwEnabledOptions);

    // need to pass the security options through to the _pDoc also.
    if (_pDoc)
    {
        _pDoc->setInterfaceSafetyOptions(riid, dwOptionSetMask, dwEnabledOptions);
    }
}

///////////////////////////////////////////////////////////////////
//  CDataSource impelementation

IUnknown*
XMLDSO::getDataMember(DataMember bstrDM, REFIID riid)
{
    IUnknown *punk = NULL;
    Element* root;
    HANDLE h;
    Element* pShapeRoot;
    Element* pFirstRowset;
    XMLRowsetProvider *pProvider;
    HRESULT hr;
    
    if (!_pDoc)
        goto Cleanup;

    // we only support the default data member
    if (bstrDM != NULL && *bstrDM != _T('\0'))
        Exception::throwE(E_INVALIDARG);
    
    root = _pDoc->getRoot();
    if (root == NULL)
        goto Cleanup;

    if (_pShape == null)
        goto Cleanup;           //if we have no shape, that means that we are not ready to expose data

    Assert(!!_pShape);
    pShapeRoot = _pShape->getRoot();
    if (pShapeRoot == NULL)
        goto Cleanup;

    // determine where the top rowset lies.  If the root has exactly two children,
    // and the first is a ROWSET node (the last is always the $Text pseudo-field),
    // we drill in one level.  Otherwise we use the root.
    if (_fJavaDSOCompatible ||
        (pShapeRoot->numElements() == 2 &&
         pShapeRoot->getFirstChild(&h)->getTagName() == XMLNames::name(NAME_ROWSET)))
    {
        _fDrillIn = true;
        pFirstRowset = pShapeRoot->getFirstChild(&h);
    }
    else
    {
        root = root->getParent();
        pFirstRowset = pShapeRoot;
    }
    
    if (pFirstRowset == null)
        goto Cleanup;       // XML data contains no repetition !!

    // Create root level provider (no parent).
    pProvider = new XMLRowsetProvider(_pDoc, root, _pShape, pFirstRowset, NULL);
    _pOSP = new OSPWrapper(pProvider, _pDoc->getMutex());
    _pOSP->Release();       // both new and op= incremented refcount
    
    hr = _pOSP->QueryInterface(riid, (void**)&punk);
    if (hr)
        Exception::throwE(hr);

Cleanup:
    return punk;
}


void
XMLDSO::addDataSourceListener(DataSourceListener *pDSL)
{
    _pDSL = pDSL;
}


void
XMLDSO::removeDataSourceListener(DataSourceListener *pDSL)
{
    if (_pDSL == (void*)pDSL)
    {
        _pDSL = null;
    }
}

XMLRowsetProvider * 
XMLDSO::getProvider()
{
    if (!_pOSP)
        return null;
    return _pOSP->getProvider();
}


//======================== PRIVATE METHODS ==================================

void
XMLDSO::DiscardShape()
{
    if (!_fShapeFromFilter)
    {
        _pShape = null;
    }
}


void
XMLDSO::makeShape()
{
    if (_pShape)        //we have already build a Shape so return
        return;
    
    if (makeShapeFromDTD(_pDoc->getDTD()))     //if internal dtd exists
    {
        Assert(_pShape != null);
        // nothing more to do
    }
    else if (_pDoc->getReadyStatus()==READYSTATE_COMPLETE && _pDoc->getRoot())      //if doc is done and root is not null
    {                                                           //need to check root node, because doc's init state is complete
        makeShapeFromData();
    }

#if DBG==1
    if (IsTagEnabled(tagDocRun) && _pShape)
        dumpTree(_pShape, "c:\\shape.xml");     //creates file with XML representation of schema tree
#endif
}

bool
XMLDSO::makeShapeFromDTD(DTD * pDTD)
{                           //only check if internal dtd loaded on doc complete
    bool fReturn = FALSE;
    NameDef *pNameDef;

    if (_fJavaDSOCompatible || !pDTD || pDTD->isEmpty())
        goto Cleanup;
    
    // use docType for ShapeTree root name.  might not have data yet
    // in schema case, there is no doctype, but we will have data by now
    pNameDef = pDTD->getDocType() ? pDTD->getDocType() :
               _pDoc->getRoot()   ? _pDoc->getRoot()->getNameDef()
                                  : null;
    if (!pNameDef)
        goto Cleanup;
    
    TRY
    {
        Document *pDocShape = Document::newDocument();
        ShapeNodeEnvironment *pEnv = new ShapeNodeEnvironment(pDocShape);
        ShapeNode *pShape = ShapeNode::newShapeNode(pNameDef);

        pShape->MarkType(SN_ROOT);
        pShape->DTDtoShape(pDTD, pEnv);   //turn the dtd into a shape
        pDocShape->setRoot((Node*)pShape->GenerateShapeTree(pEnv));
        _pShape = pDocShape;
        fReturn = TRUE;
    }
    CATCH
    {
    }
    ENDTRY

Cleanup:
    return fReturn;
}

void
XMLDSO::makeShapeFromData()
{
    TRY
    {
        Document *pDocShape = Document::newDocument();
        ShapeNodeEnvironment *pEnv = new ShapeNodeEnvironment(pDocShape);
        Element* root = _pDoc->getRoot();
        
        if (root != NULL)
        {
            ShapeNode *pShape = ShapeNode::newShapeNode(root->getNameDef());
            
            pShape->MarkType(SN_ROOT);
            GenerateShape(root, pShape);            // make shape from data
            pDocShape->setRoot((Node*)pShape->GenerateShapeTree(pEnv));
            _pShape = pDocShape;
        }
    }
    CATCH
    {
    }
    ENDTRY
}

void
XMLDSO::FireDataMemberChanged()
{
    // toss the old shape, it is not any good anymore
    DiscardShape();
    // try to build a new shape
    makeShape();

    // Notify data binding agent that entire data set has changed.
    // This results in a full refresh of the entire databound UI.
    if (_pDSL)
    {
        _pDSL->dataMemberChanged(NULL);
    }
}

void 
XMLDSO::GenerateShape(Element* pSource, ShapeNode* pSN)
{
    // restart search for repeaters
    pSN->ClearFamily();
    
    // go thru the children of source and generate shape
    // nodes added later to parent.
    HANDLE h;
    for (Element* e = pSource->getFirstChild(&h); e != NULL; e = pSource->getNextChild(&h))
    {
        int t = e->getType();
        if (t == Element::ELEMENT)
        {
            // now we know that parent is a rowset...
            ShapeNode* sn = pSN->SetField(e->getNameDef(), SN_ELEMENT);
            GenerateShape(e, sn);
        }
    }

    if (!_fJavaDSOCompatible)               //only want to include attributes if we are using new DSO algorithm
    {
        for (Element * pAttr=pSource->getFirstAttribute(&h); pAttr; pAttr=pSource->getNextAttribute(&h))
        {
            ShapeNode * pSNchild = pSN->SetField(pAttr->getNameDef(), SN_ATTRIBUTE);
        }
    }
}


//+--------------------------------------------------------------
//  Member:     FindColumn
//
//  Synopsis:   Find the column number (and child node) matching
//              the given name.
//
//  Arguments:  pNodeShape      shape node to search (should be ROWSET)
//              pName           name to search for
//              plColumn        column number (return)
//              ppNodeChild     child shape node (return)

void
FindColumn(Node *pNodeShape, Name *pName,
            LONG *plColumn, Node **ppNodeChild)
{
    Node *pSNChild;
    void *pTag;
    LONG lColumn;
    
    for (pSNChild = pNodeShape->getNodeFirstChild(&pTag), lColumn = 1;
         pSNChild;
         pSNChild = pNodeShape->getNodeNextChild(&pTag), ++lColumn
        )
    {
        if (((NameDef*)pSNChild->getAttribute(XMLNames::name(NAME_NAME)))->getName() ==
            pName)
        {
            break;
        }
    }

    *plColumn = lColumn;
    if (ppNodeChild)
        *ppNodeChild = pSNChild;
}


//+--------------------------------------------------------------
//  Member:     OnNodeChange
//
//  Synopisis:  This is where internal notifications from the XML document
//              arrive.  Our job is to turn them into OSP notifications.
//
//  Arguments:  eReason     what is happening in the XML tree
//              ePhase      AboutToDo, DidEvent, or FailedToDo
//              pNodeChanged    the node that's changing
//              pNodeParent     its (current, future, or ex-) parent
//              pNodeBefore     its (current, future, or ex-) right sibling
//                              (null means "end of the list")
//
//  Strategy:   We traverse down from the root of the XML tree to the point
//              of change, simultaneously traversing down the shape tree.
//              At "rowset" levels, we compute the index of each XML node
//              among its siblings - this tells us the row number.
//              At "column" levels, the index of the shape node tells
//              us the column number.  Traversing down through a "rowset" node
//              also takes us to a deeper XMLRowsetProvider in the OSP hierarchy.

void
XMLDSO::OnNodeChange(XMLNotifyReason eReason,
                     XMLNotifyPhase ePhase,
                     Node *pNodeChanged,
                     Node *pNodeParent,
                     Node *pNodeBefore)
{
    TraceTag((tagDSONotify, "OnNodeChange(%d, %d, %p, %p, %p)",
                eReason, ePhase, pNodeChanged, pNodeParent, pNodeBefore));

    Stack *pStack = Stack::newStack();
    Node *pNode;
    Node *pNodeShape, *pSNChild;
    void *pTag;
    XMLRowsetProvider *pProvider = null;
    Node *pRow = null;
    long lRow, lColumn;

    if (!_pShape)           
    {
        if (XML_REASON_NodeAdded == eReason && 
            XML_PHASE_DidEvent == ePhase &&
            Element::DOCUMENT == pNodeParent->getNodeType() &&
            Element::ELEMENT == pNodeChanged->getNodeType())
        {
            // build new shape and tell Listener about it
            FireDataMemberChanged();
        }
        else    // cant do anything if we have no shape tree
        {
            return;
        }
    }

    // treat add/remove of a text node as textchange on its parent
    if ((eReason == XML_REASON_NodeAdded || eReason == XML_REASON_NodeRemoved) &&
            Node::PCDATA == pNodeChanged->getType())
    {
        eReason = XML_REASON_TextChanged;
        pNodeChanged = pNodeParent;
        pNodeParent = pNodeChanged->getNodeParent();
    }
    
    // put the path from pNodeChanged up to the root onto the stack
    pStack->push(pNodeChanged);
    for (pNode=pNodeParent; pNode; pNode=pNode->getNodeParent())
    {
        pStack->push(pNode);
    }

    // Begin the downward traversal.  The root levels are a little special.
    // Get the document nodes.
    pNodeShape = _pShape->getDocNode();
    pNode = (Node*) pStack->pop();
    Assert(pNodeShape->getNodeType() == Element::DOCUMENT &&
            pNode->getNodeType() == Element::DOCUMENT);

    // Get the root nodes (if we're using the drill-in rule)
    if (_fDrillIn)
    {
        pNodeShape = pNodeShape->getNodeFirstChild(&pTag);
        if (!pNodeShape)                //if there are no children, we have nothing to do
            return;
        pNode = (Node*) pStack->pop();
        Assert(((NameDef*)pNodeShape->getAttribute(XMLNames::name(NAME_NAME)))->getName() ==
                pNode->getName());
    }

    // treat a change to the root as a brand new dataset
    if (pStack->empty())
    {
        if (ePhase == XML_PHASE_DidEvent)
        {
            // this handles updating the shape
            FireDataMemberChanged();
        }
        return;
    }
    
    // advance level-by-level until we fall off the bottom
    while (!pStack->empty())
    {
        pNode = (Node*) pStack->pop();
        
        // look up the XML node in the shape;  this gives the column number
        Name *pName = pNode->getName();
        FindColumn(pNodeShape, pName, &lColumn, &pNodeShape);
        if (!pNodeShape)
            break;

        // if it's a rowset node, advance to the OSP at the next level
        if (pNodeShape->getName() == XMLNames::name(NAME_ROWSET))
        {
            OSPWrapper *pOSP = pProvider ? (OSPWrapper *)pProvider->FindChildProvider(pRow, lColumn-1)
                                         : (OSPWrapper *)_pOSP;
            if (!pOSP)
            {
                pProvider = null;
                break;
            }
            
            pProvider = pOSP->getProvider();
            pRow = pNode;
        }
    }

    // BUGBUG insert of DOCFRAG really means lots of children are getting
    // inserted.  How do we handle this?
    
    // Now we can fire the notification.
    if (pProvider && pNodeShape && pRow)
    {
        Assert(ePhase == XML_PHASE_AboutToDo || ePhase == XML_PHASE_DidEvent);
        BOOL fBefore = (ePhase == XML_PHASE_AboutToDo);

        // compute the row number
        if (pNodeShape->getName() == XMLNames::name(NAME_COLUMN))
        {
            // insert/delete of a cell counts as a cell change
            eReason = XML_REASON_TextChanged;
            lRow = 1 + pRow->getIndex(true);
        }
        else
        {
            Node *pNodeIndex = ( eReason==XML_REASON_TextChanged ? pRow :
                ((fBefore && eReason==XML_REASON_NodeAdded) ||
                (!fBefore && eReason==XML_REASON_NodeRemoved) )
                    ? pNodeBefore : pNode );

            lRow = 1 + pNodeParent->getChildIndex(pNodeIndex, pNode->getName(), Element::ELEMENT);
        }
        
        switch (eReason)
        {
        case XML_REASON_TextChanged:
            // if the text of a ROWSET node is changing, use the
            // $Text column
            if (pNodeShape->getName() == XMLNames::name(NAME_ROWSET))
            {
                FindColumn(pNodeShape, Name::create(String::newString("$Text")),
                            &lColumn, NULL);
            }
            pProvider->FireCellChange(lRow, lColumn, fBefore);
            break;
        
        case XML_REASON_NodeAdded:
            pProvider->FireRowInsert(lRow, 1, fBefore);
            break;
            
        case XML_REASON_NodeRemoved:
            pProvider->FireRowDelete(lRow, 1, fBefore);
            break;
        }
    }
}


//+----------------------------------------------------------------
// XMLDSO::XMLListener implementation

HRESULT STDMETHODCALLTYPE
XMLDSO::XMLListener::QueryInterface(REFIID riid, void ** ppvObject)
{
    HRESULT hr = S_OK;

    if (ppvObject == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    if (riid == IID_IUnknown || riid == IID_IXMLDocumentNotify)
    {
        *ppvObject = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE
XMLDSO::XMLListener::OnNodeChange(XMLNotifyReason eReason,
                                  XMLNotifyPhase ePhase,
                                  IUnknown *punkNode,
                                  IUnknown *punkNodeParent,
                                  IUnknown *punkNodeBefore)
{
    STACK_ENTRY_OBJECT(MyDSO());
    HRESULT hr = S_OK;

    TRY
    {
        Node *pNode;
        Node *pNodeParent;
        Node *pNodeBefore = NULL;

        // Note:  these QI's do *not* AddRef
        if (SUCCEEDED(punkNode->QueryInterface(Node::s_IID, (void**)&pNode)) &&
            SUCCEEDED(punkNodeParent->QueryInterface(Node::s_IID, (void**)&pNodeParent)) &&
            (punkNodeBefore==NULL || SUCCEEDED(punkNodeBefore->QueryInterface(Node::s_IID, (void**)&pNodeBefore)))
           )
        {
            MyDSO()->OnNodeChange(eReason, ePhase,
                                    pNode, pNodeParent, pNodeBefore);
        }
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


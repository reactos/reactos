/*
 * @(#)DTD.cxx
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "dtd.hxx"
#include "entity.hxx"
#include "elementdecl.hxx"
#include "notation.hxx"
#include "xmlnames.hxx"
#include "dtdstate.hxx"
#include "xml/om/node.hxx"
#include "xml/om/domnode.hxx"
#include "attdef.hxx"

DEFINE_CLASS_MEMBERS(DTD, _T("DTD"), GenericBase);

class IDCheck : public Base
{
friend class DTD;

    DECLARE_CLASS_MEMBERS(IDCheck, Base);

    RNameDef _pNameDef;
    RName    _pID;
    int      _nLine;
    int      _nCol;
    bool     _fImplied;
    RIDCheck _pNext;
    DTD::REFTYPE _dwType;

    IDCheck(IDCheck * next, NameDef * namedef, Name * id, int line, int col, bool implied, 
                DTD::REFTYPE type)
    {
        _pNext = next;
        _pNameDef = namedef;
        _pID = id;
        _nLine = line;
        _nCol = col;
        _fImplied = implied;
        _dwType = type;
    }

    virtual void finalize()
    {
        _pNameDef = null;
        _pID = null;
        _pNext = null;
        super::finalize();
    }

    void check(DTD* dtd);
};

// use ABSTRACT because of no default constructor 
DEFINE_ABSTRACT_CLASS_MEMBERS(IDCheck, _T("IDCheck"), Base);

void
IDCheck::check(DTD* dtd)
{
    HRESULT hr;
    Object* p = null;
    switch (_dwType)
    {
    case DTD::REF_ID:
        p = dtd->findID(_pID);
        hr = XML_ELEMENT_ID_NOT_FOUND;
        break;
    case DTD::REF_NOTATION:
        p = dtd->findNotation(_pID);
        hr = XML_MISSING_NOTATION;
        break;
    }
    if (p == null)
    {    
        String* msg = Resources::FormatMessage(hr, 
                        _pNameDef->toString(), _pID->toString(), null);

        if (_fImplied)
        {
            String* defmsg = Resources::FormatMessage(XML_DEFAULT_ATTRIBUTE, null);
            msg = String::add(msg, defmsg, null);                
        }
        if (_nLine > 0)
        {
            Exception::throwE(hr, msg, _nLine, _nCol);
        }
        Exception::throwE(msg, hr);
    }
}

/**
 * This class contains all the Document Type Definition (DTD) 
 * information for an XML document.
 *
 */

/**
 * Creates a new empty DTD.
 */
DTD::DTD()
{
    _fValidate = false;
    _fSchema = false;
    _fInElementdeclContent = false;
    _pContexts = Stack::newStack();

    // smart ptrs don't need initialization
    // _pIDCheck = null;
    // _pIDs = null;
    // _pUndeclaredElements = null;
    // _pSchemaURNs = null;
    // _pGAttDefList = null;
    // _pMutexIDs = null;
    // _pMutexNotations = null;
    // _pMutexEntities = null;
    // _pSubset = null;
}

/**
 * Adds an entity
 */
void DTD::addEntity(Entity * en)
{
    Assert(_fValidate);
    if (en->par)
    {
        if (_pParEntities == null)
            _pParEntities = Hashtable::newHashtable();
        _pParEntities->put(en->name, (Base *)en);
    }
    else
    {
        if (_pGenEntities == null)
        {
            _pMutexEntities = ShareMutex::newShareMutex();
            _pMutexEntities->Release();  // smartpointer & already addref's
            _pGenEntities = Hashtable::newHashtable(11, _pMutexEntities);
        }
        _pGenEntities->put(en->name, (Base *)en);
    }
}

/**
 * Finds a named entity in the DTD.
 * @param n  The name of the entity.
 * @return  the specified <code>Entity</code> object; returns null if it 
 * is not found. 
 */
Entity * DTD::findEntity(Name * n, bool fParEntity)
{
    Hashtable *pEntities = getEntities(fParEntity);

    // First see if fully qualified entity is defined in DTD
    Object * o = null;
    if (pEntities != null) {
        o = pEntities->get(n);
    }
    if (o != null)
    {
        return (Entity *)(Base *)o;
    }
    return null;
}

Enumeration * DTD::entityDeclarations(bool fParEntity)
{
    Hashtable *pEntities = getEntities(fParEntity);

    return pEntities == null ? EnumWrapper::emptyEnumeration() : pEntities->elements();
}


/*

//MessageId = 92    Facility=Internet   Severity = Error SymbolicName = XML_ENTITYDECL_INVALID
//Language=English
//Declaration for entity %1 is invalid according to the DTD.
//.

void DTD::validateEntities()
{
    if (_pEntities != null) 
    {
        for (Enumeration * e = _pEntities->elements(); e->hasMoreElements();) 
        {
            Entity * en = ICAST_TO(Entity *, e->nextElement());
            if (!en->par && !en->ndata) // internal none-parameter entity
            {
                Node* node;
                if (_fBuildDOMNodes)
                    node = ((DOMNode*)(IUnknown*)en->pNode)->getNodeData();
                else
                    node = ((ElementNode*)(IUnknown*)en->pNode)->getNodeData();

                HRESULT hr = S_OK;
                void* ppv = null;
                for (Node* child = node->getFirstChild(&ppv); child != null; child = node->getNextChild(&ppv))
                {
                    // This is a speed vs. space trade off
                    // We check type here so that we only need to call _validateNode() for
                    // ELEMENT and ENTITYREF nodes
                    switch (child->getType())
                    {
                    case Node::ELEMENT:
                    case Node::ENTITYREF:
                        hr = _validateNode(child, false);
                        break;
                    case Node::COMMENT:
                    case Node::PI:
                    case Node::WHITESPACE:
                    case Node::PCDATA:
                    case Node::CDATA:
                        break;
                    default:
                        hr = XML_INVALID_CONTENT;
                        break;
                    }
                    if (hr != S_OK)
                    {
                        String* msg = Resources::FormatMessage(XML_ENTITYDECL_INVALID, 
                                        en->name->toString(), null);
                        if (en->line > 0)
                        {
                            String* srcmsg = Resources::FormatMessage(
                                        XML_LINE_POSITION, String::newString((int)en->line), 
                                        String::newString((int)en->column), null);

                            msg = String::add(msg, srcmsg, null);
                        }
                        Exception::throwE(msg, 
                            XML_ENTITYDECL_INVALID);
                    }
                }
            }
        }
    }
}

*/

void
DTD::_validateNode(Node* node)
{
    HRESULT hr = S_OK;
    int type = 0;
    Name* name = node->getName();

    switch (node->getType())
    {
    case Element::ELEMENT:
        {
            if (_pCurrent && _pCurrent->ed != null)
            {
                _pCurrent->ed->checkContent(_pCurrent, name, type);
            }

            ElementDecl* ed = findElementDecl(name);
            _pCurrent = DTDState::newDTDState(name, type, ed, null, 0);
            getContexts()->push((Object *)_pCurrent);
            if (ed != null)
                ed->initContent(_pCurrent);
            else
                reportUndeclaredElement(node->getNameDef());

            // validate attributes first


            // validate children then
            bool fEmptyNode = _validateChildNodes(node);

            if (_pCurrent->ed != null)
            {
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
            getContexts()->pop();
            if (getContexts()->empty())
                _pCurrent = NULL;
            else
                _pCurrent = (DTDState*)getContexts()->peek();
        }

        break;

    case Element::ENTITYREF:
        {
            Name* name = node->getName();
            Entity* en1 = findEntity(name, false);
            if (en1 != null)
            {
                // Recurrse into the nested entity and validate it also !!                            
                validateEntity(en1, false);
            }
            else
            {
                hr = XML_ENTITY_UNDEFINED; 
            }
        }
        break;
    case Element::COMMENT:
    case Element::PI:
        break;
    case Element::PCDATA:
    case Element::CDATA:
        if (_pCurrent && _pCurrent->ed != null)
        {
            _pCurrent->ed->checkContent(_pCurrent, name, XML_PCDATA);
        }
        break;
    default:
        hr = XML_INVALID_CONTENT;
        break;
    }

Cleanup:

    if (FAILED(hr))
        Exception::throwE(hr, hr, null);
}


bool 
DTD::_validateChildNodes(Node* node)
{
    bool fEmptyNode = true;
    void* ppv;

    if (node->isCollapsedText())
    {
        _pCurrent->ed->checkContent(_pCurrent, null, XML_PCDATA);
        fEmptyNode = false;
    }
    else
    {
        for (Node* child = node->getNodeFirstChild(&ppv); child != null; child = node->getNodeNextChild(&ppv))
        {
            fEmptyNode = false;
            _validateNode(child);
        }
    }
    return fEmptyNode;
}

void DTD::validateEntity(Entity* en, bool fInAttribute)
{
    HRESULT hr = S_OK;

    if (fInAttribute)
    {
        String * pText = en->getText();
        if (NULL != pText)
        {
            if (pText->indexOf('<') >= 0)
            {
                Exception::throwE(XML_E_BADCHARINSTRING, XML_E_BADCHARINSTRING, null);
            }
        }
        else
        {
            Exception::throwE(XML_EXTENT_IN_ATTR, XML_EXTENT_IN_ATTR, en->getName()->toString(), null);
        }
    }
    else
    {
        Node* node = (Node*)(Object*)en->_pNode;
        
        Assert(node != NULL);

        // This had an <!ENTITY declaration, so we need to walk the children
        // of the entity node and validate them all in the context of the
        // current element declaration.  This is the beginnings of object
        // model validation by the way :-)
       _validateChildNodes(node);
    }

Cleanup:

    if (FAILED(hr)) 
        Exception::throwE(hr, hr, null);
}

void
DTD::reportUndeclaredElement(NameDef * namedef)
{
    if (namedef->getName()->getNameSpace())
    {
        ElementDecl * pElementDecl = findMatchingElementDecl(namedef->toString());
        if (pElementDecl)
        {
            String * s = String::newString(XMLNames::pszXMLNS);
            if (pElementDecl->getNameDef()->getPrefix())
            {
                s = String::add(s, String::newString(_T(":")), 
                    pElementDecl->getNameDef()->getPrefix()->toString(), null);
            }
            Exception::throwE(XML_ATTRIBUTE_FIXED, XML_ATTRIBUTE_FIXED, s, null);
        }
    }
    Exception::throwE(XML_ELEMENT_UNDECLARED,  
                      XML_ELEMENT_UNDECLARED, namedef->toString(), null);
}

DTDState *
DTD::checkEntityRef(Name* name, DTDState * pCurrent, bool fInAttribute)
{
    _pCurrent = pCurrent;
    Entity* pEntity = findEntity(name, false);
    checkEntity(pEntity, name, fInAttribute);
    if ((pCurrent != NULL && pCurrent->ed != null) || fInAttribute)
    {
        getContexts()->push((Object *)_pCurrent);
        validateEntity(pEntity, fInAttribute);
        getContexts()->pop();
    }    
    return pCurrent;
}

void 
DTD::checkEntity(Entity* en, Name* name, bool inAttribute)
{
    HRESULT hr = S_OK;

    if (en == null)
        hr = XML_ENTITY_UNDEFINED;
    else if (en->text == null && inAttribute)
        hr = XML_EXTENT_IN_ATTR;
    else if (en->ndata)
        hr = XML_NDATA_INVALID_REF;

    if (FAILED(hr))
    {
        Exception::throwE(hr, hr, name->getName()->toString(), null);
    }
}

void DTD::addElementDecl(ElementDecl * ed)
{        
    Assert(_fValidate);
    if (_pElementdecls == null)
    {
        _pElementdecls = Hashtable::newHashtable();
    }

    if (_pUndeclaredElements != null)
    {
        // If it's in undeclared elementdecl hashtable, remove it from there
        // This is needed by schemabuilder
        _pUndeclaredElements->remove(ed->_pNamedef->getName());
    }

    _pElementdecls->put(ed->_pNamedef->getName(), (Base *)ed);
}

void DTD::addNotation(Notation * no)
{        
    Assert(_fValidate);
    if (_pNotations == null)
    {
        _pMutexNotations = ShareMutex::newShareMutex(); 
        _pMutexNotations->Release();   // smartpointer & already addref's
        _pNotations = Hashtable::newHashtable(11, _pMutexNotations);
    }
    _pNotations->put(no->name, (Base *)no);
}

void DTD::addID(Name * name, Object *node)
{
    // Note: It used to be true that we only called this if _fValidate was true,
    // but due to the fact that you can now dynamically type somethign as an ID
    // that is no longer true.
    if (_pIDs == null)
    {
        _pMutexIDs = ShareMutex::newShareMutex();
        _pMutexIDs->Release();   // smartpointer & already addref's
        _pIDs = Hashtable::newHashtable(11, _pMutexIDs, false);
    }
    _pIDs->put(name, node);
}

bool DTD::removeID(Name * name)
{
    // Note: It used to be true that we only called this if _fValidate was true,
    // but due to the fact that you can now dynamically type somethign as an ID
    // that is no longer true.
    if (_pIDs == null)
        return false;
    Object * pNode = (Object *)_pIDs->remove(name);
    if (!pNode)
        return false;
    return true;
}

void DTD::addForwardRef(NameDef * namedef, Name* id, int line, int col, bool implied, DTD::REFTYPE type)
{
    _pIDCheck = new IDCheck(_pIDCheck, namedef, id, line, col, implied, type);
}

void DTD::checkForwardRefs()
{
    IDCheck* next = _pIDCheck;

    while(next != null)
    {
        next->check(this);
        IDCheck* ptr = next->_pNext;
        next->_pNext = null; // unhook each object so it is cleaned up by GC
        next = ptr;
    }
    // not needed any more.
    _pIDCheck = null;
}

/**
 *  keep a list of all the DTD's loaded so they are loaded twice if 
 *  two name spaces happen to refer to the same DTD file
 */
Atom * DTD::findLoadedDTD(Atom * url)
{
    if (url == null) return null;
    return _pLoadedDTDs == null ? null : (Atom *)_pLoadedDTDs->get(url);
}

/**
 * add a loaded name space name to the list
 */
void DTD::addLoadedDTD(Atom * url)
{
    if (_pLoadedDTDs == null)
    {
        _pLoadedDTDs = Hashtable::newHashtable();
    }
    _pLoadedDTDs->put(url, url);
}

/**
 * Resets the DTD to its initial state.
 * @return No return value.
 */
void DTD::clear()
{
    _pParEntities = null;
    _pGenEntities = null;
    _pElementdecls = null;
    _pNotations = null;
    _pIDs = null;
    _pMutexIDs = null;
    _pMutexNotations = null;
    _pMutexEntities = null;
    _pIDCheck = null;
    _pDocType = null;
    _pLoadedDTDs = null;
    _pUndeclaredElements = null;
    _pSchemaURNs = null;
    _pGAttDefList = null;
    _pSubset = null;
    _pCurrent = null;

    // reset the state
    _fValidate = false;
    _fSchema = false;
    _fInElementdeclContent = false;
}

ElementDecl* DTD::createDeclaredElementDecl(NameDef* namedef)
{
    // Create a new element decl or remove it from the list of undeclared 
    // element decls (if any).
    ElementDecl* ed = null;
    if (_pUndeclaredElements != null)
    {
        ed = (ElementDecl*)_pUndeclaredElements->get(namedef->getName());
        if (ed != null)
            _pUndeclaredElements->remove(namedef->getName());
    }
    if (ed == null)
        ed = new ElementDecl(namedef);
    return ed;
}

ElementDecl* DTD::createUndeclaredElementDecl(NameDef* namedef)
{
    if (_pUndeclaredElements == null)
        _pUndeclaredElements = Hashtable::newHashtable();

    ElementDecl* ed = (ElementDecl*)_pUndeclaredElements->get(namedef->getName());
    if (ed == null)
    {
        ed = new ElementDecl(namedef);
        _pUndeclaredElements->put(namedef->getName(), ed);
    }
    return ed;
}

Node *
DTD::getDefaultAttributes(Name * name)
{
    ElementDecl * elc = findElementDecl(name);
    if (elc)
        return (Node*)(Object*)(elc->_pNode);
    else
        return null;
}

Node * 
DTD::getDefaultAttrNode(Name * nodeName, Name * attrName )
{
    ElementDecl * elc = findElementDecl(nodeName);
    if (elc && elc->_pNode)
    { 
        AttDef * attdef = elc->getAttDef(attrName);
        if (attdef && !attdef->_fHide)
            return attdef->getDefaultNode();
    }
    return null;
}

Node * 
DTD::getDefaultAttrNode(Name * nodeName, Atom * pAttrBaseName, Atom * pAttrPrefix)
{
    ElementDecl * elc = findElementDecl(nodeName);
    if (elc && elc->_pNode)
    {
        Node * pET = CAST_TO(Node *, (Object *)elc->_pNode);
        return pET->find(pAttrBaseName, pAttrPrefix, Node::ATTRIBUTE, null);
    }
    return null;
}

void 
DTD::addSchemaURN( Atom * pURN, Object * obj)
{
    if (!_pSchemaURNs)
        _pSchemaURNs = Hashtable::newHashtable();
    _pSchemaURNs->put( pURN, obj);
}

bool 
DTD::isLoadingSchema(Atom * pURN)
{
    if (_pSchemaURNs && pURN)
    {
        Object * pObj = _pSchemaURNs->get(pURN);
        if (pObj)
        {
            if (Atom::_getClass()->isInstance(pObj))
                return true;
            else
            {
                Assert(Document::_getClass()->isInstance(pObj));
                Document * pDoc = CAST_TO(Document*, pObj);
                return (pDoc->getReadyStatus() != READYSTATE_COMPLETE);
            }
        }
    }
    return false;
}

Document* 
DTD::getSchema(Atom* pURN)
{
    if (_pSchemaURNs && pURN)
    {
        Object * pObj = _pSchemaURNs->get(pURN);
        if (pObj)
        {
            if (Document::_getClass()->isInstance(pObj))
                return (Document*)(GenericBase*)pObj;
        }
    }
    return null;
}

void 
DTD::addGAttributeType(Name * name, AttDef * ad)
{
    if (!_pGAttDefList)
        _pGAttDefList = Hashtable::newHashtable();
    _pGAttDefList->put(name, ad);
}

AttDef * 
DTD::getGAttributeType(Name * name)
{
    if (!_pGAttDefList)
        return null;
    return (AttDef *) _pGAttDefList->get(name);
}


ElementDecl * 
DTD::findMatchingElementDecl(String * name)
{
    if (_pElementdecls)
    {
        ElementDecl* pDecl;
        Enumeration * pElementDecls = _pElementdecls->elements();
        while (pDecl = (ElementDecl*)pElementDecls->nextElement())
        {
            if (pDecl->getNameDef()->toString()->equals(name))
                return pDecl;
        }
    }
    return null;
}


Object*
DTD::clone()
{
    DTD * pClonedDTD = DTD::newDTD();
    pClonedDTD->_fValidate = _fValidate;
    pClonedDTD->_fSchema = _fSchema;
    pClonedDTD->_pDocType = _pDocType;
    pClonedDTD->_pSubset = _pSubset;
    if (_pParEntities)
        pClonedDTD->_pParEntities = CAST_TO(Hashtable*, ((Hashtable*)_pParEntities)->clone());
    if (_pGenEntities)
        pClonedDTD->_pGenEntities = CAST_TO(Hashtable*, ((Hashtable*)_pGenEntities)->clone());
    if (_pElementdecls)
        pClonedDTD->_pElementdecls = CAST_TO(Hashtable*, ((Hashtable*)_pElementdecls)->clone());
    if (_pNotations)
        pClonedDTD->_pNotations = CAST_TO(Hashtable*, ((Hashtable*)_pNotations)->clone());
    // this is incorrect, since this contains pointers into the document
    //if (_pIDs)
    //    pClonedDTD->_pIDs = CAST_TO(_Hashtable*, ((_Hashtable*)_pIDs)->clone());
    if (_pLoadedDTDs)
        pClonedDTD->_pLoadedDTDs = CAST_TO(Hashtable*, ((Hashtable*)_pLoadedDTDs)->clone());
    if (_pSchemaURNs)
        pClonedDTD->_pSchemaURNs = CAST_TO(Hashtable*, ((Hashtable*)_pSchemaURNs)->clone());
    if (_pGAttDefList)
        pClonedDTD->_pGAttDefList = CAST_TO(Hashtable*, ((Hashtable*)_pGAttDefList)->clone());

    return pClonedDTD;
}


void 
DTD::checkEntityRefLoop(Entity * pEntity)
{
    Assert(pEntity);

    Node*       pNode = (Node*)(Object*)pEntity->_pNode;
    void*       ppv = null;

    Assert(pNode);

    if (pEntity->validating)
    {
        String* s = Resources::FormatMessage(XML_INFINITE_ENTITY_LOOP, pNode->getNameDef()->toString(), NULL);
        Exception::throwE(XML_INFINITE_ENTITY_LOOP, s, pEntity->line, pEntity->column);
    }

    pEntity->validating = true;

    for (Node* child = pNode->getFirstNodeNoExpand(&ppv); child != null; child = pNode->getNodeNextChild(&ppv))
    {
        if (Element::ENTITYREF == child->getNodeType())
            checkEntityRefLoop(findEntity(child->getName(), false));
    }

    pEntity->validating = false;
}


void 
DTD::checkEntityRefs(Node * pCurNode)
{
    void * pTag;
    Document * pDocument = pCurNode->getDocument();
    for (Node * pNode = pCurNode->getFirstNodeNoExpand(&pTag); pNode; pNode = pCurNode->getNodeNextChild(&pTag))
    {
        if (Element::ENTITYREF == pNode->getNodeType() && pDocument)
        {
            Name * pName = pNode->getName();
            if (pName)
            {
                Entity * pEnt = findEntity(pName, false);
                if (!pEnt)
                {
                    Exception::throwE(XML_ENTITY_UNDEFINED, 
                        XML_ENTITY_UNDEFINED, pName->toString(), null);
                }
            }
        }
    }
}

void 
DTD::checkUndeclaredElements()
{
    if (_pUndeclaredElements != NULL && !_pUndeclaredElements->isEmpty())
    {
        ElementDecl * pDecl = (ElementDecl*)_pUndeclaredElements->elements()->nextElement();
        if (!isSchema() || hasSchema(pDecl->getName()->getNameSpace()))
        {
            Exception::throwE(XML_ELEMENT_UNDEFINED, XML_ELEMENT_UNDEFINED, 
                              pDecl->getNameDef()->toString(), null);
        }
    }
}
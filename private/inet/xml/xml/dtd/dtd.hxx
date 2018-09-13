/*
 * @(#)DTD.hxx 1.0
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_PARSER_DTD
#define _XML_PARSER_DTD

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _XML_XMLUTIL_ENUMWRAPPER
#include "core/util/enumwrapper.hxx"
#endif

#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

#ifndef _CORE_UTIL_STACK
#include "core/util/stack.hxx"
#endif

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

DEFINE_CLASS(DTD);
DEFINE_CLASS(IDCheck);
DEFINE_CLASS(DTDState);

class Node;
class Atom;

/**
 * This class contains all the Document Type Definition (DTD) 
 * information for an XML document.
 *
 */

class DTD: public GenericBase
{
friend class ElementDecl;
friend class ElementDeclEnumeration;
friend class DTD;
friend class ContentModel;
friend class Entity;
friend class Notation;
friend class AttDef;

    DECLARE_CLASS_MEMBERS(DTD, GenericBase);

protected:

    /**
     * Default constructor
     */
    DTD();

public:

    // validate() == false imples that there was no DTD (the default)
    bool validate() { return _fValidate; }
    void enableValidation() { _fValidate = true; }

    bool isSchema() { return _fSchema; }
    void setSchema(bool schema = true) { _fSchema = schema; }

    /**
     * Retrieves the name specified in the <code>DOCTYPE</code> tag.
     * @return  the name of the document type.
     */
    NameDef * getDocType() 
    { 
        return _pDocType; 
    }

    void setDocType(NameDef* docType)
    {
        _pDocType = docType;
    }

    String * getSubsetText() const
    {
        return _pSubset;
    }

    void setSubsetText(String * pSubset)
    {
        _pSubset = pSubset;
    }

    /**
     * Retrieves an object for enumerating the element declarations.
     * 
     * @return  an <code>Enumeration</code> object that returns 
     * <code>ElementDecl</code> objects when enumerated
     */
    Enumeration * elementDeclarations()
    {
        return _pElementdecls == null ? EnumWrapper::emptyEnumeration() : _pElementdecls->elements();
    }

    void checkUndeclaredElements();

    /** 
     * Finds an element declaration for the given tag name.
     * @param name  The tag name.
     * @return  the element declaration object.
     */
    ElementDecl * findElementDecl(Name * name)
    {
        return _pElementdecls == null ? null : (ElementDecl *)(Base *)_pElementdecls->get(name);
    }

    /** 
     * Finds an element declaration for the given tag name.
     * @param name  The tag name.
     * @return  the element declaration object.
     */
    ElementDecl * findMatchingElementDecl(String * name);

    Hashtable * getElementDecls()
    {
        return _pElementdecls;
    }

    void setElementDecls(Hashtable * table)
    {
        _pElementdecls = table;
    }

    /**
     * Adds an element declaration
     */
    void addElementDecl(ElementDecl * ed);

    /**
     * Return an enumeration for enumerating the entities
     * The enumeration returns entity objects.
     */
    Enumeration * entityDeclarations(bool fParEntity);

    /**
     * Adds an entity
     */
    void addEntity(Entity * en);

    Node * getDefaultAttributes(Name * name);

    Node * getDefaultAttrNode(Name * nodeName, Name * attrName);
    Node * getDefaultAttrNode(Name * nodeName, Atom * pAttrBaseName, Atom * pAttrPrefix);
    
    /**
     * Finds a named entity in the DTD.
     * @param n  The name of the entity.
     * @return  the specified <code>Entity</code> object; returns null if it 
     * is not found. 
     */
    Entity * findEntity(Name * n, bool fParEntity);

    // Check that the specified entity is defined and can be used in the current state.
    DTDState * checkEntityRef(Name* name, DTDState * pCurrent, bool fInAttribute);
    void checkEntity(Entity* en, Name* name, bool inAttribute);
    void validateEntity(Entity* en, bool fInAttribute);
    void checkEntityRefLoop(Entity * pEntity);

    /**
     * Return an enumeration for enumerating the notations
     * The enumeration returns notation objects.
     */
    Enumeration * notationDeclarations()
    {
        return _pNotations == null ? EnumWrapper::emptyEnumeration() : _pNotations->elements();
    }

    /**
     * Retrieves the named XML notation in the DTD.
     *
     * @param name  The name of the notation.
     * @return the <code>Notation</code> object; returns null if it is not 
     * found.
     */
    Notation * findNotation(Name * name)
    {
        return _pNotations == null ? null : (Notation *)(Base *)_pNotations->get(name);
    }

    void addNotation(Notation * no);   

    /**
     * 
     */
    Object * findID(Name * name)
    {
        return _pIDs == null ? null : _pIDs->get(name);
    }

    void addID(Name * name, Object *node);
    // remove the given name from the ID lookup table.  
    // return TRUE if name was there, false if name was not found
    bool removeID(Name * name);

    enum REFTYPE {
        REF_ID,
        REF_NOTATION,
    };

    // forward references (currently just notations).
    void addForwardRef(NameDef * owner, Name * ref, int line, int col, bool implied, REFTYPE type);
    void checkForwardRefs();

    /**
     *  keep a list of all name spaces loaded so they are loaded twice if 
     *  two name spaces happen to refer to the same DTD file
     */
    Atom * findLoadedDTD(Atom * url);

    /**
     * add a loaded name space name to the list
     */
    void addLoadedDTD(Atom * url);

    /**
     * Resets the DTD to its initial state.
     * @return No return value.
     */
    void clear();

    /**
     * returns true if there is no dtd (this should probably use hashtable->isEmpty()
     * but warnings caused by typecasting, or error if no typecasting
     */
    bool isEmpty()
    {
        return (_pElementdecls == null);
    }

    
    ElementDecl* createDeclaredElementDecl(NameDef* namedef);

    ElementDecl* createUndeclaredElementDecl(NameDef* namedef);

    bool isInElementdeclContent()
    {
        return _fInElementdeclContent;
    }

    void setInElementdeclContent(bool in)
    {
        _fInElementdeclContent = in; 
    }

    bool needFixNames()
    {
        return _fNeedFixNames;
    }

    void setNeedFixNames(bool fNeed)
    {
        _fNeedFixNames = fNeed;
    }

    bool hasSchema(Atom * pURN)
    {
        return (_pSchemaURNs == null || !pURN) ? false : (_pSchemaURNs->get(pURN) != null);
    }

    bool isLoadingSchema(Atom * pURN);
    void addSchemaURN( Atom * pURN, Object * obj);
    Document* getSchema(Atom* pURM);

    // global AttributeType
    void addGAttributeType(Name * name, AttDef * ad);
    AttDef * getGAttributeType(Name * name);

    Object * clone();

    void checkEntityRefs(Node * pCurNode);

    void reportUndeclaredElement(NameDef * namedef);

    // NodeDataNodeFactory for use when building nodes which will become part of this dtd
    IXMLNodeFactory * getNodeFactory() const { return _pNodeFactory; }
    void setNodeFactory(IXMLNodeFactory * pNodeFactory) { _pNodeFactory = pNodeFactory; }

protected:

    Hashtable* getEntities(bool fParEntities)
    {
        return fParEntities ? _pParEntities : _pGenEntities;
    }

    /**
     * validates a given node against its declaration if exists
     */
    void _validateNode(Node* node);
    bool _validateChildNodes(Node* node);
    Stack * getContexts() { return _pContexts; };

    RStack       _pContexts;        // verifying context
    RDTDState    _pCurrent;         // current context pointer
    bool         _fValidate;        // true if there are some rules/decls and we shoudl validate
    bool         _fInElementdeclContent; 
    bool         _fNeedFixNames;  
    bool         _fSchema;

    _reference<IXMLNodeFactory> _pNodeFactory;

    RString _pSubset;

    /**
     * Entity storage
     */    
    RHashtable _pParEntities; // Parameter entities within the DTD

    RHashtable _pGenEntities; // General entities within the document content
    RMutex     _pMutexEntities;


    /**
     * ElementDecl * storage
     */
    RHashtable _pElementdecls;

    RHashtable _pUndeclaredElements;

    /**
     * Notation storage
     */    
    RHashtable _pNotations;
    RMutex     _pMutexNotations;

    /**
     * ID storage
     * 
     * ! WARNING ! this hashtable doesn't addref the 
     * nodes at all, it relies on the object model to
     * remove the nodes from the hashtable when they 
     * are removed from the tree !
     *
     */
    RHashtable _pIDs;
    RMutex _pMutexIDs;


    /**
     * IDs to check later for matching elements
     */
    RIDCheck _pIDCheck;

    /**
     * document type name
     */
    RNameDef _pDocType;

    /**
     * a hashtable recording which actual DTD's have been loaded into this DTD object.
     */
    RHashtable _pLoadedDTDs;

    /**
     * a hashtable of the URNs of schema
     */
    RHashtable _pSchemaURNs;

    /**
     * a hashtable for schema global AttributeType
     */
    RHashtable _pGAttDefList;

    protected: virtual void finalize()
    {
        // please make sure clear is the place we do everything.
        // since we need to be able to re-use a DTD object by calling
        // clear() up in Document.cxx.
        clear();

        _pNodeFactory = null;
        super::finalize();
    }
};

#endif _XML_PARSER_DTD
/*
 * @(#)AttDef.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _ATTDEF_HXX
#define _ATTDEF_HXX

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif

#ifndef _XML_OM_NODE_HXX
#include "xml/om/node.hxx"
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _CORE_UTIL_ATOM
#include "core/util/atom.hxx"
#endif

#ifndef _XML_OUTPUTHELPER
#include "xml/util/outputhelper.hxx"
#endif

#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

#ifndef _XML_CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif

class Node;
typedef _reference<Node> RNode;

DEFINE_CLASS(AttDef);

/**
 * This object describes an attribute type and potential values.
 * This encapsulates the information for one Attribute * in an
 * Attribute * List in a DTD.
 */
class AttDef: public Base
{
friend class ElementDecl;
friend class ElementDeclEnumeration;
friend class DTD;
friend class ContentModel;
friend class Entity;
friend class Notation;
friend class DTDNodeFactory;
friend class SchemaBuilder;

    DECLARE_CLASS_MEMBERS(AttDef, Base);
//    DECLARE_CLASS_INSTANCE(AttDef, Base);

public:

    /**
     * presence of attribute
     */    
    enum
    {
    DEFAULT    = 0,
    REQUIRED   = 1,
    IMPLIED    = 2,
    FIXED      = 3,
    };

protected:
    /**
     * Construct new object for given attribute type.
     * @param name the name of the attribute
     * @param type the attribute type
     */    
    AttDef(Name * name, DataType type);
    AttDef(const AttDef *);

public:
    static AttDef * newAttDef(Name * name, DataType type);
    static AttDef * copyAttDef(const AttDef * other);

    // This cakks checkEnumeration and also checks if attribute
    // is of type #FIXED, and throws an error if it doesn't match
    // the fixed value.
    void checkValue(Object* val);

    // If attribute is ENUMERATION or NOTATION, this throws
    // an error if given object is not in the set.
    void checkEnumeration(Object* val);

    void addValue(Name * val);
    void setValues(Vector * vec);

    bool isHidden() const { return _fHide; }

    /**
     * Return the default value for the attribute.
     */    
    Object * getDefault() const
    {
        return _pDef;
    }

    Node * getDefaultNode();

    /**
     * Return the name of the attribute.
     */
    Name * getName() const
    {
        return _pName;
    }

    void setName(Name* pname)
    {
        _pName = pname;
    }

    /** 
     * Return the attribute type.
     */
    DataType getType() const
    {
        return _datatype;
    }

    void setType(DataType dt)
    {
        _datatype = dt;
    }

    String * getTypeName() const
    {
        return _datatypename;
    }

    void setTypeName(String * s)
    {
        _datatypename = s;
    }

    /**
     * Return the precence type for the attribute.
     */
    int getPresence() const
    {
        return (int)_presence;
    }

    // Takes the innerText of the given node and parses it to see if it complies
    // with this AttDef.  It also adds IDChecks to the DTD for IDs that are
    // referenced but not defined yet.  Also if "def" is true this is the default
    // value for the AttDef itself - and so it initializes the "def" member appropriately.
    void checkAttributeType(IXMLNodeSource *pSource, 
                            Node* pNode, Document* d, 
                            bool defaultValue = false);

    void checkComplete(ElementDecl *);

    // The "schema node" is the element in the schema document
    // which defines this attribute (used by node->getDefinition())
    Node* getSchemaNode() const 
    { 
        return _pSchemaNode; 
    }

    void setSchemaNode(Node* pNode) 
    { 
        _pSchemaNode = pNode; 
    }

protected:

    /**
     * type of attribute is defined by AV_XXXX enums in Element interface.
     */    
    
    String * typeToString();
    TCHAR * typeToString(DataType type);

    String * presenceToString();
    TCHAR * presenceToString(byte presence);

protected:

    /**
     * name of attribute declared
     */
    RName _pName;

    /**
     * default value, can be null
     */    
    RObject _pDef;

	//
	// BUGBUG: HACKHACK: Need to break circular references, in notation.hxx, attdef.hxx, and entity.hxx
    // This used to be a smart pointer (RObject _pNode;)
	// Have to change it this way because currently Node::Addref() addref-s the document when 
	// the reference count goes to 2 (thinking we are handing it out). In the situation we addref it 
	// up to 2 internally, we have circular references 
	//
    Object* _pDefNode;

    // This does not cause a cycle because it's pointing to a node in a different document
    // (The schema document).
    RNode _pSchemaNode;

    // attribute type/datatype
    DataType _datatype;
    RString _datatypename;

    // #IMPLIED vs #FIXED etc..
    byte _presence;

    bool _fDefault;  // keep track if there exist default in global AttributeType

    // Should we generally expose this?
    bool _fHide;     

    /**
     * array of values for enumerated and notation types
     */    
    RVector _pValues;


    virtual void finalize()
    {
        _pName = null;
        _pDef = null;
        _pValues = null;
        _pDefNode = null;
        _pSchemaNode = null;
        super::finalize();
    }
};

#endif _ATTDEF_HXX

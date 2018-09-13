/*
 * @(#)ElementDecl.hxx
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _ELEMENTDECL_HXX
#define _ELEMENTDECL_HXX

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif

#ifndef _XML_OM_NODE_HXX
#include "xml/om/node.hxx"
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

#ifndef _CONTENTMODEL_HXX
#include "contentmodel.hxx"
#endif

#ifndef _CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif


DEFINE_CLASS(ElementDecl);

class DTDState;
class Node;
class DTD;
typedef _reference<Node> RNode;

/**
 * The class represents an element declaration in an XML DTD.
 *
 * @version 1.0, 6/3/97
 */
class ElementDecl : public Base
{    
friend class DTD;
friend class ContentModel;
friend class Entity;
friend class Notation;
friend class AttDef;
friend class DTDNodeFactory;
friend class SchemaBuilder;
friend class ValidationFactory;

    DECLARE_CLASS_MEMBERS(ElementDecl, Base);
//    DECLARE_CLASS_INSTANCE(ElementDecl, Base);

public:

    ElementDecl(NameDef * namedef) 
    {
        _pNamedef = namedef;
        _fIDDeclared = false;
        _iDataType = DT_NONE;
    }

    void initContent(DTDState* context)
    {
        _pContent->initContent(context);
    }

    void checkContent(DTDState* context, Name* name, DWORD type) // throws exception 
    {
        _pContent->checkContent(context, name, type);
    }

    bool acceptEmpty()
    {
        return _pContent->acceptEmpty();
    }

    /**
     * Retrieves the content model object for the element declaration.
     * @return  the content model for the element.
     */
    ContentModel * getContent()
    {
        return _pContent;
    }

   /**
    * Retrieves the name of the element declaration.
    * @return  the <code>Name</code> object containing the element declaration 
    * name.
    */
    Name * getName()
    {
        return _pNamedef->getName();
    }

    NameDef * getNameDef()
    {
        return _pNamedef;
    }

    void setNameDef(NameDef * namedef)
    {
        _pNamedef = namedef;
    }

    Vector * expectedElements(int state)
    {
        return _pContent->expectedElements(state);
    }

    bool isMixed() const
    { 
        return _pContent->_fMixed; 
    }

    bool isOpen() const
    { 
        return _pContent->_fOpen; 
    }

    bool isIDDeclared()
    {
        return _fIDDeclared;
    }

    void setIDDeclared(bool declared)
    {
        _fIDDeclared = declared;
    }

    /**
     * Retrieves the attribute definition of the named attribute.
     * @param name  The name of the attribute.
     * @return  an attribute definition object; returns null if it is not found.
     */
    AttDef * getAttDef(Name * name);

    // return the ID param attribute definition
    Name * getIDAttDefName();

    // returns enumeration of attribute definitions
    Enumeration * getAttDefs()
    {
        if (_pAttdefs==null) return null;
        return _pAttdefs->elements();
    }

    // add a new AttDef to the ElementDecl
    void addAttDef(AttDef * attdef);

    // Check whether all required attributes are presented in a given node
    void checkRequiredAttributes(Node *pNode, DTD* dtd, IXMLNodeSource* pSource);

    DataType getDataType();
    void setDataType(DataType iDataType)
    {
        _iDataType = iDataType;
    }
    String * getDataTypeName() const { return _sDataTypeName; }
    void setDataTypeName(String * s) { _sDataTypeName = s; }

    void setSchemaNode(Node* pNode) { _pSchemaNode = pNode; }
    Node* getSchemaNode() { return (Node*)_pSchemaNode; }

    DWORD getXmlSpace() const { return _dwXmlSpace; }
    void setXmlSpace(DWORD dw) { _dwXmlSpace = dw; }

protected:
    
    RNameDef _pNamedef;       // namedef of element declared
    RContentModel _pContent;  // content model    
    RVector _pAttdefs;        // attribute list  
    bool _fIDDeclared;        // whether an ID attribute has been declared 
    RObject _pNode;
    DWORD _dwXmlSpace;        // info about default xml:space attribute (to be passed via pReserved, from ValidationFactory to NodeDataNodeFactory)

    DataType _iDataType;
    RString _sDataTypeName;

    // This does not cause a cycle because it's pointing to a node in a different document
    // (The schema document).
    RNode _pSchemaNode;

    virtual void finalize()
    {
        _pNamedef = null;
        _pAttdefs = null;
        _pContent = null;
        _pNode = null;
        _pSchemaNode = null;
        _sDataTypeName = null;
        super::finalize();
    }
};

#endif _ELEMENTDECL_HXX
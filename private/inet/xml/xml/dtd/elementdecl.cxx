/*
 * @(#)ElementDecl.cxx
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "elementdecl.hxx"
#include "attdef.hxx"
#include "xmlnames.hxx"
#include "dtd.hxx"
#include "dtdstate.hxx"
#include "xml/om/node.hxx"
#include "xml/om/domnode.hxx"

// use ABSTRACT because of no default constructor 
// DEFINE_CLASS_MEMBERS_CLASS(ElementDecl, _T("ElementDecl"), Base);
DEFINE_ABSTRACT_CLASS_MEMBERS(ElementDecl, _T("ElementDecl"), Base);


/**
 * The class represents an element declaration in an XML DTD.
 *
 */

void ElementDecl::checkRequiredAttributes(Node * pNode, DTD* dtd, IXMLNodeSource* pSource)
{
    Assert(pNode != null);
    Assert(pNode->getNodeType() == Node::ELEMENT);

    if (_pAttdefs != null)
    {
        for (Enumeration * en =  _pAttdefs->elements(); en->hasMoreElements();)
        {
            AttDef * ad = (AttDef *)en->nextElement();
            Name* name = ad->getName();

            bool required = (ad->getPresence() == AttDef::REQUIRED);
            bool hasdefault = (ad->getDefault() != null);
            if (required || hasdefault)
            {                
                // Must use low level find so we don't pick up default attributes.
                Node* pAtt = pNode->find(name, Node::ATTRIBUTE, null);
                if (pAtt)
                    continue;

                // Attribute was not specified.
                if (required)
                {
                    Exception::throwE(XML_REQUIRED_ATTRIBUTE_MISSING, XML_REQUIRED_ATTRIBUTE_MISSING,
                                    ad->getName()->toString(), null);
                }
                else 
                {
                    Assert(hasdefault);
                    // Ok, so default attribute is going to be used because attribute
                    // is not present, so if default attribute references an ID we need 
                    // to make sure that ID is defined somewhere in the document.
                    switch (ad->getType())
                    {
                    case DT_AV_IDREF:
                    case DT_AV_IDREFS:
                        {
                            Name* id = (Name*)(ad->getDefault());
                            Object* p = dtd->findID(id);
                            if (p == null)
                            {   // add it to linked list to check later
                                dtd->addForwardRef(ad->getDefaultNode()->getNameDef(), id, 
                                    pSource->GetLineNumber(),
                                    pSource->GetLinePosition(),
                                    true, DTD::REF_ID);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void ElementDecl::addAttDef(AttDef * attdef)
{
    if (_pAttdefs == null)
    {
        _pAttdefs = Vector::newVector();
    }
    _pAttdefs->addElement(attdef);
}

/**
 * Retrieves the attribute definition of the named attribute.
 * @param name  The name of the attribute.
 * @return  an attribute definition object; returns null if it is not found.
 */
AttDef * ElementDecl::getAttDef(Name * name)
{
    if (_pAttdefs != null)
    {
        for (Enumeration * en =  _pAttdefs->elements(); en->hasMoreElements();)
        {
            AttDef * attdef = (AttDef *)en->nextElement(); 
            if (attdef->_pName == name)
                return attdef;
        }
    }
    return null;
}


Name * ElementDecl::getIDAttDefName()
{
    if (_pAttdefs != null && _fIDDeclared)
    {
        for (Enumeration * en =  _pAttdefs->elements(); en->hasMoreElements();)
        {
            AttDef * attdef = (AttDef *)en->nextElement(); 
            if (attdef->getType() == DT_AV_ID)
                return attdef->getName();
        }
    }
    return null;
}

DataType 
ElementDecl::getDataType()
{
    if (DT_NONE == _iDataType && _pNode)
    {
        Node * pNode = CAST_TO(Node *, (Object *)_pNode);
        if (pNode->isTyped())
            _iDataType = pNode->getNodeDataType();
    }

    return _iDataType;
}

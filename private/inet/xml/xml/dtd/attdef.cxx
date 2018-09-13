/*
 * @(#)AttDef.cxx 1.0 6/3/97
 *
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 */

#include "core.hxx"
#pragma hdrstop

#include "xmlnames.hxx"
#include "attdef.hxx"
#include "elementdecl.hxx"
#include "dtd.hxx"
#include "domnode.hxx"
#include "xml/om/nodedatanodefactory.hxx"

// use ABSTRACT because of no default constructor
//DEFINE_CLASS_MEMBERS_CLASS(AttDef, _T("AttDef"), Base);
DEFINE_ABSTRACT_CLASS_MEMBERS(AttDef, _T("AttDef"), Base);

/**
 * This object describes an attribute type and potential values.
 * This encapsulates the information for one Attribute * in an
 * Attribute * List in a DTD as described below:
 */
AttDef::AttDef(Name * n, DataType t)
{
    _pName = n;
    _datatype = t;
    //_pDef = null;
    //_pDefNode = null;
    _presence = 0;
    _fDefault = false;
    _fHide = false;
    //_pValues = null;
    //_pSchemaNode = null;
}

AttDef::AttDef(const AttDef * other)
{
    _pName      = other->_pName;
    _datatype   = other->_datatype;
    _pDef       = other->_pDef;
    _pDefNode   = other->_pDefNode;
    _presence   = other->_presence;
    _fDefault   = other->_fDefault;
    _pValues    = other->_pValues;
    _pSchemaNode = other->_pSchemaNode;
}

AttDef * 
AttDef::newAttDef(Name * n, DataType dt)
{
    return new AttDef(n, dt);
}

AttDef * 
AttDef::copyAttDef(const AttDef * other)
{
    return new AttDef(other);
}


void 
AttDef::checkEnumeration(Object* val)
{
    if (_datatype == DT_AV_NOTATION || _datatype == DT_AV_ENUMERATION)
    {
        int i;
        for (i = _pValues->size() - 1; i >= 0; i--)
        {
            if ((Name *)_pValues->elementAt(i) == (Name*)val)
                break;
        }
        if (i < 0)
        {
            Exception::throwE(XML_ATTRIBUTE_VALUE, XML_ATTRIBUTE_VALUE, _pName->toString(), null);
        }
    }
}

void
AttDef::checkValue(Object* val)
{
    checkEnumeration(val);

    HRESULT hr = S_OK;
    if (_presence != FIXED)
        goto CleanUp;

    switch(_datatype)
    {
        default:
        case DT_NONE:
        case DT_AV_CDATA:
            Assert(CAST_TO(String*, val));
            if ((!_pDef && CAST_TO(String*, val)->length()) ||
                (_pDef && !_pDef->toString()->equals(CAST_TO(String*, val))))
            {
                hr = XML_ATTRIBUTE_FIXED;
            }
            break;

        case DT_AV_IDREFS:
        case DT_AV_ENTITIES:
        case DT_AV_NMTOKENS:
            if (!SAFE_CAST(Vector *, (Object*)_pDef)->equals(val))
            {
                Exception::throwE(XML_ATTRIBUTE_FIXED, XML_ATTRIBUTE_FIXED, 
                              _pName->toString(), null);
            }
            break;

        case DT_AV_NOTATION:
        case DT_AV_ENUMERATION:
        case DT_AV_ID:
        case DT_AV_IDREF:
        case DT_AV_ENTITY:
        case DT_AV_NMTOKEN:
            if ((Name *)(Object *)_pDef != (Name*)val)
                hr = XML_ATTRIBUTE_FIXED;
            break;
    }

CleanUp:
    if (FAILED(hr))
    {
        Exception::throwE(hr, hr, _pName->toString(), null);
    }
}

void AttDef::addValue(Name * val)
{
    if (_pValues == null)
        _pValues = Vector::newVector();

    _pValues->addElement(val);
}

void AttDef::setValues(Vector * vec)
{
    _pValues = vec;
}


TCHAR * AttDef::typeToString(DataType dt)
{
    switch(dt)
    {
    case DT_AV_ID:
        return (TCHAR *)XMLNames::pszID;
    case DT_AV_IDREF:
        return (TCHAR *)XMLNames::pszIDREF;
    case DT_AV_IDREFS:
        return (TCHAR *)XMLNames::pszIDREFS;
    case DT_AV_ENTITY:
        return (TCHAR *)XMLNames::pszENTITY;
    case DT_AV_ENTITIES:
        return (TCHAR *)XMLNames::pszENTITIES;
    case DT_AV_NMTOKEN:
        return (TCHAR *)XMLNames::pszNMTOKEN;
    case DT_AV_NMTOKENS:
        return (TCHAR *)XMLNames::pszNMTOKENS;
    case DT_AV_NOTATION:
        return (TCHAR *)XMLNames::pszNOTATION;
    case DT_AV_ENUMERATION:
        return (TCHAR *)XMLNames::pszENUMERATION;
    case DT_AV_CDATA:
    default:
        return (TCHAR *)XMLNames::pszCDATA;
    }
}

String * AttDef::typeToString()
{
    return String::newString(typeToString(_datatype));
}

TCHAR * AttDef::presenceToString(byte presence)
{
    switch(presence)
    {
    case IMPLIED:
        return (TCHAR*)XMLNames::pszIMPLIED;
    case REQUIRED:
        return (TCHAR*)XMLNames::pszREQUIRED;
    case FIXED:
        return (TCHAR*)XMLNames::pszFIXED;
    case DEFAULT:
    default:
        return (TCHAR*)XMLNames::pszDEFAULT;
    }
}

String * AttDef::presenceToString()
{
    return String::newString(presenceToString(_presence));
}

Node * AttDef::getDefaultNode()
{
    return (Node*)(Object*)_pDefNode;
}

void
AttDef::checkAttributeType(IXMLNodeSource *pSource, 
                           Node* pNode, Document* d, 
                           bool defaultValue)
{
    TRY 
    {
        DTD* dtd = d->getDTD();

        NamespaceMgr * NSMgr = d->getNamespaceMgr();
        dtd->checkEntityRefs(pNode);
        String* str = pNode->getInnerText(false, true, true); // normalize white space & newlines always.

        Object * obj;
        // if DT_NONE or DT_STRING, do nothing...
        if (_datatype <= DT_AV_CDATA || DT__NON_AV <= _datatype)
        {
            obj = str;
        }
        else
        {
            obj = NSMgr->parseNames(_datatype, str);
        }

        switch (_datatype)
        {
        case DT_AV_CDATA:
        case DT_NONE:
            if (defaultValue)
                _pDef = obj->toString();
            else
                checkValue(SAFE_CAST(String *, obj));
            break;

        case DT_AV_NMTOKEN:
        case DT_AV_NOTATION:  // notations already checked when enumerated in ATTLIST
        case DT_AV_ENUMERATION:   // nothing more to check.
        case DT_AV_ID:        // Duplicate ID's are checked for in the Node::notify method.
            if (defaultValue)
            {
                Name* n = SAFE_CAST(Name *, obj);
                checkEnumeration(n);
                _pDef = n;
            }
            else
            {
                checkValue(SAFE_CAST(Name *, obj));
            }
            break;

        case DT_AV_NAMEDEF:
            // this should never happen !!
            Assert("DT_AV_NAMEDEF unexpected");
            break;

        case DT_AV_IDREF:
        case DT_AV_IDREFS:
        case DT_AV_ENTITY:
        case DT_AV_ENTITIES:
        case DT_AV_NMTOKENS:
            Assert(Vector::_getClass()->isInstance(obj));

            for (Enumeration * e = SAFE_CAST(Vector *, obj)->elements(); 
                 e->hasMoreElements();
                 )
            {
                Name * n = SAFE_CAST(Name *, e->nextElement());

                switch (_datatype)
                {
                case DT_AV_IDREF:
                    obj = n;
                    // fall through

                case DT_AV_IDREFS:
                    // We do not add an IDCheck for AttDef default values
                    // because it may never be used in which case we should not
                    // generate the IDREF error.  But then if an Element instance is
                    // defined and this default AttDef value is implied then we have to 
                    // add the ID check at that point.  (This is done in
                    // ElementDecl::checkRequiredAttributes).
                    if (!defaultValue)
                    {
                        Object* p = dtd->findID(n);
                        if (p == null)
                        {                    // add it to linked list to check later
                            dtd->addForwardRef(pNode->getNameDef(), n, 
                                pSource ? pSource->GetLineNumber() : 0,   // BUGBUG need to store
                                pSource ? pSource->GetLinePosition() : 0, // these in AttDef.
                                false, DTD::REF_ID);
                        }
                    }
                    break;
                case DT_AV_ENTITY:
                    obj = n;
                    // fall through

                case DT_AV_ENTITIES:
                    {
                        Entity* en = dtd->findEntity(n, false);
                        if (en == null)
                        {
                            Exception::throwE(XML_ENTITY_UNDEFINED, 
                                XML_ENTITY_UNDEFINED, n->toString(), null);
                        }
                        if (! en->ndata)
                        {
                            Exception::throwE(XML_REQUIRED_NDATA, 
                                XML_REQUIRED_NDATA, n->toString(), 
                                _pName->toString(), null);
                        }
                    }
                    break;

                case DT_AV_NMTOKENS:  
                    // nothing more to check.
                    break;
                }
            }

            if (defaultValue)
                _pDef = obj;
            else 
                checkValue(obj);

            break;
        }
    }
    CATCH
    {
        if (defaultValue)
        {
            String* msg = Resources::FormatMessage(XML_DEFAULT_ATTRIBUTE, null);
            GETEXCEPTION()->addDetail(msg);
        }
        Exception::throwAgain();
    }
    ENDTRY
    
CleanUp:
    return;
}


void 
AttDef::checkComplete(ElementDecl * pED)
{
    if ((Atom*)XMLNames::atomURNXMLNS == getName()->getNameSpace())
    {
        if (_presence != FIXED)
        {
            Exception::throwE(XML_XMLNS_FIXED, 
                XML_XMLNS_FIXED, 
                String::add(XMLNames::atomXMLNS->toString(), 
                            String::newString(L":"),
                            getName()->toString(), null), null);
        }
    }
    else if (XMLNames::name(NAME_XMLSpace2) == getName())
    {
        Assert(pED);
        Assert(!pED->getXmlSpace());
        Node * pNode = (Node *)(Object *)_pDefNode;
        if (pNode)
        {
            DWORD dwReserved = XMLSPACE_DEFAULT_DEFINED;
            if (ProcessXmlSpace(pNode, false))
                dwReserved |= XMLSPACE_DEFAULT_PRESERVE;
            pED->setXmlSpace(dwReserved);
        }
    }
}

/*
 * @(#)SchemaBuilder.cxx 1.0 8/3/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _VARIANT_HXX
#include "core/com/variant.hxx" 
#endif

#ifndef _SCHEMABUILDER_HXX
#include "schemabuilder.hxx"
#endif

#ifndef _SCHEMANODEFACTORY_HXX
#include "schemanodefactory.hxx"
#endif

#ifndef _SCHEMANAMES_HXX
#include "schemanames.hxx"
#endif

#ifndef _DTDNODEFACTORY_HXX
#include "dtdnodefactory.hxx"
#endif

#include "xml/tokenizer/parser/xmlparser.hxx"

//
// Elements
//
const int SchemaRootElements[]    = {SNODE_ELEMENTTYPE, SNODE_ATTRIBUTETYPE, -1};
const int ElementTypeElements[]   = {SNODE_ELEMENT, SNODE_GROUP, SNODE_ATTRIBUTETYPE, SNODE_ATTRIBUTE, SNODE_ELEMENTDATATYPE, -1};
const int AttributeTypeElements[] = {SNODE_ATTRIBUTEDATATYPE, -1};
const int ElementElements[]       = {-1};
const int AttributeElements[]     = {-1};
const int GroupElements[]         = {SNODE_ELEMENT, SNODE_GROUP, -1};
const int DataTypeElements[]      = {-1};

//
// Attributes
//
const StateAttributes SchemaRootAttributes[] = 
{
    { SCHEMA_NAME, DT_AV_NAMEDEF, &SchemaBuilder::buildSchemaName},
    { -1, 0, 0 }
};
const StateAttributes ElementTypeAttributes[] = 
{
    { SCHEMA_NAME,      DT_AV_NAMEDEF,   &SchemaBuilder::buildElementName},
    { SCHEMA_CONTENT,   DT_AV_NAMEDEF,   &SchemaBuilder::buildElementContent},
    { SCHEMA_MODEL,     DT_AV_NAMEDEF,   &SchemaBuilder::buildElementModel },
    { SCHEMA_DTTYPE,    DT_AV_CDATA,     &SchemaBuilder::buildElementDtType },
    { SCHEMA_ORDER,     DT_AV_NAMEDEF,   &SchemaBuilder::buildElementOrder},
    { -1, 0, 0 }
};

const StateAttributes AttributeTypeAttributes[]   = 
{
    { SCHEMA_NAME,      DT_AV_NAMEDEF,   &SchemaBuilder::buildAttributeName},
    { SCHEMA_DTTYPE,    DT_AV_NAMEDEF,   &SchemaBuilder::buildAttributeDtType},
    { SCHEMA_DTVALUES,  DT_AV_NMTOKENS,  &SchemaBuilder::buildAttributeDtValues},
    { SCHEMA_REQUIRED,  DT_AV_NAMEDEF,   &SchemaBuilder::buildAttributeRequired},
    { SCHEMA_DEFAULT,   DT_AV_CDATA,     &SchemaBuilder::buildAttributeDefault},
    { -1, 0, 0 }
};
const StateAttributes ElementAttributes[]         = 
{
    { SCHEMA_TYPE,      DT_AV_NAMEDEF,   &SchemaBuilder::buildElementType },
    { SCHEMA_MINOCCURS, DT_AV_CDATA,     &SchemaBuilder::buildElementMinOccurs },
    { SCHEMA_MAXOCCURS, DT_AV_CDATA,     &SchemaBuilder::buildElementMaxOccurs },
    { -1, 0, 0 }
};
const StateAttributes AttributeAttributes[]       = 
{
    { SCHEMA_TYPE,      DT_AV_NAMEDEF,   &SchemaBuilder::buildAttributeType },
    { SCHEMA_REQUIRED,  DT_AV_NAMEDEF,   &SchemaBuilder::buildAttributeRequired },
    { SCHEMA_DEFAULT,   DT_AV_CDATA,     &SchemaBuilder::buildAttributeDefault },
    { -1, 0, 0 }
};
const StateAttributes GroupAttributes[]           = 
{
    { SCHEMA_ORDER,     DT_AV_NAMEDEF,   &SchemaBuilder::buildGroupOrder },
    { SCHEMA_MINOCCURS, DT_AV_CDATA,     &SchemaBuilder::buildGroupMinOccurs },
    { SCHEMA_MAXOCCURS, DT_AV_CDATA,     &SchemaBuilder::buildGroupMaxOccurs },
    { -1, 0, 0} 
};
const StateAttributes ElementDataTypeAttributes[]        = 
{
    { SCHEMA_DTTYPE, DT_AV_CDATA, &SchemaBuilder::buildElementDtType },
    { -1, 0, 0} 
};
const StateAttributes AttributeDataTypeAttributes[]        = 
{
    { SCHEMA_DTTYPE, DT_AV_NAMEDEF, &SchemaBuilder::buildAttributeDtType},
    { -1, 0, 0} 
};

const SchemaEntry schemaEntries [] =
{
//      eName,              next states,  attributes, 
//                          initfnc,                            
//                          beginfnc,                           
//                          endfnc  
    {SCHEMA_SCHEMAROOT,     0, SchemaRootElements, SchemaRootAttributes, 
                            null, 
                            null, 
                            null},
    {SCHEMA_ELEMENTTYPE,    0, ElementTypeElements, ElementTypeAttributes, 
                            &SchemaBuilder::initElementType,    
                            &SchemaBuilder::beginElementType,   
                            &SchemaBuilder::endElementType},
    {SCHEMA_ATTRIBUTETYPE,  0, AttributeTypeElements, AttributeTypeAttributes, 
                            &SchemaBuilder::initAttributeType,  
                            &SchemaBuilder::beginAttributeType, 
                            &SchemaBuilder::endAttributeType},
    {SCHEMA_ELEMENT,        0, ElementElements, ElementAttributes, 
                            &SchemaBuilder::initElement,        
                            null,                               
                            &SchemaBuilder::endElement},
    {SCHEMA_ATTRIBUTE,      0, AttributeElements, AttributeAttributes, 
                            &SchemaBuilder::initAttribute,      
                            null,                               
                            &SchemaBuilder::endAttribute},
    {SCHEMA_GROUP,          0, GroupElements, GroupAttributes, 
                            &SchemaBuilder::initGroup,
                            &SchemaBuilder::beginGroup,         
                            &SchemaBuilder::endGroup}, 
    {SCHEMA_DATATYPE,       0, DataTypeElements, ElementDataTypeAttributes, 
                            &SchemaBuilder::initElementDatatype,       
                            null,                               
                            &SchemaBuilder::endElementDtType},
    {SCHEMA_DATATYPE,       0, DataTypeElements, AttributeDataTypeAttributes, 
                            &SchemaBuilder::initAttributeDatatype,       
                            null,                               
                            &SchemaBuilder::endAttributeDtType},
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor & destructor
//
SchemaBuilder::SchemaBuilder(IXMLNodeFactory * fc, DTD * pDTD, NamespaceMgr * pPrevNSMgr, NamespaceMgr * pThisNSMgr, Atom * pURN, Document* pDoc)
:   _pGroupHistory(1), _pStateHistory(10)
{
    _pFactory = fc;
    _pDTD = pDTD;
    _pDoc = pDoc;
    _fValidating = pDoc->_pParent->getValidateOnParse();

    _pPrevNSMgr = pPrevNSMgr;
    _pThisNSMgr = pThisNSMgr;

    _pURN = pURN;           // we need to know the NS for the current schema
                            // (i.e. if we get <s:root xmlns:s="schema.xml"  
                            // => schema.xml will be the NS and "s" is the prefix
}

SchemaBuilder::~SchemaBuilder()
{
    // cleanup the raw stack !!
    GroupInfo ** pTempGroup = _pGroupHistory.peek(); 
    while (pTempGroup != null)
    {
        delete *pTempGroup;
        *pTempGroup = null;
        pTempGroup = _pGroupHistory.pop();
    }

    _ElementInfo.pElementDecl = null;

    _AttributeInfo.pAttDef = null;
    _AttributeInfo.pAttNamedef = null;
    _AttributeInfo.pObjDef = null;

    _ElementInfo.pAttDefList = null;
    
    _pFactory = null;
    _pDTD = null;
    _pThisNSMgr = null;
    _pPrevNSMgr = null;

    _pURN = null;
    _pDoc = null;

    _pSkipNode = null;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// public functions
//
HRESULT
SchemaBuilder::ProcessElementNode(Node * pNode)
{
    HRESULT hr = S_OK;
    Name * name;

    if (!_fSkip)
    {
        name = pNode->getName();
        if (name->getNameSpace() == XMLNames::atomSCHEMAAlias)
        {
            name = Name::create(name->toString(), XMLNames::atomSCHEMA);
        }

        if (_fGotRoot)
        {
            if (getNextState(name))
            {
                push();
                (this->*_pState->pfnInit)(pNode);
            }
            else
            {
                if (isSkipableElement(name))
                {
                    _fSkip = true;
                    _pSkipNode = pNode;
                }
                else
                {
                    Exception::throwE(SCHEMA_ELEMENT_NOSUPPORT, SCHEMA_ELEMENT_NOSUPPORT, pNode->getNameDef()->toString(), null);
                }
                goto Cleanup;
            }
        }
        else
        {
            if (SchemaNames::name(SCHEMA_SCHEMAROOT) == name ||
                SchemaNames::name(SCHEMA_SCHEMAROOT_ALIAS) == name)
            {
                _fGotRoot = true;
                _pState = (SchemaEntry*)&(schemaEntries[SNODE_SCHEMAROOT]);
            }
            else 
            {
                hr = SCHEMA_SCHEMAROOT_EXPECTED;
            }
        }
    }

Cleanup:
    return hr;
}


HRESULT
SchemaBuilder::ProcessAttributes(IXMLNodeSource * pSource, Node * pNode)
{
    HRESULT hr = S_OK;
    int i;
    Object *pObj;
    HANDLE h;
    BuildFunc pfnBuild;
    NameDef * namedef;
    Node * pAttrNode;

    if (!_fSkip)
    {
        namedef = pNode->getNameDef();
        pAttrNode = pNode->getNodeFirstAttribute(&h);
        while (pAttrNode)
        {
            NameDef * attrNameDef = pAttrNode->getNameDef();
            NameDef * pAttrNameValue;
            String * pText = pAttrNode->getInnerText(true, true, true);
            int attrEnumName = -1;
            DataType nSubType;
            Name * attrName = attrNameDef->getName();

            //
            // Check whether the attribute is allowed on the element
            //
            i = 0;
            Assert(_pState->aAttributes);

            while (_pState->aAttributes[i].attribute >= 0)
            {
                if (SchemaNames::name(_pState->aAttributes[i].attribute) == attrName)
                {
                    attrEnumName = _pState->aAttributes[i].attribute;
                    nSubType = (DataType)_pState->aAttributes[i].nSubType;
                    pfnBuild = _pState->aAttributes[i].pfnBuilder;
                    break;
                }
                i++;
            }

            // Check non-supported attribute
            if (_pState->aAttributes[i].attribute < 0)
            {
                if (isSkipableAttribute(attrName))
                {
                    goto Next;
                }
                else
                {
                    Exception::throwE(SCHEMA_ATTRIBUTE_NOTSUPPORT, SCHEMA_ATTRIBUTE_NOTSUPPORT,
                                  attrNameDef->toString(), namedef->toString(), null);
                }
            }

            pObj = _pThisNSMgr->parseNames(nSubType, pText);
            switch (nSubType)
            {
            case DT_NONE:
            case DT_AV_CDATA:
            case DT_AV_NMTOKENS:      
                (this->*pfnBuild)(pSource, pObj);
                if (SCHEMA_DTTYPE == attrEnumName && _pState->eName == SCHEMA_ELEMENTTYPE)
                {
                    endElementDtType(pSource);
                }
                break;
            case DT_AV_NAMEDEF: // AV_NMTOKEN:
                {
                    NameDef * namedef = SAFE_CAST(NameDef *, pObj);

#if DBG == 1
                    char *pText1 = (char*)AsciiText(namedef->toString());
#endif

                    if (namedef->getPrefix() != null)
                    {
#if DBG == 1
                        char *pText1 = (char*)AsciiText(namedef->getPrefix());
                        char *pText2 = (char*)AsciiText(namedef->getName()->getName());
                        char *pText3 = (char*)AsciiText(namedef->getName()->getNameSpace());
#endif

                        if (attrEnumName != SCHEMA_TYPE)    // <attribute type=
                        {
                            hr = SCHEMA_ATTRIBUTEVALUE_NOSUPPORT;
                            goto Cleanup;
                        }
                    }
                    // Now in order to support name space qualified names in 
                    // <element type="..."> we need to include the URN of the schema in the namedef
                    else if ((_pState->eName == SCHEMA_ELEMENTTYPE && attrEnumName == SCHEMA_NAME) ||  // <ElementType name=
                             (_pState->eName == SCHEMA_ELEMENT && attrEnumName == SCHEMA_TYPE))   // <element type=
                    {
                        namedef = _pPrevNSMgr->createNameDef(namedef, _pURN, _pURN);
                    }
                    (this->*pfnBuild)(pSource, namedef);
                }
                break;
        
            }

Next:
            // get next attribute
            pAttrNode = pNode->getNodeNextAttribute(&h);
        }

        if (_pState->pfnBC)
            (this->*_pState->pfnBC)(pSource);
    }

Cleanup:
    return hr;
}


HRESULT
SchemaBuilder::ProcessPCDATA(Node * pNode, PVOID pParent)
{
    HRESULT hr = S_OK;

    if (!_fSkip && pParent && CAST_TO(Node*, (Object*)pParent)->getNodeType() == Element::ELEMENT &&
        _pState->eName != SCHEMA_DESCRIPTION &&
        _pState->eName != SCHEMA_DATATYPE)
    {
        hr = XML_ILLEGAL_TEXT;
    }
        
    return hr;
}


HRESULT
SchemaBuilder::ProcessEndChildren(IXMLNodeSource *pSource, Node * pNode)
{
    if (!_fSkip)
    {
        if (_pState->pfnEC)
        {
            (this->*_pState->pfnEC)(pSource);
        }
        pop();
    }
    else
    {
        if (pNode == _pSkipNode)
            _fSkip = false;
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper public functions
//

void 
SchemaBuilder::start()
{
    _ElementInfo.pElementDecl = null;
    _ElementInfo.pAttDefList = null;
    _AttributeInfo.pAttDef = null;
    _fGotRoot = false;
    _fSkip = false;

    // enable DTD validation, but mark DTD as being associated with a Schema.
    // so that we can apply slightly looser validation rules, like allowing
    // elements that are not declared and so forth.
    _pDTD->enableValidation(); 
    _pDTD->setSchema();
}


void 
SchemaBuilder::finish()
{
    // check undeclared elements
    _pDTD->checkUndeclaredElements();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//  Private functions
//

//
// State stack push & pop
//
void 
SchemaBuilder::push()
{
    SchemaEntry ** pTempState = _pStateHistory.push();
    
    if (pTempState)
    {
        *pTempState = _pState;
        _pState = _pNextState;
    }
    else
    {
        Exception::throwE(E_OUTOFMEMORY, E_OUTOFMEMORY, null);
    }
}


void 
SchemaBuilder::pop()
{
    SchemaEntry ** pTempState = _pStateHistory.peek();
    if (pTempState != null)
    {
        _pState = *pTempState;
        _pStateHistory.pop();
    }
    else
        _pState = (SchemaEntry *) &schemaEntries[SNODE_ELEMENTTYPE];
}


///////////////////////////////////////////////////////////////////////////////////////////
// init functions: for PocessingElementNode calls
//

void SchemaBuilder::initElementType(Node* pNode)
{
    _ElementInfo.pElementDecl = new ElementDecl(null); 
    _ElementInfo.pElementDecl->_pContent = ContentModel::newContentModel();
    _ElementInfo.pElementDecl->_pContent->start();
    _ElementInfo.pElementDecl->_pContent->setType(ContentModel::EMPTY);
    _ElementInfo.pElementDecl->_pContent->setOpen(true);
    _ElementInfo.pElementDecl->setSchemaNode(pNode);

    _ElementInfo.fGotName = false;
    _ElementInfo.fGotContent = false;
    _ElementInfo.fGotModel = false;
    _ElementInfo.fGroupDisabled = false;
    _ElementInfo.fGotOrder = false;
    _ElementInfo.fGotContentMixed  = false;
    _ElementInfo.fMasterGroupRequired = false; 
    _ElementInfo.fExistTerminal = false;
    _ElementInfo.fAllowDatatype = false;
    _ElementInfo.fGotDtType = false;
    _ElementInfo.pAttDefList = Hashtable::newHashtable();

    _AttributeInfo.fGotDtType = false;
    _AttributeInfo.fDefault = false;
    _AttributeInfo.fProcessingDtType = false;
}


void 
SchemaBuilder::initElement(Node* pNode)
{
    HRESULT hr = S_OK;

    // if Group is Disabled that means textOnly is used, thus no more group nor element should be created
    if (_ElementInfo.fGroupDisabled)
    {
        hr = SCHEMA_ELEMENT_DISABLED;
        goto Error;
    }
    else if (_ElementInfo.fGotDtType)
    {
        hr = SCHEMA_ELEMENT_DATATYPE;
        goto Error;
    }
    else if (ContentModel::EMPTY != _ElementInfo.pElementDecl->_pContent->getType())
    {
        _ElementInfo.fGotType = false;
        _ElementInfo.nMinVal = 1;
        _ElementInfo.nMaxVal = 1;
    }
    else
    {
        hr = SCHEMA_ELEMENT_EMPTY;
        goto Error;
    }

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::initAttributeType(Node* pNode)
{
    _AttributeInfo.pAttDef = AttDef::newAttDef(null, DT_NONE);
    _AttributeInfo.pAttDef->setSchemaNode(pNode);

    _AttributeInfo.fGotName = false;
    _AttributeInfo.fRequired = false;
    _AttributeInfo.pAttNamedef = null;
    _AttributeInfo.fGotDefault = false;

    _AttributeInfo.fBuildDefNode = false;
    _AttributeInfo.nMinVal = 0; // optional by default.
    _AttributeInfo.nMaxVal = 1;

    // used for datatype
    _AttributeInfo.fEnumerationRequired = false;
    _AttributeInfo.fGotDtType = false;
    _AttributeInfo.fGlobal = (_pStateHistory.used() == 1);
    _AttributeInfo.fDefault = false;
    _AttributeInfo.fProcessingDtType = false;

}


void 
SchemaBuilder::initAttribute(Node* pNode)
{
    _AttributeInfo.fGotType = false;
    _AttributeInfo.pAttNamedef = null;
    _AttributeInfo.fGotDefault = false;
    _AttributeInfo.fRequired = false;
    _AttributeInfo.fBuildDefNode = false;
    _AttributeInfo.fGlobal = false;
    _AttributeInfo.fDefault = false;
}


void 
SchemaBuilder::initElementDatatype(Node* pNode)
{
    HRESULT hr = S_OK;
    if (_ElementInfo.fGotDtType)
    {
        hr = SCHEMA_DTTYPE_DUPLICATED;
        goto Error;
    }
    else if (_ElementInfo.fGotContent && !_ElementInfo.fAllowDatatype)
    {
        hr = SCHEMA_DTTYPE_DISABLED;
        goto Error;
    }

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::initAttributeDatatype(Node* pNode)
{
    if (_AttributeInfo.fGotDtType)
        Exception::throwE(SCHEMA_DTTYPE_DUPLICATED, SCHEMA_DTTYPE_DUPLICATED, null);
}


void 
SchemaBuilder::initGroup(Node* pNode)
{
    // if Group is Disabled that means textOnly is used, thus no more group nor element should be created
    if (!_ElementInfo.fGroupDisabled)
    {       
        pushGroupInfo();

        _GroupInfo.nMinVal = 1;
        _GroupInfo.nMaxVal = 1;
        _GroupInfo.fGotMax = false;
        _GroupInfo.fGotMin = false;
              
        if (_ElementInfo.fExistTerminal)
            addOrder();
        
        // now we are in a group so we reset fExistTerminal
        _ElementInfo.fExistTerminal = false;
        
        _ElementInfo.pElementDecl->_pContent->openGroup();
    }
    else
    {
        Exception::throwE(SCHEMA_GROUP_DISABLED, SCHEMA_GROUP_DISABLED, null);
    }
}


////////////////////////////////////////////////////////////////////////////////////
// begin functions: for BeginChildren Calls
//

void 
SchemaBuilder::beginElementType(IXMLNodeSource __RPC_FAR *pSource)
{
    HRESULT hr = S_OK;

    if (_ElementInfo.fGotName)
    {
        if (!_ElementInfo.fGotContent)           // if there is no content, we set the default value to SCHEMA_MIXED
        {
            buildElementContent(pSource, _pPrevNSMgr->createNameDef(SchemaNames::name(SCHEMA_MIXED)));
            _ElementInfo.fGotContent = false;
        }

        if (!_ElementInfo.fGotModel)        // default model for schema is open
            buildElementModel(pSource, _pPrevNSMgr->createNameDef(SchemaNames::name(SCHEMA_OPEN)));

        if (_ElementInfo.fGotContentMixed && _GroupInfo.cCurrentOrder != eMany)
        {
            hr = SCHEMA_ETORDER_DISABLED;
            goto Error;
        }
    }
    else
    {
        hr = SCHEMA_ETNAME_MISSING;
        goto Error;
    }

    // normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::beginAttributeType(IXMLNodeSource __RPC_FAR *pSource)
{
    if (_AttributeInfo.fGotName)
    {                
        _AttributeInfo.fBuildDefNode = false;  
    }
    else
    {
        Exception::throwE(SCHEMA_ATNAME_MISSING, SCHEMA_ATNAME_MISSING, null);
    }
}


void 
SchemaBuilder::beginGroup(IXMLNodeSource __RPC_FAR *pSource)
{
    if (_ElementInfo.fGotContentMixed && _GroupInfo.cCurrentOrder != eMany)
    {
        Exception::throwE(SCHEMA_ETORDER_DISABLED, SCHEMA_ETORDER_DISABLED, null);
    }
}


////////////////////////////////////////////////////////////////////////////////////
// build functions: for different attributes
//

void 
SchemaBuilder::buildElementName(IXMLNodeSource * pSource, Object * nd)
{
    NameDef * namedef = SAFE_CAST(NameDef *, nd);
    Name * name = namedef->getName();

    _ElementInfo.fGotName = true;
    if (_pDTD->findElementDecl(name))
    {
        Exception::throwE(XML_ELEMENT_DEFINED, XML_ELEMENT_DEFINED, namedef->toString(), null);
    }
    
    _ElementInfo.pElementDecl->_pNamedef = namedef;

    _pDTD->addElementDecl(_ElementInfo.pElementDecl);

    if (_ElementInfo.pElementDecl->_pNode)
    {
        CAST_TO(Node*, (Object*)_ElementInfo.pElementDecl->_pNode)->setName(namedef);
    }
}


void 
SchemaBuilder::buildElementContent(IXMLNodeSource * pSource, Object * namedef)
{
    HRESULT hr = S_OK;
    Name * name = SAFE_CAST(NameDef *, namedef)->getName();

    if (_AttributeInfo.fGotDtType && name != SchemaNames::name(SCHEMA_TEXTONLY))
    {
        hr = SCHEMA_ELEMENTDT_NOSUPPORT;
        goto Error;
    }

    _ElementInfo.fGotContent = true;

    if (name != SchemaNames::name(SCHEMA_EMPTY))      // since empty is the current default value, 
                                                      // there is no need to set content type to empty again.
    {
        _ElementInfo.pElementDecl->_pContent->setType(ContentModel::ELEMENTS);   
        _ElementInfo.fMasterGroupRequired = true;
        _ElementInfo.pElementDecl->_pContent->openGroup();
        
        // eltOnly, textOnly, mixed
        if (name == SchemaNames::name(SCHEMA_MIXED))       
        {   
            
            _ElementInfo.fGotContentMixed = true;

            _ElementInfo.pElementDecl->_pContent->addTerminal(XMLNames::name(NAME_PCDATA));

            if (!_ElementInfo.fGotOrder)
                buildElementOrder(pSource, _pPrevNSMgr->createNameDef(SchemaNames::name(SCHEMA_MANY)));
            
            // Mark that there is at least one Terminal
            _ElementInfo.fExistTerminal = true;
        }
        else if (name == SchemaNames::name(SCHEMA_ELTONLY))
        {
            if (!_ElementInfo.fGotOrder)
                setOrder(SchemaNames::name(SCHEMA_SEQ));                                                       // default is SEQ for ELEMENTS
        }
        else if (name == SchemaNames::name(SCHEMA_TEXTONLY))
        {
            // we need to open group of SCH::textOnly == DTD::(#PCDATA)
            // open group -> addTerminal(XMLNames::name(NAME_PCDATA))
            
            _ElementInfo.pElementDecl->_pContent->addTerminal(XMLNames::name(NAME_PCDATA));

            _ElementInfo.fAllowDatatype = true;
            _ElementInfo.fExistTerminal = true;
            
            _ElementInfo.fGroupDisabled = true;
        }
        else 
        {
            hr = SCHEMA_ETCONTENT_UNKNOWN;
            goto Error;
        }
    }

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::buildElementOrder(IXMLNodeSource * pSource, Object * namedef)
{
    Name * name = SAFE_CAST(NameDef *, namedef)->getName();
    _ElementInfo.fGotOrder = true;
    setOrder(name);
}


void 
SchemaBuilder::buildElementModel(IXMLNodeSource * pSource, Object * namedef)
{
    Name * name = SAFE_CAST(NameDef *, namedef)->getName();

    _ElementInfo.fGotModel = true;

    if (name == SchemaNames::name(SCHEMA_CLOSE))
        _ElementInfo.pElementDecl->_pContent->setOpen(false);
    else if (name != SchemaNames::name(SCHEMA_OPEN))
        Exception::throwE(SCHEMA_ETMODEL_UNKNOWN, SCHEMA_ETMODEL_UNKNOWN, null);
}


void 
SchemaBuilder::buildAttributeName(IXMLNodeSource * pSource, Object * nd)
{
    HRESULT hr = S_OK;
    NameDef * namedef = SAFE_CAST(NameDef *, nd);
    Name * name = namedef->getName();
    
    _AttributeInfo.fGotName = true;
    _AttributeInfo.pAttNamedef = namedef;
    _AttributeInfo.pAttDef->_pName = name;

    if (_ElementInfo.pElementDecl != null)          // Local AttributeType
    {
        if ((AttDef *)_ElementInfo.pAttDefList->get(name) == null)
            _ElementInfo.pAttDefList->put(name, _AttributeInfo.pAttDef);
        else
        {
            hr = SCHEMA_ATNAME_DUPLICATED;
            goto Error;
        }
    }
    else                                            // Global AttributeType
    {
        // Global AttributeTypes are URN qualified so that we can look them up
        // across schemas.
        namedef = _pPrevNSMgr->createNameDef(namedef, _pURN, _pURN);
        name = namedef->getName();
        _AttributeInfo.pAttDef->_pName = name;

        if (_pDTD->getGAttributeType(name) == null)
            _pDTD->addGAttributeType(name, _AttributeInfo.pAttDef);
        else
        {
            hr = SCHEMA_ATNAME_DUPLICATED;
            goto Error;
        }
    }

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::buildAttributeDefault(IXMLNodeSource * pSource, Object * str)
{
    _AttributeInfo.fGotDefault = true;   
    _AttributeInfo.fBuildDefNode = true;
    if (_AttributeInfo.pAttDef)
    {
        _AttributeInfo.pAttDef->_fDefault = true;   // BUGBUG, I might be able to optimize fBuildDefNOde with _fDefault
        _AttributeInfo.pAttDef->_pDef = str;
    }
    else
    {
        _AttributeInfo.pObjDef = str;
        _AttributeInfo.fDefault = true;
    }
}


void 
SchemaBuilder::buildAttributeRequired(IXMLNodeSource * pSource, Object * namedef)
{
    Name * name = SAFE_CAST(NameDef *, namedef)->getName();
  
    if (name == SchemaNames::name(SCHEMA_YES))
        _AttributeInfo.fRequired = true;
    else if (name == SchemaNames::name(SCHEMA_NO))
        _AttributeInfo.fRequired = false;
    else
        Exception::throwE(SCHEMA_ATREQUIRED_INVALID, SCHEMA_ATREQUIRED_INVALID, null);
}


void 
SchemaBuilder::buildElementDtType(IXMLNodeSource * pSource, Object * str)
{
    // if we get content, it must be eltOnly.. otherwise, we can't have a dt:type..
    if (_ElementInfo.fGotContent && !_ElementInfo.fAllowDatatype)
    {
        Exception::throwE(SCHEMA_ELEMENTDT_NOSUPPORT, SCHEMA_ELEMENTDT_NOSUPPORT, null);
    }

    // simulate the situation of having create <AttributeType name="dt:dt"
    initAttributeType(null);
    buildAttributeName(pSource, _pPrevNSMgr->createNameDef(XMLNames::name(NAME_DTDT)));

    _AttributeInfo.fRequired = true;
    _AttributeInfo.pAttDef->_datatype = DT_AV_CDATA;             // this should be default
    _AttributeInfo.pAttDef->_fHide = true;                       // we don't want others to see this attribute
    
    if (str->toString()->isWhitespace())
    {
        Exception::throwE(SCHEMA_ELEMENTDT_EMPTY, SCHEMA_ELEMENTDT_EMPTY, null);
    }
    buildAttributeDefault(pSource, str);
    _ElementInfo.fGotDtType = true;
}


void 
SchemaBuilder::buildAttributeDtType(IXMLNodeSource * pSource, Object * namedef)
{
    Name * name = SAFE_CAST(NameDef *, namedef)->getName();
    String * strname = name->toString()->trim();
    DataType type = checkDtType(strname);
    if (type < DT_USER_DEFINED)
    {
        _AttributeInfo.fGotDtType = true;
        _AttributeInfo.pAttDef->setType( type);
    }
    else if (!_fValidating)
    {
        // save info for OM/runtime error reporting
        _AttributeInfo.pAttDef->setType( type);
        _AttributeInfo.pAttDef->setTypeName( strname);
    }
    else
    {
        Exception::throwE(SCHEMA_DTTYPE_UNKNOWN, XMLOM_INVALID_DATATYPE, strname, NULL);
    }

}


void 
SchemaBuilder::buildAttributeDtValues(IXMLNodeSource * pSource, Object * values)
{
    _AttributeInfo.fEnumerationRequired = true;
    _AttributeInfo.pAttDef->setValues((Vector *)values);    
}


void 
SchemaBuilder::buildElementType(IXMLNodeSource * pSource, Object * nd)
{
    NameDef* namedef = SAFE_CAST(NameDef *, nd);
    Name * name = namedef->getName();

    if (!_pDTD->findElementDecl(name))
    {
        _pDTD->createUndeclaredElementDecl(namedef);
    }

    _ElementInfo.fGotType = true;  
    if (_ElementInfo.fExistTerminal)  
        addOrder();
    else
        _ElementInfo.fExistTerminal = true;
    _ElementInfo.pElementDecl->_pContent->addTerminal(name);        
}

HRESULT
parseInteger(String * str, int * pn)
{
    VARIANT v;
    v.vt = VT_EMPTY;

    str->AddRef();
    HRESULT hr = ParseDatatype(str->getWCHARPtr(), str->length(), DT_I4, &v, null);
    str->Release();

    if (hr)
        return E_FAIL;

    *pn = V_I4(&v);
    return S_OK;
}

void 
SchemaBuilder::buildElementMinOccurs(IXMLNodeSource * pSource, Object * s)
{
    HRESULT hr = S_OK;

    hr = parseInteger(s->toString(), &_ElementInfo.nMinVal);
    if (hr) 
    {
        hr = SCHEMA_MINOCCURS_INVALIDVALUE;
        goto Error;
    }

    if (_ElementInfo.nMinVal != 0 && _ElementInfo.nMinVal != 1)
    {
        hr = SCHEMA_MINOCCURS_INVALIDVALUE;
        goto Error;
    }

    // normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::buildElementMaxOccurs(IXMLNodeSource * pSource, Object * s)
{
    HRESULT hr = S_OK;

    VARIANT v;

    String * str = SAFE_CAST(String *, s);

    if (str->equals(SchemaNames::pszInfinite))
    {
        _ElementInfo.nMaxVal = -1;
    }
    else
    {
        hr = parseInteger(s->toString(), &_ElementInfo.nMaxVal);
        if (hr) 
        {
            hr = SCHEMA_MAXOCCURS_INVALIDVALUE;
            goto Error;
        }
    }

    if (_ElementInfo.nMaxVal != -1 && _ElementInfo.nMaxVal != 1)
    {
        hr = SCHEMA_MAXOCCURS_INVALIDVALUE;
        goto Error;
    }

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::buildAttributeType(IXMLNodeSource * pSource, Object * nd)
{
    HRESULT hr = S_OK;

    NameDef * namedef = SAFE_CAST(NameDef *, nd);
    Name * name = namedef->getName();    

    String * tmpstr;

    _AttributeInfo.fGotType = true;
    _AttributeInfo.pAttNamedef = namedef;
    
    // Local
    _AttributeInfo.pAttDef = (AttDef * )_ElementInfo.pAttDefList->get(name);

    // If not local, try global
    if (_AttributeInfo.pAttDef == null)
    {
        // if there is no URN in this name then the name is local to the
        // schema, but the global attribute was still URN qualified, so
        // we need to qualify this name now.
        Name* gname = name;
        if (name->getNameSpace() == null)
        {
            namedef = _pPrevNSMgr->createNameDef(namedef, _pURN, _pURN);
            gname = namedef->getName();
        }
        AttDef * ad = _pDTD->getGAttributeType(gname);
        if (ad != null)
        {
            _AttributeInfo.pAttDef = AttDef::copyAttDef(ad);
            _AttributeInfo.pAttDef->_pName = name;
        }
    }

    if (_AttributeInfo.pAttDef != null)
    {
        if (_AttributeInfo.pAttDef->_fDefault)
        {
            // we have to convert name to string when it is not AV_CDATA
            if (_AttributeInfo.pAttDef->getType() > DT_AV_CDATA)
                tmpstr = ((Name *)(Object *)_AttributeInfo.pAttDef->_pDef)->toString();
            else
                tmpstr = (String *)(Object *)_AttributeInfo.pAttDef->_pDef;
            if (!_AttributeInfo.pAttDef->_fHide)
            {
                addDefNode(pSource, tmpstr->getWCHARPtr(), tmpstr->length());
                // BUGBUG: GC danger on tmpstr
            }
        }
        _ElementInfo.pElementDecl->addAttDef(_AttributeInfo.pAttDef);
    }
    // we are assuming that the AttributeType is already defined before hand else we throw error
    else 
    {
        Exception::throwE(SCHEMA_ATYPE_UNDECLARED, SCHEMA_ATYPE_UNDECLARED, null); 
    }

    if (_AttributeInfo.fDefault)
    {
        _AttributeInfo.pAttDef->_pDef = _AttributeInfo.pObjDef;
        _AttributeInfo.pAttDef->_fDefault = true;
    }
}


void 
SchemaBuilder::buildGroupOrder(IXMLNodeSource * pSource, Object * namedef)
{
    setOrder(SAFE_CAST(NameDef *, namedef)->getName());
}

void 
SchemaBuilder::buildGroupMinOccurs(IXMLNodeSource * pSource, Object * s)
{
    HRESULT hr = S_OK;

    VARIANT v;

    String * str = SAFE_CAST(String *, s);

    hr = parseInteger(str, &_GroupInfo.nMinVal);
    if (hr) 
    {
        hr = SCHEMA_MINOCCURS_INVALIDVALUE;
        goto Error;
    }

    if (_GroupInfo.nMinVal != 0 && _GroupInfo.nMinVal != 1)
    {
        hr = SCHEMA_MINOCCURS_INVALIDVALUE;
        goto Error;
    }

    _GroupInfo.fGotMin = true;

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::buildGroupMaxOccurs(IXMLNodeSource * pSource, Object * s)
{
    HRESULT hr = S_OK;

    VARIANT v;

    String * str = SAFE_CAST(String *, s);

    if (str->equals(SchemaNames::pszInfinite))
    {
        _GroupInfo.nMaxVal = -1;
    }
    else
    {
        hr = parseInteger(str, &_GroupInfo.nMaxVal);
        if (hr) 
        {
            hr = SCHEMA_MAXOCCURS_INVALIDVALUE;
            goto Error;
        }
    }

    if (_GroupInfo.nMaxVal != -1 && _GroupInfo.nMaxVal != 1)
    {
        hr = SCHEMA_MAXOCCURS_INVALIDVALUE;
        goto Error;
    }

    _GroupInfo.fGotMax = true;

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


////////////////////////////////////////////////////////////////////////////////////
// begin functions: for End Children Calls
//

void 
SchemaBuilder::endElementDtType(IXMLNodeSource __RPC_FAR *pSource)
{
    // a special end for <ElementType dt:type=""
    
    // after finish creating the node, it is time to finish the <AttributeType name="dt:dt"
    if (_ElementInfo.fGotDtType)
    {
        _AttributeInfo.fProcessingDtType = true;
        String * pstr = ((String *)(Object *)_AttributeInfo.pAttDef->_pDef)->trim();
        DataType iType = LookupDataType(pstr, _fValidating);
        if (iType != DT_USER_DEFINED)
        {
            _ElementInfo.pElementDecl->setDataType(iType);
            beginAttributeType(pSource);
            _AttributeInfo.fProcessingDtType = false;
            endAttributeType(pSource);
    
            // then start create <attribute type="dt:dt"
            initAttribute(null);
    
            buildAttributeType(pSource, _pPrevNSMgr->createNameDef(XMLNames::name(NAME_DTDT)));
            endAttribute(pSource);
        }
        else
        {
            Assert(!_fValidating); // LookupDataType should have thrown an exception/error
            _ElementInfo.pElementDecl->setDataType(DT_USER_DEFINED);
            _ElementInfo.pElementDecl->setDataTypeName(pstr);
        }
    }
    else
    {
        Exception::throwE(SCHEMA_DTTYPE_MISSING, SCHEMA_DTTYPE_MISSING, null);
    }
}


void 
SchemaBuilder::endElementType(IXMLNodeSource __RPC_FAR *pSource)
{
    if (_ElementInfo.fMasterGroupRequired)
    {
        if (!_ElementInfo.fExistTerminal)
        {
            if (_ElementInfo.pElementDecl->_pContent->isOpen())
            {
                _ElementInfo.pElementDecl->_pContent->setType(ContentModel::ANY);
                // must have something here, since we opened up a group...  since when we have no elements
                // and content="eltOnly" model="open"  we can have anything here...
                _ElementInfo.pElementDecl->_pContent->addTerminal(XMLNames::name(NAME_PCDATA));
            }
            else
            {
                // we must have elements when we say content="eltOnly" model="close"
                Exception::throwE(SCHEMA_ELEMENT_MISSING, SCHEMA_ELEMENT_MISSING, null);
            }
        }
        
        // if the content is mixed, there is a group that need to be closed
        _ElementInfo.pElementDecl->_pContent->closeGroup();
        
        if (_GroupInfo.cCurrentOrder == eMany)
            _ElementInfo.pElementDecl->_pContent->star();
    }
    _ElementInfo.pElementDecl->_pContent->finish();
    _ElementInfo.pElementDecl = null;
    
    // Need to kill the AttDefList, each used ones should be assigned by now
    _ElementInfo.pAttDefList = null;        
}


void 
SchemaBuilder::endElement(IXMLNodeSource __RPC_FAR *pSource)
{
    if (_ElementInfo.fGotType)
    {       
        if (_ElementInfo.nMaxVal == -1)
        {
            if (_ElementInfo.nMinVal == 0)
                _ElementInfo.pElementDecl->_pContent->star();           // minOccurs="0" and maxOccurs="infinite"
            else
                _ElementInfo.pElementDecl->_pContent->plus();           // minOccurs="1" and maxOccurs="infinite"
        }
        else if (_ElementInfo.nMinVal == 0) // && _ElementInfo.nMaxVal == 1)
        {
            _ElementInfo.pElementDecl->_pContent->questionMark();
        }
        // a min=1 max=1 will be a default, thus no need to do anything here if it is min=1 max=1   
    }
    else
    {
        Exception::throwE(SCHEMA_ETYPE_MISSING, SCHEMA_ETYPE_MISSING, null);
    }
}


void 
SchemaBuilder::endAttributeType(IXMLNodeSource __RPC_FAR *pSource)
{
    HRESULT hr = S_OK;

    if (_AttributeInfo.pAttDef->getType() == DT_AV_ENUMERATION && !_AttributeInfo.fEnumerationRequired)
    {
        hr = SCHEMA_DTVALUES_MISSING;
        goto Error;
    }
    if (_AttributeInfo.pAttDef->getType() != DT_AV_ENUMERATION && _AttributeInfo.fEnumerationRequired)
    {
        hr = SCHEMA_ENUMERATION_MISSING;
        goto Error;
    }

    //
    // checkAttributeType re-initializes the def member based on the innerText of
    // the default node - strips whitespace parses names, checks enumerations, etc, etc.
    //
    if (_AttributeInfo.pAttDef->_pDef && _AttributeInfo.pAttDef->getType() > DT_AV_CDATA)
    {
        TRY
        {
            Object* pObj = _pThisNSMgr->parseNames(_AttributeInfo.pAttDef->getType(),  _AttributeInfo.pAttDef->_pDef->toString());
            _AttributeInfo.pAttDef->checkValue( pObj);
            _AttributeInfo.pAttDef->_pDef = pObj;
        }
        CATCH
        {
            Exception::throwE(SCHEMA_ATTRIBUTE_DEFAULTVALUE, SCHEMA_ATTRIBUTE_DEFAULTVALUE, _AttributeInfo.pAttDef->_pDef->toString()->trim(), null);
        }
        ENDTRY
    }

    setAttributePresence();
    _AttributeInfo.pAttNamedef = null;
    _AttributeInfo.pAttDef = null;

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::endAttribute(IXMLNodeSource __RPC_FAR *pSource)
{
    if (_AttributeInfo.fGotType)
    {        
        tryToAddDefNode(pSource);
        _AttributeInfo.fBuildDefNode = false;

        if (_AttributeInfo.fRequired || _AttributeInfo.fGotDefault)
        {
            setAttributePresence();
        }

        _AttributeInfo.pAttDef = null;
    }
    else
    {
        Exception::throwE(SCHEMA_ATYPE_MISSING, SCHEMA_ATYPE_MISSING, null);
    }
}


void 
SchemaBuilder::endAttributeDtType(IXMLNodeSource __RPC_FAR *pSource)
{
    HRESULT hr = S_OK;

    if (!_AttributeInfo.fGotDtType)
    {
        hr = SCHEMA_DTTYPE_MISSING;
    }
    else if (_AttributeInfo.pAttDef->getType() == DT_AV_ENUMERATION && !_AttributeInfo.fEnumerationRequired)
    {
        hr = SCHEMA_DTVALUES_MISSING;
    }
    else if (_AttributeInfo.pAttDef->getType() != DT_AV_ENUMERATION && _AttributeInfo.fEnumerationRequired)
    {
        hr = SCHEMA_ENUMERATION_MISSING;
    }

    if (hr)
        Exception::throwE(hr, hr, null);
}


void 
SchemaBuilder::endGroup(IXMLNodeSource __RPC_FAR *pSource)
{
    HRESULT hr = S_OK;

    // if there exist no Terminals, means this happened () then throw an error (not allowed)
    if (!_ElementInfo.fExistTerminal)
    {
        hr = SCHEMA_ELEMENT_MISSING;
        goto Error;
    }
        
    _ElementInfo.pElementDecl->_pContent->closeGroup();

    if (_GroupInfo.nMaxVal == -1)
    {
        if (_GroupInfo.nMinVal == 0)
            _ElementInfo.pElementDecl->_pContent->star();           // minOccurs="0" and maxOccurs="infinite"
        else
            _ElementInfo.pElementDecl->_pContent->plus();           // minOccurs="1" and maxOccurs="infinite"
    }
    else if (eMany == _GroupInfo.cCurrentOrder) 
    {
        if (!_GroupInfo.fGotMax)
        {
            if (_GroupInfo.fGotMin && _GroupInfo.nMinVal == 1)
                _ElementInfo.pElementDecl->_pContent->plus();
            else
                _ElementInfo.pElementDecl->_pContent->star();
        }
        else
        {
            hr = SCHEMA_MAXOCCURS_MUSTBESTAR;
            goto Error;
        }
    }
    else if (_GroupInfo.nMinVal == 0) // && _GroupInfo.nMaxVal == 1)  // minOccurs="0" and maxOccurs="1" 
    {
        _ElementInfo.pElementDecl->_pContent->questionMark();
    }

    popGroupInfo();

    // normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


////////////////////////////////////////////////////////////////////////////////////////
// DTD build functions
//

void 
SchemaBuilder::setOrder(Name * name)
{
    if (name == SchemaNames::name(SCHEMA_SEQ))
    {
        _GroupInfo.cCurrentOrder = eSequence;
    }
    else if (name == SchemaNames::name(SCHEMA_OR))
    {
        _GroupInfo.cCurrentOrder = eChoice;
    }
    else if (name == SchemaNames::name(SCHEMA_MANY))
    {
        _GroupInfo.cCurrentOrder = eMany;
    }
    else 
    {
        Exception::throwE(SCHEMA_ETORDER_UNKNOWN, SCHEMA_ETORDER_UNKNOWN, null);
    }
}


void 
SchemaBuilder::addOrder()
{
    // additional order can be add on by changing the setOrder and addOrder
    switch(_GroupInfo.cCurrentOrder)
    {
    case eSequence:
        _ElementInfo.pElementDecl->_pContent->addSequence();
        break;
    case eChoice:
    case eMany:
        _ElementInfo.pElementDecl->_pContent->addChoice();
        break;
    default:  
        _ElementInfo.pElementDecl->_pContent->addSequence();
        break;
    }
}


void
SchemaBuilder::setAttributePresence()
{
    if (_AttributeInfo.fRequired || AttDef::REQUIRED == _AttributeInfo.pAttDef->_presence)
    {
        // If it is required and it has a default value then it is a FIXED attribute.
        if (_AttributeInfo.pAttDef->_pDef || _AttributeInfo.fGotDefault)
        {
            _AttributeInfo.pAttDef->_presence = AttDef::FIXED;
        }
        else
        {
            _AttributeInfo.pAttDef->_presence = AttDef::REQUIRED;
        }
    }
    else if (_AttributeInfo.pAttDef->_pDef || _AttributeInfo.fGotDefault)
    {
        _AttributeInfo.pAttDef->_presence = AttDef::DEFAULT;
    }
    else
    {
        _AttributeInfo.pAttDef->_presence = AttDef::IMPLIED;
    }
}


void
SchemaBuilder::addDefNode(IXMLNodeSource* pSource,  const WCHAR * pwcText, ULONG ulLen)
{
    HRESULT hr = S_OK;
    Object* pNode;
    bool fClose = false;

    IXMLNodeFactory * pSrcDocFactory = _pDTD->getNodeFactory();
    Assert(pSrcDocFactory);

    // There may be multiple AttributeTypes with default values in one ElementType.
    if (_ElementInfo.pElementDecl->_pNode == null)
    {
        //BUGBUG: this is a HACK to fix bug 45994 
        //a better solution is to build the node and do all the checkings before
        //beginchildren on the elemenType
        NameDef * namedef;

        if (_ElementInfo.fGotName)
            namedef = _ElementInfo.pElementDecl->getNameDef();
        else
            namedef = _pPrevNSMgr->createNameDef(SchemaNames::name(SCHEMA_NAME));

        _createDTDNode(pSrcDocFactory, pSource, NULL, XML_ELEMENT, false, false, 
            NULL, 0, 0, namedef, (PVOID *)&pNode);
        _ElementInfo.pElementDecl->_pNode = pNode;

        fClose = true;
    }

    // This is called once per AttributeType - so we always create a new 
    _AttributeInfo.pAttDef->_pDefNode = null;

    _createDTDNode(pSrcDocFactory, pSource, _ElementInfo.pElementDecl->_pNode, XML_ATTRIBUTE, 
        true, false, NULL, 0, 0, _AttributeInfo.pAttNamedef, (PVOID *)&pNode);

    _AttributeInfo.pAttDef->_pDefNode = pNode;     
    
    // The attribute node is not directly set to the element, so we set specified to false
    ((Node *)pNode)->setSpecified(false);

    _createDTDNode(pSrcDocFactory, pSource, _AttributeInfo.pAttDef->_pDefNode, XML_PCDATA, 
        false, true, pwcText, ulLen, 0, NULL, NULL);

    // maybe we shouldn't call this so early, but.. this might be necessary..
    _endChildren(pSrcDocFactory, pSource, (Object*)(_AttributeInfo.pAttDef->_pDefNode), 
        XML_ATTRIBUTE, FALSE, NULL);

    if (fClose)
    {
         // Now close the ELEMENT we just created so that createNode/endChildren 
        // calls are balanced.  
         _endChildren(pSrcDocFactory, pSource, _ElementInfo.pElementDecl->_pNode, XML_ELEMENT, 
                    TRUE, NULL);
    }

    // checkAttributeType re-initializes the def member based on the innerText of
    // the default node - strips whitespace parses names, checks enumerations, etc, etc.
    _AttributeInfo.pAttDef->_pDef = null;
    _AttributeInfo.pAttDef->checkAttributeType(pSource, 
        (Node*)(Object*)_AttributeInfo.pAttDef->_pDefNode, _pDoc, true);

    // If this is an ID attribute, you are not supposed to have a default value !!
    if (_AttributeInfo.pAttDef->getType() ==  DT_AV_ID)
    {
        hr = XML_ATTLIST_ID_PRESENCE;
        goto Error;
    }

    //normal return
    return;

Error:
    Exception::throwE(hr, hr, null);
}


void
SchemaBuilder::tryToAddDefNode(IXMLNodeSource* pSource)
{
    if (_AttributeInfo.fBuildDefNode && !_AttributeInfo.fProcessingDtType &&  // we got default value
        _ElementInfo.pElementDecl != null)                                    // && if this is not global AttributeType  
    {        
        String * str = (String *)(Object *)_AttributeInfo.pAttDef->_pDef;
        addDefNode(pSource, str->getWCHARPtr(), str->length());
        // BUGBUG: GC danger on str
    } 
}

DataType 
SchemaBuilder::checkDtType(String * name)
{
    DataType type;
    type = LookupDataType(name, false);
    if (type == DT_AV_ID)
    {
        if (! _AttributeInfo.fGlobal)
        {
            if (_ElementInfo.pElementDecl->isIDDeclared())
            {
                Exception::throwE(XML_ATTLIST_DUPLICATED_ID, XML_ATTLIST_DUPLICATED_ID, null);
            }
            _ElementInfo.pElementDecl->setIDDeclared(true);
        }
    }

    return type;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// private helper functions
//

void
SchemaBuilder::pushGroupInfo()
{
    GroupInfo * copy = GroupInfo::copyGroupInfo(&_GroupInfo);
    GroupInfo ** pTempGroup = _pGroupHistory.push();

    if (pTempGroup == null)
    {
        Exception::throwE(E_OUTOFMEMORY, E_OUTOFMEMORY, null);
    }

    *pTempGroup = copy;
}


void 
SchemaBuilder::popGroupInfo()
{
    GroupInfo ** pTempGroup = _pGroupHistory.peek();
    
    if (pTempGroup != null)
    {
        GroupInfo::copyGroupInfo(*pTempGroup, &_GroupInfo);
        delete *pTempGroup;
        *pTempGroup = null;
        _pGroupHistory.pop();
    }
}


HRESULT SchemaBuilder::expandEntity(IXMLNodeSource* pSource, Name* name)
{
    HRESULT hr = S_OK;

    // This needs to be expanded so that we can parse the names
    // in the expanded entity text.
    Entity* en = _pDTD->findEntity(name, false);
    // BUGBUG -- should be calling DTD::checkEntity - but we don't know
    // whether we are in an attribute or not -- we need to remember this
    // so we get the right error checking.  Besides, you can't define
    // entities with a schema yet anyway.
    if (en == null)
    {
        Exception::throwE(XML_ENTITY_UNDEFINED, 
            XML_ENTITY_UNDEFINED, name->toString(), null);
    }
    // If the entity is parsed already we can avoid reparsing it by putting
    // the text back together.  The problem with this is it makes the simple
    //  case slower
    //        String* text = en->_pNode->getInnerText(true, true);

    String* text = en->getText();
    ATCHAR* atext = text->toCharArray();
    
    XMLParser* pParser = _getParser(pSource);
    hr = pParser->ExpandEntity(atext->getData(), text->length());
    pParser->Release();
    return hr;
}


bool
SchemaBuilder::isSkipableElement(Name * name)
{
    Atom * pNS = name->getNameSpace();
    if (pNS && pNS != XMLNames::atomSCHEMA)
        return true;

    // skip description
    if (SchemaNames::name(SCHEMA_DESCRIPTION) == name)
        return true;
   
    return false;
}


bool
SchemaBuilder::isSkipableAttribute(Name * name)
{
    Atom * pNS = name->getNameSpace();
    if (pNS && pNS != XMLNames::atomSCHEMA && pNS != XMLNames::atomDTTYPENS)
        return true;

    return (pNS == XMLNames::atomURNXMLNS);
}


bool
SchemaBuilder::getNextState(Name *name)
{
    int i = 0;
    while (_pState->aNextStates[i] >= 0)
    {
        if (SchemaNames::name(schemaEntries[_pState->aNextStates[i]].eName) == name)
        {
            _pNextState = (SchemaEntry*)&(schemaEntries[_pState->aNextStates[i]]);
            return true;
        }
        i++;
    }

    return false;
}

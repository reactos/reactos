/*
 * @(#)SchemaNames.hxx 1.0 8/3/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _SCHEMANAMES_HXX
#define _SCHEMANAMES_HXX

/***************************/
/***  Start of #include  ***/

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif


/***  End of #include  ***/
/*************************/
typedef enum
{
    //0
    SCHEMA_NAME,
    SCHEMA_TYPE,
    SCHEMA_MAXOCCURS,
    SCHEMA_MINOCCURS,
    SCHEMA_INFINITE,
    // 5
    SCHEMA_MODEL,
    SCHEMA_OPEN,
    SCHEMA_CLOSE,
    SCHEMA_CONTENT,
    SCHEMA_MIXED, 
    // 10
    SCHEMA_EMPTY, 
    SCHEMA_ELTONLY, 
    SCHEMA_TEXTONLY, 
    SCHEMA_ORDER, 
    SCHEMA_GROUPORDER, 
    // 15
    SCHEMA_SEQ, 
    SCHEMA_OR, 
    SCHEMA_MANY, 
    SCHEMA_REQUIRED, 
    SCHEMA_YES, 
    // 20
    SCHEMA_NO, 
    SCHEMA_STRING,
    SCHEMA_ID,
    SCHEMA_IDREF,
    SCHEMA_IDREFS,
    // 25
    SCHEMA_ENTITY,
    SCHEMA_ENTITIES,
    SCHEMA_NMTOKEN,
    SCHEMA_NMTOKENS,
    SCHEMA_ENUMERATION,
    SCHEMA_DEFAULT,
    SCHEMA_REQUIREDVALUE,
    
    // from this point on use SCHEMA namespace
    SCHEMA_USE_SCHEMA_NAMESPACE,
    SCHEMA_SCHEMAROOT = SCHEMA_USE_SCHEMA_NAMESPACE,
    SCHEMA_ELEMENTTYPE,
    SCHEMA_ELEMENT,
    SCHEMA_GROUP, 
    SCHEMA_ATTRIBUTETYPE,
    SCHEMA_ATTRIBUTE, 
    SCHEMA_DATATYPE, 
    SCHEMA_DESCRIPTION,

    // from this point on use Alias SCHEMA namespace
    SCHEMA_USE_SCHEMAALIAS_NAMESPACE,
    SCHEMA_SCHEMAROOT_ALIAS = SCHEMA_USE_SCHEMAALIAS_NAMESPACE,

    // from this point on use DATATYPE namespace
    SCHEMA_USE_DATATYPE_NAMESPACE,
    SCHEMA_DTTYPE = SCHEMA_USE_DATATYPE_NAMESPACE,
    SCHEMA_DTVALUES, 

    // Max value for schema names
    SCHEMA_MAX_VALUE

} SchemaEnum;


class SchemaNames
{
public:
    static void classInit();

    /** public static **/

    // (Common)
    // the NameSpace for datatype information will be in XMLNames because it is used across SchemaNF and NodeDataNF

    static const TCHAR * pszName;                   // <ElementType name="abc"  ...
                                                    // <AttributeType name="abc"  ...
                                                    // <Entity name="abc"  ...
    static const TCHAR * pszType;                   // <element type="" ...
                                                    // <attribute type="" ...
    // (Schema Root)
    static const TCHAR * pszSchemaRoot;             

    // (ElementType & AttributeType)
    static const TCHAR * pszMaxOccurs;              // default value is 1
    static const TCHAR * pszMinOccurs;              // default value is 1
    static const TCHAR * pszInfinite;

    // (ElementType)
    static const TCHAR * pszElementType;            // <ElementType ...
    static const TCHAR * pszElement;
    // attribute
    static const TCHAR * pszModel;
    static const TCHAR * pszOpen;
    static const TCHAR * pszClosed;

    static const TCHAR * pszContent;                // content model attribute for ElementType
    static const TCHAR * pszMixed;                  // default
    static const TCHAR * pszEmpty;
    static const TCHAR * pszEltOnly;
    static const TCHAR * pszTextOnly;
    // for <ElementType order= ..  and <group order=...
    static const TCHAR * pszOrder;
    // groups in ElementType
    static const TCHAR * pszGroup;
    static const TCHAR * pszGroupOrder;
    // two types of group order
    static const TCHAR * pszSEQ;                    // ORDER ONLY
    static const TCHAR * pszOR;                     // OR (ONE)
    static const TCHAR * pszMANY;                   // MANY

    // (AttributeType)
    static const TCHAR * pszAttributeType;          // <AttributeType ..
    static const TCHAR * pszAttribute;              // after an AttributeType has been defined
                                                    // we use => <attribute name="abc"
    static const TCHAR * pszRequired;               // <AttributeType require="yes" ..default
    static const TCHAR * pszYes;                    // "yes" possible require value
    static const TCHAR * pszNo;                     // "no" possible require value
    // define an attribute
    static const TCHAR * pszDatatype;               // <datatype
    // datatype attributes
    static const TCHAR * pszDTType;                 // <datatype dt:type="" ...
    static const TCHAR * pszDTValues;                // <datatype dt:value="" ...
    // Possible datatypes
    static const TCHAR * pszString;                 // CDATA, PCDATA
    static const TCHAR * pszID;
    static const TCHAR * pszIDREF;
    static const TCHAR * pszIDREFS;
    static const TCHAR * pszENTITY;
    static const TCHAR * pszENTITIES;
    static const TCHAR * pszNMTOKEN;
    static const TCHAR * pszNMTOKENS;
    static const TCHAR * pszENUMERATION;            // (edt|est|pdt)
    // set inside a datatype decl
    static const TCHAR * pszDefault;                // <default                     *sets the default value for the datatype
    static const TCHAR * pszRequiredValue;          // <requiredValue               *used for enumeration only

    static const TCHAR * pszDescription;            // <description...

    static _staticreference<AName> names;
    
    static const TCHAR * cstrings[];

    static Name * name(int i)
    {
        return (*names)[i];
    }


};

#endif
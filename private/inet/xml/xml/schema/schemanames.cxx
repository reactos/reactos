/*
 * @(#)SchemaNames.cxx 1.0 8/3/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#ifndef _SCHEMANAMES_HXX
#include "schemanames.hxx"
#endif

#ifndef _XMLNAMES_HXX
#include "xml/dtd/xmlnames.hxx"
#endif

// (Common)
const TCHAR * SchemaNames::pszName = _T("name");
const TCHAR * SchemaNames::pszType = _T("type");

// (Schema Root)
const TCHAR * SchemaNames::pszSchemaRoot = _T("Schema");

// (ElementType & AttributeType)
const TCHAR * SchemaNames::pszMaxOccurs = _T("maxOccurs");
const TCHAR * SchemaNames::pszMinOccurs = _T("minOccurs");   
const TCHAR * SchemaNames::pszInfinite = _T("*");

// (ElementType)
const TCHAR * SchemaNames::pszElementType = _T("ElementType");
const TCHAR * SchemaNames::pszElement = _T("element");
const TCHAR * SchemaNames::pszModel = _T("model");
const TCHAR * SchemaNames::pszOpen = _T("open");
const TCHAR * SchemaNames::pszClosed = _T("closed");
const TCHAR * SchemaNames::pszContent = _T("content");
const TCHAR * SchemaNames::pszMixed = _T("mixed");
const TCHAR * SchemaNames::pszEmpty = _T("empty");
const TCHAR * SchemaNames::pszEltOnly = _T("eltOnly");
const TCHAR * SchemaNames::pszTextOnly = _T("textOnly");
const TCHAR * SchemaNames::pszOrder = _T("order");
const TCHAR * SchemaNames::pszGroup = _T("group");
const TCHAR * SchemaNames::pszGroupOrder = _T("groupOrder");
const TCHAR * SchemaNames::pszSEQ = _T("seq");
const TCHAR * SchemaNames::pszOR = _T("one");
const TCHAR * SchemaNames::pszMANY = _T("many");

// (AttributeType)
const TCHAR * SchemaNames::pszAttributeType = _T("AttributeType");
const TCHAR * SchemaNames::pszAttribute = _T("attribute");
const TCHAR * SchemaNames::pszRequired = _T("required");
const TCHAR * SchemaNames::pszYes = _T("yes");
const TCHAR * SchemaNames::pszNo = _T("no");
const TCHAR * SchemaNames::pszDatatype = _T("datatype");  


const TCHAR * SchemaNames::pszString = _T("string");
const TCHAR * SchemaNames::pszID = _T("id");
const TCHAR * SchemaNames::pszIDREF = _T("idref");
const TCHAR * SchemaNames::pszIDREFS = _T("idrefs");
const TCHAR * SchemaNames::pszENTITY = _T("entity");
const TCHAR * SchemaNames::pszENTITIES = _T("entities");
const TCHAR * SchemaNames::pszNMTOKEN = _T("nmtoken");
const TCHAR * SchemaNames::pszNMTOKENS = _T("nmtokens");
const TCHAR * SchemaNames::pszENUMERATION = _T("enumeration");
const TCHAR * SchemaNames::pszDefault = _T("default");
const TCHAR * SchemaNames::pszRequiredValue = _T("requiredValue");

const TCHAR * SchemaNames::pszDTType = _T("type");
const TCHAR * SchemaNames::pszDTValues = _T("values");

const TCHAR * SchemaNames::pszDescription = _T("description");

_staticreference<AName> SchemaNames::names;

const TCHAR * SchemaNames::cstrings[] = 
{
    pszName,
    pszType,

    pszMaxOccurs,
    pszMinOccurs,
    pszInfinite,

    pszModel,
    pszOpen,
    pszClosed,

    pszContent,
    pszMixed,

    pszEmpty,
    pszEltOnly,
    pszTextOnly,

    pszOrder,
    pszGroupOrder,
    pszSEQ, 
    pszOR,

    pszMANY,
    pszRequired,

    pszYes,    
    pszNo,     
    
    pszString,         
    pszID,
    pszIDREF,
    pszIDREFS,
    pszENTITY,
    pszENTITIES,
    pszNMTOKEN,
    pszNMTOKENS,
    pszENUMERATION,     
    pszDefault,        
    pszRequiredValue,

    // from this point on use SCHEMA namespace
    pszSchemaRoot,
    pszElementType,
    pszElement,
    pszGroup,
    pszAttributeType,
    pszAttribute,
    pszDatatype,   
    pszDescription,

    // from this point on use Alias SCHEMA namespace
    pszSchemaRoot,

    // from this point on use DataType namespace
    pszDTType,         
    pszDTValues,        
    
};

extern CSMutex * g_pMutex;

void SchemaNames::classInit()
{
    if (!SchemaNames::names)
    {
        MutexLock lock(g_pMutex);
#ifdef RENTAL_MODEL
        Model model(MultiThread);
#endif

        TRY
        {
            
            if (!SchemaNames::names)
            {
                
                Assert(SCHEMA_MAX_VALUE == sizeof(SchemaNames::cstrings) / sizeof(SchemaNames::cstrings[0]));
                
                AName * pNames = new (SCHEMA_MAX_VALUE) _array<RName>;
                
                int i = 0;
                
                for (; i < SCHEMA_USE_SCHEMA_NAMESPACE; i++)
                {
                    (*pNames)[i] = Name::create((TCHAR*)SchemaNames::cstrings[i]); 
                }
                for (; i < SCHEMA_USE_SCHEMAALIAS_NAMESPACE; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)SchemaNames::cstrings[i]), XMLNames::atomSCHEMA);
                }
                for (; i < SCHEMA_USE_DATATYPE_NAMESPACE; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)SchemaNames::cstrings[i]), XMLNames::atomSCHEMAAlias);
                }
                for (; i < SCHEMA_MAX_VALUE; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)SchemaNames::cstrings[i]), XMLNames::atomDTTYPENS);
                }
                
                SchemaNames::names = pNames;
            }
        }
        CATCH
        {
            lock.Release();
#ifdef RENTAL_MODEL
            model.Release();
#endif
            Exception::throwAgain();
        }
        ENDTRY
    }
}


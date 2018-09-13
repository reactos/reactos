/*
 * @(#)XMLNames.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XMLNAMES_HXX
#define _XMLNAMES_HXX

enum
{
    NAME_CDATA,
    NAME_PCDATA,
    NAME_VERSION,
    NAME_encoding,
    NAME_DOCTYPE,

    NAME_XML,
    NAME_Standalone,
    NAME_XMLSpace,

    NAME_NAMESPACEDECL,
    NAME_PUBLIC,
    NAME_SYSTEM,
    NAME_NDATA,

    NAME_DTTYPENS,
    // from this point on use XML namespace
    NAME_USE_XML_NAMESPACE,

    NAME_ID = NAME_USE_XML_NAMESPACE, 
    NAME_DEFAULT,
    NAME_XMLSpace2,
    NAME_XMLNameSpace,

    NAME_ENTITY,
    NAME_NOTATION,
    NAME_EMPTY,
    NAME_ANY,

    NAME_COLUMN,    // XML DSO names.
    NAME_ROWSET,
    NAME_NAME,
    NAME_CHILDNAME,
    NAME_ATTR,
    NAME_TEXT,

    // from this point on use XMLNS namespace
    NAME_USE_XML_XMLNSNS,
    NAME_XMLNS = NAME_USE_XML_XMLNSNS,

    // from this point on use DT namespace
    NAME_USE_XML_DTTYPENS,
    NAME_DTDT = NAME_USE_XML_DTTYPENS,

    // from this point on use Alias DT namespace
    NAME_USE_XML_DTTYPENSAlias,
    NAME_DTDT_ALIAS = NAME_USE_XML_DTTYPENSAlias,

    // from this point on use old DT namespace
    NAME_USE_XML_DTTYPENSOld,
    NAME_DTDT_OLD = NAME_USE_XML_DTTYPENSOld,

    // maximum value
    NAME_MAX_VALUE

};

/**
 * This file defines a set of common XML names for sharing
 * across multiple files.
 */
class XMLNames
{

public: 
    
    static void classInit();

    //////////////////////////////////////////////////////////
    // Create names for the following TCHAR strings
    //////////////////////////////////////////////////////////
    static const TCHAR* pszCDATA;
    static const TCHAR* pszPCDATA;
    static const TCHAR* pszVersion;
    static const TCHAR* pszEncoding;
    static const TCHAR* pszDOCTYPE;

    static const TCHAR* pszXML;
    static const TCHAR* pszStandalone;
    static const TCHAR* pszXMLNS;
    static const TCHAR* pszXMLNSCOLON;
    static const TCHAR* pszXMLPREFIX;
    static const TCHAR* pszXMLSRC;

    static const TCHAR* pszXMLXMLNS;
    static const TCHAR* pszURNXML;
    static const TCHAR* pszURNXMLNS;

    static const TCHAR* pszXMLNamespace; 
    static const TCHAR* pszPUBLIC; 
    static const TCHAR* pszSYSTEM;
    static const TCHAR* pszNDATA;

    static const TCHAR* pszDTTYPENS;
    static const TCHAR* pszDTTYPENS2;
    static const TCHAR* pszDTTYPENSAlias;
    // from this point use xml namespace
    static const TCHAR* pszID;      
    static const TCHAR* pszDEFAULT;
    static const TCHAR* pszXMLSpace;
    static const TCHAR* pszXMLSpace2;
    static const TCHAR* pszNamespace;
    
    static const TCHAR* pszENTITY;
    static const TCHAR* pszNOTATION; 
    static const TCHAR* pszEMPTY;
    static const TCHAR* pszANY;

    static const TCHAR* pszCOLUMN;    // XML DSO names.
    static const TCHAR* pszROWSET;
    static const TCHAR* pszNAME;
    static const TCHAR* pszCHILDNAME;
    static const TCHAR* pszATTR;
    static const TCHAR* pszTEXT;

    // from this point use Datatype namespace
    static const TCHAR* pszDTDT;
    //////////////////////////////////////////////////////////
    // Other static TCHAR strings
    //////////////////////////////////////////////////////////
 
    static const TCHAR* pszIDREF;  
    static const TCHAR* pszIDREFS;  
    static const TCHAR* pszENTITIES;
    static const TCHAR* pszNMTOKEN;
    static const TCHAR* pszNMTOKENS;
    static const TCHAR* pszENUMERATION;

    static const TCHAR* pszFIXED;
    static const TCHAR* pszREQUIRED;
    static const TCHAR* pszIMPLIED;
    static const TCHAR* pszPreserve;
    static const TCHAR* pszDefault;

    static const TCHAR* pszUnknown;
    static const TCHAR* pszEquals;
    static const TCHAR* pszLessThan; 
    static const TCHAR* pszGreaterThan; 
    static const TCHAR* pszBANG;
    static const TCHAR* pszPERCENT;
    static const TCHAR* pszAMP;
    static const TCHAR* pszLEFTSQB;
    static const TCHAR* pszRIGHTSQB; 
    static const TCHAR* pszQUOTE; 
    static const TCHAR* pszSEMICOLON;
    static const TCHAR* pszCOLON;

    static const TCHAR* pszPITAGSTART;
    static const TCHAR* pszPITAGEND;
    static const TCHAR* pszDECLTAGSTART;
    static const TCHAR* pszCLOSETAGSTART;
    static const TCHAR* pszEMPTYTAGEND;
    static const TCHAR* pszIncludeStart;
    static const TCHAR* pszIgnoreStart;
    static const TCHAR* pszCDEND;

    static const TCHAR* pszCOMMENT;
    static const TCHAR* pszENDCOMMENT;

    static const TCHAR* pszSpace;


    static const TCHAR* pszCDATA2;
    static const TCHAR* pszELEMENTS;
    static const TCHAR* pszContentType;
    static const TCHAR* pszDefaultVersion;

    static const TCHAR* pszSchemaURLPrefix;
    static const TCHAR* pszSCHEMA;
    static const TCHAR* pszSCHEMAAlias;

    static SRAtom    atomEmpty;
    static SRAtom    atomXML;
    static SRAtom    atomXMLNS;
    static SRAtom    atomURNXML;
    static SRAtom    atomURNXMLNS;
    static SRAtom    atomDTTYPENS;
    static SRAtom    atomDTTYPENSAlias;
    static SRAtom    atomDTTYPENSOld;
    static SRAtom    atomSCHEMA;
    static SRAtom    atomSCHEMAAlias;
    static _staticreference<AName> names;

    static const TCHAR * cstrings[];

    static Name * name(int i)
    {
        return (*names)[i];
    }

};

#endif

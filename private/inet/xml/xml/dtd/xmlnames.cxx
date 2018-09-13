/*
 * @(#)XMLNames.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "xmlnames.hxx"

const TCHAR* XMLNames::pszNamespace = _T("namespace");
const TCHAR* XMLNames::pszXMLNamespace = _T("xml:namespace");
const TCHAR* XMLNames::pszXML = _T("xml");
const TCHAR* XMLNames::pszVersion = _T("version");
const TCHAR* XMLNames::pszEncoding = _T("encoding");
const TCHAR* XMLNames::pszStandalone = _T("standalone");

const TCHAR* XMLNames::pszXMLSpace = _T("XML-SPACE");

const TCHAR* XMLNames::pszXMLNS = _T("xmlns");
const TCHAR* XMLNames::pszXMLNSCOLON = _T("xmlns:");
const TCHAR* XMLNames::pszURNXML = _T("");
const TCHAR* XMLNames::pszURNXMLNS = _T("http://www.w3.org/XML/1998/namespace");

const TCHAR* XMLNames::pszSYSTEM = _T("SYSTEM");
const TCHAR* XMLNames::pszPUBLIC = _T("PUBLIC");
const TCHAR* XMLNames::pszNDATA = _T("NDATA");

const TCHAR* XMLNames::pszENTITY = _T("ENTITY");
const TCHAR* XMLNames::pszCDATA = _T("CDATA");
const TCHAR* XMLNames::pszCDATA2 = _T("[CDATA[");
const TCHAR* XMLNames::pszDOCTYPE = _T("DOCTYPE");
const TCHAR* XMLNames::pszPCDATA = _T("#PCDATA");
const TCHAR* XMLNames::pszEMPTY = _T("EMPTY");
const TCHAR* XMLNames::pszANY = _T("ANY");

const TCHAR* XMLNames::pszCOLUMN = _T("COLUMN");    // XML DSO names.
const TCHAR* XMLNames::pszROWSET = _T("ROWSET");
const TCHAR* XMLNames::pszNAME = _T("NAME");
const TCHAR* XMLNames::pszCHILDNAME = _T("CHILDNAME");
const TCHAR* XMLNames::pszATTR = _T("ATTR");
const TCHAR* XMLNames::pszTEXT = _T("TEXT");

const TCHAR* XMLNames::pszID = _T("ID");      
const TCHAR* XMLNames::pszIDREF = _T("IDREF");  
const TCHAR* XMLNames::pszIDREFS = _T("IDREFS");  
const TCHAR* XMLNames::pszENTITIES = _T("ENTITIES");
const TCHAR* XMLNames::pszNMTOKEN = _T("NMTOKEN"); 
const TCHAR* XMLNames::pszNMTOKENS = _T("NMTOKENS");
const TCHAR* XMLNames::pszNOTATION = _T("NOTATION");
const TCHAR* XMLNames::pszENUMERATION = _T("ENUMERATION");

const TCHAR* XMLNames::pszFIXED = _T("FIXED");
const TCHAR* XMLNames::pszREQUIRED = _T("REQUIRED");
const TCHAR* XMLNames::pszIMPLIED = _T("IMPLIED");
const TCHAR* XMLNames::pszDEFAULT = _T("DEFAULT");

const TCHAR* XMLNames::pszEquals = _T("=");
const TCHAR* XMLNames::pszUnknown = _T("???");
const TCHAR* XMLNames::pszLessThan = _T("<");
const TCHAR* XMLNames::pszGreaterThan = _T(">");
const TCHAR* XMLNames::pszBANG = _T("!");
const TCHAR* XMLNames::pszPERCENT = _T("%");
const TCHAR* XMLNames::pszAMP = _T("&");
const TCHAR* XMLNames::pszLEFTSQB = _T("[");
const TCHAR* XMLNames::pszRIGHTSQB = _T("]");
const TCHAR* XMLNames::pszQUOTE = _T("\""); 
const TCHAR* XMLNames::pszSEMICOLON = _T(";");

const TCHAR* XMLNames::pszPITAGSTART = _T("<?");
const TCHAR* XMLNames::pszPITAGEND = _T("?>");
const TCHAR* XMLNames::pszDECLTAGSTART = _T("<!");
const TCHAR* XMLNames::pszCLOSETAGSTART = _T("</");
const TCHAR* XMLNames::pszEMPTYTAGEND = _T("/>");
const TCHAR* XMLNames::pszSpace = _T(" ");
const TCHAR* XMLNames::pszCOMMENT = _T("<!--");
const TCHAR* XMLNames::pszENDCOMMENT = _T("-->");
const TCHAR* XMLNames::pszXMLSpace2 = _T("space");

const TCHAR* XMLNames::pszIncludeStart = _T("[INCLUDE[");
const TCHAR* XMLNames::pszIgnoreStart = _T("[IGNORE[");
const TCHAR* XMLNames::pszCDEND = _T("]]>");

const TCHAR* XMLNames::pszPreserve = _T("preserve");
const TCHAR* XMLNames::pszDefault = _T("default");
const TCHAR* XMLNames::pszELEMENTS = _T("ELEMENTS");
const TCHAR* XMLNames::pszContentType = _T("Content: type=");
const TCHAR* XMLNames::pszDefaultVersion = _T("1.0");
const TCHAR* XMLNames::pszSchemaURLPrefix = _T("x-schema:");

const TCHAR* XMLNames::pszDTTYPENS = _T("urn:schemas-microsoft-com:datatypes");
const TCHAR* XMLNames::pszDTTYPENSAlias = _T("uuid:C2F41010-65B3-11D1-A29F-00AA00C14882");
const TCHAR* XMLNames::pszDTTYPENS2 = _T("urn:uuid:C2F41010-65B3-11D1-A29F-00AA00C14882/");
const TCHAR* XMLNames::pszDTDT = _T("dt");

// (Schema URN)
const TCHAR * XMLNames::pszSCHEMA = _T("urn:schemas-microsoft-com:xml-data");
const TCHAR * XMLNames::pszSCHEMAAlias = _T("uuid:BDC6E3F0-6DA3-11D1-A2A3-00AA00C14882");


/**
 * This file defines a set of common XML names for sharing
 * across multiple files.
 */

SRAtom XMLNames::atomEmpty;
SRAtom XMLNames::atomXML;
SRAtom XMLNames::atomXMLNS;
SRAtom XMLNames::atomURNXML;
SRAtom XMLNames::atomURNXMLNS;
SRAtom XMLNames::atomDTTYPENS;
SRAtom XMLNames::atomDTTYPENSOld;
SRAtom XMLNames::atomDTTYPENSAlias;
SRAtom XMLNames::atomSCHEMA;
SRAtom XMLNames::atomSCHEMAAlias;

_staticreference<AName> XMLNames::names;

const TCHAR * XMLNames::cstrings[] = 
{
    pszCDATA,
    pszPCDATA,
    pszVersion,
    pszEncoding,
    pszDOCTYPE,

    pszXML,
    pszStandalone,
    pszXMLSpace,

    pszXMLNamespace,
    pszPUBLIC,
    pszSYSTEM,
    pszNDATA,

    pszDTTYPENS,

    pszID,
    pszDEFAULT,
    pszXMLSpace2, 
    pszNamespace,

    pszENTITY,
    pszNOTATION,
    pszEMPTY,
    pszANY,

    pszCOLUMN,    // XML DSO names.
    pszROWSET,
    pszNAME,
    pszCHILDNAME,
    pszATTR,
    pszTEXT,
    
    _T(""), // xmlns:

    pszDTDT, // with pretty form
    pszDTDT, // with uuid:<guid>
    pszDTDT, // with urn:uuid:<guid>/
};

extern CSMutex * g_pMutex;

void 
XMLNames::classInit()
{
    if (!XMLNames::names)
    {
        MutexLock lock(g_pMutex);
#ifdef RENTAL_MODEL
        Model model(MultiThread);
#endif

        TRY
        {
            // check if it is still null in case an other thread entered first...
            if (!XMLNames::names)
            {
                XMLNames::atomEmpty = Atom::create(_T(""), 0);
                XMLNames::atomXML = Atom::create((TCHAR*)XMLNames::pszXML);
                XMLNames::atomXMLNS = Atom::create((TCHAR*)XMLNames::pszXMLNS);
                XMLNames::atomURNXML = Atom::create((TCHAR*)XMLNames::pszURNXML);
                XMLNames::atomURNXMLNS = Atom::create((TCHAR*)XMLNames::pszURNXMLNS);
                XMLNames::atomDTTYPENS = Atom::create((TCHAR*)XMLNames::pszDTTYPENS);
                XMLNames::atomDTTYPENSAlias = Atom::create((TCHAR*)XMLNames::pszDTTYPENSAlias);
                XMLNames::atomDTTYPENSOld = Atom::create((TCHAR*)XMLNames::pszDTTYPENS2);
                XMLNames::atomSCHEMA = Atom::create((TCHAR*)XMLNames::pszSCHEMA);
                XMLNames::atomSCHEMAAlias = Atom::create((TCHAR*)XMLNames::pszSCHEMAAlias);

                Assert(NAME_MAX_VALUE == sizeof(XMLNames::cstrings) / sizeof(XMLNames::cstrings[0]));

                AName * pNames = new (NAME_MAX_VALUE) _array<RName>;

                int i = 0;

                for (; i < NAME_USE_XML_NAMESPACE; i++)
                {
                    (*pNames)[i] = Name::create((TCHAR*)XMLNames::cstrings[i]); 
                }
                for (; i < NAME_USE_XML_XMLNSNS; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)XMLNames::cstrings[i]), XMLNames::atomURNXML);
                }
                for (; i < NAME_USE_XML_DTTYPENS; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)XMLNames::cstrings[i]), XMLNames::atomURNXMLNS);
                }
                for (; i < NAME_USE_XML_DTTYPENSAlias; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)XMLNames::cstrings[i]), XMLNames::atomDTTYPENS);
                }
                for (; i < NAME_USE_XML_DTTYPENSOld; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)XMLNames::cstrings[i]), XMLNames::atomDTTYPENSAlias);
                }
                for (; i < NAME_MAX_VALUE; i++)
                {
                    (*pNames)[i] = Name::create(String::newString((TCHAR*)XMLNames::cstrings[i]), XMLNames::atomDTTYPENSOld);
                }

                XMLNames::names = pNames;
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

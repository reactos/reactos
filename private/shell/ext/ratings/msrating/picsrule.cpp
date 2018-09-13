//*******************************************************************
//*
//* PICSRule.cpp
//*
//* Revision History:
//*     Created 7/98 - Mark Hammond (t-markh)
//*
//* Implements PICSRules parsing, decision making,
//* exporting, and editing.
//*
//*******************************************************************

//*******************************************************************
//*
//* A brief rundown on PICSRules files:
//*    (The official spec is at: http://www.w3.org/TR/REC-PICSRules)
//*
//* PICSRules files have MIME type: application/pics-rules and
//* consist of a sequence of parenthesis encapsulated attribute
//* value pairs.  Values consist of quoted strings or parenthesized
//* lists of further attribute value pairs.  Every subsection of a
//* PICSRules file has a primary attribute (denoted in the outline
//* below by square brackets); if any value is not paired with an
//* attribute, it is assigned to the primary attribute.
//*
//* Whitespace consists of the characters ' ', '\t', '\r', and '\n'
//* and is ignored except between attribute value pairs.
//*
//* Quoted strings can be encapsulated in either single ticks ('')
//* or double ticks (""), but may not use mixed notation ('" or "').
//*
//* The following escape sequences are observed in the quoted strings:
//*     " = %22
//*     ' = %27
//*     % = %25
//* Any other escape sequence is invalid.
//*
//* Both attributes and values are case sensitive.
//*
//* Curly brackets denote comments.
//*
//* --- Code Requirements ---
//*
//* The rule evaluator needs to return yes (accept) or no (reject)
//* AS WELL AS the policy clause that determined the answer.
//*
//* The rule evaluator stops when it hits the first policy clause
//* which is applicable.
//*
//* If no clause is satisfied, the default clause is:
//* AcceptIf "otherwise".  In this implementation, if no clause is
//* satisfied, evaluation is passed to our non-PICSRule evaluator.
//*
//* PICSRules 1.1 does NOT support Internationalization of the name
//* section (i.e. each language needs its own PICSRules file).
//* The AddItem function of the PICSRulesName class can easily be
//* extended if this behavior changes in a future revision of the
//* PICSRules spec.
//*
//* The Ratfile attribute is either an entire machine readable .RAT
//* file, or the URL where the .RAT file can be obtained.
//*
//* --- Attribute Value specifications ---
//*
//*
//* The main body of a PICSRules has the form:
//*
//* (PicsRule-%verMajor%.%verMinor%
//*     (
//*         Attribute Value
//*         ...
//*         Tag (
//*                 Attribute Value
//*                 ...
//*         )
//*     )
//* )
//*
//* The current %verMajor% is 1
//* The current %verMinor% is 1
//*
//* Possible Tags and their Attribute Value pairs are:
//*
//* Policy (
//*     [Explanation]       quoted
//*     RejectByURL         URL | ( [patterns]  URL )
//*     AcceptByURL         URL | ( [patterns]  URL )
//*     RejectIf            PolicyExpression
//*     RejectUnless        PolicyExpression
//*     AcceptIf            PolicyExpression
//*     AcceptUnless        PolicyExpression
//*     *Extension* )
//*
//* name (
//*     [Rulename]          quoted
//*     Description         quoted
//*     *Extension* )
//*
//* source (
//*     [SourceURL]         URL
//*     CreationTool        quoted (has format application/version)
//*     author              email
//*     LastModified        ISO Date
//*     *Extension* )
//*
//* serviceinfo (
//*     [Name]              URL
//*     shortname           quoted
//*     BureauURL           URL
//*     UseEmbedded         Y|N
//*     Ratfile             quoted
//*     BureauUnavailable   PASS|FAIL
//*     *Extension* )
//*
//* optextension (
//*     [extension-name]    URL
//*         shortname       quoted
//*         *Extension* )
//*
//* reqextension (
//*     [extension-name]    URL
//*         shortname       quoted
//*         *Extension* )
//*
//* *Extension*
//*
//* Further comments are given below
//*
//*******************************************************************


//*******************************************************************
//*
//* #Includes
//*
//*******************************************************************
#include "msrating.h"
#include "mslubase.h"
#include "parselbl.h"       /* we use a couple of this guy's subroutines */
#include "msluglob.h"
#include "buffer.h"
#include "resource.h"
#include <wininet.h>
#include "picsrule.h"

#include <mluisupp.h>
#include <winsock2.h>
#include <shlwapip.h>

//*******************************************************************
//*
//* Globals
//*
//*******************************************************************
array<PICSRulesRatingSystem*>   g_arrpPRRS;         //this is the array of PICSRules systems
                                                    //which are inforced while ie is running
PICSRulesRatingSystem *         g_pApprovedPRRS;    //this is the Approved Sites PICSRules
                                                    //system
PICSRulesRatingSystem *         g_pPRRS=NULL;       //this is a temporary pointer used while
                                                    //parsing a PICSRules file
HMODULE                         g_hURLMON,g_hWININET;

BOOL                            g_fPICSRulesEnforced,g_fApprovedSitesEnforced;

char                            g_szLastURL[INTERNET_MAX_URL_LENGTH];

extern DWORD                    g_dwDataSource;

extern HANDLE g_HandleGlobalCounter,g_ApprovedSitesHandleGlobalCounter;
extern long   g_lGlobalCounterValue,g_lApprovedSitesGlobalCounterValue;

//*******************************************************************
//*
//* Function Prototypes
//*
//*******************************************************************
HRESULT PICSRulesParseSubPolicyExpression(LPSTR& lpszCurrent,PICSRulesPolicyExpression *pPRPolicyExpression,PICSRulesFileParser *pParser);
HRESULT PICSRulesParseSimplePolicyExpression(LPSTR& lpszCurrent,PICSRulesPolicyExpression *pPRPolicyExpression,PICSRulesFileParser *pParser);
BOOL IsServiceDefined(LPSTR lpszService,LPSTR lpszFullService,PICSRulesServiceInfo **ppServiceInfo);
BOOL IsOptExtensionDefined(LPSTR lpszExtension);
BOOL IsReqExtensionDefined(LPSTR lpszExtension);
HRESULT PICSRulesParseSingleByURL(LPSTR lpszByURL, PICSRulesByURLExpression *pPRByURLExpression, PICSRulesFileParser *pParser);

extern HKEY OpenHiveFile(BOOL fCreate);

//*******************************************************************
//*
//* Some definitions specific to this file
//*
//*******************************************************************
PICSRulesAllowableOption aaoPICSRules[] = {
    { PROID_PICSVERSION, 0 },
    
    { PROID_POLICY, AO_MANDATORY },
        { PROID_EXPLANATION, AO_SINGLE },
        { PROID_REJECTBYURL, AO_SINGLE },
        { PROID_ACCEPTBYURL, AO_SINGLE },
        { PROID_REJECTIF, AO_SINGLE },
        { PROID_ACCEPTIF, AO_SINGLE },
        { PROID_ACCEPTUNLESS, AO_SINGLE },
        { PROID_REJECTUNLESS, AO_SINGLE },
    { PROID_NAME, AO_SINGLE },
        { PROID_RULENAME, 0 },
        { PROID_DESCRIPTION, 0 },
    { PROID_SOURCE, AO_SINGLE },
        { PROID_SOURCEURL, 0 },
        { PROID_CREATIONTOOL, 0 },
        { PROID_AUTHOR, 0 },
        { PROID_LASTMODIFIED, 0 },
    { PROID_SERVICEINFO, 0 },
        { PROID_SINAME, AO_SINGLE },
        { PROID_SHORTNAME, AO_SINGLE },
        { PROID_BUREAUURL, 0 },
        { PROID_USEEMBEDDED, AO_SINGLE },
        { PROID_RATFILE, AO_SINGLE },
        { PROID_BUREAUUNAVAILABLE, AO_SINGLE },
    { PROID_OPTEXTENSION, 0 },
        { PROID_EXTENSIONNAME, AO_SINGLE },
      //{ PROID_SHORTNAME, AO_SINGLE },
    { PROID_REQEXTENSION, 0 },
      //{ PROID_EXTENSIONNAME, AO_SINGLE },
      //{ PROID_SHORTNAME, AO_SINGLE },
    { PROID_EXTENSION, 0 },

    { PROID_POLICYDEFAULT, AO_SINGLE },
    { PROID_NAMEDEFAULT, AO_SINGLE },
    { PROID_SOURCEDEFAULT, AO_SINGLE },
    { PROID_SERVICEINFODEFAULT, AO_SINGLE },
    { PROID_OPTEXTENSIONDEFAULT, AO_SINGLE },
    { PROID_REQEXTENSIONDEFAULT, AO_SINGLE },

    { PROID_INVALID, 0 }
};
const UINT caoPICSRules=sizeof(aaoPICSRules)/sizeof(aaoPICSRules[0]);

//The FN_INTERNETCRACKURL type describes the URLMON function InternetCrackUrl
typedef BOOL (*FN_INTERNETCRACKURL)(LPCTSTR lpszUrl,DWORD dwUrlLength,DWORD dwFlags,LPURL_COMPONENTS lpUrlComponents);

//The FN_ISVALIDURL type describes the URLMON function IsValidURL
//and is called by the three IsURLValid methods of PICSRulesQuotedURL
typedef HRESULT (*FN_ISVALIDURL)(LPBC pBC,LPCWSTR szURL,DWORD dwReserved);

//*******************************************************************
//*
//* This function is called by AdvancedDlgProc while processing
//* WM_COMMOND with LOWORD(wParam)==IDC_PICSRULESOPEN in msludlg.cpp
//* The argument lpszFileName is the name of the PICSRules file
//* selected by the user to import.
//*
//* This begins the PICSRules Import process.
//*
//*******************************************************************
HRESULT PICSRulesImport(char *lpszFileName, PICSRulesRatingSystem **pprrsOut)
{
    PICSRulesRatingSystem *pPRRS=new PICSRulesRatingSystem;

    *pprrsOut=pPRRS;
    
    if(pPRRS==NULL)
    {
        return E_OUTOFMEMORY;
    }

    UINT cbFilename=strlenf(lpszFileName)+1+1;      //room for marker character
    LPSTR lpszNameCopy=new char[cbFilename];
    
    if(lpszNameCopy==NULL)
    {
        return E_OUTOFMEMORY;
    }

    strcpyf(lpszNameCopy,lpszFileName);
    pPRRS->m_etstrFile.SetTo(lpszNameCopy);

    HRESULT hRes;

    HANDLE hFile=CreateFile(lpszNameCopy,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);

    if(hFile!=INVALID_HANDLE_VALUE)
    {
        DWORD cbFile=::GetFileSize(hFile,NULL);

        BUFFER bufData(cbFile+1);

        if(bufData.QueryPtr()!=NULL)
        {
            LPSTR lpszData=(LPSTR)bufData.QueryPtr();
            DWORD cbRead;

            if(ReadFile(hFile,lpszData,cbFile,&cbRead,NULL))
            {
                lpszData[cbRead]='\0';              //null terminate whole file

                g_pPRRS=pPRRS;                      //make data available to
                                                    //parsing functions during
                                                    //parsing

                hRes=pPRRS->Parse(lpszFileName,lpszData);

                if(SUCCEEDED(hRes))
                {
                    pPRRS->m_dwFlags|=PRRS_ISVALID;
                }
                else
                {
                    g_pPRRS=NULL;
                }
            }
            else
            {
                hRes=HRESULT_FROM_WIN32(::GetLastError());
            }
            
            CloseHandle(hFile);
        }
        else
        {
            hRes=E_OUTOFMEMORY;
        }
    }
    else
    {
        hRes=HRESULT_FROM_WIN32(::GetLastError());
    }

    if(!(pPRRS->m_dwFlags&PRS_ISVALID))
    {
        //file is invalid
    }

    return hRes;
}

/* White returns a pointer to the first whitespace character starting at pc.*/
extern char* White(char *);

/* NonWhite returns a pointer to the first non-whitespace character starting at pc.*/
extern char* NonWhite(char *);

//*******************************************************************
//*
//* The following are handler functions which parse the various
//* kinds of content which can occur within a parenthesized object.
//*
//* ppszIn is always advanced to the next non-white space token
//* ppszOut returns the processed data
//*
//*******************************************************************

//The following escape sequences are observed:
//  "   =%22
//  '   =%27
//  %   =%25
//any other escape sequence is invalid
HRESULT PICSRulesParseString(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    BOOL fQuote;
    LPSTR lpszEscapeSequence,lpszNewEnd;

    *ppOut=NULL;
    
    LPSTR pszCurrent=*ppszIn;

    if(*pszCurrent=='\"')
    {
        fQuote=PR_QUOTE_DOUBLE;
    }
    else if(*pszCurrent=='\'')
    {
        fQuote=PR_QUOTE_SINGLE;
    }
    else
    {
        return(PICSRULES_E_EXPECTEDSTRING);
    }

    pszCurrent++;

    LPSTR pszEnd=pParser->EatQuotedString(pszCurrent,fQuote);
    
    if(pszEnd==NULL)
    {
        return(PICSRULES_E_EXPECTEDSTRING);
    }

    lpszNewEnd=pszEnd;

    do
    {
        lpszEscapeSequence=strstrf(pszCurrent,"%22");

        if(lpszEscapeSequence>lpszNewEnd)
        {
            lpszEscapeSequence=NULL;
        }

        if(lpszEscapeSequence!=NULL)
        {
            *lpszEscapeSequence='\"';

            memcpyf(lpszEscapeSequence+1,lpszEscapeSequence+3,(int)(lpszNewEnd-lpszEscapeSequence-3));

            lpszNewEnd-=2;
        }

    } while(lpszEscapeSequence!=NULL);

    do
    {
        lpszEscapeSequence=strstrf(pszCurrent,"%27");

        if(lpszEscapeSequence>lpszNewEnd)
        {
            lpszEscapeSequence=NULL;
        }

        if(lpszEscapeSequence!=NULL)
        {
            *lpszEscapeSequence='\'';

            memcpyf(lpszEscapeSequence+1,lpszEscapeSequence+3,(int)(lpszNewEnd-lpszEscapeSequence-3));

            lpszNewEnd-=2;
        }

    } while(lpszEscapeSequence!=NULL);

    do
    {
        lpszEscapeSequence=strstrf(pszCurrent,"%25");

        if(lpszEscapeSequence>lpszNewEnd)
        {
            lpszEscapeSequence=NULL;
        }

        if(lpszEscapeSequence!=NULL)
        {
            *lpszEscapeSequence='%';

            memcpyf(lpszEscapeSequence+1,lpszEscapeSequence+3,(int)(lpszNewEnd-lpszEscapeSequence-3));

            lpszNewEnd-=2;
        }

    } while(lpszEscapeSequence!=NULL);

    UINT cbString= (unsigned int) (lpszNewEnd-pszCurrent);
    LPSTR pszNew = new char[cbString + 1];  //This memory gets assigned to an ET derived
                                            //type via the AddItem call for the class handling
                                            //the parenthesized object.  The memory is
                                            //deallocated when the handling class, and hence
                                            //the ET derived type, goes out of scope.
    if (pszNew==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    memcpyf(pszNew, pszCurrent, cbString);
    pszNew[cbString]='\0';

    *ppOut=(LPVOID) pszNew;
    *ppszIn=pParser->FindNonWhite(pszEnd+1);

    return(NOERROR);
}

HRESULT PICSRulesParseNumber(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    int n;

    LPSTR pszCurrent=*ppszIn;
    HRESULT hres=::ParseNumber(&pszCurrent,&n,TRUE);

    if(FAILED(hres))
    {
        return(PICSRULES_E_EXPECTEDNUMBER);
    }

    *(int *)ppOut=n;

    LPSTR pszNewline=strchrf(*ppszIn, '\n');

    while((pszNewline!=NULL)&&(pszNewline<pszCurrent))
    {
        pParser->m_nLine++;
        pszNewline=strchrf(pszNewline+1,'\n');
    }
    
    *ppszIn=pszCurrent;

    return(NOERROR);
}


HRESULT PICSRulesParseYesNo(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    BOOL b;

    //The PICSRules spec allows the following:
    //
    //            "y" == Yes
    //          "yes" == Yes
    //            "n" == no
    //           "no" == no
    //
    //  string comparison is not case sensitive
    //

    LPSTR pszCurrent=*ppszIn;

    if((*pszCurrent=='\"')||(*pszCurrent=='\''))
    {
        pszCurrent++;
    }

    HRESULT hres=::GetBool(&pszCurrent,&b,PR_BOOLEAN_YESNO);

    if (FAILED(hres))
    {
        return(PICSRULES_E_EXPECTEDBOOL);
    }

    LPSTR pszNewline=strchrf(*ppszIn,'\n');
    while((pszNewline!=NULL)&&(pszNewline<pszCurrent))
    {
        pParser->m_nLine++;
        pszNewline=strchrf(pszNewline+1,'\n');
    }

    if((*pszCurrent=='\"')||(*pszCurrent=='\''))
    {
        pszCurrent++;
    }

    *ppszIn=pszCurrent;

    *(LPBOOL)ppOut=b;

    return(NOERROR);
}

HRESULT PICSRulesParsePassFail(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    BOOL b;

    //The PICSRules spec allows the following:
    //
    //         "pass" == pass
    //         "fail" == fail
    //
    //  for completeness we add:
    //
    //            "p" == pass
    //            "f" == fail
    //
    //  string comparison is not case sensitive
    //

    LPSTR pszCurrent=*ppszIn;

    if((*pszCurrent=='\"')||(*pszCurrent=='\''))
    {
        pszCurrent++;
    }

    HRESULT hres=::GetBool(&pszCurrent,&b,PR_BOOLEAN_PASSFAIL);

    if (FAILED(hres))
    {
        return(PICSRULES_E_EXPECTEDBOOL);
    }

    LPSTR pszNewline=strchrf(*ppszIn,'\n');
    while((pszNewline!=NULL)&&(pszNewline<pszCurrent))
    {
        pParser->m_nLine++;
        pszNewline=strchrf(pszNewline+1,'\n');
    }

    if((*pszCurrent=='\"')||(*pszCurrent=='\''))
    {
        pszCurrent++;
    }

    *ppszIn=pszCurrent;

    *(LPBOOL)ppOut=b;

    return(NOERROR);
}

HRESULT PICSRulesParseVersion(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    //t-markh - 8/98 - This shouldn't get called, version info should be filled
    //                 out before processing begins

    return(E_UNEXPECTED);
}

PICSRulesAllowableOption aaoPICSRulesPolicy[] = {
    { PROID_EXPLANATION, AO_SINGLE },
    { PROID_REJECTBYURL, AO_SINGLE },
    { PROID_ACCEPTBYURL, AO_SINGLE },
    { PROID_REJECTIF, AO_SINGLE },
    { PROID_ACCEPTIF, AO_SINGLE },
    { PROID_ACCEPTUNLESS, AO_SINGLE },
    { PROID_REJECTUNLESS, AO_SINGLE },
    { PROID_EXTENSION, 0 },
    { PROID_INVALID, 0 }
};
const UINT caoPICSRulesPolicy=sizeof(aaoPICSRulesPolicy)/sizeof(aaoPICSRulesPolicy[0]);

HRESULT PICSRulesParsePolicy(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    //We must make a copy of the allowable options array because the
    //parser will fiddle with the flags in the entries -- specifically,
    //setting AO_SEEN.  It wouldn't be thread-safe to do this to a
    //static array.

    PICSRulesAllowableOption aao[caoPICSRulesPolicy];

    ::memcpyf(aao,::aaoPICSRulesPolicy,sizeof(aao));

    PICSRulesPolicy *pPolicy=new PICSRulesPolicy;
    
    if(pPolicy==NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hres=pParser->ParseParenthesizedObject(
                            ppszIn,                 //var containing current ptr
                            aao,                    //what's legal in this object
                            pPolicy);               //object to add items back to

    if (FAILED(hres))
    {
        delete pPolicy;
        return hres;
    }

    *ppOut=(LPVOID) pPolicy;

    return NOERROR;
}

//PICSRules URLpatterns can be presented to ParseByURL as either a single
//pattern, or a parenthesized list of multiple patterns. i.e.
//
//  Policy (RejectByURL "http://*@*.badsite.com:*/*" )
//  Policy (AcceptByURL (
//              "http://*@www.goodsite.com:*/*"
//              "ftp://*@www.goodsite.com:*/*" ) )
//
//The general form of an URLpattern is:
//
// internet pattern         -   internetscheme://user@hostoraddr:port/pathmatch
// other pattern            -   otherscheme:nonquotedcharacters
//
// in all cases, an ommitted section only matches to a URL if that section
// was omitted in the URL being navigated to.
//
// the wild card character '*' may be used to match any pattern as specified
// on a per section basis below.  To encode the actual character '*' the escape
// sequence '%*' is recognized.
//
// recognized internet schemes are:
//      ftp, http, gopher, nntp, irc, prospero, telnet, and *
//
// the user section consists of '*' nonquotedcharacters '*', in other words, an
// alphanumeric user name with optional wild card sections before and after the
// name.  A single * matches all names.
//
// the hostoraddr section can be in one of two forms, either:
//      '*' hostname, or ipnum.ipnum.ipnum.ipnum!bitlength
// hostname must be a substring of a fully qualified domain name
// bitlength is an integer between 0 and 32 inclusive, and
// ipnum is an integer between 0 and 255 inclusive.
// the bitlength parameter masks out the last n bits of the 32 bit ip address
// specified (i.e. treats them as a wild card)
//
// the port section can have one of four forms:
//      *
//      *-portnum
//      portnum-*
//      portnum-portnum
//
// a single * matches against all port numbers, *-portnum matches all ports
// lessthan or equal to portnum, portnum-* matches all aports greaterthan or
// equal to portnum, and portnum-portnum matches all ports between the two
// portnums, inclusive.
//
// the pathmatch section has the form:
//      '*' nonquotedchars '*'
// i.e. *foo* would match any pathname containing the word foo.  A single *
// matches all pathnames.
HRESULT PICSRulesParseByURL(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    PICSRulesByURLExpression    *pPRByURLExpression;
    PICSRulesByURL              *pPRByURL;
    HRESULT                     hRes;
    LPSTR                       lpszCurrent;

    pPRByURL=new PICSRulesByURL;

    //first, we need to find out if we have a list of URLpatterns or a single
    //URLpattern

    if(**ppszIn=='(') //we have a list of patterns
    {
        lpszCurrent=pParser->FindNonWhite(*ppszIn+1);
        
        while(*lpszCurrent!=')')
        {
            LPSTR lpszSubString;

            if(*lpszCurrent=='\0')
            {
                delete pPRByURL;

                return(E_INVALIDARG);
            }

            hRes=PICSRulesParseString(&lpszCurrent,ppOut,pParser); //get the string

            if(FAILED(hRes))
            {
                //we couldn't get the string, so lets fail
                delete pPRByURL;

                return(hRes);
            }

            lpszSubString=(char *) *ppOut;

            //we've got it, so lets instantiate the classes to fill out;

            pPRByURLExpression=new PICSRulesByURLExpression;

            hRes=pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression) ? S_OK : E_OUTOFMEMORY;
        
            if(FAILED(hRes))
            {
                delete lpszSubString;
                delete pPRByURLExpression;
                delete pPRByURL;

                return(hRes);
            }

            hRes=PICSRulesParseSingleByURL(lpszSubString,pPRByURLExpression,pParser);
        
            if(FAILED(hRes))
            {
                delete lpszSubString;
                delete pPRByURL; //deleting the array deletes the embeeded expression

                return(hRes);
            }

            delete lpszSubString;
        }

        if(*lpszCurrent==')')
        {
            *ppszIn=pParser->FindNonWhite(lpszCurrent+1);
        }
    }
    else //we have a single pattern
    {
        hRes=PICSRulesParseString(ppszIn,ppOut,pParser); //get the string

        if(FAILED(hRes))
        {
            //we couldn't get the string, so lets fail
            delete pPRByURL;

            return(hRes);
        }

        lpszCurrent=(char *) *ppOut;

        //we've got it, so lets instantiate the classes to fill out;

        pPRByURLExpression=new PICSRulesByURLExpression;

        hRes=pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression) ? S_OK : E_OUTOFMEMORY;
        
        if(FAILED(hRes))
        {
            delete lpszCurrent;
            delete pPRByURLExpression;
            delete pPRByURL;

            return(hRes);
        }

        hRes=PICSRulesParseSingleByURL(lpszCurrent,pPRByURLExpression,pParser);
    
        if(FAILED(hRes))
        {
            delete lpszCurrent;
            delete pPRByURL; //deleting the array deletes the embeeded expression

            return(hRes);
        }

        delete lpszCurrent;
    }

    *ppOut=(void *) pPRByURL;

    return(NOERROR);
}

HRESULT PICSRulesParseSingleByURL(LPSTR lpszByURL, PICSRulesByURLExpression *pPRByURLExpression, PICSRulesFileParser *pParser)
{
    LPSTR lpszMarker;

    lpszMarker=strchrf(lpszByURL,':'); //find the marker '://' for an internet
                                       //pattern or ':' for a non-internet pattern

    if(lpszMarker==NULL) //no marker, i.e. our string is invalid
    {
        return(E_INVALIDARG);
    }

    //check the scheme for a wild card
    if(*lpszByURL=='*')
    {
        if((lpszByURL+1)!=lpszMarker) //we have a non-internet scheme
        {
            pPRByURLExpression->m_fInternetPattern=FALSE;

            *lpszMarker='\0';

            pPRByURLExpression->m_etstrScheme.Set(lpszByURL);

            lpszByURL=lpszMarker+1;

            pPRByURLExpression->m_etstrPath.Set(lpszByURL);

            return(S_OK);
        }

        //no need to set a NonWild flag, just move
        //on to the user name
        pPRByURLExpression->m_bSpecified|=BYURL_SCHEME;
    }
    else
    {
        *lpszMarker='\0';

        //check for an internet pattern

        if((lstrcmpi(lpszByURL,szPICSRulesFTP)!=0)&&
           (lstrcmpi(lpszByURL,szPICSRulesHTTP)!=0)&&
           (lstrcmpi(lpszByURL,szPICSRulesGOPHER)!=0)&&
           (lstrcmpi(lpszByURL,szPICSRulesNNTP)!=0)&&
           (lstrcmpi(lpszByURL,szPICSRulesIRC)!=0)&&
           (lstrcmpi(lpszByURL,szPICSRulesPROSPERO)!=0)&&
           (lstrcmpi(lpszByURL,szPICSRulesTELNET)!=0)) //we've got a non-internet pattern
        {
            pPRByURLExpression->m_fInternetPattern=FALSE;
            pPRByURLExpression->m_bNonWild=BYURL_SCHEME|BYURL_PATH;
            pPRByURLExpression->m_bSpecified=BYURL_SCHEME|BYURL_PATH;
            pPRByURLExpression->m_etstrScheme.Set(lpszByURL);

            lpszByURL=lpszMarker+1;

            pPRByURLExpression->m_etstrPath.Set(lpszByURL);

            return(S_OK);
        }

        pPRByURLExpression->m_bNonWild|=BYURL_SCHEME;
        pPRByURLExpression->m_bSpecified|=BYURL_SCHEME;
        pPRByURLExpression->m_etstrScheme.Set(lpszByURL);   
    }

    if((*(lpszMarker+1)=='/')&&(*(lpszMarker+2)=='/'))
    {
        pPRByURLExpression->m_fInternetPattern=TRUE;
        lpszByURL=lpszMarker+3;
    }
    else
    {
        return(E_INVALIDARG);
    }

    //we've got an internet pattern, and lpszURL now points
    //to the user field

    lpszMarker=strchrf(lpszByURL,'@'); //find the marker between user and host
    
    if(lpszMarker!=NULL) //a user name was specified
    {
        pPRByURLExpression->m_bSpecified|=BYURL_USER;

        //check for a wild card
        if(!((*lpszByURL=='*')&&((lpszByURL+1)==lpszMarker)))
        {
            pPRByURLExpression->m_bNonWild|=BYURL_USER;
            
            *lpszMarker='\0';

            pPRByURLExpression->m_etstrUser.Set(lpszByURL);
        }

        lpszByURL=lpszMarker+1;
    }

    //lpszByURL now points to host

    lpszMarker=strchrf(lpszByURL,':');

    if(lpszMarker==NULL) //the port was omitted
    {
        lpszMarker=strchrf(lpszByURL,'/');

        if(lpszMarker!=NULL) //there is a pathmatch
        {
            pPRByURLExpression->m_bSpecified|=BYURL_PATH;       
        }
    }
    else //we have a host and port
    {
        pPRByURLExpression->m_bSpecified|=BYURL_PORT;
    }
    
    pPRByURLExpression->m_bSpecified|=BYURL_HOST;

    if(lpszMarker!=NULL)
    {
        *lpszMarker='\0';
    }

    if(lstrcmp(lpszByURL,"*")!=0)
    {
        pPRByURLExpression->m_bNonWild|=BYURL_HOST;
    }

    pPRByURLExpression->m_etstrHost.Set(lpszByURL);

    if(lpszMarker==NULL)
    {
        return(S_OK);
    }

    lpszByURL=lpszMarker+1;

    if(pPRByURLExpression->m_bSpecified&BYURL_PORT)
    {
        lpszMarker=strchrf(lpszByURL,'/');

        if(lpszMarker!=NULL) //there is a pathmatch
        {
            pPRByURLExpression->m_bSpecified|=BYURL_PATH;       
            *lpszMarker='\0';   
        }

        if(!((*lpszByURL=='*')&&(lpszByURL+1==lpszMarker)))
        {
            pPRByURLExpression->m_bNonWild|=BYURL_PORT;

            pPRByURLExpression->m_etstrPort.Set(lpszByURL);
        }

        if(pPRByURLExpression->m_bSpecified&BYURL_PATH)
        {
            lpszByURL=lpszMarker+1;
        }
    }

    if(pPRByURLExpression->m_bSpecified&BYURL_PATH)
    {
        if(!((*lpszByURL=='*')&&(*(lpszByURL+1)==NULL)))
        {
            pPRByURLExpression->m_bNonWild|=BYURL_PATH;

            pPRByURLExpression->m_etstrPath.Set(lpszByURL);
        }
    }

    return(S_OK);
}

//PICSRules PolicyExpressions have 6 possible expressions:
//
// simple-expression        -   ( Service.Category [Operator] [Constant] )
// or-expression            -   ( PolicyExpression or PolicyExpression )
// and-expression           -   ( PolicyExpression and PolicyExpression )
// service & category       -   ( Service.Category )
// service only             -   ( Service )
// degenerate-expression    -   "otherwise"
//
// thus, for example, embedded expressions can take the form:
//
// "((Cool.Coolness < 3) or (Cool.Graphics < 3))"
// 
// or
//
// "(((Cool.Coolness < 3) or (Cool.Graphics < 3)) and (Cool.Fun < 2))"
//
// ad infinitum
//
// thus, existing pics labels can be encoded as:
//
// "((((RSACi.s <= 0) and (RSACi.v <= 0)) and (RSACi.n <= 0)) and RSACi.l <=0)"
//
// allowable operators are: '<', '<=', '=', '>=', '>'
//
// the service only expression evaluates to TRUE iff a label from that
// service is found.
//
// the service & category expression evaluates to TRUE iff a label from
// that service is found, and it contains at least one value for the
// indicated category.
//
// the degenerate-expression always evaluates to TRUE
HRESULT PICSRulesParsePolicyExpression(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    PICSRulesPolicyExpression   *pPRPolicyExpression;
    HRESULT                     hRes;
    LPSTR                       lpszPolicyExpression,lpszCurrent;

    //first lets get the string
    hRes=PICSRulesParseString(ppszIn,ppOut,pParser);

    if(FAILED(hRes))
    {
        //we couldn't get the string, so lets fail
        return(hRes);
    }
    lpszPolicyExpression=(char *) *ppOut;

    //we've got it, so lets instantiate a PICSRulesPolicyExpression to fill out
    pPRPolicyExpression=new PICSRulesPolicyExpression;

    if(pPRPolicyExpression==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    pPRPolicyExpression->m_PROPolicyOperator=PR_OPERATOR_RESULT; //set as the topmost node
                                                                 //of the binary tree

    if(lstrcmpi(lpszPolicyExpression,szPICSRulesDegenerateExpression)==0)
    {
        //we have a degenerate expression, so delete lpszPolicyExpresion

        delete lpszPolicyExpression;
        pPRPolicyExpression->m_PROPolicyOperator=PR_OPERATOR_DEGENERATE;

        *ppOut=(LPVOID) pPRPolicyExpression;

        return(NOERROR);
    }

    //make sure we have a parenthesized object
    if(*lpszPolicyExpression!='(')
    {
        delete lpszPolicyExpression;
        delete pPRPolicyExpression;
        
        return(E_INVALIDARG);
    }
    
    lpszCurrent=NonWhite(lpszPolicyExpression+1);

    //check for an or-expression or an and-expression
    if(*lpszCurrent=='(')
    {
        hRes=PICSRulesParseSubPolicyExpression(lpszCurrent,pPRPolicyExpression,pParser);

        if(FAILED(hRes))
        {
            delete lpszPolicyExpression;
            delete pPRPolicyExpression;
            
            return(hRes);
        }
        else
        {
            if((pPRPolicyExpression->m_pPRPolicyExpressionLeft)!=NULL)
            {
                BOOL fFlag;
                
                fFlag=pPRPolicyExpression->m_pPRPolicyExpressionLeft->m_prYesNoUseEmbedded.GetYesNo();

                pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);
            }
        }
    }
    else //we've got a simple-expression
    {
        hRes=PICSRulesParseSimplePolicyExpression(lpszCurrent,pPRPolicyExpression,pParser);

        if(FAILED(hRes))
        {
            delete lpszPolicyExpression;
            delete pPRPolicyExpression;

            return(hRes);
        }
    }

    delete lpszPolicyExpression;
    *ppOut=(void *) pPRPolicyExpression;

    return(NOERROR);
}

//Our PolicyExpression is either an or-expression or an and-expression
//so pPRPolicyExpression need to have another PICSRulesPolicyExpression
//embedded in it, with all the details filled out.
HRESULT PICSRulesParseSubPolicyExpression(LPSTR& lpszCurrent,PICSRulesPolicyExpression *pPRPolicyExpression,PICSRulesFileParser *pParser)
{
    HRESULT hRes;
    PICSRulesPolicyExpression   * pPRPolicyExpressionEmbeddedLeft,
                                * pPRPolicyExpressionEmbeddedRight;
    LPSTR                       lpszNextPolicyExpression,
                                lpszOrAnd,lpszOrAndEnd;
    int                         iStringLen;
    BOOL                        fFlag;

    lpszCurrent=NonWhite(lpszCurrent+1);

    //check for nested or-expressions and and-expressions
    if(*lpszCurrent=='(')
    {
        pPRPolicyExpressionEmbeddedLeft=new PICSRulesPolicyExpression;

        if(pPRPolicyExpressionEmbeddedLeft==NULL)
        {
            return(E_OUTOFMEMORY);
        }

        pPRPolicyExpressionEmbeddedLeft->m_PROPolicyOperator=PR_OPERATOR_RESULT;

        hRes=PICSRulesParseSubPolicyExpression(lpszCurrent,pPRPolicyExpressionEmbeddedLeft,pParser);

        if(FAILED(hRes))
        {
            delete pPRPolicyExpressionEmbeddedLeft;

            return(hRes);
        }

        pPRPolicyExpression->m_pPRPolicyExpressionLeft=pPRPolicyExpressionEmbeddedLeft;
        
        fFlag=pPRPolicyExpressionEmbeddedLeft->m_prYesNoUseEmbedded.GetYesNo();

        pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);
    }
    else //only one level deep
    {
        pPRPolicyExpressionEmbeddedLeft=new PICSRulesPolicyExpression;

        if(pPRPolicyExpressionEmbeddedLeft==NULL)
        {
            return(E_OUTOFMEMORY);
        }

        hRes=PICSRulesParseSimplePolicyExpression(lpszCurrent,pPRPolicyExpressionEmbeddedLeft,pParser);

        if(FAILED(hRes))
        {
            delete pPRPolicyExpressionEmbeddedLeft;

            return(hRes);
        }

        pPRPolicyExpression->m_pPRPolicyExpressionLeft=pPRPolicyExpressionEmbeddedLeft;

        fFlag=pPRPolicyExpressionEmbeddedLeft->m_prYesNoUseEmbedded.GetYesNo();

        pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);

        lpszCurrent=strchrf(lpszCurrent,')');
        lpszCurrent=NonWhite(lpszCurrent+1);
    }

    lpszNextPolicyExpression=strchrf(lpszCurrent,'(');
    
    if(lpszNextPolicyExpression==NULL) //invalid policy expression
    {
        return(E_INVALIDARG);
    }

    lpszOrAndEnd=White(lpszCurrent);

    if(lpszOrAndEnd>lpszNextPolicyExpression) //no white space
    {
        lpszOrAndEnd=lpszNextPolicyExpression;
    }

    iStringLen=(int) (lpszOrAndEnd-lpszCurrent);

    lpszOrAnd=new char[iStringLen+1];

    if(lpszOrAnd==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    memcpyf(lpszOrAnd,lpszCurrent,iStringLen);
    lpszOrAnd[iStringLen]='\0';

    if(lstrcmpi(lpszOrAnd,szPICSRulesAnd)==0)
    {
        pPRPolicyExpression->m_PRPEPolicyEmbedded=PR_POLICYEMBEDDED_AND;
    }
    else if(lstrcmpi(lpszOrAnd,szPICSRulesOr)==0)
    {
        pPRPolicyExpression->m_PRPEPolicyEmbedded=PR_POLICYEMBEDDED_OR;
    }
    else
    {
        delete lpszOrAnd;

        return(E_INVALIDARG);
    }

    delete lpszOrAnd;

    lpszCurrent=NonWhite(lpszOrAndEnd+1);

    if(lpszCurrent!=lpszNextPolicyExpression)
    {
        return(E_INVALIDARG);
    }

    lpszCurrent=NonWhite(lpszCurrent+1);

    //do we have more embedded objects, or another simple-expression?
    if(*lpszCurrent=='(') //more embedded
    {
        pPRPolicyExpressionEmbeddedRight=new PICSRulesPolicyExpression;

        if(pPRPolicyExpressionEmbeddedRight==NULL)
        {
            return(E_OUTOFMEMORY);
        }

        pPRPolicyExpressionEmbeddedRight->m_PROPolicyOperator=PR_OPERATOR_RESULT;
        
        hRes=PICSRulesParseSubPolicyExpression(lpszCurrent,pPRPolicyExpressionEmbeddedRight,pParser);

        if(FAILED(hRes))
        {
            return(hRes);
        }

        if(*lpszCurrent!=')')
        {
            delete pPRPolicyExpressionEmbeddedRight;

            return(E_INVALIDARG);
        }

        lpszCurrent=NonWhite(lpszCurrent+1);

        pPRPolicyExpression->m_pPRPolicyExpressionRight=pPRPolicyExpressionEmbeddedRight;

        fFlag=pPRPolicyExpressionEmbeddedRight->m_prYesNoUseEmbedded.GetYesNo();

        pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);
    }
    else //simple expression
    {
        pPRPolicyExpressionEmbeddedRight=new PICSRulesPolicyExpression;

        if(pPRPolicyExpressionEmbeddedRight==NULL)
        {
            return(E_OUTOFMEMORY);
        }

        hRes=PICSRulesParseSimplePolicyExpression(lpszCurrent,pPRPolicyExpressionEmbeddedRight,pParser);

        if(FAILED(hRes))
        {
            delete pPRPolicyExpressionEmbeddedRight;

            return(hRes);
        }

        lpszCurrent=strchrf(lpszCurrent,')');
        lpszCurrent=NonWhite(lpszCurrent+1);

        if(*lpszCurrent!=')')
        {
            delete pPRPolicyExpressionEmbeddedRight;

            return(E_INVALIDARG);
        }

        lpszCurrent=NonWhite(lpszCurrent+1);

        pPRPolicyExpression->m_pPRPolicyExpressionRight=pPRPolicyExpressionEmbeddedRight;

        fFlag=pPRPolicyExpressionEmbeddedRight->m_prYesNoUseEmbedded.GetYesNo();

        pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);
    }

    return(S_OK);
}

HRESULT PICSRulesParseSimplePolicyExpression(LPSTR& lpszCurrent,PICSRulesPolicyExpression *pPRPolicyExpression,PICSRulesFileParser *pParser)
{
    LPSTR                lpszEnd,lpszDot;
    PICSRulesServiceInfo *pPRServiceInfo=NULL;
    
    lpszEnd=strchrf(lpszCurrent,')');

    if(lpszEnd==NULL) //we don't have a valid expression
    {
        return(E_INVALIDARG);
    }

    lpszDot=strchrf(lpszCurrent,'.');

    if(lpszDot==NULL) //we have a service only expression
    {
        LPSTR   lpszService,lpszServiceEnd,lpszFullService;
        int     iStringLen;

        lpszServiceEnd=White(lpszCurrent);
        
        if(lpszServiceEnd>lpszEnd) //there isn't any white space between
                                   //the service name and the closing
                                   //parenthesis
        {
            lpszServiceEnd=lpszEnd;
        }

        iStringLen=(int)(lpszServiceEnd-lpszCurrent);

        lpszService=new char[iStringLen+1];

        if(lpszService==NULL)
        {
            return(E_OUTOFMEMORY);
        }

        memcpyf(lpszService,lpszCurrent,iStringLen);
        lpszService[iStringLen]='\0';

        lpszFullService=new char[INTERNET_MAX_URL_LENGTH+1];

        if(IsServiceDefined(lpszService,lpszFullService,&pPRServiceInfo)==FALSE)
        {
            delete lpszService;
            delete lpszFullService;
            
            return(PICSRULES_E_SERVICEUNDEFINED);
        }

        //we have a valid service only expression
        if(pPRServiceInfo!=NULL)
        {
            BOOL fFlag;

            fFlag=pPRServiceInfo->m_prYesNoUseEmbedded.GetYesNo();
            pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);
        }

        pPRPolicyExpression->m_etstrServiceName.SetTo(lpszService);
        pPRPolicyExpression->m_etstrFullServiceName.SetTo(lpszFullService);
        pPRPolicyExpression->m_PROPolicyOperator=PR_OPERATOR_SERVICEONLY;
    }
    else //could be service and category or a full simple-expression
    {
        LPSTR   lpszService,lpszCategory,lpszCategoryEnd,lpszOperator,lpszFullService;
        int     iStringLen;

        lpszCategoryEnd=White(lpszCurrent);
        
        if(lpszCategoryEnd>lpszEnd) //there isn't any white space between
                                    //the category name and the closing
                                    //parenthesis
        { 
            lpszCategoryEnd=lpszEnd;
        }

        lpszOperator=strchrf(lpszCurrent,'<');

        if(lpszOperator!=NULL)
        {
            if(lpszOperator<lpszCategoryEnd) //there was an operator with
                                             //no white space
            {
                lpszCategoryEnd=lpszOperator;
            }
        }

        lpszOperator=strchrf(lpszCurrent,'>');

        if(lpszOperator!=NULL)
        {
            if(lpszOperator<lpszCategoryEnd) //there was an operator with
                                             //no white space
            {
                lpszCategoryEnd=lpszOperator;
            }
        }

        lpszOperator=strchrf(lpszCurrent,'=');

        if(lpszOperator!=NULL)
        {
            if(lpszOperator<lpszCategoryEnd) //there was an operator with
                                             //no white space
            {
                lpszCategoryEnd=lpszOperator;
            }
        }

        iStringLen=(int)(lpszDot-lpszCurrent);

        lpszService=new char[iStringLen+1];

        if(lpszService==NULL)
        {
            return(E_OUTOFMEMORY);
        }

        memcpyf(lpszService,lpszCurrent,iStringLen);
        lpszService[iStringLen]='\0';

        lpszFullService=new char[INTERNET_MAX_URL_LENGTH+1];

        if(IsServiceDefined(lpszService,lpszFullService,&pPRServiceInfo)==FALSE)
        {
            delete lpszService;
            delete lpszFullService;
            
            return(PICSRULES_E_SERVICEUNDEFINED);
        }

        iStringLen=(int)(lpszCategoryEnd-lpszDot-1);
        
        lpszCategory=new char[iStringLen+1];

        if(lpszCategory==NULL)
        {
            return(E_OUTOFMEMORY);
        }

        memcpyf(lpszCategory,lpszDot+1,iStringLen);
        lpszCategory[iStringLen]='\0';

        lpszCurrent=NonWhite(lpszCategoryEnd);

        if(*lpszCurrent==')') //we have a valid service and category expression
        {
            if(pPRServiceInfo!=NULL)
            {
                BOOL fFlag;

                fFlag=pPRServiceInfo->m_prYesNoUseEmbedded.GetYesNo();
                pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);
            }

            pPRPolicyExpression->m_etstrServiceName.SetTo(lpszService);
            pPRPolicyExpression->m_etstrFullServiceName.SetTo(lpszFullService);
            pPRPolicyExpression->m_etstrCategoryName.SetTo(lpszCategory);
            pPRPolicyExpression->m_PROPolicyOperator=PR_OPERATOR_SERVICEANDCATEGORY;
        }
        else //we have a full simple-expression
        {
            //lpszCurrent should be pointing to an operator
            enum PICSRulesOperators PROPolicyOperator;
            int                     iValue;

            switch(*lpszCurrent)
            {
                case '>':
                {
                    if(*(lpszCurrent+1)=='=')
                    {
                        PROPolicyOperator=PR_OPERATOR_GREATEROREQUAL;
                        lpszCurrent=NonWhite(lpszCurrent+2);
                    }
                    else
                    {
                        PROPolicyOperator=PR_OPERATOR_GREATER;
                        lpszCurrent=NonWhite(lpszCurrent+1);
                    }

                    break;
                }
                case '<':
                {
                    if(*(lpszCurrent+1)=='=')
                    {
                        PROPolicyOperator=PR_OPERATOR_LESSOREQUAL;
                        lpszCurrent=NonWhite(lpszCurrent+2);
                    }
                    else
                    {
                        PROPolicyOperator=PR_OPERATOR_LESS;
                        lpszCurrent=NonWhite(lpszCurrent+1);
                    }

                    break;
                }
                case '=':
                {
                    PROPolicyOperator=PR_OPERATOR_EQUAL;
                    lpszCurrent=NonWhite(lpszCurrent+1);

                    break;
                }
                default: //we didn't get a valid operator
                {
                    delete lpszService;
                    delete lpszCategory;
                    
                    return(E_INVALIDARG);
                }
            }

            //lpszCurrent now points at the Value
            if(FAILED(ParseNumber(&lpszCurrent,&iValue,FALSE)))
            {
                delete lpszService;
                delete lpszCategory;

                return(E_INVALIDARG);
            }

            if(*lpszCurrent!=')') //we should be done, so the argument is invalid
            {
                delete lpszService;
                delete lpszCategory;

                return(E_INVALIDARG);           
            }

            //we now have a complete simple-expression
            if(pPRServiceInfo!=NULL)
            {
                BOOL fFlag;

                fFlag=pPRServiceInfo->m_prYesNoUseEmbedded.GetYesNo();
                pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);
            }

            pPRPolicyExpression->m_etstrServiceName.SetTo(lpszService);
            pPRPolicyExpression->m_etstrFullServiceName.SetTo(lpszFullService);
            pPRPolicyExpression->m_etstrCategoryName.SetTo(lpszCategory);
            pPRPolicyExpression->m_etnValue.Set(iValue);
            pPRPolicyExpression->m_PROPolicyOperator=PROPolicyOperator;
        }
    }

    return(S_OK);
}

//Determines if the service name in lpszService has been read in a
//ServiceInfo section of the PICSRules file
BOOL IsServiceDefined(LPSTR lpszService,LPSTR lpszFullService,PICSRulesServiceInfo **ppServiceInfo)
{
    array<PICSRulesServiceInfo*>    *arrpPRServiceInfo;
    LPSTR                           lpszShortName;
    int                             iNumServices,iCounter;
    BOOL                            fDefined=FALSE;

    if(g_pPRRS==NULL)
    {
        return(FALSE);
    }

    arrpPRServiceInfo=(array<PICSRulesServiceInfo*> *) &(g_pPRRS->m_arrpPRServiceInfo);

    iNumServices=arrpPRServiceInfo->Length();

    for(iCounter=0;iCounter<iNumServices;iCounter++)
    {
        PICSRulesServiceInfo * pPRServiceInfo;
        
        pPRServiceInfo=(*arrpPRServiceInfo)[iCounter];

        lpszShortName=pPRServiceInfo->m_etstrShortName.Get();

        if(lstrcmp(lpszService,lpszShortName)==0)
        {
            fDefined=TRUE;

            if(ppServiceInfo!=NULL)
            {
                *ppServiceInfo=pPRServiceInfo;
            }

            lstrcpy(lpszFullService,pPRServiceInfo->m_prURLName.Get());

            break;
        }
    }

    return(fDefined);
}

//Determines if the extension name in lpszExtension has been read in a
//OptExtension of the PICSRules file
BOOL IsOptExtensionDefined(LPSTR lpszExtension)
{
    array<PICSRulesOptExtension*>   *arrpPROptExtension;
    LPSTR                           lpszShortName;
    int                             iNumExtensions,iCounter;
    BOOL                            fDefined=FALSE;

    if(g_pPRRS==NULL)
    {
        return(FALSE);
    }

    arrpPROptExtension=(array<PICSRulesOptExtension*> *) &(g_pPRRS->m_arrpPROptExtension);

    iNumExtensions=arrpPROptExtension->Length();

    for(iCounter=0;iCounter<iNumExtensions;iCounter++)
    {
        PICSRulesOptExtension * pPROptExtension;
        
        pPROptExtension=(*arrpPROptExtension)[iCounter];

        lpszShortName=pPROptExtension->m_etstrShortName.Get();

        if(lstrcmp(lpszExtension,lpszShortName)==0)
        {
            fDefined=TRUE;
            
            break;
        }
    }

    return(fDefined);
}

//Deteremines is the extension name in lpszExtension has been read in a
//ReqExtension of the PICSRules file
BOOL IsReqExtensionDefined(LPSTR lpszExtension)
{
    array<PICSRulesReqExtension*>   *arrpPRReqExtension;
    LPSTR                           lpszShortName;
    int                             iNumExtensions,iCounter;
    BOOL                            fDefined=FALSE;

    if(g_pPRRS==NULL)
    {
        return(FALSE);
    }

    arrpPRReqExtension=(array<PICSRulesReqExtension*> *) &(g_pPRRS->m_arrpPRReqExtension);

    iNumExtensions=arrpPRReqExtension->Length();

    for(iCounter=0;iCounter<iNumExtensions;iCounter++)
    {
        PICSRulesReqExtension * pPRReqExtension;
        
        pPRReqExtension=(*arrpPRReqExtension)[iCounter];

        lpszShortName=pPRReqExtension->m_etstrShortName.Get();

        if(lstrcmp(lpszExtension,lpszShortName)==0)
        {
            fDefined=TRUE;
            
            break;
        }
    }

    return(fDefined);
}

PICSRulesAllowableOption aaoPICSRulesName[] = {
    { PROID_RULENAME, 0 },
    { PROID_DESCRIPTION, 0 },
    { PROID_EXTENSION, 0 },
    { PROID_INVALID, 0 }
};
const UINT caoPICSRulesName=sizeof(aaoPICSRulesName)/sizeof(aaoPICSRulesName[0]);

HRESULT PICSRulesParseName(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    //We must make a copy of the allowable options array because the
    //parser will fiddle with the flags in the entries -- specifically,
    //setting AO_SEEN.  It wouldn't be thread-safe to do this to a
    //static array.

    PICSRulesAllowableOption aao[caoPICSRulesName];

    ::memcpyf(aao,::aaoPICSRulesName,sizeof(aao));

    PICSRulesName *pName=new PICSRulesName;
    
    if(pName==NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hres=pParser->ParseParenthesizedObject(
                            ppszIn,                 //var containing current ptr
                            aao,                    //what's legal in this object
                            pName);                 //object to add items back to

    if (FAILED(hres))
    {
        delete pName;
        return hres;
    }

    *ppOut=(LPVOID) pName;

    return NOERROR;
}

PICSRulesAllowableOption aaoPICSRulesSource[] = {
    { PROID_SOURCEURL, 0 },
    { PROID_CREATIONTOOL, 0 },
    { PROID_AUTHOR, 0 },
    { PROID_LASTMODIFIED, 0 },
    { PROID_EXTENSION, 0 },
    { PROID_INVALID, 0 }
};
const UINT caoPICSRulesSource=sizeof(aaoPICSRulesSource)/sizeof(aaoPICSRulesSource[0]);

HRESULT PICSRulesParseSource(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    //We must make a copy of the allowable options array because the
    //parser will fiddle with the flags in the entries -- specifically,
    //setting AO_SEEN.  It wouldn't be thread-safe to do this to a
    //static array.

    PICSRulesAllowableOption aao[caoPICSRulesSource];

    ::memcpyf(aao,::aaoPICSRulesSource,sizeof(aao));

    PICSRulesSource *pSource=new PICSRulesSource;
    
    if(pSource==NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hres=pParser->ParseParenthesizedObject(
                            ppszIn,                 //var containing current ptr
                            aao,                    //what's legal in this object
                            pSource);               //object to add items back to

    if (FAILED(hres))
    {
        delete pSource;
        return hres;
    }

    *ppOut=(LPVOID) pSource;

    return NOERROR;
}

PICSRulesAllowableOption aaoPICSRulesServiceInfo[] = {
    { PROID_SINAME, AO_SINGLE },
    { PROID_SHORTNAME, AO_SINGLE },
    { PROID_BUREAUURL, 0 },
    { PROID_USEEMBEDDED, AO_SINGLE },
    { PROID_RATFILE, AO_SINGLE },
    { PROID_BUREAUUNAVAILABLE, AO_SINGLE },
    { PROID_EXTENSION, 0 },
    { PROID_INVALID, 0 }
};
const UINT caoPICSRulesServiceInfo=sizeof(aaoPICSRulesServiceInfo)/sizeof(aaoPICSRulesServiceInfo[0]);

HRESULT PICSRulesParseServiceInfo(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    //We must make a copy of the allowable options array because the
    //parser will fiddle with the flags in the entries -- specifically,
    //setting AO_SEEN.  It wouldn't be thread-safe to do this to a
    //static array.

    PICSRulesAllowableOption aao[caoPICSRulesServiceInfo];

    ::memcpyf(aao,::aaoPICSRulesServiceInfo,sizeof(aao));

    PICSRulesServiceInfo *pServiceInfo=new PICSRulesServiceInfo;
    
    if(pServiceInfo==NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hres=pParser->ParseParenthesizedObject(
                            ppszIn,                 //var containing current ptr
                            aao,                    //what's legal in this object
                            pServiceInfo);          //object to add items back to

    if (FAILED(hres))
    {
        delete pServiceInfo;
        return hres;
    }

    *ppOut=(LPVOID) pServiceInfo;

    return NOERROR;
}

PICSRulesAllowableOption aaoPICSRulesOptExtension[] = {
    { PROID_EXTENSIONNAME, AO_SINGLE },
    { PROID_SHORTNAME, AO_SINGLE },
    { PROID_EXTENSION, 0 },
    { PROID_INVALID, 0 }
};
const UINT caoPICSRulesOptExtension=sizeof(aaoPICSRulesOptExtension)/sizeof(aaoPICSRulesOptExtension[0]);

HRESULT PICSRulesParseOptExtension(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    //We must make a copy of the allowable options array because the
    //parser will fiddle with the flags in the entries -- specifically,
    //setting AO_SEEN.  It wouldn't be thread-safe to do this to a
    //static array.

    PICSRulesAllowableOption aao[caoPICSRulesOptExtension];

    ::memcpyf(aao,::aaoPICSRulesOptExtension,sizeof(aao));

    PICSRulesOptExtension *pOptExtension=new PICSRulesOptExtension;
    
    if(pOptExtension==NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hres=pParser->ParseParenthesizedObject(
                            ppszIn,                 //var containing current ptr
                            aao,                    //what's legal in this object
                            pOptExtension);         //object to add items back to

    if (FAILED(hres))
    {
        delete pOptExtension;
        return hres;
    }

    *ppOut=(LPVOID) pOptExtension;

    return NOERROR;
}

PICSRulesAllowableOption aaoPICSRulesReqExtension[] = {
    { PROID_EXTENSIONNAME, AO_SINGLE },
    { PROID_SHORTNAME, AO_SINGLE },
    { PROID_EXTENSION, 0 },
    { PROID_INVALID, 0 }
};
const UINT caoPICSRulesReqExtension=sizeof(aaoPICSRulesReqExtension)/sizeof(aaoPICSRulesReqExtension[0]);

HRESULT PICSRulesParseReqExtension(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    //We must make a copy of the allowable options array because the
    //parser will fiddle with the flags in the entries -- specifically,
    //setting AO_SEEN.  It wouldn't be thread-safe to do this to a
    //static array.

    PICSRulesAllowableOption aao[caoPICSRulesReqExtension];

    ::memcpyf(aao,::aaoPICSRulesReqExtension,sizeof(aao));

    PICSRulesReqExtension *pReqExtension=new PICSRulesReqExtension;
    
    if(pReqExtension==NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hres=pParser->ParseParenthesizedObject(
                            ppszIn,                 //var containing current ptr
                            aao,                    //what's legal in this object
                            pReqExtension);         //object to add items back to

    if (FAILED(hres))
    {
        delete pReqExtension;
        return hres;
    }

    *ppOut=(LPVOID) pReqExtension;

    return NOERROR;
}

//Currently, we acknowledge no extensions.  If support for an extension
//needs to be added in the future, a PICSRulesParseExtensionName function
//should be added, similar to the other PICSRulesParseSection functions.
//This function should be called after confirming the extension string
//here.
//
//For now, we just eat the extensions
HRESULT PICSRulesParseExtension(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser)
{
    LPTSTR lpszExtension,lpszEnd;

    lpszEnd=strchrf(*ppszIn,'.');
    
    if(lpszEnd==NULL)
    {
        return(PICSRULES_E_UNKNOWNITEM);
    }

    *lpszEnd='\0';

    //*ppszIn now points to the extension name
    //if we ever implement support for extensions, we'll need to do a comparison
    //here.  After the comparison is completed, the following code will point
    //to the extension's method.

    *ppszIn=lpszEnd+1;

    lpszEnd=strchrf(*ppszIn,'(');
    
    if(lpszEnd==NULL)
    {
        return(PICSRULES_E_EXPECTEDLEFT);
    }

    lpszExtension=White(*ppszIn);

    if((lpszExtension!=NULL)&&(lpszExtension<lpszEnd))
    {
        *lpszExtension='\0';
    }
    else
    {
        *lpszEnd='\0';
    }

    lpszExtension=*ppszIn;

    //lpszExtension now points to the clause on the given extension name
    //if we ever implement support for extensions, we'll need to do a comparison
    //here.  Using both this comparison and the one above, a callback needs
    //to be implemented to support the extension, for now we'll just parse to
    //the closing parenthesis and eat the extension.

    *ppszIn=lpszEnd+1;

    int iOpenParenthesis=1;

    do
    {
        if(**ppszIn=='(')
        {
            iOpenParenthesis++;
        }
        else if (**ppszIn==')')
        {
            iOpenParenthesis--;

            if(iOpenParenthesis==0)
            {
                break;
            }
        }

        *ppszIn=pParser->FindNonWhite(*ppszIn+1);

    } while (**ppszIn!='\0');

    if(**ppszIn=='\0')
    {
        return(PICSRULES_E_EXPECTEDRIGHT);
    }
    else
    {
        *ppszIn=pParser->FindNonWhite(*ppszIn+1);
    }

    *ppOut=(LPVOID) NULL;

    return NOERROR;
}

//*******************************************************************
//*
//* Code for the PICSRulesRatingSystem class
//*
//*******************************************************************
PICSRulesRatingSystem::PICSRulesRatingSystem()
    : m_dwFlags(0),
      m_nErrLine(0)
{
    // nothing to do but construct members
}

PICSRulesRatingSystem::~PICSRulesRatingSystem()
{
    m_arrpPRPolicy.DeleteAll();
    m_arrpPRServiceInfo.DeleteAll();
    m_arrpPROptExtension.DeleteAll();
    m_arrpPRReqExtension.DeleteAll();
}

HRESULT PICSRulesRatingSystem::InitializeMyDefaults()
{
    return NOERROR;     //no defaults to initialize
}

//Allowable options from within PICSRulesRaginSystem's scope include only the
//first teer of aaoPICSRules[] defined in picsrule.h
PICSRulesAllowableOption aaoPICSRulesRatingSystem[] = {
    { PROID_PICSVERSION, 0 },
    
    { PROID_POLICY, AO_MANDATORY },
    { PROID_NAME, AO_SINGLE },
    { PROID_SOURCE, AO_SINGLE },
    { PROID_SERVICEINFO, 0 },
    { PROID_OPTEXTENSION, 0 },
    { PROID_REQEXTENSION, 0 },
    { PROID_EXTENSION, 0 },

    { PROID_INVALID, 0 }
};
const UINT caoPICSRulesRatingSystem=sizeof(aaoPICSRulesRatingSystem)/sizeof(aaoPICSRulesRatingSystem[0]);

//The following array is indexed by PICSRulesObjectID values.
//PICSRulesObjectHandler is defined in mslubase.h as:
//typedef HRESULT (*PICSRulesObjectHandler)(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser);
struct {
    LPCSTR lpszToken;                       //token by which we identify it
    PICSRulesObjectHandler pHandler;        //function which parses the object's contents
} aPRObjectDescriptions[] = {
    { szNULL, NULL },
    { szPICSRulesVersion, PICSRulesParseVersion },
    { szPICSRulesPolicy, PICSRulesParsePolicy },
    { szPICSRulesExplanation, PICSRulesParseString },
    { szPICSRulesRejectByURL, PICSRulesParseByURL },
    { szPICSRulesAcceptByURL, PICSRulesParseByURL },
    { szPICSRulesRejectIf, PICSRulesParsePolicyExpression },
    { szPICSRulesAcceptIf, PICSRulesParsePolicyExpression },
    { szPICSRulesAcceptUnless, PICSRulesParsePolicyExpression },
    { szPICSRulesRejectUnless, PICSRulesParsePolicyExpression },
    { szPICSRulesName, PICSRulesParseName },
    { szPICSRulesRuleName, PICSRulesParseString },
    { szPICSRulesDescription, PICSRulesParseString },
    { szPICSRulesSource, PICSRulesParseSource },
    { szPICSRulesSourceURL, PICSRulesParseString },
    { szPICSRulesCreationTool, PICSRulesParseString },
    { szPICSRulesAuthor, PICSRulesParseString },
    { szPICSRulesLastModified, PICSRulesParseString },
    { szPICSRulesServiceInfo, PICSRulesParseServiceInfo },
    { szPICSRulesSIName, PICSRulesParseString },
    { szPICSRulesShortName, PICSRulesParseString },
    { szPICSRulesBureauURL, PICSRulesParseString },
    { szPICSRulesUseEmbedded, PICSRulesParseYesNo },
    { szPICSRulesRATFile, PICSRulesParseString },
    { szPICSRulesBureauUnavailable, PICSRulesParsePassFail },
    { szPICSRulesOptExtension, PICSRulesParseOptExtension },
    { szPICSRulesExtensionName, PICSRulesParseString },
  //{ szPICSRulesShortName, PICSRulesParseString },
    { szPICSRulesReqExtension, PICSRulesParseReqExtension },
  //{ szPICSRulesExtensionName, PICSRulesParseString },
  //{ szPICSRulesShortName, PICSRulesParseString },
    { szPICSRulesExtension, PICSRulesParseExtension },
    { szPICSRulesOptionDefault, PICSRulesParseString },
    { szPICSRulesOptionDefault, PICSRulesParseString },
    { szPICSRulesOptionDefault, PICSRulesParseString },
    { szPICSRulesOptionDefault, PICSRulesParseString },
    { szPICSRulesOptionDefault, PICSRulesParseString },
    { szPICSRulesOptionDefault, PICSRulesParseString }
};

HRESULT PICSRulesRatingSystem::Parse(LPCSTR pszFilename, LPSTR pIn)
{
    //This guy is small enough to just init directly on the stack
    PICSRulesAllowableOption aaoRoot[] = { { PROID_PICSVERSION, 0 }, { PROID_INVALID, 0 } };
    PICSRulesAllowableOption aao[caoPICSRulesRatingSystem];

    ::memcpyf(aao,::aaoPICSRulesRatingSystem,sizeof(aao));

    PICSRulesAllowableOption *pFound;

    PICSRulesFileParser parser;

    LPSTR lpszVersionDash=strchrf(pIn,'-');     //since this is the first
                                                //time through, we need to
                                                //prepare the PicsRule
                                                //token for the parser

    if(lpszVersionDash!=NULL)                   //check for no dash we'll
                                                //fail in ParseToOpening
                                                //if this is the case
    {
        *lpszVersionDash=' ';                   //set it up for the parser
    }

    HRESULT hres=parser.ParseToOpening(&pIn,aaoRoot,&pFound);

    if (FAILED(hres))
    {
        return hres;                            //some error early on
    }
    else                                        //we got the PicsRule tag
                                                //now we need to check
                                                //the version number
    {
        LPSTR   lpszDot=strchrf(pIn,'.');
        
        if(lpszDot!=NULL)                       //continue on and fail
                                                //in ParseParenthesizedObject
        {
            int iVersion;

            *lpszDot=' ';

            ParseNumber(&pIn,&iVersion,TRUE);
            m_etnPRVerMajor.Set(iVersion);

            pIn=parser.FindNonWhite(pIn);

            ParseNumber(&pIn,&iVersion,TRUE);
            m_etnPRVerMinor.Set(iVersion);

            pIn=parser.FindNonWhite(pIn);
        }
    }

    //we'll fail if the version is 1.0, or 2.0 or higher
    //versions 1.1 - 2.0 (not including 2.0) will pass

    int iVerNumber=m_etnPRVerMajor.Get();

    if(iVerNumber!=1)
    {
        hres=PICSRULES_E_VERSION;
        m_nErrLine=parser.m_nLine;

        return(hres);
    }
    else //check the minor version number
    {
        iVerNumber=m_etnPRVerMinor.Get();

        if(iVerNumber==0)
        {
            hres=PICSRULES_E_VERSION;
            m_nErrLine=parser.m_nLine;

            return(hres);
        }
    }

    hres=parser.ParseParenthesizedObject(
                        &pIn,                   //var containing current ptr
                        aao,                    //what's legal in this object
                        this);                  //object to add items back to

    if(SUCCEEDED(hres))
    {
        if(*pIn!=')') //check for a closing parenthesis
        {
            hres=PICSRULES_E_EXPECTEDRIGHT;
        }
        else
        {
            LPTSTR lpszEnd=NonWhite(pIn+1);

            if(*lpszEnd!='\0') // make sure we're at the end of the file
            {
                hres=PICSRULES_E_EXPECTEDEND;
            }
        }
    }

    if(FAILED(hres))
    {
        m_nErrLine=parser.m_nLine;
    }

    return hres;
}

HRESULT PICSRulesRatingSystem::AddItem(PICSRulesObjectID roid, LPVOID pData)
{
    HRESULT hres = S_OK;

    switch (roid)
    {
        case PROID_PICSVERSION:
        {
            //Takes a pointer to a PICSRULES_VERSION struct (defined in picsrule.h)
            PICSRULES_VERSION * PRVer;

            if(PRVer=((PICSRULES_VERSION *) pData))
            {
                m_etnPRVerMajor.Set(PRVer->iPICSRulesVerMajor);     
                m_etnPRVerMinor.Set(PRVer->iPICSRulesVerMinor);     
            }
            else
            {
                hres=E_INVALIDARG;
            }

            break;
        }
        case PROID_OPTEXTENSION:
        {
            PICSRulesOptExtension *pOptExtension;

            if(pOptExtension=((PICSRulesOptExtension *) pData))
            {
                hres=m_arrpPROptExtension.Append(pOptExtension) ? S_OK : E_OUTOFMEMORY;
                
                if (FAILED(hres))
                {
                    delete pOptExtension;
                }
            }
            else
            {
                hres=E_INVALIDARG;
            }

            break;
        }
        case PROID_REQEXTENSION:
        {
            PICSRulesReqExtension *pReqExtension;

            if(pReqExtension=((PICSRulesReqExtension *) pData))
            {
                hres=m_arrpPRReqExtension.Append(pReqExtension) ? S_OK : E_OUTOFMEMORY;
                
                if (FAILED(hres))
                {
                    delete pReqExtension;
                }
            }
            else
            {
                hres=E_INVALIDARG;
            }

            break;
        }
        case PROID_POLICY:
        {
            PICSRulesPolicy *pPolicy;
                
            if(pPolicy=((PICSRulesPolicy *) pData))
            {
                hres=m_arrpPRPolicy.Append(pPolicy) ? S_OK : E_OUTOFMEMORY;
                
                if (FAILED(hres))
                {
                    delete pPolicy;
                }
            }
            else
            {
                hres=E_INVALIDARG;
            }

            break;
        }
        case PROID_NAME:
        {
            PICSRulesName *pName;
                
            if(pName=((PICSRulesName *) pData))
            {
                m_pPRName=pName;
            }
            else
            {
                hres=E_INVALIDARG;
            }

            break;
        }
        case PROID_SOURCE:
        {
            PICSRulesSource *pSource;
                
            if(pSource=((PICSRulesSource *) pData))
            {
                m_pPRSource=pSource;
            }
            else
            {
                hres=E_INVALIDARG;
            }

            break;
        }
        case PROID_SERVICEINFO:
        {
            PICSRulesServiceInfo *pServiceInfo;
                
            if(pServiceInfo=((PICSRulesServiceInfo *) pData))
            {
                hres=m_arrpPRServiceInfo.Append(pServiceInfo) ? S_OK : E_OUTOFMEMORY;
                
                if (FAILED(hres))
                {
                    delete pServiceInfo;
                }
            }
            else
            {
                hres=E_INVALIDARG;
            }

            break;
        }
        case PROID_EXTENSION:
        {
            //just eat extensions
            break;
        }
        case PROID_INVALID:
        default:
        {
            ASSERT(FALSE);      // shouldn't have been given a PROID that wasn't in
                                // the table we passed to the parser!
            hres=E_UNEXPECTED;
            break;
        }
    }
    return hres;
}

void PICSRulesRatingSystem::ReportError(HRESULT hres)
{
    UINT    idMsg,idTemplate;
    WCHAR   szErrorMessage[MAX_PATH],szErrorTitle[MAX_PATH],
            szLoadStringTemp[MAX_PATH];
    //we may be reporting E_OUTOFMEMORY, so we'll keep our string memory
    //on the stack so that its gauranteed to be there

    if((hres==E_OUTOFMEMORY)||((hres>PICSRULES_E_BASE)&&(hres<=PICSRULES_E_BASE+0xffff)))
    {
        idTemplate=IDS_PICSRULES_SYNTAX_TEMPLATE;   //default is PICSRules content error
        switch(hres)
        {
            case E_OUTOFMEMORY:
            {
                idMsg=IDS_PICSRULES_MEMORY;
                idTemplate=IDS_PICSRULES_GENERIC_TEMPLATE;
                break;
            }
            case PICSRULES_E_EXPECTEDLEFT:
            {
                idMsg=IDS_PICSRULES_EXPECTEDLEFT;
                break;
            }
            case PICSRULES_E_EXPECTEDRIGHT:
            {
                idMsg=IDS_PICSRULES_EXPECTEDRIGHT;
                break;
            }
            case PICSRULES_E_EXPECTEDTOKEN:
            {
                idMsg=IDS_PICSRULES_EXPECTEDTOKEN;
                break;
            }
            case PICSRULES_E_EXPECTEDSTRING:
            {
                idMsg=IDS_PICSRULES_EXPECTEDSTRING;
                break;
            }
            case PICSRULES_E_EXPECTEDNUMBER:
            {
                idMsg=IDS_PICSRULES_EXPECTEDNUMBER;
                break;
            }
            case PICSRULES_E_EXPECTEDBOOL:
            {
                idMsg=IDS_PICSRULES_EXPECTEDBOOL;
                break;
            }
            case PICSRULES_E_DUPLICATEITEM:
            {
                idMsg=IDS_PICSRULES_DUPLICATEITEM;
                break;
            }
            case PICSRULES_E_MISSINGITEM:
            {
                idMsg=IDS_PICSRULES_MISSINGITEM;
                break;
            }
            case PICSRULES_E_UNKNOWNITEM:
            {
                idMsg=IDS_PICSRULES_UNKNOWNITEM;
                break;
            }
            case PICSRULES_E_UNKNOWNMANDATORY:
            {
                idMsg=IDS_PICSRULES_UNKNOWNMANDATORY;
                break;
            }
            case PICSRULES_E_SERVICEUNDEFINED:
            {
                idMsg=IDS_PICSRULES_SERVICEUNDEFINED;
                break;
            }
            case PICSRULES_E_EXPECTEDEND:
            {
                idMsg=IDS_PICSRULES_EXPECTEDEND;

                break;
            }
            case PICSRULES_E_REQEXTENSIONUSED:
            {
                idTemplate=IDS_PICSRULES_GENERIC_TEMPLATE;
                idMsg=IDS_PICSRULES_REQEXTENSIONUSED;

                break;
            }
            case PICSRULES_E_VERSION:
            {
                idTemplate=IDS_PICSRULES_GENERIC_TEMPLATE;
                idMsg=IDS_PICSRULES_BADVERSION;

                break;
            }
            default:
            {
                ASSERT(FALSE);  //there aren't any other PICSRULES_E_ errors
                idMsg=IDS_PICSRULES_UNKNOWNERROR;
                break;
            }
        }

        MLLoadString(idTemplate,(LPTSTR) szLoadStringTemp,MAX_PATH);
        wsprintf((LPTSTR) szErrorMessage,(LPTSTR) szLoadStringTemp,m_etstrFile.Get());

        MLLoadString(idMsg,(LPTSTR) szLoadStringTemp,MAX_PATH);
        wsprintf((LPTSTR) szErrorTitle,(LPTSTR) szLoadStringTemp,m_nErrLine);

        lstrcat((LPTSTR) szErrorMessage,(LPTSTR) szErrorTitle);
    }
    else
    {
        idTemplate=IDS_PICSRULES_GENERIC_TEMPLATE;

        if(HRESULT_FACILITY(hres)==FACILITY_WIN32)
        {
            switch(hres)
            {
                case E_OUTOFMEMORY:
                {
                    idMsg=IDS_PICSRULES_MEMORY;
                    break;
                }
                case E_INVALIDARG:
                {
                    idMsg=IDS_PICSRULES_INVALID;
                    break;
                }
                default:
                {
                    idMsg=IDS_PICSRULES_WINERROR;
                    break;
                }
            }
            
            MLLoadString(idTemplate,(LPTSTR) szLoadStringTemp,MAX_PATH);
            wsprintf((LPTSTR) szErrorMessage,(LPTSTR) szLoadStringTemp,m_etstrFile.Get());

            MLLoadString(idMsg,(LPTSTR) szLoadStringTemp,MAX_PATH);
            
            if(idMsg==IDS_PICSRULES_WINERROR)
            {
                wsprintf((LPTSTR) szErrorTitle,(LPTSTR) szLoadStringTemp,HRESULT_CODE(hres));
            }
            else
            {
                wsprintf((LPTSTR) szErrorTitle,(LPTSTR) szLoadStringTemp,m_nErrLine);
            }

            lstrcat((LPTSTR) szErrorMessage,(LPTSTR) szErrorTitle);
        }
        else
        {
            idMsg=IDS_PICSRULES_MISCERROR;

            MLLoadString(idTemplate,(LPTSTR) szLoadStringTemp,MAX_PATH);
            wsprintf((LPTSTR) szErrorMessage,(LPTSTR) szLoadStringTemp,m_etstrFile.Get());

            MLLoadString(idMsg,(LPTSTR) szLoadStringTemp,MAX_PATH);
            wsprintf((LPTSTR) szErrorTitle,(LPTSTR) szLoadStringTemp,HRESULT_CODE(hres));

            lstrcat((LPTSTR) szErrorMessage,(LPTSTR) szErrorTitle);
        }
    }

    MLLoadString(IDS_ERROR,(LPTSTR) szErrorTitle,MAX_PATH);
    MessageBox(NULL,(LPCTSTR) szErrorMessage,(LPCTSTR) szErrorTitle,MB_OK|MB_ICONERROR);
}

//*******************************************************************
//*
//* Code for the PICSRulesByURL class
//*
//*******************************************************************

PICSRulesByURL::PICSRulesByURL()
{
    //nothing to do
}

PICSRulesByURL::~PICSRulesByURL()
{
    m_arrpPRByURL.DeleteAll();
}

PICSRulesEvaluation PICSRulesByURL::EvaluateRule(PICSRulesQuotedURL *pprurlComparisonURL)
{
    int                         iCounter;
    URL_COMPONENTS              URLComponents;
    FN_INTERNETCRACKURL         pfnInternetCrackUrl;
    INTERNET_SCHEME             INetScheme=INTERNET_SCHEME_DEFAULT;
    INTERNET_PORT               INetPort=INTERNET_INVALID_PORT_NUMBER;
    LPSTR                       lpszScheme,lpszHostName,lpszUserName,
                                lpszPassword,lpszUrlPath,lpszExtraInfo;
    BOOL                        fApplies=FALSE;

    lpszScheme=new char[INTERNET_MAX_SCHEME_LENGTH+1];
    lpszHostName=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszUserName=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszPassword=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszUrlPath=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszExtraInfo=new char[INTERNET_MAX_PATH_LENGTH+1];

    if(lpszScheme==NULL ||
       lpszHostName==NULL ||
       lpszUserName==NULL ||
       lpszPassword==NULL ||
       lpszUrlPath==NULL ||
       lpszExtraInfo==NULL)
    {
        return(PR_EVALUATION_DOESNOTAPPLY);
    }

    URLComponents.dwStructSize=sizeof(URL_COMPONENTS);
    URLComponents.lpszScheme=lpszScheme;
    URLComponents.dwSchemeLength=INTERNET_MAX_SCHEME_LENGTH;
    URLComponents.nScheme=INetScheme;
    URLComponents.lpszHostName=lpszHostName;
    URLComponents.dwHostNameLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.nPort=INetPort;
    URLComponents.lpszUserName=lpszUserName;
    URLComponents.dwUserNameLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszPassword=lpszPassword;
    URLComponents.dwPasswordLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszUrlPath=lpszUrlPath;
    URLComponents.dwUrlPathLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszExtraInfo=lpszExtraInfo;
    URLComponents.dwExtraInfoLength=INTERNET_MAX_PATH_LENGTH;

    pfnInternetCrackUrl=(FN_INTERNETCRACKURL) GetProcAddress(g_hWININET,"InternetCrackUrlA");

    if(pfnInternetCrackUrl==NULL)
    {
        return(PR_EVALUATION_DOESNOTAPPLY);
    }

    pfnInternetCrackUrl(pprurlComparisonURL->Get(),0,ICU_DECODE,&URLComponents);

    for(iCounter=0;iCounter<m_arrpPRByURL.Length();iCounter++)
    {
        PICSRulesByURLExpression * pPRByURLExpression;

        pPRByURLExpression=m_arrpPRByURL[iCounter];

        //schemes must be specified as per the spec, so there is no need to check
        //the m_bSpecified flag against BYURL_SCHEME

        //if the scheme is non-wild then we match against exact strings only, the
        //match is case insensitive as per the spec
        if(pPRByURLExpression->m_bNonWild&BYURL_SCHEME)
        {
            if(lstrcmp(lpszScheme,pPRByURLExpression->m_etstrScheme.Get())!=0)
            {
                continue;
            }
        }

        //if the user name is omitted we only match if the url navigated to also
        //had the user name omitted
        if(!(pPRByURLExpression->m_bSpecified&BYURL_USER))
        {
            if(*lpszUserName!=NULL)
            {
                continue;
            }
        }
        else if(pPRByURLExpression->m_bNonWild&BYURL_USER)
        {
            int     iLength;
            char    * lpszCurrent,lpszCompare[INTERNET_MAX_URL_LENGTH+1],
                    lpszCopy[INTERNET_MAX_URL_LENGTH+1];
            BOOL    fFrontWild=0,fBackWild=0,fFrontEscaped=0,fBackEscaped=0;

            //if the user was specified we match a '*' at the beginning as wild, a '*'
            //at the end as wild, and '%*' matches aginst the character '*', this
            //comparison is case sensitive

            lstrcpy(lpszCompare,pPRByURLExpression->m_etstrUser.Get());
            
            iLength=lstrlen(lpszCompare);

            if(lpszCompare[0]=='*')
            {
                fFrontWild=1;
            }
            
            if(lpszCompare[iLength-1]=='*')
            {
                fBackWild=1;
                
                lpszCompare[iLength-1]='\0';
            }

            if((lpszCompare[0]=='%')&&(lpszCompare[1]=='*'))
            {
                fFrontEscaped=1;
            }

            if((lpszCompare[iLength-2]=='%')&&fBackWild)
            {
                fBackWild=0;
                fBackEscaped=1;

                lpszCompare[iLength-2]='*';
            }

            lpszCurrent=lpszCompare+fFrontWild+fFrontEscaped;

            lstrcpy(lpszCopy,lpszCurrent);

            if(fFrontWild==1)
            {
                lpszCurrent=strstrf(lpszUserName,lpszCopy);
                
                if(lpszCurrent!=NULL)
                {
                    if(fBackWild==0)
                    {
                        if(lstrcmp(lpszCurrent,lpszUserName)!=0)
                        {
                            continue;
                        }
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                if(fBackWild==1)
                {
                    lpszUserName[lstrlen(lpszCopy)]='\0';
                }

                if(lstrcmp(lpszUserName,lpszCopy)!=0)
                {
                    continue;
                }
            }
        }

        //the host (or ipwild) must always be specified, so there is no need to
        //check against m_bSpecified

        //the host is either an ipwild (i.e. #.#.#.#!#) or a URL substring.  If
        //we have an ipwild, then we have to first resolve the site being browsed
        //to to a set of IP addresses.  We consider it a match if we match any
        //of those IPs.  If the host is an URL substring, then the first character
        //being a '*' matches any number of characters, and being '%*' matches '*'
        //itself.  Everything further must match exactly.  The compare is case-
        //insensitive.
        if(pPRByURLExpression->m_bNonWild&BYURL_HOST)
        {
            BOOL        fFrontWild=0,fWasIpWild=FALSE,fNoneMatched=TRUE;
            DWORD       dwIpRules=0;
            char        * lpszCurrent;

            lpszCurrent=pPRByURLExpression->m_etstrHost.Get();

            if((lpszCurrent[0]>='0')&&(lpszCurrent[0]<='9'))
            {
                //make a copy of the string since we are going to delete the masking '!'
                //to test for an ipwild
                char * lpszMask;
                char lpszIpWild[INTERNET_MAX_PATH_LENGTH+1];
                int  iBitMask=(sizeof(DWORD)*8);
                
                lstrcpy(lpszIpWild,lpszCurrent);

                lpszMask=strchrf(lpszIpWild,'!');

                if(lpszMask!=NULL)
                {
                    *lpszMask='\0';
                    lpszMask++;

                    ParseNumber(&lpszMask,&iBitMask,TRUE);
                }

                //test for an ipwild case
                dwIpRules = inet_addr(lpszIpWild);
                if(dwIpRules != INADDR_NONE)
                {
                    //we definately have an ipwild
                    array<DWORD*>      arrpIpCompare;
                    HOSTENT            * pHostEnt;
                    int                iCounter;

                    fWasIpWild=TRUE;

                    pHostEnt=gethostbyname(lpszHostName);

                    if(pHostEnt!=NULL)
                    {
                        char *lpszHosts;

                        lpszHosts=pHostEnt->h_addr_list[0];

                        iCounter=0;

                        while(lpszHosts!=NULL)
                        {
                            DWORD *pdwIP;
    
                            pdwIP=new DWORD;

                            *pdwIP=*((DWORD *) lpszHosts);

                            arrpIpCompare.Append(pdwIP);

                            iCounter++;

                            lpszHosts=pHostEnt->h_addr_list[iCounter];
                        }
                    }

                    //we've got all the IPs to test against, so lets do it
                    for(iCounter=0;iCounter<arrpIpCompare.Length();iCounter++)
                    {
                        DWORD dwIpCompare;
                        int   iBitCounter;
                        BOOL  fMatched;
                        
                        dwIpCompare=*(arrpIpCompare[iCounter]);
                        fMatched=TRUE;

                        //compare the first iBitMask bits as per the spec
                        for(iBitCounter=0;
                            iBitCounter<iBitMask;
                            iBitCounter++)
                        {
                            int iPower;
                            DWORD dwMask=1;

                            for(iPower=0;iPower<iBitCounter;iPower++)
                            {
                                dwMask*=2;
                            }

                            if((dwIpRules&dwMask)!=(dwIpCompare&dwMask))
                            {
                                //they don't match
                                fMatched=FALSE;

                                break;
                            }
                        }

                        if(fMatched==TRUE)
                        {
                            fNoneMatched=FALSE;

                            break;
                        }
                    }
                }
            }

            if(fWasIpWild)
            {
                if(fNoneMatched)
                {
                    //if none matched, we don't apply, so continue to the next
                    //iteration of the loop
                    continue;
                }
            }
            else
            {
                if((lpszCurrent[0]=='%')&&(lpszCurrent[1]=='*'))
                {
                    lpszCurrent++;
                }
                else if (lpszCurrent[0]=='*')
                {
                    fFrontWild=1;
                    lpszCurrent++;
                }
        
                if(fFrontWild==1)
                {
                    char * lpszTest;

                    lpszTest=strstrf(lpszHostName,lpszCurrent);

                    if(lstrcmpi(lpszTest,lpszCurrent)!=0)
                    {
                        continue;
                    }
                }
                else
                {
                    if(lstrcmpi(lpszHostName,lpszCurrent)!=0)
                    {
                        continue;
                    }
                }
            }
        }
        
        //if the port is ommitted, we only match if the port was also ommitted in
        //the URL being browsed to.
        if(!(pPRByURLExpression->m_bSpecified&BYURL_PORT))
        {
            if(URLComponents.nPort!=INTERNET_INVALID_PORT_NUMBER)
            {
                char * lpszCheck;

                //URLComponents.nPort gets filled in anyway due to the scheme, so
                //check it against the string itself

                lpszCheck=strstrf(pprurlComparisonURL->Get(),lpszHostName);

                if(lpszCheck!=NULL)
                {
                    lpszCheck+=lstrlen(lpszHostName);

                    if(*lpszCheck==':')
                    {
                        continue;
                    }
                }
            }
        }
        else if(pPRByURLExpression->m_bNonWild&BYURL_PORT)
        {
            char * lpszPort,* lpszRange;

            //the port can be a single number or a range, with wild cards at both ends
            //of the range

            lpszPort=pPRByURLExpression->m_etstrPort.Get();

            lpszRange=strchrf(lpszPort,'-');
            
            if(lpszRange==NULL)
            {
                int iPort;

                //we've got a single port

                ParseNumber(&lpszPort,&iPort,TRUE);

                if(iPort!=URLComponents.nPort)
                {
                    continue;
                }
            }
            else
            {
                int iLow,iHigh;

                *lpszRange='\0';
                lpszRange++;

                if(*lpszPort=='*')
                {
                    iLow=0;
                }
                else
                {
                    ParseNumber(&lpszPort,&iLow,TRUE);
                }

                if(*lpszRange=='*')
                {
                    iHigh=INTERNET_MAX_PORT_NUMBER_VALUE;
                }
                else
                {
                    ParseNumber(&lpszRange,&iHigh,TRUE);
                }

                if((URLComponents.nPort>iHigh)||URLComponents.nPort<iLow)
                {
                    continue;
                }
            }
        }

        //if the path is ommitted, we only match if the path was also ommitted in
        //the URL being browsed to.
        if(!(pPRByURLExpression->m_bSpecified&BYURL_PATH))
        {
            if(*lpszUrlPath!=NULL)
            {
                if(!((*lpszUrlPath=='/')&&(*(lpszUrlPath+1)==NULL)))
                {
                    continue;
                }
            }
        }
        else if(pPRByURLExpression->m_bNonWild&BYURL_PATH)
        {
            int     iLength;
            char    * lpszCurrent,lpszCompare[INTERNET_MAX_URL_LENGTH+1],
                    lpszCopy[INTERNET_MAX_URL_LENGTH+1],* lpszUrlCheck,
                    * lpszPreCompare;
            BOOL    fFrontWild=0,fBackWild=0,fFrontEscaped=0,fBackEscaped=0;

            //if the path was specified we match a '*' at the beginning as wild, a '*'
            //at the end as wild, and '%*' matches aginst the character '*', this
            //comparison is case sensitive

            //kill leading slashes
            if(*lpszUrlPath=='/')
            {
                lpszUrlCheck=lpszUrlPath+1;
            }
            else
            {
                lpszUrlCheck=lpszUrlPath;
            }

            iLength=lstrlen(lpszUrlCheck);

            //kill trailing slashes
            if(lpszUrlCheck[iLength-1]=='/')
            {
                lpszUrlCheck[iLength-1]='\0';
            }

            lpszPreCompare=pPRByURLExpression->m_etstrPath.Get();

            //kill leading slashes
            if(*lpszPreCompare=='/')
            {
                lstrcpy(lpszCompare,lpszPreCompare+1);
            }
            else
            {
                lstrcpy(lpszCompare,lpszPreCompare);
            }
            
            iLength=lstrlen(lpszCompare);

            //kill trailing slashes
            if(lpszCompare[iLength-1]=='/')
            {
                lpszCompare[iLength-1]='\0';
            }

            if(lpszCompare[0]=='*')
            {
                fFrontWild=1;
            }
            
            if(lpszCompare[iLength-1]=='*')
            {
                fBackWild=1;
                
                lpszCompare[iLength-1]='\0';
            }

            if((lpszCompare[0]=='%')&&(lpszCompare[1]=='*'))
            {
                fFrontEscaped=1;
            }

            if((lpszCompare[iLength-2]=='%')&&fBackWild)
            {
                fBackWild=0;
                fBackEscaped=1;

                lpszCompare[iLength-2]='*';
            }

            lpszCurrent=lpszCompare+fFrontWild+fFrontEscaped;

            lstrcpy(lpszCopy,lpszCurrent);

            if(fFrontWild==1)
            {
                lpszCurrent=strstrf(lpszUrlCheck,lpszCopy);
                
                if(lpszCurrent!=NULL)
                {
                    if(fBackWild==0)
                    {
                        if(lstrcmp(lpszCurrent,lpszUrlCheck)!=0)
                        {
                            continue;
                        }
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                if(fBackWild==1)
                {
                    lpszUrlCheck[lstrlen(lpszCopy)]='\0';
                }

                if(lstrcmp(lpszUrlCheck,lpszCopy)!=0)
                {
                    continue;
                }
            }
        }

        //if we made it this far we do apply!
        fApplies=TRUE;
        
        break;
    }

    delete lpszScheme;
    delete lpszHostName;
    delete lpszUserName;
    delete lpszPassword;
    delete lpszUrlPath;
    delete lpszExtraInfo;
    
    if(fApplies==TRUE)
    {
        return(PR_EVALUATION_DOESAPPLY);
    }
    else
    {
        return(PR_EVALUATION_DOESNOTAPPLY);
    }
}

//*******************************************************************
//*
//* Code for the PICSRulesFileParser class
//*
//*******************************************************************

//FindNonWhite returns a pointer to the first non-whitespace
//character starting at pc.
char* PICSRulesFileParser::FindNonWhite(char *pc)
{
    ASSERT(pc);
    while (1)
    {
        if (*pc != ' ' &&
            *pc != '\t' &&
            *pc != '\r' &&
            *pc != '\n')            /* includes null terminator */
        {
            return pc;
        }
        if (*pc == '\n')

        m_nLine++;
        pc++;
    }
}

//Returns a pointer to the closing quotation mark of a quoted
//string, counting linefeeds as we go.  Returns NULL if no closing
//quotation mark is found.
//
//fQuote can be either PR_QUOTE_DOUBLE or PR_QUOTE_SINGLE
//defaults to PR_QUOTE_DOUBLE.
LPSTR PICSRulesFileParser::EatQuotedString(LPSTR pIn,BOOL fQuote)
{
    LPSTR pszQuote;

    if(fQuote==PR_QUOTE_DOUBLE)
    {
        pszQuote=strchrf(pIn,'\"');
    }
    else
    {
        pszQuote=strchrf(pIn,'\'');
    }

    if (pszQuote == NULL)
    {
        return NULL;
    }

    pIn=strchrf(pIn,'\n');
    while ((pIn!=NULL)&&(pIn<pszQuote))
    {
        m_nLine++;
        pIn=strchrf(pIn+1,'\n');
    }

    return pszQuote;
}

//ParseToOpening eats the opening '(' of a parenthesized object, and
//verifies that the token just inside it is one of the expected ones.
//If so, *ppIn is advanced past that token to the next non-whitespace
//character;  otherwise, an error is returned.
//
//For example, if *ppIn is pointing at "(PicsRule-1.1)", and
//PROID_PICSVERSION is in the allowable option table supplied, then
//NOERROR is returned and *ppIn will point at "1.1)".
//
//If the function is successful, *ppFound is set to point to the element
//in the allowable-options table which matches the type of thing this
//object actually is.
HRESULT PICSRulesFileParser::ParseToOpening(LPSTR *ppIn,
                                            PICSRulesAllowableOption *paoExpected,
                                            PICSRulesAllowableOption **ppFound)
{
    LPSTR lpszCurrent=*ppIn;

    lpszCurrent=FindNonWhite(lpszCurrent);
    
    if(*lpszCurrent=='(')
    {
        lpszCurrent=FindNonWhite(lpszCurrent+1);    //skip ( and whitespace
    }

    if((*lpszCurrent=='\"')||(*lpszCurrent=='\''))
    {
        //we found a default option section, treat it as a string and return
        //ppFound set to PROID_NAMEDEFAULT

        paoExpected->roid=PROID_NAMEDEFAULT;
        *ppFound=paoExpected;
        *ppIn=lpszCurrent;

        return NOERROR;
    }

    LPSTR lpszTokenEnd=FindTokenEnd(lpszCurrent);

    for(;paoExpected->roid!=PROID_INVALID;paoExpected++)
    {
        LPCSTR lpszThisToken=aPRObjectDescriptions[paoExpected->roid].lpszToken;

        if(paoExpected->roid==PROID_EXTENSION)
        {
            LPTSTR lpszDot;

            lpszDot=strchrf(lpszCurrent,'.');

            if(lpszDot!=NULL)
            {
                *lpszDot='\0';

                if(IsOptExtensionDefined(lpszCurrent)==TRUE)
                {
                    *lpszDot='.';

                    lpszTokenEnd=lpszCurrent;

                    break;
                }

                if(IsReqExtensionDefined(lpszCurrent)==TRUE)
                {
                    //currently no extensions are supported so we return
                    //an error on a required extension.
                    //if support for extensions is implemented
                    //this should be identical to above with a different
                    //callback for reqextensions defined.

                    return(PICSRULES_E_REQEXTENSIONUSED);
                }
            }
        }

        if(IsEqualToken(lpszCurrent,lpszTokenEnd,lpszThisToken))
        {
            break;
        }
    }

    if(paoExpected->roid!=PROID_INVALID)
    {
        *ppIn=FindNonWhite(lpszTokenEnd);       //skip token and whitespace
        *ppFound=paoExpected;
        
        return NOERROR;
    }
    else
    {
        return PICSRULES_E_UNKNOWNITEM;
    }
}

//ParseParenthesizedObjectContents is called with a text pointer pointing at
//the first non-whitespace thing following the token identifying the type of
//object.  It parses the rest of the contents of the object, up to and
//including the ')' which closes it.  The array of PICSRulesAllowableOption
//structures specifies which understood options are allowed to occur within
//this object.
HRESULT PICSRulesFileParser::ParseParenthesizedObject(LPSTR *ppIn,
                                                      PICSRulesAllowableOption aao[],
                                                      PICSRulesObjectBase *pObject)
{
    PICSRulesAllowableOption *pFound;

    HRESULT hres=S_OK;
    LPSTR pszCurrent=*ppIn;

    for(pFound=aao;pFound->roid!=PROID_INVALID;pFound++)
    {
        pFound->fdwOptions&=~AO_SEEN;
    }

    pFound=NULL;

    while((*pszCurrent!=')')&&(*pszCurrent!='\0')&&(SUCCEEDED(hres)))
    {
        hres=ParseToOpening(&pszCurrent,aao,&pFound);

        if(SUCCEEDED(hres))
        {
            LPVOID pData;

            hres=(*(aPRObjectDescriptions[pFound->roid].pHandler))(&pszCurrent,&pData,this);
            
            if(SUCCEEDED(hres))
            {
                if((pFound->fdwOptions&(AO_SINGLE|AO_SEEN))==(AO_SINGLE|AO_SEEN))
                {
                    hres=PICSRULES_E_DUPLICATEITEM;
                }
                else
                {
                    pFound->fdwOptions|=AO_SEEN;
                    
                    hres=pObject->AddItem(pFound->roid,pData);
                    
                    if(SUCCEEDED(hres))
                    {
                        pszCurrent=FindNonWhite(pszCurrent);
                    }
                }
            }
        }
    }

    if(FAILED(hres))
    {
        return hres;
    }

    for(pFound=aao;pFound->roid!=PROID_INVALID;pFound++)
    {
        if((pFound->fdwOptions&(AO_MANDATORY|AO_SEEN))==AO_MANDATORY)
        {
            return(PICSRULES_E_MISSINGITEM);        //mandatory item not found
        }
    }

    pszCurrent=FindNonWhite(pszCurrent+1);  //skip the closing parenthesis
    *ppIn=pszCurrent;

    return(hres);
}

//*******************************************************************
//*
//* Code for the PICSRulesName class
//*
//*******************************************************************

PICSRulesName::PICSRulesName()
{
    //just need to construct members
}

PICSRulesName::~PICSRulesName()
{
    //nothing to do
}

HRESULT PICSRulesName::AddItem(PICSRulesObjectID proid, LPVOID pData)
{
    HRESULT hRes = S_OK;

    switch (proid)
    {
        case PROID_NAMEDEFAULT:
        case PROID_RULENAME:
        {
            m_etstrRuleName.SetTo((char *) pData);
            
            break;
        }
        case PROID_DESCRIPTION:
        {
            m_etstrDescription.SetTo((char *) pData);

            break;
        }
        case PROID_EXTENSION:
        {
            //just eat extensions
            break;
        }
        case PROID_INVALID:
        default:
        {
            ASSERT(FALSE);      // shouldn't have been given a PROID that wasn't in
                                // the table we passed to the parser!
            hRes=E_UNEXPECTED;
            break;
        }
    }
    return hRes;
}

HRESULT PICSRulesName::InitializeMyDefaults()
{
    //no defaults to initialize
    return(NOERROR);
}

//*******************************************************************
//*
//* Code for the PICSRulesOptExtension class
//*
//*******************************************************************

PICSRulesOptExtension::PICSRulesOptExtension()
{
    //nothing to do
}

PICSRulesOptExtension::~PICSRulesOptExtension()
{
    //nothing to do
}

HRESULT PICSRulesOptExtension::AddItem(PICSRulesObjectID proid, LPVOID pData)
{
    HRESULT hRes = S_OK;

    switch (proid)
    {
        case PROID_NAMEDEFAULT:
        case PROID_EXTENSIONNAME:
        {
            m_prURLExtensionName.SetTo((char *) pData);
            
            if(m_prURLExtensionName.IsURLValid()==FALSE)
            {
                hRes=E_INVALIDARG;
            }

            break;
        }
        case PROID_SHORTNAME:
        {
            m_etstrShortName.SetTo((char *) pData);

            break;
        }
        case PROID_EXTENSION:
        {
            //just eat extensions
            break;
        }
        case PROID_INVALID:
        default:
        {
            ASSERT(FALSE);      // shouldn't have been given a PROID that wasn't in
                                // the table we passed to the parser!
            hRes=E_UNEXPECTED;
            break;
        }
    }
    return hRes;
}

HRESULT PICSRulesOptExtension::InitializeMyDefaults()
{
    //no defaults to initialize
    return(NOERROR);
}

//*******************************************************************
//*
//* Code for the PICSRulesPassFail class
//*
//*******************************************************************

PICSRulesPassFail::PICSRulesPassFail()
{
    m_fPassOrFail=PR_PASSFAIL_PASS;
}

PICSRulesPassFail::~PICSRulesPassFail()
{
    //nothing to do
}

void PICSRulesPassFail::Set(const BOOL *pIn)
{
    switch(*pIn)
    {
        case PR_PASSFAIL_PASS:
        {
            ETS::Set(szPRPass);
            m_fPassOrFail=PR_PASSFAIL_PASS;

            break;
        }
        case PR_PASSFAIL_FAIL:
        {
            ETS::Set(szPRFail);
            m_fPassOrFail=PR_PASSFAIL_FAIL;

            break;
        }
    }
}

void PICSRulesPassFail::SetTo(BOOL *pIn)
{
    Set(pIn);
}

//*******************************************************************
//*
//* Code for the PICSRulesPolicy class
//*
//*******************************************************************

PICSRulesPolicy::PICSRulesPolicy()
{
    m_PRPolicyAttribute=PR_POLICY_NONEVALID;
}

PICSRulesPolicy::~PICSRulesPolicy()
{
    switch(m_PRPolicyAttribute)
    {
        case PR_POLICY_REJECTBYURL:
        {
            if(m_pPRRejectByURL!=NULL)
            {
                delete m_pPRRejectByURL;
            }
            break;
        }
        case PR_POLICY_ACCEPTBYURL:
        {
            if(m_pPRAcceptByURL!=NULL)
            {
                delete m_pPRAcceptByURL;
            }
            break;
        }
        case PR_POLICY_REJECTIF:
        {
            if(m_pPRRejectIf!=NULL)
            {
                delete m_pPRRejectIf;
            }
            break;
        }
        case PR_POLICY_ACCEPTIF:
        {
            if(m_pPRAcceptIf!=NULL)
            {
                delete m_pPRAcceptIf;
            }
            break;
        }
        case PR_POLICY_REJECTUNLESS:
        {
            if(m_pPRRejectUnless!=NULL)
            {
                delete m_pPRRejectUnless;
            }
            break;
        }
        case PR_POLICY_ACCEPTUNLESS:
        {
            if(m_pPRAcceptUnless!=NULL)
            {
                delete m_pPRAcceptUnless;
            }
            break;
        }
        case PR_POLICY_NONEVALID:
        default:
        {
            break;
        }
    }
}

HRESULT PICSRulesPolicy::AddItem(PICSRulesObjectID proid, LPVOID pData)
{
    HRESULT hRes = S_OK;

    switch (proid)
    {
        case PROID_NAMEDEFAULT:
        case PROID_EXPLANATION:
        {
            m_etstrExplanation.SetTo((char *) pData);
            
            break;
        }
        case PROID_REJECTBYURL:
        {
            m_pPRRejectByURL=((PICSRulesByURL *) pData);
            m_PRPolicyAttribute=PR_POLICY_REJECTBYURL;

            break;
        }
        case PROID_ACCEPTBYURL:
        {
            m_pPRAcceptByURL=((PICSRulesByURL *) pData);
            m_PRPolicyAttribute=PR_POLICY_ACCEPTBYURL;

            break;
        }
        case PROID_REJECTIF:
        {
            m_pPRRejectIf=((PICSRulesPolicyExpression *) pData);
            m_PRPolicyAttribute=PR_POLICY_REJECTIF;

            break;
        }
        case PROID_ACCEPTIF:
        {
            m_pPRAcceptIf=((PICSRulesPolicyExpression *) pData);
            m_PRPolicyAttribute=PR_POLICY_ACCEPTIF;

            break;
        }
        case PROID_REJECTUNLESS:
        {
            m_pPRRejectUnless=((PICSRulesPolicyExpression *) pData);
            m_PRPolicyAttribute=PR_POLICY_REJECTUNLESS;

            break;
        }
        case PROID_ACCEPTUNLESS:
        {
            m_pPRAcceptUnless=((PICSRulesPolicyExpression *) pData);
            m_PRPolicyAttribute=PR_POLICY_ACCEPTUNLESS;

            break;
        }
        case PROID_EXTENSION:
        {
            //just eat extensions
            break;
        }
        case PROID_INVALID:
        default:
        {
            ASSERT(FALSE);      // shouldn't have been given a PROID that wasn't in
                                // the table we passed to the parser!
            hRes=E_UNEXPECTED;
            break;
        }
    }
    return hRes;
}

HRESULT PICSRulesPolicy::InitializeMyDefaults()
{
    return(NOERROR);    //no defaults to initialize
}

//*******************************************************************
//*
//* Code for the PICSRulesPolicyExpression class
//*
//*******************************************************************

PICSRulesPolicyExpression::PICSRulesPolicyExpression()
{
    m_PRPEPolicyEmbedded=       PR_POLICYEMBEDDED_NONE;
    m_PROPolicyOperator=        PR_OPERATOR_INVALID;
    m_pPRPolicyExpressionLeft=  NULL;
    m_pPRPolicyExpressionRight= NULL;
}

PICSRulesPolicyExpression::~PICSRulesPolicyExpression()
{
    if(m_PRPEPolicyEmbedded!=PR_POLICYEMBEDDED_NONE)    //do we need to delete an
                                                        //embedded PolicyExpression?
    {
        if(m_pPRPolicyExpressionLeft!=NULL)             //double check, just to make sure
        {
            delete m_pPRPolicyExpressionLeft;
        }
        if(m_pPRPolicyExpressionRight!=NULL)            //double check, just to make sure
        {
            delete m_pPRPolicyExpressionRight;
        }
    }
}

PICSRulesEvaluation PICSRulesPolicyExpression::EvaluateRule(CParsedLabelList *pParsed)
{
    PICSRulesEvaluation PREvaluationResult;

    if((pParsed==NULL)||(m_PROPolicyOperator==PR_OPERATOR_DEGENERATE))
    {
        //we can't apply if there is no label, and we
        //don't handle the degenerate case since we have
        //to pass on to the PICS handler

        return(PR_EVALUATION_DOESNOTAPPLY);
    }

    if((m_prYesNoUseEmbedded.GetYesNo()==PR_YESNO_NO)&&(g_dwDataSource==PICS_LABEL_FROM_PAGE))
    {
        return(PR_EVALUATION_DOESNOTAPPLY);
    }

    switch(m_PRPEPolicyEmbedded)
    {
        case PR_POLICYEMBEDDED_NONE:
        {
            switch(m_PROPolicyOperator)
            {
                case PR_OPERATOR_GREATEROREQUAL:
                case PR_OPERATOR_GREATER:
                case PR_OPERATOR_EQUAL:
                case PR_OPERATOR_LESSOREQUAL:
                case PR_OPERATOR_LESS:
                {
                    LPCSTR             lpszTest;
                    CParsedServiceInfo * pCParsedServiceInfo;
                    CParsedRating      * pCParsedRating;
                    
                    PREvaluationResult=PR_EVALUATION_DOESNOTAPPLY;

                    pCParsedServiceInfo=&(pParsed->m_ServiceInfo);
                    
                    do
                    {
                        lpszTest=pCParsedServiceInfo->m_pszServiceName;

                        if(lstrcmp(lpszTest,m_etstrFullServiceName.Get())==0)
                        {
                            PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                            
                            break;
                        }

                        pCParsedServiceInfo=pCParsedServiceInfo->Next();
                    } while (pCParsedServiceInfo!=NULL);

                    if(PREvaluationResult==PR_EVALUATION_DOESAPPLY)
                    {
                        int iCounter;

                        PREvaluationResult=PR_EVALUATION_DOESNOTAPPLY;

                        //we've got the service, now check for the category

                        for(iCounter=0;iCounter<pCParsedServiceInfo->aRatings.Length();iCounter++)
                        {
                            pCParsedRating=&(pCParsedServiceInfo->aRatings[iCounter]);

                            if(lstrcmp(pCParsedRating->pszTransmitName,m_etstrCategoryName.Get())==0)
                            {
                                PREvaluationResult=PR_EVALUATION_DOESAPPLY;

                                break;
                            }
                        }
                    }

                    if(PREvaluationResult==PR_EVALUATION_DOESAPPLY)
                    {
                        int iLabelValue;
                        
                        iLabelValue=pCParsedRating->nValue;

                        //now check the values
                        PREvaluationResult=PR_EVALUATION_DOESNOTAPPLY;

                        switch(m_PROPolicyOperator)
                        {
                            case PR_OPERATOR_GREATEROREQUAL:
                            {
                                if(iLabelValue>=m_etnValue.Get())
                                {
                                    PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                                }

                                break;
                            }
                            case PR_OPERATOR_GREATER:
                            {
                                if(iLabelValue>m_etnValue.Get())
                                {
                                    PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                                }

                                break;
                            }
                            case PR_OPERATOR_EQUAL:
                            {
                                if(iLabelValue==m_etnValue.Get())
                                {
                                    PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                                }

                                break;
                            }
                            case PR_OPERATOR_LESSOREQUAL:
                            {
                                if(iLabelValue<=m_etnValue.Get())
                                {
                                    PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                                }

                                break;
                            }
                            case PR_OPERATOR_LESS:
                            {
                                if(iLabelValue<m_etnValue.Get())
                                {
                                    PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                                }

                                break;
                            }
                        }
                    }

                    break;
                }
                case PR_OPERATOR_SERVICEONLY:
                {
                    LPCSTR             lpszTest;
                    CParsedServiceInfo * pCParsedServiceInfo;
                    
                    PREvaluationResult=PR_EVALUATION_DOESNOTAPPLY;

                    pCParsedServiceInfo=&(pParsed->m_ServiceInfo);
                    
                    do
                    {
                        lpszTest=pCParsedServiceInfo->m_pszServiceName;

                        if(lstrcmp(lpszTest,m_etstrFullServiceName.Get())==0)
                        {
                            PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                            
                            break;
                        }

                        pCParsedServiceInfo=pCParsedServiceInfo->Next();
                    } while (pCParsedServiceInfo!=NULL);

                    break;
                }
                case PR_OPERATOR_SERVICEANDCATEGORY:
                {
                    LPCSTR             lpszTest;
                    CParsedServiceInfo * pCParsedServiceInfo;
                    
                    PREvaluationResult=PR_EVALUATION_DOESNOTAPPLY;

                    pCParsedServiceInfo=&(pParsed->m_ServiceInfo);
                    
                    do
                    {
                        lpszTest=pCParsedServiceInfo->m_pszServiceName;

                        if(lstrcmp(lpszTest,m_etstrFullServiceName.Get())==0)
                        {
                            PREvaluationResult=PR_EVALUATION_DOESAPPLY;
                            
                            break;
                        }

                        pCParsedServiceInfo=pCParsedServiceInfo->Next();
                    } while (pCParsedServiceInfo!=NULL);

                    if(PREvaluationResult==PR_EVALUATION_DOESAPPLY)
                    {
                        int iCounter;

                        PREvaluationResult=PR_EVALUATION_DOESNOTAPPLY;

                        //we've got the service, now check for the category

                        for(iCounter=0;iCounter<pCParsedServiceInfo->aRatings.Length();iCounter++)
                        {
                            CParsedRating * pCParsedRating;

                            pCParsedRating=&(pCParsedServiceInfo->aRatings[iCounter]);

                            if(lstrcmp(pCParsedRating->pszTransmitName,m_etstrCategoryName.Get())==0)
                            {
                                PREvaluationResult=PR_EVALUATION_DOESAPPLY;

                                break;
                            }
                        }
                    }

                    break;
                }
            }

            break;
        }
        case PR_POLICYEMBEDDED_OR:
        {
            PICSRulesEvaluation PREvaluationIntermediate;

            PREvaluationIntermediate=m_pPRPolicyExpressionLeft->EvaluateRule(pParsed);

            if(PREvaluationIntermediate==PR_EVALUATION_DOESAPPLY)
            {
                PREvaluationResult=PR_EVALUATION_DOESAPPLY;

                break;
            }
            else
            {
                PREvaluationResult=m_pPRPolicyExpressionRight->EvaluateRule(pParsed);
            }
            
            break;
        }
        case PR_POLICYEMBEDDED_AND:
        {
            PICSRulesEvaluation PREvaluationIntermediate;

            PREvaluationIntermediate=m_pPRPolicyExpressionLeft->EvaluateRule(pParsed);

            PREvaluationResult=m_pPRPolicyExpressionRight->EvaluateRule(pParsed);

            if((PREvaluationIntermediate==PR_EVALUATION_DOESAPPLY)&&
               (PREvaluationResult==PR_EVALUATION_DOESAPPLY))
            {
                break;
            }
            else
            {
                PREvaluationResult=PR_EVALUATION_DOESNOTAPPLY;
            }

            break;
        }
    }

    return(PREvaluationResult);
}

//*******************************************************************
//*
//* Code for the PICSRulesQuotedDate class
//*
//*******************************************************************

PICSRulesQuotedDate::PICSRulesQuotedDate()
{
    m_dwDate=0;
}

PICSRulesQuotedDate::~PICSRulesQuotedDate()
{
    //nothing to do
}

HRESULT PICSRulesQuotedDate::Set(const char *pIn)
{
    HRESULT hRes;
    DWORD   dwDate;

    hRes=ParseTime((char *) pIn,&dwDate);

    if(FAILED(hRes))
    {
        return(E_INVALIDARG);
    }

    m_dwDate=dwDate;

    ETS::Set(pIn);

    return(S_OK);
}

HRESULT PICSRulesQuotedDate::SetTo(char *pIn)
{
    HRESULT hRes;
    DWORD   dwDate;

    hRes=ParseTime(pIn,&dwDate,TRUE);

    if(FAILED(hRes))
    {
        return(E_INVALIDARG);
    }

    m_dwDate=dwDate;

    ETS::SetTo(pIn);

    return(S_OK);
}

BOOL PICSRulesQuotedDate::IsDateValid()
{
    if(m_dwDate)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

BOOL PICSRulesQuotedDate::IsDateValid(char * lpszDate)
{
    HRESULT hRes;
    DWORD   dwDate;

    hRes=ParseTime(lpszDate,&dwDate);

    if(SUCCEEDED(hRes))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

BOOL PICSRulesQuotedDate::IsDateValid(ETS etstrDate)
{
    HRESULT hRes;
    DWORD   dwDate;
    LPTSTR  lpszDate;

    lpszDate=etstrDate.Get();

    hRes=ParseTime(lpszDate,&dwDate);

    if(SUCCEEDED(hRes))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

//*******************************************************************
//*
//* Code for the PICSRulesQuotedEmail class
//*
//*******************************************************************

PICSRulesQuotedEmail::PICSRulesQuotedEmail()
{
    //nothing to do
}

PICSRulesQuotedEmail::~PICSRulesQuotedEmail()
{
    //nothing to do
}

BOOL PICSRulesQuotedEmail::IsEmailValid()
{
    //We don't use this internally, so as far as we are concerned
    //its always valid.
    //If we ever add UI that displays this, we can defer verification
    //of the email address to the mail client by sticking a mailto://
    //in front of our string

    return(TRUE);
}

BOOL PICSRulesQuotedEmail::IsEmailValid(char * lpszEmail)
{
    //We don't use this internally, so as far as we are concerned
    //its always valid.
    //If we ever add UI that displays this, we can defer verification
    //of the email address to the mail client by sticking a mailto://
    //in front of our string

    return(TRUE);
}

BOOL PICSRulesQuotedEmail::IsEmailValid(ETS etstrEmail)
{
    //We don't use this internally, so as far as we are concerned
    //its always valid.
    //If we ever add UI that displays this, we can defer verification
    //of the email address to the mail client by sticking a mailto://
    //in front of our string

    return(TRUE);
}

//*******************************************************************
//*
//* Code for the PICSRulesQuotedURL class
//*
//*******************************************************************

PICSRulesQuotedURL::PICSRulesQuotedURL()
{
    //nothing to do
}

PICSRulesQuotedURL::~PICSRulesQuotedURL()
{
    //nothing to do
}

BOOL IsURLValid(WCHAR wcszURL[INTERNET_MAX_URL_LENGTH])
{
    FN_ISVALIDURL   pfnIsValidURL;

    pfnIsValidURL=(FN_ISVALIDURL) GetProcAddress(g_hURLMON,"IsValidURL");

    if(pfnIsValidURL==NULL)
    {
        return(FALSE);
    }

    if(pfnIsValidURL(NULL,wcszURL,0)==S_OK)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

BOOL PICSRulesQuotedURL::IsURLValid()
{
    char            * lpszURL;
    WCHAR           wcszURL[INTERNET_MAX_URL_LENGTH];

    lpszURL=Get();

    MultiByteToWideChar(CP_OEMCP,MB_PRECOMPOSED,lpszURL,-1,wcszURL,INTERNET_MAX_URL_LENGTH);
    
    return(::IsURLValid(wcszURL));
}

BOOL PICSRulesQuotedURL::IsURLValid(char * lpszURL)
{
    WCHAR           wcszURL[INTERNET_MAX_URL_LENGTH];

    MultiByteToWideChar(CP_OEMCP,MB_PRECOMPOSED,lpszURL,-1,wcszURL,INTERNET_MAX_URL_LENGTH);

    return(::IsURLValid(wcszURL));
}

BOOL PICSRulesQuotedURL::IsURLValid(ETS etstrURL)
{
    char            * lpszURL;
    WCHAR           wcszURL[INTERNET_MAX_URL_LENGTH];

    lpszURL=etstrURL.Get();

    MultiByteToWideChar(CP_OEMCP,MB_PRECOMPOSED,lpszURL,-1,wcszURL,INTERNET_MAX_URL_LENGTH);

    return(::IsURLValid(wcszURL));
}

//*******************************************************************
//*
//* Code for the PICSRulesReqExtension class
//*
//*******************************************************************

PICSRulesReqExtension::PICSRulesReqExtension()
{
    //nothing to do
}

PICSRulesReqExtension::~PICSRulesReqExtension()
{
    //nothing to do
}

HRESULT PICSRulesReqExtension::AddItem(PICSRulesObjectID proid, LPVOID pData)
{
    HRESULT hRes = S_OK;

    switch (proid)
    {
        case PROID_NAMEDEFAULT:
        case PROID_EXTENSIONNAME:
        {
            m_prURLExtensionName.SetTo((char *) pData);
            
            if(m_prURLExtensionName.IsURLValid()==FALSE)
            {
                hRes=E_INVALIDARG;
            }

            break;
        }
        case PROID_SHORTNAME:
        {
            m_etstrShortName.SetTo((char *) pData);

            break;
        }
        case PROID_EXTENSION:
        {
            //just eat extensions
            break;
        }
        case PROID_INVALID:
        default:
        {
            ASSERT(FALSE);      // shouldn't have been given a PROID that wasn't in
                                // the table we passed to the parser!
            hRes=E_UNEXPECTED;
            break;
        }
    }
    return hRes;
}

HRESULT PICSRulesReqExtension::InitializeMyDefaults()
{
    //no defaults to initialize
    return(NOERROR);
}

//*******************************************************************
//*
//* Code for the PICSRulesServiceInfo class
//*
//*******************************************************************

PICSRulesServiceInfo::PICSRulesServiceInfo()
{
    const BOOL fYes=PR_YESNO_YES;
    const BOOL fPass=PR_PASSFAIL_PASS;

    m_prPassFailBureauUnavailable.Set(&fPass);
    m_prYesNoUseEmbedded.Set(&fYes);
}

PICSRulesServiceInfo::~PICSRulesServiceInfo()
{
    //nothing to do
}

HRESULT PICSRulesServiceInfo::AddItem(PICSRulesObjectID proid, LPVOID pData)
{
    HRESULT hRes = S_OK;

    switch (proid)
    {
        case PROID_NAMEDEFAULT:
        case PROID_NAME:
        case PROID_SINAME:
        {
            m_prURLName.SetTo((char *) pData);
            
            if(m_prURLName.IsURLValid()==FALSE)
            {
                hRes=E_INVALIDARG;
            }

            break;
        }
        case PROID_SHORTNAME:
        {
            m_etstrShortName.SetTo((char *) pData);

            break;
        }
        case PROID_BUREAUURL:
        {
            m_prURLBureauURL.SetTo((char *) pData);

            if(m_prURLBureauURL.IsURLValid()==FALSE)
            {
                hRes=E_INVALIDARG;
            }

            break;
        }
        case PROID_USEEMBEDDED:
        {
            m_prYesNoUseEmbedded.SetTo((BOOL *) &pData);

            break;
        }
        case PROID_RATFILE:
        {
            m_etstrRatfile.SetTo((char *) pData);

            break;
        }
        case PROID_BUREAUUNAVAILABLE:
        {
            m_prPassFailBureauUnavailable.SetTo((BOOL *) &pData);

            break;
        }
        case PROID_EXTENSION:
        {
            //just eat extensions
            break;
        }
        case PROID_INVALID:
        default:
        {
            ASSERT(FALSE);      // shouldn't have been given a PROID that wasn't in
                                // the table we passed to the parser!
            hRes=E_UNEXPECTED;
            break;
        }
    }
    return hRes;
}

HRESULT PICSRulesServiceInfo::InitializeMyDefaults()
{
    //nothing to do
    return(S_OK);
}

//*******************************************************************
//*
//* Code for the PICSRulesSource class
//*
//*******************************************************************

PICSRulesSource::PICSRulesSource()
{
    //nothing to do but construct members
}

PICSRulesSource::~PICSRulesSource()
{
    //nothing to do
}

HRESULT PICSRulesSource::AddItem(PICSRulesObjectID proid, LPVOID pData)
{
    HRESULT hRes = S_OK;

    switch (proid)
    {
        case PROID_NAMEDEFAULT:
        case PROID_SOURCEURL:
        {
            m_prURLSourceURL.SetTo((char *) pData);
            
            if(m_prURLSourceURL.IsURLValid()==FALSE)
            {
                hRes=E_INVALIDARG;
            }

            break;
        }
        case PROID_AUTHOR:
        {
            m_prEmailAuthor.SetTo((char *) pData);

            if(m_prEmailAuthor.IsEmailValid()==FALSE)
            {
                hRes=E_INVALIDARG;
            }

            break;
        }
        case PROID_CREATIONTOOL:
        {
            m_etstrCreationTool.SetTo((char *) pData);

            break;
        }
        case PROID_LASTMODIFIED:
        {
            m_prDateLastModified.SetTo((char *) pData);

            if(m_prDateLastModified.IsDateValid()==FALSE)
            {
                hRes=E_INVALIDARG;
            }

            break;
        }
        case PROID_EXTENSION:
        {
            //just eat extensions
            break;
        }
        case PROID_INVALID:
        default:
        {
            ASSERT(FALSE);      // shouldn't have been given a PROID that wasn't in
                                // the table we passed to the parser!
            hRes=E_UNEXPECTED;
            break;
        }
    }
    return hRes;
}

HRESULT PICSRulesSource::InitializeMyDefaults()
{
    //no defaults to initialize
    return(NOERROR);
}

char * PICSRulesSource::GetToolName()
{
    return m_etstrCreationTool.Get();
}

//*******************************************************************
//*
//* Code for the PICSRulesYesNo class
//*
//*******************************************************************

PICSRulesYesNo::PICSRulesYesNo()
{
    m_fYesOrNo=PR_YESNO_YES;
}

PICSRulesYesNo::~PICSRulesYesNo()
{
}

void PICSRulesYesNo::Set(const BOOL *pIn)
{
    switch(*pIn)
    {
        case PR_YESNO_YES:
        {
            ETS::Set(szPRYes);
            m_fYesOrNo=PR_YESNO_YES;

            break;
        }
        case PR_YESNO_NO:
        {
            ETS::Set(szPRNo);
            m_fYesOrNo=PR_YESNO_NO;

            break;
        }
    }
}

void PICSRulesYesNo::SetTo(BOOL *pIn)
{
    Set(pIn);
}

//*******************************************************************
//*
//* Code for the PICSRulesByURLExpression class
//*
//*******************************************************************
PICSRulesByURLExpression::PICSRulesByURLExpression()
{
    m_bNonWild=0;
    m_bSpecified=0;
}

PICSRulesByURLExpression::~PICSRulesByURLExpression()
{
    //nothing to do
}

HRESULT EtStringRegWriteCipher(ETS &ets,HKEY hKey,char *pKeyWord)
{
    if(pKeyWord==NULL)
    {
        return(E_INVALIDARG);
    }

    if(ets.fIsInit())
    {
        return(RegSetValueEx(hKey,pKeyWord,0,REG_SZ,(LPBYTE)ets.Get(),strlenf(ets.Get())+1));
    }

    return(NOERROR);
}

HRESULT EtNumRegWriteCipher(ETN &etn,HKEY hKey,char *pKeyWord)
{
    int iTemp;

    if(pKeyWord==NULL)
    {
        return(E_INVALIDARG);
    }

    if(etn.fIsInit())
    {
        iTemp=etn.Get();

        return(RegSetValueEx(hKey,pKeyWord,0,REG_DWORD,(LPBYTE)&iTemp,sizeof(iTemp)));
    }

    return(NOERROR);
}

HRESULT EtBoolRegWriteCipher(ETB &etb,HKEY hKey,char *pKeyWord)
{
    DWORD dwNum;

    if(pKeyWord==NULL)
    {
        return(E_INVALIDARG);
    }

    if(etb.fIsInit())
    {
        dwNum=etb.Get();

        return(RegSetValueEx(hKey,pKeyWord,0,REG_DWORD,(LPBYTE)&dwNum,sizeof(dwNum)));
    }

    return(NOERROR);
}

HRESULT EtStringRegReadCipher(ETS &ets,HKEY hKey,char *pKeyWord)
{
    unsigned long lType;

    if(pKeyWord==NULL)
    {
        return(E_INVALIDARG);
    }

    char * lpszString=new char[INTERNET_MAX_URL_LENGTH + 1];
    DWORD dwSizeOfString=INTERNET_MAX_URL_LENGTH + 1;
    
    if(lpszString==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    if(RegQueryValueEx(hKey,pKeyWord,NULL,&lType,(LPBYTE) lpszString,&dwSizeOfString)!=ERROR_SUCCESS)
    {
        ets.SetTo(NULL);

        delete lpszString;

        return(E_UNEXPECTED);
    }
    else
    {
        ets.SetTo(lpszString);
    }

    return(NOERROR);
}

HRESULT EtNumRegReadCipher(ETN &etn,HKEY hKey,char *pKeyWord)
{
    unsigned long lType;

    if(pKeyWord==NULL)
    {
        return(E_INVALIDARG);
    }

    DWORD dwNum;
    DWORD dwSizeOfNum=sizeof(DWORD);
    
    if(RegQueryValueEx(hKey,pKeyWord,NULL,&lType,(LPBYTE) &dwNum,&dwSizeOfNum)!=ERROR_SUCCESS)
    {
        etn.Set(0);

        return(E_UNEXPECTED);
    }
    else
    {
        etn.Set(dwNum);
    }

    return(NOERROR);
}

HRESULT EtBoolRegReadCipher(ETB &etb,HKEY hKey,char *pKeyWord)
{
    unsigned long lType;

    if(pKeyWord==NULL)
    {
        return(E_INVALIDARG);
    }

    BOOL fFlag;
    DWORD dwSizeOfFlag=sizeof(BOOL);
    
    if(RegQueryValueEx(hKey,pKeyWord,NULL,&lType,(LPBYTE) &fFlag,&dwSizeOfFlag)!=ERROR_SUCCESS)
    {
        etb.Set(0);

        return(E_UNEXPECTED);
    }
    else
    {
        etb.Set(fFlag);
    }

    return(NOERROR);
}

void PICSRulesOutOfMemory()
{
    char szTitle[MAX_PATH],szMessage[MAX_PATH];

    MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
    MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

    MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);
}

HRESULT CopySubPolicyExpressionFromRegistry(PICSRulesPolicyExpression * pPRPolicyExpressionBeingCopied,HKEY hKeyExpression)
{
    PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy;
    HKEY                      hKeyExpressionSubKey;
    ETB                       etb;
    ETN                       etn;
    int                       iTemp;
    long                      lError;

    EtBoolRegReadCipher(etb,hKeyExpression,(char *) szPICSRULESEXPRESSIONEMBEDDED);
    iTemp=(int) etb.Get();
    pPRPolicyExpressionBeingCopied->m_prYesNoUseEmbedded.Set(&iTemp);

    EtStringRegReadCipher(pPRPolicyExpressionBeingCopied->m_etstrServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONSERVICENAME);
    EtStringRegReadCipher(pPRPolicyExpressionBeingCopied->m_etstrCategoryName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONCATEGORYNAME);
    EtStringRegReadCipher(pPRPolicyExpressionBeingCopied->m_etstrFullServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONFULLSERVICENAME);

    EtNumRegReadCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONVALUE);
    pPRPolicyExpressionBeingCopied->m_etnValue.Set(etn.Get());

    EtNumRegReadCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONPOLICYOPERATOR);
    pPRPolicyExpressionBeingCopied->m_PROPolicyOperator=(PICSRulesOperators) etn.Get();

    EtNumRegReadCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONOPPOLICYEMBEDDED);
    pPRPolicyExpressionBeingCopied->m_PRPEPolicyEmbedded=(PICSRulesPolicyEmbedded) etn.Get();

    lError=RegOpenKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONLEFT,NULL,KEY_READ,&hKeyExpressionSubKey);

    if(lError!=ERROR_SUCCESS)
    {
        pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft=NULL;
    }
    else
    {
        pPRSubPolicyExpressionToCopy=new PICSRulesPolicyExpression;

        if(pPRSubPolicyExpressionToCopy==NULL)
        {
            PICSRulesOutOfMemory();

            return(E_OUTOFMEMORY);
        }

        pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft=pPRSubPolicyExpressionToCopy;

        if(FAILED(CopySubPolicyExpressionFromRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
        {
            return(E_OUTOFMEMORY);
        }

        RegCloseKey(hKeyExpressionSubKey);
    }

    lError=RegOpenKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONRIGHT,NULL,KEY_READ,&hKeyExpressionSubKey);

    if(lError!=ERROR_SUCCESS)
    {
        pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight=NULL;
    }
    else
    {
        pPRSubPolicyExpressionToCopy=new PICSRulesPolicyExpression;

        if(pPRSubPolicyExpressionToCopy==NULL)
        {
            PICSRulesOutOfMemory();

            return(E_OUTOFMEMORY);
        }

        pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight=pPRSubPolicyExpressionToCopy;

        if(FAILED(CopySubPolicyExpressionFromRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
        {
            return(E_OUTOFMEMORY);
        }

        RegCloseKey(hKeyExpressionSubKey);
    }

    return(NOERROR);
}

HRESULT ReadSystemFromRegistry(HKEY hKey,PICSRulesRatingSystem **ppPRRS)
{
    PICSRulesRatingSystem       * pPRRSBeingCopied;
    PICSRulesPolicy             * pPRPolicyBeingCopied;
    PICSRulesPolicyExpression   * pPRPolicyExpressionBeingCopied;
    PICSRulesServiceInfo        * pPRServiceInfoBeingCopied;
    PICSRulesOptExtension       * pPROptExtensionBeingCopied;
    PICSRulesReqExtension       * pPRReqExtensionBeingCopied;
    PICSRulesName               * pPRNameBeingCopied;
    PICSRulesSource             * pPRSourceBeingCopied;
    PICSRulesByURL              * pPRByURLToCopy;
    PICSRulesByURLExpression    * pPRByURLExpressionToCopy;
    ETN                         etn;
    ETB                         etb;
    long                        lError;
    HKEY                        hKeySubKey;
    char                        szNumber[MAX_PATH];
    DWORD                       dwNumSystems,dwSubCounter,dwNumServiceInfo,dwNumExtensions;

    pPRRSBeingCopied=*ppPRRS;

    if(pPRRSBeingCopied!=NULL)
    {
        delete pPRRSBeingCopied;
    }

    pPRRSBeingCopied=new PICSRulesRatingSystem;

    if(pPRRSBeingCopied==NULL)
    {
        PICSRulesOutOfMemory();

        return(E_OUTOFMEMORY);
    }

    *ppPRRS=pPRRSBeingCopied;

    EtStringRegReadCipher(pPRRSBeingCopied->m_etstrFile,hKey,(char *) &szPICSRULESFILENAME);
    EtNumRegReadCipher(pPRRSBeingCopied->m_etnPRVerMajor,hKey,(char *) &szPICSRULESVERMAJOR);
    EtNumRegReadCipher(pPRRSBeingCopied->m_etnPRVerMinor,hKey,(char *) &szPICSRULESVERMINOR);

    EtNumRegReadCipher(etn,hKey,(char *) &szPICSRULESDWFLAGS);
    pPRRSBeingCopied->m_dwFlags=etn.Get();

    EtNumRegReadCipher(etn,hKey,(char *) &szPICSRULESERRLINE);
    pPRRSBeingCopied->m_nErrLine=etn.Get();

    //Read in the PICSRulesName Structure
    lError=RegOpenKeyEx(hKey,(char *) &szPICSRULESPRNAME,NULL,KEY_READ,&hKeySubKey);

    if(lError!=ERROR_SUCCESS)
    {
        pPRRSBeingCopied->m_pPRName=NULL;
    }
    else
    {
        pPRNameBeingCopied=new PICSRulesName;

        if(pPRNameBeingCopied==NULL)
        {
            PICSRulesOutOfMemory();

            delete pPRRSBeingCopied;

            return(E_OUTOFMEMORY);
        }
        else
        {
            pPRRSBeingCopied->m_pPRName=pPRNameBeingCopied;
        }
    }

    if((pPRRSBeingCopied->m_pPRName)!=NULL)
    {
        EtStringRegReadCipher(pPRNameBeingCopied->m_etstrRuleName,hKeySubKey,(char *) &szPICSRULESRULENAME);
        EtStringRegReadCipher(pPRNameBeingCopied->m_etstrDescription,hKeySubKey,(char *) &szPICSRULESDESCRIPTION);

        RegCloseKey(hKeySubKey);
    }

    //Read in the PICSRulesSource Structure
    lError=RegOpenKeyEx(hKey,(char *) &szPICSRULESPRSOURCE,NULL,KEY_READ,&hKeySubKey);

    if(lError!=ERROR_SUCCESS)
    {
        pPRRSBeingCopied->m_pPRSource=NULL;
    }
    else
    {
        pPRSourceBeingCopied=new PICSRulesSource;

        if(pPRSourceBeingCopied==NULL)
        {
            PICSRulesOutOfMemory();

            delete pPRRSBeingCopied;

            return(E_OUTOFMEMORY);
        }
        else
        {
            pPRRSBeingCopied->m_pPRSource=pPRSourceBeingCopied;
        }
    }

    if((pPRRSBeingCopied->m_pPRSource)!=NULL)
    {
        EtStringRegReadCipher(pPRSourceBeingCopied->m_prURLSourceURL,hKeySubKey,(char *) &szPICSRULESSOURCEURL);
        EtStringRegReadCipher(pPRSourceBeingCopied->m_etstrCreationTool,hKeySubKey,(char *) &szPICSRULESCREATIONTOOL);
        EtStringRegReadCipher(pPRSourceBeingCopied->m_prEmailAuthor,hKeySubKey,(char *) &szPICSRULESEMAILAUTHOR);
        EtStringRegReadCipher(pPRSourceBeingCopied->m_prDateLastModified,hKeySubKey,(char *) &szPICSRULESLASTMODIFIED);

        RegCloseKey(hKeySubKey);
    }

    //Read in the PICSRulesPolicy structure    
    lError=RegOpenKeyEx(hKey,(char *) &szPICSRULESPRPOLICY,NULL,KEY_READ,&hKeySubKey);

    if(lError!=ERROR_SUCCESS)
    {
        dwNumSystems=0;
    }
    else
    {
        EtNumRegReadCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMPOLICYS);

        dwNumSystems=etn.Get();
    }

    for(dwSubCounter=0;dwSubCounter<dwNumSystems;dwSubCounter++)
    {
        DWORD dwPolicyExpressionSubCounter;
        HKEY  hKeyCopy,hKeyExpression;
        wsprintf(szNumber,"%d",dwSubCounter);

        lError=RegOpenKeyEx(hKeySubKey,szNumber,NULL,KEY_READ,&hKeyCopy);

        if(lError!=ERROR_SUCCESS)
        {
            delete pPRRSBeingCopied;

            return(E_FAIL);
        }
        else
        {
            pPRPolicyBeingCopied=new PICSRulesPolicy;

            if(pPRPolicyBeingCopied==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSBeingCopied;

                return(E_OUTOFMEMORY);
            }
        }

        pPRRSBeingCopied->m_arrpPRPolicy.Append(pPRPolicyBeingCopied);

        EtStringRegReadCipher(pPRPolicyBeingCopied->m_etstrExplanation,hKeyCopy,(char *) &szPICSRULESPOLICYEXPLANATION);

        EtNumRegReadCipher(etn,hKeyCopy,(char *) &szPICSRULESPOLICYATTRIBUTE);
        pPRPolicyBeingCopied->m_PRPolicyAttribute=(PICSRulesPolicyAttribute) etn.Get();

        lError=RegOpenKeyEx(hKeyCopy,(char *) &szPICSRULESPOLICYSUB,NULL,KEY_READ,&hKeyExpression);

        if(lError!=ERROR_SUCCESS)
        {
            delete pPRRSBeingCopied;

            return(E_FAIL);
        }

        pPRByURLToCopy=NULL;
        pPRPolicyExpressionBeingCopied=NULL;

        switch(pPRPolicyBeingCopied->m_PRPolicyAttribute)
        {
            case PR_POLICY_ACCEPTBYURL:
            case PR_POLICY_REJECTBYURL:
            {
                pPRByURLToCopy=new PICSRulesByURL;

                if(pPRByURLToCopy==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSBeingCopied;

                    return(E_OUTOFMEMORY);
                }

                break;
            }
            case PR_POLICY_REJECTIF:
            case PR_POLICY_ACCEPTIF:
            case PR_POLICY_REJECTUNLESS:
            case PR_POLICY_ACCEPTUNLESS:
            {
                pPRPolicyExpressionBeingCopied=new PICSRulesPolicyExpression;

                if(pPRPolicyExpressionBeingCopied==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSBeingCopied;

                    return(E_OUTOFMEMORY);
                }

                break;
            }
        }

        switch(pPRPolicyBeingCopied->m_PRPolicyAttribute)
        {
            case PR_POLICY_ACCEPTBYURL:
            {
                pPRPolicyBeingCopied->m_pPRAcceptByURL=pPRByURLToCopy;
            
                break;
            }
            case PR_POLICY_REJECTBYURL:
            {
                pPRPolicyBeingCopied->m_pPRRejectByURL=pPRByURLToCopy;

                break;
            }
            case PR_POLICY_REJECTIF:
            {
                pPRPolicyBeingCopied->m_pPRRejectIf=pPRPolicyExpressionBeingCopied;

                break;
            }
            case PR_POLICY_ACCEPTIF:
            {
                pPRPolicyBeingCopied->m_pPRAcceptIf=pPRPolicyExpressionBeingCopied;

                break;
            }
            case PR_POLICY_REJECTUNLESS:
            {
                pPRPolicyBeingCopied->m_pPRRejectUnless=pPRPolicyExpressionBeingCopied;

                break;
            }
            case PR_POLICY_ACCEPTUNLESS:
            {
                pPRPolicyBeingCopied->m_pPRAcceptUnless=pPRPolicyExpressionBeingCopied;

                break;
            }
        }

        if(pPRByURLToCopy!=NULL)
        {
            DWORD dwNumExpressions;

            EtNumRegReadCipher(etn,hKeyExpression,(char *) &szPICSRULESNUMBYURL);
            dwNumExpressions=etn.Get();

            for(dwPolicyExpressionSubCounter=0;
                dwPolicyExpressionSubCounter<dwNumExpressions;
                dwPolicyExpressionSubCounter++)
            {
                HKEY hByURLKey;

                wsprintf(szNumber,"%d",dwPolicyExpressionSubCounter);

                lError=RegOpenKeyEx(hKeyExpression,szNumber,NULL,KEY_READ,&hByURLKey);

                if(lError!=ERROR_SUCCESS)
                {
                    delete pPRRSBeingCopied;

                    return(E_FAIL);
                }
                else
                {
                    pPRByURLExpressionToCopy=new PICSRulesByURLExpression;

                    if(pPRByURLExpressionToCopy==NULL)
                    {
                        PICSRulesOutOfMemory();

                        delete pPRRSBeingCopied;

                        return(E_FAIL);
                    }
                }

                pPRByURLToCopy->m_arrpPRByURL.Append(pPRByURLExpressionToCopy);

                EtBoolRegReadCipher(etb,hByURLKey,(char *) szPICSRULESBYURLINTERNETPATTERN);
                pPRByURLExpressionToCopy->m_fInternetPattern=etb.Get();

                EtNumRegReadCipher(etn,hByURLKey,(char *) szPICSRULESBYURLNONWILD);
                pPRByURLExpressionToCopy->m_bNonWild = (unsigned char) etn.Get();

                EtNumRegReadCipher(etn,hByURLKey,(char *) szPICSRULESBYURLSPECIFIED);
                pPRByURLExpressionToCopy->m_bSpecified = (unsigned char) etn.Get();

                EtStringRegReadCipher(pPRByURLExpressionToCopy->m_etstrScheme,hByURLKey,(char *) &szPICSRULESBYURLSCHEME);
                EtStringRegReadCipher(pPRByURLExpressionToCopy->m_etstrUser,hByURLKey,(char *) &szPICSRULESBYURLUSER);
                EtStringRegReadCipher(pPRByURLExpressionToCopy->m_etstrHost,hByURLKey,(char *) &szPICSRULESBYURLHOST);
                EtStringRegReadCipher(pPRByURLExpressionToCopy->m_etstrPort,hByURLKey,(char *) &szPICSRULESBYURLPORT);
                EtStringRegReadCipher(pPRByURLExpressionToCopy->m_etstrPath,hByURLKey,(char *) &szPICSRULESBYURLPATH);
                EtStringRegReadCipher(pPRByURLExpressionToCopy->m_etstrURL,hByURLKey,(char *) &szPICSRULESBYURLURL);

                RegCloseKey(hByURLKey);
            }
        }

        if(pPRPolicyExpressionBeingCopied!=NULL)
        {
            PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy;
            HKEY                      hKeyExpressionSubKey;
            int                       iTemp;

            EtBoolRegReadCipher(etb,hKeyExpression,(char *) szPICSRULESEXPRESSIONEMBEDDED);
            iTemp=(int) etb.Get();
            pPRPolicyExpressionBeingCopied->m_prYesNoUseEmbedded.Set(&iTemp);

            EtStringRegReadCipher(pPRPolicyExpressionBeingCopied->m_etstrServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONSERVICENAME);
            EtStringRegReadCipher(pPRPolicyExpressionBeingCopied->m_etstrCategoryName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONCATEGORYNAME);
            EtStringRegReadCipher(pPRPolicyExpressionBeingCopied->m_etstrFullServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONFULLSERVICENAME);

            EtNumRegReadCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONVALUE);
            pPRPolicyExpressionBeingCopied->m_etnValue.Set(etn.Get());

            EtNumRegReadCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONPOLICYOPERATOR);
            pPRPolicyExpressionBeingCopied->m_PROPolicyOperator=(PICSRulesOperators) etn.Get();

            EtNumRegReadCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONOPPOLICYEMBEDDED);
            pPRPolicyExpressionBeingCopied->m_PRPEPolicyEmbedded=(PICSRulesPolicyEmbedded) etn.Get();

            lError=RegOpenKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONLEFT,NULL,KEY_READ,&hKeyExpressionSubKey);

            if(lError!=ERROR_SUCCESS)
            {
                pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft=NULL;
            }
            else
            {
                pPRSubPolicyExpressionToCopy=new PICSRulesPolicyExpression;

                if(pPRSubPolicyExpressionToCopy==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSBeingCopied;

                    return(E_OUTOFMEMORY);
                }

                pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft=pPRSubPolicyExpressionToCopy;

                if(FAILED(CopySubPolicyExpressionFromRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
                {
                    delete pPRRSBeingCopied;

                    return(E_OUTOFMEMORY);
                }

                RegCloseKey(hKeyExpressionSubKey);
            }

            lError=RegOpenKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONRIGHT,NULL,KEY_READ,&hKeyExpressionSubKey);

            if(lError!=ERROR_SUCCESS)
            {
                pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight=NULL;
            }
            else
            {
                pPRSubPolicyExpressionToCopy=new PICSRulesPolicyExpression;

                if(pPRSubPolicyExpressionToCopy==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSBeingCopied;

                    return(E_OUTOFMEMORY);
                }

                pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight=pPRSubPolicyExpressionToCopy;

                if(FAILED(CopySubPolicyExpressionFromRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
                {
                    delete pPRRSBeingCopied;

                    return(E_OUTOFMEMORY);
                }

                RegCloseKey(hKeyExpressionSubKey);
            }
        }
    
        RegCloseKey(hKeyExpression);
        RegCloseKey(hKeyCopy);
    }

    if(dwNumSystems!=0)
    {
        RegCloseKey(hKeySubKey);
    }

    //Read In PICSRulesServiceInfo Structure
    lError=RegOpenKeyEx(hKey,(char *) &szPICSRULESSERVICEINFO,NULL,KEY_READ,&hKeySubKey);

    if(lError!=ERROR_SUCCESS)
    {
        dwNumServiceInfo=0;
    }
    else
    {
        EtNumRegReadCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMSERVICEINFO);
        dwNumServiceInfo=etn.Get();
    }

    for(dwSubCounter=0;dwSubCounter<dwNumServiceInfo;dwSubCounter++)
    {
        HKEY hKeyCopy;
        int  iTemp;

        wsprintf(szNumber,"%d",dwSubCounter);

        lError=RegOpenKeyEx(hKeySubKey,szNumber,NULL,KEY_READ,&hKeyCopy);

        if(lError!=ERROR_SUCCESS)
        {
            delete pPRRSBeingCopied;

            return(E_FAIL);
        }
        else
        {
            pPRServiceInfoBeingCopied=new PICSRulesServiceInfo;

            if(pPRServiceInfoBeingCopied==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSBeingCopied;

                return(E_OUTOFMEMORY);
            }
        }

        pPRRSBeingCopied->m_arrpPRServiceInfo.Append(pPRServiceInfoBeingCopied);

        EtStringRegReadCipher(pPRServiceInfoBeingCopied->m_prURLName,hKeyCopy,(char *) &szPICSRULESSIURLNAME);
        EtStringRegReadCipher(pPRServiceInfoBeingCopied->m_prURLBureauURL,hKeyCopy,(char *) &szPICSRULESSIBUREAUURL);
        EtStringRegReadCipher(pPRServiceInfoBeingCopied->m_etstrShortName,hKeyCopy,(char *) &szPICSRULESSISHORTNAME);
        EtStringRegReadCipher(pPRServiceInfoBeingCopied->m_etstrRatfile,hKeyCopy,(char *) &szPICSRULESSIRATFILE);

        EtBoolRegReadCipher(etb,hKeyCopy,(char *) &szPICSRULESSIUSEEMBEDDED);
        iTemp=(int) etb.Get();
        pPRServiceInfoBeingCopied->m_prYesNoUseEmbedded.Set(&iTemp);

        EtBoolRegWriteCipher(etb,hKeyCopy,(char *) &szPICSRULESSIBUREAUUNAVAILABLE);
        iTemp=(int) etb.Get();
        pPRServiceInfoBeingCopied->m_prPassFailBureauUnavailable.Set(&iTemp);

        RegCloseKey(hKeyCopy);
        RegCloseKey(hKeySubKey);
    }

    //Read in OptExtension Structures
    lError=RegOpenKeyEx(hKey,(char *) &szPICSRULESOPTEXTENSION,NULL,KEY_READ,&hKeySubKey);

    if(lError!=ERROR_SUCCESS)
    {
        dwNumExtensions=0;
    }
    else
    {
        EtNumRegWriteCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMOPTEXTENSIONS);
        dwNumExtensions=etn.Get();
    }

    for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPROptExtension.Length());dwSubCounter++)
    {
        HKEY hKeyCopy;

        wsprintf(szNumber,"%d",dwSubCounter);

        lError=RegOpenKeyEx(hKeySubKey,szNumber,NULL,KEY_READ,&hKeyCopy);

        if(lError!=ERROR_SUCCESS)
        {
            delete pPRRSBeingCopied;

            return(E_FAIL);
        }
        else
        {
            pPROptExtensionBeingCopied=new PICSRulesOptExtension;

            if(pPROptExtensionBeingCopied==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSBeingCopied;

                return(E_OUTOFMEMORY);
            }
        }

        pPRRSBeingCopied->m_arrpPROptExtension.Append(pPROptExtensionBeingCopied);

        EtStringRegReadCipher(pPROptExtensionBeingCopied->m_prURLExtensionName,hKeyCopy,(char *) &szPICSRULESOPTEXTNAME);
        EtStringRegReadCipher(pPROptExtensionBeingCopied->m_etstrShortName,hKeyCopy,(char *) &szPICSRULESOPTEXTSHORTNAME);

        RegCloseKey(hKeyCopy);
    }

    if(dwNumExtensions!=0)
    {
        RegCloseKey(hKeySubKey);
    }

    //Read in ReqExtension Structures
    lError=RegOpenKeyEx(hKey,(char *) &szPICSRULESREQEXTENSION,NULL,KEY_READ,&hKeySubKey);

    if(lError!=ERROR_SUCCESS)
    {
        dwNumExtensions=0;
    }
    else
    {
        EtNumRegWriteCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMREQEXTENSIONS);
        dwNumExtensions=etn.Get();
    }

    for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRReqExtension.Length());dwSubCounter++)
    {
        HKEY hKeyCopy;

        wsprintf(szNumber,"%d",dwSubCounter);

        lError=RegOpenKeyEx(hKeySubKey,szNumber,NULL,KEY_READ,&hKeyCopy);

        if(lError!=ERROR_SUCCESS)
        {
            delete pPRRSBeingCopied;

            return(E_FAIL);
        }
        else
        {
            pPRReqExtensionBeingCopied=new PICSRulesReqExtension;

            if(pPRReqExtensionBeingCopied==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSBeingCopied;

                return(E_OUTOFMEMORY);
            }
        }

        pPRRSBeingCopied->m_arrpPRReqExtension.Append(pPRReqExtensionBeingCopied);

        EtStringRegReadCipher(pPRReqExtensionBeingCopied->m_prURLExtensionName,hKeyCopy,(char *) &szPICSRULESREQEXTNAME);
        EtStringRegReadCipher(pPRReqExtensionBeingCopied->m_etstrShortName,hKeyCopy,(char *) &szPICSRULESREQEXTSHORTNAME);

        RegCloseKey(hKeyCopy);
    }

    if(dwNumExtensions!=0)
    {
        RegCloseKey(hKeySubKey);
    }

    return(NOERROR);
}

HRESULT CopySubPolicyExpressionToRegistry(PICSRulesPolicyExpression * pPRPolicyExpressionBeingCopied,HKEY hKeyExpression)
{
    ETB  etb;
    ETN  etn;
    long lError;

    etb.Set(pPRPolicyExpressionBeingCopied->m_prYesNoUseEmbedded.GetYesNo());
    EtBoolRegWriteCipher(etb,hKeyExpression,(char *) szPICSRULESEXPRESSIONEMBEDDED);

    EtStringRegWriteCipher(pPRPolicyExpressionBeingCopied->m_etstrServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONSERVICENAME);
    EtStringRegWriteCipher(pPRPolicyExpressionBeingCopied->m_etstrCategoryName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONCATEGORYNAME);
    EtStringRegWriteCipher(pPRPolicyExpressionBeingCopied->m_etstrFullServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONFULLSERVICENAME);

    etn.Set(pPRPolicyExpressionBeingCopied->m_etnValue.Get());
    EtNumRegWriteCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONVALUE);

    etn.Set(pPRPolicyExpressionBeingCopied->m_PROPolicyOperator);
    EtNumRegWriteCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONPOLICYOPERATOR);

    etn.Set(pPRPolicyExpressionBeingCopied->m_PRPEPolicyEmbedded);
    EtNumRegWriteCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONOPPOLICYEMBEDDED);

    if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft!=NULL)
    {
        PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy;
        HKEY                      hKeyExpressionSubKey;

        lError=RegCreateKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONLEFT,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeyExpressionSubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft;

        if(FAILED(CopySubPolicyExpressionToRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
        {
            return(E_FAIL);
        }

        RegCloseKey(hKeyExpressionSubKey);
    }

    if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight!=NULL)
    {
        PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy;
        HKEY                      hKeyExpressionSubKey;

        lError=RegCreateKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONRIGHT,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeyExpressionSubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight;

        if(FAILED(CopySubPolicyExpressionToRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
        {
            return(E_FAIL);
        }

        RegCloseKey(hKeyExpressionSubKey);
    }

    return(NOERROR);
}

HRESULT WriteSystemToRegistry(HKEY hKey,PICSRulesRatingSystem **ppPRRS)
{
    PICSRulesRatingSystem       * pPRRSBeingCopied;
    PICSRulesPolicy             * pPRPolicyBeingCopied;
    PICSRulesPolicyExpression   * pPRPolicyExpressionBeingCopied;
    PICSRulesServiceInfo        * pPRServiceInfoBeingCopied;
    PICSRulesOptExtension       * pPROptExtensionBeingCopied;
    PICSRulesReqExtension       * pPRReqExtensionBeingCopied;
    PICSRulesName               * pPRNameBeingCopied;
    PICSRulesSource             * pPRSourceBeingCopied;
    PICSRulesByURL              * pPRByURLToCopy;
    PICSRulesByURLExpression    * pPRByURLExpressionToCopy;
    ETN                         etn;
    ETB                         etb;
    long                        lError;
    HKEY                        hKeySubKey;
    char                        szNumber[MAX_PATH];

    pPRRSBeingCopied=*ppPRRS;

    if(pPRRSBeingCopied==NULL)
    {
        return(E_INVALIDARG);
    }

    EtStringRegWriteCipher(pPRRSBeingCopied->m_etstrFile,hKey,(char *) &szPICSRULESFILENAME);
    EtNumRegWriteCipher(pPRRSBeingCopied->m_etnPRVerMajor,hKey,(char *) &szPICSRULESVERMAJOR);
    EtNumRegWriteCipher(pPRRSBeingCopied->m_etnPRVerMinor,hKey,(char *) &szPICSRULESVERMINOR);

    etn.Set(pPRRSBeingCopied->m_dwFlags);
    EtNumRegWriteCipher(etn,hKey,(char *) &szPICSRULESDWFLAGS);

    etn.Set(pPRRSBeingCopied->m_nErrLine);
    EtNumRegWriteCipher(etn,hKey,(char *) &szPICSRULESERRLINE);

    if((pPRRSBeingCopied->m_pPRName)!=NULL)
    {
        lError=RegCreateKeyEx(hKey,(char *) &szPICSRULESPRNAME,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        pPRNameBeingCopied=pPRRSBeingCopied->m_pPRName;

        EtStringRegWriteCipher(pPRNameBeingCopied->m_etstrRuleName,hKey,(char *) &szPICSRULESSYSTEMNAME);
        EtStringRegWriteCipher(pPRNameBeingCopied->m_etstrRuleName,hKeySubKey,(char *) &szPICSRULESRULENAME);
        EtStringRegWriteCipher(pPRNameBeingCopied->m_etstrDescription,hKeySubKey,(char *) &szPICSRULESDESCRIPTION);

        RegCloseKey(hKeySubKey);
    }

    if((pPRRSBeingCopied->m_pPRSource)!=NULL)
    {
        lError=RegCreateKeyEx(hKey,(char *) &szPICSRULESPRSOURCE,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        pPRSourceBeingCopied=pPRRSBeingCopied->m_pPRSource;

        EtStringRegWriteCipher(pPRSourceBeingCopied->m_prURLSourceURL,hKeySubKey,(char *) &szPICSRULESSOURCEURL);
        EtStringRegWriteCipher(pPRSourceBeingCopied->m_etstrCreationTool,hKeySubKey,(char *) &szPICSRULESCREATIONTOOL);
        EtStringRegWriteCipher(pPRSourceBeingCopied->m_prEmailAuthor,hKeySubKey,(char *) &szPICSRULESEMAILAUTHOR);
        EtStringRegWriteCipher(pPRSourceBeingCopied->m_prDateLastModified,hKeySubKey,(char *) &szPICSRULESLASTMODIFIED);

        RegCloseKey(hKeySubKey);
    }

    if(pPRRSBeingCopied->m_arrpPRPolicy.Length()>0)
    {
        DWORD dwSubCounter;

        lError=RegCreateKeyEx(hKey,(char *) &szPICSRULESPRPOLICY,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        etn.Set(pPRRSBeingCopied->m_arrpPRPolicy.Length());
        EtNumRegWriteCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMPOLICYS);

        for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRPolicy.Length());dwSubCounter++)
        {
            DWORD dwPolicyExpressionSubCounter;
            HKEY  hKeyCopy,hKeyExpression;

            wsprintf(szNumber,"%d",dwSubCounter);

            lError=RegCreateKeyEx(hKeySubKey,szNumber,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyCopy,NULL);

            if(lError!=ERROR_SUCCESS)
            {
                return(E_FAIL);
            }

            pPRPolicyBeingCopied=pPRRSBeingCopied->m_arrpPRPolicy[dwSubCounter];

            EtStringRegWriteCipher(pPRPolicyBeingCopied->m_etstrExplanation,hKeyCopy,(char *) &szPICSRULESPOLICYEXPLANATION);

            etn.Set(pPRPolicyBeingCopied->m_PRPolicyAttribute);
            EtNumRegWriteCipher(etn,hKeyCopy,(char *) &szPICSRULESPOLICYATTRIBUTE);

            lError=RegCreateKeyEx(hKeyCopy,(char *) &szPICSRULESPOLICYSUB,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyExpression,NULL);

            if(lError!=ERROR_SUCCESS)
            {
                return(E_FAIL);
            }

            pPRByURLToCopy=NULL;
            pPRPolicyExpressionBeingCopied=NULL;

            switch(pPRPolicyBeingCopied->m_PRPolicyAttribute)
            {
                case PR_POLICY_ACCEPTBYURL:
                {
                    pPRByURLToCopy=pPRPolicyBeingCopied->m_pPRAcceptByURL;
                
                    break;
                }
                case PR_POLICY_REJECTBYURL:
                {
                    pPRByURLToCopy=pPRPolicyBeingCopied->m_pPRRejectByURL;

                    break;
                }
                case PR_POLICY_REJECTIF:
                {
                    pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRRejectIf;

                    break;
                }
                case PR_POLICY_ACCEPTIF:
                {
                    pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRAcceptIf;

                    break;
                }
                case PR_POLICY_REJECTUNLESS:
                {
                    pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRRejectUnless;

                    break;
                }
                case PR_POLICY_ACCEPTUNLESS:
                {
                    pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRAcceptUnless;

                    break;
                }
            }

            if(pPRByURLToCopy!=NULL)
            {
                etn.Set(pPRByURLToCopy->m_arrpPRByURL.Length());
                EtNumRegWriteCipher(etn,hKeyExpression,(char *) &szPICSRULESNUMBYURL);

                for(dwPolicyExpressionSubCounter=0;
                    dwPolicyExpressionSubCounter<(DWORD) (pPRByURLToCopy->m_arrpPRByURL.Length());
                    dwPolicyExpressionSubCounter++)
                {
                    HKEY hByURLKey;

                    wsprintf(szNumber,"%d",dwPolicyExpressionSubCounter);

                    lError=RegCreateKeyEx(hKeyExpression,szNumber,NULL,NULL,
                                          REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                          NULL,&hByURLKey,NULL);

                    if(lError!=ERROR_SUCCESS)
                    {
                        return(E_FAIL);
                    }

                    pPRByURLExpressionToCopy=pPRByURLToCopy->m_arrpPRByURL[dwPolicyExpressionSubCounter];

                    etb.Set(pPRByURLExpressionToCopy->m_fInternetPattern);
                    EtBoolRegWriteCipher(etb,hByURLKey,(char *) szPICSRULESBYURLINTERNETPATTERN);

                    etn.Set(pPRByURLExpressionToCopy->m_bNonWild);
                    EtNumRegWriteCipher(etn,hByURLKey,(char *) szPICSRULESBYURLNONWILD);

                    etn.Set(pPRByURLExpressionToCopy->m_bSpecified);
                    EtNumRegWriteCipher(etn,hByURLKey,(char *) szPICSRULESBYURLSPECIFIED);

                    EtStringRegWriteCipher(pPRByURLExpressionToCopy->m_etstrScheme,hByURLKey,(char *) &szPICSRULESBYURLSCHEME);
                    EtStringRegWriteCipher(pPRByURLExpressionToCopy->m_etstrUser,hByURLKey,(char *) &szPICSRULESBYURLUSER);
                    EtStringRegWriteCipher(pPRByURLExpressionToCopy->m_etstrHost,hByURLKey,(char *) &szPICSRULESBYURLHOST);
                    EtStringRegWriteCipher(pPRByURLExpressionToCopy->m_etstrPort,hByURLKey,(char *) &szPICSRULESBYURLPORT);
                    EtStringRegWriteCipher(pPRByURLExpressionToCopy->m_etstrPath,hByURLKey,(char *) &szPICSRULESBYURLPATH);
                    EtStringRegWriteCipher(pPRByURLExpressionToCopy->m_etstrURL,hByURLKey,(char *) &szPICSRULESBYURLURL);

                    RegCloseKey(hByURLKey);
                }
            }

            if(pPRPolicyExpressionBeingCopied!=NULL)
            {
                etb.Set(pPRPolicyExpressionBeingCopied->m_prYesNoUseEmbedded.GetYesNo());
                EtBoolRegWriteCipher(etb,hKeyExpression,(char *) szPICSRULESEXPRESSIONEMBEDDED);

                EtStringRegWriteCipher(pPRPolicyExpressionBeingCopied->m_etstrServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONSERVICENAME);
                EtStringRegWriteCipher(pPRPolicyExpressionBeingCopied->m_etstrCategoryName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONCATEGORYNAME);
                EtStringRegWriteCipher(pPRPolicyExpressionBeingCopied->m_etstrFullServiceName,hKeyExpression,(char *) &szPICSRULESEXPRESSIONFULLSERVICENAME);

                etn.Set(pPRPolicyExpressionBeingCopied->m_etnValue.Get());
                EtNumRegWriteCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONVALUE);

                etn.Set(pPRPolicyExpressionBeingCopied->m_PROPolicyOperator);
                EtNumRegWriteCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONPOLICYOPERATOR);

                etn.Set(pPRPolicyExpressionBeingCopied->m_PRPEPolicyEmbedded);
                EtNumRegWriteCipher(etn,hKeyExpression,(char *) &szPICSRULESEXPRESSIONOPPOLICYEMBEDDED);

                if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft!=NULL)
                {
                    PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy;
                    HKEY                      hKeyExpressionSubKey;

                    lError=RegCreateKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONLEFT,NULL,NULL,
                                          REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                          NULL,&hKeyExpressionSubKey,NULL);

                    if(lError!=ERROR_SUCCESS)
                    {
                        return(E_FAIL);
                    }

                    pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft;

                    if(FAILED(CopySubPolicyExpressionToRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
                    {
                        return(E_FAIL);
                    }

                    RegCloseKey(hKeyExpressionSubKey);
                }

                if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight!=NULL)
                {
                    PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy;
                    HKEY                      hKeyExpressionSubKey;

                    lError=RegCreateKeyEx(hKeyExpression,(char *) &szPICSRULESEXPRESSIONRIGHT,NULL,NULL,
                                          REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                          NULL,&hKeyExpressionSubKey,NULL);

                    if(lError!=ERROR_SUCCESS)
                    {
                        return(E_FAIL);
                    }

                    pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight;

                    if(FAILED(CopySubPolicyExpressionToRegistry(pPRSubPolicyExpressionToCopy,hKeyExpressionSubKey)))
                    {
                        return(E_FAIL);
                    }

                    RegCloseKey(hKeyExpressionSubKey);
                }
            }
        
            RegCloseKey(hKeyExpression);
            RegCloseKey(hKeyCopy);
        }

        RegCloseKey(hKeySubKey);
    }

    if(pPRRSBeingCopied->m_arrpPRServiceInfo.Length()>0)
    {
        DWORD dwSubCounter;

        lError=RegCreateKeyEx(hKey,(char *) &szPICSRULESSERVICEINFO,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        etn.Set(pPRRSBeingCopied->m_arrpPRServiceInfo.Length());
        EtNumRegWriteCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMSERVICEINFO);

        for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRServiceInfo.Length());dwSubCounter++)
        {
            HKEY hKeyCopy;

            wsprintf(szNumber,"%d",dwSubCounter);

            lError=RegCreateKeyEx(hKeySubKey,szNumber,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyCopy,NULL);

            if(lError!=ERROR_SUCCESS)
            {
                return(E_FAIL);
            }

            pPRServiceInfoBeingCopied=pPRRSBeingCopied->m_arrpPRServiceInfo[dwSubCounter];

            EtStringRegWriteCipher(pPRServiceInfoBeingCopied->m_prURLName,hKeyCopy,(char *) &szPICSRULESSIURLNAME);
            EtStringRegWriteCipher(pPRServiceInfoBeingCopied->m_prURLBureauURL,hKeyCopy,(char *) &szPICSRULESSIBUREAUURL);
            EtStringRegWriteCipher(pPRServiceInfoBeingCopied->m_etstrShortName,hKeyCopy,(char *) &szPICSRULESSISHORTNAME);
            EtStringRegWriteCipher(pPRServiceInfoBeingCopied->m_etstrRatfile,hKeyCopy,(char *) &szPICSRULESSIRATFILE);

            etb.Set(pPRServiceInfoBeingCopied->m_prYesNoUseEmbedded.GetYesNo());
            EtBoolRegWriteCipher(etb,hKeyCopy,(char *) &szPICSRULESSIUSEEMBEDDED);

            etb.Set(pPRServiceInfoBeingCopied->m_prPassFailBureauUnavailable.GetPassFail());
            EtBoolRegWriteCipher(etb,hKeyCopy,(char *) &szPICSRULESSIBUREAUUNAVAILABLE);

            RegCloseKey(hKeyCopy);
        }

        RegCloseKey(hKeySubKey);
    }

    if(pPRRSBeingCopied->m_arrpPROptExtension.Length()>0)
    {
        DWORD dwSubCounter;

        lError=RegCreateKeyEx(hKey,(char *) &szPICSRULESOPTEXTENSION,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        etn.Set(pPRRSBeingCopied->m_arrpPROptExtension.Length());
        EtNumRegWriteCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMOPTEXTENSIONS);

        for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPROptExtension.Length());dwSubCounter++)
        {
            HKEY hKeyCopy;

            wsprintf(szNumber,"%d",dwSubCounter);

            lError=RegCreateKeyEx(hKeySubKey,szNumber,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyCopy,NULL);

            if(lError!=ERROR_SUCCESS)
            {
                return(E_FAIL);
            }

            pPROptExtensionBeingCopied=pPRRSBeingCopied->m_arrpPROptExtension[dwSubCounter];

            EtStringRegWriteCipher(pPROptExtensionBeingCopied->m_prURLExtensionName,hKeyCopy,(char *) &szPICSRULESOPTEXTNAME);
            EtStringRegWriteCipher(pPROptExtensionBeingCopied->m_etstrShortName,hKeyCopy,(char *) &szPICSRULESOPTEXTSHORTNAME);

            RegCloseKey(hKeyCopy);
        }

        RegCloseKey(hKeySubKey);
    }

    if(pPRRSBeingCopied->m_arrpPRReqExtension.Length()>0)
    {
        DWORD dwSubCounter;

        lError=RegCreateKeyEx(hKey,(char *) &szPICSRULESREQEXTENSION,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySubKey,NULL);

        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        etn.Set(pPRRSBeingCopied->m_arrpPRReqExtension.Length());
        EtNumRegWriteCipher(etn,hKeySubKey,(char *) &szPICSRULESNUMREQEXTENSIONS);

        for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRReqExtension.Length());dwSubCounter++)
        {
            HKEY hKeyCopy;

            wsprintf(szNumber,"%d",dwSubCounter);

            lError=RegCreateKeyEx(hKeySubKey,szNumber,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyCopy,NULL);

            if(lError!=ERROR_SUCCESS)
            {
                return(E_FAIL);
            }

            pPRReqExtensionBeingCopied=pPRRSBeingCopied->m_arrpPRReqExtension[dwSubCounter];

            EtStringRegWriteCipher(pPRReqExtensionBeingCopied->m_prURLExtensionName,hKeyCopy,(char *) &szPICSRULESREQEXTNAME);
            EtStringRegWriteCipher(pPRReqExtensionBeingCopied->m_etstrShortName,hKeyCopy,(char *) &szPICSRULESREQEXTSHORTNAME);

            RegCloseKey(hKeyCopy);
        }

        RegCloseKey(hKeySubKey);
    }

    return(NOERROR);
}

//*******************************************************************
//*
//* Code for saving and reading processed PICSRules from the registry
//*
//*******************************************************************
HRESULT WritePICSRulesToRegistry(LPCTSTR lpszUserName,HKEY hkeyUser,DWORD dwSystemToSave,PICSRulesRatingSystem **ppPRRS)
{
    HKEY    hKeyWrite=NULL,hKeyNumbered;
    long    lError;
    char    *lpszSystemNumber;
    HRESULT hRes;

    lpszSystemNumber=(char *) GlobalAlloc(GPTR,MAX_PATH);

    if(lpszSystemNumber==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    lError=RegCreateKeyEx(hkeyUser,lpszUserName,NULL,NULL,
                          REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                          NULL,&hKeyWrite,NULL);
    
    if(lError!=ERROR_SUCCESS)
    {
        return(E_FAIL);
    }

    wsprintf(lpszSystemNumber,"%d",dwSystemToSave);

    lError=RegCreateKeyEx(hKeyWrite,lpszSystemNumber,NULL,NULL,
                          REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                          NULL,&hKeyNumbered,NULL);
    
    if(lError!=ERROR_SUCCESS)
    {
        RegCloseKey(hKeyWrite);

        return(E_FAIL);
    }

    hRes=WriteSystemToRegistry(hKeyNumbered,ppPRRS);

    RegCloseKey(hKeyWrite);
    RegCloseKey(hKeyNumbered);
    
    GlobalFree(lpszSystemNumber);

    return(hRes);
}

HRESULT PICSRulesSaveToRegistry(DWORD dwSystemToSave,PICSRulesRatingSystem **ppPRRS)
{
    HKEY hkeyUser=NULL;
    HRESULT hRes;
    HKEY hKeyHive = NULL;

    if(!(gPRSI->fSettingsValid)||!(gPRSI->fRatingInstalled))
    {
        return(E_INVALIDARG); //there isn't a valid ratings system to save to
    }

    //load the hive file
    if(gPRSI->fStoreInRegistry)
    {
        RegCreateKey(HKEY_LOCAL_MACHINE,szPICSRULESSYSTEMS,&hkeyUser);
    }
    else
    {
        hKeyHive=OpenHiveFile(TRUE);

        if (hKeyHive != NULL)
            RegCreateKey(hKeyHive,szPICSRULESSYSTEMS,&hkeyUser);
    }
    
    //write information to the registry
    if(hkeyUser!=NULL)
    {
        LPCTSTR lpszUsername; 
        int     iCounter=0;

#ifdef NASH
        for (iCounter=0;iCounter<gPRSI->arrpPU.Length();++iCounter)
        {
            lpszUsername=gPRSI->arrpPU[iCounter]->nlsUsername.QueryPch();

            hRes=WritePICSRulesToRegistry(lpszUsername,hkeyUser,dwSystemToSave,ppPRRS);

            if(FAILED(hRes))
            {
                RegCloseKey(hkeyUser);
                
                return(hRes);
            }
        }
#else
        lpszUsername=gPRSI->pUserObject->nlsUsername.QueryPch();

        hRes=WritePICSRulesToRegistry(lpszUsername,hkeyUser,dwSystemToSave,ppPRRS);

        if(FAILED(hRes))
        {
            RegCloseKey(hkeyUser);
            
            return(hRes);
        }
#endif


        RegCloseKey(hkeyUser);
    }

    if(!gPRSI->fStoreInRegistry)
    {
        if (hKeyHive != NULL)
        {
            RegFlushKey(hKeyHive);
            RegCloseKey(hKeyHive);
            RegFlushKey(HKEY_LOCAL_MACHINE);
        }

        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
    }

    return(hRes);
}

HRESULT PICSRulesReadNameFromRegistry(DWORD dwSystemToRead, LPTSTR *ppszSystemName)
{
    long            lError;
    unsigned long   lType;
    char            * lpszSystem;
    DWORD           dwSizeOfSystemName=MAX_PATH;
    HKEY            hkeyUser=NULL,hKeyWrite=NULL,hKeySystem=NULL;

    if(!(gPRSI->fSettingsValid)||!(gPRSI->fRatingInstalled))
    {
        return(E_INVALIDARG); //there isn't a valid ratings system to read from
    }

    //load the hive file
    if(gPRSI->fStoreInRegistry)
    {
        RegCreateKey(HKEY_LOCAL_MACHINE,szPICSRULESSYSTEMS,&hkeyUser);
    }
    else
    {
        HKEY hKeyHive;

        hKeyHive=OpenHiveFile(TRUE);

        RegCreateKey(hKeyHive,szPICSRULESSYSTEMS,&hkeyUser);
    }
    
    //read information from the registry
    if(hkeyUser!=NULL)
    {
        LPCTSTR lpszUsername; 
        int     iCounter=0;

#ifdef NASH
        for (iCounter=0;iCounter<gPRSI->arrpPU.Length();++iCounter)
        {
            lpszUsername=gPRSI->arrpPU[iCounter]->nlsUsername.QueryPch();

            lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyWrite,NULL);
            
            if(lError!=ERROR_SUCCESS)
            {
                RegCloseKey(hkeyUser);

                return(E_FAIL);
            }

            lpszSystem=(char *) GlobalAlloc(GPTR,MAX_PATH);

            wsprintf(lpszSystem,"%d",dwSystemToRead);

            lError=RegCreateKeyEx(hKeyWrite,lpszSystem,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeySystem,NULL);
            
            GlobalFree(lpszSystem);

            if(lError!=ERROR_SUCCESS)
            {
                RegCloseKey(hkeyUser);
                RegCloseKey(hKeyWrite);

                return(E_FAIL);
            }

            if(RegQueryValueEx(hKeySystem,(char *) &szPICSRULESSYSTEMNAME,NULL,&lType,(LPBYTE) *ppszSystemName,&dwSizeOfSystemName)!=ERROR_SUCCESS)
            {
                RegCloseKey(hKeySystem);
                RegCloseKey(hkeyUser);
                RegCloseKey(hKeyWrite);

                return(E_FAIL);
            }

            RegCloseKey(hKeySystem);
            RegCloseKey(hKeyWrite);
        }
#else
        lpszUsername=gPRSI->pUserObject->nlsUsername.QueryPch();

        lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeyWrite,NULL);
        
        if(lError!=ERROR_SUCCESS)
        {
            RegCloseKey(hkeyUser);

            return(E_FAIL);
        }

        lpszSystem=(char *) GlobalAlloc(GPTR,MAX_PATH);

        wsprintf(lpszSystem,"%d",dwSystemToRead);

        lError=RegCreateKeyEx(hKeyWrite,lpszSystem,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySystem,NULL);
        
        GlobalFree(lpszSystem);

        if(lError!=ERROR_SUCCESS)
        {
            RegCloseKey(hkeyUser);
            RegCloseKey(hKeyWrite);

            return(E_FAIL);
        }

        if(RegQueryValueEx(hKeySystem,(char *) &szPICSRULESSYSTEMNAME,NULL,&lType,(LPBYTE) *ppszSystemName,&dwSizeOfSystemName)!=ERROR_SUCCESS)
        {
            RegCloseKey(hKeySystem);
            RegCloseKey(hkeyUser);
            RegCloseKey(hKeyWrite);

            return(E_FAIL);
        }

        RegCloseKey(hKeySystem);
        RegCloseKey(hKeyWrite);
#endif

        RegCloseKey(hkeyUser);
    }

    if(!gPRSI->fStoreInRegistry)
    {
        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
    }

    return(NOERROR);
}

HRESULT PICSRulesReadFromRegistry(DWORD dwSystemToRead, PICSRulesRatingSystem **ppPRRS)
{
    long            lError;
    char            * lpszSystem;
    DWORD           dwSizeOfSystemName=MAX_PATH; //we're reading a file system object
    HKEY            hkeyUser=NULL,hKeyWrite=NULL,hKeySystem=NULL;

    if(!(gPRSI->fSettingsValid)||!(gPRSI->fRatingInstalled))
    {
        return(E_INVALIDARG); //there isn't a valid ratings system to read from
    }

    //load the hive file
    if(gPRSI->fStoreInRegistry)
    {
        RegCreateKey(HKEY_LOCAL_MACHINE,szPICSRULESSYSTEMS,&hkeyUser);
    }
    else
    {
        HKEY hKeyHive;

        hKeyHive=OpenHiveFile(TRUE);

        RegCreateKey(hKeyHive,szPICSRULESSYSTEMS,&hkeyUser);
    }
    
    //read information from the registry
    if(hkeyUser!=NULL)
    {
        LPCTSTR lpszUsername; 
        int     iCounter=0;

#ifdef NASH
        for (iCounter=0;iCounter<gPRSI->arrpPU.Length();++iCounter)
        {
            lpszUsername=gPRSI->arrpPU[iCounter]->nlsUsername.QueryPch();

            lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyWrite,NULL);
            
            if(lError!=ERROR_SUCCESS)
            {
                RegCloseKey(hkeyUser);

                return(E_FAIL);
            }

            lpszSystem=(char *) GlobalAlloc(GPTR,MAX_PATH);

            wsprintf(lpszSystem,"%d",dwSystemToRead);

            lError=RegCreateKeyEx(hKeyWrite,lpszSystem,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeySystem,NULL);
            
            GlobalFree(lpszSystem);

            if(lError!=ERROR_SUCCESS)
            {
                RegCloseKey(hkeyUser);
                RegCloseKey(hKeyWrite);

                return(E_FAIL);
            }

            if(FAILED(ReadSystemFromRegistry(hKeySystem,ppPRRS)))
            {
                RegCloseKey(hKeySystem);
                RegCloseKey(hkeyUser);
                RegCloseKey(hKeyWrite);

                return(E_FAIL);
            }

            RegCloseKey(hKeySystem);
            RegCloseKey(hKeyWrite);
        }
#else
        lpszUsername=gPRSI->pUserObject->nlsUsername.QueryPch();

        lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeyWrite,NULL);
        
        if(lError!=ERROR_SUCCESS)
        {
            RegCloseKey(hkeyUser);

            return(E_FAIL);
        }

        lpszSystem=(char *) GlobalAlloc(GPTR,MAX_PATH);

        wsprintf(lpszSystem,"%d",dwSystemToRead);

        lError=RegCreateKeyEx(hKeyWrite,lpszSystem,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeySystem,NULL);
        
        GlobalFree(lpszSystem);

        if(lError!=ERROR_SUCCESS)
        {
            RegCloseKey(hkeyUser);
            RegCloseKey(hKeyWrite);

            return(E_FAIL);
        }

        if(FAILED(ReadSystemFromRegistry(hKeySystem,ppPRRS)))
        {
            RegCloseKey(hKeySystem);
            RegCloseKey(hkeyUser);
            RegCloseKey(hKeyWrite);

            return(E_FAIL);
        }

        RegCloseKey(hKeySystem);
        RegCloseKey(hKeyWrite);
#endif

        RegCloseKey(hkeyUser);
    }

    if(!gPRSI->fStoreInRegistry)
    {
        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
    }
   
    return(S_OK);
}

HRESULT PICSRulesDeleteSystem(DWORD dwSystemToDelete)
{
    long            lError;
    char            * lpszSystem;
    DWORD           dwSizeOfSystemName=MAX_PATH;
    HKEY            hkeyUser=NULL,hKeyWrite=NULL;
    HKEY hKeyHive=NULL;

    if(!(gPRSI->fSettingsValid)||!(gPRSI->fRatingInstalled))
    {
        return(E_INVALIDARG); //there isn't a valid ratings system to read from
    }

    //load the hive file
    if(gPRSI->fStoreInRegistry)
    {
        RegCreateKey(HKEY_LOCAL_MACHINE,szPICSRULESSYSTEMS,&hkeyUser);
    }
    else
    {

        hKeyHive=OpenHiveFile(TRUE);

        if (hKeyHive != NULL)
            RegCreateKey(hKeyHive,szPICSRULESSYSTEMS,&hkeyUser);
    }
    
    //delete information from the registry
    if(hkeyUser!=NULL)
    {
        LPCTSTR lpszUsername; 
        int     iCounter=0;

#ifdef NASH
        for (iCounter=0;iCounter<gPRSI->arrpPU.Length();++iCounter)
        {
            lpszUsername=gPRSI->arrpPU[iCounter]->nlsUsername.QueryPch();

            lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyWrite,NULL);
            
            if(lError!=ERROR_SUCCESS)
            {
                RegCloseKey(hkeyUser);

                return(E_FAIL);
            }

            lpszSystem=(char *) GlobalAlloc(GPTR,MAX_PATH);

            wsprintf(lpszSystem,"%d",dwSystemToDelete);

            MyRegDeleteKey(hKeyWrite,lpszSystem);

            GlobalFree(lpszSystem);

            RegCloseKey(hKeyWrite);
        }
#else
        lpszUsername=gPRSI->pUserObject->nlsUsername.QueryPch();

        lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeyWrite,NULL);
        
        if(lError!=ERROR_SUCCESS)
        {
            RegCloseKey(hkeyUser);

            return(E_FAIL);
        }

        lpszSystem=(char *) GlobalAlloc(GPTR,MAX_PATH);

        wsprintf(lpszSystem,"%d",dwSystemToDelete);

        MyRegDeleteKey(hKeyWrite,lpszSystem);

        GlobalFree(lpszSystem);

        RegCloseKey(hKeyWrite);
#endif

        RegCloseKey(hkeyUser);
    }

    if(!gPRSI->fStoreInRegistry)
    {
        if (hKeyHive != NULL)
        {
            RegFlushKey(hKeyHive);
            RegCloseKey(hKeyHive);
        }

        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
    }

    return(NOERROR);
}

HRESULT PICSRulesGetNumSystems(DWORD * pdwNumSystems)
{
    long            lError;
    unsigned long   lType;
    DWORD           dwSizeOfNumSystems=sizeof(pdwNumSystems);
    HKEY            hkeyUser=NULL,hKeyWrite=NULL;

    if (pdwNumSystems)
        *pdwNumSystems = 0;

    if(!(gPRSI->fSettingsValid)||!(gPRSI->fRatingInstalled))
    {
        return(E_INVALIDARG); //there isn't a valid ratings system to read from
    }

    //load the hive file
    if(gPRSI->fStoreInRegistry)
    {
        RegCreateKey(HKEY_LOCAL_MACHINE,szPICSRULESSYSTEMS,&hkeyUser);
    }
    else
    {
        HKEY hKeyHive;

        hKeyHive=OpenHiveFile(TRUE);

        RegCreateKey(hKeyHive,szPICSRULESSYSTEMS,&hkeyUser);
    }
    
    //read information from the registry
    if(hkeyUser!=NULL)
    {
        LPCTSTR lpszUsername; 
        int     iCounter=0;

#ifdef NASH
        for (iCounter=0;iCounter<gPRSI->arrpPU.Length();++iCounter)
        {
            lpszUsername=gPRSI->arrpPU[iCounter]->nlsUsername.QueryPch();

            lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyWrite,NULL);
            
            if(lError!=ERROR_SUCCESS)
            {
                return(E_FAIL);
            }

            if(RegQueryValueEx(hKeyWrite,(char *) &szPICSRULESNUMSYS,NULL,&lType,(LPBYTE) pdwNumSystems,&dwSizeOfNumSystems)!=ERROR_SUCCESS)
            {
                //no value set, so we have zero systems installed

                *pdwNumSystems=0;
            }

            RegCloseKey(hKeyWrite);
        }
#else
        lpszUsername=gPRSI->pUserObject->nlsUsername.QueryPch();

        lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeyWrite,NULL);
        
        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        if(RegQueryValueEx(hKeyWrite,(char *) &szPICSRULESNUMSYS,NULL,&lType,(LPBYTE) pdwNumSystems,&dwSizeOfNumSystems)!=ERROR_SUCCESS)
        {
            //no value set, so we have zero systems installed

            *pdwNumSystems=0;
        }

        RegCloseKey(hKeyWrite);
#endif

        RegCloseKey(hkeyUser);
    }

    if(!gPRSI->fStoreInRegistry)
    {
        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
    }

    return(NOERROR);
}

HRESULT PICSRulesSetNumSystems(DWORD dwNumSystems)
{
    long        lError;
    HKEY        hkeyUser=NULL,hKeyWrite=NULL;
    HKEY hKeyHive=NULL;

    if(!(gPRSI->fSettingsValid)||!(gPRSI->fRatingInstalled))
    {
        return(E_INVALIDARG); //there isn't a valid ratings system to save to
    }

    //load the hive file
    if(gPRSI->fStoreInRegistry)
    {
        RegCreateKey(HKEY_LOCAL_MACHINE,szPICSRULESSYSTEMS,&hkeyUser);
    }
    else
    {

        hKeyHive=OpenHiveFile(TRUE);

        if (hKeyHive != NULL)
            RegCreateKey(hKeyHive,szPICSRULESSYSTEMS,&hkeyUser);
    }
    
    //write information to the registry
    if(hkeyUser!=NULL)
    {
        LPCTSTR lpszUsername; 
        int     iCounter=0;

#ifdef NASH
        for (iCounter=0;iCounter<gPRSI->arrpPU.Length();++iCounter)
        {
            lpszUsername=gPRSI->arrpPU[iCounter]->nlsUsername.QueryPch();

            lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                                  NULL,&hKeyWrite,NULL);
            
            if(lError!=ERROR_SUCCESS)
            {
                return(E_FAIL);
            }

            if(RegSetValueEx(hKeyWrite,(char *) &szPICSRULESNUMSYS,0,REG_DWORD,(LPBYTE) &dwNumSystems,sizeof(dwNumSystems))!=ERROR_SUCCESS)
            {
                RegCloseKey(hKeyWrite);
                RegCloseKey(hkeyUser);

                return(E_FAIL);
            }

            RegCloseKey(hKeyWrite);
        }
#else
        lpszUsername=gPRSI->pUserObject->nlsUsername.QueryPch();

        lError=RegCreateKeyEx(hkeyUser,lpszUsername,NULL,NULL,
                              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
                              NULL,&hKeyWrite,NULL);
        
        if(lError!=ERROR_SUCCESS)
        {
            return(E_FAIL);
        }

        if(RegSetValueEx(hKeyWrite,(char *) &szPICSRULESNUMSYS,0,REG_DWORD,(LPBYTE) &dwNumSystems,sizeof(dwNumSystems))!=ERROR_SUCCESS)
        {
            RegCloseKey(hKeyWrite);
            RegCloseKey(hkeyUser);

            return(E_FAIL);
        }

        RegCloseKey(hKeyWrite);
#endif

        RegCloseKey(hkeyUser);
    }

    if(!gPRSI->fStoreInRegistry)
    {
        if (hKeyHive != NULL)
        {
            RegFlushKey(hKeyHive);
            RegCloseKey(hKeyHive);
        }

        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
    }

    return(NOERROR);
}

HRESULT PICSRulesCheckApprovedSitesAccess(LPCSTR lpszUrl,BOOL *fPassFail)
{
    int                     iCounter;
    PICSRulesEvaluation     PREvaluation = PR_EVALUATION_DOESNOTAPPLY;

    if(g_pApprovedPRRS==NULL)
    {
        return(E_FAIL);
    }

    if(g_lApprovedSitesGlobalCounterValue!=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter))
    {
        PICSRulesRatingSystem * pPRRS=NULL;
        HRESULT               hRes;

        hRes=PICSRulesReadFromRegistry(PICSRULES_APPROVEDSITES,&pPRRS);

        if(SUCCEEDED(hRes))
        {
            if(g_pApprovedPRRS!=NULL)
            {
                delete g_pApprovedPRRS;
            }

            g_pApprovedPRRS=pPRRS;
        }

        g_lApprovedSitesGlobalCounterValue=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter);
    }

    for(iCounter=0;iCounter<g_pApprovedPRRS->m_arrpPRPolicy.Length();iCounter++)
    {
        PICSRulesPolicy     * pPRPolicy;
        PICSRulesByURL      * pPRByURL;
        PICSRulesQuotedURL  PRQuotedURL;

        pPRPolicy=g_pApprovedPRRS->m_arrpPRPolicy[iCounter];

        if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
        {
            *fPassFail=PR_PASSFAIL_PASS;

            pPRByURL=pPRPolicy->m_pPRAcceptByURL;
        }
        else
        {
            *fPassFail=PR_PASSFAIL_FAIL;

            pPRByURL=pPRPolicy->m_pPRRejectByURL;
        }

        PRQuotedURL.Set(lpszUrl);

        PREvaluation=pPRByURL->EvaluateRule(&PRQuotedURL);

        if(PREvaluation!=PR_EVALUATION_DOESNOTAPPLY)
        {
            break;
        }
    }
    
    if(PREvaluation==PR_EVALUATION_DOESAPPLY)
    {
        return(S_OK);
    }
    else
    {
        return(E_FAIL);
    }
}

HRESULT PICSRulesCheckAccess(LPCSTR lpszUrl,LPCSTR lpszRatingInfo,BOOL *fPassFail,CParsedLabelList **ppParsed)
{
    int                     iCounter,iSystemCounter;
    PICSRulesEvaluation     PREvaluation;
    CParsedLabelList        *pParsed=NULL;
    
    if(g_arrpPRRS.Length()==0)
    {
        return(E_FAIL);
    }

    if(g_lGlobalCounterValue!=SHGlobalCounterGetValue(g_HandleGlobalCounter))
    {
        HRESULT                 hRes;
        DWORD                   dwNumSystems;
        PICSRulesRatingSystem   * pPRRS=NULL;

        g_arrpPRRS.DeleteAll();

        //someone modified our settings, so we'd better reload them.
        hRes=PICSRulesGetNumSystems(&dwNumSystems);

        if(SUCCEEDED(hRes))
        {
            DWORD dwCounter;

            for(dwCounter=PICSRULES_FIRSTSYSTEMINDEX;
                dwCounter<(dwNumSystems+PICSRULES_FIRSTSYSTEMINDEX);
                dwCounter++)
            {
                hRes=PICSRulesReadFromRegistry(dwCounter,&pPRRS);

                if(FAILED(hRes))
                {
                    char    *lpszTitle,*lpszMessage;

                    //we couldn't read in the systems, so don't inforce PICSRules,
                    //and notify the user
        
                    g_arrpPRRS.DeleteAll();

                    lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                    lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

                    MLLoadString(IDS_PICSRULES_TAMPEREDREADTITLE,(LPTSTR) lpszTitle,MAX_PATH);
                    MLLoadString(IDS_PICSRULES_TAMPEREDREADMSG,(LPTSTR) lpszMessage,MAX_PATH);

                    MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

                    GlobalFree(lpszTitle);
                    GlobalFree(lpszMessage);

                    break;
                }
                else
                {
                    g_arrpPRRS.Append(pPRRS);

                    pPRRS=NULL;
                }
            }
        }

        g_lGlobalCounterValue=SHGlobalCounterGetValue(g_HandleGlobalCounter);
    }

    if(lpszRatingInfo!=NULL)
    {
        ParseLabelList(lpszRatingInfo,ppParsed);
        pParsed=*ppParsed;
    }

    for(iSystemCounter=0;iSystemCounter<g_arrpPRRS.Length();iSystemCounter++)
    {
        PICSRulesRatingSystem * pPRRSCheck;

        pPRRSCheck=g_arrpPRRS[iSystemCounter];

        for(iCounter=0;iCounter<pPRRSCheck->m_arrpPRPolicy.Length();iCounter++)
        {
            PICSRulesPolicy           * pPRPolicy;
            PICSRulesPolicyExpression * pPRPolicyExpression;
            PICSRulesByURL            * pPRByURL;
            PICSRulesQuotedURL        PRQuotedURL;

            pPRPolicy=pPRRSCheck->m_arrpPRPolicy[iCounter];

            switch(pPRPolicy->m_PRPolicyAttribute)
            {
                case PR_POLICY_ACCEPTBYURL:
                {
                    *fPassFail=PR_PASSFAIL_PASS;

                    pPRByURL=pPRPolicy->m_pPRAcceptByURL;

                    PRQuotedURL.Set(lpszUrl);

                    PREvaluation=pPRByURL->EvaluateRule(&PRQuotedURL);

                    break;
                }
                case PR_POLICY_REJECTBYURL:
                {
                    *fPassFail=PR_PASSFAIL_FAIL;

                    pPRByURL=pPRPolicy->m_pPRRejectByURL;

                    PRQuotedURL.Set(lpszUrl);

                    PREvaluation=pPRByURL->EvaluateRule(&PRQuotedURL);

                    break;
                }
                case PR_POLICY_REJECTIF:
                {
                    *fPassFail=PR_PASSFAIL_FAIL;

                    pPRPolicyExpression=pPRPolicy->m_pPRRejectIf;
                    
                    PREvaluation=pPRPolicyExpression->EvaluateRule(pParsed);

                    break;
                }
                case PR_POLICY_ACCEPTIF:
                {
                    *fPassFail=PR_PASSFAIL_PASS;

                    pPRPolicyExpression=pPRPolicy->m_pPRAcceptIf;
                    
                    PREvaluation=pPRPolicyExpression->EvaluateRule(pParsed);

                    break;
                }
                case PR_POLICY_REJECTUNLESS:
                {
                    *fPassFail=PR_PASSFAIL_FAIL;

                    pPRPolicyExpression=pPRPolicy->m_pPRRejectUnless;
                    
                    PREvaluation=pPRPolicyExpression->EvaluateRule(pParsed);

                    if(PREvaluation==PR_EVALUATION_DOESNOTAPPLY)
                    {
                        PREvaluation=PR_EVALUATION_DOESAPPLY;
                    }
                    else
                    {
                        PREvaluation=PR_EVALUATION_DOESNOTAPPLY;
                    }

                    break;
                }
                case PR_POLICY_ACCEPTUNLESS:
                {
                    *fPassFail=PR_PASSFAIL_PASS;

                    pPRPolicyExpression=pPRPolicy->m_pPRAcceptUnless;
                    
                    PREvaluation=pPRPolicyExpression->EvaluateRule(pParsed);

                    if(PREvaluation==PR_EVALUATION_DOESNOTAPPLY)
                    {
                        PREvaluation=PR_EVALUATION_DOESAPPLY;
                    }
                    else
                    {
                        PREvaluation=PR_EVALUATION_DOESNOTAPPLY;
                    }

                    break;
                }
                case PR_POLICY_NONEVALID:
                default:
                {
                    PREvaluation=PR_EVALUATION_DOESNOTAPPLY;

                    continue;
                }
            }

            if(PREvaluation!=PR_EVALUATION_DOESNOTAPPLY)
            {
                break;
            }
        }

        if(PREvaluation!=PR_EVALUATION_DOESNOTAPPLY)
        {
            break;
        }
    }
    
    if(PREvaluation==PR_EVALUATION_DOESAPPLY)
    {
        return(S_OK);
    }
    else
    {
        return(E_FAIL);
    }
}

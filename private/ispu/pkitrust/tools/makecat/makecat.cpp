//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       makecat.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  wmain
//
//  History:    05-May-1997 pberkman   created
//
//--------------------------------------------------------------------------


#include    <stdio.h>
#include    <windows.h>
#include    <io.h>
#include    <wchar.h>

#include    "unicode.h"
#include    "wincrypt.h"
#include    "wintrust.h"
#include    "mssip.h"
#include    "mscat.h"
#include    "dbgdef.h"

#include    "gendefs.h"
#include    "printfu.hxx"
#include    "cwargv.hxx"

#include    "resource.h"


BOOL        fVerbose        = FALSE;
BOOL        fFailAllErrors  = FALSE;
BOOL        fParseError     = FALSE;
BOOL        fTesting        = FALSE;
DWORD       dwExpectedError = 0;

WCHAR       *pwszCDFFile    = NULL;
PrintfU_    *pPrint         = NULL;

int         iRet            = 0;

void WINAPI DisplayParseError(DWORD dwErrorArea, DWORD dwLocalError, WCHAR *wszName);

extern "C" CRYPTCATATTRIBUTE * WINAPI CryptCATCDFEnumAttributesWithCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszMemberTag, CRYPTCATMEMBER *pMember,
                                             CRYPTCATATTRIBUTE *pPrevAttr,
                                             PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError);

extern "C" LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTagEx(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember, BOOL fContinueOnError,
                                       LPVOID pvReserved);

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    int                 cMember;
    cWArgv_             *pArgs;
    CRYPTCATCDF         *pCDF;
    CRYPTCATMEMBER      *pMember;
    LPWSTR              pwszMemberTag;
    CRYPTCATATTRIBUTE   *pAttr;
    BOOL                fContinueOnError;

    pCDF = NULL;

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,         IDS_PARAMTEXT_HELP,       WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,      IDS_PARAMTEXT_VERBOSE,    WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_FAILALWAYS,   IDS_PARAMTEXT_FAILALWAYS, WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_EXPERROR,     IDS_PARAMTEXT_EXPERROR,   WARGV_VALUETYPE_DWORDH, NULL, TRUE);
    pArgs->Add2List(IDS_PARAM_NOSTOPONERROR,      IDS_PARAMTEXT_NOSTOPONERROR,    WARGV_VALUETYPE_BOOL, (void *)FALSE);

    pArgs->Fill(argc, wargv);

    if (!(pArgs->Fill(argc, wargv)) ||
        (pArgs->GetValue(IDS_PARAM_HELP)))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto NeededHelp;
    }

    fVerbose        = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));
    fFailAllErrors  = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_FAILALWAYS));
    fContinueOnError = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_NOSTOPONERROR));

    if (pArgs->IsSet(IDS_PARAM_EXPERROR))
    {
        dwExpectedError = (DWORD)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_EXPERROR));
        fTesting        = TRUE;
    }

    if (!(pwszCDFFile = pArgs->GetFileName()))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto ParamError;
    }

    pPrint = new PrintfU_;

    if (pPrint == NULL)
    {
        goto MemoryError;
    }

    SetLastError(0);

    if (!(pCDF = CryptCATCDFOpen(pwszCDFFile, DisplayParseError)))
    {
        if (fVerbose)
        {
            pPrint->Display(IDS_ERROR_FUNCTION, L"CryptCATCDFOpen", GetLastError());
        }

        goto CDFOpenError;
    }

    if (fVerbose)
    {
        pPrint->Display(IDS_STATUS_FMT, pPrint->get_String(IDS_STATUS_OPENED), pwszCDFFile);
    }

    pAttr   = NULL;

    while (pAttr = CryptCATCDFEnumCatAttributes(pCDF, pAttr, DisplayParseError))
    {
        if (fVerbose)
        {
            pPrint->Display(IDS_STATUS_FMT, pPrint->get_String(IDS_STATUS_ATTR),
                            pAttr->pwszReferenceTag);
        }
    }

    pMember = NULL;
    pwszMemberTag = NULL;
    cMember = 0;

    while (pwszMemberTag = CryptCATCDFEnumMembersByCDFTagEx(pCDF, pwszMemberTag, DisplayParseError, &pMember, fContinueOnError, NULL))
    {
        if (fVerbose)
        {
            pPrint->Display(IDS_STATUS_FMT, pPrint->get_String(IDS_STATUS_PROCESSED), pwszMemberTag);
        }
        cMember++;

        pAttr = NULL;
        while (pAttr = CryptCATCDFEnumAttributesWithCDFTag(pCDF, pwszMemberTag, pMember, pAttr, DisplayParseError))
        {
            if (fVerbose)
            {
                pPrint->Display(IDS_STATUS_FMT, pPrint->get_String(IDS_STATUS_ATTR),
                                pAttr->pwszReferenceTag);
            }
        }
    }

    if ((fVerbose) && (cMember == 0))
    {
        pPrint->Display(IDS_ERROR_FUNCTION, pPrint->get_String(IDS_ERROR_NOMEMBERS), GetLastError());
    }

    if (!(CryptCATCDFClose(pCDF)))
    {
        if (fVerbose)
        {
            pPrint->Display(IDS_ERROR_FUNCTION, L"CryptCATCDFClose", GetLastError());
        }

        if (fFailAllErrors)
        {
            goto CATCloseError;
        }
    }

    if (fTesting)
    {
        pPrint->Display(IDS_FILEREF, pwszCDFFile);

        if (GetLastError() != dwExpectedError)
        {
            iRet = 1;
            pPrint->Display(IDS_EXPECTED_HRESULT, dwExpectedError, GetLastError());
        }
        else
        {
            iRet = 0;
            pPrint->Display(IDS_STATUS_FMT, pPrint->get_String(IDS_SUCCEEDED), L"");
        }
    }
    else if ((cMember > 0) && (!(fParseError)))
    {
        pPrint->Display(IDS_STATUS_FMT, pPrint->get_String(IDS_SUCCEEDED), L"");
    }
    else
    {
        if (fParseError)
        {
            pPrint->Display(IDS_ERROR_PARSE);
        }
        else
        {
            pPrint->Display(IDS_FAILED, GetLastError(), GetLastError());
        }
        iRet = 1;
    }


    if ((fFailAllErrors) && (cMember == 0) && !(fTesting))
    {
        iRet = 1;
    }

    CommonReturn:
        DELETE_OBJECT(pArgs);
        DELETE_OBJECT(pPrint);

        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, ParamError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
    TRACE_ERROR_EX(DBG_SS_APP, CATCloseError);
    TRACE_ERROR_EX(DBG_SS_APP, CDFOpenError);
}

void WINAPI DisplayParseError(DWORD dwWhichArea, DWORD dwLocalError, WCHAR *pwszLine)
{
    DWORD   idErr;
    DWORD   idFmt;

    fParseError = TRUE;

    switch (dwWhichArea)
    {
        case CRYPTCAT_E_AREA_HEADER:                idFmt = IDS_PARSE_E_HEADER_FMT;         break;
        case CRYPTCAT_E_AREA_MEMBER:                idFmt = IDS_PARSE_E_MEMBER_FMT;         break;
        case CRYPTCAT_E_AREA_ATTRIBUTE:             idFmt = IDS_PARSE_E_ATTRIBUTE_FMT;      break;
        default:                                    idFmt = IDS_PARSE_E_ATTRIBUTE_FMT;      break;
    }

    switch (dwLocalError)
    {
        case CRYPTCAT_E_CDF_MEMBER_FILE_PATH:       idErr = IDS_PARSE_ERROR_FILE_PATH;      break;
        case CRYPTCAT_E_CDF_MEMBER_INDIRECTDATA:    idErr = IDS_PARSE_ERROR_INDIRECTDATA;   break;
        case CRYPTCAT_E_CDF_MEMBER_FILENOTFOUND:    idErr = IDS_PARSE_ERROR_FILENOTFOUND;   break;
        case CRYPTCAT_E_CDF_BAD_GUID_CONV:          idErr = IDS_PARSE_ERROR_GUID_CONV;      break;
        case CRYPTCAT_E_CDF_ATTR_TYPECOMBO:         idErr = IDS_PARSE_ERROR_TYPECOMBO;      break;
        case CRYPTCAT_E_CDF_ATTR_TOOFEWVALUES:      idErr = IDS_PARSE_ERROR_TOOFEWVALUES;   break;
        case CRYPTCAT_E_CDF_UNSUPPORTED:            idErr = IDS_PARSE_ERROR_UNSUPPORTED;    break;
        case CRYPTCAT_E_CDF_DUPLICATE:              idErr = IDS_PARSE_ERROR_DUPLICATE;      break;
        case CRYPTCAT_E_CDF_TAGNOTFOUND:            idErr = IDS_PARSE_ERROR_NOTAG;          break;
        default:                                    idErr = IDS_PARSE_ERROR_UNKNOWN;        break;
    }

    pPrint->Display(idFmt, pPrint->get_String(idErr), pwszLine);

    if (fFailAllErrors)
    {
        iRet = 1;
    }
}


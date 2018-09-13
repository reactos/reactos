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
#include    <malloc.h>
#include    <memory.h>

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


WCHAR       *pwszFile    = NULL;
PrintfU_    *pPrint         = NULL;
int         iRet            = 0;

WCHAR       gszUsage[] = L"usage: calchash filename\n   -?:         this screen\n"  ;

const char     RgchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void FormatHashString(LPSTR *ppString, DWORD cbBlob, BYTE *pblob)
{
    DWORD   i, j = 0;
    BYTE    *pb = NULL;
    DWORD   numCharsInserted = 0;

    *ppString = NULL;
    pb = pblob;
    
    // fill the buffer
    i=0;
    while (j < cbBlob) 
    {
        if ((*ppString) == NULL)
        {
            *ppString = (LPSTR) malloc(3 * sizeof(char));
        }
        else
        {
            *ppString = (LPSTR) realloc(*ppString, (j+1) * 3 * sizeof(char));
        }

        (*ppString)[i++] = RgchHex[(*pb & 0xf0) >> 4];
        (*ppString)[i++] = RgchHex[*pb & 0x0f];
        (*ppString)[i++] = ' ';
        
        pb++;
        j++;
    }
    
    (*ppString)[i-1] = 0;
}


extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    int                 cMember;
    cWArgv_             *pArgs;
    CRYPTCATCDF         *pCDF;
    CRYPTCATMEMBER      *pMember;
    LPWSTR              pwszMemberTag;
    CRYPTCATATTRIBUTE   *pAttr;
    BOOL                fContinueOnError;
    BYTE                pbHash[40];
    DWORD               cbHash = sizeof(pbHash);
    HANDLE               hFile;
    LPSTR               psz;

    pCDF = NULL;

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_OPTPARAM, IDS_USAGETEXT_FILENAME, IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,         IDS_PARAMTEXT_HELP,       WARGV_VALUETYPE_BOOL, (void *)FALSE);
    
    pArgs->Fill(argc, wargv);

    if (!(pArgs->Fill(argc, wargv)) ||
        (pArgs->GetValue(IDS_PARAM_HELP)))
    {
        wprintf(L"%s", gszUsage);
        goto NeededHelp;
    }

    if (!(pwszFile = pArgs->GetFileName()))
    {
        wprintf(L"%s",gszUsage);
        goto ParamError;
    }

    pPrint = new PrintfU_;

    SetLastError(0);

    if ((hFile = CreateFileU(pwszFile,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL)) == INVALID_HANDLE_VALUE)
    {
        
        wprintf(L"Cannot open file - GLE = %lx\n", GetLastError());
        goto CATCloseError;
    }

    if (!CryptCATAdminCalcHashFromFileHandle(hFile, 
                                             &cbHash, 
                                             pbHash,
                                             0))
    {
        goto CATCloseError;
    }
    
    FormatHashString(&psz, cbHash, pbHash);
    printf("%s\n", psz);
    free(psz);

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
}




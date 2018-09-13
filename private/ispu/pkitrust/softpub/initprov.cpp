//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       initprov.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubInitialize
//
//  History:    05-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

static char *pwszUsageOID = szOID_PKIX_KP_CODE_SIGNING;

HRESULT WINAPI SoftpubInitialize(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]  = ERROR_SUCCESS;
    pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FILEIO]          = ERROR_SUCCESS;

    if (!(_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, fRecallWithState)) ||
        (!(pProvData->fRecallWithState)))
    {
        if (_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pszUsageOID))
        {
            pProvData->pszUsageOID = pwszUsageOID;
        }

        if (!_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct,
                dwProvFlags) ||
                    pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG)
        {
            //
            //  open "known" stores
            //
            if (!(WTHelperOpenKnownStores(pProvData)))
            {
                return(S_FALSE);
            }
        }
    }

    //
    //  for file type calls, make sure the file handle is valid -- open if necessary.
    //
    HANDLE      *phFile;
    const WCHAR *pcwszFile;

    switch (pProvData->pWintrustData->dwUnionChoice)
    {
        case WTD_CHOICE_FILE:
                phFile      = &pProvData->pWintrustData->pFile->hFile;
                pcwszFile   = pProvData->pWintrustData->pFile->pcwszFilePath;
                break;

        case WTD_CHOICE_CATALOG:
                phFile      = &pProvData->pWintrustData->pCatalog->hMemberFile;
                pcwszFile   = pProvData->pWintrustData->pCatalog->pcwszMemberFilePath;
                break;

        case WTD_CHOICE_BLOB:
                pcwszFile   = NULL;
                break;

        default:
                return(ERROR_SUCCESS);
    }

    if (!(pProvData->pPDSip))
    {
        if (!(pProvData->pPDSip = (PROVDATA_SIP *)pProvData->psPfns->pfnAlloc(sizeof(PROVDATA_SIP))))
        {
            pProvData->dwError = GetLastError();
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]  = TRUST_E_SYSTEM_ERROR;
            return(S_FALSE);
        }

        pProvData->dwSubjectChoice  = CPD_CHOICE_SIP;

        memset(pProvData->pPDSip, 0x00, sizeof(PROVDATA_SIP));
        pProvData->pPDSip->cbStruct = sizeof(PROVDATA_SIP);
    }


    if (pcwszFile)
    {
        //
        //  we're looking at a file based object...
        //
        pProvData->fOpenedFile = FALSE;

        if (!(*phFile) || (*phFile == INVALID_HANDLE_VALUE))
        {
            if ((*phFile = CreateFileU(pcwszFile,
                                        GENERIC_READ,
                                        FILE_SHARE_READ, // we're only reading!
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL)) == INVALID_HANDLE_VALUE)
            {
                pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FILEIO]          = GetLastError();
                pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]  = CRYPT_E_FILE_ERROR;
            }
            else
            {
                pProvData->fOpenedFile = TRUE;
            }
        }
    }

    return(ERROR_SUCCESS);
}

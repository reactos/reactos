//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       certprov.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubDefCertInit
//
//  History:    02-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

HRESULT WINAPI SoftpubDefCertInit(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    if (!(_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pszUsageOID)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_INVALID_PARAMETER;
        return(S_FALSE);
    }

    HRESULT                     hr;
    GUID                        gAuthenticode = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    CRYPT_PROVIDER_FUNCTIONS    sAuthenticodePfns;

    //
    //  fill in the Authenticode Functions
    //
    memset(&sAuthenticodePfns, 0x00, sizeof(CRYPT_PROVIDER_FUNCTIONS));
    sAuthenticodePfns.cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);

    if (!(WintrustLoadFunctionPointers(&gAuthenticode, &sAuthenticodePfns)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_PROVIDER_UNKNOWN;
        return(S_FALSE);
    }

    hr = S_OK;

    if (sAuthenticodePfns.pfnInitialize)
    {
        hr = sAuthenticodePfns.pfnInitialize(pProvData);
    }

    //
    //  assign our usage
    //
    if (pProvData->pWintrustData)
    {
        if (pProvData->pWintrustData->pPolicyCallbackData)
        {
            pProvData->pszUsageOID = (char *)pProvData->pWintrustData->pPolicyCallbackData;
        }
    }

    return(hr);
}


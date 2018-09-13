//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       provider.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  WintrustAddProvider
//              WintrustRemoveProvider
//
//  History:    30-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "cryptreg.h"

BOOL WINAPI WintrustAddActionID(IN GUID *pgActionID, 
                                IN DWORD fdwReserved,    // future use.
                                IN CRYPT_REGISTER_ACTIONID *psProvInfo)
{
    if (!(psProvInfo) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_REGISTER_ACTIONID, psProvInfo->cbStruct, sTestPolicyProvider)))
    {
        return(FALSE);
    }

    BOOL    fRet;

    fRet = TRUE;

    SetRegProvider(pgActionID, 
                   REG_INIT_PROVIDER_KEY,
                   psProvInfo->sInitProvider.pwszDLLName,
                   psProvInfo->sInitProvider.pwszFunctionName);

    SetRegProvider(pgActionID, 
                   REG_OBJTRUST_PROVIDER_KEY,
                   psProvInfo->sObjectProvider.pwszDLLName,
                   psProvInfo->sObjectProvider.pwszFunctionName);

    SetRegProvider(pgActionID, 
                   REG_SIGTRUST_PROVIDER_KEY,
                   psProvInfo->sSignatureProvider.pwszDLLName,
                   psProvInfo->sSignatureProvider.pwszFunctionName);

    SetRegProvider(pgActionID, 
                   REG_CERTTRUST_PROVIDER_KEY,
                   psProvInfo->sCertificateProvider.pwszDLLName,
                   psProvInfo->sCertificateProvider.pwszFunctionName);

    SetRegProvider(pgActionID, 
                   REG_CERTPOL_PROVIDER_KEY,
                   psProvInfo->sCertificatePolicyProvider.pwszDLLName,
                   psProvInfo->sCertificatePolicyProvider.pwszFunctionName);

    SetRegProvider(pgActionID, 
                   REG_FINALPOL_PROVIDER_KEY,
                   psProvInfo->sFinalPolicyProvider.pwszDLLName,
                   psProvInfo->sFinalPolicyProvider.pwszFunctionName);

    SetRegProvider(pgActionID, 
                   REG_TESTPOL_PROVIDER_KEY,
                   psProvInfo->sTestPolicyProvider.pwszDLLName,
                   psProvInfo->sTestPolicyProvider.pwszFunctionName);

    // this member was added 7/23/1997 pberkman
    if (WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_REGISTER_ACTIONID, psProvInfo->cbStruct, sCleanupProvider))
    {
        SetRegProvider(pgActionID, 
                       REG_CLEANUP_PROVIDER_KEY,
                       psProvInfo->sCleanupProvider.pwszDLLName,
                       psProvInfo->sCleanupProvider.pwszFunctionName);
    }

    return(TRUE);
}


BOOL WINAPI  WintrustRemoveActionID(IN GUID *pgActionID)
{
    RemoveRegProvider(pgActionID, REG_INIT_PROVIDER_KEY);
    RemoveRegProvider(pgActionID, REG_OBJTRUST_PROVIDER_KEY);
    RemoveRegProvider(pgActionID, REG_SIGTRUST_PROVIDER_KEY);
    RemoveRegProvider(pgActionID, REG_CERTTRUST_PROVIDER_KEY);
    RemoveRegProvider(pgActionID, REG_CERTPOL_PROVIDER_KEY);
    RemoveRegProvider(pgActionID, REG_FINALPOL_PROVIDER_KEY);
    RemoveRegProvider(pgActionID, REG_TESTPOL_PROVIDER_KEY);
    RemoveRegProvider(pgActionID, REG_CLEANUP_PROVIDER_KEY);

    return(TRUE);
}



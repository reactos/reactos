//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       provload.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  WintrustLoadFunctionPointers
//              WintrustFindProvider
//              WintrustUnloadProviderList
//
//              *** local functions ***
//              _CheckLoadedProviders
//              _CheckRegisteredProviders
//              _provLoadDLL
//              _provUnloadDLL
//              _provLoadFunction
//
//  History:    29-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "cryptreg.h"
#include    "eventlst.h"

LOADED_PROVIDER *_CheckLoadedProviders(GUID *pgActionID);
LOADED_PROVIDER *_CheckRegisteredProviders(GUID *pgActionID);

BOOL _provLoadDLL(WCHAR *pwszDLL, HINSTANCE *phDLL);
void _provUnloadDLL(HINSTANCE hDLL, WCHAR *pwszDLLName);
BOOL _provLoadFunction(char *pszFunc, HINSTANCE hDLL, void **pfn);

LOADED_PROVIDER                     *pProviderList  = NULL;


BOOL WINAPI WintrustLoadFunctionPointers(GUID *pgActionID, CRYPT_PROVIDER_FUNCTIONS *pPfns)
{
    LOADED_PROVIDER *pProvFuncs;

    if (!(pPfns) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_PROVIDER_FUNCTIONS, pPfns->cbStruct, psUIpfns)))
    {
        return(FALSE);
    }

    if (!(pProvFuncs = WintrustFindProvider(pgActionID)))
    {
        return(FALSE);
    }

    pPfns->pfnAlloc             = WVTNew;
    pPfns->pfnFree              = WVTDelete;
    pPfns->pfnAddStore2Chain    = WVTAddStore;
    pPfns->pfnAddSgnr2Chain     = WVTAddSigner;
    pPfns->pfnAddCert2Chain     = WVTAddCertContext;
    pPfns->pfnAddPrivData2Chain = WVTAddPrivateData;

    pPfns->pfnInitialize        = pProvFuncs->pfnInitialize;
    pPfns->pfnObjectTrust       = pProvFuncs->pfnObjectTrust;
    pPfns->pfnSignatureTrust    = pProvFuncs->pfnSignatureTrust;
    pPfns->pfnCertificateTrust  = pProvFuncs->pfnCertificateTrust;
    pPfns->pfnFinalPolicy       = pProvFuncs->pfnFinalPolicy;
    pPfns->pfnCertCheckPolicy   = pProvFuncs->pfnCertCheckPolicy;
    pPfns->pfnTestFinalPolicy   = pProvFuncs->pfnTestFinalPolicy;
    
    if (WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_PROVIDER_FUNCTIONS, pPfns->cbStruct, pfnCleanupPolicy))
    {
        pPfns->pfnCleanupPolicy = pProvFuncs->pfnCleanupPolicy;
    }

    return(TRUE);
}



LOADED_PROVIDER *WintrustFindProvider(GUID *pgActionID)
{
    LOADED_PROVIDER *pProvider;

    if (!(pProvider = _CheckLoadedProviders(pgActionID)))
    {
#       if (DBG)
            DbgPrintf(DBG_SS, "Loading Provider: %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                        pgActionID->Data1, pgActionID->Data2, pgActionID->Data3, pgActionID->Data4[0],
                        pgActionID->Data4[1], pgActionID->Data4[2], pgActionID->Data4[3], pgActionID->Data4[4],
                        pgActionID->Data4[5], pgActionID->Data4[6], pgActionID->Data4[7]);
#       endif // DBG

        pProvider = _CheckRegisteredProviders(pgActionID);
    }

#   if (DBG)

        if (!(pProvider))
        {
            DbgPrintf(DBG_SS, "PROV NOT FOUND: %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                        pgActionID->Data1, pgActionID->Data2, pgActionID->Data3, pgActionID->Data4[0],
                        pgActionID->Data4[1], pgActionID->Data4[2], pgActionID->Data4[3], pgActionID->Data4[4],
                        pgActionID->Data4[5], pgActionID->Data4[6], pgActionID->Data4[7]);
        }

#   endif

    return(pProvider);
}

LOADED_PROVIDER *_CheckLoadedProviders(GUID *pgActionID)
{
    LOADED_PROVIDER     *pProvider;

    AcquireReadLock(sProvLock);

    pProvider = pProviderList;

    while (pProvider)
    {
        if (memcmp(pgActionID, &pProvider->gActionID, sizeof(GUID)) == 0)
        {
            ReleaseReadLock(sProvLock);

            return(pProvider);
        }

        pProvider = pProvider->pNext;
    }

    ReleaseReadLock(sProvLock);

    return(NULL);
}

BOOL WintrustUnloadProviderList(void)
{
    LOADED_PROVIDER *pProvider;
    LOADED_PROVIDER *pProvHold;

    AcquireWriteLock(sProvLock);

    pProvider = pProviderList;

    while (pProvider)
    {
        if (pProvider->hInitDLL)            FreeLibrary(pProvider->hInitDLL);
        if (pProvider->hObjectDLL)          FreeLibrary(pProvider->hObjectDLL);
        if (pProvider->hSignatureDLL)       FreeLibrary(pProvider->hSignatureDLL);
        if (pProvider->hCertTrustDLL)       FreeLibrary(pProvider->hCertTrustDLL);
        if (pProvider->hFinalPolicyDLL)     FreeLibrary(pProvider->hFinalPolicyDLL);
        if (pProvider->hCertPolicyDLL)      FreeLibrary(pProvider->hCertPolicyDLL);
        if (pProvider->hTestFinalPolicyDLL) FreeLibrary(pProvider->hTestFinalPolicyDLL);
        if (pProvider->hCleanupPolicyDLL)   FreeLibrary(pProvider->hCleanupPolicyDLL);

#       if (DBG)
            DbgPrintf(DBG_SS, "Unloading Provider: %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                        pProvider->gActionID.Data1, pProvider->gActionID.Data2, pProvider->gActionID.Data3, 
                        pProvider->gActionID.Data4[0], pProvider->gActionID.Data4[1], 
                        pProvider->gActionID.Data4[2], pProvider->gActionID.Data4[3], 
                        pProvider->gActionID.Data4[4], pProvider->gActionID.Data4[5], 
                        pProvider->gActionID.Data4[6], pProvider->gActionID.Data4[7]);
#       endif // DBG

        pProvHold = pProvider->pNext;

        delete pProvider;

        pProvider = pProvHold;
    }

    pProviderList = NULL;

    ReleaseWriteLock(sProvLock);

    return(TRUE);
}

LOADED_PROVIDER *_CheckRegisteredProviders(GUID *pgActionID)
{
    LOADED_PROVIDER *pProvider;
    BOOL            fRet;

    WCHAR           wszInitDLL[REG_MAX_KEY_NAME];
    WCHAR           wszObjTrustDLL[REG_MAX_KEY_NAME];
    WCHAR           wszSigTrustDLL[REG_MAX_KEY_NAME];
    WCHAR           wszCertTrustDLL[REG_MAX_KEY_NAME];
    WCHAR           wszCertPolDLL[REG_MAX_KEY_NAME];
    WCHAR           wszFinalPolDLL[REG_MAX_KEY_NAME];
    WCHAR           wszTestFinalPolDLL[REG_MAX_KEY_NAME];
    WCHAR           wszCleanupPolDLL[REG_MAX_KEY_NAME];

    char            szInitFunc[REG_MAX_FUNC_NAME];
    char            szObjTrustFunc[REG_MAX_FUNC_NAME];
    char            szSigTrustFunc[REG_MAX_FUNC_NAME];
    char            szCertTrustFunc[REG_MAX_FUNC_NAME];
    char            szCertPolFunc[REG_MAX_FUNC_NAME];
    char            szFinalPolFunc[REG_MAX_FUNC_NAME];
    char            szTestFinalPolFunc[REG_MAX_FUNC_NAME];
    char            szCleanupPolFunc[REG_MAX_FUNC_NAME];
    
    if (!(GetRegProvider(pgActionID, REG_CERTTRUST_PROVIDER_KEY, &wszCertTrustDLL[0], &szCertTrustFunc[0])))
    {
        return(NULL);
    }

    if (!(GetRegProvider(pgActionID, REG_FINALPOL_PROVIDER_KEY, &wszFinalPolDLL[0], &szFinalPolFunc[0])))
    {
        return(NULL);
    }

    // optional!
    GetRegProvider(pgActionID, REG_INIT_PROVIDER_KEY, &wszInitDLL[0], &szInitFunc[0]);
    GetRegProvider(pgActionID, REG_OBJTRUST_PROVIDER_KEY, &wszObjTrustDLL[0], &szObjTrustFunc[0]);
    GetRegProvider(pgActionID, REG_SIGTRUST_PROVIDER_KEY, &wszSigTrustDLL[0], &szSigTrustFunc[0]);
    GetRegProvider(pgActionID, REG_CERTPOL_PROVIDER_KEY, &wszCertPolDLL[0], &szCertPolFunc[0]);
    GetRegProvider(pgActionID, REG_TESTPOL_PROVIDER_KEY, &wszTestFinalPolDLL[0], &szTestFinalPolFunc[0]);
    GetRegProvider(pgActionID, REG_CLEANUP_PROVIDER_KEY, &wszCleanupPolDLL[0], &szCleanupPolFunc[0]);

    AcquireWriteLock(sProvLock);

    if (!(pProvider = (LOADED_PROVIDER *)WVTNew(sizeof(LOADED_PROVIDER))))
    {
        ReleaseWriteLock(sProvLock);
        return(NULL);
    }

    memset(pProvider, 0x00, sizeof(LOADED_PROVIDER));

    memcpy(&pProvider->gActionID, pgActionID, sizeof(GUID));

    fRet = TRUE;

    fRet &= _provLoadDLL(&wszCertTrustDLL[0],        &pProvider->hCertTrustDLL);
    fRet &= _provLoadDLL(&wszFinalPolDLL[0],         &pProvider->hFinalPolicyDLL);
    
    // optional!
    _provLoadDLL(&wszInitDLL[0],                     &pProvider->hInitDLL);
    _provLoadDLL(&wszObjTrustDLL[0],                 &pProvider->hObjectDLL);
    _provLoadDLL(&wszSigTrustDLL[0],                 &pProvider->hSignatureDLL);
    _provLoadDLL(&wszCertPolDLL[0],                  &pProvider->hCertPolicyDLL);
    _provLoadDLL(&wszTestFinalPolDLL[0],             &pProvider->hTestFinalPolicyDLL);
    _provLoadDLL(&wszCleanupPolDLL[0],               &pProvider->hCleanupPolicyDLL);

    fRet &= _provLoadFunction(&szCertTrustFunc[0], pProvider->hCertTrustDLL, (void **)&pProvider->pfnCertificateTrust);
    fRet &= _provLoadFunction(&szFinalPolFunc[0],  pProvider->hFinalPolicyDLL, (void **)&pProvider->pfnFinalPolicy);

    // optional!
    _provLoadFunction(&szInitFunc[0],                pProvider->hInitDLL, (void **)&pProvider->pfnInitialize);
    _provLoadFunction(&szObjTrustFunc[0],            pProvider->hObjectDLL, (void **)&pProvider->pfnObjectTrust);
    _provLoadFunction(&szSigTrustFunc[0],            pProvider->hSignatureDLL, (void **)&pProvider->pfnSignatureTrust);
    _provLoadFunction(&szCertPolFunc[0],             pProvider->hCertPolicyDLL, (void **)&pProvider->pfnCertCheckPolicy);
    _provLoadFunction(&szTestFinalPolFunc[0],        pProvider->hTestFinalPolicyDLL, (void **)&pProvider->pfnTestFinalPolicy);
    _provLoadFunction(&szCleanupPolFunc[0],          pProvider->hCleanupPolicyDLL, (void **)&pProvider->pfnCleanupPolicy);

    if (!(fRet))
    {
        ReleaseWriteLock(sProvLock);

        _provUnloadDLL(pProvider->hInitDLL,              &wszInitDLL[0]);
        _provUnloadDLL(pProvider->hObjectDLL,            &wszObjTrustDLL[0]);
        _provUnloadDLL(pProvider->hSignatureDLL,         &wszSigTrustDLL[0]);
        _provUnloadDLL(pProvider->hCertTrustDLL,         &wszCertTrustDLL[0]);
        _provUnloadDLL(pProvider->hFinalPolicyDLL,       &wszFinalPolDLL[0]);
        _provUnloadDLL(pProvider->hCertPolicyDLL,        &wszCertPolDLL[0]);
        _provUnloadDLL(pProvider->hTestFinalPolicyDLL,   &wszTestFinalPolDLL[0]);
        _provUnloadDLL(pProvider->hCleanupPolicyDLL,     &wszCleanupPolDLL[0]);

        delete pProvider;
        
        return(NULL);
    }

    pProvider->pNext            = pProviderList;
    pProvider->pPrev            = NULL;

    if (pProvider->pNext)
    {
        pProvider->pNext->pPrev = pProvider;
    }

    pProviderList = pProvider;

    ReleaseWriteLock(sProvLock);

    return(pProvider);
}

BOOL _provLoadDLL(WCHAR *pwszDLL, HINSTANCE *phDLL)
{
    *phDLL = NULL;

    if (!(pwszDLL[0]))
    {
        return(FALSE);
    }

    if (_wcsicmp(pwszDLL, W_MY_NAME) == 0)
    {
        *phDLL = (HINSTANCE)hMeDLL;
    }
    else
    {
        *phDLL = LoadLibraryU(pwszDLL);
    }

    if (*phDLL)
    {
        return(TRUE);
    }

    return(FALSE);
}

BOOL _provLoadFunction(char *pszFunc, HINSTANCE hDLL, void **pfn)
{
    *pfn = NULL;

    if (!(pszFunc[0]) ||
        !(hDLL))
    {
        return(FALSE);
    }

    *pfn = (void *)GetProcAddress(hDLL, pszFunc);

    if (*pfn)
    {
        return(TRUE);
    }

    return(FALSE);
}

void _provUnloadDLL(HINSTANCE hDLL, WCHAR *pwszDLLName)
{
    if ((hDLL) &&
        (_wcsicmp(pwszDLLName, W_MY_NAME) != 0))
    {
        FreeLibrary(hDLL);
    }
}

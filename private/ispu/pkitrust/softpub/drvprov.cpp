//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drvprov.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  DriverInitializePolicy
//              DriverCleanupPolicy
//              DriverFinalPolicy
//              DriverRegisterServer
//              DriverUnregisterServer
//
//              *** local functions ***
//              _CheckAttr
//              _CheckVersionAttribute
//              _CheckVersion
//              _FillVersionLongs
//              _FindVersionInAttr
//
//  History:    29-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------


#include        "global.hxx"

BOOL _CheckAttr(DRIVER_VER_INFO *pVerInfo, CRYPTCATATTRIBUTE *pAttr, BOOL *pfFailedCheck);
BOOL _CheckVersionAttribute(DRIVER_VER_INFO *pVerInfo, CRYPTCATATTRIBUTE *pAttr, BOOL *pfFailedCheck);
BOOL _CheckVersion(DRIVER_VER_INFO *pVerInfo, OSVERSIONINFO *pVersion, WCHAR *pwszAttr);
BOOL _FillVersionLongs(WCHAR *pwszMM, long *plMajor, long *plMinor, WCHAR *pwcFlag);
BOOL _FindVersionInAttr(DRIVER_VER_INFO *pVerInfo, OSVERSIONINFO *pVersion, CRYPTCATATTRIBUTE *pAttr);

static LPSTR   rgDriverUsages[] = {szOID_WHQL_CRYPTO, szOID_NT5_CRYPTO, szOID_OEM_WHQL_CRYPTO};
static CERT_USAGE_MATCH RequestUsage = {USAGE_MATCH_TYPE_OR, {sizeof(rgDriverUsages)/sizeof(LPSTR), rgDriverUsages}};

typedef struct _DRVPROV_PRIVATE_DATA
{
    DWORD                       cbStruct;

    CRYPT_PROVIDER_FUNCTIONS    sAuthenticodePfns;

} DRVPROV_PRIVATE_DATA, *PDRVPROV_PRIVATE_DATA;


HRESULT WINAPI DriverInitializePolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return (S_FALSE);
    }

    if (!(_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pRequestUsage)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_INVALID_PARAMETER;
        return (S_FALSE);
    }

    GUID                        gAuthenticode   = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID                        gDriverProv     = DRIVER_ACTION_VERIFY;
    CRYPT_PROVIDER_PRIVDATA     sPrivData;
    CRYPT_PROVIDER_PRIVDATA     *pPrivData;
    DRVPROV_PRIVATE_DATA        *pDriverData;
    HRESULT                     hr;

    hr = S_OK;

    pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);

    if (!(pPrivData))
    {
        memset(&sPrivData, 0x00, sizeof(CRYPT_PROVIDER_PRIVDATA));
        sPrivData.cbStruct      = sizeof(CRYPT_PROVIDER_PRIVDATA);

        memcpy(&sPrivData.gProviderID, &gDriverProv, sizeof(GUID));

        //
        //  add my data to the chain!
        //
        pProvData->psPfns->pfnAddPrivData2Chain(pProvData, &sPrivData);

        //
        //  get the new reference
        //
        pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);
    }


    //
    //  allocate space for my struct
    //
    if (!(pPrivData->pvProvData = pProvData->psPfns->pfnAlloc(sizeof(DRVPROV_PRIVATE_DATA))))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_SYSTEM_ERROR;
        return (S_FALSE);
    }

    memset(pPrivData->pvProvData, 0x00, sizeof(DRVPROV_PRIVATE_DATA));
    pPrivData->cbProvData   = sizeof(DRVPROV_PRIVATE_DATA);

    pDriverData             = (DRVPROV_PRIVATE_DATA *)pPrivData->pvProvData;
    pDriverData->cbStruct   = sizeof(DRVPROV_PRIVATE_DATA);

    //
    //  fill in the Authenticode Functions
    //
    pDriverData->sAuthenticodePfns.cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);

    if (!(WintrustLoadFunctionPointers(&gAuthenticode, &pDriverData->sAuthenticodePfns)))
    {
        pProvData->psPfns->pfnFree(sPrivData.pvProvData);
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_PROVIDER_UNKNOWN;
        return (S_FALSE);
    }

    if (pDriverData->sAuthenticodePfns.pfnInitialize)
    {
        hr = pDriverData->sAuthenticodePfns.pfnInitialize(pProvData);
    }

    //
    //  assign our usage
    //
    pProvData->pRequestUsage = &RequestUsage;

    // for backwards compatibility
    pProvData->pszUsageOID  = szOID_WHQL_CRYPTO;


#if 0
    //
    //  do NOT allow test certs EVER!
    //
    pProvData->dwRegPolicySettings  &= ~(WTPF_TRUSTTEST);
    pProvData->dwRegPolicySettings  &= ~(WTPF_TESTCANBEVALID);
#endif

    return (hr);
}

HRESULT WINAPI DriverCleanupPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    GUID                        gDriverProv = DRIVER_ACTION_VERIFY;
    CRYPT_PROVIDER_PRIVDATA     *pMyData;
    DRVPROV_PRIVATE_DATA        *pDriverData;
    HRESULT                     hr;

    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return (S_FALSE);
    }

    hr = S_OK;

    pMyData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);

    if (pMyData)
    {
        pDriverData = (DRVPROV_PRIVATE_DATA *)pMyData->pvProvData;

        //
        //  remove the data we allocated except for the "MyData" which WVT will clean up for us!
        //
        if (pDriverData->sAuthenticodePfns.pfnCleanupPolicy)
        {
            hr = pDriverData->sAuthenticodePfns.pfnCleanupPolicy(pProvData);
        }

        pProvData->psPfns->pfnFree(pMyData->pvProvData);
        pMyData->pvProvData = NULL;
        pMyData->cbProvData = 0;
    }

    return (hr);
}

//+-------------------------------------------------------------------------
//  Allocates and returns the specified cryptographic message parameter.
//--------------------------------------------------------------------------
static void *AllocAndGetMsgParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT DWORD *pcbData
    )
{
    void *pvData;
    DWORD cbData;

    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            NULL,           // pvData
            &cbData) || 0 == cbData)
        goto GetParamError;
    if (NULL == (pvData = malloc(cbData)))
        goto OutOfMemory;
    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            pvData,
            &cbData)) {
        free(pvData);
        goto GetParamError;
    }

CommonReturn:
    *pcbData = cbData;
    return pvData;
ErrorReturn:
    pvData = NULL;
    cbData = 0;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetParamError)
}

//+-------------------------------------------------------------------------
//  Alloc and NOCOPY Decode
//--------------------------------------------------------------------------
static void *AllocAndDecodeObject(
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            NULL,                   // pvStructInfo
            &cbStructInfo
            );
    if (cbStructInfo == 0)
        goto ErrorReturn;
    if (NULL == (pvStructInfo = malloc(cbStructInfo)))
        goto ErrorReturn;
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            pvStructInfo,
            &cbStructInfo
            )) {
        free(pvStructInfo);
        goto ErrorReturn;
    }

CommonReturn:
    return pvStructInfo;
ErrorReturn:
    pvStructInfo = NULL;
    goto CommonReturn;
}

static void CopyBytesToMaxPathString(
    IN const BYTE *pbData,
    IN DWORD cbData,
    OUT WCHAR wszDst[MAX_PATH]
    )
{
    DWORD cchDst;

    if (pbData) {
        cchDst = cbData / sizeof(WCHAR);
        if (cchDst > MAX_PATH - 1)
            cchDst = MAX_PATH - 1;
    } else
        cchDst = 0;

    if (cchDst)
        memcpy(wszDst, pbData, cchDst * sizeof(WCHAR));

    wszDst[cchDst] = L'\0';
}

void UpdateDriverVersion(
    IN CRYPT_PROVIDER_DATA *pProvData,
    OUT WCHAR wszVersion[MAX_PATH]
    )
{
    HCRYPTMSG hMsg = pProvData->hMsg;
    BYTE *pbContent = NULL;
    DWORD cbContent;
    PCTL_INFO pCtlInfo = NULL;
    PCERT_EXTENSION pExt;               // not allocated
    PCAT_NAMEVALUE pNameValue = NULL;

    if (NULL == hMsg)
        goto NoMessage;

    // Get the inner content.
    if (NULL == (pbContent = (BYTE *) AllocAndGetMsgParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            &cbContent))) goto GetContentError;

    if (NULL == (pCtlInfo = (PCTL_INFO) AllocAndDecodeObject(
            PKCS_CTL,
            pbContent,
            cbContent
            )))
        goto DecodeCtlError;

    if (NULL == (pExt = CertFindExtension(
            CAT_NAMEVALUE_OBJID,
            pCtlInfo->cExtension,
            pCtlInfo->rgExtension
            )))
        goto NoVersionExt;

    if (NULL == (pNameValue = (PCAT_NAMEVALUE) AllocAndDecodeObject(
            CAT_NAMEVALUE_STRUCT,
            pExt->Value.pbData,
            pExt->Value.cbData
            )))
        goto DecodeNameValueError;

    CopyBytesToMaxPathString(pNameValue->Value.pbData,
        pNameValue->Value.cbData, wszVersion);

CommonReturn:
    if (pNameValue)
        free(pNameValue);
    if (pCtlInfo)
        free(pCtlInfo);
    if (pbContent)
        free(pbContent);

    return;
ErrorReturn:
    wszVersion[0] = L'\0';
    goto CommonReturn;

TRACE_ERROR(NoMessage)
TRACE_ERROR(GetContentError)
TRACE_ERROR(DecodeCtlError)
TRACE_ERROR(NoVersionExt)
TRACE_ERROR(DecodeNameValueError)
}

HRESULT WINAPI DriverFinalPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    GUID                        gDriverProv = DRIVER_ACTION_VERIFY;
    HRESULT                     hr;
    CRYPT_PROVIDER_PRIVDATA     *pMyData;

    CRYPTCATATTRIBUTE           *pCatAttr;
    CRYPTCATATTRIBUTE           *pMemAttr;

    DRIVER_VER_INFO             *pVerInfo;

    DWORD                       dwExceptionCode;

    hr  = ERROR_SUCCESS;

    if (!(_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pszUsageOID)) ||
        !(pProvData->pWintrustData) ||
        !(_ISINSTRUCT(WINTRUST_DATA, pProvData->pWintrustData->cbStruct, hWVTStateData)))
    {
        goto ErrorInvalidParam;
    }

    //
    //  if pVerInfo is there, we are being called from SigVerif
    //
    pVerInfo = (DRIVER_VER_INFO *)pProvData->pWintrustData->pPolicyCallbackData;

    if (pVerInfo)
    {
        CRYPT_PROVIDER_SGNR *pSgnr;
    	CRYPT_PROVIDER_CERT *pCert;

    	// KeithV
    	// Today we do not support ranges of versions, so the version
    	// number must be the same. Also must be none zero
        if ((_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, sOSVersionLow)) &&
            (_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, sOSVersionHigh)))
        {
            if(memcmp(&pVerInfo->sOSVersionLow,
            	  &pVerInfo->sOSVersionHigh,
            	  sizeof(DRIVER_VER_MAJORMINOR)) )
            {
                    goto ErrorInvalidParam;
            }
        }

        if (!(_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, pcSignerCertContext)))
        {
            goto ErrorInvalidParam;
        }

        pVerInfo->wszVersion[0] = NULL;

        if (!(pSgnr = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
        {
            goto ErrorNoSigner;
        }

        if (!(pCert = WTHelperGetProvCertFromChain(pSgnr, 0)))
        {
            goto ErrorNoCert;
        }

        if (pCert->pCert)
        {
            CertGetNameStringW(
                pCert->pCert,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,                                  // dwFlags
                NULL,                               // pvTypePara
                pVerInfo->wszSignedBy,
                MAX_PATH
                );

            pVerInfo->pcSignerCertContext = CertDuplicateCertificateContext(pCert->pCert);

            if (pVerInfo->dwReserved1 == 0x1 && pVerInfo->dwReserved2 == 0) {
                HCRYPTMSG hMsg = pProvData->hMsg;

                // Return the message's store
                if (hMsg) {
                    HCERTSTORE hStore;
                    hStore = CertOpenStore(
                        CERT_STORE_PROV_MSG,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        0,                      // hCryptProv
                        0,                      // dwFlags
                        (const void *) hMsg
                        );
                    pVerInfo->dwReserved2 = (ULONG_PTR) hStore;
                }
            }
        }

    }


    if (pProvData->padwTrustStepErrors)
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] = ERROR_SUCCESS;
    }

    if ((hr = checkGetErrorBasedOnStepErrors(pProvData)) != ERROR_SUCCESS)
    {
        goto StepError;
    }

    pCatAttr = NULL;
    pMemAttr = NULL;


    if ((pProvData->pPDSip) &&
        (_ISINSTRUCT(PROVDATA_SIP, pProvData->pPDSip->cbStruct, psIndirectData)) &&
        (pProvData->pPDSip->psSipSubjectInfo) &&
        (pProvData->pPDSip->psSipSubjectInfo->dwUnionChoice == MSSIP_ADDINFO_CATMEMBER) &&
        (pProvData->pPDSip->psSipSubjectInfo->psCatMember) &&
        (pProvData->pPDSip->psSipSubjectInfo->psCatMember->pStore) &&
        (pProvData->pPDSip->psSipSubjectInfo->psCatMember->pMember) &&
        (pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_CATALOG))
    {
      // The following APIs are in DELAYLOAD'ed mscat32.dll. If the
      // DELAYLOAD fails an exception is raised.
      __try {
        HANDLE  hCatStore;

        hCatStore   = CryptCATHandleFromStore(pProvData->pPDSip->psSipSubjectInfo->psCatMember->pStore);

        //
        //  first look at the members attr
        //
        pMemAttr = CryptCATGetAttrInfo(hCatStore,
                                       pProvData->pPDSip->psSipSubjectInfo->psCatMember->pMember,
                                       L"OSAttr");

        BOOL    fFailedCheck;

        fFailedCheck = FALSE;

        if (!(_CheckAttr(pVerInfo, pMemAttr, &fFailedCheck)))
        {
            if (fFailedCheck)
            {
                goto OSAttrVersionError;
            }

            pCatAttr = CryptCATGetCatAttrInfo(hCatStore, L"OSAttr");

            if (!(_CheckAttr(pVerInfo, pCatAttr, &fFailedCheck)))
            {
                if (fFailedCheck)
                {
                    goto OSAttrVersionError;
                }

                if (!(pCatAttr) && !(pMemAttr))
                {
                    goto OSAttrNotFound;
                }
            }
        }
      } __except(EXCEPTION_EXECUTE_HANDLER) {
          dwExceptionCode = GetExceptionCode();
          goto CryptCATException;
      }
    }
    else if ((pProvData->pWintrustData) &&
             (pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_CATALOG))
    {
        goto ErrorInvalidParam;
    }

    //
    //  fill our name for SigVerif...
    //
    if (pVerInfo)
    {
        if (!(pVerInfo->wszVersion[0]))
        {
            if ((pMemAttr) && (pMemAttr->cbValue > 0) && (pMemAttr->pbValue))
            {
                CopyBytesToMaxPathString(pMemAttr->pbValue, pMemAttr->cbValue,
                    pVerInfo->wszVersion);
            }
            else if ((pCatAttr) && (pCatAttr->cbValue > 0) && (pCatAttr->pbValue))
            {
                CopyBytesToMaxPathString(pCatAttr->pbValue, pCatAttr->cbValue,
                    pVerInfo->wszVersion);
            }
            else
            {
                UpdateDriverVersion(pProvData, pVerInfo->wszVersion);
            }
        }
    }

    //
    //  retrieve my data from the provider struct
    //
    pMyData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);

    if (pMyData)
    {
        DRVPROV_PRIVATE_DATA    *pDriverData;

        pDriverData = (DRVPROV_PRIVATE_DATA *)pMyData->pvProvData;

        //
        //  call the standard final policy
        //
        if (pDriverData)
        {
            if (pDriverData->sAuthenticodePfns.pfnFinalPolicy)
            {
                DWORD   dwOldUIFlags;

                dwOldUIFlags = pProvData->pWintrustData->dwUIChoice;
                pProvData->pWintrustData->dwUIChoice    = WTD_UI_NONE;

                hr = pDriverData->sAuthenticodePfns.pfnFinalPolicy(pProvData);

                pProvData->pWintrustData->dwUIChoice    = dwOldUIFlags;
            }
        }
    }

    CommonReturn:
        if (hr != ERROR_INVALID_PARAMETER)
        {
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] = hr;
        }

        return (hr);

    ErrorReturn:
        hr = GetLastError();
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, ErrorInvalidParam, ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, ErrorNoSigner,     TRUST_E_NOSIGNATURE);
    SET_ERROR_VAR_EX(DBG_SS, ErrorNoCert,       TRUST_E_NO_SIGNER_CERT);
    SET_ERROR_VAR_EX(DBG_SS, OSAttrNotFound,    ERROR_APP_WRONG_OS);
    SET_ERROR_VAR_EX(DBG_SS, OSAttrVersionError,ERROR_APP_WRONG_OS);
    SET_ERROR_VAR_EX(DBG_SS, StepError,         hr);
    SET_ERROR_VAR_EX(DBG_SS, CryptCATException, dwExceptionCode);
}

BOOL _CheckAttr(DRIVER_VER_INFO *pVerInfo, CRYPTCATATTRIBUTE *pAttr, BOOL *pfFailedCheck)
{
    if (!(pAttr) || (pAttr->cbValue < 1) || !(pAttr->pbValue))
    {
        return(FALSE);
    }

    if (!(pVerInfo) ||
        ((pVerInfo->dwPlatform != 0) || (pVerInfo->dwVersion != 0)))
    {
        if (!(_CheckVersionAttribute(pVerInfo, pAttr, pfFailedCheck)))
        {
            return(FALSE);
        }
    }

    return(TRUE);
}

BOOL _CheckVersionAttribute(DRIVER_VER_INFO *pVerInfo, CRYPTCATATTRIBUTE *pAttr, BOOL *pfFailedCheck)
{
    *pfFailedCheck = FALSE;

    if (!(pAttr) || (pAttr->cbValue < 1) || !(pAttr->pbValue))
    {
        return (FALSE);
    }

    OSVERSIONINFO   sVersion;

    memset(&sVersion, 0x00, sizeof(OSVERSIONINFO));

    sVersion.dwOSVersionInfoSize    = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&sVersion))
    {

	// BUG BUG
	// update the version to the OS, this only works if
	// sOSVersionLow == pVerInfo->sOSVersionHigh
	if(NULL != pVerInfo) {
	    sVersion.dwMajorVersion = pVerInfo->sOSVersionLow.dwMajor;
	    sVersion.dwMinorVersion = pVerInfo->sOSVersionLow.dwMinor;
	    sVersion.dwPlatformId   = pVerInfo->dwPlatform;
	}

        if (_FindVersionInAttr(pVerInfo, &sVersion, pAttr))
        {
            if (pVerInfo)
            {
                CopyBytesToMaxPathString(pAttr->pbValue, pAttr->cbValue,
                    pVerInfo->wszVersion);
            }

            return (TRUE);
        }

        *pfFailedCheck = TRUE;
    }

    return (FALSE);
}

#define         OSATTR_ALL          L'X'
#define         OSATTR_GTEQ         L'>'
#define         OSATTR_LTEQ         L'-'
#define         OSATTR_LTEQ2        L'<'
#define         OSATTR_OSSEP        L':'
#define         OSATTR_VERSEP       L'.'
#define         OSATTR_SEP          L','

BOOL _FindVersionInAttr(DRIVER_VER_INFO *pVerInfo, OSVERSIONINFO *pVersion, CRYPTCATATTRIBUTE *pAttr)
{
    WCHAR   *pwszCurrent;
    WCHAR   *pwszEnd;
    BOOL    fRet;

    fRet        = FALSE;
    pwszCurrent = (WCHAR *)pAttr->pbValue;

    //  format:  os:major.minor, os:major.minor, ...
    while ((pwszCurrent) && (*pwszCurrent))
    {
        pwszEnd = wcschr(pwszCurrent, OSATTR_SEP);

        if (pwszEnd)
        {
            *pwszEnd = NULL;
        }

        if (fRet = _CheckVersion(pVerInfo, pVersion, pwszCurrent))
        {
            break;
        }

        if (!(pwszEnd))
        {
            break;
        }

        *pwszEnd = OSATTR_SEP;

        pwszCurrent = pwszEnd;

        pwszCurrent++;
    }

    return(fRet);
}

BOOL _CheckVersion(DRIVER_VER_INFO *pVerInfo, OSVERSIONINFO *pVersion, WCHAR *pwszAttr)
{
    WCHAR   *pwszCurrent;
    WCHAR   *pwszEnd;
    long    lMajor;
    long    lMinor;
    WCHAR   wcFlag;

    pwszCurrent = pwszAttr;

    //
    //  format:  os:major.minor, os:major.minor, ...
    //          2:4.x   = NT 4 (all)
    //          2:4.>   = NT 4 (all) and beyond
    //          2:4.-   = NT 4 (all) and before
    //          2:4.1.> = NT 4.1 (all) and beyond
    //          2:4.1.- = NT 4.1 (all) and before
    //          2:4.1   = NT 4.1 only
    //
    if (!(pwszEnd = wcschr(pwszAttr, OSATTR_OSSEP)))
    {
        return(FALSE);
    }

    *pwszEnd = NULL;

    if (_wtol(pwszCurrent) != (long)pVersion->dwPlatformId)
    {
        *pwszEnd = OSATTR_OSSEP;

        return(FALSE);
    }

    *pwszEnd = OSATTR_OSSEP;

    pwszCurrent = pwszEnd;

    pwszCurrent++;

    if (!(_FillVersionLongs(pwszCurrent, &lMajor, &lMinor, &wcFlag)))
    {
        return(FALSE);
    }

    if (lMinor == (-1))
    {
        //  2:4     = NT 4 only
        //  2:4.x   = NT 4 any
        if (lMajor == (long)pVersion->dwMajorVersion)
        {
            return(TRUE);
        }

        //  2:4.-   = NT 4 and previous
        //  2:4.<   = NT 4 and previous
        if ((lMajor > (long)pVersion->dwMajorVersion) &&
            ((wcFlag == OSATTR_LTEQ) || (wcFlag == OSATTR_LTEQ2)))
        {
            return(TRUE);
        }

        // 2:4.>    = NT 4 and greater
        if ((lMajor < (long)pVersion->dwMajorVersion) &&
            (wcFlag == OSATTR_GTEQ))
        {
            return(TRUE);
        }

        return(FALSE);
    }

    if (lMajor == (long)pVersion->dwMajorVersion)
    {
        // 2:4.1    = NT 4.1 only
        if (lMinor == (long)pVersion->dwMinorVersion)
        {
            return(TRUE);
        }

        //  2:4.1.-     = NT 4.1 and previous
        //  2:4.1.<     = NT 4.1 and previous
        if ((lMinor > (long)pVersion->dwMinorVersion) &&
            ((wcFlag == OSATTR_LTEQ) || (wcFlag == OSATTR_LTEQ2)))
        {
            return(TRUE);
        }

        //  2:4.1.>     = NT 4.1 and greater
        if ((lMinor < (long)pVersion->dwMinorVersion) &&
            (wcFlag == OSATTR_GTEQ))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

BOOL _FillVersionLongs(WCHAR *pwszMM, long *plMajor, long *plMinor, WCHAR *pwcFlag)
{
    //
    //  special characters:
    //      - = all versions less than or equal to
    //      < = all versions less than or equal to
    //      > = all versions greater than or equal to
    //      X = all sub-versions.
    //
    WCHAR   *pwszEnd;

    *plMinor = (-1L);

    if (pwszEnd = wcschr(pwszMM, OSATTR_VERSEP))
    {
        *pwszEnd = NULL;
    }

    *plMajor = _wtol(pwszMM);

    *pwszEnd = OSATTR_VERSEP;

    pwszMM = pwszEnd;

    pwszMM++;

    if (*pwszMM)
    {
       if ((*pwszMM == OSATTR_GTEQ) ||
           (*pwszMM == OSATTR_LTEQ) ||
           (*pwszMM == OSATTR_LTEQ2) ||
           (towupper(*pwszMM) == OSATTR_ALL))
        {
            *pwcFlag = towupper(*pwszMM);
            return(TRUE);
        }

        if (!(pwszEnd = wcschr(pwszMM, OSATTR_VERSEP)))
        {
            *plMinor = _wtol(pwszMM);
            return(TRUE);
        }

        *pwszEnd = NULL;

        *plMinor = _wtol(pwszMM);

        *pwszEnd = OSATTR_VERSEP;

        pwszMM = pwszEnd;

        pwszMM++;
    }
    else
    {
        return(TRUE);
    }

    if ((*pwszMM == OSATTR_GTEQ) ||
        (*pwszMM == OSATTR_LTEQ) ||
        (*pwszMM == OSATTR_LTEQ2) ||
        (towupper(*pwszMM) == OSATTR_ALL))
    {
        *pwcFlag = towupper(*pwszMM);
        return(TRUE);
    }

    return(TRUE);
}

STDAPI DriverRegisterServer(void)
{
    GUID                            gDriver = DRIVER_ACTION_VERIFY;
    CRYPT_REGISTER_ACTIONID         sRegAID;

    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    //  use our init policy
    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = DRIVER_INITPROV_FUNCTION;

    //  use standard object policy
    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = SP_OBJTRUST_FUNCTION;

    //  use standard signature policy
    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = SP_SIGTRUST_FUNCTION;

    //  use standard cert builder
    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION;

    //  use standard cert policy
    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = SP_CHKCERT_FUNCTION;

    //  use our final policy
    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = DRIVER_FINALPOLPROV_FUNCTION;

    //  use our cleanup policy
    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = DRIVER_CLEANUPPOLICY_FUNCTION;

    //
    //  Register our provider GUID...
    //
    if (!(WintrustAddActionID(&gDriver, 0, &sRegAID)))
    {
        return (S_FALSE);
    }

    return (S_OK);
}

STDAPI DriverUnregisterServer(void)
{
    GUID    gDriver = DRIVER_ACTION_VERIFY;

    if (!(WintrustRemoveActionID(&gDriver)))
    {
        return (S_FALSE);
    }

    return (S_OK);

}

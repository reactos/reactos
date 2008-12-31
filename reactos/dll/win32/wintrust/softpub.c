/*
 * Copyright 2007 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wintrust.h"
#include "mssip.h"
#include "softpub.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

HRESULT WINAPI SoftpubDefCertInit(CRYPT_PROVIDER_DATA *data)
{
    HRESULT ret = S_FALSE;

    TRACE("(%p)\n", data);

    if (data->padwTrustStepErrors &&
     !data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT])
        ret = S_OK;
    TRACE("returning %08x\n", ret);
    return ret;
}

HRESULT WINAPI SoftpubInitialize(CRYPT_PROVIDER_DATA *data)
{
    HRESULT ret = S_FALSE;

    TRACE("(%p)\n", data);

    if (data->padwTrustStepErrors &&
     !data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT])
        ret = S_OK;
    TRACE("returning %08x\n", ret);
    return ret;
}

/* Assumes data->pWintrustData->u.pFile exists.  Makes sure a file handle is
 * open for the file.
 */
static BOOL SOFTPUB_OpenFile(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret = TRUE;

    /* PSDK implies that all values should be initialized to NULL, so callers
     * typically have hFile as NULL rather than INVALID_HANDLE_VALUE.  Check
     * for both.
     */
    if (!data->pWintrustData->u.pFile->hFile ||
     data->pWintrustData->u.pFile->hFile == INVALID_HANDLE_VALUE)
    {
        data->pWintrustData->u.pFile->hFile =
            CreateFileW(data->pWintrustData->u.pFile->pcwszFilePath, GENERIC_READ,
          FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (data->pWintrustData->u.pFile->hFile != INVALID_HANDLE_VALUE)
            data->fOpenedFile = TRUE;
        else
            ret = FALSE;
    }
    if (ret)
        GetFileTime(data->pWintrustData->u.pFile->hFile, &data->sftSystemTime,
         NULL, NULL);
    TRACE("returning %d\n", ret);
    return ret;
}

/* Assumes data->pWintrustData->u.pFile exists.  Sets data->pPDSip->gSubject to
 * the file's subject GUID.
 */
static BOOL SOFTPUB_GetFileSubject(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    if (!data->pWintrustData->u.pFile->pgKnownSubject)
    {
        ret = CryptSIPRetrieveSubjectGuid(
         data->pWintrustData->u.pFile->pcwszFilePath,
         data->pWintrustData->u.pFile->hFile,
         &data->u.pPDSip->gSubject);
    }
    else
    {
        data->u.pPDSip->gSubject = *data->pWintrustData->u.pFile->pgKnownSubject;
        ret = TRUE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Assumes data->u.pPDSip exists, and its gSubject member set.
 * Allocates data->u.pPDSip->pSip and loads it, if possible.
 */
static BOOL SOFTPUB_GetSIP(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    data->u.pPDSip->pSip = data->psPfns->pfnAlloc(sizeof(SIP_DISPATCH_INFO));
    if (data->u.pPDSip->pSip)
        ret = CryptSIPLoad(&data->u.pPDSip->gSubject, 0, data->u.pPDSip->pSip);
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Assumes data->u.pPDSip has been loaded, and data->u.pPDSip->pSip allocated.
 * Calls data->u.pPDSip->pSip->pfGet to construct data->hMsg.
 */
static BOOL SOFTPUB_GetMessageFromFile(CRYPT_PROVIDER_DATA *data, HANDLE file,
 LPCWSTR filePath)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    data->u.pPDSip->psSipSubjectInfo =
     data->psPfns->pfnAlloc(sizeof(SIP_SUBJECTINFO));
    if (!data->u.pPDSip->psSipSubjectInfo)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    data->u.pPDSip->psSipSubjectInfo->cbSize = sizeof(SIP_SUBJECTINFO);
    data->u.pPDSip->psSipSubjectInfo->pgSubjectType = &data->u.pPDSip->gSubject;
    data->u.pPDSip->psSipSubjectInfo->hFile = file;
    data->u.pPDSip->psSipSubjectInfo->pwsFileName = filePath;
    data->u.pPDSip->psSipSubjectInfo->hProv = data->hProv;
    ret = data->u.pPDSip->pSip->pfGet(data->u.pPDSip->psSipSubjectInfo,
     &data->dwEncoding, 0, &size, 0);
    if (!ret)
    {
        SetLastError(TRUST_E_NOSIGNATURE);
        return FALSE;
    }

    buf = data->psPfns->pfnAlloc(size);
    if (!buf)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    ret = data->u.pPDSip->pSip->pfGet(data->u.pPDSip->psSipSubjectInfo,
     &data->dwEncoding, 0, &size, buf);
    if (ret)
    {
        data->hMsg = CryptMsgOpenToDecode(data->dwEncoding, 0, 0, data->hProv,
         NULL, NULL);
        if (data->hMsg)
            ret = CryptMsgUpdate(data->hMsg, buf, size, TRUE);
    }

    data->psPfns->pfnFree(buf);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL SOFTPUB_CreateStoreFromMessage(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret = FALSE;
    HCERTSTORE store;

    store = CertOpenStore(CERT_STORE_PROV_MSG, data->dwEncoding,
     data->hProv, CERT_STORE_NO_CRYPT_RELEASE_FLAG, data->hMsg);
    if (store)
    {
        ret = data->psPfns->pfnAddStore2Chain(data, store);
        CertCloseStore(store, 0);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static DWORD SOFTPUB_DecodeInnerContent(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    DWORD size;
    LPSTR oid = NULL;
    LPBYTE buf = NULL;

    ret = CryptMsgGetParam(data->hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, NULL,
     &size);
    if (!ret)
        goto error;
    oid = data->psPfns->pfnAlloc(size);
    if (!oid)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
        goto error;
    }
    ret = CryptMsgGetParam(data->hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, oid,
     &size);
    if (!ret)
        goto error;
    ret = CryptMsgGetParam(data->hMsg, CMSG_CONTENT_PARAM, 0, NULL, &size);
    if (!ret)
        goto error;
    buf = data->psPfns->pfnAlloc(size);
    if (!buf)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
        goto error;
    }
    ret = CryptMsgGetParam(data->hMsg, CMSG_CONTENT_PARAM, 0, buf, &size);
    if (!ret)
        goto error;
    ret = CryptDecodeObject(data->dwEncoding, oid, buf, size, 0, NULL, &size);
    if (!ret)
        goto error;
    data->u.pPDSip->psIndirectData = data->psPfns->pfnAlloc(size);
    if (!data->u.pPDSip->psIndirectData)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
        goto error;
    }
    ret = CryptDecodeObject(data->dwEncoding, oid, buf, size, 0,
     data->u.pPDSip->psIndirectData, &size);

error:
    TRACE("returning %d\n", ret);
    data->psPfns->pfnFree(oid);
    data->psPfns->pfnFree(buf);
    return ret;
}

static BOOL SOFTPUB_LoadCertMessage(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    if (data->pWintrustData->u.pCert &&
     data->pWintrustData->u.pCert->cbStruct == sizeof(WINTRUST_CERT_INFO))
    {
        if (data->psPfns)
        {
            CRYPT_PROVIDER_SGNR signer = { sizeof(signer), { 0 } };
            DWORD i;

            /* Add a signer with nothing but the time to verify, so we can
             * add a cert to it
             */
            if (data->pWintrustData->u.pCert->psftVerifyAsOf)
                data->sftSystemTime = signer.sftVerifyAsOf;
            else
            {
                SYSTEMTIME sysTime;

                GetSystemTime(&sysTime);
                SystemTimeToFileTime(&sysTime, &signer.sftVerifyAsOf);
            }
            ret = data->psPfns->pfnAddSgnr2Chain(data, FALSE, 0, &signer);
            if (ret)
            {
                ret = data->psPfns->pfnAddCert2Chain(data, 0, FALSE, 0,
                 data->pWintrustData->u.pCert->psCertContext);
                for (i = 0; ret && i < data->pWintrustData->u.pCert->chStores;
                 i++)
                    ret = data->psPfns->pfnAddStore2Chain(data,
                     data->pWintrustData->u.pCert->pahStores[i]);
            }
        }
        else
        {
            /* Do nothing!?  See the tests */
            ret = TRUE;
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
    }
    return ret;
}

static BOOL SOFTPUB_LoadFileMessage(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    if (!data->pWintrustData->u.pFile)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
        goto error;
    }
    ret = SOFTPUB_OpenFile(data);
    if (!ret)
        goto error;
    ret = SOFTPUB_GetFileSubject(data);
    if (!ret)
        goto error;
    ret = SOFTPUB_GetSIP(data);
    if (!ret)
        goto error;
    ret = SOFTPUB_GetMessageFromFile(data, data->pWintrustData->u.pFile->hFile,
     data->pWintrustData->u.pFile->pcwszFilePath);
    if (!ret)
        goto error;
    ret = SOFTPUB_CreateStoreFromMessage(data);
    if (!ret)
        goto error;
    ret = SOFTPUB_DecodeInnerContent(data);
error:
    return ret;
}

static BOOL SOFTPUB_LoadCatalogMessage(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    HANDLE catalog = INVALID_HANDLE_VALUE;

    if (!data->pWintrustData->u.pCatalog)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    catalog = CreateFileW(data->pWintrustData->u.pCatalog->pcwszCatalogFilePath,
     GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
     NULL);
    if (catalog == INVALID_HANDLE_VALUE)
        return FALSE;
    ret = CryptSIPRetrieveSubjectGuid(
     data->pWintrustData->u.pCatalog->pcwszCatalogFilePath, catalog,
     &data->u.pPDSip->gSubject);
    if (!ret)
        goto error;
    ret = SOFTPUB_GetSIP(data);
    if (!ret)
        goto error;
    ret = SOFTPUB_GetMessageFromFile(data, catalog,
     data->pWintrustData->u.pCatalog->pcwszCatalogFilePath);
    if (!ret)
        goto error;
    ret = SOFTPUB_CreateStoreFromMessage(data);
    if (!ret)
        goto error;
    ret = SOFTPUB_DecodeInnerContent(data);
    /* FIXME: this loads the catalog file, but doesn't validate the member. */
error:
    CloseHandle(catalog);
    return ret;
}

HRESULT WINAPI SoftpubLoadMessage(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    TRACE("(%p)\n", data);

    if (!data->padwTrustStepErrors)
        return S_FALSE;

    switch (data->pWintrustData->dwUnionChoice)
    {
    case WTD_CHOICE_CERT:
        ret = SOFTPUB_LoadCertMessage(data);
        break;
    case WTD_CHOICE_FILE:
        ret = SOFTPUB_LoadFileMessage(data);
        break;
    case WTD_CHOICE_CATALOG:
        ret = SOFTPUB_LoadCatalogMessage(data);
        break;
    default:
        FIXME("unimplemented for %d\n", data->pWintrustData->dwUnionChoice);
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
    }

    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] =
         GetLastError();
    TRACE("returning %d (%08x)\n", ret ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
    return ret ? S_OK : S_FALSE;
}

static CMSG_SIGNER_INFO *WINTRUST_GetSigner(CRYPT_PROVIDER_DATA *data,
 DWORD signerIdx)
{
    BOOL ret;
    CMSG_SIGNER_INFO *signerInfo = NULL;
    DWORD size;

    ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_INFO_PARAM, signerIdx,
     NULL, &size);
    if (ret)
    {
        signerInfo = data->psPfns->pfnAlloc(size);
        if (signerInfo)
        {
            ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_INFO_PARAM,
             signerIdx, signerInfo, &size);
            if (!ret)
            {
                data->psPfns->pfnFree(signerInfo);
                signerInfo = NULL;
            }
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
    }
    return signerInfo;
}

static BOOL WINTRUST_SaveSigner(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    BOOL ret;
    CMSG_SIGNER_INFO *signerInfo = WINTRUST_GetSigner(data, signerIdx);

    if (signerInfo)
    {
        CRYPT_PROVIDER_SGNR sgnr = { sizeof(sgnr), { 0 } };

        sgnr.psSigner = signerInfo;
        sgnr.sftVerifyAsOf = data->sftSystemTime;
        ret = data->psPfns->pfnAddSgnr2Chain(data, FALSE, signerIdx, &sgnr);
    }
    else
        ret = FALSE;
    return ret;
}

static CERT_INFO *WINTRUST_GetSignerCertInfo(CRYPT_PROVIDER_DATA *data,
 DWORD signerIdx)
{
    BOOL ret;
    CERT_INFO *certInfo = NULL;
    DWORD size;

    ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_CERT_INFO_PARAM, signerIdx,
     NULL, &size);
    if (ret)
    {
        certInfo = data->psPfns->pfnAlloc(size);
        if (certInfo)
        {
            ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_CERT_INFO_PARAM,
             signerIdx, certInfo, &size);
            if (!ret)
            {
                data->psPfns->pfnFree(certInfo);
                certInfo = NULL;
            }
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
    }
    return certInfo;
}

static BOOL WINTRUST_VerifySigner(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    BOOL ret;
    CERT_INFO *certInfo = WINTRUST_GetSignerCertInfo(data, signerIdx);

    if (certInfo)
    {
        PCCERT_CONTEXT subject = CertGetSubjectCertificateFromStore(
         data->pahStores[0], data->dwEncoding, certInfo);

        if (subject)
        {
            CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA para = { sizeof(para), 0,
             signerIdx, CMSG_VERIFY_SIGNER_CERT, (LPVOID)subject };

            ret = CryptMsgControl(data->hMsg, 0, CMSG_CTRL_VERIFY_SIGNATURE_EX,
             &para);
            if (!ret)
                SetLastError(TRUST_E_CERT_SIGNATURE);
            else
                data->psPfns->pfnAddCert2Chain(data, signerIdx, FALSE, 0,
                 subject);
            CertFreeCertificateContext(subject);
        }
        else
        {
            SetLastError(TRUST_E_NO_SIGNER_CERT);
            ret = FALSE;
        }
        data->psPfns->pfnFree(certInfo);
    }
    else
        ret = FALSE;
    return ret;
}

HRESULT WINAPI SoftpubLoadSignature(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    TRACE("(%p)\n", data);

    if (!data->padwTrustStepErrors)
        return S_FALSE;

    if (data->hMsg)
    {
        DWORD signerCount, size;

        size = sizeof(signerCount);
        ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_COUNT_PARAM, 0,
         &signerCount, &size);
        if (ret)
        {
            DWORD i;

            for (i = 0; ret && i < signerCount; i++)
            {
                if ((ret = WINTRUST_SaveSigner(data, i)))
                    ret = WINTRUST_VerifySigner(data, i);
            }
        }
        else
            SetLastError(TRUST_E_NOSIGNATURE);
    }
    else
        ret = TRUE;
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] =
         GetLastError();
    return ret ? S_OK : S_FALSE;
}

static DWORD WINTRUST_TrustStatusToConfidence(DWORD errorStatus)
{
    DWORD confidence = 0;

    confidence = 0;
    if (!(errorStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID))
        confidence |= CERT_CONFIDENCE_SIG;
    if (!(errorStatus & CERT_TRUST_IS_NOT_TIME_VALID))
        confidence |= CERT_CONFIDENCE_TIME;
    if (!(errorStatus & CERT_TRUST_IS_NOT_TIME_NESTED))
        confidence |= CERT_CONFIDENCE_TIMENEST;
    return confidence;
}

BOOL WINAPI SoftpubCheckCert(CRYPT_PROVIDER_DATA *data, DWORD idxSigner,
 BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    BOOL ret;

    TRACE("(%p, %d, %d, %d)\n", data, idxSigner, fCounterSignerChain,
     idxCounterSigner);

    if (fCounterSignerChain)
    {
        FIXME("unimplemented for counter signers\n");
        ret = FALSE;
    }
    else
    {
        PCERT_SIMPLE_CHAIN simpleChain =
         data->pasSigners[idxSigner].pChainContext->rgpChain[0];
        DWORD i;

        ret = TRUE;
        for (i = 0; i < simpleChain->cElement; i++)
        {
            /* Set confidence */
            data->pasSigners[idxSigner].pasCertChain[i].dwConfidence =
             WINTRUST_TrustStatusToConfidence(
             simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus);
            /* Set additional flags */
            if (!(simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_UNTRUSTED_ROOT))
                data->pasSigners[idxSigner].pasCertChain[i].fTrustedRoot = TRUE;
            if (simpleChain->rgpElement[i]->TrustStatus.dwInfoStatus &
             CERT_TRUST_IS_SELF_SIGNED)
                data->pasSigners[idxSigner].pasCertChain[i].fSelfSigned = TRUE;
            if (simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_CYCLIC)
                data->pasSigners[idxSigner].pasCertChain[i].fIsCyclic = TRUE;
        }
    }
    return ret;
}

static DWORD WINTRUST_TrustStatusToError(DWORD errorStatus)
{
    DWORD error;

    if (errorStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID)
        error = TRUST_E_CERT_SIGNATURE;
    else if (errorStatus & CERT_TRUST_IS_UNTRUSTED_ROOT)
        error = CERT_E_UNTRUSTEDROOT;
    else if (errorStatus & CERT_TRUST_IS_NOT_TIME_VALID)
        error = CERT_E_EXPIRED;
    else if (errorStatus & CERT_TRUST_IS_NOT_TIME_NESTED)
        error = CERT_E_VALIDITYPERIODNESTING;
    else if (errorStatus & CERT_TRUST_IS_REVOKED)
        error = CERT_E_REVOKED;
    else if (errorStatus & CERT_TRUST_IS_OFFLINE_REVOCATION ||
     errorStatus & CERT_TRUST_REVOCATION_STATUS_UNKNOWN)
        error = CERT_E_REVOCATION_FAILURE;
    else if (errorStatus & CERT_TRUST_IS_NOT_VALID_FOR_USAGE)
        error = CERT_E_WRONG_USAGE;
    else if (errorStatus & CERT_TRUST_IS_CYCLIC)
        error = CERT_E_CHAINING;
    else if (errorStatus & CERT_TRUST_INVALID_EXTENSION)
        error = CERT_E_CRITICAL;
    else if (errorStatus & CERT_TRUST_INVALID_POLICY_CONSTRAINTS)
        error = CERT_E_INVALID_POLICY;
    else if (errorStatus & CERT_TRUST_INVALID_BASIC_CONSTRAINTS)
        error = TRUST_E_BASIC_CONSTRAINTS;
    else if (errorStatus & CERT_TRUST_INVALID_NAME_CONSTRAINTS ||
     errorStatus & CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT ||
     errorStatus & CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT ||
     errorStatus & CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT ||
     errorStatus & CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT)
        error = CERT_E_INVALID_NAME;
    else if (errorStatus & CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY)
        error = CERT_E_INVALID_POLICY;
    else if (errorStatus)
    {
        FIXME("unknown error status %08x\n", errorStatus);
        error = TRUST_E_SYSTEM_ERROR;
    }
    else
        error = S_OK;
    return error;
}

static BOOL WINTRUST_CopyChain(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    BOOL ret;
    PCERT_SIMPLE_CHAIN simpleChain =
     data->pasSigners[signerIdx].pChainContext->rgpChain[0];
    DWORD i;

    data->pasSigners[signerIdx].pasCertChain[0].dwConfidence =
     WINTRUST_TrustStatusToConfidence(
     simpleChain->rgpElement[0]->TrustStatus.dwErrorStatus);
    data->pasSigners[signerIdx].pasCertChain[0].pChainElement =
     simpleChain->rgpElement[0];
    ret = TRUE;
    for (i = 1; ret && i < simpleChain->cElement; i++)
    {
        ret = data->psPfns->pfnAddCert2Chain(data, signerIdx, FALSE, 0,
         simpleChain->rgpElement[i]->pCertContext);
        if (ret)
        {
            data->pasSigners[signerIdx].pasCertChain[i].pChainElement =
             simpleChain->rgpElement[i];
            data->pasSigners[signerIdx].pasCertChain[i].dwConfidence =
             WINTRUST_TrustStatusToConfidence(
             simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus);
        }
    }
    data->pasSigners[signerIdx].pasCertChain[simpleChain->cElement - 1].dwError
     = WINTRUST_TrustStatusToError(
     simpleChain->rgpElement[simpleChain->cElement - 1]->
     TrustStatus.dwErrorStatus);
    return ret;
}

static void WINTRUST_CreateChainPolicyCreateInfo(
 const CRYPT_PROVIDER_DATA *data, PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO info,
 PCERT_CHAIN_PARA chainPara)
{
    chainPara->cbSize = sizeof(CERT_CHAIN_PARA);
    if (data->pRequestUsage)
        chainPara->RequestedUsage = *data->pRequestUsage;
    else
    {
        chainPara->RequestedUsage.dwType = 0;
        chainPara->RequestedUsage.Usage.cUsageIdentifier = 0;
    }
    info->u.cbSize = sizeof(WTD_GENERIC_CHAIN_POLICY_CREATE_INFO);
    info->hChainEngine = NULL;
    info->pChainPara = chainPara;
    if (data->dwProvFlags & CPD_REVOCATION_CHECK_END_CERT)
        info->dwFlags = CERT_CHAIN_REVOCATION_CHECK_END_CERT;
    else if (data->dwProvFlags & CPD_REVOCATION_CHECK_CHAIN)
        info->dwFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    else if (data->dwProvFlags & CPD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT)
        info->dwFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    else
        info->dwFlags = 0;
    info->pvReserved = NULL;
}

static BOOL WINTRUST_CreateChainForSigner(CRYPT_PROVIDER_DATA *data,
 DWORD signer, PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO createInfo,
 PCERT_CHAIN_PARA chainPara)
{
    BOOL ret = TRUE;
    HCERTSTORE store = NULL;

    if (data->chStores)
    {
        store = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);
        if (store)
        {
            DWORD i;

            for (i = 0; i < data->chStores; i++)
                CertAddStoreToCollection(store, data->pahStores[i], 0, 0);
        }
    }
    /* Expect the end certificate for each signer to be the only cert in the
     * chain:
     */
    if (data->pasSigners[signer].csCertChain)
    {
        /* Create a certificate chain for each signer */
        ret = CertGetCertificateChain(createInfo->hChainEngine,
         data->pasSigners[signer].pasCertChain[0].pCert,
         &data->pasSigners[signer].sftVerifyAsOf, store,
         chainPara, createInfo->dwFlags, createInfo->pvReserved,
         &data->pasSigners[signer].pChainContext);
        if (ret)
        {
            if (data->pasSigners[signer].pChainContext->cChain != 1)
            {
                FIXME("unimplemented for more than 1 simple chain\n");
                ret = FALSE;
            }
            else
            {
                if ((ret = WINTRUST_CopyChain(data, signer)))
                {
                    if (data->psPfns->pfnCertCheckPolicy)
                        ret = data->psPfns->pfnCertCheckPolicy(data, signer,
                         FALSE, 0);
                    else
                        TRACE("no cert check policy, skipping policy check\n");
                }
            }
        }
    }
    CertCloseStore(store, 0);
    return ret;
}

HRESULT WINAPI WintrustCertificateTrust(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    TRACE("(%p)\n", data);

    if (!data->csSigners)
    {
        ret = FALSE;
        SetLastError(TRUST_E_NOSIGNATURE);
    }
    else
    {
        DWORD i;
        WTD_GENERIC_CHAIN_POLICY_CREATE_INFO createInfo;
        CERT_CHAIN_PARA chainPara;

        WINTRUST_CreateChainPolicyCreateInfo(data, &createInfo, &chainPara);
        ret = TRUE;
        for (i = 0; i < data->csSigners; i++)
            ret = WINTRUST_CreateChainForSigner(data, i, &createInfo,
             &chainPara);
    }
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
         GetLastError();
    TRACE("returning %d (%08x)\n", ret ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    return ret ? S_OK : S_FALSE;
}

HRESULT WINAPI GenericChainCertificateTrust(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    WTD_GENERIC_CHAIN_POLICY_DATA *policyData =
     (WTD_GENERIC_CHAIN_POLICY_DATA *)data->pWintrustData->pPolicyCallbackData;

    TRACE("(%p)\n", data);

    if (policyData && policyData->u.cbSize !=
     sizeof(WTD_GENERIC_CHAIN_POLICY_CREATE_INFO))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
        goto end;
    }
    if (!data->csSigners)
    {
        ret = FALSE;
        SetLastError(TRUST_E_NOSIGNATURE);
    }
    else
    {
        DWORD i;
        WTD_GENERIC_CHAIN_POLICY_CREATE_INFO createInfo, *pCreateInfo;
        CERT_CHAIN_PARA chainPara, *pChainPara;

        if (policyData)
        {
            pCreateInfo = policyData->pSignerChainInfo;
            pChainPara = pCreateInfo->pChainPara;
        }
        else
        {
            WINTRUST_CreateChainPolicyCreateInfo(data, &createInfo, &chainPara);
            pChainPara = &chainPara;
            pCreateInfo = &createInfo;
        }
        ret = TRUE;
        for (i = 0; i < data->csSigners; i++)
            ret = WINTRUST_CreateChainForSigner(data, i, pCreateInfo,
             pChainPara);
    }

end:
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
         GetLastError();
    TRACE("returning %d (%08x)\n", ret ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    return ret ? S_OK : S_FALSE;
}

HRESULT WINAPI SoftpubAuthenticode(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    CERT_CHAIN_POLICY_STATUS policyStatus = { sizeof(policyStatus), 0 };

    TRACE("(%p)\n", data);

    if (data->pWintrustData->dwUIChoice != WTD_UI_NONE)
        FIXME("unimplemented for UI choice %d\n",
         data->pWintrustData->dwUIChoice);
    if (!data->csSigners)
    {
        ret = FALSE;
        policyStatus.dwError = TRUST_E_NOSIGNATURE;
    }
    else
    {
        DWORD i;

        ret = TRUE;
        for (i = 0; ret && i < data->csSigners; i++)
        {
            BYTE hash[20];
            DWORD size = sizeof(hash);

            /* First make sure cert isn't disallowed */
            if ((ret = CertGetCertificateContextProperty(
             data->pasSigners[i].pasCertChain[0].pCert,
             CERT_SIGNATURE_HASH_PROP_ID, hash, &size)))
            {
                static const WCHAR disallowedW[] =
                 { 'D','i','s','a','l','l','o','w','e','d',0 };
                HCERTSTORE disallowed = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                 X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER,
                 disallowedW);

                if (disallowed)
                {
                    PCCERT_CONTEXT found = CertFindCertificateInStore(
                     disallowed, X509_ASN_ENCODING, 0, CERT_FIND_SIGNATURE_HASH,
                     hash, NULL);

                    if (found)
                    {
                        /* Disallowed!  Can't verify it. */
                        policyStatus.dwError = TRUST_E_SUBJECT_NOT_TRUSTED;
                        ret = FALSE;
                        CertFreeCertificateContext(found);
                    }
                    CertCloseStore(disallowed, 0);
                }
            }
            if (ret)
            {
                CERT_CHAIN_POLICY_PARA policyPara = { sizeof(policyPara), 0 };

                if (data->dwRegPolicySettings & WTPF_TRUSTTEST)
                    policyPara.dwFlags |= CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG;
                if (data->dwRegPolicySettings & WTPF_TESTCANBEVALID)
                    policyPara.dwFlags |= CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG;
                if (data->dwRegPolicySettings & WTPF_IGNOREEXPIRATION)
                    policyPara.dwFlags |=
                     CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG |
                     CERT_CHAIN_POLICY_IGNORE_CTL_NOT_TIME_VALID_FLAG |
                     CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;
                if (data->dwRegPolicySettings & WTPF_IGNOREREVOKATION)
                    policyPara.dwFlags |=
                     CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG |
                     CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG |
                     CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG |
                     CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG;
                CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_AUTHENTICODE,
                 data->pasSigners[i].pChainContext, &policyPara, &policyStatus);
                if (policyStatus.dwError != NO_ERROR)
                    ret = FALSE;
            }
        }
    }
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] =
         policyStatus.dwError;
    TRACE("returning %d (%08x)\n", ret ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV]);
    return ret ? S_OK : S_FALSE;
}

static HRESULT WINAPI WINTRUST_DefaultPolicy(CRYPT_PROVIDER_DATA *pProvData,
 DWORD dwStepError, DWORD dwRegPolicySettings, DWORD cSigner,
 PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO rgpSigner, void *pvPolicyArg)
{
    DWORD i;
    CERT_CHAIN_POLICY_STATUS policyStatus = { sizeof(policyStatus), 0 };

    for (i = 0; !policyStatus.dwError && i < cSigner; i++)
    {
        CERT_CHAIN_POLICY_PARA policyPara = { sizeof(policyPara), 0 };

        if (dwRegPolicySettings & WTPF_IGNOREEXPIRATION)
            policyPara.dwFlags |=
             CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG |
             CERT_CHAIN_POLICY_IGNORE_CTL_NOT_TIME_VALID_FLAG |
             CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;
        if (dwRegPolicySettings & WTPF_IGNOREREVOKATION)
            policyPara.dwFlags |=
             CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG |
             CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG |
             CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG |
             CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG;
        CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,
         rgpSigner[i].pChainContext, &policyPara, &policyStatus);
    }
    return policyStatus.dwError;
}

HRESULT WINAPI GenericChainFinalProv(CRYPT_PROVIDER_DATA *data)
{
    HRESULT err = NO_ERROR; /* not a typo, MS confused the types */
    WTD_GENERIC_CHAIN_POLICY_DATA *policyData =
     (WTD_GENERIC_CHAIN_POLICY_DATA *)data->pWintrustData->pPolicyCallbackData;

    TRACE("(%p)\n", data);

    if (data->pWintrustData->dwUIChoice != WTD_UI_NONE)
        FIXME("unimplemented for UI choice %d\n",
         data->pWintrustData->dwUIChoice);
    if (!data->csSigners)
        err = TRUST_E_NOSIGNATURE;
    else
    {
        PFN_WTD_GENERIC_CHAIN_POLICY_CALLBACK policyCallback;
        void *policyArg;
        WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *signers = NULL;

        if (policyData)
        {
            policyCallback = policyData->pfnPolicyCallback;
            policyArg = policyData->pvPolicyArg;
        }
        else
        {
            policyCallback = WINTRUST_DefaultPolicy;
            policyArg = NULL;
        }
        if (data->csSigners)
        {
            DWORD i;

            signers = data->psPfns->pfnAlloc(
             data->csSigners * sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO));
            if (signers)
            {
                for (i = 0; i < data->csSigners; i++)
                {
                    signers[i].u.cbSize =
                     sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO);
                    signers[i].pChainContext =
                     data->pasSigners[i].pChainContext;
                    signers[i].dwSignerType = data->pasSigners[i].dwSignerType;
                    signers[i].pMsgSignerInfo = data->pasSigners[i].psSigner;
                    signers[i].dwError = data->pasSigners[i].dwError;
                    if (data->pasSigners[i].csCounterSigners)
                        FIXME("unimplemented for counter signers\n");
                    signers[i].cCounterSigner = 0;
                    signers[i].rgpCounterSigner = NULL;
                }
            }
            else
                err = ERROR_OUTOFMEMORY;
        }
        if (err == NO_ERROR)
            err = policyCallback(data, TRUSTERROR_STEP_FINAL_POLICYPROV,
             data->dwRegPolicySettings, data->csSigners, signers, policyArg);
        data->psPfns->pfnFree(signers);
    }
    if (err != NO_ERROR)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] = err;
    TRACE("returning %d (%08x)\n", err == NO_ERROR ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV]);
    return err == NO_ERROR ? S_OK : S_FALSE;
}

HRESULT WINAPI SoftpubCleanup(CRYPT_PROVIDER_DATA *data)
{
    DWORD i, j;

    for (i = 0; i < data->csSigners; i++)
    {
        for (j = 0; j < data->pasSigners[i].csCertChain; j++)
            CertFreeCertificateContext(data->pasSigners[i].pasCertChain[j].pCert);
        data->psPfns->pfnFree(data->pasSigners[i].pasCertChain);
        data->psPfns->pfnFree(data->pasSigners[i].psSigner);
        CertFreeCertificateChain(data->pasSigners[i].pChainContext);
    }
    data->psPfns->pfnFree(data->pasSigners);

    for (i = 0; i < data->chStores; i++)
        CertCloseStore(data->pahStores[i], 0);
    data->psPfns->pfnFree(data->pahStores);

    if (data->u.pPDSip)
    {
        data->psPfns->pfnFree(data->u.pPDSip->pSip);
        data->psPfns->pfnFree(data->u.pPDSip->pCATSip);
        data->psPfns->pfnFree(data->u.pPDSip->psSipSubjectInfo);
        data->psPfns->pfnFree(data->u.pPDSip->psSipCATSubjectInfo);
        data->psPfns->pfnFree(data->u.pPDSip->psIndirectData);
    }

    CryptMsgClose(data->hMsg);

    if (data->fOpenedFile)
        CloseHandle(data->pWintrustData->u.pFile->hFile);

    return S_OK;
}

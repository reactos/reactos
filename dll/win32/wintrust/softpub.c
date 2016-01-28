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

#include "wintrust_priv.h"

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

HRESULT WINAPI DriverInitializePolicy(CRYPT_PROVIDER_DATA *data)
{
    FIXME("stub\n");
    return S_OK;
}

HRESULT WINAPI DriverCleanupPolicy(CRYPT_PROVIDER_DATA *data)
{
    FIXME("stub\n");
    return S_OK;
}

HRESULT WINAPI DriverFinalPolicy(CRYPT_PROVIDER_DATA *data)
{
    FIXME("stub\n");
    return S_OK;
}

/* Assumes data->pWintrustData->u.pFile exists.  Makes sure a file handle is
 * open for the file.
 */
static DWORD SOFTPUB_OpenFile(CRYPT_PROVIDER_DATA *data)
{
    DWORD err = ERROR_SUCCESS;

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
            err = GetLastError();
    }
    if (!err)
        GetFileTime(data->pWintrustData->u.pFile->hFile, &data->sftSystemTime,
         NULL, NULL);
    TRACE("returning %d\n", err);
    return err;
}

/* Assumes data->pWintrustData->u.pFile exists.  Sets data->pPDSip->gSubject to
 * the file's subject GUID.
 */
static DWORD SOFTPUB_GetFileSubject(CRYPT_PROVIDER_DATA *data)
{
    DWORD err = ERROR_SUCCESS;

    if (!WVT_ISINSTRUCT(WINTRUST_FILE_INFO,
     data->pWintrustData->u.pFile->cbStruct, pgKnownSubject) ||
     !data->pWintrustData->u.pFile->pgKnownSubject)
    {
        if (!CryptSIPRetrieveSubjectGuid(
         data->pWintrustData->u.pFile->pcwszFilePath,
         data->pWintrustData->u.pFile->hFile,
         &data->u.pPDSip->gSubject))
        {
            LARGE_INTEGER fileSize;
            DWORD sipError = GetLastError();

            /* Special case for empty files: the error is expected to be
             * TRUST_E_SUBJECT_FORM_UNKNOWN, rather than whatever
             * CryptSIPRetrieveSubjectGuid returns.
             */
            if (GetFileSizeEx(data->pWintrustData->u.pFile->hFile, &fileSize)
             && !fileSize.QuadPart)
                err = TRUST_E_SUBJECT_FORM_UNKNOWN;
            else
                err = sipError;
        }
    }
    else
        data->u.pPDSip->gSubject = *data->pWintrustData->u.pFile->pgKnownSubject;
    TRACE("returning %d\n", err);
    return err;
}

/* Assumes data->u.pPDSip exists, and its gSubject member set.
 * Allocates data->u.pPDSip->pSip and loads it, if possible.
 */
static DWORD SOFTPUB_GetSIP(CRYPT_PROVIDER_DATA *data)
{
    DWORD err = ERROR_SUCCESS;

    data->u.pPDSip->pSip = data->psPfns->pfnAlloc(sizeof(SIP_DISPATCH_INFO));
    if (data->u.pPDSip->pSip)
    {
        if (!CryptSIPLoad(&data->u.pPDSip->gSubject, 0, data->u.pPDSip->pSip))
            err = GetLastError();
    }
    else
        err = ERROR_OUTOFMEMORY;
    TRACE("returning %d\n", err);
    return err;
}

/* Assumes data->u.pPDSip has been loaded, and data->u.pPDSip->pSip allocated.
 * Calls data->u.pPDSip->pSip->pfGet to construct data->hMsg.
 */
static DWORD SOFTPUB_GetMessageFromFile(CRYPT_PROVIDER_DATA *data, HANDLE file,
 LPCWSTR filePath)
{
    DWORD err = ERROR_SUCCESS;
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    data->u.pPDSip->psSipSubjectInfo =
     data->psPfns->pfnAlloc(sizeof(SIP_SUBJECTINFO));
    if (!data->u.pPDSip->psSipSubjectInfo)
        return ERROR_OUTOFMEMORY;

    data->u.pPDSip->psSipSubjectInfo->cbSize = sizeof(SIP_SUBJECTINFO);
    data->u.pPDSip->psSipSubjectInfo->pgSubjectType = &data->u.pPDSip->gSubject;
    data->u.pPDSip->psSipSubjectInfo->hFile = file;
    data->u.pPDSip->psSipSubjectInfo->pwsFileName = filePath;
    data->u.pPDSip->psSipSubjectInfo->hProv = data->hProv;
    ret = data->u.pPDSip->pSip->pfGet(data->u.pPDSip->psSipSubjectInfo,
     &data->dwEncoding, 0, &size, 0);
    if (!ret)
        return TRUST_E_NOSIGNATURE;

    buf = data->psPfns->pfnAlloc(size);
    if (!buf)
        return ERROR_OUTOFMEMORY;

    ret = data->u.pPDSip->pSip->pfGet(data->u.pPDSip->psSipSubjectInfo,
     &data->dwEncoding, 0, &size, buf);
    if (ret)
    {
        data->hMsg = CryptMsgOpenToDecode(data->dwEncoding, 0, 0, data->hProv,
         NULL, NULL);
        if (data->hMsg)
        {
            ret = CryptMsgUpdate(data->hMsg, buf, size, TRUE);
            if (!ret)
                err = GetLastError();
        }
    }
    else
        err = GetLastError();

    data->psPfns->pfnFree(buf);
    TRACE("returning %d\n", err);
    return err;
}

static DWORD SOFTPUB_CreateStoreFromMessage(CRYPT_PROVIDER_DATA *data)
{
    DWORD err = ERROR_SUCCESS;
    HCERTSTORE store;

    store = CertOpenStore(CERT_STORE_PROV_MSG, data->dwEncoding,
     data->hProv, CERT_STORE_NO_CRYPT_RELEASE_FLAG, data->hMsg);
    if (store)
    {
        if (!data->psPfns->pfnAddStore2Chain(data, store))
            err = GetLastError();
        CertCloseStore(store, 0);
    }
    else
        err = GetLastError();
    TRACE("returning %d\n", err);
    return err;
}

static DWORD SOFTPUB_DecodeInnerContent(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    DWORD size, err = ERROR_SUCCESS;
    LPSTR oid = NULL;
    LPBYTE buf = NULL;

    ret = CryptMsgGetParam(data->hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, NULL,
     &size);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }
    oid = data->psPfns->pfnAlloc(size);
    if (!oid)
    {
        err = ERROR_OUTOFMEMORY;
        goto error;
    }
    ret = CryptMsgGetParam(data->hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, oid,
     &size);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }
    ret = CryptMsgGetParam(data->hMsg, CMSG_CONTENT_PARAM, 0, NULL, &size);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }
    buf = data->psPfns->pfnAlloc(size);
    if (!buf)
    {
        err = ERROR_OUTOFMEMORY;
        goto error;
    }
    ret = CryptMsgGetParam(data->hMsg, CMSG_CONTENT_PARAM, 0, buf, &size);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }
    ret = CryptDecodeObject(data->dwEncoding, oid, buf, size, 0, NULL, &size);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }
    data->u.pPDSip->psIndirectData = data->psPfns->pfnAlloc(size);
    if (!data->u.pPDSip->psIndirectData)
    {
        err = ERROR_OUTOFMEMORY;
        goto error;
    }
    ret = CryptDecodeObject(data->dwEncoding, oid, buf, size, 0,
     data->u.pPDSip->psIndirectData, &size);
    if (!ret)
        err = GetLastError();

error:
    TRACE("returning %d\n", err);
    data->psPfns->pfnFree(oid);
    data->psPfns->pfnFree(buf);
    return err;
}

static DWORD SOFTPUB_LoadCertMessage(CRYPT_PROVIDER_DATA *data)
{
    DWORD err = ERROR_SUCCESS;

    if (data->pWintrustData->u.pCert &&
     WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_CERT_INFO,
     data->pWintrustData->u.pCert->cbStruct, psCertContext))
    {
        if (data->psPfns)
        {
            CRYPT_PROVIDER_SGNR signer = { sizeof(signer), { 0 } };
            DWORD i;
            BOOL ret;

            /* Add a signer with nothing but the time to verify, so we can
             * add a cert to it
             */
            if (WVT_ISINSTRUCT(WINTRUST_CERT_INFO,
             data->pWintrustData->u.pCert->cbStruct, psftVerifyAsOf) &&
             data->pWintrustData->u.pCert->psftVerifyAsOf)
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
                if (WVT_ISINSTRUCT(WINTRUST_CERT_INFO,
                 data->pWintrustData->u.pCert->cbStruct, pahStores))
                        for (i = 0;
                         ret && i < data->pWintrustData->u.pCert->chStores; i++)
                            ret = data->psPfns->pfnAddStore2Chain(data,
                             data->pWintrustData->u.pCert->pahStores[i]);
            }
            if (!ret)
                err = GetLastError();
        }
    }
    else
        err = ERROR_INVALID_PARAMETER;
    return err;
}

static DWORD SOFTPUB_LoadFileMessage(CRYPT_PROVIDER_DATA *data)
{
    DWORD err = ERROR_SUCCESS;

    if (!data->pWintrustData->u.pFile)
    {
        err = ERROR_INVALID_PARAMETER;
        goto error;
    }
    err = SOFTPUB_OpenFile(data);
    if (err)
        goto error;
    err = SOFTPUB_GetFileSubject(data);
    if (err)
        goto error;
    err = SOFTPUB_GetSIP(data);
    if (err)
        goto error;
    err = SOFTPUB_GetMessageFromFile(data, data->pWintrustData->u.pFile->hFile,
     data->pWintrustData->u.pFile->pcwszFilePath);
    if (err)
        goto error;
    err = SOFTPUB_CreateStoreFromMessage(data);
    if (err)
        goto error;
    err = SOFTPUB_DecodeInnerContent(data);

error:
    if (err && data->fOpenedFile && data->pWintrustData->u.pFile)
    {
        /* The caller won't expect the file to be open on failure, so close it.
         */
        CloseHandle(data->pWintrustData->u.pFile->hFile);
        data->pWintrustData->u.pFile->hFile = INVALID_HANDLE_VALUE;
        data->fOpenedFile = FALSE;
    }
    return err;
}

static DWORD SOFTPUB_LoadCatalogMessage(CRYPT_PROVIDER_DATA *data)
{
    DWORD err;
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
        return GetLastError();
    if (!CryptSIPRetrieveSubjectGuid(
     data->pWintrustData->u.pCatalog->pcwszCatalogFilePath, catalog,
     &data->u.pPDSip->gSubject))
    {
        err = GetLastError();
        goto error;
    }
    err = SOFTPUB_GetSIP(data);
    if (err)
        goto error;
    err = SOFTPUB_GetMessageFromFile(data, catalog,
     data->pWintrustData->u.pCatalog->pcwszCatalogFilePath);
    if (err)
        goto error;
    err = SOFTPUB_CreateStoreFromMessage(data);
    if (err)
        goto error;
    err = SOFTPUB_DecodeInnerContent(data);
    /* FIXME: this loads the catalog file, but doesn't validate the member. */
error:
    CloseHandle(catalog);
    return err;
}

HRESULT WINAPI SoftpubLoadMessage(CRYPT_PROVIDER_DATA *data)
{
    DWORD err = ERROR_SUCCESS;

    TRACE("(%p)\n", data);

    if (!data->padwTrustStepErrors)
        return S_FALSE;

    switch (data->pWintrustData->dwUnionChoice)
    {
    case WTD_CHOICE_CERT:
        err = SOFTPUB_LoadCertMessage(data);
        break;
    case WTD_CHOICE_FILE:
        err = SOFTPUB_LoadFileMessage(data);
        break;
    case WTD_CHOICE_CATALOG:
        err = SOFTPUB_LoadCatalogMessage(data);
        break;
    default:
        FIXME("unimplemented for %d\n", data->pWintrustData->dwUnionChoice);
        err = ERROR_INVALID_PARAMETER;
    }

    if (err)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] = err;
    TRACE("returning %d (%08x)\n", !err ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
    return !err ? S_OK : S_FALSE;
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

static BOOL WINTRUST_GetTimeFromCounterSigner(
 const CMSG_CMS_SIGNER_INFO *counterSignerInfo, FILETIME *time)
{
    DWORD i;
    BOOL foundTimeStamp = FALSE;

    for (i = 0; !foundTimeStamp && i < counterSignerInfo->AuthAttrs.cAttr; i++)
    {
        if (!strcmp(counterSignerInfo->AuthAttrs.rgAttr[i].pszObjId,
         szOID_RSA_signingTime))
        {
            const CRYPT_ATTRIBUTE *attr =
             &counterSignerInfo->AuthAttrs.rgAttr[i];
            DWORD j;

            for (j = 0; !foundTimeStamp && j < attr->cValue; j++)
            {
                static const DWORD encoding = X509_ASN_ENCODING |
                 PKCS_7_ASN_ENCODING;
                DWORD size = sizeof(FILETIME);

                foundTimeStamp = CryptDecodeObjectEx(encoding,
                 X509_CHOICE_OF_TIME,
                 attr->rgValue[j].pbData, attr->rgValue[j].cbData, 0, NULL,
                 time, &size);
            }
        }
    }
    return foundTimeStamp;
}

static LPCSTR filetime_to_str(const FILETIME *time)
{
    static char date[80];
    char dateFmt[80]; /* sufficient for all versions of LOCALE_SSHORTDATE */
    SYSTEMTIME sysTime;

    if (!time) return NULL;

    GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, dateFmt,
     sizeof(dateFmt) / sizeof(dateFmt[0]));
    FileTimeToSystemTime(time, &sysTime);
    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, date,
     sizeof(date) / sizeof(date[0]));
    return date;
}

static FILETIME WINTRUST_GetTimeFromSigner(const CRYPT_PROVIDER_DATA *data,
 const CMSG_SIGNER_INFO *signerInfo)
{
    DWORD i;
    FILETIME time;
    BOOL foundTimeStamp = FALSE;

    for (i = 0; !foundTimeStamp && i < signerInfo->UnauthAttrs.cAttr; i++)
    {
        if (!strcmp(signerInfo->UnauthAttrs.rgAttr[i].pszObjId,
         szOID_RSA_counterSign))
        {
            const CRYPT_ATTRIBUTE *attr = &signerInfo->UnauthAttrs.rgAttr[i];
            DWORD j;

            for (j = 0; j < attr->cValue; j++)
            {
                static const DWORD encoding = X509_ASN_ENCODING |
                 PKCS_7_ASN_ENCODING;
                CMSG_CMS_SIGNER_INFO *counterSignerInfo;
                DWORD size;
                BOOL ret = CryptDecodeObjectEx(encoding, CMS_SIGNER_INFO,
                 attr->rgValue[j].pbData, attr->rgValue[j].cbData,
                 CRYPT_DECODE_ALLOC_FLAG, NULL, &counterSignerInfo, &size);
                if (ret)
                {
                    /* FIXME: need to verify countersigner signature too */
                    foundTimeStamp = WINTRUST_GetTimeFromCounterSigner(
                     counterSignerInfo, &time);
                    LocalFree(counterSignerInfo);
                }
            }
        }
    }
    if (!foundTimeStamp)
    {
        TRACE("returning system time %s\n",
         filetime_to_str(&data->sftSystemTime));
        time = data->sftSystemTime;
    }
    else
        TRACE("returning time from message %s\n", filetime_to_str(&time));
    return time;
}

static DWORD WINTRUST_SaveSigner(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    DWORD err;
    CMSG_SIGNER_INFO *signerInfo = WINTRUST_GetSigner(data, signerIdx);

    if (signerInfo)
    {
        CRYPT_PROVIDER_SGNR sgnr = { sizeof(sgnr), { 0 } };

        sgnr.psSigner = signerInfo;
        sgnr.sftVerifyAsOf = WINTRUST_GetTimeFromSigner(data, signerInfo);
        if (!data->psPfns->pfnAddSgnr2Chain(data, FALSE, signerIdx, &sgnr))
            err = GetLastError();
        else
            err = ERROR_SUCCESS;
    }
    else
        err = GetLastError();
    return err;
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

static DWORD WINTRUST_VerifySigner(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    DWORD err;
    CERT_INFO *certInfo = WINTRUST_GetSignerCertInfo(data, signerIdx);

    if (certInfo)
    {
        PCCERT_CONTEXT subject = CertGetSubjectCertificateFromStore(
         data->pahStores[0], data->dwEncoding, certInfo);

        if (subject)
        {
            CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA para = { sizeof(para), 0,
             signerIdx, CMSG_VERIFY_SIGNER_CERT, (LPVOID)subject };

            if (!CryptMsgControl(data->hMsg, 0, CMSG_CTRL_VERIFY_SIGNATURE_EX,
             &para))
                err = TRUST_E_CERT_SIGNATURE;
            else
            {
                data->psPfns->pfnAddCert2Chain(data, signerIdx, FALSE, 0,
                 subject);
                err = ERROR_SUCCESS;
            }
            CertFreeCertificateContext(subject);
        }
        else
            err = TRUST_E_NO_SIGNER_CERT;
        data->psPfns->pfnFree(certInfo);
    }
    else
        err = GetLastError();
    return err;
}

HRESULT WINAPI SoftpubLoadSignature(CRYPT_PROVIDER_DATA *data)
{
    DWORD err;

    TRACE("(%p)\n", data);

    if (!data->padwTrustStepErrors)
        return S_FALSE;

    if (data->hMsg)
    {
        DWORD signerCount, size;

        size = sizeof(signerCount);
        if (CryptMsgGetParam(data->hMsg, CMSG_SIGNER_COUNT_PARAM, 0,
         &signerCount, &size))
        {
            DWORD i;

            err = ERROR_SUCCESS;
            for (i = 0; !err && i < signerCount; i++)
            {
                if (!(err = WINTRUST_SaveSigner(data, i)))
                    err = WINTRUST_VerifySigner(data, i);
            }
        }
        else
            err = TRUST_E_NOSIGNATURE;
    }
    else
        err = ERROR_SUCCESS;
    if (err)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = err;
    return !err ? S_OK : S_FALSE;
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

static DWORD WINTRUST_CopyChain(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    DWORD err, i;
    PCERT_SIMPLE_CHAIN simpleChain =
     data->pasSigners[signerIdx].pChainContext->rgpChain[0];

    data->pasSigners[signerIdx].pasCertChain[0].dwConfidence =
     WINTRUST_TrustStatusToConfidence(
     simpleChain->rgpElement[0]->TrustStatus.dwErrorStatus);
    data->pasSigners[signerIdx].pasCertChain[0].pChainElement =
     simpleChain->rgpElement[0];
    err = ERROR_SUCCESS;
    for (i = 1; !err && i < simpleChain->cElement; i++)
    {
        if (data->psPfns->pfnAddCert2Chain(data, signerIdx, FALSE, 0,
         simpleChain->rgpElement[i]->pCertContext))
        {
            data->pasSigners[signerIdx].pasCertChain[i].pChainElement =
             simpleChain->rgpElement[i];
            data->pasSigners[signerIdx].pasCertChain[i].dwConfidence =
             WINTRUST_TrustStatusToConfidence(
             simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus);
        }
        else
            err = GetLastError();
    }
    data->pasSigners[signerIdx].pasCertChain[simpleChain->cElement - 1].dwError
     = WINTRUST_TrustStatusToError(
     simpleChain->rgpElement[simpleChain->cElement - 1]->
     TrustStatus.dwErrorStatus);
    return err;
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

static DWORD WINTRUST_CreateChainForSigner(CRYPT_PROVIDER_DATA *data,
 DWORD signer, PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO createInfo,
 PCERT_CHAIN_PARA chainPara)
{
    DWORD err = ERROR_SUCCESS;
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
        else
            err = GetLastError();
    }
    if (!err)
    {
        /* Expect the end certificate for each signer to be the only cert in
         * the chain:
         */
        if (data->pasSigners[signer].csCertChain)
        {
            BOOL ret;

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
                    err = E_NOTIMPL;
                }
                else
                {
                    if (!(err = WINTRUST_CopyChain(data, signer)))
                    {
                        if (data->psPfns->pfnCertCheckPolicy)
                        {
                            ret = data->psPfns->pfnCertCheckPolicy(data, signer,
                             FALSE, 0);
                            if (!ret)
                                err = GetLastError();
                        }
                        else
                            TRACE(
                             "no cert check policy, skipping policy check\n");
                    }
                }
            }
            else
                err = GetLastError();
        }
        CertCloseStore(store, 0);
    }
    return err;
}

HRESULT WINAPI WintrustCertificateTrust(CRYPT_PROVIDER_DATA *data)
{
    DWORD err;

    TRACE("(%p)\n", data);

    if (!data->csSigners)
        err = TRUST_E_NOSIGNATURE;
    else
    {
        DWORD i;
        WTD_GENERIC_CHAIN_POLICY_CREATE_INFO createInfo;
        CERT_CHAIN_PARA chainPara;

        WINTRUST_CreateChainPolicyCreateInfo(data, &createInfo, &chainPara);
        err = ERROR_SUCCESS;
        for (i = 0; !err && i < data->csSigners; i++)
            err = WINTRUST_CreateChainForSigner(data, i, &createInfo,
             &chainPara);
    }
    if (err)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = err;
    TRACE("returning %d (%08x)\n", !err ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    return !err ? S_OK : S_FALSE;
}

HRESULT WINAPI GenericChainCertificateTrust(CRYPT_PROVIDER_DATA *data)
{
    DWORD err;
    WTD_GENERIC_CHAIN_POLICY_DATA *policyData =
     data->pWintrustData->pPolicyCallbackData;

    TRACE("(%p)\n", data);

    if (policyData && policyData->u.cbSize !=
     sizeof(WTD_GENERIC_CHAIN_POLICY_CREATE_INFO))
    {
        err = ERROR_INVALID_PARAMETER;
        goto end;
    }
    if (!data->csSigners)
        err = TRUST_E_NOSIGNATURE;
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
        err = ERROR_SUCCESS;
        for (i = 0; !err && i < data->csSigners; i++)
            err = WINTRUST_CreateChainForSigner(data, i, pCreateInfo,
             pChainPara);
    }

end:
    if (err)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = err;
    TRACE("returning %d (%08x)\n", !err ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    return !err ? S_OK : S_FALSE;
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
     data->pWintrustData->pPolicyCallbackData;

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

    if (data->fOpenedFile &&
     data->pWintrustData->dwUnionChoice == WTD_CHOICE_FILE &&
     data->pWintrustData->u.pFile)
    {
        CloseHandle(data->pWintrustData->u.pFile->hFile);
        data->pWintrustData->u.pFile->hFile = INVALID_HANDLE_VALUE;
        data->fOpenedFile = FALSE;
    }

    return S_OK;
}

HRESULT WINAPI HTTPSCertificateTrust(CRYPT_PROVIDER_DATA *data)
{
    FIXME("(%p)\n", data);
    return S_OK;
}

HRESULT WINAPI HTTPSFinalProv(CRYPT_PROVIDER_DATA *data)
{
    FIXME("(%p)\n", data);
    return S_OK;
}

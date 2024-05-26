/*
 * Copyright 2001 Rein Klazes
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "guiddef.h"
#include "wintrust.h"
#include "softpub.h"
#include "mscat.h"
#include "objbase.h"
#include "winuser.h"
#include "cryptdlg.h"
#include "cryptuiapi.h"
#include "wintrust_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

/***********************************************************************
 *		TrustIsCertificateSelfSigned (WINTRUST.@)
 */
BOOL WINAPI TrustIsCertificateSelfSigned( PCCERT_CONTEXT cert )
{
    PCERT_EXTENSION ext;
    DWORD size;
    BOOL ret;

    TRACE("%p\n", cert);
    if ((ext = CertFindExtension(szOID_AUTHORITY_KEY_IDENTIFIER2,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        CERT_AUTHORITY_KEY_ID2_INFO *info;

        ret = CryptDecodeObjectEx(cert->dwCertEncodingType,
         X509_AUTHORITY_KEY_ID2, ext->Value.pbData, ext->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         &info, &size);
        if (ret)
        {
            if (info->AuthorityCertIssuer.cAltEntry &&
             info->AuthorityCertSerialNumber.cbData)
            {
                PCERT_ALT_NAME_ENTRY directoryName = NULL;
                DWORD i;

                for (i = 0; !directoryName &&
                 i < info->AuthorityCertIssuer.cAltEntry; i++)
                    if (info->AuthorityCertIssuer.rgAltEntry[i].dwAltNameChoice
                     == CERT_ALT_NAME_DIRECTORY_NAME)
                        directoryName =
                         &info->AuthorityCertIssuer.rgAltEntry[i];
                if (directoryName)
                {
                    ret = CertCompareCertificateName(cert->dwCertEncodingType,
                     &directoryName->DirectoryName, &cert->pCertInfo->Issuer)
                     && CertCompareIntegerBlob(&info->AuthorityCertSerialNumber,
                     &cert->pCertInfo->SerialNumber);
                }
                else
                {
                    FIXME("no supported name type in authority key id2\n");
                    ret = FALSE;
                }
            }
            else if (info->KeyId.cbData)
            {
                ret = CertGetCertificateContextProperty(cert,
                 CERT_KEY_IDENTIFIER_PROP_ID, NULL, &size);
                if (ret && size == info->KeyId.cbData)
                {
                    LPBYTE buf = CryptMemAlloc(size);

                    if (buf)
                    {
                        CertGetCertificateContextProperty(cert,
                         CERT_KEY_IDENTIFIER_PROP_ID, buf, &size);
                        ret = !memcmp(buf, info->KeyId.pbData, size);
                        CryptMemFree(buf);
                    }
                    else
                        ret = FALSE;
                }
                else
                    ret = FALSE;
            }
            LocalFree(info);
        }
    }
    else if ((ext = CertFindExtension(szOID_AUTHORITY_KEY_IDENTIFIER,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        CERT_AUTHORITY_KEY_ID_INFO *info;

        ret = CryptDecodeObjectEx(cert->dwCertEncodingType,
         X509_AUTHORITY_KEY_ID, ext->Value.pbData, ext->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         &info, &size);
        if (ret)
        {
            if (info->CertIssuer.cbData && info->CertSerialNumber.cbData)
            {
                ret = CertCompareCertificateName(cert->dwCertEncodingType,
                 &info->CertIssuer, &cert->pCertInfo->Issuer) &&
                 CertCompareIntegerBlob(&info->CertSerialNumber,
                 &cert->pCertInfo->SerialNumber);
            }
            else if (info->KeyId.cbData)
            {
                ret = CertGetCertificateContextProperty(cert,
                 CERT_KEY_IDENTIFIER_PROP_ID, NULL, &size);
                if (ret && size == info->KeyId.cbData)
                {
                    LPBYTE buf = CryptMemAlloc(size);

                    if (buf)
                    {
                        CertGetCertificateContextProperty(cert,
                         CERT_KEY_IDENTIFIER_PROP_ID, buf, &size);
                        ret = !memcmp(buf, info->KeyId.pbData, size);
                        CryptMemFree(buf);
                    }
                    else
                        ret = FALSE;
                }
                else
                    ret = FALSE;
            }
            else
                ret = FALSE;
            LocalFree(info);
        }
    }
    else
        ret = CertCompareCertificateName(cert->dwCertEncodingType,
         &cert->pCertInfo->Subject, &cert->pCertInfo->Issuer);
    return ret;
}

typedef HRESULT (WINAPI *wintrust_step_func)(CRYPT_PROVIDER_DATA *data);

struct wintrust_step
{
    wintrust_step_func func;
    DWORD              error_index;
};

static DWORD WINTRUST_ExecuteSteps(const struct wintrust_step *steps,
 DWORD numSteps, CRYPT_PROVIDER_DATA *provData)
{
    DWORD i, err = ERROR_SUCCESS;

    for (i = 0; !err && i < numSteps; i++)
    {
        err = steps[i].func(provData);
        if (err)
            err = provData->padwTrustStepErrors[steps[i].error_index];
    }
    return err;
}

static CRYPT_PROVIDER_DATA *WINTRUST_AllocateProviderData(void)
{
    CRYPT_PROVIDER_DATA *provData;

    provData = calloc(1, sizeof(CRYPT_PROVIDER_DATA));
    if (!provData)
        goto oom;
    provData->cbStruct = sizeof(CRYPT_PROVIDER_DATA);

    provData->padwTrustStepErrors = calloc(TRUSTERROR_MAX_STEPS, sizeof(DWORD));
    if (!provData->padwTrustStepErrors)
        goto oom;
    provData->cdwTrustStepErrors = TRUSTERROR_MAX_STEPS;

    provData->pPDSip = calloc(1, sizeof(PROVDATA_SIP));
    if (!provData->pPDSip)
        goto oom;
    provData->pPDSip->cbStruct = sizeof(PROVDATA_SIP);

    provData->psPfns = calloc(1, sizeof(CRYPT_PROVIDER_FUNCTIONS));
    if (!provData->psPfns)
        goto oom;
    provData->psPfns->cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);
    return provData;

oom:
    if (provData)
    {
        free(provData->padwTrustStepErrors);
        free(provData->pPDSip);
        free(provData->psPfns);
        free(provData);
    }
    return NULL;
}

/* Adds trust steps for each function in psPfns.  Assumes steps has at least
 * 5 entries.  Returns the number of steps added.
 */
static DWORD WINTRUST_AddTrustStepsFromFunctions(struct wintrust_step *steps,
 const CRYPT_PROVIDER_FUNCTIONS *psPfns)
{
    DWORD numSteps = 0;

    if (psPfns->pfnInitialize)
    {
        steps[numSteps].func = psPfns->pfnInitialize;
        steps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_WVTINIT;
    }
    if (psPfns->pfnObjectTrust)
    {
        steps[numSteps].func = psPfns->pfnObjectTrust;
        steps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_OBJPROV;
    }
    if (psPfns->pfnSignatureTrust)
    {
        steps[numSteps].func = psPfns->pfnSignatureTrust;
        steps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_SIGPROV;
    }
    if (psPfns->pfnCertificateTrust)
    {
        steps[numSteps].func = psPfns->pfnCertificateTrust;
        steps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_CERTPROV;
    }
    if (psPfns->pfnFinalPolicy)
    {
        steps[numSteps].func = psPfns->pfnFinalPolicy;
        steps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_POLICYPROV;
    }
    return numSteps;
}

static LONG WINTRUST_DefaultVerify(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD err = ERROR_SUCCESS, numSteps = 0;
    CRYPT_PROVIDER_DATA *provData;
    BOOL ret;
    struct wintrust_step verifySteps[5];

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    provData = WINTRUST_AllocateProviderData();
    if (!provData)
        return ERROR_OUTOFMEMORY;

    ret = WintrustLoadFunctionPointers(actionID, provData->psPfns);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }

    data->hWVTStateData = provData;
    provData->pWintrustData = data;

    if (WVT_ISINSTRUCT(WINTRUST_DATA, data->cbStruct, pSignatureSettings))
        provData->pSigSettings = data->pSignatureSettings;

    if (hwnd == INVALID_HANDLE_VALUE)
        provData->hWndParent = GetDesktopWindow();
    else
        provData->hWndParent = hwnd;
    provData->pgActionID = actionID;
    WintrustGetRegPolicyFlags(&provData->dwRegPolicySettings);

    numSteps = WINTRUST_AddTrustStepsFromFunctions(verifySteps,
     provData->psPfns);
    err = WINTRUST_ExecuteSteps(verifySteps, numSteps, provData);
    goto done;

error:
    free(provData->padwTrustStepErrors);
    free(provData->pPDSip);
    free(provData->psPfns);
    free(provData);

done:
    TRACE("returning %08lx\n", err);
    return err;
}

static LONG WINTRUST_DefaultClose(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD err = ERROR_SUCCESS;
    CRYPT_PROVIDER_DATA *provData = data->hWVTStateData;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    if (provData)
    {
        if (provData->psPfns->pfnCleanupPolicy)
            err = provData->psPfns->pfnCleanupPolicy(provData);

        free(provData->padwTrustStepErrors);
        free(provData->pPDSip);
        free(provData->psPfns);
        free(provData);
        data->hWVTStateData = NULL;
    }
    TRACE("returning %08lx\n", err);
    return err;
}

static LONG WINTRUST_DefaultVerifyAndClose(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    LONG err;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    err = WINTRUST_DefaultVerify(hwnd, actionID, data);
    WINTRUST_DefaultClose(hwnd, actionID, data);
    TRACE("returning %08lx\n", err);
    return err;
}

static LONG WINTRUST_PublishedSoftware(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    WINTRUST_DATA wintrust_data = { sizeof(wintrust_data), 0 };
    /* Undocumented: the published software action is passed a path,
     * and pSIPClientData points to a WIN_TRUST_SUBJECT_FILE.
     */
    LPWIN_TRUST_SUBJECT_FILE subjectFile = data->pSIPClientData;
    WINTRUST_FILE_INFO fileInfo = { sizeof(fileInfo), 0 };

    TRACE("subjectFile->hFile: %p\n", subjectFile->hFile);
    TRACE("subjectFile->lpPath: %s\n", debugstr_w(subjectFile->lpPath));
    fileInfo.pcwszFilePath = subjectFile->lpPath;
    fileInfo.hFile = subjectFile->hFile;
    wintrust_data.pFile = &fileInfo;
    wintrust_data.dwUnionChoice = WTD_CHOICE_FILE;
    wintrust_data.dwUIChoice = WTD_UI_NONE;

    return WINTRUST_DefaultVerifyAndClose(hwnd, actionID, &wintrust_data);
}

/* Sadly, the function to load the cert for the CERT_CERTIFICATE_ACTION_VERIFY
 * action is not stored in the registry and is located in wintrust, not in
 * cryptdlg along with the rest of the implementation (verified by running the
 * action with a native wintrust.dll.)
 */
static HRESULT WINAPI WINTRUST_CertVerifyObjTrust(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    TRACE("(%p)\n", data);

    if (!data->padwTrustStepErrors)
        return S_FALSE;

    switch (data->pWintrustData->dwUnionChoice)
    {
    case WTD_CHOICE_BLOB:
        if (data->pWintrustData->pBlob &&
         WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_BLOB_INFO,
         data->pWintrustData->pBlob->cbStruct, pbMemObject) &&
         data->pWintrustData->pBlob->cbMemObject ==
         sizeof(CERT_VERIFY_CERTIFICATE_TRUST) &&
         data->pWintrustData->pBlob->pbMemObject)
        {
            CERT_VERIFY_CERTIFICATE_TRUST *pCert =
             (CERT_VERIFY_CERTIFICATE_TRUST *)
             data->pWintrustData->pBlob->pbMemObject;

            if (pCert->cbSize == sizeof(CERT_VERIFY_CERTIFICATE_TRUST) &&
             pCert->pccert)
            {
                CRYPT_PROVIDER_SGNR signer = { sizeof(signer), { 0 } };
                DWORD i;
                SYSTEMTIME sysTime;

                /* Add a signer with nothing but the time to verify, so we can
                 * add a cert to it
                 */
                GetSystemTime(&sysTime);
                SystemTimeToFileTime(&sysTime, &signer.sftVerifyAsOf);
                ret = data->psPfns->pfnAddSgnr2Chain(data, FALSE, 0, &signer);
                if (!ret)
                    goto error;
                ret = data->psPfns->pfnAddCert2Chain(data, 0, FALSE, 0,
                 pCert->pccert);
                if (!ret)
                    goto error;
                for (i = 0; ret && i < pCert->cRootStores; i++)
                    ret = data->psPfns->pfnAddStore2Chain(data,
                     pCert->rghstoreRoots[i]);
                for (i = 0; ret && i < pCert->cStores; i++)
                    ret = data->psPfns->pfnAddStore2Chain(data,
                     pCert->rghstoreCAs[i]);
                for (i = 0; ret && i < pCert->cTrustStores; i++)
                    ret = data->psPfns->pfnAddStore2Chain(data,
                     pCert->rghstoreTrust[i]);
            }
            else
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                ret = FALSE;
            }
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            ret = FALSE;
        }
        break;
    default:
        FIXME("unimplemented for %ld\n", data->pWintrustData->dwUnionChoice);
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
    }

error:
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] =
         GetLastError();
    TRACE("returning %ld (%08lx)\n", ret ? S_OK : S_FALSE,
     data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
    return ret ? S_OK : S_FALSE;
}

static LONG WINTRUST_CertVerify(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD err = ERROR_SUCCESS, numSteps = 0;
    CRYPT_PROVIDER_DATA *provData;
    BOOL ret;
    struct wintrust_step verifySteps[5];

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    provData = WINTRUST_AllocateProviderData();
    if (!provData)
        return ERROR_OUTOFMEMORY;

    ret = WintrustLoadFunctionPointers(actionID, provData->psPfns);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }
    if (!provData->psPfns->pfnObjectTrust)
        provData->psPfns->pfnObjectTrust = WINTRUST_CertVerifyObjTrust;
    /* Not sure why, but native skips the policy check */
    provData->psPfns->pfnCertCheckPolicy = NULL;

    data->hWVTStateData = provData;
    provData->pWintrustData = data;
    if (hwnd == INVALID_HANDLE_VALUE)
        provData->hWndParent = GetDesktopWindow();
    else
        provData->hWndParent = hwnd;
    provData->pgActionID = actionID;
    WintrustGetRegPolicyFlags(&provData->dwRegPolicySettings);

    numSteps = WINTRUST_AddTrustStepsFromFunctions(verifySteps,
     provData->psPfns);
    err = WINTRUST_ExecuteSteps(verifySteps, numSteps, provData);
    goto done;

error:
    free(provData->padwTrustStepErrors);
    free(provData->pPDSip);
    free(provData->psPfns);
    free(provData);

done:
    TRACE("returning %08lx\n", err);
    return err;
}

static LONG WINTRUST_CertVerifyAndClose(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    LONG err;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    err = WINTRUST_CertVerify(hwnd, actionID, data);
    WINTRUST_DefaultClose(hwnd, actionID, data);
    TRACE("returning %08lx\n", err);
    return err;
}

static LONG WINTRUST_CertActionVerify(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD stateAction;
    LONG err = ERROR_SUCCESS;

    if (WVT_ISINSTRUCT(WINTRUST_DATA, data->cbStruct, dwStateAction))
        stateAction = data->dwStateAction;
    else
    {
        TRACE("no dwStateAction, assuming WTD_STATEACTION_IGNORE\n");
        stateAction = WTD_STATEACTION_IGNORE;
    }
    switch (stateAction)
    {
    case WTD_STATEACTION_IGNORE:
        err = WINTRUST_CertVerifyAndClose(hwnd, actionID, data);
        break;
    case WTD_STATEACTION_VERIFY:
        err = WINTRUST_CertVerify(hwnd, actionID, data);
        break;
    case WTD_STATEACTION_CLOSE:
        err = WINTRUST_DefaultClose(hwnd, actionID, data);
        break;
    default:
        FIXME("unimplemented for %ld\n", data->dwStateAction);
    }
    return err;
}

static void dump_file_info(WINTRUST_FILE_INFO *pFile)
{
    TRACE("%p\n", pFile);
    if (pFile)
    {
        TRACE("cbStruct: %ld\n", pFile->cbStruct);
        TRACE("pcwszFilePath: %s\n", debugstr_w(pFile->pcwszFilePath));
        TRACE("hFile: %p\n", pFile->hFile);
        TRACE("pgKnownSubject: %s\n", debugstr_guid(pFile->pgKnownSubject));
    }
}

static void dump_catalog_info(WINTRUST_CATALOG_INFO *catalog)
{
    TRACE("%p\n", catalog);
    if (catalog)
    {
        TRACE("cbStruct: %ld\n", catalog->cbStruct);
        TRACE("dwCatalogVersion: %ld\n", catalog->dwCatalogVersion);
        TRACE("pcwszCatalogFilePath: %s\n",
         debugstr_w(catalog->pcwszCatalogFilePath));
        TRACE("pcwszMemberTag: %s\n", debugstr_w(catalog->pcwszMemberTag));
        TRACE("pcwszMemberFilePath: %s\n",
         debugstr_w(catalog->pcwszMemberFilePath));
        TRACE("hMemberFile: %p\n", catalog->hMemberFile);
        TRACE("pbCalculatedFileHash: %p\n", catalog->pbCalculatedFileHash);
        TRACE("cbCalculatedFileHash: %ld\n", catalog->cbCalculatedFileHash);
        TRACE("pcCatalogContext: %p\n", catalog->pcCatalogContext);
    }
}

static void dump_blob_info(WINTRUST_BLOB_INFO *blob)
{
    TRACE("%p\n", blob);
    if (blob)
    {
        TRACE("cbStruct: %ld\n", blob->cbStruct);
        TRACE("gSubject: %s\n", debugstr_guid(&blob->gSubject));
        TRACE("pcwszDisplayName: %s\n", debugstr_w(blob->pcwszDisplayName));
        TRACE("cbMemObject: %ld\n", blob->cbMemObject);
        TRACE("pbMemObject: %p\n", blob->pbMemObject);
        TRACE("cbMemSignedMsg: %ld\n", blob->cbMemSignedMsg);
        TRACE("pbMemSignedMsg: %p\n", blob->pbMemSignedMsg);
    }
}

static void dump_sgnr_info(WINTRUST_SGNR_INFO *sgnr)
{
    TRACE("%p\n", sgnr);
    if (sgnr)
    {
        TRACE("cbStruct: %ld\n", sgnr->cbStruct);
        TRACE("pcwszDisplayName: %s\n", debugstr_w(sgnr->pcwszDisplayName));
        TRACE("psSignerInfo: %p\n", sgnr->psSignerInfo);
        TRACE("chStores: %ld\n", sgnr->chStores);
    }
}

static void dump_cert_info(WINTRUST_CERT_INFO *cert)
{
    TRACE("%p\n", cert);
    if (cert)
    {
        TRACE("cbStruct: %ld\n", cert->cbStruct);
        TRACE("pcwszDisplayName: %s\n", debugstr_w(cert->pcwszDisplayName));
        TRACE("psCertContext: %p\n", cert->psCertContext);
        TRACE("chStores: %ld\n", cert->chStores);
        TRACE("dwFlags: %08lx\n", cert->dwFlags);
        TRACE("psftVerifyAsOf: %p\n", cert->psftVerifyAsOf);
    }
}

static void dump_wintrust_data(WINTRUST_DATA *data)
{
    TRACE("%p\n", data);
    if (data)
    {
        TRACE("cbStruct: %ld\n", data->cbStruct);
        TRACE("pPolicyCallbackData: %p\n", data->pPolicyCallbackData);
        TRACE("pSIPClientData: %p\n", data->pSIPClientData);
        TRACE("dwUIChoice: %ld\n", data->dwUIChoice);
        TRACE("fdwRevocationChecks: %08lx\n", data->fdwRevocationChecks);
        TRACE("dwUnionChoice: %ld\n", data->dwUnionChoice);
        switch (data->dwUnionChoice)
        {
        case WTD_CHOICE_FILE:
            dump_file_info(data->pFile);
            break;
        case WTD_CHOICE_CATALOG:
            dump_catalog_info(data->pCatalog);
            break;
        case WTD_CHOICE_BLOB:
            dump_blob_info(data->pBlob);
            break;
        case WTD_CHOICE_SIGNER:
            dump_sgnr_info(data->pSgnr);
            break;
        case WTD_CHOICE_CERT:
            dump_cert_info(data->pCert);
            break;
        }
        TRACE("dwStateAction: %ld\n", data->dwStateAction);
        TRACE("hWVTStateData: %p\n", data->hWVTStateData);
        TRACE("pwszURLReference: %s\n", debugstr_w(data->pwszURLReference));
        TRACE("dwProvFlags: %08lx\n", data->dwProvFlags);
        TRACE("dwUIContext: %ld\n", data->dwUIContext);
    }
}

/***********************************************************************
 *		WinVerifyTrust (WINTRUST.@)
 *
 * Verifies an object by calling the specified trust provider.
 *
 * PARAMS
 *   hwnd       [I] Handle to a caller window.
 *   ActionID   [I] Pointer to a GUID that identifies the action to perform.
 *   ActionData [I] Information used by the trust provider to verify the object.
 *
 * RETURNS
 *   Success: Zero.
 *   Failure: A TRUST_E_* error code.
 *
 * NOTES
 *   Trust providers can be found at:
 *   HKLM\SOFTWARE\Microsoft\Cryptography\Providers\Trust\
 */
LONG WINAPI WinVerifyTrust( HWND hwnd, GUID *ActionID, LPVOID ActionData )
{
    static const GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,
     0x00,0xC0,0x4F,0xC2,0x95,0xEE } };
    static const GUID published_software = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    static const GUID generic_verify_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    static const GUID generic_cert_verify = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    static const GUID generic_chain_verify = WINTRUST_ACTION_GENERIC_CHAIN_VERIFY;
    static const GUID cert_action_verify = CERT_CERTIFICATE_ACTION_VERIFY;
    LONG err = ERROR_SUCCESS;
    WINTRUST_DATA *actionData = ActionData;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(ActionID), ActionData);
    dump_wintrust_data(ActionData);

    /* Support for known old-style callers: */
    if (IsEqualGUID(ActionID, &published_software))
        err = WINTRUST_PublishedSoftware(hwnd, ActionID, ActionData);
    else if (IsEqualGUID(ActionID, &cert_action_verify))
        err = WINTRUST_CertActionVerify(hwnd, ActionID, ActionData);
    else
    {
        DWORD stateAction;

        /* Check known actions to warn of possible problems */
        if (!IsEqualGUID(ActionID, &unknown) &&
         !IsEqualGUID(ActionID, &generic_verify_v2) &&
         !IsEqualGUID(ActionID, &generic_cert_verify) &&
         !IsEqualGUID(ActionID, &generic_chain_verify))
            WARN("unknown action %s, default behavior may not be right\n",
             debugstr_guid(ActionID));
        if (WVT_ISINSTRUCT(WINTRUST_DATA, actionData->cbStruct, dwStateAction))
            stateAction = actionData->dwStateAction;
        else
        {
            TRACE("no dwStateAction, assuming WTD_STATEACTION_IGNORE\n");
            stateAction = WTD_STATEACTION_IGNORE;
        }
        switch (stateAction)
        {
        case WTD_STATEACTION_IGNORE:
            err = WINTRUST_DefaultVerifyAndClose(hwnd, ActionID, ActionData);
            break;
        case WTD_STATEACTION_VERIFY:
            err = WINTRUST_DefaultVerify(hwnd, ActionID, ActionData);
            break;
        case WTD_STATEACTION_CLOSE:
            err = WINTRUST_DefaultClose(hwnd, ActionID, ActionData);
            break;
        default:
            FIXME("unimplemented for %ld\n", actionData->dwStateAction);
        }
    }

    if (err) SetLastError(err);
    TRACE("returning %08lx\n", err);
    return err;
}

/***********************************************************************
 *		WinVerifyTrustEx (WINTRUST.@)
 */
HRESULT WINAPI WinVerifyTrustEx( HWND hwnd, GUID *ActionID,
 WINTRUST_DATA* ActionData )
{
    return WinVerifyTrust(hwnd, ActionID, ActionData);
}

/***********************************************************************
 *		WTHelperGetProvSignerFromChain (WINTRUST.@)
 */
CRYPT_PROVIDER_SGNR * WINAPI WTHelperGetProvSignerFromChain(
 CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSigner,
 DWORD idxCounterSigner)
{
    CRYPT_PROVIDER_SGNR *sgnr;

    TRACE("(%p %ld %d %ld)\n", pProvData, idxSigner, fCounterSigner,
     idxCounterSigner);

    if (idxSigner >= pProvData->csSigners || !pProvData->pasSigners)
        return NULL;
    sgnr = &pProvData->pasSigners[idxSigner];
    if (fCounterSigner)
    {
        if (idxCounterSigner >= sgnr->csCounterSigners ||
         !sgnr->pasCounterSigners)
            return NULL;
        sgnr = &sgnr->pasCounterSigners[idxCounterSigner];
    }
    TRACE("returning %p\n", sgnr);
    return sgnr;
}

/***********************************************************************
 *		WTHelperGetProvCertFromChain (WINTRUST.@)
 */
CRYPT_PROVIDER_CERT * WINAPI WTHelperGetProvCertFromChain(
 CRYPT_PROVIDER_SGNR *pSgnr, DWORD idxCert)
{
    CRYPT_PROVIDER_CERT *cert;

    TRACE("(%p %ld)\n", pSgnr, idxCert);

    if (!pSgnr || idxCert >= pSgnr->csCertChain || !pSgnr->pasCertChain)
        return NULL;
    cert = &pSgnr->pasCertChain[idxCert];
    TRACE("returning %p\n", cert);
    return cert;
}

CRYPT_PROVIDER_PRIVDATA *WINAPI WTHelperGetProvPrivateDataFromChain(
 CRYPT_PROVIDER_DATA* pProvData,
 GUID* pgProviderID)
{
    CRYPT_PROVIDER_PRIVDATA *privdata = NULL;
    DWORD i;

    TRACE("(%p, %s)\n", pProvData, debugstr_guid(pgProviderID));

    for (i = 0; i < pProvData->csProvPrivData; i++)
        if (IsEqualGUID(pgProviderID, &pProvData->pasProvPrivData[i].gProviderID))
        {
            privdata = &pProvData->pasProvPrivData[i];
            break;
        }

    return privdata;
}

/***********************************************************************
 *		WTHelperProvDataFromStateData (WINTRUST.@)
 */
CRYPT_PROVIDER_DATA * WINAPI WTHelperProvDataFromStateData(HANDLE hStateData)
{
    TRACE("%p\n", hStateData);
    return hStateData;
}

/***********************************************************************
 *		WTHelperGetFileName(WINTRUST.@)
 */
LPCWSTR WINAPI WTHelperGetFileName(WINTRUST_DATA *data)
{
    TRACE("%p\n",data);
    if (data->dwUnionChoice == WTD_CHOICE_FILE)
        return data->pFile->pcwszFilePath;
    else
        return NULL;
}

/***********************************************************************
 *		WTHelperGetFileHandle(WINTRUST.@)
 */
HANDLE WINAPI WTHelperGetFileHandle(WINTRUST_DATA *data)
{
    TRACE("%p\n",data);
    if (data->dwUnionChoice == WTD_CHOICE_FILE)
        return data->pFile->hFile;
    else
        return INVALID_HANDLE_VALUE;
}

static BOOL WINAPI WINTRUST_enumUsages(PCCRYPT_OID_INFO pInfo, void *pvArg)
{
    PCCRYPT_OID_INFO **usages = pvArg;
    DWORD cUsages;
    BOOL ret;

    if (!*usages)
    {
        cUsages = 0;
        *usages = malloc(2 * sizeof(PCCRYPT_OID_INFO));
    }
    else
    {
        PCCRYPT_OID_INFO *ptr;

        /* Count the existing usages.
         * FIXME: make sure the new usage doesn't duplicate any in the list?
         */
        for (cUsages = 0, ptr = *usages; *ptr; ptr++, cUsages++)
            ;
        *usages = realloc(*usages, (cUsages + 2) * sizeof(PCCRYPT_OID_INFO));
    }
    if (*usages)
    {
        (*usages)[cUsages] = pInfo;
        (*usages)[cUsages + 1] = NULL;
        ret = TRUE;
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
    }
    return ret;
}

/***********************************************************************
 *		WTHelperGetKnownUsages(WINTRUST.@)
 *
 * Enumerates the known enhanced key usages as an array of PCCRYPT_OID_INFOs.
 *
 * PARAMS
 *  action      [In]     1 => allocate and return known usages, 2 => free previously
 *                       allocated usages.
 *  usages      [In/Out] If action == 1, *usages is set to an array of
 *                       PCCRYPT_OID_INFO *.  The array is terminated with a NULL
 *                       pointer.
 *                       If action == 2, *usages is freed.
 *
 * RETURNS
 *  TRUE on success, FALSE on failure.
 */
BOOL WINAPI WTHelperGetKnownUsages(DWORD action, PCCRYPT_OID_INFO **usages)
{
    BOOL ret;

    TRACE("(%ld, %p)\n", action, usages);

    if (!usages)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (action == 1)
    {
        *usages = NULL;
        ret = CryptEnumOIDInfo(CRYPT_ENHKEY_USAGE_OID_GROUP_ID, 0, usages,
         WINTRUST_enumUsages);
    }
    else if (action == 2)
    {
        free(*usages);
        *usages = NULL;
        ret = TRUE;
    }
    else
    {
        WARN("unknown action %ld\n", action);
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
    }
    return ret;
}

static const WCHAR Software_Publishing[] = {
 'S','o','f','t','w','a','r','e','\\',
 'M','i','c','r','o','s','o','f','t','\\',
 'W','i','n','d','o','w','s','\\',
 'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
 'W','i','n','t','r','u','s','t','\\',
 'T','r','u','s','t',' ','P','r','o','v','i','d','e','r','s','\\',
 'S','o','f','t','w','a','r','e',' ',
 'P','u','b','l','i','s','h','i','n','g',0 };
static const WCHAR State[] = { 'S','t','a','t','e',0 };

/***********************************************************************
 *		WintrustGetRegPolicyFlags (WINTRUST.@)
 */
void WINAPI WintrustGetRegPolicyFlags( DWORD* pdwPolicyFlags )
{
    HKEY key;
    LONG r;

    TRACE("%p\n", pdwPolicyFlags);

    *pdwPolicyFlags = 0;
    r = RegCreateKeyExW(HKEY_CURRENT_USER, Software_Publishing, 0, NULL, 0,
     KEY_READ, NULL, &key, NULL);
    if (!r)
    {
        DWORD size = sizeof(DWORD);

        r = RegQueryValueExW(key, State, NULL, NULL, (LPBYTE)pdwPolicyFlags,
         &size);
        RegCloseKey(key);
        if (r)
        {
            /* Failed to query, create and return default value */
            *pdwPolicyFlags = WTPF_IGNOREREVOCATIONONTS |
             WTPF_OFFLINEOKNBU_COM |
             WTPF_OFFLINEOKNBU_IND |
             WTPF_OFFLINEOK_COM |
             WTPF_OFFLINEOK_IND;
            WintrustSetRegPolicyFlags(*pdwPolicyFlags);
        }
    }
}

/***********************************************************************
 *		WintrustSetRegPolicyFlags (WINTRUST.@)
 */
BOOL WINAPI WintrustSetRegPolicyFlags( DWORD dwPolicyFlags)
{
    HKEY key;
    LONG r;

    TRACE("%lx\n", dwPolicyFlags);

    r = RegCreateKeyExW(HKEY_CURRENT_USER, Software_Publishing, 0,
     NULL, 0, KEY_WRITE, NULL, &key, NULL);
    if (!r)
    {
        r = RegSetValueExW(key, State, 0, REG_DWORD, (LPBYTE)&dwPolicyFlags,
         sizeof(DWORD));
        RegCloseKey(key);
    }
    if (r) SetLastError(r);
    return r == ERROR_SUCCESS;
}

/* Utility functions */

BOOL WINAPI WINTRUST_AddStore(CRYPT_PROVIDER_DATA *data, HCERTSTORE store)
{
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", data, store);

    data->pahStores = realloc(data->pahStores, (data->chStores + 1) * sizeof(HCERTSTORE));
    if (data->pahStores)
    {
        data->pahStores[data->chStores++] = CertDuplicateStore(store);
        ret = TRUE;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}

BOOL WINAPI WINTRUST_AddSgnr(CRYPT_PROVIDER_DATA *data,
 BOOL fCounterSigner, DWORD idxSigner, CRYPT_PROVIDER_SGNR *sgnr)
{
    BOOL ret = FALSE;

    TRACE("(%p, %d, %ld, %p)\n", data, fCounterSigner, idxSigner, sgnr);

    if (sgnr->cbStruct > sizeof(CRYPT_PROVIDER_SGNR))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (fCounterSigner)
    {
        FIXME("unimplemented for counter signers\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    data->pasSigners = realloc(data->pasSigners,
     (data->csSigners + 1) * sizeof(CRYPT_PROVIDER_SGNR));
    if (data->pasSigners)
    {
        if (idxSigner < data->csSigners)
            memmove(&data->pasSigners[idxSigner],
             &data->pasSigners[idxSigner + 1],
             (data->csSigners - idxSigner) * sizeof(CRYPT_PROVIDER_SGNR));
        ret = TRUE;
        if (sgnr->cbStruct == sizeof(CRYPT_PROVIDER_SGNR))
        {
            /* The PSDK says psSigner should be allocated using pfnAlloc, but
             * it doesn't say anything about ownership.  Since callers are
             * internal, assume ownership is passed, and just store the
             * pointer.
             */
            memcpy(&data->pasSigners[idxSigner], sgnr,
             sizeof(CRYPT_PROVIDER_SGNR));
        }
        else
            memset(&data->pasSigners[idxSigner], 0,
             sizeof(CRYPT_PROVIDER_SGNR));
        data->csSigners++;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}

BOOL WINAPI WINTRUST_AddCert(CRYPT_PROVIDER_DATA *data, DWORD idxSigner,
 BOOL fCounterSigner, DWORD idxCounterSigner, PCCERT_CONTEXT pCert2Add)
{
    BOOL ret = FALSE;

    TRACE("(%p, %ld, %d, %ld, %p)\n", data, idxSigner, fCounterSigner,
     idxSigner, pCert2Add);

    if (fCounterSigner)
    {
        FIXME("unimplemented for counter signers\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    data->pasSigners[idxSigner].pasCertChain =
     realloc(data->pasSigners[idxSigner].pasCertChain,
     (data->pasSigners[idxSigner].csCertChain + 1) *
     sizeof(CRYPT_PROVIDER_CERT));
    if (data->pasSigners[idxSigner].pasCertChain)
    {
        CRYPT_PROVIDER_CERT *cert = &data->pasSigners[idxSigner].pasCertChain[
         data->pasSigners[idxSigner].csCertChain];

        cert->cbStruct = sizeof(CRYPT_PROVIDER_CERT);
        cert->pCert = CertDuplicateCertificateContext(pCert2Add);
        data->pasSigners[idxSigner].csCertChain++;
        ret = TRUE;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}

BOOL WINAPI WINTRUST_AddPrivData(CRYPT_PROVIDER_DATA *data,
 CRYPT_PROVIDER_PRIVDATA *pPrivData2Add)
{
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", data, pPrivData2Add);

    if (pPrivData2Add->cbStruct > sizeof(CRYPT_PROVIDER_PRIVDATA))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        WARN("invalid struct size\n");
        return FALSE;
    }
    data->pasProvPrivData = realloc(data->pasProvPrivData,
     (data->csProvPrivData + 1) * sizeof(CRYPT_PROVIDER_SGNR));
    if (data->pasProvPrivData)
    {
        DWORD i;

        for (i = 0; i < data->csProvPrivData; i++)
            if (IsEqualGUID(&pPrivData2Add->gProviderID, &data->pasProvPrivData[i]))
                break;

        data->pasProvPrivData[i] = *pPrivData2Add;
        if (i == data->csProvPrivData)
            data->csProvPrivData++;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}

/***********************************************************************
 *		OpenPersonalTrustDBDialog (WINTRUST.@)
 *
 * Opens the certificate manager dialog, showing only the stores that
 * contain trusted software publishers.
 *
 * PARAMS
 *  hwnd [I] handle of parent window
 *
 * RETURNS
 *  TRUE if the dialog could be opened, FALSE if not.
 */
BOOL WINAPI OpenPersonalTrustDBDialog(HWND hwnd)
{
    CRYPTUI_CERT_MGR_STRUCT uiCertMgr;

    uiCertMgr.dwSize = sizeof(uiCertMgr);
    uiCertMgr.hwndParent = hwnd;
    uiCertMgr.dwFlags = CRYPTUI_CERT_MGR_PUBLISHER_TAB;
    uiCertMgr.pwszTitle = NULL;
    uiCertMgr.pszInitUsageOID = NULL;
    return CryptUIDlgCertMgr(&uiCertMgr);
}

/***********************************************************************
 *		WTHelperCertCheckValidSignature
 */
HRESULT WINAPI WTHelperCertCheckValidSignature(CRYPT_PROVIDER_DATA *pProvData)
{
    FIXME("Stub\n");
    return S_OK;
}

/***********************************************************************
 *              IsCatalogFile
 */
BOOL WINAPI IsCatalogFile(HANDLE hFile, WCHAR *pwszFileName)
{
    static const GUID catGUID = { 0xDE351A43, 0x8E59, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    GUID guid;

    TRACE("(%p, %s)\n", hFile, debugstr_w(pwszFileName));

    if (!CryptSIPRetrieveSubjectGuid(pwszFileName, hFile, &guid))
        return FALSE;
    return IsEqualGUID(&guid, &catGUID);
}

/***********************************************************************
 *              FindCertsByIssuer
 */
HRESULT WINAPI FindCertsByIssuer(PCERT_CHAIN pCertChains, DWORD *pcbCertChains,
 DWORD *pcCertChains, BYTE* pbEncodedIssuerName, DWORD cbEncodedIssuerName,
 LPCWSTR pwszPurpose, DWORD dwKeySpec)
{
    FIXME("(%p, %p, %p, %p, %ld, %s, %ld): stub\n", pCertChains, pcbCertChains,
     pcCertChains, pbEncodedIssuerName, cbEncodedIssuerName,
     debugstr_w(pwszPurpose), dwKeySpec);
    return E_FAIL;
}

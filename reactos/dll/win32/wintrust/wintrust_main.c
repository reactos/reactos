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

#include "config.h"

#include <stdarg.h>

#define NONAMELESSUNION

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
#include "wintrust_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);


/***********************************************************************
 *		DllMain  (WINTRUST.@)
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		TrustIsCertificateSelfSigned (WINTRUST.@)
 */
BOOL WINAPI TrustIsCertificateSelfSigned( PCCERT_CONTEXT cert )
{
    BOOL ret;

    TRACE("%p\n", cert);
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

static LONG WINTRUST_DefaultVerify(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD err = ERROR_SUCCESS, numSteps = 0;
    CRYPT_PROVIDER_DATA *provData;
    BOOL ret;
    struct wintrust_step verifySteps[5];

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    provData = WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_DATA));
    if (!provData)
        goto oom;
    provData->cbStruct = sizeof(CRYPT_PROVIDER_DATA);

    provData->padwTrustStepErrors =
     WINTRUST_Alloc(TRUSTERROR_MAX_STEPS * sizeof(DWORD));
    if (!provData->padwTrustStepErrors)
        goto oom;
    provData->cdwTrustStepErrors = TRUSTERROR_MAX_STEPS;

    provData->u.pPDSip = WINTRUST_Alloc(sizeof(PROVDATA_SIP));
    if (!provData->u.pPDSip)
        goto oom;
    provData->u.pPDSip->cbStruct = sizeof(PROVDATA_SIP);

    provData->psPfns = WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_FUNCTIONS));
    if (!provData->psPfns)
        goto oom;
    provData->psPfns->cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);
    ret = WintrustLoadFunctionPointers(actionID, provData->psPfns);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }

    data->hWVTStateData = (HANDLE)provData;
    provData->pWintrustData = data;
    if (hwnd == INVALID_HANDLE_VALUE)
        provData->hWndParent = GetDesktopWindow();
    else
        provData->hWndParent = hwnd;
    provData->pgActionID = actionID;
    WintrustGetRegPolicyFlags(&provData->dwRegPolicySettings);

    if (provData->psPfns->pfnInitialize)
    {
        verifySteps[numSteps].func = provData->psPfns->pfnInitialize;
        verifySteps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_WVTINIT;
    }
    if (provData->psPfns->pfnObjectTrust)
    {
        verifySteps[numSteps].func = provData->psPfns->pfnObjectTrust;
        verifySteps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_OBJPROV;
    }
    if (provData->psPfns->pfnSignatureTrust)
    {
        verifySteps[numSteps].func = provData->psPfns->pfnSignatureTrust;
        verifySteps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_SIGPROV;
    }
    if (provData->psPfns->pfnCertificateTrust)
    {
        verifySteps[numSteps].func = provData->psPfns->pfnCertificateTrust;
        verifySteps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_CERTPROV;
    }
    if (provData->psPfns->pfnFinalPolicy)
    {
        verifySteps[numSteps].func = provData->psPfns->pfnFinalPolicy;
        verifySteps[numSteps++].error_index = TRUSTERROR_STEP_FINAL_POLICYPROV;
    }
    err = WINTRUST_ExecuteSteps(verifySteps, numSteps, provData);
    goto done;

oom:
    err = ERROR_OUTOFMEMORY;
error:
    if (provData)
    {
        WINTRUST_Free(provData->padwTrustStepErrors);
        WINTRUST_Free(provData->u.pPDSip);
        WINTRUST_Free(provData->psPfns);
        WINTRUST_Free(provData);
    }
done:
    TRACE("returning %08x\n", err);
    return err;
}

static LONG WINTRUST_DefaultClose(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD err = ERROR_SUCCESS;
    CRYPT_PROVIDER_DATA *provData = (CRYPT_PROVIDER_DATA *)data->hWVTStateData;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    if (provData)
    {
        if (provData->psPfns->pfnCleanupPolicy)
            err = provData->psPfns->pfnCleanupPolicy(provData);

        WINTRUST_Free(provData->padwTrustStepErrors);
        WINTRUST_Free(provData->u.pPDSip);
        WINTRUST_Free(provData->psPfns);
        WINTRUST_Free(provData);
        data->hWVTStateData = NULL;
    }
    TRACE("returning %08x\n", err);
    return err;
}

static LONG WINTRUST_DefaultVerifyAndClose(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    LONG err;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    err = WINTRUST_DefaultVerify(hwnd, actionID, data);
    WINTRUST_DefaultClose(hwnd, actionID, data);
    TRACE("returning %08x\n", err);
    return err;
}

static LONG WINTRUST_PublishedSoftware(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    WINTRUST_DATA wintrust_data = { sizeof(wintrust_data), 0 };
    /* Undocumented: the published software action is passed a path,
     * and pSIPClientData points to a WIN_TRUST_SUBJECT_FILE.
     */
    LPWIN_TRUST_SUBJECT_FILE subjectFile =
     (LPWIN_TRUST_SUBJECT_FILE)data->pSIPClientData;
    WINTRUST_FILE_INFO fileInfo = { sizeof(fileInfo), 0 };

    TRACE("subjectFile->hFile: %p\n", subjectFile->hFile);
    TRACE("subjectFile->lpPath: %s\n", debugstr_w(subjectFile->lpPath));
    fileInfo.pcwszFilePath = subjectFile->lpPath;
    fileInfo.hFile = subjectFile->hFile;
    wintrust_data.u.pFile = &fileInfo;
    wintrust_data.dwUnionChoice = WTD_CHOICE_FILE;
    wintrust_data.dwUIChoice = WTD_UI_NONE;

    return WINTRUST_DefaultVerifyAndClose(hwnd, actionID, &wintrust_data);
}

static void dump_file_info(WINTRUST_FILE_INFO *pFile)
{
    TRACE("%p\n", pFile);
    if (pFile)
    {
        TRACE("cbStruct: %d\n", pFile->cbStruct);
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
        TRACE("cbStruct: %d\n", catalog->cbStruct);
        TRACE("dwCatalogVersion: %d\n", catalog->dwCatalogVersion);
        TRACE("pcwszCatalogFilePath: %s\n",
         debugstr_w(catalog->pcwszCatalogFilePath));
        TRACE("pcwszMemberTag: %s\n", debugstr_w(catalog->pcwszMemberTag));
        TRACE("pcwszMemberFilePath: %s\n",
         debugstr_w(catalog->pcwszMemberFilePath));
        TRACE("hMemberFile: %p\n", catalog->hMemberFile);
        TRACE("pbCalculatedFileHash: %p\n", catalog->pbCalculatedFileHash);
        TRACE("cbCalculatedFileHash: %d\n", catalog->cbCalculatedFileHash);
        TRACE("pcCatalogContext: %p\n", catalog->pcCatalogContext);
    }
}

static void dump_blob_info(WINTRUST_BLOB_INFO *blob)
{
    TRACE("%p\n", blob);
    if (blob)
    {
        TRACE("cbStruct: %d\n", blob->cbStruct);
        TRACE("gSubject: %s\n", debugstr_guid(&blob->gSubject));
        TRACE("pcwszDisplayName: %s\n", debugstr_w(blob->pcwszDisplayName));
        TRACE("cbMemObject: %d\n", blob->cbMemObject);
        TRACE("pbMemObject: %p\n", blob->pbMemObject);
        TRACE("cbMemSignedMsg: %d\n", blob->cbMemSignedMsg);
        TRACE("pbMemSignedMsg: %p\n", blob->pbMemSignedMsg);
    }
}

static void dump_sgnr_info(WINTRUST_SGNR_INFO *sgnr)
{
    TRACE("%p\n", sgnr);
    if (sgnr)
    {
        TRACE("cbStruct: %d\n", sgnr->cbStruct);
        TRACE("pcwszDisplayName: %s\n", debugstr_w(sgnr->pcwszDisplayName));
        TRACE("psSignerInfo: %p\n", sgnr->psSignerInfo);
        TRACE("chStores: %d\n", sgnr->chStores);
    }
}

static void dump_cert_info(WINTRUST_CERT_INFO *cert)
{
    TRACE("%p\n", cert);
    if (cert)
    {
        TRACE("cbStruct: %d\n", cert->cbStruct);
        TRACE("pcwszDisplayName: %s\n", debugstr_w(cert->pcwszDisplayName));
        TRACE("psCertContext: %p\n", cert->psCertContext);
        TRACE("chStores: %d\n", cert->chStores);
        TRACE("dwFlags: %08x\n", cert->dwFlags);
        TRACE("psftVerifyAsOf: %p\n", cert->psftVerifyAsOf);
    }
}

static void dump_wintrust_data(WINTRUST_DATA *data)
{
    TRACE("%p\n", data);
    if (data)
    {
        TRACE("cbStruct: %d\n", data->cbStruct);
        TRACE("pPolicyCallbackData: %p\n", data->pPolicyCallbackData);
        TRACE("pSIPClientData: %p\n", data->pSIPClientData);
        TRACE("dwUIChoice: %d\n", data->dwUIChoice);
        TRACE("fdwRevocationChecks: %08x\n", data->fdwRevocationChecks);
        TRACE("dwUnionChoice: %d\n", data->dwUnionChoice);
        switch (data->dwUnionChoice)
        {
        case WTD_CHOICE_FILE:
            dump_file_info(data->u.pFile);
            break;
        case WTD_CHOICE_CATALOG:
            dump_catalog_info(data->u.pCatalog);
            break;
        case WTD_CHOICE_BLOB:
            dump_blob_info(data->u.pBlob);
            break;
        case WTD_CHOICE_SIGNER:
            dump_sgnr_info(data->u.pSgnr);
            break;
        case WTD_CHOICE_CERT:
            dump_cert_info(data->u.pCert);
            break;
        }
        TRACE("dwStateAction: %d\n", data->dwStateAction);
        TRACE("hWVTStateData: %p\n", data->hWVTStateData);
        TRACE("pwszURLReference: %s\n", debugstr_w(data->pwszURLReference));
        TRACE("dwProvFlags: %08x\n", data->dwProvFlags);
        TRACE("dwUIContext: %d\n", data->dwUIContext);
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
    LONG err = ERROR_SUCCESS;
    WINTRUST_DATA *actionData = (WINTRUST_DATA *)ActionData;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(ActionID), ActionData);
    dump_wintrust_data(ActionData);

    /* Support for known old-style callers: */
    if (IsEqualGUID(ActionID, &published_software))
        err = WINTRUST_PublishedSoftware(hwnd, ActionID, ActionData);
    else
    {
        /* Check known actions to warn of possible problems */
        if (!IsEqualGUID(ActionID, &unknown) &&
         !IsEqualGUID(ActionID, &generic_verify_v2) &&
         !IsEqualGUID(ActionID, &generic_cert_verify) &&
         !IsEqualGUID(ActionID, &generic_chain_verify))
            WARN("unknown action %s, default behavior may not be right\n",
             debugstr_guid(ActionID));
        switch (actionData->dwStateAction)
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
            FIXME("unimplemented for %d\n", actionData->dwStateAction);
        }
    }

    TRACE("returning %08x\n", err);
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

    TRACE("(%p %d %d %d)\n", pProvData, idxSigner, fCounterSigner,
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

    TRACE("(%p %d)\n", pSgnr, idxCert);

    if (idxCert >= pSgnr->csCertChain || !pSgnr->pasCertChain)
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
    return (CRYPT_PROVIDER_DATA *)hStateData;
}

/***********************************************************************
 *		WTHelperGetFileName(WINTRUST.@)
 */
LPCWSTR WINAPI WTHelperGetFileName(WINTRUST_DATA *data)
{
    TRACE("%p\n",data);
    if (data->dwUnionChoice == WTD_CHOICE_FILE)
        return data->u.pFile->pcwszFilePath;
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
        return data->u.pFile->hFile;
    else
        return INVALID_HANDLE_VALUE;
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

    TRACE("%x\n", dwPolicyFlags);

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
void * WINAPI WINTRUST_Alloc(DWORD cb)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}

void * WINAPI WINTRUST_ReAlloc(void *ptr, DWORD cb)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, cb);
}

void WINAPI WINTRUST_Free(void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

BOOL WINAPI WINTRUST_AddStore(CRYPT_PROVIDER_DATA *data, HCERTSTORE store)
{
    BOOL ret = FALSE;

    if (data->chStores)
        data->pahStores = WINTRUST_ReAlloc(data->pahStores,
         (data->chStores + 1) * sizeof(HCERTSTORE));
    else
    {
        data->pahStores = WINTRUST_Alloc(sizeof(HCERTSTORE));
        data->chStores = 0;
    }
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
    if (data->csSigners)
        data->pasSigners = WINTRUST_ReAlloc(data->pasSigners,
         (data->csSigners + 1) * sizeof(CRYPT_PROVIDER_SGNR));
    else
    {
        data->pasSigners = WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_SGNR));
        data->csSigners = 0;
    }
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

    if (fCounterSigner)
    {
        FIXME("unimplemented for counter signers\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (data->pasSigners[idxSigner].csCertChain)
        data->pasSigners[idxSigner].pasCertChain =
         WINTRUST_ReAlloc(data->pasSigners[idxSigner].pasCertChain,
         (data->pasSigners[idxSigner].csCertChain + 1) *
         sizeof(CRYPT_PROVIDER_CERT));
    else
    {
        data->pasSigners[idxSigner].pasCertChain =
         WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_CERT));
        data->pasSigners[idxSigner].csCertChain = 0;
    }
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
    if (data->csProvPrivData)
        data->pasProvPrivData = WINTRUST_ReAlloc(data->pasProvPrivData,
         (data->csProvPrivData + 1) * sizeof(CRYPT_PROVIDER_SGNR));
    else
    {
        data->pasProvPrivData = WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_SGNR));
        data->csProvPrivData = 0;
    }
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

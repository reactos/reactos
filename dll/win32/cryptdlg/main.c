/*
 * Copyright 2008 Maarten Lankhorst
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
#include "winnls.h"
#include "winreg.h"
#include "wincrypt.h"
#include "wintrust.h"
#include "winuser.h"
#include "objbase.h"
#include "cryptdlg.h"
#include "cryptuiapi.h"
#include "cryptres.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptdlg);

static HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %ld, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hInstance = hinstDLL;
            break;
    }
    return TRUE;
}

/***********************************************************************
 *		GetFriendlyNameOfCertA (CRYPTDLG.@)
 */
DWORD WINAPI GetFriendlyNameOfCertA(PCCERT_CONTEXT pccert, LPSTR pchBuffer,
                             DWORD cchBuffer)
{
    return CertGetNameStringA(pccert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
     pchBuffer, cchBuffer);
}

/***********************************************************************
 *		GetFriendlyNameOfCertW (CRYPTDLG.@)
 */
DWORD WINAPI GetFriendlyNameOfCertW(PCCERT_CONTEXT pccert, LPWSTR pchBuffer,
                             DWORD cchBuffer)
{
    return CertGetNameStringW(pccert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
     pchBuffer, cchBuffer);
}

/***********************************************************************
 *		CertTrustInit (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustInit(CRYPT_PROVIDER_DATA *pProvData)
{
    HRESULT ret = S_FALSE;

    TRACE("(%p)\n", pProvData);

    if (pProvData->padwTrustStepErrors &&
     !pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT])
        ret = S_OK;
    TRACE("returning %08lx\n", ret);
    return ret;
}

/***********************************************************************
 *		CertTrustCertPolicy (CRYPTDLG.@)
 */
BOOL WINAPI CertTrustCertPolicy(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    FIXME("(%p, %ld, %s, %ld)\n", pProvData, idxSigner, fCounterSignerChain ? "TRUE" : "FALSE", idxCounterSigner);
    return FALSE;
}

/***********************************************************************
 *		CertTrustCleanup (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustCleanup(CRYPT_PROVIDER_DATA *pProvData)
{
    FIXME("(%p)\n", pProvData);
    return E_NOTIMPL;
}

static BOOL CRYPTDLG_CheckOnlineCRL(void)
{
    HKEY key;
    BOOL ret = FALSE;

    if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE,
     L"Software\\Microsoft\\Cryptography\\{7801ebd0-cf4b-11d0-851f-0060979387ea}", 0, KEY_READ, &key))
    {
        DWORD type, flags, size = sizeof(flags);

        if (!RegQueryValueExW(key, L"PolicyFlags", NULL, &type, (BYTE *)&flags,
         &size) && type == REG_DWORD)
        {
            /* The flag values aren't defined in any header I'm aware of, but
             * this value is well documented on the net.
             */
            if (flags & 0x00010000)
                ret = TRUE;
        }
        RegCloseKey(key);
    }
    return ret;
}

/* Returns TRUE if pCert is not in the Disallowed system store, or FALSE if it
 * is.
 */
static BOOL CRYPTDLG_IsCertAllowed(PCCERT_CONTEXT pCert)
{
    BOOL ret;
    BYTE hash[20];
    DWORD size = sizeof(hash);

    if ((ret = CertGetCertificateContextProperty(pCert,
     CERT_SIGNATURE_HASH_PROP_ID, hash, &size)))
    {
        HCERTSTORE disallowed = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
         X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"Disallowed");

        if (disallowed)
        {
            PCCERT_CONTEXT found = CertFindCertificateInStore(disallowed,
             X509_ASN_ENCODING, 0, CERT_FIND_SIGNATURE_HASH, hash, NULL);

            if (found)
            {
                ret = FALSE;
                CertFreeCertificateContext(found);
            }
            CertCloseStore(disallowed, 0);
        }
    }
    return ret;
}

static DWORD CRYPTDLG_TrustStatusToConfidence(DWORD errorStatus)
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

static BOOL CRYPTDLG_CopyChain(CRYPT_PROVIDER_DATA *data,
 PCCERT_CHAIN_CONTEXT chain)
{
    BOOL ret;
    CRYPT_PROVIDER_SGNR signer;
    PCERT_SIMPLE_CHAIN simpleChain = chain->rgpChain[0];
    DWORD i;

    memset(&signer, 0, sizeof(signer));
    signer.cbStruct = sizeof(signer);
    ret = data->psPfns->pfnAddSgnr2Chain(data, FALSE, 0, &signer);
    if (ret)
    {
        CRYPT_PROVIDER_SGNR *sgnr = WTHelperGetProvSignerFromChain(data, 0,
         FALSE, 0);

        if (sgnr)
        {
            sgnr->dwError = simpleChain->TrustStatus.dwErrorStatus;
            sgnr->pChainContext = CertDuplicateCertificateChain(chain);
        }
        else
            ret = FALSE;
        for (i = 0; ret && i < simpleChain->cElement; i++)
        {
            ret = data->psPfns->pfnAddCert2Chain(data, 0, FALSE, 0,
             simpleChain->rgpElement[i]->pCertContext);
            if (ret)
            {
                CRYPT_PROVIDER_CERT *cert;

                if ((cert = WTHelperGetProvCertFromChain(sgnr, i)))
                {
                    CERT_CHAIN_ELEMENT *element = simpleChain->rgpElement[i];

                    cert->dwConfidence = CRYPTDLG_TrustStatusToConfidence(
                     element->TrustStatus.dwErrorStatus);
                    cert->dwError = element->TrustStatus.dwErrorStatus;
                    cert->pChainElement = element;
                }
                else
                    ret = FALSE;
            }
        }
    }
    return ret;
}

static CERT_VERIFY_CERTIFICATE_TRUST *CRYPTDLG_GetVerifyData(
 CRYPT_PROVIDER_DATA *data)
{
    CERT_VERIFY_CERTIFICATE_TRUST *pCert = NULL;

    /* This should always be true, but just in case the calling function is
     * called directly:
     */
    if (data->pWintrustData->dwUnionChoice == WTD_CHOICE_BLOB &&
     data->pWintrustData->pBlob && data->pWintrustData->pBlob->cbMemObject ==
     sizeof(CERT_VERIFY_CERTIFICATE_TRUST) &&
     data->pWintrustData->pBlob->pbMemObject)
         pCert = (CERT_VERIFY_CERTIFICATE_TRUST *)
          data->pWintrustData->pBlob->pbMemObject;
    return pCert;
}

static HCERTCHAINENGINE CRYPTDLG_MakeEngine(CERT_VERIFY_CERTIFICATE_TRUST *cert)
{
    HCERTCHAINENGINE engine = NULL;
    HCERTSTORE root = NULL, trust = NULL;
    DWORD i;

    if (cert->cRootStores)
    {
        root = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);
        if (root)
        {
            for (i = 0; i < cert->cRootStores; i++)
                CertAddStoreToCollection(root, cert->rghstoreRoots[i], 0, 0);
        }
    }
    if (cert->cTrustStores)
    {
        trust = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);
        if (trust)
        {
            for (i = 0; i < cert->cTrustStores; i++)
                CertAddStoreToCollection(trust, cert->rghstoreTrust[i], 0, 0);
        }
    }
    if (cert->cRootStores || cert->cStores || cert->cTrustStores)
    {
        CERT_CHAIN_ENGINE_CONFIG config;

        memset(&config, 0, sizeof(config));
        config.cbSize = sizeof(config);
        config.hRestrictedRoot = root;
        config.hRestrictedTrust = trust;
        config.cAdditionalStore = cert->cStores;
        config.rghAdditionalStore = cert->rghstoreCAs;
        config.hRestrictedRoot = root;
        CertCreateCertificateChainEngine(&config, &engine);
        CertCloseStore(root, 0);
        CertCloseStore(trust, 0);
    }
    return engine;
}

/***********************************************************************
 *		CertTrustFinalPolicy (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustFinalPolicy(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    DWORD err = S_OK;
    CERT_VERIFY_CERTIFICATE_TRUST *pCert = CRYPTDLG_GetVerifyData(data);

    TRACE("(%p)\n", data);

    if (data->pWintrustData->dwUIChoice != WTD_UI_NONE)
        FIXME("unimplemented for UI choice %ld\n",
         data->pWintrustData->dwUIChoice);
    if (pCert)
    {
        DWORD flags = 0;
        CERT_CHAIN_PARA chainPara;
        HCERTCHAINENGINE engine;

        memset(&chainPara, 0, sizeof(chainPara));
        chainPara.cbSize = sizeof(chainPara);
        if (CRYPTDLG_CheckOnlineCRL())
            flags |= CERT_CHAIN_REVOCATION_CHECK_END_CERT;
        engine = CRYPTDLG_MakeEngine(pCert);
        GetSystemTimeAsFileTime(&data->sftSystemTime);
        ret = CRYPTDLG_IsCertAllowed(pCert->pccert);
        if (ret)
        {
            PCCERT_CHAIN_CONTEXT chain;

            ret = CertGetCertificateChain(engine, pCert->pccert,
             &data->sftSystemTime, NULL, &chainPara, flags, NULL, &chain);
            if (ret)
            {
                if (chain->cChain != 1)
                {
                    FIXME("unimplemented for more than 1 simple chain\n");
                    err = TRUST_E_SUBJECT_FORM_UNKNOWN;
                    ret = FALSE;
                }
                else if ((ret = CRYPTDLG_CopyChain(data, chain)))
                {
                    if (CertVerifyTimeValidity(&data->sftSystemTime,
                     pCert->pccert->pCertInfo))
                    {
                        ret = FALSE;
                        err = CERT_E_EXPIRED;
                    }
                }
                else
                    err = TRUST_E_SYSTEM_ERROR;
                CertFreeCertificateChain(chain);
            }
            else
                err = TRUST_E_SUBJECT_NOT_TRUSTED;
        }
        CertFreeCertificateChainEngine(engine);
    }
    else
    {
        ret = FALSE;
        err = TRUST_E_NOSIGNATURE;
    }
    /* Oddly, native doesn't set the error in the trust step error location,
     * probably because this action is more advisory than anything else.
     * Instead it stores it as the final error, but the function "succeeds" in
     * any case.
     */
    if (!ret)
        data->dwFinalError = err;
    TRACE("returning %ld (%08lx)\n", S_OK, data->dwFinalError);
    return S_OK;
}

/***********************************************************************
 *		CertViewPropertiesA (CRYPTDLG.@)
 */
BOOL WINAPI CertViewPropertiesA(CERT_VIEWPROPERTIES_STRUCT_A *info)
{
    CERT_VIEWPROPERTIES_STRUCT_W infoW;
    LPWSTR title = NULL;
    BOOL ret;

    TRACE("(%p)\n", info);

    memcpy(&infoW, info, sizeof(infoW));
    if (info->szTitle)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, info->szTitle, -1, NULL, 0);

        title = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (title)
        {
            MultiByteToWideChar(CP_ACP, 0, info->szTitle, -1, title, len);
            infoW.szTitle = title;
        }
        else
        {
            ret = FALSE;
            goto error;
        }
    }
    ret = CertViewPropertiesW(&infoW);
    HeapFree(GetProcessHeap(), 0, title);
error:
    return ret;
}

/***********************************************************************
 *		CertViewPropertiesW (CRYPTDLG.@)
 */
BOOL WINAPI CertViewPropertiesW(CERT_VIEWPROPERTIES_STRUCT_W *info)
{
    static GUID cert_action_verify = CERT_CERTIFICATE_ACTION_VERIFY;
    CERT_VERIFY_CERTIFICATE_TRUST trust;
    WINTRUST_BLOB_INFO blob;
    WINTRUST_DATA wtd;
    LONG err;
    BOOL ret;

    TRACE("(%p)\n", info);

    memset(&trust, 0, sizeof(trust));
    trust.cbSize = sizeof(trust);
    trust.pccert = info->pCertContext;
    trust.cRootStores = info->cRootStores;
    trust.rghstoreRoots = info->rghstoreRoots;
    trust.cStores = info->cStores;
    trust.rghstoreCAs = info->rghstoreCAs;
    trust.cTrustStores = info->cTrustStores;
    trust.rghstoreTrust = info->rghstoreTrust;
    memset(&blob, 0, sizeof(blob));
    blob.cbStruct = sizeof(blob);
    blob.cbMemObject = sizeof(trust);
    blob.pbMemObject = (BYTE *)&trust;
    memset(&wtd, 0, sizeof(wtd));
    wtd.cbStruct = sizeof(wtd);
    wtd.dwUIChoice = WTD_UI_NONE;
    wtd.dwUnionChoice = WTD_CHOICE_BLOB;
    wtd.pBlob = &blob;
    wtd.dwStateAction = WTD_STATEACTION_VERIFY;
    err = WinVerifyTrust(NULL, &cert_action_verify, &wtd);
    if (err == ERROR_SUCCESS)
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCTW uiInfo;
        BOOL propsChanged = FALSE;

        memset(&uiInfo, 0, sizeof(uiInfo));
        uiInfo.dwSize = sizeof(uiInfo);
        uiInfo.hwndParent = info->hwndParent;
        uiInfo.dwFlags =
         CRYPTUI_DISABLE_ADDTOSTORE | CRYPTUI_ENABLE_EDITPROPERTIES;
        uiInfo.szTitle = info->szTitle;
        uiInfo.pCertContext = info->pCertContext;
        uiInfo.cPurposes = info->cArrayPurposes;
        uiInfo.rgszPurposes = (LPCSTR *)info->arrayPurposes;
        uiInfo.hWVTStateData = wtd.hWVTStateData;
        uiInfo.fpCryptProviderDataTrustedUsage = TRUE;
        uiInfo.cPropSheetPages = info->cArrayPropSheetPages;
        uiInfo.rgPropSheetPages = info->arrayPropSheetPages;
        uiInfo.nStartPage = info->nStartPage;
        ret = CryptUIDlgViewCertificateW(&uiInfo, &propsChanged);
        wtd.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(NULL, &cert_action_verify, &wtd);
    }
    else
        ret = FALSE;
    return ret;
}

static BOOL CRYPT_FormatHexString(const BYTE *pbEncoded, DWORD cbEncoded,
 WCHAR *str, DWORD *pcchStr)
{
    BOOL ret;
    DWORD charsNeeded;

    if (cbEncoded)
        charsNeeded = (cbEncoded * 3);
    else
        charsNeeded = 1;
    if (!str)
    {
        *pcchStr = charsNeeded;
        ret = TRUE;
    }
    else if (*pcchStr < charsNeeded)
    {
        *pcchStr = charsNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        DWORD i;
        LPWSTR ptr = str;

        *pcchStr = charsNeeded;
        if (cbEncoded)
        {
            for (i = 0; i < cbEncoded; i++)
            {
                if (i < cbEncoded - 1)
                    ptr += swprintf(ptr, 4, L"%02x ", pbEncoded[i]);
                else
                    ptr += swprintf(ptr, 3, L"%02x", pbEncoded[i]);
            }
        }
        else
            *ptr = 0;
        ret = TRUE;
    }
    return ret;
}

static const WCHAR indent[] = L"     ";

static BOOL CRYPT_FormatCPS(DWORD dwCertEncodingType,
 DWORD dwFormatStrType, const BYTE *pbEncoded, DWORD cbEncoded,
 WCHAR *str, DWORD *pcchStr)
{
    BOOL ret;
    DWORD size, charsNeeded = 1;
    CERT_NAME_VALUE *cpsValue;

    if ((ret = CryptDecodeObjectEx(dwCertEncodingType, X509_UNICODE_ANY_STRING,
     pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL, &cpsValue, &size)))
    {
        LPCWSTR sep;
        DWORD sepLen;

        if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            sep = L"\r\n";
        else
            sep = L", ";

        sepLen = lstrlenW(sep);

        if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        {
            charsNeeded += 3 * lstrlenW(indent);
            if (str && *pcchStr >= charsNeeded)
            {
                lstrcpyW(str, indent);
                str += lstrlenW(indent);
                lstrcpyW(str, indent);
                str += lstrlenW(indent);
                lstrcpyW(str, indent);
                str += lstrlenW(indent);
            }
        }
        charsNeeded += cpsValue->Value.cbData / sizeof(WCHAR);
        if (str && *pcchStr >= charsNeeded)
        {
            lstrcpyW(str, (LPWSTR)cpsValue->Value.pbData);
            str += cpsValue->Value.cbData / sizeof(WCHAR);
        }
        charsNeeded += sepLen;
        if (str && *pcchStr >= charsNeeded)
        {
            lstrcpyW(str, sep);
            str += sepLen;
        }
        LocalFree(cpsValue);
        if (!str)
            *pcchStr = charsNeeded;
        else if (*pcchStr < charsNeeded)
        {
            *pcchStr = charsNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
            *pcchStr = charsNeeded;
    }
    return ret;
}

static BOOL CRYPT_FormatUserNotice(DWORD dwCertEncodingType,
 DWORD dwFormatStrType, const BYTE *pbEncoded, DWORD cbEncoded,
 WCHAR *str, DWORD *pcchStr)
{
    BOOL ret;
    DWORD size, charsNeeded = 1;
    CERT_POLICY_QUALIFIER_USER_NOTICE *notice;

    if ((ret = CryptDecodeObjectEx(dwCertEncodingType,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE, pbEncoded, cbEncoded,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &notice, &size)))
    {
        CERT_POLICY_QUALIFIER_NOTICE_REFERENCE *pNoticeRef =
         notice->pNoticeReference;
        LPCWSTR headingSep, sep;
        DWORD headingSepLen, sepLen;
        LPWSTR noticeRef, organization, noticeNum, noticeText;
        DWORD noticeRefLen, organizationLen, noticeNumLen, noticeTextLen;
        WCHAR noticeNumStr[11];

        noticeRefLen = LoadStringW(hInstance, IDS_NOTICE_REF,
         (LPWSTR)&noticeRef, 0);
        organizationLen = LoadStringW(hInstance, IDS_ORGANIZATION,
         (LPWSTR)&organization, 0);
        noticeNumLen = LoadStringW(hInstance, IDS_NOTICE_NUM,
         (LPWSTR)&noticeNum, 0);
        noticeTextLen = LoadStringW(hInstance, IDS_NOTICE_TEXT,
         (LPWSTR)&noticeText, 0);
        if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        {
            headingSep = L":\r\n";
            sep = L"\r\n";
        }
        else
        {
            headingSep = L": ";
            sep = L", ";
        }
        sepLen = lstrlenW(sep);
        headingSepLen = lstrlenW(headingSep);

        if (pNoticeRef)
        {
            DWORD k;
            LPCSTR src;

            if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            {
                charsNeeded += 3 * lstrlenW(indent);
                if (str && *pcchStr >= charsNeeded)
                {
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                }
            }
            charsNeeded += noticeRefLen;
            if (str && *pcchStr >= charsNeeded)
            {
                memcpy(str, noticeRef, noticeRefLen * sizeof(WCHAR));
                str += noticeRefLen;
            }
            charsNeeded += headingSepLen;
            if (str && *pcchStr >= charsNeeded)
            {
                lstrcpyW(str, headingSep);
                str += headingSepLen;
            }
            if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            {
                charsNeeded += 4 * lstrlenW(indent);
                if (str && *pcchStr >= charsNeeded)
                {
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                }
            }
            charsNeeded += organizationLen;
            if (str && *pcchStr >= charsNeeded)
            {
                memcpy(str, organization, organizationLen * sizeof(WCHAR));
                str += organizationLen;
            }
            charsNeeded += strlen(pNoticeRef->pszOrganization);
            if (str && *pcchStr >= charsNeeded)
                for (src = pNoticeRef->pszOrganization; src && *src;
                 src++, str++)
                    *str = *src;
            charsNeeded += sepLen;
            if (str && *pcchStr >= charsNeeded)
            {
                lstrcpyW(str, sep);
                str += sepLen;
            }
            for (k = 0; k < pNoticeRef->cNoticeNumbers; k++)
            {
                if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                {
                    charsNeeded += 4 * lstrlenW(indent);
                    if (str && *pcchStr >= charsNeeded)
                    {
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                    }
                }
                charsNeeded += noticeNumLen;
                if (str && *pcchStr >= charsNeeded)
                {
                    memcpy(str, noticeNum, noticeNumLen * sizeof(WCHAR));
                    str += noticeNumLen;
                }
                swprintf(noticeNumStr, ARRAY_SIZE(noticeNumStr), L"%d", k + 1);
                charsNeeded += lstrlenW(noticeNumStr);
                if (str && *pcchStr >= charsNeeded)
                {
                    lstrcpyW(str, noticeNumStr);
                    str += lstrlenW(noticeNumStr);
                }
                charsNeeded += sepLen;
                if (str && *pcchStr >= charsNeeded)
                {
                    lstrcpyW(str, sep);
                    str += sepLen;
                }
            }
        }
        if (notice->pszDisplayText)
        {
            if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            {
                charsNeeded += 3 * lstrlenW(indent);
                if (str && *pcchStr >= charsNeeded)
                {
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                }
            }
            charsNeeded += noticeTextLen;
            if (str && *pcchStr >= charsNeeded)
            {
                memcpy(str, noticeText, noticeTextLen * sizeof(WCHAR));
                str += noticeTextLen;
            }
            charsNeeded += lstrlenW(notice->pszDisplayText);
            if (str && *pcchStr >= charsNeeded)
            {
                lstrcpyW(str, notice->pszDisplayText);
                str += lstrlenW(notice->pszDisplayText);
            }
            charsNeeded += sepLen;
            if (str && *pcchStr >= charsNeeded)
            {
                lstrcpyW(str, sep);
                str += sepLen;
            }
        }
        LocalFree(notice);
        if (!str)
            *pcchStr = charsNeeded;
        else if (*pcchStr < charsNeeded)
        {
            *pcchStr = charsNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
            *pcchStr = charsNeeded;
    }
    return ret;
}

/***********************************************************************
 *		FormatVerisignExtension (CRYPTDLG.@)
 */
BOOL WINAPI FormatVerisignExtension(DWORD dwCertEncodingType,
 DWORD dwFormatType, DWORD dwFormatStrType, void *pFormatStruct,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, void *pbFormat,
 DWORD *pcbFormat)
{
    CERT_POLICIES_INFO *policies;
    DWORD size;
    BOOL ret = FALSE;

    if (!cbEncoded)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if ((ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT_POLICIES,
     pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL, &policies, &size)))
    {
        DWORD charsNeeded = 1; /* space for NULL terminator */
        LPCWSTR headingSep, sep;
        DWORD headingSepLen, sepLen;
        WCHAR policyNum[11], policyQualifierNum[11];
        LPWSTR certPolicy, policyId, policyQualifierInfo, policyQualifierId;
        LPWSTR cps, userNotice, qualifier;
        DWORD certPolicyLen, policyIdLen, policyQualifierInfoLen;
        DWORD policyQualifierIdLen, cpsLen, userNoticeLen, qualifierLen;
        DWORD i;
        LPWSTR str = pbFormat;

        certPolicyLen = LoadStringW(hInstance, IDS_CERT_POLICY,
         (LPWSTR)&certPolicy, 0);
        policyIdLen = LoadStringW(hInstance, IDS_POLICY_ID, (LPWSTR)&policyId,
         0);
        policyQualifierInfoLen = LoadStringW(hInstance,
         IDS_POLICY_QUALIFIER_INFO, (LPWSTR)&policyQualifierInfo, 0);
        policyQualifierIdLen = LoadStringW(hInstance, IDS_POLICY_QUALIFIER_ID,
         (LPWSTR)&policyQualifierId, 0);
        cpsLen = LoadStringW(hInstance, IDS_CPS, (LPWSTR)&cps, 0);
        userNoticeLen = LoadStringW(hInstance, IDS_USER_NOTICE,
         (LPWSTR)&userNotice, 0);
        qualifierLen = LoadStringW(hInstance, IDS_QUALIFIER,
         (LPWSTR)&qualifier, 0);
        if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        {
            headingSep = L":\r\n";
            sep = L"\r\n";
        }
        else
        {
            headingSep = L": ";
            sep = L", ";
        }
        sepLen = lstrlenW(sep);
        headingSepLen = lstrlenW(headingSep);

        for (i = 0; ret && i < policies->cPolicyInfo; i++)
        {
            CERT_POLICY_INFO *policy = &policies->rgPolicyInfo[i];
            DWORD j;
            LPCSTR src;

            charsNeeded += 1; /* '['*/
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                *str++ = '[';
            swprintf(policyNum, ARRAY_SIZE(policyNum), L"%d", i + 1);
            charsNeeded += lstrlenW(policyNum);
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
            {
                lstrcpyW(str, policyNum);
                str += lstrlenW(policyNum);
            }
            charsNeeded += 1; /* ']'*/
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                *str++ = ']';
            charsNeeded += certPolicyLen;
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
            {
                memcpy(str, certPolicy, certPolicyLen * sizeof(WCHAR));
                str += certPolicyLen;
            }
            charsNeeded += headingSepLen;
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
            {
                lstrcpyW(str, headingSep);
                str += headingSepLen;
            }
            if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            {
                charsNeeded += lstrlenW(indent);
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    lstrcpyW(str, indent);
                    str += lstrlenW(indent);
                }
            }
            charsNeeded += policyIdLen;
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
            {
                memcpy(str, policyId, policyIdLen * sizeof(WCHAR));
                str += policyIdLen;
            }
            charsNeeded += strlen(policy->pszPolicyIdentifier);
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
            {
                for (src = policy->pszPolicyIdentifier; src && *src;
                 src++, str++)
                    *str = *src;
            }
            charsNeeded += sepLen;
            if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
            {
                lstrcpyW(str, sep);
                str += sepLen;
            }
            for (j = 0; j < policy->cPolicyQualifier; j++)
            {
                CERT_POLICY_QUALIFIER_INFO *qualifierInfo =
                 &policy->rgPolicyQualifier[j];
                DWORD sizeRemaining;

                if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                {
                    charsNeeded += lstrlenW(indent);
                    if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    {
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                    }
                }
                charsNeeded += 1; /* '['*/
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    *str++ = '[';
                charsNeeded += lstrlenW(policyNum);
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    lstrcpyW(str, policyNum);
                    str += lstrlenW(policyNum);
                }
                charsNeeded += 1; /* ','*/
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    *str++ = ',';
                swprintf(policyQualifierNum, ARRAY_SIZE(policyQualifierNum), L"%d", j + 1);
                charsNeeded += lstrlenW(policyQualifierNum);
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    lstrcpyW(str, policyQualifierNum);
                    str += lstrlenW(policyQualifierNum);
                }
                charsNeeded += 1; /* ']'*/
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    *str++ = ']';
                charsNeeded += policyQualifierInfoLen;
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    memcpy(str, policyQualifierInfo,
                     policyQualifierInfoLen * sizeof(WCHAR));
                    str += policyQualifierInfoLen;
                }
                charsNeeded += headingSepLen;
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    lstrcpyW(str, headingSep);
                    str += headingSepLen;
                }
                if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                {
                    charsNeeded += 2 * lstrlenW(indent);
                    if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    {
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                    }
                }
                charsNeeded += policyQualifierIdLen;
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    memcpy(str, policyQualifierId,
                     policyQualifierIdLen * sizeof(WCHAR));
                    str += policyQualifierIdLen;
                }
                if (!strcmp(qualifierInfo->pszPolicyQualifierId,
                 szOID_PKIX_POLICY_QUALIFIER_CPS))
                {
                    charsNeeded += cpsLen;
                    if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    {
                        memcpy(str, cps, cpsLen * sizeof(WCHAR));
                        str += cpsLen;
                    }
                }
                else if (!strcmp(qualifierInfo->pszPolicyQualifierId,
                 szOID_PKIX_POLICY_QUALIFIER_USERNOTICE))
                {
                    charsNeeded += userNoticeLen;
                    if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    {
                        memcpy(str, userNotice, userNoticeLen * sizeof(WCHAR));
                        str += userNoticeLen;
                    }
                }
                else
                {
                    charsNeeded += strlen(qualifierInfo->pszPolicyQualifierId);
                    if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    {
                        for (src = qualifierInfo->pszPolicyQualifierId;
                         src && *src; src++, str++)
                            *str = *src;
                    }
                }
                charsNeeded += sepLen;
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    lstrcpyW(str, sep);
                    str += sepLen;
                }
                if (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                {
                    charsNeeded += 2 * lstrlenW(indent);
                    if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                    {
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                        lstrcpyW(str, indent);
                        str += lstrlenW(indent);
                    }
                }
                charsNeeded += qualifierLen;
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    memcpy(str, qualifier, qualifierLen * sizeof(WCHAR));
                    str += qualifierLen;
                }
                charsNeeded += headingSepLen;
                if (str && *pcbFormat >= charsNeeded * sizeof(WCHAR))
                {
                    lstrcpyW(str, headingSep);
                    str += headingSepLen;
                }
                /* This if block is deliberately redundant with the same if
                 * block above, in order to keep the code more readable (the
                 * code flow follows the order in which the strings are output.)
                 */
                if (!strcmp(qualifierInfo->pszPolicyQualifierId,
                 szOID_PKIX_POLICY_QUALIFIER_CPS))
                {
                    if (!str || *pcbFormat < charsNeeded * sizeof(WCHAR))
                    {
                        /* Insufficient space, determine how much is needed. */
                        ret = CRYPT_FormatCPS(dwCertEncodingType,
                         dwFormatStrType, qualifierInfo->Qualifier.pbData,
                         qualifierInfo->Qualifier.cbData, NULL, &size);
                        if (ret)
                            charsNeeded += size - 1;
                    }
                    else
                    {
                        sizeRemaining = *pcbFormat / sizeof(WCHAR);
                        sizeRemaining -= str - (LPWSTR)pbFormat;
                        ret = CRYPT_FormatCPS(dwCertEncodingType,
                         dwFormatStrType, qualifierInfo->Qualifier.pbData,
                         qualifierInfo->Qualifier.cbData, str, &sizeRemaining);
                        if (ret || GetLastError() == ERROR_MORE_DATA)
                        {
                            charsNeeded += sizeRemaining - 1;
                            str += sizeRemaining - 1;
                        }
                    }
                }
                else if (!strcmp(qualifierInfo->pszPolicyQualifierId,
                 szOID_PKIX_POLICY_QUALIFIER_USERNOTICE))
                {
                    if (!str || *pcbFormat < charsNeeded * sizeof(WCHAR))
                    {
                        /* Insufficient space, determine how much is needed. */
                        ret = CRYPT_FormatUserNotice(dwCertEncodingType,
                         dwFormatStrType, qualifierInfo->Qualifier.pbData,
                         qualifierInfo->Qualifier.cbData, NULL, &size);
                        if (ret)
                            charsNeeded += size - 1;
                    }
                    else
                    {
                        sizeRemaining = *pcbFormat / sizeof(WCHAR);
                        sizeRemaining -= str - (LPWSTR)pbFormat;
                        ret = CRYPT_FormatUserNotice(dwCertEncodingType,
                         dwFormatStrType, qualifierInfo->Qualifier.pbData,
                         qualifierInfo->Qualifier.cbData, str, &sizeRemaining);
                        if (ret || GetLastError() == ERROR_MORE_DATA)
                        {
                            charsNeeded += sizeRemaining - 1;
                            str += sizeRemaining - 1;
                        }
                    }
                }
                else
                {
                    if (!str || *pcbFormat < charsNeeded * sizeof(WCHAR))
                    {
                        /* Insufficient space, determine how much is needed. */
                        ret = CRYPT_FormatHexString(
                         qualifierInfo->Qualifier.pbData,
                         qualifierInfo->Qualifier.cbData, NULL, &size);
                        if (ret)
                            charsNeeded += size - 1;
                    }
                    else
                    {
                        sizeRemaining = *pcbFormat / sizeof(WCHAR);
                        sizeRemaining -= str - (LPWSTR)pbFormat;
                        ret = CRYPT_FormatHexString(
                         qualifierInfo->Qualifier.pbData,
                         qualifierInfo->Qualifier.cbData, str, &sizeRemaining);
                        if (ret || GetLastError() == ERROR_MORE_DATA)
                        {
                            charsNeeded += sizeRemaining - 1;
                            str += sizeRemaining - 1;
                        }
                    }
                }
            }
        }
        LocalFree(policies);
        if (ret)
        {
            if (!pbFormat)
                *pcbFormat = charsNeeded * sizeof(WCHAR);
            else if (*pcbFormat < charsNeeded * sizeof(WCHAR))
            {
                *pcbFormat = charsNeeded * sizeof(WCHAR);
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
                *pcbFormat = charsNeeded * sizeof(WCHAR);
        }
    }
    return ret;
}

#define szOID_MICROSOFT_Encryption_Key_Preference "1.3.6.1.4.1.311.16.4"

/***********************************************************************
 *		DllRegisterServer (CRYPTDLG.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static WCHAR cryptdlg[] = L"cryptdlg.dll";
    static WCHAR wintrust[] = L"wintrust.dll";
    static WCHAR certTrustInit[] = L"CertTrustInit";
    static WCHAR wintrustCertificateTrust[] = L"WintrustCertificateTrust";
    static WCHAR certTrustCertPolicy[] = L"CertTrustCertPolicy";
    static WCHAR certTrustFinalPolicy[] = L"CertTrustFinalPolicy";
    static WCHAR certTrustCleanup[] = L"CertTrustCleanup";
    CRYPT_REGISTER_ACTIONID reg;
    GUID guid = CERT_CERTIFICATE_ACTION_VERIFY;
    HRESULT hr = S_OK;

    memset(&reg, 0, sizeof(reg));
    reg.cbStruct = sizeof(reg);
    reg.sInitProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sInitProvider.pwszDLLName = cryptdlg;
    reg.sInitProvider.pwszFunctionName = certTrustInit;
    reg.sCertificateProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sCertificateProvider.pwszDLLName = wintrust;
    reg.sCertificateProvider.pwszFunctionName = wintrustCertificateTrust;
    reg.sCertificatePolicyProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sCertificatePolicyProvider.pwszDLLName = cryptdlg;
    reg.sCertificatePolicyProvider.pwszFunctionName = certTrustCertPolicy;
    reg.sFinalPolicyProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sFinalPolicyProvider.pwszDLLName = cryptdlg;
    reg.sFinalPolicyProvider.pwszFunctionName = certTrustFinalPolicy;
    reg.sCleanupProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sCleanupProvider.pwszDLLName = cryptdlg;
    reg.sCleanupProvider.pwszFunctionName = certTrustCleanup;
    if (!WintrustAddActionID(&guid, WT_ADD_ACTION_ID_RET_RESULT_FLAG, &reg))
        hr = GetLastError();
    CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_ENCODE_OBJECT_FUNC,
     "1.3.6.1.4.1.311.16.1.1", L"cryptdlg.dll", "EncodeAttrSequence");
    CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_ENCODE_OBJECT_FUNC,
     szOID_MICROSOFT_Encryption_Key_Preference, L"cryptdlg.dll", "EncodeRecipientID");
    CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_DECODE_OBJECT_FUNC,
     "1.3.6.1.4.1.311.16.1.1", L"cryptdlg.dll", "DecodeAttrSequence");
    CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_DECODE_OBJECT_FUNC,
     szOID_MICROSOFT_Encryption_Key_Preference, L"cryptdlg.dll", "DecodeRecipientID");
    CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_FORMAT_OBJECT_FUNC,
     szOID_PKIX_KP_EMAIL_PROTECTION, L"cryptdlg.dll", "FormatPKIXEmailProtection");
    CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_FORMAT_OBJECT_FUNC,
     szOID_CERT_POLICIES, L"cryptdlg.dll", "FormatVerisignExtension");
    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (CRYPTDLG.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    GUID guid = CERT_CERTIFICATE_ACTION_VERIFY;

    WintrustRemoveActionID(&guid);
    CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_ENCODE_OBJECT_FUNC,
     "1.3.6.1.4.1.311.16.1.1");
    CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_ENCODE_OBJECT_FUNC,
     szOID_MICROSOFT_Encryption_Key_Preference);
    CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_DECODE_OBJECT_FUNC,
     "1.3.6.1.4.1.311.16.1.1");
    CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_DECODE_OBJECT_FUNC,
     szOID_MICROSOFT_Encryption_Key_Preference);
    CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_FORMAT_OBJECT_FUNC,
     szOID_PKIX_KP_EMAIL_PROTECTION);
    CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_FORMAT_OBJECT_FUNC,
     szOID_CERT_POLICIES);
    return S_OK;
}

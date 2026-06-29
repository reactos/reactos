/*
 * Copyright (C) 2006 Maarten Lankhorst
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
 *
 */

#define CERT_REVOCATION_PARA_HAS_EXTRA_FIELDS

#include <share.h>
#include <stdio.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winnls.h"
#include "wininet.h"
#include "objbase.h"
#include "wincrypt.h"
#include "initguid.h"
#include "knownfolders.h"
#include "shlobj.h"
#ifdef __REACTOS__
#include <shlwapi.h>
#endif

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptnet);

#define IS_INTOID(x)    (((ULONG_PTR)(x) >> 16) == 0)

#ifdef __REACTOS__
/* We don't have SHGetKnownFolderPath yet */
#define SHGetKnownFolderPath(a,b,c,out) CryptnetSHGetKnownFolderPath(out)
static HRESULT CryptnetSHGetKnownFolderPath(WCHAR **out)
{
    WCHAR buf[MAX_PATH];
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
    if (SUCCEEDED(hr))
    {
        hr = SHStrDupW(buf, out);
    }
    return hr;
}
#endif /* __REACTOS__ */

/***********************************************************************
 *    DllRegisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
   TRACE("\n");
   CryptRegisterDefaultOIDFunction(X509_ASN_ENCODING,
    CRYPT_OID_VERIFY_REVOCATION_FUNC, 0, L"cryptnet.dll");
   CryptRegisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC, "Ldap",
    L"cryptnet.dll", "LdapProvOpenStore");
   CryptRegisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC,
    CERT_STORE_PROV_LDAP_W, L"cryptnet.dll", "LdapProvOpenStore");
   return S_OK;
}

/***********************************************************************
 *    DllUnregisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
   TRACE("\n");
   CryptUnregisterDefaultOIDFunction(X509_ASN_ENCODING,
    CRYPT_OID_VERIFY_REVOCATION_FUNC, L"cryptnet.dll");
   CryptUnregisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC, "Ldap");
   CryptUnregisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC,
    CERT_STORE_PROV_LDAP_W);
   return S_OK;
}

static const char *url_oid_to_str(LPCSTR oid)
{
    if (IS_INTOID(oid))
    {
        static char buf[10];

        switch (LOWORD(oid))
        {
#define _x(oid) case LOWORD(oid): return #oid
        _x(URL_OID_CERTIFICATE_ISSUER);
        _x(URL_OID_CERTIFICATE_CRL_DIST_POINT);
        _x(URL_OID_CTL_ISSUER);
        _x(URL_OID_CTL_NEXT_UPDATE);
        _x(URL_OID_CRL_ISSUER);
        _x(URL_OID_CERTIFICATE_FRESHEST_CRL);
        _x(URL_OID_CRL_FRESHEST_CRL);
        _x(URL_OID_CROSS_CERT_DIST_POINT);
#undef _x
        default:
            snprintf(buf, sizeof(buf), "%d", LOWORD(oid));
            return buf;
        }
    }
    else
        return oid;
}

typedef BOOL (WINAPI *UrlDllGetObjectUrlFunc)(LPCSTR, LPVOID, DWORD,
 PCRYPT_URL_ARRAY, DWORD *, PCRYPT_URL_INFO, DWORD *, LPVOID);

static BOOL WINAPI CRYPT_GetUrlFromCertificateIssuer(LPCSTR pszUrlOid,
 LPVOID pvPara, DWORD dwFlags, PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray,
 PCRYPT_URL_INFO pUrlInfo, DWORD *pcbUrlInfo, LPVOID pvReserved)
{
    PCCERT_CONTEXT cert = pvPara;
    PCERT_EXTENSION ext;
    BOOL ret = FALSE;

    /* The only applicable flag is CRYPT_GET_URL_FROM_EXTENSION */
    if (dwFlags && !(dwFlags & CRYPT_GET_URL_FROM_EXTENSION))
    {
        SetLastError(CRYPT_E_NOT_FOUND);
        return FALSE;
    }
    if ((ext = CertFindExtension(szOID_AUTHORITY_INFO_ACCESS,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        CERT_AUTHORITY_INFO_ACCESS *aia;
        DWORD size;

        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_AUTHORITY_INFO_ACCESS,
         ext->Value.pbData, ext->Value.cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
         &aia, &size);
        if (ret)
        {
            DWORD i, cUrl, bytesNeeded = sizeof(CRYPT_URL_ARRAY);

            for (i = 0, cUrl = 0; i < aia->cAccDescr; i++)
                if (!strcmp(aia->rgAccDescr[i].pszAccessMethod,
                 szOID_PKIX_CA_ISSUERS))
                {
                    if (aia->rgAccDescr[i].AccessLocation.dwAltNameChoice ==
                     CERT_ALT_NAME_URL)
                    {
                        if (aia->rgAccDescr[i].AccessLocation.pwszURL)
                        {
                            cUrl++;
                            bytesNeeded += sizeof(LPWSTR) +
                             (lstrlenW(aia->rgAccDescr[i].AccessLocation.
                             pwszURL) + 1) * sizeof(WCHAR);
                        }
                    }
                    else
                        FIXME("unsupported alt name type %ld\n",
                         aia->rgAccDescr[i].AccessLocation.dwAltNameChoice);
                }
            if (!pcbUrlArray)
            {
                SetLastError(E_INVALIDARG);
                ret = FALSE;
            }
            else if (!pUrlArray)
                *pcbUrlArray = bytesNeeded;
            else if (*pcbUrlArray < bytesNeeded)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbUrlArray = bytesNeeded;
                ret = FALSE;
            }
            else
            {
                LPWSTR nextUrl;

                *pcbUrlArray = bytesNeeded;
                pUrlArray->cUrl = 0;
                pUrlArray->rgwszUrl =
                 (LPWSTR *)((BYTE *)pUrlArray + sizeof(CRYPT_URL_ARRAY));
                nextUrl = (LPWSTR)((BYTE *)pUrlArray + sizeof(CRYPT_URL_ARRAY)
                 + cUrl * sizeof(LPWSTR));
                for (i = 0; i < aia->cAccDescr; i++)
                    if (!strcmp(aia->rgAccDescr[i].pszAccessMethod,
                     szOID_PKIX_CA_ISSUERS))
                    {
                        if (aia->rgAccDescr[i].AccessLocation.dwAltNameChoice
                         == CERT_ALT_NAME_URL)
                        {
                            if (aia->rgAccDescr[i].AccessLocation.pwszURL)
                            {
                                lstrcpyW(nextUrl,
                                 aia->rgAccDescr[i].AccessLocation.pwszURL);
                                pUrlArray->rgwszUrl[pUrlArray->cUrl++] =
                                 nextUrl;
                                nextUrl += (lstrlenW(nextUrl) + 1);
                            }
                        }
                    }
            }
            if (ret)
            {
                if (pcbUrlInfo)
                {
                    FIXME("url info: stub\n");
                    if (!pUrlInfo)
                        *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                    else if (*pcbUrlInfo < sizeof(CRYPT_URL_INFO))
                    {
                        *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                        SetLastError(ERROR_MORE_DATA);
                        ret = FALSE;
                    }
                    else
                    {
                        *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                        memset(pUrlInfo, 0, sizeof(CRYPT_URL_INFO));
                    }
                }
            }
            LocalFree(aia);
        }
    }
    else
        SetLastError(CRYPT_E_NOT_FOUND);
    return ret;
}

static BOOL CRYPT_GetUrlFromCRLDistPointsExt(const CRYPT_DATA_BLOB *value,
 PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray, PCRYPT_URL_INFO pUrlInfo,
 DWORD *pcbUrlInfo)
{
    BOOL ret;
    CRL_DIST_POINTS_INFO *info;
    DWORD size;

    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CRL_DIST_POINTS,
     value->pbData, value->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    if (ret)
    {
        DWORD i, cUrl, bytesNeeded = sizeof(CRYPT_URL_ARRAY);

        for (i = 0, cUrl = 0; i < info->cDistPoint; i++)
            if (info->rgDistPoint[i].DistPointName.dwDistPointNameChoice
             == CRL_DIST_POINT_FULL_NAME)
            {
                DWORD j;
                CERT_ALT_NAME_INFO *name =
                 &info->rgDistPoint[i].DistPointName.FullName;

                for (j = 0; j < name->cAltEntry; j++)
                    if (name->rgAltEntry[j].dwAltNameChoice ==
                     CERT_ALT_NAME_URL)
                    {
                        if (name->rgAltEntry[j].pwszURL)
                        {
                            cUrl++;
                            bytesNeeded += sizeof(LPWSTR) +
                             (lstrlenW(name->rgAltEntry[j].pwszURL) + 1)
                             * sizeof(WCHAR);
                        }
                    }
            }
        if (!pcbUrlArray)
        {
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        else if (!pUrlArray)
            *pcbUrlArray = bytesNeeded;
        else if (*pcbUrlArray < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbUrlArray = bytesNeeded;
            ret = FALSE;
        }
        else
        {
            LPWSTR nextUrl;

            *pcbUrlArray = bytesNeeded;
            pUrlArray->cUrl = 0;
            pUrlArray->rgwszUrl =
             (LPWSTR *)((BYTE *)pUrlArray + sizeof(CRYPT_URL_ARRAY));
            nextUrl = (LPWSTR)((BYTE *)pUrlArray + sizeof(CRYPT_URL_ARRAY)
             + cUrl * sizeof(LPWSTR));
            for (i = 0; i < info->cDistPoint; i++)
                if (info->rgDistPoint[i].DistPointName.dwDistPointNameChoice
                 == CRL_DIST_POINT_FULL_NAME)
                {
                    DWORD j;
                    CERT_ALT_NAME_INFO *name =
                     &info->rgDistPoint[i].DistPointName.FullName;

                    for (j = 0; j < name->cAltEntry; j++)
                        if (name->rgAltEntry[j].dwAltNameChoice ==
                         CERT_ALT_NAME_URL)
                        {
                            if (name->rgAltEntry[j].pwszURL)
                            {
                                lstrcpyW(nextUrl,
                                 name->rgAltEntry[j].pwszURL);
                                pUrlArray->rgwszUrl[pUrlArray->cUrl++] =
                                 nextUrl;
                                nextUrl +=
                                 (lstrlenW(name->rgAltEntry[j].pwszURL) + 1);
                            }
                        }
                }
        }
        if (ret)
        {
            if (pcbUrlInfo)
            {
                FIXME("url info: stub\n");
                if (!pUrlInfo)
                    *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                else if (*pcbUrlInfo < sizeof(CRYPT_URL_INFO))
                {
                    *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                    memset(pUrlInfo, 0, sizeof(CRYPT_URL_INFO));
                }
            }
        }
        LocalFree(info);
    }
    return ret;
}

static BOOL WINAPI CRYPT_GetUrlFromCertificateCRLDistPoint(LPCSTR pszUrlOid,
 LPVOID pvPara, DWORD dwFlags, PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray,
 PCRYPT_URL_INFO pUrlInfo, DWORD *pcbUrlInfo, LPVOID pvReserved)
{
    PCCERT_CONTEXT cert = pvPara;
    PCERT_EXTENSION ext;
    BOOL ret = FALSE;

    /* The only applicable flag is CRYPT_GET_URL_FROM_EXTENSION */
    if (dwFlags && !(dwFlags & CRYPT_GET_URL_FROM_EXTENSION))
    {
        SetLastError(CRYPT_E_NOT_FOUND);
        return FALSE;
    }
    if ((ext = CertFindExtension(szOID_CRL_DIST_POINTS,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
        ret = CRYPT_GetUrlFromCRLDistPointsExt(&ext->Value, pUrlArray,
         pcbUrlArray, pUrlInfo, pcbUrlInfo);
    else
        SetLastError(CRYPT_E_NOT_FOUND);
    return ret;
}

/***********************************************************************
 *    CryptGetObjectUrl (CRYPTNET.@)
 */
BOOL WINAPI CryptGetObjectUrl(LPCSTR pszUrlOid, LPVOID pvPara, DWORD dwFlags,
 PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray, PCRYPT_URL_INFO pUrlInfo,
 DWORD *pcbUrlInfo, LPVOID pvReserved)
{
    UrlDllGetObjectUrlFunc func = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;
    BOOL ret = FALSE;

    TRACE("(%s, %p, %08lx, %p, %p, %p, %p, %p)\n", debugstr_a(pszUrlOid),
     pvPara, dwFlags, pUrlArray, pcbUrlArray, pUrlInfo, pcbUrlInfo, pvReserved);

    if (IS_INTOID(pszUrlOid))
    {
        switch (LOWORD(pszUrlOid))
        {
        case LOWORD(URL_OID_CERTIFICATE_ISSUER):
            func = CRYPT_GetUrlFromCertificateIssuer;
            break;
        case LOWORD(URL_OID_CERTIFICATE_CRL_DIST_POINT):
            func = CRYPT_GetUrlFromCertificateCRLDistPoint;
            break;
        default:
            FIXME("unimplemented for %s\n", url_oid_to_str(pszUrlOid));
            SetLastError(ERROR_FILE_NOT_FOUND);
        }
    }
    else
    {
        static HCRYPTOIDFUNCSET set = NULL;

        if (!set)
            set = CryptInitOIDFunctionSet(URL_OID_GET_OBJECT_URL_FUNC, 0);
        CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, pszUrlOid, 0,
         (void **)&func, &hFunc);
    }
    if (func)
        ret = func(pszUrlOid, pvPara, dwFlags, pUrlArray, pcbUrlArray,
         pUrlInfo, pcbUrlInfo, pvReserved);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

/***********************************************************************
 *    CryptRetrieveObjectByUrlA (CRYPTNET.@)
 */
BOOL WINAPI CryptRetrieveObjectByUrlA(LPCSTR pszURL, LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, DWORD dwTimeout, LPVOID *ppvObject,
 HCRYPTASYNC hAsyncRetrieve, PCRYPT_CREDENTIALS pCredentials, LPVOID pvVerify,
 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    BOOL ret = FALSE;
    int len;

    TRACE("(%s, %s, %08lx, %ld, %p, %p, %p, %p, %p)\n", debugstr_a(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, ppvObject,
     hAsyncRetrieve, pCredentials, pvVerify, pAuxInfo);

    if (!pszURL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    len = MultiByteToWideChar(CP_ACP, 0, pszURL, -1, NULL, 0);
    if (len)
    {
        LPWSTR url = CryptMemAlloc(len * sizeof(WCHAR));

        if (url)
        {
            MultiByteToWideChar(CP_ACP, 0, pszURL, -1, url, len);
            ret = CryptRetrieveObjectByUrlW(url, pszObjectOid,
             dwRetrievalFlags, dwTimeout, ppvObject, hAsyncRetrieve,
             pCredentials, pvVerify, pAuxInfo);
            CryptMemFree(url);
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
    }
    return ret;
}

static void WINAPI CRYPT_FreeBlob(LPCSTR pszObjectOid,
 PCRYPT_BLOB_ARRAY pObject, void *pvFreeContext)
{
    DWORD i;

    for (i = 0; i < pObject->cBlob; i++)
        CryptMemFree(pObject->rgBlob[i].pbData);
    CryptMemFree(pObject->rgBlob);
}

static BOOL CRYPT_GetObjectFromFile(HANDLE hFile, PCRYPT_BLOB_ARRAY pObject)
{
    BOOL ret;
    LARGE_INTEGER size;

    if ((ret = GetFileSizeEx(hFile, &size)))
    {
        if (size.HighPart)
        {
            WARN("file too big\n");
            SetLastError(ERROR_INVALID_DATA);
            ret = FALSE;
        }
        else
        {
            CRYPT_DATA_BLOB blob;

            blob.pbData = CryptMemAlloc(size.LowPart);
            if (blob.pbData)
            {
                ret = ReadFile(hFile, blob.pbData, size.LowPart, &blob.cbData,
                 NULL);
                if (ret)
                {
                    pObject->rgBlob = CryptMemAlloc(sizeof(CRYPT_DATA_BLOB));
                    if (pObject->rgBlob)
                    {
                        pObject->cBlob = 1;
                        memcpy(pObject->rgBlob, &blob, sizeof(CRYPT_DATA_BLOB));
                    }
                    else
                    {
                        SetLastError(ERROR_OUTOFMEMORY);
                        ret = FALSE;
                    }
                }
                if (!ret)
                    CryptMemFree(blob.pbData);
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                ret = FALSE;
            }
        }
    }
    return ret;
}

static BOOL CRYPT_GetObjectFromCache(LPCWSTR pszURL, PCRYPT_BLOB_ARRAY pObject,
 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    BOOL ret = FALSE;
    INTERNET_CACHE_ENTRY_INFOW *pCacheInfo = NULL;
    DWORD size = 0;

    TRACE("(%s, %p, %p)\n", debugstr_w(pszURL), pObject, pAuxInfo);

    RetrieveUrlCacheEntryFileW(pszURL, NULL, &size, 0);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return FALSE;

    pCacheInfo = CryptMemAlloc(size);
    if (!pCacheInfo)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if ((ret = RetrieveUrlCacheEntryFileW(pszURL, pCacheInfo, &size, 0)))
    {
        FILETIME ft;

        GetSystemTimeAsFileTime(&ft);
        if (CompareFileTime(&pCacheInfo->ExpireTime, &ft) >= 0)
        {
            HANDLE hFile = CreateFileW(pCacheInfo->lpszLocalFileName, GENERIC_READ,
             FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                if ((ret = CRYPT_GetObjectFromFile(hFile, pObject)))
                {
                    if (pAuxInfo && pAuxInfo->cbSize >= RTL_SIZEOF_THROUGH_FIELD(CRYPT_RETRIEVE_AUX_INFO, pLastSyncTime)
                            && pAuxInfo->pLastSyncTime)
                    {
                        memcpy(pAuxInfo->pLastSyncTime,
                         &pCacheInfo->LastSyncTime,
                         sizeof(FILETIME));
                    }
                }
                CloseHandle(hFile);
            }
            else
            {
                DeleteUrlCacheEntryW(pszURL);
                ret = FALSE;
            }
        }
        else
        {
            DeleteUrlCacheEntryW(pszURL);
            ret = FALSE;
        }
        UnlockUrlCacheEntryFileW(pszURL, 0);
    }
    CryptMemFree(pCacheInfo);
    TRACE("returning %d\n", ret);
    return ret;
}

/* Parses the URL, and sets components' lpszHostName and lpszUrlPath members
 * to NULL-terminated copies of those portions of the URL (to be freed with
 * CryptMemFree.)
 */
static BOOL CRYPT_CrackUrl(LPCWSTR pszURL, URL_COMPONENTSW *components)
{
    BOOL ret;

    TRACE("(%s, %p)\n", debugstr_w(pszURL), components);

    memset(components, 0, sizeof(*components));
    components->dwStructSize = sizeof(*components);
    components->lpszHostName = CryptMemAlloc(INTERNET_MAX_HOST_NAME_LENGTH * sizeof(WCHAR));
    components->dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
    if (!components->lpszHostName)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    components->lpszUrlPath = CryptMemAlloc(INTERNET_MAX_PATH_LENGTH * sizeof(WCHAR));
    components->dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
    if (!components->lpszUrlPath)
    {
        CryptMemFree(components->lpszHostName);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    ret = InternetCrackUrlW(pszURL, 0, ICU_DECODE, components);
    if (ret)
    {
        switch (components->nScheme)
        {
        case INTERNET_SCHEME_FTP:
            if (!components->nPort)
                components->nPort = INTERNET_DEFAULT_FTP_PORT;
            break;
        case INTERNET_SCHEME_HTTP:
            if (!components->nPort)
                components->nPort = INTERNET_DEFAULT_HTTP_PORT;
            break;
        default:
            ; /* do nothing */
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

struct InetContext
{
    HANDLE event;
    DWORD  timeout;
    DWORD  error;
};

static struct InetContext *CRYPT_MakeInetContext(DWORD dwTimeout)
{
    struct InetContext *context = CryptMemAlloc(sizeof(struct InetContext));

    if (context)
    {
        context->event = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!context->event)
        {
            CryptMemFree(context);
            context = NULL;
        }
        else
        {
            context->timeout = dwTimeout;
            context->error = ERROR_SUCCESS;
        }
    }
    return context;
}

static BOOL CRYPT_DownloadObject(DWORD dwRetrievalFlags, HINTERNET hHttp,
 struct InetContext *context, PCRYPT_BLOB_ARRAY pObject,
 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    CRYPT_DATA_BLOB object = { 0, NULL };
    DWORD bytesAvailable;
    BOOL ret;

    do {
        if ((ret = InternetQueryDataAvailable(hHttp, &bytesAvailable, 0, 0)))
        {
            if (bytesAvailable)
            {
                if (object.pbData)
                    object.pbData = CryptMemRealloc(object.pbData,
                     object.cbData + bytesAvailable);
                else
                    object.pbData = CryptMemAlloc(bytesAvailable);
                if (object.pbData)
                {
                    INTERNET_BUFFERSA buffer = { sizeof(buffer), 0 };

                    buffer.dwBufferLength = bytesAvailable;
                    buffer.lpvBuffer = object.pbData + object.cbData;
                    if (!(ret = InternetReadFileExA(hHttp, &buffer, IRF_NO_WAIT,
                     (DWORD_PTR)context)))
                    {
                        if (GetLastError() == ERROR_IO_PENDING)
                        {
                            if (WaitForSingleObject(context->event,
                             context->timeout) == WAIT_TIMEOUT)
                                SetLastError(ERROR_TIMEOUT);
                            else if (context->error)
                                SetLastError(context->error);
                            else
                                ret = TRUE;
                        }
                    }
                    if (ret)
                        object.cbData += buffer.dwBufferLength;
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
        }
        else if (GetLastError() == ERROR_IO_PENDING)
        {
            if (WaitForSingleObject(context->event, context->timeout) ==
             WAIT_TIMEOUT)
                SetLastError(ERROR_TIMEOUT);
            else
                ret = TRUE;
        }
    } while (ret && bytesAvailable);
    if (ret)
    {
        pObject->rgBlob = CryptMemAlloc(sizeof(CRYPT_DATA_BLOB));
        if (!pObject->rgBlob)
        {
            CryptMemFree(object.pbData);
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
        else
        {
            pObject->rgBlob[0].cbData = object.cbData;
            pObject->rgBlob[0].pbData = object.pbData;
            pObject->cBlob = 1;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Finds the object specified by pszURL in the cache.  If it's not found,
 * creates a new cache entry for the object and writes the object to it.
 * Sets the expiration time of the cache entry to expires.
 */
static void CRYPT_CacheURL(LPCWSTR pszURL, const CRYPT_BLOB_ARRAY *pObject,
 DWORD dwRetrievalFlags, FILETIME expires)
{
    WCHAR cacheFileName[MAX_PATH];
    HANDLE hCacheFile;
    DWORD size = 0, entryType;
    FILETIME ft;

    GetUrlCacheEntryInfoW(pszURL, NULL, &size);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        INTERNET_CACHE_ENTRY_INFOW *info = CryptMemAlloc(size);

        if (!info)
        {
            ERR("out of memory\n");
            return;
        }

        if (GetUrlCacheEntryInfoW(pszURL, info, &size))
        {
            lstrcpyW(cacheFileName, info->lpszLocalFileName);
            /* Check if the existing cache entry is up to date.  If it isn't,
             * remove the existing cache entry, and create a new one with the
             * new value.
             */
            GetSystemTimeAsFileTime(&ft);
            if (CompareFileTime(&info->ExpireTime, &ft) < 0)
            {
                DeleteUrlCacheEntryW(pszURL);
            }
            else
            {
                info->ExpireTime = expires;
                SetUrlCacheEntryInfoW(pszURL, info, CACHE_ENTRY_EXPTIME_FC);
                CryptMemFree(info);
                return;
            }
        }
        CryptMemFree(info);
    }

    if (!CreateUrlCacheEntryW(pszURL, pObject->rgBlob[0].cbData, NULL, cacheFileName, 0))
        return;

    hCacheFile = CreateFileW(cacheFileName, GENERIC_WRITE, 0,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hCacheFile == INVALID_HANDLE_VALUE)
        return;

    WriteFile(hCacheFile, pObject->rgBlob[0].pbData,
            pObject->rgBlob[0].cbData, &size, NULL);
    CloseHandle(hCacheFile);

    if (!(dwRetrievalFlags & CRYPT_STICKY_CACHE_RETRIEVAL))
        entryType = NORMAL_CACHE_ENTRY;
    else
        entryType = STICKY_CACHE_ENTRY;
    memset(&ft, 0, sizeof(ft));
    CommitUrlCacheEntryW(pszURL, cacheFileName, expires, ft, entryType,
            NULL, 0, NULL, NULL);
}

static void CALLBACK CRYPT_InetStatusCallback(HINTERNET hInt,
 DWORD_PTR dwContext, DWORD status, void *statusInfo, DWORD statusInfoLen)
{
    struct InetContext *context = (struct InetContext *)dwContext;
    LPINTERNET_ASYNC_RESULT result;

    switch (status)
    {
    case INTERNET_STATUS_REQUEST_COMPLETE:
        result = statusInfo;
        context->error = result->dwError;
        SetEvent(context->event);
    }
}

static BOOL CRYPT_Connect(const URL_COMPONENTSW *components,
 struct InetContext *context, PCRYPT_CREDENTIALS pCredentials,
 HINTERNET *phInt, HINTERNET *phHost)
{
    BOOL ret;

    TRACE("(%s:%d, %p, %p, %p, %p)\n", debugstr_w(components->lpszHostName),
     components->nPort, context, pCredentials, phInt, phInt);

    *phHost = NULL;
    *phInt = InternetOpenW(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,
     context ? INTERNET_FLAG_ASYNC : 0);
    if (*phInt)
    {
        DWORD service;

        if (context)
            InternetSetStatusCallbackW(*phInt, CRYPT_InetStatusCallback);
        switch (components->nScheme)
        {
        case INTERNET_SCHEME_FTP:
            service = INTERNET_SERVICE_FTP;
            break;
        case INTERNET_SCHEME_HTTP:
            service = INTERNET_SERVICE_HTTP;
            break;
        default:
            service = 0;
        }
        /* FIXME: use pCredentials for username/password */
        *phHost = InternetConnectW(*phInt, components->lpszHostName,
         components->nPort, NULL, NULL, service, 0, (DWORD_PTR)context);
        if (!*phHost)
        {
            InternetCloseHandle(*phInt);
            *phInt = NULL;
            ret = FALSE;
        }
        else
            ret = TRUE;
    }
    else
        ret = FALSE;
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI FTP_RetrieveEncodedObjectW(LPCWSTR pszURL,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    FIXME("(%s, %s, %08lx, %ld, %p, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, pObject,
     ppfnFreeObject, ppvFreeContext, hAsyncRetrieve, pCredentials, pAuxInfo);

    pObject->cBlob = 0;
    pObject->rgBlob = NULL;
    *ppfnFreeObject = CRYPT_FreeBlob;
    *ppvFreeContext = NULL;
    return FALSE;
}

static BOOL WINAPI HTTP_RetrieveEncodedObjectW(LPCWSTR pszURL,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    BOOL ret = FALSE;

    TRACE("(%s, %s, %08lx, %ld, %p, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, pObject,
     ppfnFreeObject, ppvFreeContext, hAsyncRetrieve, pCredentials, pAuxInfo);

    pObject->cBlob = 0;
    pObject->rgBlob = NULL;
    *ppfnFreeObject = CRYPT_FreeBlob;
    *ppvFreeContext = NULL;

    if (!(dwRetrievalFlags & CRYPT_WIRE_ONLY_RETRIEVAL))
        ret = CRYPT_GetObjectFromCache(pszURL, pObject, pAuxInfo);
    if (!ret && (!(dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL) ||
     (dwRetrievalFlags & CRYPT_WIRE_ONLY_RETRIEVAL)))
    {
        URL_COMPONENTSW components;

        if ((ret = CRYPT_CrackUrl(pszURL, &components)))
        {
            HINTERNET hInt, hHost;
            struct InetContext *context = NULL;

            if (dwTimeout)
                context = CRYPT_MakeInetContext(dwTimeout);
            ret = CRYPT_Connect(&components, context, pCredentials, &hInt,
             &hHost);
            if (ret)
            {
                static LPCWSTR types[] =
                {
                 L"application/x-x509-ca-cert", L"application/x-x509-email-cert",
                 L"application/x-x509-server-cert", L"application/x-x509-user-cert",
                 L"application/x-pkcs7-certificates", L"application/pkix-crl",
                 L"application/x-pkcs7-crl", L"application/x-pkcs7-signature",
                 L"application/x-pkcs7-mime", NULL
                };
                HINTERNET hHttp = HttpOpenRequestW(hHost, NULL,
                 components.lpszUrlPath, NULL, NULL, types,
                 INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI,
                 (DWORD_PTR)context);

                if (hHttp)
                {
                    if (dwTimeout)
                    {
                        InternetSetOptionW(hHttp,
                         INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeout,
                         sizeof(dwTimeout));
                        InternetSetOptionW(hHttp, INTERNET_OPTION_SEND_TIMEOUT,
                         &dwTimeout, sizeof(dwTimeout));
                    }
                    ret = HttpSendRequestExW(hHttp, NULL, NULL, 0,
                     (DWORD_PTR)context);
                    if (!ret && GetLastError() == ERROR_IO_PENDING)
                    {
                        if (WaitForSingleObject(context->event,
                         context->timeout) == WAIT_TIMEOUT)
                            SetLastError(ERROR_TIMEOUT);
                        else
                            ret = TRUE;
                    }
                    if (ret &&
                     !(ret = HttpEndRequestW(hHttp, NULL, 0, (DWORD_PTR)context)) &&
                     GetLastError() == ERROR_IO_PENDING)
                    {
                        if (WaitForSingleObject(context->event,
                         context->timeout) == WAIT_TIMEOUT)
                            SetLastError(ERROR_TIMEOUT);
                        else
                            ret = TRUE;
                    }
                    if (ret)
                        ret = CRYPT_DownloadObject(dwRetrievalFlags, hHttp,
                         context, pObject, pAuxInfo);
                    if (ret && !(dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT))
                    {
                        SYSTEMTIME st;
                        FILETIME ft;
                        DWORD len = sizeof(st);

                        if (HttpQueryInfoW(hHttp, HTTP_QUERY_EXPIRES | HTTP_QUERY_FLAG_SYSTEMTIME,
                                    &st, &len, NULL) && SystemTimeToFileTime(&st, &ft))
                            CRYPT_CacheURL(pszURL, pObject, dwRetrievalFlags, ft);
                    }
                    InternetCloseHandle(hHttp);
                }
                InternetCloseHandle(hHost);
                InternetCloseHandle(hInt);
            }
            if (context)
            {
                CloseHandle(context->event);
                CryptMemFree(context);
            }
            CryptMemFree(components.lpszUrlPath);
            CryptMemFree(components.lpszHostName);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI File_RetrieveEncodedObjectW(LPCWSTR pszURL,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    URL_COMPONENTSW components = { sizeof(components), 0 };
    BOOL ret;

    TRACE("(%s, %s, %08lx, %ld, %p, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, pObject,
     ppfnFreeObject, ppvFreeContext, hAsyncRetrieve, pCredentials, pAuxInfo);

    pObject->cBlob = 0;
    pObject->rgBlob = NULL;
    *ppfnFreeObject = CRYPT_FreeBlob;
    *ppvFreeContext = NULL;

    components.lpszUrlPath = CryptMemAlloc(INTERNET_MAX_PATH_LENGTH * sizeof(WCHAR));
    components.dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
    if (!components.lpszUrlPath)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    ret = InternetCrackUrlW(pszURL, 0, ICU_DECODE, &components);
    if (ret)
    {
        LPWSTR path;

        /* 3 == lstrlenW(L"c:") + 1 */
        path = CryptMemAlloc((components.dwUrlPathLength + 3) * sizeof(WCHAR));
        if (path)
        {
            HANDLE hFile;

            /* Try to create the file directly - Wine handles / in pathnames */
            lstrcpynW(path, components.lpszUrlPath,
             components.dwUrlPathLength + 1);
            hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#ifdef __REACTOS__
            if ((hFile == INVALID_HANDLE_VALUE) && (lstrlenW(components.lpszUrlPath) > 1) && (components.lpszUrlPath[1] != ':'))
#else
            if (hFile == INVALID_HANDLE_VALUE)
#endif
            {
                /* Try again on the current drive */
                GetCurrentDirectoryW(components.dwUrlPathLength, path);
                if (path[1] == ':')
                {
                    lstrcpynW(path + 2, components.lpszUrlPath,
                     components.dwUrlPathLength + 1);
                    hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                }
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    /* Try again on the Windows drive */
                    GetWindowsDirectoryW(path, components.dwUrlPathLength);
                    if (path[1] == ':')
                    {
                        lstrcpynW(path + 2, components.lpszUrlPath,
                         components.dwUrlPathLength + 1);
                        hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    }
                }
            }
            if (hFile != INVALID_HANDLE_VALUE)
            {
                if ((ret = CRYPT_GetObjectFromFile(hFile, pObject)))
                {
                    if (pAuxInfo && pAuxInfo->cbSize >= RTL_SIZEOF_THROUGH_FIELD(CRYPT_RETRIEVE_AUX_INFO, pLastSyncTime)
                            && pAuxInfo->pLastSyncTime)
                    {
                        GetFileTime(hFile, NULL, NULL,
                         pAuxInfo->pLastSyncTime);
                    }
                }
                CloseHandle(hFile);
            }
            else
                ret = FALSE;
            CryptMemFree(path);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
    }
    CryptMemFree(components.lpszUrlPath);
    return ret;
}

typedef BOOL (WINAPI *SchemeDllRetrieveEncodedObjectW)(LPCWSTR pwszUrl,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo);

static BOOL CRYPT_GetRetrieveFunction(LPCWSTR pszURL,
 SchemeDllRetrieveEncodedObjectW *pFunc, HCRYPTOIDFUNCADDR *phFunc)
{
    URL_COMPONENTSW components = { sizeof(components), 0 };
    BOOL ret;

    TRACE("(%s, %p, %p)\n", debugstr_w(pszURL), pFunc, phFunc);

    *pFunc = NULL;
    *phFunc = 0;
    components.dwSchemeLength = 1;
    ret = InternetCrackUrlW(pszURL, 0, 0, &components);
    if (ret)
    {
        /* Microsoft always uses CryptInitOIDFunctionSet/
         * CryptGetOIDFunctionAddress, but there doesn't seem to be a pressing
         * reason to do so for builtin schemes.
         */
        switch (components.nScheme)
        {
        case INTERNET_SCHEME_FTP:
            *pFunc = FTP_RetrieveEncodedObjectW;
            break;
        case INTERNET_SCHEME_HTTP:
            *pFunc = HTTP_RetrieveEncodedObjectW;
            break;
        case INTERNET_SCHEME_FILE:
            *pFunc = File_RetrieveEncodedObjectW;
            break;
        default:
        {
            int len = WideCharToMultiByte(CP_ACP, 0, components.lpszScheme,
             components.dwSchemeLength, NULL, 0, NULL, NULL);

            if (len)
            {
                LPSTR scheme = CryptMemAlloc(len);

                if (scheme)
                {
                    static HCRYPTOIDFUNCSET set = NULL;

                    if (!set)
                        set = CryptInitOIDFunctionSet(
                         SCHEME_OID_RETRIEVE_ENCODED_OBJECTW_FUNC, 0);
                    WideCharToMultiByte(CP_ACP, 0, components.lpszScheme,
                     components.dwSchemeLength, scheme, len, NULL, NULL);
                    ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING,
                     scheme, 0, (void **)pFunc, phFunc);
                    CryptMemFree(scheme);
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
            else
                ret = FALSE;
        }
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_CreateBlob(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, const CRYPT_BLOB_ARRAY *pObject, void **ppvContext)
{
    DWORD size, i;
    CRYPT_BLOB_ARRAY *context;
    BOOL ret = FALSE;

    size = sizeof(CRYPT_BLOB_ARRAY) + pObject->cBlob * sizeof(CRYPT_DATA_BLOB);
    for (i = 0; i < pObject->cBlob; i++)
        size += pObject->rgBlob[i].cbData;
    context = CryptMemAlloc(size);
    if (context)
    {
        LPBYTE nextData;

        context->cBlob = 0;
        context->rgBlob =
         (CRYPT_DATA_BLOB *)((LPBYTE)context + sizeof(CRYPT_BLOB_ARRAY));
        nextData =
         (LPBYTE)context->rgBlob + pObject->cBlob * sizeof(CRYPT_DATA_BLOB);
        for (i = 0; i < pObject->cBlob; i++)
        {
            memcpy(nextData, pObject->rgBlob[i].pbData,
             pObject->rgBlob[i].cbData);
            context->rgBlob[i].pbData = nextData;
            context->rgBlob[i].cbData = pObject->rgBlob[i].cbData;
            nextData += pObject->rgBlob[i].cbData;
            context->cBlob++;
        }
        *ppvContext = context;
        ret = TRUE;
    }
    return ret;
}

typedef BOOL (WINAPI *AddContextToStore)(HCERTSTORE hCertStore,
 const void *pContext, DWORD dwAddDisposition, const void **ppStoreContext);

static BOOL decode_base64_blob( const CRYPT_DATA_BLOB *in, CRYPT_DATA_BLOB *out )
{
    BOOL ret;
    DWORD len = in->cbData;

    while (len && !in->pbData[len - 1]) len--;
    if (!CryptStringToBinaryA( (char *)in->pbData, len, CRYPT_STRING_BASE64_ANY,
                               NULL, &out->cbData, NULL, NULL )) return FALSE;

    if (!(out->pbData = CryptMemAlloc( out->cbData ))) return FALSE;
    ret = CryptStringToBinaryA( (char *)in->pbData, len, CRYPT_STRING_BASE64_ANY,
                                out->pbData, &out->cbData, NULL, NULL );
    if (!ret) CryptMemFree( out->pbData );
    return ret;
}

static BOOL CRYPT_CreateContext(const CRYPT_BLOB_ARRAY *pObject,
 DWORD dwExpectedContentTypeFlags, AddContextToStore addFunc, void **ppvContext)
{
    BOOL ret = TRUE;
    CRYPT_DATA_BLOB blob;

    if (!pObject->cBlob)
    {
        SetLastError(ERROR_INVALID_DATA);
        *ppvContext = NULL;
        ret = FALSE;
    }
    else if (pObject->cBlob == 1)
    {
        if (decode_base64_blob(&pObject->rgBlob[0], &blob))
        {
            ret = CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
             dwExpectedContentTypeFlags, CERT_QUERY_FORMAT_FLAG_BINARY, 0,
             NULL, NULL, NULL, NULL, NULL, (const void **)ppvContext);
            CryptMemFree(blob.pbData);
        }
        else
        {
            ret = CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &pObject->rgBlob[0],
             dwExpectedContentTypeFlags, CERT_QUERY_FORMAT_FLAG_BINARY, 0,
             NULL, NULL, NULL, NULL, NULL, (const void **)ppvContext);
        }
        if (!ret)
        {
            SetLastError(CRYPT_E_NO_MATCH);
            ret = FALSE;
        }
    }
    else
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        if (store)
        {
            DWORD i;
            const void *context;

            for (i = 0; i < pObject->cBlob; i++)
            {
                if (decode_base64_blob(&pObject->rgBlob[i], &blob))
                {
                    ret = CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
                     dwExpectedContentTypeFlags, CERT_QUERY_FORMAT_FLAG_BINARY,
                     0, NULL, NULL, NULL, NULL, NULL, &context);
                    CryptMemFree(blob.pbData);
                }
                else
                {
                    ret = CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                     &pObject->rgBlob[i], dwExpectedContentTypeFlags,
                     CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL, NULL, NULL,
                     NULL, &context);
                }
                if (ret)
                {
                    if (!addFunc(store, context, CERT_STORE_ADD_ALWAYS, NULL))
                        ret = FALSE;
                }
                else
                {
                    SetLastError(CRYPT_E_NO_MATCH);
                    ret = FALSE;
                }
            }
        }
        else
            ret = FALSE;
        *ppvContext = store;
    }
    return ret;
}

static BOOL WINAPI CRYPT_CreateCert(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, const CRYPT_BLOB_ARRAY *pObject, void **ppvContext)
{
    return CRYPT_CreateContext(pObject, CERT_QUERY_CONTENT_FLAG_CERT,
     (AddContextToStore)CertAddCertificateContextToStore, ppvContext);
}

static BOOL WINAPI CRYPT_CreateCRL(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, const CRYPT_BLOB_ARRAY *pObject, void **ppvContext)
{
    return CRYPT_CreateContext(pObject, CERT_QUERY_CONTENT_FLAG_CRL,
     (AddContextToStore)CertAddCRLContextToStore, ppvContext);
}

static BOOL WINAPI CRYPT_CreateCTL(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, const CRYPT_BLOB_ARRAY *pObject, void **ppvContext)
{
    return CRYPT_CreateContext(pObject, CERT_QUERY_CONTENT_FLAG_CTL,
     (AddContextToStore)CertAddCTLContextToStore, ppvContext);
}

static BOOL WINAPI CRYPT_CreatePKCS7(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, const CRYPT_BLOB_ARRAY *pObject, void **ppvContext)
{
    BOOL ret;

    if (!pObject->cBlob)
    {
        SetLastError(ERROR_INVALID_DATA);
        *ppvContext = NULL;
        ret = FALSE;
    }
    else if (pObject->cBlob == 1)
        ret = CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &pObject->rgBlob[0],
         CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
         CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED, CERT_QUERY_FORMAT_FLAG_BINARY,
         0, NULL, NULL, NULL, ppvContext, NULL, NULL);
    else
    {
        FIXME("multiple messages unimplemented\n");
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_CreateAny(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, const CRYPT_BLOB_ARRAY *pObject, void **ppvContext)
{
    BOOL ret;

    if (!pObject->cBlob)
    {
        SetLastError(ERROR_INVALID_DATA);
        *ppvContext = NULL;
        ret = FALSE;
    }
    else
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        if (store)
        {
            HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);

            if (memStore)
            {
                CertAddStoreToCollection(store, memStore,
                 CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
                CertCloseStore(memStore, 0);
            }
            else
            {
                CertCloseStore(store, 0);
                store = NULL;
            }
        }
        if (store)
        {
            DWORD i;

            ret = TRUE;
            for (i = 0; i < pObject->cBlob; i++)
            {
                DWORD contentType, expectedContentTypes =
                 CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                 CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED |
                 CERT_QUERY_CONTENT_FLAG_CERT |
                 CERT_QUERY_CONTENT_FLAG_CRL |
                 CERT_QUERY_CONTENT_FLAG_CTL;
                HCERTSTORE contextStore;
                const void *context;

                if (CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                 &pObject->rgBlob[i], expectedContentTypes,
                 CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, &contentType, NULL,
                 &contextStore, NULL, &context))
                {
                    switch (contentType)
                    {
                    case CERT_QUERY_CONTENT_CERT:
                        if (!CertAddCertificateContextToStore(store,
                         context, CERT_STORE_ADD_ALWAYS, NULL))
                            ret = FALSE;
                        CertFreeCertificateContext(context);
                        break;
                    case CERT_QUERY_CONTENT_CRL:
                        if (!CertAddCRLContextToStore(store,
                         context, CERT_STORE_ADD_ALWAYS, NULL))
                             ret = FALSE;
                        CertFreeCRLContext(context);
                        break;
                    case CERT_QUERY_CONTENT_CTL:
                        if (!CertAddCTLContextToStore(store,
                         context, CERT_STORE_ADD_ALWAYS, NULL))
                             ret = FALSE;
                        CertFreeCTLContext(context);
                        break;
                    default:
                        CertAddStoreToCollection(store, contextStore, 0, 0);
                    }
                    CertCloseStore(contextStore, 0);
                }
                else
                    ret = FALSE;
            }
        }
        else
            ret = FALSE;
        *ppvContext = store;
    }
    return ret;
}

typedef BOOL (WINAPI *ContextDllCreateObjectContext)(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, const CRYPT_BLOB_ARRAY *pObject, void **ppvContext);

static BOOL CRYPT_GetCreateFunction(LPCSTR pszObjectOid,
 ContextDllCreateObjectContext *pFunc, HCRYPTOIDFUNCADDR *phFunc)
{
    BOOL ret = TRUE;

    TRACE("(%s, %p, %p)\n", debugstr_a(pszObjectOid), pFunc, phFunc);

    *pFunc = NULL;
    *phFunc = 0;
    if (IS_INTOID(pszObjectOid))
    {
        switch (LOWORD(pszObjectOid))
        {
        case 0:
            *pFunc = CRYPT_CreateBlob;
            break;
        case LOWORD(CONTEXT_OID_CERTIFICATE):
            *pFunc = CRYPT_CreateCert;
            break;
        case LOWORD(CONTEXT_OID_CRL):
            *pFunc = CRYPT_CreateCRL;
            break;
        case LOWORD(CONTEXT_OID_CTL):
            *pFunc = CRYPT_CreateCTL;
            break;
        case LOWORD(CONTEXT_OID_PKCS7):
            *pFunc = CRYPT_CreatePKCS7;
            break;
        case LOWORD(CONTEXT_OID_CAPI2_ANY):
            *pFunc = CRYPT_CreateAny;
            break;
        }
    }
    if (!*pFunc)
    {
        static HCRYPTOIDFUNCSET set = NULL;

        if (!set)
            set = CryptInitOIDFunctionSet(
             CONTEXT_OID_CREATE_OBJECT_CONTEXT_FUNC, 0);
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, pszObjectOid,
         0, (void **)pFunc, phFunc);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_GetExpiration(const void *object, const char *pszObjectOid, FILETIME *expiration)
{
    if (!IS_INTOID(pszObjectOid))
        return FALSE;

    switch (LOWORD(pszObjectOid)) {
    case LOWORD(CONTEXT_OID_CERTIFICATE):
        *expiration = ((const CERT_CONTEXT*)object)->pCertInfo->NotAfter;
        return TRUE;
    case LOWORD(CONTEXT_OID_CRL):
        *expiration = ((const CRL_CONTEXT*)object)->pCrlInfo->NextUpdate;
        return TRUE;
    case LOWORD(CONTEXT_OID_CTL):
        *expiration = ((const CTL_CONTEXT*)object)->pCtlInfo->NextUpdate;
        return TRUE;
    }

    return FALSE;
}

/***********************************************************************
 *    CryptRetrieveObjectByUrlW (CRYPTNET.@)
 */
BOOL WINAPI CryptRetrieveObjectByUrlW(LPCWSTR pszURL, LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, DWORD dwTimeout, LPVOID *ppvObject,
 HCRYPTASYNC hAsyncRetrieve, PCRYPT_CREDENTIALS pCredentials, LPVOID pvVerify,
 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    BOOL ret;
    SchemeDllRetrieveEncodedObjectW retrieve;
    ContextDllCreateObjectContext create;
    HCRYPTOIDFUNCADDR hRetrieve = 0, hCreate = 0;

    TRACE("(%s, %s, %08lx, %ld, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, ppvObject,
     hAsyncRetrieve, pCredentials, pvVerify, pAuxInfo);

    if (!pszURL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ret = CRYPT_GetRetrieveFunction(pszURL, &retrieve, &hRetrieve);
    if (ret)
        ret = CRYPT_GetCreateFunction(pszObjectOid, &create, &hCreate);
    if (ret)
    {
        CRYPT_BLOB_ARRAY object = { 0, NULL };
        PFN_FREE_ENCODED_OBJECT_FUNC freeObject;
        void *freeContext;
        FILETIME expires;

        ret = retrieve(pszURL, pszObjectOid, dwRetrievalFlags, dwTimeout,
         &object, &freeObject, &freeContext, hAsyncRetrieve, pCredentials,
         pAuxInfo);
        if (ret)
        {
            ret = create(pszObjectOid, dwRetrievalFlags, &object, ppvObject);
            if (ret && !(dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT) &&
                CRYPT_GetExpiration(*ppvObject, pszObjectOid, &expires))
            {
                CRYPT_CacheURL(pszURL, &object, dwRetrievalFlags, expires);
            }
            freeObject(pszObjectOid, &object, freeContext);
        }
    }
    if (hCreate)
        CryptFreeOIDFunctionAddress(hCreate, 0);
    if (hRetrieve)
        CryptFreeOIDFunctionAddress(hRetrieve, 0);
    TRACE("returning %d\n", ret);
    return ret;
}

/* Store successful revocation checks (whether the certificate was revoked or
 * not) in an on-disk cache. This is not because of network latency—we already
 * have a cache for that—but rather because parsing very large CRLs can take a
 * long time (at the time of writing, 20 MB CRLs have been seen in the wild and
 * can take several hundred milliseconds) and applications expect chain building
 * to be much faster.
 *
 * The cache is treated as invalid once we pass the nextUpdate field of the CRL.
 * This isn't quite what the field is meant for (it's rather meant to specify a
 * later bound for the next time the CRL will be reissued, and doesn't prescribe
 * a date by which the CRL is invalid; see RFC 5280 § 5.1.2.5) but it's the way
 * it's used in practice.
 *
 * The location of the cache roughly matches Windows, but the file name and
 * contents do not.
 */

static const char revocation_cache_signature[] = "Wine cached revocation";

#define CACHED_CERT_HASH_SIZE 20

static FILE *open_cached_revocation_file(const CERT_CONTEXT *cert, const CERT_REVOCATION_PARA *params,
        const WCHAR *mode, int sharing)
{
    BYTE hash_data[CACHED_CERT_HASH_SIZE];
    WCHAR path[MAX_PATH];
    WCHAR *appdata_path;
    DWORD len, i, size;
    HCRYPTPROV prov;
    HCRYPTHASH hash;
    HRESULT hr;

    if (FAILED(hr = SHGetKnownFolderPath(&FOLDERID_LocalAppDataLow, 0, NULL, &appdata_path)))
    {
        ERR("Failed to get LocalAppDataLow path, hr %#lx.\n", hr);
        return INVALID_HANDLE_VALUE;
    }

    len = swprintf(path, ARRAY_SIZE(path), L"%s\\Microsoft\\CryptnetUrlCache\\Content\\", appdata_path);
    CoTaskMemFree(appdata_path);

    if (len + CACHED_CERT_HASH_SIZE * 2 * sizeof(WCHAR) > ARRAY_SIZE(path) - 1)
    {
        WARN("Hash length exceeds static buffer; not caching.\n");
        return INVALID_HANDLE_VALUE;
    }

    CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(prov, CALG_SHA1, 0, 0, &hash);
    CryptHashData(hash, cert->pbCertEncoded, cert->cbCertEncoded, 0);
    if (params && params->pIssuerCert)
    {
        CryptHashData(hash, (BYTE *)&params->pIssuerCert->cbCertEncoded, sizeof(params->pIssuerCert->cbCertEncoded), 0);
        CryptHashData(hash, params->pIssuerCert->pbCertEncoded, params->pIssuerCert->cbCertEncoded, 0);
    }
    else
    {
        size = 0;
        CryptHashData(hash, (BYTE *)&size, sizeof(size), 0);
    }
    size = sizeof(hash_data);
    CryptGetHashParam(hash, HP_HASHVAL, hash_data, &size, 0);
    CryptDestroyHash(hash);
    CryptReleaseContext(prov, 0);

    SHCreateDirectoryExW(NULL, path, NULL);

    for (i = 0; i < CACHED_CERT_HASH_SIZE; ++i)
    {
        swprintf(path + len, 3, L"%02x", hash_data[i]);
        len += 2;
    }

    return _wfsopen(path, mode, sharing);
}

static BOOL find_cached_revocation_status(const CERT_CONTEXT *cert, const CERT_REVOCATION_PARA *params,
        const FILETIME *time, CERT_REVOCATION_STATUS *status)
{
    char buffer[sizeof(revocation_cache_signature)];
    FILETIME update_time;
    FILE *file;
    int len;

    if (!(file = open_cached_revocation_file(cert, params, L"rb", _SH_DENYWR)))
        return FALSE;

    if ((len = fread(buffer, 1, sizeof(buffer), file)) != sizeof(buffer)
            || memcmp(buffer, revocation_cache_signature, len))
    {
        ERR("Invalid cache signature.\n");
        fclose(file);
        return FALSE;
    }

    if (fread(&update_time, sizeof(update_time), 1, file) != 1)
    {
        ERR("Failed to read update time.\n");
        fclose(file);
        return FALSE;
    }

    if (CompareFileTime(time, &update_time) > 0)
    {
        TRACE("Cached revocation status is potentially out of date.\n");
        fclose(file);
        return FALSE;
    }

    if (fread(&status->dwError, sizeof(status->dwError), 1, file) != 1)
    {
        ERR("Failed to read error code.\n");
        fclose(file);
        return FALSE;
    }

    if (status->dwError == CERT_E_REVOKED && fread(&status->dwReason, sizeof(status->dwReason), 1, file) != 1)
    {
        ERR("Failed to read revocation reason.\n");
        fclose(file);
        return FALSE;
    }

    TRACE("Using cached status %#lx, reason %#lx.\n", status->dwError, status->dwReason);
    return TRUE;
}

static void cache_revocation_status(const CERT_CONTEXT *cert, const CERT_REVOCATION_PARA *params,
        const FILETIME *time, const CERT_REVOCATION_STATUS *status)
{
    FILE *file;

    if (!(file = open_cached_revocation_file(cert, params, L"wb", _SH_DENYRW)))
        return;
    fwrite(revocation_cache_signature, 1, sizeof(revocation_cache_signature), file);
    fwrite(time, sizeof(*time), 1, file);
    fwrite(&status->dwError, sizeof(status->dwError), 1, file);
    if (status->dwError == CERT_E_REVOKED)
        fwrite(&status->dwReason, sizeof(status->dwReason), 1, file);
    fclose(file);
}

static DWORD verify_cert_revocation_with_crl_online(const CERT_CONTEXT *cert,
        const CRL_CONTEXT *crl, FILETIME *pTime, CERT_REVOCATION_STATUS *pRevStatus)
{
    PCRL_ENTRY entry = NULL;

    CertFindCertificateInCRL(cert, crl, 0, NULL, &entry);
    if (entry)
        return CRYPT_E_REVOKED;

    /* Since the CRL was retrieved for the cert being checked, then it's
     * guaranteed to be fresh, and the cert is not revoked. */
    return ERROR_SUCCESS;
}

/* Try to retrieve a CRL from any one of the specified distribution points. */
static const CRL_CONTEXT *retrieve_crl_from_dist_points(const CRYPT_URL_ARRAY *array,
        DWORD verify_flags, DWORD timeout)
{
    DWORD retrieve_flags = 0;
    const CRL_CONTEXT *crl;
    DWORD i;

    if (verify_flags & CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION)
        retrieve_flags |= CRYPT_CACHE_ONLY_RETRIEVAL;

    /* Yes, this is a weird algorithm, but the documentation for
     * CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT specifies this, and
     * tests seem to bear it out for CertVerifyRevocation() as well. */
    if (verify_flags & CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG)
        timeout /= 2;

    for (i = 0; i < array->cUrl; ++i)
    {
        if (CryptRetrieveObjectByUrlW(array->rgwszUrl[i], CONTEXT_OID_CRL, retrieve_flags,
                timeout, (void **)&crl, NULL, NULL, NULL, NULL))
            return crl;

        /* We don't check the current time here. This may result in less
         * accurate timeouts, but this too seems to be true of Windows. */
        if ((verify_flags & CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG) && GetLastError() == ERROR_TIMEOUT)
            timeout /= 2;
    }

    return NULL;
}

static DWORD verify_cert_revocation_from_dist_points_ext(const CRYPT_DATA_BLOB *value, const CERT_CONTEXT *cert,
        FILETIME *time, DWORD flags, const CERT_REVOCATION_PARA *params, CERT_REVOCATION_STATUS *status,
        FILETIME *next_update)
{
    DWORD url_array_size, error;
    CRYPT_URL_ARRAY *url_array;
    const CRL_CONTEXT *crl;
    DWORD timeout = 0;

    if (!params || !params->pIssuerCert)
    {
        TRACE("no issuer certificate\n");
        return CRYPT_E_REVOCATION_OFFLINE;
    }

    if (!CRYPT_GetUrlFromCRLDistPointsExt(value, NULL, &url_array_size, NULL, NULL))
        return GetLastError();

    if (!(url_array = CryptMemAlloc(url_array_size)))
        return ERROR_OUTOFMEMORY;

    if (!CRYPT_GetUrlFromCRLDistPointsExt(value, url_array, &url_array_size, NULL, NULL))
    {
        CryptMemFree(url_array);
        return GetLastError();
    }

    if (params && params->cbSize >= RTL_SIZEOF_THROUGH_FIELD(CERT_REVOCATION_PARA, dwUrlRetrievalTimeout))
        timeout = params->dwUrlRetrievalTimeout;

    if (!(crl = retrieve_crl_from_dist_points(url_array, flags, timeout)))
    {
        CryptMemFree(url_array);
        return CRYPT_E_REVOCATION_OFFLINE;
    }

    error = verify_cert_revocation_with_crl_online(cert, crl, time, status);

    *next_update = crl->pCrlInfo->NextUpdate;

    CertFreeCRLContext(crl);
    CryptMemFree(url_array);
    return error;
}

static void sha1_hash(const BYTE *data, DWORD datalen, BYTE *buf, DWORD *buflen)
{
    HCRYPTPROV prov;
    HCRYPTHASH hash;

    CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(prov, CALG_SHA1, 0, 0, &hash);
    CryptHashData(hash, data, datalen, 0);
    CryptGetHashParam(hash, HP_HASHVAL, buf, buflen, 0);

    CryptDestroyHash(hash);
    CryptReleaseContext(prov, 0);
}

static BYTE *build_ocsp_request(const CERT_CONTEXT *cert, const CERT_CONTEXT *issuer_cert, DWORD *ret_size)
{
    OCSP_REQUEST_ENTRY entry;
    OCSP_REQUEST_INFO request;
    OCSP_SIGNED_REQUEST_INFO request_signed;
    CERT_INFO *issuer = issuer_cert->pCertInfo;
    BYTE issuer_name_hash[20], issuer_key_hash[20], *buf, *ret;
    DWORD size = 0, hash_len = sizeof(issuer_name_hash);

    memset(&entry, 0, sizeof(entry));
    entry.CertId.HashAlgorithm.pszObjId = (char *)szOID_OIWSEC_sha1;

    sha1_hash(issuer->Subject.pbData, issuer->Subject.cbData, issuer_name_hash, &hash_len);
    entry.CertId.IssuerNameHash.cbData = sizeof(issuer_name_hash);
    entry.CertId.IssuerNameHash.pbData = issuer_name_hash;

    sha1_hash(issuer->SubjectPublicKeyInfo.PublicKey.pbData, issuer->SubjectPublicKeyInfo.PublicKey.cbData,
              issuer_key_hash, &hash_len);
    entry.CertId.IssuerKeyHash.cbData = sizeof(issuer_key_hash);
    entry.CertId.IssuerKeyHash.pbData = issuer_key_hash;

    entry.CertId.SerialNumber.cbData = cert->pCertInfo->SerialNumber.cbData;
    entry.CertId.SerialNumber.pbData = cert->pCertInfo->SerialNumber.pbData;

    request.dwVersion      = OCSP_REQUEST_V1;
    request.pRequestorName = NULL;
    request.cRequestEntry  = 1;
    request.rgRequestEntry = &entry;
    request.cExtension     = 0;
    request.rgExtension    = NULL;
    if (!CryptEncodeObjectEx(X509_ASN_ENCODING, OCSP_REQUEST, &request, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size))
    {
        ERR("failed to encode request %#lx\n", GetLastError());
        return NULL;
    }

    request_signed.ToBeSigned.pbData = buf;
    request_signed.ToBeSigned.cbData = size;
    request_signed.pOptionalSignatureInfo = NULL;
    if (!CryptEncodeObjectEx(X509_ASN_ENCODING, OCSP_SIGNED_REQUEST, &request_signed, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                             &ret, &size))
    {
        ERR("failed to encode signed request %#lx\n", GetLastError());
        LocalFree(buf);
        return NULL;
    }

    LocalFree(buf);
    *ret_size = size;
    return ret;
}

static void escape_path(const WCHAR *src, DWORD src_len, WCHAR *dst, DWORD *dst_len)
{
    static const WCHAR hex[] = L"0123456789ABCDEF";
    WCHAR *ptr = dst;
    DWORD i;

    *dst_len = src_len;
    for (i = 0; i < src_len; i++)
    {
        if (src[i] == '+' || src[i] == '/' || src[i] == '=')
        {
            if (dst)
            {
                ptr[0] = '%';
                ptr[1] = hex[(src[i] >> 4) & 0xf];
                ptr[2] = hex[src[i] & 0xf];
                ptr += 3;
            }
            *dst_len += 2;
        }
        else if (dst) *ptr++ = src[i];
    }
}

static WCHAR *build_request_path(const BYTE *data, DWORD data_size)
{
    WCHAR *path, *ret;
    DWORD path_len, ret_len;

    if (!CryptBinaryToStringW(data, data_size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &path_len)) return NULL;
    if (!(path = malloc(path_len * sizeof(WCHAR)))) return NULL;
    CryptBinaryToStringW(data, data_size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, path, &path_len);

    escape_path(path, path_len, NULL, &ret_len);
    if (!(ret = malloc((ret_len + 2) * sizeof(WCHAR))))
    {
        free(path);
        return NULL;
    }
    escape_path(path, path_len, ret + 1, &ret_len);
    ret[ret_len + 1] = 0;
    ret[0] = '/';

    free(path);
    return ret;
}

static WCHAR *build_request_url(const WCHAR *base_url, const BYTE *data, DWORD data_size)
{
    WCHAR *path, *ret;
    DWORD len = 0;

    if (!(path = build_request_path(data, data_size))) return NULL;
    len = (wcslen(base_url) + wcslen(path) + 1) * sizeof(WCHAR);
    if (!(ret = malloc(len * sizeof(WCHAR))))
    {
        free(path);
        return NULL;
    }
    wcscpy(ret, base_url);
    wcscat(ret, path);
    free(path);
    TRACE("-> %s.\n", debugstr_w(ret));
    return ret;
}

static DWORD map_ocsp_status(DWORD status)
{
    switch (status)
    {
    case OCSP_BASIC_GOOD_CERT_STATUS: return ERROR_SUCCESS;
    case OCSP_BASIC_REVOKED_CERT_STATUS: return CRYPT_E_REVOKED;
    case OCSP_BASIC_UNKNOWN_CERT_STATUS: return CRYPT_E_REVOCATION_OFFLINE;
    default:
        FIXME("unhandled status %lu\n", status);
        return CRYPT_E_REVOCATION_OFFLINE;
    }
}

static BOOL match_cert_id(const OCSP_CERT_ID *id, const CERT_INFO *cert, const CERT_INFO *issuer)
{
    BYTE hash[20];
    DWORD hash_len = sizeof(hash);

    if (!id->HashAlgorithm.pszObjId || strcmp(id->HashAlgorithm.pszObjId, szOID_OIWSEC_sha1))
    {
        FIXME("hash algorithm %s not supported\n", debugstr_a(id->HashAlgorithm.pszObjId));
        return FALSE;
    }

    sha1_hash(issuer->Subject.pbData, issuer->Subject.cbData, hash, &hash_len);
    if (id->IssuerNameHash.cbData != hash_len) return FALSE;
    if (memcmp(id->IssuerNameHash.pbData, hash, hash_len)) return FALSE;

    sha1_hash(issuer->SubjectPublicKeyInfo.PublicKey.pbData,
              issuer->SubjectPublicKeyInfo.PublicKey.cbData, hash, &hash_len);
    if (id->IssuerKeyHash.cbData != hash_len) return FALSE;
    if (memcmp(id->IssuerKeyHash.pbData, hash, hash_len)) return FALSE;

    if (cert->SerialNumber.cbData != id->SerialNumber.cbData) return FALSE;
    return !memcmp(cert->SerialNumber.pbData, id->SerialNumber.pbData, id->SerialNumber.cbData);
}

static DWORD check_ocsp_response_info(const CERT_INFO *cert, const CERT_INFO *issuer,
                                      const CRYPT_OBJID_BLOB *blob, DWORD *status, FILETIME *next_update)
{
    OCSP_BASIC_RESPONSE_INFO *info;
    DWORD size, i;

    memset(next_update, 0, sizeof(*next_update));
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, OCSP_BASIC_RESPONSE, blob->pbData, blob->cbData,
                             CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size)) return GetLastError();

    FIXME("check responder id\n");
    for (i = 0; i < info->cResponseEntry; i++)
    {
        OCSP_BASIC_RESPONSE_ENTRY *entry = &info->rgResponseEntry[i];
        if (match_cert_id(&entry->CertId, cert, issuer))
        {
            *status = map_ocsp_status(entry->dwCertStatus);
            *next_update = entry->NextUpdate;
        }
    }

    LocalFree(info);
    return ERROR_SUCCESS;
}

static DWORD verify_signed_ocsp_response_info(const CERT_INFO *cert, const CERT_INFO *issuer,
                                              const CRYPT_OBJID_BLOB *blob, FILETIME *next_update)
{
    OCSP_BASIC_SIGNED_RESPONSE_INFO *info;
    DWORD size, error, status = CRYPT_E_REVOCATION_OFFLINE;
    CRYPT_ALGORITHM_IDENTIFIER *alg;
    CRYPT_BIT_BLOB *sig;
    HCRYPTPROV prov = 0;
    HCRYPTHASH hash = 0;
    HCRYPTKEY key = 0;
    DWORD algid;

    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, OCSP_BASIC_SIGNED_RESPONSE, blob->pbData, blob->cbData,
                             CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size)) return GetLastError();

    if ((error = check_ocsp_response_info(cert, issuer, &info->ToBeSigned, &status, next_update))) goto done;

    alg = &info->SignatureInfo.SignatureAlgorithm;
    if (!alg->pszObjId || !(algid = CertOIDToAlgId(alg->pszObjId)))
    {
        FIXME("unhandled signature algorithm %s\n", debugstr_a(alg->pszObjId));
        error = CRYPT_E_NO_REVOCATION_CHECK;
        goto done;
    }

    if (!CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) goto done;
    if (!CryptCreateHash(prov, algid, 0, 0, &hash)) goto done;
    if (!CryptHashData(hash, info->ToBeSigned.pbData, info->ToBeSigned.cbData, 0)) goto done;

    sig = &info->SignatureInfo.Signature;
    if (!CryptImportPublicKeyInfoEx(prov, X509_ASN_ENCODING, (CERT_PUBLIC_KEY_INFO *)&issuer->SubjectPublicKeyInfo,
                                    0, 0, NULL, &key))
    {
        error = GetLastError();
        TRACE("failed to import public key %#lx\n", error);
    }
    else if (!CryptVerifySignatureW(hash, sig->pbData, sig->cbData, key, NULL, 0))
    {
        error = GetLastError();
        TRACE("failed to verify signature %#lx\n", error);
    }
    else error = ERROR_SUCCESS;

done:
    CryptDestroyKey(key);
    CryptDestroyHash(hash);
    CryptReleaseContext(prov, 0);
    LocalFree(info);
    if (error) return error;
    return status;
}

static DWORD handle_ocsp_response(const CERT_INFO *cert, const CERT_INFO *issuer, const BYTE *encoded,
                                  DWORD encoded_size, FILETIME *next_update)
{
    OCSP_RESPONSE_INFO *info;
    DWORD size, error = CRYPT_E_NO_REVOCATION_CHECK;

    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, OCSP_RESPONSE, encoded, encoded_size, CRYPT_DECODE_ALLOC_FLAG, NULL,
                             &info, &size)) return GetLastError();

    switch (info->dwStatus)
    {
    case OCSP_SUCCESSFUL_RESPONSE:
        if (!info->pszObjId || strcmp(info->pszObjId, szOID_PKIX_OCSP_BASIC_SIGNED_RESPONSE))
        {
            FIXME("unhandled response type %s\n", debugstr_a(info->pszObjId));
            break;
        }
        error = verify_signed_ocsp_response_info(cert, issuer, &info->Value, next_update);
        break;

    default:
        FIXME("unhandled status %lu\n", info->dwStatus);
        break;
    }

    LocalFree(info);
    return error;
}

static DWORD verify_cert_revocation_with_ocsp(const CERT_CONTEXT *cert, const WCHAR *base_url,
                                              const CERT_REVOCATION_PARA *revpara, FILETIME *next_update)
{
    HINTERNET ses, con, req = NULL;
    BYTE *request_data = NULL, *response_data = NULL;
    DWORD size, flags, status, request_len, response_len, count, ret = CRYPT_E_REVOCATION_OFFLINE;
    URL_COMPONENTSW comp;
    WCHAR *url;

    if (!revpara || !revpara->pIssuerCert)
    {
        TRACE("no issuer certificate\n");
        return CRYPT_E_REVOCATION_OFFLINE;
    }
    if (!(request_data = build_ocsp_request(cert, revpara->pIssuerCert, &request_len)))
        return CRYPT_E_REVOCATION_OFFLINE;

    url = build_request_url(base_url, request_data, request_len);
    LocalFree(request_data);
    if (!url) return CRYPT_E_REVOCATION_OFFLINE;

    memset(&comp, 0, sizeof(comp));
    comp.dwStructSize     = sizeof(comp);
    comp.dwHostNameLength = ~0u;
    comp.dwUrlPathLength  = ~0u;
    if (!InternetCrackUrlW(url, 0, 0, &comp))
    {
        free(url);
        return CRYPT_E_REVOCATION_OFFLINE;
    }

    switch (comp.nScheme)
    {
    case INTERNET_SCHEME_HTTP:
        flags = 0;
        break;
    case INTERNET_SCHEME_HTTPS:
        flags = INTERNET_FLAG_SECURE;
        break;
    default:
        FIXME("scheme %u not supported\n", comp.nScheme);
        free(url);
        return ERROR_NOT_SUPPORTED;
    }

    if (!(ses = InternetOpenW(L"CryptoAPI", 0, NULL, NULL, 0))) return GetLastError();
    comp.lpszHostName[comp.dwHostNameLength] = 0;
    if (!(con = InternetConnectW(ses, comp.lpszHostName, comp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0)))
    {
        free(url);
        InternetCloseHandle(ses);
        return GetLastError();
    }
    comp.lpszHostName[comp.dwHostNameLength] = '/';
    if (!(req = HttpOpenRequestW(con, NULL, comp.lpszUrlPath, NULL, NULL, NULL, flags, 0)) ||
        !HttpSendRequestW(req, NULL, 0, NULL, 0)) goto done;

    size = sizeof(status);
    if (!HttpQueryInfoW(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL)) goto done;
    if (status != HTTP_STATUS_OK)
    {
        WARN("request status %lu\n", status);
        goto done;
    }

    size = sizeof(response_len);
    if (!HttpQueryInfoW(req, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, &response_len, &size, 0) ||
        !response_len || !(response_data = malloc(response_len)) ||
        !InternetReadFile(req, response_data, response_len, &count) || count != response_len) goto done;

    ret = handle_ocsp_response(cert->pCertInfo, revpara->pIssuerCert->pCertInfo, response_data, response_len,
                               next_update);

done:
    free(url);
    free(response_data);
    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
    return ret;
}

static DWORD verify_cert_revocation_from_aia_ext(const CRYPT_DATA_BLOB *value, const CERT_CONTEXT *cert,
        FILETIME *pTime, DWORD dwFlags, CERT_REVOCATION_PARA *pRevPara, CERT_REVOCATION_STATUS *pRevStatus,
        FILETIME *next_update)
{
    BOOL ret;
    DWORD size, i, error = CRYPT_E_NO_REVOCATION_CHECK;
    CERT_AUTHORITY_INFO_ACCESS *aia;

    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_AUTHORITY_INFO_ACCESS, value->pbData, value->cbData,
                              CRYPT_DECODE_ALLOC_FLAG, NULL, &aia, &size);
    if (!ret) return GetLastError();

    for (i = 0; i < aia->cAccDescr; i++)
    {
        if (!strcmp(aia->rgAccDescr[i].pszAccessMethod, szOID_PKIX_OCSP))
        {
            if (aia->rgAccDescr[i].AccessLocation.dwAltNameChoice == CERT_ALT_NAME_URL)
            {
                const WCHAR *url = aia->rgAccDescr[i].AccessLocation.pwszURL;
                TRACE("OCSP URL = %s\n", debugstr_w(url));
                error = verify_cert_revocation_with_ocsp(cert, url, pRevPara, next_update);
            }
            else
            {
                FIXME("unsupported AccessLocation type %lu\n", aia->rgAccDescr[i].AccessLocation.dwAltNameChoice);
                error = ERROR_NOT_SUPPORTED;
            }
            break;
        }
    }

    LocalFree(aia);
    return error;
}

static DWORD verify_cert_revocation_with_crl_offline(PCCERT_CONTEXT cert,
        const CRL_CONTEXT *crl, FILETIME *pTime, CERT_REVOCATION_STATUS *pRevStatus)
{
    PCRL_ENTRY entry = NULL;
    LONG valid;

    valid = CompareFileTime(pTime, &crl->pCrlInfo->ThisUpdate);
    if (valid <= 0)
    {
        /* If this CRL is not older than the time being verified, there's no
         * way to know whether the certificate was revoked.
         */
        TRACE("CRL not old enough\n");
        return CRYPT_E_REVOCATION_OFFLINE;
    }

    CertFindCertificateInCRL(cert, crl, 0, NULL, &entry);
    if (entry)
        return CRYPT_E_REVOKED;

    /* Since the CRL was not retrieved for the cert being checked, there's no
     * guarantee it's fresh, so the cert *might* be okay, but it's safer not to
     * guess. */
    TRACE("certificate not found\n");
    return CRYPT_E_REVOCATION_OFFLINE;
}

static DWORD verify_cert_revocation(const CERT_CONTEXT *cert, FILETIME *pTime,
        DWORD dwFlags, CERT_REVOCATION_PARA *pRevPara, CERT_REVOCATION_STATUS *pRevStatus)
{
    DWORD error = ERROR_SUCCESS;
    FILETIME next_update = {0};
    PCERT_EXTENSION ext;

    if (find_cached_revocation_status(cert, pRevPara, pTime, pRevStatus))
    {
        if (pRevStatus->dwError == ERROR_SUCCESS || pRevStatus->dwError == CRYPT_E_REVOKED)
        {
            TRACE("Returning cached status.\n");
            return pRevStatus->dwError;
        }
    }

    if ((ext = CertFindExtension(szOID_AUTHORITY_INFO_ACCESS, cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        error = verify_cert_revocation_from_aia_ext(&ext->Value, cert, pTime, dwFlags, pRevPara, pRevStatus,
                                                    &next_update);
        TRACE("verify_cert_revocation_from_aia_ext() returned %08lx\n", error);
        if (error == ERROR_SUCCESS || error == CRYPT_E_REVOKED) goto done;
    }
    if ((ext = CertFindExtension(szOID_CRL_DIST_POINTS, cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        error = verify_cert_revocation_from_dist_points_ext(&ext->Value, cert, pTime, dwFlags, pRevPara, pRevStatus,
                                                            &next_update);
        TRACE("verify_cert_revocation_from_dist_points_ext() returned %08lx\n", error);
        if (error == ERROR_SUCCESS || error == CRYPT_E_REVOKED) goto done;
    }
    if (!ext)
    {
        if (pRevPara && pRevPara->hCrlStore && pRevPara->pIssuerCert)
        {
            PCCRL_CONTEXT crl = NULL;
            BOOL canSignCRLs;

            /* If the caller told us about the issuer, make sure the issuer
             * can sign CRLs before looking for one.
             */
            if ((ext = CertFindExtension(szOID_KEY_USAGE,
             pRevPara->pIssuerCert->pCertInfo->cExtension,
             pRevPara->pIssuerCert->pCertInfo->rgExtension)))
            {
                CRYPT_BIT_BLOB usage;
                DWORD size = sizeof(usage);

                if (!CryptDecodeObjectEx(cert->dwCertEncodingType, X509_BITS,
                 ext->Value.pbData, ext->Value.cbData,
                 CRYPT_DECODE_NOCOPY_FLAG, NULL, &usage, &size))
                    canSignCRLs = FALSE;
                else if (usage.cbData > 2)
                {
                    /* The key usage extension only defines 9 bits => no more
                     * than 2 bytes are needed to encode all known usages.
                     */
                    canSignCRLs = FALSE;
                }
                else
                {
                    BYTE usageBits = usage.pbData[usage.cbData - 1];

                    canSignCRLs = usageBits & CERT_CRL_SIGN_KEY_USAGE;
                }
            }
            else
                canSignCRLs = TRUE;
            if (canSignCRLs)
            {
                /* If the caller was helpful enough to tell us where to find a
                 * CRL for the cert, look for one and check it.
                 */
                crl = CertFindCRLInStore(pRevPara->hCrlStore,
                 cert->dwCertEncodingType,
                 CRL_FIND_ISSUED_BY_SIGNATURE_FLAG |
                 CRL_FIND_ISSUED_BY_AKI_FLAG,
                 CRL_FIND_ISSUED_BY, pRevPara->pIssuerCert, NULL);
            }
            if (crl)
            {
                error = verify_cert_revocation_with_crl_offline(cert, crl, pTime, pRevStatus);
                CertFreeCRLContext(crl);
            }
            else
            {
                TRACE("no CRL found\n");
                error = CRYPT_E_NO_REVOCATION_CHECK;
            }
        }
        else
        {
            if (!pRevPara)
                WARN("no CERT_REVOCATION_PARA\n");
            else if (!pRevPara->hCrlStore)
                WARN("no dist points/aia extension and no CRL store\n");
            else if (!pRevPara->pIssuerCert)
                WARN("no dist points/aia extension and no issuer\n");
            error = CRYPT_E_NO_REVOCATION_CHECK;
        }
    }
done:
    if ((next_update.dwLowDateTime || next_update.dwHighDateTime)
        && (error == ERROR_SUCCESS || error == CRYPT_E_REVOKED))
    {
        CERT_REVOCATION_STATUS rev_status;

        memset(&rev_status, 0, sizeof(rev_status));
        rev_status.cbSize = sizeof(rev_status);
        rev_status.dwError = error;
        cache_revocation_status(cert, pRevPara, &next_update, &rev_status);
    }
    return error;
}

typedef struct _CERT_REVOCATION_PARA_NO_EXTRA_FIELDS {
    DWORD                     cbSize;
    PCCERT_CONTEXT            pIssuerCert;
    DWORD                     cCertStore;
    HCERTSTORE               *rgCertStore;
    HCERTSTORE                hCrlStore;
    LPFILETIME                pftTimeToUse;
} CERT_REVOCATION_PARA_NO_EXTRA_FIELDS;

typedef struct _OLD_CERT_REVOCATION_STATUS {
    DWORD cbSize;
    DWORD dwIndex;
    DWORD dwError;
    DWORD dwReason;
} OLD_CERT_REVOCATION_STATUS;

/***********************************************************************
 *    CertDllVerifyRevocation (CRYPTNET.@)
 */
BOOL WINAPI CertDllVerifyRevocation(DWORD dwEncodingType, DWORD dwRevType,
 DWORD cContext, PVOID rgpvContext[], DWORD dwFlags,
 PCERT_REVOCATION_PARA pRevPara, PCERT_REVOCATION_STATUS pRevStatus)
{
    DWORD error = 0, i;
    FILETIME now;
    LPFILETIME pTime = NULL;

    TRACE("(%08lx, %ld, %ld, %p, %08lx, %p, %p)\n", dwEncodingType, dwRevType,
     cContext, rgpvContext, dwFlags, pRevPara, pRevStatus);

    if (pRevStatus->cbSize != sizeof(OLD_CERT_REVOCATION_STATUS) &&
     pRevStatus->cbSize != sizeof(CERT_REVOCATION_STATUS))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (!cContext)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (pRevPara && pRevPara->cbSize >=
     sizeof(CERT_REVOCATION_PARA_NO_EXTRA_FIELDS))
        pTime = pRevPara->pftTimeToUse;
    if (!pTime)
    {
        GetSystemTimeAsFileTime(&now);
        pTime = &now;
    }
    memset(&pRevStatus->dwIndex, 0, pRevStatus->cbSize - sizeof(DWORD));
    if (dwRevType != CERT_CONTEXT_REVOCATION_TYPE)
        error = CRYPT_E_NO_REVOCATION_CHECK;
    else
    {
        for (i = 0; i < cContext; i++)
        {
            if ((error = verify_cert_revocation(rgpvContext[i], pTime, dwFlags, pRevPara, pRevStatus)))
            {
                pRevStatus->dwIndex = i;
                break;
            }
        }
    }
    if (error)
    {
        SetLastError(error);
        pRevStatus->dwError = error;
    }
    TRACE("returning %d (%08lx)\n", !error, error);
    return !error;
}

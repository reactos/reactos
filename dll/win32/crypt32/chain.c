/*
 * Copyright 2006 Juan Lang
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
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

/* This represents a subset of a certificate chain engine:  it doesn't include
 * the "hOther" store described by MSDN, because I'm not sure how that's used.
 * It also doesn't include the "hTrust" store, because I don't yet implement
 * CTLs or complex certificate chains.
 */
typedef struct _CertificateChainEngine
{
    LONG       ref;
    HCERTSTORE hRoot;
    HCERTSTORE hWorld;
    DWORD      dwFlags;
    DWORD      dwUrlRetrievalTimeout;
    DWORD      MaximumCachedCertificates;
    DWORD      CycleDetectionModulus;
} CertificateChainEngine, *PCertificateChainEngine;

static inline void CRYPT_AddStoresToCollection(HCERTSTORE collection,
 DWORD cStores, HCERTSTORE *stores)
{
    DWORD i;

    for (i = 0; i < cStores; i++)
        CertAddStoreToCollection(collection, stores[i], 0, 0);
}

static inline void CRYPT_CloseStores(DWORD cStores, HCERTSTORE *stores)
{
    DWORD i;

    for (i = 0; i < cStores; i++)
        CertCloseStore(stores[i], 0);
}

static const WCHAR rootW[] = { 'R','o','o','t',0 };

static BOOL CRYPT_CheckRestrictedRoot(HCERTSTORE store)
{
    BOOL ret = TRUE;

    if (store)
    {
        HCERTSTORE rootStore = CertOpenSystemStoreW(0, rootW);
        PCCERT_CONTEXT cert = NULL, check;
        BYTE hash[20];
        DWORD size;

        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
            {
                size = sizeof(hash);

                ret = CertGetCertificateContextProperty(cert, CERT_HASH_PROP_ID,
                 hash, &size);
                if (ret)
                {
                    CRYPT_HASH_BLOB blob = { sizeof(hash), hash };

                    check = CertFindCertificateInStore(rootStore,
                     cert->dwCertEncodingType, 0, CERT_FIND_SHA1_HASH, &blob,
                     NULL);
                    if (!check)
                        ret = FALSE;
                    else
                        CertFreeCertificateContext(check);
                }
            }
        } while (ret && cert);
        if (cert)
            CertFreeCertificateContext(cert);
        CertCloseStore(rootStore, 0);
    }
    return ret;
}

BOOL WINAPI CertCreateCertificateChainEngine(PCERT_CHAIN_ENGINE_CONFIG pConfig,
 HCERTCHAINENGINE *phChainEngine)
{
    static const WCHAR caW[] = { 'C','A',0 };
    static const WCHAR myW[] = { 'M','y',0 };
    static const WCHAR trustW[] = { 'T','r','u','s','t',0 };
    BOOL ret;

    TRACE("(%p, %p)\n", pConfig, phChainEngine);

    if (pConfig->cbSize != sizeof(*pConfig))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    *phChainEngine = NULL;
    ret = CRYPT_CheckRestrictedRoot(pConfig->hRestrictedRoot);
    if (ret)
    {
        PCertificateChainEngine engine =
         CryptMemAlloc(sizeof(CertificateChainEngine));

        if (engine)
        {
            HCERTSTORE worldStores[4];

            engine->ref = 1;
            if (pConfig->hRestrictedRoot)
                engine->hRoot = CertDuplicateStore(pConfig->hRestrictedRoot);
            else
                engine->hRoot = CertOpenSystemStoreW(0, rootW);
            engine->hWorld = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);
            worldStores[0] = CertDuplicateStore(engine->hRoot);
            worldStores[1] = CertOpenSystemStoreW(0, caW);
            worldStores[2] = CertOpenSystemStoreW(0, myW);
            worldStores[3] = CertOpenSystemStoreW(0, trustW);
            CRYPT_AddStoresToCollection(engine->hWorld,
             sizeof(worldStores) / sizeof(worldStores[0]), worldStores);
            CRYPT_AddStoresToCollection(engine->hWorld,
             pConfig->cAdditionalStore, pConfig->rghAdditionalStore);
            CRYPT_CloseStores(sizeof(worldStores) / sizeof(worldStores[0]),
             worldStores);
            engine->dwFlags = pConfig->dwFlags;
            engine->dwUrlRetrievalTimeout = pConfig->dwUrlRetrievalTimeout;
            engine->MaximumCachedCertificates =
             pConfig->MaximumCachedCertificates;
            engine->CycleDetectionModulus = pConfig->CycleDetectionModulus;
            *phChainEngine = (HCERTCHAINENGINE)engine;
            ret = TRUE;
        }
        else
            ret = FALSE;
    }
    return ret;
}

void WINAPI CertFreeCertificateChainEngine(HCERTCHAINENGINE hChainEngine)
{
    PCertificateChainEngine engine = (PCertificateChainEngine)hChainEngine;

    TRACE("(%p)\n", hChainEngine);

    if (engine && InterlockedDecrement(&engine->ref) == 0)
    {
        CertCloseStore(engine->hWorld, 0);
        CertCloseStore(engine->hRoot, 0);
        CryptMemFree(engine);
    }
}

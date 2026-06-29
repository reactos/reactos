/*
 * PROJECT:     ReactOS Certificate Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CStoreList implementation
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "certmgr.h"

CStoreList::CStoreList(StoreType type) : m_Type(type)
{
}

DWORD
CStoreList::StoreTypeFlags() const
{
    switch (m_Type)
    {
        case StoreType::User:
            return CERT_SYSTEM_STORE_CURRENT_USER;
        case StoreType::Service:
            return CERT_SYSTEM_STORE_SERVICES;
        case StoreType::Computer:
            return CERT_SYSTEM_STORE_LOCAL_MACHINE;
        default:
            return 0;
    }
}

BOOL CALLBACK
CStoreList::s_StoreCallback(
    const void *pvSystemStore,
    DWORD dwFlags,
    PCERT_SYSTEM_STORE_INFO pStoreInfo,
    void *pvReserved,
    void *pvArg)
{
    CStoreList *storeList = static_cast<CStoreList *>(pvArg);
    const wchar_t *storeName = static_cast<const wchar_t *>(pvSystemStore);
    CStore *newStore = new CStore(storeName, storeList->StoreTypeFlags());
    storeList->m_Stores.AddTail(newStore);
    return TRUE; // Continue enumeration
}

void
CStoreList::LoadStores()
{
    CertEnumSystemStore(StoreTypeFlags(), NULL, this, s_StoreCallback);
}

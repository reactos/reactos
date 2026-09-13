/*
 * PROJECT:     ReactOS win32 DLLs
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS store related code for crypt32
 * COPYRIGHT:   Copyright 2025 Ratin Gao <ratin@knsoft.org>
 */

#include <windef.h>
#include <winbase.h>
#include <wincrypt.h>
#include <wine/debug.h>
#include <crypt32_private.h>

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

typedef struct _CERT_SYSTEM_STORE_LOCATION
{
    DWORD dwFlags;
    PCWSTR pwszStoreLocation;
} CERT_SYSTEM_STORE_LOCATION, *PCERT_SYSTEM_STORE_LOCATION;

static const CERT_SYSTEM_STORE_LOCATION gSystemStoreLocations[] = {
    { CERT_SYSTEM_STORE_CURRENT_USER, L"CurrentUser" },
    { CERT_SYSTEM_STORE_LOCAL_MACHINE, L"LocalMachine" },
    { CERT_SYSTEM_STORE_CURRENT_SERVICE, L"CurrentService" },
    { CERT_SYSTEM_STORE_SERVICES, L"Services" },
    { CERT_SYSTEM_STORE_USERS, L"Users" },
    { CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY, L"CurrentUserGroupPolicy" },
    { CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY, L"LocalMachineGroupPolicy" },
    { CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, L"LocalMachineEnterprise" },
};

BOOL
WINAPI
CertEnumSystemStoreLocation(
    _In_ DWORD dwFlags,
    _Inout_opt_ void *pvArg,
    __callback PFN_CERT_ENUM_SYSTEM_STORE_LOCATION pfnEnum)
{
    DWORD i;

    /* Check input flags */
    if (dwFlags != 0)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    /* Return fixed system stores */
    for (i = 0; i < ARRAYSIZE(gSystemStoreLocations); i++)
    {
        if (!pfnEnum(gSystemStoreLocations[i].pwszStoreLocation,
                     gSystemStoreLocations[i].dwFlags,
                     NULL,
                     pvArg))
        {
            return FALSE;
        }
    }

    /* FIXME: Return registered OID system stores by calling CryptEnumOIDFunction */
    FIXME("Registered OID system stores is not enumerated\n");

    return TRUE;
}

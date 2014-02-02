#include "precomp.h"

#define NDEBUG
#include <reactos/debug.h>

#define SEC_ENTRY WINAPI

typedef PVOID PSECURITY_PACKAGE_OPTIONS, PSecurityUserData;

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityPackageA(LPSTR pszPackageName)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityPackageW(LPWSTR pszPackageName)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

SECURITY_STATUS
SEC_ENTRY
AddSecurityPackageA(LPSTR pszPackageName, PSECURITY_PACKAGE_OPTIONS pOptions)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

SECURITY_STATUS
SEC_ENTRY
AddSecurityPackageW(LPWSTR pszPackageName, PSECURITY_PACKAGE_OPTIONS pOptions)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

SECURITY_STATUS
SEC_ENTRY
GetSecurityUserInfo(
    PLUID LogonId,
    ULONG Flags,
    PSecurityUserData *UserInformation)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

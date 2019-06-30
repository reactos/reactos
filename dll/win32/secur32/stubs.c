
#include <precomp.h>

#include <precomp.h>

#define NDEBUG
#include <reactos/debug.h>

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

SECURITY_STATUS
SEC_ENTRY
UnsealMessage(
    LSA_SEC_HANDLE ContextHandle,
    PSecBufferDesc MessageBuffers,
    ULONG MessageSequenceNumber,
    PULONG QualityOfProtection)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

SECURITY_STATUS
SEC_ENTRY
SealMessage(
    LSA_SEC_HANDLE ContextHandle,
    ULONG QualityOfProtection,
    PSecBufferDesc MessageBuffers,
    ULONG MessageSequenceNumber)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

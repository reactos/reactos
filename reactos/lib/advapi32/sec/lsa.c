/* $Id: lsa.c,v 1.7 2004/01/20 01:40:19 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/lsa.c
 * PURPOSE:         Local security authority functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990322 EA created
 *	19990515 EA stubs
 *      20030202 KJK compressed stubs
 */

#include <windows.h>
#include <ddk/ntddk.h>
#include <ntsecapi.h>

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaOpenPolicy(PLSA_UNICODE_STRING lsaucs,PLSA_OBJECT_ATTRIBUTES lsaoa,ACCESS_MASK access,PLSA_HANDLE lsah)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryInformationPolicy(LSA_HANDLE lsah,POLICY_INFORMATION_CLASS pic,PVOID* pv)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaFreeMemory(PVOID pv)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaClose(LSA_HANDLE ObjectHandle)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaAddAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    PLSA_UNICODE_STRING UserRights,
    ULONG CountOfRights
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaCreateTrustedDomainEx(
    LSA_HANDLE PolicyHandle,
    PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PTRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    ACCESS_MASK DesiredAccess,
    PLSA_HANDLE TrustedDomainHandle
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaDeleteTrustedDomain(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaEnumerateAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    PLSA_UNICODE_STRING *UserRights,
    PULONG CountOfRights
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaEnumerateAccountsWithUserRight(
    LSA_HANDLE PolicyHandle,
    OPTIONAL PLSA_UNICODE_STRING UserRights,
    PVOID *EnumerationBuffer,
    PULONG CountReturned
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaEnumerateTrustedDomains(
    LSA_HANDLE PolicyHandle,
    PLSA_ENUMERATION_HANDLE EnumerationContext,
    PVOID *Buffer,
    ULONG PreferedMaximumLength,
    PULONG CountReturned
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaEnumerateTrustedDomainsEx(
    LSA_HANDLE PolicyHandle,
    PLSA_ENUMERATION_HANDLE EnumerationContext,
    PVOID *Buffer,
    ULONG PreferedMaximumLength,
    PULONG CountReturned
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaLookupNames(
    LSA_HANDLE PolicyHandle,
    ULONG Count,
    PLSA_UNICODE_STRING Names,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_SID *Sids
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaLookupNames2(
    LSA_HANDLE PolicyHandle,
    ULONG Flags,
    ULONG Count,
    PLSA_UNICODE_STRING Names,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_SID2 *Sids
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaLookupSids(
    LSA_HANDLE PolicyHandle,
    ULONG Count,
    PSID *Sids,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_NAME *Names
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaOpenTrustedDomainByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    ACCESS_MASK DesiredAccess,
    PLSA_HANDLE TrustedDomainHandle
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryDomainInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    PVOID *Buffer
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryForestTrustInformation(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryTrustedDomainInfoByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID *Buffer
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryTrustedDomainInfo(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID *Buffer
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaRemoveAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    BOOL AllRights,
    PLSA_UNICODE_STRING UserRights,
    ULONG CountOfRights
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaRetrievePrivateData(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING KeyName,
    PLSA_UNICODE_STRING * PrivateData
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetDomainInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    PVOID Buffer
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PVOID Buffer
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetForestTrustInformation(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    BOOL CheckOnly,
    PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetTrustedDomainInfoByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID Buffer
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetTrustedDomainInformation(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID Buffer
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaStorePrivateData(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING KeyName,
    PLSA_UNICODE_STRING PrivateData
    )
{
  return(FALSE);
}

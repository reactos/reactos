/* $Id: lsa.c,v 1.5 2003/02/02 19:24:44 hyperion Exp $
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

/*
LsaClearAuditLog
LsaClose
LsaDelete
LsaFreeMemory
*/
NTSTATUS STDCALL _LsaStub1(ULONG_PTR Unknown0)
{
 return STATUS_NOT_IMPLEMENTED;
}

/*
LsaAddPrivilegesToAccount
LsaDeleteTrustedDomain
LsaEnumeratePrivilegesOfAccount
LsaGetQuotasForAccount
LsaGetSystemAccessAccount
LsaGetUserName
LsaSetQuotasForAccount
LsaSetSystemAccessAccount
*/
NTSTATUS STDCALL _LsaStub2(ULONG_PTR Unknown0, ULONG_PTR Unknown1)
{
 return STATUS_NOT_IMPLEMENTED;
}

/*
LsaLookupPrivilegeName
LsaLookupPrivilegeValue
LsaQueryInfoTrustedDomain
LsaQueryInformationPolicy
LsaQuerySecurityObject
LsaRemovePrivilegesFromAccount
LsaRetrievePrivateData
LsaSetInformationPolicy
LsaSetInformationTrustedDomain
LsaSetSecret
LsaSetSecurityObject
LsaStorePrivateData
*/
NTSTATUS STDCALL _LsaStub3
(
 ULONG_PTR Unknown0,
 ULONG_PTR Unknown1,
 ULONG_PTR Unknown2
)
{
 return STATUS_NOT_IMPLEMENTED;
}

/*
LsaAddAccountRights
LsaCreateAccount
LsaCreateSecret
LsaCreateTrustedDomain
LsaEnumerateAccountRights
LsaEnumerateAccountsWithUserRight
LsaLookupPrivilegeDisplayName
LsaOpenAccount
LsaOpenPolicy
LsaOpenSecret
LsaOpenTrustedDomain
LsaSetTrustedDomainInformation
*/
NTSTATUS STDCALL _LsaStub4
(
 ULONG_PTR Unknown0,
 ULONG_PTR Unknown1,
 ULONG_PTR Unknown2,
 ULONG_PTR Unknown3
)
{
 return STATUS_NOT_IMPLEMENTED;
}

/*
LsaEnumerateAccounts
LsaEnumeratePrivileges
LsaEnumerateTrustedDomains
LsaLookupNames
LsaLookupSids
LsaQuerySecret
LsaQueryTrustedDomainInfo
LsaRemoveAccountRights
*/
NTSTATUS STDCALL _LsaStub5
(
 ULONG_PTR Unknown0,
 ULONG_PTR Unknown1,
 ULONG_PTR Unknown2,
 ULONG_PTR Unknown3,
 ULONG_PTR Unknown4
)
{
 return STATUS_NOT_IMPLEMENTED;
}

/*
LsaICLookupNames
LsaICLookupSids
*/
NTSTATUS STDCALL _LsaStub8
(
 ULONG_PTR Unknown0,
 ULONG_PTR Unknown1,
 ULONG_PTR Unknown2,
 ULONG_PTR Unknown3,
 ULONG_PTR Unknown4,
 ULONG_PTR Unknown5,
 ULONG_PTR Unknown6,
 ULONG_PTR Unknown7
)
{
 return STATUS_NOT_IMPLEMENTED;
}



/* EOF */


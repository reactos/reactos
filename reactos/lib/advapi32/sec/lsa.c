/* $Id: lsa.c,v 1.11 2004/06/17 09:07:12 ekohl Exp $
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
 *
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <ntsecapi.h>

#define NDEBUG
#include <debug.h>


#ifndef SID_REVISION /* FIXME - Winnt.h */
#define	SID_REVISION (1)	/* Current revision */
#endif


/*
 * @unimplemented
 */
NTSTATUS STDCALL
LsaClose(LSA_HANDLE ObjectHandle)
{
  DPRINT1("(%p):LsaClose stub\n",ObjectHandle);
  return 0xc0000000;
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
    ULONG CountOfRights)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PLSA_HANDLE TrustedDomainHandle)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaDeleteTrustedDomain(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PULONG CountOfRights)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PULONG CountReturned)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PULONG CountReturned)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PULONG CountReturned)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
LsaFreeMemory(PVOID Buffer)
{
  return RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
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
    PLSA_TRANSLATED_SID *Sids)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PLSA_TRANSLATED_SID2 *Sids)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PLSA_TRANSLATED_NAME *Names)
{
  return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 * LsaNtStatusToWinError
 *
 * PARAMS
 *   Status [I]
 *
 * @implemented
 */
ULONG STDCALL
LsaNtStatusToWinError(NTSTATUS Status)
{
  return RtlNtStatusToDosError(Status);
}

/******************************************************************************
 * LsaOpenPolicy
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 *
 * @unimplemented
 */
NTSTATUS STDCALL
LsaOpenPolicy(PLSA_UNICODE_STRING lsaucs,
	      PLSA_OBJECT_ATTRIBUTES lsaoa,
	      ACCESS_MASK access,
	      PLSA_HANDLE PolicyHandle)
{
  DPRINT1("LsaOpenPolicy - stub\n");
  return STATUS_SUCCESS;
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
    PLSA_HANDLE TrustedDomainHandle)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryDomainInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_DOMAIN_INFORMATION_CLASS pic,
    PVOID *Buffer)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryForestTrustInformation(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
LsaQueryInformationPolicy(LSA_HANDLE PolicyHandle,
			  POLICY_INFORMATION_CLASS pic,
			  PVOID *Buffer)
{
  DPRINT1("(%p,0x%08x,%p):LsaQueryInformationPolicy stub\n",
          PolicyHandle, pic, Buffer);

  if (!Buffer)
    return FALSE;

	switch (pic)
	{
	  case PolicyAuditEventsInformation: /* 2 */
	    {
	      PPOLICY_AUDIT_EVENTS_INFO p = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POLICY_AUDIT_EVENTS_INFO));
	      p->AuditingMode = FALSE; /* no auditing */
	      *Buffer = p;
	    }
	    break;
	  case PolicyPrimaryDomainInformation: /* 3 */
	  case PolicyAccountDomainInformation: /* 5 */
	    {
	      struct di
	      { POLICY_PRIMARY_DOMAIN_INFO ppdi;
		SID sid;
	      };
	      SID_IDENTIFIER_AUTHORITY localSidAuthority = {SECURITY_NT_AUTHORITY};

	      struct di * xdi = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(xdi));
              HKEY key;
              BOOL useDefault = TRUE;
              LONG ret;

              if ((ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
               "System\\CurrentControlSet\\Services\\VxD\\VNETSUP", 0,
               KEY_READ, &key)) == ERROR_SUCCESS)
              {
                  DWORD size = 0;
                  WCHAR wg[] = { 'W','o','r','k','g','r','o','u','p',0 };

                  ret = RegQueryValueExW(key, wg, NULL, NULL, NULL, &size);
                  if (ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
                  {
                      xdi->ppdi.Name.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                       HEAP_ZERO_MEMORY, size);
                      if ((ret = RegQueryValueExW(key, wg, NULL, NULL,
                       (LPBYTE)xdi->ppdi.Name.Buffer, &size)) == ERROR_SUCCESS)
                      {
                          xdi->ppdi.Name.Length = (USHORT)size;
                          useDefault = FALSE;
                      }
                      else
                      {
                          RtlFreeHeap(RtlGetProcessHeap(), 0, xdi->ppdi.Name.Buffer);
                          xdi->ppdi.Name.Buffer = NULL;
                      }
                  }
                  RegCloseKey(key);
              }
              if (useDefault)
                  RtlCreateUnicodeStringFromAsciiz(&(xdi->ppdi.Name), "DOMAIN");
              DPRINT1("setting domain to \n");

	      xdi->ppdi.Sid = &(xdi->sid);
	      xdi->sid.Revision = SID_REVISION;
	      xdi->sid.SubAuthorityCount = 1;
	      xdi->sid.IdentifierAuthority = localSidAuthority;
	      xdi->sid.SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
	      *Buffer = xdi;
	    }
	    break;
	  case PolicyAuditLogInformation:
	  case PolicyPdAccountInformation:
	  case PolicyLsaServerRoleInformation:
	  case PolicyReplicaSourceInformation:
	  case PolicyDefaultQuotaInformation:
	  case PolicyModificationInformation:
	  case PolicyAuditFullSetInformation:
	  case PolicyAuditFullQueryInformation:
	  case PolicyDnsDomainInformation:
	  case PolicyEfsInformation:
	    {
	      DPRINT1("category not implemented\n");
	      return FALSE;
	    }
	}
	return TRUE;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryTrustedDomainInfoByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    TRUSTED_INFORMATION_CLASS pic,
    PVOID *Buffer)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryTrustedDomainInfo(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS pic,
    PVOID *Buffer)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaRemoveAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    BOOLEAN AllRights,
    PLSA_UNICODE_STRING UserRights,
    ULONG CountOfRights)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaRetrievePrivateData(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING KeyName,
    PLSA_UNICODE_STRING *PrivateData)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetDomainInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_DOMAIN_INFORMATION_CLASS pic,
    PVOID Buffer)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS pic,
    PVOID Buffer)
{
  return STATUS_NOT_IMPLEMENTED;
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
    PLSA_FOREST_TRUST_COLLISION_INFORMATION *CollisionInfo)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetTrustedDomainInfoByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    TRUSTED_INFORMATION_CLASS pic,
    PVOID Buffer)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaSetTrustedDomainInformation(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS pic,
    PVOID Buffer)
{
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaStorePrivateData(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING KeyName,
    PLSA_UNICODE_STRING PrivateData)
{
  return STATUS_NOT_IMPLEMENTED;
}

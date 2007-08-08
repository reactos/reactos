/*
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

#include <advapi32.h>

#define NDEBUG
#include <debug.h>

static handle_t LSABindingHandle = NULL;

static VOID
LSAHandleUnbind(handle_t *Handle)
{
    RPC_STATUS status;

    if (*Handle == NULL)
        return;

    status = RpcBindingFree(Handle);
    if (status)
    {
        DPRINT1("RpcBindingFree returned 0x%x\n", status);
    }
}

static VOID
LSAHandleBind(VOID)
{
    LPWSTR pszStringBinding;
    RPC_STATUS status;
    handle_t Handle;

    if (LSABindingHandle != NULL)
        return;

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      NULL,
                                      L"\\pipe\\lsarpc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        DPRINT1("RpcStringBindingCompose returned 0x%x\n", status);
        return;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &Handle);
    if (status)
    {
        DPRINT1("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
        DPRINT1("RpcStringFree returned 0x%x\n", status);
    }

    if (InterlockedCompareExchangePointer(&LSABindingHandle,
                                          (PVOID)Handle,
                                          NULL) != NULL)
    {
        LSAHandleUnbind(&Handle);
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LsaClose(LSA_HANDLE ObjectHandle)
{
    DPRINT("LsaClose(0x%p) called\n", ObjectHandle);

    LSAHandleBind();

    return LsarClose(LSABindingHandle,
                     (unsigned long)ObjectHandle);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LsaDelete(LSA_HANDLE ObjectHandle)
{
    DPRINT("LsaDelete(0x%p) called\n", ObjectHandle);

    LSAHandleBind();

    return LsarDelete(LSABindingHandle,
                      (unsigned long)ObjectHandle);
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
 * @implemented
 */
NTSTATUS
WINAPI
LsaLookupNames(
    LSA_HANDLE PolicyHandle,
    ULONG Count,
    PLSA_UNICODE_STRING Names,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_SID *Sids)
{
    PLSA_TRANSLATED_SID2 Sids2;
    LSA_TRANSLATED_SID *TranslatedSids;
    ULONG i;
    NTSTATUS Status;

    /* Call LsaLookupNames2, which supersedes this function */
    Status = LsaLookupNames2(PolicyHandle, Count, 0, Names, ReferencedDomains, &Sids2);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Translate the returned structure */
    TranslatedSids = RtlAllocateHeap(RtlGetProcessHeap(), 0, Count * sizeof(LSA_TRANSLATED_SID));
    if (!TranslatedSids)
    {
        LsaFreeMemory(Sids2);
        return SCESTATUS_NOT_ENOUGH_RESOURCE;
    }
    RtlZeroMemory(Sids, Count * sizeof(PLSA_TRANSLATED_SID));
    for (i = 0; i < Count; i++)
    {
        TranslatedSids[i].Use = Sids2[i].Use;
        if (Sids2[i].Use != SidTypeInvalid && Sids2[i].Use != SidTypeUnknown)
        {
            TranslatedSids[i].DomainIndex = Sids2[i].DomainIndex;
            if (Sids2[i].Use != SidTypeDomain)
                TranslatedSids[i].RelativeId = *GetSidSubAuthority(Sids2[i].Sid, 0);
        }
    }
    LsaFreeMemory(Sids2);

    *Sids = TranslatedSids;

    return Status;
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
WINAPI
LsaLookupSids(
    LSA_HANDLE PolicyHandle,
    ULONG Count,
    PSID *Sids,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_NAME *Names)
{
    static const UNICODE_STRING UserName = RTL_CONSTANT_STRING(L"Administrator");
    PLSA_REFERENCED_DOMAIN_LIST LocalDomains;
    PLSA_TRANSLATED_NAME LocalNames;

    DPRINT1("LsaLookupSids(): stub. Always returning 'Administrator'\n");
    if (Count != 1)
        return STATUS_NONE_MAPPED;
    LocalDomains = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(LSA_TRANSLATED_SID));
    if (!LocalDomains)
        return SCESTATUS_NOT_ENOUGH_RESOURCE;
    LocalNames = RtlAllocateHeap(RtlGetProcessHeap(), 0,  sizeof(LSA_TRANSLATED_NAME) + UserName.MaximumLength);
    if (!LocalNames)
    {
        LsaFreeMemory(LocalDomains);
        return SCESTATUS_NOT_ENOUGH_RESOURCE;
    }
    LocalDomains[0].Entries = 0;
    LocalDomains[0].Domains = NULL;
    LocalNames[0].Use = SidTypeWellKnownGroup;
    LocalNames[0].Name.Buffer = (LPWSTR)((ULONG_PTR)(LocalNames) + sizeof(LSA_TRANSLATED_NAME));
    LocalNames[0].Name.Length = UserName.Length;
    LocalNames[0].Name.MaximumLength = UserName.MaximumLength;
    RtlCopyMemory(LocalNames[0].Name.Buffer, UserName.Buffer, UserName.MaximumLength);

    *ReferencedDomains = LocalDomains;
    *Names = LocalNames;
    return STATUS_SUCCESS;
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
  static int count = 0;
  if (count++ < 20)
  {
     DPRINT1("LsaOpenPolicy - stub\n");
  }
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

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaGetUserName(
    PUNICODE_STRING *UserName,
    PUNICODE_STRING *DomainName)
{
  DPRINT1("LsaGetUserName not implemented\n");

  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LsaQueryInfoTrustedDomain (DWORD Unknonw0,
			   DWORD Unknonw1,
			   DWORD Unknonw2)
{
  DPRINT1("LsaQueryInfoTrustedDomain not implemented\n");

  return STATUS_NOT_IMPLEMENTED;
}


/* EOF */

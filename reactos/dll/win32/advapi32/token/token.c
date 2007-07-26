/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/token.c
 * PURPOSE:         Token functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <advapi32.h>

#define NDEBUG
#include <wine/debug.h>
#include <debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi32);

/*
 * @implemented
 */
BOOL STDCALL
AdjustTokenGroups (HANDLE TokenHandle,
                   BOOL ResetToDefault,
                   PTOKEN_GROUPS NewState,
                   DWORD BufferLength,
                   PTOKEN_GROUPS PreviousState,
                   PDWORD ReturnLength)
{
  NTSTATUS Status;

  Status = NtAdjustGroupsToken (TokenHandle,
                                ResetToDefault,
                                NewState,
                                BufferLength,
                                PreviousState,
                                (PULONG)ReturnLength);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
AdjustTokenPrivileges (HANDLE TokenHandle,
                       BOOL DisableAllPrivileges,
                       PTOKEN_PRIVILEGES NewState,
                       DWORD BufferLength,
                       PTOKEN_PRIVILEGES PreviousState,
                       PDWORD ReturnLength)
{
  NTSTATUS Status;

  Status = NtAdjustPrivilegesToken (TokenHandle,
                                    DisableAllPrivileges,
                                    NewState,
                                    BufferLength,
                                    PreviousState,
                                    (PULONG)ReturnLength);
  if (STATUS_NOT_ALL_ASSIGNED == Status)
    {
      SetLastError(ERROR_NOT_ALL_ASSIGNED);
      return TRUE;
    }
  if (! NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  SetLastError(ERROR_SUCCESS); /* AdjustTokenPrivileges is documented to do this */
  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
GetTokenInformation (HANDLE TokenHandle,
                     TOKEN_INFORMATION_CLASS TokenInformationClass,
                     LPVOID TokenInformation,
                     DWORD TokenInformationLength,
                     PDWORD ReturnLength)
{
  NTSTATUS Status;

  Status = NtQueryInformationToken (TokenHandle,
                                    TokenInformationClass,
                                    TokenInformation,
                                    TokenInformationLength,
                                    (PULONG)ReturnLength);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetTokenInformation (HANDLE TokenHandle,
                     TOKEN_INFORMATION_CLASS TokenInformationClass,
                     LPVOID TokenInformation,
                     DWORD TokenInformationLength)
{
  NTSTATUS Status;

  Status = NtSetInformationToken (TokenHandle,
                                  TokenInformationClass,
                                  TokenInformation,
                                  TokenInformationLength);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
AccessCheck (PSECURITY_DESCRIPTOR pSecurityDescriptor,
             HANDLE ClientToken,
             DWORD DesiredAccess,
             PGENERIC_MAPPING GenericMapping,
             PPRIVILEGE_SET PrivilegeSet,
             LPDWORD PrivilegeSetLength,
             LPDWORD GrantedAccess,
             LPBOOL AccessStatus)
{
  NTSTATUS Status;
  NTSTATUS AccessStat;

  Status = NtAccessCheck (pSecurityDescriptor,
                          ClientToken,
                          DesiredAccess,
                          GenericMapping,
                          PrivilegeSet,
                          (PULONG)PrivilegeSetLength,
                          (PACCESS_MASK)GrantedAccess,
                          &AccessStat);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  if (!NT_SUCCESS (AccessStat))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      *AccessStatus = FALSE;
      return TRUE;
    }

  *AccessStatus = TRUE;

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
OpenProcessToken (HANDLE ProcessHandle,
                  DWORD DesiredAccess,
                  PHANDLE TokenHandle)
{
  NTSTATUS Status;

  Status = NtOpenProcessToken (ProcessHandle,
                               DesiredAccess,
                               TokenHandle);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
OpenThreadToken (HANDLE ThreadHandle,
                 DWORD DesiredAccess,
                 BOOL OpenAsSelf,
                 PHANDLE TokenHandle)
{
  NTSTATUS Status;

  Status = NtOpenThreadToken (ThreadHandle,
                              DesiredAccess,
                              OpenAsSelf,
                              TokenHandle);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetThreadToken (IN PHANDLE ThreadHandle  OPTIONAL,
                IN HANDLE TokenHandle)
{
  NTSTATUS Status;
  HANDLE hThread;

  hThread = ((ThreadHandle != NULL) ? *ThreadHandle : NtCurrentThread());

  Status = NtSetInformationThread (hThread,
                                   ThreadImpersonationToken,
                                   &TokenHandle,
                                   sizeof(HANDLE));
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
DuplicateTokenEx (IN HANDLE ExistingTokenHandle,
                  IN DWORD dwDesiredAccess,
                  IN LPSECURITY_ATTRIBUTES lpTokenAttributes  OPTIONAL,
                  IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                  IN TOKEN_TYPE TokenType,
                  OUT PHANDLE DuplicateTokenHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;
  SECURITY_QUALITY_OF_SERVICE Sqos;

  Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  Sqos.ImpersonationLevel = ImpersonationLevel;
  Sqos.ContextTrackingMode = 0;
  Sqos.EffectiveOnly = FALSE;

  if (lpTokenAttributes != NULL)
    {
      InitializeObjectAttributes(&ObjectAttributes,
                                 NULL,
                                 lpTokenAttributes->bInheritHandle ? OBJ_INHERIT : 0,
                                 NULL,
                                 lpTokenAttributes->lpSecurityDescriptor);
    }
  else
    {
      InitializeObjectAttributes(&ObjectAttributes,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
    }

  ObjectAttributes.SecurityQualityOfService = &Sqos;

  Status = NtDuplicateToken (ExistingTokenHandle,
                             dwDesiredAccess,
                             &ObjectAttributes,
                             FALSE,
                             TokenType,
                             DuplicateTokenHandle);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
DuplicateToken (IN HANDLE ExistingTokenHandle,
                IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                OUT PHANDLE DuplicateTokenHandle)
{
  return DuplicateTokenEx (ExistingTokenHandle,
                           TOKEN_IMPERSONATE | TOKEN_QUERY,
                           NULL,
                           ImpersonationLevel,
                           TokenImpersonation,
                           DuplicateTokenHandle);
}


/*
 * @implemented
 */
BOOL STDCALL
CheckTokenMembership(IN HANDLE ExistingTokenHandle,
                     IN PSID SidToCheck,
                     OUT PBOOL IsMember)
{
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ACCESS_MASK GrantedAccess;
    struct
    {
        PRIVILEGE_SET PrivilegeSet;
        LUID_AND_ATTRIBUTES Privileges[4];
    } PrivBuffer;
    ULONG PrivBufferSize = sizeof(PrivBuffer);
    GENERIC_MAPPING GenericMapping =
    {
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_ALL
    };
    PACL Dacl;
    ULONG SidLen;
    HANDLE hToken = NULL;
    NTSTATUS Status, AccessStatus;

    /* doesn't return gracefully if IsMember is NULL! */
    *IsMember = FALSE;

    SidLen = RtlLengthSid(SidToCheck);

    if (ExistingTokenHandle == NULL)
    {
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_QUERY,
                                   FALSE,
                                   &hToken);

        if (Status == STATUS_NO_TOKEN)
        {
            /* we're not impersonating, open the primary token */
            Status = NtOpenProcessToken(NtCurrentProcess(),
                                        TOKEN_QUERY | TOKEN_DUPLICATE,
                                        &hToken);
            if (NT_SUCCESS(Status))
            {
                HANDLE hNewToken = FALSE;
                BOOL DupRet;

                /* duplicate the primary token to create an impersonation token */
                DupRet = DuplicateTokenEx(hToken,
                                          TOKEN_QUERY | TOKEN_IMPERSONATE,
                                          NULL,
                                          SecurityImpersonation,
                                          TokenImpersonation,
                                          &hNewToken);

                NtClose(hToken);

                if (!DupRet)
                {
                    DPRINT1("Failed to duplicate the primary token!\n");
                    return FALSE;
                }

                hToken = hNewToken;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else
    {
        hToken = ExistingTokenHandle;
    }

    /* create a security descriptor */
    SecurityDescriptor = RtlAllocateHeap(RtlGetProcessHeap(),
                                         0,
                                         sizeof(SECURITY_DESCRIPTOR) +
                                             sizeof(ACL) + SidLen +
                                             sizeof(ACCESS_ALLOWED_ACE));
    if (SecurityDescriptor == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* set the owner and group */
    Status = RtlSetOwnerSecurityDescriptor(SecurityDescriptor,
                                           SidToCheck,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlSetGroupSecurityDescriptor(SecurityDescriptor,
                                           SidToCheck,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* create the DACL */
    Dacl = (PACL)(SecurityDescriptor + 1);
    Status = RtlCreateAcl(Dacl,
                          sizeof(ACL) + SidLen + sizeof(ACCESS_ALLOWED_ACE),
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    0x1,
                                    SidToCheck);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* assign the DACL to the security descriptor */
    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* it's time to perform the access check. Just use _some_ desired access right
       (same as for the ACE) and see if we're getting it granted. This indicates
       our SID is a member of the token. We however can't use a generic access
       right as those aren't mapped and return an error (STATUS_GENERIC_NOT_MAPPED). */
    Status = NtAccessCheck(SecurityDescriptor,
                           hToken,
                           0x1,
                           &GenericMapping,
                           &PrivBuffer.PrivilegeSet,
                           &PrivBufferSize,
                           &GrantedAccess,
                           &AccessStatus);

    if (NT_SUCCESS(Status) && NT_SUCCESS(AccessStatus) && (GrantedAccess == 0x1))
    {
        *IsMember = TRUE;
    }

Cleanup:
    if (hToken != NULL && hToken != ExistingTokenHandle)
    {
        NtClose(hToken);
    }

    if (SecurityDescriptor != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    SecurityDescriptor);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
IsTokenRestricted(HANDLE TokenHandle)
{
  ULONG RetLength;
  PTOKEN_GROUPS lpGroups;
  NTSTATUS Status;
  BOOL Ret = FALSE;
  
  /* determine the required buffer size and allocate enough memory to read the
     list of restricted SIDs */

  Status = NtQueryInformationToken(TokenHandle,
                                   TokenRestrictedSids,
                                   NULL,
                                   0,
                                   &RetLength);
  if (Status != STATUS_BUFFER_TOO_SMALL)
  {
    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
  }
  
AllocAndReadRestrictedSids:
  lpGroups = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
                                      0,
                                      RetLength);
  if (lpGroups == NULL)
  {
    SetLastError(ERROR_OUTOFMEMORY);
    return FALSE;
  }
  
  /* actually read the list of the restricted SIDs */
  
  Status = NtQueryInformationToken(TokenHandle,
                                   TokenRestrictedSids,
                                   lpGroups,
                                   RetLength,
                                   &RetLength);
  if (NT_SUCCESS(Status))
  {
    Ret = (lpGroups->GroupCount != 0);
  }
  else if (Status == STATUS_BUFFER_TOO_SMALL)
  {
    /* looks like the token was modified in the meanwhile, let's just try again */

    HeapFree(GetProcessHeap(),
             0,
             lpGroups);

    goto AllocAndReadRestrictedSids;
  }
  else
  {
    SetLastError(RtlNtStatusToDosError(Status));
  }
  
  /* free allocated memory */

  HeapFree(GetProcessHeap(),
           0,
           lpGroups);

  return Ret;
}

BOOL STDCALL 
CreateRestrictedToken(
            HANDLE TokenHandle,
            DWORD Flags,
            DWORD DisableSidCount,
            PSID_AND_ATTRIBUTES pSidAndAttributes,
            DWORD DeletePrivilegeCount,
            PLUID_AND_ATTRIBUTES pLUIDAndAttributes,
            DWORD RestrictedSidCount,
            PSID_AND_ATTRIBUTES pSIDAndAttributes,
            PHANDLE NewTokenHandle
)
{
    FIXME("unimplemented!\n", __FUNCTION__);
    return FALSE;
}

/*
 * @unimplemented
 */
PSID
WINAPI
GetSiteSidFromToken(IN HANDLE TokenHandle)
{
    PTOKEN_GROUPS RestrictedSids;
    ULONG RetLen;
    UINT i;
    NTSTATUS Status;
    PSID PSiteSid = NULL;
    SID_IDENTIFIER_AUTHORITY InternetSiteAuthority = {SECURITY_INTERNETSITE_AUTHORITY};

    Status = NtQueryInformationToken(TokenHandle,
                                     TokenRestrictedSids,
                                     NULL,
                                     0,
                                     &RetLen);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    RestrictedSids = (PTOKEN_GROUPS)RtlAllocateHeap(RtlGetProcessHeap(),
                                                    0,
                                                    RetLen);
    if (RestrictedSids == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    Status = NtQueryInformationToken(TokenHandle,
                                     TokenRestrictedSids,
                                     RestrictedSids,
                                     RetLen,
                                     &RetLen);
    if (NT_SUCCESS(Status))
    {
        for (i = 0; i < RestrictedSids->GroupCount; i++) 
        {
            SID* RSSid = RestrictedSids->Groups[i].Sid;

            if (RtlCompareMemory(&(RSSid->IdentifierAuthority),
                                 &InternetSiteAuthority,
                                 sizeof(SID_IDENTIFIER_AUTHORITY)) ==
                                 sizeof(SID_IDENTIFIER_AUTHORITY))
            {
                PSiteSid = RtlAllocateHeap(RtlGetProcessHeap(),
                                           0,
                                           RtlLengthSid((RestrictedSids->
                                                         Groups[i]).Sid));
                if (PSiteSid == NULL) 
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                }
                else
                {
                    RtlCopySid(RtlLengthSid(RestrictedSids->Groups[i].Sid),
                               PSiteSid,
                               RestrictedSids->Groups[i].Sid);
                }

                break;
            }
        }
    }
    else
    {
        SetLastError(RtlNtStatusToDosError(Status));
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, RestrictedSids);
    return PSiteSid;
}


/*
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>


WINBOOL STDCALL
AreAllAccessesGranted(DWORD GrantedAccess,
		      DWORD DesiredAccess)
{
  return((BOOL)RtlAreAllAccessesGranted(GrantedAccess,
					DesiredAccess));
}


WINBOOL STDCALL
AreAnyAccessesGranted(DWORD GrantedAccess,
		      DWORD DesiredAccess)
{
  return((BOOL)RtlAreAnyAccessesGranted(GrantedAccess,
					DesiredAccess));
}


WINBOOL STDCALL
GetKernelObjectSecurity(HANDLE Handle,
			SECURITY_INFORMATION RequestedInformation,
			PSECURITY_DESCRIPTOR pSecurityDescriptor,
			DWORD nLength,
			LPDWORD lpnLengthNeeded)
{
  NTSTATUS Status;

  Status = NtQuerySecurityObject(Handle,
				 RequestedInformation,
				 pSecurityDescriptor,
				 nLength,
				 lpnLengthNeeded);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}


BOOL STDCALL
SetKernelObjectSecurity(HANDLE Handle,
			SECURITY_INFORMATION SecurityInformation,
			PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  NTSTATUS Status;

  Status = NtSetSecurityObject(Handle,
			       SecurityInformation,
			       SecurityDescriptor);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}


VOID STDCALL
MapGenericMask(PDWORD AccessMask,
	       PGENERIC_MAPPING GenericMapping)
{
  RtlMapGenericMask(AccessMask,
		    GenericMapping);
}


WINBOOL
STDCALL
ImpersonateLoggedOnUser(HANDLE hToken)
{
    return FALSE;
}

BOOL STDCALL
ImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  NTSTATUS Status;

  Status = RtlImpersonateSelf(ImpersonationLevel);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}


BOOL STDCALL
RevertToSelf(VOID)
{
  NTSTATUS Status;
  HANDLE Token = NULL;

  Status = NtSetInformationThread(NtCurrentThread(),
				  ThreadImpersonationToken,
				  &Token,
				  sizeof(HANDLE));
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}

/* EOF */


/* $Id: security.c,v 1.7 2002/09/08 10:23:06 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/security.c
 * PURPOSE:         Miscellaneous securitiy related functions
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  21/11/2001 Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>


NTSTATUS STDCALL
RtlImpersonateSelf(IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  SECURITY_QUALITY_OF_SERVICE SecQos;
  HANDLE ProcessToken;
  HANDLE ImpersonationToken;
  NTSTATUS Status;

  Status = NtOpenProcessToken(NtCurrentProcess(),
			      TOKEN_DUPLICATE,
			      &ProcessToken);
  if (!NT_SUCCESS(Status))
    return(Status);

  SecQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  SecQos.ImpersonationLevel = ImpersonationLevel;
  SecQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
  SecQos.EffectiveOnly = FALSE;

  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = 0;
  ObjectAttributes.ObjectName = NULL;
  ObjectAttributes.Attributes = 0;
  ObjectAttributes.SecurityDescriptor = NULL;
  ObjectAttributes.SecurityQualityOfService = &SecQos;

  Status = NtDuplicateToken(ProcessToken,
			    TOKEN_IMPERSONATE,
			    &ObjectAttributes,
			    0,
			    TokenImpersonation,
			    &ImpersonationToken);
  if (!NT_SUCCESS(Status))
    {
      NtClose(ProcessToken);
      return(Status);
    }

  Status = NtSetInformationThread(NtCurrentThread(),
				  ThreadImpersonationToken,
				  &ImpersonationToken,
				  sizeof(HANDLE));
  NtClose(ImpersonationToken);
  NtClose(ProcessToken);

  return(Status);
}

/* EOF */

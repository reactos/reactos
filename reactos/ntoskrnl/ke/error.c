/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/error.c
 * PURPOSE:         Error reason setting/getting
 *
 * PROGRAMMERS:     David Welch
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN ExReadyForErrors = FALSE;
PEPORT ExpDefaultErrorPort = NULL;
PEPROCESS ExpDefaultErrorPortProcess = NULL;

/*
 * @unimplemented
 */
VOID
STDCALL
KiCoprocessorError(
    VOID
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
KiUnexpectedInterrupt(
    VOID
)
{
	UNIMPLEMENTED;
}

NTSTATUS STDCALL 
NtRaiseHardError(IN NTSTATUS Status,
		 ULONG Unknown2,
		 ULONG Unknown3,
		 ULONG Unknown4,
		 ULONG Unknown5,
		 ULONG Unknown6)
{
  DPRINT1("Hard error %x\n", Status);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtSetDefaultHardErrorPort(IN HANDLE PortHandle)
{
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;
  
  PreviousMode = ExGetPreviousMode();
  
  if(!SeSinglePrivilegeCheck(SeTcbPrivilege,
                             PreviousMode))
  {
    DPRINT1("NtSetDefaultHardErrorPort: Caller requires the SeTcbPrivilege privilege!\n");
    return STATUS_PRIVILEGE_NOT_HELD;
  }
  
  /* serialization shouldn't be required here as it usually is just called once
     during startup */
  
  if(!ExReadyForErrors)
  {
    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&ExpDefaultErrorPort,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
      ExpDefaultErrorPortProcess = PsGetCurrentProcess();
      ExReadyForErrors = TRUE;
    }
  }
  else
  {
    Status = STATUS_UNSUCCESSFUL;
  }
  
  return Status;
}

/* EOF */

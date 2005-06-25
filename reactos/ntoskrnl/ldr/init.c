/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/init.c
 * PURPOSE:         Loaders for PE executables
 *
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/


#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

INIT_FUNCTION
NTSTATUS
LdrLoadInitialProcess(PHANDLE ProcessHandle,
		      PHANDLE ThreadHandle)
{
   UNICODE_STRING ImagePath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\smss.exe");
   HANDLE SystemProcessHandle;
   NTSTATUS Status;
   PRTL_USER_PROCESS_PARAMETERS Params=NULL;
   RTL_PROCESS_INFO Info;

   Status = ObpCreateHandle(
      PsGetCurrentProcess(),
      PsInitialSystemProcess,
      PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
      FALSE,
      &SystemProcessHandle
      );

   if(!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create a handle for the system process!\n");
      return Status;
   }


   Status = RtlCreateProcessParameters(
      &Params,
      &ImagePath,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
      );

  if(!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to create ppb!\n");
    ZwClose(SystemProcessHandle);
    return Status;
  }


   DPRINT("Creating process\n");

   Status = RtlCreateUserProcess(
      &ImagePath,
      OBJ_CASE_INSENSITIVE, //Valid are OBJ_INHERIT and OBJ_CASE_INSENSITIVE.
      Params,
      NULL,
      NULL,
      SystemProcessHandle,
      FALSE,
      NULL,
      NULL,
      &Info
      );

   ZwClose(SystemProcessHandle);
   RtlDestroyProcessParameters(Params);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("NtCreateProcess() failed (Status %lx)\n", Status);
      return(Status);
   }

   ZwResumeThread(Info.ThreadHandle, NULL);

   *ProcessHandle = Info.ProcessHandle;
   *ThreadHandle= Info.ThreadHandle;

   DPRINT("Process created successfully\n");

   return(STATUS_SUCCESS);
}

/* EOF */

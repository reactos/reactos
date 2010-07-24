/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Initialization routine.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"
#include <reactos/buildno.h>

#define NDEBUG
#include <debug.h>

ULONG SmSsProcessId = 0;

/* Native image's entry point */

NTSTATUS __cdecl _main(int argc,
			char *argv[],
			char *envp[],
			ULONG DebugFlag)
{
  NTSTATUS Status = STATUS_SUCCESS;
  PROCESS_BASIC_INFORMATION PBI = {0};

  /* Lookup yourself */
  Status = NtQueryInformationProcess (NtCurrentProcess(),
		    		      ProcessBasicInformation,
				      & PBI,
				      sizeof PBI,
      				      NULL);
  if(NT_SUCCESS(Status))
  {
	  SmSsProcessId = (ULONG) PBI.UniqueProcessId;
  }
  /* Initialize the system */
  Status = InitSessionManager();
  /* Watch required subsystems TODO */
#if 0
  if (!NT_SUCCESS(Status))
    {
      int i;
      for (i=0; i < (sizeof Children / sizeof Children[0]); i++)
      {
        if (Children[i])
        {
          NtTerminateProcess(Children[i],0);
        }
      }
      DPRINT1("SM: Initialization failed!\n");
      goto ByeBye;
    }

  Status = NtWaitForMultipleObjects(((LONG) sizeof(Children) / sizeof(HANDLE)),
				    Children,
				    WaitAny,
				    TRUE,	/* alertable */
				    NULL);	/* NULL for infinite */
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("SM: NtWaitForMultipleObjects failed! (Status=0x%08lx)\n", Status);
    }
  else
    {
      DPRINT1("SM: Process terminated!\n");
    }

ByeBye:
  /* Raise a hard error (crash the system/BSOD) */
  NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
		   0,0,0,0,0);

//   NtTerminateProcess(NtCurrentProcess(), 0);
#endif
	return NtTerminateThread(NtCurrentThread(), Status);
}

/* EOF */

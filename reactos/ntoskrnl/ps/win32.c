/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/win32.c
 * PURPOSE:         win32k support
 *
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

static PW32_PROCESS_CALLBACK PspWin32ProcessCallback = NULL;
static PW32_THREAD_CALLBACK PspWin32ThreadCallback = NULL;
static ULONG PspWin32ProcessSize = 0;
static ULONG PspWin32ThreadSize = 0;

/* FUNCTIONS ***************************************************************/

PW32THREAD STDCALL
PsGetWin32Thread(VOID)
{
  return(PsGetCurrentThread()->Tcb.Win32Thread);
}

PW32PROCESS STDCALL
PsGetWin32Process(VOID)
{
  return(PsGetCurrentProcess()->Win32Process);
}

NTSTATUS STDCALL
PsCreateWin32Process(PEPROCESS Process)
{
  if (Process->Win32Process != NULL)
    return(STATUS_SUCCESS);

  Process->Win32Process = ExAllocatePool(NonPagedPool,
					 PspWin32ProcessSize);
  if (Process->Win32Process == NULL)
    return(STATUS_NO_MEMORY);

  RtlZeroMemory(Process->Win32Process,
		PspWin32ProcessSize);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
PsEstablishWin32Callouts (PW32_PROCESS_CALLBACK W32ProcessCallback,
			  PW32_THREAD_CALLBACK W32ThreadCallback,
			  PVOID Param3,
			  PVOID Param4,
			  ULONG W32ThreadSize,
			  ULONG W32ProcessSize)
{
  PspWin32ProcessCallback = W32ProcessCallback;
  PspWin32ThreadCallback = W32ThreadCallback;

  PspWin32ProcessSize = W32ProcessSize;
  PspWin32ThreadSize = W32ThreadSize;
}


NTSTATUS
PsInitWin32Thread (PETHREAD Thread)
{
  PEPROCESS Process;

  Process = Thread->ThreadsProcess;

  if (Process->Win32Process == NULL)
    {
      /* FIXME - lock the process */
      Process->Win32Process = ExAllocatePool (NonPagedPool,
					      PspWin32ProcessSize);

      if (Process->Win32Process == NULL)
	return STATUS_NO_MEMORY;

      RtlZeroMemory (Process->Win32Process,
		     PspWin32ProcessSize);
      /* FIXME - unlock the process */

      if (PspWin32ProcessCallback != NULL)
	{
          PspWin32ProcessCallback (Process, TRUE);
	}
    }

  if (Thread->Tcb.Win32Thread == NULL)
    {
      Thread->Tcb.Win32Thread = ExAllocatePool (NonPagedPool,
						PspWin32ThreadSize);
      if (Thread->Tcb.Win32Thread == NULL)
	return STATUS_NO_MEMORY;

      RtlZeroMemory (Thread->Tcb.Win32Thread,
		     PspWin32ThreadSize);

      if (PspWin32ThreadCallback != NULL)
	{
	  PspWin32ThreadCallback (Thread, TRUE);
	}
    }

  return(STATUS_SUCCESS);
}


VOID
PsTerminateWin32Process (PEPROCESS Process)
{
  if (Process->Win32Process == NULL)
    return;

  if (PspWin32ProcessCallback != NULL)
    {
      PspWin32ProcessCallback (Process, FALSE);
    }

  /* don't delete the W32PROCESS structure at this point, wait until the
     EPROCESS structure is being freed */
}


VOID
PsTerminateWin32Thread (PETHREAD Thread)
{
  if (Thread->Tcb.Win32Thread != NULL)
  {
    if (PspWin32ThreadCallback != NULL)
    {
      PspWin32ThreadCallback (Thread, FALSE);
    }

    /* don't delete the W32THREAD structure at this point, wait until the
       ETHREAD structure is being freed */
  }
}

/* EOF */

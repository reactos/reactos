/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/usertrap.c
 * PURPOSE:         Handling usermode exceptions.
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
print_user_address(PVOID address)
{
   PLIST_ENTRY current_entry;
   PLDR_MODULE current;
   PEPROCESS CurrentProcess;
   PPEB Peb = NULL;
   ULONG_PTR RelativeAddress;
   PPEB_LDR_DATA Ldr;
   NTSTATUS Status;

   CurrentProcess = PsGetCurrentProcess();
   if (NULL != CurrentProcess)
     {
       Peb = CurrentProcess->Peb;
     }

   if (NULL == Peb)
     {
       DbgPrint("<%x>", address);
       return(TRUE);
     }

   Status = MmSafeCopyFromUser(&Ldr, &Peb->Ldr, sizeof(PPEB_LDR_DATA));
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("<%x>", address);
       return(TRUE);
     }
   current_entry = Ldr->InLoadOrderModuleList.Flink;

   while (current_entry != &Ldr->InLoadOrderModuleList &&
	  current_entry != NULL)
     {
	current =
	  CONTAINING_RECORD(current_entry, LDR_MODULE, InLoadOrderModuleList);

	if (address >= (PVOID)current->BaseAddress &&
	    address < (PVOID)((char*)current->BaseAddress + current->ResidentSize))
	  {
            RelativeAddress =
	      (ULONG_PTR) address - (ULONG_PTR)current->BaseAddress;
	    DbgPrint("<%wZ: %x>", &current->BaseDllName, RelativeAddress);
	    return(TRUE);
	  }

	current_entry = current_entry->Flink;
     }
   return(FALSE);
}

ULONG
KiUserTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2)
{
  EXCEPTION_RECORD Er;

  if (ExceptionNr == 0)
    {
      Er.ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
    }
  else if (ExceptionNr == 1)
    {
      Er.ExceptionCode = STATUS_SINGLE_STEP;
    }
  else if (ExceptionNr == 3)
    {
      Er.ExceptionCode = STATUS_BREAKPOINT;
    }
  else if (ExceptionNr == 4)
    {
      Er.ExceptionCode = STATUS_INTEGER_OVERFLOW;
    }
  else if (ExceptionNr == 5)
    {
      Er.ExceptionCode = STATUS_ARRAY_BOUNDS_EXCEEDED;
    }
  else if (ExceptionNr == 6)
    {
      Er.ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
    }
  else
    {
      Er.ExceptionCode = STATUS_ACCESS_VIOLATION;
    }
  Er.ExceptionFlags = 0;
  Er.ExceptionRecord = NULL;
  Er.ExceptionAddress = (PVOID)Tf->Eip;
  if (ExceptionNr == 14)
    {
      Er.NumberParameters = 2;
      Er.ExceptionInformation[0] = Tf->ErrorCode & 0x1;
      Er.ExceptionInformation[1] = (ULONG)Cr2;
    }
  else
    {
      Er.NumberParameters = 0;
    }

  /* FIXME: Which exceptions are noncontinuable? */
  Er.ExceptionFlags = 0;

  KiDispatchException(&Er, 0, Tf, UserMode, TRUE);
  return(0);
}

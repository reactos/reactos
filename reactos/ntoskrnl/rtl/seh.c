/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/seh.c
 * PURPOSE:         Compiler level Structured Exception Handling
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *   2001/10/10 CSH Created
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

int CDECL
_abnormal_termination(VOID)
{
  DPRINT("_abnormal_termination() called\n");
  return 0;
}

VOID
CDECL
_local_unwind2(
  PEXCEPTION_REGISTRATION RegistrationFrame,
  DWORD TryLevel)
{
  PSCOPETABLE_ENTRY ScopeTableHead;
  PRTL_EXCEPTION_REGISTRATION RtlRegistrationFrame =
    (PRTL_EXCEPTION_REGISTRATION)RegistrationFrame;

  DbgPrint("RegistrationFrame (0x%X)  - TryLevel (0x%X)\n",
    RegistrationFrame, TryLevel);
return;

  ScopeTableHead = RtlRegistrationFrame->ScopeTable;

  /* Begin traversing the list of SCOPETABLE_ENTRY */
  while ((ULONG_PTR)ScopeTableHead != -1)
  {

  }
}


VOID
CDECL
_global_unwind2(
  PVOID RegistrationFrame)
{
  RtlUnwind(RegistrationFrame, &&__return_label, NULL, 0);
  __return_label:;
}

#if 1
extern DWORD CDECL SEHFilterRoutine(VOID);
extern VOID CDECL SEHHandlerRoutine(VOID);
#endif


EXCEPTION_DISPOSITION
CDECL
_except_handler2(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PCONTEXT Context,
  PVOID DispatcherContext)
{
  /* FIXME: */
  return ExceptionContinueSearch;
}


EXCEPTION_DISPOSITION
CDECL
_except_handler3(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PCONTEXT Context,
  PVOID DispatcherContext)
{
  DWORD FilterFuncRet;
  DWORD TryLevel;
  EXCEPTION_POINTERS ExceptionPointers;
  //PSCOPETABLE_ENTRY pScopeTable;
  PSCOPETABLE_ENTRY ScopeTable;
  EXCEPTION_DISPOSITION ReturnValue;
  PVOID Handler;
  PVOID NewEBP;
  PRTL_EXCEPTION_REGISTRATION RtlRegistrationFrame =
    (PRTL_EXCEPTION_REGISTRATION)RegistrationFrame;

  DPRINT("__except_handler3() called\n");
  DPRINT("ExceptionRecord 0x%X\n", ExceptionRecord);
  DPRINT("    ExceptionCode 0x%X\n", ExceptionRecord->ExceptionCode);
  DPRINT("    ExceptionFlags 0x%X\n", ExceptionRecord->ExceptionFlags);
  DPRINT("RegistrationFrame 0x%X\n", RegistrationFrame);
  DPRINT("Context 0x%X\n", Context);
  DPRINT("DispatcherContext 0x%X\n", DispatcherContext);

  // Clear the direction flag (make no assumptions!)
  __asm__ ("cld");

  // if neither the EXCEPTION_UNWINDING nor EXCEPTION_EXIT_UNWIND bit
  // is set...  This is true the first time through the handler (the
  // non-unwinding case)

  if (!(ExceptionRecord->ExceptionFlags
     & (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND)))
  {
    DPRINT("Exception caught\n");

    // Build the EXCEPTION_POINTERS structure on the stack
    ExceptionPointers.ExceptionRecord = ExceptionRecord;
    ExceptionPointers.ContextRecord = Context;

    // Put the pointer to the EXCEPTION_POINTERS 4 bytes below the
    // establisher frame
    *(PDWORD)((ULONG_PTR)RtlRegistrationFrame - 4) = (ULONG_PTR)&ExceptionPointers;

    // Get a pointer to the scopetable array
    ScopeTable = RtlRegistrationFrame->ScopeTable;

    DPRINT("ScopeTable is at 0x%X\n", ScopeTable);

    // Get initial "try-level" value
    TryLevel = RtlRegistrationFrame->TryLevel;

    DPRINT("TryLevel is 0x%X\n", TryLevel);

search_for_handler:

    DPRINT("RtlRegistrationFrame->TryLevel: 0x%X\n",
      RtlRegistrationFrame->TryLevel);

    if (RtlRegistrationFrame->TryLevel != TRYLEVEL_NONE)
    {
      if (ScopeTable[TryLevel].FilterRoutine)
      {
        NewEBP = (PVOID)RtlRegistrationFrame->Ebp;
        Handler = (PVOID)ScopeTable[TryLevel].FilterRoutine;

        DPRINT("Original EBP is: 0x%X\n", NewEBP);

        DPRINT("Calling filter routine at 0x%X\n", Handler);

        // Save this frame EBP
        __asm__ ("pushl %ebp");
DPRINT("\n");
        // Switch to original EBP.  This is what allows all locals in the
        // frame to have the same value as before the exception occurred.
        __asm__ ("movl %0,%%eax; movl %0,%%ebp; call _SEHFilterRoutine;" : : \
          "g" (Handler), \
          "g" (NewEBP));
DPRINT("\n");

        // Call the filter function
        //FilterFuncRet = ScopeTable[TryLevel].FilterRoutine();
        //FilterFuncRet = SEHFilterRoutine();
DPRINT("\n");
        // Restore this frame EBP
        __asm__ ("popl %%ebp" : "=a" (FilterFuncRet));

        DPRINT("FilterRoutine returned: 0x%X\n", FilterFuncRet);

        if (FilterFuncRet != EXCEPTION_CONTINUE_SEARCH)
        {
          if (FilterFuncRet < 0) // EXCEPTION_CONTINUE_EXECUTION
            return ExceptionContinueExecution;

          DPRINT("Filter routine said: execute handler\n");

          // If we get here, EXCEPTION_EXECUTE_HANDLER was specified

          // Does the actual OS cleanup of registration frames
          // Causes this function to recurse
          _global_unwind2(RtlRegistrationFrame);

          _local_unwind2(RegistrationFrame, TryLevel);

          // NLG == "non-local-goto" (setjmp/longjmp stuff)
          //__NLG_Notify( 1 );  // EAX == scopetable->lpfnHandler

          // Set the current trylevel to whatever SCOPETABLE entry
          // was being used when a handler was found
          RtlRegistrationFrame->TryLevel = ScopeTable->PreviousTryLevel;

          // Once we get here, everything is all cleaned up, except
          // for the last frame, where we'll continue execution

          NewEBP = (PVOID)RtlRegistrationFrame->Ebp;
          Handler = (PVOID)ScopeTable[TryLevel].HandlerRoutine;

          // Switch to original EBP and call the __except block.
          // If the function returns bugcheck the system.
          __asm__ ("movl %0,%%eax; movl %0,%%ebp; call _SEHHandlerRoutine;" : : \
            "g" (Handler), \
            "g" (NewEBP));

          KeBugCheck(KMODE_EXCEPTION_NOT_HANDLED);
        }
      }

      ScopeTable = RtlRegistrationFrame->ScopeTable;

      DPRINT("ScopeTable is at 0x%X\n", ScopeTable);

      TryLevel = ScopeTable->PreviousTryLevel;

      DPRINT("TryLevel is 0x%X\n", TryLevel);

      goto search_for_handler;
    }
    else // TryLevel == TRYLEVEL_NONE
    {
      ReturnValue = ExceptionContinueSearch;
    }
  }
  else // Either EXCEPTION_UNWINDING or EXCEPTION_EXIT_UNWIND flags is set
  {
    DPRINT("Local unwind\n");

    // Save EBP
    __asm__ ("pushl %%ebp; movl %0,%%edx;" : : \
      "m" (RtlRegistrationFrame));

    /* FIXME: Why is "addl $0x10,%%esp" needed? We only push 2*4 bytes, so why pop 16? */
    __asm__ ("pushl $-1; pushl %%edx; movl %0,%%ebp; call __local_unwind2; addl $0x10,%%esp;" : : \
      "m" (RtlRegistrationFrame->Ebp));

    DPRINT("Local unwind3\n");

    // Restore EBP for this frame
    __asm__ ("popl %ebp");

    ReturnValue = ExceptionContinueSearch;
  }

  DPRINT("ReturnValue 0x%X\n", ReturnValue);

  return ReturnValue;
}

/* EOF */

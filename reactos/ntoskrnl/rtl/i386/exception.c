/* $Id: exception.c,v 1.2 2002/10/26 09:53:16 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Kernel-mode exception support for IA-32
 * FILE:              ntoskrnl/rtl/i386/exception.c
 * PROGRAMER:         Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

#if 1
VOID STDCALL
MsvcrtDebug(ULONG Value)
{
  DbgPrint("KernelDebug 0x%.08x\n", Value);
}
#endif

int
_abnormal_termination(void)
{
   DbgPrint("Abnormal Termination\n");
   return 0;
}

struct _CONTEXT;

EXCEPTION_DISPOSITION
_except_handler2(
   struct _EXCEPTION_RECORD *ExceptionRecord,
   void *RegistrationFrame,
   struct _CONTEXT *ContextRecord,
   void *DispatcherContext)
{
	DbgPrint("_except_handler2()\n");
	return (EXCEPTION_DISPOSITION)0;
}

void __cdecl
_global_unwind2(PEXCEPTION_REGISTRATION RegistrationFrame)
{
   RtlUnwind(RegistrationFrame, &&__ret_label, NULL, 0);
__ret_label:
   // return is important
   return;
}


/* Implemented in except.s */

VOID
RtlpCaptureContext(PCONTEXT pContext);

/* Macros that will help streamline the SEH implementations for
   kernel mode and user mode */

#define SehpGetStackLimits(StackBase, StackLimit) \
{ \
	(*(StackBase)) = KeGetCurrentKPCR()->StackBase; \
	(*(StackLimit)) = KeGetCurrentKPCR()->StackLimit; \
}

#define SehpGetExceptionList() \
	(PEXCEPTION_REGISTRATION)(KeGetCurrentThread()->TrapFrame ->ExceptionList)

#define SehpSetExceptionList(NewExceptionList) \
	KeGetCurrentThread()->TrapFrame->ExceptionList = (PVOID)(NewExceptionList)

#define SehpCaptureContext(Context) \
{ \
	KeTrapFrameToContext(KeGetCurrentThread()->TrapFrame, (Context)); \
}

/*** Code below this line is shared with lib/ntdll/arch/ia32/exception.c - please keep in sync ***/

VOID STDCALL
AsmDebug(ULONG Value)
{
  DbgPrint("Value 0x%.08x\n", Value);
}


/* Declare a few prototypes for the functions in except.s */

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PCONTEXT Context,
  PVOID DispatcherContext,
  PEXCEPTION_HANDLER ExceptionHandler);

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PCONTEXT Context,
  PVOID DispatcherContext,
  PEXCEPTION_HANDLER ExceptionHandler);


#ifndef NDEBUG

VOID RtlpDumpExceptionRegistrations(VOID)
{
  PEXCEPTION_REGISTRATION Current;

  DbgPrint("Dumping exception registrations:\n");

  Current = SehpGetExceptionList();

  if ((ULONG_PTR)Current != -1)
  {
    while ((ULONG_PTR)Current != -1)
    {
      DbgPrint("   (0x%08X)   HANDLER (0x%08X)\n", Current, Current->handler);
      Current = Current->prev;
    }
    DbgPrint("   End-Of-List\n");
  } else {
    DbgPrint("   No exception registrations exists.\n");
  }
}

#endif /* NDEBUG */

ULONG
RtlpDispatchException(IN PEXCEPTION_RECORD  ExceptionRecord,
	IN PCONTEXT  Context)
{
  PEXCEPTION_REGISTRATION RegistrationFrame;
  DWORD DispatcherContext;
  DWORD ReturnValue;

  DPRINT("RtlpDispatchException()\n");

#ifndef NDEBUG
  RtlpDumpExceptionRegistrations();
#endif /* NDEBUG */

  RegistrationFrame = SehpGetExceptionList();
 
  DPRINT("RegistrationFrame is 0x%X\n", RegistrationFrame);

  while ((ULONG_PTR)RegistrationFrame != -1)
  {
    EXCEPTION_RECORD ExceptionRecord2;
    DWORD Temp = 0;
    //PVOID RegistrationFrameEnd = (PVOID)RegistrationFrame + 8;

    // Make sure the registration frame is located within the stack

    DPRINT("Error checking\n");
#if 0
    if (Teb->Tib.StackBase > RegistrationFrameEnd)
    {
      DPRINT("Teb->Tib.StackBase (0x%.08x) > RegistrationFrameEnd (0x%.08x)\n",
        Teb->Tib.StackBase, RegistrationFrameEnd);
      ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
      return ExceptionContinueExecution;
    }
    // FIXME: Stack top, correct?
    if (Teb->Tib.StackLimit < RegistrationFrameEnd)
    {
      DPRINT("Teb->Tib.StackLimit (0x%.08x) > RegistrationFrameEnd (0x%.08x)\n",
        Teb->Tib.StackLimit, RegistrationFrameEnd);
      ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
      return ExceptionContinueExecution;
    }
 
    // Make sure stack is DWORD aligned
    if ((ULONG_PTR)RegistrationFrame & 3)
    {
      DPRINT("RegistrationFrameEnd (0x%.08x) is not DWORD aligned.\n",
        RegistrationFrameEnd);
      ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
      return ExceptionContinueExecution;
    }
#endif

#if 0
    /* FIXME: */
    if (someFlag)
      RtlpLogLastExceptionDisposition( hLog, retValue );
#endif

    DPRINT("Calling handler at 0x%X\n", RegistrationFrame->handler);
    DPRINT("ExceptionRecord 0x%X\n", ExceptionRecord);
    DPRINT("RegistrationFrame 0x%X\n", RegistrationFrame);
    DPRINT("Context 0x%X\n", Context);
    DPRINT("&DispatcherContext 0x%X\n", &DispatcherContext);

    ReturnValue = RtlpExecuteHandlerForException(
      ExceptionRecord,
      RegistrationFrame,
      Context,
      &DispatcherContext,
      RegistrationFrame->handler);

#ifndef NDEBUG

    DPRINT("Exception handler said 0x%X\n", ReturnValue);
	DPRINT("RegistrationFrame == 0x%.08x\n", RegistrationFrame);
	{
		PULONG sp = (PULONG)((PVOID)RegistrationFrame - 0x08);
		DPRINT("StandardESP == 0x%.08x\n", sp[0]);
		DPRINT("Exception Pointers == 0x%.08x\n", sp[1]);
		DPRINT("PrevFrame == 0x%.08x\n", sp[2]);
		DPRINT("Handler == 0x%.08x\n", sp[3]);
		DPRINT("ScopeTable == 0x%.08x\n", sp[4]);
		DPRINT("TryLevel == 0x%.08x\n", sp[5]);
		DPRINT("EBP == 0x%.08x\n", sp[6]);
	}

#endif

    if (RegistrationFrame == NULL)
    {
      ExceptionRecord->ExceptionFlags &= ~EXCEPTION_NESTED_CALL;  // Turn off flag
    }

    if (ReturnValue == ExceptionContinueExecution)
    {
      DPRINT("ReturnValue == ExceptionContinueExecution\n");
      if (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
      {
        DPRINT("(ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) == TRUE\n");

        ExceptionRecord2.ExceptionRecord = ExceptionRecord;
        ExceptionRecord2.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
        ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        ExceptionRecord2.NumberParameters = 0;
        RtlRaiseException(&ExceptionRecord2);
      }
      else
      {
        /* Copy the (possibly changed) context back to the trap frame and return */
        NtContinue(Context, FALSE);
        return ExceptionContinueExecution;
      }
    }
    else if (ReturnValue == ExceptionContinueSearch)
    {
      DPRINT("ReturnValue == ExceptionContinueSearch\n");

      /* Nothing to do here */
    }
    else if (ReturnValue == ExceptionNestedException)
    {
      DPRINT("ReturnValue == ExceptionNestedException\n");

      ExceptionRecord->ExceptionFlags |= EXCEPTION_EXIT_UNWIND;
      if (DispatcherContext > Temp)
	  {
          Temp = DispatcherContext;
	  }
    }
    else /* if (ReturnValue == ExceptionCollidedUnwind) */
    {
      DPRINT("ReturnValue == ExceptionCollidedUnwind or unknown\n");

      ExceptionRecord2.ExceptionRecord = ExceptionRecord;
      ExceptionRecord2.ExceptionCode = STATUS_INVALID_DISPOSITION;
      ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
      ExceptionRecord2.NumberParameters = 0;
      RtlRaiseException(&ExceptionRecord2);
    }

    RegistrationFrame = RegistrationFrame->prev;  // Go to previous frame
  }
 
  /* No exception handler will handle this exception */

  DPRINT("RtlpDispatchException(): Return ExceptionContinueExecution\n");

  return ExceptionContinueExecution;  
}

VOID STDCALL
RtlRaiseStatus(NTSTATUS Status)
{
  EXCEPTION_RECORD ExceptionRecord;

  DPRINT("RtlRaiseStatus(Status 0x%.08x)\n", Status);

  ExceptionRecord.ExceptionCode    = Status;
  ExceptionRecord.ExceptionRecord  = NULL;
  ExceptionRecord.NumberParameters = 0;
  ExceptionRecord.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
  RtlRaiseException (& ExceptionRecord);
}

VOID STDCALL
RtlUnwind(PEXCEPTION_REGISTRATION RegistrationFrame,
  PVOID ReturnAddress,
  PEXCEPTION_RECORD ExceptionRecord,
  DWORD EaxValue)
{
  PEXCEPTION_REGISTRATION ERHead;
  PEXCEPTION_RECORD pExceptRec;
  EXCEPTION_RECORD TempER;    
  CONTEXT Context;

  DPRINT("RtlUnwind(). RegistrationFrame 0x%X\n", RegistrationFrame);

#ifndef NDEBUG
  RtlpDumpExceptionRegistrations();
#endif /* NDEBUG */

  ERHead = SehpGetExceptionList();
 
  DPRINT("ERHead is 0x%X\n", ERHead);

  if (ExceptionRecord == NULL) // The normal case
  {
	DPRINT("ExceptionRecord == NULL (normal)\n");

    pExceptRec = &TempER;
    pExceptRec->ExceptionFlags = 0;
    pExceptRec->ExceptionCode = STATUS_UNWIND;
    pExceptRec->ExceptionRecord = NULL;
    pExceptRec->ExceptionAddress = ReturnAddress;
    pExceptRec->ExceptionInformation[0] = 0;
  }

  if (RegistrationFrame)
    pExceptRec->ExceptionFlags |= EXCEPTION_UNWINDING;
  else
    pExceptRec->ExceptionFlags |= (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND);

#ifndef NDEBUG
  DPRINT("ExceptionFlags == 0x%x:\n", pExceptRec->ExceptionFlags);
  if (pExceptRec->ExceptionFlags & EXCEPTION_UNWINDING)
  {
	  DPRINT("  * EXCEPTION_UNWINDING (0x%x)\n", EXCEPTION_UNWINDING);
  }
  if (pExceptRec->ExceptionFlags & EXCEPTION_EXIT_UNWIND)
  {
	  DPRINT("  * EXCEPTION_EXIT_UNWIND (0x%x)\n", EXCEPTION_EXIT_UNWIND);
  }
#endif /* NDEBUG */

  Context.ContextFlags =
    (CONTEXT_i386 | CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS);

  SehpCaptureContext(&Context);

  DPRINT("Context.Eip = 0x%.08x\n", Context.Eip);
  DPRINT("Context.Ebp = 0x%.08x\n", Context.Ebp);
  DPRINT("Context.Esp = 0x%.08x\n", Context.Esp);

  Context.Esp += 0x10;
  Context.Eax = EaxValue;
 
  // Begin traversing the list of EXCEPTION_REGISTRATION
  while ((ULONG_PTR)ERHead != -1)
  {
    EXCEPTION_RECORD er2;
 
    DPRINT("ERHead 0x%X\n", ERHead);

    if (ERHead == RegistrationFrame)
    {
      DPRINT("Continueing execution\n");
      NtContinue(&Context, FALSE);
      return;
    }
    else
    {
      // If there's an exception frame, but it's lower on the stack
      // than the head of the exception list, something's wrong!
      if (RegistrationFrame && (RegistrationFrame <= ERHead))
      {
        DPRINT("The exception frame is bad\n");

        // Generate an exception to bail out
        er2.ExceptionRecord = pExceptRec;
        er2.NumberParameters = 0;
        er2.ExceptionCode = STATUS_INVALID_UNWIND_TARGET;
        er2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;    
 
        RtlRaiseException(&er2);
      }
    }
 
#if 0
    Stack = ERHead + sizeof(EXCEPTION_REGISTRATION);
    if ( (Teb->Tib.StackBase <= (PVOID)ERHead )      // Make sure that ERHead
      && (Teb->Tib.->StackLimit >= (PVOID)Stack )      // is in range, and a multiple
      && (0 == ((ULONG_PTR)ERHead & 3)) )         // of 4 (i.e., sane)
    {
#else
    if (1) {
#endif
      PEXCEPTION_REGISTRATION NewERHead;
      PEXCEPTION_REGISTRATION pCurrExceptReg;
      EXCEPTION_DISPOSITION ReturnValue;
  
      DPRINT("Executing handler at 0x%X for unwind\n", ERHead->handler);

      ReturnValue = RtlpExecuteHandlerForUnwind(
        pExceptRec,
        ERHead,
        &Context,
        &NewERHead,
        ERHead->handler);
 
      DPRINT("Handler at 0x%X returned 0x%X\n", ERHead->handler, ReturnValue);

      if (ReturnValue != ExceptionContinueSearch)
      {
        if (ReturnValue != ExceptionCollidedUnwind)
        {
          DPRINT("Bad return value\n");

          er2.ExceptionRecord = pExceptRec;
          er2.NumberParameters = 0;
          er2.ExceptionCode = STATUS_INVALID_DISPOSITION;
          er2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;    
 
          RtlRaiseException(&er2);
        }
        else
        {
          ERHead = NewERHead;
        }
      }
 
      pCurrExceptReg = ERHead;
      ERHead = ERHead->prev;

      DPRINT("New ERHead is 0x%X\n", ERHead);

      DPRINT("Setting exception registration at 0x%X as current\n",
        RegistrationFrame->prev);

      // Unlink the exception handler
      SehpSetExceptionList(RegistrationFrame->prev);
    }
    else // The stack looks goofy! Raise an exception to bail out
    {
      DPRINT("Bad stack\n");

      er2.ExceptionRecord = pExceptRec;
      er2.NumberParameters = 0;
      er2.ExceptionCode = STATUS_BAD_STACK;
      er2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;    
 
      RtlRaiseException(&er2);
    }
  }
 
  // If we get here, we reached the end of the EXCEPTION_REGISTRATION list.
  // This shouldn't happen normally.

  DPRINT("Ran out of exception registrations. RegistrationFrame is (0x%X)\n",
    RegistrationFrame);

  if ((ULONG_PTR)RegistrationFrame == -1)
    NtContinue(&Context, FALSE);
  else
    NtRaiseException(pExceptRec, &Context, 0);
}

/* EOF */

/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: catch.c,v 1.21 2002/07/18 00:25:30 dwelch Exp $
 *
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/catch.c
 * PURPOSE:              Exception handling
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ke.h>
#include <internal/ldr.h>
#include <internal/ps.h>
#include <internal/kd.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext,
  PEXCEPTION_HANDLER Handler);


#ifndef NDEBUG

VOID RtlpDumpExceptionRegistrations(VOID)
{
  PEXCEPTION_REGISTRATION Current;
  PKTHREAD Thread;

  DbgPrint("Dumping exception registrations:\n");

  Thread = KeGetCurrentThread();

  assert(Thread);
  assert(Thread->TrapFrame);

  Current = Thread->TrapFrame->ExceptionList;

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

EXCEPTION_DISPOSITION
RtlpDispatchException(
  PEXCEPTION_RECORD ExceptionRecord,
  PCONTEXT Context)
{
  PEXCEPTION_REGISTRATION RegistrationFrame;
  DWORD DispatcherContext;
  DWORD ReturnValue;
  PKPCR KPCR;
  PKTHREAD Thread;

  DPRINT("RtlpDispatchException() called\n");
#ifndef NDEBUG
  RtlpDumpExceptionRegistrations();
#endif /* NDEBUG */
  Thread = KeGetCurrentThread();

  DPRINT("Thread is 0x%X\n", Thread);

  KPCR = KeGetCurrentKPCR();

  RegistrationFrame = Thread->TrapFrame->ExceptionList;
 
  DPRINT("RegistrationFrame is 0x%X\n", RegistrationFrame);

  while ((ULONG_PTR)RegistrationFrame != -1)
  {
    EXCEPTION_RECORD ExceptionRecord2;
    DWORD Temp = 0;
    //PVOID RegistrationFrameEnd = (PVOID)RegistrationFrame + 8;

    // Make sure the registration frame is located within the stack

    DPRINT("Error checking\n");
#if 0
    if (Thread->KernelStack > RegistrationFrameEnd)
    {
      ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
      return ExceptionDismiss;
    }
    // FIXME: Correct?
    if (Thread->StackLimit < RegistrationFrameEnd)
    {
      ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
      return ExceptionDismiss;
    }
 
    // Make sure stack is DWORD aligned
    if ((ULONG_PTR)RegistrationFrame & 3)
    {
      ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
      return ExceptionDismiss;
    }
#endif

    DPRINT("Calling handler at 0x%X\n", RegistrationFrame->handler);

    ReturnValue = RtlpExecuteHandlerForException(
      ExceptionRecord,
      RegistrationFrame,
      Context,
      &DispatcherContext,
      RegistrationFrame->handler);

    if (RegistrationFrame == NULL)
    {
      ExceptionRecord->ExceptionFlags &= ~EXCEPTION_NESTED_CALL;  // Turn off flag
    }

    if (ReturnValue == ExceptionContinueExecution)
    {
      /* Copy the changed context back to the trap frame and return */
      NtContinue(Context, FALSE);
      return ExceptionContinueExecution;
    }
    else if (ReturnValue == ExceptionDismiss)
    {
      if (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
      {
        ExceptionRecord2.ExceptionRecord = ExceptionRecord;
        ExceptionRecord2.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
        ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        ExceptionRecord2.NumberParameters = 0;
        RtlRaiseException(&ExceptionRecord2);
      }
      /* Else continue search */
    }
    else if (ReturnValue == ExceptionNestedException)
    {
      ExceptionRecord->ExceptionFlags |= EXCEPTION_EXIT_UNWIND;
      if (DispatcherContext > Temp)
          Temp = DispatcherContext;
    }
    else if (ReturnValue == ExceptionCollidedUnwind)
    {
      ExceptionRecord2.ExceptionRecord = ExceptionRecord;
      ExceptionRecord2.ExceptionCode = STATUS_INVALID_DISPOSITION;
      ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
      ExceptionRecord2.NumberParameters = 0;
      RtlRaiseException(&ExceptionRecord2);
    }

    RegistrationFrame = RegistrationFrame->prev;  // Go to previous frame
  }
 
  /* No exception handler will handle this exception */

  return ExceptionDismiss;
}


VOID 
KiDispatchException(PEXCEPTION_RECORD ExceptionRecord,
		    PCONTEXT Context,
		    PKTRAP_FRAME Tf,
		    KPROCESSOR_MODE PreviousMode,
		    BOOLEAN SearchFrames)
{
  EXCEPTION_DISPOSITION Value;
  CONTEXT TContext;

  DPRINT("KiDispatchException() called\n");

  /* PCR->KeExceptionDispatchCount++; */

  if (Context == NULL)
    {
      TContext.ContextFlags = CONTEXT_FULL;
      if (PreviousMode == UserMode)
	{
	  TContext.ContextFlags = TContext.ContextFlags | CONTEXT_DEBUGGER;
	}
  
      KeTrapFrameToContext(Tf, &TContext);

      Context = &TContext;
    }
#if 0
  if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) 
    {
      Context->Eip--;
    }
#endif
  if (PreviousMode == UserMode)
    {
      if (SearchFrames)
	{
	  PULONG Stack;
	  ULONG CDest;

	  /* FIXME: Give the kernel debugger a chance */

	  /* FIXME: Forward exception to user mode debugger */

	  /* FIXME: Check user mode stack for enough space */

	  
	  /*
	   * Let usermode try and handle the exception
	   */
	  Tf->Esp = Tf->Esp - 
	    (12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT));
	  Stack = (PULONG)Tf->Esp;
	  CDest = 3 + (ROUND_UP(sizeof(EXCEPTION_RECORD), 4) / 4);
	  /* Return address */
	  Stack[0] = 0;    
	  /* Pointer to EXCEPTION_RECORD structure */
	  Stack[1] = (ULONG)&Stack[3];   
	  /* Pointer to CONTEXT structure */
	  Stack[2] = (ULONG)&Stack[CDest];     
	  memcpy(&Stack[3], ExceptionRecord, sizeof(EXCEPTION_RECORD));
	  memcpy(&Stack[CDest], Context, sizeof(CONTEXT));

	  Tf->Eip = (ULONG)LdrpGetSystemDllExceptionDispatcher();
	  return;
	}
      
      /* FIXME: Forward the exception to the debugger */

      /* FIXME: Forward the exception to the process exception port */

      /* Terminate the offending thread */
      ZwTerminateThread(NtCurrentThread(), ExceptionRecord->ExceptionCode);

      /* If that fails then bugcheck */
      DbgPrint("Could not terminate thread\n");
      KeBugCheck(KMODE_EXCEPTION_NOT_HANDLED);
    }
  else
    {
      KD_CONTINUE_TYPE Action;

      /* PreviousMode == KernelMode */
      
      if ((!KdDebuggerEnabled) || (!(KdDebugState & KD_DEBUG_GDB)))
        {
	  /* FIXME: Get ExceptionNr and CR2 */
	  KeBugCheckWithTf (KMODE_EXCEPTION_NOT_HANDLED, 0, 0, 0, 0, Tf);
	}

      Action = KdEnterDebuggerException (ExceptionRecord, Context, Tf);
      if (Action != kdHandleException)
	{
	  Value = RtlpDispatchException (ExceptionRecord, Context);
	  
	  DPRINT("RtlpDispatchException() returned with 0x%X\n", Value);
	  /* 
	   * If RtlpDispatchException() does not handle the exception then 
	   * bugcheck 
	   */
	  if (Value != ExceptionContinueExecution)
	    {
	      KeBugCheck (KMODE_EXCEPTION_NOT_HANDLED);	      
	    }
	}
      else
        {
          KeContextToTrapFrame (Context, KeGetCurrentThread()->TrapFrame);
        }
    }
}

VOID STDCALL
ExRaiseAccessViolation (VOID)
{
  ExRaiseStatus (STATUS_ACCESS_VIOLATION);
}

VOID STDCALL
ExRaiseDatatypeMisalignment (VOID)
{
  ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
}

VOID STDCALL
ExRaiseStatus (IN NTSTATUS Status)
{
  EXCEPTION_RECORD ExceptionRecord;

  DPRINT("ExRaiseStatus(%x)\n", Status);

  ExceptionRecord.ExceptionRecord = NULL;
  ExceptionRecord.NumberParameters = 0;
  ExceptionRecord.ExceptionCode = Status;
  ExceptionRecord.ExceptionFlags = 0;

  RtlRaiseException(&ExceptionRecord);
}


NTSTATUS STDCALL
NtRaiseException (IN PEXCEPTION_RECORD ExceptionRecord,
		  IN PCONTEXT Context,
		  IN BOOLEAN SearchFrames)
{
  KiDispatchException(ExceptionRecord,
		      Context,
		      PsGetCurrentThread()->Tcb.TrapFrame,
		      ExGetPreviousMode(),
		      SearchFrames);
  return(STATUS_SUCCESS);
}


VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{
  ZwRaiseException(ExceptionRecord, NULL, TRUE);
}


inline
EXCEPTION_DISPOSITION
RtlpExecuteHandler(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext,
  PEXCEPTION_HANDLER Handler,
  PEXCEPTION_HANDLER RawHandler)
{
  EXCEPTION_DISPOSITION Value;

  // Set up an EXCEPTION_REGISTRATION
  __asm__ ("pushl %0;pushl %%fs:0;movl %%esp,%%fs:0;" : : "g" (RawHandler));

  // Invoke the exception callback function
  Value = Handler(
    ExceptionRecord,
    ExceptionRegistration,
    Context,
    DispatcherContext);
 
  // Remove the minimal EXCEPTION_REGISTRATION frame 
  //__asm__ ("movl %fs:0,%esp; popl %fs:0");

  __asm__ ("movl (%%esp),%%eax;movl %%eax,%%fs:0;addl $8,%%esp;" : : : "%eax");

  return Value;
}


EXCEPTION_DISPOSITION
RtlpExceptionHandler(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext)
{
  // If unwind flag set, return DISPOSITION_CONTINUE_SEARCH, else
  // assign DispatcherContext context and return DISPOSITION_NESTED_EXCEPTION

  if (ExceptionRecord->ExceptionFlags & EXCEPTION_UNWINDING)
  {
    DPRINT("RtlpExceptionHandler(). Returning ExceptionContinueSearch\n");
    return ExceptionContinueSearch;
  }
  else
  {
    DPRINT("RtlpExceptionHandler(). Returning ExceptionNestedException\n");
    *(PEXCEPTION_REGISTRATION*)DispatcherContext = ExceptionRegistration->prev;
    return ExceptionNestedException;
  }
}


EXCEPTION_DISPOSITION
RtlpUnwindHandler(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext)
{
  // If unwind flag set, return DISPOSITION_CONTINUE_SEARCH, else
  // assign DispatcherContext and return DISPOSITION_COLLIDED_UNWIND

  if (ExceptionRecord->ExceptionFlags & EXCEPTION_UNWINDING)
  {
    DPRINT("RtlpUnwindHandler(). Returning ExceptionContinueSearch\n");
    return ExceptionContinueSearch;
  }
  else
  {
    DPRINT("RtlpUnwindHandler(). Returning ExceptionCollidedUnwind\n");
    *(PEXCEPTION_REGISTRATION*)DispatcherContext = ExceptionRegistration->prev;
    return ExceptionCollidedUnwind;
  }
}


EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext,
  PEXCEPTION_HANDLER Handler)
{
  return RtlpExecuteHandler(
    ExceptionRecord,
    ExceptionRegistration,
    Context,
    DispatcherContext,
    Handler,
    RtlpExceptionHandler);
}


EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext,
  PEXCEPTION_HANDLER Handler)
{
  return RtlpExecuteHandler(
    ExceptionRecord,
    ExceptionRegistration,
    Context,
    DispatcherContext,
    Handler,
    RtlpUnwindHandler);
}


VOID STDCALL
RtlUnwind(
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PVOID ReturnAddress,
  PEXCEPTION_RECORD ExceptionRecord,
  DWORD EaxValue)
{
  PEXCEPTION_REGISTRATION ERHead;
  PEXCEPTION_RECORD pExceptRec;
  EXCEPTION_RECORD TempER;    
  CONTEXT Context;
  //PVOID Stack;
  PKTHREAD Thread;

  DPRINT("RtlUnwind() called. RegistrationFrame 0x%X\n", RegistrationFrame);
#ifndef NDEBUG
  RtlpDumpExceptionRegistrations();
#endif /* NDEBUG */
  Thread = KeGetCurrentThread();

  ERHead = Thread->TrapFrame->ExceptionList;

  if (ExceptionRecord == NULL) // The normal case
  {
    pExceptRec = &TempER;
 
    pExceptRec->ExceptionFlags = 0;
    pExceptRec->ExceptionCode = STATUS_UNWIND;
    pExceptRec->ExceptionRecord = NULL;
    // FIXME: Find out if NT retrieves the return address from the stack instead
    pExceptRec->ExceptionAddress = ReturnAddress;
    //pExceptRec->ExceptionInformation[0] = 0;
  }
 
  if (RegistrationFrame)
    pExceptRec->ExceptionFlags |= EXCEPTION_UNWINDING;
  else
    pExceptRec->ExceptionFlags |= (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND);
 
  Context.ContextFlags =
    (CONTEXT_i386 | CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS);

  KeTrapFrameToContext(Thread->TrapFrame, &Context);

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
      // then the head of the exception list, something's wrong!
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
    if ( (KPCR->StackBase <= (PVOID)ERHead )      // Make sure that ERHead
      && (KPCR->StackLimit >= (PVOID)Stack )      // is in range, and a multiple
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
        } else
          ERHead = NewERHead;
      }
 
      pCurrExceptReg = ERHead;
      ERHead = ERHead->prev;

      DPRINT("New ERHead is 0x%X\n", ERHead);

      DPRINT("Setting exception registration at 0x%X as current\n",
        RegistrationFrame->prev);

      // Unlink the exception handler
      KeGetCurrentKPCR()->ExceptionList = RegistrationFrame->prev;
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

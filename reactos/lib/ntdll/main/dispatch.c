/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         User-mode APC support
 * FILE:            lib/ntdll/main/dispatch.c
 * PROGRAMER:       David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

typedef NTSTATUS (STDCALL *KERNEL_CALLBACK_FUNCTION)(PVOID Argument,
                                                    ULONG ArgumentLength);

EXCEPTION_DISPOSITION
RtlpExecuteVectoredExceptionHandlers(IN PEXCEPTION_RECORD ExceptionRecord,
                                     IN PCONTEXT Context);

ULONG
RtlpDispatchException(IN PEXCEPTION_RECORD  ExceptionRecord,
                      IN PCONTEXT  Context);
/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
KiUserApcDispatcher(PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK Iosb,
		    ULONG Reserved,
		    PCONTEXT Context)
{
   /*
    * Call the APC
    */
   //DPRINT1("ITS ME\n");
   ApcRoutine(ApcContext,
	      Iosb,
	      Reserved);
   /*
    * Switch back to the interrupted context
    */
    //DPRINT1("switch back\n");
   NtContinue(Context, 1);
}

/*
 * @implemented
 */
VOID
STDCALL
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context)
{
  EXCEPTION_RECORD NestedExceptionRecord;
  NTSTATUS Status;

  if(RtlpExecuteVectoredExceptionHandlers(ExceptionRecord,
                                          Context) != ExceptionContinueExecution)
    {
      Status = NtContinue(Context, FALSE);
    }
  else
    {
      if(RtlpDispatchException(ExceptionRecord, Context) != ExceptionContinueExecution)
        {
          Status = NtContinue(Context, FALSE);
        }
      else
        {
          Status = NtRaiseException(ExceptionRecord, Context, FALSE);
        }
    }

  NestedExceptionRecord.ExceptionCode = Status;
  NestedExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
  NestedExceptionRecord.ExceptionRecord = ExceptionRecord;
  NestedExceptionRecord.NumberParameters = Status;

  RtlRaiseException(&NestedExceptionRecord);
}

/*
 * @implemented
 */
VOID
STDCALL
KiRaiseUserExceptionDispatcher(VOID)
{
  EXCEPTION_RECORD ExceptionRecord;

  ExceptionRecord.ExceptionCode = ((PTEB)NtCurrentTeb())->ExceptionCode;
  ExceptionRecord.ExceptionFlags = 0;
  ExceptionRecord.ExceptionRecord = NULL;
  ExceptionRecord.NumberParameters = 0;

  RtlRaiseException(&ExceptionRecord);
}

/*
 * @implemented
 */
VOID
STDCALL
KiUserCallbackDispatcher(ULONG RoutineIndex,
			 PVOID Argument,
			 ULONG ArgumentLength)
{
   PPEB Peb;
   NTSTATUS Status;
   KERNEL_CALLBACK_FUNCTION Callback;

   Peb = NtCurrentPeb();
   Callback = (KERNEL_CALLBACK_FUNCTION)Peb->KernelCallbackTable[RoutineIndex];
   Status = Callback(Argument, ArgumentLength);
   ZwCallbackReturn(NULL, 0, Status);
}

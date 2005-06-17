/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode exception support
 * FILE:              lib/ntdll/rtl/exception.c
 * PROGRAMERS:        David Welch <welch@cwcom.net>
 *                    Skywing <skywing@valhallalegends.com>
 *                    KJK::Hyperion <noog@libero.it>
 * UPDATES:           Skywing, 09/11/2003: Implemented RtlRaiseException and
 *                    KiUserRaiseExceptionDispatcher.
 *                    KJK::Hyperion, 22/06/2003: Moved common parts to rtl
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include <ntdll.h>

static RTL_CRITICAL_SECTION RtlpVectoredExceptionLock;
static LIST_ENTRY RtlpVectoredExceptionHead;

typedef struct _RTL_VECTORED_EXCEPTION_HANDLER
{
  LIST_ENTRY ListEntry;
  PVECTORED_EXCEPTION_HANDLER VectoredHandler;
} RTL_VECTORED_EXCEPTION_HANDLER, *PRTL_VECTORED_EXCEPTION_HANDLER;

/* FIXME - stupid ld won't resolve RtlDecodePointer! Since their implementation
           is the same just use RtlEncodePointer for now! */
#define RtlDecodePointer RtlEncodePointer

/* FUNCTIONS ***************************************************************/

VOID STDCALL
RtlBaseProcessStart(PTHREAD_START_ROUTINE StartAddress,
  PVOID Parameter);

__declspec(dllexport)
PRTL_BASE_PROCESS_START_ROUTINE RtlBaseProcessStartRoutine = RtlBaseProcessStart;

ULONG
RtlpDispatchException(IN PEXCEPTION_RECORD  ExceptionRecord,
	IN PCONTEXT  Context);

EXCEPTION_DISPOSITION
RtlpExecuteVectoredExceptionHandlers(IN PEXCEPTION_RECORD  ExceptionRecord,
                                     IN PCONTEXT  Context)
{
  PLIST_ENTRY CurrentEntry;
  PRTL_VECTORED_EXCEPTION_HANDLER veh;
  PVECTORED_EXCEPTION_HANDLER VectoredHandler;
  EXCEPTION_POINTERS ExceptionInfo;

  ExceptionInfo.ExceptionRecord = ExceptionRecord;
  ExceptionInfo.ContextRecord = Context;

  if(RtlpVectoredExceptionHead.Flink != &RtlpVectoredExceptionHead)
  {
    RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
    for(CurrentEntry = RtlpVectoredExceptionHead.Flink;
        CurrentEntry != &RtlpVectoredExceptionHead;
        CurrentEntry = CurrentEntry->Flink)
    {
      veh = CONTAINING_RECORD(CurrentEntry,
                              RTL_VECTORED_EXCEPTION_HANDLER,
                              ListEntry);
      VectoredHandler = RtlDecodePointer(veh->VectoredHandler);
      if(VectoredHandler(&ExceptionInfo) == EXCEPTION_CONTINUE_EXECUTION)
      {
        RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);
        return ExceptionContinueSearch;
      }
    }
    RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);
  }

  return ExceptionContinueExecution;
}

VOID STDCALL
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
VOID STDCALL
KiRaiseUserExceptionDispatcher(VOID)
{
  EXCEPTION_RECORD ExceptionRecord;

  ExceptionRecord.ExceptionCode = ((PTEB)NtCurrentTeb())->ExceptionCode;
  ExceptionRecord.ExceptionFlags = 0;
  ExceptionRecord.ExceptionRecord = NULL;
  ExceptionRecord.NumberParameters = 0;

  RtlRaiseException(&ExceptionRecord);
}

VOID STDCALL
RtlBaseProcessStart(PTHREAD_START_ROUTINE StartAddress,
  PVOID Parameter)
{
  NTSTATUS ExitStatus = STATUS_SUCCESS;

  ExitStatus = (NTSTATUS) (StartAddress)(Parameter);

  NtTerminateProcess(NtCurrentProcess(), ExitStatus);
}


VOID
RtlpInitializeVectoredExceptionHandling(VOID)
{
  InitializeListHead(&RtlpVectoredExceptionHead);
  RtlInitializeCriticalSection(&RtlpVectoredExceptionLock);
}


/*
 * @implemented
 */
PVOID STDCALL
RtlAddVectoredExceptionHandler(IN ULONG FirstHandler,
                               IN PVECTORED_EXCEPTION_HANDLER VectoredHandler)
{
  PRTL_VECTORED_EXCEPTION_HANDLER veh;

  veh = RtlAllocateHeap(RtlGetProcessHeap(),
                        0,
                        sizeof(RTL_VECTORED_EXCEPTION_HANDLER));
  if(veh != NULL)
  {
    veh->VectoredHandler = RtlEncodePointer(VectoredHandler);
    RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
    if(FirstHandler != 0)
    {
      InsertHeadList(&RtlpVectoredExceptionHead,
                     &veh->ListEntry);
    }
    else
    {
      InsertTailList(&RtlpVectoredExceptionHead,
                     &veh->ListEntry);
    }
    RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);
  }

  return veh;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlRemoveVectoredExceptionHandler(IN PVOID VectoredHandlerHandle)
{
  PLIST_ENTRY CurrentEntry;
  PRTL_VECTORED_EXCEPTION_HANDLER veh = NULL;
  ULONG Removed = FALSE;

  RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
  for(CurrentEntry = RtlpVectoredExceptionHead.Flink;
      CurrentEntry != &RtlpVectoredExceptionHead;
      CurrentEntry = CurrentEntry->Flink)
  {
    veh = CONTAINING_RECORD(CurrentEntry,
                            RTL_VECTORED_EXCEPTION_HANDLER,
                            ListEntry);
    if(veh == VectoredHandlerHandle)
    {
      RemoveEntryList(&veh->ListEntry);
      Removed = TRUE;
      break;
    }
  }
  RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);

  if(Removed)
  {
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                veh);
  }

  return Removed;
}

/* EOF */

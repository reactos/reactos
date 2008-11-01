/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS sysem libraries
 * PURPOSE:           Vectored Exception Handling
 * FILE:              lib/rtl/vectoreh.c
 * PROGRAMERS:        Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

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

BOOLEAN
NTAPI
RtlCallVectoredExceptionHandlers(IN PEXCEPTION_RECORD  ExceptionRecord,
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
      RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);

      if(VectoredHandler(&ExceptionInfo) == EXCEPTION_CONTINUE_EXECUTION)
      {
        return TRUE;
      }

      RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
    }
    RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);
  }

  return FALSE;
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
PVOID NTAPI
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
ULONG NTAPI
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

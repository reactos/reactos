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
static volatile LONG RtlpVectoredExceptionsInstalled;

typedef struct _RTL_VECTORED_EXCEPTION_HANDLER
{
  LIST_ENTRY ListEntry;
  PVECTORED_EXCEPTION_HANDLER VectoredHandler;
  ULONG Refs;
  BOOLEAN Deleted;
} RTL_VECTORED_EXCEPTION_HANDLER, *PRTL_VECTORED_EXCEPTION_HANDLER;

/* FUNCTIONS ***************************************************************/

BOOLEAN
NTAPI
RtlCallVectoredExceptionHandlers(IN PEXCEPTION_RECORD  ExceptionRecord,
                                 IN PCONTEXT  Context)
{
  PLIST_ENTRY CurrentEntry;
  PRTL_VECTORED_EXCEPTION_HANDLER veh = NULL;
  PVECTORED_EXCEPTION_HANDLER VectoredHandler;
  EXCEPTION_POINTERS ExceptionInfo;
  BOOLEAN Remove = FALSE;
  BOOLEAN Ret = FALSE;

  ExceptionInfo.ExceptionRecord = ExceptionRecord;
  ExceptionInfo.ContextRecord = Context;

  if(RtlpVectoredExceptionsInstalled)
  {
    RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
    CurrentEntry = RtlpVectoredExceptionHead.Flink;
    while (CurrentEntry != &RtlpVectoredExceptionHead)
    {
      veh = CONTAINING_RECORD(CurrentEntry,
                              RTL_VECTORED_EXCEPTION_HANDLER,
                              ListEntry);
      veh->Refs++;
      RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);
      
      VectoredHandler = RtlDecodePointer(veh->VectoredHandler);
      if(VectoredHandler(&ExceptionInfo) == EXCEPTION_CONTINUE_EXECUTION)
      {
        RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
        if (--veh->Refs == 0)
        {
          RemoveEntryList (&veh->ListEntry);
          _InterlockedDecrement (&RtlpVectoredExceptionsInstalled);
          Remove = TRUE;
        }
        Ret = TRUE;
        break;
      }

      RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
      
      if (--veh->Refs == 0)
      {
        CurrentEntry = veh->ListEntry.Flink;
        RemoveEntryList (&veh->ListEntry);
        _InterlockedDecrement (&RtlpVectoredExceptionsInstalled);
        RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);
        
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    veh);
        RtlEnterCriticalSection(&RtlpVectoredExceptionLock);
      }
      else
        CurrentEntry = CurrentEntry->Flink;
    }
    
    RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);
  }
  
  if (Remove)
  {
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                veh);
  }

  return Ret;
}

VOID
RtlpInitializeVectoredExceptionHandling(VOID)
{
  InitializeListHead(&RtlpVectoredExceptionHead);
  RtlInitializeCriticalSection(&RtlpVectoredExceptionLock);
  RtlpVectoredExceptionsInstalled = 0;
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
    veh->Refs = 1;
    veh->Deleted = FALSE;
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
    _InterlockedIncrement (&RtlpVectoredExceptionsInstalled);
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
  BOOLEAN Remove = FALSE;
  ULONG Ret = FALSE;

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
      if (!veh->Deleted)
      {
        if (--veh->Refs == 0)
        {
          RemoveEntryList (&veh->ListEntry);
          Remove = TRUE;
        }
        veh->Deleted = TRUE;
        Ret = TRUE;
        break;
      }
    }
  }
  RtlLeaveCriticalSection(&RtlpVectoredExceptionLock);

  if(Remove)
  {
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                veh);
  }

  return Ret;
}

/* EOF */

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

RTL_CRITICAL_SECTION RtlpVectoredHandlerLock;
LIST_ENTRY RtlpVectoredExceptionList, RtlpVectoredContinueList;

typedef struct _RTL_VECTORED_HANDLER_ENTRY
{
    LIST_ENTRY ListEntry;
    PVECTORED_EXCEPTION_HANDLER VectoredHandler;
    ULONG Refs;
} RTL_VECTORED_HANDLER_ENTRY, *PRTL_VECTORED_HANDLER_ENTRY;

/* FUNCTIONS ***************************************************************/

VOID
NTAPI
RtlpInitializeVectoredExceptionHandling(VOID)
{
    /* Initialize our two lists and the common lock */
    RtlInitializeCriticalSection(&RtlpVectoredHandlerLock);
    InitializeListHead(&RtlpVectoredExceptionList);
    InitializeListHead(&RtlpVectoredContinueList);
}

BOOLEAN
NTAPI
RtlpCallVectoredHandlers(IN PEXCEPTION_RECORD ExceptionRecord,
                         IN PCONTEXT Context,
                         IN PLIST_ENTRY VectoredHandlerList)
{
    PLIST_ENTRY CurrentEntry;
    PRTL_VECTORED_HANDLER_ENTRY VectoredExceptionHandler;
    PVECTORED_EXCEPTION_HANDLER VectoredHandler;
    EXCEPTION_POINTERS ExceptionInfo;
    BOOLEAN HandlerRemoved;
    LONG HandlerReturn;

    /*
     * Initialize these in case there are no entries,
     * or if no one handled the exception
     */
    HandlerRemoved = FALSE;
    HandlerReturn = EXCEPTION_CONTINUE_SEARCH;

    /* Set up the data to pass to the handler */
    ExceptionInfo.ExceptionRecord = ExceptionRecord;
    ExceptionInfo.ContextRecord = Context;

    /* Grab the lock */
    RtlEnterCriticalSection(&RtlpVectoredHandlerLock);

    /* Loop entries */
    CurrentEntry = VectoredHandlerList->Flink;
    while (CurrentEntry != VectoredHandlerList)
    {
        /* Get the struct */
        VectoredExceptionHandler = CONTAINING_RECORD(CurrentEntry,
                                                     RTL_VECTORED_HANDLER_ENTRY,
                                                     ListEntry);

        /* Reference it so it doesn't go away while we are using it */
        VectoredExceptionHandler->Refs++;

        /* Drop the lock before calling the handler */
        RtlLeaveCriticalSection(&RtlpVectoredHandlerLock);

        /*
         * Get the function pointer, decoding it so we will crash
         * if malicious code has altered it. That is, if something has
         * set VectoredHandler to a non-encoded pointer
         */
        VectoredHandler = RtlDecodePointer(VectoredExceptionHandler->VectoredHandler);

        /* Call the handler */
        HandlerReturn = VectoredHandler(&ExceptionInfo);

        /* Handler called -- grab the lock to dereference */
        RtlEnterCriticalSection(&RtlpVectoredHandlerLock);

        /* Dereference and see if it got deleted */
        VectoredExceptionHandler->Refs--;
        if (VectoredExceptionHandler->Refs == 0)
        {
            /* It did -- do we have to free it now? */
            if (HandlerReturn == EXCEPTION_CONTINUE_EXECUTION)
            {
                /* We don't, just remove it from the list and break out */
                RemoveEntryList(&VectoredExceptionHandler->ListEntry);
                HandlerRemoved = TRUE;
                break;
            }

            /*
             * Get the next entry before freeing,
             * and remove the current one from the list
             */
            CurrentEntry = VectoredExceptionHandler->ListEntry.Flink;
            RemoveEntryList(&VectoredExceptionHandler->ListEntry);

            /* Free the entry outside of the lock, then reacquire it */
            RtlLeaveCriticalSection(&RtlpVectoredHandlerLock);
            RtlFreeHeap(RtlGetProcessHeap(),
                        0,
                        VectoredExceptionHandler);
            RtlEnterCriticalSection(&RtlpVectoredHandlerLock);
        }
        else
        {
            /* No delete -- should we continue execution? */
            if (HandlerReturn == EXCEPTION_CONTINUE_EXECUTION)
            {
                /* Break out */
                break;
            }
            else
            {
                /* Continue searching the list */
                CurrentEntry = CurrentEntry->Flink;
            }
        }
    }

    /* Let go of the lock now */
    RtlLeaveCriticalSection(&RtlpVectoredHandlerLock);

    /* Anything to free? */
    if (HandlerRemoved)
    {
        /* Get rid of it */
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    VectoredExceptionHandler);
    }

    /* Return whether to continue execution (ignored for continue handlers) */
    return (HandlerReturn == EXCEPTION_CONTINUE_EXECUTION) ? TRUE : FALSE;
}

PVOID
NTAPI
RtlpAddVectoredHandler(IN ULONG FirstHandler,
                       IN PVECTORED_EXCEPTION_HANDLER VectoredHandler,
                       IN PLIST_ENTRY VectoredHandlerList)
{
    PRTL_VECTORED_HANDLER_ENTRY VectoredHandlerEntry;

    /* Allocate our structure */
    VectoredHandlerEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                           0,
                                           sizeof(RTL_VECTORED_HANDLER_ENTRY));
    if (!VectoredHandlerEntry) return NULL;

    /* Set it up, encoding the pointer for security */
    VectoredHandlerEntry->VectoredHandler = RtlEncodePointer(VectoredHandler);
    VectoredHandlerEntry->Refs = 1;

    /* Lock the list before modifying it */
    RtlEnterCriticalSection(&RtlpVectoredHandlerLock);

    /*
     * While holding the list lock, insert the handler
     * at beginning or end of list according to caller.
     */
    if (FirstHandler)
    {
        InsertHeadList(VectoredHandlerList,
                       &VectoredHandlerEntry->ListEntry);
    }
    else
    {
        InsertTailList(VectoredHandlerList,
                       &VectoredHandlerEntry->ListEntry);
    }

    /* Done with the list, unlock it */
    RtlLeaveCriticalSection(&RtlpVectoredHandlerLock);

    /* Return pointer to the structure as the handle */
    return VectoredHandlerEntry;
}

ULONG
NTAPI
RtlpRemoveVectoredHandler(IN PVOID VectoredHandlerHandle,
                          IN PLIST_ENTRY VectoredHandlerList)
{
    PLIST_ENTRY CurrentEntry;
    PRTL_VECTORED_HANDLER_ENTRY VectoredExceptionHandler;
    BOOLEAN HandlerRemoved;
    BOOLEAN HandlerFound;

    /* Initialize these in case we don't find anything */
    HandlerRemoved = FALSE;
    HandlerFound = FALSE;

    /* Acquire list lock */
    RtlEnterCriticalSection(&RtlpVectoredHandlerLock);

    /* Loop the list */
    CurrentEntry = VectoredHandlerList->Flink;
    while (CurrentEntry != VectoredHandlerList)
    {
        /* Get the struct */
        VectoredExceptionHandler = CONTAINING_RECORD(CurrentEntry,
                                                     RTL_VECTORED_HANDLER_ENTRY,
                                                     ListEntry);

        /* Does it match? */
        if (VectoredExceptionHandler == VectoredHandlerHandle)
        {
            /*
             * Great, this means it is a valid entry.
             * However, it may be in use by the exception
             * dispatcher, so we have a ref count to respect.
             * If we can't remove it now then it will be done
             * right after it is not in use anymore.
             *
             * Caller is supposed to keep track of if it has deleted the
             * entry and should not call us twice for the same entry.
             * We could maybe throw in some kind of ASSERT to detect this
             * if this was to become a problem.
             */
            VectoredExceptionHandler->Refs--;
            if (VectoredExceptionHandler->Refs == 0)
            {
                /* Not in use, ok to remove and free */
                RemoveEntryList(&VectoredExceptionHandler->ListEntry);
                HandlerRemoved = TRUE;
            }

            /* Found what we are looking for, stop searching */
            HandlerFound = TRUE;
            break;
        }
        else
        {
            /* Get the next entry */
            CurrentEntry = CurrentEntry->Flink;
        }
    }

    /* Done with the list, let go of the lock */
    RtlLeaveCriticalSection(&RtlpVectoredHandlerLock);

    /* Can we free what we found? */
    if (HandlerRemoved)
    {
        /* Do it */
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    VectoredExceptionHandler);
    }

    /* Return whether we found it */
    return (ULONG)HandlerFound;
}

BOOLEAN
NTAPI
RtlCallVectoredExceptionHandlers(IN PEXCEPTION_RECORD ExceptionRecord,
                                 IN PCONTEXT Context)
{
    /* Call the shared routine */
    return RtlpCallVectoredHandlers(ExceptionRecord,
                                    Context,
                                    &RtlpVectoredExceptionList);
}

VOID
NTAPI
RtlCallVectoredContinueHandlers(IN PEXCEPTION_RECORD ExceptionRecord,
                                IN PCONTEXT Context)
{
    /*
     * Call the shared routine (ignoring result,
     * execution always continues at this point)
     */
    (VOID)RtlpCallVectoredHandlers(ExceptionRecord,
                                   Context,
                                   &RtlpVectoredContinueList);
}

DECLSPEC_HOTPATCH
PVOID
NTAPI
RtlAddVectoredExceptionHandler(IN ULONG FirstHandler,
                               IN PVECTORED_EXCEPTION_HANDLER VectoredHandler)
{
    /* Call the shared routine */
    return RtlpAddVectoredHandler(FirstHandler,
                                  VectoredHandler,
                                  &RtlpVectoredExceptionList);
}

DECLSPEC_HOTPATCH
PVOID
NTAPI
RtlAddVectoredContinueHandler(IN ULONG FirstHandler,
                              IN PVECTORED_EXCEPTION_HANDLER VectoredHandler)
{
    /* Call the shared routine */
    return RtlpAddVectoredHandler(FirstHandler,
                                  VectoredHandler,
                                  &RtlpVectoredContinueList);
}

//DECLSPEC_HOTPATCH
ULONG
NTAPI
RtlRemoveVectoredExceptionHandler(IN PVOID VectoredHandlerHandle)
{
    /* Call the shared routine */
    return RtlpRemoveVectoredHandler(VectoredHandlerHandle,
                                     &RtlpVectoredExceptionList);
}

//DECLSPEC_HOTPATCH
ULONG
NTAPI
RtlRemoveVectoredContinueHandler(IN PVOID VectoredHandlerHandle)
{
    /* Call the shared routine */
    return RtlpRemoveVectoredHandler(VectoredHandlerHandle,
                                     &RtlpVectoredContinueList);
}

/* EOF */

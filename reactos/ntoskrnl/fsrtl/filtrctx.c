/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/filtrctx.c
 * PURPOSE:         File Stream Filter Context support for File System Drivers
 * PROGRAMMERS:     Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

typedef struct _FILE_OBJECT_FILTER_CONTEXTS
{
    FAST_MUTEX FilterContextsMutex;
    LIST_ENTRY FilterContexts;
} FILE_OBJECT_FILTER_CONTEXTS, *PFILE_OBJECT_FILTER_CONTEXTS;

VOID
FsRtlPTeardownPerFileObjectContexts(IN PFILE_OBJECT FileObject)
{
}


/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlIsPagingFile
 * @implemented NT 5.2
 *
 *     The FsRtlIsPagingFile routine checks if the FileObject is a Paging File.
 *
 * @param FileObject
 *        A pointer to the File Object to be tested.
 *
 * @return TRUE if the File is a Paging File, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
LOGICAL
NTAPI
FsRtlIsPagingFile(IN PFILE_OBJECT FileObject)
{
    return MmIsFileObjectAPagingFile(FileObject);
}

/*
 * @unimplemented
 */
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlLookupPerFileObjectContext(IN PFILE_OBJECT FileObject,
                                IN PVOID OwnerId OPTIONAL,
                                IN PVOID InstanceId OPTIONAL)
{
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
}

/*
 * @implemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlLookupPerStreamContextInternal(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
                                    IN PVOID OwnerId OPTIONAL,
                                    IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFSRTL_PER_STREAM_CONTEXT TmpPerStreamContext, PerStreamContext = NULL;

    ASSERT(AdvFcbHeader);
    ASSERT(FlagOn(AdvFcbHeader->Flags2, FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS));

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);

    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(AdvFcbHeader->FilterContexts)))
    {
        for (NextEntry = AdvFcbHeader->FilterContexts.Flink;
             NextEntry != &(AdvFcbHeader->FilterContexts);
             NextEntry = NextEntry->Flink)
        {
            /* If we don't have any criteria for search, first entry will be enough */
            if (!OwnerId && !InstanceId)
            {
                PerStreamContext = (PFSRTL_PER_STREAM_CONTEXT)NextEntry;
                break;
            }
            /* Else, we've to find something that matches with the parameters. */
            else
            {
                TmpPerStreamContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_STREAM_CONTEXT, Links);
                if ((InstanceId && TmpPerStreamContext->InstanceId == InstanceId && TmpPerStreamContext->OwnerId == OwnerId) ||
                    (OwnerId && TmpPerStreamContext->OwnerId == OwnerId))
                {
                    PerStreamContext = TmpPerStreamContext;
                    break;
                }
            }
        }
    }
    ExReleaseFastMutex(AdvFcbHeader->FastMutex);

    return PerStreamContext;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerFileObjectContext(IN PFILE_OBJECT FileObject,
                                IN PFSRTL_PER_FILEOBJECT_CONTEXT Ptr)
{
    KeBugCheck(FILE_SYSTEM);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
                            IN PFSRTL_PER_STREAM_CONTEXT PerStreamContext)
{
    if (!(AdvFcbHeader) || !(AdvFcbHeader->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);
    InsertHeadList(&(AdvFcbHeader->FilterContexts), &(PerStreamContext->Links));
    ExReleaseFastMutex(AdvFcbHeader->FastMutex);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlRemovePerFileObjectContext(IN PFILE_OBJECT PerFileObjectContext,
                                IN PVOID OwnerId OPTIONAL,
                                IN PVOID InstanceId OPTIONAL)
{
    KeBugCheck(FILE_SYSTEM);
    return NULL;
}

/*
 * @implemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlRemovePerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
                            IN PVOID OwnerId OPTIONAL,
                            IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFSRTL_PER_STREAM_CONTEXT TmpPerStreamContext, PerStreamContext = NULL;

    if (!(AdvFcbHeader) || !(AdvFcbHeader->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        return NULL;
    }

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);
    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(AdvFcbHeader->FilterContexts)))
    {
        for (NextEntry = AdvFcbHeader->FilterContexts.Flink;
             NextEntry != &(AdvFcbHeader->FilterContexts);
             NextEntry = NextEntry->Flink)
        {
            /* If we don't have any criteria for search, first entry will be enough */
            if (!OwnerId && !InstanceId)
            {
                PerStreamContext = (PFSRTL_PER_STREAM_CONTEXT)NextEntry;
                break;
            }
            /* Else, we've to find something that matches with the parameters. */
            else
            {
                TmpPerStreamContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_STREAM_CONTEXT, Links);
                if ((InstanceId && TmpPerStreamContext->InstanceId == InstanceId && TmpPerStreamContext->OwnerId == OwnerId) ||
                    (OwnerId && TmpPerStreamContext->OwnerId == OwnerId))
                {
                    PerStreamContext = TmpPerStreamContext;
                    break;
                }
            }
        }
        /* Finally remove entry from list */
        if (PerStreamContext)
        {
            RemoveEntryList(&(PerStreamContext->Links));
        }
    }
    ExReleaseFastMutex(AdvFcbHeader->FastMutex);

    return PerStreamContext;

}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlTeardownPerStreamContexts(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader)
{
    PLIST_ENTRY NextEntry;
    BOOLEAN IsMutexLocked = FALSE;
    PFSRTL_PER_STREAM_CONTEXT PerStreamContext;

    _SEH2_TRY
    {
        /* Acquire mutex to deal with the list */
        ExAcquireFastMutex(AdvFcbHeader->FastMutex);
        IsMutexLocked = TRUE;

        /* While there are items... */
        while (!IsListEmpty(&(AdvFcbHeader->FilterContexts)))
        {
            /* ...remove one */
            NextEntry = RemoveHeadList(&(AdvFcbHeader->FilterContexts));
            PerStreamContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_STREAM_CONTEXT, Links);

            /* Release mutex before calling callback */
            ExReleaseFastMutex(AdvFcbHeader->FastMutex);
            IsMutexLocked = FALSE;

            /* Call the callback */
            ASSERT(PerStreamContext->FreeCallback);
            (*PerStreamContext->FreeCallback)(PerStreamContext);

            /* Relock the list to continue */
            ExAcquireFastMutex(AdvFcbHeader->FastMutex);
            IsMutexLocked = TRUE;
        }
    }
    _SEH2_FINALLY
    {
        /* If mutex was locked, release */
        if (IsMutexLocked)
        {
            ExReleaseFastMutex(AdvFcbHeader->FastMutex);
        }
    }
    _SEH2_END;
}

/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/filtrctx.c
 * PURPOSE:         File Stream Filter Context support for File System Drivers
 * PROGRAMMERS:     Pierre Schweitzer (heis_spiter@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

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
 * @implemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlLookupPerStreamContextInternal(IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
                                    IN PVOID OwnerId OPTIONAL,
                                    IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFSRTL_PER_STREAM_CONTEXT TmpPerStreamContext, PerStreamContext = NULL;

    if (!(StreamContext) || !(StreamContext->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        return NULL;
    }

    ExAcquireFastMutex(StreamContext->FastMutex);
    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(StreamContext->FilterContexts)))
    {
        for (NextEntry = StreamContext->FilterContexts.Flink;
             NextEntry != &(StreamContext->FilterContexts);
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
    ExReleaseFastMutex(StreamContext->FastMutex);

    return PerStreamContext;
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
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
                            IN PFSRTL_PER_STREAM_CONTEXT Ptr)
{
    if (!(PerStreamContext) || !(PerStreamContext->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ExAcquireFastMutex(PerStreamContext->FastMutex);
    InsertHeadList(&(PerStreamContext->FilterContexts), &(Ptr->Links));
    ExReleaseFastMutex(PerStreamContext->FastMutex);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlRemovePerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
                            IN PVOID OwnerId OPTIONAL,
                            IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFSRTL_PER_STREAM_CONTEXT TmpPerStreamContext, PerStreamContext = NULL;

    if (!(StreamContext) || !(StreamContext->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        return NULL;
    }

    ExAcquireFastMutex(StreamContext->FastMutex);
    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(StreamContext->FilterContexts)))
    {
        for (NextEntry = StreamContext->FilterContexts.Flink;
             NextEntry != &(StreamContext->FilterContexts);
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
    ExReleaseFastMutex(StreamContext->FastMutex);

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
    if (!FileObject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (FileObject->Flags & FO_NAMED_PIPE)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    KEBUGCHECK(0);
    return STATUS_NOT_IMPLEMENTED;
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
    KEBUGCHECK(0);
    return NULL;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlTeardownPerStreamContexts(IN PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader)
{
    PLIST_ENTRY NextEntry;
    PFSRTL_PER_STREAM_CONTEXT PerStreamContext;

    ExAcquireFastMutex(AdvancedHeader->FastMutex);
    for (NextEntry = AdvancedHeader->FilterContexts.Flink;
         NextEntry != &(AdvancedHeader->FilterContexts);
         NextEntry = NextEntry->Flink)
    {
        PerStreamContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_STREAM_CONTEXT, Links);
        (*PerStreamContext->FreeCallback)(PerStreamContext);
    }
    InitializeListHead(&(AdvancedHeader->FilterContexts));
    ExReleaseFastMutex(AdvancedHeader->FastMutex);
}


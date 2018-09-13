/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    FiltrCtx.c

Abstract:

    This module provides three routines that allow filesystem filter drivers
    to associate state with FILE_OBJECTs -- for filesystems which support
    an extended FSRTL_COMMON_HEADER with FsContext.

    These routines depend on fields (FastMutext and FilterContexts)
    added at the end of FSRTL_COMMON_HEADER in NT 5.0.

    Filesystems should set FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS if
    these new fields are supported.  They must also initialize the mutex
    and list head.

    Filter drivers must use a common header for the context they wish to
    associate with a file object:

        FSRTL_FILTER_CONTEXT:
                LIST_ENTRY  Links;
                PVOID       OwnerId;
                PVOID       InstanceId;

    The OwnerId is a bit pattern unique to each filter driver
    (e.g. the device object).

    The InstanceId is used to specify a particular instance of the context
    data owned by a filter driver (e.g. the file object).

Author:

    Dave Probert     [DavePr]    30-May-1997

Revision History:

--*/

#include "FsRtlP.h"

#define MySearchList(pHdr, Ptr) \
    for ( Ptr = (pHdr)->Flink;  Ptr != (pHdr);  Ptr = Ptr->Flink )

//
//  Trace level for the module
//

#define Dbg                              (0x80000000)


NTKERNELAPI
NTSTATUS
FsRtlInsertFilterContext (
  IN PFILE_OBJECT           FileObject,
  IN PFSRTL_FILTER_CONTEXT  Ptr
  )
/*++

Routine Description:

    This routine associates filter driver context with a stream.

Arguments:

    FileObject - Specifies the stream of interest.

    Ptr - Pointer to the filter-specific context structure.
        The common header fields OwnerId and InstanceId should
        be filled in by the filter driver before calling.

Return Value:

    STATUS_SUCCESS - operation succeeded.

    STATUS_INVALID_DEVICE_REQUEST - underlying filesystem does not support
        filter contexts.

--*/

{
    PFSRTL_COMMON_FCB_HEADER  Header;

    ASSERT(FileObject);
    ASSERT(FileObject->FsContext);

    Header = FileObject->FsContext;

    if (! (Header->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS) ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ExAcquireFastMutex (Header->FastMutex);

    InsertHeadList (&Header->FilterContexts, &Ptr->Links);

    ExReleaseFastMutex(Header->FastMutex);
    return STATUS_SUCCESS;
}


NTKERNELAPI
PFSRTL_FILTER_CONTEXT
FsRtlLookupFilterContextInternal (
  IN PFILE_OBJECT  FileObject,
  IN PVOID         OwnerId     OPTIONAL,
  IN PVOID         InstanceId  OPTIONAL
  )
/*++

Routine Description:

    This routine lookups filter driver context associated with a stream.

    The macro FsRtlLookupFilterContext should be used instead of calling
    this routine directly.  The macro optimizes for the common case
    of an empty list.

Arguments:

    FileObject - Specifies the stream of interest.

    OwnerId - Used to identify context information belonging to a particular
        filter driver.

    InstanceId - Used to search for a particular instance of a filter driver
        context.  If not provided, any of the contexts owned by the filter
        driver is returned.

    If neither the OwnerId nor the InstanceId is provided, any associated
        filter context will be returned.

Return Value:

    A pointer to the filter context, or NULL if no match found.

--*/

{
    PFSRTL_COMMON_FCB_HEADER  Header;
    PFSRTL_FILTER_CONTEXT     Ctx, RtnCtx;
    PLIST_ENTRY               List;

    ASSERT(FileObject);
    ASSERT(FileObject->FsContext);

    Header = FileObject->FsContext;

    ASSERT (Header->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS);

    ExAcquireFastMutex (Header->FastMutex);
    RtnCtx = NULL;

  // Use different loops depending on whether we are comparing both Ids or not.
    if ( ARGUMENT_PRESENT(InstanceId) ) {

        MySearchList (&Header->FilterContexts, List) {
            Ctx  = CONTAINING_RECORD (List, FSRTL_FILTER_CONTEXT, Links);
            if (Ctx->OwnerId == OwnerId && Ctx->InstanceId == InstanceId) {
                RtnCtx = Ctx;
                break;
            }
        }
    } else if ( ARGUMENT_PRESENT(OwnerId) ) {

        MySearchList (&Header->FilterContexts, List) {
            Ctx  = CONTAINING_RECORD (List, FSRTL_FILTER_CONTEXT, Links);
            if (Ctx->OwnerId == OwnerId) {
                RtnCtx = Ctx;
                break;
            }
        }
    } else if (!IsListEmpty(&Header->FilterContexts)) {

        RtnCtx = (PFSRTL_FILTER_CONTEXT) Header->FilterContexts.Flink;
    }

    ExReleaseFastMutex(Header->FastMutex);
    return RtnCtx;
}


NTKERNELAPI
PFSRTL_FILTER_CONTEXT
FsRtlRemoveFilterContext (
  IN PFILE_OBJECT  FileObject,
  IN PVOID         OwnerId     OPTIONAL,
  IN PVOID         InstanceId  OPTIONAL
  )
/*++

Routine Description:

    This routine deletes filter driver context associated with a stream.

    Filter drivers must explicitly remove all context they associate with
    a stream (otherwise the underlying filesystem will BugCheck at close).

    FsRtlRemoveFilterContext functions identically to FsRtlLookupFilterContext,
    except that the returned context has been removed from the list.

Arguments:

    FileObject - Specifies the stream of interest.

    OwnerId - Used to identify context information belonging to a particular
        filter driver.

    InstanceId - Used to search for a particular instance of a filter driver
        context.  If not provided, any of the contexts owned by the filter
        driver is removed and returned.

    If neither the OwnerId nor the InstanceId is provided, any associated
        filter context will be removed and returned.

Return Value:

    A pointer to the filter context, or NULL if no match found.

--*/

{
    PFSRTL_COMMON_FCB_HEADER  Header;
    PFSRTL_FILTER_CONTEXT     Ctx, RtnCtx;
    PLIST_ENTRY               List;

    ASSERT(FileObject);

    Header = FileObject->FsContext;

    if ( !Header || !(Header->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS) ) {
        return NULL;
    }

    ExAcquireFastMutex (Header->FastMutex);
    RtnCtx = NULL;

  // Use different loops depending on whether we are comparing both Ids or not.
    if ( ARGUMENT_PRESENT(InstanceId) ) {

        MySearchList (&Header->FilterContexts, List) {
            Ctx  = CONTAINING_RECORD (List, FSRTL_FILTER_CONTEXT, Links);
            if (Ctx->OwnerId == OwnerId && Ctx->InstanceId == InstanceId) {
                RtnCtx = Ctx;
                break;
            }
        }
    } else if ( ARGUMENT_PRESENT(OwnerId) ) {

        MySearchList (&Header->FilterContexts, List) {
            Ctx  = CONTAINING_RECORD (List, FSRTL_FILTER_CONTEXT, Links);
            if (Ctx->OwnerId == OwnerId) {
                RtnCtx = Ctx;
                break;
            }
        }
    } else if (!IsListEmpty(&Header->FilterContexts)) {

        RtnCtx = (PFSRTL_FILTER_CONTEXT) Header->FilterContexts.Flink;
    }

    if (RtnCtx) {
        RemoveEntryList(&RtnCtx->Links);   // remove the matched entry
    }
    ExReleaseFastMutex(Header->FastMutex);
    return RtnCtx;
}


NTKERNELAPI
VOID
FsRtlTeardownFilterContexts (
  IN PLIST_ENTRY FilterContexts
  )
/*++

Routine Description:

    This routine is called by filesystems to free the filter contexts
    associated with an FSRTL_COMMON_FCB_HEADER by calling the FreeCallback
    routine for each FilterContext.

    It is assumed that we do not need to acquire the FastMutex.

Arguments:

    FilterContexts - the address of the FilterContexts field within
        the FSRTL_COMMON_FCB_HEADER of the structure being torn down
        by the filesystem.

Return Value:

    None.

--*/

{
    PFSRTL_FILTER_CONTEXT ctx;
    PLIST_ENTRY           ptr;

    ptr = FilterContexts->Flink;

    while ( ptr != FilterContexts ) {
        ctx = CONTAINING_RECORD (ptr, FSRTL_FILTER_CONTEXT, Links);
        ptr = ptr->Flink;
        (*ctx->FreeCallback)(ctx);
    }

    InitializeListHead( FilterContexts );
    return;
}


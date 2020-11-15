/*++

  Copyright (c) Microsoft Corporation

  Module Name:

  FxIoQueueUm.hpp

  Abstract:

  This module implements um specific functions for FxIoQueue.

  Author:



  Environment:

  User mode only

  Revision History:

  --*/

#ifndef _FXIOQUEUEUM_HPP_
#define _FXIOQUEUEUM_HPP_

__inline
BOOLEAN
IsPagingIo(
    __in PIRP Irp
    )
/*++

Routine Description:
    Dummy UM implementation.
--*/
{
    UNREFERENCED_PARAMETER(Irp);
    return TRUE;
}

__inline
_Must_inspect_result_
NTSTATUS
FxIoQueue::QueueForwardProgressIrpLocked(
    __in MdIrp Irp
    )
{
    UNREFERENCED_PARAMETER(Irp);

    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}


__inline
_Must_inspect_result_
MdIrp
FxIoQueue::GetForwardProgressIrpLocked(
    __in_opt PFILE_OBJECT FileObject
    )
/*++

    Routine Description:
        Remove an IRP from the pending irp list if it matches with the input
        fileobject. If the fileobject value is NULL, return the first one from
        the pending list.

--*/
{
    UNREFERENCED_PARAMETER(FileObject);

    UfxVerifierTrapNotImpl();
    return NULL;

}

__inline
VOID
FxIoQueue::FreeAllReservedRequests(
    __in BOOLEAN Verify
    )
/*++

Routine Description:
    Called from dispose to Free all the reserved requests.

    Verify -
        TRUE - Make sure the number of request freed matches with the count of
               request created.
        FALSE - Called when we fail to allocate all the reserved requests
                during config at init time. So we don't verify because the
                count of request freed wouldn't match with the configured value.
--*/
{
    UNREFERENCED_PARAMETER(Verify);

    UfxVerifierTrapNotImpl();
    return;

}

__inline
VOID
FxIoQueue::ReturnReservedRequest(
    __in FxRequest *ReservedRequest
    )
/*++

Routine Description:
    Reuse the ReservedRequest if there are pended IRPs otherwise
    add it back to the reserve list.

--*/
{

    UNREFERENCED_PARAMETER(ReservedRequest);

    UfxVerifierTrapNotImpl();
    return ;

}

__inline
VOID
FxIoQueue::GetForwardProgressIrps(
    __in     PLIST_ENTRY    IrpListHead,
    __in_opt MdFileObject   FileObject
    )
/*++

Routine Description:

    This function is called to retrieve the list of reserved queued IRPs.
    The IRP's Tail.Overlay.ListEntry field is used to link these structs together.

--*/
{
    UNREFERENCED_PARAMETER(IrpListHead);
    UNREFERENCED_PARAMETER(FileObject);

    UfxVerifierTrapNotImpl();
    return;

}

__inline
VOID
FxIoQueue::FlushQueuedDpcs(
    VOID
    )
/*++

Routine Description:

    This is the kernel mode routine to flush queued DPCs.

Arguments:

Return Value:

--*/
{
    UfxVerifierTrapNotImpl();
}


__inline
VOID
FxIoQueue::InsertQueueDpc(
    VOID
    )
/*++

Routine Description:

    This is the kernel mode routine to insert a dpc.

Arguments:

Return Value:

--*/
{
    UfxVerifierTrapNotImpl();
}

__inline
_Must_inspect_result_
NTSTATUS
FxIoQueue::GetReservedRequest(
    __in MdIrp Irp,
    __deref_out_opt FxRequest **ReservedRequest
    )
/*++

Routine Description:
    Use the policy configured on the queue to decide whether to allocate a
    reserved request.

--*/
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(ReservedRequest);

    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

__inline
_Must_inspect_result_
NTSTATUS
FxIoQueue::AssignForwardProgressPolicy(
    __in PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Policy
    )
/*++

Routine Description:
    Configure the queue for forward Progress.

--*/
{
    UNREFERENCED_PARAMETER(Policy);

    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}


#endif // _FXIOQUEUEUM_HPP

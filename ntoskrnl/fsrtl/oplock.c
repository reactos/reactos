/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/oplock.c
 * PURPOSE:         Provides an Opportunistic Lock for file system drivers.
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlCheckOplock
 * @unimplemented
 *
 * FILLME
 *
 * @param Oplock
 *        FILLME
 *
 * @param Irp
 *        FILLME
 *
 * @param Context
 *        FILLME
 *
 * @param CompletionRoutine
 *        FILLME
 *
 * @param PostIrpRoutine
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlCheckOplock(IN POPLOCK Oplock,
                 IN PIRP Irp,
                 IN PVOID Context,
                 IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
                 IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL)
{
    /* Unimplemented */
    KEBUGCHECK(0);
    return STATUS_NOT_IMPLEMENTED;
}

/*++
 * @name FsRtlCurrentBatchOplock
 * @unimplemented
 *
 * FILLME
 *
 * @param Oplock
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlCurrentBatchOplock(IN POPLOCK Oplock)
{
    /* Unimplemented */
    KEBUGCHECK(0);
    return FALSE;
}

/*++
 * @name FsRtlInitializeOplock
 * @implemented
 *
 * This function initializes the given opportunistic lock pointer
 *
 * @param Oplock
 *        The opportunistic lock to initialize
 *
 * @return None
 *
 * @remarks Truth is this function does nothing!
 *
 *--*/
VOID
NTAPI
FsRtlInitializeOplock(IN OUT POPLOCK Oplock)
{
    UNREFERENCED_PARAMETER(Oplock);
}

/*++
 * @name FsRtlOplockFsctrl
 * @unimplemented
 *
 * FILLME
 *
 * @param Oplock
 *        FILLME
 *
 * @param Irp
 *        FILLME
 *
 * @param OpenCount
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlOplockFsctrl(IN POPLOCK Oplock,
                  IN PIRP Irp,
                  IN ULONG OpenCount)
{
    /* Unimplemented */
    KEBUGCHECK(0);
    return STATUS_NOT_IMPLEMENTED;
}

/*++
 * @name FsRtlOplockIsFastIoPossible
 * @unimplemented
 *
 * FILLME
 *
 * @param Oplock
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlOplockIsFastIoPossible(IN POPLOCK Oplock)
{
    /* Unimplemented */
    KEBUGCHECK(0);
    return FALSE;
}

/*++
 * @name FsRtlUninitializeOplock
 * @unimplemented
 *
 * FILLME
 *
 * @param Oplock
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlUninitializeOplock(IN POPLOCK Oplock)
{
    /* Unimplemented */
    KEBUGCHECK(0);
}


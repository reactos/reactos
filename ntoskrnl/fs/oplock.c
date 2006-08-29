/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fs/oplock.c
 * PURPOSE:         Oplock-functions
 *
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return FALSE;
}

/*++
 * @name FsRtlInitializeOplock
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
FsRtlInitializeOplock(IN OUT POPLOCK Oplock)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
}

/* EOF */

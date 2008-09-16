/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/pnp.c
 * PURPOSE:         Manages PnP support routines for file system drivers.
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlNotifyVolumeEvent
 * @unimplemented
 *
 * FILLME
 *
 * @param FileObject
 *        FILLME
 *
 * @param EventCode
 *        FILLME
 *
 * @return None
 *
 * @remarks Only present in NT 5+.
 *
 *--*/
NTSTATUS
NTAPI
FsRtlNotifyVolumeEvent(IN PFILE_OBJECT FileObject,
                       IN ULONG EventCode)
{
    /* Unimplemented */
    KeBugCheck(FILE_SYSTEM);
    return STATUS_NOT_IMPLEMENTED;
}

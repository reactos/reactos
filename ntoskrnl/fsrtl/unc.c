/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/unc.c
 * PURPOSE:         Manages UNC support routines for file system drivers.
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlDeregisterUncProvider
 * @unimplemented
 *
 * FILLME
 *
 * @param Handle
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlDeregisterUncProvider(IN HANDLE Handle)
{
    /* Unimplemented */
    KeBugCheck(FILE_SYSTEM);
}

/*++
 * @name FsRtlRegisterUncProvider
 * @unimplemented
 *
 * FILLME
 *
 * @param Handle
 *        FILLME
 *
 * @param RedirectorDeviceName
 *        FILLME
 *
 * @param MailslotsSupported
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlRegisterUncProvider(IN OUT PHANDLE Handle,
                         IN PUNICODE_STRING RedirectorDeviceName,
                         IN BOOLEAN MailslotsSupported)
{
    /* Unimplemented */
    KeBugCheck(FILE_SYSTEM);
    return STATUS_NOT_IMPLEMENTED;
}

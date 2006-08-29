/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fs/unc.c
 * PURPOSE:         Functions to work with UNC providers
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */

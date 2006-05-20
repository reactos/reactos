/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/unc.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Eric Kohl
 */

#include <ntoskrnl.h>


/**********************************************************************
 * NAME                             EXPORTED
 *  FsRtlDeregisterUncProvider@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
VOID
NTAPI
FsRtlDeregisterUncProvider(IN HANDLE Handle)
{
}


/**********************************************************************
 * NAME                             EXPORTED
 *  FsRtlRegisterUncProvider@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
NTSTATUS
NTAPI
FsRtlRegisterUncProvider(IN OUT PHANDLE Handle,
                         IN PUNICODE_STRING RedirectorDeviceName,
                         IN BOOLEAN MailslotsSupported)
{
    return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */

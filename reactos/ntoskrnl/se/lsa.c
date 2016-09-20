/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/lsa.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * NOTE: The following LSA functions:
 *  LsaCallAuthenticationPackage
 *  LsaFreeReturnBuffer
 *  LsaLogonUser
 *  LsaLookupAuthenticationPackage
 *  LsaRegisterLogonProcess
 *  LsaDeregisterLogonProcess
 * are already implemented in the 'lsalib' library (sdk/lib/lsalib/lsa.c).
 */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification(IN PLUID LogonId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine(IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeUnregisterLogonSessionTerminatedRoutine(IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

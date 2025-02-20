/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power request object implementation base support routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

GENERIC_MAPPING PopPowerRequestGenericMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

POBJECT_TYPE PopPowerRequestObjectType = NULL;
LIST_ENTRY PopPowerRequestsList;
ULONG PopTotalKernelPowerRequestsCount;
ULONG PopTotalUserPowerRequestsCount;
KSPIN_LOCK PopPowerRequestLock;
PKTHREAD PopPowerRequestOwnerLockThread;

/* PRIVATE FUNCTIONS **********************************************************/

static
NTSTATUS
PopCreatePowerRequestAsLegacy(
    _In_ EXECUTION_STATE EsFlags,
    _In_ BOOLEAN NewRegister,
    _Inout_ PPOP_POWER_REQUEST *Handle)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopDeletePowerRequestObject(
    _In_ PVOID PowerRequestObject)
{
    return;
}

VOID
NTAPI
PopClosePowerRequestObject(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID PowerRequestObject,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount)
{
    return;
}

NTSTATUS
NTAPI
PopRegisterPowerRequest(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ POP_POWER_REQUEST_INQUIRE_TYPE Request,
    _In_ BOOLEAN ComingFromuserMode,
    _In_ BOOLEAN NewRegister,
    _In_ EXECUTION_STATE EsFlags,
    _In_opt_ PCOUNTED_REASON_CONTEXT Context,
    _Inout_opt_ PPOP_POWER_REQUEST *PowerRequestHandle)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* The caller should submit a valid request otherwise  */
    ASSERT((Request >= RegisterLegacyRequest) && (Request <= RegisterALaVistaRequest));

    /* Forward the request to the appropriate helper */
    switch (Request)
    {
        case RegisterLegacyRequest:
        {
            /*
             * The caller invoked a legacy request. This typically means a call of
             * PoRegisterSystemState was invoked, therefore we must relay the management
             * of this request to the legacy helper.
             */
            Status = PopCreatePowerRequestAsLegacy(EsFlags,
                                                   NewRegister,
                                                   PowerRequestHandle);
            break;
        }

        case RegisterALaVistaRequest:
        {
            /*
             * This is a Vista+ request. This typically means a call of
             * PoCreatePowerRequest was invoked.
             */
            break;
        }

        /* The kernel SHOULD NOT BE REACHING HERE */
        DEFAULT_UNREACHABLE;
    }

    return Status;
}

/* EOF */

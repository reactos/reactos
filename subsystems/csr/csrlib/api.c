/*
 * PROJECT:     ReactOS Client/Server Runtime SubSystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CSR Client Library - API LPC Implementation
 * COPYRIGHT:   Copyright 2005-2012 Alex Ionescu <alex@relsoft.net>
 *              Copyright 2012-2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "csrlib.h"

#define NTOS_MODE_USER
#include <ndk/psfuncs.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrNewThread(VOID)
{
    /* Register the termination port to CSR's */
    return NtRegisterThreadTerminatePort(CsrApiPort);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrIdentifyAlertableThread(VOID)
{
#if (NTDDI_VERSION < NTDDI_WS03)
    NTSTATUS Status;
    CSR_API_MESSAGE ApiMessage;
    PCSR_IDENTIFY_ALERTABLE_THREAD IdentifyAlertableThread;

    /* Set up the data for CSR */
    IdentifyAlertableThread = &ApiMessage.Data.IdentifyAlertableThread;
    IdentifyAlertableThread->Cid = NtCurrentTeb()->ClientId;

    /* Call it */
    Status = CsrClientCallServer(&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSRSRV_SERVERDLL_INDEX, CsrpIdentifyAlertableThread),
                                 sizeof(*IdentifyAlertableThread));

    /* Return to caller */
    return Status;
#else
    /* Deprecated */
    return STATUS_SUCCESS;
#endif
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrSetPriorityClass(
    _In_ HANDLE Process,
    _Inout_ PULONG PriorityClass)
{
#if (NTDDI_VERSION < NTDDI_WS03)
    NTSTATUS Status;
    CSR_API_MESSAGE ApiMessage;
    PCSR_SET_PRIORITY_CLASS SetPriorityClass = &ApiMessage.Data.SetPriorityClass;

    /* Set up the data for CSR */
    SetPriorityClass->hProcess = Process;
    SetPriorityClass->PriorityClass = *PriorityClass;

    /* Call it */
    Status = CsrClientCallServer(&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSRSRV_SERVERDLL_INDEX, CsrpSetPriorityClass),
                                 sizeof(*SetPriorityClass));

    /* Return what we got, if requested */
    if (*PriorityClass) *PriorityClass = SetPriorityClass->PriorityClass;

    /* Return to caller */
    return Status;
#else
    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(PriorityClass);

    /* Deprecated */
    return STATUS_INVALID_PARAMETER;
#endif
}

/* EOF */

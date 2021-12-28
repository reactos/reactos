/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            dll/ntdll/csr/api.c
 * PURPOSE:         CSR APIs exported through NTDLL
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *******************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern HANDLE CsrApiPort;

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
CsrSetPriorityClass(IN HANDLE Process,
                    IN OUT PULONG PriorityClass)
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

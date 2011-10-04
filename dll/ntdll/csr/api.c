/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/api.c
 * PURPOSE:         CSR APIs exported through NTDLL
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/
extern HANDLE CsrApiPort;

/* FUNCTIONS *****************************************************************/

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
CsrSetPriorityClass(HANDLE hProcess,
                    PULONG PriorityClass)
{
    NTSTATUS Status;
    CSR_API_MESSAGE2 ApiMessage; /* <- Remove the "2" when CSR is commited */
    PCSR_SET_PRIORITY_CLASS SetPriorityClass = &ApiMessage.SetPriorityClass;

    /* Set up the data for CSR */
    DbgBreakPoint();
    SetPriorityClass->hProcess = hProcess;
    SetPriorityClass->PriorityClass = *PriorityClass;

    /* Call it */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_MAKE_OPCODE(CsrSrvSetPriorityClass,
                                                 CSR_SRV_SERVER),
                                 sizeof(CSR_SET_PRIORITY_CLASS));
    
    /* Return what we got, if requested */
    if (*PriorityClass) *PriorityClass = SetPriorityClass->PriorityClass;

    /* Return to caller */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrIdentifyAlertableThread (VOID)
{
    NTSTATUS Status;
    CSR_API_MESSAGE2 ApiMessage; /* <- Remove the "2" when CSR is commited */
    PCSR_IDENTIFY_ALTERTABLE_THREAD IdentifyAlertableThread;
    
    /* Set up the data for CSR */
    DbgBreakPoint();
    IdentifyAlertableThread = &ApiMessage.IdentifyAlertableThread;
    IdentifyAlertableThread->Cid = NtCurrentTeb()->ClientId;

    /* Call it */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_MAKE_OPCODE(CsrSrvIdentifyAlertableThread,
                                                 CSR_SRV_SERVER),
                                 sizeof(CSR_SET_PRIORITY_CLASS));

    /* Return to caller */
    return Status;
}

/* EOF */

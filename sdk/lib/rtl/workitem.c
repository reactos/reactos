/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Work Item implementation
 * FILE:              lib/rtl/workitem.c
 * PROGRAMMER:
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS
NTAPI
RtlpStartThread(IN PTHREAD_START_ROUTINE Function,
                IN PVOID Parameter,
                OUT PHANDLE ThreadHandle)
{
    /* Create a native worker thread -- used for SMSS, CSRSS, etc... */
    return RtlCreateUserThread(NtCurrentProcess(),
                               NULL,
                               TRUE,
                               0,
                               0,
                               0,
                               Function,
                               Parameter,
                               ThreadHandle,
                               NULL);
}

NTSTATUS
NTAPI
RtlpExitThread(IN NTSTATUS ExitStatus)
{
    /* Kill a native worker thread -- used for SMSS, CSRSS, etc... */
    return NtTerminateThread(NtCurrentThread(), ExitStatus);
}

PRTL_START_POOL_THREAD RtlpStartThreadFunc = RtlpStartThread;
PRTL_EXIT_POOL_THREAD RtlpExitThreadFunc = RtlpExitThread;

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetThreadPoolStartFunc(IN PRTL_START_POOL_THREAD StartPoolThread,
                          IN PRTL_EXIT_POOL_THREAD ExitPoolThread)
{
    RtlpStartThreadFunc = StartPoolThread;
    RtlpExitThreadFunc = ExitPoolThread;
    return STATUS_SUCCESS;
}

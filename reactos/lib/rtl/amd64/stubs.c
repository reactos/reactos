/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           AMD64 stubs
 * FILE:              lib/rtl/amd64/stubs.c
 * PROGRAMMERS:        Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
VOID
NTAPI
RtlInitializeContext(IN HANDLE ProcessHandle,
                     OUT PCONTEXT ThreadContext,
                     IN PVOID ThreadStartParam  OPTIONAL,
                     IN PTHREAD_START_ROUTINE ThreadStartAddress,
                     IN PINITIAL_TEB InitialTeb)
{
    UNIMPLEMENTED;
    return;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlpGetExceptionAddress(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                     IN PCONTEXT Context)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSYSAPI
VOID
RtlRestoreContext(
    PCONTEXT ContextRecord,
    PEXCEPTION_RECORD ExceptionRecord)
{
    UNIMPLEMENTED;
}

NTSYSAPI
BOOLEAN
RtlInstallFunctionTableCallback(
    DWORD64 TableIdentifier,
    DWORD64 BaseAddress,
    DWORD Length,
    PGET_RUNTIME_FUNCTION_CALLBACK Callback,
    PVOID Context,
    PCWSTR OutOfProcessCallbackDll)
{
    UNIMPLREMENTED;
    return FALSE;
}


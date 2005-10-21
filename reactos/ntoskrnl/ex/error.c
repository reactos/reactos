/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/error.c
 * PURPOSE:         Error Functions and Status/Exception Dispatching/Raising
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Created File
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

BOOLEAN ExReadyForErrors = FALSE;
PEPORT ExpDefaultErrorPort = NULL;
PEPROCESS ExpDefaultErrorPortProcess = NULL;

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
ExRaiseAccessViolation(VOID)
{
    /* Raise the Right Status */
    RtlRaiseStatus(STATUS_ACCESS_VIOLATION);
}

/*
 * @implemented
 */
VOID
STDCALL
ExRaiseDatatypeMisalignment (VOID)
{
    /* Raise the Right Status */
    RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
}

/*
 * @implemented
 */
LONG
STDCALL
ExSystemExceptionFilter(VOID)
{
    return KeGetPreviousMode() != KernelMode ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ExRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    DPRINT1("Hard error %x\n", ErrorStatus);

    /* Call the Executive Function (WE SHOULD PUT SEH HERE/CAPTURE!) */
    ExRaiseHardError(ErrorStatus,
                     NumberOfParameters,
                     UnicodeStringParameterMask,
                     Parameters,
                     ValidResponseOptions,
                     Response);

    /* Return Success */
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
NtSetDefaultHardErrorPort(IN HANDLE PortHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Check if we have the Privilege */
    if(!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode)) {

        DPRINT1("NtSetDefaultHardErrorPort: Caller requires the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Only called once during bootup, make sure we weren't called yet */
    if(!ExReadyForErrors) {

        Status = ObReferenceObjectByHandle(PortHandle,
                                           0,
                                           LpcPortObjectType,
                                           PreviousMode,
                                           (PVOID*)&ExpDefaultErrorPort,
                                           NULL);

        /* Check for Success */
        if(NT_SUCCESS(Status)) {

            /* Save the data */
            ExpDefaultErrorPortProcess = PsGetCurrentProcess();
            ExReadyForErrors = TRUE;
        }
    }

    return Status;
}

VOID
__cdecl
_purecall(VOID)
{
    /* Not supported in Kernel Mode */
    RtlRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

/* EOF */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/error.c
 * PURPOSE:         Error Functions and Status/Exception Dispatching/Raising
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define TAG_ERR TAG('E', 'r', 'r', ' ')

/* GLOBALS ****************************************************************/

BOOLEAN ExReadyForErrors = FALSE;
PEPORT ExpDefaultErrorPort = NULL;
PEPROCESS ExpDefaultErrorPortProcess = NULL;

/* FUNCTIONS ****************************************************************/

VOID
NTAPI
ExpRaiseHardError(IN NTSTATUS ErrorStatus,
                  IN ULONG NumberOfParameters,
                  IN ULONG UnicodeStringParameterMask,
                  IN PULONG_PTR Parameters,
                  IN ULONG ValidResponseOptions,
                  OUT PULONG Response)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
ExRaiseAccessViolation(VOID)
{
    /* Raise the Right Status */
    RtlRaiseStatus(STATUS_ACCESS_VIOLATION);
}

/*
 * @implemented
 */
VOID
NTAPI
ExRaiseDatatypeMisalignment (VOID)
{
    /* Raise the Right Status */
    RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
}

/*
 * @implemented
 */
LONG
NTAPI
ExSystemExceptionFilter(VOID)
{
    return KeGetPreviousMode() != KernelMode ?
           EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/*
 * @unimplemented
 */
VOID
NTAPI
ExRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    /* FIXME: Capture to user-mode! */

    /* Now call the worker function */
    ExpRaiseHardError(ErrorStatus,
                      NumberOfParameters,
                      UnicodeStringParameterMask,
                      Parameters,
                      ValidResponseOptions,
                      Response);
}

NTSTATUS
NTAPI
NtRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    NTSTATUS Status;
    PULONG_PTR SafeParams = NULL;
    ULONG SafeResponse;
    UNICODE_STRING SafeString;
    ULONG i;
    ULONG ParamSize;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    DPRINT1("Hard error %x\n", ErrorStatus);

    /* Validate parameter count */
    if (NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS)
    {
        /* Fail */
        DPRINT1("Invalid parameters\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Make sure we have some at least */
    if ((Parameters) && !(NumberOfParameters))
    {
        /* Fail */
        DPRINT1("Invalid parameters\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Check if we were called from user-mode */
    if (PreviousMode != KernelMode)
    {
        /* First validate the responses */
        switch (ValidResponseOptions)
        {
            /* Check all valid cases */
            case OptionAbortRetryIgnore:
            case OptionOk:
            case OptionOkCancel:
            case OptionRetryCancel:
            case OptionYesNo:
            case OptionYesNoCancel:
            case OptionShutdownSystem:
                break;

            /* Anything else is invalid */
            default:
                return STATUS_INVALID_PARAMETER_4;
        }

        /* Enter SEH Block */
        _SEH_TRY
        {
            /* Validate the response pointer */
            ProbeForWriteUlong(Response);

            /* Check if we have parameters */
            if (Parameters)
            {
                /* Validate the parameter pointers */
                ParamSize = sizeof(ULONG_PTR) * NumberOfParameters;
                ProbeForRead(Parameters, ParamSize, sizeof(ULONG_PTR));

                /* Allocate a safe buffer */
                SafeParams = ExAllocatePoolWithTag(PagedPool,
                                                   ParamSize,
                                                   TAG_ERR);

                /* Copy them */
                RtlMoveMemory(SafeParams, Parameters, ParamSize);

                /* Nowo check if there's strings in it */
                if (UnicodeStringParameterMask)
                {
                    /* Loop every string */
                    for (i = 0; i < NumberOfParameters; i++)
                    {
                        /* Check if this parameter is a string */
                        if (UnicodeStringParameterMask & (1 << i))
                        {
                            /* Probe the structure */
                            ProbeForRead((PVOID)SafeParams[i],
                                         sizeof(UNICODE_STRING),
                                         sizeof(ULONG_PTR));

                            /* Capture it */
                            RtlMoveMemory(&SafeString,
                                          (PVOID)SafeParams[i],
                                          sizeof(UNICODE_STRING));

                            /* Probe the buffer */
                            ProbeForRead(SafeString.Buffer,
                                         SafeString.MaximumLength,
                                         sizeof(UCHAR));
                        }
                    }
                }
            }
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            /* Free captured buffer */
            if (SafeParams) ExFreePool(SafeParams);
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* If we failed to capture/probe, bail out */
        if (!NT_SUCCESS(Status)) return Status;

        /* Call the system function directly, because we probed */
        ExpRaiseHardError(ErrorStatus,
                          NumberOfParameters,
                          UnicodeStringParameterMask,
                          SafeParams,
                          ValidResponseOptions,
                          &SafeResponse);
    }
    else
    {
        /* Reuse variable */
        SafeParams = Parameters;

        /*
         * Call the Executive Function. It will probe and copy pointers to 
         * user-mode
         */
        ExRaiseHardError(ErrorStatus,
                         NumberOfParameters,
                         UnicodeStringParameterMask,
                         SafeParams,
                         ValidResponseOptions,
                         &SafeResponse);
    }

    /* Check if we were called in user-mode */
    if (PreviousMode != KernelMode)
    {
        /* That means we have a buffer to free */
        if (SafeParams) ExFreePool(SafeParams);

        /* Enter SEH Block for return */
        _SEH_TRY
        {
            /* Return the response */
            *Response = SafeResponse;
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    else
    {
        /* Return the response */
        *Response = SafeResponse;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtSetDefaultHardErrorPort(IN HANDLE PortHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Check if we have the Privilege */
    if(!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
    {
        DPRINT1("NtSetDefaultHardErrorPort: Caller requires "
                "the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Only called once during bootup, make sure we weren't called yet */
    if(!ExReadyForErrors)
    {
        /* Reference the port */
        Status = ObReferenceObjectByHandle(PortHandle,
                                           0,
                                           LpcPortObjectType,
                                           PreviousMode,
                                           (PVOID*)&ExpDefaultErrorPort,
                                           NULL);

        /* Check for Success */
        if(NT_SUCCESS(Status))
        {
            /* Save the data */
            ExpDefaultErrorPortProcess = PsGetCurrentProcess();
            ExReadyForErrors = TRUE;
        }
    }

    /* Return status to caller */
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

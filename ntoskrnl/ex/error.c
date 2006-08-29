/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
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

/*++
 * @name ExpRaiseHardError
 *
 * For now it's a stub
 *
 * @param ErrorStatus
 *        FILLME
 *
 * @param NumberOfParameters
 *        FILLME
 *
 * @param UnicodeStringParameterMask
 *        FILLME
 *
 * @param Parameters
 *        FILLME
 *
 * @param ValidResponseOptions
 *        FILLME
 *
 * @param Response
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
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

/*++
 * @name ExRaiseAccessViolation
 * @implemented
 *
 * The ExRaiseAccessViolation routine can be used with structured exception
 * handling to throw a driver-determined exception for a memory access
 * violation that occurs when a driver processes I/O requests.
 * See: http://msdn.microsoft.com/library/en-us/Kernel_r/hh/Kernel_r/k102_71b4c053-599c-4a6d-8a59-08aae6bdc534.xml.asp?frame=true
 *      http://www.osronline.com/ddkx/kmarch/k102_814i.htm
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
ExRaiseAccessViolation(VOID)
{
    /* Raise the Right Status */
    RtlRaiseStatus(STATUS_ACCESS_VIOLATION);
}

/*++
 * @name ExRaiseDatatypeMisalignment
 * @implemented
 *
 * ExRaiseDatatypeMisalignment raises an exception with the exception
 * code set to STATUS_DATATYPE_MISALIGNMENT
 * See: MSDN / DDK
 *      http://www.osronline.com/ddkx/kmarch/k102_814i.htm
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
ExRaiseDatatypeMisalignment(VOID)
{
    /* Raise the Right Status */
    RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
}

/*++
 * @name ExSystemExceptionFilter
 * @implemented
 *
 * TODO: Add description
 *
 * @return FILLME
 *
 * @remarks None
 *
 *--*/
LONG
NTAPI
ExSystemExceptionFilter(VOID)
{
    return KeGetPreviousMode() != KernelMode ?
           EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/*++
 * @name ExRaiseHardError
 * @implemented
 *
 * See NtRaiseHardError
 *
 * @param ErrorStatus
 *        Error Code
 *
 * @param NumberOfParameters
 *        Number of optional parameters in Parameters array
 *
 * @param UnicodeStringParameterMask
 *        Optional string parameter (can be only one per error code)
 *
 * @param Parameters
 *        Array of DWORD parameters for use in error message string
 *
 * @param ValidResponseOptions
 *        See HARDERROR_RESPONSE_OPTION for possible values description
 *
 * @param Response
 *        Pointer to HARDERROR_RESPONSE enumeration
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
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

/*++
 * @name NtRaiseHardError
 * @implemented
 *
 * This function sends HARDERROR_MSG LPC message to listener
 * (typically CSRSS.EXE). See NtSetDefaultHardErrorPort for more information
 * See: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Error/NtRaiseHardError.html
 *
 * @param ErrorStatus
 *        Error Code
 *
 * @param NumberOfParameters
 *        Number of optional parameters in Parameters array
 *
 * @param UnicodeStringParameterMask
 *        Optional string parameter (can be only one per error code)
 *
 * @param Parameters
 *        Array of DWORD parameters for use in error message string
 *
 * @param ValidResponseOptions
 *        See HARDERROR_RESPONSE_OPTION for possible values description
 *
 * @param Response
 *        Pointer to HARDERROR_RESPONSE enumeration
 *
 * @return Status
 *
 * @remarks NtRaiseHardError is easy way to display message in GUI
 *          without loading Win32 API libraries
 *
 *--*/
NTSTATUS
NTAPI
NtRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    NTSTATUS Status = STATUS_SUCCESS;
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

/*++
 * @name NtSetDefaultHardErrorPort
 * @implemented
 *
 * NtSetDefaultHardErrorPort is typically called only once. After call,
 * kernel set BOOLEAN flag named _ExReadyForErrors to TRUE, and all other
 * tries to change default port are broken with STATUS_UNSUCCESSFUL error code
 * See: http://www.windowsitlibrary.com/Content/356/08/2.html
 *      http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Error/NtSetDefaultHardErrorPort.html
 *
 * @param PortHandle
 *        Handle to named port object
 *
 * @return Status
 *
 * @remarks Privileges: SE_TCB_PRIVILEGE
 *
 *--*/
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

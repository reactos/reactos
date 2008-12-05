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
#include <debug.h>

#define TAG_ERR TAG('E', 'r', 'r', ' ')

/* GLOBALS ****************************************************************/

BOOLEAN ExReadyForErrors = FALSE;
PVOID ExpDefaultErrorPort = NULL;
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
NTSTATUS
NTAPI
ExpSystemErrorHandler(IN NTSTATUS ErrorStatus,
                      IN ULONG NumberOfParameters,
                      IN ULONG UnicodeStringParameterMask,
                      IN PULONG_PTR Parameters,
                      IN BOOLEAN Shutdown)
{
    /* FIXME: STUB */
    KeBugCheckEx(FATAL_UNHANDLED_HARD_ERROR,
                 ErrorStatus,
                 0,
                 0,
                 0);
    return STATUS_SUCCESS;
}

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
NTSTATUS
NTAPI
ExpRaiseHardError(IN NTSTATUS ErrorStatus,
                  IN ULONG NumberOfParameters,
                  IN ULONG UnicodeStringParameterMask,
                  IN PULONG_PTR Parameters,
                  IN ULONG ValidResponseOptions,
                  OUT PULONG Response)
{
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    UCHAR Buffer[PORT_MAXIMUM_MESSAGE_LENGTH];
    PHARDERROR_MSG Message = (PHARDERROR_MSG)Buffer;
    NTSTATUS Status;
    HANDLE PortHandle;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PAGED_CODE();

    /* Check if this error will shutdown the system */
    if (ValidResponseOptions == OptionShutdownSystem)
    {
        /* Check for privilege */
        if (!SeSinglePrivilegeCheck(SeShutdownPrivilege, PreviousMode))
        {
            /* No rights */
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        /* Don't handle any new hard errors */
        ExReadyForErrors = FALSE;
    }

    /* Check if hard errors are not disabled */
    if (!Thread->HardErrorsAreDisabled)
    {
        /* Check if we can't do errors anymore, and this is serious */
        if ((!ExReadyForErrors) && (NT_ERROR(ErrorStatus)))
        {
            /* Use the system handler */
            ExpSystemErrorHandler(ErrorStatus,
                                  NumberOfParameters,
                                  UnicodeStringParameterMask,
                                  Parameters,
                                  (PreviousMode != KernelMode) ? TRUE: FALSE);
        }
    }

    /* Check if we have an exception port */
    if (Process->ExceptionPort)
    {
        /* Check if hard errors should be processed */
        if (Process->DefaultHardErrorProcessing & 1)
        {
            /* Use the port */
            PortHandle = Process->ExceptionPort;
        }
        else
        {
            /* It's disabled, check if the error overrides it */
            if (ErrorStatus & 0x10000000)
            {
                /* Use the port anyway */
                PortHandle = Process->ExceptionPort;
            }
            else
            {
                /* No such luck */
                PortHandle = NULL;
            }
        }
    }
    else
    {
        /* Check if hard errors are enabled */
        if (Process->DefaultHardErrorProcessing & 1)
        {
            /* Use our default system port */
            PortHandle = ExpDefaultErrorPort;
        }
        else
        {
            /* It's disabled, check if the error overrides it */
            if (ErrorStatus & 0x10000000)
            {
                /* Use the port anyway */
                PortHandle = ExpDefaultErrorPort;
            }
            else
            {
                /* No such luck */
                PortHandle = NULL;
            }
        }
    }

    /* If hard errors are disabled, do nothing */
    if (Thread->HardErrorsAreDisabled) PortHandle = NULL;

    /* Now check if we have a port */
    if (PortHandle)
    {
        /* Check if this is the default process */
        if (Process == ExpDefaultErrorPortProcess)
        {
            /* We can't handle the error, check if this is critical */
            if (NT_ERROR(ErrorStatus))
            {
                /* It is, invoke the system handler */
                ExpSystemErrorHandler(ErrorStatus,
                                      NumberOfParameters,
                                      UnicodeStringParameterMask,
                                      Parameters,
                                      (PreviousMode != KernelMode) ? TRUE: FALSE);

                /* If we survived, return to caller */
                *Response = ResponseReturnToCaller;
                return STATUS_SUCCESS;
            }
        }

        /* Setup the LPC Message */
        Message->h.u1.Length = (sizeof(HARDERROR_MSG) << 16) |
                               (sizeof(HARDERROR_MSG) - sizeof(PORT_MESSAGE));
        Message->h.u2.ZeroInit = LPC_ERROR_EVENT;
        Message->Status = ErrorStatus &~ 0x10000000;
        Message->ValidResponseOptions = ValidResponseOptions;
        Message->UnicodeStringParameterMask = UnicodeStringParameterMask;
        Message->NumberOfParameters = NumberOfParameters;
        KeQuerySystemTime(&Message->ErrorTime);

        /* Copy the parameters */
        if (Parameters) RtlMoveMemory(&Message->Parameters,
                                      Parameters,
                                      sizeof(ULONG_PTR) * NumberOfParameters);

        /* Send the LPC Message */
        Status = LpcRequestWaitReplyPort(PortHandle,
                                         (PVOID)Message,
                                         (PVOID)Message);
        if (NT_SUCCESS(Status))
        {
            /* Check what kind of response we got */
            if ((Message->Response != ResponseReturnToCaller) &&
                (Message->Response != ResponseNotHandled) &&
                (Message->Response != ResponseAbort) &&
                (Message->Response != ResponseCancel) &&
                (Message->Response != ResponseIgnore) &&
                (Message->Response != ResponseNo) &&
                (Message->Response != ResponseOk) &&
                (Message->Response != ResponseRetry) &&
                (Message->Response != ResponseYes) &&
                (Message->Response != ResponseTryAgain) &&
                (Message->Response != ResponseContinue))
            {
                /* Reset to a default one */
                Message->Response = ResponseReturnToCaller;
            }

            /* Set the response */
            *Response = Message->Response;
        }
    }
    else
    {
        /* Set defaults */
        *Response = ResponseReturnToCaller;
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
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
 *        Array of ULONG parameters for use in error message string
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
NTSTATUS
NTAPI
ExRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    SIZE_T Size;
    UNICODE_STRING CapturedParams[MAXIMUM_HARDERROR_PARAMETERS];
    ULONG i;
    PULONG_PTR UserData = NULL, ParameterBase;
    PUNICODE_STRING StringBase;
    PWSTR BufferBase;
    ULONG SafeResponse;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we have parameters */
    if (Parameters)
    {
        /* Check if we have strings */
        if (UnicodeStringParameterMask)
        {
            /* Add the maximum possible size */
            Size = (sizeof(ULONG_PTR) + sizeof(UNICODE_STRING)) *
                    MAXIMUM_HARDERROR_PARAMETERS + sizeof(UNICODE_STRING);

            /* Loop each parameter */
            for (i = 0; i < NumberOfParameters; i++)
            {
                /* Check if it's part of the mask */
                if (UnicodeStringParameterMask & (1 << i))
                {
                    /* Copy it */
                    RtlMoveMemory(&CapturedParams[i],
                                  &Parameters[i],
                                  sizeof(UNICODE_STRING));

                    /* Increase the size */
                    Size += CapturedParams[i].MaximumLength;
                }
            }

            /* Allocate the user data region */
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             (PVOID*)&UserData,
                                             0,
                                             &Size,
                                             MEM_COMMIT,
                                             PAGE_READWRITE);
            if (!NT_SUCCESS(Status)) return Status;

            /* Set the pointers to our various data */
            ParameterBase = UserData;
            StringBase = (PVOID)((ULONG_PTR)UserData +
                                 sizeof(ULONG_PTR) *
                                 MAXIMUM_HARDERROR_PARAMETERS);
            BufferBase = (PVOID)((ULONG_PTR)StringBase +
                                 sizeof(UNICODE_STRING) *
                                 MAXIMUM_HARDERROR_PARAMETERS);

            /* Loop parameters again */
            for (i = 0; i < NumberOfParameters; i++)
            {
                /* Check if we're in the mask */
                if (UnicodeStringParameterMask & (1 << i))
                {
                    /* Update the base */
                    ParameterBase[i] = (ULONG_PTR)&StringBase[i];

                    /* Copy the string buffer */
                    RtlMoveMemory(BufferBase,
                                  CapturedParams[i].Buffer,
                                  CapturedParams[i].MaximumLength);

                    /* Set buffer */
                    CapturedParams[i].Buffer = BufferBase;

                    /* Copy the string structure */
                    RtlMoveMemory(&StringBase[i],
                                  &CapturedParams[i],
                                  sizeof(UNICODE_STRING));

                    /* Update the pointer */
                    BufferBase += CapturedParams[i].MaximumLength;
                }
                else
                {
                    /* No need to copy any strings */
                    ParameterBase[i] = Parameters[i];
                }
            }
        }
        else
        {
            /* Just keep the data as is */
            UserData = Parameters;
        }
    }

    /* Now call the worker function */
    Status = ExpRaiseHardError(ErrorStatus,
                               NumberOfParameters,
                               UnicodeStringParameterMask,
                               UserData,
                               ValidResponseOptions,
                               &SafeResponse);

    /* Check if we had done user-mode allocation */
    if ((UserData) && (UserData != Parameters))
    {
        /* We did! Delete it */
        Size = 0;
        ZwFreeVirtualMemory(NtCurrentProcess(),
                            (PVOID*)&UserData,
                            &Size,
                            MEM_RELEASE);
    }

    /* Return status and the response */
    *Response = SafeResponse;
    return Status;
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
 *        Array of ULONG parameters for use in error message string
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

    /* Validate parameter count */
    if (NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Make sure we have some at least */
    if ((Parameters) && !(NumberOfParameters))
    {
        /* Fail */
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
        _SEH2_TRY
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
                RtlCopyMemory(SafeParams, Parameters, ParamSize);

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
                            RtlCopyMemory(&SafeString,
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
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Free captured buffer */
            if (SafeParams) ExFreePool(SafeParams);
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

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
        _SEH2_TRY
        {
            /* Return the response */
            *Response = SafeResponse;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
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
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
    {
        DPRINT1("NtSetDefaultHardErrorPort: Caller requires "
                "the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Only called once during bootup, make sure we weren't called yet */
    if (!ExReadyForErrors)
    {
        /* Reference the port */
        Status = ObReferenceObjectByHandle(PortHandle,
                                           0,
                                           LpcPortObjectType,
                                           PreviousMode,
                                           (PVOID*)&ExpDefaultErrorPort,
                                           NULL);
        if (NT_SUCCESS(Status))
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

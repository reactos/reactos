/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/harderr.c
 * PURPOSE:         Error Functions and Status/Exception Dispatching/Raising
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

BOOLEAN ExReadyForErrors = FALSE;
PVOID ExpDefaultErrorPort = NULL;
PEPROCESS ExpDefaultErrorPortProcess = NULL;

/* FUNCTIONS ****************************************************************/

/*++
* @name ExpSystemErrorHandler
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
    ULONG_PTR BugCheckParameters[MAXIMUM_HARDERROR_PARAMETERS] = {0, 0, 0, 0};
    ULONG i;

    /* Sanity check */
    ASSERT(NumberOfParameters <= MAXIMUM_HARDERROR_PARAMETERS);

    /*
     * KeBugCheck expects MAXIMUM_HARDERROR_PARAMETERS parameters,
     * but we might get called with less, so use a local buffer here.
     */
    for (i = 0; i < NumberOfParameters; i++)
    {
        /* Copy them over */
        BugCheckParameters[i] = Parameters[i];
    }

    /* FIXME: STUB */
    KeBugCheckEx(FATAL_UNHANDLED_HARD_ERROR,
                 ErrorStatus,
                 (ULONG_PTR)BugCheckParameters,
                 0,
                 0);
    return STATUS_SUCCESS;
}

/*++
 * @name ExpRaiseHardError
 * @implemented
 *
 * See ExRaiseHardError and NtRaiseHardError, same parameters.
 *
 * This function performs the central work for both ExRaiseHardError
 * and NtRaiseHardError. ExRaiseHardError is the service for kernel-mode
 * that copies the parameters to user-mode, and NtRaiseHardError is the
 * service for both kernel-mode and user-mode that performs parameters
 * validation and capture if necessary.
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
    NTSTATUS Status;
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    UCHAR Buffer[PORT_MAXIMUM_MESSAGE_LENGTH];
    PHARDERROR_MSG Message = (PHARDERROR_MSG)Buffer;
    HANDLE PortHandle;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();

    PAGED_CODE();

    /* Check if this error will shutdown the system */
    if (ValidResponseOptions == OptionShutdownSystem)
    {
        /*
         * Check if we have the privileges.
         *
         * NOTE: In addition to the Shutdown privilege we also check whether
         * the caller has the Tcb privilege. The purpose is to allow only
         * SYSTEM processes to "shutdown" the system on hard errors (BSOD)
         * while forbidding regular processes to do so. This behaviour differs
         * from Windows, where any user-mode process, as soon as it has the
         * Shutdown privilege, can trigger a hard-error BSOD.
         */
        if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode) ||
            !SeSinglePrivilegeCheck(SeShutdownPrivilege, PreviousMode))
        {
            /* No rights */
            *Response = ResponseNotHandled;
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        /* Don't handle any new hard errors */
        ExReadyForErrors = FALSE;
    }

    /* Check if hard errors are not disabled */
    if (!Thread->HardErrorsAreDisabled)
    {
        /* Check if we can't do errors anymore, and this is serious */
        if (!ExReadyForErrors && NT_ERROR(ErrorStatus))
        {
            /* Use the system handler */
            ExpSystemErrorHandler(ErrorStatus,
                                  NumberOfParameters,
                                  UnicodeStringParameterMask,
                                  Parameters,
                                  (PreviousMode != KernelMode) ? TRUE : FALSE);
        }
    }

    /*
     * Enable hard error processing if it is enabled for the process
     * or if the exception status forces it.
     */
    if ((Process->DefaultHardErrorProcessing & SEM_FAILCRITICALERRORS) ||
        (ErrorStatus & HARDERROR_OVERRIDE_ERRORMODE))
    {
        /* Check if we have an exception port */
        if (Process->ExceptionPort)
        {
            /* Use the port */
            PortHandle = Process->ExceptionPort;
        }
        else
        {
            /* Use our default system port */
            PortHandle = ExpDefaultErrorPort;
        }
    }
    else
    {
        /* Don't process the error */
        PortHandle = NULL;
    }

    /* If hard errors are disabled, do nothing */
    if (Thread->HardErrorsAreDisabled) PortHandle = NULL;

    /*
     * If this is not the system thread, check whether hard errors are
     * disabled for this thread on user-mode side, and if so, do nothing.
     */
    if (!Thread->SystemThread && (PortHandle != NULL))
    {
        /* Check if we have a TEB */
        PTEB Teb = PsGetCurrentThread()->Tcb.Teb;
        if (Teb)
        {
            _SEH2_TRY
            {
                if (Teb->HardErrorMode & RTL_SEM_FAILCRITICALERRORS)
                {
                    PortHandle = NULL;
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING;
            }
            _SEH2_END;
        }
    }

    /* Now check if we have a port */
    if (PortHandle == NULL)
    {
        /* Just return to caller */
        *Response = ResponseReturnToCaller;
        return STATUS_SUCCESS;
    }

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
                                  (PreviousMode != KernelMode) ? TRUE : FALSE);

            /* If we survived, return to caller */
            *Response = ResponseReturnToCaller;
            return STATUS_SUCCESS;
        }
    }

    /* Setup the LPC Message */
    Message->h.u1.Length = (sizeof(HARDERROR_MSG) << 16) |
                           (sizeof(HARDERROR_MSG) - sizeof(PORT_MESSAGE));
    Message->h.u2.ZeroInit = 0;
    Message->h.u2.s2.Type = LPC_ERROR_EVENT;
    Message->Status = ErrorStatus & ~HARDERROR_OVERRIDE_ERRORMODE;
    Message->ValidResponseOptions = ValidResponseOptions;
    Message->UnicodeStringParameterMask = UnicodeStringParameterMask;
    Message->NumberOfParameters = NumberOfParameters;
    KeQuerySystemTime(&Message->ErrorTime);

    /* Copy the parameters */
    if (Parameters)
    {
        RtlMoveMemory(&Message->Parameters,
                      Parameters,
                      sizeof(ULONG_PTR) * NumberOfParameters);
    }

    /* Send the LPC Message */
    Status = LpcRequestWaitReplyPort(PortHandle,
                                     (PPORT_MESSAGE)Message,
                                     (PPORT_MESSAGE)Message);
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
    else
    {
        /* Set the response */
        *Response = ResponseReturnToCaller;
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
 * See NtRaiseHardError and ExpRaiseHardError.
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
    NTSTATUS Status;
    SIZE_T Size;
    UNICODE_STRING CapturedParams[MAXIMUM_HARDERROR_PARAMETERS];
    ULONG i;
    PVOID UserData = NULL;
    PHARDERROR_USER_PARAMETERS UserParams;
    PWSTR BufferBase;
    ULONG SafeResponse = ResponseNotHandled;

    PAGED_CODE();

    /* Check if we have parameters */
    if (Parameters)
    {
        /* Check if we have strings */
        if (UnicodeStringParameterMask)
        {
            /* Calculate the required size */
            Size = FIELD_OFFSET(HARDERROR_USER_PARAMETERS, Buffer[0]);

            /* Loop each parameter */
            for (i = 0; i < NumberOfParameters; i++)
            {
                /* Check if it's part of the mask */
                if (UnicodeStringParameterMask & (1 << i))
                {
                    /* Copy it */
                    RtlMoveMemory(&CapturedParams[i],
                                  (PVOID)Parameters[i],
                                  sizeof(UNICODE_STRING));

                    /* Increase the size */
                    Size += CapturedParams[i].MaximumLength;
                }
            }

            /* Allocate the user data region */
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             &UserData,
                                             0,
                                             &Size,
                                             MEM_COMMIT,
                                             PAGE_READWRITE);
            if (!NT_SUCCESS(Status))
            {
                /* Return failure */
                *Response = ResponseNotHandled;
                return Status;
            }

            /* Set the pointers to our data */
            UserParams = UserData;
            BufferBase = UserParams->Buffer;

            /* Enter SEH block as we are writing to user-mode space */
            _SEH2_TRY
            {
                /* Loop parameters again */
                for (i = 0; i < NumberOfParameters; i++)
                {
                    /* Check if we are in the mask */
                    if (UnicodeStringParameterMask & (1 << i))
                    {
                        /* Update the base */
                        UserParams->Parameters[i] = (ULONG_PTR)&UserParams->Strings[i];

                        /* Copy the string buffer */
                        RtlMoveMemory(BufferBase,
                                      CapturedParams[i].Buffer,
                                      CapturedParams[i].MaximumLength);

                        /* Set buffer */
                        CapturedParams[i].Buffer = BufferBase;

                        /* Copy the string structure */
                        UserParams->Strings[i] = CapturedParams[i];

                        /* Update the pointer */
                        BufferBase += CapturedParams[i].MaximumLength;
                    }
                    else
                    {
                        /* No need to copy any strings */
                        UserParams->Parameters[i] = Parameters[i];
                    }
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return the exception code */
                Status = _SEH2_GetExceptionCode();
                DPRINT1("ExRaiseHardError - Exception when writing data to user-mode, Status 0x%08lx\n", Status);
            }
            _SEH2_END;
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
                            &UserData,
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
 * This function sends HARDERROR_MSG LPC message to a hard-error listener,
 * typically CSRSS.EXE. See NtSetDefaultHardErrorPort for more information.
 * See also: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Error/NtRaiseHardError.html
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
 *        Array of ULONG_PTR parameters for use in error message string
 *
 * @param ValidResponseOptions
 *        See HARDERROR_RESPONSE_OPTION for possible values description
 *
 * @param Response
 *        Pointer to HARDERROR_RESPONSE enumeration
 *
 * @return Status
 *
 * @remarks NtRaiseHardError constitutes an easy way to display messages
 *          in GUI without loading any Win32 API libraries.
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
    ULONG SafeResponse = ResponseNotHandled;
    UNICODE_STRING SafeString;
    ULONG i;
    ULONG ParamSize = 0;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    PAGED_CODE();

    /* Validate parameter count */
    if (NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Make sure we have some at least */
    if ((Parameters != NULL) && (NumberOfParameters == 0))
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
            case OptionOkNoWait:
            case OptionCancelTryContinue:
                break;

            /* Anything else is invalid */
            default:
                return STATUS_INVALID_PARAMETER_4;
        }

        /* Check if we have parameters */
        if (Parameters)
        {
            /* Calculate size of the parameters */
            ParamSize = sizeof(ULONG_PTR) * NumberOfParameters;

            /* Allocate a safe buffer */
            SafeParams = ExAllocatePoolWithTag(PagedPool, ParamSize, TAG_ERR);
            if (!SafeParams)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
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
                ProbeForRead(Parameters, ParamSize, sizeof(ULONG_PTR));

                /* Copy them */
                RtlCopyMemory(SafeParams, Parameters, ParamSize);

                /* Now check if there's strings in it */
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
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Free captured buffer */
            if (SafeParams) ExFreePoolWithTag(SafeParams, TAG_ERR);

            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        /* Call the system function directly, because we probed */
        Status = ExpRaiseHardError(ErrorStatus,
                                   NumberOfParameters,
                                   UnicodeStringParameterMask,
                                   SafeParams,
                                   ValidResponseOptions,
                                   &SafeResponse);

        /* Free captured buffer */
        if (SafeParams) ExFreePoolWithTag(SafeParams, TAG_ERR);

        /* Enter SEH Block to return the response */
        _SEH2_TRY
        {
            /* Return the response */
            *Response = SafeResponse;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        /* Reuse variable */
        SafeParams = Parameters;

        /*
         * Call the Executive Function. It will probe
         * and copy pointers to user-mode.
         */
        Status = ExRaiseHardError(ErrorStatus,
                                  NumberOfParameters,
                                  UnicodeStringParameterMask,
                                  SafeParams,
                                  ValidResponseOptions,
                                  &SafeResponse);

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
 * NtSetDefaultHardErrorPort is typically called only once. After the call,
 * the kernel sets a BOOLEAN flag named ExReadyForErrors to TRUE, and all other
 * attempts to change the default port fail with STATUS_UNSUCCESSFUL error code.
 * See: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Error/NtSetDefaultHardErrorPort.html
 *      https://web.archive.org/web/20070716133753/http://www.windowsitlibrary.com/Content/356/08/2.html
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

    PAGED_CODE();

    /* Check if we have the privileges */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
    {
        DPRINT1("NtSetDefaultHardErrorPort: Caller requires "
                "the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Only called once during bootup, make sure we weren't called yet */
    if (!ExReadyForErrors)
    {
        /* Reference the hard-error port */
        Status = ObReferenceObjectByHandle(PortHandle,
                                           0,
                                           LpcPortObjectType,
                                           PreviousMode,
                                           (PVOID*)&ExpDefaultErrorPort,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            /* Keep also a reference to the process handling the hard errors */
            ExpDefaultErrorPortProcess = PsGetCurrentProcess();
            ObReferenceObject(ExpDefaultErrorPortProcess);
            ExReadyForErrors = TRUE;
            Status = STATUS_SUCCESS;
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

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            subsystems/win32/csrsrv/server.c
 * PURPOSE:         CSR Server DLL Server Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *******************************************************************/

#include "srv.h"

#include <ndk/mmfuncs.h>

#define NDEBUG
#include <debug.h>

/* DATA ***********************************************************************/

PCSR_SERVER_DLL CsrLoadedServerDll[CSR_SERVER_DLL_MAX];
PVOID CsrSrvSharedSectionHeap = NULL;
PVOID CsrSrvSharedSectionBase = NULL;
PVOID *CsrSrvSharedStaticServerData = NULL;
ULONG CsrSrvSharedSectionSize = 0;
HANDLE CsrSrvSharedSection = NULL;

PCSR_API_ROUTINE CsrServerApiDispatchTable[CsrpMaxApiNumber] =
{
    CsrSrvClientConnect,
    CsrSrvUnusedFunction, // <= WinNT4: CsrSrvThreadConnect
    CsrSrvUnusedFunction, // <= WinNT4: CsrSrvProfileControl
#if (NTDDI_VERSION < NTDDI_WS03)
    CsrSrvIdentifyAlertableThread
    CsrSrvSetPriorityClass
#else
    CsrSrvUnusedFunction, // <= WinXP : CsrSrvIdentifyAlertableThread
    CsrSrvUnusedFunction  // <= WinXP : CsrSrvSetPriorityClass
#endif
};

BOOLEAN CsrServerApiServerValidTable[CsrpMaxApiNumber] =
{
    TRUE,
    FALSE,
    FALSE,
#if (NTDDI_VERSION < NTDDI_WS03)
    TRUE,
    TRUE
#else
    FALSE,
    FALSE
#endif
};

/*
 * On Windows Server 2003, CSR Servers contain
 * the API Names Table only in Debug Builds.
 */
#ifdef CSR_DBG
PCHAR CsrServerApiNameTable[CsrpMaxApiNumber] =
{
    "ClientConnect",
    "ThreadConnect",
    "ProfileControl",
    "IdentifyAlertableThread",
    "SetPriorityClass"
};
#endif

/* PRIVATE FUNCTIONS **********************************************************/

/*++
 * @name CsrServerDllInitialization
 * @implemented NT4
 *
 * The CsrServerDllInitialization is the initialization routine
 * for this Server DLL.
 *
 * @param LoadedServerDll
 *        Pointer to the CSR Server DLL structure representing this Server DLL.
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks None.
 *
 *--*/
CSR_SERVER_DLL_INIT(CsrServerDllInitialization)
{
    /* Setup the DLL Object */
    LoadedServerDll->ApiBase = CSRSRV_FIRST_API_NUMBER;
    LoadedServerDll->HighestApiSupported = CsrpMaxApiNumber;
    LoadedServerDll->DispatchTable = CsrServerApiDispatchTable;
    LoadedServerDll->ValidTable = CsrServerApiServerValidTable;
#ifdef CSR_DBG
    LoadedServerDll->NameTable = CsrServerApiNameTable;
#endif
    LoadedServerDll->SizeOfProcessData = 0;
    LoadedServerDll->ConnectCallback = NULL;
    LoadedServerDll->DisconnectCallback = NULL;

    /* All done */
    return STATUS_SUCCESS;
}

/*++
 * @name CsrLoadServerDll
 * @implemented NT4
 *
 * The CsrLoadServerDll routine loads a CSR Server DLL and calls its entrypoint.
 *
 * @param DllString
 *        Pointer to the CSR Server DLL to load and call.
 *
 * @param EntryPoint
 *        Pointer to the name of the server's initialization function.
 *        If this parameter is NULL, the default ServerDllInitialize
 *        will be assumed.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL otherwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrLoadServerDll(IN PCHAR DllString,
                 IN PCHAR EntryPoint OPTIONAL,
                 IN ULONG ServerId)
{
    NTSTATUS Status;
    ANSI_STRING DllName;
    UNICODE_STRING TempString, ErrorString;
    ULONG_PTR Parameters[2];
    HANDLE hServerDll = NULL;
    ULONG Size;
    PCSR_SERVER_DLL ServerDll;
    STRING EntryPointString;
    PCSR_SERVER_DLL_INIT_CALLBACK ServerDllInitProcedure;
    ULONG Response;

    /* Check if it's beyond the maximum we support */
    if (ServerId >= CSR_SERVER_DLL_MAX) return STATUS_TOO_MANY_NAMES;

    /* Check if it's already been loaded */
    if (CsrLoadedServerDll[ServerId]) return STATUS_INVALID_PARAMETER;

    /* Convert the name to Unicode */
    ASSERT(DllString != NULL);
    RtlInitAnsiString(&DllName, DllString);
    Status = RtlAnsiStringToUnicodeString(&TempString, &DllName, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* If we are loading ourselves, don't actually load us */
    if (ServerId != CSRSRV_SERVERDLL_INDEX)
    {
        /* Load the DLL */
        Status = LdrLoadDll(NULL, 0, &TempString, &hServerDll);
        if (!NT_SUCCESS(Status))
        {
            /* Setup error parameters */
            Parameters[0] = (ULONG_PTR)&TempString;
            Parameters[1] = (ULONG_PTR)&ErrorString;
            RtlInitUnicodeString(&ErrorString, L"Default Load Path");

            /* Send a hard error */
            NtRaiseHardError(Status,
                             2,
                             3,
                             Parameters,
                             OptionOk,
                             &Response);
        }

        /* Get rid of the string */
        RtlFreeUnicodeString(&TempString);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Allocate a CSR DLL Object */
    Size = sizeof(CSR_SERVER_DLL) + DllName.MaximumLength;
    ServerDll = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, Size);
    if (!ServerDll)
    {
        if (hServerDll) LdrUnloadDll(hServerDll);
        return STATUS_NO_MEMORY;
    }

    /* Set up the Object */
    ServerDll->Length = Size;
    ServerDll->SizeOfProcessData = 0;
    ServerDll->SharedSection = CsrSrvSharedSectionHeap; // Send to the server dll our shared heap pointer.
    ServerDll->Name.Length = DllName.Length;
    ServerDll->Name.MaximumLength = DllName.MaximumLength;
    ServerDll->Name.Buffer = (PCHAR)(ServerDll + 1);
    if (DllName.Length)
    {
        strncpy(ServerDll->Name.Buffer, DllName.Buffer, DllName.Length);
    }
    ServerDll->ServerId = ServerId;
    ServerDll->ServerHandle = hServerDll;

    /* Now get the entrypoint */
    if (hServerDll)
    {
        /* Initialize a string for the entrypoint, or use the default */
        RtlInitAnsiString(&EntryPointString,
                          EntryPoint ? EntryPoint : "ServerDllInitialization");

        /* Get a pointer to it */
        Status = LdrGetProcedureAddress(hServerDll,
                                        &EntryPointString,
                                        0,
                                        (PVOID)&ServerDllInitProcedure);
    }
    else
    {
        /* No handle, so we are loading ourselves */
#ifdef CSR_DBG
        RtlInitAnsiString(&EntryPointString, "CsrServerDllInitialization");
#endif
        ServerDllInitProcedure = CsrServerDllInitialization;
        Status = STATUS_SUCCESS;
    }

    /* Check if we got the pointer, and call it */
    if (NT_SUCCESS(Status))
    {
        /* Call the Server DLL entrypoint */
        _SEH2_TRY
        {
            Status = ServerDllInitProcedure(ServerDll);
        }
        _SEH2_EXCEPT(CsrUnhandledExceptionFilter(_SEH2_GetExceptionInformation()))
        {
            Status = _SEH2_GetExceptionCode();
#ifdef CSR_DBG
            DPRINT1("CSRSS: Exception 0x%lx while calling Server DLL entrypoint %Z!%Z()\n",
                    Status, &DllName, &EntryPointString);
#endif
        }
        _SEH2_END;

        if (NT_SUCCESS(Status))
        {
            /*
             * Add this Server's Per-Process Data Size to the total that each
             * process will need.
             */
            CsrTotalPerProcessDataLength += ServerDll->SizeOfProcessData;

            /* Save the pointer in our list */
            CsrLoadedServerDll[ServerDll->ServerId] = ServerDll;

            /* Does it use our generic heap? */
            if (ServerDll->SharedSection != CsrSrvSharedSectionHeap)
            {
                /* No, save the pointer to its shared section in our list */
                CsrSrvSharedStaticServerData[ServerDll->ServerId] = ServerDll->SharedSection;
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        /* Server Init failed, unload it */
        if (hServerDll) LdrUnloadDll(hServerDll);

        /* Delete the Object */
        RtlFreeHeap(CsrHeap, 0, ServerDll);
    }

    /* Return to caller */
    return Status;
}

/*++
 * @name CsrSrvClientConnect
 *
 * The CsrSrvClientConnect CSR API handles a new connection to a server DLL.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message for this request.
 *
 * @param ReplyCode
 *        Optional reply to this request.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_INVALID_PARAMETER
 *         or STATUS_TOO_MANY_NAMES in case of failure.
 *
 * @remarks None.
 *
 *--*/
CSR_API(CsrSrvClientConnect)
{
    NTSTATUS Status;
    PCSR_CLIENT_CONNECT ClientConnect = &ApiMessage->Data.CsrClientConnect;
    PCSR_SERVER_DLL ServerDll;
    PCSR_PROCESS CurrentProcess = CsrGetClientThread()->Process;

    /* Set default reply */
    *ReplyCode = CsrReplyImmediately;

    /* Validate the ServerID */
    if (ClientConnect->ServerId >= CSR_SERVER_DLL_MAX)
    {
        return STATUS_TOO_MANY_NAMES;
    }
    else if (!CsrLoadedServerDll[ClientConnect->ServerId])
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate the Message Buffer */
    if (!(CsrValidateMessageBuffer(ApiMessage,
                                   &ClientConnect->ConnectionInfo,
                                   ClientConnect->ConnectionInfoSize,
                                   sizeof(BYTE))))
    {
        /* Fail due to buffer overflow or other invalid buffer */
        return STATUS_INVALID_PARAMETER;
    }

    /* Load the Server DLL */
    ServerDll = CsrLoadedServerDll[ClientConnect->ServerId];

    /* Check if it has a Connect Callback */
    if (ServerDll->ConnectCallback)
    {
        /* Call the callback */
        Status = ServerDll->ConnectCallback(CurrentProcess,
                                            ClientConnect->ConnectionInfo,
                                            &ClientConnect->ConnectionInfoSize);
    }
    else
    {
        /* Assume success */
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
}

/*++
 * @name CsrSrvCreateSharedSection
 *
 * The CsrSrvCreateSharedSection creates the Shared Section that all
 * CSR Server DLLs and Clients can use to share data.
 *
 * @param ParameterValue
 *        Specially formatted string from our registry command-line which
 *        specifies various arguments for the shared section.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL otherwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSrvCreateSharedSection(IN PCHAR ParameterValue)
{
    PCHAR SizeValue = ParameterValue;
    ULONG Size;
    NTSTATUS Status;
    LARGE_INTEGER SectionSize;
    SIZE_T ViewSize = 0;
    PPEB Peb = NtCurrentPeb();

    /* If there's no parameter, fail */
    if (!ParameterValue) return STATUS_INVALID_PARAMETER;

    /* Find the first comma, and null terminate */
    while (*SizeValue)
    {
        if (*SizeValue == ',')
        {
            *SizeValue++ = ANSI_NULL;
            break;
        }
        else
        {
            SizeValue++;
        }
    }

    /* Make sure it's valid */
    if (!*SizeValue) return STATUS_INVALID_PARAMETER;

    /* Convert it to an integer */
    Status = RtlCharToInteger(SizeValue, 0, &Size);
    if (!NT_SUCCESS(Status)) return Status;

    /* Multiply by 1024 entries and round to page size */
    CsrSrvSharedSectionSize = ROUND_UP(Size * 1024, CsrNtSysInfo.PageSize);

    /* Create the Secion */
    SectionSize.LowPart = CsrSrvSharedSectionSize;
    SectionSize.HighPart = 0;
    Status = NtCreateSection(&CsrSrvSharedSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SectionSize,
                             PAGE_EXECUTE_READWRITE,
                             SEC_BASED | SEC_RESERVE,
                             NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Map the section */
    Status = NtMapViewOfSection(CsrSrvSharedSection,
                                NtCurrentProcess(),
                                &CsrSrvSharedSectionBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                MEM_TOP_DOWN,
                                PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        NtClose(CsrSrvSharedSection);
        return Status;
    }

    /* FIXME: Write the value to registry */

    /* The Heap is the same place as the Base */
    CsrSrvSharedSectionHeap = CsrSrvSharedSectionBase;

    /* Create the heap */
    if (!(RtlCreateHeap(HEAP_ZERO_MEMORY | HEAP_CLASS_7,
                        CsrSrvSharedSectionHeap,
                        CsrSrvSharedSectionSize,
                        PAGE_SIZE,
                        0,
                        0)))
    {
        /* Failure, unmap section and return */
        NtUnmapViewOfSection(NtCurrentProcess(), CsrSrvSharedSectionBase);
        NtClose(CsrSrvSharedSection);
        return STATUS_NO_MEMORY;
    }

    /* Now allocate space from the heap for the Shared Data */
    CsrSrvSharedStaticServerData = RtlAllocateHeap(CsrSrvSharedSectionHeap,
                                                   HEAP_ZERO_MEMORY,
                                                   CSR_SERVER_DLL_MAX * sizeof(PVOID));
    if (!CsrSrvSharedStaticServerData) return STATUS_NO_MEMORY;

    /* Write the values to the PEB */
    Peb->ReadOnlySharedMemoryBase = CsrSrvSharedSectionBase;
    Peb->ReadOnlySharedMemoryHeap = CsrSrvSharedSectionHeap;
    Peb->ReadOnlyStaticServerData = CsrSrvSharedStaticServerData;

    /* Return */
    return STATUS_SUCCESS;
}

/*++
 * @name CsrSrvAttachSharedSection
 *
 * The CsrSrvAttachSharedSection maps the CSR Shared Section into a new
 * CSR Process' address space, and returns the pointers to the section
 * through the Connection Info structure.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process that is attempting a connection.
 *
 * @param ConnectInfo
 *        Pointer to the CSR Connection Info structure for the incoming
 *        connection.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL otherwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSrvAttachSharedSection(IN PCSR_PROCESS CsrProcess OPTIONAL,
                          OUT PCSR_API_CONNECTINFO ConnectInfo)
{
    NTSTATUS Status;
    SIZE_T ViewSize = 0;

    /* Check if we have a process */
    if (CsrProcess)
    {
        /* Map the section into this process */
        Status = NtMapViewOfSection(CsrSrvSharedSection,
                                    CsrProcess->ProcessHandle,
                                    &CsrSrvSharedSectionBase,
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    ViewUnmap,
                                    SEC_NO_CHANGE,
                                    PAGE_EXECUTE_READ);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Write the values in the Connection Info structure */
    ConnectInfo->SharedSectionBase = CsrSrvSharedSectionBase;
    ConnectInfo->SharedSectionHeap = CsrSrvSharedSectionHeap;
    ConnectInfo->SharedStaticServerData = CsrSrvSharedStaticServerData;

    /* Return success */
    return STATUS_SUCCESS;
}

#if (NTDDI_VERSION < NTDDI_WS03)

/*++
 * @name CsrSrvIdentifyAlertableThread
 * @implemented NT4, up to WinXP
 *
 * The CsrSrvIdentifyAlertableThread CSR API marks a CSR Thread as alertable.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message for this request.
 *
 * @param ReplyCode
 *        Pointer to an optional reply to this request.
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks Deprecated.
 *
 *--*/
CSR_API(CsrSrvIdentifyAlertableThread)
{
    PCSR_THREAD CsrThread = CsrGetClientThread();

    UNREFERENCED_PARAMETER(ApiMessage);
    UNREFERENCED_PARAMETER(ReplyCode);

    /* Set the alertable flag */
    CsrThread->Flags |= CsrThreadAlertable;

    /* Return success */
    return STATUS_SUCCESS;
}

/*++
 * @name CsrSrvSetPriorityClass
 * @implemented NT4, up to WinXP
 *
 * The CsrSrvSetPriorityClass CSR API is deprecated.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message for this request.
 *
 * @param ReplyCode
 *        Pointer to an optional reply to this request.
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks Deprecated.
 *
 *--*/
CSR_API(CsrSrvSetPriorityClass)
{
    UNREFERENCED_PARAMETER(ApiMessage);
    UNREFERENCED_PARAMETER(ReplyCode);

    /* Deprecated */
    return STATUS_SUCCESS;
}

#endif // (NTDDI_VERSION < NTDDI_WS03)

/*++
 * @name CsrSrvUnusedFunction
 * @implemented NT4
 *
 * The CsrSrvUnusedFunction CSR API is a stub for deprecated APIs.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message for this request.
 *
 * @param ReplyCode
 *        Pointer to an optional reply to this request.
 *
 * @return STATUS_INVALID_PARAMETER.
 *
 * @remarks CsrSrvSetPriorityClass does not use this stub because
 *          it must return success.
 *
 *--*/
CSR_API(CsrSrvUnusedFunction)
{
    UNREFERENCED_PARAMETER(ApiMessage);
    UNREFERENCED_PARAMETER(ReplyCode);

    /* Deprecated */
    return STATUS_INVALID_PARAMETER;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*++
 * @name CsrSetCallingSpooler
 * @implemented NT4
 *
 * the CsrSetCallingSpooler routine is deprecated.
 *
 * @param Reserved
 *        Deprecated
 *
 * @return None.
 *
 * @remarks This routine was used in archaic versions of NT for Printer Drivers.
 *
 *--*/
VOID
NTAPI
CsrSetCallingSpooler(ULONG Reserved)
{
    /* Deprecated */
    return;
}

/*++
 * @name CsrUnhandledExceptionFilter
 * @implemented NT5
 *
 * The CsrUnhandledExceptionFilter routine handles all exceptions
 * within SEH-protected blocks.
 *
 * @param ExceptionPointers
 *        System-defined Argument.
 *
 * @return EXCEPTION_EXECUTE_HANDLER.
 *
 * @remarks None.
 *
 *--*/
EXCEPTION_DISPOSITION
NTAPI
CsrUnhandledExceptionFilter(IN PEXCEPTION_POINTERS ExceptionInfo)
{
    SYSTEM_KERNEL_DEBUGGER_INFORMATION DebuggerInfo;
    EXCEPTION_DISPOSITION Result = EXCEPTION_EXECUTE_HANDLER;
    BOOLEAN OldValue;
    NTSTATUS Status;
    UNICODE_STRING ErrorSource;
    ULONG_PTR ErrorParameters[4];
    ULONG Response;

    DPRINT1("CsrUnhandledExceptionFilter called\n");

    /* Check if a debugger is installed */
    Status = NtQuerySystemInformation(SystemKernelDebuggerInformation,
                                      &DebuggerInfo,
                                      sizeof(DebuggerInfo),
                                      NULL);

    /* Check if this is Session 0, and the Debugger is Enabled */
    if ((NtCurrentPeb()->SessionId != 0) && (NT_SUCCESS(Status)) &&
        (DebuggerInfo.KernelDebuggerEnabled))
    {
        /* Call the Unhandled Exception Filter */
        Result = RtlUnhandledExceptionFilter(ExceptionInfo);
        if (Result != EXCEPTION_CONTINUE_EXECUTION)
        {
            /* We're going to raise an error. Get Shutdown Privilege first */
            Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                        TRUE,
                                        TRUE,
                                        &OldValue);

            /* Use the Process token if that failed */
            if (Status == STATUS_NO_TOKEN)
            {
                Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                            TRUE,
                                            FALSE,
                                            &OldValue);
            }
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CsrUnhandledExceptionFilter(): RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE) failed, Status = 0x%08lx\n", Status);
                goto NoPrivilege;
            }

            /* Initialize our Name String */
            RtlInitUnicodeString(&ErrorSource, L"Windows SubSystem");

            /* Set the parameters */
            ErrorParameters[0] = (ULONG_PTR)&ErrorSource;
            ErrorParameters[1] = ExceptionInfo->ExceptionRecord->ExceptionCode;
            ErrorParameters[2] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;
            ErrorParameters[3] = (ULONG_PTR)ExceptionInfo->ContextRecord;

            /* Bugcheck */
            NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
                             4,
                             1,
                             ErrorParameters,
                             OptionShutdownSystem,
                             &Response);
        }

NoPrivilege:
        /* Just terminate us */
        NtTerminateProcess(NtCurrentProcess(),
                           ExceptionInfo->ExceptionRecord->ExceptionCode);
    }

    return Result;
}

/* EOF */

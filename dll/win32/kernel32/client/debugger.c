/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/debugger.c
 * PURPOSE:         Wrappers for the NT Debug Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#include <ndk/dbgkfuncs.h>

#define NDEBUG
#include <debug.h>

typedef struct _DBGSS_THREAD_DATA
{
    struct _DBGSS_THREAD_DATA *Next;
    HANDLE ThreadHandle;
    HANDLE ProcessHandle;
    DWORD ProcessId;
    DWORD ThreadId;
    BOOLEAN HandleMarked;
} DBGSS_THREAD_DATA, *PDBGSS_THREAD_DATA;

#define DbgSsSetThreadData(d) \
    NtCurrentTeb()->DbgSsReserved[0] = d

#define DbgSsGetThreadData() \
    ((PDBGSS_THREAD_DATA)NtCurrentTeb()->DbgSsReserved[0])

/* PRIVATE FUNCTIONS *********************************************************/

static
HANDLE
K32CreateDBMonMutex(void)
{
    static SID_IDENTIFIER_AUTHORITY siaNTAuth = {SECURITY_NT_AUTHORITY};
    static SID_IDENTIFIER_AUTHORITY siaWorldAuth = {SECURITY_WORLD_SID_AUTHORITY};
    HANDLE hMutex;

    /* SIDs to be used in the DACL */
    PSID psidSystem = NULL;
    PSID psidAdministrators = NULL;
    PSID psidEveryone = NULL;

    /* buffer for the DACL */
    PVOID pDaclBuf = NULL;

    /* minimum size of the DACL: an ACL descriptor and three ACCESS_ALLOWED_ACE
       headers. We'll add the size of SIDs when we'll know it
    */
    SIZE_T nDaclBufSize =
         sizeof(ACL) + (sizeof(ACCESS_ALLOWED_ACE) -
                        sizeof(((ACCESS_ALLOWED_ACE*)0)->SidStart)) * 3;

    /* security descriptor of the mutex */
    SECURITY_DESCRIPTOR sdMutexSecurity;

    /* attributes of the mutex object we'll create */
    SECURITY_ATTRIBUTES saMutexAttribs = {sizeof(saMutexAttribs),
                                          &sdMutexSecurity,
                                          TRUE};

    NTSTATUS nErrCode;

    /* first, try to open the mutex */
    hMutex = OpenMutexW (SYNCHRONIZE | READ_CONTROL | MUTANT_QUERY_STATE,
                         TRUE,
                         L"DBWinMutex");

    if (hMutex != NULL)
    {
        /* success */
        return hMutex;
    }
    /* error other than the mutex not being found */
    else if (GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        /* failure */
        return NULL;
    }

    /* if the mutex doesn't exist, create it */

    /* first, set up the mutex security */
    /* allocate the NT AUTHORITY\SYSTEM SID */
    nErrCode = RtlAllocateAndInitializeSid(&siaNTAuth,
                                           1,
                                           SECURITY_LOCAL_SYSTEM_RID,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           &psidSystem);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* allocate the BUILTIN\Administrators SID */
    nErrCode = RtlAllocateAndInitializeSid(&siaNTAuth,
                                           2,
                                           SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           &psidAdministrators);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* allocate the Everyone SID */
    nErrCode = RtlAllocateAndInitializeSid(&siaWorldAuth,
                                           1,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           &psidEveryone);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* allocate space for the SIDs too */
    nDaclBufSize += RtlLengthSid(psidSystem);
    nDaclBufSize += RtlLengthSid(psidAdministrators);
    nDaclBufSize += RtlLengthSid(psidEveryone);

    /* allocate the buffer for the DACL */
    pDaclBuf = GlobalAlloc(GMEM_FIXED, nDaclBufSize);

    /* failure */
    if (pDaclBuf == NULL) goto l_Cleanup;

    /* create the DACL */
    nErrCode = RtlCreateAcl(pDaclBuf, nDaclBufSize, ACL_REVISION);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* grant the minimum required access to Everyone */
    nErrCode = RtlAddAccessAllowedAce(pDaclBuf,
                                      ACL_REVISION,
                                      SYNCHRONIZE |
                                      READ_CONTROL |
                                      MUTANT_QUERY_STATE,
                                      psidEveryone);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* grant full access to BUILTIN\Administrators */
    nErrCode = RtlAddAccessAllowedAce(pDaclBuf,
                                      ACL_REVISION,
                                      MUTANT_ALL_ACCESS,
                                      psidAdministrators);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* grant full access to NT AUTHORITY\SYSTEM */
    nErrCode = RtlAddAccessAllowedAce(pDaclBuf,
                                      ACL_REVISION,
                                      MUTANT_ALL_ACCESS,
                                      psidSystem);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* create the security descriptor */
    nErrCode = RtlCreateSecurityDescriptor(&sdMutexSecurity,
                                           SECURITY_DESCRIPTOR_REVISION);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* set the descriptor's DACL to the ACL we created */
    nErrCode = RtlSetDaclSecurityDescriptor(&sdMutexSecurity,
                                            TRUE,
                                            pDaclBuf,
                                            FALSE);

    /* failure */
    if (!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* create the mutex */
    hMutex = CreateMutexW(&saMutexAttribs, FALSE, L"DBWinMutex");

l_Cleanup:
    /* free the buffers */
    if (pDaclBuf) GlobalFree(pDaclBuf);
    if (psidEveryone) RtlFreeSid(psidEveryone);
    if (psidAdministrators) RtlFreeSid(psidAdministrators);
    if (psidSystem) RtlFreeSid(psidSystem);

    return hMutex;
}

VOID
WINAPI
SaveThreadHandle(IN DWORD dwProcessId,
                 IN DWORD dwThreadId,
                 IN HANDLE hThread)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Allocate a thread structure */
    ThreadData = RtlAllocateHeap(RtlGetProcessHeap(),
                                 0,
                                 sizeof(DBGSS_THREAD_DATA));
    if (!ThreadData) return;

    /* Fill it out */
    ThreadData->ThreadHandle = hThread;
    ThreadData->ProcessId = dwProcessId;
    ThreadData->ThreadId = dwThreadId;
    ThreadData->ProcessHandle = NULL;
    ThreadData->HandleMarked = FALSE;

    /* Link it */
    ThreadData->Next = DbgSsGetThreadData();
    DbgSsSetThreadData(ThreadData);
}

VOID
WINAPI
SaveProcessHandle(IN DWORD dwProcessId,
                  IN HANDLE hProcess)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Allocate a thread structure */
    ThreadData = RtlAllocateHeap(RtlGetProcessHeap(),
                                 0,
                                 sizeof(DBGSS_THREAD_DATA));
    if (!ThreadData) return;

    /* Fill it out */
    ThreadData->ProcessHandle = hProcess;
    ThreadData->ProcessId = dwProcessId;
    ThreadData->ThreadId = 0;
    ThreadData->ThreadHandle = NULL;
    ThreadData->HandleMarked = FALSE;

    /* Link it */
    ThreadData->Next = DbgSsGetThreadData();
    DbgSsSetThreadData(ThreadData);
}

VOID
WINAPI
MarkThreadHandle(IN DWORD dwThreadId)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Loop all thread data events */
    for (ThreadData = DbgSsGetThreadData(); ThreadData; ThreadData = ThreadData->Next)
    {
        /* Check if this one matches */
        if (ThreadData->ThreadId == dwThreadId)
        {
            /* Mark the structure and break out */
            ThreadData->HandleMarked = TRUE;
            break;
        }
    }
}

VOID
WINAPI
MarkProcessHandle(IN DWORD dwProcessId)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Loop all thread data events */
    for (ThreadData = DbgSsGetThreadData(); ThreadData; ThreadData = ThreadData->Next)
    {
        /* Check if this one matches */
        if ((ThreadData->ProcessId == dwProcessId) && !(ThreadData->ThreadId))
        {
            /* Mark the structure and break out */
            ThreadData->HandleMarked = TRUE;
            break;
        }
    }
}

VOID
WINAPI
RemoveHandles(IN DWORD dwProcessId,
              IN DWORD dwThreadId)
{
    PDBGSS_THREAD_DATA *ThreadData;
    PDBGSS_THREAD_DATA ThisData;

    /* Loop all thread data events */
    ThreadData = (PDBGSS_THREAD_DATA*)NtCurrentTeb()->DbgSsReserved;
    ThisData = *ThreadData;
    while(ThisData)
    {
        /* Check if this one matches */
        if ((ThisData->HandleMarked) &&
            ((ThisData->ProcessId == dwProcessId) || (ThisData->ThreadId == dwThreadId)))
        {
            /* Close open handles */
            if (ThisData->ThreadHandle) CloseHandle(ThisData->ThreadHandle);
            if (ThisData->ProcessHandle) CloseHandle(ThisData->ProcessHandle);

            /* Unlink the thread data */
            *ThreadData = ThisData->Next;

            /* Free it*/
            RtlFreeHeap(RtlGetProcessHeap(), 0, ThisData);
        }
        else
        {
            /* Move to the next one */
            ThreadData = &ThisData->Next;
        }
        ThisData = *ThreadData;
    }
}

VOID
WINAPI
CloseAllProcessHandles(IN DWORD dwProcessId)
{
    PDBGSS_THREAD_DATA *ThreadData;
    PDBGSS_THREAD_DATA ThisData;

    /* Loop all thread data events */
    ThreadData = (PDBGSS_THREAD_DATA*)NtCurrentTeb()->DbgSsReserved;
    ThisData = *ThreadData;
    while(ThisData)
    {
        /* Check if this one matches */
        if (ThisData->ProcessId == dwProcessId)
        {
            /* Close open handles */
            if (ThisData->ThreadHandle) CloseHandle(ThisData->ThreadHandle);
            if (ThisData->ProcessHandle) CloseHandle(ThisData->ProcessHandle);

            /* Unlink the thread data */
            *ThreadData = ThisData->Next;

            /* Free it*/
            RtlFreeHeap(RtlGetProcessHeap(), 0, ThisData);
        }
        else
        {
            /* Move to the next one */
            ThreadData = &ThisData->Next;
        }
        ThisData = *ThreadData;
    }
}

HANDLE
WINAPI
ProcessIdToHandle(IN DWORD dwProcessId)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    CLIENT_ID ClientId;

    /* If we don't have a PID, look it up */
    if (dwProcessId == MAXDWORD) dwProcessId = (DWORD_PTR)CsrGetProcessId();

    /* Open a handle to the process */
    ClientId.UniqueThread = NULL;
    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&Handle,
                           PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                           PROCESS_VM_WRITE | PROCESS_VM_READ |
                           PROCESS_SUSPEND_RESUME | PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes,
                           &ClientId);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Return the handle */
    return Handle;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
CheckRemoteDebuggerPresent(IN HANDLE hProcess,
                           OUT PBOOL pbDebuggerPresent)
{
    HANDLE DebugPort;
    NTSTATUS Status;

    /* Make sure we have an output and process*/
    if (!(pbDebuggerPresent) || !(hProcess))
    {
        /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check if the process has a debug object/port */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessDebugPort,
                                       &DebugPort,
                                       sizeof(DebugPort),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Return the current state */
        *pbDebuggerPresent = DebugPort != NULL;
        return TRUE;
    }

    /* Otherwise, fail */
    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ContinueDebugEvent(IN DWORD dwProcessId,
                   IN DWORD dwThreadId,
                   IN DWORD dwContinueStatus)
{
    CLIENT_ID ClientId;
    NTSTATUS Status;

    /* Set the Client ID */
    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    ClientId.UniqueThread = UlongToHandle(dwThreadId);

    /* Continue debugging */
    Status = DbgUiContinue(&ClientId, dwContinueStatus);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Remove the process/thread handles */
    RemoveHandles(dwProcessId, dwThreadId);

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugActiveProcess(IN DWORD dwProcessId)
{
    NTSTATUS Status, Status1;
    HANDLE Handle;

    /* Connect to the debugger */
    Status = DbgUiConnectToDbg();
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Get the process handle */
    Handle = ProcessIdToHandle(dwProcessId);
    if (!Handle) return FALSE;

    /* Now debug the process */
    Status = DbgUiDebugActiveProcess(Handle);

    /* Close the handle since we're done */
    Status1 = NtClose(Handle);
    ASSERT(NT_SUCCESS(Status1));

    /* Check if debugging worked */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugActiveProcessStop(IN DWORD dwProcessId)
{
    NTSTATUS Status, Status1;
    HANDLE Handle;

    /* Get the process handle */
    Handle = ProcessIdToHandle(dwProcessId);
    if (!Handle) return FALSE;

    /* Close all the process handles */
    CloseAllProcessHandles(dwProcessId);

    /* Now stop debugging the process */
    Status = DbgUiStopDebugging(Handle);
    Status1 = NtClose(Handle);
    ASSERT(NT_SUCCESS(Status1));

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugBreakProcess(IN HANDLE Process)
{
    NTSTATUS Status;

    /* Send the breakin request */
    Status = DbgUiIssueRemoteBreakin(Process);
    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugSetProcessKillOnExit(IN BOOL KillOnExit)
{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG State;

    /* Get the debug object */
    Handle = DbgUiGetThreadDebugObject();
    if (!Handle)
    {
        /* Fail */
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
    }

    /* Now set the kill-on-exit state */
    State = KillOnExit != 0;
    Status = NtSetInformationDebugObject(Handle,
                                         DebugObjectKillProcessOnExitInformation,
                                         &State,
                                         sizeof(State),
                                         NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsDebuggerPresent(VOID)
{
    return (BOOL)NtCurrentPeb()->BeingDebugged;
}

/*
 * @implemented
 */
BOOL
WINAPI
WaitForDebugEvent(IN LPDEBUG_EVENT lpDebugEvent,
                  IN DWORD dwMilliseconds)
{
    LARGE_INTEGER WaitTime;
    PLARGE_INTEGER Timeout;
    DBGUI_WAIT_STATE_CHANGE WaitStateChange;
    NTSTATUS Status;

    /* Convert to NT Timeout */
    Timeout = BaseFormatTimeOut(&WaitTime, dwMilliseconds);

    /* Loop while we keep getting interrupted */
    do
    {
        /* Call the native API */
        Status = DbgUiWaitStateChange(&WaitStateChange, Timeout);
    } while ((Status == STATUS_ALERTED) || (Status == STATUS_USER_APC));

    /* Check if the wait failed */
    if (!(NT_SUCCESS(Status)) || (Status == DBG_UNABLE_TO_PROVIDE_HANDLE))
    {
        /* Set the error code and quit */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Check if we timed out */
    if (Status == STATUS_TIMEOUT)
    {
        /* Fail with a timeout error */
        SetLastError(ERROR_SEM_TIMEOUT);
        return FALSE;
    }

    /* Convert the structure */
    Status = DbgUiConvertStateChangeStructure(&WaitStateChange, lpDebugEvent);
    if (!NT_SUCCESS(Status))
    {
        /* Set the error code and quit */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Check what kind of event this was */
    switch (lpDebugEvent->dwDebugEventCode)
    {
        /* New thread was created */
        case CREATE_THREAD_DEBUG_EVENT:

            /* Setup the thread data */
            SaveThreadHandle(lpDebugEvent->dwProcessId,
                             lpDebugEvent->dwThreadId,
                             lpDebugEvent->u.CreateThread.hThread);
            break;

        /* New process was created */
        case CREATE_PROCESS_DEBUG_EVENT:

            /* Setup the process data */
            SaveProcessHandle(lpDebugEvent->dwProcessId,
                              lpDebugEvent->u.CreateProcessInfo.hProcess);

            /* Setup the thread data */
            SaveThreadHandle(lpDebugEvent->dwProcessId,
                             lpDebugEvent->dwThreadId,
                             lpDebugEvent->u.CreateProcessInfo.hThread);
            break;

        /* Process was exited */
        case EXIT_PROCESS_DEBUG_EVENT:

            /* Mark the thread data as such and fall through */
            MarkProcessHandle(lpDebugEvent->dwProcessId);

        /* Thread was exited */
        case EXIT_THREAD_DEBUG_EVENT:

            /* Mark the thread data */
            MarkThreadHandle(lpDebugEvent->dwThreadId);
            break;

        /* Nothing to do */
        case EXCEPTION_DEBUG_EVENT:
        case LOAD_DLL_DEBUG_EVENT:
        case UNLOAD_DLL_DEBUG_EVENT:
        case OUTPUT_DEBUG_STRING_EVENT:
        case RIP_EVENT:
            break;

        /* Fail anything else */
        default:
            return FALSE;
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
VOID
WINAPI
OutputDebugStringA(IN LPCSTR _OutputString)
{
    _SEH2_TRY
    {
        ULONG_PTR a_nArgs[2];

        a_nArgs[0] = (ULONG_PTR)(strlen(_OutputString) + 1);
        a_nArgs[1] = (ULONG_PTR)_OutputString;

        /* send the string to the user-mode debugger */
        RaiseException(DBG_PRINTEXCEPTION_C, 0, 2, a_nArgs);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* no user-mode debugger: try the systemwide debug message monitor, or the
           kernel debugger as a last resort */

        /* mutex used to synchronize invocations of OutputDebugString */
        static HANDLE s_hDBMonMutex = NULL;
        /* true if we already attempted to open/create the mutex */
        static BOOL s_bDBMonMutexTriedOpen = FALSE;

        /* local copy of the mutex handle */
        volatile HANDLE hDBMonMutex = s_hDBMonMutex;
        /* handle to the Section of the shared buffer */
        volatile HANDLE hDBMonBuffer = NULL;

        /* pointer to the mapped view of the shared buffer. It consist of the current
           process id followed by the message string */
        struct { DWORD ProcessId; CHAR Buffer[1]; } * pDBMonBuffer = NULL;

        /* event: signaled by the debug message monitor when OutputDebugString can write
           to the shared buffer */
        volatile HANDLE hDBMonBufferReady = NULL;

        /* event: to be signaled by OutputDebugString when it's done writing to the
           shared buffer */
        volatile HANDLE hDBMonDataReady = NULL;

        /* mutex not opened, and no previous attempts to open/create it */
        if (hDBMonMutex == NULL && !s_bDBMonMutexTriedOpen)
        {
            /* open/create the mutex */
            hDBMonMutex = K32CreateDBMonMutex();
            /* store the handle */
            s_hDBMonMutex = hDBMonMutex;
        }

        _SEH2_TRY
        {
            volatile PCHAR a_cBuffer = NULL;

            /* opening the mutex failed */
            if (hDBMonMutex == NULL)
            {
                /* remember next time */
                s_bDBMonMutexTriedOpen = TRUE;
            }
            /* opening the mutex succeeded */
            else
            {
                do
                {
                    /* synchronize with other invocations of OutputDebugString */
                    WaitForSingleObject(hDBMonMutex, INFINITE);

                    /* buffer of the system-wide debug message monitor */
                    hDBMonBuffer = OpenFileMappingW(SECTION_MAP_WRITE, FALSE, L"DBWIN_BUFFER");

                    /* couldn't open the buffer: send the string to the kernel debugger */
                    if (hDBMonBuffer == NULL) break;

                    /* map the buffer */
                    pDBMonBuffer = MapViewOfFile(hDBMonBuffer,
                                                 SECTION_MAP_READ | SECTION_MAP_WRITE,
                                                 0,
                                                 0,
                                                 0);

                    /* couldn't map the buffer: send the string to the kernel debugger */
                    if (pDBMonBuffer == NULL) break;

                    /* open the event signaling that the buffer can be accessed */
                    hDBMonBufferReady = OpenEventW(SYNCHRONIZE, FALSE, L"DBWIN_BUFFER_READY");

                    /* couldn't open the event: send the string to the kernel debugger */
                    if (hDBMonBufferReady == NULL) break;

                    /* open the event to be signaled when the buffer has been filled */
                    hDBMonDataReady = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"DBWIN_DATA_READY");
                }
                while(0);

                /* we couldn't connect to the system-wide debug message monitor: send the
                   string to the kernel debugger */
                if (hDBMonDataReady == NULL) ReleaseMutex(hDBMonMutex);
            }

            _SEH2_TRY
            {
                /* size of the current output block */
                volatile SIZE_T nRoundLen;

                /* size of the remainder of the string */
                volatile SIZE_T nOutputStringLen;

                /* output the whole string */
                nOutputStringLen = strlen(_OutputString);

                do
                {
                    /* we're connected to the debug monitor:
                       write the current block to the shared buffer */
                    if (hDBMonDataReady)
                    {
                        /* wait a maximum of 10 seconds for the debug monitor
                           to finish processing the shared buffer */
                        if (WaitForSingleObject(hDBMonBufferReady, 10000) != WAIT_OBJECT_0)
                        {
                            /* timeout or failure: give up */
                            break;
                        }

                        /* write the process id into the buffer */
                        pDBMonBuffer->ProcessId = GetCurrentProcessId();

                        /* write only as many bytes as they fit in the buffer */
                        if (nOutputStringLen > (PAGE_SIZE - sizeof(DWORD) - 1))
                            nRoundLen = PAGE_SIZE - sizeof(DWORD) - 1;
                        else
                            nRoundLen = nOutputStringLen;

                        /* copy the current block into the buffer */
                        memcpy(pDBMonBuffer->Buffer, _OutputString, nRoundLen);

                        /* null-terminate the current block */
                        pDBMonBuffer->Buffer[nRoundLen] = 0;

                        /* signal that the data contains meaningful data and can be read */
                        SetEvent(hDBMonDataReady);
                    }
                    /* else, send the current block to the kernel debugger */
                    else
                    {
                        /* output in blocks of 512 characters */
                        a_cBuffer = (CHAR*)HeapAlloc(GetProcessHeap(), 0, 512);

                        if (!a_cBuffer)
                        {
                            DbgPrint("OutputDebugStringA: Failed\n");
                            break;
                        }

                        /* write a maximum of 511 bytes */
                        if (nOutputStringLen > 510)
                            nRoundLen = 510;
                        else
                            nRoundLen = nOutputStringLen;

                        /* copy the current block */
                        memcpy(a_cBuffer, _OutputString, nRoundLen);

                        /* null-terminate the current block */
                        a_cBuffer[nRoundLen] = 0;

                        /* send the current block to the kernel debugger */
                        DbgPrint("%s", a_cBuffer);

                        if (a_cBuffer)
                        {
                            HeapFree(GetProcessHeap(), 0, a_cBuffer);
                            a_cBuffer = NULL;
                        }
                    }

                    /* move to the next block */
                    _OutputString += nRoundLen;
                    nOutputStringLen -= nRoundLen;
                }
                /* repeat until the string has been fully output */
                while (nOutputStringLen > 0);
            }
            /* ignore access violations and let other exceptions fall through */
            _SEH2_EXCEPT((_SEH2_GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                if (a_cBuffer)
                    HeapFree(GetProcessHeap(), 0, a_cBuffer);

                /* string copied verbatim from Microsoft's kernel32.dll */
                DbgPrint("\nOutputDebugString faulted during output\n");
            }
            _SEH2_END;
        }
        _SEH2_FINALLY
        {
            /* close all the still open resources */
            if (hDBMonBufferReady) CloseHandle(hDBMonBufferReady);
            if (pDBMonBuffer) UnmapViewOfFile(pDBMonBuffer);
            if (hDBMonBuffer) CloseHandle(hDBMonBuffer);
            if (hDBMonDataReady) CloseHandle(hDBMonDataReady);

            /* leave the critical section */
            if (hDBMonDataReady != NULL)
                ReleaseMutex(hDBMonMutex);
        }
        _SEH2_END;
    }
    _SEH2_END;
}

/*
 * @implemented
 */
VOID
WINAPI
OutputDebugStringW(IN LPCWSTR OutputString)
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    /* convert the string in ANSI */
    RtlInitUnicodeString(&UnicodeString, OutputString);
    Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);

    /* OutputDebugStringW always prints something, even if conversion fails */
    if (!NT_SUCCESS(Status)) AnsiString.Buffer = "";

    /* Output the converted string */
    OutputDebugStringA(AnsiString.Buffer);

    /* free the converted string */
    if (NT_SUCCESS(Status)) RtlFreeAnsiString(&AnsiString);
}

/* EOF */

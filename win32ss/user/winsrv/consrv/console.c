/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/console.c
 * PURPOSE:         Console Management Functions
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#include <ndk/psfuncs.h>

#include <alias.h>
#include "procinit.h"

#define NDEBUG
#include <debug.h>

// FIXME: Add this prototype to winternl.h / rtlfuncs.h / ...
NTSTATUS NTAPI RtlGetLastNtStatus(VOID);

/* GLOBALS ********************************************************************/

static ULONG ConsoleListSize;
static PCONSRV_CONSOLE* ConsoleList;   /* The list of the ConSrv consoles */
static RTL_RESOURCE ListLock;

#define ConSrvLockConsoleListExclusive()    \
    RtlAcquireResourceExclusive(&ListLock, TRUE)

#define ConSrvLockConsoleListShared()       \
    RtlAcquireResourceShared(&ListLock, TRUE)

#define ConSrvUnlockConsoleList()           \
    RtlReleaseResource(&ListLock)


static NTSTATUS
InsertConsole(OUT PHANDLE Handle,
              IN PCONSRV_CONSOLE Console)
{
#define CONSOLE_HANDLES_INCREMENT   2 * 3

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i = 0;
    PCONSRV_CONSOLE* Block;

    ASSERT( (ConsoleList == NULL && ConsoleListSize == 0) ||
            (ConsoleList != NULL && ConsoleListSize != 0) );

    /* All went right, so add the console to the list */
    ConSrvLockConsoleListExclusive();
    DPRINT1("Insert in the list\n");

    if (ConsoleList)
    {
        for (i = 0; i < ConsoleListSize; i++)
        {
            if (ConsoleList[i] == NULL) break;
        }
    }

    if (i >= ConsoleListSize)
    {
        DPRINT1("Creation of a new handles table\n");
        /* Allocate a new handles table */
        Block = ConsoleAllocHeap(HEAP_ZERO_MEMORY,
                                 (ConsoleListSize +
                                    CONSOLE_HANDLES_INCREMENT) * sizeof(PCONSRV_CONSOLE));
        if (Block == NULL)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Quit;
        }

        /* If we previously had a handles table, free it and use the new one */
        if (ConsoleList)
        {
            /* Copy the handles from the old table to the new one */
            RtlCopyMemory(Block,
                          ConsoleList,
                          ConsoleListSize * sizeof(PCONSRV_CONSOLE));
            ConsoleFreeHeap(ConsoleList);
        }
        ConsoleList = Block;
        ConsoleListSize += CONSOLE_HANDLES_INCREMENT;
    }

    ConsoleList[i] = Console;
    *Handle = ULongToHandle((i << 2) | 0x3);

Quit:
    /* Unlock the console list and return status */
    ConSrvUnlockConsoleList();
    return Status;
}

/* Unused */
#if 0
static NTSTATUS
RemoveConsoleByHandle(IN HANDLE Handle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSRV_CONSOLE Console;

    BOOLEAN ValidHandle = ((HandleToULong(Handle) & 0x3) == 0x3);
    ULONG Index = HandleToULong(Handle) >> 2;

    if (!ValidHandle) return STATUS_INVALID_HANDLE;

    ASSERT( (ConsoleList == NULL && ConsoleListSize == 0) ||
            (ConsoleList != NULL && ConsoleListSize != 0) );

    /* Remove the console from the list */
    ConSrvLockConsoleListExclusive();

    if (Index >= ConsoleListSize ||
        (Console = ConsoleList[Index]) == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    ConsoleList[Index] = NULL;

Quit:
    /* Unlock the console list and return status */
    ConSrvUnlockConsoleList();
    return Status;
}
#endif

static NTSTATUS
RemoveConsoleByPointer(IN PCONSRV_CONSOLE Console)
{
    ULONG i = 0;

    if (!Console) return STATUS_INVALID_PARAMETER;

    ASSERT( (ConsoleList == NULL && ConsoleListSize == 0) ||
            (ConsoleList != NULL && ConsoleListSize != 0) );

    /* Remove the console from the list */
    ConSrvLockConsoleListExclusive();

    if (ConsoleList)
    {
        for (i = 0; i < ConsoleListSize; i++)
        {
            if (ConsoleList[i] == Console) ConsoleList[i] = NULL;
        }
    }

    /* Unlock the console list */
    ConSrvUnlockConsoleList();

    return STATUS_SUCCESS;
}

BOOLEAN NTAPI
ConSrvValidateConsole(OUT PCONSRV_CONSOLE* Console,
                      IN HANDLE ConsoleHandle,
                      IN CONSOLE_STATE ExpectedState,
                      IN BOOLEAN LockConsole)
{
    BOOLEAN RetVal = FALSE;
    PCONSRV_CONSOLE ValidatedConsole;

    BOOLEAN ValidHandle = ((HandleToULong(ConsoleHandle) & 0x3) == 0x3);
    ULONG Index = HandleToULong(ConsoleHandle) >> 2;

    if (!ValidHandle) return FALSE;

    if (!Console) return FALSE;
    *Console = NULL;

    /*
     * Forbid creation or deletion of consoles when
     * checking for the existence of a console.
     */
    ConSrvLockConsoleListShared();

    if (Index >= ConsoleListSize ||
        (ValidatedConsole = ConsoleList[Index]) == NULL)
    {
        /* Unlock the console list */
        ConSrvUnlockConsoleList();

        return FALSE;
    }

    ValidatedConsole = ConsoleList[Index];

    /* Unlock the console list and return */
    ConSrvUnlockConsoleList();

    RetVal = ConDrvValidateConsoleUnsafe(ValidatedConsole,
                                         ExpectedState,
                                         LockConsole);
    if (RetVal) *Console = ValidatedConsole;

    return RetVal;
}


/* PRIVATE FUNCTIONS **********************************************************/

VOID
ConioPause(PCONSRV_CONSOLE Console, UINT Flags)
{
    Console->PauseFlags |= Flags;
    ConDrvPause(Console);
}

VOID
ConioUnpause(PCONSRV_CONSOLE Console, UINT Flags)
{
    Console->PauseFlags &= ~Flags;

    // if ((Console->PauseFlags & (PAUSED_FROM_KEYBOARD | PAUSED_FROM_SCROLLBAR | PAUSED_FROM_SELECTION)) == 0)
    if (Console->PauseFlags == 0)
    {
        ConDrvUnpause(Console);

        CsrNotifyWait(&Console->WriteWaitQueue,
                      TRUE,
                      NULL,
                      NULL);
        if (!IsListEmpty(&Console->WriteWaitQueue))
        {
            CsrDereferenceWait(&Console->WriteWaitQueue);
        }
    }
}

NTSTATUS
ConSrvGetConsole(IN PCONSOLE_PROCESS_DATA ProcessData,
                 OUT PCONSRV_CONSOLE* Console,
                 IN BOOLEAN LockConsole)
{
    NTSTATUS Status = STATUS_INVALID_HANDLE;
    PCONSRV_CONSOLE GrabConsole;

    // if (Console == NULL) return STATUS_INVALID_PARAMETER;
    ASSERT(Console);
    *Console = NULL;

    // RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (ConSrvValidateConsole(&GrabConsole,
                              ProcessData->ConsoleHandle,
                              CONSOLE_RUNNING,
                              LockConsole))
    {
        InterlockedIncrement(&GrabConsole->ReferenceCount);
        *Console = GrabConsole;
        Status = STATUS_SUCCESS;
    }

    // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return Status;
}

VOID
ConSrvReleaseConsole(IN PCONSRV_CONSOLE Console,
                     IN BOOLEAN WasConsoleLocked)
{
    LONG RefCount = 0;

    if (!Console) return;
    // if (Console->ReferenceCount == 0) return; // This shouldn't happen
    ASSERT(Console->ReferenceCount > 0);

    /* The console must be locked */
    // ASSERT(Console_locked);

    /*
     * Decrement the reference count. Save the new value too,
     * because Console->ReferenceCount might be modified after
     * the console gets unlocked but before we check whether we
     * can destroy it.
     */
    RefCount = _InterlockedDecrement(&Console->ReferenceCount);

    /* Unlock the console if needed */
    if (WasConsoleLocked) LeaveCriticalSection(&Console->Lock);

    /* Delete the console if needed */
    if (RefCount <= 0) ConSrvDeleteConsole(Console);
}


/* CONSOLE INITIALIZATION FUNCTIONS *******************************************/

VOID NTAPI
ConSrvInitConsoleSupport(VOID)
{
    DPRINT("CONSRV: ConSrvInitConsoleSupport()\n");

    /* Initialize the console list and its lock */
    ConsoleListSize = 0;
    ConsoleList = NULL;
    RtlInitializeResource(&ListLock);

    /* Should call LoadKeyboardLayout */
}

NTSTATUS NTAPI
ConSrvInitTerminal(IN OUT PTERMINAL Terminal,
                   IN OUT PCONSOLE_INFO ConsoleInfo,
                   IN OUT PVOID ExtraConsoleInfo,
                   IN ULONG ProcessId);
NTSTATUS NTAPI
ConSrvDeinitTerminal(IN OUT PTERMINAL Terminal);

NTSTATUS NTAPI
ConSrvInitConsole(OUT PHANDLE NewConsoleHandle,
                  OUT PCONSRV_CONSOLE* NewConsole,
                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN ULONG ConsoleLeaderProcessId)
{
    NTSTATUS Status;
    HANDLE ConsoleHandle;
    PCONSRV_CONSOLE Console;
    CONSOLE_INFO ConsoleInfo;
    SIZE_T Length = 0;

    TERMINAL Terminal; /* The ConSrv terminal for this console */

    if (NewConsole == NULL || ConsoleStartInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *NewConsole = NULL;

    /*
     * Load the console settings
     */

    /* 1. Load the default settings */
    ConSrvGetDefaultSettings(&ConsoleInfo, ConsoleLeaderProcessId);

    /* 2. Get the title of the console (initialize ConsoleInfo.ConsoleTitle) */
    Length = min(wcslen(ConsoleStartInfo->ConsoleTitle),
                 sizeof(ConsoleInfo.ConsoleTitle) / sizeof(ConsoleInfo.ConsoleTitle[0]) - 1);
    wcsncpy(ConsoleInfo.ConsoleTitle, ConsoleStartInfo->ConsoleTitle, Length);
    ConsoleInfo.ConsoleTitle[Length] = L'\0';

    /* 3. Initialize the ConSrv terminal */
    Status = ConSrvInitTerminal(&Terminal,
                                &ConsoleInfo,
                                ConsoleStartInfo,
                                ConsoleLeaderProcessId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CONSRV: Failed to initialize a terminal, Status = 0x%08lx\n", Status);
        return Status;
    }
    DPRINT("CONSRV: Terminal initialized\n");

    /*
     * 4. Load the remaining console settings via the registry.
     */
    if ((ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
    {
        /*
         * Either we weren't created by an app launched via a shell-link,
         * or we failed to load shell-link console properties.
         * Therefore, load the console infos for the application from the registry.
         */
        ConSrvReadUserSettings(&ConsoleInfo, ConsoleLeaderProcessId);

        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEFILLATTRIBUTE)
        {
            ConsoleInfo.ScreenAttrib = (USHORT)ConsoleStartInfo->wFillAttribute;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USECOUNTCHARS)
        {
            ConsoleInfo.ScreenBufferSize = ConsoleStartInfo->dwScreenBufferSize;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESIZE)
        {
            ConsoleInfo.ConsoleSize = ConsoleStartInfo->dwWindowSize;
        }
    }

    /* Set-up the code page */
    ConsoleInfo.CodePage = GetOEMCP();

    /* Initialize a new console via the driver */
    Status = ConDrvInitConsole(&Console, &ConsoleInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Creating a new console failed, Status = 0x%08lx\n", Status);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }

    ASSERT(Console);
    DPRINT("Console initialized\n");

    /*** Register ConSrv features ***/

    /* Initialize process support */
    InitializeListHead(&Console->ProcessList);
    Console->NotifiedLastCloseProcess = NULL;
    Console->NotifyLastClose = FALSE;

    /* Initialize pausing support */
    Console->PauseFlags = 0;
    InitializeListHead(&Console->ReadWaitQueue);
    InitializeListHead(&Console->WriteWaitQueue);

    /* Initialize the alias and history buffers */
    Console->Aliases = NULL;
    InitializeListHead(&Console->HistoryBuffers);
    Console->HistoryBufferSize      = ConsoleInfo.HistoryBufferSize;
    Console->NumberOfHistoryBuffers = ConsoleInfo.NumberOfHistoryBuffers;
    Console->HistoryNoDup           = ConsoleInfo.HistoryNoDup;

    Console->QuickEdit = ConsoleInfo.QuickEdit;

    /* Colour table */
    memcpy(Console->Colors, ConsoleInfo.Colors, sizeof(ConsoleInfo.Colors));

    /* Attach the ConSrv terminal to the console */
    Status = ConDrvRegisterTerminal(Console, &Terminal);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register terminal to the given console, Status = 0x%08lx\n", Status);
        ConDrvDeleteConsole(Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }
    DPRINT("Terminal registered\n");

    /* All went right, so add the console to the list */
    Status = InsertConsole(&ConsoleHandle, Console);

    /* Return the newly created console to the caller and a success code too */
    *NewConsoleHandle = ConsoleHandle;
    *NewConsole       = Console;
    return STATUS_SUCCESS;
}

VOID NTAPI
ConSrvDeleteConsole(PCONSRV_CONSOLE Console)
{
    DPRINT("ConSrvDeleteConsole\n");

    // FIXME: Send a terminate message to all the processes owning this console

    /* Remove the console from the list */
    RemoveConsoleByPointer(Console);

    /* Clean aliases and history */
    IntDeleteAllAliases(Console);
    HistoryDeleteBuffers(Console);

    /* Now, call the driver. ConDrvDeregisterTerminal is called on-demand. */
    ConDrvDeleteConsole(Console);
}






static NTSTATUS
ConSrvConsoleCtrlEventTimeout(IN ULONG CtrlEvent,
                              IN PCONSOLE_PROCESS_DATA ProcessData,
                              IN ULONG Timeout)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ConSrvConsoleCtrlEventTimeout Parent ProcessId = %x\n", ProcessData->Process->ClientId.UniqueProcess);

    if (ProcessData->CtrlDispatcher)
    {
        _SEH2_TRY
        {
            HANDLE Thread = NULL;

            _SEH2_TRY
            {
                Thread = CreateRemoteThread(ProcessData->Process->ProcessHandle, NULL, 0,
                                            ProcessData->CtrlDispatcher,
                                            UlongToPtr(CtrlEvent), 0, NULL);
                if (NULL == Thread)
                {
                    Status = RtlGetLastNtStatus();
                    DPRINT1("Failed thread creation, Status = 0x%08lx\n", Status);
                }
                else
                {
                    DPRINT("ProcessData->CtrlDispatcher remote thread creation succeeded, ProcessId = %x, Process = 0x%p\n", ProcessData->Process->ClientId.UniqueProcess, ProcessData->Process);
                    WaitForSingleObject(Thread, Timeout);
                }
            }
            _SEH2_FINALLY
            {
                CloseHandle(Thread);
            }
            _SEH2_END;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            DPRINT1("ConSrvConsoleCtrlEventTimeout - Caught an exception, Status = 0x%08lx\n", Status);
        }
        _SEH2_END;
    }

    return Status;
}

NTSTATUS
ConSrvConsoleCtrlEvent(IN ULONG CtrlEvent,
                       IN PCONSOLE_PROCESS_DATA ProcessData)
{
    return ConSrvConsoleCtrlEventTimeout(CtrlEvent, ProcessData, 0);
}

PCONSOLE_PROCESS_DATA NTAPI
ConSrvGetConsoleLeaderProcess(IN PCONSRV_CONSOLE Console)
{
    if (Console == NULL) return NULL;

    return CONTAINING_RECORD(Console->ProcessList.Blink,
                             CONSOLE_PROCESS_DATA,
                             ConsoleLink);
}

NTSTATUS NTAPI
ConSrvGetConsoleProcessList(IN PCONSRV_CONSOLE Console,
                            IN OUT PULONG ProcessIdsList,
                            IN ULONG MaxIdListItems,
                            OUT PULONG ProcessIdsTotal)
{
    PCONSOLE_PROCESS_DATA current;
    PLIST_ENTRY current_entry;

    if (Console == NULL || ProcessIdsList == NULL || ProcessIdsTotal == NULL)
        return STATUS_INVALID_PARAMETER;

    *ProcessIdsTotal = 0;

    for (current_entry = Console->ProcessList.Flink;
         current_entry != &Console->ProcessList;
         current_entry = current_entry->Flink)
    {
        current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
        if (++(*ProcessIdsTotal) <= MaxIdListItems)
        {
            *ProcessIdsList++ = HandleToUlong(current->Process->ClientId.UniqueProcess);
        }
    }

    return STATUS_SUCCESS;
}

// ConSrvGenerateConsoleCtrlEvent
NTSTATUS NTAPI
ConSrvConsoleProcessCtrlEvent(IN PCONSRV_CONSOLE Console,
                              IN ULONG ProcessGroupId,
                              IN ULONG CtrlEvent)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY current_entry;
    PCONSOLE_PROCESS_DATA current;

    /* If the console is already being destroyed, just return */
    if (!ConDrvValidateConsoleState(Console, CONSOLE_RUNNING))
        return STATUS_UNSUCCESSFUL;

    /*
     * Loop through the process list, from the most recent process
     * (the active one) to the oldest one (the first created, i.e.
     * the console leader process), and for each, send an event
     * (new processes are inserted at the head of the console process list).
     */
    current_entry = Console->ProcessList.Flink;
    while (current_entry != &Console->ProcessList)
    {
        current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
        current_entry = current_entry->Flink;

        /*
         * Only processes belonging to the same process group are signaled.
         * If the process group ID is zero, then all the processes are signaled.
         */
        if (ProcessGroupId == 0 || current->Process->ProcessGroupId == ProcessGroupId)
        {
            Status = ConSrvConsoleCtrlEvent(CtrlEvent, current);
        }
    }

    return Status;
}





/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvAllocConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_ALLOCCONSOLE AllocConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.AllocConsoleRequest;
    PCSR_PROCESS CsrProcess = CsrGetClientThread()->Process;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    if (ProcessData->ConsoleHandle != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&AllocConsoleRequest->ConsoleStartInfo,
                                  1,
                                  sizeof(CONSOLE_START_INFO)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize a new Console owned by the Console Leader Process */
    Status = ConSrvAllocateConsole(ProcessData,
                                   &AllocConsoleRequest->InputHandle,
                                   &AllocConsoleRequest->OutputHandle,
                                   &AllocConsoleRequest->ErrorHandle,
                                   AllocConsoleRequest->ConsoleStartInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console allocation failed\n");
        return Status;
    }

    /* Return the console handle and the input wait handle to the caller */
    AllocConsoleRequest->ConsoleHandle   = ProcessData->ConsoleHandle;
    AllocConsoleRequest->InputWaitHandle = ProcessData->InputWaitHandle;

    /* Set the Property-Dialog and Control-Dispatcher handlers */
    ProcessData->PropDispatcher = AllocConsoleRequest->PropDispatcher;
    ProcessData->CtrlDispatcher = AllocConsoleRequest->CtrlDispatcher;

    return STATUS_SUCCESS;
}

CSR_API(SrvAttachConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_ATTACHCONSOLE AttachConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.AttachConsoleRequest;
    PCSR_PROCESS SourceProcess = NULL;  // The parent process.
    PCSR_PROCESS TargetProcess = CsrGetClientThread()->Process; // Ourselves.
    HANDLE ProcessId = ULongToHandle(AttachConsoleRequest->ProcessId);
    PCONSOLE_PROCESS_DATA SourceProcessData, TargetProcessData;

    TargetProcessData = ConsoleGetPerProcessData(TargetProcess);

    if (TargetProcessData->ConsoleHandle != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check whether we try to attach to the parent's console */
    if (ProcessId == ULongToHandle(ATTACH_PARENT_PROCESS))
    {
        PROCESS_BASIC_INFORMATION ProcessInfo;
        ULONG Length = sizeof(ProcessInfo);

        /* Get the real parent's ID */

        Status = NtQueryInformationProcess(TargetProcess->ProcessHandle,
                                           ProcessBasicInformation,
                                           &ProcessInfo,
                                           Length, &Length);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SrvAttachConsole - Cannot retrieve basic process info, Status = %lu\n", Status);
            return Status;
        }

        ProcessId = ULongToHandle(ProcessInfo.InheritedFromUniqueProcessId);
    }

    /* Lock the source process via its PID */
    Status = CsrLockProcessByClientId(ProcessId, &SourceProcess);
    if (!NT_SUCCESS(Status)) return Status;

    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

    if (SourceProcessData->ConsoleHandle == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    /*
     * Inherit the console from the parent,
     * if any, otherwise return an error.
     */
    Status = ConSrvInheritConsole(TargetProcessData,
                                  SourceProcessData->ConsoleHandle,
                                  TRUE,
                                  &AttachConsoleRequest->InputHandle,
                                  &AttachConsoleRequest->OutputHandle,
                                  &AttachConsoleRequest->ErrorHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console inheritance failed\n");
        goto Quit;
    }

    /* Return the console handle and the input wait handle to the caller */
    AttachConsoleRequest->ConsoleHandle   = TargetProcessData->ConsoleHandle;
    AttachConsoleRequest->InputWaitHandle = TargetProcessData->InputWaitHandle;

    /* Set the Property-Dialog and Control-Dispatcher handlers */
    TargetProcessData->PropDispatcher = AttachConsoleRequest->PropDispatcher;
    TargetProcessData->CtrlDispatcher = AttachConsoleRequest->CtrlDispatcher;

    Status = STATUS_SUCCESS;

Quit:
    /* Unlock the "source" process and exit */
    CsrUnlockProcess(SourceProcess);
    return Status;
}

CSR_API(SrvFreeConsole)
{
    ConSrvRemoveConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process));
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     OUT PULONG ConsoleMode);
CSR_API(SrvGetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE_IO_OBJECT Object;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->Handle,
                             &Object, NULL, GENERIC_READ, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleMode(Object->Console, Object,
                                  &ConsoleModeRequest->Mode);

    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     IN ULONG ConsoleMode);
CSR_API(SrvSetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE_IO_OBJECT Object;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->Handle,
                             &Object, NULL, GENERIC_WRITE, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleMode(Object->Console, Object,
                                  ConsoleModeRequest->Mode);

    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleTitle(IN PCONSOLE Console,
                      IN BOOLEAN Unicode,
                      IN OUT PVOID TitleBuffer,
                      IN OUT PULONG BufLength);
CSR_API(SrvGetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    PCONSRV_CONSOLE Console;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console\n");
        return Status;
    }

    Status = ConDrvGetConsoleTitle(Console,
                                   TitleRequest->Unicode,
                                   TitleRequest->Title,
                                   &TitleRequest->Length);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleTitle(IN PCONSOLE Console,
                      IN BOOLEAN Unicode,
                      IN PVOID TitleBuffer,
                      IN ULONG BufLength);
CSR_API(SrvSetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    PCONSRV_CONSOLE Console;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console\n");
        return Status;
    }

    Status = ConDrvSetConsoleTitle(Console,
                                   TitleRequest->Unicode,
                                   TitleRequest->Title,
                                   TitleRequest->Length);
    if (NT_SUCCESS(Status)) TermChangeTitle(Console);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleCP(IN PCONSOLE Console,
                   OUT PUINT CodePage,
                   IN BOOLEAN OutputCP);
CSR_API(SrvGetConsoleCP)
{
    NTSTATUS Status;
    PCONSOLE_GETINPUTOUTPUTCP GetConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetConsoleCPRequest;
    PCONSRV_CONSOLE Console;

    DPRINT("SrvGetConsoleCP, getting %s Code Page\n",
            GetConsoleCPRequest->OutputCP ? "Output" : "Input");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleCP(Console,
                                &GetConsoleCPRequest->CodePage,
                                GetConsoleCPRequest->OutputCP);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleCP(IN PCONSOLE Console,
                   IN UINT CodePage,
                   IN BOOLEAN OutputCP);
CSR_API(SrvSetConsoleCP)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PCONSOLE_SETINPUTOUTPUTCP SetConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetConsoleCPRequest;
    PCONSRV_CONSOLE Console;

    DPRINT("SrvSetConsoleCP, setting %s Code Page\n",
            SetConsoleCPRequest->OutputCP ? "Output" : "Input");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleCP(Console,
                                SetConsoleCPRequest->CodePage,
                                SetConsoleCPRequest->OutputCP);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleProcessList)
{
    NTSTATUS Status;
    PCONSOLE_GETPROCESSLIST GetProcessListRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetProcessListRequest;
    PCONSRV_CONSOLE Console;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetProcessListRequest->ProcessIdsList,
                                  GetProcessListRequest->ProcessCount,
                                  sizeof(DWORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConSrvGetConsoleProcessList(Console,
                                         GetProcessListRequest->ProcessIdsList,
                                         GetProcessListRequest->ProcessCount,
                                         &GetProcessListRequest->ProcessCount);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGenerateConsoleCtrlEvent)
{
    NTSTATUS Status;
    PCONSOLE_GENERATECTRLEVENT GenerateCtrlEventRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GenerateCtrlEventRequest;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConSrvConsoleProcessCtrlEvent(Console,
                                           GenerateCtrlEventRequest->ProcessGroupId,
                                           GenerateCtrlEventRequest->CtrlEvent);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvConsoleNotifyLastClose)
{
    NTSTATUS Status;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Only one process is allowed to be registered for last close notification */
    if (!Console->NotifyLastClose)
    {
        Console->NotifyLastClose = TRUE;
        Console->NotifiedLastCloseProcess = ProcessData;
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_ACCESS_DENIED;
    }

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}



CSR_API(SrvGetConsoleMouseInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETMOUSEINFO GetMouseInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetMouseInfoRequest;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Just retrieve the number of buttons of the mouse attached to this console */
    GetMouseInfoRequest->NumButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleKeyShortcuts)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvGetConsoleKeyboardLayoutName)
{
    NTSTATUS Status;
    PCONSOLE_GETKBDLAYOUTNAME GetKbdLayoutNameRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetKbdLayoutNameRequest;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Retrieve the keyboard layout name of the system */
    if (GetKbdLayoutNameRequest->Ansi)
        GetKeyboardLayoutNameA((PCHAR)GetKbdLayoutNameRequest->LayoutBuffer);
    else
        GetKeyboardLayoutNameW((PWCHAR)GetKbdLayoutNameRequest->LayoutBuffer);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleCharType)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvSetConsoleLocalEUDC)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvSetConsoleCursorMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvGetConsoleCursorMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvGetConsoleNlsMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvSetConsoleNlsMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvGetConsoleLangId)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

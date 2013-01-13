/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/console.c
 * PURPOSE:         Console I/O functions
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include "consrv.h"
#include "guiconsole.h"
#include "tuiconsole.h"

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL
DtbgIsDesktopVisible(VOID)
{
    HWND VisibleDesktopWindow = GetDesktopWindow(); // DESKTOPWNDPROC

    if (VisibleDesktopWindow != NULL &&
            !IsWindowVisible(VisibleDesktopWindow))
    {
        VisibleDesktopWindow = NULL;
    }

    return VisibleDesktopWindow != NULL;
}

NTSTATUS FASTCALL
ConioConsoleFromProcessData(PCONSOLE_PROCESS_DATA ProcessData,
                            PCONSOLE *Console)
{
    PCONSOLE ProcessConsole;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    ProcessConsole = ProcessData->Console;

    if (!ProcessConsole)
    {
        *Console = NULL;
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    InterlockedIncrement(&ProcessConsole->ReferenceCount);
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    EnterCriticalSection(&(ProcessConsole->Lock));
    *Console = ProcessConsole;

    return STATUS_SUCCESS;
}

VOID FASTCALL
ConioConsoleCtrlEventTimeout(DWORD Event,
                             PCONSOLE_PROCESS_DATA ProcessData,
                             DWORD Timeout)
{
    HANDLE Thread;

    DPRINT("ConioConsoleCtrlEvent Parent ProcessId = %x\n", ProcessData->Process->ClientId.UniqueProcess);

    if (ProcessData->CtrlDispatcher)
    {
        Thread = CreateRemoteThread(ProcessData->Process->ProcessHandle, NULL, 0,
                                    ProcessData->CtrlDispatcher,
                                    UlongToPtr(Event), 0, NULL);
        if (NULL == Thread)
        {
            DPRINT1("Failed thread creation (Error: 0x%x)\n", GetLastError());
            return;
        }

        DPRINT1("We succeeded at creating ProcessData->CtrlDispatcher remote thread, ProcessId = %x, Process = 0x%p\n", ProcessData->Process->ClientId.UniqueProcess, ProcessData->Process);
        WaitForSingleObject(Thread, Timeout);
        CloseHandle(Thread);
    }
}

VOID FASTCALL
ConioConsoleCtrlEvent(DWORD Event, PCONSOLE_PROCESS_DATA ProcessData)
{
    ConioConsoleCtrlEventTimeout(Event, ProcessData, 0);
}

NTSTATUS WINAPI
CsrInitConsole(PCONSOLE* NewConsole, int ShowCmd, PCSR_PROCESS ConsoleLeaderProcess)
{
    NTSTATUS Status;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER NewBuffer;
    BOOL GuiMode;
    WCHAR Title[255];

    if (NewConsole == NULL) return STATUS_INVALID_PARAMETER;

    *NewConsole = NULL;

    /* Allocate a console structure */
    Console = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY, sizeof(CONSOLE));
    if (NULL == Console)
    {
        DPRINT1("Not enough memory for console creation.\n");
        return STATUS_NO_MEMORY;
    }

    /* Initialize the console */
    Console->Title.MaximumLength = Console->Title.Length = 0;
    Console->Title.Buffer = NULL;

    if (LoadStringW(ConSrvDllInstance, IDS_COMMAND_PROMPT, Title, sizeof(Title) / sizeof(Title[0])))
    {
        RtlCreateUnicodeString(&Console->Title, Title);
    }
    else
    {
        RtlCreateUnicodeString(&Console->Title, L"Command Prompt");
    }

    Console->ReferenceCount = 0;
    Console->LineBuffer = NULL;
    Console->Header.Type = CONIO_CONSOLE_MAGIC;
    Console->Header.Console = Console;
    Console->Mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    Console->ConsoleLeaderCID = ConsoleLeaderProcess->ClientId;
    InitializeListHead(&Console->ProcessList);
    InitializeListHead(&Console->BufferList);
    Console->ActiveBuffer = NULL;
    InitializeListHead(&Console->ReadWaitQueue);
    InitializeListHead(&Console->WriteWaitQueue);
    InitializeListHead(&Console->InputEvents);
    InitializeListHead(&Console->HistoryBuffers);
    Console->CodePage = GetOEMCP();
    Console->OutputCodePage = GetOEMCP();

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = TRUE;

    Console->ActiveEvent = CreateEventW(&SecurityAttributes, TRUE, FALSE, NULL);
    if (NULL == Console->ActiveEvent)
    {
        RtlFreeUnicodeString(&Console->Title);
        RtlFreeHeap(ConSrvHeap, 0, Console);
        return STATUS_UNSUCCESSFUL;
    }
    Console->PrivateData = NULL;
    InitializeCriticalSection(&Console->Lock);

    GuiMode = DtbgIsDesktopVisible();

    /* allocate console screen buffer */
    NewBuffer = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY, sizeof(CONSOLE_SCREEN_BUFFER));
    if (NULL == NewBuffer)
    {
        RtlFreeUnicodeString(&Console->Title);
        DeleteCriticalSection(&Console->Lock);
        CloseHandle(Console->ActiveEvent);
        RtlFreeHeap(ConSrvHeap, 0, Console);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    /* init screen buffer with defaults */
    NewBuffer->CursorInfo.bVisible = TRUE;
    NewBuffer->CursorInfo.dwSize = CSR_DEFAULT_CURSOR_SIZE;
    /* make console active, and insert into console list */
    Console->ActiveBuffer = (PCONSOLE_SCREEN_BUFFER) NewBuffer;

    /*
     * If we are not in GUI-mode, start the text-mode console. If we fail,
     * try to start the GUI-mode console (win32k will automatically switch
     * to graphical mode, therefore no additional code is needed).
     */
    if (!GuiMode)
    {
        DPRINT1("CONSRV: Opening text-mode console\n");
        Status = TuiInitConsole(Console);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open text-mode console, switching to gui-mode, Status = 0x%08lx\n", Status);
            GuiMode = TRUE;
        }
    }

    /*
     * Try to open the GUI-mode console. Two cases are possible:
     * - We are in GUI-mode, therefore GuiMode == TRUE, the previous test-case
     *   failed and we start GUI-mode console.
     * - We are in text-mode, therefore GuiMode == FALSE, the previous test-case
     *   succeeded BUT we failed at starting text-mode console. Then GuiMode
     *   was switched to TRUE in order to try to open the console in GUI-mode.
     */
    if (GuiMode)
    {
        DPRINT1("CONSRV: Opening GUI-mode console\n");
        Status = GuiInitConsole(Console, ShowCmd);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(ConSrvHeap,0, NewBuffer);
            RtlFreeUnicodeString(&Console->Title);
            DeleteCriticalSection(&Console->Lock);
            CloseHandle(Console->ActiveEvent);
            DPRINT1("GuiInitConsole: failed, Status = 0x%08lx\n", Status);
            RtlFreeHeap(ConSrvHeap, 0, Console);
            return Status;
        }
    }

    Status = CsrInitConsoleScreenBuffer(Console, NewBuffer);
    if (!NT_SUCCESS(Status))
    {
        ConioCleanupConsole(Console);
        RtlFreeUnicodeString(&Console->Title);
        DeleteCriticalSection(&Console->Lock);
        CloseHandle(Console->ActiveEvent);
        RtlFreeHeap(ConSrvHeap, 0, NewBuffer);
        DPRINT1("CsrInitConsoleScreenBuffer: failed\n");
        RtlFreeHeap(ConSrvHeap, 0, Console);
        return Status;
    }

    /* Copy buffer contents to screen */
    ConioDrawConsole(Console);

    *NewConsole = Console;

    return STATUS_SUCCESS;
}

CSR_API(SrvOpenConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_OPENCONSOLE OpenConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.OpenConsoleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);

    DPRINT("SrvOpenConsole\n");

    OpenConsoleRequest->ConsoleHandle = INVALID_HANDLE_VALUE;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    DPRINT1("SrvOpenConsole - Checkpoint 1\n");
    DPRINT1("ProcessData = 0x%p ; ProcessData->Console = 0x%p\n", ProcessData, ProcessData->Console);

    if (ProcessData->Console)
    {
        DWORD DesiredAccess = OpenConsoleRequest->Access;
        DWORD ShareMode = OpenConsoleRequest->ShareMode;

        PCONSOLE Console = ProcessData->Console;
        Object_t *Object;

        DPRINT1("SrvOpenConsole - Checkpoint 2\n");
        EnterCriticalSection(&Console->Lock);
        DPRINT1("SrvOpenConsole - Checkpoint 3\n");

        if (OpenConsoleRequest->HandleType == HANDLE_OUTPUT)
            Object = &Console->ActiveBuffer->Header;
        else // HANDLE_INPUT
            Object = &Console->Header;

        if (((DesiredAccess & GENERIC_READ)  && Object->ExclusiveRead  != 0) ||
            ((DesiredAccess & GENERIC_WRITE) && Object->ExclusiveWrite != 0) ||
            (!(ShareMode & FILE_SHARE_READ)  && Object->AccessRead     != 0) ||
            (!(ShareMode & FILE_SHARE_WRITE) && Object->AccessWrite    != 0))
        {
            DPRINT1("Sharing violation\n");
            Status = STATUS_SHARING_VIOLATION;
        }
        else
        {
            Status = Win32CsrInsertObject(ProcessData,
                                          &OpenConsoleRequest->ConsoleHandle,
                                          Object,
                                          DesiredAccess,
                                          OpenConsoleRequest->Inheritable,
                                          ShareMode);
        }

        LeaveCriticalSection(&Console->Lock);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

CSR_API(SrvAllocConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_ALLOCCONSOLE AllocConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.AllocConsoleRequest;
    PCSR_PROCESS ConsoleLeader = CsrGetClientThread()->Process;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(ConsoleLeader);

    DPRINT("SrvAllocConsole\n");

    if (ProcessData->Console != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

/******************************************************************************/
/** This comes from ConsoleConnect!!                                         **/
    DPRINT1("SrvAllocConsole - Checkpoint 1\n");

#if 0000
    /*
     * We are about to create a new console. However when ConsoleNewProcess
     * was called, we didn't know that we wanted to create a new console and
     * therefore, we by default inherited the handles table from our parent
     * process. It's only now that we notice that in fact we do not need
     * them, because we've created a new console and thus we must use it.
     *
     * Therefore, free the console we can have and our handles table,
     * and recreate a new one later on.
     */
    Win32CsrReleaseConsole(ProcessData);
    // Win32CsrFreeHandlesTable(ProcessData);
#endif

    /* Initialize a new Console owned by the Console Leader Process */
    Status = CsrInitConsole(&ProcessData->Console, AllocConsoleRequest->ShowCmd, ConsoleLeader);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console initialization failed\n");
        return Status;
    }

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&ProcessData->Console->ReferenceCount);

    /* Insert the process into the processes list of the console */
    InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ConsoleLink);

    /* Return it to the caller */
    AllocConsoleRequest->Console = ProcessData->Console;


    /*
     * Create a new handle table - Insert the IO handles
     */

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    /* Insert the Input handle */
    Status = Win32CsrInsertObject(ProcessData,
                                  &AllocConsoleRequest->InputHandle,
                                  &ProcessData->Console->Header,
                                  GENERIC_READ | GENERIC_WRITE,
                                  TRUE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the input handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        Win32CsrReleaseConsole(ProcessData);
        // ConioDeleteConsole(ProcessData->Console);
        // ProcessData->Console = NULL;
        return Status;
    }

    /* Insert the Output handle */
    Status = Win32CsrInsertObject(ProcessData,
                                  &AllocConsoleRequest->OutputHandle,
                                  &ProcessData->Console->ActiveBuffer->Header,
                                  GENERIC_READ | GENERIC_WRITE,
                                  TRUE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the output handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        Win32CsrReleaseConsole(ProcessData);
        // Win32CsrReleaseObject(ProcessData,
                              // AllocConsoleRequest->InputHandle);
        // ConioDeleteConsole(ProcessData->Console);
        // ProcessData->Console = NULL;
        return Status;
    }

    /* Insert the Error handle */
    Status = Win32CsrInsertObject(ProcessData,
                                  &AllocConsoleRequest->ErrorHandle,
                                  &ProcessData->Console->ActiveBuffer->Header,
                                  GENERIC_READ | GENERIC_WRITE,
                                  TRUE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the error handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        Win32CsrReleaseConsole(ProcessData);
        // Win32CsrReleaseObject(ProcessData,
                              // AllocConsoleRequest->OutputHandle);
        // Win32CsrReleaseObject(ProcessData,
                              // AllocConsoleRequest->InputHandle);
        // ConioDeleteConsole(ProcessData->Console);
        // ProcessData->Console = NULL;
        return Status;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    /* Duplicate the Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               ProcessData->Console->ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        Win32CsrReleaseConsole(ProcessData);
        // if (NewConsole)
        // {
            // Win32CsrReleaseObject(ProcessData,
                                  // AllocConsoleRequest->ErrorHandle);
            // Win32CsrReleaseObject(ProcessData,
                                  // AllocConsoleRequest->OutputHandle);
            // Win32CsrReleaseObject(ProcessData,
                                  // AllocConsoleRequest->InputHandle);
        // }
        // ConioDeleteConsole(ProcessData->Console); // FIXME: Just release the console ?
        // ProcessData->Console = NULL;
        return Status;
    }
    /* Input Wait Handle */
    AllocConsoleRequest->InputWaitHandle = ProcessData->ConsoleEvent;

    /* Set the Ctrl Dispatcher */
    ProcessData->CtrlDispatcher = AllocConsoleRequest->CtrlDispatcher;
    DPRINT("CONSRV: CtrlDispatcher address: %x\n", ProcessData->CtrlDispatcher);
/******************************************************************************/

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

    DPRINT("SrvAttachConsole\n");

    TargetProcessData = ConsoleGetPerProcessData(TargetProcess);

    if (TargetProcessData->Console != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

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

        DPRINT("We, process (ID) %lu;%lu\n", TargetProcess->ClientId.UniqueProcess, TargetProcess->ClientId.UniqueThread);
        ProcessId = ULongToHandle(ProcessInfo.InheritedFromUniqueProcessId);
        DPRINT("Parent process ID = %lu\n", ProcessId);
    }

    /* Lock the target process via its PID */
    DPRINT1("Lock process Id %lu\n", ProcessId);
    Status = CsrLockProcessByClientId(ProcessId, &SourceProcess);
    DPRINT1("Lock process Status %lu\n", Status);
    if (!NT_SUCCESS(Status)) return Status;
    DPRINT1("AttachConsole OK\n");

/******************************************************************************/
/** This comes from ConsoleNewProcess!!                                      **/
    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

    /*
     * Inherit the console from the parent,
     * if any, otherwise return an error.
     */
    DPRINT1("SourceProcessData->Console = 0x%p\n", SourceProcessData->Console);
    if (SourceProcessData->Console == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }
    TargetProcessData->Console = SourceProcessData->Console;

    DPRINT1("SrvAttachConsole - Copy the handle table (1)\n");
    Status = Win32CsrInheritHandlesTable(SourceProcessData, TargetProcessData);
    DPRINT1("SrvAttachConsole - Copy the handle table (2)\n");
    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

/******************************************************************************/

/******************************************************************************/
/** This comes from ConsoleConnect / SrvAllocConsole!!                       **/
    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&TargetProcessData->Console->ReferenceCount);

    /* Insert the process into the processes list of the console */
    InsertHeadList(&TargetProcessData->Console->ProcessList, &TargetProcessData->ConsoleLink);

    /* Return it to the caller */
    AttachConsoleRequest->Console = TargetProcessData->Console;

    /** Here, we inherited the console handles from the "source" process,
     ** so no need to reinitialize the handles table. **/

    DPRINT1("SrvAttachConsole - Checkpoint\n");

    /* Duplicate the Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               TargetProcessData->Console->ActiveEvent,
                               TargetProcessData->Process->ProcessHandle,
                               &TargetProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        Win32CsrReleaseConsole(TargetProcessData);
// #if 0
        // if (NewConsole)
        // {
            // Win32CsrReleaseObject(TargetProcessData,
                                  // AttachConsoleRequest->ErrorHandle);
            // Win32CsrReleaseObject(TargetProcessData,
                                  // AttachConsoleRequest->OutputHandle);
            // Win32CsrReleaseObject(TargetProcessData,
                                  // AttachConsoleRequest->InputHandle);
        // }
// #endif
        // ConioDeleteConsole(TargetProcessData->Console); // FIXME: Just release the console ?
        // TargetProcessData->Console = NULL;
        goto Quit;
    }
    /* Input Wait Handle */
    AttachConsoleRequest->InputWaitHandle = TargetProcessData->ConsoleEvent;

    /* Set the Ctrl Dispatcher */
    TargetProcessData->CtrlDispatcher = AttachConsoleRequest->CtrlDispatcher;
    DPRINT("CONSRV: CtrlDispatcher address: %x\n", TargetProcessData->CtrlDispatcher);

    Status = STATUS_SUCCESS;
/******************************************************************************/

Quit:
    DPRINT1("SrvAttachConsole - exiting 1\n");
    /* Unlock the "source" process */
    CsrUnlockProcess(SourceProcess);
    DPRINT1("SrvAttachConsole - exiting 2\n");

    return Status;
}

CSR_API(SrvFreeConsole)
{
    DPRINT1("SrvFreeConsole\n");
    Win32CsrReleaseConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process));
    return STATUS_SUCCESS;
}

VOID WINAPI
ConioDeleteConsole(PCONSOLE Console)
{
    ConsoleInput *Event;

    DPRINT("ConioDeleteConsole\n");

    /* Drain input event queue */
    while (Console->InputEvents.Flink != &Console->InputEvents)
    {
        Event = (ConsoleInput *) Console->InputEvents.Flink;
        Console->InputEvents.Flink = Console->InputEvents.Flink->Flink;
        Console->InputEvents.Flink->Flink->Blink = &Console->InputEvents;
        RtlFreeHeap(ConSrvHeap, 0, Event);
    }

    ConioCleanupConsole(Console);
    if (Console->LineBuffer)
        RtlFreeHeap(ConSrvHeap, 0, Console->LineBuffer);
    while (!IsListEmpty(&Console->HistoryBuffers))
        HistoryDeleteBuffer((struct _HISTORY_BUFFER *)Console->HistoryBuffers.Flink);

    ConioDeleteScreenBuffer(Console->ActiveBuffer);
    if (!IsListEmpty(&Console->BufferList))
    {
        DPRINT1("BUG: screen buffer list not empty\n");
    }

    CloseHandle(Console->ActiveEvent);
    if (Console->UnpauseEvent) CloseHandle(Console->UnpauseEvent);
    DeleteCriticalSection(&Console->Lock);
    RtlFreeUnicodeString(&Console->Title);
    IntDeleteAllAliases(Console->Aliases);
    RtlFreeHeap(ConSrvHeap, 0, Console);
}

VOID WINAPI
CsrInitConsoleSupport(VOID)
{
    DPRINT("CSR: CsrInitConsoleSupport()\n");

    /* Should call LoadKeyboardLayout */
}

VOID FASTCALL
ConioPause(PCONSOLE Console, UINT Flags)
{
    Console->PauseFlags |= Flags;
    if (!Console->UnpauseEvent)
        Console->UnpauseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

VOID FASTCALL
ConioUnpause(PCONSOLE Console, UINT Flags)
{
    Console->PauseFlags &= ~Flags;

    // if ((Console->PauseFlags & (PAUSED_FROM_KEYBOARD | PAUSED_FROM_SCROLLBAR | PAUSED_FROM_SELECTION)) == 0)
    if (Console->PauseFlags == 0 && Console->UnpauseEvent)
    {
        SetEvent(Console->UnpauseEvent);
        CloseHandle(Console->UnpauseEvent);
        Console->UnpauseEvent = NULL;

        CsrNotifyWait(&Console->WriteWaitQueue,
                      WaitAll,
                      NULL,
                      NULL);
    }
}

CSR_API(SrvSetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvSetConsoleMode\n");

    Status = Win32CsrLockObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                ConsoleModeRequest->ConsoleHandle,
                                (Object_t **) &Console, GENERIC_WRITE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Buff = (PCONSOLE_SCREEN_BUFFER)Console;

    if (CONIO_CONSOLE_MAGIC == Console->Header.Type)
    {
        Console->Mode = ConsoleModeRequest->ConsoleMode & CONSOLE_INPUT_MODE_VALID;
    }
    else if (CONIO_SCREEN_BUFFER_MAGIC == Console->Header.Type)
    {
        Buff->Mode = ConsoleModeRequest->ConsoleMode & CONSOLE_OUTPUT_MODE_VALID;
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

    Win32CsrUnlockObject((Object_t *)Console);

    return Status;
}

CSR_API(SrvGetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvGetConsoleMode\n");

    Status = Win32CsrLockObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                ConsoleModeRequest->ConsoleHandle,
                                (Object_t **) &Console, GENERIC_READ, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = STATUS_SUCCESS;
    Buff = (PCONSOLE_SCREEN_BUFFER) Console;

    if (CONIO_CONSOLE_MAGIC == Console->Header.Type)
    {
        ConsoleModeRequest->ConsoleMode = Console->Mode;
    }
    else if (CONIO_SCREEN_BUFFER_MAGIC == Buff->Header.Type)
    {
        ConsoleModeRequest->ConsoleMode = Buff->Mode;
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

    Win32CsrUnlockObject((Object_t *)Console);
    return Status;
}

CSR_API(SrvSetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCONSOLE Console;
    PWCHAR Buffer;

    DPRINT("SrvSetConsoleTitle\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if(NT_SUCCESS(Status))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, TitleRequest->Length);
        if (Buffer)
        {
            /* Copy title to console */
            RtlFreeUnicodeString(&Console->Title);
            Console->Title.Buffer = Buffer;
            Console->Title.Length = Console->Title.MaximumLength = TitleRequest->Length;
            memcpy(Console->Title.Buffer, TitleRequest->Title, Console->Title.Length);

            if (!ConioChangeTitle(Console))
            {
                Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }

        ConioUnlockConsole(Console);
    }

    return Status;
}

CSR_API(SrvGetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCONSOLE Console;
    DWORD Length;

    DPRINT("SrvGetConsoleTitle\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console\n");
        return Status;
    }

    /* Copy title of the console to the user title buffer */
    if (TitleRequest->Length >= sizeof(WCHAR))
    {
        Length = min(TitleRequest->Length - sizeof(WCHAR), Console->Title.Length);
        memcpy(TitleRequest->Title, Console->Title.Buffer, Length);
        TitleRequest->Title[Length / sizeof(WCHAR)] = L'\0';
    }

    TitleRequest->Length = Console->Title.Length;

    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

/**********************************************************************
 *  HardwareStateProperty
 *
 *  DESCRIPTION
 *      Set/Get the value of the HardwareState and switch
 *      between direct video buffer ouput and GDI windowed
 *      output.
 *  ARGUMENTS
 *      Client hands us a CONSOLE_GETSETHWSTATE object.
 *      We use the same object to Request.
 *  NOTE
 *      ConsoleHwState has the correct size to be compatible
 *      with NT's, but values are not.
 */
static NTSTATUS FASTCALL
SetConsoleHardwareState(PCONSOLE Console, DWORD ConsoleHwState)
{
    DPRINT1("Console Hardware State: %d\n", ConsoleHwState);

    if ((CONSOLE_HARDWARE_STATE_GDI_MANAGED == ConsoleHwState)
            ||(CONSOLE_HARDWARE_STATE_DIRECT == ConsoleHwState))
    {
        if (Console->HardwareState != ConsoleHwState)
        {
            /* TODO: implement switching from full screen to windowed mode */
            /* TODO: or back; now simply store the hardware state */
            Console->HardwareState = ConsoleHwState;
        }

        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER_3; /* Client: (handle, set_get, [mode]) */
}

CSR_API(SrvGetConsoleHardwareState)
{
    NTSTATUS Status;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.HardwareStateRequest;
    PCONSOLE Console;

    DPRINT("SrvGetConsoleHardwareState\n");

    Status = ConioLockConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              HardwareStateRequest->OutputHandle,
                              &Console,
                              GENERIC_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvGetConsoleHardwareState\n");
        return Status;
    }

    HardwareStateRequest->State = Console->HardwareState;

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvSetConsoleHardwareState)
{
    NTSTATUS Status;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.HardwareStateRequest;
    PCONSOLE Console;

    DPRINT("SrvSetConsoleHardwareState\n");

    Status = ConioLockConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              HardwareStateRequest->OutputHandle,
                              &Console,
                              GENERIC_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvSetConsoleHardwareState\n");
        return Status;
    }

    DPRINT("Setting console hardware state.\n");
    Status = SetConsoleHardwareState(Console, HardwareStateRequest->State);

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvGetConsoleWindow)
{
    NTSTATUS Status;
    PCONSOLE_GETWINDOW GetWindowRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetWindowRequest;
    PCONSOLE Console;

    DPRINT("SrvGetConsoleWindow\n");

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    GetWindowRequest->WindowHandle = Console->hWindow;
    ConioUnlockConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleIcon)
{
    NTSTATUS Status;
    PCONSOLE_SETICON SetIconRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetIconRequest;
    PCONSOLE Console;

    DPRINT("SrvSetConsoleIcon\n");

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    Status = (ConioChangeIcon(Console, SetIconRequest->WindowIcon)
                ? STATUS_SUCCESS
                : STATUS_UNSUCCESSFUL);

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvGetConsoleCP)
{
    NTSTATUS Status;
    PCONSOLE_GETSETINPUTOUTPUTCP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCONSOLE Console;

    DPRINT("SrvGetConsoleCP, getting %s Code Page\n",
            ConsoleCPRequest->InputCP ? "Input" : "Output");

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    ConsoleCPRequest->CodePage = (ConsoleCPRequest->InputCP ? Console->CodePage
                                                            : Console->OutputCodePage);
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleCP)
{
    NTSTATUS Status;
    PCONSOLE_GETSETINPUTOUTPUTCP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCONSOLE Console;

    DPRINT("SrvSetConsoleCP, setting %s Code Page\n",
            ConsoleCPRequest->InputCP ? "Input" : "Output");

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    if (IsValidCodePage(ConsoleCPRequest->CodePage))
    {
        if (ConsoleCPRequest->InputCP)
            Console->CodePage = ConsoleCPRequest->CodePage;
        else
            Console->OutputCodePage = ConsoleCPRequest->CodePage;

        ConioUnlockConsole(Console);
        return STATUS_SUCCESS;
    }

    ConioUnlockConsole(Console);
    return STATUS_INVALID_PARAMETER;
}

CSR_API(SrvGetConsoleProcessList)
{
    NTSTATUS Status;
    PCONSOLE_GETPROCESSLIST GetProcessListRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetProcessListRequest;
    PDWORD Buffer;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCONSOLE Console;
    PCONSOLE_PROCESS_DATA current;
    PLIST_ENTRY current_entry;
    ULONG nItems = 0;

    DPRINT("SrvGetConsoleProcessList\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetProcessListRequest->pProcessIds,
                                  GetProcessListRequest->nMaxIds,
                                  sizeof(DWORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Buffer = GetProcessListRequest->pProcessIds;

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    for (current_entry = Console->ProcessList.Flink;
         current_entry != &Console->ProcessList;
         current_entry = current_entry->Flink)
    {
        current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
        if (++nItems <= GetProcessListRequest->nMaxIds)
        {
            *Buffer++ = HandleToUlong(current->Process->ClientId.UniqueProcess);
        }
    }

    ConioUnlockConsole(Console);

    GetProcessListRequest->nProcessIdsTotal = nItems;
    return STATUS_SUCCESS;
}

CSR_API(SrvGenerateConsoleCtrlEvent)
{
    NTSTATUS Status;
    PCONSOLE_GENERATECTRLEVENT GenerateCtrlEventRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GenerateCtrlEventRequest;
    PCONSOLE Console;
    PCONSOLE_PROCESS_DATA current;
    PLIST_ENTRY current_entry;
    DWORD Group;

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    Group = GenerateCtrlEventRequest->ProcessGroup;
    Status = STATUS_INVALID_PARAMETER;
    for (current_entry  = Console->ProcessList.Flink;
         current_entry != &Console->ProcessList;
         current_entry  = current_entry->Flink)
    {
        current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
        if (Group == 0 || current->Process->ProcessGroupId == Group)
        {
            ConioConsoleCtrlEvent(GenerateCtrlEventRequest->Event, current);
            Status = STATUS_SUCCESS;
        }
    }

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvGetConsoleSelectionInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSELECTIONINFO GetSelectionInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetSelectionInfoRequest;
    PCONSOLE Console;

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (NT_SUCCESS(Status))
    {
        memset(&GetSelectionInfoRequest->Info, 0, sizeof(CONSOLE_SELECTION_INFO));
        if (Console->Selection.dwFlags != 0)
            GetSelectionInfoRequest->Info = Console->Selection;
        ConioUnlockConsole(Console);
    }

    return Status;
}

/* EOF */

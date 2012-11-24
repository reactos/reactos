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

#define NDEBUG
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
                            PCSRSS_CONSOLE *Console)
{
    PCSRSS_CONSOLE ProcessConsole;

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

        WaitForSingleObject(Thread, Timeout);
        CloseHandle(Thread);
    }
}

VOID FASTCALL
ConioConsoleCtrlEvent(DWORD Event, PCONSOLE_PROCESS_DATA ProcessData)
{
    ConioConsoleCtrlEventTimeout(Event, ProcessData, 0);
}

static NTSTATUS WINAPI
CsrInitConsole(PCSRSS_CONSOLE Console, int ShowCmd)
{
    NTSTATUS Status;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PCSRSS_SCREEN_BUFFER NewBuffer;
    BOOL GuiMode;
    WCHAR Title[255];

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
    InitializeListHead(&Console->BufferList);
    Console->ActiveBuffer = NULL;
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
        return STATUS_UNSUCCESSFUL;
    }
    Console->PrivateData = NULL;
    InitializeCriticalSection(&Console->Lock);

    GuiMode = DtbgIsDesktopVisible();

    /* allocate console screen buffer */
    NewBuffer = HeapAlloc(ConSrvHeap, HEAP_ZERO_MEMORY, sizeof(CSRSS_SCREEN_BUFFER));
    if (NULL == NewBuffer)
    {
        RtlFreeUnicodeString(&Console->Title);
        DeleteCriticalSection(&Console->Lock);
        CloseHandle(Console->ActiveEvent);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    /* init screen buffer with defaults */
    NewBuffer->CursorInfo.bVisible = TRUE;
    NewBuffer->CursorInfo.dwSize = CSR_DEFAULT_CURSOR_SIZE;
    /* make console active, and insert into console list */
    Console->ActiveBuffer = (PCSRSS_SCREEN_BUFFER) NewBuffer;

    /*
     * If we are not in GUI-mode, start the text-mode console. If we fail,
     * try to start the GUI-mode console (win32k will automatically switch
     * to graphical mode, therefore no additional code is needed).
     */
    if (!GuiMode)
    {
        DPRINT1("WIN32CSR: Opening text-mode console\n");
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
        DPRINT1("WIN32CSR: Opening GUI-mode console\n");
        Status = GuiInitConsole(Console, ShowCmd);
        if (!NT_SUCCESS(Status))
        {
            HeapFree(ConSrvHeap,0, NewBuffer);
            RtlFreeUnicodeString(&Console->Title);
            DeleteCriticalSection(&Console->Lock);
            CloseHandle(Console->ActiveEvent);
            DPRINT1("GuiInitConsole: failed, Status = 0x%08lx\n", Status);
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
        HeapFree(ConSrvHeap, 0, NewBuffer);
        DPRINT1("CsrInitConsoleScreenBuffer: failed\n");
        return Status;
    }

    /* copy buffer contents to screen */
    ConioDrawConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(SrvOpenConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCSRSS_OPEN_CONSOLE OpenConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.OpenConsoleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);

    DPRINT("SrvOpenConsole\n");

    OpenConsoleRequest->Handle = INVALID_HANDLE_VALUE;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    DPRINT1("SrvOpenConsole - Checkpoint 1\n");
    DPRINT1("ProcessData = 0x%p ; ProcessData->Console = 0x%p\n", ProcessData, ProcessData->Console);

    if (ProcessData->Console)
    {
        DWORD DesiredAccess = OpenConsoleRequest->Access;
        DWORD ShareMode = OpenConsoleRequest->ShareMode;

        PCSRSS_CONSOLE Console = ProcessData->Console;
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
                                          &OpenConsoleRequest->Handle,
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
    PCSRSS_ALLOC_CONSOLE AllocConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.AllocConsoleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCSRSS_CONSOLE Console;
    BOOLEAN NewConsole = FALSE;

    DPRINT("SrvAllocConsole\n");

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (ProcessData->Console)
    {
        DPRINT1("Process already has a console\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_PARAMETER;
    }

    DPRINT1("SrvAllocConsole - Checkpoint 1\n");

    /* If we don't need a console, then get out of here */
    if (!AllocConsoleRequest->ConsoleNeeded)
    {
        DPRINT("No console needed\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_SUCCESS;
    }

    /* If we already have one, then don't create a new one... */
    if (!AllocConsoleRequest->Console ||
         AllocConsoleRequest->Console != ProcessData->ParentConsole)
    {
        /* Allocate a console structure */
        NewConsole = TRUE;
        Console = HeapAlloc(ConSrvHeap, HEAP_ZERO_MEMORY, sizeof(CSRSS_CONSOLE));
        if (NULL == Console)
        {
            DPRINT1("Not enough memory for console\n");
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_NO_MEMORY;
        }

        /* Initialize list head */
        InitializeListHead(&Console->ProcessList);

        /* Insert process data required for GUI initialization */
        InsertHeadList(&Console->ProcessList, &ProcessData->ConsoleLink);

        /* Initialize the Console */
        Status = CsrInitConsole(Console, AllocConsoleRequest->ShowCmd);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console init failed\n");
            HeapFree(ConSrvHeap, 0, Console);
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }
    }
    else
    {
        /* Reuse our current console */
        Console = AllocConsoleRequest->Console;
    }

    /* Set the Process Console */
    ProcessData->Console = Console;

    /* Return it to the caller */
    AllocConsoleRequest->Console = Console;

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&Console->ReferenceCount);

    if (NewConsole || !ProcessData->bInheritHandles)
    {
        /* Insert the Objects */
        Status = Win32CsrInsertObject(ProcessData,
                                      &AllocConsoleRequest->InputHandle,
                                      &Console->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            ProcessData->Console = NULL;
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }

        Status = Win32CsrInsertObject(ProcessData,
                                      &AllocConsoleRequest->OutputHandle,
                                      &Console->ActiveBuffer->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            Win32CsrReleaseObject(ProcessData,
                                  AllocConsoleRequest->InputHandle);
            ProcessData->Console = NULL;
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }
    }

    /* Duplicate the Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               ProcessData->Console->ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        ConioDeleteConsole((Object_t *) Console);
        if (NewConsole || !ProcessData->bInheritHandles)
        {
            Win32CsrReleaseObject(ProcessData,
                                  AllocConsoleRequest->OutputHandle);
            Win32CsrReleaseObject(ProcessData,
                                  AllocConsoleRequest->InputHandle);
        }
        ProcessData->Console = NULL;
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return Status;
    }

    /* Set the Ctrl Dispatcher */
    ProcessData->CtrlDispatcher = AllocConsoleRequest->CtrlDispatcher;
    DPRINT("CSRSS:CtrlDispatcher address: %x\n", ProcessData->CtrlDispatcher);

    if (!NewConsole)
    {
        /* Insert into the list if it has not been added */
        InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ConsoleLink);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return STATUS_SUCCESS;
}

CSR_API(SrvFreeConsole)
{
    Win32CsrReleaseConsole(CsrGetClientThread()->Process);
    return STATUS_SUCCESS;
}

VOID WINAPI
ConioDeleteConsole(Object_t *Object)
{
    PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) Object;
    ConsoleInput *Event;

    DPRINT("ConioDeleteConsole\n");

    /* Drain input event queue */
    while (Console->InputEvents.Flink != &Console->InputEvents)
    {
        Event = (ConsoleInput *) Console->InputEvents.Flink;
        Console->InputEvents.Flink = Console->InputEvents.Flink->Flink;
        Console->InputEvents.Flink->Flink->Blink = &Console->InputEvents;
        HeapFree(ConSrvHeap, 0, Event);
    }

    ConioCleanupConsole(Console);
    if (Console->LineBuffer)
        RtlFreeHeap(ConSrvHeap, 0, Console->LineBuffer);
    while (!IsListEmpty(&Console->HistoryBuffers))
        HistoryDeleteBuffer((struct tagHISTORY_BUFFER *)Console->HistoryBuffers.Flink);

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
    HeapFree(ConSrvHeap, 0, Console);
}

VOID WINAPI
CsrInitConsoleSupport(VOID)
{
    DPRINT("CSR: CsrInitConsoleSupport()\n");

    /* Should call LoadKeyboardLayout */
}

VOID FASTCALL
ConioPause(PCSRSS_CONSOLE Console, UINT Flags)
{
    Console->PauseFlags |= Flags;
    if (!Console->UnpauseEvent)
        Console->UnpauseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

VOID FASTCALL
ConioUnpause(PCSRSS_CONSOLE Console, UINT Flags)
{
    Console->PauseFlags &= ~Flags;
    if (Console->PauseFlags == 0 && Console->UnpauseEvent)
    {
        SetEvent(Console->UnpauseEvent);
        CloseHandle(Console->UnpauseEvent);
        Console->UnpauseEvent = NULL;
    }
}

CSR_API(SrvSetConsoleMode)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE_MODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;

    DPRINT("SrvSetConsoleMode\n");

    Status = Win32CsrLockObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                ConsoleModeRequest->ConsoleHandle,
                                (Object_t **) &Console, GENERIC_WRITE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Buff = (PCSRSS_SCREEN_BUFFER)Console;

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
    PCSRSS_CONSOLE_MODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;

    DPRINT("SrvGetConsoleMode\n");

    Status = Win32CsrLockObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                ConsoleModeRequest->ConsoleHandle,
                                (Object_t **) &Console, GENERIC_READ, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = STATUS_SUCCESS;
    Buff = (PCSRSS_SCREEN_BUFFER) Console;

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
    PCSRSS_CONSOLE_TITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCSRSS_CONSOLE Console;
    PWCHAR Buffer;

    DPRINT("SrvSetConsoleTitle\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }
/*
    if (!Win32CsrValidateBuffer(Process, TitleRequest->Title,
                                TitleRequest->Length, 1))
    {
        return STATUS_ACCESS_VIOLATION;
    }
*/

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
    PCSRSS_CONSOLE_TITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCSRSS_CONSOLE Console;
    DWORD Length;

    DPRINT("SrvGetConsoleTitle\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }
/*
    if (!Win32CsrValidateBuffer(Process, TitleRequest->Title,
                                TitleRequest->Length, 1))
    {
        return STATUS_ACCESS_VIOLATION;
    }
*/

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
 *      Client hands us a CSRSS_CONSOLE_HARDWARE_STATE
 *      object. We use the same object to Request.
 *  NOTE
 *      ConsoleHwState has the correct size to be compatible
 *      with NT's, but values are not.
 */
static NTSTATUS FASTCALL
SetConsoleHardwareState(PCSRSS_CONSOLE Console, DWORD ConsoleHwState)
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
    PCSRSS_CONSOLE_HW_STATE ConsoleHardwareStateRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleHardwareStateRequest;
    PCSRSS_CONSOLE Console;

    DPRINT("SrvGetConsoleHardwareState\n");

    Status = ConioLockConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              ConsoleHardwareStateRequest->ConsoleHandle,
                              &Console,
                              GENERIC_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvGetConsoleHardwareState\n");
        return Status;
    }

    ConsoleHardwareStateRequest->State = Console->HardwareState;

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvSetConsoleHardwareState)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE_HW_STATE ConsoleHardwareStateRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleHardwareStateRequest;
    PCSRSS_CONSOLE Console;

    DPRINT("SrvSetConsoleHardwareState\n");

    Status = ConioLockConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              ConsoleHardwareStateRequest->ConsoleHandle,
                              &Console,
                              GENERIC_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvSetConsoleHardwareState\n");
        return Status;
    }

    DPRINT("Setting console hardware state.\n");
    Status = SetConsoleHardwareState(Console, ConsoleHardwareStateRequest->State);

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvGetConsoleWindow)
{
    NTSTATUS Status;
    PCSRSS_GET_CONSOLE_WINDOW GetConsoleWindowRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetConsoleWindowRequest;
    PCSRSS_CONSOLE Console;

    DPRINT("SrvGetConsoleWindow\n");

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    GetConsoleWindowRequest->WindowHandle = Console->hWindow;
    ConioUnlockConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleIcon)
{
    NTSTATUS Status;
    PCSRSS_SET_CONSOLE_ICON SetConsoleIconRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetConsoleIconRequest;
    PCSRSS_CONSOLE Console;

    DPRINT("SrvSetConsoleIcon\n");

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    Status = (ConioChangeIcon(Console, SetConsoleIconRequest->WindowIcon)
                ? STATUS_SUCCESS
                : STATUS_UNSUCCESSFUL);

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvGetConsoleCP)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE_CP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCSRSS_CONSOLE Console;

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
    PCSRSS_CONSOLE_CP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCSRSS_CONSOLE Console;

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
    PCSRSS_GET_PROCESS_LIST GetProcessListRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetProcessListRequest;
    PDWORD Buffer;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCSRSS_CONSOLE Console;
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

/*
    if (!Win32CsrValidateBuffer(ProcessData, Buffer, GetProcessListRequest->nMaxIds, sizeof(DWORD)))
        return STATUS_ACCESS_VIOLATION;
*/

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
    PCSRSS_GENERATE_CTRL_EVENT GenerateCtrlEvent = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GenerateCtrlEvent;
    PCSRSS_CONSOLE Console;
    PCONSOLE_PROCESS_DATA current;
    PLIST_ENTRY current_entry;
    DWORD Group;

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(Status)) return Status;

    Group = GenerateCtrlEvent->ProcessGroup;
    Status = STATUS_INVALID_PARAMETER;
    for (current_entry  = Console->ProcessList.Flink;
         current_entry != &Console->ProcessList;
         current_entry  = current_entry->Flink)
    {
        current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
        if (Group == 0 || current->Process->ProcessGroupId == Group)
        {
            ConioConsoleCtrlEvent(GenerateCtrlEvent->Event, current);
            Status = STATUS_SUCCESS;
        }
    }

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(SrvGetConsoleSelectionInfo)
{
    NTSTATUS Status;
    PCSRSS_GET_CONSOLE_SELECTION_INFO GetConsoleSelectionInfo = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetConsoleSelectionInfo;
    PCSRSS_CONSOLE Console;

    Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (NT_SUCCESS(Status))
    {
        memset(&GetConsoleSelectionInfo->Info, 0, sizeof(CONSOLE_SELECTION_INFO));
        if (Console->Selection.dwFlags != 0)
            GetConsoleSelectionInfo->Info = Console->Selection;
        ConioUnlockConsole(Console);
    }

    return Status;
}

/* EOF */

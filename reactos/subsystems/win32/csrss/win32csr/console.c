/*
 * reactos/subsys/csrss/win32csr/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include "w32csr.h"
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
ConioConsoleFromProcessData(PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE *Console)
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
ConioConsoleCtrlEventTimeout(DWORD Event, PCSRSS_PROCESS_DATA ProcessData, DWORD Timeout)
{
    HANDLE Thread;

    DPRINT("ConioConsoleCtrlEvent Parent ProcessId = %x\n", ProcessData->ProcessId);

    if (ProcessData->CtrlDispatcher)
    {

        Thread = CreateRemoteThread(ProcessData->Process, NULL, 0,
                                    (LPTHREAD_START_ROUTINE) ProcessData->CtrlDispatcher,
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
ConioConsoleCtrlEvent(DWORD Event, PCSRSS_PROCESS_DATA ProcessData)
{
    ConioConsoleCtrlEventTimeout(Event, ProcessData, 0);
}

static NTSTATUS WINAPI
CsrInitConsole(PCSRSS_CONSOLE Console, BOOL Visible)
{
    NTSTATUS Status;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PCSRSS_SCREEN_BUFFER NewBuffer;
    BOOL GuiMode;

    Console->Title.MaximumLength = Console->Title.Length = 0;
    Console->Title.Buffer = NULL;

    //FIXME
    RtlCreateUnicodeString(&Console->Title, L"Command Prompt");

    Console->ReferenceCount = 0;
    Console->WaitingChars = 0;
    Console->WaitingLines = 0;
    Console->EchoCount = 0;
    Console->Header.Type = CONIO_CONSOLE_MAGIC;
    Console->Header.Console = Console;
    Console->Mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    Console->EarlyReturn = FALSE;
    InitializeListHead(&Console->BufferList);
    Console->ActiveBuffer = NULL;
    InitializeListHead(&Console->InputEvents);
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
    NewBuffer = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, sizeof(CSRSS_SCREEN_BUFFER));
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

    if (! GuiMode)
    {
        Status = TuiInitConsole(Console);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open text-mode console, switching to gui-mode\n");
            GuiMode = TRUE;
        }
    }
    if (GuiMode)
    {
        Status = GuiInitConsole(Console, Visible);
        if (! NT_SUCCESS(Status))
        {
            HeapFree(Win32CsrApiHeap,0, NewBuffer);
            RtlFreeUnicodeString(&Console->Title);
            DeleteCriticalSection(&Console->Lock);
            CloseHandle(Console->ActiveEvent);
            DPRINT1("GuiInitConsole: failed\n");
            return Status;
        }
    }

    Status = CsrInitConsoleScreenBuffer(Console, NewBuffer);
    if (! NT_SUCCESS(Status))
    {
        ConioCleanupConsole(Console);
        RtlFreeUnicodeString(&Console->Title);
        DeleteCriticalSection(&Console->Lock);
        CloseHandle(Console->ActiveEvent);
        HeapFree(Win32CsrApiHeap, 0, NewBuffer);
        DPRINT1("CsrInitConsoleScreenBuffer: failed\n");
        return Status;
    }

    /* copy buffer contents to screen */
    ConioDrawConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(CsrAllocConsole)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN NewConsole = FALSE;

    DPRINT("CsrAllocConsole\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (ProcessData->Console)
    {
        DPRINT1("Process already has a console\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_PARAMETER;
    }

    /* If we don't need a console, then get out of here */
    if (!Request->Data.AllocConsoleRequest.ConsoleNeeded)
    {
        DPRINT("No console needed\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_SUCCESS;
    }

    /* If we already have one, then don't create a new one... */
    if (!Request->Data.AllocConsoleRequest.Console ||
            Request->Data.AllocConsoleRequest.Console != ProcessData->ParentConsole)
    {
        /* Allocate a console structure */
        NewConsole = TRUE;
        Console = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, sizeof(CSRSS_CONSOLE));
        if (NULL == Console)
        {
            DPRINT1("Not enough memory for console\n");
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_NO_MEMORY;
        }
        /* initialize list head */
        InitializeListHead(&Console->ProcessList);
        /* insert process data required for GUI initialization */
        InsertHeadList(&Console->ProcessList, &ProcessData->ProcessEntry);
        /* Initialize the Console */
        Status = CsrInitConsole(Console, Request->Data.AllocConsoleRequest.Visible);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console init failed\n");
            HeapFree(Win32CsrApiHeap, 0, Console);
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }
    }
    else
    {
        /* Reuse our current console */
        Console = Request->Data.AllocConsoleRequest.Console;
    }

    /* Set the Process Console */
    ProcessData->Console = Console;

    /* Return it to the caller */
    Request->Data.AllocConsoleRequest.Console = Console;

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&Console->ReferenceCount);

    if (NewConsole || !ProcessData->bInheritHandles)
    {
        /* Insert the Objects */
        Status = Win32CsrInsertObject(ProcessData,
                                      &Request->Data.AllocConsoleRequest.InputHandle,
                                      &Console->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            ProcessData->Console = 0;
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }

        Status = Win32CsrInsertObject(ProcessData,
                                      &Request->Data.AllocConsoleRequest.OutputHandle,
                                      &Console->ActiveBuffer->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            Win32CsrReleaseObject(ProcessData,
                                  Request->Data.AllocConsoleRequest.InputHandle);
            ProcessData->Console = 0;
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }
    }

    /* Duplicate the Event */
    if (!DuplicateHandle(GetCurrentProcess(),
                         ProcessData->Console->ActiveEvent,
                         ProcessData->Process,
                         &ProcessData->ConsoleEvent,
                         EVENT_ALL_ACCESS,
                         FALSE,
                         0))
    {
        DPRINT1("DuplicateHandle() failed: %d\n", GetLastError);
        ConioDeleteConsole((Object_t *) Console);
        if (NewConsole || !ProcessData->bInheritHandles)
        {
            Win32CsrReleaseObject(ProcessData,
                                  Request->Data.AllocConsoleRequest.OutputHandle);
            Win32CsrReleaseObject(ProcessData,
                                  Request->Data.AllocConsoleRequest.InputHandle);
        }
        ProcessData->Console = 0;
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return Status;
    }

    /* Set the Ctrl Dispatcher */
    ProcessData->CtrlDispatcher = Request->Data.AllocConsoleRequest.CtrlDispatcher;
    DPRINT("CSRSS:CtrlDispatcher address: %x\n", ProcessData->CtrlDispatcher);

    if (!NewConsole)
    {
        /* Insert into the list if it has not been added */
        InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ProcessEntry);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return STATUS_SUCCESS;
}

CSR_API(CsrFreeConsole)
{
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    return Win32CsrReleaseConsole(ProcessData);
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
        HeapFree(Win32CsrApiHeap, 0, Event);
    }

    ConioCleanupConsole(Console);
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
    HeapFree(Win32CsrApiHeap, 0, Console);
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

CSR_API(CsrSetConsoleMode)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;

    DPRINT("CsrSetConsoleMode\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Status = Win32CsrLockObject(ProcessData,
                                Request->Data.SetConsoleModeRequest.ConsoleHandle,
                                (Object_t **) &Console, GENERIC_WRITE, 0);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Buff = (PCSRSS_SCREEN_BUFFER)Console;
    if (CONIO_CONSOLE_MAGIC == Console->Header.Type)
    {
        Console->Mode = Request->Data.SetConsoleModeRequest.Mode & CONSOLE_INPUT_MODE_VALID;
    }
    else if (CONIO_SCREEN_BUFFER_MAGIC == Console->Header.Type)
    {
        Buff->Mode = Request->Data.SetConsoleModeRequest.Mode & CONSOLE_OUTPUT_MODE_VALID;
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

    Win32CsrUnlockObject((Object_t *)Console);

    return Status;
}

CSR_API(CsrGetConsoleMode)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;   /* gee, I really wish I could use an anonymous union here */

    DPRINT("CsrGetConsoleMode\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Status = Win32CsrLockObject(ProcessData, Request->Data.GetConsoleModeRequest.ConsoleHandle,
                                (Object_t **) &Console, GENERIC_READ, 0);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = STATUS_SUCCESS;
    Buff = (PCSRSS_SCREEN_BUFFER) Console;
    if (CONIO_CONSOLE_MAGIC == Console->Header.Type)
    {
        Request->Data.GetConsoleModeRequest.ConsoleMode = Console->Mode;
    }
    else if (CONIO_SCREEN_BUFFER_MAGIC == Buff->Header.Type)
    {
        Request->Data.GetConsoleModeRequest.ConsoleMode = Buff->Mode;
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

    Win32CsrUnlockObject((Object_t *)Console);
    return Status;
}

CSR_API(CsrSetTitle)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PWCHAR Buffer;

    DPRINT("CsrSetTitle\n");

    if (Request->Header.u1.s1.TotalLength
            < CSR_API_MESSAGE_HEADER_SIZE(CSRSS_SET_TITLE)
            + Request->Data.SetTitleRequest.Length)
    {
        DPRINT1("Invalid request size\n");
        Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
        Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    if(NT_SUCCESS(Status))
    {
        Buffer =  RtlAllocateHeap(RtlGetProcessHeap(), 0, Request->Data.SetTitleRequest.Length);
        if (Buffer)
        {
            /* copy title to console */
            RtlFreeUnicodeString(&Console->Title);
            Console->Title.Buffer = Buffer;
            Console->Title.Length = Console->Title.MaximumLength = Request->Data.SetTitleRequest.Length;
            memcpy(Console->Title.Buffer, Request->Data.SetTitleRequest.Title, Console->Title.Length);
            if (! ConioChangeTitle(Console))
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

CSR_API(CsrGetTitle)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    DWORD Length;

    DPRINT("CsrGetTitle\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console\n");
        return Status;
    }

    /* Copy title of the console to the user title buffer */
    RtlZeroMemory(&Request->Data.GetTitleRequest, sizeof(CSRSS_GET_TITLE));
    Request->Data.GetTitleRequest.Length = Console->Title.Length;
    memcpy (Request->Data.GetTitleRequest.Title, Console->Title.Buffer,
            Console->Title.Length);
    Length = CSR_API_MESSAGE_HEADER_SIZE(CSRSS_SET_TITLE) + Console->Title.Length;

    ConioUnlockConsole(Console);

    if (Length > sizeof(CSR_API_MESSAGE))
    {
        Request->Header.u1.s1.TotalLength = Length;
        Request->Header.u1.s1.DataLength = Length - sizeof(PORT_MESSAGE);
    }
    return STATUS_SUCCESS;
}

/**********************************************************************
 *	HardwareStateProperty
 *
 *	DESCRIPTION
 *		Set/Get the value of the HardwareState and switch
 *		between direct video buffer ouput and GDI windowed
 *		output.
 *	ARGUMENTS
 *		Client hands us a CSRSS_CONSOLE_HARDWARE_STATE
 *		object. We use the same object to Request.
 *	NOTE
 *		ConsoleHwState has the correct size to be compatible
 *		with NT's, but values are not.
 */
static NTSTATUS FASTCALL
SetConsoleHardwareState (PCSRSS_CONSOLE Console, DWORD ConsoleHwState)
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

CSR_API(CsrHardwareStateProperty)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrHardwareStateProperty\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockConsole(ProcessData,
                              Request->Data.ConsoleHardwareStateRequest.ConsoleHandle,
                              &Console,
                              GENERIC_READ);
    if (! NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SetConsoleHardwareState\n");
        return Status;
    }

    switch (Request->Data.ConsoleHardwareStateRequest.SetGet)
    {
    case CONSOLE_HARDWARE_STATE_GET:
        Request->Data.ConsoleHardwareStateRequest.State = Console->HardwareState;
        break;

    case CONSOLE_HARDWARE_STATE_SET:
        DPRINT("Setting console hardware state.\n");
        Status = SetConsoleHardwareState(Console, Request->Data.ConsoleHardwareStateRequest.State);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER_2; /* Client: (handle, [set_get], mode) */
        break;
    }

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(CsrGetConsoleWindow)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrGetConsoleWindow\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->Data.GetConsoleWindowRequest.WindowHandle = Console->hWindow;
    ConioUnlockConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleIcon)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrSetConsoleIcon\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = (ConioChangeIcon(Console, Request->Data.SetConsoleIconRequest.WindowIcon)
              ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(CsrGetConsoleCodePage)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrGetConsoleCodePage\n");

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Request->Data.GetConsoleCodePage.CodePage = Console->CodePage;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleCodePage)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrSetConsoleCodePage\n");

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    if (IsValidCodePage(Request->Data.SetConsoleCodePage.CodePage))
    {
        Console->CodePage = Request->Data.SetConsoleCodePage.CodePage;
        ConioUnlockConsole(Console);
        return STATUS_SUCCESS;
    }

    ConioUnlockConsole(Console);
    return STATUS_INVALID_PARAMETER;
}

CSR_API(CsrGetConsoleOutputCodePage)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrGetConsoleOutputCodePage\n");

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Request->Data.GetConsoleOutputCodePage.CodePage = Console->OutputCodePage;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleOutputCodePage)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrSetConsoleOutputCodePage\n");

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    if (IsValidCodePage(Request->Data.SetConsoleOutputCodePage.CodePage))
    {
        Console->OutputCodePage = Request->Data.SetConsoleOutputCodePage.CodePage;
        ConioUnlockConsole(Console);
        return STATUS_SUCCESS;
    }

    ConioUnlockConsole(Console);
    return STATUS_INVALID_PARAMETER;
}

CSR_API(CsrGetProcessList)
{
    PDWORD Buffer;
    PCSRSS_CONSOLE Console;
    PCSRSS_PROCESS_DATA current;
    PLIST_ENTRY current_entry;
    ULONG nItems = 0;
    NTSTATUS Status;
    ULONG_PTR Offset;

    DPRINT("CsrGetProcessList\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Buffer = Request->Data.GetProcessListRequest.ProcessId;
    Offset = (PBYTE)Buffer - (PBYTE)ProcessData->CsrSectionViewBase;
    if (Offset >= ProcessData->CsrSectionViewSize
        || (Request->Data.GetProcessListRequest.nMaxIds * sizeof(DWORD)) > (ProcessData->CsrSectionViewSize - Offset)
        || Offset & (sizeof(DWORD) - 1))
    {
        return STATUS_ACCESS_VIOLATION;
    }

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    for (current_entry = Console->ProcessList.Flink;
         current_entry != &Console->ProcessList;
         current_entry = current_entry->Flink)
    {
        current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
        if (++nItems <= Request->Data.GetProcessListRequest.nMaxIds)
        {
            *Buffer++ = (DWORD)current->ProcessId;
        }
    }

    ConioUnlockConsole(Console);

    Request->Data.GetProcessListRequest.nProcessIdsTotal = nItems;
    return STATUS_SUCCESS;
}

CSR_API(CsrGenerateCtrlEvent)
{
    PCSRSS_CONSOLE Console;
    PCSRSS_PROCESS_DATA current;
    PLIST_ENTRY current_entry;
    DWORD Group;
    NTSTATUS Status;

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    Group = Request->Data.GenerateCtrlEvent.ProcessGroup;
    Status = STATUS_INVALID_PARAMETER;
    for (current_entry = Console->ProcessList.Flink;
            current_entry != &Console->ProcessList;
            current_entry = current_entry->Flink)
    {
        current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
        if (Group == 0 || current->ProcessGroup == Group)
        {
            ConioConsoleCtrlEvent(Request->Data.GenerateCtrlEvent.Event, current);
            Status = STATUS_SUCCESS;
        }
    }

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(CsrGetConsoleSelectionInfo)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Status))
    {
        memset(&Request->Data.GetConsoleSelectionInfo.Info, 0, sizeof(CONSOLE_SELECTION_INFO));
        if (Console->Selection.dwFlags != 0)
            Request->Data.GetConsoleSelectionInfo.Info = Console->Selection;
        ConioUnlockConsole(Console);
    }
    return Status;
}

/* EOF */

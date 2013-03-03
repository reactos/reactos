/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/console.c
 * PURPOSE:         Console I/O functions
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#define COBJMACROS
#define NONAMELESSUNION

#include "consrv.h"
#include "settings.h"
#include "guiconsole.h"
#include "tuiconsole.h"

#include <shlwapi.h>
#include <shlobj.h>

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

VOID FASTCALL
ConSrvConsoleCtrlEventTimeout(DWORD Event,
                              PCONSOLE_PROCESS_DATA ProcessData,
                              DWORD Timeout)
{
    HANDLE Thread;

    DPRINT("ConSrvConsoleCtrlEvent Parent ProcessId = %x\n", ProcessData->Process->ClientId.UniqueProcess);

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
ConSrvConsoleCtrlEvent(DWORD Event, PCONSOLE_PROCESS_DATA ProcessData)
{
    ConSrvConsoleCtrlEventTimeout(Event, ProcessData, 0);
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
        if (!IsListEmpty(&Console->WriteWaitQueue))
        {
            CsrDereferenceWait(&Console->WriteWaitQueue);
        }
    }
}

static BOOL
LoadShellLinkConsoleInfo(IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                         OUT PCONSOLE_INFO ConsoleInfo,
                         OUT LPWSTR IconPath,
                         IN SIZE_T IconPathLength,
                         OUT PINT piIcon)
{
#define PATH_SEPARATOR L'\\'

    BOOL   RetVal = FALSE;
    LPWSTR LinkName = NULL;
    SIZE_T Length = 0;

    if ((ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
        return FALSE;

    if (IconPath == NULL || piIcon == NULL)
        return FALSE;

    IconPath[0] = L'\0';
    *piIcon = 0;

    /* 1- Find the last path separator if any */
    LinkName = wcsrchr(ConsoleStartInfo->ConsoleTitle, PATH_SEPARATOR);
    if (LinkName == NULL)
    {
        LinkName = ConsoleStartInfo->ConsoleTitle;
    }
    else
    {
        /* Skip the path separator */
        ++LinkName;
    }

    /* 2- Check for the link extension. The name ".lnk" is considered invalid. */
    Length = wcslen(LinkName);
    if ( (Length <= 4) || (wcsicmp(LinkName + (Length - 4), L".lnk") != 0) )
        return FALSE;

    /* 3- It may be a link. Try to retrieve some properties */
    HRESULT hRes = CoInitialize(NULL);
    if (SUCCEEDED(hRes))
    {
        /* Get a pointer to the IShellLink interface */
        IShellLinkW* pshl = NULL;
        hRes = CoCreateInstance(&CLSID_ShellLink,
                                NULL, 
                                CLSCTX_INPROC_SERVER,
                                &IID_IShellLinkW,
                                (LPVOID*)&pshl);
        if (SUCCEEDED(hRes))
        {
            /* Get a pointer to the IPersistFile interface */
            IPersistFile* ppf = NULL;
            hRes = IPersistFile_QueryInterface(pshl, &IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hRes))
            {
                /* Load the shortcut */
                hRes = IPersistFile_Load(ppf, ConsoleStartInfo->ConsoleTitle, STGM_READ);
                if (SUCCEEDED(hRes))
                {
                    /*
                     * Finally we can get the properties !
                     * Update the old ones if needed.
                     */
                    INT ShowCmd = 0;
                    // WORD HotKey = 0;

                    /* Get the name of the shortcut */
                    Length = min(Length - 4,
                                 sizeof(ConsoleStartInfo->ConsoleTitle) / sizeof(ConsoleStartInfo->ConsoleTitle[0]) - 1);
                    wcsncpy(ConsoleStartInfo->ConsoleTitle, LinkName, Length);
                    ConsoleStartInfo->ConsoleTitle[Length] = L'\0';

                    // HACK: Copy also the name of the shortcut
                    Length = min(Length,
                                 sizeof(ConsoleInfo->ConsoleTitle) / sizeof(ConsoleInfo->ConsoleTitle[0]) - 1);
                    wcsncpy(ConsoleInfo->ConsoleTitle, LinkName, Length);
                    ConsoleInfo->ConsoleTitle[Length] = L'\0';

                    /* Get the window showing command */
                    hRes = IShellLinkW_GetShowCmd(pshl, &ShowCmd);
                    if (SUCCEEDED(hRes)) ConsoleStartInfo->ShowWindow = (WORD)ShowCmd;

                    /* Get the hotkey */
                    // hRes = pshl->GetHotkey(&ShowCmd);
                    // if (SUCCEEDED(hRes)) ConsoleStartInfo->HotKey = HotKey;

                    /* Get the icon location, if any */
                    hRes = IShellLinkW_GetIconLocation(pshl, IconPath, IconPathLength, piIcon);
                    if (!SUCCEEDED(hRes))
                    {
                        IconPath[0] = L'\0';
                    }

                    // FIXME: Since we still don't load console properties from the shortcut,
                    // return false. When this will be done, we will return true instead.
                    RetVal = FALSE;
                }
                IPersistFile_Release(ppf);
            }
            IShellLinkW_Release(pshl);
        }
    }
    CoUninitialize();

    return RetVal;
}

NTSTATUS WINAPI
ConSrvInitConsole(OUT PCONSOLE* NewConsole,
                  IN LPCWSTR AppPath,
                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN PCSR_PROCESS ConsoleLeaderProcess)
{
    NTSTATUS Status;
    SECURITY_ATTRIBUTES SecurityAttributes;
    CONSOLE_INFO ConsoleInfo;
    SIZE_T Length = 0;
    DWORD ProcessId = HandleToUlong(ConsoleLeaderProcess->ClientId.UniqueProcess);
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER NewBuffer;
    BOOL GuiMode;
    WCHAR Title[128];
    WCHAR IconPath[MAX_PATH + 1] = L"";
    INT iIcon = 0;

    if (NewConsole == NULL) return STATUS_INVALID_PARAMETER;
    *NewConsole = NULL;

    /*
     * Allocate a console structure
     */
    Console = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY, sizeof(CONSOLE));
    if (NULL == Console)
    {
        DPRINT1("Not enough memory for console creation.\n");
        return STATUS_NO_MEMORY;
    }

    /*
     * Load the console settings
     */

    /* 1. Load the default settings */
    ConSrvGetDefaultSettings(&ConsoleInfo, ProcessId);

    /* 2. Get the title of the console (initialize ConsoleInfo.ConsoleTitle) */
    Length = min(wcslen(ConsoleStartInfo->ConsoleTitle),
                 sizeof(ConsoleInfo.ConsoleTitle) / sizeof(ConsoleInfo.ConsoleTitle[0]) - 1);
    wcsncpy(ConsoleInfo.ConsoleTitle, ConsoleStartInfo->ConsoleTitle, Length);
    ConsoleInfo.ConsoleTitle[Length] = L'\0';

    /*
     * 3. Check whether the process creating the
     *    console was launched via a shell-link
     *    (ConsoleInfo.ConsoleTitle may be updated).
     */
    if (ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME)
    {
        if (!LoadShellLinkConsoleInfo(ConsoleStartInfo,
                                      &ConsoleInfo,
                                      IconPath,
                                      MAX_PATH,
                                      &iIcon))
        {
            ConsoleStartInfo->dwStartupFlags &= ~STARTF_TITLEISLINKNAME;
        }
    }

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
        ConSrvReadUserSettings(&ConsoleInfo, ProcessId);

        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         */
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEFILLATTRIBUTE)
        {
            ConsoleInfo.ScreenAttrib = ConsoleStartInfo->FillAttribute;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USECOUNTCHARS)
        {
            ConsoleInfo.ScreenBufferSize = ConsoleStartInfo->ScreenBufferSize;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESHOWWINDOW)
        {
            ConsoleInfo.u.GuiInfo.ShowWindow = ConsoleStartInfo->ShowWindow;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEPOSITION)
        {
            ConsoleInfo.u.GuiInfo.AutoPosition = FALSE;
            ConsoleInfo.u.GuiInfo.WindowOrigin = ConsoleStartInfo->ConsoleWindowOrigin;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESIZE)
        {
            // ConsoleInfo.ConsoleSize = ConsoleStartInfo->ConsoleWindowSize;
            ConsoleInfo.ConsoleSize.X = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cx;
            ConsoleInfo.ConsoleSize.Y = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cy;
        }
        /*
        if (ConsoleStartInfo->dwStartupFlags & STARTF_RUNFULLSCREEN)
        {
        }
        */
    }

    /*
     * Initialize the console
     */
    InitializeCriticalSection(&Console->Lock);
    Console->ReferenceCount = 0;
    Console->ConsoleLeaderCID = ConsoleLeaderProcess->ClientId;
    InitializeListHead(&Console->ProcessList);
    memcpy(Console->Colors, ConsoleInfo.Colors, sizeof(ConsoleInfo.Colors));
    Console->Size = ConsoleInfo.ConsoleSize;

    /*
     * Initialize the input buffer
     */
    Console->InputBuffer.Header.Type = CONIO_INPUT_BUFFER_MAGIC;
    Console->InputBuffer.Header.Console = Console;

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = TRUE;
    Console->InputBuffer.ActiveEvent = CreateEventW(&SecurityAttributes, TRUE, FALSE, NULL);
    if (NULL == Console->InputBuffer.ActiveEvent)
    {
        DeleteCriticalSection(&Console->Lock);
        RtlFreeHeap(ConSrvHeap, 0, Console);
        return STATUS_UNSUCCESSFUL;
    }

    // TODO: Use the values from ConsoleInfo.
    Console->InputBuffer.Mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                                ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    Console->QuickEdit  = ConsoleInfo.QuickEdit;
    Console->InsertMode = ConsoleInfo.InsertMode;
    InitializeListHead(&Console->InputBuffer.ReadWaitQueue);
    InitializeListHead(&Console->InputBuffer.InputEvents);
    Console->LineBuffer = NULL;
    Console->CodePage = GetOEMCP();
    Console->OutputCodePage = GetOEMCP();

    /* Initialize a new screen buffer with default settings */
    InitializeListHead(&Console->BufferList);
    Status = ConSrvCreateScreenBuffer(Console,
                                      &NewBuffer,
                                      ConsoleInfo.ScreenBufferSize,
                                      ConsoleInfo.ScreenAttrib,
                                      ConsoleInfo.PopupAttrib);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConSrvCreateScreenBuffer: failed, Status = 0x%08lx\n", Status);
        CloseHandle(Console->InputBuffer.ActiveEvent);
        DeleteCriticalSection(&Console->Lock);
        RtlFreeHeap(ConSrvHeap, 0, Console);
        return Status;
    }
    /* Make the new screen buffer active */
    Console->ActiveBuffer = NewBuffer;
    Console->FullScreen = ConsoleInfo.FullScreen;
    InitializeListHead(&Console->WriteWaitQueue);

    /*
     * Initialize the history buffers
     */
    InitializeListHead(&Console->HistoryBuffers);
    Console->HistoryBufferSize = ConsoleInfo.HistoryBufferSize;
    Console->NumberOfHistoryBuffers = ConsoleInfo.NumberOfHistoryBuffers;
    Console->HistoryNoDup = ConsoleInfo.HistoryNoDup;

    /* Finish initialization */
    Console->GuiData = NULL;
    Console->hIcon = Console->hIconSm = NULL;

    /* Initialize the console title */
    RtlCreateUnicodeString(&Console->OriginalTitle, ConsoleStartInfo->ConsoleTitle);
    if (ConsoleStartInfo->ConsoleTitle[0] == L'\0')
    {
        if (LoadStringW(ConSrvDllInstance, IDS_CONSOLE_TITLE, Title, sizeof(Title) / sizeof(Title[0])))
        {
            RtlCreateUnicodeString(&Console->Title, Title);
        }
        else
        {
            RtlCreateUnicodeString(&Console->Title, L"ReactOS Console");
        }
    }
    else
    {
        RtlCreateUnicodeString(&Console->Title, ConsoleStartInfo->ConsoleTitle);
    }

    /*
     * If we are not in GUI-mode, start the text-mode console.
     * If we fail, try to start the GUI-mode console.
     */
    GuiMode = DtbgIsDesktopVisible();

    if (!GuiMode)
    {
        DPRINT1("CONSRV: Opening text-mode console\n");
        Status = TuiInitConsole(Console, &ConsoleInfo);
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
     *   was switched to TRUE in order to try to open the console in GUI-mode
     *   (win32k will automatically switch to graphical mode, therefore
     *   no additional code is needed).
     */
    if (GuiMode)
    {
        DPRINT1("CONSRV: Opening GUI-mode console\n");
        Status = GuiInitConsole(Console,
                                AppPath,
                                &ConsoleInfo,
                                IconPath,
                                iIcon);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("GuiInitConsole: failed, Status = 0x%08lx\n", Status);
            RtlFreeUnicodeString(&Console->Title);
            RtlFreeUnicodeString(&Console->OriginalTitle);
            ConioDeleteScreenBuffer(NewBuffer);
            CloseHandle(Console->InputBuffer.ActiveEvent);
            DeleteCriticalSection(&Console->Lock);
            RtlFreeHeap(ConSrvHeap, 0, Console);
            return Status;
        }
    }

    /* Copy buffer contents to screen */
    ConioDrawConsole(Console);

    /* Return the newly created console to the caller and a success code too */
    *NewConsole = Console;
    return STATUS_SUCCESS;
}

VOID WINAPI
ConSrvInitConsoleSupport(VOID)
{
    DPRINT("CONSRV: ConSrvInitConsoleSupport()\n");

    /* Should call LoadKeyboardLayout */
}

VOID WINAPI
ConSrvDeleteConsole(PCONSOLE Console)
{
    ConsoleInput *Event;

    DPRINT("ConSrvDeleteConsole\n");

    /* Drain input event queue */
    while (Console->InputBuffer.InputEvents.Flink != &Console->InputBuffer.InputEvents)
    {
        Event = (ConsoleInput *) Console->InputBuffer.InputEvents.Flink;
        Console->InputBuffer.InputEvents.Flink = Console->InputBuffer.InputEvents.Flink->Flink;
        Console->InputBuffer.InputEvents.Flink->Flink->Blink = &Console->InputBuffer.InputEvents;
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

    CloseHandle(Console->InputBuffer.ActiveEvent);
    if (Console->UnpauseEvent) CloseHandle(Console->UnpauseEvent);
    DeleteCriticalSection(&Console->Lock);

    RtlFreeUnicodeString(&Console->OriginalTitle);
    RtlFreeUnicodeString(&Console->Title);
    IntDeleteAllAliases(Console->Aliases);
    RtlFreeHeap(ConSrvHeap, 0, Console);
}

CSR_API(SrvOpenConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_OPENCONSOLE OpenConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.OpenConsoleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);

    DPRINT("SrvOpenConsole\n");

    OpenConsoleRequest->ConsoleHandle = INVALID_HANDLE_VALUE;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    DPRINT1("ProcessData = 0x%p ; ProcessData->Console = 0x%p\n", ProcessData, ProcessData->Console);

    if (ProcessData->Console)
    {
        DWORD DesiredAccess = OpenConsoleRequest->Access;
        DWORD ShareMode = OpenConsoleRequest->ShareMode;

        PCONSOLE Console = ProcessData->Console;
        Object_t *Object;

        DPRINT1("SrvOpenConsole - Checkpoint 1\n");
        EnterCriticalSection(&Console->Lock);
        DPRINT1("SrvOpenConsole - Checkpoint 2\n");

        if (OpenConsoleRequest->HandleType == HANDLE_OUTPUT)
        {
            Object = &Console->ActiveBuffer->Header;
        }
        else // HANDLE_INPUT
        {
            Object = &Console->InputBuffer.Header;
        }

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
            Status = ConSrvInsertObject(ProcessData,
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
    PCSR_PROCESS CsrProcess = CsrGetClientThread()->Process;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    DPRINT("SrvAllocConsole\n");

    if (ProcessData->Console != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&AllocConsoleRequest->ConsoleStartInfo,
                                   1,
                                   sizeof(CONSOLE_START_INFO))      ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&AllocConsoleRequest->AppPath,
                                   MAX_PATH + 1,
                                   sizeof(WCHAR)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * We are about to create a new console. However when ConSrvNewProcess
     * was called, we didn't know that we wanted to create a new console and
     * therefore, we by default inherited the handles table from our parent
     * process. It's only now that we notice that in fact we do not need
     * them, because we've created a new console and thus we must use it.
     *
     * Therefore, free the console we can have and our handles table,
     * and recreate a new one later on.
     */
    ConSrvRemoveConsole(ProcessData);
    // ConSrvFreeHandlesTable(ProcessData);

    /* Initialize a new Console owned by the Console Leader Process */
    Status = ConSrvAllocateConsole(ProcessData,
                                   AllocConsoleRequest->AppPath,
                                   &AllocConsoleRequest->InputHandle,
                                   &AllocConsoleRequest->OutputHandle,
                                   &AllocConsoleRequest->ErrorHandle,
                                   AllocConsoleRequest->ConsoleStartInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console allocation failed\n");
        return Status;
    }

    /* Return it to the caller */
    AllocConsoleRequest->Console = ProcessData->Console;

    /* Input Wait Handle */
    AllocConsoleRequest->InputWaitHandle = ProcessData->ConsoleEvent;

    /* Set the Property Dialog Handler */
    ProcessData->PropDispatcher = AllocConsoleRequest->PropDispatcher;

    /* Set the Ctrl Dispatcher */
    ProcessData->CtrlDispatcher = AllocConsoleRequest->CtrlDispatcher;
    DPRINT("CONSRV: CtrlDispatcher address: %x\n", ProcessData->CtrlDispatcher);

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

        DPRINT("We, process (ID) %lu;%lu\n", TargetProcess->ClientId.UniqueProcess, TargetProcess->ClientId.UniqueThread);
        ProcessId = ULongToHandle(ProcessInfo.InheritedFromUniqueProcessId);
        DPRINT("Parent process ID = %lu\n", ProcessId);
    }

    /* Lock the source process via its PID */
    Status = CsrLockProcessByClientId(ProcessId, &SourceProcess);
    DPRINT1("Lock process Id %lu - Status %lu\n", ProcessId, Status);
    if (!NT_SUCCESS(Status)) return Status;

    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

    DPRINT1("SourceProcessData->Console = 0x%p\n", SourceProcessData->Console);
    if (SourceProcessData->Console == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    /*
     * We are about to create a new console. However when ConSrvNewProcess
     * was called, we didn't know that we wanted to create a new console and
     * therefore, we by default inherited the handles table from our parent
     * process. It's only now that we notice that in fact we do not need
     * them, because we've created a new console and thus we must use it.
     *
     * Therefore, free the console we can have and our handles table,
     * and recreate a new one later on.
     */
    ConSrvRemoveConsole(TargetProcessData);
    // ConSrvFreeHandlesTable(TargetProcessData);

    /*
     * Inherit the console from the parent,
     * if any, otherwise return an error.
     */
    Status = ConSrvInheritConsole(TargetProcessData,
                                  SourceProcessData->Console,
                                  TRUE,
                                  &AttachConsoleRequest->InputHandle,
                                  &AttachConsoleRequest->OutputHandle,
                                  &AttachConsoleRequest->ErrorHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console inheritance failed\n");
        goto Quit;
    }

    /* Return it to the caller */
    AttachConsoleRequest->Console = TargetProcessData->Console;

    /* Input Wait Handle */
    AttachConsoleRequest->InputWaitHandle = TargetProcessData->ConsoleEvent;

    /* Set the Property Dialog Handler */
    TargetProcessData->PropDispatcher = AttachConsoleRequest->PropDispatcher;

    /* Set the Ctrl Dispatcher */
    TargetProcessData->CtrlDispatcher = AttachConsoleRequest->CtrlDispatcher;
    DPRINT("CONSRV: CtrlDispatcher address: %x\n", TargetProcessData->CtrlDispatcher);

    Status = STATUS_SUCCESS;

Quit:
    /* Unlock the "source" process and exit */
    CsrUnlockProcess(SourceProcess);
    return Status;
}

CSR_API(SrvFreeConsole)
{
    DPRINT1("SrvFreeConsole\n");
    ConSrvRemoveConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process));
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleMode)
{
#define CONSOLE_INPUT_MODE_VALID  ( ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT   | \
                                    ENABLE_ECHO_INPUT      | ENABLE_WINDOW_INPUT | \
                                    ENABLE_MOUSE_INPUT | \
                                    ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS )
#define CONSOLE_OUTPUT_MODE_VALID ( ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT )

    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    Object_t* Object = NULL;

    DPRINT("SrvSetConsoleMode\n");

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                ConsoleModeRequest->ConsoleHandle,
                                &Object, NULL, GENERIC_WRITE, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = STATUS_SUCCESS;

    if (CONIO_INPUT_BUFFER_MAGIC == Object->Type)
    {
        PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;
        InputBuffer->Mode = ConsoleModeRequest->ConsoleMode & CONSOLE_INPUT_MODE_VALID;
    }
    else if (CONIO_SCREEN_BUFFER_MAGIC == Object->Type)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;
        Buffer->Mode = ConsoleModeRequest->ConsoleMode & CONSOLE_OUTPUT_MODE_VALID;
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

    ConSrvReleaseObject(Object, TRUE);

    return Status;
}

CSR_API(SrvGetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    Object_t* Object = NULL;

    DPRINT("SrvGetConsoleMode\n");

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                ConsoleModeRequest->ConsoleHandle,
                                &Object, NULL, GENERIC_READ, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = STATUS_SUCCESS;

    if (CONIO_INPUT_BUFFER_MAGIC == Object->Type)
    {
        PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;
        ConsoleModeRequest->ConsoleMode = InputBuffer->Mode;
    }
    else if (CONIO_SCREEN_BUFFER_MAGIC == Object->Type)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;
        ConsoleModeRequest->ConsoleMode = Buffer->Mode;
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

    ConSrvReleaseObject(Object, TRUE);

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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console\n");
        return Status;
    }

    /* Allocate a new buffer to hold the new title (NULL-terminated) */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, TitleRequest->Length + sizeof(WCHAR));
    if (Buffer)
    {
        /* Free the old title */
        RtlFreeUnicodeString(&Console->Title);

        /* Copy title to console */
        Console->Title.Buffer = Buffer;
        Console->Title.Length = TitleRequest->Length;
        Console->Title.MaximumLength = Console->Title.Length + sizeof(WCHAR);
        RtlCopyMemory(Console->Title.Buffer,
                      TitleRequest->Title,
                      Console->Title.Length);
        Console->Title.Buffer[Console->Title.Length / sizeof(WCHAR)] = L'\0';

        ConioChangeTitle(Console);
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

    ConSrvReleaseConsole(Console, TRUE);
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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
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

    ConSrvReleaseConsole(Console, TRUE);
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
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE Console;

    DPRINT("SrvGetConsoleHardwareState\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                  HardwareStateRequest->OutputHandle,
                                  &Buff,
                                  GENERIC_READ,
                                  TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvGetConsoleHardwareState\n");
        return Status;
    }

    Console = Buff->Header.Console;
    HardwareStateRequest->State = Console->HardwareState;

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    return Status;
}

CSR_API(SrvSetConsoleHardwareState)
{
    NTSTATUS Status;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.HardwareStateRequest;
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE Console;

    DPRINT("SrvSetConsoleHardwareState\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                  HardwareStateRequest->OutputHandle,
                                  &Buff,
                                  GENERIC_READ,
                                  TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvSetConsoleHardwareState\n");
        return Status;
    }

    DPRINT("Setting console hardware state.\n");
    Console = Buff->Header.Console;
    Status = SetConsoleHardwareState(Console, HardwareStateRequest->State);

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    return Status;
}

CSR_API(SrvGetConsoleWindow)
{
    NTSTATUS Status;
    PCONSOLE_GETWINDOW GetWindowRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetWindowRequest;
    PCONSOLE Console;

    DPRINT("SrvGetConsoleWindow\n");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetWindowRequest->WindowHandle = Console->hWindow;
    ConSrvReleaseConsole(Console, TRUE);

    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleIcon)
{
    NTSTATUS Status;
    PCONSOLE_SETICON SetIconRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetIconRequest;
    PCONSOLE Console;

    DPRINT("SrvSetConsoleIcon\n");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = (ConioChangeIcon(Console, SetIconRequest->WindowIcon)
                ? STATUS_SUCCESS
                : STATUS_UNSUCCESSFUL);

    ConSrvReleaseConsole(Console, TRUE);

    return Status;
}

CSR_API(SrvGetConsoleCP)
{
    NTSTATUS Status;
    PCONSOLE_GETSETINPUTOUTPUTCP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCONSOLE Console;

    DPRINT("SrvGetConsoleCP, getting %s Code Page\n",
            ConsoleCPRequest->InputCP ? "Input" : "Output");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    ConsoleCPRequest->CodePage = (ConsoleCPRequest->InputCP ? Console->CodePage
                                                            : Console->OutputCodePage);
    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleCP)
{
    NTSTATUS Status;
    PCONSOLE_GETSETINPUTOUTPUTCP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCONSOLE Console;

    DPRINT("SrvSetConsoleCP, setting %s Code Page\n",
            ConsoleCPRequest->InputCP ? "Input" : "Output");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    if (IsValidCodePage(ConsoleCPRequest->CodePage))
    {
        if (ConsoleCPRequest->InputCP)
            Console->CodePage = ConsoleCPRequest->CodePage;
        else
            Console->OutputCodePage = ConsoleCPRequest->CodePage;

        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_SUCCESS;
    }

    ConSrvReleaseConsole(Console, TRUE);
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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
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

    ConSrvReleaseConsole(Console, TRUE);

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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
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
            ConSrvConsoleCtrlEvent(GenerateCtrlEventRequest->Event, current);
            Status = STATUS_SUCCESS;
        }
    }

    ConSrvReleaseConsole(Console, TRUE);

    return Status;
}

CSR_API(SrvGetConsoleSelectionInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSELECTIONINFO GetSelectionInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetSelectionInfoRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (NT_SUCCESS(Status))
    {
        memset(&GetSelectionInfoRequest->Info, 0, sizeof(CONSOLE_SELECTION_INFO));
        if (Console->Selection.dwFlags != 0)
            GetSelectionInfoRequest->Info = Console->Selection;
        ConSrvReleaseConsole(Console, TRUE);
    }

    return Status;
}

/* EOF */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/console.c
 * PURPOSE:         Console Management Functions
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define COBJMACROS
#define NONAMELESSUNION

#include "consrv.h"
#include "include/conio.h"
#include "conio.h"
#include "handle.h"
#include "procinit.h"
#include "alias.h"
#include "coninput.h"
#include "conoutput.h"
#include "lineinput.h"
#include "include/settings.h"

#include "frontends/gui/guiterm.h"
#include "frontends/tui/tuiterm.h"

#include "include/console.h"
#include "console.h"
#include "resource.h"

#include <shlwapi.h>
#include <shlobj.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static LIST_ENTRY ConsoleList;  /* The list of all the allocated consoles */
static RTL_RESOURCE ListLock;

#define ConSrvLockConsoleListExclusive()    \
    RtlAcquireResourceExclusive(&ListLock, TRUE)

#define ConSrvLockConsoleListShared()       \
    RtlAcquireResourceShared(&ListLock, TRUE)

#define ConSrvUnlockConsoleList()           \
    RtlReleaseResource(&ListLock)

// Adapted from reactos/lib/rtl/unicode.c, RtlCreateUnicodeString line 2180
BOOLEAN
ConsoleCreateUnicodeString(IN OUT PUNICODE_STRING UniDest,
                           IN PCWSTR Source)
{
    SIZE_T Size = (wcslen(Source) + 1) * sizeof(WCHAR);
    if (Size > MAXUSHORT) return FALSE;

    UniDest->Buffer = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY, Size);
    if (UniDest->Buffer == NULL) return FALSE;

    RtlCopyMemory(UniDest->Buffer, Source, Size);
    UniDest->MaximumLength = (USHORT)Size;
    UniDest->Length = (USHORT)Size - sizeof(WCHAR);

    return TRUE;
}

// Adapted from reactos/lib/rtl/unicode.c, RtlFreeUnicodeString line 431
VOID
ConsoleFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
    if (UnicodeString->Buffer)
    {
        RtlFreeHeap(ConSrvHeap, 0, UnicodeString->Buffer);
        RtlZeroMemory(UnicodeString, sizeof(UNICODE_STRING));
    }
}


/* PRIVATE FUNCTIONS **********************************************************/

static BOOL
DtbgIsDesktopVisible(VOID)
{
    return !((BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_ISCONSOLEMODE));
}

static ULONG
ConSrvConsoleCtrlEventTimeout(DWORD Event,
                              PCONSOLE_PROCESS_DATA ProcessData,
                              DWORD Timeout)
{
    ULONG Status = ERROR_SUCCESS;

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
                                            UlongToPtr(Event), 0, NULL);
                if (NULL == Thread)
                {
                    Status = GetLastError();
                    DPRINT1("Failed thread creation (Error: 0x%x)\n", Status);
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
            Status = RtlNtStatusToDosError(_SEH2_GetExceptionCode());
            DPRINT1("ConSrvConsoleCtrlEventTimeout - Caught an exception, Status = %08X\n", Status);
        }
        _SEH2_END;
    }

    return Status;
}

static ULONG
ConSrvConsoleCtrlEvent(DWORD Event,
                       PCONSOLE_PROCESS_DATA ProcessData)
{
    return ConSrvConsoleCtrlEventTimeout(Event, ProcessData, 0);
}

ULONG FASTCALL
ConSrvConsoleProcessCtrlEvent(PCONSOLE Console,
                              ULONG ProcessGroupId,
                              DWORD Event)
{
    ULONG Status = ERROR_SUCCESS;
    PLIST_ENTRY current_entry;
    PCONSOLE_PROCESS_DATA current;

    /* If the console is already being destroyed, just return */
    if (!ConSrvValidateConsole(Console, CONSOLE_RUNNING, FALSE))
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
            Status = ConSrvConsoleCtrlEvent(Event, current);
        }
    }

    return Status;
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

BOOL FASTCALL
ConSrvValidateConsolePointer(PCONSOLE Console)
{
    PLIST_ENTRY ConsoleEntry;
    PCONSOLE CurrentConsole = NULL;

    if (!Console) return FALSE;

    /* The console list must be locked */
    // ASSERT(Console_list_locked);

    ConsoleEntry = ConsoleList.Flink;
    while (ConsoleEntry != &ConsoleList)
    {
        CurrentConsole = CONTAINING_RECORD(ConsoleEntry, CONSOLE, Entry);
        ConsoleEntry = ConsoleEntry->Flink;
        if (CurrentConsole == Console) return TRUE;
    }

    return FALSE;
}

BOOL FASTCALL
ConSrvValidateConsoleState(PCONSOLE Console,
                           CONSOLE_STATE ExpectedState)
{
    // if (!Console) return FALSE;

    /* The console must be locked */
    // ASSERT(Console_locked);

    return (Console->State == ExpectedState);
}

BOOL FASTCALL
ConSrvValidateConsoleUnsafe(PCONSOLE Console,
                            CONSOLE_STATE ExpectedState,
                            BOOL LockConsole)
{
    if (!Console) return FALSE;

    /*
     * Lock the console to forbid possible console's state changes
     * (which must be done when the console is already locked).
     * If we don't want to lock it, it's because the lock is already
     * held. So there must be no problems.
     */
    if (LockConsole) EnterCriticalSection(&Console->Lock);

    // ASSERT(Console_locked);

    /* Check whether the console's state is what we expect */
    if (!ConSrvValidateConsoleState(Console, ExpectedState))
    {
        if (LockConsole) LeaveCriticalSection(&Console->Lock);
        return FALSE;
    }

    return TRUE;
}

BOOL FASTCALL
ConSrvValidateConsole(PCONSOLE Console,
                      CONSOLE_STATE ExpectedState,
                      BOOL LockConsole)
{
    BOOL RetVal = FALSE;

    if (!Console) return FALSE;

    /*
     * Forbid creation or deletion of consoles when
     * checking for the existence of a console.
     */
    ConSrvLockConsoleListShared();

    if (ConSrvValidateConsolePointer(Console))
    {
        RetVal = ConSrvValidateConsoleUnsafe(Console,
                                             ExpectedState,
                                             LockConsole);
    }

    /* Unlock the console list and return */
    ConSrvUnlockConsoleList();
    return RetVal;
}

NTSTATUS
FASTCALL
ConSrvGetConsole(PCONSOLE_PROCESS_DATA ProcessData,
                 PCONSOLE* Console,
                 BOOL LockConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE ProcessConsole;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    ProcessConsole = ProcessData->Console;

    if (ConSrvValidateConsole(ProcessConsole, CONSOLE_RUNNING, LockConsole))
    {
        InterlockedIncrement(&ProcessConsole->ReferenceCount);
        *Console = ProcessConsole;
    }
    else
    {
        *Console = NULL;
        Status = STATUS_INVALID_HANDLE;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return Status;
}

VOID FASTCALL
ConSrvReleaseConsole(PCONSOLE Console,
                     BOOL WasConsoleLocked)
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

VOID WINAPI
ConSrvInitConsoleSupport(VOID)
{
    DPRINT("CONSRV: ConSrvInitConsoleSupport()\n");

    /* Initialize the console list and its lock */
    InitializeListHead(&ConsoleList);
    RtlInitializeResource(&ListLock);

    /* Should call LoadKeyboardLayout */
}

static BOOL
LoadShellLinkConsoleInfo(IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                         IN OUT PCONSOLE_INFO ConsoleInfo,
                         OUT LPWSTR IconPath,
                         IN SIZE_T IconPathLength,
                         OUT PINT piIcon)
{
#define PATH_SEPARATOR L'\\'

    BOOL    RetVal   = FALSE;
    HRESULT hRes     = S_OK;
    LPWSTR  LinkName = NULL;
    SIZE_T  Length   = 0;

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
    hRes = CoInitialize(NULL);
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

                    /* Reset the name of the console with the name of the shortcut */
                    Length = min(/*Length*/ Length - 4, // 4 == len(".lnk")
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
     * 3. Check whether the process creating the console was launched
     *    via a shell-link. ConsoleInfo.ConsoleTitle may be updated by
     *    the name of the shortcut.
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
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEFILLATTRIBUTE)
        {
            ConsoleInfo.ScreenAttrib = (USHORT)ConsoleStartInfo->FillAttribute;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USECOUNTCHARS)
        {
            ConsoleInfo.ScreenBufferSize = ConsoleStartInfo->ScreenBufferSize;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESIZE)
        {
            // ConsoleInfo->ConsoleSize = ConsoleStartInfo->ConsoleWindowSize;
            ConsoleInfo.ConsoleSize.X = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cx;
            ConsoleInfo.ConsoleSize.Y = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cy;
        }
        /*
        if (ConsoleStartInfo->dwStartupFlags & STARTF_RUNFULLSCREEN)
        {
            ConsoleInfo.FullScreen = TRUE;
        }
        */
    }

    /*
     * Initialize the console
     */
    Console->State = CONSOLE_INITIALIZING;
    InitializeCriticalSection(&Console->Lock);
    Console->ReferenceCount = 0;
    InitializeListHead(&Console->ProcessList);
    memcpy(Console->Colors, ConsoleInfo.Colors, sizeof(ConsoleInfo.Colors));
    Console->ConsoleSize = ConsoleInfo.ConsoleSize;

    /*
     * Initialize the input buffer
     */
    Console->InputBuffer.Header.Type = INPUT_BUFFER;
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

    Console->InputBuffer.Mode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                                ENABLE_ECHO_INPUT      | ENABLE_MOUSE_INPUT;
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
                                      ConsoleInfo.PopupAttrib,
                                      (ConsoleInfo.FullScreen ? CONSOLE_FULLSCREEN_MODE
                                                              : CONSOLE_WINDOWED_MODE),
                                      TRUE,
                                      ConsoleInfo.CursorSize);
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
    InitializeListHead(&Console->WriteWaitQueue);

    /*
     * Initialize the history buffers
     */
    InitializeListHead(&Console->HistoryBuffers);
    Console->HistoryBufferSize = ConsoleInfo.HistoryBufferSize;
    Console->NumberOfHistoryBuffers = ConsoleInfo.NumberOfHistoryBuffers;
    Console->HistoryNoDup = ConsoleInfo.HistoryNoDup;

    /* Initialize the console title */
    ConsoleCreateUnicodeString(&Console->OriginalTitle, ConsoleInfo.ConsoleTitle);
    if (ConsoleInfo.ConsoleTitle[0] == L'\0')
    {
        if (LoadStringW(ConSrvDllInstance, IDS_CONSOLE_TITLE, Title, sizeof(Title) / sizeof(Title[0])))
        {
            ConsoleCreateUnicodeString(&Console->Title, Title);
        }
        else
        {
            ConsoleCreateUnicodeString(&Console->Title, L"ReactOS Console");
        }
    }
    else
    {
        ConsoleCreateUnicodeString(&Console->Title, ConsoleInfo.ConsoleTitle);
    }

    /* Lock the console until its initialization is finished */
    // EnterCriticalSection(&Console->Lock);

    /*
     * If we are not in GUI-mode, start the text-mode terminal emulator.
     * If we fail, try to start the GUI-mode terminal emulator.
     */
    GuiMode = DtbgIsDesktopVisible();

    if (!GuiMode)
    {
        DPRINT("CONSRV: Opening text-mode terminal emulator\n");
        Status = TuiInitConsole(Console,
                                ConsoleStartInfo,
                                &ConsoleInfo,
                                ProcessId);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open text-mode terminal emulator, switching to gui-mode, Status = 0x%08lx\n", Status);
            GuiMode = TRUE;
        }
    }

    /*
     * Try to open the GUI-mode terminal emulator. Two cases are possible:
     * - We are in GUI-mode, therefore GuiMode == TRUE, the previous test-case
     *   failed and we start GUI-mode terminal emulator.
     * - We are in text-mode, therefore GuiMode == FALSE, the previous test-case
     *   succeeded BUT we failed at starting text-mode terminal emulator.
     *   Then GuiMode was switched to TRUE in order to try to open the GUI-mode
     *   terminal emulator (Win32k will automatically switch to graphical mode,
     *   therefore no additional code is needed).
     */
    if (GuiMode)
    {
        DPRINT("CONSRV: Opening GUI-mode terminal emulator\n");
        Status = GuiInitConsole(Console,
                                ConsoleStartInfo,
                                &ConsoleInfo,
                                ProcessId,
                                IconPath,
                                iIcon);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("GuiInitConsole: failed, Status = 0x%08lx\n", Status);
            ConsoleFreeUnicodeString(&Console->OriginalTitle);
            ConsoleFreeUnicodeString(&Console->Title);
            ConioDeleteScreenBuffer(NewBuffer);
            CloseHandle(Console->InputBuffer.ActiveEvent);
            // LeaveCriticalSection(&Console->Lock);
            DeleteCriticalSection(&Console->Lock);
            RtlFreeHeap(ConSrvHeap, 0, Console);
            return Status;
        }
    }

    DPRINT("Terminal initialized\n");

    /* All went right, so add the console to the list */
    ConSrvLockConsoleListExclusive();
    DPRINT("Insert in the list\n");
    InsertTailList(&ConsoleList, &Console->Entry);

    /* The initialization is finished */
    DPRINT("Change state\n");
    Console->State = CONSOLE_RUNNING;

    /* Unlock the console */
    // LeaveCriticalSection(&Console->Lock);

    /* Unlock the console list */
    ConSrvUnlockConsoleList();

    /* Copy buffer contents to screen */
    ConioDrawConsole(Console);
    DPRINT("Console drawn\n");

    /* Return the newly created console to the caller and a success code too */
    *NewConsole = Console;
    return STATUS_SUCCESS;
}

VOID WINAPI
ConSrvDeleteConsole(PCONSOLE Console)
{
    DPRINT("ConSrvDeleteConsole\n");

    /*
     * Forbid validation of any console by other threads
     * during the deletion of this console.
     */
    ConSrvLockConsoleListExclusive();

    /* Check the existence of the console, and if it's ok, continue */
    if (!ConSrvValidateConsolePointer(Console))
    {
        /* Unlock the console list and return */
        ConSrvUnlockConsoleList();
        return;
    }

    /*
     * If the console is already being destroyed
     * (thus not running), just return.
     */
    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        /* Unlock the console list and return */
        ConSrvUnlockConsoleList();
        return;
    }

    /*
     * We are about to be destroyed. Signal it to other people
     * so that they can terminate what they are doing, and that
     * they cannot longer validate the console.
     */
    Console->State = CONSOLE_TERMINATING;

    /*
     * Allow other threads to finish their job: basically, unlock
     * all other calls to EnterCriticalSection(&Console->Lock); by
     * ConSrvValidateConsole(Unsafe) functions so that they just see
     * that we are not in CONSOLE_RUNNING state anymore, or unlock
     * other concurrent calls to ConSrvDeleteConsole so that they
     * can see that we are in fact already deleting the console.
     */
    LeaveCriticalSection(&Console->Lock);
    ConSrvUnlockConsoleList();

    /* FIXME: Send a terminate message to all the processes owning this console */

    /* Cleanup the UI-oriented part */
    ConioCleanupConsole(Console);

    /***
     * Check that the console is in terminating state before continuing
     * (the cleanup code must not change the state of the console...
     * ...unless to cancel console deletion ?).
     ***/

    ConSrvLockConsoleListExclusive();

    /* Re-check the existence of the console, and if it's ok, continue */
    if (!ConSrvValidateConsolePointer(Console))
    {
        /* Unlock the console list and return */
        ConSrvUnlockConsoleList();
        return;
    }

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_TERMINATING, TRUE))
    {
        ConSrvUnlockConsoleList();
        return;
    }

    /* We are in destruction */
    Console->State = CONSOLE_IN_DESTRUCTION;

    /* Remove the console from the list */
    RemoveEntryList(&Console->Entry);

    /* Reset the count to be sure */
    Console->ReferenceCount = 0;

    /* Discard all entries in the input event queue */
    PurgeInputBuffer(Console);

    if (Console->LineBuffer) RtlFreeHeap(ConSrvHeap, 0, Console->LineBuffer);

    IntDeleteAllAliases(Console);
    HistoryDeleteBuffers(Console);

    ConioDeleteScreenBuffer(Console->ActiveBuffer);
    if (!IsListEmpty(&Console->BufferList))
    {
        DPRINT1("BUG: screen buffer list not empty\n");
    }

    // CloseHandle(Console->InputBuffer.ActiveEvent);
    if (Console->UnpauseEvent) CloseHandle(Console->UnpauseEvent);

    ConsoleFreeUnicodeString(&Console->OriginalTitle);
    ConsoleFreeUnicodeString(&Console->Title);

    DPRINT("ConSrvDeleteConsole - Unlocking\n");
    LeaveCriticalSection(&Console->Lock);
    DPRINT("ConSrvDeleteConsole - Destroying lock\n");
    DeleteCriticalSection(&Console->Lock);
    DPRINT("ConSrvDeleteConsole - Lock destroyed ; freeing console\n");

    RtlFreeHeap(ConSrvHeap, 0, Console);
    DPRINT("ConSrvDeleteConsole - Console freed\n");

    /* Unlock the console list and return */
    ConSrvUnlockConsoleList();
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvAllocConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_ALLOCCONSOLE AllocConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.AllocConsoleRequest;
    PCSR_PROCESS CsrProcess = CsrGetClientThread()->Process;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    if (ProcessData->Console != NULL)
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

    /* Return it to the caller */
    AllocConsoleRequest->Console = ProcessData->Console;

    /* Input Wait Handle */
    AllocConsoleRequest->InputWaitHandle = ProcessData->ConsoleEvent;

    /* Set the Property Dialog Handler */
    ProcessData->PropDispatcher = AllocConsoleRequest->PropDispatcher;

    /* Set the Ctrl Dispatcher */
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

        ProcessId = ULongToHandle(ProcessInfo.InheritedFromUniqueProcessId);
    }

    /* Lock the source process via its PID */
    Status = CsrLockProcessByClientId(ProcessId, &SourceProcess);
    if (!NT_SUCCESS(Status)) return Status;

    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

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

CSR_API(SrvGetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE_IO_OBJECT Object = NULL;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->ConsoleHandle,
                             &Object, NULL, GENERIC_READ, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = STATUS_SUCCESS;

    if (INPUT_BUFFER == Object->Type)
    {
        PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;
        PCONSOLE Console  = InputBuffer->Header.Console;
        DWORD ConsoleMode = InputBuffer->Mode;

        if (Console->QuickEdit || Console->InsertMode)
        {
            // Windows does this, even if it's not documented on MSDN
            ConsoleMode |= ENABLE_EXTENDED_FLAGS;

            if (Console->QuickEdit ) ConsoleMode |= ENABLE_QUICK_EDIT_MODE;
            if (Console->InsertMode) ConsoleMode |= ENABLE_INSERT_MODE;
        }

        ConsoleModeRequest->ConsoleMode = ConsoleMode;
    }
    else if (SCREEN_BUFFER == Object->Type)
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

CSR_API(SrvSetConsoleMode)
{
#define CONSOLE_VALID_CONTROL_MODES ( ENABLE_EXTENDED_FLAGS   | ENABLE_INSERT_MODE  | ENABLE_QUICK_EDIT_MODE )
#define CONSOLE_VALID_INPUT_MODES   ( ENABLE_PROCESSED_INPUT  | ENABLE_LINE_INPUT   | \
                                      ENABLE_ECHO_INPUT       | ENABLE_WINDOW_INPUT | \
                                      ENABLE_MOUSE_INPUT )
#define CONSOLE_VALID_OUTPUT_MODES  ( ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT )

    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    DWORD ConsoleMode = ConsoleModeRequest->ConsoleMode;
    PCONSOLE_IO_OBJECT Object  = NULL;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->ConsoleHandle,
                             &Object, NULL, GENERIC_WRITE, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = STATUS_SUCCESS;

    if (INPUT_BUFFER == Object->Type)
    {
        PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;
        PCONSOLE Console = InputBuffer->Header.Console;

        DPRINT("SetConsoleMode(Input, %d)\n", ConsoleMode);

        /*
         * 1. Only the presence of valid mode flags is allowed.
         */
        if (ConsoleMode & ~(CONSOLE_VALID_INPUT_MODES | CONSOLE_VALID_CONTROL_MODES))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quit;
        }

        /*
         * 2. If we use control mode flags without ENABLE_EXTENDED_FLAGS,
         *    then consider the flags invalid.
         *
        if ( (ConsoleMode & CONSOLE_VALID_CONTROL_MODES) &&
             (ConsoleMode & ENABLE_EXTENDED_FLAGS) == 0 )
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quit;
        }
        */

        /*
         * 3. Now we can continue.
         */
        if (ConsoleMode & CONSOLE_VALID_CONTROL_MODES)
        {
            Console->QuickEdit  = !!(ConsoleMode & ENABLE_QUICK_EDIT_MODE);
            Console->InsertMode = !!(ConsoleMode & ENABLE_INSERT_MODE);
        }
        InputBuffer->Mode = (ConsoleMode & CONSOLE_VALID_INPUT_MODES);
    }
    else if (SCREEN_BUFFER == Object->Type)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;

        DPRINT("SetConsoleMode(Output, %d)\n", ConsoleMode);

        if (ConsoleMode & ~CONSOLE_VALID_OUTPUT_MODES)
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            Buffer->Mode = (ConsoleMode & CONSOLE_VALID_OUTPUT_MODES);
        }
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

Quit:
    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCONSOLE Console;
    DWORD Length;

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

CSR_API(SrvSetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    // PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCONSOLE Console;
    PWCHAR Buffer;

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
    Buffer = RtlAllocateHeap(ConSrvHeap, 0, TitleRequest->Length + sizeof(WCHAR));
    if (Buffer)
    {
        /* Free the old title */
        ConsoleFreeUnicodeString(&Console->Title);

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
SetConsoleHardwareState(PCONSOLE Console, ULONG ConsoleHwState)
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

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   HardwareStateRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
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

CSR_API(SrvGetConsoleDisplayMode)
{
    NTSTATUS Status;
    PCONSOLE_GETDISPLAYMODE GetDisplayModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetDisplayModeRequest;
    PCONSOLE Console;
    ULONG DisplayMode = 0;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvGetConsoleDisplayMode\n");
        return Status;
    }

    if (Console->ActiveBuffer->DisplayMode & CONSOLE_FULLSCREEN_MODE)
        DisplayMode |= CONSOLE_FULLSCREEN_HARDWARE; // CONSOLE_FULLSCREEN
    else if (Console->ActiveBuffer->DisplayMode & CONSOLE_WINDOWED_MODE)
        DisplayMode |= CONSOLE_WINDOWED;

    GetDisplayModeRequest->DisplayMode = DisplayMode;
    Status = STATUS_SUCCESS;

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvSetConsoleDisplayMode)
{
    NTSTATUS Status;
    PCONSOLE_SETDISPLAYMODE SetDisplayModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetDisplayModeRequest;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   SetDisplayModeRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvSetConsoleDisplayMode\n");
        return Status;
    }

    if (SetDisplayModeRequest->DisplayMode & ~(CONSOLE_FULLSCREEN_MODE | CONSOLE_WINDOWED_MODE))
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        Buff->DisplayMode = SetDisplayModeRequest->DisplayMode;
        // TODO: Change the display mode
        SetDisplayModeRequest->NewSBDim = Buff->ScreenBufferSize;

        Status = STATUS_SUCCESS;
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
}

CSR_API(SrvGetLargestConsoleWindowSize)
{
    NTSTATUS Status;
    PCONSOLE_GETLARGESTWINDOWSIZE GetLargestWindowSizeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetLargestWindowSizeRequest;
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE Console;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   GetLargestWindowSizeRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_READ,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;
    ConioGetLargestConsoleWindowSize(Console, &GetLargestWindowSizeRequest->Size);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleWindowInfo)
{
#if 0
    NTSTATUS Status;
#endif
    PCONSOLE_SETWINDOWINFO SetWindowInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetWindowInfoRequest;
#if 0
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE Console;
#endif
    SMALL_RECT WindowRect = SetWindowInfoRequest->WindowRect;

#if 0
    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   SetWindowInfoRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_READ,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    if (SetWindowInfoRequest->Absolute == FALSE)
    {
        /* Relative positions given. Transform them to absolute ones */
        WindowRect.Left   += Buff->ShowX;
        WindowRect.Top    += Buff->ShowY;
        WindowRect.Right  += Buff->ShowX + Console->ConsoleSize.X - 1;
        WindowRect.Bottom += Buff->ShowY + Console->ConsoleSize.Y - 1;
    }

    if ( (WindowRect.Left < 0) || (WindowRect.Top < 0) ||
         (WindowRect.Right  > ScreenBufferSize.X)      ||
         (WindowRect.Bottom > ScreenBufferSize.Y)      ||
         (WindowRect.Right  <= WindowRect.Left)        ||
         (WindowRect.Bottom <= WindowRect.Top) )
    {
        ConSrvReleaseScreenBuffer(Buff, TRUE);
        return STATUS_INVALID_PARAMETER;
    }

    Buff->ShowX = WindowRect.Left;
    Buff->ShowY = WindowRect.Top;

    // These two lines are frontend-specific.
    Console->ConsoleSize.X = WindowRect.Right - WindowRect.Left + 1;
    Console->ConsoleSize.Y = WindowRect.Bottom - WindowRect.Top + 1;

    // ConioGetLargestConsoleWindowSize(Console, &GetLargestWindowSizeRequest->Size);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
#else
    DPRINT1("SrvSetConsoleWindowInfo(0x%08x, %d, {L%d, T%d, R%d, B%d}) UNIMPLEMENTED\n",
            SetWindowInfoRequest->OutputHandle, SetWindowInfoRequest->Absolute,
            WindowRect.Left, WindowRect.Top, WindowRect.Right, WindowRect.Bottom);
    return STATUS_NOT_IMPLEMENTED;
#endif
}

CSR_API(SrvGetConsoleWindow)
{
    NTSTATUS Status;
    PCONSOLE_GETWINDOW GetWindowRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetWindowRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetWindowRequest->WindowHandle = ConioGetConsoleWindowHandle(Console);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleIcon)
{
    NTSTATUS Status;
    PCONSOLE_SETICON SetIconRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetIconRequest;
    PCONSOLE Console;

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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConSrvConsoleProcessCtrlEvent(Console,
                                           GenerateCtrlEventRequest->ProcessGroup,
                                           GenerateCtrlEventRequest->Event);

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

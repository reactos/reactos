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

/* This is for COM usage */
#define COBJMACROS
#include <shlobj.h>


#include <alias.h>
#include <history.h>
#include "procinit.h"

#define NDEBUG
#include <debug.h>

// FIXME: Add this prototype to winternl.h / rtlfuncs.h / ...
NTSTATUS NTAPI RtlGetLastNtStatus(VOID);

/* GLOBALS ********************************************************************/

/* The list of the ConSrv consoles */
static ULONG ConsoleListSize;
static PCONSRV_CONSOLE* ConsoleList;
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
    DPRINT("Insert in the list\n");

    if (ConsoleList)
    {
        for (i = 0; i < ConsoleListSize; i++)
        {
            if (ConsoleList[i] == NULL) break;
        }
    }

    if (i >= ConsoleListSize)
    {
        DPRINT("Creation of a new handles table\n");
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

    /* Unlock the console list and return */
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
        /* Unlock the console list and return */
        ConSrvUnlockConsoleList();
        return FALSE;
    }

    ValidatedConsole = ConsoleList[Index];

    /* Unlock the console list and return */
    ConSrvUnlockConsoleList();

    RetVal = ConDrvValidateConsoleUnsafe((PCONSOLE)ValidatedConsole,
                                         ExpectedState,
                                         LockConsole);
    if (RetVal) *Console = ValidatedConsole;

    return RetVal;
}


/* PRIVATE FUNCTIONS **********************************************************/

// Adapted from reactos/lib/rtl/unicode.c, RtlCreateUnicodeString line 2180
static BOOLEAN
ConsoleCreateUnicodeString(IN OUT PUNICODE_STRING UniDest,
                           IN PCWSTR Source)
{
    SIZE_T Size = (wcslen(Source) + 1) * sizeof(WCHAR);
    if (Size > MAXUSHORT) return FALSE;

    UniDest->Buffer = ConsoleAllocHeap(HEAP_ZERO_MEMORY, Size);
    if (UniDest->Buffer == NULL) return FALSE;

    RtlCopyMemory(UniDest->Buffer, Source, Size);
    UniDest->MaximumLength = (USHORT)Size;
    UniDest->Length = (USHORT)Size - sizeof(WCHAR);

    return TRUE;
}

// Adapted from reactos/lib/rtl/unicode.c, RtlFreeUnicodeString line 431
static VOID
ConsoleFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
    if (UnicodeString->Buffer)
    {
        ConsoleFreeHeap(UnicodeString->Buffer);
        RtlZeroMemory(UnicodeString, sizeof(UNICODE_STRING));
    }
}

VOID
ConioPause(PCONSRV_CONSOLE Console, UINT Flags)
{
    Console->PauseFlags |= Flags;
    ConDrvPause((PCONSOLE)Console);
}

VOID
ConioUnpause(PCONSRV_CONSOLE Console, UINT Flags)
{
    Console->PauseFlags &= ~Flags;

    // if ((Console->PauseFlags & (PAUSED_FROM_KEYBOARD | PAUSED_FROM_SCROLLBAR | PAUSED_FROM_SELECTION)) == 0)
    if (Console->PauseFlags == 0)
    {
        ConDrvUnpause((PCONSOLE)Console);

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

    if (ConSrvValidateConsole(&GrabConsole,
                              ProcessData->ConsoleHandle,
                              CONSOLE_RUNNING,
                              LockConsole))
    {
        InterlockedIncrement(&GrabConsole->ReferenceCount);
        *Console = GrabConsole;
        Status = STATUS_SUCCESS;
    }

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


static BOOL
LoadShellLinkConsoleInfo(IN OUT PCONSOLE_INFO ConsoleInfo,
                         IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo)
{
#define PATH_SEPARATOR L'\\'

    BOOL    RetVal   = FALSE;
    HRESULT hRes     = S_OK;
    SIZE_T  Length   = 0;
    LPWSTR  LinkName = NULL;
    LPWSTR  IconPath = NULL;
    WCHAR   Buffer[MAX_PATH + 1];

    ConsoleInitInfo->ConsoleStartInfo->IconIndex = 0;

    if ((ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
    {
        // return FALSE; // FIXME!! (for icon loading)
        RetVal = TRUE;
        goto Finish;
    }

    /* 1- Find the last path separator if any */
    LinkName = wcsrchr(ConsoleInfo->ConsoleTitle, PATH_SEPARATOR);
    if (LinkName == NULL)
        LinkName = ConsoleInfo->ConsoleTitle;
    else
        ++LinkName; // Skip the path separator

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
                hRes = IPersistFile_Load(ppf, ConsoleInfo->ConsoleTitle, STGM_READ);
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
                    if (SUCCEEDED(hRes)) ConsoleInitInfo->ConsoleStartInfo->wShowWindow = (WORD)ShowCmd;

                    /* Get the hotkey */
                    // hRes = pshl->GetHotkey(&ShowCmd);
                    // if (SUCCEEDED(hRes)) ConsoleInitInfo->ConsoleStartInfo->HotKey = HotKey;

                    /* Get the icon location, if any */
                    hRes = IShellLinkW_GetIconLocation(pshl,
                                                       Buffer,
                                                       sizeof(Buffer)/sizeof(Buffer[0]) - 1, // == MAX_PATH
                                                       &ConsoleInitInfo->ConsoleStartInfo->IconIndex);
                    if (!SUCCEEDED(hRes))
                    {
                        ConsoleInitInfo->ConsoleStartInfo->IconIndex = 0;
                    }
                    else
                    {
                        IconPath = Buffer;
                    }

                    // FIXME: Since we still don't load console properties from the shortcut,
                    // return false. When this will be done, we will return true instead.
                    RetVal = TRUE; // FALSE;
                }
                IPersistFile_Release(ppf);
            }
            IShellLinkW_Release(pshl);
        }
    }
    CoUninitialize();

Finish:

    if (RetVal)
    {
        /* Get the associated icon, if any */
        if (IconPath == NULL)
        {
            // Question: How to retrieve the full path name
            // of the app we are going to run??
            Length = RtlDosSearchPath_U(ConsoleInitInfo->CurDir,
                                        ConsoleInitInfo->AppName,
                                        NULL,
                                        sizeof(Buffer),
                                        Buffer,
                                        NULL);
            if (Length > 0 && Length < sizeof(Buffer))
                IconPath = Buffer;
            else
                IconPath = ConsoleInitInfo->AppName;

            // ConsoleInitInfo->ConsoleStartInfo->IconIndex = 0;
        }
        DPRINT("IconPath = '%S' ; IconIndex = %lu\n",
               IconPath, ConsoleInitInfo->ConsoleStartInfo->IconIndex);
        if (IconPath && *IconPath)
        {
            HICON hIcon = NULL, hIconSm = NULL;
            PrivateExtractIconExW(IconPath,
                                  ConsoleInitInfo->ConsoleStartInfo->IconIndex,
                                  &hIcon,
                                  &hIconSm,
                                  1);
            DPRINT("hIcon = 0x%p ; hIconSm = 0x%p\n", hIcon, hIconSm);
            if (hIcon   != NULL) ConsoleInitInfo->ConsoleStartInfo->hIcon   = hIcon;
            if (hIconSm != NULL) ConsoleInitInfo->ConsoleStartInfo->hIconSm = hIconSm;
        }
    }

    // FIXME: See the previous FIXME above.
    RetVal = FALSE;

    return RetVal;
}

NTSTATUS NTAPI
ConSrvInitConsole(OUT PHANDLE NewConsoleHandle,
                  OUT PCONSRV_CONSOLE* NewConsole,
                  IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                  IN ULONG ConsoleLeaderProcessId)
{
    NTSTATUS Status;
    HANDLE ConsoleHandle;
    PCONSRV_CONSOLE Console;
    CONSOLE_INFO ConsoleInfo;
    SIZE_T Length = 0;

    TERMINAL Terminal; /* The ConSrv terminal for this console */

    if (NewConsole == NULL || ConsoleInitInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *NewConsole = NULL;

    /*
     * Load the console settings
     */

    /* 1. Load the default settings */
    ConSrvGetDefaultSettings(&ConsoleInfo, ConsoleLeaderProcessId);

    /* 2. Get the title of the console (initialize ConsoleInfo.ConsoleTitle) */
    Length = min(ConsoleInitInfo->TitleLength,
                 sizeof(ConsoleInfo.ConsoleTitle) / sizeof(ConsoleInfo.ConsoleTitle[0]) - 1);
    wcsncpy(ConsoleInfo.ConsoleTitle, ConsoleInitInfo->ConsoleTitle, Length);
    ConsoleInfo.ConsoleTitle[Length] = L'\0'; // NULL-terminate it.

    /* 3. Initialize the ConSrv terminal */
    Status = ConSrvInitTerminal(&Terminal,
                                &ConsoleInfo,
                                ConsoleInitInfo,
                                ConsoleLeaderProcessId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CONSRV: Failed to initialize a terminal, Status = 0x%08lx\n", Status);
        return Status;
    }
    DPRINT("CONSRV: Terminal initialized\n");

    /*
     * Load per-application terminal settings.
     *
     * Check whether the process creating the console was launched via
     * a shell-link. ConsoleInfo->ConsoleTitle may be updated with the
     * name of the shortcut, and ConsoleStartInfo->Icon[Path|Index] too.
     */
    // if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) // FIXME!! (for icon loading)
    {
        if (!LoadShellLinkConsoleInfo(&ConsoleInfo, ConsoleInitInfo))
        {
            ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags &= ~STARTF_TITLEISLINKNAME;
        }
    }

    /*
     * 4. Load the remaining console settings via the registry.
     */
    if ((ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
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
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_USEFILLATTRIBUTE)
        {
            ConsoleInfo.ScreenAttrib = (USHORT)ConsoleInitInfo->ConsoleStartInfo->wFillAttribute;
        }
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_USECOUNTCHARS)
        {
            ConsoleInfo.ScreenBufferSize = ConsoleInitInfo->ConsoleStartInfo->dwScreenBufferSize;
        }
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_USESIZE)
        {
            ConsoleInfo.ConsoleSize = ConsoleInitInfo->ConsoleStartInfo->dwWindowSize;
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

    /* Initialize the console title */
#if 0
    WCHAR DefaultTitle[128];
#endif
    ConsoleCreateUnicodeString(&Console->OriginalTitle, ConsoleInfo.ConsoleTitle);
#if 0
    if (ConsoleInfo.ConsoleTitle[0] == L'\0')
    {
        if (LoadStringW(ConSrvDllInstance, IDS_CONSOLE_TITLE, DefaultTitle, sizeof(DefaultTitle) / sizeof(DefaultTitle[0])))
        {
            ConsoleCreateUnicodeString(&Console->Title, DefaultTitle);
        }
        else
        {
            ConsoleCreateUnicodeString(&Console->Title, L"ReactOS Console");
        }
    }
    else
    {
#endif
        ConsoleCreateUnicodeString(&Console->Title, ConsoleInfo.ConsoleTitle);
#if 0
    }
#endif

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

    /* Initialize the Input Line Discipline */
    Console->LineBuffer = NULL;
    Console->LinePos = Console->LineMaxSize = Console->LineSize = 0;
    Console->LineComplete = Console->LineUpPressed = FALSE;
    // LineWakeupMask
    Console->LineInsertToggle =
    Console->InsertMode = ConsoleInfo.InsertMode;
    Console->QuickEdit  = ConsoleInfo.QuickEdit;

    /* Popup windows */
    InitializeListHead(&Console->PopupWindows);

    /* Colour table */
    memcpy(Console->Colors, ConsoleInfo.Colors, sizeof(ConsoleInfo.Colors));

    /* Create the Initialization Events */
    Status = NtCreateEvent(&Console->InitEvents[INIT_SUCCESS], EVENT_ALL_ACCESS,
                           NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateEvent(InitEvents[INIT_SUCCESS]) failed: %lu\n", Status);
        ConDrvDeleteConsole(Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }
    Status = NtCreateEvent(&Console->InitEvents[INIT_FAILURE], EVENT_ALL_ACCESS,
                           NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateEvent(InitEvents[INIT_FAILURE]) failed: %lu\n", Status);
        NtClose(Console->InitEvents[INIT_SUCCESS]);
        ConDrvDeleteConsole(Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }

    /*
     * Attach the ConSrv terminal to the console.
     * This call makes a copy of our local Terminal variable.
     */
    Status = ConDrvRegisterTerminal(Console, &Terminal);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register terminal to the given console, Status = 0x%08lx\n", Status);
        NtClose(Console->InitEvents[INIT_FAILURE]);
        NtClose(Console->InitEvents[INIT_SUCCESS]);
        ConDrvDeleteConsole(Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }
    DPRINT("Terminal registered\n");

    /* All went right, so add the console to the list */
    Status = InsertConsole(&ConsoleHandle, Console);

    // FIXME! We do not support at all asynchronous console creation!
    NtSetEvent(Console->InitEvents[INIT_SUCCESS], NULL);
    // NtSetEvent(Console->InitEvents[INIT_FAILURE], NULL);

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

    /* Destroy the Initialization Events */
    NtClose(Console->InitEvents[INIT_FAILURE]);
    NtClose(Console->InitEvents[INIT_SUCCESS]);

    /* Clean the Input Line Discipline */
    if (Console->LineBuffer) ConsoleFreeHeap(Console->LineBuffer);

    /* Clean aliases and history */
    IntDeleteAllAliases(Console);
    HistoryDeleteBuffers(Console);

    /* Free the console title */
    ConsoleFreeUnicodeString(&Console->OriginalTitle);
    ConsoleFreeUnicodeString(&Console->Title);

    /* Now, call the driver. ConDrvDeregisterTerminal is called on-demand. */
    ConDrvDeleteConsole((PCONSOLE)Console);

    /* Deinit the ConSrv terminal */
    // FIXME!!
    // ConSrvDeinitTerminal(&Terminal); // &ConSrvConsole->Console->TermIFace
}






static NTSTATUS
ConSrvConsoleCtrlEventTimeout(IN ULONG CtrlEvent,
                              IN PCONSOLE_PROCESS_DATA ProcessData,
                              IN ULONG Timeout)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ConSrvConsoleCtrlEventTimeout Parent ProcessId = %x\n", ProcessData->Process->ClientId.UniqueProcess);

    if (ProcessData->CtrlRoutine)
    {
        _SEH2_TRY
        {
            HANDLE Thread = NULL;

            _SEH2_TRY
            {
                Thread = CreateRemoteThread(ProcessData->Process->ProcessHandle, NULL, 0,
                                            ProcessData->CtrlRoutine,
                                            UlongToPtr(CtrlEvent), 0, NULL);
                if (NULL == Thread)
                {
                    Status = RtlGetLastNtStatus();
                    DPRINT1("Failed thread creation, Status = 0x%08lx\n", Status);
                }
                else
                {
                    DPRINT("ProcessData->CtrlRoutine remote thread creation succeeded, ProcessId = %x, Process = 0x%p\n",
                           ProcessData->Process->ClientId.UniqueProcess, ProcessData->Process);
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
    if (!ConDrvValidateConsoleState((PCONSOLE)Console, CONSOLE_RUNNING))
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
    CONSOLE_INIT_INFO ConsoleInitInfo;

    if (ProcessData->ConsoleHandle != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&AllocConsoleRequest->ConsoleStartInfo,
                                   1,
                                   sizeof(CONSOLE_START_INFO))      ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&AllocConsoleRequest->ConsoleTitle,
                                   AllocConsoleRequest->TitleLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&AllocConsoleRequest->Desktop,
                                   AllocConsoleRequest->DesktopLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&AllocConsoleRequest->CurDir,
                                   AllocConsoleRequest->CurDirLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&AllocConsoleRequest->AppName,
                                   AllocConsoleRequest->AppNameLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize the console initialization info structure */
    ConsoleInitInfo.ConsoleStartInfo = AllocConsoleRequest->ConsoleStartInfo;
    ConsoleInitInfo.IsWindowVisible  = TRUE; // The console window is always visible.
    ConsoleInitInfo.TitleLength      = AllocConsoleRequest->TitleLength;
    ConsoleInitInfo.ConsoleTitle     = AllocConsoleRequest->ConsoleTitle;
    ConsoleInitInfo.DesktopLength    = AllocConsoleRequest->DesktopLength;
    ConsoleInitInfo.Desktop          = AllocConsoleRequest->Desktop;
    ConsoleInitInfo.AppNameLength    = AllocConsoleRequest->AppNameLength;
    ConsoleInitInfo.AppName          = AllocConsoleRequest->AppName;
    ConsoleInitInfo.CurDirLength     = AllocConsoleRequest->CurDirLength;
    ConsoleInitInfo.CurDir           = AllocConsoleRequest->CurDir;

    /* Initialize a new Console owned by the Console Leader Process */
    Status = ConSrvAllocateConsole(ProcessData,
                                   &AllocConsoleRequest->ConsoleStartInfo->InputHandle,
                                   &AllocConsoleRequest->ConsoleStartInfo->OutputHandle,
                                   &AllocConsoleRequest->ConsoleStartInfo->ErrorHandle,
                                   &ConsoleInitInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console allocation failed\n");
        return Status;
    }

    /* Set the Property-Dialog and Control-Dispatcher handlers */
    ProcessData->PropRoutine = AllocConsoleRequest->PropRoutine;
    ProcessData->CtrlRoutine = AllocConsoleRequest->CtrlRoutine;

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

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&AttachConsoleRequest->ConsoleStartInfo,
                                  1,
                                  sizeof(CONSOLE_START_INFO)))
    {
        return STATUS_INVALID_PARAMETER;
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
            DPRINT1("SrvAttachConsole - Cannot retrieve basic process info, Status = 0x%08lx\n", Status);
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
                                  &AttachConsoleRequest->ConsoleStartInfo->InputHandle,
                                  &AttachConsoleRequest->ConsoleStartInfo->OutputHandle,
                                  &AttachConsoleRequest->ConsoleStartInfo->ErrorHandle,
                                  AttachConsoleRequest->ConsoleStartInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console inheritance failed\n");
        goto Quit;
    }

    /* Set the Property-Dialog and Control-Dispatcher handlers */
    TargetProcessData->PropRoutine = AttachConsoleRequest->PropRoutine;
    TargetProcessData->CtrlRoutine = AttachConsoleRequest->CtrlRoutine;

    Status = STATUS_SUCCESS;

Quit:
    /* Unlock the "source" process and exit */
    CsrUnlockProcess(SourceProcess);
    return Status;
}

CSR_API(SrvFreeConsole)
{
    return ConSrvRemoveConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process));
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

    PULONG ConsoleMode = &ConsoleModeRequest->Mode;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->Handle,
                             &Object, NULL, GENERIC_READ, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the standard console modes */
    Status = ConDrvGetConsoleMode(Object->Console, Object,
                                  ConsoleMode);
    if (NT_SUCCESS(Status))
    {
        /*
         * If getting the console modes succeeds, then retrieve
         * the extended CONSRV-specific input modes.
         */
        if (INPUT_BUFFER == Object->Type)
        {
            if (Object->Console->InsertMode || Object->Console->QuickEdit)
            {
                /* Windows also adds ENABLE_EXTENDED_FLAGS, even if it's not documented on MSDN */
                *ConsoleMode |= ENABLE_EXTENDED_FLAGS;

                if (Object->Console->InsertMode) *ConsoleMode |= ENABLE_INSERT_MODE;
                if (Object->Console->QuickEdit ) *ConsoleMode |= ENABLE_QUICK_EDIT_MODE;
            }
        }
    }

    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     IN ULONG ConsoleMode);
CSR_API(SrvSetConsoleMode)
{
#define CONSOLE_VALID_CONTROL_MODES ( ENABLE_EXTENDED_FLAGS | \
                                      ENABLE_INSERT_MODE    | ENABLE_QUICK_EDIT_MODE )

    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE_IO_OBJECT Object;

    ULONG ConsoleMode = ConsoleModeRequest->Mode;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->Handle,
                             &Object, NULL, GENERIC_WRITE, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    /* Set the standard console modes (without the CONSRV-specific input modes) */
    ConsoleMode &= ~CONSOLE_VALID_CONTROL_MODES; // Remove CONSRV-specific input modes.
    Status = ConDrvSetConsoleMode(Object->Console, Object,
                                  ConsoleMode);
    if (NT_SUCCESS(Status))
    {
        /*
         * If setting the console modes succeeds, then set
         * the extended CONSRV-specific input modes.
         */
        if (INPUT_BUFFER == Object->Type)
        {
            ConsoleMode = ConsoleModeRequest->Mode;

            if (ConsoleMode & CONSOLE_VALID_CONTROL_MODES)
            {
                /*
                 * If we use control mode flags without ENABLE_EXTENDED_FLAGS,
                 * then consider the flags invalid.
                 */
                if ((ConsoleMode & ENABLE_EXTENDED_FLAGS) == 0)
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    Object->Console->InsertMode = !!(ConsoleMode & ENABLE_INSERT_MODE);
                    Object->Console->QuickEdit  = !!(ConsoleMode & ENABLE_QUICK_EDIT_MODE);
                }
            }
        }
    }

    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    PCONSRV_CONSOLE Console;
    ULONG Length;

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
    if (TitleRequest->Unicode)
    {
        if (TitleRequest->Length >= sizeof(WCHAR))
        {
            Length = min(TitleRequest->Length - sizeof(WCHAR), Console->Title.Length);
            RtlCopyMemory(TitleRequest->Title, Console->Title.Buffer, Length);
            ((PWCHAR)TitleRequest->Title)[Length / sizeof(WCHAR)] = L'\0';
            TitleRequest->Length = Length;
        }
        else
        {
            TitleRequest->Length = Console->Title.Length;
        }
    }
    else
    {
        if (TitleRequest->Length >= sizeof(CHAR))
        {
            Length = min(TitleRequest->Length - sizeof(CHAR), Console->Title.Length / sizeof(WCHAR));
            Length = WideCharToMultiByte(Console->InputCodePage, 0,
                                         Console->Title.Buffer, Length,
                                         TitleRequest->Title, Length,
                                         NULL, NULL);
            ((PCHAR)TitleRequest->Title)[Length] = '\0';
            TitleRequest->Length = Length;
        }
        else
        {
            TitleRequest->Length = Console->Title.Length / sizeof(WCHAR);
        }
    }

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvSetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    PCONSRV_CONSOLE Console;

    PWCHAR Buffer;
    ULONG  Length;

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

    if (TitleRequest->Unicode)
    {
        /* Length is in bytes */
        Length = TitleRequest->Length;
    }
    else
    {
        /* Use the console input CP for the conversion */
        Length = MultiByteToWideChar(Console->InputCodePage, 0,
                                     TitleRequest->Title, TitleRequest->Length,
                                     NULL, 0);
        /* The returned Length was in number of wchars, convert it in bytes */
        Length *= sizeof(WCHAR);
    }

    /* Allocate a new buffer to hold the new title (NULL-terminated) */
    Buffer = ConsoleAllocHeap(HEAP_ZERO_MEMORY, Length + sizeof(WCHAR));
    if (!Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    /* Free the old title */
    ConsoleFreeUnicodeString(&Console->Title);

    /* Copy title to console */
    Console->Title.Buffer = Buffer;
    Console->Title.Length = Length;
    Console->Title.MaximumLength = Console->Title.Length + sizeof(WCHAR);

    if (TitleRequest->Unicode)
    {
        RtlCopyMemory(Console->Title.Buffer, TitleRequest->Title, Console->Title.Length);
    }
    else
    {
        MultiByteToWideChar(Console->InputCodePage, 0,
                            TitleRequest->Title, TitleRequest->Length,
                            Console->Title.Buffer,
                            Console->Title.Length / sizeof(WCHAR));
    }

    /* NULL-terminate */
    Console->Title.Buffer[Console->Title.Length / sizeof(WCHAR)] = L'\0';

    TermChangeTitle(Console);
    Status = STATUS_SUCCESS;

Quit:
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

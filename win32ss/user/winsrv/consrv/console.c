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

#include "../concfg/font.h"
#include <alias.h>
#include <history.h>
#include "procinit.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

// static ULONG CurrentConsoleID = 0;

/* The list of the ConSrv consoles */
static ULONG ConsoleListSize;
static PCONSRV_CONSOLE* ConsoleList;
// static LIST_ENTRY ConDrvConsoleList;
static RTL_RESOURCE ListLock;

#define ConSrvLockConsoleListExclusive()    \
    RtlAcquireResourceExclusive(&ListLock, TRUE)

#define ConSrvLockConsoleListShared()       \
    RtlAcquireResourceShared(&ListLock, TRUE)

#define ConSrvUnlockConsoleList()           \
    RtlReleaseResource(&ListLock)

#if 0
static NTSTATUS
ConDrvInsertConsole(
    IN PCONSOLE Console)
{
    ASSERT(Console);

    /* All went right, so add the console to the list */
    ConSrvLockConsoleListExclusive();

    DPRINT("Insert in the list\n");
    InsertTailList(&ConDrvConsoleList, &Console->ListEntry);

    // FIXME: Move this code to the caller function!!
    /* Get a new console ID */
    _InterlockedExchange((PLONG)&Console->ConsoleID, CurrentConsoleID);
    _InterlockedIncrement((PLONG)&CurrentConsoleID);

    /* Unlock the console list and return success */
    ConSrvUnlockConsoleList();
    return STATUS_SUCCESS;
}
#endif

static NTSTATUS
InsertConsole(
    OUT PHANDLE Handle,
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

#if 0
static NTSTATUS
RemoveConsole(IN PCONSOLE Console)
{
    // ASSERT(Console);
    if (!Console) return STATUS_INVALID_PARAMETER;

    /* Remove the console from the list */
    ConSrvLockConsoleListExclusive();

    RemoveEntryList(&Console->ListEntry);

    /* Unlock the console list and return success */
    ConSrvUnlockConsoleList();
    return STATUS_SUCCESS;
}
#endif


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


/* CONSOLE VALIDATION FUNCTIONS ***********************************************/

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

    /* Unlock the console list */
    ConSrvUnlockConsoleList();

    RetVal = ConDrvValidateConsoleUnsafe((PCONSOLE)ValidatedConsole,
                                         ExpectedState,
                                         LockConsole);
    if (RetVal) *Console = ValidatedConsole;

    return RetVal;
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
        _InterlockedIncrement(&GrabConsole->ReferenceCount);
        *Console = GrabConsole;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
ConSrvReleaseConsole(IN PCONSRV_CONSOLE Console,
                     IN BOOLEAN IsConsoleLocked)
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
    if (IsConsoleLocked) LeaveCriticalSection(&Console->Lock);

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
    // InitializeListHead(&ConDrvConsoleList);
    RtlInitializeResource(&ListLock);

    /* Should call LoadKeyboardLayout */
}

NTSTATUS NTAPI
ConSrvInitTerminal(IN OUT PTERMINAL Terminal,
                   IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
                   IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                   IN HANDLE ConsoleLeaderProcessHandle);
NTSTATUS NTAPI
ConSrvDeinitTerminal(IN OUT PTERMINAL Terminal);


static BOOL
LoadShellLinkConsoleInfo(IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
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
    if ( (Length <= 4) || (_wcsicmp(LinkName + (Length - 4), L".lnk") != 0) )
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
                                 (ConsoleInfo->cbSize - FIELD_OFFSET(CONSOLE_STATE_INFO, ConsoleTitle) - sizeof(UNICODE_NULL)) / sizeof(WCHAR));
                    wcsncpy(ConsoleInfo->ConsoleTitle, LinkName, Length);
                    ConsoleInfo->ConsoleTitle[Length] = UNICODE_NULL;

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
        CoUninitialize();
    }

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
            /*
             * FIXME!! Because of a strange bug we have in PrivateExtractIconExW
             * (see r65683 for more details), we cannot use this API to extract
             * at the same time the large and small icons from the app.
             * Instead we just use PrivateExtractIconsW.
             *
            PrivateExtractIconExW(IconPath,
                                  ConsoleInitInfo->ConsoleStartInfo->IconIndex,
                                  &hIcon,
                                  &hIconSm,
                                  1);
             */
            PrivateExtractIconsW(IconPath,
                                 ConsoleInitInfo->ConsoleStartInfo->IconIndex,
                                 32, 32,
                                 &hIcon, NULL, 1, LR_COPYFROMRESOURCE);
            PrivateExtractIconsW(IconPath,
                                 ConsoleInitInfo->ConsoleStartInfo->IconIndex,
                                 16, 16,
                                 &hIconSm, NULL, 1, LR_COPYFROMRESOURCE);

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
                  IN PCSR_PROCESS ConsoleLeaderProcess)
{
    NTSTATUS Status;
    HANDLE ConsoleHandle;
    PCONSRV_CONSOLE Console;

    BYTE ConsoleInfoBuffer[sizeof(CONSOLE_STATE_INFO) + MAX_PATH * sizeof(WCHAR)]; // CONSRV console information
    PCONSOLE_STATE_INFO ConsoleInfo = (PCONSOLE_STATE_INFO)&ConsoleInfoBuffer;
    CONSOLE_INFO DrvConsoleInfo; // Console information for CONDRV

    SIZE_T Length = 0;

    TERMINAL Terminal; /* The ConSrv terminal for this console */

    if (NewConsole == NULL || ConsoleInitInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *NewConsole = NULL;

    DPRINT("Initialization of console '%S' for process '%S' on desktop '%S'\n",
           ConsoleInitInfo->ConsoleTitle ? ConsoleInitInfo->ConsoleTitle : L"n/a",
           ConsoleInitInfo->AppName ? ConsoleInitInfo->AppName : L"n/a",
           ConsoleInitInfo->Desktop ? ConsoleInitInfo->Desktop : L"n/a");

    /*
     * Load the console settings
     */
    RtlZeroMemory(ConsoleInfo, sizeof(ConsoleInfoBuffer));
    ConsoleInfo->cbSize = sizeof(ConsoleInfoBuffer);

    /* 1. Get the title of the console (initialize ConsoleInfo->ConsoleTitle) */
    Length = min(ConsoleInitInfo->TitleLength,
                 (ConsoleInfo->cbSize - FIELD_OFFSET(CONSOLE_STATE_INFO, ConsoleTitle) - sizeof(UNICODE_NULL)) / sizeof(WCHAR));
    wcsncpy(ConsoleInfo->ConsoleTitle, ConsoleInitInfo->ConsoleTitle, Length);
    ConsoleInfo->ConsoleTitle[Length] = UNICODE_NULL; // NULL-terminate it.

    /* 2. Impersonate the caller in order to retrieve settings in its context */
    if (!CsrImpersonateClient(NULL))
        return STATUS_BAD_IMPERSONATION_LEVEL;

    /* 3. Load the default settings */
    ConCfgGetDefaultSettings(ConsoleInfo);

    /*
     * 4. Load per-application terminal settings.
     *
     * Check whether the process creating the console was launched via
     * a shell-link. ConsoleInfo->ConsoleTitle may be updated with the
     * name of the shortcut, and ConsoleStartInfo->Icon[Path|Index] too.
     */
    // if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) // FIXME!! (for icon loading)
    {
        if (!LoadShellLinkConsoleInfo(ConsoleInfo, ConsoleInitInfo))
        {
            ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags &= ~STARTF_TITLEISLINKNAME;
        }
    }

    /*
     * 5. Load the remaining console settings via the registry.
     */
    if ((ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
    {
        /*
         * Either we weren't created by an app launched via a shell-link,
         * or we failed to load shell-link console properties.
         * Therefore, load the console infos for the application from the registry.
         */
        ConCfgReadUserSettings(ConsoleInfo, FALSE);

        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_USEFILLATTRIBUTE)
        {
            ConsoleInfo->ScreenAttributes = (USHORT)ConsoleInitInfo->ConsoleStartInfo->wFillAttribute;
        }
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_USECOUNTCHARS)
        {
            ConsoleInfo->ScreenBufferSize = ConsoleInitInfo->ConsoleStartInfo->dwScreenBufferSize;
        }
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_USESIZE)
        {
            ConsoleInfo->WindowSize = ConsoleInitInfo->ConsoleStartInfo->dwWindowSize;
        }

#if 0
        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_USEPOSITION)
        {
            ConsoleInfo->AutoPosition = FALSE;
            ConsoleInfo->WindowPosition.x = ConsoleInitInfo->ConsoleStartInfo->dwWindowOrigin.X;
            ConsoleInfo->WindowPosition.y = ConsoleInitInfo->ConsoleStartInfo->dwWindowOrigin.Y;
        }
        if (ConsoleInitInfo->ConsoleStartInfo->dwStartupFlags & STARTF_RUNFULLSCREEN)
        {
            ConsoleInfo->FullScreen = TRUE;
        }
#endif
    }

    /* 6. Revert impersonation */
    CsrRevertToSelf();

    /* Set-up the code page */
    ConsoleInfo->CodePage = GetOEMCP();

    /*
     * Initialize the ConSrv terminal and give it a chance to load
     * its own settings and override the console settings.
     */
    Status = ConSrvInitTerminal(&Terminal,
                                ConsoleInfo,
                                ConsoleInitInfo,
                                ConsoleLeaderProcess->ProcessHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CONSRV: Failed to initialize a terminal, Status = 0x%08lx\n", Status);
        return Status;
    }
    DPRINT("CONSRV: Terminal initialized\n");

    /* Initialize a new console via the driver */
    // DrvConsoleInfo.InputBufferSize = 0;
    DrvConsoleInfo.ScreenBufferSize = ConsoleInfo->ScreenBufferSize;
    DrvConsoleInfo.ConsoleSize = ConsoleInfo->WindowSize;
    DrvConsoleInfo.CursorSize = ConsoleInfo->CursorSize;
    // DrvConsoleInfo.CursorBlinkOn = ConsoleInfo->CursorBlinkOn;
    DrvConsoleInfo.ScreenAttrib = ConsoleInfo->ScreenAttributes;
    DrvConsoleInfo.PopupAttrib = ConsoleInfo->PopupAttributes;
    DrvConsoleInfo.CodePage = ConsoleInfo->CodePage;

    /*
     * Allocate a new console
     */
    Console = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(*Console));
    if (Console == NULL)
    {
        DPRINT1("Not enough memory for console creation.\n");
        ConSrvDeinitTerminal(&Terminal);
        return STATUS_NO_MEMORY;
    }

    Status = ConDrvInitConsole((PCONSOLE)Console, &DrvConsoleInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Creating a new console failed, Status = 0x%08lx\n", Status);
        ConsoleFreeHeap(Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }

    DPRINT("Console initialized\n");

    /*** Register ConSrv features ***/

    /* Initialize the console title */
#if 0
    WCHAR DefaultTitle[128];
#endif
    ConsoleCreateUnicodeString(&Console->OriginalTitle, ConsoleInfo->ConsoleTitle);
#if 0
    if (ConsoleInfo->ConsoleTitle[0] == UNICODE_NULL)
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
        ConsoleCreateUnicodeString(&Console->Title, ConsoleInfo->ConsoleTitle);
#if 0
    }
#endif

    /* Initialize process support */
    // InitProcessSupport(Console);
    InitializeListHead(&Console->ProcessList);
    Console->NotifiedLastCloseProcess = NULL;
    Console->NotifyLastClose = FALSE;
    Console->HasFocus = FALSE;

    /* Initialize pausing support */
    Console->PauseFlags = 0;
    InitializeListHead(&Console->ReadWaitQueue);
    InitializeListHead(&Console->WriteWaitQueue);

    /* Initialize the alias and history buffers */
    // InitAliasesHistory(Console);
    Console->Aliases = NULL;
    InitializeListHead(&Console->HistoryBuffers);
    Console->NumberOfHistoryBuffers = 0;
    Console->MaxNumberOfHistoryBuffers = ConsoleInfo->NumberOfHistoryBuffers;
    Console->HistoryBufferSize = ConsoleInfo->HistoryBufferSize;
    Console->HistoryNoDup      = ConsoleInfo->HistoryNoDup;

    /* Initialize the Input Line Discipline */
    // InitLineInput(Console);
    Console->LineBuffer = NULL;
    Console->LinePos = Console->LineMaxSize = Console->LineSize = 0;
    Console->LineComplete = Console->LineUpPressed = FALSE;
    // LineWakeupMask
    Console->LineInsertToggle =
    Console->InsertMode = ConsoleInfo->InsertMode;
    Console->QuickEdit  = ConsoleInfo->QuickEdit;

    /* Popup windows */
    InitializeListHead(&Console->PopupWindows);

    /* Colour table */
    RtlCopyMemory(Console->Colors, ConsoleInfo->ColorTable,
                  sizeof(ConsoleInfo->ColorTable));

    /* Create the Initialization Events */
    Status = NtCreateEvent(&Console->InitEvents[INIT_SUCCESS], EVENT_ALL_ACCESS,
                           NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateEvent(InitEvents[INIT_SUCCESS]) failed: %lu\n", Status);
        ConDrvDeleteConsole((PCONSOLE)Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }
    Status = NtCreateEvent(&Console->InitEvents[INIT_FAILURE], EVENT_ALL_ACCESS,
                           NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateEvent(InitEvents[INIT_FAILURE]) failed: %lu\n", Status);
        NtClose(Console->InitEvents[INIT_SUCCESS]);
        ConDrvDeleteConsole((PCONSOLE)Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }

    /*
     * Attach the ConSrv terminal to the console.
     * This call makes a copy of our local Terminal variable.
     */
    Status = ConDrvAttachTerminal((PCONSOLE)Console, &Terminal);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register terminal to the given console, Status = 0x%08lx\n", Status);
        NtClose(Console->InitEvents[INIT_FAILURE]);
        NtClose(Console->InitEvents[INIT_SUCCESS]);
        ConDrvDeleteConsole((PCONSOLE)Console);
        ConSrvDeinitTerminal(&Terminal);
        return Status;
    }
    DPRINT("Terminal attached\n");

    /* All went right, so add the console to the list */
#if 0
    Status = ConDrvInsertConsole((PCONSOLE)Console);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ConDrvDeleteConsole((PCONSOLE)Console);
        return Status;
    }
#endif
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

    // FIXME: Send a terminate message to all the processes owning this console.
    // NOTE: In principle there should be none, because such processes would
    // have a reference to the console and thus this function would not have
    // been called in the first place.

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

    /* Now, call the driver. ConDrvDetachTerminal is called on-demand. */
    ConDrvDeleteConsole((PCONSOLE)Console);

    /* Deinit the ConSrv terminal */
    // FIXME!!
    // ConSrvDeinitTerminal(&Terminal);
}


VOID
ConioPause(PCONSRV_CONSOLE Console, UCHAR Flags)
{
    Console->PauseFlags |= Flags;
    ConDrvPause((PCONSOLE)Console);
}

VOID
ConioUnpause(PCONSRV_CONSOLE Console, UCHAR Flags)
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


/* CONSOLE PROCESS INITIALIZATION FUNCTIONS ***********************************/

static NTSTATUS
ConSrvInitProcessHandles(
    IN OUT PCONSOLE_PROCESS_DATA ProcessData,
    IN PCONSRV_CONSOLE Console,
    OUT PHANDLE pInputHandle,
    OUT PHANDLE pOutputHandle,
    OUT PHANDLE pErrorHandle)
{
    NTSTATUS Status;
    HANDLE InputHandle  = INVALID_HANDLE_VALUE,
           OutputHandle = INVALID_HANDLE_VALUE,
           ErrorHandle  = INVALID_HANDLE_VALUE;

    /*
     * Initialize the process handles. Use temporary variables to store
     * the handles values in such a way that, if we fail, we don't
     * return to the caller invalid handle values.
     *
     * Insert the IO handles.
     */

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    /* Insert the Input handle */
    Status = ConSrvInsertObject(ProcessData,
                                &InputHandle,
                                &Console->InputBuffer.Header,
                                GENERIC_READ | GENERIC_WRITE,
                                TRUE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the input handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        ConSrvFreeHandlesTable(ProcessData);
        return Status;
    }

    /* Insert the Output handle */
    Status = ConSrvInsertObject(ProcessData,
                                &OutputHandle,
                                &Console->ActiveBuffer->Header,
                                GENERIC_READ | GENERIC_WRITE,
                                TRUE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the output handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        ConSrvFreeHandlesTable(ProcessData);
        return Status;
    }

    /* Insert the Error handle */
    Status = ConSrvInsertObject(ProcessData,
                                &ErrorHandle,
                                &Console->ActiveBuffer->Header,
                                GENERIC_READ | GENERIC_WRITE,
                                TRUE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the error handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        ConSrvFreeHandlesTable(ProcessData);
        return Status;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    /* Return the newly created handles */
    *pInputHandle  = InputHandle;
    *pOutputHandle = OutputHandle;
    *pErrorHandle  = ErrorHandle;

    return STATUS_SUCCESS;
}

NTSTATUS
ConSrvAllocateConsole(
    IN OUT PCONSOLE_PROCESS_DATA ProcessData,
    OUT PHANDLE pInputHandle,
    OUT PHANDLE pOutputHandle,
    OUT PHANDLE pErrorHandle,
    IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE ConsoleHandle;
    PCONSRV_CONSOLE Console;

    /*
     * We are about to create a new console. However when ConSrvNewProcess()
     * was called, we didn't know that we wanted to create a new console and
     * therefore, we by default inherited the handle table from our parent
     * process. It's only now that we notice that in fact we do not need
     * them, because we've created a new console and thus we must use it.
     *
     * Therefore, free the handle table so that we can recreate
     * a new one later on.
     */
    ConSrvFreeHandlesTable(ProcessData);

    /* Initialize a new Console owned by this process */
    Status = ConSrvInitConsole(&ConsoleHandle,
                               &Console,
                               ConsoleInitInfo,
                               ProcessData->Process);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console initialization failed\n");
        return Status;
    }

    /* Assign the new console handle */
    ProcessData->ConsoleHandle = ConsoleHandle;

    /* Initialize the process handles */
    Status = ConSrvInitProcessHandles(ProcessData,
                                      Console,
                                      pInputHandle,
                                      pOutputHandle,
                                      pErrorHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize the process handles\n");
        ConSrvDeleteConsole(Console);
        ProcessData->ConsoleHandle = NULL;
        return Status;
    }

    /* Duplicate the Initialization Events */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InitEvents[INIT_SUCCESS],
                               ProcessData->Process->ProcessHandle,
                               &ConsoleInitInfo->ConsoleStartInfo->InitEvents[INIT_SUCCESS],
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject(InitEvents[INIT_SUCCESS]) failed: %lu\n", Status);
        ConSrvFreeHandlesTable(ProcessData);
        ConSrvDeleteConsole(Console);
        ProcessData->ConsoleHandle = NULL;
        return Status;
    }

    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InitEvents[INIT_FAILURE],
                               ProcessData->Process->ProcessHandle,
                               &ConsoleInitInfo->ConsoleStartInfo->InitEvents[INIT_FAILURE],
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject(InitEvents[INIT_FAILURE]) failed: %lu\n", Status);
        NtDuplicateObject(ProcessData->Process->ProcessHandle,
                          ConsoleInitInfo->ConsoleStartInfo->InitEvents[INIT_SUCCESS],
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        ConSrvFreeHandlesTable(ProcessData);
        ConSrvDeleteConsole(Console);
        ProcessData->ConsoleHandle = NULL;
        return Status;
    }

    /* Duplicate the Input Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InputBuffer.ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ConsoleInitInfo->ConsoleStartInfo->InputWaitHandle,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject(InputWaitHandle) failed: %lu\n", Status);
        NtDuplicateObject(ProcessData->Process->ProcessHandle,
                          ConsoleInitInfo->ConsoleStartInfo->InitEvents[INIT_FAILURE],
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        NtDuplicateObject(ProcessData->Process->ProcessHandle,
                          ConsoleInitInfo->ConsoleStartInfo->InitEvents[INIT_SUCCESS],
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        ConSrvFreeHandlesTable(ProcessData);
        ConSrvDeleteConsole(Console);
        ProcessData->ConsoleHandle = NULL;
        return Status;
    }

    /* Mark the process as having a console */
    ProcessData->ConsoleApp = TRUE;
    ProcessData->Process->Flags |= CsrProcessIsConsoleApp;

    /* Return the console handle to the caller */
    ConsoleInitInfo->ConsoleStartInfo->ConsoleHandle = ProcessData->ConsoleHandle;

    /*
     * Insert the process into the processes list of the console,
     * and set its foreground priority.
     */
    InsertHeadList(&Console->ProcessList, &ProcessData->ConsoleLink);
    ConSrvSetProcessFocus(ProcessData->Process, Console->HasFocus);

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&Console->ReferenceCount);

    /* Update the internal info of the terminal */
    TermRefreshInternalInfo(Console);

    return STATUS_SUCCESS;
}

NTSTATUS
ConSrvInheritConsole(
    IN OUT PCONSOLE_PROCESS_DATA ProcessData,
    IN HANDLE ConsoleHandle,
    IN BOOLEAN CreateNewHandleTable,
    OUT PHANDLE pInputHandle,
    OUT PHANDLE pOutputHandle,
    OUT PHANDLE pErrorHandle,
    IN OUT PCONSOLE_START_INFO ConsoleStartInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSRV_CONSOLE Console;

    /* Validate and lock the console */
    if (!ConSrvValidateConsole(&Console,
                               ConsoleHandle,
                               CONSOLE_RUNNING, TRUE))
    {
        // FIXME: Find another status code
        return STATUS_UNSUCCESSFUL;
    }

    /* Inherit the console */
    ProcessData->ConsoleHandle = ConsoleHandle;

    if (CreateNewHandleTable)
    {
        /*
         * We are about to create a new console. However when ConSrvNewProcess()
         * was called, we didn't know that we wanted to create a new console and
         * therefore, we by default inherited the handle table from our parent
         * process. It's only now that we notice that in fact we do not need
         * them, because we've created a new console and thus we must use it.
         *
         * Therefore, free the handle table so that we can recreate
         * a new one later on.
         */
        ConSrvFreeHandlesTable(ProcessData);

        /* Initialize the process handles */
        Status = ConSrvInitProcessHandles(ProcessData,
                                          Console,
                                          pInputHandle,
                                          pOutputHandle,
                                          pErrorHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to initialize the process handles\n");
            ProcessData->ConsoleHandle = NULL;
            goto Quit;
        }
    }

    /* Duplicate the Initialization Events */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InitEvents[INIT_SUCCESS],
                               ProcessData->Process->ProcessHandle,
                               &ConsoleStartInfo->InitEvents[INIT_SUCCESS],
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject(InitEvents[INIT_SUCCESS]) failed: %lu\n", Status);
        ConSrvFreeHandlesTable(ProcessData);
        ProcessData->ConsoleHandle = NULL;
        goto Quit;
    }

    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InitEvents[INIT_FAILURE],
                               ProcessData->Process->ProcessHandle,
                               &ConsoleStartInfo->InitEvents[INIT_FAILURE],
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject(InitEvents[INIT_FAILURE]) failed: %lu\n", Status);
        NtDuplicateObject(ProcessData->Process->ProcessHandle,
                          ConsoleStartInfo->InitEvents[INIT_SUCCESS],
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        ConSrvFreeHandlesTable(ProcessData);
        ProcessData->ConsoleHandle = NULL;
        goto Quit;
    }

    /* Duplicate the Input Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InputBuffer.ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ConsoleStartInfo->InputWaitHandle,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject(InputWaitHandle) failed: %lu\n", Status);
        NtDuplicateObject(ProcessData->Process->ProcessHandle,
                          ConsoleStartInfo->InitEvents[INIT_FAILURE],
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        NtDuplicateObject(ProcessData->Process->ProcessHandle,
                          ConsoleStartInfo->InitEvents[INIT_SUCCESS],
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        ConSrvFreeHandlesTable(ProcessData); // NOTE: Always free the handle table.
        ProcessData->ConsoleHandle = NULL;
        goto Quit;
    }

    /* Mark the process as having a console */
    ProcessData->ConsoleApp = TRUE;
    ProcessData->Process->Flags |= CsrProcessIsConsoleApp;

    /* Return the console handle to the caller */
    ConsoleStartInfo->ConsoleHandle = ProcessData->ConsoleHandle;

    /*
     * Insert the process into the processes list of the console,
     * and set its foreground priority.
     */
    InsertHeadList(&Console->ProcessList, &ProcessData->ConsoleLink);
    ConSrvSetProcessFocus(ProcessData->Process, Console->HasFocus);

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&Console->ReferenceCount);

    /* Update the internal info of the terminal */
    TermRefreshInternalInfo(Console);

    Status = STATUS_SUCCESS;

Quit:
    /* Unlock the console and return */
    LeaveCriticalSection(&Console->Lock);
    return Status;
}

NTSTATUS
ConSrvRemoveConsole(
    IN OUT PCONSOLE_PROCESS_DATA ProcessData)
{
    PCONSRV_CONSOLE Console;
    PCONSOLE_PROCESS_DATA ConsoleLeaderProcess;

    DPRINT("ConSrvRemoveConsole\n");

    /* Mark the process as not having a console anymore */
    ProcessData->ConsoleApp = FALSE;
    ProcessData->Process->Flags &= ~CsrProcessIsConsoleApp;

    /* Validate and lock the console */
    if (!ConSrvValidateConsole(&Console,
                               ProcessData->ConsoleHandle,
                               CONSOLE_RUNNING, TRUE))
    {
        // FIXME: Find another status code
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT("ConSrvRemoveConsole - Locking OK\n");

    /* Retrieve the console leader process */
    ConsoleLeaderProcess = ConSrvGetConsoleLeaderProcess(Console);

    /* Close all console handles and free the handle table */
    ConSrvFreeHandlesTable(ProcessData);

    /* Detach the process from the console */
    ProcessData->ConsoleHandle = NULL;

    /* Remove the process from the console's list of processes */
    RemoveEntryList(&ProcessData->ConsoleLink);

    /* Check whether the console should send a last close notification */
    if (Console->NotifyLastClose)
    {
        /* If we are removing the process which wants the last close notification... */
        if (ProcessData == Console->NotifiedLastCloseProcess)
        {
            /* ... just reset the flag and the pointer... */
            Console->NotifyLastClose = FALSE;
            Console->NotifiedLastCloseProcess = NULL;
        }
        /*
         * ... otherwise, if we are removing the console leader process
         * (that cannot be the process wanting the notification, because
         * the previous case already dealt with it)...
         */
        else if (ProcessData == ConsoleLeaderProcess)
        {
            /*
             * ... reset the flag first (so that we avoid multiple notifications)
             * and then send the last close notification.
             */
            Console->NotifyLastClose = FALSE;
            ConSrvConsoleCtrlEvent(CTRL_LAST_CLOSE_EVENT, Console->NotifiedLastCloseProcess);

            /* Only now, reset the pointer */
            Console->NotifiedLastCloseProcess = NULL;
        }
    }

    /* Update the internal info of the terminal */
    TermRefreshInternalInfo(Console);

    /* Release the console */
    DPRINT("ConSrvRemoveConsole - Decrement Console->ReferenceCount = %lu\n", Console->ReferenceCount);
    ConSrvReleaseConsole(Console, TRUE);

    return STATUS_SUCCESS;
}


/* CONSOLE PROCESS MANAGEMENT FUNCTIONS ***************************************/

NTSTATUS
ConSrvConsoleCtrlEventTimeout(IN ULONG CtrlEvent,
                              IN PCONSOLE_PROCESS_DATA ProcessData,
                              IN ULONG Timeout)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ConSrvConsoleCtrlEventTimeout Parent ProcessId = %x\n", ProcessData->Process->ClientId.UniqueProcess);

    /*
     * Be sure we effectively have a control routine. It resides in kernel32.dll (client).
     */
    if (ProcessData->CtrlRoutine == NULL) return Status;

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

VOID
ConSrvSetProcessFocus(IN PCSR_PROCESS CsrProcess,
                      IN BOOLEAN SetForeground)
{
    // FIXME: Call NtUserSetInformationProcess (currently unimplemented!)
    // for setting Win32 foreground/background flags.

    if (SetForeground)
        CsrSetForegroundPriority(CsrProcess);
    else
        CsrSetBackgroundPriority(CsrProcess);
}

NTSTATUS NTAPI
ConSrvSetConsoleProcessFocus(IN PCONSRV_CONSOLE Console,
                             IN BOOLEAN SetForeground)
{
    PLIST_ENTRY current_entry;
    PCONSOLE_PROCESS_DATA current;

    /* If the console is already being destroyed, just return */
    if (!ConDrvValidateConsoleState((PCONSOLE)Console, CONSOLE_RUNNING))
        return STATUS_UNSUCCESSFUL;

    /*
     * Loop through the process list, from the most recent process
     * to the oldest one, and for each, set its foreground priority.
     */
    current_entry = Console->ProcessList.Flink;
    while (current_entry != &Console->ProcessList)
    {
        current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
        current_entry = current_entry->Flink;

        ConSrvSetProcessFocus(current->Process, SetForeground);
    }

    return STATUS_SUCCESS;
}


/* PUBLIC SERVER APIS *********************************************************/

/* API_NUMBER: ConsolepAlloc */
CON_API_NOCONSOLE(SrvAllocConsole,
                  CONSOLE_ALLOCCONSOLE, AllocConsoleRequest)
{
    NTSTATUS Status = STATUS_SUCCESS;
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

/* API_NUMBER: ConsolepAttach */
CON_API_NOCONSOLE(SrvAttachConsole,
                  CONSOLE_ATTACHCONSOLE, AttachConsoleRequest)
{
    NTSTATUS Status = STATUS_SUCCESS;
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

        /* Get the real parent's PID */

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

/* API_NUMBER: ConsolepFree */
CON_API_NOCONSOLE(SrvFreeConsole,
                  CONSOLE_FREECONSOLE, FreeConsoleRequest)
{
    /*
     * If this process doesn't have a console handle, bail out immediately.
     * Also the passed console handle should be the same as the process' one.
     */
    if ((FreeConsoleRequest->ConsoleHandle == NULL) ||
        (FreeConsoleRequest->ConsoleHandle != ProcessData->ConsoleHandle))
    {
        return STATUS_INVALID_HANDLE; // STATUS_ACCESS_DENIED;
    }

    return ConSrvRemoveConsole(ProcessData);
}

NTSTATUS NTAPI
ConDrvGetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     OUT PULONG ConsoleMode);
/* API_NUMBER: ConsolepGetMode */
CON_API(SrvGetConsoleMode,
        CONSOLE_GETSETCONSOLEMODE, ConsoleModeRequest)
{
    NTSTATUS Status;
    PCONSOLE_IO_OBJECT Object;
    PULONG ConsoleMode = &ConsoleModeRequest->Mode;

    Status = ConSrvGetObject(ProcessData,
                             ConsoleModeRequest->Handle,
                             &Object, NULL, GENERIC_READ, TRUE, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Object->Console);

    /* Get the standard console modes */
    Status = ConDrvGetConsoleMode((PCONSOLE)Console, Object, ConsoleMode);
    if (NT_SUCCESS(Status))
    {
        /*
         * If getting the console modes succeeds, then retrieve
         * the extended CONSRV-specific input modes.
         */
        if (INPUT_BUFFER == Object->Type)
        {
            if (Console->InsertMode || Console->QuickEdit)
            {
                /* Windows also adds ENABLE_EXTENDED_FLAGS, even if it's not documented on MSDN */
                *ConsoleMode |= ENABLE_EXTENDED_FLAGS;

                if (Console->InsertMode) *ConsoleMode |= ENABLE_INSERT_MODE;
                if (Console->QuickEdit ) *ConsoleMode |= ENABLE_QUICK_EDIT_MODE;
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
/* API_NUMBER: ConsolepSetMode */
CON_API(SrvSetConsoleMode,
        CONSOLE_GETSETCONSOLEMODE, ConsoleModeRequest)
{
#define CONSOLE_VALID_CONTROL_MODES ( ENABLE_EXTENDED_FLAGS | \
                                      ENABLE_INSERT_MODE    | ENABLE_QUICK_EDIT_MODE )
// NOTE: Vista+ ENABLE_AUTO_POSITION is also a control mode.

    NTSTATUS Status;
    PCONSOLE_IO_OBJECT Object;
    ULONG ConsoleMode = ConsoleModeRequest->Mode;

    Status = ConSrvGetObject(ProcessData,
                             ConsoleModeRequest->Handle,
                             &Object, NULL, GENERIC_WRITE, TRUE, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Object->Console);

    /* Set the standard console modes (without the CONSRV-specific input modes) */
    ConsoleMode &= ~CONSOLE_VALID_CONTROL_MODES; // Remove CONSRV-specific input modes.
    Status = ConDrvSetConsoleMode((PCONSOLE)Console, Object, ConsoleMode);
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
                    Console->InsertMode = !!(ConsoleMode & ENABLE_INSERT_MODE);
                    Console->QuickEdit  = !!(ConsoleMode & ENABLE_QUICK_EDIT_MODE);
                }
            }
        }
    }

    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

/* API_NUMBER: ConsolepGetTitle */
CON_API(SrvGetConsoleTitle,
        CONSOLE_GETSETCONSOLETITLE, TitleRequest)
{
    ULONG Length;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Copy title of the console to the user title buffer */
    if (TitleRequest->Unicode)
    {
        if (TitleRequest->Length >= sizeof(WCHAR))
        {
            Length = min(TitleRequest->Length - sizeof(WCHAR), Console->Title.Length);
            RtlCopyMemory(TitleRequest->Title, Console->Title.Buffer, Length);
            ((PWCHAR)TitleRequest->Title)[Length / sizeof(WCHAR)] = UNICODE_NULL;
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
            ((PCHAR)TitleRequest->Title)[Length] = ANSI_NULL;
            TitleRequest->Length = Length;
        }
        else
        {
            TitleRequest->Length = Console->Title.Length / sizeof(WCHAR);
        }
    }

    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetTitle */
CON_API(SrvSetConsoleTitle,
        CONSOLE_GETSETCONSOLETITLE, TitleRequest)
{
    PWCHAR Buffer;
    ULONG  Length;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
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
        return STATUS_NO_MEMORY;

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
    Console->Title.Buffer[Console->Title.Length / sizeof(WCHAR)] = UNICODE_NULL;

    TermChangeTitle(Console);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleCP(IN PCONSOLE Console,
                   OUT PUINT CodePage,
                   IN BOOLEAN OutputCP);
/* API_NUMBER: ConsolepGetCP */
CON_API(SrvGetConsoleCP,
        CONSOLE_GETINPUTOUTPUTCP, GetConsoleCPRequest)
{
    DPRINT("SrvGetConsoleCP, getting %s Code Page\n",
            GetConsoleCPRequest->OutputCP ? "Output" : "Input");

    return ConDrvGetConsoleCP((PCONSOLE)Console,
                              &GetConsoleCPRequest->CodePage,
                              GetConsoleCPRequest->OutputCP);
}

NTSTATUS NTAPI
ConDrvSetConsoleCP(IN PCONSOLE Console,
                   IN UINT CodePage,
                   IN BOOLEAN OutputCP);
/* API_NUMBER: ConsolepSetCP */
CON_API(SrvSetConsoleCP,
        CONSOLE_SETINPUTOUTPUTCP, SetConsoleCPRequest)
{
    DPRINT("SrvSetConsoleCP, setting %s Code Page\n",
            SetConsoleCPRequest->OutputCP ? "Output" : "Input");

    return ConDrvSetConsoleCP((PCONSOLE)Console,
                              SetConsoleCPRequest->CodePage,
                              SetConsoleCPRequest->OutputCP);
}

/* API_NUMBER: ConsolepGetProcessList */
CON_API(SrvGetConsoleProcessList,
        CONSOLE_GETPROCESSLIST, GetProcessListRequest)
{
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetProcessListRequest->ProcessIdsList,
                                  GetProcessListRequest->ProcessCount,
                                  sizeof(DWORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    return ConSrvGetConsoleProcessList(Console,
                                       GetProcessListRequest->ProcessIdsList,
                                       GetProcessListRequest->ProcessCount,
                                       &GetProcessListRequest->ProcessCount);
}

/* API_NUMBER: ConsolepGenerateCtrlEvent */
CON_API(SrvGenerateConsoleCtrlEvent,
        CONSOLE_GENERATECTRLEVENT, GenerateCtrlEventRequest)
{
    return ConSrvConsoleProcessCtrlEvent(Console,
                                         GenerateCtrlEventRequest->ProcessGroupId,
                                         GenerateCtrlEventRequest->CtrlEvent);
}

/* API_NUMBER: ConsolepNotifyLastClose */
CON_API(SrvConsoleNotifyLastClose,
        CONSOLE_NOTIFYLASTCLOSE, NotifyLastCloseRequest)
{
    /* Only one process is allowed to be registered for last close notification */
    if (Console->NotifyLastClose)
        return STATUS_ACCESS_DENIED;

    Console->NotifyLastClose = TRUE;
    Console->NotifiedLastCloseProcess = ProcessData;
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepGetMouseInfo */
CON_API(SrvGetConsoleMouseInfo,
        CONSOLE_GETMOUSEINFO, GetMouseInfoRequest)
{
    /* Just retrieve the number of buttons of the mouse attached to this console */
    GetMouseInfoRequest->NumButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetKeyShortcuts */
CSR_API(SrvSetConsoleKeyShortcuts)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepGetKeyboardLayoutName */
CON_API(SrvGetConsoleKeyboardLayoutName,
        CONSOLE_GETKBDLAYOUTNAME, GetKbdLayoutNameRequest)
{
    /* Retrieve the keyboard layout name of the system */
    if (GetKbdLayoutNameRequest->Ansi)
        GetKeyboardLayoutNameA((PCHAR)GetKbdLayoutNameRequest->LayoutBuffer);
    else
        GetKeyboardLayoutNameW((PWCHAR)GetKbdLayoutNameRequest->LayoutBuffer);

    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepCharType */
CSR_API(SrvGetConsoleCharType)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepSetLocalEUDC */
CSR_API(SrvSetConsoleLocalEUDC)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepSetCursorMode */
CSR_API(SrvSetConsoleCursorMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepGetCursorMode */
CSR_API(SrvGetConsoleCursorMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepGetNlsMode */
CSR_API(SrvGetConsoleNlsMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepSetNlsMode */
CSR_API(SrvSetConsoleNlsMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepGetLangId */
CON_API(SrvGetConsoleLangId,
        CONSOLE_GETLANGID, LangIdRequest)
{
    /*
     * Quoting MS Terminal, see function GetConsoleLangId() at
     * https://github.com/microsoft/terminal/blob/main/src/host/srvinit.cpp#L655
     * "Only attempt to return the Lang ID if the Windows ACP on console
     * launch was an East Asian Code Page."
     *
     * The underlying logic is as follows:
     *
     * - When the current user's UI language is *not* CJK, the user expects
     *   to not see any CJK output to the console by default, even if its
     *   output has been set to a CJK code page (this is possible when CJK
     *   fonts are installed on the system). That is, of course, unless if
     *   the attached console program chooses to actually output CJK text.
     *   Whatever current language of the running program's thread should
     *   be kept: STATUS_NOT_SUPPORTED is returned.
     *
     * - When the current user's UI language *is* CJK, the user expects to
     *   see CJK output to the console by default when its code page is CJK.
     *   A valid LangId is returned in this case to ensure this.
     *   However, if the console code page is not CJK, then it is evident
     *   that CJK text will not be able to be correctly shown, and therefore
     *   we should fall back to a standard language that can be shown, namely
     *   en-US english, instead of keeping the current language.
     */

    BYTE UserCharSet = CodePageToCharSet(GetACP());
    if (!IsCJKCharSet(UserCharSet))
        return STATUS_NOT_SUPPORTED;

    /* Return a "best-suited" language ID corresponding
     * to the active console output code page. */
    switch (Console->OutputCodePage)
    {
/** ReactOS-specific: do nothing if the code page is UTF-8. This will allow
 ** programs to naturally output in whatever current language they are. **/
    case CP_UTF8:
        return STATUS_NOT_SUPPORTED;
/** End ReactOS-specific **/
    case CP_JAPANESE:
        LangIdRequest->LangId = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
        break;
    case CP_KOREAN:
        LangIdRequest->LangId = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
        break;
    case CP_CHINESE_SIMPLIFIED:
        LangIdRequest->LangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
        break;
    case CP_CHINESE_TRADITIONAL:
        LangIdRequest->LangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
        break;
    default:
        /* Default to en-US english otherwise */
        LangIdRequest->LangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        break;
    }

    return STATUS_SUCCESS;
}

/* EOF */

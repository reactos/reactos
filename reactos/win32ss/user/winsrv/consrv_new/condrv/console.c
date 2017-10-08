/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            win32ss/user/winsrv/consrv_new/condrv/console.c
 * PURPOSE:         Console Management Functions
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/conio2.h"
#include "handle.h"
#include "procinit.h"
#include "alias.h"
#include "coninput.h"
#include "conoutput.h"
#include "lineinput.h"
#include "include/settings.h"

#include "include/console.h"
#include "console.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>

// FIXME: Add this prototype to winternl.h / rtlfuncs.h / ...
NTSTATUS NTAPI RtlGetLastNtStatus(VOID);


/* GLOBALS ********************************************************************/

static ULONG ConsoleListSize;
static PCONSOLE* ConsoleList;   /* The list of all the allocated consoles */
static RTL_RESOURCE ListLock;

#define ConDrvLockConsoleListExclusive()    \
    RtlAcquireResourceExclusive(&ListLock, TRUE)

#define ConDrvLockConsoleListShared()       \
    RtlAcquireResourceShared(&ListLock, TRUE)

#define ConDrvUnlockConsoleList()           \
    RtlReleaseResource(&ListLock)

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


static NTSTATUS
InsertConsole(OUT PHANDLE Handle,
              IN PCONSOLE Console)
{
#define CONSOLE_HANDLES_INCREMENT   2 * 3

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i = 0;
    PCONSOLE* Block;

    ASSERT( (ConsoleList == NULL && ConsoleListSize == 0) ||
            (ConsoleList != NULL && ConsoleListSize != 0) );

    /* All went right, so add the console to the list */
    ConDrvLockConsoleListExclusive();
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
                                    CONSOLE_HANDLES_INCREMENT) * sizeof(PCONSOLE));
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
                          ConsoleListSize * sizeof(PCONSOLE));
            ConsoleFreeHeap(ConsoleList);
        }
        ConsoleList = Block;
        ConsoleListSize += CONSOLE_HANDLES_INCREMENT;
    }

    ConsoleList[i] = Console;
    *Handle = ULongToHandle((i << 2) | 0x3);

Quit:
    /* Unlock the console list and return status */
    ConDrvUnlockConsoleList();
    return Status;
}

/* Unused */
#if 0
static NTSTATUS
RemoveConsoleByHandle(IN HANDLE Handle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = HandleToULong(Handle) >> 2;
    PCONSOLE Console;

    ASSERT( (ConsoleList == NULL && ConsoleListSize == 0) ||
            (ConsoleList != NULL && ConsoleListSize != 0) );

    /* Remove the console from the list */
    ConDrvLockConsoleListExclusive();

    if (Index >= ConsoleListSize ||
        (Console = ConsoleList[Index]) == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    ConsoleList[Index] = NULL;

Quit:
    /* Unlock the console list and return status */
    ConDrvUnlockConsoleList();
    return Status;
}
#endif

static NTSTATUS
RemoveConsoleByPointer(IN PCONSOLE Console)
{
    ULONG i = 0;

    if (!Console) return STATUS_INVALID_PARAMETER;

    ASSERT( (ConsoleList == NULL && ConsoleListSize == 0) ||
            (ConsoleList != NULL && ConsoleListSize != 0) );

    /* Remove the console from the list */
    ConDrvLockConsoleListExclusive();

    if (ConsoleList)
    {
        for (i = 0; i < ConsoleListSize; i++)
        {
            if (ConsoleList[i] == Console) ConsoleList[i] = NULL;
        }
    }

    /* Unlock the console list */
    ConDrvUnlockConsoleList();

    return STATUS_SUCCESS;
}


/* For resetting the frontend - defined in dummyfrontend.c */
VOID ResetFrontEnd(IN PCONSOLE Console);


/* PRIVATE FUNCTIONS **********************************************************/

static NTSTATUS
ConDrvConsoleCtrlEventTimeout(IN ULONG Event,
                              IN PCONSOLE_PROCESS_DATA ProcessData,
                              IN ULONG Timeout)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ConDrvConsoleCtrlEventTimeout Parent ProcessId = %x\n", ProcessData->Process->ClientId.UniqueProcess);

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
            DPRINT1("ConDrvConsoleCtrlEventTimeout - Caught an exception, Status = 0x%08lx\n", Status);
        }
        _SEH2_END;
    }

    return Status;
}

static NTSTATUS
ConDrvConsoleCtrlEvent(IN ULONG Event,
                       IN PCONSOLE_PROCESS_DATA ProcessData)
{
    return ConDrvConsoleCtrlEventTimeout(Event, ProcessData, 0);
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


/*
 * Console accessibility check helpers
 */

BOOLEAN NTAPI
ConDrvValidateConsoleState(IN PCONSOLE Console,
                           IN CONSOLE_STATE ExpectedState)
{
    // if (!Console) return FALSE;

    /* The console must be locked */
    // ASSERT(Console_locked);

    return (Console->State == ExpectedState);
}

BOOLEAN NTAPI
ConDrvValidateConsoleUnsafe(IN PCONSOLE Console,
                            IN CONSOLE_STATE ExpectedState,
                            IN BOOLEAN LockConsole)
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
    if (!ConDrvValidateConsoleState(Console, ExpectedState))
    {
        if (LockConsole) LeaveCriticalSection(&Console->Lock);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN NTAPI
ConDrvValidateConsole(OUT PCONSOLE* Console,
                      IN HANDLE ConsoleHandle,
                      IN CONSOLE_STATE ExpectedState,
                      IN BOOLEAN LockConsole)
{
    BOOLEAN RetVal = FALSE;

    ULONG Index = HandleToULong(ConsoleHandle) >> 2;
    PCONSOLE ValidatedConsole;

    if (!Console) return FALSE;
    *Console = NULL;

    /*
     * Forbid creation or deletion of consoles when
     * checking for the existence of a console.
     */
    ConDrvLockConsoleListShared();

    if (Index >= ConsoleListSize ||
        (ValidatedConsole = ConsoleList[Index]) == NULL)
    {
        /* Unlock the console list */
        ConDrvUnlockConsoleList();

        return FALSE;
    }

    ValidatedConsole = ConsoleList[Index];

    /* Unlock the console list and return */
    ConDrvUnlockConsoleList();

    RetVal = ConDrvValidateConsoleUnsafe(ValidatedConsole,
                                         ExpectedState,
                                         LockConsole);
    if (RetVal) *Console = ValidatedConsole;

    return RetVal;
}

NTSTATUS NTAPI
ConDrvGetConsole(OUT PCONSOLE* Console,
                 IN HANDLE ConsoleHandle,
                 IN BOOLEAN LockConsole)
{
    NTSTATUS Status = STATUS_INVALID_HANDLE;
    PCONSOLE GrabConsole;

    if (Console == NULL) return STATUS_INVALID_PARAMETER;
    *Console = NULL;

    if (ConDrvValidateConsole(&GrabConsole,
                              ConsoleHandle,
                              CONSOLE_RUNNING,
                              LockConsole))
    {
        InterlockedIncrement(&GrabConsole->ReferenceCount);
        *Console = GrabConsole;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID NTAPI
ConDrvReleaseConsole(IN PCONSOLE Console,
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
    if (RefCount <= 0) ConDrvDeleteConsole(Console);
}


/* CONSOLE INITIALIZATION FUNCTIONS *******************************************/

VOID NTAPI
ConDrvInitConsoleSupport(VOID)
{
    DPRINT("CONSRV: ConDrvInitConsoleSupport()\n");

    /* Initialize the console list and its lock */
    ConsoleListSize = 0;
    ConsoleList = NULL;
    RtlInitializeResource(&ListLock);

    /* Should call LoadKeyboardLayout */
}

NTSTATUS NTAPI
ConDrvInitConsole(OUT PHANDLE NewConsoleHandle,
                  OUT PCONSOLE* NewConsole,
                  IN PCONSOLE_INFO ConsoleInfo,
                  IN ULONG ConsoleLeaderProcessId)
{
    NTSTATUS Status;
    SECURITY_ATTRIBUTES SecurityAttributes;
    // CONSOLE_INFO CapturedConsoleInfo;
    TEXTMODE_BUFFER_INFO ScreenBufferInfo;
    HANDLE ConsoleHandle;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER NewBuffer;
    // WCHAR DefaultTitle[128];

    if (NewConsoleHandle == NULL || NewConsole == NULL || ConsoleInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *NewConsoleHandle = NULL;
    *NewConsole = NULL;

    /*
     * Allocate a console structure
     */
    Console = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(CONSOLE));
    if (NULL == Console)
    {
        DPRINT1("Not enough memory for console creation.\n");
        return STATUS_NO_MEMORY;
    }

    /*
     * Load the console settings
     */

    /* 1. Load the default settings */
    // ConSrvGetDefaultSettings(ConsoleInfo, ProcessId);

    // /* 2. Get the title of the console (initialize ConsoleInfo.ConsoleTitle) */
    // Length = min(wcslen(ConsoleStartInfo->ConsoleTitle),
                 // sizeof(ConsoleInfo.ConsoleTitle) / sizeof(ConsoleInfo.ConsoleTitle[0]) - 1);
    // wcsncpy(ConsoleInfo.ConsoleTitle, ConsoleStartInfo->ConsoleTitle, Length);
    // ConsoleInfo.ConsoleTitle[Length] = L'\0';

    /*
     * 4. Load the remaining console settings via the registry.
     */
#if 0
    if ((ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
    {
        /*
         * Either we weren't created by an app launched via a shell-link,
         * or we failed to load shell-link console properties.
         * Therefore, load the console infos for the application from the registry.
         */
        ConSrvReadUserSettings(ConsoleInfo, ProcessId);

        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEFILLATTRIBUTE)
        {
            ConsoleInfo->ScreenAttrib = (USHORT)ConsoleStartInfo->FillAttribute;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USECOUNTCHARS)
        {
            ConsoleInfo->ScreenBufferSize = ConsoleStartInfo->ScreenBufferSize;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESIZE)
        {
            // ConsoleInfo->ConsoleSize = ConsoleStartInfo->ConsoleWindowSize;
            ConsoleInfo->ConsoleSize.X = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cx;
            ConsoleInfo->ConsoleSize.Y = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cy;
        }
    }
#endif

    /*
     * Fix the screen buffer size if needed. The rule is:
     * ScreenBufferSize >= ConsoleSize
     */
    if (ConsoleInfo->ScreenBufferSize.X < ConsoleInfo->ConsoleSize.X)
        ConsoleInfo->ScreenBufferSize.X = ConsoleInfo->ConsoleSize.X;
    if (ConsoleInfo->ScreenBufferSize.Y < ConsoleInfo->ConsoleSize.Y)
        ConsoleInfo->ScreenBufferSize.Y = ConsoleInfo->ConsoleSize.Y;

    /*
     * Initialize the console
     */
    Console->State = CONSOLE_INITIALIZING;
    Console->ReferenceCount = 0;
    InitializeCriticalSection(&Console->Lock);
    InitializeListHead(&Console->ProcessList);

    /* Initialize the frontend interface */
    ResetFrontEnd(Console);

    memcpy(Console->Colors, ConsoleInfo->Colors, sizeof(ConsoleInfo->Colors));
    Console->ConsoleSize = ConsoleInfo->ConsoleSize;
    Console->FixedSize   = FALSE; // Value by default; is reseted by the front-ends if needed.

    /*
     * Initialize the input buffer
     */
    ConSrvInitObject(&Console->InputBuffer.Header, INPUT_BUFFER, Console);

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = TRUE;
    Console->InputBuffer.ActiveEvent = CreateEventW(&SecurityAttributes, TRUE, FALSE, NULL);
    if (NULL == Console->InputBuffer.ActiveEvent)
    {
        DeleteCriticalSection(&Console->Lock);
        ConsoleFreeHeap(Console);
        return STATUS_UNSUCCESSFUL;
    }

    Console->InputBuffer.InputBufferSize = 0; // FIXME!
    InitializeListHead(&Console->InputBuffer.InputEvents);
    InitializeListHead(&Console->InputBuffer.ReadWaitQueue);
    Console->InputBuffer.Mode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                                ENABLE_ECHO_INPUT      | ENABLE_MOUSE_INPUT;

    Console->QuickEdit  = ConsoleInfo->QuickEdit;
    Console->InsertMode = ConsoleInfo->InsertMode;
    Console->LineBuffer = NULL;
    Console->LineMaxSize = Console->LineSize = Console->LinePos = 0;
    Console->LineComplete = Console->LineUpPressed = Console->LineInsertToggle = FALSE;
    // LineWakeupMask

    // FIXME: This is terminal-specific !! VV
    RtlZeroMemory(&Console->Selection, sizeof(CONSOLE_SELECTION_INFO));
    Console->Selection.dwFlags = CONSOLE_NO_SELECTION;
    // dwSelectionCursor

    /* Set-up the code page */
    Console->CodePage = Console->OutputCodePage = ConsoleInfo->CodePage;

    /* Initialize a new text-mode screen buffer with default settings */
    ScreenBufferInfo.ScreenBufferSize = ConsoleInfo->ScreenBufferSize;
    ScreenBufferInfo.ScreenAttrib     = ConsoleInfo->ScreenAttrib;
    ScreenBufferInfo.PopupAttrib      = ConsoleInfo->PopupAttrib;
    ScreenBufferInfo.IsCursorVisible  = TRUE;
    ScreenBufferInfo.CursorSize       = ConsoleInfo->CursorSize;

    InitializeListHead(&Console->BufferList);
    Status = ConDrvCreateScreenBuffer(&NewBuffer,
                                      Console,
                                      CONSOLE_TEXTMODE_BUFFER,
                                      &ScreenBufferInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConDrvCreateScreenBuffer: failed, Status = 0x%08lx\n", Status);
        CloseHandle(Console->InputBuffer.ActiveEvent);
        DeleteCriticalSection(&Console->Lock);
        ConsoleFreeHeap(Console);
        return Status;
    }
    /* Make the new screen buffer active */
    Console->ActiveBuffer = NewBuffer;
    InitializeListHead(&Console->WriteWaitQueue);
    Console->PauseFlags = 0;
    Console->UnpauseEvent = NULL;

    /*
     * Initialize the alias and history buffers
     */
    Console->Aliases = NULL;
    InitializeListHead(&Console->HistoryBuffers);
    Console->HistoryBufferSize      = ConsoleInfo->HistoryBufferSize;
    Console->NumberOfHistoryBuffers = ConsoleInfo->NumberOfHistoryBuffers;
    Console->HistoryNoDup           = ConsoleInfo->HistoryNoDup;

    /* Initialize the console title */
    ConsoleCreateUnicodeString(&Console->OriginalTitle, ConsoleInfo->ConsoleTitle);
    // if (ConsoleInfo.ConsoleTitle[0] == L'\0')
    // {
        // if (LoadStringW(ConSrvDllInstance, IDS_CONSOLE_TITLE, DefaultTitle, sizeof(DefaultTitle) / sizeof(DefaultTitle[0])))
        // {
            // ConsoleCreateUnicodeString(&Console->Title, DefaultTitle);
        // }
        // else
        // {
            // ConsoleCreateUnicodeString(&Console->Title, L"ReactOS Console");
        // }
    // }
    // else
    // {
        ConsoleCreateUnicodeString(&Console->Title, ConsoleInfo->ConsoleTitle);
    // }

    /* Lock the console until its initialization is finished */
    // EnterCriticalSection(&Console->Lock);

    DPRINT("Console initialized\n");

    /* All went right, so add the console to the list */
    Status = InsertConsole(&ConsoleHandle, Console);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ConDrvDeleteConsole(Console);
        return Status;
    }

    /* The initialization is finished */
    DPRINT("Change state\n");
    Console->State = CONSOLE_RUNNING;

    /* Unlock the console */
    // LeaveCriticalSection(&Console->Lock);

    /* Return the newly created console to the caller and a success code too */
    *NewConsoleHandle = ConsoleHandle;
    *NewConsole       = Console;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvRegisterFrontEnd(IN PCONSOLE Console,
                       IN PFRONTEND FrontEnd)
{
    NTSTATUS Status;

    if (Console == NULL || FrontEnd == NULL)
        return STATUS_INVALID_PARAMETER;

    /* FIXME: Lock the console before ?? */

    /*
     * Attach the frontend to the console. Use now the TermIFace of the console,
     * and not the user-defined temporary FrontEnd pointer.
     */
    Console->TermIFace = *FrontEnd;
    Console->TermIFace.Console = Console;

    /* Initialize the frontend AFTER having attached it to the console */
    DPRINT("Finish initialization of frontend\n");
    Status = Console->TermIFace.Vtbl->InitFrontEnd(&Console->TermIFace, Console);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("FrontEnd initialization failed, Status = 0x%08lx\n", Status);

        /* We failed, detach the frontend from the console */
        FrontEnd->Console = NULL; // For the caller
        ResetFrontEnd(Console);

        return Status;
    }

    /* Copy buffer contents to screen */
    // FrontEnd.Draw();
    // ConioDrawConsole(Console);
    DPRINT("Console drawn\n");

    DPRINT("Terminal FrontEnd initialization done\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvDeregisterFrontEnd(IN PCONSOLE Console)
{
    if (Console == NULL) return STATUS_INVALID_PARAMETER;

    /* FIXME: Lock the console before ?? */

    /* Deinitialize the frontend BEFORE detaching it from the console */
    Console->TermIFace.Vtbl->DeinitFrontEnd(&Console->TermIFace/*, Console*/);

    /*
     * Detach the frontend from the console:
     * reinitialize the frontend interface.
     */
    ResetFrontEnd(Console);

    DPRINT("Terminal FrontEnd unregistered\n");
    return STATUS_SUCCESS;
}

VOID NTAPI
ConDrvDeleteConsole(IN PCONSOLE Console)
{
    DPRINT("ConDrvDeleteConsole(0x%p)\n", Console);

    /*
     * Forbid validation of any console by other threads
     * during the deletion of this console.
     */
    ConDrvLockConsoleListExclusive();

    /*
     * If the console is already being destroyed, i.e. not running
     * or finishing to be initialized, just return.
     */
    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE) &&
        !ConDrvValidateConsoleUnsafe(Console, CONSOLE_INITIALIZING, TRUE))
    {
        /* Unlock the console list and return */
        ConDrvUnlockConsoleList();
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
     * ConDrvValidateConsole(Unsafe) functions so that they just see
     * that we are not in CONSOLE_RUNNING state anymore, or unlock
     * other concurrent calls to ConDrvDeleteConsole so that they
     * can see that we are in fact already deleting the console.
     */
    LeaveCriticalSection(&Console->Lock);
    ConDrvUnlockConsoleList();

    /* FIXME: Send a terminate message to all the processes owning this console */

    /* Cleanup the UI-oriented part */
    DPRINT("Deregister console\n");
    ConDrvDeregisterFrontEnd(Console);
    DPRINT("Console deregistered\n");

    /***
     * Check that the console is in terminating state before continuing
     * (the cleanup code must not change the state of the console...
     * ...unless to cancel console deletion ?).
     ***/

    ConDrvLockConsoleListExclusive();

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_TERMINATING, TRUE))
    {
        ConDrvUnlockConsoleList();
        return;
    }

    /* We are now in destruction */
    Console->State = CONSOLE_IN_DESTRUCTION;

    /* We really delete the console. Reset the count to be sure. */
    Console->ReferenceCount = 0;

    /* Remove the console from the list */
    RemoveConsoleByPointer(Console);

    /* Discard all entries in the input event queue */
    PurgeInputBuffer(Console);

    if (Console->LineBuffer) ConsoleFreeHeap(Console->LineBuffer);

    IntDeleteAllAliases(Console);
    HistoryDeleteBuffers(Console);

    ConioDeleteScreenBuffer(Console->ActiveBuffer);
    Console->ActiveBuffer = NULL;
    if (!IsListEmpty(&Console->BufferList))
    {
        DPRINT1("BUG: screen buffer list not empty\n");
        ASSERT(FALSE);
    }

    /**/ CloseHandle(Console->InputBuffer.ActiveEvent); /**/
    if (Console->UnpauseEvent) CloseHandle(Console->UnpauseEvent);

    ConsoleFreeUnicodeString(&Console->OriginalTitle);
    ConsoleFreeUnicodeString(&Console->Title);

    DPRINT("ConDrvDeleteConsole - Unlocking\n");
    LeaveCriticalSection(&Console->Lock);
    DPRINT("ConDrvDeleteConsole - Destroying lock\n");
    DeleteCriticalSection(&Console->Lock);
    DPRINT("ConDrvDeleteConsole - Lock destroyed ; freeing console\n");

    ConsoleFreeHeap(Console);
    DPRINT("ConDrvDeleteConsole - Console destroyed\n");

    /* Unlock the console list and return */
    ConDrvUnlockConsoleList();
}


/* PUBLIC DRIVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvGetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     OUT PULONG ConsoleMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Console == NULL || Object == NULL || ConsoleMode == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Object->Console);

    /*** FIXME: */ *ConsoleMode = 0; /***/

    if (INPUT_BUFFER == Object->Type)
    {
        PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;

        *ConsoleMode = InputBuffer->Mode;

        if (Console->QuickEdit || Console->InsertMode)
        {
            // Windows does this, even if it's not documented on MSDN
            *ConsoleMode |= ENABLE_EXTENDED_FLAGS;

            if (Console->QuickEdit ) *ConsoleMode |= ENABLE_QUICK_EDIT_MODE;
            if (Console->InsertMode) *ConsoleMode |= ENABLE_INSERT_MODE;
        }
    }
    else if (TEXTMODE_BUFFER == Object->Type || GRAPHICS_BUFFER == Object->Type)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;
        *ConsoleMode = Buffer->Mode;
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }

    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     IN ULONG ConsoleMode)
{
#define CONSOLE_VALID_CONTROL_MODES ( ENABLE_EXTENDED_FLAGS   | ENABLE_INSERT_MODE  | ENABLE_QUICK_EDIT_MODE )
#define CONSOLE_VALID_INPUT_MODES   ( ENABLE_PROCESSED_INPUT  | ENABLE_LINE_INPUT   | \
                                      ENABLE_ECHO_INPUT       | ENABLE_WINDOW_INPUT | \
                                      ENABLE_MOUSE_INPUT )
#define CONSOLE_VALID_OUTPUT_MODES  ( ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT )

    NTSTATUS Status = STATUS_SUCCESS;

    if (Console == NULL || Object == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Object->Console);

    if (INPUT_BUFFER == Object->Type)
    {
        PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;

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
    else if (TEXTMODE_BUFFER == Object->Type || GRAPHICS_BUFFER == Object->Type)
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
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleTitle(IN PCONSOLE Console,
                      IN OUT PWCHAR Title,
                      IN OUT PULONG BufLength)
{
    ULONG Length;

    if (Console == NULL || Title == NULL || BufLength == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Copy title of the console to the user title buffer */
    if (*BufLength >= sizeof(WCHAR))
    {
        Length = min(*BufLength - sizeof(WCHAR), Console->Title.Length);
        RtlCopyMemory(Title, Console->Title.Buffer, Length);
        Title[Length / sizeof(WCHAR)] = L'\0';
    }

    *BufLength = Console->Title.Length;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleTitle(IN PCONSOLE Console,
                      IN PWCHAR Title,
                      IN ULONG BufLength)
{
    PWCHAR Buffer;

    if (Console == NULL || Title == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Allocate a new buffer to hold the new title (NULL-terminated) */
    Buffer = ConsoleAllocHeap(0, BufLength + sizeof(WCHAR));
    if (!Buffer) return STATUS_NO_MEMORY;

    /* Free the old title */
    ConsoleFreeUnicodeString(&Console->Title);

    /* Copy title to console */
    Console->Title.Buffer = Buffer;
    Console->Title.Length = BufLength;
    Console->Title.MaximumLength = Console->Title.Length + sizeof(WCHAR);
    RtlCopyMemory(Console->Title.Buffer, Title, Console->Title.Length);
    Console->Title.Buffer[Console->Title.Length / sizeof(WCHAR)] = L'\0';

    // ConioChangeTitle(Console);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleCP(IN PCONSOLE Console,
                   OUT PUINT CodePage,
                   IN BOOLEAN InputCP)
{
    if (Console == NULL || CodePage == NULL)
        return STATUS_INVALID_PARAMETER;

    *CodePage = (InputCP ? Console->CodePage : Console->OutputCodePage);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleCP(IN PCONSOLE Console,
                   IN UINT CodePage,
                   IN BOOLEAN InputCP)
{
    if (Console == NULL || !IsValidCodePage(CodePage))
        return STATUS_INVALID_PARAMETER;

    if (InputCP)
        Console->CodePage = CodePage;
    else
        Console->OutputCodePage = CodePage;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleProcessList(IN PCONSOLE Console,
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

// ConDrvGenerateConsoleCtrlEvent
NTSTATUS NTAPI
ConDrvConsoleProcessCtrlEvent(IN PCONSOLE Console,
                              IN ULONG ProcessGroupId,
                              IN ULONG Event)
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
            Status = ConDrvConsoleCtrlEvent(Event, current);
        }
    }

    return Status;
}

/* EOF */

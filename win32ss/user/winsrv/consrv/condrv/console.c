/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            win32ss/user/winsrv/consrv/condrv/console.c
 * PURPOSE:         Console Management Functions
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#include <coninput.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

static ULONG CurrentConsoleID = 0;

/* Linked list of consoles */
static LIST_ENTRY ConsoleList;
static RTL_RESOURCE ListLock;

#define ConDrvLockConsoleListExclusive()    \
    RtlAcquireResourceExclusive(&ListLock, TRUE)

#define ConDrvLockConsoleListShared()       \
    RtlAcquireResourceShared(&ListLock, TRUE)

#define ConDrvUnlockConsoleList()           \
    RtlReleaseResource(&ListLock)


static NTSTATUS
InsertConsole(IN PCONSOLE Console)
{
    ASSERT(Console);

    /* All went right, so add the console to the list */
    ConDrvLockConsoleListExclusive();

    DPRINT("Insert in the list\n");
    InsertTailList(&ConsoleList, &Console->ListEntry);

    // FIXME: Move this code to the caller function!!
    /* Get a new console ID */
    _InterlockedExchange((PLONG)&Console->ConsoleID, CurrentConsoleID);
    _InterlockedIncrement((PLONG)&CurrentConsoleID);

    /* Unlock the console list and return success */
    ConDrvUnlockConsoleList();
    return STATUS_SUCCESS;
}

static NTSTATUS
RemoveConsole(IN PCONSOLE Console)
{
    // ASSERT(Console);
    if (!Console) return STATUS_INVALID_PARAMETER;

    /* Remove the console from the list */
    ConDrvLockConsoleListExclusive();

    RemoveEntryList(&Console->ListEntry);

    /* Unlock the console list and return success */
    ConDrvUnlockConsoleList();
    return STATUS_SUCCESS;
}


/* PRIVATE FUNCTIONS **********************************************************/

VOID NTAPI
ConDrvPause(PCONSOLE Console)
{
    if (!Console->UnpauseEvent)
    {
        NtCreateEvent(&Console->UnpauseEvent, EVENT_ALL_ACCESS,
                      NULL, NotificationEvent, FALSE);
    }
}

VOID NTAPI
ConDrvUnpause(PCONSOLE Console)
{
    if (Console->UnpauseEvent)
    {
        NtSetEvent(Console->UnpauseEvent, NULL);
        NtClose(Console->UnpauseEvent);
        Console->UnpauseEvent = NULL;
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


/* CONSOLE INITIALIZATION FUNCTIONS *******************************************/

VOID NTAPI
ConDrvInitConsoleSupport(VOID)
{
    DPRINT("CONSRV: ConDrvInitConsoleSupport()\n");

    /* Initialize the console list and its lock */
    InitializeListHead(&ConsoleList);
    RtlInitializeResource(&ListLock);

    /* Should call LoadKeyboardLayout */
}

/* For resetting the terminal - defined in dummyterm.c */
VOID ResetTerminal(IN PCONSOLE Console);

NTSTATUS NTAPI
ConDrvInitConsole(OUT PCONSOLE* NewConsole,
                  IN PCONSOLE_INFO ConsoleInfo)
{
    NTSTATUS Status;
    // CONSOLE_INFO CapturedConsoleInfo;
    TEXTMODE_BUFFER_INFO ScreenBufferInfo;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER NewBuffer;

    if (NewConsole == NULL || ConsoleInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *NewConsole = NULL;

    /*
     * Allocate a new console
     */
    Console = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(*Console));
    if (NULL == Console)
    {
        DPRINT1("Not enough memory for console creation.\n");
        return STATUS_NO_MEMORY;
    }

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

    /* Initialize the terminal interface */
    ResetTerminal(Console);

    Console->ConsoleSize = ConsoleInfo->ConsoleSize;
    Console->FixedSize   = FALSE; // Value by default; is reseted by the terminals if needed.

    /* Initialize the input buffer */
    Status = ConDrvInitInputBuffer(Console, 0 /* ConsoleInfo->InputBufferSize */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConDrvInitInputBuffer: failed, Status = 0x%08lx\n", Status);
        DeleteCriticalSection(&Console->Lock);
        ConsoleFreeHeap(Console);
        return Status;
    }

    /* Set-up the code page */
    Console->InputCodePage = Console->OutputCodePage = ConsoleInfo->CodePage;

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
        ConDrvDeinitInputBuffer(Console);
        DeleteCriticalSection(&Console->Lock);
        ConsoleFreeHeap(Console);
        return Status;
    }
    /* Make the new screen buffer active */
    Console->ActiveBuffer = NewBuffer;
    Console->UnpauseEvent = NULL;

    DPRINT("Console initialized\n");

    /* All went right, so add the console to the list */
    Status = InsertConsole(Console);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ConDrvDeleteConsole(Console);
        return Status;
    }

    /* The initialization is finished */
    DPRINT("Change state\n");
    Console->State = CONSOLE_RUNNING;

    /* Return the newly created console to the caller and a success code too */
    *NewConsole = Console;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvRegisterTerminal(IN PCONSOLE Console,
                       IN PTERMINAL Terminal)
{
    NTSTATUS Status;

    if (Console == NULL || Terminal == NULL)
        return STATUS_INVALID_PARAMETER;

    /* FIXME: Lock the console before ?? */

    /*
     * Attach the terminal to the console. Use now the TermIFace of the console,
     * and not the user-defined temporary Terminal pointer.
     */
    Console->TermIFace = *Terminal;
    Console->TermIFace.Console = Console;

    /* Initialize the terminal AFTER having attached it to the console */
    DPRINT("Finish initialization of terminal\n");
    Status = Console->TermIFace.Vtbl->InitTerminal(&Console->TermIFace, Console);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Terminal initialization failed, Status = 0x%08lx\n", Status);

        /* We failed, detach the terminal from the console */
        Terminal->Console = NULL; // For the caller
        ResetTerminal(Console);

        return Status;
    }

    /* Copy buffer contents to screen */
    // Terminal.Draw();
    // ConioDrawConsole(Console);
    DPRINT("Console drawn\n");

    DPRINT("Terminal initialization done\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvDeregisterTerminal(IN PCONSOLE Console)
{
    if (Console == NULL) return STATUS_INVALID_PARAMETER;

    /* FIXME: Lock the console before ?? */

    /* Deinitialize the terminal BEFORE detaching it from the console */
    Console->TermIFace.Vtbl->DeinitTerminal(&Console->TermIFace/*, Console*/);

    /*
     * Detach the terminal from the console:
     * reinitialize the terminal interface.
     */
    ResetTerminal(Console);

    DPRINT("Terminal unregistered\n");
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
     * ConDrvValidateConsoleUnsafe functions so that they just see
     * that we are not in CONSOLE_RUNNING state anymore, or unlock
     * other concurrent calls to ConDrvDeleteConsole so that they
     * can see that we are in fact already deleting the console.
     */
    LeaveCriticalSection(&Console->Lock);
    ConDrvUnlockConsoleList();

    /* FIXME: Send a terminate message to all the processes owning this console */

    /* Deregister the terminal */
    DPRINT("Deregister terminal\n");
    ConDrvDeregisterTerminal(Console);
    DPRINT("Terminal deregistered\n");

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
    RemoveConsole(Console);

    /* Delete the last screen buffer */
    ConDrvDeleteScreenBuffer(Console->ActiveBuffer);
    Console->ActiveBuffer = NULL;
    if (!IsListEmpty(&Console->BufferList))
    {
        /***ConDrvUnlockConsoleList();***/
        ASSERTMSG("BUGBUGBUG!! screen buffer list not empty\n", FALSE);
    }

    /* Deinitialize the input buffer */
    ConDrvDeinitInputBuffer(Console);

    if (Console->UnpauseEvent) CloseHandle(Console->UnpauseEvent);

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

        /* Only the presence of valid mode flags is allowed */
        if (ConsoleMode & ~CONSOLE_VALID_INPUT_MODES)
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            InputBuffer->Mode = (ConsoleMode & CONSOLE_VALID_INPUT_MODES);
        }
    }
    else if (TEXTMODE_BUFFER == Object->Type || GRAPHICS_BUFFER == Object->Type)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;

        /* Only the presence of valid mode flags is allowed */
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

    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleCP(IN PCONSOLE Console,
                   OUT PUINT CodePage,
                   IN BOOLEAN OutputCP)
{
    if (Console == NULL || CodePage == NULL)
        return STATUS_INVALID_PARAMETER;

    *CodePage = (OutputCP ? Console->OutputCodePage : Console->InputCodePage);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleCP(IN PCONSOLE Console,
                   IN UINT CodePage,
                   IN BOOLEAN OutputCP)
{
    if (Console == NULL || !IsValidCodePage(CodePage))
        return STATUS_INVALID_PARAMETER;

    if (OutputCP)
        Console->OutputCodePage = CodePage;
    else
        Console->InputCodePage = CodePage;

    return STATUS_SUCCESS;
}

/* EOF */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/terminal.c
 * PURPOSE:         ConSrv terminal.
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>
#include "../include/vt.h"
#include "concfg/font.h"

// #include "frontends/gui/guiterm.h"
#ifdef TUITERM_COMPILE
#include "frontends/tui/tuiterm.h"
#endif

#define NDEBUG
#include <debug.h>






/********** HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK ************/

/* GLOBALS ********************************************************************/

/*
 * From MSDN:
 * "The lpMultiByteStr and lpWideCharStr pointers must not be the same.
 *  If they are the same, the function fails, and GetLastError returns
 *  ERROR_INVALID_PARAMETER."
 */
#define ConsoleInputUnicodeCharToAnsiChar(Console, dChar, sWChar) \
do { \
    ASSERT((ULONG_PTR)(dChar) != (ULONG_PTR)(sWChar)); \
    WideCharToMultiByte((Console)->InputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL); \
} while (0)

#define ConsoleInputAnsiCharToUnicodeChar(Console, dWChar, sChar) \
do { \
    ASSERT((ULONG_PTR)(dWChar) != (ULONG_PTR)(sChar)); \
    MultiByteToWideChar((Console)->InputCodePage, 0, (sChar), 1, (dWChar), 1); \
} while (0)

/* PRIVATE FUNCTIONS **********************************************************/

#if 0

static VOID
ConioInputEventToAnsi(PCONSOLE Console, PINPUT_RECORD InputEvent)
{
    if (InputEvent->EventType == KEY_EVENT)
    {
        WCHAR UnicodeChar = InputEvent->Event.KeyEvent.uChar.UnicodeChar;
        InputEvent->Event.KeyEvent.uChar.UnicodeChar = 0;
        ConsoleInputUnicodeCharToAnsiChar(Console,
                                          &InputEvent->Event.KeyEvent.uChar.AsciiChar,
                                          &UnicodeChar);
    }
}

static VOID
ConioInputEventToUnicode(PCONSOLE Console, PINPUT_RECORD InputEvent)
{
    if (InputEvent->EventType == KEY_EVENT)
    {
        CHAR AsciiChar = InputEvent->Event.KeyEvent.uChar.AsciiChar;
        InputEvent->Event.KeyEvent.uChar.AsciiChar = 0;
        ConsoleInputAnsiCharToUnicodeChar(Console,
                                          &InputEvent->Event.KeyEvent.uChar.UnicodeChar,
                                          &AsciiChar);
    }
}

#endif

/********** HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK ************/








/* CONSRV TERMINAL FRONTENDS INTERFACE ****************************************/

/***************/
#ifdef TUITERM_COMPILE
NTSTATUS NTAPI
TuiLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
                IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                IN HANDLE ConsoleLeaderProcessHandle);
NTSTATUS NTAPI
TuiUnloadFrontEnd(IN OUT PFRONTEND FrontEnd);
#endif

NTSTATUS NTAPI
GuiLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
                IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                IN HANDLE ConsoleLeaderProcessHandle);
NTSTATUS NTAPI
GuiUnloadFrontEnd(IN OUT PFRONTEND FrontEnd);
/***************/

typedef
NTSTATUS (NTAPI *FRONTEND_LOAD)(IN OUT PFRONTEND FrontEnd,
                                IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
                                IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                                IN HANDLE ConsoleLeaderProcessHandle);

typedef
NTSTATUS (NTAPI *FRONTEND_UNLOAD)(IN OUT PFRONTEND FrontEnd);

/*
 * If we are not in GUI-mode, start the text-mode terminal emulator.
 * If we fail, try to start the GUI-mode terminal emulator.
 *
 * Try to open the GUI-mode terminal emulator. Two cases are possible:
 * - We are in GUI-mode, therefore GuiMode == TRUE, the previous test-case
 *   failed and we start GUI-mode terminal emulator.
 * - We are in text-mode, therefore GuiMode == FALSE, the previous test-case
 *   succeeded BUT we failed at starting text-mode terminal emulator.
 *   Then GuiMode was switched to TRUE in order to try to open the GUI-mode
 *   terminal emulator (Win32k will automatically switch to graphical mode,
 *   therefore no additional code is needed).
 */

/*
 * NOTE: Each entry of the table should be retrieved when loading a front-end
 *       (examples of the CSR servers which register some data for CSRSS).
 */
static struct
{
    CHAR            FrontEndName[80];
    FRONTEND_LOAD   FrontEndLoad;
    FRONTEND_UNLOAD FrontEndUnload;
} FrontEndLoadingMethods[] =
{
#ifdef TUITERM_COMPILE
    {"TUI", TuiLoadFrontEnd,    TuiUnloadFrontEnd},
#endif
    {"GUI", GuiLoadFrontEnd,    GuiUnloadFrontEnd},

//  {"Not found", 0, NULL}
};

static NTSTATUS
ConSrvLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                   IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
                   IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                   IN HANDLE ConsoleLeaderProcessHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    /*
     * Choose an adequate terminal front-end to load, and load it
     */
    for (i = 0; i < ARRAYSIZE(FrontEndLoadingMethods); ++i)
    {
        DPRINT("CONSRV: Trying to load %s frontend...\n",
               FrontEndLoadingMethods[i].FrontEndName);
        Status = FrontEndLoadingMethods[i].FrontEndLoad(FrontEnd,
                                                        ConsoleInfo,
                                                        ConsoleInitInfo,
                                                        ConsoleLeaderProcessHandle);
        if (NT_SUCCESS(Status))
        {
            /* Save the unload callback */
            FrontEnd->UnloadFrontEnd = FrontEndLoadingMethods[i].FrontEndUnload;

            DPRINT("CONSRV: %s frontend loaded successfully\n",
                   FrontEndLoadingMethods[i].FrontEndName);
            break;
        }
        else
        {
            DPRINT1("CONSRV: Loading %s frontend failed, Status = 0x%08lx , continuing...\n",
                    FrontEndLoadingMethods[i].FrontEndName, Status);
        }
    }

    return Status;
}

static NTSTATUS
ConSrvUnloadFrontEnd(IN PFRONTEND FrontEnd)
{
    if (FrontEnd == NULL) return STATUS_INVALID_PARAMETER;
    // return FrontEnd->Vtbl->UnloadFrontEnd(FrontEnd);
    return FrontEnd->UnloadFrontEnd(FrontEnd);
}

// See after...
static TERMINAL_VTBL ConSrvTermVtbl;

NTSTATUS NTAPI
ConSrvInitTerminal(IN OUT PTERMINAL Terminal,
                   IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
                   IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                   IN HANDLE ConsoleLeaderProcessHandle)
{
    NTSTATUS Status;
    PFRONTEND FrontEnd;

    /* Load a suitable frontend for the ConSrv terminal */
    FrontEnd = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(*FrontEnd));
    if (!FrontEnd) return STATUS_NO_MEMORY;

    Status = ConSrvLoadFrontEnd(FrontEnd,
                                ConsoleInfo,
                                ConsoleInitInfo,
                                ConsoleLeaderProcessHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CONSRV: Failed to initialize a frontend, Status = 0x%08lx\n", Status);
        ConsoleFreeHeap(FrontEnd);
        return Status;
    }
    DPRINT("CONSRV: Frontend initialized\n");

    /* Initialize the ConSrv terminal */
    Terminal->Vtbl = &ConSrvTermVtbl;
    // Terminal->Console will be initialized by ConDrvAttachTerminal
    Terminal->Context = FrontEnd; /* We store the frontend pointer in the terminal private context */

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConSrvDeinitTerminal(IN OUT PTERMINAL Terminal)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFRONTEND FrontEnd = Terminal->Context;

    /* Reset the ConSrv terminal */
    Terminal->Context = NULL;
    Terminal->Vtbl = NULL;

    /* Unload the frontend */
    if (FrontEnd != NULL)
    {
        Status = ConSrvUnloadFrontEnd(FrontEnd);
        ConsoleFreeHeap(FrontEnd);
    }

    return Status;
}


/* CONSRV TERMINAL INTERFACE **************************************************/

static NTSTATUS NTAPI
ConSrvTermInitTerminal(IN OUT PTERMINAL This,
                       IN PCONSOLE Console)
{
    NTSTATUS Status;
    PFRONTEND FrontEnd = This->Context;
    PCONSRV_CONSOLE ConSrvConsole = (PCONSRV_CONSOLE)Console;

    /* Initialize the console pointer for our frontend */
    FrontEnd->Console = ConSrvConsole;

    /** HACK HACK!! Copy FrontEnd into the console!! **/
    DPRINT("Using FrontEndIFace HACK(1), should be removed after proper implementation!\n");
    ConSrvConsole->FrontEndIFace = *FrontEnd;

    Status = FrontEnd->Vtbl->InitFrontEnd(FrontEnd, ConSrvConsole);
    if (!NT_SUCCESS(Status))
        DPRINT1("InitFrontEnd failed, Status = 0x%08lx\n", Status);

    /** HACK HACK!! Be sure FrontEndIFace is correctly updated in the console!! **/
    DPRINT("Using FrontEndIFace HACK(2), should be removed after proper implementation!\n");
    ConSrvConsole->FrontEndIFace = *FrontEnd;

    return Status;
}

static VOID NTAPI
ConSrvTermDeinitTerminal(IN OUT PTERMINAL This)
{
    PFRONTEND FrontEnd = This->Context;
    FrontEnd->Vtbl->DeinitFrontEnd(FrontEnd);
}



/************ Line discipline ***************/

static NTSTATUS NTAPI
ConSrvTermReadStream(IN OUT PTERMINAL This,
                     IN BOOLEAN Unicode,
                     /**PWCHAR Buffer,**/
                     OUT PVOID Buffer,
                     IN OUT PCONSOLE_READCONSOLE_CONTROL ReadControl,
                     IN PVOID Parameter OPTIONAL,
                     IN ULONG NumCharsToRead,
                     OUT PULONG NumCharsRead OPTIONAL)
{
    PFRONTEND FrontEnd = This->Context;
    PCONSRV_CONSOLE Console = FrontEnd->Console;
    PCONSOLE_INPUT_BUFFER InputBuffer = &Console->InputBuffer;
    PUNICODE_STRING ExeName = Parameter;

    // STATUS_PENDING : Wait if more to read ; STATUS_SUCCESS : Don't wait.
    NTSTATUS Status = STATUS_PENDING;

    PLIST_ENTRY CurrentEntry;
    ConsoleInput *Input;
    ULONG i = 0;

    /* Validity checks */
    // ASSERT(Console == InputBuffer->Header.Console);
    ASSERT((Buffer != NULL) || (Buffer == NULL && NumCharsToRead == 0));

    /* We haven't read anything (yet) */

    if (InputBuffer->Mode & ENABLE_LINE_INPUT)
    {
        /* COOKED mode, call the line discipline */

        if (Console->LineBuffer == NULL)
        {
            /* Start a new line */
            Console->LineMaxSize = max(256, NumCharsToRead);

            /*
             * Fixup ReadControl->nInitialChars in case the number of initial
             * characters is bigger than the number of characters to be read.
             * It will always be, lesser than or equal to Console->LineMaxSize.
             */
            ReadControl->nInitialChars = min(ReadControl->nInitialChars, NumCharsToRead);

            Console->LineBuffer = ConsoleAllocHeap(0, Console->LineMaxSize * sizeof(WCHAR));
            if (Console->LineBuffer == NULL) return STATUS_NO_MEMORY;

            Console->LinePos = Console->LineSize = ReadControl->nInitialChars;
            Console->LineComplete = Console->LineUpPressed = FALSE;
            Console->LineInsertToggle = Console->InsertMode;
            Console->LineWakeupMask = ReadControl->dwCtrlWakeupMask;

            /*
             * Pre-fill the buffer with the nInitialChars from the user buffer.
             * Since pre-filling is only allowed in Unicode, we don't need to
             * worry about ANSI <-> Unicode conversion.
             */
            memcpy(Console->LineBuffer, Buffer, Console->LineSize * sizeof(WCHAR));
            if (Console->LineSize >= Console->LineMaxSize)
            {
                Console->LineComplete = TRUE;
                Console->LinePos = 0;
            }
        }

        /* If we don't have a complete line yet, process the pending input */
        while (!Console->LineComplete && !IsListEmpty(&InputBuffer->InputEvents))
        {
            /* Remove an input event from the queue */
            _InterlockedDecrement((PLONG)&InputBuffer->NumberOfEvents);
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                NtClearEvent(InputBuffer->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* Only pay attention to key down */
            if (Input->InputEvent.EventType == KEY_EVENT &&
                Input->InputEvent.Event.KeyEvent.bKeyDown)
            {
                LineInputKeyDown(Console, ExeName,
                                 &Input->InputEvent.Event.KeyEvent);
                ReadControl->dwControlKeyState = Input->InputEvent.Event.KeyEvent.dwControlKeyState;
            }
            ConsoleFreeHeap(Input);
        }

        /* Check if we have a complete line to read from */
        if (Console->LineComplete)
        {
            /*
             * Console->LinePos keeps the next position of the character to read
             * in the line buffer across the different calls of the function,
             * so that the line buffer can be read by chunks after all the input
             * has been buffered.
             */

            while (i < NumCharsToRead && Console->LinePos < Console->LineSize)
            {
                WCHAR Char = Console->LineBuffer[Console->LinePos++];

                if (Unicode)
                {
                    ((PWCHAR)Buffer)[i] = Char;
                }
                else
                {
                    ConsoleInputUnicodeCharToAnsiChar(Console, &((PCHAR)Buffer)[i], &Char);
                }
                ++i;
            }

            if (Console->LinePos >= Console->LineSize)
            {
                /* The entire line has been read */
                ConsoleFreeHeap(Console->LineBuffer);
                Console->LineBuffer = NULL;
                Console->LinePos = Console->LineMaxSize = Console->LineSize = 0;
                // Console->LineComplete = Console->LineUpPressed = FALSE;
                Console->LineComplete = FALSE;
            }

            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        /* RAW mode */

        /* Character input */
        while (i < NumCharsToRead && !IsListEmpty(&InputBuffer->InputEvents))
        {
            /* Remove an input event from the queue */
            _InterlockedDecrement((PLONG)&InputBuffer->NumberOfEvents);
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                NtClearEvent(InputBuffer->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* Only pay attention to valid characters, on key down */
            if (Input->InputEvent.EventType == KEY_EVENT  &&
                Input->InputEvent.Event.KeyEvent.bKeyDown &&
                Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar != L'\0')
            {
                WCHAR Char = Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar;

                if (Unicode)
                {
                    ((PWCHAR)Buffer)[i] = Char;
                }
                else
                {
                    ConsoleInputUnicodeCharToAnsiChar(Console, &((PCHAR)Buffer)[i], &Char);
                }
                ++i;

                /* Did read something */
                Status = STATUS_SUCCESS;
            }
            ConsoleFreeHeap(Input);
        }
    }

    // FIXME: Only set if Status == STATUS_SUCCESS ???
    if (NumCharsRead) *NumCharsRead = i;

    return Status;
}




/* GLOBALS ********************************************************************/

#define TAB_WIDTH   8

// See condrv/text.c
/*static*/ VOID
ClearLineBuffer(PTEXTMODE_SCREEN_BUFFER Buff);

static VOID
ConioNextLine(PCONSRV_CONSOLE Console,
              PTEXTMODE_SCREEN_BUFFER Buff,
              PSMALL_RECT UpdateRect,
              PUINT ScrolledLines)
{
    Buff->VtState.PrivateModes &= ~VT_PRIVMODE_DELAYED_EOL_WRAP;

    if (ConioIsVtActive(Buff))
    {
        ConDrvVtAdvanceLine((PCONSOLE)Console, Buff);
        UpdateRect->Left = 0;
        UpdateRect->Right = Buff->ScreenBufferSize.X - 1;
        UpdateRect->Top = min(UpdateRect->Top, Buff->CursorPosition.Y);
        UpdateRect->Bottom = max(UpdateRect->Bottom, Buff->CursorPosition.Y);
        return;
    }

    /* If we hit bottom, slide the viewable screen */
    if (++Buff->CursorPosition.Y == Buff->ScreenBufferSize.Y)
    {
        Buff->CursorPosition.Y--;
        if (++Buff->VirtualY == Buff->ScreenBufferSize.Y)
        {
            Buff->VirtualY = 0;
        }
        (*ScrolledLines)++;
        ClearLineBuffer(Buff);
        if (UpdateRect->Top != 0)
        {
            UpdateRect->Top--;
        }
    }
    UpdateRect->Left = 0;
    UpdateRect->Right = Buff->ScreenBufferSize.X - 1;
    UpdateRect->Bottom = Buff->CursorPosition.Y;
}

/*
 * The cursor has run past the last column: decide what that means.
 *
 * With DISABLE_NEWLINE_AUTO_RETURN the wrap is deferred - the cursor stays on
 * the last column and the line only advances when the next printable character
 * arrives (VT "delayed EOL wrap"). Without wrapping at all, the cursor snaps
 * back to where this write started.
 */
static VOID
ConioWrapAtEol(PCONSRV_CONSOLE Console,
               PTEXTMODE_SCREEN_BUFFER Buff,
               PSMALL_RECT UpdateRect,
               PUINT ScrolledLines,
               PSHORT CursorStartX)
{
    if (!(Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT))
    {
        /* The cursor wraps back to its starting position on the same line */
        Buff->CursorPosition.X = *CursorStartX;
        return;
    }

    if (Buff->Mode & DISABLE_NEWLINE_AUTO_RETURN)
    {
        Buff->CursorPosition.X = Buff->ScreenBufferSize.X - 1;
        Buff->VtState.PrivateModes |= VT_PRIVMODE_DELAYED_EOL_WRAP;
        return;
    }

    /* Wrapping mode: Go to next line */
    Buff->CursorPosition.X = 0;
    *CursorStartX = Buff->CursorPosition.X;
    ConioNextLine(Console, Buff, UpdateRect, ScrolledLines);
}

static NTSTATUS
ConioWriteConsole(PFRONTEND FrontEnd,
                  PTEXTMODE_SCREEN_BUFFER Buff,
                  PWCHAR Buffer,
                  DWORD Length,
                  BOOL Attrib)
{
    PCONSRV_CONSOLE Console = FrontEnd->Console;

    UINT i;
    PCHAR_INFO Ptr;
    SMALL_RECT UpdateRect;
    SHORT CursorStartX, CursorStartY;
    UINT ScrolledLines;
    int CellWidth;
    BOOLEAN bFullwidth;
    BOOLEAN bCJK = Console->IsCJK;
    BOOLEAN bVtActive = ConioIsVtActive(Buff);
    COLORREF FgColorValue;
    COLORREF BgColorValue;

    /* If nothing to write, bail out now */
    if (Length == 0)
        return STATUS_SUCCESS;

    /*
     * The current VT colour cannot change while we consume this buffer: the VT
     * parser applies SGR between calls to us, never during one. Resolve it once.
     */
    FgColorValue = (bVtActive && Buff->VtState.UseRgbForeground) ? Buff->VtState.CurrentFgColor : CLR_INVALID;
    BgColorValue = (bVtActive && Buff->VtState.UseRgbBackground) ? Buff->VtState.CurrentBgColor : CLR_INVALID;

    /* Extended colours are only ever stored while VT is driving the buffer */
    if (FgColorValue != CLR_INVALID || BgColorValue != CLR_INVALID)
        ConioEnsureCellColors(Buff);

    CursorStartX = Buff->CursorPosition.X;
    CursorStartY = Buff->CursorPosition.Y;
    UpdateRect.Left = Buff->ScreenBufferSize.X;
    UpdateRect.Top  = Buff->CursorPosition.Y;
    UpdateRect.Right  = -1;
    UpdateRect.Bottom = Buff->CursorPosition.Y;
    ScrolledLines = 0;

    for (i = 0; i < Length; i++)
    {
        if (Buff->VtState.PrivateModes & VT_PRIVMODE_DELAYED_EOL_WRAP)
        {
            if (!(Buff->Mode & ENABLE_PROCESSED_OUTPUT) ||
                (Buffer[i] != L'\r' && Buffer[i] != L'\n' &&
                 Buffer[i] != L'\b' && Buffer[i] != L'\a'))
            {
                Buff->CursorPosition.X = 0;
                CursorStartX = Buff->CursorPosition.X;
                ConioNextLine(Console, Buff, &UpdateRect, &ScrolledLines);
            }
            else if (Buffer[i] != L'\a')
            {
                Buff->VtState.PrivateModes &= ~VT_PRIVMODE_DELAYED_EOL_WRAP;
            }
        }

        /*
         * If we are in processed mode, interpret special characters and
         * display them correctly. Otherwise, just put them into the buffer.
         */
        if (Buff->Mode & ENABLE_PROCESSED_OUTPUT)
        {
            /* --- CR --- */
            if (Buffer[i] == L'\r')
            {
                Buff->CursorPosition.X = 0;
                CursorStartX = Buff->CursorPosition.X;
                UpdateRect.Left  = min(UpdateRect.Left , Buff->CursorPosition.X);
                UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
                continue;
            }
            /* --- LF --- */
            else if (Buffer[i] == L'\n')
            {
                Buff->VtState.PrivateModes &= ~VT_PRIVMODE_DELAYED_EOL_WRAP;
                if (Buff->Mode & DISABLE_NEWLINE_AUTO_RETURN)
                {
                    CursorStartX = Buff->CursorPosition.X;
                }
                else
                {
                    Buff->CursorPosition.X = 0;
                    CursorStartX = Buff->CursorPosition.X;
                }
                ConioNextLine(Console, Buff, &UpdateRect, &ScrolledLines);
                continue;
            }
            /* --- BS --- */
            else if (Buffer[i] == L'\b')
            {
                INT OldX = Buff->CursorPosition.X;

                /* Only handle BS if we are not on the first position of the first line */
                if (Buff->CursorPosition.X == 0 && Buff->CursorPosition.Y == 0)
                    continue;

                if (Buff->CursorPosition.X == 0)
                {
                    /* Slide virtual position up */
                    Buff->CursorPosition.X = Buff->ScreenBufferSize.X - 1;
                    Buff->CursorPosition.Y--;
                    // TODO? : Update CursorStartY = Buff->CursorPosition.Y;
                    UpdateRect.Top = min(UpdateRect.Top, Buff->CursorPosition.Y);
                }
                else
                {
                    Buff->CursorPosition.X--;
                }
                Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);

                if (Ptr->Attributes & COMMON_LVB_LEADING_BYTE)
                {
                    /*
                     * The cursor just moved on the leading byte of the same
                     * current character. We should go one position before to
                     * go to the actual previous character to erase.
                     */

                    /* Only handle BS if we are not on the first position of the first line */
                    if (Buff->CursorPosition.X == 0 && Buff->CursorPosition.Y == 0)
                        continue;

                    if (Buff->CursorPosition.X == 0)
                    {
                        /* Slide virtual position up */
                        Buff->CursorPosition.X = Buff->ScreenBufferSize.X - 1;
                        Buff->CursorPosition.Y--;
                        // TODO? : Update CursorStartY = Buff->CursorPosition.Y;
                        UpdateRect.Top = min(UpdateRect.Top, Buff->CursorPosition.Y);
                    }
                    else
                    {
                        Buff->CursorPosition.X--;
                    }
                    Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                }

                if (Ptr->Attributes & COMMON_LVB_TRAILING_BYTE)
                {
                    /* The cursor is on the trailing byte of a full-width character */

                    /* Delete its trailing byte... */
                    Ptr->Char.UnicodeChar = L' ';
                    if (Attrib)
                        Ptr->Attributes = Buff->ScreenDefaultAttrib;
                    Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
                    ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, CLR_INVALID, CLR_INVALID);

                    if (Buff->CursorPosition.X > 0)
                        Buff->CursorPosition.X--;
                    /* ... and now its leading byte */
                    Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                }

                Ptr->Char.UnicodeChar = L' ';
                if (Attrib)
                    Ptr->Attributes = Buff->ScreenDefaultAttrib;
                Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
                ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, CLR_INVALID, CLR_INVALID);

                UpdateRect.Left  = min(min(UpdateRect.Left , Buff->CursorPosition.X), OldX);
                UpdateRect.Right = max(max(UpdateRect.Right, Buff->CursorPosition.X), OldX);
                continue;
            }
            /* --- TAB --- */
            else if (Buffer[i] == L'\t')
            {
                UINT EndX;

                Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);

                if (Ptr->Attributes & COMMON_LVB_TRAILING_BYTE)
                {
                    /*
                     * The cursor is on the trailing byte of a full-width character.
                     * Go back one position to be on its leading byte.
                     */
                    if (Buff->CursorPosition.X > 0)
                        Buff->CursorPosition.X--;
                    Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                }

                UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);

                EndX = (Buff->CursorPosition.X + TAB_WIDTH) & ~(TAB_WIDTH - 1);
                EndX = min(EndX, (UINT)Buff->ScreenBufferSize.X);

                while ((UINT)Buff->CursorPosition.X < EndX)
                {
                    Ptr->Char.UnicodeChar = L' ';
                    if (Attrib)
                        Ptr->Attributes = Buff->ScreenDefaultAttrib;
                    Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
                    ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, CLR_INVALID, CLR_INVALID);

                    ++Ptr;
                    Buff->CursorPosition.X++;
                }
                if (Buff->CursorPosition.X < Buff->ScreenBufferSize.X)
                {
                    /* If the following cell is the trailing byte of a full-width character, reset it */
                    if (Ptr->Attributes & COMMON_LVB_TRAILING_BYTE)
                    {
                        Ptr->Char.UnicodeChar = L' ';
                        if (Attrib)
                            Ptr->Attributes = Buff->ScreenDefaultAttrib;
                        Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
                        ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, CLR_INVALID, CLR_INVALID);
                    }
                }
                UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);

                if (Buff->CursorPosition.X >= Buff->ScreenBufferSize.X)
                {
                    ConioWrapAtEol(Console, Buff, &UpdateRect, &ScrolledLines, &CursorStartX);
                }
                continue;
            }
            /* --- BEL ---*/
            else if (Buffer[i] == L'\a')
            {
                FrontEnd->Vtbl->RingBell(FrontEnd);
                continue;
            }
        }
        /*
         * Determine the display width. Everything below U+0080 occupies exactly
         * one cell, so skip the (non-inlined, bisecting) helper for the case
         * that dominates console output - this is what IS_FULL_WIDTH does too.
         */
        if ((USHORT)Buffer[i] < 0x0080)
        {
            CellWidth = 1;
        }
        else
        {
            CellWidth = mk_wcwidth_cjk(Buffer[i]);
            if (CellWidth < 0)
                CellWidth = 1;

            /*
             * Zero-width (combining) characters have no cell of their own.
             * Dropping them is a terminal-emulation decision, so only do it
             * while VT is driving this buffer; a plain WriteConsole keeps its
             * old contract of storing whatever the client wrote.
             */
            if (CellWidth == 0)
            {
                if (bVtActive) continue;
                CellWidth = 1;
            }
        }

        UpdateRect.Left  = min(UpdateRect.Left , Buff->CursorPosition.X);
        UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);

        bFullwidth = (bCJK && CellWidth == 2);

        /* Check whether we can insert the full-width character */
        if (bFullwidth)
        {
            /* It spans two cells and should all fit on the current line */
            if (Buff->CursorPosition.X >= Buff->ScreenBufferSize.X - 1)
            {
                /*
                 * Deliberately not ConioWrapAtEol(): a delayed wrap would leave
                 * the cursor on the last column, where a two-cell character
                 * still does not fit, and the check below would drop it. A
                 * full-width character at the right edge must wrap immediately.
                 */
                if (Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT)
                {
                    /* Wrapping mode: Go to next line */
                    Buff->CursorPosition.X = 0;
                    CursorStartX = Buff->CursorPosition.X;
                    ConioNextLine(Console, Buff, &UpdateRect, &ScrolledLines);
                }
                else
                {
                    /* The cursor wraps back to its starting position on the same line */
                    Buff->CursorPosition.X = CursorStartX;
                }
            }

            /*
             * Now be sure we can fit the full-width character.
             * If the screenbuffer is one cell wide we cannot display
             * the full-width character, so just skip it.
             */
            if (Buff->CursorPosition.X >= Buff->ScreenBufferSize.X - 1)
            {
                DPRINT1("Cannot display full-width character! CursorPosition.X = %d, ScreenBufferSize.X = %d\n",
                        Buff->CursorPosition.X, Buff->ScreenBufferSize.X);
                continue;
            }
        }

        Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);

        /*
         * Check whether we are overwriting part of a full-width character,
         * in which case we need to invalidate it.
         */
        if (Ptr->Attributes & COMMON_LVB_TRAILING_BYTE)
        {
            /*
             * The cursor is on the trailing byte of a full-width character.
             * Go back one position to kill the previous leading byte.
             */
            if (Buff->CursorPosition.X > 0)
            {
                Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X - 1, Buff->CursorPosition.Y);
                Ptr->Char.UnicodeChar = L' ';
                if (Attrib)
                    Ptr->Attributes = Buff->ScreenDefaultAttrib;
                Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
                ConioSetCellColors(Buff, Buff->CursorPosition.X - 1, Buff->CursorPosition.Y, CLR_INVALID, CLR_INVALID);
            }
            Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
        }

        /* Insert the character */
        if (bFullwidth)
        {
            ASSERT(Buff->CursorPosition.X < Buff->ScreenBufferSize.X - 1);

            /* Set the leading byte */
            Ptr->Char.UnicodeChar = Buffer[i];
            if (Attrib)
                Ptr->Attributes = Buff->ScreenDefaultAttrib;
            Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
            Ptr->Attributes |= COMMON_LVB_LEADING_BYTE;
            ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, FgColorValue, BgColorValue);

            /* Set the trailing byte */
            Buff->CursorPosition.X++;
            Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
            // Ptr->Char.UnicodeChar = Buffer[i]; // L' ';
            if (Attrib)
                Ptr->Attributes = Buff->ScreenDefaultAttrib;
            Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
            Ptr->Attributes |= COMMON_LVB_TRAILING_BYTE;
            ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, FgColorValue, BgColorValue);

            UpdateRect.Right++;
        }
        else
        {
            Ptr->Char.UnicodeChar = Buffer[i];
            if (Attrib)
                Ptr->Attributes = Buff->ScreenDefaultAttrib;
            Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
            ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, FgColorValue, BgColorValue);
        }

        ++Ptr;
        Buff->CursorPosition.X++;

        if (Buff->CursorPosition.X < Buff->ScreenBufferSize.X)
        {
            /* If the following cell is the trailing byte of a full-width character, reset it */
            if (Ptr->Attributes & COMMON_LVB_TRAILING_BYTE)
            {
                Ptr->Char.UnicodeChar = L' ';
                if (Attrib)
                    Ptr->Attributes = Buff->ScreenDefaultAttrib;
                Ptr->Attributes &= ~COMMON_LVB_SBCSDBCS;
                ConioSetCellColors(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y, CLR_INVALID, CLR_INVALID);
            }
        }

        if (Buff->CursorPosition.X >= Buff->ScreenBufferSize.X)
        {
            ConioWrapAtEol(Console, Buff, &UpdateRect, &ScrolledLines, &CursorStartX);
        }
    }

    if (!ConioIsRectEmpty(&UpdateRect) && (PCONSOLE_SCREEN_BUFFER)Buff == Console->ActiveBuffer)
    {
        // TermWriteStream(Console, &UpdateRect, CursorStartX, CursorStartY,
                        // ScrolledLines, Buffer, Length);
        FrontEnd->Vtbl->WriteStream(FrontEnd,
                                    &UpdateRect,
                                    CursorStartX,
                                    CursorStartY,
                                    ScrolledLines,
                                    Buffer,
                                    Length);
    }

    return STATUS_SUCCESS;
}



static NTSTATUS NTAPI
ConSrvTermWriteStream(IN OUT PTERMINAL This,
                      PTEXTMODE_SCREEN_BUFFER Buff,
                      PWCHAR Buffer,
                      DWORD Length,
                      BOOL Attrib)
{
    PFRONTEND FrontEnd = This->Context;
    return ConioWriteConsole(FrontEnd,
                             Buff,
                             Buffer,
                             Length,
                             Attrib);
}

/************ Line discipline ***************/



VOID
ConioDrawConsole(PCONSRV_CONSOLE Console)
{
    SMALL_RECT Region;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = Console->ActiveBuffer;

    if (!ActiveBuffer) return;

    ConioInitRect(&Region, 0, 0,
                  ActiveBuffer->ViewSize.Y - 1,
                  ActiveBuffer->ViewSize.X - 1);
    TermDrawRegion(Console, &Region);
    // Console->FrontEndIFace.Vtbl->DrawRegion(&Console->FrontEndIFace, &Region);
}

static VOID NTAPI
ConSrvTermDrawRegion(IN OUT PTERMINAL This,
                SMALL_RECT* Region)
{
    PFRONTEND FrontEnd = This->Context;
    FrontEnd->Vtbl->DrawRegion(FrontEnd, Region);
}

static BOOL NTAPI
ConSrvTermSetCursorInfo(IN OUT PTERMINAL This,
                   PCONSOLE_SCREEN_BUFFER ScreenBuffer)
{
    PFRONTEND FrontEnd = This->Context;
    return FrontEnd->Vtbl->SetCursorInfo(FrontEnd, ScreenBuffer);
}

static BOOL NTAPI
ConSrvTermSetScreenInfo(IN OUT PTERMINAL This,
                   PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                   SHORT OldCursorX,
                   SHORT OldCursorY)
{
    PFRONTEND FrontEnd = This->Context;
    return FrontEnd->Vtbl->SetScreenInfo(FrontEnd,
                                         ScreenBuffer,
                                         OldCursorX,
                                         OldCursorY);
}

static VOID NTAPI
ConSrvTermResizeTerminal(IN OUT PTERMINAL This)
{
    PFRONTEND FrontEnd = This->Context;
    FrontEnd->Vtbl->ResizeTerminal(FrontEnd);
}

static VOID NTAPI
ConSrvTermSetActiveScreenBuffer(IN OUT PTERMINAL This)
{
    PFRONTEND FrontEnd = This->Context;
    FrontEnd->Vtbl->SetActiveScreenBuffer(FrontEnd);
}

static VOID NTAPI
ConSrvTermReleaseScreenBuffer(IN OUT PTERMINAL This,
                         IN PCONSOLE_SCREEN_BUFFER ScreenBuffer)
{
    PFRONTEND FrontEnd = This->Context;
    FrontEnd->Vtbl->ReleaseScreenBuffer(FrontEnd, ScreenBuffer);
}

static VOID NTAPI
ConSrvTermGetLargestConsoleWindowSize(IN OUT PTERMINAL This,
                                 PCOORD pSize)
{
    PFRONTEND FrontEnd = This->Context;
    FrontEnd->Vtbl->GetLargestConsoleWindowSize(FrontEnd, pSize);
}

static BOOL NTAPI
ConSrvTermSetPalette(IN OUT PTERMINAL This,
                HPALETTE PaletteHandle,
                UINT PaletteUsage)
{
    PFRONTEND FrontEnd = This->Context;
    return FrontEnd->Vtbl->SetPalette(FrontEnd, PaletteHandle, PaletteUsage);
}

static BOOL NTAPI
ConSrvTermSetCodePage(IN OUT PTERMINAL This,
                      UINT CodePage)
{
    PFRONTEND FrontEnd = This->Context;
    return FrontEnd->Vtbl->SetCodePage(FrontEnd, CodePage);
}

static INT NTAPI
ConSrvTermShowMouseCursor(IN OUT PTERMINAL This,
                     BOOL Show)
{
    PFRONTEND FrontEnd = This->Context;
    return FrontEnd->Vtbl->ShowMouseCursor(FrontEnd, Show);
}

/*
 * Console-level state the driver needs but does not own.
 *
 * The VT engine lives in condrv/ and only has a PCONSOLE, yet DECSET/OSC
 * sequences legitimately change the window title, the palette and the
 * clipboard - all of which belong to CONSRV_CONSOLE or to a frontend. These
 * entries give it a PCONSOLE-typed way in, so condrv/ never casts to
 * PCONSRV_CONSOLE nor calls USER32 itself.
 */

static BOOL NTAPI
ConSrvTermSetTitle(IN OUT PTERMINAL This,
                   IN PCWSTR Title,
                   IN ULONG Length)
{
    PFRONTEND FrontEnd = This->Context;
    PCONSRV_CONSOLE Console = FrontEnd->Console;
    PWCHAR Buffer;

    if (!Title) return FALSE;

    /* Allocate first, so a failure leaves the existing title untouched */
    Buffer = ConsoleAllocHeap(HEAP_ZERO_MEMORY, Length + sizeof(WCHAR));
    if (!Buffer) return FALSE;

    RtlCopyMemory(Buffer, Title, Length);
    Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Same ownership rules as SrvSetConsoleTitle */
    ConsoleFreeUnicodeString(&Console->Title);
    Console->Title.Buffer = Buffer;
    Console->Title.Length = Length;
    Console->Title.MaximumLength = Length + sizeof(WCHAR);

    TermChangeTitle(Console);
    return TRUE;
}

static BOOL NTAPI
ConSrvTermGetColorTable(IN OUT PTERMINAL This,
                        OUT COLORREF* Colors,
                        IN ULONG Count)
{
    PFRONTEND FrontEnd = This->Context;
    PCONSRV_CONSOLE Console = FrontEnd->Console;

    if (!Colors || Count == 0 || Count > ARRAYSIZE(Console->Colors))
        return FALSE;

    RtlCopyMemory(Colors, Console->Colors, Count * sizeof(COLORREF));
    return TRUE;
}

static BOOL NTAPI
ConSrvTermSetColorTable(IN OUT PTERMINAL This,
                        IN const COLORREF* Colors,
                        IN ULONG Count)
{
    PFRONTEND FrontEnd = This->Context;
    PCONSRV_CONSOLE Console = FrontEnd->Console;

    if (!Colors || Count == 0 || Count > ARRAYSIZE(Console->Colors))
        return FALSE;

    RtlCopyMemory(Console->Colors, Colors, Count * sizeof(COLORREF));
    return TRUE;
}

static BOOL NTAPI
ConSrvTermGetClipboardText(IN OUT PTERMINAL This,
                           OUT PWCHAR* Text,
                           OUT PULONG Length)
{
    PFRONTEND FrontEnd = This->Context;
    PCONSRV_CONSOLE Console = FrontEnd->Console;
    HANDLE hData;
    PCWSTR ClipText;
    SIZE_T Chars;
    PWCHAR Copy = NULL;

    *Text = NULL;
    *Length = 0;

    /*
     * Use the console window as the clipboard owner: the CSR API thread has no
     * window of its own and no guaranteed window-station association.
     */
    if (!OpenClipboard(TermGetConsoleWindowHandle(Console)))
        return FALSE;

    hData = GetClipboardData(CF_UNICODETEXT);
    if (hData)
    {
        ClipText = GlobalLock(hData);
        if (ClipText)
        {
            Chars = wcslen(ClipText);
            Copy = ConsoleAllocHeap(0, (Chars + 1) * sizeof(WCHAR));
            if (Copy)
            {
                RtlCopyMemory(Copy, ClipText, Chars * sizeof(WCHAR));
                Copy[Chars] = UNICODE_NULL;
                *Text = Copy;
                *Length = (ULONG)Chars;
            }
            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
    return (Copy != NULL);
}

static BOOL NTAPI
ConSrvTermSetClipboardText(IN OUT PTERMINAL This,
                           IN PCWSTR Text,
                           IN ULONG Length)
{
    PFRONTEND FrontEnd = This->Context;
    PCONSRV_CONSOLE Console = FrontEnd->Console;
    HANDLE hData;
    PWCHAR Dest;
    BOOL Success = FALSE;

    if (!Text) return FALSE;

    if (!OpenClipboard(TermGetConsoleWindowHandle(Console)))
        return FALSE;

    hData = GlobalAlloc(GMEM_MOVEABLE, (Length + 1) * sizeof(WCHAR));
    if (hData)
    {
        Dest = GlobalLock(hData);
        if (Dest)
        {
            RtlCopyMemory(Dest, Text, Length * sizeof(WCHAR));
            Dest[Length] = UNICODE_NULL;
            GlobalUnlock(hData);

            EmptyClipboard();
            if (SetClipboardData(CF_UNICODETEXT, hData))
                Success = TRUE;
        }

        /* The clipboard owns hData once SetClipboardData succeeds */
        if (!Success) GlobalFree(hData);
    }

    CloseClipboard();
    return Success;
}

static TERMINAL_VTBL ConSrvTermVtbl =
{
    ConSrvTermInitTerminal,
    ConSrvTermDeinitTerminal,

    ConSrvTermReadStream,
    ConSrvTermWriteStream,

    ConSrvTermDrawRegion,
    ConSrvTermSetCursorInfo,
    ConSrvTermSetScreenInfo,
    ConSrvTermResizeTerminal,
    ConSrvTermSetActiveScreenBuffer,
    ConSrvTermReleaseScreenBuffer,
    ConSrvTermGetLargestConsoleWindowSize,
    ConSrvTermSetPalette,
    ConSrvTermSetCodePage,
    ConSrvTermShowMouseCursor,
    ConSrvTermSetTitle,
    ConSrvTermGetColorTable,
    ConSrvTermSetColorTable,
    ConSrvTermGetClipboardText,
    ConSrvTermSetClipboardText,
};

#if 0
VOID
ResetFrontEnd(IN PCONSOLE Console)
{
    PCONSRV_CONSOLE ConSrvConsole = (PCONSRV_CONSOLE)Console;
    if (!Console) return;

    /* Reinitialize the frontend interface */
    RtlZeroMemory(&ConSrvConsole->FrontEndIFace, sizeof(ConSrvConsole->FrontEndIFace));
    ConSrvConsole->FrontEndIFace.Vtbl = &ConSrvTermVtbl;
}
#endif

/* EOF */

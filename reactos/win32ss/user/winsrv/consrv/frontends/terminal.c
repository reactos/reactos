/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            frontends/terminal.c
 * PURPOSE:         ConSrv terminal.
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

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
    ASSERT((ULONG_PTR)dChar != (ULONG_PTR)sWChar); \
    WideCharToMultiByte((Console)->InputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleInputAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    ASSERT((ULONG_PTR)dWChar != (ULONG_PTR)sChar); \
    MultiByteToWideChar((Console)->InputCodePage, 0, (sChar), 1, (dWChar), 1)

typedef struct ConsoleInput_t
{
    LIST_ENTRY ListEntry;
    INPUT_RECORD InputEvent;
} ConsoleInput;


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

    /* Initialize the console pointer for our frontend */
    FrontEnd->Console = Console;

    /** HACK HACK!! Copy FrontEnd into the console!! **/
    DPRINT1("Using FrontEndIFace HACK(1), should be removed after proper implementation!\n");
    Console->FrontEndIFace = *FrontEnd;

    Status = FrontEnd->Vtbl->InitFrontEnd(FrontEnd, FrontEnd->Console);
    if (!NT_SUCCESS(Status))
        DPRINT1("InitFrontEnd failed, Status = 0x%08lx\n", Status);

    /** HACK HACK!! Be sure FrontEndIFace is correctly updated in the console!! **/
    DPRINT("Using FrontEndIFace HACK(2), should be removed after proper implementation!\n");
    Console->FrontEndIFace = *FrontEnd;

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
    ULONG i;

    /* Validity checks */
    // ASSERT(Console == InputBuffer->Header.Console);
    ASSERT((Buffer != NULL) || (Buffer == NULL && NumCharsToRead == 0));

    /* We haven't read anything (yet) */
    i = ReadControl->nInitialChars;

    if (InputBuffer->Mode & ENABLE_LINE_INPUT)
    {
        /* COOKED mode, call the line discipline */

        if (Console->LineBuffer == NULL)
        {
            /* Starting a new line */
            Console->LineMaxSize = max(256, NumCharsToRead);

            Console->LineBuffer = ConsoleAllocHeap(0, Console->LineMaxSize * sizeof(WCHAR));
            if (Console->LineBuffer == NULL) return STATUS_NO_MEMORY;

            Console->LinePos = Console->LineSize = ReadControl->nInitialChars;
            Console->LineComplete = Console->LineUpPressed = FALSE;
            Console->LineInsertToggle = Console->InsertMode;
            Console->LineWakeupMask = ReadControl->dwCtrlWakeupMask;

            /*
             * Pre-filling the buffer is only allowed in the Unicode API,
             * so we don't need to worry about ANSI <-> Unicode conversion.
             */
            memcpy(Console->LineBuffer, Buffer, Console->LineSize * sizeof(WCHAR));
            if (Console->LineSize == Console->LineMaxSize)
            {
                Console->LineComplete = TRUE;
                Console->LinePos = 0;
            }
        }

        /* If we don't have a complete line yet, process the pending input */
        while (!Console->LineComplete && !IsListEmpty(&InputBuffer->InputEvents))
        {
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                ResetEvent(InputBuffer->ActiveEvent);
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
            while (i < NumCharsToRead && Console->LinePos != Console->LineSize)
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

            if (Console->LinePos == Console->LineSize)
            {
                /* Entire line has been read */
                ConsoleFreeHeap(Console->LineBuffer);
                Console->LineBuffer = NULL;
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
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                ResetEvent(InputBuffer->ActiveEvent);
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
ConioNextLine(PTEXTMODE_SCREEN_BUFFER Buff, PSMALL_RECT UpdateRect, PUINT ScrolledLines)
{
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

    CursorStartX = Buff->CursorPosition.X;
    CursorStartY = Buff->CursorPosition.Y;
    UpdateRect.Left = Buff->ScreenBufferSize.X;
    UpdateRect.Top = Buff->CursorPosition.Y;
    UpdateRect.Right = -1;
    UpdateRect.Bottom = Buff->CursorPosition.Y;
    ScrolledLines = 0;

    for (i = 0; i < Length; i++)
    {
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
                UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
                UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
                continue;
            }
            /* --- LF --- */
            else if (Buffer[i] == L'\n')
            {
                Buff->CursorPosition.X = 0;
                ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
                continue;
            }
            /* --- BS --- */
            else if (Buffer[i] == L'\b')
            {
                /* Only handle BS if we're not on the first pos of the first line */
                if (0 != Buff->CursorPosition.X || 0 != Buff->CursorPosition.Y)
                {
                    if (0 == Buff->CursorPosition.X)
                    {
                        /* slide virtual position up */
                        Buff->CursorPosition.X = Buff->ScreenBufferSize.X - 1;
                        Buff->CursorPosition.Y--;
                        UpdateRect.Top = min(UpdateRect.Top, Buff->CursorPosition.Y);
                    }
                    else
                    {
                        Buff->CursorPosition.X--;
                    }
                    Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                    Ptr->Char.UnicodeChar = L' ';
                    Ptr->Attributes  = Buff->ScreenDefaultAttrib;
                    UpdateRect.Left  = min(UpdateRect.Left, Buff->CursorPosition.X);
                    UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
                }
                continue;
            }
            /* --- TAB --- */
            else if (Buffer[i] == L'\t')
            {
                UINT EndX;

                UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
                EndX = (Buff->CursorPosition.X + TAB_WIDTH) & ~(TAB_WIDTH - 1);
                EndX = min(EndX, (UINT)Buff->ScreenBufferSize.X);
                Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                while ((UINT)Buff->CursorPosition.X < EndX)
                {
                    Ptr->Char.UnicodeChar = L' ';
                    Ptr->Attributes = Buff->ScreenDefaultAttrib;
                    ++Ptr;
                    Buff->CursorPosition.X++;
                }
                UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X - 1);
                if (Buff->CursorPosition.X == Buff->ScreenBufferSize.X)
                {
                    if (Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT)
                    {
                        Buff->CursorPosition.X = 0;
                        ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
                    }
                    else
                    {
                        Buff->CursorPosition.X--;
                    }
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
        UpdateRect.Left  = min(UpdateRect.Left, Buff->CursorPosition.X);
        UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);

        Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
        Ptr->Char.UnicodeChar = Buffer[i];
        if (Attrib) Ptr->Attributes = Buff->ScreenDefaultAttrib;

        Buff->CursorPosition.X++;
        if (Buff->CursorPosition.X == Buff->ScreenBufferSize.X)
        {
            if (Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT)
            {
                Buff->CursorPosition.X = 0;
                ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
            }
            else
            {
                Buff->CursorPosition.X = CursorStartX;
            }
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

static INT NTAPI
ConSrvTermShowMouseCursor(IN OUT PTERMINAL This,
                     BOOL Show)
{
    PFRONTEND FrontEnd = This->Context;
    return FrontEnd->Vtbl->ShowMouseCursor(FrontEnd, Show);
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
    ConSrvTermShowMouseCursor,
};

#if 0
VOID
ResetFrontEnd(IN PCONSOLE Console)
{
    if (!Console) return;

    /* Reinitialize the frontend interface */
    RtlZeroMemory(&Console->FrontEndIFace, sizeof(Console->FrontEndIFace));
    Console->FrontEndIFace.Vtbl = &ConSrvTermVtbl;
}
#endif

/* EOF */

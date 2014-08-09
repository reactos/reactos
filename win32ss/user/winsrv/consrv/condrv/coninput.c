/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            win32ss/user/winsrv/consrv/condrv/coninput.c
 * PURPOSE:         Console Input functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#define NDEBUG
#include <debug.h>

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


NTSTATUS
ConDrvAddInputEvents(PCONSOLE Console,
                     PINPUT_RECORD InputRecords, // InputEvent
                     ULONG NumEventsToWrite,
                     PULONG NumEventsWritten,
                     BOOLEAN AppendToEnd)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i = 0;
    BOOLEAN SetWaitEvent = FALSE;

    if (NumEventsWritten) *NumEventsWritten = 0;

    /*
     * When adding many single events, in the case of repeated mouse move or
     * key down events, we try to coalesce them so that we do not saturate
     * too quickly the input buffer.
     */
    if (NumEventsToWrite == 1 && !IsListEmpty(&Console->InputBuffer.InputEvents))
    {
        PINPUT_RECORD InputRecord = InputRecords; // Only one element
        PINPUT_RECORD LastInputRecord;
        ConsoleInput* ConInRec; // Input

        /* Get the "next" event of the input buffer */
        if (AppendToEnd)
        {
            /* Get the tail element */
            ConInRec = CONTAINING_RECORD(Console->InputBuffer.InputEvents.Blink,
                                         ConsoleInput, ListEntry);
        }
        else
        {
            /* Get the head element */
            ConInRec = CONTAINING_RECORD(Console->InputBuffer.InputEvents.Flink,
                                         ConsoleInput, ListEntry);
        }
        LastInputRecord = &ConInRec->InputEvent;

        if (InputRecord->EventType == MOUSE_EVENT &&
            InputRecord->Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
        {
            if (LastInputRecord->EventType == MOUSE_EVENT &&
                LastInputRecord->Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
            {
                /* Update the mouse position */
                LastInputRecord->Event.MouseEvent.dwMousePosition.X =
                    InputRecord->Event.MouseEvent.dwMousePosition.X;
                LastInputRecord->Event.MouseEvent.dwMousePosition.Y =
                    InputRecord->Event.MouseEvent.dwMousePosition.Y;

                i = 1;
                // return STATUS_SUCCESS;
                Status = STATUS_SUCCESS;
            }
        }
        else if (InputRecord->EventType == KEY_EVENT &&
                 InputRecord->Event.KeyEvent.bKeyDown)
        {
            if (LastInputRecord->EventType == KEY_EVENT &&
                LastInputRecord->Event.KeyEvent.bKeyDown &&
                (LastInputRecord->Event.KeyEvent.wVirtualScanCode ==    // Same scancode
                     InputRecord->Event.KeyEvent.wVirtualScanCode) &&
                (LastInputRecord->Event.KeyEvent.uChar.UnicodeChar ==   // Same character
                     InputRecord->Event.KeyEvent.uChar.UnicodeChar) &&
                (LastInputRecord->Event.KeyEvent.dwControlKeyState ==   // Same Ctrl/Alt/Shift state
                     InputRecord->Event.KeyEvent.dwControlKeyState) )
            {
                /* Update the repeat count */
                LastInputRecord->Event.KeyEvent.wRepeatCount +=
                    InputRecord->Event.KeyEvent.wRepeatCount;

                i = 1;
                // return STATUS_SUCCESS;
                Status = STATUS_SUCCESS;
            }
        }
    }

    /* If we coalesced the only one element, we can quit */
    if (i == 1 && Status == STATUS_SUCCESS /* && NumEventsToWrite == 1 */)
        goto Done;

    /*
     * No event coalesced, add them in the usual way.
     */

    if (AppendToEnd)
    {
        /* Go to the beginning of the list */
        // InputRecords = InputRecords;
    }
    else
    {
        /* Go to the end of the list */
        InputRecords = &InputRecords[NumEventsToWrite - 1];
    }

    /* Set the event if the list is going to be non-empty */
    if (IsListEmpty(&Console->InputBuffer.InputEvents))
        SetWaitEvent = TRUE;

    for (i = 0; i < NumEventsToWrite && NT_SUCCESS(Status); ++i)
    {
        PINPUT_RECORD InputRecord;
        ConsoleInput* ConInRec;

        if (AppendToEnd)
        {
            /* Select the event and go to the next one */
            InputRecord = InputRecords++;
        }
        else
        {
            /* Select the event and go to the previous one */
            InputRecord = InputRecords--;
        }

        /* Add event to the queue */
        ConInRec = ConsoleAllocHeap(0, sizeof(ConsoleInput));
        if (ConInRec == NULL)
        {
            // return STATUS_INSUFFICIENT_RESOURCES;
            Status = STATUS_INSUFFICIENT_RESOURCES;
            continue;
        }

        ConInRec->InputEvent = *InputRecord;

        if (AppendToEnd)
        {
            /* Append the event to the end of the queue */
            InsertTailList(&Console->InputBuffer.InputEvents, &ConInRec->ListEntry);
        }
        else
        {
            /* Append the event to the beginning of the queue */
            InsertHeadList(&Console->InputBuffer.InputEvents, &ConInRec->ListEntry);
        }

        // return STATUS_SUCCESS;
        Status = STATUS_SUCCESS;
    }

    if (SetWaitEvent) SetEvent(Console->InputBuffer.ActiveEvent);

Done:
    if (NumEventsWritten) *NumEventsWritten = i;

    return Status;
}


ULONG
PreprocessInput(PCONSOLE Console,
                PINPUT_RECORD InputEvent,
                ULONG NumEventsToWrite);
VOID
PostprocessInput(PCONSOLE Console);

NTSTATUS
ConioAddInputEvents(PCONSOLE Console,
                    PINPUT_RECORD InputRecords, // InputEvent
                    ULONG NumEventsToWrite,
                    PULONG NumEventsWritten,
                    BOOLEAN AppendToEnd)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (NumEventsWritten) *NumEventsWritten = 0;

    /*
     * This pre-processing code MUST be IN consrv ONLY!!
     */
    NumEventsToWrite = PreprocessInput(Console, InputRecords, NumEventsToWrite);
    if (NumEventsToWrite == 0) return STATUS_SUCCESS;

    Status = ConDrvAddInputEvents(Console,
                                  InputRecords,
                                  NumEventsToWrite,
                                  NumEventsWritten,
                                  AppendToEnd);

    /*
     * This post-processing code MUST be IN consrv ONLY!!
     */
    // if (NT_SUCCESS(Status))
    if (Status == STATUS_SUCCESS) PostprocessInput(Console);

    return Status;
}

/* Move elsewhere...*/
NTSTATUS
ConioProcessInputEvent(PCONSOLE Console,
                       PINPUT_RECORD InputEvent)
{
    ULONG NumEventsWritten;
    return ConioAddInputEvents(Console,
                               InputEvent,
                               1,
                               &NumEventsWritten,
                               TRUE);
}


VOID
PurgeInputBuffer(PCONSOLE Console)
{
    PLIST_ENTRY CurrentEntry;
    ConsoleInput* Event;

    while (!IsListEmpty(&Console->InputBuffer.InputEvents))
    {
        CurrentEntry = RemoveHeadList(&Console->InputBuffer.InputEvents);
        Event = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        ConsoleFreeHeap(Event);
    }

    CloseHandle(Console->InputBuffer.ActiveEvent);
}


/* PUBLIC DRIVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvReadConsole(IN PCONSOLE Console,
                  IN PCONSOLE_INPUT_BUFFER InputBuffer,
                  /**/IN PUNICODE_STRING ExeName /**/OPTIONAL/**/,/**/
                  IN BOOLEAN Unicode,
                  OUT PVOID Buffer,
                  IN OUT PCONSOLE_READCONSOLE_CONTROL ReadControl,
                  IN ULONG NumCharsToRead,
                  OUT PULONG NumCharsRead OPTIONAL)
{
    // STATUS_PENDING : Wait if more to read ; STATUS_SUCCESS : Don't wait.
    NTSTATUS Status = STATUS_PENDING;
    PLIST_ENTRY CurrentEntry;
    ConsoleInput *Input;
    ULONG i;

    if (Console == NULL || InputBuffer == NULL || /* Buffer == NULL  || */
        ReadControl == NULL || ReadControl->nLength != sizeof(CONSOLE_READCONSOLE_CONTROL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT((Buffer != NULL) || (Buffer == NULL && NumCharsToRead == 0));

    /* We haven't read anything (yet) */

    i = ReadControl->nInitialChars;

    if (InputBuffer->Mode & ENABLE_LINE_INPUT)
    {
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

            /* Only pay attention to valid ASCII chars, on key down */
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

NTSTATUS NTAPI
ConDrvGetConsoleInput(IN PCONSOLE Console,
                      IN PCONSOLE_INPUT_BUFFER InputBuffer,
                      IN BOOLEAN KeepEvents,
                      IN BOOLEAN WaitForMoreEvents,
                      IN BOOLEAN Unicode,
                      OUT PINPUT_RECORD InputRecord,
                      IN ULONG NumEventsToRead,
                      OUT PULONG NumEventsRead OPTIONAL)
{
    PLIST_ENTRY CurrentInput;
    ConsoleInput* Input;
    ULONG i = 0;

    if (Console == NULL || InputBuffer == NULL /* || InputRecord == NULL */)
        return STATUS_INVALID_PARAMETER;

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT((InputRecord != NULL) || (InputRecord == NULL && NumEventsToRead == 0));

    if (NumEventsRead) *NumEventsRead = 0;

    if (IsListEmpty(&InputBuffer->InputEvents))
    {
        /*
         * No input is available. Wait for more input if requested,
         * otherwise, we don't wait, so we return success.
         */
        return (WaitForMoreEvents ? STATUS_PENDING : STATUS_SUCCESS);
    }

    /* Only get input if there is any */
    CurrentInput = InputBuffer->InputEvents.Flink;
    i = 0;
    while ((CurrentInput != &InputBuffer->InputEvents) && (i < NumEventsToRead))
    {
        Input = CONTAINING_RECORD(CurrentInput, ConsoleInput, ListEntry);

        *InputRecord = Input->InputEvent;

        ++InputRecord;
        ++i;
        CurrentInput = CurrentInput->Flink;

        /* Remove the events from the queue if needed */
        if (!KeepEvents)
        {
            RemoveEntryList(&Input->ListEntry);
            ConsoleFreeHeap(Input);
        }
    }

    if (NumEventsRead) *NumEventsRead = i;

    /* Now translate everything to ANSI */
    if (!Unicode)
    {
        for (; i > 0; --i)
        {
            ConioInputEventToAnsi(InputBuffer->Header.Console, --InputRecord);
        }
    }

    if (IsListEmpty(&InputBuffer->InputEvents))
    {
        ResetEvent(InputBuffer->ActiveEvent);
    }

    /* We read all the inputs available, we return success */
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvWriteConsoleInput(IN PCONSOLE Console,
                        IN PCONSOLE_INPUT_BUFFER InputBuffer,
                        IN BOOLEAN Unicode,
                        IN BOOLEAN AppendToEnd,
                        IN PINPUT_RECORD InputRecord,
                        IN ULONG NumEventsToWrite,
                        OUT PULONG NumEventsWritten OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    if (Console == NULL || InputBuffer == NULL /* || InputRecord == NULL */)
        return STATUS_INVALID_PARAMETER;

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT((InputRecord != NULL) || (InputRecord == NULL && NumEventsToWrite == 0));

    /* First translate everything to UNICODE */
    if (!Unicode)
    {
        for (i = 0; i < NumEventsToWrite; ++i)
        {
            ConioInputEventToUnicode(Console, &InputRecord[i]);
        }
    }

    /* Now, add the events */
    // if (NumEventsWritten) *NumEventsWritten = 0;
    // ConDrvAddInputEvents
    Status = ConioAddInputEvents(Console,
                                 InputRecord,
                                 NumEventsToWrite,
                                 NumEventsWritten,
                                 AppendToEnd);
    // if (NumEventsWritten) *NumEventsWritten = i;

    return Status;
}

NTSTATUS NTAPI
ConDrvFlushConsoleInputBuffer(IN PCONSOLE Console,
                              IN PCONSOLE_INPUT_BUFFER InputBuffer)
{
    PLIST_ENTRY CurrentEntry;
    ConsoleInput* Event;

    if (Console == NULL || InputBuffer == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == InputBuffer->Header.Console);

    /* Discard all entries in the input event queue */
    while (!IsListEmpty(&InputBuffer->InputEvents))
    {
        CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
        Event = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        ConsoleFreeHeap(Event);
    }
    ResetEvent(InputBuffer->ActiveEvent);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleNumberOfInputEvents(IN PCONSOLE Console,
                                    IN PCONSOLE_INPUT_BUFFER InputBuffer,
                                    OUT PULONG NumberOfEvents)
{
    PLIST_ENTRY CurrentInput;

    if (Console == NULL || InputBuffer == NULL || NumberOfEvents == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == InputBuffer->Header.Console);

    *NumberOfEvents = 0;

    /* If there are any events ... */
    CurrentInput = InputBuffer->InputEvents.Flink;
    while (CurrentInput != &InputBuffer->InputEvents)
    {
        CurrentInput = CurrentInput->Flink;
        (*NumberOfEvents)++;
    }

    return STATUS_SUCCESS;
}

/* EOF */

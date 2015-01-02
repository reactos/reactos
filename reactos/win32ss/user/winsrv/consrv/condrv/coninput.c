/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            consrv/condrv/coninput.c
 * PURPOSE:         Console Input functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct ConsoleInput_t
{
    LIST_ENTRY ListEntry;
    INPUT_RECORD InputEvent;
} ConsoleInput;


/* PRIVATE FUNCTIONS **********************************************************/

// ConDrvAddInputEvents
static NTSTATUS
AddInputEvents(PCONSOLE Console,
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

    if (SetWaitEvent) NtSetEvent(Console->InputBuffer.ActiveEvent, NULL);

Done:
    if (NumEventsWritten) *NumEventsWritten = i;

    return Status;
}

static VOID
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

    // CloseHandle(Console->InputBuffer.ActiveEvent);
}

NTSTATUS NTAPI
ConDrvInitInputBuffer(IN PCONSOLE Console,
                      IN ULONG InputBufferSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    ConSrvInitObject(&Console->InputBuffer.Header, INPUT_BUFFER, Console);

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_INHERIT,
                               NULL,
                               NULL);

    Status = NtCreateEvent(&Console->InputBuffer.ActiveEvent, EVENT_ALL_ACCESS,
                           &ObjectAttributes, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    Console->InputBuffer.InputBufferSize = InputBufferSize;
    InitializeListHead(&Console->InputBuffer.InputEvents);
    Console->InputBuffer.Mode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                                ENABLE_ECHO_INPUT      | ENABLE_MOUSE_INPUT;

    return STATUS_SUCCESS;
}

VOID NTAPI
ConDrvDeinitInputBuffer(IN PCONSOLE Console)
{
    PurgeInputBuffer(Console);
    CloseHandle(Console->InputBuffer.ActiveEvent);
}


/* PUBLIC DRIVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvReadConsole(IN PCONSOLE Console,
                  IN PCONSOLE_INPUT_BUFFER InputBuffer,
                  IN BOOLEAN Unicode,
                  OUT PVOID Buffer,
                  IN OUT PCONSOLE_READCONSOLE_CONTROL ReadControl,
                  IN PVOID Parameter OPTIONAL,
                  IN ULONG NumCharsToRead,
                  OUT PULONG NumCharsRead OPTIONAL)
{
    // STATUS_PENDING : Wait if more to read ; STATUS_SUCCESS : Don't wait.
    // NTSTATUS Status; = STATUS_PENDING;

    if (Console == NULL || InputBuffer == NULL || /* Buffer == NULL  || */
        ReadControl == NULL || ReadControl->nLength != sizeof(CONSOLE_READCONSOLE_CONTROL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT((Buffer != NULL) || (Buffer == NULL && NumCharsToRead == 0));

    /* Call the line-discipline */
    return TermReadStream(Console,
                          Unicode,
                          Buffer,
                          ReadControl,
                          Parameter,
                          NumCharsToRead,
                          NumCharsRead);
}

NTSTATUS NTAPI
ConDrvGetConsoleInput(IN PCONSOLE Console,
                      IN PCONSOLE_INPUT_BUFFER InputBuffer,
                      IN BOOLEAN KeepEvents,
                      IN BOOLEAN WaitForMoreEvents,
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

    if (IsListEmpty(&InputBuffer->InputEvents))
    {
        ResetEvent(InputBuffer->ActiveEvent);
    }

    // FIXME: If we add back UNICODE support, it's here that we need to do the translation.

    /* We read all the inputs available, we return success */
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvWriteConsoleInput(IN PCONSOLE Console,
                        IN PCONSOLE_INPUT_BUFFER InputBuffer,
                        IN BOOLEAN AppendToEnd,
                        IN PINPUT_RECORD InputRecord,
                        IN ULONG NumEventsToWrite,
                        OUT PULONG NumEventsWritten OPTIONAL)
{
    if (Console == NULL || InputBuffer == NULL /* || InputRecord == NULL */)
        return STATUS_INVALID_PARAMETER;

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT((InputRecord != NULL) || (InputRecord == NULL && NumEventsToWrite == 0));

    /* Now, add the events */
    if (NumEventsWritten) *NumEventsWritten = 0;

    // FIXME: If we add back UNICODE support, it's here that we need to do the translation.

    return AddInputEvents(Console,
                          InputRecord,
                          NumEventsToWrite,
                          NumEventsWritten,
                          AppendToEnd);
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

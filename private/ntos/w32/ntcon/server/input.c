/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    input.c

Abstract:

        This file implements the circular buffer management for
        input events.

        The circular buffer is described by a header,
        which resides in the beginning of the memory allocated when the
        buffer is created.  The header contains all of the
        per-buffer information, such as reader, writer, and
        reference counts, and also holds the pointers into
        the circular buffer proper.

        When the in and out pointers are equal, the circular buffer
        is empty.  When the in pointer trails the out pointer
        by 1, the buffer is full.  Thus, a 512 byte buffer can hold
        only 511 bytes; one byte is lost so that full and empty
        conditions can be distinguished. So that the user can
        put 512 bytes in a buffer that they created with a size
        of 512, we allow for this byte lost when allocating
        the memory.

Author:

    Therese Stowell (thereses) 6-Nov-1990
    Adapted from OS/2 subsystem server\srvpipe.c

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define CTRL_BUT_NOT_ALT(n) \
        (((n) & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) && \
        !((n) & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))

//
// this boolean is TRUE while we are processing a WM_QUERYENDSESSION message.
// since shutdown is guaranteed to be done serially (one window at a time),
// we can use one boolean.
//

UINT ProgmanHandleMessage;

int DialogBoxCount;

LPTHREAD_START_ROUTINE CtrlRoutine;  // address of client side ctrl-thread routine

DWORD InputThreadTlsIndex;

#define MAX_CHARS_FROM_1_KEYSTROKE 6


//
// the following data structures are a hack to work around the fact that
// MapVirtualKey does not return the correct virtual key code in many cases.
// we store the correct info (from the keydown message) in the CONSOLE_KEY_INFO
// structure when a keydown message is translated.  then when we receive a
// wm_[sys][dead]char message, we retrieve it and clear out the record.
//

#define CONSOLE_FREE_KEY_INFO 0
#define CONSOLE_MAX_KEY_INFO 32

typedef struct _CONSOLE_KEY_INFO {
    HWND hWnd;
    WORD wVirtualKeyCode;
    WORD wVirtualScanCode;
} CONSOLE_KEY_INFO, *PCONSOLE_KEY_INFO;

CONSOLE_KEY_INFO ConsoleKeyInfo[CONSOLE_MAX_KEY_INFO];

VOID
UserExitWorkerThread(VOID);

BOOL
InitWindowClass( VOID );

#if !defined(FE_SB)
NTSTATUS
ReadBuffer(
    IN PINPUT_INFORMATION InputInformation,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG EventsRead,
    IN BOOL Peek,
    IN BOOL StreamRead,
    OUT PBOOL ResetWaitEvent
    );
#endif


NTSTATUS
CreateInputBuffer(
    IN ULONG NumberOfEvents OPTIONAL,
    IN PINPUT_INFORMATION InputBufferInformation
#if defined(FE_SB)
    ,
    IN PCONSOLE_INFORMATION Console
#endif
    )

/*++

Routine Description:

    This routine creates an input buffer.  It allocates the circular
    buffer and initializes the information fields.

Arguments:

    NumberOfEvents - Size of input buffer in events.

    InputBufferInformation - Pointer to input buffer information structure.

Return Value:


--*/

{
    ULONG BufferSize;
    NTSTATUS Status;

    if (NumberOfEvents == 0) {
        NumberOfEvents = DEFAULT_NUMBER_OF_EVENTS;
    }

    // allocate memory for circular buffer

    BufferSize =  sizeof(INPUT_RECORD) * (NumberOfEvents+1);
    InputBufferInformation->InputBuffer = (PINPUT_RECORD)ConsoleHeapAlloc(MAKE_TAG( BUFFER_TAG ),BufferSize);
    if (InputBufferInformation->InputBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    Status = NtCreateEvent(&InputBufferInformation->InputWaitEvent,
                           EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        ConsoleHeapFree(InputBufferInformation->InputBuffer);
        return STATUS_NO_MEMORY;
    }
    InitializeListHead(&InputBufferInformation->ReadWaitQueue);

    // initialize buffer header

    InputBufferInformation->InputBufferSize = NumberOfEvents;
    InputBufferInformation->ShareAccess.OpenCount = 0;
    InputBufferInformation->ShareAccess.Readers = 0;
    InputBufferInformation->ShareAccess.Writers = 0;
    InputBufferInformation->ShareAccess.SharedRead = 0;
    InputBufferInformation->ShareAccess.SharedWrite = 0;
    InputBufferInformation->InputMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT;
    InputBufferInformation->AllocatedBufferSize = BufferSize;
    InputBufferInformation->RefCount = 0;
    InputBufferInformation->First = (ULONG_PTR) InputBufferInformation->InputBuffer;
    InputBufferInformation->In = (ULONG_PTR) InputBufferInformation->InputBuffer;
    InputBufferInformation->Out = (ULONG_PTR) InputBufferInformation->InputBuffer;
    InputBufferInformation->Last = (ULONG_PTR) InputBufferInformation->InputBuffer + BufferSize;
#if defined(FE_SB)
#if defined(FE_IME)
    InputBufferInformation->ImeMode.Disable     = FALSE;
    InputBufferInformation->ImeMode.Unavailable = FALSE;
    InputBufferInformation->ImeMode.Open        = FALSE;
    InputBufferInformation->ImeMode.ReadyConversion = FALSE;
#endif // FE_IME
    InputBufferInformation->Console = Console;
    RtlZeroMemory(&InputBufferInformation->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
    RtlZeroMemory(&InputBufferInformation->WriteConInpDbcsLeadByte,sizeof(INPUT_RECORD));
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
ReinitializeInputBuffer(
    OUT PINPUT_INFORMATION InputBufferInformation
    )

/*++

Routine Description:

    This routine resets the input buffer information fields to their
    initial values.

Arguments:

    InputBufferInformation - Pointer to input buffer information structure.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    NtClearEvent(InputBufferInformation->InputWaitEvent);
    InputBufferInformation->ShareAccess.OpenCount = 0;
    InputBufferInformation->ShareAccess.Readers = 0;
    InputBufferInformation->ShareAccess.Writers = 0;
    InputBufferInformation->ShareAccess.SharedRead = 0;
    InputBufferInformation->ShareAccess.SharedWrite = 0;
    InputBufferInformation->InputMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT  | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT;
    InputBufferInformation->In = (ULONG_PTR) InputBufferInformation->InputBuffer;
    InputBufferInformation->Out = (ULONG_PTR) InputBufferInformation->InputBuffer;
    return STATUS_SUCCESS;
}

VOID
FreeInputBuffer(
    IN PINPUT_INFORMATION InputBufferInformation
    )

/*++

Routine Description:

    This routine frees the resources associated with an input buffer.

Arguments:

    InputBufferInformation - Pointer to input buffer information structure.

Return Value:


--*/

{
    ASSERT(InputBufferInformation->RefCount == 0);
    CloseHandle(InputBufferInformation->InputWaitEvent);
    ConsoleHeapFree(InputBufferInformation->InputBuffer);
}


NTSTATUS
WaitForMoreToRead(
    IN PINPUT_INFORMATION InputInformation,
    IN PCSR_API_MSG Message OPTIONAL,
    IN CSR_WAIT_ROUTINE WaitRoutine OPTIONAL,
    IN PVOID WaitParameter OPTIONAL,
    IN ULONG WaitParameterLength  OPTIONAL,
    IN BOOLEAN WaitBlockExists OPTIONAL
    )

/*++

Routine Description:

    This routine waits for a writer to add data to the buffer.

Arguments:

    InputInformation - buffer to wait for

    Console - Pointer to console buffer information.

    Message - if called from dll (not InputThread), points to api
    message.  this parameter is used for wait block processing.

    WaitRoutine - Routine to call when wait is woken up.

    WaitParameter - Parameter to pass to wait routine.

    WaitParameterLength - Length of wait parameter.

    WaitBlockExists - TRUE if wait block has already been created.

Return Value:

    STATUS_WAIT - call was from client and wait block has been created.

    STATUS_SUCCESS - call was from server and wait has been satisfied.

--*/

{
    PVOID WaitParameterBuffer;

    if (!WaitBlockExists) {
        WaitParameterBuffer = (PVOID)ConsoleHeapAlloc(MAKE_TAG( WAIT_TAG ),WaitParameterLength);
        if (WaitParameterBuffer == NULL) {
            return STATUS_NO_MEMORY;
        }
        RtlCopyMemory(WaitParameterBuffer,WaitParameter,WaitParameterLength);
#if defined(FE_SB)
        if (WaitParameterLength == sizeof(COOKED_READ_DATA) &&
            InputInformation->Console->lpCookedReadData == WaitParameter) {
            InputInformation->Console->lpCookedReadData = WaitParameterBuffer;
        }
#endif
        if (!CsrCreateWait(&InputInformation->ReadWaitQueue,
                          WaitRoutine,
                          CSR_SERVER_QUERYCLIENTTHREAD(),
                          Message,
                          WaitParameterBuffer,
                          NULL
                         )) {
            ConsoleHeapFree(WaitParameterBuffer);
#if defined(FE_SB)
            InputInformation->Console->lpCookedReadData = NULL;
#endif
            return STATUS_NO_MEMORY;
        }
    }
    return CONSOLE_STATUS_WAIT;
}


VOID
WakeUpReadersWaitingForData(
    IN PCONSOLE_INFORMATION Console,
    PINPUT_INFORMATION InputInformation
    )

/*++

Routine Description:

    This routine wakes up readers waiting for data to read.

Arguments:

    InputInformation - buffer to alert readers for

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed.

--*/

{
    BOOLEAN WaitSatisfied;
    WaitSatisfied = CsrNotifyWait(&InputInformation->ReadWaitQueue,
                  FALSE,
                  NULL,
                  NULL
                 );
    if (WaitSatisfied) {
        // #334370 under stress, WaitQueue may already hold the satisfied waits
        ASSERT ((Console->WaitQueue == NULL) ||
                (Console->WaitQueue == &InputInformation->ReadWaitQueue));
        Console->WaitQueue = &InputInformation->ReadWaitQueue;
    }
}


NTSTATUS
GetNumberOfReadyEvents(
    IN PINPUT_INFORMATION InputInformation,
    OUT PULONG NumberOfEvents
    )

/*++

Routine Description:

    This routine returns the number of events in the input buffer.

Arguments:

    InputInformation - Pointer to input buffer information structure.

    NumberOfEvents - On output contains the number of events.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    if (InputInformation->In < InputInformation->Out) {
        *NumberOfEvents = (ULONG)(InputInformation->Last - InputInformation->Out);
        *NumberOfEvents += (ULONG)(InputInformation->In - InputInformation->First);
    }
    else {
        *NumberOfEvents = (ULONG)(InputInformation->In - InputInformation->Out);
    }
    *NumberOfEvents /= sizeof(INPUT_RECORD);

    return STATUS_SUCCESS;
}

NTSTATUS
FlushAllButKeys(
    PINPUT_INFORMATION InputInformation
    )

/*++

Routine Description:

    This routine removes all but the key events from the buffer.

Arguments:

    InputInformation - Pointer to input buffer information structure.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    ULONG NumberOfEventsRead,i;
    NTSTATUS Status;
    PINPUT_RECORD TmpInputBuffer,InPtr,TmpInputBufferPtr;
    ULONG BufferSize;
    BOOL Dummy;

    if (InputInformation->In != InputInformation->Out)  {

        //
        // allocate memory for temp buffer
        //

        BufferSize =  sizeof(INPUT_RECORD) * (InputInformation->InputBufferSize+1);
        TmpInputBuffer = (PINPUT_RECORD)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),BufferSize);
        if (TmpInputBuffer == NULL) {
            return STATUS_NO_MEMORY;
        }
        TmpInputBufferPtr = TmpInputBuffer;

        //
        // copy input buffer.
        // let ReadBuffer do any compaction work.
        //

        Status = ReadBuffer(InputInformation,
                            TmpInputBuffer,
                            InputInformation->InputBufferSize,
                            &NumberOfEventsRead,
                            TRUE,
                            FALSE,
                            &Dummy
#if defined(FE_SB)
                            ,
                            TRUE
#endif
                           );

        if (!NT_SUCCESS(Status)) {
            ConsoleHeapFree(TmpInputBuffer);
            return Status;
        }

        InputInformation->Out = (ULONG_PTR) InputInformation->InputBuffer;
        InPtr = InputInformation->InputBuffer;
        for (i=0;i<NumberOfEventsRead;i++) {
            if (TmpInputBuffer->EventType == KEY_EVENT) {
                *InPtr = *TmpInputBuffer;
                InPtr++;
            }
            TmpInputBuffer++;
        }
        InputInformation->In = (ULONG_PTR) InPtr;
        if (InputInformation->In == InputInformation->Out) {
            NtClearEvent(InputInformation->InputWaitEvent);
        }
        ConsoleHeapFree(TmpInputBufferPtr);
    }
    return STATUS_SUCCESS;
}

NTSTATUS
FlushInputBuffer(
    PINPUT_INFORMATION InputInformation
    )

/*++

Routine Description:

    This routine empties the input buffer

Arguments:

    InputInformation - Pointer to input buffer information structure.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    InputInformation->In = (ULONG_PTR) InputInformation->InputBuffer;
    InputInformation->Out = (ULONG_PTR) InputInformation->InputBuffer;
    NtClearEvent(InputInformation->InputWaitEvent);
    return STATUS_SUCCESS;
}


NTSTATUS
SetInputBufferSize(
    IN PINPUT_INFORMATION InputInformation,
    IN ULONG Size
    )

/*++

Routine Description:

    This routine resizes the input buffer.

Arguments:

    InputInformation - Pointer to input buffer information structure.

    Size - New size in number of events.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    ULONG NumberOfEventsRead;
    NTSTATUS Status;
    PINPUT_RECORD InputBuffer;
    ULONG BufferSize;
    BOOL Dummy;

#if DBG
    ULONG_PTR NumberOfEvents;
    if (InputInformation->In < InputInformation->Out) {
        NumberOfEvents = InputInformation->Last - InputInformation->Out;
        NumberOfEvents += InputInformation->In - InputInformation->First;
    }
    else {
        NumberOfEvents = InputInformation->In - InputInformation->Out;
    }
    NumberOfEvents /= sizeof(INPUT_RECORD);
#endif
    ASSERT( Size > InputInformation->InputBufferSize );

    //
    // allocate memory for new input buffer
    //

    BufferSize =  sizeof(INPUT_RECORD) * (Size+1);
    InputBuffer = (PINPUT_RECORD)ConsoleHeapAlloc(MAKE_TAG( BUFFER_TAG ),BufferSize);
    if (InputBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // copy old input buffer.
    // let the ReadBuffer do any compaction work.
    //

    Status = ReadBuffer(InputInformation,
                        InputBuffer,
                        Size,
                        &NumberOfEventsRead,
                        TRUE,
                        FALSE,
                        &Dummy
#if defined(FE_SB)
                        ,
                        TRUE
#endif
                       );

    if (!NT_SUCCESS(Status)) {
        ConsoleHeapFree(InputBuffer);
        return Status;
    }
    InputInformation->Out = (ULONG_PTR)InputBuffer;
    InputInformation->In = (ULONG_PTR)InputBuffer + sizeof(INPUT_RECORD) * NumberOfEventsRead;

    //
    // adjust pointers
    //

    InputInformation->First = (ULONG_PTR) InputBuffer;
    InputInformation->Last = (ULONG_PTR) InputBuffer + BufferSize;

    //
    // free old input buffer
    //

    ConsoleHeapFree(InputInformation->InputBuffer);
    InputInformation->InputBufferSize = Size;
    InputInformation->AllocatedBufferSize = BufferSize;
    InputInformation->InputBuffer = InputBuffer;
    return Status;
}


NTSTATUS
ReadBuffer(
    IN PINPUT_INFORMATION InputInformation,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG EventsRead,
    IN BOOL Peek,
    IN BOOL StreamRead,
    OUT PBOOL ResetWaitEvent
#ifdef FE_SB
    , IN BOOLEAN Unicode
#endif
    )
/*++

Routine Description:

    This routine reads from a buffer.  It does the actual circular buffer
    manipulation.

Arguments:

    InputInformation - buffer to read from

    Buffer - buffer to read into

    Length - length of buffer in events

    EventsRead - where to store number of events read

    Peek - if TRUE, don't remove data from buffer, just copy it.

    StreamRead - if TRUE, events with repeat counts > 1 are returned
    as multiple events.  also, EventsRead == 1.

    ResetWaitEvent - on exit, TRUE if buffer became empty.

Return Value:

    ??

Note:

    The console lock must be held when calling this routine.

--*/

{
    ULONG TransferLength,OldTransferLength;
    ULONG BufferLengthInBytes;
#ifdef FE_SB
    PCONSOLE_INFORMATION Console;
    ULONG Length2;
    PINPUT_RECORD BufferRecords;
    PINPUT_RECORD QueueRecords;
    WCHAR UniChar;
    WORD EventType;
#endif

#ifdef FE_SB
    Console = InputInformation->Console;
#endif
    *ResetWaitEvent = FALSE;

    //
    // if StreamRead, just return one record.  if repeat count is greater
    // than one, just decrement it.  the repeat count is > 1 if more than
    // one event of the same type was merged.  we need to expand them back
    // to individual events here.
    //

    if (StreamRead &&
        ((PINPUT_RECORD)(InputInformation->Out))->EventType == KEY_EVENT) {

        ASSERT(Length == 1);
        ASSERT(InputInformation->In != InputInformation->Out);
        RtlMoveMemory((PBYTE)Buffer,
                      (PBYTE)InputInformation->Out,
                      sizeof(INPUT_RECORD)
                     );
        InputInformation->Out += sizeof(INPUT_RECORD);
        if (InputInformation->Last == InputInformation->Out) {
            InputInformation->Out = InputInformation->First;
        }
        if (InputInformation->Out == InputInformation->In) {
            *ResetWaitEvent = TRUE;
        }
        *EventsRead = 1;
        return STATUS_SUCCESS;
    }

    BufferLengthInBytes = Length * sizeof(INPUT_RECORD);

    //
    // if in > out, buffer looks like this:
    //
    //         out     in
    //    ______ _____________
    //   |      |      |      |
    //   | free | data | free |
    //   |______|______|______|
    //
    // we transfer the requested number of events or the amount in the buffer
    //

    if (InputInformation->In > InputInformation->Out) {
        if  ((InputInformation->In - InputInformation->Out) > BufferLengthInBytes) {
            TransferLength = BufferLengthInBytes;
        }
        else {
            TransferLength = (ULONG)(InputInformation->In - InputInformation->Out);
        }
#ifdef FE_SB
        if (!Unicode) {
            BufferLengthInBytes = 0;
            OldTransferLength = TransferLength / sizeof(INPUT_RECORD);
            BufferRecords = (PINPUT_RECORD)Buffer;
            QueueRecords = (PINPUT_RECORD)InputInformation->Out;

            while (BufferLengthInBytes < Length &&
                   OldTransferLength) {
                UniChar = QueueRecords->Event.KeyEvent.uChar.UnicodeChar;
                EventType = QueueRecords->EventType;
                *BufferRecords++ = *QueueRecords++;
                if (EventType == KEY_EVENT) {
                    if (IsConsoleFullWidth(Console->hDC,
                                           Console->CP,
                                           UniChar)) {
                        BufferLengthInBytes += 2;
                    }
                    else {
                        BufferLengthInBytes++;
                    }
                }
                else {
                    BufferLengthInBytes++;
                }
                OldTransferLength--;
            }
            ASSERT(TransferLength >= OldTransferLength * sizeof(INPUT_RECORD));
            TransferLength -= OldTransferLength * sizeof(INPUT_RECORD);
        }
        else
#endif
        {
            RtlMoveMemory((PBYTE)Buffer,
                          (PBYTE)InputInformation->Out,
                          TransferLength
                         );
        }
        *EventsRead = TransferLength / sizeof(INPUT_RECORD);
#ifdef FE_SB
        ASSERT(*EventsRead <= Length);
#endif
        if (!Peek) {
            InputInformation->Out += TransferLength;
#ifdef FE_SB
            ASSERT(InputInformation->Out <= InputInformation->Last);
#endif
        }
        if (InputInformation->Out == InputInformation->In) {
            *ResetWaitEvent = TRUE;
        }
        return STATUS_SUCCESS;
    }

    //
    // if out > in, buffer looks like this:
    //
    //         in     out
    //    ______ _____________
    //   |      |      |      |
    //   | data | free | data |
    //   |______|______|______|
    //
    // we read from the out pointer to the end of the buffer then from the
    // beginning of the buffer, until we hit the in pointer or enough bytes
    // are read.
    //

    else {

        if  ((InputInformation->Last - InputInformation->Out) > BufferLengthInBytes) {
            TransferLength = BufferLengthInBytes;
        }
        else {
            TransferLength = (ULONG)(InputInformation->Last - InputInformation->Out);
        }
#ifdef FE_SB
        if (!Unicode) {
            BufferLengthInBytes = 0;
            OldTransferLength = TransferLength / sizeof(INPUT_RECORD);
            BufferRecords = (PINPUT_RECORD)Buffer;
            QueueRecords = (PINPUT_RECORD)InputInformation->Out;

            while (BufferLengthInBytes < Length &&
                   OldTransferLength) {
                UniChar = QueueRecords->Event.KeyEvent.uChar.UnicodeChar;
                EventType = QueueRecords->EventType;
                *BufferRecords++ = *QueueRecords++;
                if (EventType == KEY_EVENT) {
                    if (IsConsoleFullWidth(Console->hDC,
                                           Console->CP,
                                    UniChar)) {
                        BufferLengthInBytes += 2;
                    }
                    else {
                        BufferLengthInBytes++;
                    }
                }
                else {
                    BufferLengthInBytes++;
                }
                OldTransferLength--;
            }
            ASSERT(TransferLength >= OldTransferLength * sizeof(INPUT_RECORD));
            TransferLength -= OldTransferLength * sizeof(INPUT_RECORD);
        }
        else
#endif
        {
            RtlMoveMemory((PBYTE)Buffer,
                          (PBYTE)InputInformation->Out,
                          TransferLength
                         );
        }
        *EventsRead = TransferLength / sizeof(INPUT_RECORD);
#ifdef FE_SB
        ASSERT(*EventsRead <= Length);
#endif

        if (!Peek) {
            InputInformation->Out += TransferLength;
#ifdef FE_SB
            ASSERT(InputInformation->Out <= InputInformation->Last);
#endif
            if (InputInformation->Out == InputInformation->Last) {
                InputInformation->Out = InputInformation->First;
            }
        }
#ifdef FE_SB
        if (!Unicode) {
            if (BufferLengthInBytes >= Length) {
                if (InputInformation->Out == InputInformation->In) {
                    *ResetWaitEvent = TRUE;
                }
                return STATUS_SUCCESS;
            }
        }
        else
#endif
        if (*EventsRead == Length) {
            if (InputInformation->Out == InputInformation->In) {
                *ResetWaitEvent = TRUE;
            }
            return STATUS_SUCCESS;
        }

        //
        // hit end of buffer, read from beginning
        //

        OldTransferLength = TransferLength;
#ifdef FE_SB
        Length2 = Length;
        if (!Unicode) {
            ASSERT(Length > BufferLengthInBytes);
            Length -= BufferLengthInBytes;
            if (Length == 0) {
                if (InputInformation->Out == InputInformation->In) {
                    *ResetWaitEvent = TRUE;
                }
            return STATUS_SUCCESS;
            }
            BufferLengthInBytes = Length * sizeof(INPUT_RECORD);

            if ((InputInformation->In - InputInformation->First) > BufferLengthInBytes) {
                TransferLength = BufferLengthInBytes;
            }
            else {
                TransferLength = (ULONG)(InputInformation->In - InputInformation->First);
            }
        }
        else
#endif
        if  ((InputInformation->In - InputInformation->First) > (BufferLengthInBytes - OldTransferLength)) {
            TransferLength = BufferLengthInBytes - OldTransferLength;
        }
        else {
            TransferLength = (ULONG)(InputInformation->In - InputInformation->First);
        }
#ifdef FE_SB
        if (!Unicode) {
            BufferLengthInBytes = 0;
            OldTransferLength = TransferLength / sizeof(INPUT_RECORD);
            QueueRecords = (PINPUT_RECORD)InputInformation->First;

            while (BufferLengthInBytes < Length &&
                   OldTransferLength) {
                UniChar = QueueRecords->Event.KeyEvent.uChar.UnicodeChar;
                EventType = QueueRecords->EventType;
                *BufferRecords++ = *QueueRecords++;
                if (EventType == KEY_EVENT) {
                    if (IsConsoleFullWidth(Console->hDC,
                                           Console->CP,
                                    UniChar)) {
                        BufferLengthInBytes += 2;
                    }
                    else {
                        BufferLengthInBytes++;
                    }
                }
                else {
                    BufferLengthInBytes++;
                }
                OldTransferLength--;
            }
            ASSERT(TransferLength >= OldTransferLength * sizeof(INPUT_RECORD));
            TransferLength -= OldTransferLength * sizeof(INPUT_RECORD);
        }
        else
#endif
        {
            RtlMoveMemory((PBYTE)Buffer+OldTransferLength,
                          (PBYTE)InputInformation->First,
                          TransferLength
                         );
        }
        *EventsRead += TransferLength / sizeof(INPUT_RECORD);
#ifdef FE_SB
        ASSERT(*EventsRead <= Length2);
#endif
        if (!Peek) {
            InputInformation->Out = InputInformation->First + TransferLength;
        }
        if (InputInformation->Out == InputInformation->In) {
            *ResetWaitEvent = TRUE;
        }
        return STATUS_SUCCESS;
    }
}


NTSTATUS
ReadInputBuffer(
    IN PINPUT_INFORMATION InputInformation,
    OUT PINPUT_RECORD lpBuffer,
    IN OUT PDWORD nLength,
    IN BOOL Peek,
    IN BOOL WaitForData,
    IN BOOL StreamRead,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData OPTIONAL,
    IN PCSR_API_MSG Message OPTIONAL,
    IN CSR_WAIT_ROUTINE WaitRoutine OPTIONAL,
    IN PVOID WaitParameter OPTIONAL,
    IN ULONG WaitParameterLength  OPTIONAL,
    IN BOOLEAN WaitBlockExists OPTIONAL
#if defined(FE_SB)
    ,
    IN BOOLEAN Unicode
#endif
    )

/*++

Routine Description:

    This routine reads from the input buffer.

Arguments:

    InputInformation - Pointer to input buffer information structure.

    lpBuffer - Buffer to read into.

    nLength - On input, number of events to read.  On output, number of
    events read.

    Peek - If TRUE, copy events to lpBuffer but don't remove them from
    the input buffer.

    WaitForData - if TRUE, wait until an event is input.  if FALSE, return
        immediately

    StreamRead - if TRUE, events with repeat counts > 1 are returned
    as multiple events.  also, EventsRead == 1.

    Console - Pointer to console buffer information.

    HandleData - Pointer to handle data structure.  This parameter is
    optional if WaitForData is false.

    Message - if called from dll (not InputThread), points to api
    message.  this parameter is used for wait block processing.

    WaitRoutine - Routine to call when wait is woken up.

    WaitParameter - Parameter to pass to wait routine.

    WaitParameterLength - Length of wait parameter.

    WaitBlockExists - TRUE if wait block has already been created.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    ULONG EventsRead;
    NTSTATUS Status;
    BOOL ResetWaitEvent;

    if (InputInformation->In == InputInformation->Out)  {
        if (!WaitForData) {
            *nLength = 0;
            return STATUS_SUCCESS;
        }
        LockReadCount(HandleData);
        HandleData->InputReadData->ReadCount += 1;
        UnlockReadCount(HandleData);
        Status = WaitForMoreToRead(InputInformation,
                                   Message,
                                   WaitRoutine,
                                   WaitParameter,
                                   WaitParameterLength,
                                   WaitBlockExists
                                  );

        if (!NT_SUCCESS(Status)) {
            if (Status != CONSOLE_STATUS_WAIT) {
                /*
                 * WaitForMoreToRead failed, restore ReadCount and bale out
                 */
                LockReadCount(HandleData);
                HandleData->InputReadData->ReadCount -= 1;
                UnlockReadCount(HandleData);
            }
            *nLength = 0;
            return Status;
        }

        //
        // we will only get to this point if we were called by GetInput.
        //
        ASSERT(FALSE); // I say we never get here !  IANJA

        LockConsole(Console);
    }

    //
    // read from buffer
    //

    Status = ReadBuffer(InputInformation,
                        lpBuffer,
                        *nLength,
                        &EventsRead,
                        Peek,
                        StreamRead,
                        &ResetWaitEvent
#if defined(FE_SB)
                        ,
                        Unicode
#endif
                       );
    if (ResetWaitEvent) {
        NtClearEvent(InputInformation->InputWaitEvent);
    }

    *nLength = EventsRead;
    return Status;
}


NTSTATUS
WriteBuffer(
    OUT PINPUT_INFORMATION InputInformation,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG EventsWritten,
    OUT PBOOL SetWaitEvent
    )

/*++

Routine Description:

    This routine writes to a buffer.  It does the actual circular buffer
    manipulation.

Arguments:

    InputInformation - buffer to write to

    Buffer - buffer to write from

    Length - length of buffer in events

    BytesRead - where to store number of bytes written.

    SetWaitEvent - on exit, TRUE if buffer became non-empty.

Return Value:

    ERROR_BROKEN_PIPE - no more readers.

Note:

    The console lock must be held when calling this routine.

--*/

{
    NTSTATUS Status;
    ULONG TransferLength;
    ULONG BufferLengthInBytes;
#if defined(FE_SB)
    PCONSOLE_INFORMATION Console = InputInformation->Console;
#endif

    *SetWaitEvent = FALSE;

    //
    // windows sends a mouse_move message each time a window is updated.
    // coalesce these.
    //

    if (Length == 1 && InputInformation->Out != InputInformation->In) {
        PINPUT_RECORD InputEvent=Buffer;

        if (InputEvent->EventType == MOUSE_EVENT &&
            InputEvent->Event.MouseEvent.dwEventFlags == MOUSE_MOVED) {
            PINPUT_RECORD LastInputEvent;

            if (InputInformation->In == InputInformation->First) {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->Last - sizeof(INPUT_RECORD));
            }
            else {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->In - sizeof(INPUT_RECORD));
            }
            if (LastInputEvent->EventType == MOUSE_EVENT &&
                LastInputEvent->Event.MouseEvent.dwEventFlags == MOUSE_MOVED) {
                LastInputEvent->Event.MouseEvent.dwMousePosition.X =
                    InputEvent->Event.MouseEvent.dwMousePosition.X;
                LastInputEvent->Event.MouseEvent.dwMousePosition.Y =
                    InputEvent->Event.MouseEvent.dwMousePosition.Y;
                *EventsWritten = 1;
                return STATUS_SUCCESS;
            }
        }
        else if (InputEvent->EventType == KEY_EVENT &&
                 InputEvent->Event.KeyEvent.bKeyDown) {
            PINPUT_RECORD LastInputEvent;
            if (InputInformation->In == InputInformation->First) {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->Last - sizeof(INPUT_RECORD));
            }
            else {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->In - sizeof(INPUT_RECORD));
            }
#if defined(FE_SB)
            if (IsConsoleFullWidth(Console->hDC,
                                   Console->CP,InputEvent->Event.KeyEvent.uChar.UnicodeChar)) {
                ;
            }
            else
            if (InputEvent->Event.KeyEvent.dwControlKeyState & NLS_IME_CONVERSION) {
                if (LastInputEvent->EventType == KEY_EVENT &&
                    LastInputEvent->Event.KeyEvent.bKeyDown &&
                    (LastInputEvent->Event.KeyEvent.uChar.UnicodeChar ==
                        InputEvent->Event.KeyEvent.uChar.UnicodeChar) &&
                    (LastInputEvent->Event.KeyEvent.dwControlKeyState ==
                        InputEvent->Event.KeyEvent.dwControlKeyState) ) {
                    LastInputEvent->Event.KeyEvent.wRepeatCount +=
                        InputEvent->Event.KeyEvent.wRepeatCount;
                    *EventsWritten = 1;
                    return STATUS_SUCCESS;
                }
            }
            else
#endif
            if (LastInputEvent->EventType == KEY_EVENT &&
                LastInputEvent->Event.KeyEvent.bKeyDown &&
                (LastInputEvent->Event.KeyEvent.wVirtualScanCode == // scancode same
                    InputEvent->Event.KeyEvent.wVirtualScanCode) &&
                (LastInputEvent->Event.KeyEvent.uChar.UnicodeChar == // character same
                    InputEvent->Event.KeyEvent.uChar.UnicodeChar) &&
                (LastInputEvent->Event.KeyEvent.dwControlKeyState == // ctrl/alt/shift state same
                    InputEvent->Event.KeyEvent.dwControlKeyState) ) {
                LastInputEvent->Event.KeyEvent.wRepeatCount +=
                    InputEvent->Event.KeyEvent.wRepeatCount;
                *EventsWritten = 1;
                return STATUS_SUCCESS;
            }
        }
    }

    BufferLengthInBytes = Length*sizeof(INPUT_RECORD);
    *EventsWritten = 0;
    while (*EventsWritten < Length) {

        //
        //
        // if out > in, buffer looks like this:
        //
        //             in     out
        //        ______ _____________
        //       |      |      |      |
        //       | data | free | data |
        //       |______|______|______|
        //
        // we can write from in to out-1
        //

        if (InputInformation->Out > InputInformation->In)       {
            TransferLength = BufferLengthInBytes;
            if  ((InputInformation->Out - InputInformation->In - sizeof(INPUT_RECORD))
                   < BufferLengthInBytes) {
                Status = SetInputBufferSize(InputInformation,
                                            InputInformation->InputBufferSize+Length+INPUT_BUFFER_SIZE_INCREMENT);
                if (!NT_SUCCESS(Status)) {
                    KdPrint(("CONSRV: Couldn't grow input buffer, Status == %lX\n",Status));
                    TransferLength = (ULONG)(InputInformation->Out - InputInformation->In - sizeof(INPUT_RECORD));
                    if (TransferLength == 0) {
                        return Status;
                    }
                } else {
                    goto OutPath;   // after resizing, in > out
                }
            }
            RtlMoveMemory((PBYTE)InputInformation->In,
                          (PBYTE)Buffer,
                          TransferLength
                         );
            Buffer = (PVOID) (((PBYTE) Buffer)+TransferLength);
            *EventsWritten += TransferLength/sizeof(INPUT_RECORD);
            BufferLengthInBytes -= TransferLength;
            InputInformation->In += TransferLength;
        }

        //
        // if in >= out, buffer looks like this:
        //
        //             out     in
        //        ______ _____________
        //       |      |      |      |
        //       | free | data | free |
        //       |______|______|______|
        //
        // we write from the in pointer to the end of the buffer then from the
        // beginning of the buffer, until we hit the out pointer or enough bytes
        // are written.
        //

        else {
            if (InputInformation->Out == InputInformation->In) {
                *SetWaitEvent = TRUE;
            }
OutPath:
            if  ((InputInformation->Last - InputInformation->In) > BufferLengthInBytes) {
                TransferLength = BufferLengthInBytes;
            }
            else {
                if (InputInformation->First == InputInformation->Out &&
                    InputInformation->In == (InputInformation->Last-sizeof(INPUT_RECORD))) {
                    TransferLength = BufferLengthInBytes;
                    Status = SetInputBufferSize(InputInformation,
                                                InputInformation->InputBufferSize+Length+INPUT_BUFFER_SIZE_INCREMENT);
                    if (!NT_SUCCESS(Status)) {
                        KdPrint(("CONSRV: Couldn't grow input buffer, Status == %lX\n",Status));
                        return Status;
                    }
                }
                else {
                    TransferLength = (ULONG)(InputInformation->Last - InputInformation->In);
                    if (InputInformation->First == InputInformation->Out) {
                        TransferLength -= sizeof(INPUT_RECORD);
                    }
                }
            }
            RtlMoveMemory((PBYTE)InputInformation->In,
                          (PBYTE)Buffer,
                          TransferLength
                         );
            Buffer = (PVOID) (((PBYTE) Buffer)+TransferLength);
            *EventsWritten += TransferLength/sizeof(INPUT_RECORD);
            BufferLengthInBytes -= TransferLength;
            InputInformation->In += TransferLength;
            if (InputInformation->In == InputInformation->Last) {
                InputInformation->In = InputInformation->First;
            }
        }
        if (TransferLength == 0) {
            ASSERT (FALSE);
        }
    }
    return STATUS_SUCCESS;
}


__inline BOOL
IsSystemKey(
    WORD wVirtualKeyCode
    )
{
    switch (wVirtualKeyCode) {
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
    case VK_PAUSE:
    case VK_CAPITAL:
    case VK_LWIN:
    case VK_RWIN:
    case VK_NUMLOCK:
    case VK_SCROLL:
        return TRUE;
    }
    return FALSE;
}

DWORD
PreprocessInput(
    IN PCONSOLE_INFORMATION Console,
    IN PINPUT_RECORD InputEvent,
    IN DWORD nLength
    )

/*++

Routine Description:

    This routine processes special characters in the input stream.

Arguments:

    Console - Pointer to console structure.

    InputEvent - Buffer to write from.

    nLength - Number of events to write.

Return Value:

    Number of events to write after special characters have been stripped.

Note:

    The console lock must be held when calling this routine.

--*/

{
    ULONG NumEvents;


    for (NumEvents = nLength; NumEvents != 0; NumEvents--) {
        if (InputEvent->EventType == KEY_EVENT && InputEvent->Event.KeyEvent.bKeyDown) {
            //
            // if output is suspended, any keyboard input releases it.
            //

            if ((Console->Flags & CONSOLE_SUSPENDED) &&
                !IsSystemKey(InputEvent->Event.KeyEvent.wVirtualKeyCode)) {

                UnblockWriteConsole(Console, CONSOLE_OUTPUT_SUSPENDED);
                RtlMoveMemory(InputEvent, InputEvent + 1, (NumEvents - 1) * sizeof(INPUT_RECORD));
                nLength--;
                continue;
            }

            //
            // intercept control-s
            //

            if ((Console->InputBuffer.InputMode & ENABLE_LINE_INPUT) &&
                    (InputEvent->Event.KeyEvent.wVirtualKeyCode == VK_PAUSE ||
                     IsPauseKey(&InputEvent->Event.KeyEvent))) {

                Console->Flags |= CONSOLE_OUTPUT_SUSPENDED;
                RtlMoveMemory(InputEvent, InputEvent + 1, (NumEvents - 1) * sizeof(INPUT_RECORD));
                nLength--;
                continue;
            }
        }
        InputEvent++;
    }
    return nLength;
}


DWORD
PrependInputBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PINPUT_INFORMATION InputInformation,
    IN PINPUT_RECORD lpBuffer,
    IN DWORD nLength
    )

/*++

Routine Description:

    This routine writes to the beginning of the input buffer.

Arguments:

    InputInformation - Pointer to input buffer information structure.

    lpBuffer - Buffer to write from.

    nLength - On input, number of events to write.  On output, number of
    events written.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    NTSTATUS Status;
    ULONG EventsWritten,EventsRead;
    BOOL SetWaitEvent;
    ULONG NumExistingEvents;
    PINPUT_RECORD pExistingEvents;
    BOOL Dummy;

    nLength = PreprocessInput(Console, lpBuffer, nLength);
    if (nLength == 0) {
        return 0;
    }

    Status = GetNumberOfReadyEvents(InputInformation,
                                    &NumExistingEvents
                                   );

    if (NumExistingEvents) {

        pExistingEvents = ConsoleHeapAlloc(MAKE_TAG( BUFFER_TAG ),NumExistingEvents * sizeof(INPUT_RECORD));
        if (pExistingEvents == NULL)
            return (DWORD)STATUS_NO_MEMORY;
        Status = ReadBuffer(InputInformation,
                            pExistingEvents,
                            NumExistingEvents,
                            &EventsRead,
                            FALSE,
                            FALSE,
                            &Dummy
#if defined(FE_SB)
                            ,
                            TRUE
#endif
                           );

        if (!NT_SUCCESS(Status)) {
            ConsoleHeapFree(pExistingEvents);
            return Status;
        }
    } else {
        pExistingEvents = NULL;
    }

    //
    // write new info to buffer
    //

    Status = WriteBuffer(InputInformation,
                         lpBuffer,
                         nLength,
                         &EventsWritten,
                         &SetWaitEvent
                        );

    //
    // write existing info to buffer
    //

    if (pExistingEvents) {
        Status = WriteBuffer(InputInformation,
                             pExistingEvents,
                             EventsRead,
                             &EventsWritten,
                             &Dummy
                            );
        ConsoleHeapFree(pExistingEvents);
    }

    if (SetWaitEvent) {
        NtSetEvent(InputInformation->InputWaitEvent,NULL);
    }

    //
    // alert any writers waiting for space
    //

    WakeUpReadersWaitingForData(Console,InputInformation);

    return nLength;
}

DWORD
WriteInputBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PINPUT_INFORMATION InputInformation,
    IN PINPUT_RECORD lpBuffer,
    IN DWORD nLength
    )

/*++

Routine Description:

    This routine writes to the input buffer.

Arguments:

    InputInformation - Pointer to input buffer information structure.

    lpBuffer - Buffer to write from.

    nLength - On input, number of events to write.  On output, number of
    events written.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    ULONG EventsWritten;
    BOOL SetWaitEvent;

    nLength = PreprocessInput(Console, lpBuffer, nLength);
    if (nLength == 0) {
        return 0;
    }

    //
    // write to buffer
    //

    WriteBuffer(InputInformation,
                lpBuffer,
                nLength,
                &EventsWritten,
                &SetWaitEvent
                );

    if (SetWaitEvent) {
        NtSetEvent(InputInformation->InputWaitEvent,NULL);
    }

    //
    // alert any writers waiting for space
    //

    WakeUpReadersWaitingForData(Console,InputInformation);


    return EventsWritten;
}

VOID
StoreKeyInfo(
    IN PMSG msg
    )
{
    int i;

    for (i=0;i<CONSOLE_MAX_KEY_INFO;i++) {
        if (ConsoleKeyInfo[i].hWnd == CONSOLE_FREE_KEY_INFO ||
            ConsoleKeyInfo[i].hWnd == msg->hwnd) {
            break;
        }
    }
    if (i!=CONSOLE_MAX_KEY_INFO) {
        ConsoleKeyInfo[i].hWnd = msg->hwnd;
        ConsoleKeyInfo[i].wVirtualKeyCode = LOWORD(msg->wParam);
        ConsoleKeyInfo[i].wVirtualScanCode = (BYTE)(HIWORD(msg->lParam));
    } else {
        KdPrint(("CONSRV: ConsoleKeyInfo buffer is full\n"));
    }
}

VOID
RetrieveKeyInfo(
    IN HWND hWnd,
    OUT PWORD pwVirtualKeyCode,
    OUT PWORD pwVirtualScanCode,
    IN BOOL FreeKeyInfo
    )
{
    int i;

    for (i=0;i<CONSOLE_MAX_KEY_INFO;i++) {
        if (ConsoleKeyInfo[i].hWnd == hWnd) {
            break;
        }
    }
    if (i!=CONSOLE_MAX_KEY_INFO) {
        *pwVirtualKeyCode = ConsoleKeyInfo[i].wVirtualKeyCode;
        *pwVirtualScanCode = ConsoleKeyInfo[i].wVirtualScanCode;
        if (FreeKeyInfo)
            ConsoleKeyInfo[i].hWnd = CONSOLE_FREE_KEY_INFO;
    } else {
        *pwVirtualKeyCode = (WORD)MapVirtualKey(*pwVirtualScanCode, 3);
    }
}

VOID
ClearKeyInfo(
    IN HWND hWnd
    )
{
    int i;

    for (i=0;i<CONSOLE_MAX_KEY_INFO;i++) {
        if (ConsoleKeyInfo[i].hWnd == hWnd) {
            ConsoleKeyInfo[i].hWnd = CONSOLE_FREE_KEY_INFO;
        }
    }
}


/***************************************************************************\
* ProcessCreateConsoleWindow
*
* This routine processes a CM_CREATE_CONSOLE_WINDOW message. It is called
* from the InputThread message loop under normal circumstances and from
* the DialogHookProc if we have a dialog box up. The USER critical section
* should not be held when calling this routine.
\***************************************************************************/
VOID
ProcessCreateConsoleWindow(
    IN LPMSG lpMsg
    )
{
    NTSTATUS Status;
    // make sure this is a valid message
    PCONSOLE_INFORMATION pConsole;

    if (NT_SUCCESS(RevalidateConsole((HANDLE)lpMsg->wParam, &pConsole))) {

        //
        // Make sure the console doesn't already have a window.
        //

        if (pConsole->hWnd) {
            RIPMSG1(RIP_WARNING, "Console %#p already has a window", pConsole);
            UnlockConsole(pConsole);
            return;
        }

        pConsole->InputThreadInfo = TlsGetValue(InputThreadTlsIndex);
        DBGPRINT(("Before CreateWindowsWindow cWindows = %d\n",
                  pConsole->InputThreadInfo->WindowCount));

        Status = CreateWindowsWindow(pConsole);
        DBGPRINT(("After  CreateWindowsWindow cWindows = %d\n",
                  pConsole->InputThreadInfo->WindowCount));
        switch (Status) {
        case STATUS_SUCCESS:

            //
            // If we changed the screen buffer size, let the user know about it
            // with a message box. Make sure we don't recurse too deeply in
            // this code by limiting the number of message boxes on the screen
            // at once. If there are already a bunch up there, the user should
            // have the idea by now anyway.
            //

            if ((pConsole->Flags & CONSOLE_DEFAULT_BUFFER_SIZE) && (DialogBoxCount < 8)) {
                WCHAR ItemString[120];
                WCHAR Title[120];
                HWND hWnd = pConsole->hWnd;
                ULONG TitleLength = min(sizeof(Title)-sizeof(WCHAR), pConsole->TitleLength);
                RtlCopyMemory(Title, pConsole->Title, TitleLength);
                Title[TitleLength/sizeof(WCHAR)] = 0;
                UnlockConsole(pConsole);
                LoadString(ghInstance,msgBufferTooBig,ItemString,NELEM(ItemString));
                DialogBoxCount++;
                MessageBox(hWnd, ItemString, Title, MB_OK);
                DialogBoxCount--;
                break;
            }
            // FALL THRU
        case STATUS_NO_MEMORY:
            UnlockConsole(pConsole);
            break;
        case STATUS_INVALID_HANDLE:
            // Console is gone, don't do anything.
            break;
        default:
            KdPrint(("CONSRV: CreateWindowsWindow returned %x\n", Status));
            ASSERT(FALSE);
            break;
        }
    }
}


LRESULT
DialogHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )

// this routine gets called to filter input to console dialogs so
// that we can do the special processing that StoreKeyInfo does.

{
    MSG *pmsg = (PMSG)lParam;

    UNREFERENCED_PARAMETER(wParam);

    if (pmsg->message == CM_CREATE_CONSOLE_WINDOW) {
        ProcessCreateConsoleWindow(pmsg);
        return TRUE;
    }

    if (CONSOLE_IS_IME_ENABLED()) {
        if (pmsg->message == CM_CONSOLE_INPUT_THREAD_MSG) {
            PINPUT_THREAD_INFO pThreadInfo = TlsGetValue(InputThreadTlsIndex);
            MSG msg;

            ASSERT(pThreadInfo);

            if (UnqueueThreadMessage(pThreadInfo->ThreadId, &msg.message, &msg.wParam, &msg.lParam)) {
                RIPMSG3(RIP_WARNING, "DialogHookProc: %04x (%08x, %08x)", msg.message, msg.wParam, msg.lParam);
                switch (msg.message) {
                case CM_CONIME_CREATE:
                    ProcessCreateConsoleIME(&msg, pThreadInfo->ThreadId);
                    return TRUE;
                case CM_WAIT_CONIME_PROCESS:
                    WaitConsoleIMEStuff((HDESK)msg.wParam, (HANDLE)msg.lParam);
                    return TRUE;
                case CM_SET_CONSOLEIME_WINDOW:
                    pThreadInfo->hWndConsoleIME = (HWND)msg.wParam;
                    return TRUE;
                default:
                    RIPMSG1(RIP_WARNING, "DialogHookProc: invalid thread message(%04x) !!", msg.message);
                    break;
                }
            }
            else {
                RIPMSG0(RIP_WARNING, "DialogHookProc: bogus thread message is posted. ignored");
            }
        }
    }

    if (nCode == MSGF_DIALOGBOX) {
        if (pmsg->message >= WM_KEYFIRST &&
            pmsg->message <= WM_KEYLAST) {
            if (pmsg->message != WM_CHAR &&
                pmsg->message != WM_DEADCHAR &&
                pmsg->message != WM_SYSCHAR &&
                pmsg->message != WM_SYSDEADCHAR) {

                // don't store key info if dialog box input
                if (GetWindowLongPtr(pmsg->hwnd, GWLP_HWNDPARENT) == 0) {
                    StoreKeyInfo(pmsg);
                }
            }
        }
    }
    return 0;
}

#undef DbgPrint  // Need this to build on free systems

ULONG InputExceptionFilter(
    PEXCEPTION_POINTERS pexi)
{
    NTSTATUS Status;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInfo;

    if (pexi->ExceptionRecord->ExceptionCode != STATUS_PORT_DISCONNECTED) {
        Status = NtQuerySystemInformation( SystemKernelDebuggerInformation,
                                           &KernelDebuggerInfo,
                                           sizeof(KernelDebuggerInfo),
                                           NULL
                                         );

        if (NT_SUCCESS(Status) && KernelDebuggerInfo.KernelDebuggerEnabled) {
            DbgPrint("Unhandled Exception hit in csrss.exe InputExceptionFilter\n");
            DbgPrint("first, enter !exr %p for the exception record\n", pexi->ExceptionRecord);
            DbgPrint("next, enter !cxr %p for the context\n", pexi->ContextRecord);
            DbgPrint("then !kb to get the faulting stack\n");
            DbgBreakPoint();
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

/////////////////////////////////////////
// Input Thread internal Message Queue:
// Mainly used for Console IME stuff
/////////////////////////////////////////

LIST_ENTRY gInputThreadMsg;
CRITICAL_SECTION gInputThreadMsgLock;

VOID
InitializeThreadMessages()
{
    RtlEnterCriticalSection(&gInputThreadMsgLock);
    InitializeListHead(&gInputThreadMsg);
    RtlLeaveCriticalSection(&gInputThreadMsgLock);
}

VOID
CleanupInputThreadMessages(
    DWORD dwThreadId)
{
    UINT message;
    WPARAM wParam;
    LPARAM lParam;

    ASSERT(dwThreadId);

    while (UnqueueThreadMessage(dwThreadId, &message, &wParam, &lParam)) {
        RIPMSG3(RIP_WARNING, "CleanupInputThreadMessages: %04x (%08x, %08x)", message, wParam, lParam);
    }
}

//
// QueueThreadMessage
//
// Posts a message to Input Thread, specified by dwThreadId.
// CM_CONSOLE_INPUT_THEAD_MSG is used as a stub message. Actual parameters are
// stored in gInputThreadMsg. Input thread should call UnqueueThreadMessage
// when it gets CM_CONSOLE_INPUT_THREAD_MSG.
//
NTSTATUS
QueueThreadMessage(
    DWORD dwThreadId,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PCONSOLE_THREAD_MSG pConMsg;

    // NOTE HIROYAMA: change this to RIP_VERBOSE
    RIPMSG4(RIP_VERBOSE, "QueueThreadMessage: TID=%08x msg:%04x (%08x, %08x)",
            dwThreadId, message, wParam, lParam);

    pConMsg = ConsoleHeapAlloc(MAKE_TAG(TMP_TAG), sizeof *pConMsg);
    if (pConMsg == NULL) {
        RIPMSG0(RIP_WARNING, "QueueThreadMessage: failed to allocate pConMsg");
        return STATUS_NO_MEMORY;
    }

    pConMsg->dwThreadId = dwThreadId;
    pConMsg->Message = message;
    pConMsg->wParam = wParam;
    pConMsg->lParam = lParam;

    RtlEnterCriticalSection(&gInputThreadMsgLock);
    InsertHeadList(&gInputThreadMsg, &pConMsg->ListLink);
    RtlLeaveCriticalSection(&gInputThreadMsgLock);

    if (!PostThreadMessage(dwThreadId, CM_CONSOLE_INPUT_THREAD_MSG, 0, 0)) {
        RIPMSG1(RIP_WARNING, "QueueThreadMessage: failed to post thread msg(%04x)", message);
        RtlEnterCriticalSection(&gInputThreadMsgLock);
        RemoveEntryList(&pConMsg->ListLink);
        RtlLeaveCriticalSection(&gInputThreadMsgLock);
        ConsoleHeapFree(pConMsg);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

//
// UnqueueThreadMessage
//
// return value:
//  TRUE  -- a message found.
//  FALSE -- no message for dwThreadId found.
//
BOOL UnqueueThreadMessage(
    DWORD dwThreadId,
    UINT* pMessage,
    WPARAM* pwParam,
    LPARAM* plParam)
{
    BOOL fResult = FALSE;       // if message is found, set this to TRUE
    PLIST_ENTRY pEntry;

    ASSERT(dwThreadId);

    RtlEnterCriticalSection(&gInputThreadMsgLock);

    //
    // Search for dwThreadId message from the tail of the queue.
    //
    pEntry = gInputThreadMsg.Blink;

    while (pEntry != &gInputThreadMsg) {
        PCONSOLE_THREAD_MSG pConMsg = CONTAINING_RECORD(pEntry, CONSOLE_THREAD_MSG, ListLink);

        if (pConMsg->dwThreadId == dwThreadId) {
            *pMessage = pConMsg->Message;
            *pwParam = pConMsg->wParam;
            *plParam = pConMsg->lParam;

            RemoveEntryList(pEntry);
            ConsoleHeapFree(pConMsg);
            fResult = TRUE;
            break;
        }
        pEntry = pEntry->Blink;
    }

    RtlLeaveCriticalSection(&gInputThreadMsgLock);

    return fResult;
}

VOID
ConsoleInputThread(
    PINPUT_THREAD_INIT_INFO pInputThreadInitInfo)
{
    MSG msg;
    PTEB Teb;
    PCSR_THREAD pcsrt = NULL;
    INPUT_THREAD_INFO ThreadInfo;
    int i;
    HANDLE hThread = NULL;
    HHOOK hhook = NULL;
    BOOL fQuit = FALSE;
    CONSOLEDESKTOPCONSOLETHREAD ConsoleDesktopInfo;
    NTSTATUS Status;

    //
    // Initialize GDI accelerators.
    //

    Teb = NtCurrentTeb();

    try {

        /*
         * Set this thread's desktop to the one we just created/opened.
         * When the very first app is loaded, the desktop hasn't been
         * created yet so the above call might fail.  Make sure we don't
         * accidentally call SetThreadDesktop with a NULL pdesk.  The
         * first app will create the desktop and open it for itself.
         */
        ThreadInfo.Desktop = pInputThreadInitInfo->DesktopHandle;
        ThreadInfo.WindowCount = 0;
        ThreadInfo.ThreadHandle = pInputThreadInitInfo->ThreadHandle;
        ThreadInfo.ThreadId = HandleToUlong(Teb->ClientId.UniqueThread);
#if defined(FE_IME)
        ThreadInfo.hWndConsoleIME = NULL;
#endif
        TlsSetValue(InputThreadTlsIndex, &ThreadInfo);
        ConsoleDesktopInfo.hdesk = pInputThreadInitInfo->DesktopHandle;
        ConsoleDesktopInfo.dwThreadId = HandleToUlong(Teb->ClientId.UniqueThread);
        Status = NtUserConsoleControl(ConsoleDesktopConsoleThread, &ConsoleDesktopInfo,
                sizeof(ConsoleDesktopInfo));
        if (NT_SUCCESS(Status)) {

            //
            // This call forces the client-side desktop information
            // to be updated.
            //

            pcsrt = CsrConnectToUser();
            if (pcsrt == NULL ||
                    !SetThreadDesktop(pInputThreadInitInfo->DesktopHandle)) {
                Status = STATUS_UNSUCCESSFUL;
            } else {

                //
                // Save our thread handle for cleanup purposes
                //

                hThread = pcsrt->ThreadHandle;

                if (!fOneTimeInitialized) {

                    InitializeCustomCP();

                    //
                    // Initialize default screen dimensions.  we have to initialize
                    // the font info here (in the input thread) so that GDI doesn't
                    // get completely confused on process termination (since a
                    // process that looks like it's terminating created all the
                    // server HFONTS).
                    //

                    EnumerateFonts(EF_DEFFACE);

                    InitializeScreenInfo();

                    if (!InitWindowClass())
                        Status = STATUS_UNSUCCESSFUL;

                    for (i=0;i<CONSOLE_MAX_KEY_INFO;i++) {
                        ConsoleKeyInfo[i].hWnd = CONSOLE_FREE_KEY_INFO;
                    }

                    ProgmanHandleMessage = RegisterWindowMessage(TEXT(CONSOLE_PROGMAN_HANDLE_MESSAGE));
                }
            }
        }

        //
        // If we successfully initialized, the input thread is ready to run.
        // Otherwise, kill the thread.
        //

        pInputThreadInitInfo->InitStatus = Status;
        NtSetEvent(pInputThreadInitInfo->InitCompleteEventHandle, NULL);

        if (!NT_SUCCESS(Status))
            RtlRaiseStatus(STATUS_PORT_DISCONNECTED);

        hhook = SetWindowsHookEx(WH_MSGFILTER, DialogHookProc, NULL,
                                HandleToUlong(Teb->ClientId.UniqueThread));

        while (TRUE) {

            //
            // If a WM_QUIT has been received and all windows
            // are gone, get out.
            //

            if (fQuit && ThreadInfo.WindowCount == 0)
                break;

            //
            // Make sure we don't hold any locks while we're idle.
            //

            ASSERT(NtCurrentTeb()->CountOfOwnedCriticalSections == 0);

            GetMessage(&msg, NULL, 0, 0);

            //
            // Trap messages posted to the thread.
            //

            if (msg.message == CM_CREATE_CONSOLE_WINDOW) {
                ProcessCreateConsoleWindow(&msg);
                continue;
            } else if (msg.message == WM_QUIT) {

                //
                // The message was posted from ExitWindows.  This
                // means that it's OK to terminate the thread.
                //

                fQuit = TRUE;

                //
                // Only exit the loop if there are no windows,
                //

                if (ThreadInfo.WindowCount == 0) {
                    break;
                }
                KdPrint(("WM_QUIT received by console with windows\n"));
                continue;
            }
            else if (CONSOLE_IS_IME_ENABLED()) {
                if (msg.message == CM_CONSOLE_INPUT_THREAD_MSG) {
                    MSG msg;

                    if (UnqueueThreadMessage(ThreadInfo.ThreadId, &msg.message, &msg.wParam, &msg.lParam)) {
                        // NOTE HIROYAMA: change this to RIP_VERBOSE
                        RIPMSG3(RIP_VERBOSE, "InputThread: Unqueue: msg=%04x (%08x, %08x)",
                                msg.message, msg.wParam, msg.lParam);
                        switch (msg.message) {
                        case CM_CONIME_CREATE:
                            ProcessCreateConsoleIME(&msg, ThreadInfo.ThreadId);
                            continue;
                        case CM_WAIT_CONIME_PROCESS:
                            WaitConsoleIMEStuff((HDESK)msg.wParam, (HANDLE)msg.lParam);
                            continue;
                        case CM_SET_CONSOLEIME_WINDOW:
                            ThreadInfo.hWndConsoleIME = (HWND)msg.wParam;
                            continue;
                        default:
                            RIPMSG1(RIP_WARNING, "ConsoleInputThread: invalid thread message(%04x) !!", msg.message);
                            break;
                        }
                    }
                    else {
                        RIPMSG0(RIP_WARNING, "ConsoleInputThread: bogus thread message is post. ignored");
                        continue;
                    }
                }
            }

            if (!TranslateMessageEx(&msg, TM_POSTCHARBREAKS)) {
                DispatchMessage(&msg);
            } else {
                // do this so that alt-tab works while journalling
                if (msg.message == WM_SYSKEYDOWN &&
                    msg.wParam == VK_TAB &&
                    (msg.lParam & 0x20000000) ) {   // alt is really down
                    DispatchMessage(&msg);
                } else {
                    StoreKeyInfo(&msg);
                }
            }
        }

        //
        // Cleanup the input thread messages
        //
        CleanupInputThreadMessages(ThreadInfo.ThreadId);

        RtlRaiseStatus(STATUS_PORT_DISCONNECTED);

    } except (InputExceptionFilter(GetExceptionInformation())) {
        BOOL fSuccess;

        //
        // Free all resources used by this thread
        //

        if (hhook != NULL)
            UnhookWindowsHookEx(hhook);
        ConsoleDesktopInfo.dwThreadId = 0;
        NtUserConsoleControl(ConsoleDesktopConsoleThread,
                &ConsoleDesktopInfo, sizeof(ConsoleDesktopInfo));

        //
        // Close the desktop handle.  CSR is special cased to close
        // the handle even if the thread has windows.  The desktop
        // remains assigned to the thread.
        //

        fSuccess = CloseDesktop(ThreadInfo.Desktop);
        ASSERT(fSuccess);

        //
        // Restore thread handle so that CSR won't get confused.
        //

        if (hThread != NULL)
            pcsrt->ThreadHandle = hThread;
    }

    if (pcsrt != NULL)
        CsrDereferenceThread(pcsrt);
    UserExitWorkerThread();
}

ULONG
GetControlKeyState(
    LPARAM lParam
    )
{
    ULONG ControlKeyState=0;

    if (GetKeyState(VK_LMENU) & KEY_PRESSED) {
        ControlKeyState |= LEFT_ALT_PRESSED;
    }
    if (GetKeyState(VK_RMENU) & KEY_PRESSED) {
        ControlKeyState |= RIGHT_ALT_PRESSED;
    }
    if (GetKeyState(VK_LCONTROL) & KEY_PRESSED) {
        ControlKeyState |= LEFT_CTRL_PRESSED;
    }
    if (GetKeyState(VK_RCONTROL) & KEY_PRESSED) {
        ControlKeyState |= RIGHT_CTRL_PRESSED;
    }
    if (GetKeyState(VK_SHIFT) & KEY_PRESSED) {
        ControlKeyState |= SHIFT_PRESSED;
    }
    if (GetKeyState(VK_NUMLOCK) & KEY_TOGGLED) {
        ControlKeyState |= NUMLOCK_ON;
    }
    if (GetKeyState(VK_OEM_SCROLL) & KEY_TOGGLED) {
        ControlKeyState |= SCROLLLOCK_ON;
    }
    if (GetKeyState(VK_CAPITAL) & KEY_TOGGLED) {
        ControlKeyState |= CAPSLOCK_ON;
    }
    if (lParam & KEY_ENHANCED) {
        ControlKeyState |= ENHANCED_KEY;
    }
    ControlKeyState |= (lParam & ALTNUMPAD_BIT);
    return ControlKeyState;
}

ULONG
ConvertMouseButtonState(
    IN ULONG Flag,
    IN ULONG State
    )
{
    if (State & MK_LBUTTON) {
        Flag |= FROM_LEFT_1ST_BUTTON_PRESSED;
    }
    if (State & MK_MBUTTON) {
        Flag |= FROM_LEFT_2ND_BUTTON_PRESSED;
    }
    if (State & MK_RBUTTON) {
        Flag |= RIGHTMOST_BUTTON_PRESSED;
    }
    return Flag;
}

VOID
TerminateRead(
    IN PCONSOLE_INFORMATION Console,
    IN PINPUT_INFORMATION InputInfo,
    IN DWORD Flag
    )

/*++

Routine Description:

    This routine wakes up any readers waiting for data when a ctrl-c
    or ctrl-break is input.

Arguments:

    InputInfo - pointer to input buffer

    Flag - flag indicating whether ctrl-break or ctrl-c was input

--*/

{
    BOOLEAN WaitSatisfied;
    WaitSatisfied = CsrNotifyWait(&InputInfo->ReadWaitQueue,
                  TRUE,
                  NULL,
                  (PVOID)Flag
                 );
    if (WaitSatisfied) {
        // #334370 under stress, WaitQueue may already hold the satisfied waits
        ASSERT ((Console->WaitQueue == NULL) ||
                (Console->WaitQueue == &InputInfo->ReadWaitQueue));
        Console->WaitQueue = &InputInfo->ReadWaitQueue;
    }
}

BOOL
HandleSysKeyEvent(
    IN PCONSOLE_INFORMATION Console,
    IN HWND hWnd,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*

    returns TRUE if DefWindowProc should be called.

*/

{
    WORD VirtualKeyCode;
    BOOL bCtrlDown;

#if defined (FE_IME)
// Sep.16.1995 Support Console IME by v-HirShi(Hirotoshi Shimizu)
    if (Message == WM_SYSCHAR || Message == WM_SYSDEADCHAR ||
        Message == WM_SYSCHAR+CONIME_KEYDATA || Message == WM_SYSDEADCHAR+CONIME_KEYDATA)
#else
    if (Message == WM_SYSCHAR || Message == WM_SYSDEADCHAR)
#endif
    {
        VirtualKeyCode = (WORD)MapVirtualKey(LOBYTE(HIWORD(lParam)), 1);
    } else {
        VirtualKeyCode = LOWORD(wParam);
    }

    //
    // check for ctrl-esc
    //
    bCtrlDown = GetKeyState(VK_CONTROL) & KEY_PRESSED;

    if (VirtualKeyCode == VK_ESCAPE &&
        bCtrlDown &&
        !(GetKeyState(VK_MENU) & KEY_PRESSED) &&
        !(GetKeyState(VK_SHIFT) & KEY_PRESSED) &&
        !(Console->ReserveKeys & CONSOLE_CTRLESC) ) {
        return TRUE;    // call DefWindowProc
    }

    if ((lParam & 0x20000000) == 0) {   // we're iconic


        //
        // Check for ENTER while ICONic (Restore accelerator)
        //

        if (VirtualKeyCode == VK_RETURN && !(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE)) {

            return TRUE;    // call DefWindowProc
        } else {
            HandleKeyEvent(Console,
                           hWnd,
                           Message,
                           wParam,
                           lParam
                          );
            return FALSE;
        }
    }

    if (VirtualKeyCode == VK_RETURN && !bCtrlDown &&
            !(Console->ReserveKeys & CONSOLE_ALTENTER)) {
#ifdef i386
        if (!(Message & KEY_UP_TRANSITION)) {
            if (FullScreenInitialized) {
                if (Console->FullScreenFlags == 0) {
                    ConvertToFullScreen(Console);
                    Console->FullScreenFlags = CONSOLE_FULLSCREEN;

                    ChangeDispSettings(Console, Console->hWnd,CDS_FULLSCREEN);
                } else {
                    ConvertToWindowed(Console);
                    Console->FullScreenFlags &= ~CONSOLE_FULLSCREEN;

                    ChangeDispSettings(Console, Console->hWnd,0);

                    ShowWindow(Console->hWnd, SW_RESTORE);
                }
            } else {
                WCHAR ItemString[70];
                LoadString(ghInstance,msgNoFullScreen,ItemString,70);
                MessageBoxEx(Console->hWnd,
                            ItemString,
                            Console->Title,
                            MB_SYSTEMMODAL | MB_OK,
                            0L
                           );
            }
        }
#endif
        return FALSE;
    }

    //
    // make sure alt-space gets translated so that the system
    // menu is displayed.
    //

    if (!(GetKeyState(VK_CONTROL) & KEY_PRESSED)) {
        if (VirtualKeyCode == VK_SPACE && !(Console->ReserveKeys & CONSOLE_ALTSPACE)) {
            return TRUE; // call DefWindowProc
        }

        if (VirtualKeyCode == VK_ESCAPE && !(Console->ReserveKeys & CONSOLE_ALTESC)) {
            return TRUE;  // call DefWindowProc
        }
        if (VirtualKeyCode == VK_TAB && !(Console->ReserveKeys & CONSOLE_ALTTAB)) {
            return TRUE;  // call DefWindowProc
        }
    }
    HandleKeyEvent(Console,
                   hWnd,
                   Message,
                   wParam,
                   lParam
                  );
    return FALSE;
}

VOID
HandleKeyEvent(
    IN PCONSOLE_INFORMATION Console,
    IN HWND hWnd,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    INPUT_RECORD InputEvent;
    BOOLEAN ContinueProcessing;
    ULONG EventsWritten;
    WORD VirtualKeyCode;
    ULONG ControlKeyState;
    BOOL bKeyDown;
    BOOL bGenerateBreak=FALSE;
#ifdef FE_SB
    BOOL KeyMessageFromConsoleIME;
#endif

#ifdef FE_SB
    // v-HirShi Sep.21.1995 For Console IME
    if ((WM_KEYFIRST+CONIME_KEYDATA) <= Message && Message <= (WM_KEYLAST+CONIME_KEYDATA)) {
        Message -= CONIME_KEYDATA ;
        KeyMessageFromConsoleIME = TRUE ;
    }
    else {
        KeyMessageFromConsoleIME = FALSE ;
    }
#endif
    /*
     * BOGUS for WM_CHAR/WM_DEADCHAR, in which LOWORD(lParam) is a character
     */
    VirtualKeyCode = LOWORD(wParam);
    ControlKeyState = GetControlKeyState(lParam);
    bKeyDown = !(lParam & KEY_TRANSITION_UP);

    //
    // Make sure we retrieve the key info first, or we could chew up
    // unneeded space in the key info table if we bail out early.
    //

    InputEvent.Event.KeyEvent.wVirtualKeyCode = VirtualKeyCode;
    InputEvent.Event.KeyEvent.wVirtualScanCode = (BYTE)(HIWORD(lParam));
    if (Message == WM_CHAR || Message == WM_SYSCHAR ||
        Message == WM_DEADCHAR || Message == WM_SYSDEADCHAR) {
        RetrieveKeyInfo(hWnd,
                        &InputEvent.Event.KeyEvent.wVirtualKeyCode,
                        &InputEvent.Event.KeyEvent.wVirtualScanCode,
                        !(Console->InputBuffer.ImeMode.Open ^ KeyMessageFromConsoleIME));
        VirtualKeyCode = InputEvent.Event.KeyEvent.wVirtualKeyCode;
    }

    //
    // If this is a key up message, should we ignore it? We do this
    // so that if a process reads a line from the input buffer, the
    // key up event won't get put in the buffer after the read
    // completes.
    //

    if (Console->Flags & CONSOLE_IGNORE_NEXT_KEYUP) {
        Console->Flags &= ~CONSOLE_IGNORE_NEXT_KEYUP;
        if (!bKeyDown)
            return;
    }

#ifdef FE_SB
    // v-HirShi Sep.21.1995 For Console IME
    if (KeyMessageFromConsoleIME) {
        goto FromConsoleIME ;
    }
#endif

    if (Console->Flags & CONSOLE_SELECTING) {

        if (!bKeyDown) {
            return;
        }

        //
        // if escape or ctrl-c, cancel selection
        //

        if (!(Console->SelectionFlags & CONSOLE_MOUSE_DOWN) ) {
            if (VirtualKeyCode == VK_ESCAPE ||
                (VirtualKeyCode == 'C' &&
                 (GetKeyState(VK_CONTROL) & KEY_PRESSED) )) {
                ClearSelection(Console);
                return;
            } else if (VirtualKeyCode == VK_RETURN) {

                // if return, copy selection

                DoCopy(Console);
                return;
            }
        }
        if (!(Console->SelectionFlags & CONSOLE_MOUSE_SELECTION)) {
            if ((Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) &&
                (VirtualKeyCode == VK_RIGHT ||
                 VirtualKeyCode == VK_LEFT ||
                 VirtualKeyCode == VK_UP ||
                 VirtualKeyCode == VK_DOWN ||
                 VirtualKeyCode == VK_NEXT ||
                 VirtualKeyCode == VK_PRIOR ||
                 VirtualKeyCode == VK_END ||
                 VirtualKeyCode == VK_HOME
                ) ) {
                PSCREEN_INFORMATION ScreenInfo;
#ifdef FE_SB
                SHORT RowIndex;
                PROW Row;
                BYTE KAttrs;
                SHORT NextRightX;
                SHORT NextLeftX;
#endif

                ScreenInfo = Console->CurrentScreenBuffer;

                //
                // see if shift is down.  if so, we're extending
                // the selection.  otherwise, we're resetting the
                // anchor
                //

                ConsoleHideCursor(ScreenInfo);
#ifdef FE_SB
                RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y) % ScreenInfo->ScreenBufferSize.Y;
                Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];

               if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
               {
                    KAttrs = Row->CharRow.KAttrs[ScreenInfo->BufferInfo.TextInfo.CursorPosition.X];
                    if (KAttrs & ATTR_LEADING_BYTE)
                        NextRightX = 2;
                    else
                        NextRightX = 1;
                }
                else
                {
                    NextRightX = 1;
                }
                if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.X > 0) {
                    if (CONSOLE_IS_DBCS_OUTPUTCP(Console)) {
                        KAttrs = Row->CharRow.KAttrs[ScreenInfo->BufferInfo.TextInfo.CursorPosition.X-1];
                        if (KAttrs & ATTR_TRAILING_BYTE)
                            NextLeftX = 2;
                        else if (KAttrs & ATTR_LEADING_BYTE) {
                            if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.X-1 > 0) {
                                KAttrs = Row->CharRow.KAttrs[ScreenInfo->BufferInfo.TextInfo.CursorPosition.X-2];
                                if (KAttrs & ATTR_TRAILING_BYTE)
                                    NextLeftX = 3;
                                else
                                    NextLeftX = 2;
                            }
                            else
                                NextLeftX = 1;
                        }
                        else
                            NextLeftX = 1;
                    }
                    else
                        NextLeftX = 1;
                }

                switch (VirtualKeyCode) {
                    case VK_RIGHT:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.X+NextRightX < ScreenInfo->ScreenBufferSize.X) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.X+=NextRightX;
                        }
                        break;
                    case VK_LEFT:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.X > 0) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.X-=NextLeftX;
                        }
                        break;
                    case VK_UP:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y > 0) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y-=1;
                        }
                        break;
                    case VK_DOWN:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+1 < ScreenInfo->ScreenBufferSize.Y) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+=1;
                        }
                        break;
                    case VK_NEXT:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y += CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1;
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y >= ScreenInfo->ScreenBufferSize.Y) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = ScreenInfo->ScreenBufferSize.Y-1;
                        }
                        break;
                    case VK_PRIOR:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y -= CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1;
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y < 0) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = 0;
                        }
                        break;
                    case VK_END:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = ScreenInfo->ScreenBufferSize.Y-CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
                        break;
                    case VK_HOME:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.X = 0;
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = 0;
                        break;
                    default:
                        ASSERT(FALSE);
                }
#else   // FE_SB
                switch (VirtualKeyCode) {
                    case VK_RIGHT:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.X+1 < ScreenInfo->ScreenBufferSize.X) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.X+=1;
                        }
                        break;
                    case VK_LEFT:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.X > 0) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.X-=1;
                        }
                        break;
                    case VK_UP:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y > 0) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y-=1;
                        }
                        break;
                    case VK_DOWN:
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+1 < ScreenInfo->ScreenBufferSize.Y) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+=1;
                        }
                        break;
                    case VK_NEXT:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y += CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1;
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y >= ScreenInfo->ScreenBufferSize.Y) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = ScreenInfo->ScreenBufferSize.Y-1;
                        }
                        break;
                    case VK_PRIOR:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y -= CONSOLE_WINDOW_SIZE_Y(ScreenInfo)-1;
                        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y < 0) {
                            ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = 0;
                        }
                        break;
                    case VK_END:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = ScreenInfo->ScreenBufferSize.Y-CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
                        break;
                    case VK_HOME:
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.X = 0;
                        ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y = 0;
                        break;
                    default:
                        ASSERT(FALSE);
                }
#endif  // FE_SB
                ConsoleShowCursor(ScreenInfo);
                if (GetKeyState(VK_SHIFT) & KEY_PRESSED) {
                    {
                        ExtendSelection(Console,ScreenInfo->BufferInfo.TextInfo.CursorPosition);
                    }
                } else {
                    if (Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) {
                        MyInvert(Console,&Console->SelectionRect);
                        Console->SelectionFlags &= ~CONSOLE_SELECTION_NOT_EMPTY;
                        ConsoleShowCursor(ScreenInfo);
                    }
                    Console->SelectionAnchor = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
                    MakeCursorVisible(ScreenInfo,Console->SelectionAnchor);
                    Console->SelectionRect.Left = Console->SelectionRect.Right = Console->SelectionAnchor.X;
                    Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
                }
                return;
            }
        } else if (!(Console->SelectionFlags & CONSOLE_MOUSE_DOWN)) {

            //
            // if in mouse selection mode and user hits a key, cancel selection
            //

            if (!IsSystemKey(VirtualKeyCode)) {
                ClearSelection(Console);
            }
        }
    } else if (Console->Flags & CONSOLE_SCROLLING) {

        if (!bKeyDown) {
            return;
        }

        //
        // if escape, enter or ctrl-c, cancel scroll
        //

        if (VirtualKeyCode == VK_ESCAPE ||
            VirtualKeyCode == VK_RETURN ||
            (VirtualKeyCode == 'C' &&
             (GetKeyState(VK_CONTROL) & KEY_PRESSED) )) {
            ClearScroll(Console);
        } else {
            WORD ScrollCommand;
            BOOL Horizontal=FALSE;
            switch (VirtualKeyCode) {
                case VK_UP:
                    ScrollCommand = SB_LINEUP;
                    break;
                case VK_DOWN:
                    ScrollCommand = SB_LINEDOWN;
                    break;
                case VK_LEFT:
                    ScrollCommand = SB_LINEUP;
                    Horizontal=TRUE;
                    break;
                case VK_RIGHT:
                    ScrollCommand = SB_LINEDOWN;
                    Horizontal=TRUE;
                    break;
                case VK_NEXT:
                    ScrollCommand = SB_PAGEDOWN;
                    break;
                case VK_PRIOR:
                    ScrollCommand = SB_PAGEUP;
                    break;
                case VK_END:
                    ScrollCommand = SB_PAGEDOWN;
                    Horizontal=TRUE;
                    break;
                case VK_HOME:
                    ScrollCommand = SB_PAGEUP;
                    Horizontal=TRUE;
                    break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                    return;
                default:
                    Beep(800, 200);
                    return;
            }
            if (Horizontal)
                HorizontalScroll(Console->CurrentScreenBuffer, ScrollCommand, 0);
            else
                VerticalScroll(Console, Console->CurrentScreenBuffer,ScrollCommand,0);
        }
        return;
    }

    //
    // if the user is inputting chars at an inappropriate time, beep.
    //

    if ((Console->Flags & (CONSOLE_SELECTING | CONSOLE_SCROLLING | CONSOLE_SCROLLBAR_TRACKING)) &&
        bKeyDown &&
        !IsSystemKey(VirtualKeyCode)) {
        Beep(800, 200);
        return;
    }

    //
    // if in fullscreen mode, process PrintScreen
    //

#ifdef LATER
//
// Changed this code to get commas to work (build 485).
//
// Therese, the problem is that WM_CHAR/WM_SYSCHAR messages come through
// here - in this case, LOWORD(wParam) is a character value and not a virtual
// key. It happens that VK_SNAPSHOT == 0x2c, and the character value for a
// comma is also == 0x2c, so execution enters this conditional when a comma
// is hit. Commas aren't coming out because of the newly entered return
// statement.
//
// HandleKeyEvent() is making many virtual key comparisons - need to make
// sure that for each one, there is either no corresponding character value,
// or that you check before you compare so that you are comparing two values
// that have the same data type.
//
// I added the message comparison so that we know we're checking virtual
// keys against virtual keys and not characters.
//
// - scottlu
//

#endif

    if (Message != WM_CHAR && Message != WM_SYSCHAR &&
        VirtualKeyCode == VK_SNAPSHOT &&
        !(Console->ReserveKeys & (CONSOLE_ALTPRTSC | CONSOLE_PRTSC )) ) {
        if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
            Console->SelectionFlags |= CONSOLE_SELECTION_NOT_EMPTY;
            Console->SelectionRect = Console->CurrentScreenBuffer->Window;
            StoreSelection(Console);
            Console->SelectionFlags &= ~CONSOLE_SELECTION_NOT_EMPTY;
        }
        return;
    }

    //
    // IME stuff
    //
    if (!(Console->Flags & CONSOLE_VDM_REGISTERED)) {
        LPARAM lParamForHotKey ;
        DWORD HotkeyID ;
        lParamForHotKey = lParam ;

        HotkeyID = NtUserCheckImeHotKey( (VirtualKeyCode & 0x00ff),lParamForHotKey) ;
        //
        // If it's direct KL switching hokey, handle it here
        // regardless the system is IME enabled or not.
        //
        if (HotkeyID >= IME_HOTKEY_DSWITCH_FIRST && HotkeyID <= IME_HOTKEY_DSWITCH_LAST) {
            UINT uModifier, uVkey;
            HKL hkl;

            RIPMSG1(RIP_VERBOSE, "HandleKeyEvent: handling IME HOTKEY id=%x", HotkeyID);
            if (NtUserGetImeHotKey(HotkeyID, &uModifier, &uVkey, &hkl) && hkl != NULL) {
                BYTE bCharSetSys = CodePageToCharSet(GetACP());
                WPARAM wpSysChar = 0;
                CHARSETINFO cs;

                if (TranslateCharsetInfo((LPDWORD)LOWORD(hkl), &cs, TCI_SRCLOCALE)) {
                    if (bCharSetSys == cs.ciCharset) {
                        wpSysChar = INPUTLANGCHANGE_SYSCHARSET;
                    }
                }
                PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, wpSysChar, (LPARAM)hkl);
            }
            return;
        }

        if (!(Console->InputBuffer.ImeMode.Disable) && CONSOLE_IS_IME_ENABLED()) {

            if (HotkeyID != IME_INVALID_HOTKEY) {
                switch(HotkeyID) {
                case IME_JHOTKEY_CLOSE_OPEN:
                    {
                        BOOL fOpen = Console->InputBuffer.ImeMode.Open;
                        if (!bKeyDown)
                            break ;

                        Console->InputBuffer.ImeMode.Open = !fOpen ;
                        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                              CONIME_HOTKEY,
                                              (WPARAM)Console->ConsoleHandle,
                                              HotkeyID))) {
                            break;
                        }

                        // Update in the system conversion mode buffer.
                        GetImeKeyState(Console, NULL);

                        break ;
                    }
                case IME_CHOTKEY_IME_NONIME_TOGGLE:
                case IME_THOTKEY_IME_NONIME_TOGGLE:
                case IME_CHOTKEY_SHAPE_TOGGLE:
                case IME_THOTKEY_SHAPE_TOGGLE:
                case IME_CHOTKEY_SYMBOL_TOGGLE:
                case IME_THOTKEY_SYMBOL_TOGGLE:
                case IME_KHOTKEY_SHAPE_TOGGLE:
                case IME_KHOTKEY_HANJACONVERT:
                case IME_KHOTKEY_ENGLISH:
                case IME_ITHOTKEY_RESEND_RESULTSTR:
                case IME_ITHOTKEY_PREVIOUS_COMPOSITION:
                case IME_ITHOTKEY_UISTYLE_TOGGLE:
                default:
                    {
                        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                              CONIME_HOTKEY,
                                              (WPARAM)Console->ConsoleHandle,
                                              HotkeyID))) {
                            break;
                        }

                        // Update in the system conversion mode buffer.
                        GetImeKeyState(Console, NULL);

                        break ;
                    }
                }
                return ;
            }

            if ( CTRL_BUT_NOT_ALT(ControlKeyState) &&
                    (bKeyDown) ) {
                if (VirtualKeyCode == 'C' &&
                        Console->InputBuffer.InputMode & ENABLE_PROCESSED_INPUT) {
                    goto FromConsoleIME ;
                }
                else if (VirtualKeyCode == VK_CANCEL) {
                    goto FromConsoleIME ;
                }
                else if (VirtualKeyCode == 'S'){
                    goto FromConsoleIME ;
                }
            }
            else if (VirtualKeyCode == VK_PAUSE ){
                goto FromConsoleIME ;
            }
            else if ( ((VirtualKeyCode == VK_SHIFT)   ||
                       (VirtualKeyCode == VK_CONTROL) ||
                       (VirtualKeyCode == VK_CAPITAL) ||
                       (VirtualKeyCode == VK_KANA)    ||    // VK_KANA == VK_HANGUL
                       (VirtualKeyCode == VK_JUNJA)   ||
                       (VirtualKeyCode == VK_HANJA)   ||
                       (VirtualKeyCode == VK_NUMLOCK) ||
                       (VirtualKeyCode == VK_SCROLL)     )
                      &&
                      !(Console->InputBuffer.ImeMode.Unavailable) &&
                      !(Console->InputBuffer.ImeMode.Open)
                    )
            {
                if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                      Message+CONIME_KEYDATA,
                                      (WPARAM)LOWORD(wParam)<<16|LOWORD(VirtualKeyCode),
                                      lParam
                                     ))) {
                    return;
                }
                goto FromConsoleIME ;
            }

            if (!Console->InputBuffer.ImeMode.Unavailable && Console->InputBuffer.ImeMode.Open) {
                if (! (HIWORD(lParam) & KF_REPEAT))
                {
                    if (PRIMARYLANGID(LOWORD(Console->hklActive)) == LANG_JAPANESE &&
                            (BYTE)wParam == VK_KANA) {
                        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                                              CONIME_NOTIFY_VK_KANA,
                                              0,
                                              0
                                             ))) {
                            return;
                        }
                    }
                }

                ConsoleImeMessagePump(Console,
                                      Message+CONIME_KEYDATA,
                                      LOWORD(wParam)<<16|LOWORD(VirtualKeyCode),
                                      lParam
                                     );
                return ;
            }
        }
    }
FromConsoleIME:

    //
    // ignore key strokes that will generate CHAR messages.  this is only
    // necessary while a dialog box is up.
    //

    if (DialogBoxCount > 0) {
        if (Message != WM_CHAR && Message != WM_SYSCHAR && Message != WM_DEADCHAR && Message != WM_SYSDEADCHAR) {
            WCHAR awch[MAX_CHARS_FROM_1_KEYSTROKE];
            int cwch;
            BYTE KeyState[256];

            GetKeyboardState(KeyState);
            cwch = ToUnicodeEx((UINT)wParam,HIWORD(lParam),KeyState,awch,
                               MAX_CHARS_FROM_1_KEYSTROKE,
                               //TM_POSTCHARBREAKS | (KeyState(VK_MENU) & 1));
                               TM_POSTCHARBREAKS,
                               (HKL)NULL);
            if (cwch != 0) {
                return;
            }
        } else {
            // remember to generate break
            if (Message == WM_CHAR) {
                bGenerateBreak=TRUE;
            }
        }
    }

#ifdef FE_IME
    // ignore key stroke while IME property is up.
    if (Console->InputBuffer.hWndConsoleIME)
        return;
#endif

    InputEvent.EventType = KEY_EVENT;
    InputEvent.Event.KeyEvent.bKeyDown = bKeyDown;
    InputEvent.Event.KeyEvent.wRepeatCount = LOWORD(lParam);

    if (Message == WM_CHAR || Message == WM_SYSCHAR || Message == WM_DEADCHAR || Message == WM_SYSDEADCHAR) {
        // If this is a fake character, zero the scancode.
        if (lParam & 0x02000000) {
            InputEvent.Event.KeyEvent.wVirtualScanCode = 0;
        }
        InputEvent.Event.KeyEvent.dwControlKeyState = GetControlKeyState(lParam);
        if (Message == WM_CHAR || Message == WM_SYSCHAR) {
            InputEvent.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)wParam;
        } else {
            InputEvent.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)0;
        }
    } else {
        // if alt-gr, ignore
        if (lParam & 0x02000000) {
            return;
        }
        InputEvent.Event.KeyEvent.dwControlKeyState = ControlKeyState;
        InputEvent.Event.KeyEvent.uChar.UnicodeChar = 0;
    }

#ifdef FE_IME
    if (CONSOLE_IS_IME_ENABLED()) {
        // MSKK August.22.1993 KazuM
        DWORD dwConversion;

        if (!NT_SUCCESS(GetImeKeyState(Console, &dwConversion))) {
            return;
        }

        InputEvent.Event.KeyEvent.dwControlKeyState |= ImmConversionToConsole(dwConversion);
    }
#endif

    ContinueProcessing=TRUE;

    if (CTRL_BUT_NOT_ALT(InputEvent.Event.KeyEvent.dwControlKeyState) &&
        InputEvent.Event.KeyEvent.bKeyDown) {

        //
        // check for ctrl-c, if in line input mode.
        //

        if (InputEvent.Event.KeyEvent.wVirtualKeyCode == 'C' &&
            Console->InputBuffer.InputMode & ENABLE_PROCESSED_INPUT) {
            HandleCtrlEvent(Console,CTRL_C_EVENT);
            if (!Console->PopupCount)
                TerminateRead(Console,&Console->InputBuffer,CONSOLE_CTRL_C_SEEN);
            if (!(Console->Flags & CONSOLE_SUSPENDED)) {
                ContinueProcessing=FALSE;
            }
        }

        //
        // check for ctrl-break.
        //

        else if (InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL) {
            FlushInputBuffer(&Console->InputBuffer);
            HandleCtrlEvent(Console,CTRL_BREAK_EVENT);
            if (!Console->PopupCount)
                TerminateRead(Console,&Console->InputBuffer,CONSOLE_CTRL_BREAK_SEEN);
            if (!(Console->Flags & CONSOLE_SUSPENDED)) {
                ContinueProcessing=FALSE;
            }
        }

        //
        // don't write ctrl-esc to the input buffer
        //

        else if (InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE &&
                 !(Console->ReserveKeys & CONSOLE_CTRLESC)) {
            ContinueProcessing=FALSE;
        }
    } else if (InputEvent.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED) &&
               InputEvent.Event.KeyEvent.bKeyDown &&
               InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE &&
               !(Console->ReserveKeys & CONSOLE_ALTESC)) {
        ContinueProcessing=FALSE;
    }

    if (ContinueProcessing) {
        EventsWritten = WriteInputBuffer( Console,
                                          &Console->InputBuffer,
                                          &InputEvent,
                                          1
                                         );
        if (EventsWritten && bGenerateBreak) {
            InputEvent.Event.KeyEvent.bKeyDown = FALSE;
            WriteInputBuffer( Console,
                              &Console->InputBuffer,
                              &InputEvent,
                              1
                             );
        }
    }
    return;
}

BOOL
HandleMouseEvent(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*

    returns TRUE if DefWindowProc should be called.

*/

{
    ULONG ButtonFlags,EventFlags;
    INPUT_RECORD InputEvent;
    ULONG EventsWritten;
    COORD MousePosition;
    SHORT RowIndex;
    PROW Row;

    if (!(Console->Flags & CONSOLE_HAS_FOCUS) &&
        !(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) &&
        !(Console->SelectionFlags & CONSOLE_MOUSE_DOWN)) {
        return TRUE;
    }

    if (Console->Flags & CONSOLE_IGNORE_NEXT_MOUSE_INPUT) {
        // only reset on up transition
        if (Message != WM_LBUTTONDOWN &&
            Message != WM_MBUTTONDOWN &&
            Message != WM_RBUTTONDOWN) {
            Console->Flags &= ~CONSOLE_IGNORE_NEXT_MOUSE_INPUT;
            return FALSE;
        }
        return TRUE;
    }

    //
    // translate mouse position into characters, if necessary.
    //

    MousePosition.X = LOWORD(lParam);
    MousePosition.Y = HIWORD(lParam);
    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        MousePosition.X /= SCR_FONTSIZE(ScreenInfo).X;
        MousePosition.Y /= SCR_FONTSIZE(ScreenInfo).Y;
    }
    MousePosition.X += ScreenInfo->Window.Left;
    MousePosition.Y += ScreenInfo->Window.Top;

    //
    // make sure mouse position is clipped to screen buffer
    //

    if (MousePosition.X < 0) {
        MousePosition.X = 0;
    } else if (MousePosition.X >= ScreenInfo->ScreenBufferSize.X) {
        MousePosition.X = ScreenInfo->ScreenBufferSize.X-1;
    }
    if (MousePosition.Y < 0) {
        MousePosition.Y = 0;
    } else if (MousePosition.Y >= ScreenInfo->ScreenBufferSize.Y) {
        MousePosition.Y = ScreenInfo->ScreenBufferSize.Y-1;
    }

    if (Console->Flags & CONSOLE_SELECTING ||
        ((Console->Flags & CONSOLE_QUICK_EDIT_MODE) &&
         (Console->FullScreenFlags == 0)) ) {
        if (Message == WM_LBUTTONDOWN) {

            //
            // make sure message matches button state
            //

            if (!(GetKeyState(VK_LBUTTON) & KEY_PRESSED)) {
                return FALSE;
            }

            if (Console->Flags & CONSOLE_QUICK_EDIT_MODE &&
                !(Console->Flags & CONSOLE_SELECTING)) {
                Console->Flags |= CONSOLE_SELECTING;
                Console->SelectionFlags = CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN | CONSOLE_SELECTION_NOT_EMPTY;

                //
                // invert selection
                //

                Console->SelectionAnchor = MousePosition;
                Console->SelectionRect.Left =Console->SelectionRect.Right = Console->SelectionAnchor.X;
                Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
                MyInvert(Console,&Console->SelectionRect);
                SetWinText(Console,msgSelectMode,TRUE);
                SetCapture(Console->hWnd);
            } else {

            //
            // We now capture the mouse to our Window. We do this so that the user can
            //  "scroll" the selection endpoint to an off screen position by moving the
            //  mouse off the client area.
            //

            if (Console->SelectionFlags & CONSOLE_MOUSE_SELECTION) {

                //
                // Check for SHIFT-Mouse Down "continue previous selection" command
                //

                if (GetKeyState(VK_SHIFT) & KEY_PRESSED) {
                    Console->SelectionFlags |= CONSOLE_MOUSE_DOWN; // BUGBUG necessary flag?
                    SetCapture(Console->hWnd);
                    ExtendSelection(Console,
                                    MousePosition
                                   );
                } else {

                    //
                    // invert old selection, reset anchor, and invert
                    // new selection.
                    //

                    MyInvert(Console,&Console->SelectionRect);
                    Console->SelectionFlags |= CONSOLE_MOUSE_DOWN; // BUGBUG necessary flag?
                    SetCapture(Console->hWnd);
                    Console->SelectionAnchor = MousePosition;
                    Console->SelectionRect.Left =Console->SelectionRect.Right = Console->SelectionAnchor.X;
                    Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
                    MyInvert(Console,&Console->SelectionRect);
                }
            } else {
                ConvertToMouseSelect(Console,
                                     MousePosition
                                    );
            }
            }
        } else if (Message == WM_LBUTTONUP) {
            if (Console->SelectionFlags & CONSOLE_MOUSE_SELECTION) {
                Console->SelectionFlags &= ~CONSOLE_MOUSE_DOWN;
                ReleaseCapture();
            }
        } else if (Message == WM_LBUTTONDBLCLK) {
            if ((MousePosition.X == Console->SelectionAnchor.X) &&
                (MousePosition.Y == Console->SelectionAnchor.Y)) {
                RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+MousePosition.Y) % ScreenInfo->ScreenBufferSize.Y;
                Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
                while (Console->SelectionAnchor.X > 0) {
                    if (IS_WORD_DELIM(Row->CharRow.Chars[Console->SelectionAnchor.X - 1]))
                        break;
                    Console->SelectionAnchor.X--;
                }
                while (MousePosition.X < ScreenInfo->ScreenBufferSize.X) {
                    if (IS_WORD_DELIM(Row->CharRow.Chars[MousePosition.X]))
                        break;
                    MousePosition.X++;
                }
                if (gfTrimLeadingZeros) {
                    //
                    // Trim the leading zeros: 000fe12 -> fe12
                    // Usefull for debugging
                    //
                    if (MousePosition.X > Console->SelectionAnchor.X + 2 &&
                            Row->CharRow.Chars[Console->SelectionAnchor.X + 1] != L'x' &&
                            Row->CharRow.Chars[Console->SelectionAnchor.X + 1] != L'X') {
                        // Don't touch the selection begins with 0x
                        while (Row->CharRow.Chars[Console->SelectionAnchor.X] == L'0' && Console->SelectionAnchor.X < MousePosition.X - 1) {
                            Console->SelectionAnchor.X++;
                        }
                    }
                }
                ExtendSelection(Console, MousePosition);
            }
        } else if ((Message == WM_RBUTTONDOWN) || (Message == WM_RBUTTONDBLCLK)) {
            if (!(Console->SelectionFlags & CONSOLE_MOUSE_DOWN)) {
                if (Console->Flags & CONSOLE_SELECTING) {
                    DoCopy(Console);
                } else if (Console->Flags & CONSOLE_QUICK_EDIT_MODE) {
                    DoPaste(Console);
                }
                Console->Flags |= CONSOLE_IGNORE_NEXT_MOUSE_INPUT;
            }
        } else if (Message == WM_MOUSEMOVE) {
            if (Console->SelectionFlags & CONSOLE_MOUSE_DOWN) {
                ExtendSelection(Console,
                                MousePosition
                               );
            }
        } else if (Message == WM_MOUSEWHEEL) {
            return TRUE;
        }
        return FALSE;
    }

    if (!(Console->InputBuffer.InputMode & ENABLE_MOUSE_INPUT)) {
        ReleaseCapture();
        if (Console->FullScreenFlags == 0) {
            return TRUE;
        }
        return FALSE;
    }

    // BUGBUG alt is not set correctly
    InputEvent.Event.MouseEvent.dwControlKeyState = GetControlKeyState(0);

    if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
        if (MousePosition.X > ScreenInfo->Window.Right) {
            MousePosition.X = ScreenInfo->Window.Right;
        }
        if (MousePosition.Y > ScreenInfo->Window.Bottom) {
            MousePosition.Y = ScreenInfo->Window.Bottom;
        }
    }
    switch (Message) {
        case WM_LBUTTONDOWN:
            SetCapture(Console->hWnd);
            ButtonFlags = FROM_LEFT_1ST_BUTTON_PRESSED;
            EventFlags = 0;
            break;
        case WM_LBUTTONUP:
            ReleaseCapture();
            ButtonFlags = 0;
            EventFlags = 0;
            break;
        case WM_RBUTTONDOWN:
            SetCapture(Console->hWnd);
            ButtonFlags = RIGHTMOST_BUTTON_PRESSED;
            EventFlags = 0;
            break;
        case WM_RBUTTONUP:
            ReleaseCapture();
            ButtonFlags = 0;
            EventFlags = 0;
            break;
        case WM_MBUTTONDOWN:
            SetCapture(Console->hWnd);
            ButtonFlags = FROM_LEFT_2ND_BUTTON_PRESSED;
            EventFlags = 0;
            break;
        case WM_MBUTTONUP:
            ReleaseCapture();
            ButtonFlags = 0;
            EventFlags = 0;
            break;
        case WM_MOUSEMOVE:
            ButtonFlags = 0;
            EventFlags = MOUSE_MOVED;
            break;
        case WM_LBUTTONDBLCLK:
            ButtonFlags = FROM_LEFT_1ST_BUTTON_PRESSED;
            EventFlags = DOUBLE_CLICK;
            break;
        case WM_RBUTTONDBLCLK:
            ButtonFlags = RIGHTMOST_BUTTON_PRESSED;
            EventFlags = DOUBLE_CLICK;
            break;
        case WM_MBUTTONDBLCLK:
            ButtonFlags = FROM_LEFT_2ND_BUTTON_PRESSED;
            EventFlags = DOUBLE_CLICK;
            break;
        case WM_MOUSEWHEEL:
            ButtonFlags = ((UINT)wParam & 0xFFFF0000);
            EventFlags = MOUSE_WHEELED;
            break;
        default:
            ASSERT(FALSE);
    }
    InputEvent.EventType = MOUSE_EVENT;
    InputEvent.Event.MouseEvent.dwMousePosition = MousePosition;
    InputEvent.Event.MouseEvent.dwEventFlags = EventFlags;
    InputEvent.Event.MouseEvent.dwButtonState =
        ConvertMouseButtonState(ButtonFlags,(UINT)wParam);
    EventsWritten = WriteInputBuffer( Console,
                                      &Console->InputBuffer,
                                      &InputEvent,
                                      1
                                     );
#if DBG
    if (EventsWritten != 1) {
        OutputDebugStringA("PutInputInBuffer: EventsWritten != 1, 1 expected\n");
    }
#endif
#ifdef i386
    if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
        UpdateMousePosition(ScreenInfo,InputEvent.Event.MouseEvent.dwMousePosition);
    }
#endif
    return FALSE;
}

VOID
HandleFocusEvent(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL bSetFocus
    )
{
    INPUT_RECORD InputEvent;
    ULONG EventsWritten;
    USERTHREAD_FLAGS Flags;

    InputEvent.EventType = FOCUS_EVENT;
    InputEvent.Event.FocusEvent.bSetFocus = bSetFocus;

    Flags.dwFlags = 0;
    if (bSetFocus) {
        if (Console->Flags & CONSOLE_VDM_REGISTERED) {
            Flags.dwFlags |= TIF_VDMAPP;
        }
        if (Console->Flags & CONSOLE_CONNECTED_TO_EMULATOR) {
            Flags.dwFlags |= TIF_DOSEMULATOR;
        }
    }

    Flags.dwMask = (TIF_VDMAPP | TIF_DOSEMULATOR);
    NtUserSetInformationThread(Console->InputThreadInfo->ThreadHandle,
            UserThreadFlags, &Flags, sizeof(Flags));
    EventsWritten = WriteInputBuffer( Console,
                                      &Console->InputBuffer,
                                      &InputEvent,
                                      1
                                     );
#if DBG
    if (EventsWritten != 1) {
        OutputDebugStringA("PutInputInBuffer: EventsWritten != 1, 1 expected\n");
    }
#endif
}

VOID
HandleMenuEvent(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD wParam
    )
{
    INPUT_RECORD InputEvent;
    ULONG EventsWritten;

    InputEvent.EventType = MENU_EVENT;
    InputEvent.Event.MenuEvent.dwCommandId = wParam;
    EventsWritten = WriteInputBuffer( Console,
                                      &Console->InputBuffer,
                                      &InputEvent,
                                      1
                                     );
#if DBG
    if (EventsWritten != 1) {
        OutputDebugStringA("PutInputInBuffer: EventsWritten != 1, 1 expected\n");
    }
#endif
}

VOID
HandleCtrlEvent(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD EventType
    )
{
    switch (EventType) {
        case CTRL_C_EVENT:
            Console->CtrlFlags |= CONSOLE_CTRL_C_FLAG;
            break;
        case CTRL_BREAK_EVENT:
            Console->CtrlFlags |= CONSOLE_CTRL_BREAK_FLAG;
            break;
        case CTRL_CLOSE_EVENT:
            Console->CtrlFlags |= CONSOLE_CTRL_CLOSE_FLAG;
            break;
        default:
            ASSERT (FALSE);
    }
}

VOID
KillProcess(
    PCONSOLE_PROCESS_TERMINATION_RECORD ProcessHandleRecord,
    ULONG_PTR ProcessId
    )
{
    NTSTATUS status;

    //
    // Just terminate the process outright.
    //

    status = NtTerminateProcess(ProcessHandleRecord->ProcessHandle,
                ProcessHandleRecord->bDebugee ? DBG_TERMINATE_PROCESS : CONTROL_C_EXIT);

#if DBG
    if (status != STATUS_SUCCESS &&
            status != STATUS_PROCESS_IS_TERMINATING &&
            status != STATUS_THREAD_WAS_SUSPENDED &&
            !(status == STATUS_ACCESS_DENIED && ProcessHandleRecord->bDebugee)) {
        DbgPrint("NtTerminateProcess failed - status = %x\n", status);
        DbgBreakPoint();
    }
#endif

    //
    // Clear any remaining hard errors for the process.
    //

    if (ProcessId)
        BoostHardError(ProcessId, BHE_FORCE);

    //
    // Give the process 5 seconds to exit.
    //

    if (NT_SUCCESS(status)) {
        LARGE_INTEGER li;

        li.QuadPart = (LONGLONG)-10000 * CMSHUNGAPPTIMEOUT;
        status = NtWaitForSingleObject(ProcessHandleRecord->ProcessHandle,
                                       FALSE,
                                       &li);
        if (status != STATUS_WAIT_0) {
            RIPMSG2(RIP_WARNING,
                    "KillProcess: wait for process %x failed with status %x",
                    ProcessId, status);
        }
    }
}

int
CreateCtrlThread(
    IN PCONSOLE_PROCESS_TERMINATION_RECORD ProcessHandleList,
    IN ULONG ProcessHandleListLength,
    IN PWCHAR Title,
    IN DWORD EventType,
    IN BOOL fForce
    )

// this routine must be called not holding the console lock.
// returns true if process is exiting

{
    HANDLE Thread;
    DWORD Status;
    NTSTATUS status;
    DWORD ShutdownFlags;
    int Success=CONSOLE_SHUTDOWN_SUCCEEDED;
    ULONG i;
    DWORD EventFlags;
    PROCESS_BASIC_INFORMATION BasicInfo;
    PCSR_PROCESS Process;
    BOOL fForceProcess;
    BOOL fExitProcess;
    BOOL fFirstPass=TRUE;
    BOOL fSecondPassNeeded=FALSE;
    BOOL fHasError;
    BOOL fFirstWait;
    BOOL fEventProcessed;
    BOOL fBreakEvent;

BigLoop:
    for (i=0;i<ProcessHandleListLength;i++) {

        //
        // If the user has already cancelled shutdown, don't try to kill
        // any more processes.
        //

        if (Success == CONSOLE_SHUTDOWN_FAILED) {
            break;
        }

        //
        // Get the process shutdown parameters here. First get the process
        // id so we can get the csr process structure pointer.
        //

        status = NtQueryInformationProcess(
                ProcessHandleList[i].ProcessHandle,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL);

        //
        // Grab the shutdown flags from the csr process structure.  If
        // the structure cannot be found, terminate the process.
        //

        ProcessHandleList[i].bDebugee = FALSE;
        ShutdownFlags = 0;
        if (NT_SUCCESS(status)) {
            CsrLockProcessByClientId(
                    (HANDLE)BasicInfo.UniqueProcessId, &Process);
            if (Process == NULL) {
                KillProcess(&ProcessHandleList[i],
                        BasicInfo.UniqueProcessId);
                continue;
            }
        } else {
            KillProcess(&ProcessHandleList[i], 0);
            continue;
        }
        ShutdownFlags = Process->ShutdownFlags;
        ProcessHandleList[i].bDebugee = Process->DebugUserInterface.UniqueProcess!=NULL;
        CsrUnlockProcess(Process);

        if (!ProcessHandleList[i].bDebugee) {
            HANDLE DebugPort;

            // see if we're a OS/2 app that's being debugged
            DebugPort = (HANDLE)NULL;
            status = NtQueryInformationProcess(
                        ProcessHandleList[i].ProcessHandle,
                        ProcessDebugPort,
                        (PVOID)&DebugPort,
                        sizeof(DebugPort),
                        NULL
                        );
            if ( NT_SUCCESS(status) && DebugPort ) {
                ProcessHandleList[i].bDebugee = TRUE;
            }
        }
        if (EventType != CTRL_C_EVENT && EventType != CTRL_BREAK_EVENT) {
            fBreakEvent = FALSE;
            if (fFirstPass) {
                if (ProcessHandleList[i].bDebugee) {
                    fSecondPassNeeded = TRUE;
                    continue;
                }
            } else {
                if (!ProcessHandleList[i].bDebugee) {
                    continue;
                }
            }
        } else {
            fBreakEvent = TRUE;
            fFirstPass=FALSE;
        }

        //
        // fForce is whether ExitWindowsEx was called with EWX_FORCE.
        // ShutdownFlags are the shutdown flags for this process. If
        // either are force (noretry is the same as force), then force:
        // which means if the app doesn't exit, don't bring up the retry
        // dialog - just force it to exit right away.
        //

        fForceProcess = fForce || (ShutdownFlags & SHUTDOWN_NORETRY);

        //
        // Only notify system security and service context processes.
        // Don't bring up retry dialogs for them.
        //

        fExitProcess = TRUE;
        EventFlags = 0;
        if (ShutdownFlags & (SHUTDOWN_SYSTEMCONTEXT | SHUTDOWN_OTHERCONTEXT)) {

            //
            // System context - make sure we don't cause it to exit, make
            // sure we don't bring up retry dialogs.
            //

            fExitProcess = FALSE;
            fForceProcess = TRUE;

            //
            // This EventFlag will be passed on down to the CtrlRoutine()
            // on the client side. That way that side knows not to exit
            // this process.
            //

            EventFlags = 0x80000000;
        }

        //
        // Is this the first time we're waiting for this process to die?
        //

        fFirstWait = TRUE;
        fEventProcessed = FALSE;

        while (!fEventProcessed) {
            DWORD ThreadExitCode;
            DWORD ProcessExitCode;
            DWORD cMsTimeout;

            Thread = InternalCreateCallbackThread(
                    ProcessHandleList[i].ProcessHandle,
                    (ULONG_PTR)ProcessHandleList[i].CtrlRoutine,
                    EventType | EventFlags);

            //
            // If the thread cannot be created, terminate the process.
            //

            if (Thread == NULL) {
                KdPrint(("CONSRV: CreateRemoteThread failed %x\n",GetLastError()));
                break;
            }

            //
            // Mark the event as processed.
            //

            fEventProcessed = TRUE;

            /*
             * if it's a ctrl-c or ctrl-break event, just close our
             * handle to the thread.  otherwise it's a close.  wait
             * for client-side thread to terminate.
             */

            if (EventType == CTRL_CLOSE_EVENT) {
                cMsTimeout = gCmsHungAppTimeout;
            } else if (EventType == CTRL_LOGOFF_EVENT) {
                cMsTimeout = gCmsWaitToKillTimeout;
            } else if (EventType == CTRL_SHUTDOWN_EVENT) {

                //
                // If we are shutting down services.exe, we need to look in the
                // registry to see how long to wait.
                //

                if (fFirstWait && BasicInfo.UniqueProcessId == gdwServicesProcessId) {
                    cMsTimeout = gdwServicesWaitToKillTimeout;
                } else {
                    cMsTimeout = gCmsWaitToKillTimeout;
                }
            } else {
                CloseHandle(Thread);
                fExitProcess = FALSE;
                break;
            }

            while (TRUE) {
                fHasError = BoostHardError(BasicInfo.UniqueProcessId,
                        (fForceProcess ? BHE_FORCE : BHE_ACTIVATE));

                //
                // Use a 1 second wait if there was a hard error, otherwise
                // wait cMsTimeout ms.
                //

                Status = InternalWaitCancel(Thread,
                        (fHasError && fForceProcess) ? 1000 : cMsTimeout);
                if (Status == WAIT_TIMEOUT) {
                    int Action;

                    //
                    // If there was a hard error, see if there is another one.
                    //

                    if (fHasError && fForceProcess) {
                        continue;
                    }

                    if (!fForceProcess) {

                        //
                        // we timed out in the handler.  ask the user what
                        // to do.
                        //

                        DialogBoxCount++;
                        Action = ThreadShutdownNotify(WMCS_CONSOLE, (ULONG_PTR)Thread, (LPARAM)Title);
                        DialogBoxCount--;

                        //
                        // If the response is Cancel or EndTask, exit the loop.
                        // Otherwise retry the wait.
                        //

                        if (Action == TSN_USERSAYSCANCEL) {
                            Success = CONSOLE_SHUTDOWN_FAILED;
                        }
                    }
                } else if (Status == 0) {
                    ThreadExitCode = 0;
                    GetExitCodeThread(Thread,&ThreadExitCode);
                    GetExitCodeProcess(ProcessHandleList[i].ProcessHandle,
                            &ProcessExitCode);

                    //
                    // if the app returned TRUE (event handled)
                    // notify the user and see if the app should
                    // be terminated anyway.
                    //

                    if (fHasError || (ThreadExitCode == EventType &&
                            ProcessExitCode == STILL_ACTIVE)) {
                        int Action;

                        if (!fForceProcess) {

                            //
                            // Wait for the process to exit.  If it does exit,
                            // don't bring up the end task dialog.
                            //

                            Status = InternalWaitCancel(ProcessHandleList[i].ProcessHandle,
                                    (fHasError || fFirstWait) ? 1000 : cMsTimeout);
                            if (Status == 0) {

                                //
                                // The process exited.
                                //

                                fExitProcess = FALSE;
                            } else if (Status == WAIT_TIMEOUT) {
                                DialogBoxCount++;
                                Action = ThreadShutdownNotify(WMCS_CONSOLE,
                                                              (ULONG_PTR)ProcessHandleList[i].ProcessHandle,
                                                               (LPARAM)Title);
                                DialogBoxCount--;

                                if (Action == TSN_USERSAYSCANCEL) {
                                    Success = CONSOLE_SHUTDOWN_FAILED;
                                }
                            }
                        }
                    } else {

                        //
                        // The process exited.
                        //

                        fExitProcess = FALSE;
                    }
                }

                //
                // If we get here, we know that all wait conditions have
                // been satisfied.  Time to finish with the process.
                //

                break;
            }

            CloseHandle(Thread);
        }

        //
        // If the process is shutting down, mark it as terminated.
        // This prevents the process from raising any hard error popups
        // after we're done shutting it down.
        //

        if (!fBreakEvent &&
                !(ShutdownFlags & (SHUTDOWN_SYSTEMCONTEXT | SHUTDOWN_OTHERCONTEXT)) &&
                Success == CONSOLE_SHUTDOWN_SUCCEEDED) {
            CsrLockProcessByClientId(
                    (HANDLE)BasicInfo.UniqueProcessId, &Process);
            if (Process) {
                Process->Flags |= CSR_PROCESS_TERMINATED;
                CsrUnlockProcess(Process);
            }

            //
            // Force the termination of the process if needed.  Otherwise,
            // acknowledge any remaining hard errors.
            //
            if (fExitProcess) {
                KillProcess(&ProcessHandleList[i],
                        BasicInfo.UniqueProcessId);
            } else {
                BoostHardError(BasicInfo.UniqueProcessId, BHE_FORCE);
            }
        }
    }

    //
    // If this was our first time through and we skipped one of the
    // processes because it was being debugged, we'll go back for a
    // second pass.
    //

    if (fFirstPass && fSecondPassNeeded) {
        fFirstPass = FALSE;
        goto BigLoop;
    }

    // if we're shutting down a system or service security context
    // thread, don't wait for the process to terminate

    if (ShutdownFlags & (SHUTDOWN_SYSTEMCONTEXT | SHUTDOWN_OTHERCONTEXT)) {
        return CONSOLE_SHUTDOWN_SYSTEM;
    }
    return Success;
}

int
ProcessCtrlEvents(
    IN PCONSOLE_INFORMATION Console
    )
/* returns TRUE if a ctrl thread was created */
{
    PWCHAR Title;
    CONSOLE_PROCESS_TERMINATION_RECORD ProcessHandles[2];
    PCONSOLE_PROCESS_TERMINATION_RECORD ProcessHandleList;
    ULONG ProcessHandleListLength,i;
    ULONG CtrlFlags;
    PLIST_ENTRY ListHead, ListNext;
    BOOL FreeTitle;
    int Success;
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    DWORD EventType;
    DWORD LimitingProcessId;
    NTSTATUS Status;

    //
    // If the console was marked for destruction, do it now.
    //

    if (Console->Flags & CONSOLE_IN_DESTRUCTION) {
        DestroyConsole(Console);
        return CONSOLE_SHUTDOWN_FAILED;
    }

    //
    // make sure we don't try to process control events if this
    // console is already going away
    //

    if (Console->Flags & CONSOLE_TERMINATING) {
        Console->CtrlFlags = 0;
    }

    if (Console->CtrlFlags == 0) {
        RtlLeaveCriticalSection(&Console->ConsoleLock);
        return CONSOLE_SHUTDOWN_FAILED;
    }

    //
    // make our own copy of the console process handle list
    //

    LimitingProcessId = Console->LimitingProcessId;
    Console->LimitingProcessId = 0;

    ListHead = &Console->ProcessHandleList;
    ListNext = ListHead->Flink;
    ProcessHandleListLength = 0;
    while (ListNext != ListHead) {
        ProcessHandleRecord = CONTAINING_RECORD( ListNext, CONSOLE_PROCESS_HANDLE, ListLink );
        ListNext = ListNext->Flink;
        if ( LimitingProcessId ) {
            if ( ProcessHandleRecord->Process->ProcessGroupId == LimitingProcessId ) {
                ProcessHandleListLength += 1;
            }
        } else {
            ProcessHandleListLength += 1;
        }
    }

    //
    // Use the stack buffer to hold the process handles if there are only a
    // few, otherwise allocate a buffer from the heap.
    //

    if (ProcessHandleListLength <= NELEM(ProcessHandles)) {
        ProcessHandleList = ProcessHandles;
    } else {
        ProcessHandleList = (PCONSOLE_PROCESS_TERMINATION_RECORD)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),ProcessHandleListLength * sizeof(CONSOLE_PROCESS_TERMINATION_RECORD));
        if (ProcessHandleList == NULL) {
            RtlLeaveCriticalSection(&Console->ConsoleLock);
            return CONSOLE_SHUTDOWN_FAILED;
        }
    }

    ListNext = ListHead->Flink;
    i=0;
    while (ListNext != ListHead) {
        BOOLEAN ProcessIsIn;

        ASSERT(i<=ProcessHandleListLength);
        ProcessHandleRecord = CONTAINING_RECORD( ListNext, CONSOLE_PROCESS_HANDLE, ListLink );
        ListNext = ListNext->Flink;

        if ( LimitingProcessId ) {
            if ( ProcessHandleRecord->Process->ProcessGroupId == LimitingProcessId ) {
                ProcessIsIn = TRUE;
            } else {
                ProcessIsIn = FALSE;
            }
        } else {
            ProcessIsIn = TRUE;
        }

        if ( ProcessIsIn ) {
            Success = (int)DuplicateHandle(NtCurrentProcess(),
                           ProcessHandleRecord->ProcessHandle,
                           NtCurrentProcess(),
                           &ProcessHandleList[i].ProcessHandle,
                           0,
                           FALSE,
                           DUPLICATE_SAME_ACCESS);

            //
            // If the duplicate failed, the best we can do is to skip
            // including the process in the list and hope it goes
            // away.
            //
            if (!Success) {
                KdPrint(("CONSRV: dup handle failed for %d of %d in %lx\n",
                         i, ProcessHandleListLength, Console));
                continue;
            }

            if (Console->CtrlFlags & CONSOLE_CTRL_CLOSE_FLAG) {
                ProcessHandleRecord->TerminateCount++;
            } else {
                ProcessHandleRecord->TerminateCount = 0;
            }
            ProcessHandleList[i].TerminateCount = ProcessHandleRecord->TerminateCount;

            if (ProcessHandleRecord->CtrlRoutine) {
                ProcessHandleList[i].CtrlRoutine = ProcessHandleRecord->CtrlRoutine;
            } else {
                ProcessHandleList[i].CtrlRoutine = CtrlRoutine;
            }

            //
            // If this is the VDM process and we're closing the
            // console window, move it to the front of the list
            //

            if (i > 0 && Console->VDMProcessId && Console->VDMProcessId ==
                    ProcessHandleRecord->Process->ClientId.UniqueProcess &&
                    ProcessHandleRecord->TerminateCount > 0) {
                CONSOLE_PROCESS_TERMINATION_RECORD ProcessHandle;
                ProcessHandle = ProcessHandleList[0];
                ProcessHandleList[0] = ProcessHandleList[i];
                ProcessHandleList[i] = ProcessHandle;
            }

            i++;
        }
    }
    ProcessHandleListLength = i;
    ASSERT(ProcessHandleListLength > 0);

    // copy title.  titlelength does not include terminating null.

    Title = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TITLE_TAG ),Console->TitleLength+sizeof(WCHAR));
    if (Title) {
        FreeTitle = TRUE;
        RtlCopyMemory(Title,Console->Title,Console->TitleLength+sizeof(WCHAR));
    } else {
        FreeTitle = FALSE;
        Title = L"Command Window";
    }

    // copy ctrl flags

    CtrlFlags = Console->CtrlFlags;
    ASSERT( !((CtrlFlags & (CONSOLE_CTRL_CLOSE_FLAG | CONSOLE_CTRL_BREAK_FLAG | CONSOLE_CTRL_C_FLAG)) &&
              (CtrlFlags & (CONSOLE_CTRL_LOGOFF_FLAG | CONSOLE_CTRL_SHUTDOWN_FLAG)) ));

    Console->CtrlFlags = 0;

    RtlLeaveCriticalSection(&Console->ConsoleLock);

    //
    // the ctrl flags could be a combination of the following
    // values:
    //
    //        CONSOLE_CTRL_C_FLAG
    //        CONSOLE_CTRL_BREAK_FLAG
    //        CONSOLE_CTRL_CLOSE_FLAG
    //        CONSOLE_CTRL_LOGOFF_FLAG
    //        CONSOLE_CTRL_SHUTDOWN_FLAG
    //

    Success = CONSOLE_SHUTDOWN_FAILED;

    EventType = (DWORD)-1;
    switch (CtrlFlags & (CONSOLE_CTRL_CLOSE_FLAG | CONSOLE_CTRL_BREAK_FLAG |
            CONSOLE_CTRL_C_FLAG | CONSOLE_CTRL_LOGOFF_FLAG |
            CONSOLE_CTRL_SHUTDOWN_FLAG)) {

    case CONSOLE_CTRL_CLOSE_FLAG:
        EventType = CTRL_CLOSE_EVENT;
        break;

    case CONSOLE_CTRL_BREAK_FLAG:
        EventType = CTRL_BREAK_EVENT;
        break;

    case CONSOLE_CTRL_C_FLAG:
        EventType = CTRL_C_EVENT;
        break;

    case CONSOLE_CTRL_LOGOFF_FLAG:
        EventType = CTRL_LOGOFF_EVENT;
        break;

    case CONSOLE_CTRL_SHUTDOWN_FLAG:
        EventType = CTRL_SHUTDOWN_EVENT;
        break;
    }

    if (EventType != (DWORD)-1) {

        Success = CreateCtrlThread(ProcessHandleList,
                ProcessHandleListLength,
                Title,
                EventType,
                (CtrlFlags & CONSOLE_FORCE_SHUTDOWN_FLAG) != 0
                );
    }

    if (FreeTitle) {
        ConsoleHeapFree(Title);
    }

    for (i=0;i<ProcessHandleListLength;i++) {
        Status = NtClose(ProcessHandleList[i].ProcessHandle);
        ASSERT(NT_SUCCESS(Status));
    }

    if (ProcessHandleList != ProcessHandles) {
        ConsoleHeapFree(ProcessHandleList);
    }

    return Success;
}


VOID
UnlockConsole(
    IN PCONSOLE_INFORMATION Console
    )
{
    LIST_ENTRY WaitQueue;

    //
    // Make sure the console pointer is still valid
    //
    ASSERT(NT_SUCCESS(ValidateConsole(Console)));

#ifdef i386
    //
    // do nothing if we are in screen switching(handshaking with ntvdm)
    // we don't check anything else because we are in a safe state here.
    //
    if (ConsoleVDMOnSwitching == Console &&
        ConsoleVDMOnSwitching->VDMProcessId == CONSOLE_CLIENTPROCESSID()) {
        KdPrint(("    UnlockConsole - Thread %lx is leaving VDM CritSec\n", GetCurrentThreadId()));
        RtlLeaveCriticalSection(&ConsoleVDMCriticalSection);
        return;
    }
#endif

    //
    // if we're about to release the console lock, see if there
    // are any satisfied wait blocks that need to be dereferenced.
    // this code avoids a deadlock between grabbing the console
    // lock and then grabbing the process structure lock.
    //
#if defined(_X86_)
    if (Console->ConsoleLock.RecursionCount == 1) {
#endif
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) || defined(_IA64_)
    if (Console->ConsoleLock.RecursionCount == 0) {
#endif
        InitializeListHead(&WaitQueue);
        if (Console->WaitQueue) {
            CsrMoveSatisfiedWait(&WaitQueue, Console->WaitQueue);
            Console->WaitQueue = NULL;
        }
        ProcessCtrlEvents(Console);

        /*
         * Can't call CsrDereferenceWait with the console locked or we could deadlock.
         */
        if (!IsListEmpty(&WaitQueue)) {
            CsrDereferenceWait(&WaitQueue);
        }
    } else {
        RtlLeaveCriticalSection(&Console->ConsoleLock);
    }
}

ULONG
ShutdownConsole(
    IN HANDLE ConsoleHandle,
    IN DWORD dwFlags
    )
/*
    returns TRUE if console shutdown.  we recurse here so we don't
    return from the WM_QUERYENDSESSION until the console is gone.

*/

{
    DWORD EventFlag;
    int WaitForShutdown;
    PCONSOLE_INFORMATION Console;

    EventFlag = 0;

    //
    // Transmit the force bit (meaning don't bring up the retry dialog
    // if the app times out.
    //

    if (dwFlags & EWX_FORCE)
        EventFlag |= CONSOLE_FORCE_SHUTDOWN_FLAG;

    //
    // Remember if this is shutdown or logoff - inquiring apps want to know.
    //

    if (dwFlags & EWX_SHUTDOWN) {
        EventFlag |= CONSOLE_CTRL_SHUTDOWN_FLAG;
    } else {
        EventFlag |= CONSOLE_CTRL_LOGOFF_FLAG;
    }

    //
    // see if console already going away
    //

    if (!NT_SUCCESS(RevalidateConsole(ConsoleHandle, &Console))) {
        KdPrint(("CONSRV: Shutting down terminating console\n"));
        return SHUTDOWN_KNOWN_PROCESS;
    }

    Console->Flags |= CONSOLE_SHUTTING_DOWN;
    Console->CtrlFlags = EventFlag;
    Console->LimitingProcessId = 0;

    WaitForShutdown = ProcessCtrlEvents(Console);
    if (WaitForShutdown == CONSOLE_SHUTDOWN_SUCCEEDED) {
        return (ULONG)STATUS_PROCESS_IS_TERMINATING;
    } else {
        if (!NT_SUCCESS(RevalidateConsole(ConsoleHandle, &Console))) {
            return SHUTDOWN_KNOWN_PROCESS;
        }
        Console->Flags &= ~CONSOLE_SHUTTING_DOWN;
        UnlockConsole(Console);
        if (WaitForShutdown == CONSOLE_SHUTDOWN_SYSTEM) {
            return SHUTDOWN_KNOWN_PROCESS;
        } else {
            return SHUTDOWN_CANCEL;
        }
    }
}

VOID UserExitWorkerThread(VOID)

/*++

Routine Description:

    The current thread can exit using ExitThread.

    ExitThread is the prefered method of exiting a thread.  When this
    API is called (either explicitly or by returning from a thread
    procedure), The current thread's stack is deallocated and the thread
    terminates.  If the thread is the last thread in the process when
    this API is called, the behavior of this API does not change.  DLLs
    are not notified as a result of a call to ExitThread.

Arguments:

    dwExitCode - Supplies the termination status for the thread.

Return Value:

    None.

--*/

{
    MEMORY_BASIC_INFORMATION MemInfo;
    NTSTATUS st;
    VOID SwitchStackThenTerminate(PVOID CurrentStack, PVOID NewStack, DWORD ExitCode);

    st = NtQueryVirtualMemory(
            NtCurrentProcess(),
            NtCurrentTeb()->NtTib.StackLimit,
            MemoryBasicInformation,
            (PVOID)&MemInfo,
            sizeof(MemInfo),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        RtlRaiseStatus(st);
        }

    SwitchStackThenTerminate(
        MemInfo.AllocationBase,
        &NtCurrentTeb()->UserReserved[0],
        0
        );
}

VOID FreeStackAndTerminate(
    IN PVOID OldStack,
    IN DWORD ExitCode)

/*++

Routine Description:

    This API is called during thread termination to delete a thread's
    stack and then terminate.

Arguments:

    OldStack - Supplies the address of the stack to free.

    ExitCode - Supplies the termination status that the thread
        is to exit with.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    SIZE_T Zero;
    PVOID BaseAddress;

    Zero = 0;
    BaseAddress = OldStack;

    Status = NtFreeVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &Zero,
                MEM_RELEASE
                );
    ASSERT(NT_SUCCESS(Status));

    //
    // Don't worry, no commenting precedent has been set by SteveWo.  this
    // comment was added by an innocent bystander.
    //
    // NtTerminateThread will return if this thread is the last one in
    // the process.  So ExitProcess will only be called if that is the
    // case.
    //

    NtTerminateThread(NULL,(NTSTATUS)ExitCode);
}

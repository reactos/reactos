/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/coninput.c
 * PURPOSE:         Console Input functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "conio.h"
#include "handle.h"
#include "lineinput.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

#define ConSrvGetInputBuffer(ProcessData, Handle, Ptr, Access, LockConsole)     \
    ConSrvGetObject((ProcessData), (Handle), (PCONSOLE_IO_OBJECT*)(Ptr), NULL,  \
                    (Access), (LockConsole), INPUT_BUFFER)
#define ConSrvGetInputBufferAndHandleEntry(ProcessData, Handle, Ptr, Entry, Access, LockConsole)    \
    ConSrvGetObject((ProcessData), (Handle), (PCONSOLE_IO_OBJECT*)(Ptr), (Entry),                   \
                    (Access), (LockConsole), INPUT_BUFFER)
#define ConSrvReleaseInputBuffer(Buff, IsConsoleLocked) \
    ConSrvReleaseObject(&(Buff)->Header, (IsConsoleLocked))


#define ConsoleInputUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->CodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleInputAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->CodePage, 0, (sChar), 1, (dWChar), 1)

typedef struct ConsoleInput_t
{
    LIST_ENTRY ListEntry;
    INPUT_RECORD InputEvent;
} ConsoleInput;

typedef struct _GET_INPUT_INFO
{
    PCSR_THREAD           CallingThread;    // The thread which called the input API.
    PVOID                 HandleEntry;      // The handle data associated with the wait thread.
    PCONSOLE_INPUT_BUFFER InputBuffer;      // The input buffer corresponding to the handle.
} GET_INPUT_INFO, *PGET_INPUT_INFO;


/* PRIVATE FUNCTIONS **********************************************************/

#if 0
static VOID FASTCALL
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
#endif

NTSTATUS FASTCALL
ConioProcessInputEvent(PCONSOLE Console,
                       PINPUT_RECORD InputEvent)
{
    ConsoleInput *ConInRec;

    /* Check for pause or unpause */
    if (InputEvent->EventType == KEY_EVENT && InputEvent->Event.KeyEvent.bKeyDown)
    {
        WORD vk = InputEvent->Event.KeyEvent.wVirtualKeyCode;
        if (!(Console->PauseFlags & PAUSED_FROM_KEYBOARD))
        {
            DWORD cks = InputEvent->Event.KeyEvent.dwControlKeyState;
            if (Console->InputBuffer.Mode & ENABLE_LINE_INPUT &&
                (vk == VK_PAUSE || (vk == 'S' &&
                                    (cks & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) &&
                                   !(cks & (LEFT_ALT_PRESSED  | RIGHT_ALT_PRESSED)))))
            {
                ConioPause(Console, PAUSED_FROM_KEYBOARD);
                return STATUS_SUCCESS;
            }
        }
        else
        {
            if ((vk < VK_SHIFT || vk > VK_CAPITAL) && vk != VK_LWIN &&
                vk != VK_RWIN && vk != VK_NUMLOCK && vk != VK_SCROLL)
            {
                ConioUnpause(Console, PAUSED_FROM_KEYBOARD);
                return STATUS_SUCCESS;
            }
        }
    }

    /* Add event to the queue */
    ConInRec = ConsoleAllocHeap(0, sizeof(ConsoleInput));
    if (ConInRec == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    ConInRec->InputEvent = *InputEvent;
    InsertTailList(&Console->InputBuffer.InputEvents, &ConInRec->ListEntry);

    SetEvent(Console->InputBuffer.ActiveEvent);
    CsrNotifyWait(&Console->InputBuffer.ReadWaitQueue,
                  WaitAny,
                  NULL,
                  NULL);
    if (!IsListEmpty(&Console->InputBuffer.ReadWaitQueue))
    {
        CsrDereferenceWait(&Console->InputBuffer.ReadWaitQueue);
    }

    return STATUS_SUCCESS;
}

VOID FASTCALL
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

VOID NTAPI
ConDrvProcessKey(IN PCONSOLE Console,
                 IN BOOLEAN Down,
                 IN UINT VirtualKeyCode,
                 IN UINT VirtualScanCode,
                 IN WCHAR UnicodeChar,
                 IN ULONG ShiftState,
                 IN BYTE KeyStateCtrl)
{
    INPUT_RECORD er;

    /* process Ctrl-C and Ctrl-Break */
    if ( Console->InputBuffer.Mode & ENABLE_PROCESSED_INPUT &&
         Down && (VirtualKeyCode == VK_PAUSE || VirtualKeyCode == 'C') &&
         (ShiftState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) || KeyStateCtrl & 0x80) )
    {
        DPRINT1("Console_Api Ctrl-C\n");
        ConDrvConsoleProcessCtrlEvent(Console, 0, CTRL_C_EVENT);

        if (Console->LineBuffer && !Console->LineComplete)
        {
            /* Line input is in progress; end it */
            Console->LinePos = Console->LineSize = 0;
            Console->LineComplete = TRUE;
        }
        return;
    }

    if ( (ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) != 0 &&
         (VK_UP == VirtualKeyCode || VK_DOWN == VirtualKeyCode) )
    {
        if (!Down) return;

        /* scroll up or down */
        if (VK_UP == VirtualKeyCode)
        {
            /* only scroll up if there is room to scroll up into */
            if (Console->ActiveBuffer->CursorPosition.Y != Console->ActiveBuffer->ScreenBufferSize.Y - 1)
            {
                Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY +
                                                   Console->ActiveBuffer->ScreenBufferSize.Y - 1) %
                                                   Console->ActiveBuffer->ScreenBufferSize.Y;
                Console->ActiveBuffer->CursorPosition.Y++;
            }
        }
        else
        {
            /* only scroll down if there is room to scroll down into */
            if (Console->ActiveBuffer->CursorPosition.Y != 0)
            {
                Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY + 1) %
                                                   Console->ActiveBuffer->ScreenBufferSize.Y;
                Console->ActiveBuffer->CursorPosition.Y--;
            }
        }

        ConioDrawConsole(Console);
        return;
    }

    er.EventType                        = KEY_EVENT;
    er.Event.KeyEvent.bKeyDown          = Down;
    er.Event.KeyEvent.wRepeatCount      = 1;
    er.Event.KeyEvent.wVirtualKeyCode   = VirtualKeyCode;
    er.Event.KeyEvent.wVirtualScanCode  = VirtualScanCode;
    er.Event.KeyEvent.uChar.UnicodeChar = UnicodeChar;
    er.Event.KeyEvent.dwControlKeyState = ShiftState;

    ConioProcessInputEvent(Console, &er);
}


/* PUBLIC SERVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvWriteConsoleInput(IN PCONSOLE Console,
                        IN PCONSOLE_INPUT_BUFFER InputBuffer,
                        IN BOOLEAN Unicode,
                        IN PINPUT_RECORD InputRecord,
                        IN ULONG NumEventsToWrite,
                        OUT PULONG NumEventsWritten)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    if (Console == NULL || InputBuffer == NULL /* || InputRecord == NULL */)
        return STATUS_INVALID_PARAMETER;

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT( (InputRecord != NULL && NumEventsToWrite >= 0) ||
            (InputRecord == NULL && NumEventsToWrite == 0) );

    if (NumEventsWritten) *NumEventsWritten = 0;

    for (i = 0; i < NumEventsToWrite && NT_SUCCESS(Status); ++i)
    {
        if (InputRecord->EventType == KEY_EVENT && !Unicode)
        {
            CHAR AsciiChar = InputRecord->Event.KeyEvent.uChar.AsciiChar;
            ConsoleInputAnsiCharToUnicodeChar(Console,
                                              &InputRecord->Event.KeyEvent.uChar.UnicodeChar,
                                              &AsciiChar);
        }

        Status = ConioProcessInputEvent(Console, InputRecord++);
    }

    if (NumEventsWritten) *NumEventsWritten = i;

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
                                    OUT PULONG NumEvents)
{
    PLIST_ENTRY CurrentInput;

    if (Console == NULL || InputBuffer == NULL || NumEvents == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == InputBuffer->Header.Console);

    *NumEvents = 0;

    /* If there are any events ... */
    CurrentInput = InputBuffer->InputEvents.Flink;
    while (CurrentInput != &InputBuffer->InputEvents)
    {
        CurrentInput = CurrentInput->Flink;
        (*NumEvents)++;
    }

    return STATUS_SUCCESS;
}

/* EOF */

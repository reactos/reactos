/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/lineinput.c
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "popup.h"

#define NDEBUG
#include <debug.h>


BOOLEAN
ConvertInputAnsiToUnicode(PCONSRV_CONSOLE Console,
                          PVOID    Source,
                          USHORT   SourceLength,
                          // BOOLEAN  IsUnicode,
                          PWCHAR*  Target,
                          PUSHORT  TargetLength);
BOOLEAN
ConvertInputUnicodeToAnsi(PCONSRV_CONSOLE Console,
                          PVOID    Source,
                          USHORT   SourceLength,
                          // BOOLEAN  IsAnsi,
                          PCHAR/* * */   Target,
                          /*P*/USHORT  TargetLength);


VOID
HistoryAddEntry(PCONSRV_CONSOLE Console,
                PUNICODE_STRING ExeName,
                PUNICODE_STRING Entry);
BOOL
HistoryRecallHistory(PCONSRV_CONSOLE Console,
                     PUNICODE_STRING ExeName,
                     INT Offset,
                     PUNICODE_STRING Entry);
VOID
HistoryGetCurrentEntry(PCONSRV_CONSOLE Console,
                       PUNICODE_STRING ExeName,
                       PUNICODE_STRING Entry);
VOID
HistoryDeleteCurrentBuffer(PCONSRV_CONSOLE Console,
                           PUNICODE_STRING ExeName);
BOOL
HistoryFindEntryByPrefix(PCONSRV_CONSOLE Console,
                         PUNICODE_STRING ExeName,
                         PUNICODE_STRING Prefix,
                         PUNICODE_STRING Entry);


/* PRIVATE FUNCTIONS **********************************************************/

static VOID
LineInputSetPos(PCONSRV_CONSOLE Console,
                UINT Pos)
{
    if (Pos != Console->LinePos && GetConsoleInputBufferMode(Console) & ENABLE_ECHO_INPUT)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;
        SHORT OldCursorX = Buffer->CursorPosition.X;
        SHORT OldCursorY = Buffer->CursorPosition.Y;
        INT XY = OldCursorY * Buffer->ScreenBufferSize.X + OldCursorX;

        XY += (Pos - Console->LinePos);
        if (XY < 0)
            XY = 0;
        else if (XY >= Buffer->ScreenBufferSize.Y * Buffer->ScreenBufferSize.X)
            XY = Buffer->ScreenBufferSize.Y * Buffer->ScreenBufferSize.X - 1;

        Buffer->CursorPosition.X = XY % Buffer->ScreenBufferSize.X;
        Buffer->CursorPosition.Y = XY / Buffer->ScreenBufferSize.X;
        TermSetScreenInfo(Console, Buffer, OldCursorX, OldCursorY);
    }

    Console->LinePos = Pos;
}

static VOID
LineInputEdit(PCONSRV_CONSOLE Console,
              UINT NumToDelete,
              UINT NumToInsert,
              PWCHAR Insertion)
{
    PTEXTMODE_SCREEN_BUFFER ActiveBuffer;
    UINT Pos = Console->LinePos;
    UINT NewSize = Console->LineSize - NumToDelete + NumToInsert;
    UINT i;

    if (GetType(Console->ActiveBuffer) != TEXTMODE_BUFFER) return;
    ActiveBuffer = (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer;

    /* Make sure there's always enough room for ending \r\n */
    if (NewSize + 2 > Console->LineMaxSize)
        return;

    memmove(&Console->LineBuffer[Pos + NumToInsert],
            &Console->LineBuffer[Pos + NumToDelete],
            (Console->LineSize - (Pos + NumToDelete)) * sizeof(WCHAR));
    memcpy(&Console->LineBuffer[Pos], Insertion, NumToInsert * sizeof(WCHAR));

    if (GetConsoleInputBufferMode(Console) & ENABLE_ECHO_INPUT)
    {
        for (i = Pos; i < NewSize; i++)
        {
            TermWriteStream(Console, ActiveBuffer, &Console->LineBuffer[i], 1, TRUE);
        }
        for (; i < Console->LineSize; i++)
        {
            TermWriteStream(Console, ActiveBuffer, L" ", 1, TRUE);
        }
        Console->LinePos = i;
    }

    Console->LineSize = NewSize;
    LineInputSetPos(Console, Pos + NumToInsert);
}

#if 0
static VOID
LineInputRecallHistory(PCONSRV_CONSOLE Console,
                       PUNICODE_STRING ExeName,
                       INT Offset)
{
    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);
    UINT Position = 0;

    if (!Hist || Hist->NumEntries == 0) return;

    Position = Hist->Position + Offset;
    Position = min(max(Position, 0), Hist->NumEntries - 1);
    Hist->Position = Position;

    LineInputSetPos(Console, 0);
    LineInputEdit(Console, Console->LineSize,
                  Hist->Entries[Hist->Position].Length / sizeof(WCHAR),
                  Hist->Entries[Hist->Position].Buffer);
}
#else
static VOID
LineInputRecallHistory(PCONSRV_CONSOLE Console,
                       PUNICODE_STRING ExeName,
                       INT Offset)
{
    UNICODE_STRING Entry;

    if (!HistoryRecallHistory(Console, ExeName, Offset, &Entry)) return;

    LineInputSetPos(Console, 0);
    LineInputEdit(Console, Console->LineSize,
                  Entry.Length / sizeof(WCHAR),
                  Entry.Buffer);
}
#endif


// TESTS!!
PPOPUP_WINDOW Popup = NULL;

PPOPUP_WINDOW
HistoryDisplayCurrentHistory(PCONSRV_CONSOLE Console,
                             PUNICODE_STRING ExeName);

VOID
LineInputKeyDown(PCONSRV_CONSOLE Console,
                 PUNICODE_STRING ExeName,
                 KEY_EVENT_RECORD *KeyEvent)
{
    UINT Pos = Console->LinePos;
    UNICODE_STRING Entry;

    /*
     * First, deal with control keys...
     */

    switch (KeyEvent->wVirtualKeyCode)
    {
        case VK_ESCAPE:
        {
            /* Clear entire line */
            LineInputSetPos(Console, 0);
            LineInputEdit(Console, Console->LineSize, 0, NULL);

            // TESTS!!
            if (Popup)
            {
                DestroyPopupWindow(Popup);
                Popup = NULL;
            }
            return;
        }

        case VK_HOME:
        {
            /* Move to start of line. With CTRL, erase everything left of cursor */
            LineInputSetPos(Console, 0);
            if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
                LineInputEdit(Console, Pos, 0, NULL);
            return;
        }

        case VK_END:
        {
            /* Move to end of line. With CTRL, erase everything right of cursor */
            if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
                LineInputEdit(Console, Console->LineSize - Pos, 0, NULL);
            else
                LineInputSetPos(Console, Console->LineSize);
            return;
        }

        case VK_LEFT:
        {
            /* Move left. With CTRL, move to beginning of previous word */
            if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            {
                while (Pos > 0 && Console->LineBuffer[Pos - 1] == L' ') Pos--;
                while (Pos > 0 && Console->LineBuffer[Pos - 1] != L' ') Pos--;
            }
            else
            {
                Pos -= (Pos > 0);
            }
            LineInputSetPos(Console, Pos);
            return;
        }

        case VK_RIGHT:
        case VK_F1:
        {
            /* Move right. With CTRL, move to beginning of next word */
            if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            {
                while (Pos < Console->LineSize && Console->LineBuffer[Pos] != L' ') Pos++;
                while (Pos < Console->LineSize && Console->LineBuffer[Pos] == L' ') Pos++;
                LineInputSetPos(Console, Pos);
            }
            else
            {
                /* Recall one character (but don't overwrite current line) */
                HistoryGetCurrentEntry(Console, ExeName, &Entry);
                if (Pos < Console->LineSize)
                    LineInputSetPos(Console, Pos + 1);
                else if (Pos * sizeof(WCHAR) < Entry.Length)
                    LineInputEdit(Console, 0, 1, &Entry.Buffer[Pos]);
            }
            return;
        }

        case VK_INSERT:
        {
            /* Toggle between insert and overstrike */
            Console->LineInsertToggle = !Console->LineInsertToggle;
            TermSetCursorInfo(Console, Console->ActiveBuffer);
            return;
        }

        case VK_DELETE:
        {
            /* Remove character to right of cursor */
            if (Pos != Console->LineSize)
                LineInputEdit(Console, 1, 0, NULL);
            return;
        }

        case VK_PRIOR:
        {
            /* Recall first history entry */
            LineInputRecallHistory(Console, ExeName, -((WORD)-1));
            return;
        }

        case VK_NEXT:
        {
            /* Recall last history entry */
            LineInputRecallHistory(Console, ExeName, +((WORD)-1));
            return;
        }

        case VK_UP:
        case VK_F5:
        {
            /*
             * Recall previous history entry. On first time, actually recall the
             * current (usually last) entry; on subsequent times go back.
             */
            LineInputRecallHistory(Console, ExeName, Console->LineUpPressed ? -1 : 0);
            Console->LineUpPressed = TRUE;
            return;
        }

        case VK_DOWN:
        {
            /* Recall next history entry */
            LineInputRecallHistory(Console, ExeName, +1);
            return;
        }

        case VK_F3:
        {
            /* Recall remainder of current history entry */
            HistoryGetCurrentEntry(Console, ExeName, &Entry);
            if (Pos * sizeof(WCHAR) < Entry.Length)
            {
                UINT InsertSize = (Entry.Length / sizeof(WCHAR) - Pos);
                UINT DeleteSize = min(Console->LineSize - Pos, InsertSize);
                LineInputEdit(Console, DeleteSize, InsertSize, &Entry.Buffer[Pos]);
            }
            return;
        }

        case VK_F6:
        {
            /* Insert a ^Z character */
            KeyEvent->uChar.UnicodeChar = 26;
            break;
        }

        case VK_F7:
        {
            if (KeyEvent->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
                HistoryDeleteCurrentBuffer(Console, ExeName);
            else
            {
                if (Popup) DestroyPopupWindow(Popup);
                Popup = HistoryDisplayCurrentHistory(Console, ExeName);
            }
            return;
        }

        case VK_F8:
        {
            UNICODE_STRING EntryFound;

            Entry.Length = Console->LinePos * sizeof(WCHAR); // == Pos * sizeof(WCHAR)
            Entry.Buffer = Console->LineBuffer;

            if (HistoryFindEntryByPrefix(Console, ExeName, &Entry, &EntryFound))
            {
                LineInputEdit(Console, Console->LineSize - Pos,
                              EntryFound.Length / sizeof(WCHAR) - Pos,
                              &EntryFound.Buffer[Pos]);
                /* Cursor stays where it was */
                LineInputSetPos(Console, Pos);
            }

            return;
        }
#if 0
        {
            PHISTORY_BUFFER Hist;
            INT HistPos;

            /* Search for history entries starting with input. */
            Hist = HistoryCurrentBuffer(Console, ExeName);
            if (!Hist || Hist->NumEntries == 0) return;

            /*
             * Like Up/F5, on first time start from current (usually last) entry,
             * but on subsequent times start at previous entry.
             */
            if (Console->LineUpPressed)
                Hist->Position = (Hist->Position ? Hist->Position : Hist->NumEntries) - 1;
            Console->LineUpPressed = TRUE;

            Entry.Length = Console->LinePos * sizeof(WCHAR); // == Pos * sizeof(WCHAR)
            Entry.Buffer = Console->LineBuffer;

            /*
             * Keep going backwards, even wrapping around to the end,
             * until we get back to starting point.
             */
            HistPos = Hist->Position;
            do
            {
                if (RtlPrefixUnicodeString(&Entry, &Hist->Entries[HistPos], FALSE))
                {
                    Hist->Position = HistPos;
                    LineInputEdit(Console, Console->LineSize - Pos,
                                  Hist->Entries[HistPos].Length / sizeof(WCHAR) - Pos,
                                  &Hist->Entries[HistPos].Buffer[Pos]);
                    /* Cursor stays where it was */
                    LineInputSetPos(Console, Pos);
                    return;
                }
                if (--HistPos < 0) HistPos += Hist->NumEntries;
            } while (HistPos != Hist->Position);

            return;
        }
#endif

        return;
    }


    /*
     * OK, we deal with normal keys, we can continue...
     */

    if (KeyEvent->uChar.UnicodeChar == L'\b' && GetConsoleInputBufferMode(Console) & ENABLE_PROCESSED_INPUT)
    {
        /* backspace handling - if processed input enabled then we handle it here
         * otherwise we treat it like a normal char. */
        if (Pos > 0)
        {
            LineInputSetPos(Console, Pos - 1);
            LineInputEdit(Console, 1, 0, NULL);
        }
    }
    else if (KeyEvent->uChar.UnicodeChar == L'\r')
    {
        Entry.Length = Entry.MaximumLength = Console->LineSize * sizeof(WCHAR);
        Entry.Buffer = Console->LineBuffer;
        HistoryAddEntry(Console, ExeName, &Entry);

        /* TODO: Expand aliases */
        DPRINT1("TODO: Expand aliases\n");

        LineInputSetPos(Console, Console->LineSize);
        Console->LineBuffer[Console->LineSize++] = L'\r';
        if (GetConsoleInputBufferMode(Console) & ENABLE_ECHO_INPUT)
        {
            if (GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
            {
                TermWriteStream(Console, (PTEXTMODE_SCREEN_BUFFER)(Console->ActiveBuffer), L"\r", 1, TRUE);
            }
        }

        /*
         * Add \n if processed input. There should usually be room for it,
         * but an exception to the rule exists: the buffer could have been 
         * pre-filled with LineMaxSize - 1 characters.
         */
        if (GetConsoleInputBufferMode(Console) & ENABLE_PROCESSED_INPUT &&
            Console->LineSize < Console->LineMaxSize)
        {
            Console->LineBuffer[Console->LineSize++] = L'\n';
            if (GetConsoleInputBufferMode(Console) & ENABLE_ECHO_INPUT)
            {
                if (GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
                {
                    TermWriteStream(Console, (PTEXTMODE_SCREEN_BUFFER)(Console->ActiveBuffer), L"\n", 1, TRUE);
                }
            }
        }
        Console->LineComplete = TRUE;
        Console->LinePos = 0;
    }
    else if (KeyEvent->uChar.UnicodeChar != L'\0')
    {
        if (KeyEvent->uChar.UnicodeChar < 0x20 &&
            Console->LineWakeupMask & (1 << KeyEvent->uChar.UnicodeChar))
        {
            /* Control key client wants to handle itself (e.g. for tab completion) */
            Console->LineBuffer[Console->LineSize++] = L' ';
            Console->LineBuffer[Console->LinePos] = KeyEvent->uChar.UnicodeChar;
            Console->LineComplete = TRUE;
            Console->LinePos = 0;
        }
        else
        {
            /* Normal character */
            BOOL Overstrike = !Console->LineInsertToggle && (Console->LinePos != Console->LineSize);
            DPRINT("Overstrike = %s\n", Overstrike ? "true" : "false");
            LineInputEdit(Console, (Overstrike ? 1 : 0), 1, &KeyEvent->uChar.UnicodeChar);
        }
    }
}


/* PUBLIC SERVER APIS *********************************************************/

/* EOF */

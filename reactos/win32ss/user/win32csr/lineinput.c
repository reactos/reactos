/*
 * PROJECT:         ReactOS CSRSS
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/csrss/win32csr/lineinput.c
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include "w32csr.h"
#include <debug.h>

typedef struct tagHISTORY_BUFFER
{
    LIST_ENTRY ListEntry;
    WORD Position;
    WORD MaxEntries;
    WORD NumEntries;
    PUNICODE_STRING Entries;
    UNICODE_STRING ExeName;
} HISTORY_BUFFER, *PHISTORY_BUFFER;

/* FUNCTIONS *****************************************************************/

static PHISTORY_BUFFER
HistoryCurrentBuffer(PCSRSS_CONSOLE Console)
{
    /* TODO: use actual EXE name sent from process that called ReadConsole */
    UNICODE_STRING ExeName = { 14, 14, L"cmd.exe" };
    PLIST_ENTRY Entry = Console->HistoryBuffers.Flink;
    PHISTORY_BUFFER Hist;

    for (; Entry != &Console->HistoryBuffers; Entry = Entry->Flink)
    {
        Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);
        if (RtlEqualUnicodeString(&ExeName, &Hist->ExeName, FALSE))
            return Hist;
    }

    /* Couldn't find the buffer, create a new one */
    Hist = HeapAlloc(Win32CsrApiHeap, 0, sizeof(HISTORY_BUFFER) + ExeName.Length);
    if (!Hist)
        return NULL;
    Hist->MaxEntries = Console->HistoryBufferSize;
    Hist->NumEntries = 0;
    Hist->Entries = HeapAlloc(Win32CsrApiHeap, 0, Hist->MaxEntries * sizeof(UNICODE_STRING));
    if (!Hist->Entries)
    {
        HeapFree(Win32CsrApiHeap, 0, Hist);
        return NULL;
    }
    Hist->ExeName.Length = Hist->ExeName.MaximumLength = ExeName.Length;
    Hist->ExeName.Buffer = (PWCHAR)(Hist + 1);
    memcpy(Hist->ExeName.Buffer, ExeName.Buffer, ExeName.Length);
    InsertHeadList(&Console->HistoryBuffers, &Hist->ListEntry);
    return Hist;
}

static VOID
HistoryAddEntry(PCSRSS_CONSOLE Console)
{
    UNICODE_STRING NewEntry;
    PHISTORY_BUFFER Hist;
    INT i;

    NewEntry.Length = NewEntry.MaximumLength = Console->LineSize * sizeof(WCHAR);
    NewEntry.Buffer = Console->LineBuffer;

    if (!(Hist = HistoryCurrentBuffer(Console)))
        return;

    /* Don't add blank or duplicate entries */
    if (NewEntry.Length == 0 || Hist->MaxEntries == 0 ||
        (Hist->NumEntries > 0 &&
         RtlEqualUnicodeString(&Hist->Entries[Hist->NumEntries - 1], &NewEntry, FALSE)))
    {
        return;
    }

    if (Console->HistoryNoDup)
    {
        /* Check if this line has been entered before */
        for (i = Hist->NumEntries - 1; i >= 0; i--)
        {
            if (RtlEqualUnicodeString(&Hist->Entries[i], &NewEntry, FALSE))
            {
                /* Just rotate the list to bring this entry to the end */
                NewEntry = Hist->Entries[i];
                memmove(&Hist->Entries[i], &Hist->Entries[i + 1],
                        (Hist->NumEntries - (i + 1)) * sizeof(UNICODE_STRING));
                Hist->Entries[Hist->NumEntries - 1] = NewEntry;
                Hist->Position = Hist->NumEntries - 1;
                return;
            }
        }
    }

    if (Hist->NumEntries == Hist->MaxEntries)
    {
        /* List is full, remove oldest entry */
        RtlFreeUnicodeString(&Hist->Entries[0]);
        memmove(&Hist->Entries[0], &Hist->Entries[1],
                --Hist->NumEntries * sizeof(UNICODE_STRING));
    }

    if (NT_SUCCESS(RtlDuplicateUnicodeString(0, &NewEntry, &Hist->Entries[Hist->NumEntries])))
        Hist->NumEntries++;
    Hist->Position = Hist->NumEntries - 1;
}

static VOID
HistoryGetCurrentEntry(PCSRSS_CONSOLE Console, PUNICODE_STRING Entry)
{
    PHISTORY_BUFFER Hist;
    if (!(Hist = HistoryCurrentBuffer(Console)) || Hist->NumEntries == 0)
        Entry->Length = 0;
    else
        *Entry = Hist->Entries[Hist->Position];
}

static PHISTORY_BUFFER
HistoryFindBuffer(PCSRSS_CONSOLE Console, PUNICODE_STRING ExeName)
{
    PLIST_ENTRY Entry = Console->HistoryBuffers.Flink;
    while (Entry != &Console->HistoryBuffers)
    {
        /* For the history APIs, the caller is allowed to give only part of the name */
        PHISTORY_BUFFER Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);
        if (RtlPrefixUnicodeString(ExeName, &Hist->ExeName, TRUE))
            return Hist;
        Entry = Entry->Flink;
    }
    return NULL;
}

VOID FASTCALL
HistoryDeleteBuffer(PHISTORY_BUFFER Hist)
{
    if (!Hist)
        return;
    while (Hist->NumEntries != 0)
        RtlFreeUnicodeString(&Hist->Entries[--Hist->NumEntries]);
    HeapFree(Win32CsrApiHeap, 0, Hist->Entries);
    RemoveEntryList(&Hist->ListEntry);
    HeapFree(Win32CsrApiHeap, 0, Hist);
}

CSR_API(CsrGetCommandHistoryLength)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;
    PHISTORY_BUFFER Hist;
    ULONG Length = 0;
    INT i;

    if (!Win32CsrValidateBuffer(ProcessData,
                                Request->Data.GetCommandHistoryLength.ExeName.Buffer,
                                Request->Data.GetCommandHistoryLength.ExeName.Length, 1))
    {
        return STATUS_ACCESS_VIOLATION;
    }

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Status))
    {
        Hist = HistoryFindBuffer(Console, &Request->Data.GetCommandHistory.ExeName);
        if (Hist)
        {
            for (i = 0; i < Hist->NumEntries; i++)
                Length += Hist->Entries[i].Length + sizeof(WCHAR);
        }
        Request->Data.GetCommandHistoryLength.Length = Length;
        ConioUnlockConsole(Console);
    }
    return Status;
}

CSR_API(CsrGetCommandHistory)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;
    PHISTORY_BUFFER Hist;
    PBYTE Buffer = (PBYTE)Request->Data.GetCommandHistory.History;
    ULONG BufferSize = Request->Data.GetCommandHistory.Length;
    INT i;

    if (!Win32CsrValidateBuffer(ProcessData, Buffer, BufferSize, 1) ||
        !Win32CsrValidateBuffer(ProcessData,
                                Request->Data.GetCommandHistory.ExeName.Buffer,
                                Request->Data.GetCommandHistory.ExeName.Length, 1))
    {
        return STATUS_ACCESS_VIOLATION;
    }

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Status))
    {
        Hist = HistoryFindBuffer(Console, &Request->Data.GetCommandHistory.ExeName);
        if (Hist)
        {
            for (i = 0; i < Hist->NumEntries; i++)
            {
                if (BufferSize < (Hist->Entries[i].Length + sizeof(WCHAR)))
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    break;
                }
                memcpy(Buffer, Hist->Entries[i].Buffer, Hist->Entries[i].Length);
                Buffer += Hist->Entries[i].Length;
                *(PWCHAR)Buffer = L'\0';
                Buffer += sizeof(WCHAR);
            }
        }
        Request->Data.GetCommandHistory.Length = Buffer - (PBYTE)Request->Data.GetCommandHistory.History;
        ConioUnlockConsole(Console);
    }
    return Status;
}

CSR_API(CsrExpungeCommandHistory)
{
    PCSRSS_CONSOLE Console;
    PHISTORY_BUFFER Hist;
    NTSTATUS Status;

    if (!Win32CsrValidateBuffer(ProcessData,
                                Request->Data.ExpungeCommandHistory.ExeName.Buffer,
                                Request->Data.ExpungeCommandHistory.ExeName.Length, 1))
    {
        return STATUS_ACCESS_VIOLATION;
    }

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Status))
    {
        Hist = HistoryFindBuffer(Console, &Request->Data.ExpungeCommandHistory.ExeName);
        HistoryDeleteBuffer(Hist);
        ConioUnlockConsole(Console);
    }
    return Status;
}

CSR_API(CsrSetHistoryNumberCommands)
{
    PCSRSS_CONSOLE Console;
    PHISTORY_BUFFER Hist;
    NTSTATUS Status;
    WORD MaxEntries = Request->Data.SetHistoryNumberCommands.NumCommands;
    PUNICODE_STRING OldEntryList, NewEntryList;

    if (!Win32CsrValidateBuffer(ProcessData,
                                Request->Data.SetHistoryNumberCommands.ExeName.Buffer,
                                Request->Data.SetHistoryNumberCommands.ExeName.Length, 1))
    {
        return STATUS_ACCESS_VIOLATION;
    }

    Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Status))
    {
        Hist = HistoryFindBuffer(Console, &Request->Data.SetHistoryNumberCommands.ExeName);
        if (Hist)
        {
            OldEntryList = Hist->Entries;
            NewEntryList = HeapAlloc(Win32CsrApiHeap, 0,
                                     MaxEntries * sizeof(UNICODE_STRING));
            if (!NewEntryList)
            {
                Status = STATUS_NO_MEMORY;
            }
            else
            {
                /* If necessary, shrink by removing oldest entries */
                for (; Hist->NumEntries > MaxEntries; Hist->NumEntries--)
                {
                    RtlFreeUnicodeString(Hist->Entries++);
                    Hist->Position += (Hist->Position == 0);
                }

                Hist->MaxEntries = MaxEntries;
                Hist->Entries = memcpy(NewEntryList, Hist->Entries,
                                       Hist->NumEntries * sizeof(UNICODE_STRING));
                HeapFree(Win32CsrApiHeap, 0, OldEntryList);
            }
        }
        ConioUnlockConsole(Console);
    }
    return Status;
}

CSR_API(CsrGetHistoryInfo)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Status))
    {
        Request->Data.SetHistoryInfo.HistoryBufferSize      = Console->HistoryBufferSize;
        Request->Data.SetHistoryInfo.NumberOfHistoryBuffers = Console->NumberOfHistoryBuffers;
        Request->Data.SetHistoryInfo.dwFlags                = Console->HistoryNoDup;
        ConioUnlockConsole(Console);
    }
    return Status;
}

CSR_API(CsrSetHistoryInfo)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Status))
    {
        Console->HistoryBufferSize      = (WORD)Request->Data.SetHistoryInfo.HistoryBufferSize;
        Console->NumberOfHistoryBuffers = (WORD)Request->Data.SetHistoryInfo.NumberOfHistoryBuffers;
        Console->HistoryNoDup           = Request->Data.SetHistoryInfo.dwFlags & HISTORY_NO_DUP_FLAG;
        ConioUnlockConsole(Console);
    }
    return Status;
}

static VOID
LineInputSetPos(PCSRSS_CONSOLE Console, UINT Pos)
{
    if (Pos != Console->LinePos && Console->Mode & ENABLE_ECHO_INPUT)
    {
        PCSRSS_SCREEN_BUFFER Buffer = Console->ActiveBuffer;
        UINT OldCursorX = Buffer->CurrentX;
        UINT OldCursorY = Buffer->CurrentY;
        INT XY = OldCursorY * Buffer->MaxX + OldCursorX;

        XY += (Pos - Console->LinePos);
        if (XY < 0)
            XY = 0;
        else if (XY >= Buffer->MaxY * Buffer->MaxX)
            XY = Buffer->MaxY * Buffer->MaxX - 1;

        Buffer->CurrentX = XY % Buffer->MaxX;
        Buffer->CurrentY = XY / Buffer->MaxX;
        ConioSetScreenInfo(Console, Buffer, OldCursorX, OldCursorY);
    }

    Console->LinePos = Pos;
}

static VOID
LineInputEdit(PCSRSS_CONSOLE Console, UINT NumToDelete, UINT NumToInsert, WCHAR *Insertion)
{
    UINT Pos = Console->LinePos;
    UINT NewSize = Console->LineSize - NumToDelete + NumToInsert;
    INT i;

    /* Make sure there's always enough room for ending \r\n */
    if (NewSize + 2 > Console->LineMaxSize)
        return;

    memmove(&Console->LineBuffer[Pos + NumToInsert],
            &Console->LineBuffer[Pos + NumToDelete],
            (Console->LineSize - (Pos + NumToDelete)) * sizeof(WCHAR));
    memcpy(&Console->LineBuffer[Pos], Insertion, NumToInsert * sizeof(WCHAR));

    if (Console->Mode & ENABLE_ECHO_INPUT)
    {
        for (i = Pos; i < NewSize; i++)
        {
            CHAR AsciiChar;
            WideCharToMultiByte(Console->OutputCodePage, 0,
                                &Console->LineBuffer[i], 1,
                                &AsciiChar, 1, NULL, NULL);
            ConioWriteConsole(Console, Console->ActiveBuffer, &AsciiChar, 1, TRUE);
        }
        for (; i < Console->LineSize; i++)
        {
            ConioWriteConsole(Console, Console->ActiveBuffer, " ", 1, TRUE);
        }
        Console->LinePos = i;
    }

    Console->LineSize = NewSize;
    LineInputSetPos(Console, Pos + NumToInsert);
}

static VOID
LineInputRecallHistory(PCSRSS_CONSOLE Console, INT Offset)
{
    PHISTORY_BUFFER Hist;

    if (!(Hist = HistoryCurrentBuffer(Console)) || Hist->NumEntries == 0)
        return;

    Offset += Hist->Position;
    Offset = max(Offset, 0);
    Offset = min(Offset, Hist->NumEntries - 1);
    Hist->Position = Offset;

    LineInputSetPos(Console, 0);
    LineInputEdit(Console, Console->LineSize,
                  Hist->Entries[Offset].Length / sizeof(WCHAR),
                  Hist->Entries[Offset].Buffer);
}

VOID FASTCALL
LineInputKeyDown(PCSRSS_CONSOLE Console, KEY_EVENT_RECORD *KeyEvent)
{
    UINT Pos = Console->LinePos;
    PHISTORY_BUFFER Hist;
    UNICODE_STRING Entry;
    INT HistPos;

    switch (KeyEvent->wVirtualKeyCode)
    {
    case VK_ESCAPE:
        /* Clear entire line */
        LineInputSetPos(Console, 0);
        LineInputEdit(Console, Console->LineSize, 0, NULL);
        return;
    case VK_HOME:
        /* Move to start of line. With ctrl, erase everything left of cursor */
        LineInputSetPos(Console, 0);
        if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            LineInputEdit(Console, Pos, 0, NULL);
        return;
    case VK_END:
        /* Move to end of line. With ctrl, erase everything right of cursor */
        if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            LineInputEdit(Console, Console->LineSize - Pos, 0, NULL);
        else
            LineInputSetPos(Console, Console->LineSize);
        return;
    case VK_LEFT:
        /* Move left. With ctrl, move to beginning of previous word */
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
    case VK_RIGHT:
    case VK_F1:
        /* Move right. With ctrl, move to beginning of next word */
        if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
        {
            while (Pos < Console->LineSize && Console->LineBuffer[Pos] != L' ') Pos++;
            while (Pos < Console->LineSize && Console->LineBuffer[Pos] == L' ') Pos++;
            LineInputSetPos(Console, Pos);
            return;
        }
        else
        {
            /* Recall one character (but don't overwrite current line) */
            HistoryGetCurrentEntry(Console, &Entry);
            if (Pos < Console->LineSize)
                LineInputSetPos(Console, Pos + 1);
            else if (Pos * sizeof(WCHAR) < Entry.Length)
                LineInputEdit(Console, 0, 1, &Entry.Buffer[Pos]);
        }
        return;
    case VK_INSERT:
        /* Toggle between insert and overstrike */
        Console->LineInsertToggle = !Console->LineInsertToggle;
        ConioSetCursorInfo(Console, Console->ActiveBuffer);
        return;
    case VK_DELETE:
        /* Remove character to right of cursor */
        if (Pos != Console->LineSize)
            LineInputEdit(Console, 1, 0, NULL);
        return;
    case VK_PRIOR:
        /* Recall first history entry */
        LineInputRecallHistory(Console, -((WORD)-1));
        return;
    case VK_NEXT:
        /* Recall last history entry */
        LineInputRecallHistory(Console, +((WORD)-1));
        return;
    case VK_UP:
    case VK_F5:
        /* Recall previous history entry. On first time, actually recall the
         * current (usually last) entry; on subsequent times go back. */
        LineInputRecallHistory(Console, Console->LineUpPressed ? -1 : 0);
        Console->LineUpPressed = TRUE;
        return;
    case VK_DOWN:
        /* Recall next history entry */
        LineInputRecallHistory(Console, +1);
        return;
    case VK_F3:
        /* Recall remainder of current history entry */
        HistoryGetCurrentEntry(Console, &Entry);
        if (Pos * sizeof(WCHAR) < Entry.Length)
        {
            UINT InsertSize = (Entry.Length / sizeof(WCHAR) - Pos);
            UINT DeleteSize = min(Console->LineSize - Pos, InsertSize);
            LineInputEdit(Console, DeleteSize, InsertSize, &Entry.Buffer[Pos]);
        }
        return;
    case VK_F6:
        /* Insert a ^Z character */
        KeyEvent->uChar.UnicodeChar = 26;
        break;
    case VK_F7:
        if (KeyEvent->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
            HistoryDeleteBuffer(HistoryCurrentBuffer(Console));
        return;
    case VK_F8:
        /* Search for history entries starting with input. */
        if (!(Hist = HistoryCurrentBuffer(Console)) || Hist->NumEntries == 0)
            return;

        /* Like Up/F5, on first time start from current (usually last) entry,
         * but on subsequent times start at previous entry. */
        if (Console->LineUpPressed)
            Hist->Position = (Hist->Position ? Hist->Position : Hist->NumEntries) - 1;
        Console->LineUpPressed = TRUE;

        Entry.Length = Console->LinePos * sizeof(WCHAR);
        Entry.Buffer = Console->LineBuffer;

        /* Keep going backwards, even wrapping around to the end,
         * until we get back to starting point */
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

    if (KeyEvent->uChar.UnicodeChar == L'\b' && Console->Mode & ENABLE_PROCESSED_INPUT)
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
        HistoryAddEntry(Console);

        /* TODO: Expand aliases */

        LineInputSetPos(Console, Console->LineSize);
        Console->LineBuffer[Console->LineSize++] = L'\r';
        if (Console->Mode & ENABLE_ECHO_INPUT)
            ConioWriteConsole(Console, Console->ActiveBuffer, "\r", 1, TRUE);

        /* Add \n if processed input. There should usually be room for it,
         * but an exception to the rule exists: the buffer could have been 
         * pre-filled with LineMaxSize - 1 characters. */
        if (Console->Mode & ENABLE_PROCESSED_INPUT &&
            Console->LineSize < Console->LineMaxSize)
        {
            Console->LineBuffer[Console->LineSize++] = L'\n';
            if (Console->Mode & ENABLE_ECHO_INPUT)
                ConioWriteConsole(Console, Console->ActiveBuffer, "\n", 1, TRUE);
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
            BOOL Overstrike = Console->LineInsertToggle && Console->LinePos != Console->LineSize;
            LineInputEdit(Console, Overstrike, 1, &KeyEvent->uChar.UnicodeChar);
        }
    }
}

/* EOF */

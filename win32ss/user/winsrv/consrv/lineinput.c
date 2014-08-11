/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/lineinput.c
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

typedef struct _HISTORY_BUFFER
{
    LIST_ENTRY ListEntry;
    UINT Position;
    UINT MaxEntries;
    UINT NumEntries;
    UNICODE_STRING ExeName;
    PUNICODE_STRING Entries;
} HISTORY_BUFFER, *PHISTORY_BUFFER;


BOOLEAN
ConvertInputAnsiToUnicode(PCONSOLE Console,
                          PVOID    Source,
                          USHORT   SourceLength,
                          // BOOLEAN  IsUnicode,
                          PWCHAR*  Target,
                          PUSHORT  TargetLength);
BOOLEAN
ConvertInputUnicodeToAnsi(PCONSOLE Console,
                          PVOID    Source,
                          USHORT   SourceLength,
                          // BOOLEAN  IsAnsi,
                          PCHAR/* * */   Target,
                          /*P*/USHORT  TargetLength);


/* PRIVATE FUNCTIONS **********************************************************/

static PHISTORY_BUFFER
HistoryCurrentBuffer(PCONSRV_CONSOLE Console,
                     PUNICODE_STRING ExeName)
{
    PLIST_ENTRY Entry = Console->HistoryBuffers.Flink;
    PHISTORY_BUFFER Hist;

    for (; Entry != &Console->HistoryBuffers; Entry = Entry->Flink)
    {
        Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);
        if (RtlEqualUnicodeString(ExeName, &Hist->ExeName, FALSE))
            return Hist;
    }

    /* Couldn't find the buffer, create a new one */
    Hist = ConsoleAllocHeap(0, sizeof(HISTORY_BUFFER) + ExeName->Length);
    if (!Hist) return NULL;
    Hist->MaxEntries = Console->HistoryBufferSize;
    Hist->NumEntries = 0;
    Hist->Entries = ConsoleAllocHeap(0, Hist->MaxEntries * sizeof(UNICODE_STRING));
    if (!Hist->Entries)
    {
        ConsoleFreeHeap(Hist);
        return NULL;
    }
    Hist->ExeName.Length = Hist->ExeName.MaximumLength = ExeName->Length;
    Hist->ExeName.Buffer = (PWCHAR)(Hist + 1);
    memcpy(Hist->ExeName.Buffer, ExeName->Buffer, ExeName->Length);
    InsertHeadList(&Console->HistoryBuffers, &Hist->ListEntry);
    return Hist;
}

static VOID
HistoryAddEntry(PCONSRV_CONSOLE Console,
                PUNICODE_STRING ExeName)
{
    UNICODE_STRING NewEntry;
    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);
    INT i;

    if (!Hist) return;

    NewEntry.Length = NewEntry.MaximumLength = Console->LineSize * sizeof(WCHAR);
    NewEntry.Buffer = Console->LineBuffer;

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
HistoryGetCurrentEntry(PCONSRV_CONSOLE Console,
                       PUNICODE_STRING ExeName,
                       PUNICODE_STRING Entry)
{
    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);

    if (!Hist || Hist->NumEntries == 0)
        Entry->Length = 0;
    else
        *Entry = Hist->Entries[Hist->Position];
}

static PHISTORY_BUFFER
HistoryFindBuffer(PCONSRV_CONSOLE Console,
                  PVOID    ExeName,
                  USHORT   ExeLength,
                  BOOLEAN  UnicodeExe)
{
    UNICODE_STRING ExeNameU;

    PLIST_ENTRY Entry;
    PHISTORY_BUFFER Hist = NULL;

    if (ExeName == NULL) return NULL;

    if (UnicodeExe)
    {
        ExeNameU.Buffer = ExeName;
        /* Length is in bytes */
        ExeNameU.MaximumLength = ExeLength;
    }
    else
    {
        if (!ConvertInputAnsiToUnicode(Console,
                                       ExeName, ExeLength,
                                       &ExeNameU.Buffer, &ExeNameU.MaximumLength))
        {
            return NULL;
        }
    }
    ExeNameU.Length = ExeNameU.MaximumLength;

    Entry = Console->HistoryBuffers.Flink;
    while (Entry != &Console->HistoryBuffers)
    {
        Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);

        /* For the history APIs, the caller is allowed to give only part of the name */
        if (RtlPrefixUnicodeString(&ExeNameU, &Hist->ExeName, TRUE))
        {
            if (!UnicodeExe) ConsoleFreeHeap(ExeNameU.Buffer);
            return Hist;
        }

        Entry = Entry->Flink;
    }

    if (!UnicodeExe) ConsoleFreeHeap(ExeNameU.Buffer);
    return NULL;
}

static VOID
HistoryDeleteBuffer(PHISTORY_BUFFER Hist)
{
    if (!Hist) return;

    while (Hist->NumEntries != 0)
        RtlFreeUnicodeString(&Hist->Entries[--Hist->NumEntries]);

    ConsoleFreeHeap(Hist->Entries);
    RemoveEntryList(&Hist->ListEntry);
    ConsoleFreeHeap(Hist);
}

VOID
HistoryDeleteBuffers(PCONSRV_CONSOLE Console)
{
    PLIST_ENTRY CurrentEntry;
    PHISTORY_BUFFER HistoryBuffer;

    while (!IsListEmpty(&Console->HistoryBuffers))
    {
        CurrentEntry = RemoveHeadList(&Console->HistoryBuffers);
        HistoryBuffer = CONTAINING_RECORD(CurrentEntry, HISTORY_BUFFER, ListEntry);
        HistoryDeleteBuffer(HistoryBuffer);
    }
}

static VOID
LineInputSetPos(PCONSRV_CONSOLE Console, UINT Pos)
{
    if (Pos != Console->LinePos && Console->InputBuffer.Mode & ENABLE_ECHO_INPUT)
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
LineInputEdit(PCONSRV_CONSOLE Console, UINT NumToDelete, UINT NumToInsert, PWCHAR Insertion)
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

    if (Console->InputBuffer.Mode & ENABLE_ECHO_INPUT)
    {
        for (i = Pos; i < NewSize; i++)
        {
            ConioWriteConsole(Console, ActiveBuffer, &Console->LineBuffer[i], 1, TRUE);
        }
        for (; i < Console->LineSize; i++)
        {
            ConioWriteConsole(Console, ActiveBuffer, L" ", 1, TRUE);
        }
        Console->LinePos = i;
    }

    Console->LineSize = NewSize;
    LineInputSetPos(Console, Pos + NumToInsert);
}

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

VOID
LineInputKeyDown(PCONSRV_CONSOLE Console,
                 PUNICODE_STRING ExeName,
                 KEY_EVENT_RECORD *KeyEvent)
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
            HistoryGetCurrentEntry(Console, ExeName, &Entry);
            if (Pos < Console->LineSize)
                LineInputSetPos(Console, Pos + 1);
            else if (Pos * sizeof(WCHAR) < Entry.Length)
                LineInputEdit(Console, 0, 1, &Entry.Buffer[Pos]);
        }
        return;
    case VK_INSERT:
        /* Toggle between insert and overstrike */
        Console->LineInsertToggle = !Console->LineInsertToggle;
        TermSetCursorInfo(Console, Console->ActiveBuffer);
        return;
    case VK_DELETE:
        /* Remove character to right of cursor */
        if (Pos != Console->LineSize)
            LineInputEdit(Console, 1, 0, NULL);
        return;
    case VK_PRIOR:
        /* Recall first history entry */
        LineInputRecallHistory(Console, ExeName, -((WORD)-1));
        return;
    case VK_NEXT:
        /* Recall last history entry */
        LineInputRecallHistory(Console, ExeName, +((WORD)-1));
        return;
    case VK_UP:
    case VK_F5:
        /* Recall previous history entry. On first time, actually recall the
         * current (usually last) entry; on subsequent times go back. */
        LineInputRecallHistory(Console, ExeName, Console->LineUpPressed ? -1 : 0);
        Console->LineUpPressed = TRUE;
        return;
    case VK_DOWN:
        /* Recall next history entry */
        LineInputRecallHistory(Console, ExeName, +1);
        return;
    case VK_F3:
        /* Recall remainder of current history entry */
        HistoryGetCurrentEntry(Console, ExeName, &Entry);
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
            HistoryDeleteBuffer(HistoryCurrentBuffer(Console, ExeName));
        return;
    case VK_F8:
        /* Search for history entries starting with input. */
        Hist = HistoryCurrentBuffer(Console, ExeName);
        if (!Hist || Hist->NumEntries == 0) return;

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

    if (KeyEvent->uChar.UnicodeChar == L'\b' && Console->InputBuffer.Mode & ENABLE_PROCESSED_INPUT)
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
        HistoryAddEntry(Console, ExeName);

        /* TODO: Expand aliases */

        LineInputSetPos(Console, Console->LineSize);
        Console->LineBuffer[Console->LineSize++] = L'\r';
        if (Console->InputBuffer.Mode & ENABLE_ECHO_INPUT)
        {
            if (GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
            {
                ConioWriteConsole(Console, (PTEXTMODE_SCREEN_BUFFER)(Console->ActiveBuffer), L"\r", 1, TRUE);
            }
        }

        /* Add \n if processed input. There should usually be room for it,
         * but an exception to the rule exists: the buffer could have been 
         * pre-filled with LineMaxSize - 1 characters. */
        if (Console->InputBuffer.Mode & ENABLE_PROCESSED_INPUT &&
            Console->LineSize < Console->LineMaxSize)
        {
            Console->LineBuffer[Console->LineSize++] = L'\n';
            if (Console->InputBuffer.Mode & ENABLE_ECHO_INPUT)
            {
                if (GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
                {
                    ConioWriteConsole(Console, (PTEXTMODE_SCREEN_BUFFER)(Console->ActiveBuffer), L"\n", 1, TRUE);
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
            LineInputEdit(Console, (Overstrike ? 1 : 0), 1, &KeyEvent->uChar.UnicodeChar);
        }
    }
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvGetConsoleCommandHistory)
{
    NTSTATUS Status;
    PCONSOLE_GETCOMMANDHISTORY GetCommandHistoryRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetCommandHistoryRequest;
    PCONSRV_CONSOLE Console;
    ULONG BytesWritten = 0;
    PHISTORY_BUFFER Hist;

    DPRINT1("SrvGetConsoleCommandHistory entered\n");

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&GetCommandHistoryRequest->History,
                                   GetCommandHistoryRequest->HistoryLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&GetCommandHistoryRequest->ExeName,
                                   GetCommandHistoryRequest->ExeLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Hist = HistoryFindBuffer(Console,
                             GetCommandHistoryRequest->ExeName,
                             GetCommandHistoryRequest->ExeLength,
                             GetCommandHistoryRequest->Unicode2);
    if (Hist)
    {
        UINT i;

        LPSTR  TargetBufferA;
        LPWSTR TargetBufferW;
        ULONG BufferSize = GetCommandHistoryRequest->HistoryLength;

        UINT Offset = 0;
        UINT SourceLength;

        if (GetCommandHistoryRequest->Unicode)
        {
            TargetBufferW = GetCommandHistoryRequest->History;
            BufferSize /= sizeof(WCHAR);
        }
        else
        {
            TargetBufferA = GetCommandHistoryRequest->History;
        }

        for (i = 0; i < Hist->NumEntries; i++)
        {
            SourceLength = Hist->Entries[i].Length / sizeof(WCHAR);
            if (Offset + SourceLength + 1 > BufferSize)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            if (GetCommandHistoryRequest->Unicode)
            {
                RtlCopyMemory(&TargetBufferW[Offset], Hist->Entries[i].Buffer, SourceLength * sizeof(WCHAR));
                Offset += SourceLength;
                TargetBufferW[Offset++] = L'\0';
            }
            else
            {
                ConvertInputUnicodeToAnsi(Console,
                                          Hist->Entries[i].Buffer, SourceLength * sizeof(WCHAR),
                                          &TargetBufferA[Offset], SourceLength);
                Offset += SourceLength;
                TargetBufferA[Offset++] = '\0';
            }
        }

        if (GetCommandHistoryRequest->Unicode)
            BytesWritten = Offset * sizeof(WCHAR);
        else
            BytesWritten = Offset;
    }

    // GetCommandHistoryRequest->HistoryLength = TargetBuffer - (PBYTE)GetCommandHistoryRequest->History;
    GetCommandHistoryRequest->HistoryLength = BytesWritten;

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleCommandHistoryLength)
{
    NTSTATUS Status;
    PCONSOLE_GETCOMMANDHISTORYLENGTH GetCommandHistoryLengthRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetCommandHistoryLengthRequest;
    PCONSRV_CONSOLE Console;
    PHISTORY_BUFFER Hist;
    ULONG Length = 0;
    UINT  i;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&GetCommandHistoryLengthRequest->ExeName,
                                  GetCommandHistoryLengthRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Hist = HistoryFindBuffer(Console,
                             GetCommandHistoryLengthRequest->ExeName,
                             GetCommandHistoryLengthRequest->ExeLength,
                             GetCommandHistoryLengthRequest->Unicode2);
    if (Hist)
    {
        for (i = 0; i < Hist->NumEntries; i++)
            Length += Hist->Entries[i].Length + sizeof(WCHAR); // Each entry is returned NULL-terminated
    }
    /*
     * Quick and dirty way of getting the number of bytes of the
     * corresponding ANSI string from the one in UNICODE.
     */
    if (!GetCommandHistoryLengthRequest->Unicode)
        Length /= sizeof(WCHAR);

    GetCommandHistoryLengthRequest->HistoryLength = Length;

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvExpungeConsoleCommandHistory)
{
    NTSTATUS Status;
    PCONSOLE_EXPUNGECOMMANDHISTORY ExpungeCommandHistoryRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ExpungeCommandHistoryRequest;
    PCONSRV_CONSOLE Console;
    PHISTORY_BUFFER Hist;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ExpungeCommandHistoryRequest->ExeName,
                                  ExpungeCommandHistoryRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Hist = HistoryFindBuffer(Console,
                             ExpungeCommandHistoryRequest->ExeName,
                             ExpungeCommandHistoryRequest->ExeLength,
                             ExpungeCommandHistoryRequest->Unicode2);
    HistoryDeleteBuffer(Hist);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvSetConsoleNumberOfCommands)
{
    NTSTATUS Status;
    PCONSOLE_SETHISTORYNUMBERCOMMANDS SetHistoryNumberCommandsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetHistoryNumberCommandsRequest;
    PCONSRV_CONSOLE Console;
    PHISTORY_BUFFER Hist;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&SetHistoryNumberCommandsRequest->ExeName,
                                  SetHistoryNumberCommandsRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Hist = HistoryFindBuffer(Console,
                             SetHistoryNumberCommandsRequest->ExeName,
                             SetHistoryNumberCommandsRequest->ExeLength,
                             SetHistoryNumberCommandsRequest->Unicode2);
    if (Hist)
    {
        UINT MaxEntries = SetHistoryNumberCommandsRequest->NumCommands;
        PUNICODE_STRING OldEntryList = Hist->Entries;
        PUNICODE_STRING NewEntryList = ConsoleAllocHeap(0, MaxEntries * sizeof(UNICODE_STRING));
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
            ConsoleFreeHeap(OldEntryList);
        }
    }

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleHistory)
{
#if 0 // Vista+
    PCONSOLE_GETSETHISTORYINFO HistoryInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.HistoryInfoRequest;
    PCONSRV_CONSOLE Console;
    NTSTATUS Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (NT_SUCCESS(Status))
    {
        HistoryInfoRequest->HistoryBufferSize      = Console->HistoryBufferSize;
        HistoryInfoRequest->NumberOfHistoryBuffers = Console->NumberOfHistoryBuffers;
        HistoryInfoRequest->dwFlags                = Console->HistoryNoDup;
        ConSrvReleaseConsole(Console, TRUE);
    }
    return Status;
#else
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
#endif
}

CSR_API(SrvSetConsoleHistory)
{
#if 0 // Vista+
    PCONSOLE_GETSETHISTORYINFO HistoryInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.HistoryInfoRequest;
    PCONSRV_CONSOLE Console;
    NTSTATUS Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (NT_SUCCESS(Status))
    {
        Console->HistoryBufferSize      = HistoryInfoRequest->HistoryBufferSize;
        Console->NumberOfHistoryBuffers = HistoryInfoRequest->NumberOfHistoryBuffers;
        Console->HistoryNoDup           = HistoryInfoRequest->dwFlags & HISTORY_NO_DUP_FLAG;
        ConSrvReleaseConsole(Console, TRUE);
    }
    return Status;
#else
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
#endif
}

CSR_API(SrvSetConsoleCommandHistoryMode)
{
    NTSTATUS Status;
    PCONSOLE_SETHISTORYMODE SetHistoryModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetHistoryModeRequest;
    PCONSRV_CONSOLE Console;

    DPRINT1("SrvSetConsoleCommandHistoryMode(Mode = %d) is not yet implemented\n",
            SetHistoryModeRequest->Mode);

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* This API is not yet implemented */
    Status = STATUS_NOT_IMPLEMENTED;

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

/* EOF */

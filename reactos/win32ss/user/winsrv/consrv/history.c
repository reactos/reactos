/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/history.c
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "popup.h"

#define NDEBUG
#include <debug.h>

typedef struct _HISTORY_BUFFER
{
    LIST_ENTRY ListEntry;
    ULONG Position;
    ULONG MaxEntries;
    ULONG NumEntries;
    UNICODE_STRING  ExeName;
    PUNICODE_STRING Entries;
} HISTORY_BUFFER, *PHISTORY_BUFFER;


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
HistoryAddEntry(PCONSRV_CONSOLE Console,
                PUNICODE_STRING ExeName,
                PUNICODE_STRING Entry)
{
    // UNICODE_STRING NewEntry;
    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);

    if (!Hist) return;

    // NewEntry.Length = NewEntry.MaximumLength = Console->LineSize * sizeof(WCHAR);
    // NewEntry.Buffer = Console->LineBuffer;

    /* Don't add blank or duplicate entries */
    if (Entry->Length == 0 || Hist->MaxEntries == 0 ||
        (Hist->NumEntries > 0 &&
         RtlEqualUnicodeString(&Hist->Entries[Hist->NumEntries - 1], Entry, FALSE)))
    {
        return;
    }

    if (Console->HistoryNoDup)
    {
        INT i;

        /* Check if this line has been entered before */
        for (i = Hist->NumEntries - 1; i >= 0; i--)
        {
            if (RtlEqualUnicodeString(&Hist->Entries[i], Entry, FALSE))
            {
                UNICODE_STRING NewEntry;

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

    if (NT_SUCCESS(RtlDuplicateUnicodeString(0, Entry, &Hist->Entries[Hist->NumEntries])))
        Hist->NumEntries++;
    Hist->Position = Hist->NumEntries - 1;
}

VOID
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

BOOL
HistoryRecallHistory(PCONSRV_CONSOLE Console,
                     PUNICODE_STRING ExeName,
                     INT Offset,
                     PUNICODE_STRING Entry)
{
    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);
    ULONG Position = 0;

    if (!Hist || Hist->NumEntries == 0) return FALSE;

    Position = Hist->Position + Offset;
    Position = min(max(Position, 0), Hist->NumEntries - 1);
    Hist->Position = Position;

    *Entry = Hist->Entries[Hist->Position];
    return TRUE;
}

BOOL
HistoryFindEntryByPrefix(PCONSRV_CONSOLE Console,
                         PUNICODE_STRING ExeName,
                         PUNICODE_STRING Prefix,
                         PUNICODE_STRING Entry)
{
    INT HistPos;

    /* Search for history entries starting with input. */
    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);
    if (!Hist || Hist->NumEntries == 0) return FALSE;

    /*
     * Like Up/F5, on first time start from current (usually last) entry,
     * but on subsequent times start at previous entry.
     */
    if (Console->LineUpPressed)
        Hist->Position = (Hist->Position ? Hist->Position : Hist->NumEntries) - 1;
    Console->LineUpPressed = TRUE;

    // Entry.Length = Console->LinePos * sizeof(WCHAR); // == Pos * sizeof(WCHAR)
    // Entry.Buffer = Console->LineBuffer;

    /*
     * Keep going backwards, even wrapping around to the end,
     * until we get back to starting point.
     */
    HistPos = Hist->Position;
    do
    {
        if (RtlPrefixUnicodeString(Prefix, &Hist->Entries[HistPos], FALSE))
        {
            Hist->Position = HistPos;
            *Entry = Hist->Entries[HistPos];
            return TRUE;
        }
        if (--HistPos < 0) HistPos += Hist->NumEntries;
    } while (HistPos != Hist->Position);

    return FALSE;
}

PPOPUP_WINDOW
HistoryDisplayCurrentHistory(PCONSRV_CONSOLE Console,
                             PUNICODE_STRING ExeName)
{
    PTEXTMODE_SCREEN_BUFFER ActiveBuffer;
    PPOPUP_WINDOW Popup;

    SHORT xLeft, yTop;
    SHORT Width, Height;

    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);

    if (!Hist) return NULL;
    if (Hist->NumEntries == 0) return NULL;

    if (GetType(Console->ActiveBuffer) != TEXTMODE_BUFFER) return NULL;
    ActiveBuffer = (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer;

    Width  = 40;
    Height = 10;

    /* Center the popup window on the screen */
    xLeft = ActiveBuffer->ViewOrigin.X + (ActiveBuffer->ViewSize.X - Width ) / 2;
    yTop  = ActiveBuffer->ViewOrigin.Y + (ActiveBuffer->ViewSize.Y - Height) / 2;

    /* Create the popup */
    Popup = CreatePopupWindow(Console, ActiveBuffer,
                              xLeft, yTop, Width, Height);
    if (Popup == NULL) return NULL;

    Popup->PopupInputRoutine = NULL;

    return Popup;
}

VOID
HistoryDeleteCurrentBuffer(PCONSRV_CONSOLE Console,
                           PUNICODE_STRING ExeName)
{
    HistoryDeleteBuffer(HistoryCurrentBuffer(Console, ExeName));
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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Hist = HistoryFindBuffer(Console,
                             GetCommandHistoryRequest->ExeName,
                             GetCommandHistoryRequest->ExeLength,
                             GetCommandHistoryRequest->Unicode2);
    if (Hist)
    {
        ULONG i;

        LPSTR  TargetBufferA;
        LPWSTR TargetBufferW;
        ULONG BufferSize = GetCommandHistoryRequest->HistoryLength;

        ULONG Offset = 0;
        ULONG SourceLength;

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

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&GetCommandHistoryLengthRequest->ExeName,
                                  GetCommandHistoryLengthRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Hist = HistoryFindBuffer(Console,
                             GetCommandHistoryLengthRequest->ExeName,
                             GetCommandHistoryLengthRequest->ExeLength,
                             GetCommandHistoryLengthRequest->Unicode2);
    if (Hist)
    {
        ULONG i;
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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Hist = HistoryFindBuffer(Console,
                             SetHistoryNumberCommandsRequest->ExeName,
                             SetHistoryNumberCommandsRequest->ExeLength,
                             SetHistoryNumberCommandsRequest->Unicode2);
    if (Hist)
    {
        ULONG MaxEntries = SetHistoryNumberCommandsRequest->NumCommands;
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
    NTSTATUS Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                       &Console, TRUE);
    if (NT_SUCCESS(Status))
    {
        HistoryInfoRequest->HistoryBufferSize      = Console->HistoryBufferSize;
        HistoryInfoRequest->NumberOfHistoryBuffers = Console->NumberOfHistoryBuffers;
        HistoryInfoRequest->dwFlags                = (Console->HistoryNoDup ? HISTORY_NO_DUP_FLAG : 0);
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
    NTSTATUS Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                       &Console, TRUE);
    if (NT_SUCCESS(Status))
    {
        Console->HistoryBufferSize      = HistoryInfoRequest->HistoryBufferSize;
        Console->NumberOfHistoryBuffers = HistoryInfoRequest->NumberOfHistoryBuffers;
        Console->HistoryNoDup           = !!(HistoryInfoRequest->dwFlags & HISTORY_NO_DUP_FLAG);
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

    DPRINT("SrvSetConsoleCommandHistoryMode(Mode = %d) is not yet implemented\n",
            SetHistoryModeRequest->Mode);

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console->InsertMode = !!(SetHistoryModeRequest->Mode & CONSOLE_OVERSTRIKE);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

/* EOF */

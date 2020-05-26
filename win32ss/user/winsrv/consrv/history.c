/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/history.c
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
HistoryCurrentBuffer(
    IN PCONSRV_CONSOLE Console,
    IN PUNICODE_STRING ExeName)
{
    PLIST_ENTRY Entry;
    PHISTORY_BUFFER Hist;

    for (Entry = Console->HistoryBuffers.Flink;
         Entry != &Console->HistoryBuffers;
         Entry = Entry->Flink)
    {
        Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);
        if (RtlEqualUnicodeString(ExeName, &Hist->ExeName, FALSE))
            return Hist;
    }

    /* Could not find the buffer, create a new one */

    if (Console->NumberOfHistoryBuffers < Console->MaxNumberOfHistoryBuffers)
    {
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
        RtlCopyMemory(Hist->ExeName.Buffer, ExeName->Buffer, ExeName->Length);
        InsertHeadList(&Console->HistoryBuffers, &Hist->ListEntry);
        Console->NumberOfHistoryBuffers++;
        return Hist;
    }
    else
    {
        // FIXME: TODO !!!!!!!
        // Reuse an older history buffer, if possible with the same Exe name,
        // otherwise take the oldest one and reset its contents.
        // And move the history buffer back to the head of the list.
        UNIMPLEMENTED;
        return NULL;
    }
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

    for (Entry = Console->HistoryBuffers.Flink;
         Entry != &Console->HistoryBuffers;
         Entry = Entry->Flink)
    {
        Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);

        /* For the history APIs, the caller is allowed to give only part of the name */
        if (RtlPrefixUnicodeString(&ExeNameU, &Hist->ExeName, TRUE))
        {
            if (!UnicodeExe) ConsoleFreeHeap(ExeNameU.Buffer);
            return Hist;
        }
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

static NTSTATUS
HistoryResizeBuffer(
    IN PHISTORY_BUFFER Hist,
    IN ULONG NumCommands)
{
    PUNICODE_STRING OldEntryList = Hist->Entries;
    PUNICODE_STRING NewEntryList;

    NewEntryList = ConsoleAllocHeap(0, NumCommands * sizeof(UNICODE_STRING));
    if (!NewEntryList)
        return STATUS_NO_MEMORY;

    /* If necessary, shrink by removing oldest entries */
    for (; Hist->NumEntries > NumCommands; Hist->NumEntries--)
    {
        RtlFreeUnicodeString(Hist->Entries++);
        Hist->Position += (Hist->Position == 0);
    }

    Hist->MaxEntries = NumCommands;
    RtlCopyMemory(NewEntryList, Hist->Entries,
                  Hist->NumEntries * sizeof(UNICODE_STRING));
    Hist->Entries = NewEntryList;
    ConsoleFreeHeap(OldEntryList);

    return STATUS_SUCCESS;
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
                RtlMoveMemory(&Hist->Entries[i], &Hist->Entries[i + 1],
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
        RtlMoveMemory(&Hist->Entries[0], &Hist->Entries[1],
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
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;
    PPOPUP_WINDOW Popup;

    SHORT xLeft, yTop;
    SHORT Width, Height;

    PHISTORY_BUFFER Hist = HistoryCurrentBuffer(Console, ExeName);

    if (!Hist) return NULL;
    if (Hist->NumEntries == 0) return NULL;

    if (GetType(Console->ActiveBuffer) != TEXTMODE_BUFFER) return NULL;
    ActiveBuffer = Console->ActiveBuffer;

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

VOID
HistoryReshapeAllBuffers(
    IN PCONSRV_CONSOLE Console,
    IN ULONG HistoryBufferSize,
    IN ULONG MaxNumberOfHistoryBuffers,
    IN BOOLEAN HistoryNoDup)
{
    PLIST_ENTRY Entry;
    PHISTORY_BUFFER Hist;
    NTSTATUS Status;

    // ASSERT(Console->NumberOfHistoryBuffers <= Console->MaxNumberOfHistoryBuffers);
    if (MaxNumberOfHistoryBuffers < Console->NumberOfHistoryBuffers)
    {
        /*
         * Trim the history buffers list and reduce it up to MaxNumberOfHistoryBuffers.
         * We loop the history buffers list backwards so as to trim the oldest
         * buffers first.
         */
        while (!IsListEmpty(&Console->HistoryBuffers) &&
               (Console->NumberOfHistoryBuffers > MaxNumberOfHistoryBuffers))
        {
            Entry = RemoveTailList(&Console->HistoryBuffers);
            Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);
            HistoryDeleteBuffer(Hist);
            Console->NumberOfHistoryBuffers--;
        }
        ASSERT(Console->NumberOfHistoryBuffers == MaxNumberOfHistoryBuffers);
        ASSERT(((Console->NumberOfHistoryBuffers == 0) &&  IsListEmpty(&Console->HistoryBuffers)) ||
               ((Console->NumberOfHistoryBuffers  > 0) && !IsListEmpty(&Console->HistoryBuffers)));
    }
    Console->MaxNumberOfHistoryBuffers = MaxNumberOfHistoryBuffers;

    /* Resize each history buffer */
    for (Entry = Console->HistoryBuffers.Flink;
         Entry != &Console->HistoryBuffers;
         Entry = Entry->Flink)
    {
        Hist = CONTAINING_RECORD(Entry, HISTORY_BUFFER, ListEntry);
        Status = HistoryResizeBuffer(Hist, HistoryBufferSize);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HistoryResizeBuffer(0x%p, %lu) failed, Status 0x%08lx\n",
                    Hist, HistoryBufferSize, Status);
        }
    }
    Console->HistoryBufferSize = HistoryBufferSize;

    /* No duplicates */
    Console->HistoryNoDup = !!HistoryNoDup;
}


/* PUBLIC SERVER APIS *********************************************************/

/* API_NUMBER: ConsolepGetCommandHistory */
CON_API(SrvGetConsoleCommandHistory,
        CONSOLE_GETCOMMANDHISTORY, GetCommandHistoryRequest)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesWritten = 0;
    PHISTORY_BUFFER Hist;

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

    return Status;
}

/* API_NUMBER: ConsolepGetCommandHistoryLength */
CON_API(SrvGetConsoleCommandHistoryLength,
        CONSOLE_GETCOMMANDHISTORYLENGTH, GetCommandHistoryLengthRequest)
{
    PHISTORY_BUFFER Hist;
    ULONG Length = 0;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&GetCommandHistoryLengthRequest->ExeName,
                                  GetCommandHistoryLengthRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

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

    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepExpungeCommandHistory */
CON_API(SrvExpungeConsoleCommandHistory,
        CONSOLE_EXPUNGECOMMANDHISTORY, ExpungeCommandHistoryRequest)
{
    PHISTORY_BUFFER Hist;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ExpungeCommandHistoryRequest->ExeName,
                                  ExpungeCommandHistoryRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Hist = HistoryFindBuffer(Console,
                             ExpungeCommandHistoryRequest->ExeName,
                             ExpungeCommandHistoryRequest->ExeLength,
                             ExpungeCommandHistoryRequest->Unicode2);
    HistoryDeleteBuffer(Hist);

    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetNumberOfCommands */
CON_API(SrvSetConsoleNumberOfCommands,
        CONSOLE_SETHISTORYNUMBERCOMMANDS, SetHistoryNumberCommandsRequest)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PHISTORY_BUFFER Hist;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&SetHistoryNumberCommandsRequest->ExeName,
                                  SetHistoryNumberCommandsRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Hist = HistoryFindBuffer(Console,
                             SetHistoryNumberCommandsRequest->ExeName,
                             SetHistoryNumberCommandsRequest->ExeLength,
                             SetHistoryNumberCommandsRequest->Unicode2);
    if (Hist)
    {
        Status = HistoryResizeBuffer(Hist, SetHistoryNumberCommandsRequest->NumCommands);
    }

    return Status;
}

/* API_NUMBER: ConsolepGetHistory */
CON_API_NOCONSOLE(SrvGetConsoleHistory,
                  CONSOLE_GETSETHISTORYINFO, HistoryInfoRequest)
{
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    NTSTATUS Status;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ProcessData,
                              &Console, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    HistoryInfoRequest->HistoryBufferSize      = Console->HistoryBufferSize;
    HistoryInfoRequest->NumberOfHistoryBuffers = Console->MaxNumberOfHistoryBuffers;
    HistoryInfoRequest->dwFlags                = (Console->HistoryNoDup ? HISTORY_NO_DUP_FLAG : 0);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
#else
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
#endif
}

/* API_NUMBER: ConsolepSetHistory */
CON_API_NOCONSOLE(SrvSetConsoleHistory,
                  CONSOLE_GETSETHISTORYINFO, HistoryInfoRequest)
{
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    NTSTATUS Status;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ProcessData,
                              &Console, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    HistoryReshapeAllBuffers(Console,
                             HistoryInfoRequest->HistoryBufferSize,
                             HistoryInfoRequest->NumberOfHistoryBuffers,
                             !!(HistoryInfoRequest->dwFlags & HISTORY_NO_DUP_FLAG));

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
#else
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
#endif
}

/* API_NUMBER: ConsolepSetCommandHistoryMode */
CON_API(SrvSetConsoleCommandHistoryMode,
        CONSOLE_SETHISTORYMODE, SetHistoryModeRequest)
{
    DPRINT("SrvSetConsoleCommandHistoryMode(Mode = %d) is not yet implemented\n",
            SetHistoryModeRequest->Mode);

    Console->InsertMode = !!(SetHistoryModeRequest->Mode & CONSOLE_OVERSTRIKE);
    return STATUS_SUCCESS;
}

/* EOF */

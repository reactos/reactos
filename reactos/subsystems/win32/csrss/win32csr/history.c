/*
 * PROJECT:         ReactOS CSRSS
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/csrss/win32csr/history.c
 * PURPOSE:         Console input history functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include "w32csr.h"
#include <debug.h>

typedef struct tagHISTORY_BUFFER
{
    LIST_ENTRY ListEntry;
    WORD MaxEntries;
    WORD NumEntries;
    PUNICODE_STRING Entries;
    UNICODE_STRING ExeName;
} HISTORY_BUFFER, *PHISTORY_BUFFER;

/* FUNCTIONS *****************************************************************/

static PHISTORY_BUFFER
HistoryGetBuffer(PCSRSS_CONSOLE Console)
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

VOID FASTCALL
HistoryAddEntry(PCSRSS_CONSOLE Console)
{
    UNICODE_STRING NewEntry;
    PHISTORY_BUFFER Hist;
    INT i;

    NewEntry.Length = NewEntry.MaximumLength = Console->LineSize * sizeof(WCHAR);
    NewEntry.Buffer = Console->LineBuffer;

    if (!(Hist = HistoryGetBuffer(Console)))
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
        if (Hist)
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
                    RtlFreeUnicodeString(Hist->Entries++);

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

/* EOF */

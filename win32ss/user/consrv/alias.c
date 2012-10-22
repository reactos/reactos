/* $Id: init.c 31400 2007-12-22 17:18:32Z fireball $
 * PROJECT:     ReactOS CSRSS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsystems/win32/csrss/api/alias.c
 * PURPOSE:     CSRSS alias support functions
 * COPYRIGHT:   Christoph Wittich
 *              Johannes Anderwald
 *
 */


/* INCLUDES ******************************************************************/

#define NDEBUG
#include "w32csr.h"
#include <debug.h>

typedef struct tagALIAS_ENTRY
{
    LPCWSTR lpSource;
    LPCWSTR lpTarget;
    struct tagALIAS_ENTRY * Next;
} ALIAS_ENTRY, *PALIAS_ENTRY;


typedef struct tagALIAS_HEADER
{
    LPCWSTR lpExeName;
    PALIAS_ENTRY Data;
    struct tagALIAS_HEADER * Next;

} ALIAS_HEADER, *PALIAS_HEADER;

static
PALIAS_HEADER
IntFindAliasHeader(PALIAS_HEADER RootHeader, LPCWSTR lpExeName)
{
    while(RootHeader)
    {
        INT diff = _wcsicmp(RootHeader->lpExeName, lpExeName);
        if (!diff)
            return RootHeader;

        if (diff > 0)
            break;

        RootHeader = RootHeader->Next;
    }
    return NULL;
}

PALIAS_HEADER
IntCreateAliasHeader(LPCWSTR lpExeName)
{
    PALIAS_HEADER Entry;
    UINT dwLength = wcslen(lpExeName) + 1;

    Entry = RtlAllocateHeap(ConSrvHeap, 0, sizeof(ALIAS_HEADER) + sizeof(WCHAR) * dwLength);
    if (!Entry)
        return Entry;

    Entry->lpExeName = (LPCWSTR)(Entry + 1);
    wcscpy((WCHAR*)Entry->lpExeName, lpExeName);
    Entry->Data = NULL;
    Entry->Next = NULL;
    return Entry;
}

VOID
IntInsertAliasHeader(PALIAS_HEADER * RootHeader, PALIAS_HEADER NewHeader)
{
    PALIAS_HEADER CurrentHeader;
    PALIAS_HEADER *LastLink = RootHeader;

    while ((CurrentHeader = *LastLink) != NULL)
    {
        INT Diff = _wcsicmp(NewHeader->lpExeName, CurrentHeader->lpExeName);
        if (Diff < 0)
        {
            break;
        }
        LastLink = &CurrentHeader->Next;
    }

    *LastLink = NewHeader;
    NewHeader->Next = CurrentHeader;
}

PALIAS_ENTRY
IntGetAliasEntry(PALIAS_HEADER Header, LPCWSTR lpSrcName)
{
    PALIAS_ENTRY RootHeader;

    if (Header == NULL)
        return NULL;

    RootHeader = Header->Data;
    while(RootHeader)
    {
        INT diff;
        DPRINT("IntGetAliasEntry>lpSource %S\n", RootHeader->lpSource);
        diff = _wcsicmp(RootHeader->lpSource, lpSrcName);
        if (!diff)
            return RootHeader;

        if (diff > 0)
            break;

        RootHeader = RootHeader->Next;
    }
    return NULL;
}


VOID
IntInsertAliasEntry(PALIAS_HEADER Header, PALIAS_ENTRY NewEntry)
{
    PALIAS_ENTRY CurrentEntry;
    PALIAS_ENTRY *LastLink = &Header->Data;

    while ((CurrentEntry = *LastLink) != NULL)
    {
        INT Diff = _wcsicmp(NewEntry->lpSource, CurrentEntry->lpSource);
        if (Diff < 0)
        {
            break;
        }
        LastLink = &CurrentEntry->Next;
    }

    *LastLink = NewEntry;
    NewEntry->Next = CurrentEntry;
}

PALIAS_ENTRY
IntCreateAliasEntry(LPCWSTR lpSource, LPCWSTR lpTarget)
{
    UINT dwSource;
    UINT dwTarget;
    PALIAS_ENTRY Entry;

    dwSource = wcslen(lpSource) + 1;
    dwTarget = wcslen(lpTarget) + 1;

    Entry = RtlAllocateHeap(ConSrvHeap, 0, sizeof(ALIAS_ENTRY) + sizeof(WCHAR) * (dwSource + dwTarget));
    if (!Entry)
        return Entry;

    Entry->lpSource = (LPCWSTR)(Entry + 1);
    wcscpy((LPWSTR)Entry->lpSource, lpSource);
    Entry->lpTarget = Entry->lpSource + dwSource;
    wcscpy((LPWSTR)Entry->lpTarget, lpTarget);
    Entry->Next = NULL;

    return Entry;
}

UINT
IntGetConsoleAliasesExesLength(PALIAS_HEADER RootHeader)
{
    UINT length = 0;

    while(RootHeader)
    {
        length += (wcslen(RootHeader->lpExeName) + 1) * sizeof(WCHAR);
        RootHeader = RootHeader->Next;
    }
    if (length)
        length += sizeof(WCHAR); // last entry entry is terminated with 2 zero bytes

    return length;
}

UINT
IntGetConsoleAliasesExes(PALIAS_HEADER RootHeader, LPWSTR TargetBuffer, UINT TargetBufferSize)
{
    UINT Offset = 0;
    UINT Length;

    TargetBufferSize /= sizeof(WCHAR);
    while(RootHeader)
    {
        Length = wcslen(RootHeader->lpExeName) + 1;
        if (TargetBufferSize > Offset + Length)
        {
            wcscpy(&TargetBuffer[Offset], RootHeader->lpExeName);
            Offset += Length;
        }
        else
        {
            break;
        }
        RootHeader = RootHeader->Next;
    }
    Length = min(Offset+1, TargetBufferSize);
    TargetBuffer[Length] = L'\0';
    return Length * sizeof(WCHAR);
}

UINT
IntGetAllConsoleAliasesLength(PALIAS_HEADER Header)
{
    UINT Length = 0;
    PALIAS_ENTRY CurEntry = Header->Data;

    while(CurEntry)
    {
        Length += wcslen(CurEntry->lpSource);
        Length += wcslen(CurEntry->lpTarget);
        Length += 2; // zero byte and '='
        CurEntry = CurEntry->Next;
    }

    if (Length)
    {
        return (Length+1) * sizeof(WCHAR);
    }
    return 0;
}
UINT
IntGetAllConsoleAliases(PALIAS_HEADER Header, LPWSTR TargetBuffer, UINT TargetBufferLength)
{
    PALIAS_ENTRY CurEntry = Header->Data;
    UINT Offset = 0;
    UINT SrcLength, TargetLength;

    TargetBufferLength /= sizeof(WCHAR);
    while(CurEntry)
    {
        SrcLength = wcslen(CurEntry->lpSource) + 1;
        TargetLength = wcslen(CurEntry->lpTarget) + 1;
        if (Offset + TargetLength + SrcLength >= TargetBufferLength)
            break;

        wcscpy(&TargetBuffer[Offset], CurEntry->lpSource);
        Offset += SrcLength;
        TargetBuffer[Offset] = L'=';
        wcscpy(&TargetBuffer[Offset], CurEntry->lpTarget);
        Offset += TargetLength;

        CurEntry = CurEntry->Next;
    }
    TargetBuffer[Offset] = L'\0';
    return Offset * sizeof(WCHAR);
}
VOID
IntDeleteAliasEntry(PALIAS_HEADER Header, PALIAS_ENTRY Entry)
{
    PALIAS_ENTRY *LastLink = &Header->Data;
    PALIAS_ENTRY CurEntry;

    while ((CurEntry = *LastLink) != NULL)
    {
        if (CurEntry == Entry)
        {
            *LastLink = Entry->Next;
            RtlFreeHeap(ConSrvHeap, 0, Entry);
            return;
        }
        LastLink = &CurEntry->Next;
    }
}
VOID
IntDeleteAllAliases(PALIAS_HEADER RootHeader)
{
    PALIAS_HEADER Header, NextHeader;
    PALIAS_ENTRY Entry, NextEntry;
    for (Header = RootHeader; Header; Header = NextHeader)
    {
        NextHeader = Header->Next;
        for (Entry = Header->Data; Entry; Entry = NextEntry)
        {
            NextEntry = Entry->Next;
            RtlFreeHeap(ConSrvHeap, 0, Entry);
        }
        RtlFreeHeap(ConSrvHeap, 0, Header);
    }
}

CSR_API(SrvAddConsoleAlias)
{
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    WCHAR * lpExeName;
    WCHAR * lpSource;
    WCHAR * lpTarget;
    //ULONG TotalLength;
    //WCHAR * Ptr;

    //TotalLength = ApiMessage->Data.AddConsoleAlias.SourceLength + ApiMessage->Data.AddConsoleAlias.ExeLength + ApiMessage->Data.AddConsoleAlias.TargetLength;
    //Ptr = (WCHAR*)((ULONG_PTR)ApiMessage + sizeof(CSR_API_MESSAGE));

    lpSource = (WCHAR*)((ULONG_PTR)ApiMessage + sizeof(CSR_API_MESSAGE));
    lpExeName = (WCHAR*)((ULONG_PTR)ApiMessage + sizeof(CSR_API_MESSAGE) + ApiMessage->Data.AddConsoleAlias.SourceLength * sizeof(WCHAR));
    lpTarget = (ApiMessage->Data.AddConsoleAlias.TargetLength != 0 ? lpExeName + ApiMessage->Data.AddConsoleAlias.ExeLength : NULL);

    DPRINT("SrvAddConsoleAlias entered ApiMessage %p lpSource %p lpExeName %p lpTarget %p\n", ApiMessage, lpSource, lpExeName, lpTarget);

    if (lpExeName == NULL || lpSource == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(CsrGetClientThread()->Process, &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, lpExeName);
    if (!Header && lpTarget != NULL)
    {
        Header = IntCreateAliasHeader(lpExeName);
        if (!Header)
        {
            ConioUnlockConsole(Console);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        IntInsertAliasHeader(&Console->Aliases, Header);
    }

    if (lpTarget == NULL) // delete the entry
    {
        Entry = IntGetAliasEntry(Header, lpSource);
        if (Entry)
        {
            IntDeleteAliasEntry(Header, Entry);
            ApiMessage->Status = STATUS_SUCCESS;
        }
        else
        {
            ApiMessage->Status = STATUS_INVALID_PARAMETER;
        }
        ConioUnlockConsole(Console);
        return ApiMessage->Status;
    }

    Entry = IntCreateAliasEntry(lpSource, lpTarget);

    if (!Entry)
    {
        ConioUnlockConsole(Console);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IntInsertAliasEntry(Header, Entry);
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAlias)
{
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    UINT Length;
    WCHAR * lpExeName;
    WCHAR * lpSource;
    WCHAR * lpTarget;

    lpSource = (LPWSTR)((ULONG_PTR)ApiMessage + sizeof(CSR_API_MESSAGE));
    lpExeName = lpSource + ApiMessage->Data.GetConsoleAlias.SourceLength;
    lpTarget = ApiMessage->Data.GetConsoleAlias.TargetBuffer;


    DPRINT("SrvGetConsoleAlias entered lpExeName %p lpSource %p TargetBuffer %p TargetBufferLength %u\n",
           lpExeName, lpSource, lpTarget, ApiMessage->Data.GetConsoleAlias.TargetBufferLength);

    if (ApiMessage->Data.GetConsoleAlias.ExeLength == 0 || lpTarget == NULL ||
            ApiMessage->Data.GetConsoleAlias.TargetBufferLength == 0 || ApiMessage->Data.GetConsoleAlias.SourceLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(CsrGetClientThread()->Process, &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, lpExeName);
    if (!Header)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    Entry = IntGetAliasEntry(Header, lpSource);
    if (!Entry)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    Length = (wcslen(Entry->lpTarget)+1) * sizeof(WCHAR);
    if (Length > ApiMessage->Data.GetConsoleAlias.TargetBufferLength)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Win32CsrValidateBuffer(CsrGetClientThread()->Process, lpTarget,
                                ApiMessage->Data.GetConsoleAlias.TargetBufferLength, 1))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    wcscpy(lpTarget, Entry->lpTarget);
    ApiMessage->Data.GetConsoleAlias.BytesWritten = Length;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliases)
{
    PCSRSS_CONSOLE Console;
    ULONG BytesWritten;
    PALIAS_HEADER Header;

    if (ApiMessage->Data.GetAllConsoleAlias.lpExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(CsrGetClientThread()->Process, &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, ApiMessage->Data.GetAllConsoleAlias.lpExeName);
    if (!Header)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    if (IntGetAllConsoleAliasesLength(Header) > ApiMessage->Data.GetAllConsoleAlias.AliasBufferLength)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (!Win32CsrValidateBuffer(CsrGetClientThread()->Process,
                                ApiMessage->Data.GetAllConsoleAlias.AliasBuffer,
                                ApiMessage->Data.GetAllConsoleAlias.AliasBufferLength,
                                1))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    BytesWritten = IntGetAllConsoleAliases(Header,
                                           ApiMessage->Data.GetAllConsoleAlias.AliasBuffer,
                                           ApiMessage->Data.GetAllConsoleAlias.AliasBufferLength);

    ApiMessage->Data.GetAllConsoleAlias.BytesWritten = BytesWritten;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasesLength)
{
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    UINT Length;

    if (ApiMessage->Data.GetAllConsoleAliasesLength.lpExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(CsrGetClientThread()->Process, &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, ApiMessage->Data.GetAllConsoleAliasesLength.lpExeName);
    if (!Header)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    Length = IntGetAllConsoleAliasesLength(Header);
    ApiMessage->Data.GetAllConsoleAliasesLength.Length = Length;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasExes)
{
    PCSRSS_CONSOLE Console;
    UINT BytesWritten;
    UINT ExesLength;

    DPRINT("SrvGetConsoleAliasExes entered\n");

    ApiMessage->Status = ConioConsoleFromProcessData(CsrGetClientThread()->Process, &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    ExesLength = IntGetConsoleAliasesExesLength(Console->Aliases);

    if (ExesLength > ApiMessage->Data.GetConsoleAliasesExes.Length)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (ApiMessage->Data.GetConsoleAliasesExes.ExeNames == NULL)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    if (!Win32CsrValidateBuffer(CsrGetClientThread()->Process,
                                ApiMessage->Data.GetConsoleAliasesExes.ExeNames,
                                ApiMessage->Data.GetConsoleAliasesExes.Length,
                                1))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    BytesWritten = IntGetConsoleAliasesExes(Console->Aliases,
                                            ApiMessage->Data.GetConsoleAliasesExes.ExeNames,
                                            ApiMessage->Data.GetConsoleAliasesExes.Length);

    ApiMessage->Data.GetConsoleAliasesExes.BytesWritten = BytesWritten;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasExesLength)
{
    PCSRSS_CONSOLE Console;
    DPRINT("SrvGetConsoleAliasExesLength entered\n");

    ApiMessage->Status = ConioConsoleFromProcessData(CsrGetClientThread()->Process, &Console);
    if (NT_SUCCESS(ApiMessage->Status))
    {
        ApiMessage->Data.GetConsoleAliasesExesLength.Length = IntGetConsoleAliasesExesLength(Console->Aliases);
        ConioUnlockConsole(Console);
    }
    return ApiMessage->Status;
}

/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/alias.c
 * PURPOSE:         Alias support functions
 * PROGRAMMERS:     Christoph Wittich
 *                  Johannes Anderwald
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "conio.h"

#define NDEBUG
#include <debug.h>


/* TYPES **********************************************************************/

typedef struct tagALIAS_ENTRY
{
    LPCWSTR lpSource;
    LPCWSTR lpTarget;
    struct tagALIAS_ENTRY* Next;
} ALIAS_ENTRY, *PALIAS_ENTRY;

typedef struct tagALIAS_HEADER
{
    LPCWSTR lpExeName;
    PALIAS_ENTRY Data;
    struct tagALIAS_HEADER* Next;
} ALIAS_HEADER, *PALIAS_HEADER;


/* PRIVATE FUNCTIONS **********************************************************/

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
        DPRINT("IntGetAliasEntry->lpSource %S\n", RootHeader->lpSource);
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


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvAddConsoleAlias)
{
    PCSRSS_CONSOLE_ALIAS ConsoleAlias = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleAlias;
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    LPWSTR lpSource, lpTarget, lpExeName;

    DPRINT("SrvAddConsoleAlias entered ApiMessage %p\n", ApiMessage);

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAlias->Source,
                                   ConsoleAlias->SourceLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAlias->Target,
                                   ConsoleAlias->TargetLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAlias->Exe,
                                   ConsoleAlias->ExeLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    lpSource  = ConsoleAlias->Source;
    lpTarget  = (ConsoleAlias->TargetLength != 0 ? ConsoleAlias->Target : NULL);
    lpExeName = ConsoleAlias->Exe;

    DPRINT("SrvAddConsoleAlias lpSource %p lpExeName %p lpTarget %p\n", lpSource, lpExeName, lpTarget);

    if (lpExeName == NULL || lpSource == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
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

    if (lpTarget == NULL) // Delete the entry
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
    PCSRSS_CONSOLE_ALIAS ConsoleAlias = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleAlias;
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    UINT Length;
    LPWSTR lpSource, lpTarget, lpExeName;

    DPRINT("SrvGetConsoleAlias entered ApiMessage %p\n", ApiMessage);

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAlias->Source,
                                   ConsoleAlias->SourceLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAlias->Target,
                                   ConsoleAlias->TargetLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAlias->Exe,
                                   ConsoleAlias->ExeLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    lpSource  = ConsoleAlias->Source;
    lpTarget  = ConsoleAlias->Target;
    lpExeName = ConsoleAlias->Exe;

    DPRINT("SrvGetConsoleAlias lpExeName %p lpSource %p TargetBuffer %p TargetLength %u\n",
           lpExeName, lpSource, lpTarget, ConsoleAlias->TargetLength);

    if (ConsoleAlias->ExeLength == 0 || lpTarget == NULL ||
            ConsoleAlias->TargetLength == 0 || ConsoleAlias->SourceLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
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

    Length = (wcslen(Entry->lpTarget) + 1) * sizeof(WCHAR);
    if (Length > ConsoleAlias->TargetLength)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_TOO_SMALL;
    }

/*
    if (!Win32CsrValidateBuffer(CsrGetClientThread()->Process, lpTarget,
                                ConsoleAlias->TargetLength, 1))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }
*/

    wcscpy(lpTarget, Entry->lpTarget);
    ConsoleAlias->TargetLength = Length;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliases)
{
    PCSRSS_GET_ALL_CONSOLE_ALIASES GetAllConsoleAliases = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAllConsoleAliases;
    PCSRSS_CONSOLE Console;
    ULONG BytesWritten;
    PALIAS_HEADER Header;

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID)&GetAllConsoleAliases->ExeName,
                                   GetAllConsoleAliases->ExeLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID)&GetAllConsoleAliases->AliasesBuffer,
                                   GetAllConsoleAliases->AliasesBufferLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (GetAllConsoleAliases->ExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, GetAllConsoleAliases->ExeName);
    if (!Header)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    if (IntGetAllConsoleAliasesLength(Header) > GetAllConsoleAliases->AliasesBufferLength)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_OVERFLOW;
    }

/*
    if (!Win32CsrValidateBuffer(CsrGetClientThread()->Process,
                                GetAllConsoleAliases->AliasesBuffer,
                                GetAllConsoleAliases->AliasesBufferLength,
                                1))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }
*/

    BytesWritten = IntGetAllConsoleAliases(Header,
                                           GetAllConsoleAliases->AliasesBuffer,
                                           GetAllConsoleAliases->AliasesBufferLength);

    GetAllConsoleAliases->AliasesBufferLength = BytesWritten;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasesLength)
{
    PCSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH GetAllConsoleAliasesLength = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAllConsoleAliasesLength;
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    UINT Length;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetAllConsoleAliasesLength->ExeName,
                                  GetAllConsoleAliasesLength->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (GetAllConsoleAliasesLength->ExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, GetAllConsoleAliasesLength->ExeName);
    if (!Header)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    Length = IntGetAllConsoleAliasesLength(Header);
    GetAllConsoleAliasesLength->Length = Length;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasExes)
{
    PCSRSS_GET_CONSOLE_ALIASES_EXES GetConsoleAliasesExes = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetConsoleAliasesExes;
    PCSRSS_CONSOLE Console;
    UINT BytesWritten;
    UINT ExesLength;

    DPRINT("SrvGetConsoleAliasExes entered\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetConsoleAliasesExes->ExeNames,
                                  GetConsoleAliasesExes->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    ExesLength = IntGetConsoleAliasesExesLength(Console->Aliases);

    if (ExesLength > GetConsoleAliasesExes->Length)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (GetConsoleAliasesExes->ExeNames == NULL)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

/*
    if (!Win32CsrValidateBuffer(CsrGetClientThread()->Process,
                                GetConsoleAliasesExes->ExeNames,
                                GetConsoleAliasesExes->Length,
                                1))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }
*/

    BytesWritten = IntGetConsoleAliasesExes(Console->Aliases,
                                            GetConsoleAliasesExes->ExeNames,
                                            GetConsoleAliasesExes->Length);

    GetConsoleAliasesExes->Length = BytesWritten;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasExesLength)
{
    PCSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH GetConsoleAliasesExesLength = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetConsoleAliasesExesLength;
    PCSRSS_CONSOLE Console;
    DPRINT("SrvGetConsoleAliasExesLength entered\n");

    ApiMessage->Status = ConioConsoleFromProcessData(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console);
    if (NT_SUCCESS(ApiMessage->Status))
    {
        GetConsoleAliasesExesLength->Length = IntGetConsoleAliasesExesLength(Console->Aliases);
        ConioUnlockConsole(Console);
    }
    return ApiMessage->Status;
}

/* EOF */

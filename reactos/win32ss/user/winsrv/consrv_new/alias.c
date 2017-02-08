/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/alias.c
 * PURPOSE:         Alias support functions
 * PROGRAMMERS:     Christoph Wittich
 *                  Johannes Anderwald
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "console.h"
#include "include/conio.h"

#define NDEBUG
#include <debug.h>


/* TYPES **********************************************************************/

typedef struct _ALIAS_ENTRY
{
    LPCWSTR lpSource;
    LPCWSTR lpTarget;
    struct _ALIAS_ENTRY* Next;
} ALIAS_ENTRY, *PALIAS_ENTRY;

typedef struct _ALIAS_HEADER
{
    LPCWSTR lpExeName;
    PALIAS_ENTRY Data;
    struct _ALIAS_HEADER* Next;
} ALIAS_HEADER, *PALIAS_HEADER;


/* PRIVATE FUNCTIONS **********************************************************/

static
PALIAS_HEADER
IntFindAliasHeader(PALIAS_HEADER RootHeader, LPCWSTR lpExeName)
{
    while (RootHeader)
    {
        INT diff = _wcsicmp(RootHeader->lpExeName, lpExeName);
        if (!diff) return RootHeader;
        if (diff > 0) break;

        RootHeader = RootHeader->Next;
    }
    return NULL;
}

PALIAS_HEADER
IntCreateAliasHeader(LPCWSTR lpExeName)
{
    PALIAS_HEADER Entry;
    UINT dwLength = wcslen(lpExeName) + 1;

    Entry = ConsoleAllocHeap(0, sizeof(ALIAS_HEADER) + sizeof(WCHAR) * dwLength);
    if (!Entry) return Entry;

    Entry->lpExeName = (LPCWSTR)(Entry + 1);
    wcscpy((PWCHAR)Entry->lpExeName, lpExeName);
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
        if (Diff < 0) break;

        LastLink = &CurrentHeader->Next;
    }

    *LastLink = NewHeader;
    NewHeader->Next = CurrentHeader;
}

PALIAS_ENTRY
IntGetAliasEntry(PALIAS_HEADER Header, LPCWSTR lpSrcName)
{
    PALIAS_ENTRY RootHeader;

    if (Header == NULL) return NULL;

    RootHeader = Header->Data;
    while (RootHeader)
    {
        INT diff;
        DPRINT("IntGetAliasEntry->lpSource %S\n", RootHeader->lpSource);
        diff = _wcsicmp(RootHeader->lpSource, lpSrcName);
        if (!diff) return RootHeader;
        if (diff > 0) break;

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
        if (Diff < 0) break;

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

    Entry = ConsoleAllocHeap(0, sizeof(ALIAS_ENTRY) + sizeof(WCHAR) * (dwSource + dwTarget));
    if (!Entry) return Entry;

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

    while (RootHeader)
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
    while (RootHeader)
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

    while (CurEntry)
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
    while (CurEntry)
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
            ConsoleFreeHeap(Entry);
            return;
        }
        LastLink = &CurEntry->Next;
    }
}

VOID
IntDeleteAllAliases(PCONSOLE Console)
{
    PALIAS_HEADER Header, NextHeader;
    PALIAS_ENTRY Entry, NextEntry;

    for (Header = Console->Aliases; Header; Header = NextHeader)
    {
        NextHeader = Header->Next;
        for (Entry = Header->Data; Entry; Entry = NextEntry)
        {
            NextEntry = Entry->Next;
            ConsoleFreeHeap(Entry);
        }
        ConsoleFreeHeap(Header);
    }
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvAddConsoleAlias)
{
    PCONSOLE_ADDGETALIAS ConsoleAliasRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleAliasRequest;
    PCONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    LPWSTR lpSource, lpTarget, lpExeName;

    DPRINT("SrvAddConsoleAlias entered ApiMessage %p\n", ApiMessage);

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAliasRequest->Source,
                                   ConsoleAliasRequest->SourceLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAliasRequest->Target,
                                   ConsoleAliasRequest->TargetLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAliasRequest->Exe,
                                   ConsoleAliasRequest->ExeLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    lpSource  = ConsoleAliasRequest->Source;
    lpTarget  = (ConsoleAliasRequest->TargetLength != 0 ? ConsoleAliasRequest->Target : NULL);
    lpExeName = ConsoleAliasRequest->Exe;

    DPRINT("SrvAddConsoleAlias lpSource %p lpExeName %p lpTarget %p\n", lpSource, lpExeName, lpTarget);

    if (lpExeName == NULL || lpSource == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
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
            ConSrvReleaseConsole(Console, TRUE);
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
        ConSrvReleaseConsole(Console, TRUE);
        return ApiMessage->Status;
    }

    Entry = IntCreateAliasEntry(lpSource, lpTarget);

    if (!Entry)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IntInsertAliasEntry(Header, Entry);
    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAlias)
{
    PCONSOLE_ADDGETALIAS ConsoleAliasRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleAliasRequest;
    PCONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    UINT Length;
    LPWSTR lpSource, lpTarget, lpExeName;

    DPRINT("SrvGetConsoleAlias entered ApiMessage %p\n", ApiMessage);

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAliasRequest->Source,
                                   ConsoleAliasRequest->SourceLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAliasRequest->Target,
                                   ConsoleAliasRequest->TargetLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID*)&ConsoleAliasRequest->Exe,
                                   ConsoleAliasRequest->ExeLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    lpSource  = ConsoleAliasRequest->Source;
    lpTarget  = ConsoleAliasRequest->Target;
    lpExeName = ConsoleAliasRequest->Exe;

    DPRINT("SrvGetConsoleAlias lpExeName %p lpSource %p TargetBuffer %p TargetLength %u\n",
           lpExeName, lpSource, lpTarget, ConsoleAliasRequest->TargetLength);

    if (ConsoleAliasRequest->ExeLength == 0 || lpTarget == NULL ||
            ConsoleAliasRequest->TargetLength == 0 || ConsoleAliasRequest->SourceLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, lpExeName);
    if (!Header)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_INVALID_PARAMETER;
    }

    Entry = IntGetAliasEntry(Header, lpSource);
    if (!Entry)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_INVALID_PARAMETER;
    }

    Length = (wcslen(Entry->lpTarget) + 1) * sizeof(WCHAR);
    if (Length > ConsoleAliasRequest->TargetLength)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    wcscpy(lpTarget, Entry->lpTarget);
    ConsoleAliasRequest->TargetLength = Length;
    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliases)
{
    PCONSOLE_GETALLALIASES GetAllAliasesRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAllAliasesRequest;
    PCONSOLE Console;
    ULONG BytesWritten;
    PALIAS_HEADER Header;

    if ( !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID)&GetAllAliasesRequest->ExeName,
                                   GetAllAliasesRequest->ExeLength,
                                   sizeof(BYTE))                    ||
         !CsrValidateMessageBuffer(ApiMessage,
                                   (PVOID)&GetAllAliasesRequest->AliasesBuffer,
                                   GetAllAliasesRequest->AliasesBufferLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (GetAllAliasesRequest->ExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, GetAllAliasesRequest->ExeName);
    if (!Header)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_INVALID_PARAMETER;
    }

    if (IntGetAllConsoleAliasesLength(Header) > GetAllAliasesRequest->AliasesBufferLength)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_BUFFER_OVERFLOW;
    }

    BytesWritten = IntGetAllConsoleAliases(Header,
                                           GetAllAliasesRequest->AliasesBuffer,
                                           GetAllAliasesRequest->AliasesBufferLength);

    GetAllAliasesRequest->AliasesBufferLength = BytesWritten;
    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasesLength)
{
    PCONSOLE_GETALLALIASESLENGTH GetAllAliasesLengthRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAllAliasesLengthRequest;
    PCONSOLE Console;
    PALIAS_HEADER Header;
    UINT Length;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetAllAliasesLengthRequest->ExeName,
                                  GetAllAliasesLengthRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (GetAllAliasesLengthRequest->ExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, GetAllAliasesLengthRequest->ExeName);
    if (!Header)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_INVALID_PARAMETER;
    }

    Length = IntGetAllConsoleAliasesLength(Header);
    GetAllAliasesLengthRequest->Length = Length;
    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasExes)
{
    PCONSOLE_GETALIASESEXES GetAliasesExesRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAliasesExesRequest;
    PCONSOLE Console;
    UINT BytesWritten;
    UINT ExesLength;

    DPRINT("SrvGetConsoleAliasExes entered\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetAliasesExesRequest->ExeNames,
                                  GetAliasesExesRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ApiMessage->Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(ApiMessage->Status))
    {
        return ApiMessage->Status;
    }

    ExesLength = IntGetConsoleAliasesExesLength(Console->Aliases);

    if (ExesLength > GetAliasesExesRequest->Length)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (GetAliasesExesRequest->ExeNames == NULL)
    {
        ConSrvReleaseConsole(Console, TRUE);
        return STATUS_INVALID_PARAMETER;
    }

    BytesWritten = IntGetConsoleAliasesExes(Console->Aliases,
                                            GetAliasesExesRequest->ExeNames,
                                            GetAliasesExesRequest->Length);

    GetAliasesExesRequest->Length = BytesWritten;
    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleAliasExesLength)
{
    PCONSOLE_GETALIASESEXESLENGTH GetAliasesExesLengthRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAliasesExesLengthRequest;
    PCONSOLE Console;
    DPRINT("SrvGetConsoleAliasExesLength entered\n");

    ApiMessage->Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (NT_SUCCESS(ApiMessage->Status))
    {
        GetAliasesExesLengthRequest->Length = IntGetConsoleAliasesExesLength(Console->Aliases);
        ConSrvReleaseConsole(Console, TRUE);
    }
    return ApiMessage->Status;
}

/* EOF */

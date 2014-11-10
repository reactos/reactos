/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/alias.c
 * PURPOSE:         Alias support functions
 * PROGRAMMERS:     Christoph Wittich
 *                  Johannes Anderwald
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

/* TYPES **********************************************************************/

typedef struct _ALIAS_ENTRY
{
    struct _ALIAS_ENTRY* Next;
    UNICODE_STRING Source;
    UNICODE_STRING Target;
} ALIAS_ENTRY, *PALIAS_ENTRY;

typedef struct _ALIAS_HEADER
{
    struct _ALIAS_HEADER* Next;
    UNICODE_STRING ExeName;
    PALIAS_ENTRY   Data;
} ALIAS_HEADER, *PALIAS_HEADER;




BOOLEAN
ConvertInputAnsiToUnicode(PCONSRV_CONSOLE Console,
                          PVOID    Source,
                          USHORT   SourceLength,
                          // BOOLEAN  IsUnicode,
                          PWCHAR*  Target,
                          PUSHORT  TargetLength)
{
    ASSERT(Source && Target && TargetLength);

    /* Use the console input CP for the conversion */
    *TargetLength = MultiByteToWideChar(Console->InputCodePage, 0,
                                        Source, SourceLength,
                                        NULL, 0);
    *Target = ConsoleAllocHeap(0, *TargetLength * sizeof(WCHAR));
    if (*Target == NULL) return FALSE;

    MultiByteToWideChar(Console->InputCodePage, 0,
                        Source, SourceLength,
                        *Target, *TargetLength);

    /* The returned Length was in number of WCHARs, convert it in bytes */
    *TargetLength *= sizeof(WCHAR);

    return TRUE;
}

BOOLEAN
ConvertInputUnicodeToAnsi(PCONSRV_CONSOLE Console,
                          PVOID    Source,
                          USHORT   SourceLength,
                          // BOOLEAN  IsAnsi,
                          PCHAR/* * */   Target,
                          /*P*/USHORT  TargetLength)
{
    ASSERT(Source && Target && TargetLength);

    /*
     * From MSDN:
     * "The lpMultiByteStr and lpWideCharStr pointers must not be the same.
     *  If they are the same, the function fails, and GetLastError returns
     *  ERROR_INVALID_PARAMETER."
     */
    ASSERT((ULONG_PTR)Source != (ULONG_PTR)Target);

    /* Use the console input CP for the conversion */
    // *TargetLength = WideCharToMultiByte(Console->InputCodePage, 0,
                                        // Source, SourceLength,
                                        // NULL, 0, NULL, NULL);
    // *Target = ConsoleAllocHeap(0, *TargetLength * sizeof(WCHAR));
    // if (*Target == NULL) return FALSE;

    WideCharToMultiByte(Console->InputCodePage, 0,
                        Source, SourceLength,
                        /* * */Target, /* * */TargetLength,
                        NULL, NULL);

    // /* The returned Length was in number of WCHARs, convert it in bytes */
    // *TargetLength *= sizeof(WCHAR);

    return TRUE;
}




/* PRIVATE FUNCTIONS **********************************************************/

static PALIAS_HEADER
IntFindAliasHeader(PCONSRV_CONSOLE Console,
                   PVOID    ExeName,
                   USHORT   ExeLength,
                   BOOLEAN  UnicodeExe)
{
    UNICODE_STRING ExeNameU;

    PALIAS_HEADER RootHeader = Console->Aliases;
    INT Diff;

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

    while (RootHeader)
    {
        Diff = RtlCompareUnicodeString(&RootHeader->ExeName, &ExeNameU, TRUE);
        if (!Diff)
        {
            if (!UnicodeExe) ConsoleFreeHeap(ExeNameU.Buffer);
            return RootHeader;
        }
        if (Diff > 0) break;

        RootHeader = RootHeader->Next;
    }

    if (!UnicodeExe) ConsoleFreeHeap(ExeNameU.Buffer);
    return NULL;
}

static PALIAS_HEADER
IntCreateAliasHeader(PCONSRV_CONSOLE Console,
                     PVOID    ExeName,
                     USHORT   ExeLength,
                     BOOLEAN  UnicodeExe)
{
    UNICODE_STRING ExeNameU;

    PALIAS_HEADER Entry;

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

    Entry = ConsoleAllocHeap(0, sizeof(ALIAS_HEADER) + ExeNameU.Length);
    if (!Entry)
    {
        if (!UnicodeExe) ConsoleFreeHeap(ExeNameU.Buffer);
        return Entry;
    }

    Entry->ExeName.Buffer = (PWSTR)(Entry + 1);
    Entry->ExeName.Length = 0;
    Entry->ExeName.MaximumLength = ExeNameU.Length;
    RtlCopyUnicodeString(&Entry->ExeName, &ExeNameU);

    Entry->Data = NULL;
    Entry->Next = NULL;

    if (!UnicodeExe) ConsoleFreeHeap(ExeNameU.Buffer);
    return Entry;
}

static VOID
IntInsertAliasHeader(PALIAS_HEADER* RootHeader,
                     PALIAS_HEADER  NewHeader)
{
    PALIAS_HEADER CurrentHeader;
    PALIAS_HEADER *LastLink = RootHeader;
    INT Diff;

    while ((CurrentHeader = *LastLink) != NULL)
    {
        Diff = RtlCompareUnicodeString(&NewHeader->ExeName, &CurrentHeader->ExeName, TRUE);
        if (Diff < 0) break;

        LastLink = &CurrentHeader->Next;
    }

    *LastLink = NewHeader;
    NewHeader->Next = CurrentHeader;
}

static PALIAS_ENTRY
IntGetAliasEntry(PCONSRV_CONSOLE Console,
                 PALIAS_HEADER Header,
                 PVOID    Source,
                 USHORT   SourceLength,
                 BOOLEAN  Unicode)
{
    UNICODE_STRING SourceU;

    PALIAS_ENTRY Entry;
    INT Diff;

    if (Header == NULL || Source == NULL) return NULL;

    if (Unicode)
    {
        SourceU.Buffer = Source;
        /* Length is in bytes */
        SourceU.MaximumLength = SourceLength;
    }
    else
    {
        if (!ConvertInputAnsiToUnicode(Console,
                                       Source, SourceLength,
                                       &SourceU.Buffer, &SourceU.MaximumLength))
        {
            return NULL;
        }
    }
    SourceU.Length = SourceU.MaximumLength;

    Entry = Header->Data;
    while (Entry)
    {
        Diff = RtlCompareUnicodeString(&Entry->Source, &SourceU, TRUE);
        if (!Diff)
        {
            if (!Unicode) ConsoleFreeHeap(SourceU.Buffer);
            return Entry;
        }
        if (Diff > 0) break;

        Entry = Entry->Next;
    }

    if (!Unicode) ConsoleFreeHeap(SourceU.Buffer);
    return NULL;
}

static PALIAS_ENTRY
IntCreateAliasEntry(PCONSRV_CONSOLE Console,
                    PVOID    Source,
                    USHORT   SourceLength,
                    PVOID    Target,
                    USHORT   TargetLength,
                    BOOLEAN  Unicode)
{
    UNICODE_STRING SourceU;
    UNICODE_STRING TargetU;

    PALIAS_ENTRY Entry;

    if (Unicode)
    {
        SourceU.Buffer = Source;
        TargetU.Buffer = Target;
        /* Length is in bytes */
        SourceU.MaximumLength = SourceLength;
        TargetU.MaximumLength = TargetLength;
    }
    else
    {
        if (!ConvertInputAnsiToUnicode(Console,
                                       Source, SourceLength,
                                       &SourceU.Buffer, &SourceU.MaximumLength))
        {
            return NULL;
        }

        if (!ConvertInputAnsiToUnicode(Console,
                                       Target, TargetLength,
                                       &TargetU.Buffer, &TargetU.MaximumLength))
        {
            ConsoleFreeHeap(SourceU.Buffer);
            return NULL;
        }
    }
    SourceU.Length = SourceU.MaximumLength;
    TargetU.Length = TargetU.MaximumLength;

    Entry = ConsoleAllocHeap(0, sizeof(ALIAS_ENTRY) +
                                SourceU.Length + TargetU.Length);
    if (!Entry)
    {
        if (!Unicode)
        {
            ConsoleFreeHeap(TargetU.Buffer);
            ConsoleFreeHeap(SourceU.Buffer);
        }
        return Entry;
    }

    Entry->Source.Buffer = (PWSTR)(Entry + 1);
    Entry->Source.Length = 0;
    Entry->Source.MaximumLength = SourceU.Length;
    RtlCopyUnicodeString(&Entry->Source, &SourceU);

    Entry->Target.Buffer = (PWSTR)((ULONG_PTR)Entry->Source.Buffer + Entry->Source.MaximumLength);
    Entry->Target.Length = 0;
    Entry->Target.MaximumLength = TargetU.Length;
    RtlCopyUnicodeString(&Entry->Target, &TargetU);

    Entry->Next = NULL;

    if (!Unicode)
    {
        ConsoleFreeHeap(TargetU.Buffer);
        ConsoleFreeHeap(SourceU.Buffer);
    }
    return Entry;
}

static VOID
IntInsertAliasEntry(PALIAS_HEADER Header,
                    PALIAS_ENTRY  NewEntry)
{
    PALIAS_ENTRY CurrentEntry;
    PALIAS_ENTRY *LastLink = &Header->Data;
    INT Diff;

    while ((CurrentEntry = *LastLink) != NULL)
    {
        Diff = RtlCompareUnicodeString(&NewEntry->Source, &CurrentEntry->Source, TRUE);
        if (Diff < 0) break;

        LastLink = &CurrentEntry->Next;
    }

    *LastLink = NewEntry;
    NewEntry->Next = CurrentEntry;
}

static VOID
IntDeleteAliasEntry(PALIAS_HEADER Header,
                    PALIAS_ENTRY  Entry)
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

static UINT
IntGetConsoleAliasesExesLength(PALIAS_HEADER RootHeader,
                               BOOLEAN IsUnicode)
{
    UINT Length = 0;

    while (RootHeader)
    {
        Length += RootHeader->ExeName.Length + sizeof(WCHAR); // NULL-termination
        RootHeader = RootHeader->Next;
    }

    /*
     * Quick and dirty way of getting the number of bytes of the
     * corresponding ANSI string from the one in UNICODE.
     */
    if (!IsUnicode)
        Length /= sizeof(WCHAR);

    return Length;
}

static UINT
IntGetAllConsoleAliasesLength(PALIAS_HEADER Header,
                              BOOLEAN IsUnicode)
{
    UINT Length = 0;
    PALIAS_ENTRY CurEntry = Header->Data;

    while (CurEntry)
    {
        Length += CurEntry->Source.Length;
        Length += CurEntry->Target.Length;
        Length += 2 * sizeof(WCHAR); // '=' and NULL-termination
        CurEntry = CurEntry->Next;
    }

    /*
     * Quick and dirty way of getting the number of bytes of the
     * corresponding ANSI string from the one in UNICODE.
     */
    if (!IsUnicode)
        Length /= sizeof(WCHAR);

    return Length;
}

VOID
IntDeleteAllAliases(PCONSRV_CONSOLE Console)
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
    NTSTATUS Status;
    PCONSOLE_ADDGETALIAS ConsoleAliasRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleAliasRequest;
    PCONSRV_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    PVOID lpTarget;

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
                                   (PVOID*)&ConsoleAliasRequest->ExeName,
                                   ConsoleAliasRequest->ExeLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    lpTarget = (ConsoleAliasRequest->TargetLength != 0 ? ConsoleAliasRequest->Target : NULL);

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = STATUS_SUCCESS;

    Header = IntFindAliasHeader(Console,
                                ConsoleAliasRequest->ExeName,
                                ConsoleAliasRequest->ExeLength,
                                ConsoleAliasRequest->Unicode2);
    if (!Header && lpTarget != NULL)
    {
        Header = IntCreateAliasHeader(Console,
                                      ConsoleAliasRequest->ExeName,
                                      ConsoleAliasRequest->ExeLength,
                                      ConsoleAliasRequest->Unicode2);
        if (!Header)
        {
            Status = STATUS_NO_MEMORY;
            goto Quit;
        }

        IntInsertAliasHeader(&Console->Aliases, Header);
    }

    if (lpTarget == NULL) // Delete the entry
    {
        Entry = IntGetAliasEntry(Console, Header,
                                 ConsoleAliasRequest->Source,
                                 ConsoleAliasRequest->SourceLength,
                                 ConsoleAliasRequest->Unicode);
        if (!Entry)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Quit;
        }

        IntDeleteAliasEntry(Header, Entry);
    }
    else // Add the entry
    {
        Entry = IntCreateAliasEntry(Console,
                                    ConsoleAliasRequest->Source,
                                    ConsoleAliasRequest->SourceLength,
                                    ConsoleAliasRequest->Target,
                                    ConsoleAliasRequest->TargetLength,
                                    ConsoleAliasRequest->Unicode);
        if (!Entry)
        {
            Status = STATUS_NO_MEMORY;
            goto Quit;
        }

        IntInsertAliasEntry(Header, Entry);
    }

Quit:
    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleAlias)
{
    NTSTATUS Status;
    PCONSOLE_ADDGETALIAS ConsoleAliasRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleAliasRequest;
    PCONSRV_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    UINT Length;
    PVOID lpTarget;

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
                                   (PVOID*)&ConsoleAliasRequest->ExeName,
                                   ConsoleAliasRequest->ExeLength,
                                   sizeof(BYTE)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    lpTarget = ConsoleAliasRequest->Target;

    if (ConsoleAliasRequest->ExeLength == 0 || lpTarget == NULL ||
            ConsoleAliasRequest->TargetLength == 0 || ConsoleAliasRequest->SourceLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Header = IntFindAliasHeader(Console,
                                ConsoleAliasRequest->ExeName,
                                ConsoleAliasRequest->ExeLength,
                                ConsoleAliasRequest->Unicode2);
    if (!Header)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    Entry = IntGetAliasEntry(Console, Header,
                             ConsoleAliasRequest->Source,
                             ConsoleAliasRequest->SourceLength,
                             ConsoleAliasRequest->Unicode);
    if (!Entry)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    if (ConsoleAliasRequest->Unicode)
    {
        Length = Entry->Target.Length + sizeof(WCHAR);
        if (Length > ConsoleAliasRequest->TargetLength) // FIXME: Refine computation.
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto Quit;
        }

        RtlCopyMemory(lpTarget, Entry->Target.Buffer, Entry->Target.Length);
        ConsoleAliasRequest->TargetLength = Length;
    }
    else
    {
        Length = (Entry->Target.Length + sizeof(WCHAR)) / sizeof(WCHAR);
        if (Length > ConsoleAliasRequest->TargetLength) // FIXME: Refine computation.
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto Quit;
        }

        ConvertInputUnicodeToAnsi(Console,
                                  Entry->Target.Buffer, Entry->Target.Length,
                                  lpTarget, Entry->Target.Length / sizeof(WCHAR));
        ConsoleAliasRequest->TargetLength = Length;
    }

Quit:
    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleAliases)
{
    NTSTATUS Status;
    PCONSOLE_GETALLALIASES GetAllAliasesRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAllAliasesRequest;
    PCONSRV_CONSOLE Console;
    ULONG BytesWritten = 0;
    PALIAS_HEADER Header;

    DPRINT("SrvGetConsoleAliases entered ApiMessage %p\n", ApiMessage);

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

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Header = IntFindAliasHeader(Console,
                                GetAllAliasesRequest->ExeName,
                                GetAllAliasesRequest->ExeLength,
                                GetAllAliasesRequest->Unicode2);
    if (!Header) goto Quit;

    if (IntGetAllConsoleAliasesLength(Header, GetAllAliasesRequest->Unicode) > GetAllAliasesRequest->AliasesBufferLength)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Quit;
    }

    {
        LPSTR  TargetBufferA;
        LPWSTR TargetBufferW;
        UINT TargetBufferLength = GetAllAliasesRequest->AliasesBufferLength;

        PALIAS_ENTRY CurEntry = Header->Data;
        UINT Offset = 0;
        UINT SourceLength, TargetLength;

        if (GetAllAliasesRequest->Unicode)
        {
            TargetBufferW = GetAllAliasesRequest->AliasesBuffer;
            TargetBufferLength /= sizeof(WCHAR);
        }
        else
        {
            TargetBufferA = GetAllAliasesRequest->AliasesBuffer;
        }

        while (CurEntry)
        {
            SourceLength = CurEntry->Source.Length / sizeof(WCHAR);
            TargetLength = CurEntry->Target.Length / sizeof(WCHAR);
            if (Offset + TargetLength + SourceLength + 2 > TargetBufferLength)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            if (GetAllAliasesRequest->Unicode)
            {
                RtlCopyMemory(&TargetBufferW[Offset], CurEntry->Source.Buffer, SourceLength * sizeof(WCHAR));
                Offset += SourceLength;
                TargetBufferW[Offset++] = L'=';
                RtlCopyMemory(&TargetBufferW[Offset], CurEntry->Target.Buffer, TargetLength * sizeof(WCHAR));
                Offset += TargetLength;
                TargetBufferW[Offset++] = L'\0';
            }
            else
            {
                ConvertInputUnicodeToAnsi(Console,
                                          CurEntry->Source.Buffer, SourceLength * sizeof(WCHAR),
                                          &TargetBufferA[Offset], SourceLength);
                Offset += SourceLength;
                TargetBufferA[Offset++] = '=';
                ConvertInputUnicodeToAnsi(Console,
                                          CurEntry->Target.Buffer, TargetLength * sizeof(WCHAR),
                                          &TargetBufferA[Offset], TargetLength);
                Offset += TargetLength;
                TargetBufferA[Offset++] = '\0';
            }

            CurEntry = CurEntry->Next;
        }

        if (GetAllAliasesRequest->Unicode)
            BytesWritten = Offset * sizeof(WCHAR);
        else
            BytesWritten = Offset;
    }

Quit:
    GetAllAliasesRequest->AliasesBufferLength = BytesWritten;

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleAliasesLength)
{
    NTSTATUS Status;
    PCONSOLE_GETALLALIASESLENGTH GetAllAliasesLengthRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAllAliasesLengthRequest;
    PCONSRV_CONSOLE Console;
    PALIAS_HEADER Header;

    DPRINT("SrvGetConsoleAliasesLength entered ApiMessage %p\n", ApiMessage);

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetAllAliasesLengthRequest->ExeName,
                                  GetAllAliasesLengthRequest->ExeLength,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Header = IntFindAliasHeader(Console,
                                GetAllAliasesLengthRequest->ExeName,
                                GetAllAliasesLengthRequest->ExeLength,
                                GetAllAliasesLengthRequest->Unicode2);
    if (Header)
    {
        GetAllAliasesLengthRequest->Length =
            IntGetAllConsoleAliasesLength(Header,
                                          GetAllAliasesLengthRequest->Unicode);
        Status = STATUS_SUCCESS;
    }
    else
    {
        GetAllAliasesLengthRequest->Length = 0;
    }

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleAliasExes)
{
    NTSTATUS Status;
    PCONSOLE_GETALIASESEXES GetAliasesExesRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAliasesExesRequest;
    PCONSRV_CONSOLE Console;
    UINT BytesWritten = 0;

    DPRINT("SrvGetConsoleAliasExes entered\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&GetAliasesExesRequest->ExeNames,
                                  GetAliasesExesRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    if (IntGetConsoleAliasesExesLength(Console->Aliases, GetAliasesExesRequest->Unicode) > GetAliasesExesRequest->Length)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Quit;
    }

    {
        PALIAS_HEADER RootHeader = Console->Aliases;

        LPSTR  TargetBufferA;
        LPWSTR TargetBufferW;
        UINT TargetBufferSize = GetAliasesExesRequest->Length;

        UINT Offset = 0;
        UINT Length;

        if (GetAliasesExesRequest->Unicode)
        {
            TargetBufferW = GetAliasesExesRequest->ExeNames;
            TargetBufferSize /= sizeof(WCHAR);
        }
        else
        {
            TargetBufferA = GetAliasesExesRequest->ExeNames;
        }

        while (RootHeader)
        {
            Length = RootHeader->ExeName.Length / sizeof(WCHAR);
            if (Offset + Length + 1 > TargetBufferSize)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            if (GetAliasesExesRequest->Unicode)
            {
                RtlCopyMemory(&TargetBufferW[Offset], RootHeader->ExeName.Buffer, Length * sizeof(WCHAR));
                Offset += Length;
                TargetBufferW[Offset++] = L'\0';
            }
            else
            {
                ConvertInputUnicodeToAnsi(Console,
                                          RootHeader->ExeName.Buffer, Length * sizeof(WCHAR),
                                          &TargetBufferA[Offset], Length);
                Offset += Length;
                TargetBufferA[Offset++] = '\0';
            }

            RootHeader = RootHeader->Next;
        }

        if (GetAliasesExesRequest->Unicode)
            BytesWritten = Offset * sizeof(WCHAR);
        else
            BytesWritten = Offset;
    }

Quit:
    GetAliasesExesRequest->Length = BytesWritten;

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleAliasExesLength)
{
    NTSTATUS Status;
    PCONSOLE_GETALIASESEXESLENGTH GetAliasesExesLengthRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetAliasesExesLengthRequest;
    PCONSRV_CONSOLE Console;

    DPRINT("SrvGetConsoleAliasExesLength entered ApiMessage %p\n", ApiMessage);

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetAliasesExesLengthRequest->Length =
        IntGetConsoleAliasesExesLength(Console->Aliases,
                                       GetAliasesExesLengthRequest->Unicode);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

/* EOF */

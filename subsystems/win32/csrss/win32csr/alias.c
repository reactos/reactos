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
}ALIAS_ENTRY, *PALIAS_ENTRY;


typedef struct tagALIAS_HEADER
{
    LPCWSTR lpExeName;
    PALIAS_ENTRY Data;
    struct tagALIAS_HEADER * Next;

}ALIAS_HEADER, *PALIAS_HEADER;

/* Ensure that a buffer is contained within the process's shared memory section. */
static BOOL
ValidateBuffer(PCSRSS_PROCESS_DATA ProcessData, PVOID Buffer, ULONG Size)
{
    ULONG Offset = (BYTE *)Buffer - (BYTE *)ProcessData->CsrSectionViewBase;
    if (Offset >= ProcessData->CsrSectionViewSize
        || Size > (ProcessData->CsrSectionViewSize - Offset))
    {
        DPRINT1("Invalid buffer %p %d; not within %p %d\n",
            Buffer, Size, ProcessData->CsrSectionViewBase, ProcessData->CsrSectionViewSize);
        return FALSE;
    }
    return TRUE;
}

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

  Entry = RtlAllocateHeap(Win32CsrApiHeap, 0, sizeof(ALIAS_HEADER) + sizeof(WCHAR) * dwLength);
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
    if (Header == NULL)
        return NULL;

    PALIAS_ENTRY RootHeader = Header->Data;
    while(RootHeader)
    {
        DPRINT("IntGetAliasEntry>lpSource %S\n", RootHeader->lpSource);
        INT diff = _wcsicmp(RootHeader->lpSource, lpSrcName);
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

   Entry = RtlAllocateHeap(Win32CsrApiHeap, 0, sizeof(ALIAS_ENTRY) + sizeof(WCHAR) * (dwSource + dwTarget));
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
            RtlFreeHeap(Win32CsrApiHeap, 0, Entry);
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
            RtlFreeHeap(Win32CsrApiHeap, 0, Entry);
        }
        RtlFreeHeap(Win32CsrApiHeap, 0, Header);
    }
}

CSR_API(CsrAddConsoleAlias)
{
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    WCHAR * lpExeName;
    WCHAR * lpSource;
    WCHAR * lpTarget;
    ULONG TotalLength;
    WCHAR * Ptr;

    TotalLength = Request->Data.AddConsoleAlias.SourceLength + Request->Data.AddConsoleAlias.ExeLength + Request->Data.AddConsoleAlias.TargetLength;
    Ptr = (WCHAR*)((ULONG_PTR)Request + sizeof(CSR_API_MESSAGE));

    lpSource = (WCHAR*)((ULONG_PTR)Request + sizeof(CSR_API_MESSAGE));
    lpExeName = (WCHAR*)((ULONG_PTR)Request + sizeof(CSR_API_MESSAGE) + Request->Data.AddConsoleAlias.SourceLength * sizeof(WCHAR));
    lpTarget = (Request->Data.AddConsoleAlias.TargetLength != 0 ? lpExeName + Request->Data.AddConsoleAlias.ExeLength : NULL);

    DPRINT("CsrAddConsoleAlias entered Request %p lpSource %p lpExeName %p lpTarget %p\n", Request, lpSource, lpExeName, lpTarget);

    if (lpExeName == NULL || lpSource == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    Request->Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (!NT_SUCCESS(Request->Status))
    {
        return Request->Status;
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
            Request->Status = STATUS_SUCCESS;
        }
        else
        {
            Request->Status = STATUS_INVALID_PARAMETER;
        }
        ConioUnlockConsole(Console);
        return Request->Status;
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

CSR_API(CsrGetConsoleAlias)
{
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    UINT Length;
    WCHAR * lpExeName;
    WCHAR * lpSource;
    WCHAR * lpTarget;

    lpSource = (LPWSTR)((ULONG_PTR)Request + sizeof(CSR_API_MESSAGE));
    lpExeName = lpSource + Request->Data.GetConsoleAlias.SourceLength;
    lpTarget = Request->Data.GetConsoleAlias.TargetBuffer;


    DPRINT("CsrGetConsoleAlias entered lpExeName %p lpSource %p TargetBuffer %p TargetBufferLength %u\n", 
        lpExeName, lpSource, lpTarget, Request->Data.GetConsoleAlias.TargetBufferLength);
    
    if (Request->Data.GetConsoleAlias.ExeLength == 0 || lpTarget == NULL || 
        Request->Data.GetConsoleAlias.TargetBufferLength == 0 || Request->Data.GetConsoleAlias.SourceLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Request->Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (!NT_SUCCESS(Request->Status))
    {
        return Request->Status;
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
    if (Length > Request->Data.GetConsoleAlias.TargetBufferLength)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!ValidateBuffer(ProcessData, lpTarget, Request->Data.GetConsoleAlias.TargetBufferLength))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    wcscpy(lpTarget, Entry->lpTarget);
    Request->Data.GetConsoleAlias.BytesWritten = Length;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(CsrGetAllConsoleAliases)
{
    PCSRSS_CONSOLE Console;
    ULONG BytesWritten;
    PALIAS_HEADER Header;

    if (Request->Data.GetAllConsoleAlias.lpExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Request->Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (!NT_SUCCESS(Request->Status))
    {
        return Request->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, Request->Data.GetAllConsoleAlias.lpExeName);
    if (!Header)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    if (IntGetAllConsoleAliasesLength(Header) > Request->Data.GetAllConsoleAlias.AliasBufferLength)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (!ValidateBuffer(ProcessData,
                        Request->Data.GetAllConsoleAlias.AliasBuffer,
                        Request->Data.GetAllConsoleAlias.AliasBufferLength))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    BytesWritten = IntGetAllConsoleAliases(Header, 
                                           Request->Data.GetAllConsoleAlias.AliasBuffer,
                                           Request->Data.GetAllConsoleAlias.AliasBufferLength);

    Request->Data.GetAllConsoleAlias.BytesWritten = BytesWritten;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(CsrGetAllConsoleAliasesLength)
{
    PCSRSS_CONSOLE Console;
    PALIAS_HEADER Header;
    UINT Length;

    if (Request->Data.GetAllConsoleAliasesLength.lpExeName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Request->Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (!NT_SUCCESS(Request->Status))
    {
        return Request->Status;
    }

    Header = IntFindAliasHeader(Console->Aliases, Request->Data.GetAllConsoleAliasesLength.lpExeName);
    if (!Header)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    Length = IntGetAllConsoleAliasesLength(Header);
    Request->Data.GetAllConsoleAliasesLength.Length = Length;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(CsrGetConsoleAliasesExes)
{
    PCSRSS_CONSOLE Console;
    UINT BytesWritten;
    UINT ExesLength;
    
    DPRINT("CsrGetConsoleAliasesExes entered\n");

    Request->Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (!NT_SUCCESS(Request->Status))
    {
        return Request->Status;
    }

    ExesLength = IntGetConsoleAliasesExesLength(Console->Aliases);
    
    if (ExesLength > Request->Data.GetConsoleAliasesExes.Length)
    {
        ConioUnlockConsole(Console);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (Request->Data.GetConsoleAliasesExes.ExeNames == NULL)
    {
        ConioUnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }
    
    if (!ValidateBuffer(ProcessData,
                        Request->Data.GetConsoleAliasesExes.ExeNames,
                        Request->Data.GetConsoleAliasesExes.Length))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    BytesWritten = IntGetConsoleAliasesExes(Console->Aliases, 
                                            Request->Data.GetConsoleAliasesExes.ExeNames,
                                            Request->Data.GetConsoleAliasesExes.Length);

    Request->Data.GetConsoleAliasesExes.BytesWritten = BytesWritten;
    ConioUnlockConsole(Console);
    return STATUS_SUCCESS;
}

CSR_API(CsrGetConsoleAliasesExesLength)
{
    PCSRSS_CONSOLE Console;
    DPRINT("CsrGetConsoleAliasesExesLength entered\n");

    Request->Status = ConioConsoleFromProcessData(ProcessData, &Console);
    if (NT_SUCCESS(Request->Status))
    {
        Request->Data.GetConsoleAliasesExesLength.Length = IntGetConsoleAliasesExesLength(Console->Aliases);
        ConioUnlockConsole(Console);
    }
    return Request->Status;
}

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

#include <csrss.h>

#define NDEBUG
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

static PALIAS_HEADER RootHeader = NULL;

static
PALIAS_HEADER
IntFindAliasHeader(PALIAS_HEADER RootHeader, LPCWSTR lpExeName)
{
    while(RootHeader)
    {
        INT diff = _wcsicmp(RootHeader->lpExeName, lpExeName);
        if (!diff)
            return RootHeader;

        if (diff < 0)
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

  Entry = RtlAllocateHeap(CsrssApiHeap, 0, sizeof(ALIAS_HEADER) + sizeof(WCHAR) * dwLength);
  if (!Entry)
      return Entry;
  
  Entry->lpExeName = (LPCWSTR)(Entry + sizeof(ALIAS_HEADER));
  wcscpy((WCHAR*)Entry->lpExeName, lpExeName);
  Entry->Next = NULL;
  return Entry;
}

VOID
IntInsertAliasHeader(PALIAS_HEADER * RootHeader, PALIAS_HEADER NewHeader)
{
  PALIAS_HEADER CurrentHeader;
  PALIAS_HEADER LastHeader = NULL;

  if (*RootHeader == 0)
  {
    *RootHeader = NewHeader;
     return;
  }

  CurrentHeader = *RootHeader;

  while(CurrentHeader)
  {
      INT Diff = _wcsicmp(NewHeader->lpExeName, CurrentHeader->lpExeName);
      if (Diff < 0)
      {
        if (!LastHeader)
            *RootHeader = NewHeader;
        else
            LastHeader->Next = NewHeader;

        NewHeader->Next = CurrentHeader;
        return;
      }
      LastHeader = CurrentHeader;
      CurrentHeader = CurrentHeader->Next;
  }

  LastHeader->Next = NewHeader;
  NewHeader->Next = NULL;
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

        if (diff < 0)
            break;

        RootHeader = RootHeader->Next;
    }
    return NULL;
}


VOID
IntInsertAliasEntry(PALIAS_HEADER Header, PALIAS_ENTRY NewEntry)
{
  PALIAS_ENTRY CurrentEntry;
  PALIAS_ENTRY LastEntry = NULL;

  CurrentEntry = Header->Data;

  if (!CurrentEntry)
  {
    Header->Data = NewEntry;
    NewEntry->Next = NULL;
    return;
  }

  while(CurrentEntry)
  {
    INT Diff = _wcsicmp(NewEntry->lpSource, CurrentEntry->lpSource);
    if (Diff < 0)
    {
        if (!LastEntry)
            Header->Data = NewEntry;
        else
            LastEntry->Next = NewEntry;
        NewEntry->Next = CurrentEntry;
        return;
    }
    LastEntry = CurrentEntry;
    CurrentEntry = CurrentEntry->Next;
  }

  LastEntry->Next = NewEntry;
  NewEntry->Next = NULL;
}

PALIAS_ENTRY
IntCreateAliasEntry(LPCWSTR lpSource, LPCWSTR lpTarget)
{
   UINT dwSource;
   UINT dwTarget;
   PALIAS_ENTRY Entry;

   dwSource = wcslen(lpSource) + 1;
   dwTarget = wcslen(lpTarget) + 1;

   Entry = RtlAllocateHeap(CsrssApiHeap, 0, sizeof(ALIAS_ENTRY) + sizeof(WCHAR) * (dwSource + dwTarget));
   if (!Entry)
       return Entry;

   Entry->lpSource = (LPCWSTR)(Entry + sizeof(ALIAS_ENTRY));
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
   }
   if (length)
       length++; // last entry entry is terminated with 2 zero bytes

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
    PALIAS_ENTRY LastEntry;
    PALIAS_ENTRY CurEntry;

    if (Header->Data == Entry)
    {
        Header->Data = Entry->Next;
        RtlFreeHeap(CsrssApiHeap, 0, Entry);
        return;
    }
    LastEntry = Header->Data;
    CurEntry = LastEntry->Next;

    while(CurEntry)
    {
        if (CurEntry == Entry)
        {
            LastEntry->Next = Entry->Next;
            RtlFreeHeap(CsrssApiHeap, 0, Entry);
            return;
        }
        LastEntry = CurEntry;
        CurEntry = CurEntry->Next;
    }
}

CSR_API(CsrAddConsoleAlias)
{
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
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }
    
    Header = IntFindAliasHeader(RootHeader, lpExeName);
    if (!Header && lpTarget != NULL)
    {
        Header = IntCreateAliasHeader(lpExeName);
        if (!Header)
        {
            Request->Status = STATUS_INSUFFICIENT_RESOURCES;
            return Request->Status;
        }
        IntInsertAliasHeader(&RootHeader, Header);
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
        return Request->Status;
    }

    Entry = IntCreateAliasEntry(lpSource, lpTarget);

    if (!Entry)
    {
        Request->Status = STATUS_INSUFFICIENT_RESOURCES;
        return Request->Status;
    }

    IntInsertAliasEntry(Header, Entry);
    Request->Status = STATUS_SUCCESS;
    return Request->Status;
}

CSR_API(CsrGetConsoleAlias)
{
    PALIAS_HEADER Header;
    PALIAS_ENTRY Entry;
    UINT Length;
    WCHAR * lpExeName;
    WCHAR * lpSource;
    WCHAR * lpTarget;

    lpSource = (LPWSTR)((ULONG_PTR)Request + sizeof(CSR_API_MESSAGE));
    lpExeName = lpSource + Request->Data.GetConsoleAlias.SourceLength;
    lpTarget = (LPWSTR)lpExeName + Request->Data.GetConsoleAlias.ExeLength;


    DPRINT("CsrGetConsoleAlias entered lpExeName %p lpSource %p TargetBuffer %p TargetBufferLength %u\n", 
        lpExeName, lpSource, lpTarget, Request->Data.GetConsoleAlias.TargetBufferLength);
    
    if (Request->Data.GetConsoleAlias.ExeLength == 0 || lpTarget == NULL || 
        Request->Data.GetConsoleAlias.TargetBufferLength == 0 || Request->Data.GetConsoleAlias.SourceLength == 0)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }

    Header = IntFindAliasHeader(RootHeader, lpExeName);
    if (!Header)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }

    Entry = IntGetAliasEntry(Header, lpSource);
    if (!Entry)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }

    Length = (wcslen(Entry->lpTarget)+1) * sizeof(WCHAR);
    if (Length > Request->Data.GetConsoleAlias.TargetBufferLength)
    {
        Request->Status = ERROR_INSUFFICIENT_BUFFER;
        return Request->Status;      
    }

#if 0
    if (((PVOID)lpTarget < ProcessData->CsrSectionViewBase)
      || (((ULONG_PTR)lpTarget + Request->Data.GetConsoleAlias.TargetBufferLength) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
        Request->Status = STATUS_ACCESS_VIOLATION;
        DPRINT1("CsrGetConsoleAlias out of range lpTarget %p LowerViewBase %p UpperViewBase %p Size %p\n", lpTarget, 
            ProcessData->CsrSectionViewBase, (ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize, ProcessData->CsrSectionViewSize);
        return Request->Status;
    }
#endif

    wcscpy(lpTarget, Entry->lpTarget);
    lpTarget[CSRSS_MAX_ALIAS_TARGET_LENGTH-1] = '\0';
    Request->Data.GetConsoleAlias.BytesWritten = Length;
    Request->Status = STATUS_SUCCESS;
    return Request->Status;
}

CSR_API(CsrGetAllConsoleAliases)
{
    ULONG BytesWritten;
    PALIAS_HEADER Header;

    if (Request->Data.GetAllConsoleAlias.lpExeName == NULL)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }

    Header = IntFindAliasHeader(RootHeader, Request->Data.GetAllConsoleAlias.lpExeName);
    if (!Header)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }

    if (IntGetAllConsoleAliasesLength(Header) > Request->Data.GetAllConsoleAlias.AliasBufferLength)
    {
        Request->Status = ERROR_INSUFFICIENT_BUFFER;
        return Request->Status;
    }

    BytesWritten = IntGetAllConsoleAliases(Header, 
                                           Request->Data.GetAllConsoleAlias.AliasBuffer,
                                           Request->Data.GetAllConsoleAlias.AliasBufferLength);

    Request->Data.GetAllConsoleAlias.BytesWritten = BytesWritten;
    Request->Status = STATUS_SUCCESS;
    return Request->Status;
}

CSR_API(CsrGetAllConsoleAliasesLength)
{
    PALIAS_HEADER Header;
    UINT Length;

    if (Request->Data.GetAllConsoleAliasesLength.lpExeName == NULL)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }

    Header = IntFindAliasHeader(RootHeader, Request->Data.GetAllConsoleAliasesLength.lpExeName);
    if (!Header)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }

    Length = IntGetAllConsoleAliasesLength(Header);
    Request->Data.GetAllConsoleAliasesLength.Length = Length;
    Request->Status = STATUS_SUCCESS;
    return Request->Status;

}

CSR_API(CsrGetConsoleAliasesExes)
{
    UINT BytesWritten;
    UINT ExesLength;
    
    DPRINT("CsrGetConsoleAliasesExes entered\n");

    ExesLength = IntGetConsoleAliasesExesLength(RootHeader);
    
    if (ExesLength > Request->Data.GetConsoleAliasesExes.Length)
    {
        Request->Status = ERROR_INSUFFICIENT_BUFFER;
        return Request->Status;
    }

    if (Request->Data.GetConsoleAliasesExes.ExeNames == NULL)
    {
        Request->Status = STATUS_INVALID_PARAMETER;
        return Request->Status;
    }
    
    BytesWritten = IntGetConsoleAliasesExes(RootHeader, 
                                            Request->Data.GetConsoleAliasesExes.ExeNames,
                                            Request->Data.GetConsoleAliasesExes.Length);

    Request->Data.GetConsoleAliasesExes.BytesWritten = BytesWritten;
    Request->Status = STATUS_SUCCESS;
    return Request->Status;
}

CSR_API(CsrGetConsoleAliasesExesLength)
{
    DPRINT("CsrGetConsoleAliasesExesLength entered\n");

    Request->Status = STATUS_SUCCESS;
    Request->Data.GetConsoleAliasesExesLength.Length = IntGetConsoleAliasesExesLength(RootHeader);
    return Request->Status;
}

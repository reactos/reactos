/*
 * Parts of this file based on work Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb_symbols.c
 * PURPOSE:         Getting symbol information...
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/module.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/trap.h>
#include <ntdll/ldr.h>
#include <internal/safe.h>
#include <internal/kd.h>
#include <rosrtl/string.h>
#include <coff.h>

#define NDEBUG
#include <internal/debug.h>

#include "kdb.h"
#include "kdb_stabs.h"

/* GLOBALS ******************************************************************/

typedef struct _SYMBOLFILE_HEADER {
  ULONG StabsOffset;
  ULONG StabsLength;
  ULONG StabstrOffset;
  ULONG StabstrLength;
  ULONG SymbolsOffset;
  ULONG SymbolsLength;
  ULONG SymbolstrOffset;
  ULONG SymbolstrLength;
} SYMBOLFILE_HEADER, *PSYMBOLFILE_HEADER;

typedef struct _IMAGE_SYMBOL_INFO_CACHE {
  LIST_ENTRY ListEntry;
  ULONG RefCount;
  UNICODE_STRING FileName;
  PVOID FileBuffer;
  PVOID StabsBase;
  ULONG StabsLength;
  PVOID StabStringsBase;
  ULONG StabStringsLength;
  PVOID SymbolsBase;
  ULONG SymbolsLength;
  PVOID SymbolStringsBase;
  ULONG SymbolStringsLength;
} IMAGE_SYMBOL_INFO_CACHE, *PIMAGE_SYMBOL_INFO_CACHE;


static LIST_ENTRY SymbolFileListHead;
static KSPIN_LOCK SymbolFileListLock;


/* FUNCTIONS ****************************************************************/

/*! \brief Find a user-mode module...
 *
 * \param Address  If \a Address is not NULL the module containing \a Address
 *                 is searched.
 * \param Name     If \a Name is not NULL the module named \a Name will be
 *                 searched.
 * \param Index    If \a Index is >= 0 the Index'th module will be returned.
 * \param pInfo    Pointer to a KDB_MODULE_INFO which is filled.
 *
 * \retval TRUE   Module was found, \a pInfo was filled.
 * \retval FALSE  No module was found.
 *
 * \sa KdbpSymFindModule
 */
STATIC BOOLEAN
KdbpSymFindUserModule(IN PVOID Address  OPTIONAL,
                      IN LPCWSTR Name  OPTIONAL,
                      IN INT Index  OPTIONAL,
                      OUT PKDB_MODULE_INFO pInfo)
{
  PLIST_ENTRY current_entry;
  PLDR_MODULE current;
  PEPROCESS CurrentProcess;
  PPEB Peb = NULL;
  INT Count = 0;

  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != NULL)
    {
      Peb = CurrentProcess->Peb;
    }

  if (Peb == NULL)
    {
      return FALSE;
    }

  current_entry = Peb->Ldr->InLoadOrderModuleList.Flink;

  while (current_entry != &Peb->Ldr->InLoadOrderModuleList &&
         current_entry != NULL)
    {
      current = CONTAINING_RECORD(current_entry, LDR_MODULE, InLoadOrderModuleList);

      if ((Address != NULL && (Address >= (PVOID)current->BaseAddress &&
                               Address < (PVOID)((char *)current->BaseAddress + current->SizeOfImage))) ||
          (Name != NULL && _wcsicmp(current->BaseDllName.Buffer, Name) == 0) ||
          (Index >= 0 && Count++ == Index))
        {
	  INT Length = current->BaseDllName.Length;
	  if (Length > 255)
	    Length = 255;
	  wcsncpy(pInfo->Name, current->BaseDllName.Buffer, Length);
	  pInfo->Name[Length] = L'\0';
          pInfo->Base = (ULONG_PTR)current->BaseAddress;
          pInfo->Size = current->SizeOfImage;
          pInfo->SymbolInfo = &current->SymbolInfo;
          return TRUE;
        }
      current_entry = current_entry->Flink;
    }

  return FALSE;
}

/*! \brief Find a kernel-mode module...
 *
 * Works like \a KdbpSymFindUserModule.
 *
 * \sa KdbpSymFindUserModule
 */
STATIC BOOLEAN
KdbpSymFindModule(IN PVOID Address  OPTIONAL,
                  IN LPCWSTR Name  OPTIONAL,
                  IN INT Index  OPTIONAL,
                  OUT PKDB_MODULE_INFO pInfo)
{
  PLIST_ENTRY current_entry;
  MODULE_TEXT_SECTION* current;
  extern LIST_ENTRY ModuleTextListHead;
  INT Count = 0;

  current_entry = ModuleTextListHead.Flink;

  while (current_entry != &ModuleTextListHead &&
         current_entry != NULL)
    {
      current = CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);

      if ((Address != NULL && (Address >= (PVOID)current->Base &&
                               Address < (PVOID)(current->Base + current->Length))) ||
          (Name != NULL && _wcsicmp(current->Name, Name) == 0) ||
          (Index >= 0 && Count++ == Index))
        {
	  wcsncpy(pInfo->Name, current->Name, 255);
	  pInfo->Name[255] = L'\0';
          pInfo->Base = (ULONG_PTR)current->Base;
          pInfo->Size = current->Length;
          pInfo->SymbolInfo = &current->SymbolInfo;
          return TRUE;
        }
      current_entry = current_entry->Flink;
    }

  return KdbpSymFindUserModule(Address, Name, Index-Count, pInfo);
}

/*! \brief Find module by address...
 *
 * \param Address  Any address inside the module to look for.
 * \param pInfo    Pointer to a KDB_MODULE_INFO struct which is filled on
 *                 success.
 *
 * \retval TRUE   Success - module found.
 * \retval FALSE  Failure - module not found.
 *
 * \sa KdbpSymFindModuleByName
 * \sa KdbpSymFindModuleByIndex
 */
BOOLEAN
KdbpSymFindModuleByAddress(IN PVOID Address,
                           OUT PKDB_MODULE_INFO pInfo)
{
  return KdbpSymFindModule(Address, NULL, -1, pInfo);
}

/*! \brief Find module by name...
 *
 * \param Name   Name of the module to look for.
 * \param pInfo  Pointer to a KDB_MODULE_INFO struct which is filled on
 *               success.
 *
 * \retval TRUE   Success - module found.
 * \retval FALSE  Failure - module not found.
 *
 * \sa KdbpSymFindModuleByAddress
 * \sa KdbpSymFindModuleByIndex
 */
BOOLEAN
KdbpSymFindModuleByName(IN LPCWSTR Name,
                        OUT PKDB_MODULE_INFO pInfo)
{
  return KdbpSymFindModule(NULL, Name, -1, pInfo);
}

/*! \brief Find module by index...
 *
 * \param Index  Index of the module to return.
 * \param pInfo  Pointer to a KDB_MODULE_INFO struct which is filled on
 *               success.
 *
 * \retval TRUE   Success - module found.
 * \retval FALSE  Failure - module not found.
 *
 * \sa KdbpSymFindModuleByName
 * \sa KdbpSymFindModuleByAddress
 */
BOOLEAN
KdbpSymFindModuleByIndex(IN INT Index,
                         OUT PKDB_MODULE_INFO pInfo)
{
  return KdbpSymFindModule(NULL, NULL, Index, pInfo);
}

/*! \brief Print address...
 *
 * Tries to lookup line number, file name and function name for the given
 * address and prints it.
 * If no such information is found the address is printed in the format
 * <module: offset>, otherwise the format will be
 * <module: offset (filename:linenumber (functionname))>
 *
 * \retval TRUE  Module containing \a Address was found, \a Address was printed.
 * \retval FALSE  No module containing \a Address was found, nothing was printed.
 */
BOOLEAN
KdbSymPrintAddress(IN PVOID Address)
{
  KDB_MODULE_INFO Info;
  ULONG_PTR RelativeAddress;
  NTSTATUS Status;
  ULONG LineNumber;
  CHAR FileName[256];
  CHAR FunctionName[256];

  if (!KdbpSymFindModuleByAddress(Address, &Info))
    return FALSE;

  RelativeAddress = (ULONG_PTR) Address - Info.Base;
  Status = KdbSymGetAddressInformation(Info.SymbolInfo,
                                       RelativeAddress,
                                       &LineNumber,
                                       FileName,
                                       FunctionName);
  if (NT_SUCCESS(Status))
    {
      DbgPrint("<%ws: %x (%s:%d (%s))>",
               Info.Name, RelativeAddress, FileName, LineNumber, FunctionName);
    }
  else
    {
      DbgPrint("<%ws: %x>", Info.Name, RelativeAddress);
    }

  return TRUE;
}

/*! \brief Find a COFF symbol entry...
 *
 * Finds the COFF symbol as close as possible before the specified address
 *
 * \param SymbolInfo       Pointer to the symbol info.
 * \param RelativeAddress  Relative address of address to look for.
 *
 * \returns Pointer to a external_syment
 * \retval NULL  No entry found.
 */
static struct external_syment *
KdbpSymbolsFindEntry(IN PIMAGE_SYMBOL_INFO SymbolInfo,
                     IN ULONG_PTR RelativeAddress)
{
  /*
   * Perform a binary search.
   *
   * The code below is a bit sneaky.  After a comparison fails, we
   * divide the work in half by moving either left or right. If lim
   * is odd, moving left simply involves halving lim: e.g., when lim
   * is 5 we look at item 2, so we change lim to 2 so that we will
   * look at items 0 & 1.  If lim is even, the same applies.  If lim
   * is odd, moving right again involes halving lim, this time moving
   * the base up one item past p: e.g., when lim is 5 we change base
   * to item 3 and make lim 2 so that we will look at items 3 and 4.
   * If lim is even, however, we have to shrink it by one before
   * halving: e.g., when lim is 4, we still looked at item 2, so we
   * have to make lim 3, then halve, obtaining 1, so that we will only
   * look at item 3.
   */
  struct external_syment *Base = (struct external_syment *) SymbolInfo->SymbolsBase;
  ULONG Lim;
  struct external_syment *Mid, *Low;

  if (SymbolInfo->SymbolsLength < sizeof(struct external_syment)
      || RelativeAddress < Base->e_value)
    {
      return NULL;
    }

  Low = Base;
  for (Lim = SymbolInfo->SymbolsLength / sizeof(struct external_syment); Lim != 0; Lim >>= 1)
    {
      Mid = Base + (Lim >> 1);
      if (RelativeAddress == Mid->e_value)
        {
          return Mid;
        }
      if (Mid->e_value < RelativeAddress)  /* key > mid: move right */
        {
          Low = Mid;
          Base = Mid + 1;
          Lim--;
        }               /* else move left */
    }

  ASSERT(Low->e_value < RelativeAddress);

  return Low;
}


/*! \brief Get information for an address (source file, line number,
 *         function name)
 *
 * \param SymbolInfo       Pointer to IMAGE_SYMBOL_INFO.
 * \param RelativeAddress  Relative address to look up.
 * \param LineNumber       Pointer to an ULONG which is filled with the line
 *                         number (can be NULL)
 * \param FileName         Pointer to an array of CHARs which gets filled with
 *                         the filename (can be NULL)
 * \param FunctionName     Pointer to an array of CHARs which gets filled with
 *                         the function name (can be NULL)
 *
 * \returns NTSTATUS error code.
 * \retval STATUS_SUCCESS  At least one of the requested informations was found.
 * \retval STATUS_UNSUCCESSFUL  None of the requested information was found.
 */
NTSTATUS
KdbSymGetAddressInformation(IN PIMAGE_SYMBOL_INFO SymbolInfo,
                            IN ULONG_PTR RelativeAddress,
                            OUT PULONG LineNumber  OPTIONAL,
                            OUT PCH FileName  OPTIONAL,
                            OUT PCH FunctionName  OPTIONAL)
{
  PSTAB_ENTRY StabsFunctionEntry = NULL, FileEntry = NULL, LineEntry = NULL;
  struct external_syment *SymbolsFunctionEntry = NULL;

  DPRINT("RelativeAddress = 0x%08x\n", RelativeAddress);

  if (SymbolInfo->StabsBase == NULL || SymbolInfo->StabsLength == 0 ||
      SymbolInfo->StabStringsBase == NULL || SymbolInfo->StabStringsLength == 0)
    {
      return STATUS_UNSUCCESSFUL;
    }

#ifdef PEDANTIC_STABS
  if (RelativeAddress >= SymbolInfo->ImageSize)
    {
      DPRINT("Address is not within .text section. RelativeAddress %p  Length 0x%x\n",
        RelativeAddress, SymbolInfo->ImageSize);
      return STATUS_UNSUCCESSFUL;
    }
#endif

  ASSERT(LineNumber || FileName || FunctionName);

  if (LineNumber != NULL || FunctionName != NULL)
    {
      /* find stab entry for function */
      StabsFunctionEntry = KdbpStabFindEntry(SymbolInfo, N_FUN, (PVOID)RelativeAddress, NULL);
      if (StabsFunctionEntry == NULL)
        {
          DPRINT("No function stab entry found. RelativeAddress %p\n", RelativeAddress);
        }

      if (LineNumber != NULL && StabsFunctionEntry != NULL)
        {
          /* find stab entry for line number */
          ULONG_PTR FunctionRelativeAddress = RelativeAddress - StabsFunctionEntry->n_value;
          ULONG_PTR AddrFound = 0;
          PSTAB_ENTRY NextLineEntry;

          LineEntry = NextLineEntry = StabsFunctionEntry;
          while (NextLineEntry != NULL)
            {
              NextLineEntry++;
              if ((ULONG_PTR)NextLineEntry >= ((ULONG_PTR)SymbolInfo->StabsBase + SymbolInfo->StabsLength))
                break;
              if (NextLineEntry->n_type == N_FUN)
                break;
              if (NextLineEntry->n_type != N_SLINE)
                continue;

              if ( NextLineEntry->n_value <= FunctionRelativeAddress
                && NextLineEntry->n_value >= AddrFound )
                {
                  AddrFound = NextLineEntry->n_value;
                  LineEntry = NextLineEntry;
                }
            }
        }
    }

  if (FunctionName != NULL
      && SymbolInfo->SymbolsBase != NULL && SymbolInfo->SymbolsLength != 0
      && SymbolInfo->SymbolStringsBase != NULL && SymbolInfo->SymbolStringsLength != 0)
    {
      /* find symbol entry for function */
      SymbolsFunctionEntry = KdbpSymbolsFindEntry(SymbolInfo, RelativeAddress);
    }

  if (FileName != NULL)
    {
      /* find stab entry for file name */
      PCHAR p;
      INT Length;

      FileEntry = KdbpStabFindEntry(SymbolInfo, N_SO, (PVOID)RelativeAddress, NULL);
      if (FileEntry != NULL)
        {
          p = (PCHAR)SymbolInfo->StabStringsBase + FileEntry->n_strx;
          Length = strlen(p);
          if (p[Length - 1] == '/' || p[Length - 1] == '\\') /* source dir */
            FileEntry = KdbpStabFindEntry(SymbolInfo, N_SO, (PVOID)RelativeAddress, FileEntry + 1);
        }
      if (FileEntry == NULL)
        {
          DPRINT("No filename stab entry found. RelativeAddress %p\n", RelativeAddress);
        }
    }

  if (((LineNumber   != NULL && LineEntry          == NULL) || LineNumber   == NULL) &&
      ((FileName     != NULL && FileEntry          == NULL) || FileName     == NULL) &&
      ((FunctionName != NULL && StabsFunctionEntry == NULL && SymbolsFunctionEntry == NULL) ||
       FunctionName == NULL))
    {
      DPRINT("None of the requested information was found!\n");
      return STATUS_UNSUCCESSFUL;
    }

  if (LineNumber != NULL)
    {
      *LineNumber = 0;
      if (LineEntry != NULL)
        *LineNumber = LineEntry->n_desc;
    }
  if (FileName != NULL)
    {
      PCHAR Name = "";
      if (FileEntry != NULL)
        {
          Name = (PCHAR)SymbolInfo->StabStringsBase + FileEntry->n_strx;
        }
      strcpy(FileName, Name);
    }
  if (FunctionName != NULL)
    {
      PCHAR Name = "", p;
      if (StabsFunctionEntry != NULL)
        {
          if (SymbolsFunctionEntry == NULL ||
              SymbolsFunctionEntry->e_value <= StabsFunctionEntry->n_value)
            {
              Name = (PCHAR)SymbolInfo->StabStringsBase + StabsFunctionEntry->n_strx;
              strcpy(FunctionName, Name);
              if ((p = strchr(FunctionName, ':')) != NULL) /* remove extra info from function name */
                {
                  *p = '\0';
                }
            }
          else if (SymbolsFunctionEntry != NULL)
            {
              if (SymbolsFunctionEntry->e.e.e_zeroes == 0)
                {
                  Name = (PCHAR) SymbolInfo->SymbolStringsBase + SymbolsFunctionEntry->e.e.e_offset;
                  strcpy(FunctionName, Name);
                }
              else
                {
                  memcpy(FunctionName, SymbolsFunctionEntry->e.e_name, E_SYMNMLEN);
                  FunctionName[E_SYMNMLEN] = '\0';
                }
            }
        }
      else if (SymbolsFunctionEntry != NULL)
        {
          if (SymbolsFunctionEntry->e.e.e_zeroes == 0)
            {
              Name = (PCHAR) SymbolInfo->SymbolStringsBase + SymbolsFunctionEntry->e.e.e_offset;
              strcpy(FunctionName, Name);
            }
          else
            {
              memcpy(FunctionName, SymbolsFunctionEntry->e.e_name, E_SYMNMLEN);
              FunctionName[E_SYMNMLEN] = '\0';
            }
        }
      else
        {
          FunctionName[0] = '\0';
        }
    }

  return STATUS_SUCCESS;
}


/*! \brief Get absolute source-line number or function address
 *
 * \param SymbolInfo  IMAGE_SYMBOL_INFO of the module containing source file/line number.
 * \param FileName    Source filename.
 * \param LineNumber  Line number in source file.
 * \param FuncName    Function name.
 * \param Address     Filled with the address on success.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 */
BOOLEAN
KdbpSymGetSourceAddress(IN PIMAGE_SYMBOL_INFO SymbolInfo,
                        IN PCHAR FileName,
                        IN ULONG LineNumber  OPTIONAL,
                        IN PCHAR FuncName  OPTIONAL,
                        OUT PVOID *Address)
{
  PSTAB_ENTRY Entry, FunctionEntry = NULL;
  PCHAR SymbolName, p;
  CHAR Buffer[512] = "";
  INT Length, FileNameLength, FuncNameLength = 0;

  if (FuncName == NULL && LineNumber < 1)
    return FALSE;

  FileNameLength = strlen(FileName);
  FuncNameLength = strlen(FuncName);
  for (Entry = SymbolInfo->StabsBase;
       (ULONG_PTR)Entry < (ULONG_PTR)(SymbolInfo->StabsBase + SymbolInfo->StabsLength);
       Entry++)
    {
      if (Entry->n_type != N_SO)
        continue;

      SymbolName = (PCHAR)SymbolInfo->StabStringsBase + Entry->n_strx;
      Length = strlen(SymbolName);
      if (SymbolName[Length -  1] == '/' ||
          SymbolName[Length -  1] == '\\')
        {
          strncpy(Buffer, SymbolName, sizeof (Buffer) - 1);
          Buffer[sizeof (Buffer) - 1] = '\0';
          continue;
        }
      strncat(Buffer, SymbolName, sizeof (Buffer) - 1);
      Buffer[sizeof (Buffer) - 1] = '\0';

      Length = strlen(Buffer);
      if (strcmp(Buffer + Length - FileNameLength, FileName) != 0)
        continue;

      Entry++;
      for (;(ULONG_PTR)Entry < (ULONG_PTR)(SymbolInfo->StabsBase + SymbolInfo->StabsLength);
           Entry++)
        {
          if (Entry->n_type == N_FUN)
            FunctionEntry = Entry;
          else if (Entry->n_type == N_SO)
            break;
          else if (Entry->n_type != N_SLINE || LineNumber < 1)
            continue;

          if (LineNumber > 0 && Entry->n_desc != LineNumber)
            continue;
          else /* if (FunctionName != NULL) */
            {
              SymbolName = (PCHAR)SymbolInfo->StabStringsBase + Entry->n_strx;
              p = strchr(SymbolName, ':');
              if (p == NULL)
                return FALSE;
              Length = p - SymbolName;
              if (Length != FuncNameLength)
                continue;
              if (strncmp(FuncName, SymbolName, Length) != 0)
                continue;
            }

          /* found */
          if (Entry->n_type == N_FUN)
            {
              *Address = (PVOID)Entry->n_value; /* FIXME: relocate address */
              return TRUE;
            }

          if (FunctionEntry == NULL)
            return FALSE;

          *Address = (PVOID)((ULONG_PTR)Entry->n_value + FunctionEntry->n_value); /* FIXME: relocate address */
          return TRUE;
        }
      break;
    }

    return FALSE;
}

/*! \brief Find cached symbol file.
 *
 * Looks through the list of cached symbol files and tries to find an already
 * loaded one.
 *
 * \param FileName  FileName of the symbol file to look for.
 *
 * \returns A pointer to the cached symbol info.
 * \retval NULL  No cached info found.
 *
 * \sa KdbpSymAddCachedFile
 */
STATIC PIMAGE_SYMBOL_INFO_CACHE
KdbpSymFindCachedFile(IN PUNICODE_STRING FileName)
{
  PIMAGE_SYMBOL_INFO_CACHE Current;
  PLIST_ENTRY CurrentEntry;
  KIRQL Irql;

  DPRINT("Looking for cached symbol file %wZ\n", FileName);

  KeAcquireSpinLock(&SymbolFileListLock, &Irql);

  CurrentEntry = SymbolFileListHead.Flink;
  while (CurrentEntry != (&SymbolFileListHead))
    {
      Current = CONTAINING_RECORD(CurrentEntry, IMAGE_SYMBOL_INFO_CACHE, ListEntry);

      DPRINT("Current->FileName %wZ FileName %wZ\n", &Current->FileName, FileName);
      if (RtlEqualUnicodeString(&Current->FileName, FileName, TRUE))
        {
          Current->RefCount++;
          KeReleaseSpinLock(&SymbolFileListLock, Irql);
          DPRINT("Found cached file!\n");
          return Current;
        }

      CurrentEntry = CurrentEntry->Flink;
    }

  KeReleaseSpinLock(&SymbolFileListLock, Irql);

  DPRINT("Cached file not found!\n");
  return NULL;
}

/*! \brief Add a symbol file to the cache.
 *
 * \param FileName    Filename of the symbol file.
 * \param SymbolInfo  Pointer to the symbol info.
 *
 * \sa KdbpSymRemoveCachedFile
 */
STATIC VOID
KdbpSymAddCachedFile(IN PUNICODE_STRING FileName,
		     IN PIMAGE_SYMBOL_INFO SymbolInfo)
{
  PIMAGE_SYMBOL_INFO_CACHE CacheEntry;

  DPRINT("Adding symbol file: FileBuffer = %p, ImageBase = %p\n",
         SymbolInfo->FileBuffer, SymbolInfo->ImageBase);

  /* allocate entry */
  CacheEntry = ExAllocatePool(NonPagedPool, sizeof (IMAGE_SYMBOL_INFO_CACHE));
  ASSERT(CacheEntry);
  RtlZeroMemory(CacheEntry, sizeof (IMAGE_SYMBOL_INFO_CACHE));

  /* fill entry */
  RtlCreateUnicodeString(&CacheEntry->FileName, FileName->Buffer);
  ASSERT(CacheEntry->FileName.Buffer);
  CacheEntry->RefCount = 1;
  CacheEntry->FileBuffer = SymbolInfo->FileBuffer;
  CacheEntry->StabsBase = SymbolInfo->StabsBase;
  CacheEntry->StabsLength = SymbolInfo->StabsLength;
  CacheEntry->StabStringsBase = SymbolInfo->StabStringsBase;
  CacheEntry->StabStringsLength = SymbolInfo->StabStringsLength;
  CacheEntry->SymbolsBase = SymbolInfo->SymbolsBase;
  CacheEntry->SymbolsLength = SymbolInfo->SymbolsLength;
  CacheEntry->SymbolStringsBase = SymbolInfo->SymbolStringsBase;
  CacheEntry->SymbolStringsLength = SymbolInfo->SymbolStringsLength;
  InsertTailList(&SymbolFileListHead, &CacheEntry->ListEntry); /* FIXME: Lock list? */
}

/*! \brief Remove a symbol file (reference) from the cache.
 *
 * Tries to find a cache entry matching the given symbol info and decreases
 * it's reference count. If the refcount is 0 after decreasing it the cache
 * entry will be removed from the list and freed.
 *
 * \param SymbolInfo  Pointer to the symbol info.
 *
 * \sa KdbpSymAddCachedFile
 */
STATIC VOID
KdbpSymRemoveCachedFile(IN PIMAGE_SYMBOL_INFO SymbolInfo)
{
  PIMAGE_SYMBOL_INFO_CACHE Current;
  PLIST_ENTRY CurrentEntry;
  KIRQL Irql;

  KeAcquireSpinLock(&SymbolFileListLock, &Irql);

  CurrentEntry = SymbolFileListHead.Flink;
  while (CurrentEntry != (&SymbolFileListHead))
    {
      Current = CONTAINING_RECORD(CurrentEntry, IMAGE_SYMBOL_INFO_CACHE, ListEntry);

      if (Current->FileBuffer == SymbolInfo->FileBuffer) /* found */
        {
          ASSERT(Current->RefCount > 0);
          Current->RefCount--;
          if (Current->RefCount < 1)
            {
              RemoveEntryList(&Current->ListEntry);
              ExFreePool(Current->FileBuffer);
              ExFreePool(Current);
            }
          KeReleaseSpinLock(&SymbolFileListLock, Irql);
          return;
        }

      CurrentEntry = CurrentEntry->Flink;
    }

  KeReleaseSpinLock(&SymbolFileListLock, Irql);
  DPRINT1("Warning: Removing unknown symbol file: FileBuffer = %p, ImageBase = %p\n",
          SymbolInfo->FileBuffer, SymbolInfo->ImageBase);
}

/*! \brief Loads a symbol file.
 *
 * \param FileName    Filename of the symbol file to load.
 * \param SymbolInfo  Pointer to a SymbolInfo which gets filled.
 *
 * \sa KdbpSymUnloadModuleSymbols
 */
STATIC VOID
KdbpSymLoadModuleSymbols(IN PUNICODE_STRING FileName,
                         OUT PIMAGE_SYMBOL_INFO SymbolInfo)
{
  FILE_STANDARD_INFORMATION FileStdInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR TmpFileName[MAX_PATH];
  UNICODE_STRING SymFileName;
  LPWSTR Start, Ext;
  HANDLE FileHandle;
  PVOID FileBuffer;
  NTSTATUS Status;
  ULONG Length;
  IO_STATUS_BLOCK IoStatusBlock;
  PSYMBOLFILE_HEADER SymbolFileHeader;
  PIMAGE_SYMBOL_INFO_CACHE CachedSymbolFile;

#ifdef KDBG
  /* Allow KDB to break on module load */
  KdbModuleLoaded(FileName);
#endif

  /*  Get the path to the symbol store  */
  wcscpy(TmpFileName, L"\\SystemRoot\\symbols\\");

  /*  Get the symbol filename from the module name  */
  Start = wcsrchr(FileName->Buffer, L'\\');
  if (Start == NULL)
    Start = FileName->Buffer;
  else
    Start++;

  Ext = wcsrchr(FileName->Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncat(TmpFileName, Start, Length);
  wcscat(TmpFileName, L".sym");
  RtlInitUnicodeString(&SymFileName, TmpFileName);

  /*  Try to find cached (already loaded) symbol file  */
  CachedSymbolFile = KdbpSymFindCachedFile(&SymFileName);
  if (CachedSymbolFile != NULL)
    {
      DPRINT("Found cached symbol file %wZ\n", &SymFileName);
      SymbolInfo->FileBuffer = CachedSymbolFile->FileBuffer;
      SymbolInfo->StabsBase = CachedSymbolFile->StabsBase;
      SymbolInfo->StabsLength = CachedSymbolFile->StabsLength;
      SymbolInfo->StabStringsBase = CachedSymbolFile->StabStringsBase;
      SymbolInfo->StabStringsLength = CachedSymbolFile->StabStringsLength;
      SymbolInfo->SymbolsBase = CachedSymbolFile->SymbolsBase;
      SymbolInfo->SymbolsLength = CachedSymbolFile->SymbolsLength;
      SymbolInfo->SymbolStringsBase = CachedSymbolFile->SymbolStringsBase;
      SymbolInfo->SymbolStringsLength = CachedSymbolFile->SymbolStringsLength;
      return;
    }

  /*  Open the file  */
  InitializeObjectAttributes(&ObjectAttributes,
                             &SymFileName,
                             0,
                             NULL,
                             NULL);

  DPRINT("Attempting to open symbols: %wZ\n", &SymFileName);

  Status = ZwOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      FILE_SYNCHRONOUS_IO_NONALERT|FILE_NO_INTERMEDIATE_BUFFERING);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not open symbol file: %wZ\n", &SymFileName);
      return;
    }

  DPRINT("Loading symbols from %wZ...\n", &SymFileName);

  /*  Get the size of the file  */
  Status = ZwQueryInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not get file size\n");
      ZwClose(FileHandle);
      return;
    }

  DPRINT("Symbol file is %08x bytes\n", FileStdInfo.EndOfFile.u.LowPart);

  /*  Allocate nonpageable memory for symbol file  */
  FileBuffer = ExAllocatePool(NonPagedPool,
                              FileStdInfo.AllocationSize.u.LowPart);

  if (FileBuffer == NULL)
    {
      DPRINT("Could not allocate memory for symbol file\n");
      ZwClose(FileHandle);
      return;
    }

  /*  Load file into memory chunk  */
  Status = ZwReadFile(FileHandle,
                      0, 0, 0,
                      &IoStatusBlock,
                      FileBuffer,
                      FileStdInfo.EndOfFile.u.LowPart,
                      0, 0);
  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
      DPRINT("Could not read symbol file into memory (Status 0x%x)\n", Status);
      ExFreePool(FileBuffer);
      ZwClose(FileHandle);
      return;
    }

  ZwClose(FileHandle);

  DPRINT("Symbols loaded.\n");

  SymbolFileHeader = (PSYMBOLFILE_HEADER) FileBuffer;
  SymbolInfo->FileBuffer = FileBuffer;
  SymbolInfo->StabsBase = FileBuffer + SymbolFileHeader->StabsOffset;
  SymbolInfo->StabsLength = SymbolFileHeader->StabsLength;
  SymbolInfo->StabStringsBase = FileBuffer + SymbolFileHeader->StabstrOffset;
  SymbolInfo->StabStringsLength = SymbolFileHeader->StabstrLength;
  SymbolInfo->SymbolsBase = FileBuffer + SymbolFileHeader->SymbolsOffset;
  SymbolInfo->SymbolsLength = SymbolFileHeader->SymbolsLength;
  SymbolInfo->SymbolStringsBase = FileBuffer + SymbolFileHeader->SymbolstrOffset;
  SymbolInfo->SymbolStringsLength = SymbolFileHeader->SymbolstrLength;

  /* add file to cache */
  KdbpSymAddCachedFile(&SymFileName, SymbolInfo);

  DPRINT("Installed stabs: %wZ (%08x-%08x,%08x) (%08x-%08x,%08x)\n",
	 FileName,
	 SymbolInfo->StabsBase,
	 SymbolInfo->StabsLength + SymbolInfo->StabsBase,
	 SymbolInfo->StabStringsBase,
	 SymbolInfo->SymbolsBase,
	 SymbolInfo->SymbolsLength + SymbolInfo->SymbolsBase,
	 SymbolInfo->SymbolStringsBase);
}

/*! \brief Unloads symbol info.
 *
 * \param SymbolInfo  Pointer to the symbol info to unload.
 *
 * \sa KdbpSymLoadModuleSymbols
 */
STATIC VOID
KdbpSymUnloadModuleSymbols(IN PIMAGE_SYMBOL_INFO SymbolInfo)
{
  DPRINT("Unloading symbols\n");

  if (SymbolInfo != NULL && SymbolInfo->FileBuffer != NULL &&
      (PVOID)SymbolInfo->ImageBase != NULL)
    {
      KdbpSymRemoveCachedFile(SymbolInfo);
      SymbolInfo->FileBuffer = NULL;
      SymbolInfo->StabsBase = NULL;
      SymbolInfo->StabsLength = 0;
      SymbolInfo->SymbolsBase = NULL;
      SymbolInfo->SymbolsLength = 0;
    }
}

/*! \brief Load symbol info for a user module.
 *
 * \param LdrModule Pointer to the module to load symbols for.
 */
VOID
KdbSymLoadUserModuleSymbols(IN PLDR_MODULE LdrModule)
{
  DPRINT("LdrModule %p\n", LdrModule);

  RtlZeroMemory(&LdrModule->SymbolInfo, sizeof (LdrModule->SymbolInfo));
  LdrModule->SymbolInfo.ImageBase = (ULONG_PTR)LdrModule->BaseAddress;
  LdrModule->SymbolInfo.ImageSize = LdrModule->SizeOfImage;

  KdbpSymLoadModuleSymbols(&LdrModule->FullDllName, &LdrModule->SymbolInfo);
}

/*! \brief Frees all symbols loaded for a process.
 *
 * \param Process  Pointer to a process.
 */
VOID
KdbSymFreeProcessSymbols(IN PEPROCESS Process)
{
  PLIST_ENTRY CurrentEntry;
  PLDR_MODULE Current;
  PIMAGE_SYMBOL_INFO SymbolInfo;
  PEPROCESS CurrentProcess;
  PPEB Peb;

  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
  {
    KeAttachProcess(&Process->Pcb);
  }
  Peb = Process->Peb;
  ASSERT(Peb);
  ASSERT(Peb->Ldr);

  CurrentEntry = Peb->Ldr->InLoadOrderModuleList.Flink;
  while (CurrentEntry != &Peb->Ldr->InLoadOrderModuleList &&
	 CurrentEntry != NULL)
    {
      Current = CONTAINING_RECORD(CurrentEntry, LDR_MODULE, InLoadOrderModuleList);

      SymbolInfo = &Current->SymbolInfo;
      KdbpSymUnloadModuleSymbols(SymbolInfo);

      CurrentEntry = CurrentEntry->Flink;
    }
  if (CurrentProcess != Process)
  {
    KeDetachProcess();
  }
}

/*! \brief Load symbol info for a driver.
 *
 * \param Filename  Filename of the driver.
 * \param Module    Pointer to the driver MODULE_OBJECT.
 */
VOID
KdbSymLoadDriverSymbols(IN PUNICODE_STRING Filename,
                        IN PMODULE_OBJECT Module)
{
  /* Load symbols for the image if available */
  DPRINT("Loading driver %wZ symbols (driver @ %08x)\n", Filename, Module->Base);

  RtlZeroMemory(&Module->TextSection->SymbolInfo, sizeof (Module->TextSection->SymbolInfo));
  Module->TextSection->SymbolInfo.ImageBase = Module->TextSection->Base;
  Module->TextSection->SymbolInfo.ImageSize = Module->TextSection->Length;

  KdbpSymLoadModuleSymbols(Filename, &Module->TextSection->SymbolInfo);
}

/*! \brief Unloads symbol info for a driver.
 *
 * \param ModuleObject  Pointer to the driver MODULE_OBJECT.
 */
VOID
KdbSymUnloadDriverSymbols(IN PMODULE_OBJECT ModuleObject)
{
  /* Unload symbols for module if available */
  KdbpSymUnloadModuleSymbols(&ModuleObject->TextSection->SymbolInfo);
}

/*! \brief Called when a symbol file is loaded by the loader?
 *
 * Tries to find a driver (.sys) or executable (.exe) with the same base name
 * as the symbol file and sets the drivers/exes symbol info to the loaded
 * module.
 * Used to load ntoskrnl and hal symbols before the SystemRoot is available to us.
 *
 * \param FileName        Filename for which the symbols are loaded.
 */
VOID
KdbSymProcessBootSymbols(IN PCHAR FileName)
{
  PMODULE_OBJECT ModuleObject;
  UNICODE_STRING UnicodeString;
  PLOADER_MODULE KeLoaderModules = (PLOADER_MODULE)KeLoaderBlock.ModsAddr;
  CHAR SymbolName[MAX_PATH];
  PSYMBOLFILE_HEADER SymbolFileHeader;
  PIMAGE_SYMBOL_INFO SymbolInfo;
  ANSI_STRING AnsiString;
  PCHAR Extension;
  ULONG i;

  DPRINT("KdbSymProcessBootSymbols(%s)\n", FileName);

  RtlInitAnsiString(&AnsiString, FileName);
  RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
  ModuleObject = LdrGetModuleObject(&UnicodeString);
  RtlFreeUnicodeString(&UnicodeString);

  if (ModuleObject != NULL)
  {
     strcpy(SymbolName, FileName);
     Extension = strrchr(SymbolName, '.');
     if (Extension == NULL)
     {
        Extension = SymbolName + strlen(SymbolName);
     }
     strcpy(Extension, ".sym");
     for (i = 0; i < KeLoaderBlock.ModsCount; i++)
     {
        if (KeLoaderModules[i].Reserved == 0 && !_stricmp(SymbolName, (PCHAR)KeLoaderModules[i].String))
	{
	   break;
	}
     }
     if (i < KeLoaderBlock.ModsCount)
     {
        KeLoaderModules[i].Reserved = 1;
        SymbolInfo = (PIMAGE_SYMBOL_INFO) &ModuleObject->TextSection->SymbolInfo;
        if (SymbolInfo->FileBuffer != NULL)
        {
           KdbpSymRemoveCachedFile(SymbolInfo);
        }

	SymbolFileHeader = ExAllocatePool(NonPagedPool, KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart);
        if (SymbolFileHeader == NULL)
        {
           DPRINT("Could not allocate memory for symbol file\n");
           return;
        }
	memcpy(SymbolFileHeader, 
	       (PVOID)KeLoaderModules[i].ModStart, 
	       KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart);
 
        SymbolInfo->FileBuffer = SymbolFileHeader;
        SymbolInfo->StabsBase = (PVOID)SymbolFileHeader + SymbolFileHeader->StabsOffset;
        SymbolInfo->StabsLength = SymbolFileHeader->StabsLength;
        SymbolInfo->StabStringsBase = (PVOID)SymbolFileHeader + SymbolFileHeader->StabstrOffset;
        SymbolInfo->StabStringsLength = SymbolFileHeader->StabstrLength;
        SymbolInfo->SymbolsBase = (PVOID)SymbolFileHeader + SymbolFileHeader->SymbolsOffset;
        SymbolInfo->SymbolsLength = SymbolFileHeader->SymbolsLength;
        SymbolInfo->SymbolStringsBase = (PVOID)SymbolFileHeader + SymbolFileHeader->SymbolstrOffset;
        SymbolInfo->SymbolStringsLength = SymbolFileHeader->SymbolstrLength;

        /* add file to cache */
        RtlInitAnsiString(&AnsiString, SymbolName);
	RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
        KdbpSymAddCachedFile(&UnicodeString, SymbolInfo);
        RtlFreeUnicodeString(&UnicodeString);

        DPRINT("Installed stabs: %s@%08x-%08x (%08x-%08x,%08x) (%08x-%08x,%08x)\n",
	       FileName,
	       ModuleObject->Base,
	       ModuleObject->Length + ModuleObject->Base,
	       SymbolInfo->StabsBase,
	       SymbolInfo->StabsLength + SymbolInfo->StabsBase,
	       SymbolInfo->StabStringsBase,
	       SymbolInfo->SymbolsBase,
	       SymbolInfo->SymbolsLength + SymbolInfo->SymbolsBase,
	       SymbolInfo->SymbolStringsBase);
     }
  }
}

/*! \brief Initializes the KDB symbols implementation.
 *
 * \param NtoskrnlTextSection  MODULE_TEXT_SECTION of ntoskrnl.exe
 * \param LdrHalTextSection    MODULE_TEXT_SECTION of hal.sys
 */
VOID
KdbSymInit(IN PMODULE_TEXT_SECTION NtoskrnlTextSection,
	   IN PMODULE_TEXT_SECTION LdrHalTextSection)
{
  RtlZeroMemory(&NtoskrnlTextSection->SymbolInfo, sizeof(NtoskrnlTextSection->SymbolInfo));
  NtoskrnlTextSection->SymbolInfo.ImageBase = NtoskrnlTextSection->OptionalHeader->ImageBase;
  NtoskrnlTextSection->SymbolInfo.ImageSize = NtoskrnlTextSection->Length;

  RtlZeroMemory(&LdrHalTextSection->SymbolInfo, sizeof(LdrHalTextSection->SymbolInfo));
  LdrHalTextSection->SymbolInfo.ImageBase = LdrHalTextSection->OptionalHeader->ImageBase;
  LdrHalTextSection->SymbolInfo.ImageSize = LdrHalTextSection->Length;

  InitializeListHead(&SymbolFileListHead);
  KeInitializeSpinLock(&SymbolFileListLock);
}

/* EOF */

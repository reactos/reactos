/* 
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
#include <ntoskrnl.h>
#include <reactos/rossym.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

#define TAG_KDBS TAG('K', 'D', 'B', 'S')

typedef struct _IMAGE_SYMBOL_INFO_CACHE {
  LIST_ENTRY ListEntry;
  ULONG RefCount;
  UNICODE_STRING FileName;
  PROSSYM_INFO RosSymInfo;
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
                               Address < (PVOID)((char *)current->BaseAddress + current->ResidentSize))) ||
          (Name != NULL && _wcsicmp(current->BaseDllName.Buffer, Name) == 0) ||
          (Index >= 0 && Count++ == Index))
        {
	  INT Length = current->BaseDllName.Length;
	  if (Length > 255)
	    Length = 255;
	  wcsncpy(pInfo->Name, current->BaseDllName.Buffer, Length);
	  pInfo->Name[Length] = L'\0';
          pInfo->Base = (ULONG_PTR)current->BaseAddress;
          pInfo->Size = current->ResidentSize;
          pInfo->RosSymInfo = current->RosSymInfo;
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
          pInfo->RosSymInfo = current->RosSymInfo;
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
  Status = KdbSymGetAddressInformation(Info.RosSymInfo,
                                       RelativeAddress,
                                       &LineNumber,
                                       FileName,
                                       FunctionName);
  if (NT_SUCCESS(Status))
    {
      DbgPrint("<%ws:%x (%s:%d (%s))>",
               Info.Name, RelativeAddress, FileName, LineNumber, FunctionName);
    }
  else
    {
      DbgPrint("<%ws:%x>", Info.Name, RelativeAddress);
    }

  return TRUE;
}


/*! \brief Get information for an address (source file, line number,
 *         function name)
 *
 * \param SymbolInfo       Pointer to ROSSYM_INFO.
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
KdbSymGetAddressInformation(IN PROSSYM_INFO RosSymInfo,
                            IN ULONG_PTR RelativeAddress,
                            OUT PULONG LineNumber  OPTIONAL,
                            OUT PCH FileName  OPTIONAL,
                            OUT PCH FunctionName  OPTIONAL)
{
  if (NULL == RosSymInfo)
    {
      return STATUS_UNSUCCESSFUL;
    }

  if (! RosSymGetAddressInformation(RosSymInfo, RelativeAddress, LineNumber,
                                    FileName, FunctionName))
    {
      return STATUS_UNSUCCESSFUL;
    }

  return STATUS_SUCCESS;
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
STATIC PROSSYM_INFO
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
          return Current->RosSymInfo;
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
 * \param RosSymInfo  Pointer to the symbol info.
 *
 * \sa KdbpSymRemoveCachedFile
 */
STATIC VOID
KdbpSymAddCachedFile(IN PUNICODE_STRING FileName,
		     IN PROSSYM_INFO RosSymInfo)
{
  PIMAGE_SYMBOL_INFO_CACHE CacheEntry;

  DPRINT("Adding symbol file: RosSymInfo = %p\n", RosSymInfo);

  /* allocate entry */
  CacheEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof (IMAGE_SYMBOL_INFO_CACHE), TAG_KDBS);
  ASSERT(CacheEntry);
  RtlZeroMemory(CacheEntry, sizeof (IMAGE_SYMBOL_INFO_CACHE));

  /* fill entry */
  RtlpCreateUnicodeString(&CacheEntry->FileName, FileName->Buffer, PagedPool);
  ASSERT(CacheEntry->FileName.Buffer);
  CacheEntry->RefCount = 1;
  CacheEntry->RosSymInfo = RosSymInfo;
  InsertTailList(&SymbolFileListHead, &CacheEntry->ListEntry); /* FIXME: Lock list? */
}

/*! \brief Remove a symbol file (reference) from the cache.
 *
 * Tries to find a cache entry matching the given symbol info and decreases
 * it's reference count. If the refcount is 0 after decreasing it the cache
 * entry will be removed from the list and freed.
 *
 * \param RosSymInfo  Pointer to the symbol info.
 *
 * \sa KdbpSymAddCachedFile
 */
STATIC VOID
KdbpSymRemoveCachedFile(IN PROSSYM_INFO RosSymInfo)
{
  PIMAGE_SYMBOL_INFO_CACHE Current;
  PLIST_ENTRY CurrentEntry;
  KIRQL Irql;

  KeAcquireSpinLock(&SymbolFileListLock, &Irql);

  CurrentEntry = SymbolFileListHead.Flink;
  while (CurrentEntry != (&SymbolFileListHead))
    {
      Current = CONTAINING_RECORD(CurrentEntry, IMAGE_SYMBOL_INFO_CACHE, ListEntry);

      if (Current->RosSymInfo == RosSymInfo) /* found */
        {
          ASSERT(Current->RefCount > 0);
          Current->RefCount--;
          if (Current->RefCount < 1)
            {
              RemoveEntryList(&Current->ListEntry);
              RosSymDelete(Current->RosSymInfo);
              ExFreePool(Current);
            }
          KeReleaseSpinLock(&SymbolFileListLock, Irql);
          return;
        }

      CurrentEntry = CurrentEntry->Flink;
    }

  KeReleaseSpinLock(&SymbolFileListLock, Irql);
  DPRINT1("Warning: Removing unknown symbol file: RosSymInfo = %p\n", RosSymInfo);
}

/*! \brief Loads a symbol file.
 *
 * \param FileName    Filename of the symbol file to load.
 * \param RosSymInfo  Pointer to a ROSSYM_INFO which gets filled.
 *
 * \sa KdbpSymUnloadModuleSymbols
 */
STATIC VOID
KdbpSymLoadModuleSymbols(IN PUNICODE_STRING FileName,
                         OUT PROSSYM_INFO *RosSymInfo)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE FileHandle;
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;

  /* Allow KDB to break on module load */
  KdbModuleLoaded(FileName);

  /*  Try to find cached (already loaded) symbol file  */
  *RosSymInfo = KdbpSymFindCachedFile(FileName);
  if (*RosSymInfo != NULL)
    {
      DPRINT("Found cached symbol file %wZ\n", FileName);
      return;
    }

  /*  Open the file  */
  InitializeObjectAttributes(&ObjectAttributes,
                             FileName,
                             0,
                             NULL,
                             NULL);

  DPRINT("Attempting to open image: %wZ\n", FileName);

  Status = ZwOpenFile(&FileHandle,
                      FILE_READ_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      FILE_SHARE_READ|FILE_SHARE_WRITE,
                      FILE_SYNCHRONOUS_IO_NONALERT|FILE_NO_INTERMEDIATE_BUFFERING);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not open image file: %wZ\n", &FileName);
      return;
    }

  DPRINT("Loading symbols from %wZ...\n", FileName);

  if (! RosSymCreateFromFile(&FileHandle, RosSymInfo))
    {
      DPRINT("Failed to load symbols from %wZ\n", FileName);
      return;
    }

  ZwClose(FileHandle);

  DPRINT("Symbols loaded.\n");

  /* add file to cache */
  KdbpSymAddCachedFile(FileName, *RosSymInfo);

  DPRINT("Installed symbols: %wZ %p\n", FileName, *RosSymInfo);
}

/*! \brief Unloads symbol info.
 *
 * \param RosSymInfo  Pointer to the symbol info to unload.
 *
 * \sa KdbpSymLoadModuleSymbols
 */
STATIC VOID
KdbpSymUnloadModuleSymbols(IN PROSSYM_INFO RosSymInfo)
{
  DPRINT("Unloading symbols\n");

  if (RosSymInfo != NULL)
    {
      KdbpSymRemoveCachedFile(RosSymInfo);
    }
}

/*! \brief Load symbol info for a user module.
 *
 * \param LdrModule Pointer to the module to load symbols for.
 */
VOID
KdbSymLoadUserModuleSymbols(IN PLDR_MODULE LdrModule)
{
  static WCHAR Prefix[] = L"\\??\\";
  UNICODE_STRING KernelName;
  DPRINT("LdrModule %p\n", LdrModule);

  KernelName.MaximumLength = sizeof(Prefix) + LdrModule->FullDllName.Length;
  KernelName.Length = KernelName.MaximumLength - sizeof(WCHAR);
  KernelName.Buffer = ExAllocatePoolWithTag(PagedPool, KernelName.MaximumLength, TAG_KDBS);
  if (NULL == KernelName.Buffer)
    {
      return;
    }
  memcpy(KernelName.Buffer, Prefix, sizeof(Prefix) - sizeof(WCHAR));
  memcpy(KernelName.Buffer + sizeof(Prefix) / sizeof(WCHAR) - 1, LdrModule->FullDllName.Buffer,
         LdrModule->FullDllName.Length);
  KernelName.Buffer[KernelName.Length / sizeof(WCHAR)] = L'\0';

  LdrModule->RosSymInfo = NULL;

  KdbpSymLoadModuleSymbols(&KernelName, &LdrModule->RosSymInfo);

  ExFreePool(KernelName.Buffer);
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
  PEPROCESS CurrentProcess;
  PPEB Peb;

  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
  {
    KeAttachProcess(EPROCESS_TO_KPROCESS(Process));
  }
  Peb = Process->Peb;
  ASSERT(Peb);
  ASSERT(Peb->Ldr);

  CurrentEntry = Peb->Ldr->InLoadOrderModuleList.Flink;
  while (CurrentEntry != &Peb->Ldr->InLoadOrderModuleList &&
	 CurrentEntry != NULL)
    {
      Current = CONTAINING_RECORD(CurrentEntry, LDR_MODULE, InLoadOrderModuleList);

      KdbpSymUnloadModuleSymbols(Current->RosSymInfo);

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

  Module->TextSection->RosSymInfo = NULL;

  KdbpSymLoadModuleSymbols(Filename, &Module->TextSection->RosSymInfo);
}

/*! \brief Unloads symbol info for a driver.
 *
 * \param ModuleObject  Pointer to the driver MODULE_OBJECT.
 */
VOID
KdbSymUnloadDriverSymbols(IN PMODULE_OBJECT ModuleObject)
{
  /* Unload symbols for module if available */
  KdbpSymUnloadModuleSymbols(ModuleObject->TextSection->RosSymInfo);
  ModuleObject->TextSection->RosSymInfo = NULL;
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
  ANSI_STRING AnsiString;
  ULONG i;
  BOOLEAN IsRaw;

  DPRINT("KdbSymProcessBootSymbols(%s)\n", FileName);

  if (0 == _stricmp(FileName, "ntoskrnl.sym"))
    {
      RtlInitAnsiString(&AnsiString, "ntoskrnl.exe");
      IsRaw = TRUE;
    }
  else
    {
      RtlInitAnsiString(&AnsiString, FileName);
      IsRaw = FALSE;
    }
  RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
  ModuleObject = LdrGetModuleObject(&UnicodeString);
  RtlFreeUnicodeString(&UnicodeString);

  if (ModuleObject != NULL)
  {
     for (i = 0; i < KeLoaderBlock.ModsCount; i++)
     {
        if (0 == _stricmp(FileName, (PCHAR)KeLoaderModules[i].String))
	{
	   break;
	}
     }
     if (i < KeLoaderBlock.ModsCount)
     {
        KeLoaderModules[i].Reserved = 1;
        if (ModuleObject->TextSection->RosSymInfo != NULL)
        {
           KdbpSymRemoveCachedFile(ModuleObject->TextSection->RosSymInfo);
        }

        if (IsRaw)
        {
           if (! RosSymCreateFromRaw((PVOID) KeLoaderModules[i].ModStart,
                                     KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart,
                                     &ModuleObject->TextSection->RosSymInfo))
           {
              return;
           }
        }
        else
        {
           if (! RosSymCreateFromMem((PVOID) KeLoaderModules[i].ModStart,
                                     KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart,
                                     &ModuleObject->TextSection->RosSymInfo))
           {
              return;
           }
        }

        /* add file to cache */
        RtlInitAnsiString(&AnsiString, FileName);
	RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
        KdbpSymAddCachedFile(&UnicodeString, ModuleObject->TextSection->RosSymInfo);
        RtlFreeUnicodeString(&UnicodeString);

        DPRINT("Installed symbols: %s@%08x-%08x %p\n",
	       FileName,
	       ModuleObject->Base,
	       ModuleObject->Length + ModuleObject->Base,
	       ModuleObject->TextSection->RosSymInfo);
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
  NtoskrnlTextSection->RosSymInfo = NULL;
  LdrHalTextSection->RosSymInfo = NULL;

  InitializeListHead(&SymbolFileListHead);
  KeInitializeSpinLock(&SymbolFileListLock);

  RosSymInitKernelMode();
}

/* EOF */

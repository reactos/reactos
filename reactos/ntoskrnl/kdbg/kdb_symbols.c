/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb_symbols.c
 * PURPOSE:         Getting symbol information...
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

typedef struct _IMAGE_SYMBOL_INFO_CACHE {
  LIST_ENTRY ListEntry;
  ULONG RefCount;
  UNICODE_STRING FileName;
  PROSSYM_INFO RosSymInfo;
} IMAGE_SYMBOL_INFO_CACHE, *PIMAGE_SYMBOL_INFO_CACHE;

static BOOLEAN LoadSymbols;
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
  PLDR_DATA_TABLE_ENTRY current;
  PEPROCESS CurrentProcess;
  PPEB Peb = NULL;
  INT Count = 0;
  INT Length;

  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != NULL)
    {
      Peb = CurrentProcess->Peb;
    }

  if (Peb == NULL || Peb->Ldr == NULL)
    {
      return FALSE;
    }

  current_entry = Peb->Ldr->InLoadOrderModuleList.Flink;

  while (current_entry != &Peb->Ldr->InLoadOrderModuleList &&
         current_entry != NULL)
    {
      current = CONTAINING_RECORD(current_entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
      Length = min(current->BaseDllName.Length / sizeof(WCHAR), 255);
      if ((Address != NULL && (Address >= (PVOID)current->DllBase &&
                               Address < (PVOID)((char *)current->DllBase + current->SizeOfImage))) ||
          (Name != NULL && _wcsnicmp(current->BaseDllName.Buffer, Name, Length) == 0) ||
          (Index >= 0 && Count++ == Index))
        {
	  wcsncpy(pInfo->Name, current->BaseDllName.Buffer, Length);
	  pInfo->Name[Length] = L'\0';
          pInfo->Base = (ULONG_PTR)current->DllBase;
          pInfo->Size = current->SizeOfImage;
          pInfo->RosSymInfo = current->PatchInformation;
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
  PLDR_DATA_TABLE_ENTRY current;
  INT Count = 0;
  INT Length;

  current_entry = PsLoadedModuleList.Flink;

  while (current_entry != &PsLoadedModuleList)
    {
      current = CONTAINING_RECORD(current_entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

      Length = min(current->BaseDllName.Length / sizeof(WCHAR), 255);
      if ((Address != NULL && (Address >= (PVOID)current->DllBase &&
                               Address < (PVOID)((ULONG_PTR)current->DllBase + current->SizeOfImage))) ||
          (Name != NULL && _wcsnicmp(current->BaseDllName.Buffer, Name, Length) == 0) ||
          (Index >= 0 && Count++ == Index))
        {
	  wcsncpy(pInfo->Name, current->BaseDllName.Buffer, Length);
	  pInfo->Name[Length] = L'\0';
          pInfo->Base = (ULONG_PTR)current->DllBase;
          pInfo->Size = current->SizeOfImage;
          pInfo->RosSymInfo = current->PatchInformation;
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
  RtlCreateUnicodeString(&CacheEntry->FileName, FileName->Buffer);
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

  if (! LoadSymbols)
    {
      *RosSymInfo = NULL;
      return;
    }

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
                      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not open image file: %wZ\n", FileName);
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
KdbSymLoadUserModuleSymbols(IN PLDR_DATA_TABLE_ENTRY LdrModule)
{
  static WCHAR Prefix[] = L"\\??\\";
  UNICODE_STRING KernelName;
  DPRINT("LdrModule %p\n", LdrModule);

  LdrModule->PatchInformation = NULL;

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

  KdbpSymLoadModuleSymbols(&KernelName, (PROSSYM_INFO*)&LdrModule->PatchInformation);

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
  PLDR_DATA_TABLE_ENTRY Current;
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
      Current = CONTAINING_RECORD(CurrentEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

      KdbpSymUnloadModuleSymbols(Current->PatchInformation);

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
 * \param Module    Pointer to the driver LDR_DATA_TABLE_ENTRY.
 */
VOID
KdbSymLoadDriverSymbols(IN PUNICODE_STRING Filename,
                        IN PLDR_DATA_TABLE_ENTRY Module)
{
  /* Load symbols for the image if available */
  DPRINT("Loading driver %wZ symbols (driver @ %08x)\n", Filename, Module->DllBase);

  Module->PatchInformation = NULL;

  KdbpSymLoadModuleSymbols(Filename, (PROSSYM_INFO*)&Module->PatchInformation);
}

/*! \brief Unloads symbol info for a driver.
 *
 * \param ModuleObject  Pointer to the driver LDR_DATA_TABLE_ENTRY.
 */
VOID
KdbSymUnloadDriverSymbols(IN PLDR_DATA_TABLE_ENTRY ModuleObject)
{
  /* Unload symbols for module if available */
  KdbpSymUnloadModuleSymbols(ModuleObject->PatchInformation);
  ModuleObject->PatchInformation = NULL;
}

VOID
KdbSymProcessSymbols(IN PANSI_STRING AnsiFileName, IN PKD_SYMBOLS_INFO SymbolInfo)
{
    BOOLEAN Found = FALSE;
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;

    //DPRINT("KdbSymProcessSymbols(%Z)\n", AnsiFileName);

    /* We use PsLoadedModuleList here, otherwise (in case of
       using KeLoaderBlock) all our data will be just lost */
    ListHead = &PsLoadedModuleList;

    /* Found module we are interested in */
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        if (SymbolInfo->BaseOfDll == LdrEntry->DllBase)
        {
            Found = TRUE;
            break;
        }

        /* Go to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Exit if we didn't find the module requested */
    if (!Found)
        return;

    DPRINT("Found LdrEntry=%p\n", LdrEntry);
    if (!LoadSymbols)
    {
        LdrEntry->PatchInformation = NULL;
        return;
    }

    /* Remove symbol info if it already exists */
    if (LdrEntry->PatchInformation != NULL)
    {
        KdbpSymRemoveCachedFile(LdrEntry->PatchInformation);
    }

    /* Load new symbol information */
    if (! RosSymCreateFromMem(LdrEntry->DllBase,
        LdrEntry->SizeOfImage,
        (PROSSYM_INFO*)&LdrEntry->PatchInformation))
    {
        /* Error loading symbol info, try to load it from file */
        KdbpSymLoadModuleSymbols(&LdrEntry->FullDllName,
            (PROSSYM_INFO*)&LdrEntry->PatchInformation);

        /* It already added symbols to cache */
    }
    else
    {
        /* Add file to cache */
        KdbpSymAddCachedFile(&LdrEntry->FullDllName, LdrEntry->PatchInformation);
    }

    DPRINT("Installed symbols: %wZ@%08x-%08x %p\n",
           &LdrEntry->BaseDllName,
           LdrEntry->DllBase,
           LdrEntry->SizeOfImage + (ULONG)LdrEntry->DllBase,
           LdrEntry->PatchInformation);

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
KdbSymProcessBootSymbols(IN PANSI_STRING AnsiFileName,
                         IN BOOLEAN FullName,
                         IN BOOLEAN LoadFromFile)
{
    BOOLEAN Found = FALSE;
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING ModuleName;
    NTSTATUS Status;

    /* Convert file name to unicode */
    ModuleName.MaximumLength = (MAX_PATH-1)*sizeof(WCHAR);
    ModuleName.Length = 0;
    ModuleName.Buffer = Buffer;

    Status = RtlAnsiStringToUnicodeString(&ModuleName, AnsiFileName, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to convert Ansi to Unicode with Status=0x%08X, Length=%d\n",
            Status, ModuleName.Length);
        return;
    }

    DPRINT("KdbSymProcessBootSymbols(%wZ)\n", &ModuleName);

    /* We use PsLoadedModuleList here, otherwise (in case of
       using KeLoaderBlock) all our data will be just lost */
    ListHead = &PsLoadedModuleList;

    /* Found module we are interested in */
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        if (FullName)
        {
            if (RtlEqualUnicodeString(&ModuleName, &LdrEntry->FullDllName, TRUE))
            {
                Found = TRUE;
                break;
            }
        }
        else
        {
            if (RtlEqualUnicodeString(&ModuleName, &LdrEntry->BaseDllName, TRUE))
            {
                Found = TRUE;
                break;
            }
        }

        /* Go to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Exit if we didn't find the module requested */
    if (!Found)
        return;

    DPRINT("Found LdrEntry=%p\n", LdrEntry);
    if (!LoadSymbols)
    {
        LdrEntry->PatchInformation = NULL;
        return;
    }

    /* Remove symbol info if it already exists */
    if (LdrEntry->PatchInformation != NULL)
    {
        KdbpSymRemoveCachedFile(LdrEntry->PatchInformation);
    }

    if (LoadFromFile)
    {
        /* Load symbol info from file */
        KdbpSymLoadModuleSymbols(&LdrEntry->FullDllName,
            (PROSSYM_INFO*)&LdrEntry->PatchInformation);
    }
    else
    {
        /* Load new symbol information */
        if (! RosSymCreateFromMem(LdrEntry->DllBase,
            LdrEntry->SizeOfImage,
            (PROSSYM_INFO*)&LdrEntry->PatchInformation))
        {
            /* Error loading symbol info, exit */
            return;
        }

        /* Add file to cache */
        KdbpSymAddCachedFile(&ModuleName, LdrEntry->PatchInformation);
    }

    DPRINT("Installed symbols: %wZ@%08x-%08x %p\n",
           &ModuleName,
           LdrEntry->DllBase,
           LdrEntry->SizeOfImage + (ULONG)LdrEntry->DllBase,
           LdrEntry->PatchInformation);
}

VOID
NTAPI
KdbDebugPrint(PCH Message, ULONG Length)
{
    /* Nothing here */
}


/*! \brief Initializes the KDB symbols implementation.
 *
 * \param DispatchTable         Pointer to the KD dispatch table
 * \param BootPhase             Phase of initialization
 */
VOID
NTAPI
KdbInitialize(PKD_DISPATCH_TABLE DispatchTable,
              ULONG BootPhase)
{
    PCHAR p1, p2;
    int Found;
    char YesNo;
    ANSI_STRING FileName;

    DPRINT("KdbSymInit() BootPhase=%d\n", BootPhase);

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpKdbgInit;
        DispatchTable->KdpPrintRoutine = KdbDebugPrint;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);

        /* Perform actual initialization of symbol module */
        //NtoskrnlModuleObject->PatchInformation = NULL;
        //LdrHalModuleObject->PatchInformation = NULL;

        InitializeListHead(&SymbolFileListHead);
        KeInitializeSpinLock(&SymbolFileListLock);

#ifdef DBG
        LoadSymbols = TRUE;
#else
        LoadSymbols = FALSE;
#endif

        /* Check the command line for /LOADSYMBOLS, /NOLOADSYMBOLS,
        * /LOADSYMBOLS={YES|NO}, /NOLOADSYMBOLS={YES|NO} */
        ASSERT(KeLoaderBlock);
        p1 = KeLoaderBlock->LoadOptions;
        while('\0' != *p1 && NULL != (p2 = strchr(p1, '/')))
        {
            p2++;
            Found = 0;
            if (0 == _strnicmp(p2, "LOADSYMBOLS", 11))
            {
                Found = +1;
                p2 += 11;
            }
            else if (0 == _strnicmp(p2, "NOLOADSYMBOLS", 13))
            {
                Found = -1;
                p2 += 13;
            }
            if (0 != Found)
            {
                while (isspace(*p2))
                {
                    p2++;
                }
                if ('=' == *p2)
                {
                    p2++;
                    while (isspace(*p2))
                    {
                        p2++;
                    }
                    YesNo = toupper(*p2);
                    if ('N' == YesNo || 'F' == YesNo || '0' == YesNo)
                    {
                        Found = -1 * Found;
                    }
                }
                LoadSymbols = (0 < Found);
            }
            p1 = p2;
        }

        RosSymInitKernelMode();
    }
    else if (BootPhase == 1)
    {
        /* Load symbols for NTOSKRNL.EXE and HAL.DLL*/
        /* FIXME: Load as 1st and 2nd entries of InLoadOrderList instead
                  of hardcoding them here! */
        RtlInitAnsiString(&FileName, "\\SystemRoot\\System32\\NTOSKRNL.EXE");
        KdbSymProcessBootSymbols(&FileName, TRUE, FALSE);
        RtlInitAnsiString(&FileName, "\\SystemRoot\\System32\\HAL.DLL");
        KdbSymProcessBootSymbols(&FileName, TRUE, FALSE);
    }
}

/* EOF */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kdbg/kdb_symbols.c
 * PURPOSE:         Getting symbol information...
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Colin Finck (colin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

typedef struct _IMAGE_SYMBOL_INFO_CACHE
{
    LIST_ENTRY ListEntry;
    ULONG RefCount;
    UNICODE_STRING FileName;
    PROSSYM_INFO RosSymInfo;
}
IMAGE_SYMBOL_INFO_CACHE, *PIMAGE_SYMBOL_INFO_CACHE;

static BOOLEAN LoadSymbols;
static LIST_ENTRY SymbolFileListHead;
static KSPIN_LOCK SymbolFileListLock;
BOOLEAN KdbpSymbolsInitialized = FALSE;

/* FUNCTIONS ****************************************************************/

static BOOLEAN
KdbpSymSearchModuleList(
    IN PLIST_ENTRY current_entry,
    IN PLIST_ENTRY end_entry,
    IN PLONG Count,
    IN PVOID Address,
    IN LPCWSTR Name,
    IN INT Index,
    OUT PLDR_DATA_TABLE_ENTRY* pLdrEntry)
{
    while (current_entry && current_entry != end_entry)
    {
        *pLdrEntry = CONTAINING_RECORD(current_entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if ((Address && Address >= (PVOID)(*pLdrEntry)->DllBase && Address < (PVOID)((ULONG_PTR)(*pLdrEntry)->DllBase + (*pLdrEntry)->SizeOfImage)) ||
            (Name && !_wcsnicmp((*pLdrEntry)->BaseDllName.Buffer, Name, (*pLdrEntry)->BaseDllName.Length / sizeof(WCHAR))) ||
            (Index >= 0 && (*Count)++ == Index))
        {
            return TRUE;
        }

        current_entry = current_entry->Flink;
    }

    return FALSE;
}

/*! \brief Find a module...
 *
 * \param Address      If \a Address is not NULL the module containing \a Address
 *                     is searched.
 * \param Name         If \a Name is not NULL the module named \a Name will be
 *                     searched.
 * \param Index        If \a Index is >= 0 the Index'th module will be returned.
 * \param pLdrEntry    Pointer to a PLDR_DATA_TABLE_ENTRY which is filled.
 *
 * \retval TRUE    Module was found, \a pLdrEntry was filled.
 * \retval FALSE   No module was found.
 */
BOOLEAN
KdbpSymFindModule(
    IN PVOID Address  OPTIONAL,
    IN LPCWSTR Name  OPTIONAL,
    IN INT Index  OPTIONAL,
    OUT PLDR_DATA_TABLE_ENTRY* pLdrEntry)
{
    LONG Count = 0;
    PEPROCESS CurrentProcess;

    /* First try to look up the module in the kernel module list. */
    if(KdbpSymSearchModuleList(PsLoadedModuleList.Flink,
                               &PsLoadedModuleList,
                               &Count,
                               Address,
                               Name,
                               Index,
                               pLdrEntry))
    {
        return TRUE;
    }

    /* That didn't succeed. Try the module list of the current process now. */
    CurrentProcess = PsGetCurrentProcess();

    if(!CurrentProcess || !CurrentProcess->Peb || !CurrentProcess->Peb->Ldr)
        return FALSE;

    return KdbpSymSearchModuleList(CurrentProcess->Peb->Ldr->InLoadOrderModuleList.Flink,
                                   &CurrentProcess->Peb->Ldr->InLoadOrderModuleList,
                                   &Count,
                                   Address,
                                   Name,
                                   Index,
                                   pLdrEntry);
}

PCHAR
NTAPI
KdbpSymUnicodeToAnsi(IN PUNICODE_STRING Unicode,
                     OUT PCHAR Ansi,
                     IN ULONG Length)
{
    PCHAR p;
    PWCHAR pw;
    ULONG i;

    /* Set length and normalize it */
    i = Unicode->Length / sizeof(WCHAR);
    i = min(i, Length - 1);

    /* Set source and destination, and copy */
    pw = Unicode->Buffer;
    p = Ansi;
    while (i--) *p++ = (CHAR)*pw++;

    /* Null terminate and return */
    *p = ANSI_NULL;
    return Ansi;
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
KdbSymPrintAddress(
    IN PVOID Address,
    IN PKTRAP_FRAME Context)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG_PTR RelativeAddress;
    NTSTATUS Status;
    ULONG LineNumber;
    CHAR FileName[256];
    CHAR FunctionName[256];
    CHAR ModuleNameAnsi[64];

    if (!KdbpSymbolsInitialized || !KdbpSymFindModule(Address, NULL, -1, &LdrEntry))
        return FALSE;
        
    KdbpSymUnicodeToAnsi(&LdrEntry->BaseDllName,
                         ModuleNameAnsi,
                         sizeof(ModuleNameAnsi));

    RelativeAddress = (ULONG_PTR)Address - (ULONG_PTR)LdrEntry->DllBase;
    Status = KdbSymGetAddressInformation(LdrEntry->PatchInformation,
                                         RelativeAddress,
                                         &LineNumber,
                                         FileName,
                                         FunctionName);
    if (NT_SUCCESS(Status))
    {
        DbgPrint("<%s:%x (%s:%d (%s))>",
            ModuleNameAnsi, RelativeAddress, FileName, LineNumber, FunctionName);
    }
    else
    {
        DbgPrint("<%s:%x>", ModuleNameAnsi, RelativeAddress);
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
KdbSymGetAddressInformation(
    IN PROSSYM_INFO RosSymInfo,
    IN ULONG_PTR RelativeAddress,
    OUT PULONG LineNumber  OPTIONAL,
    OUT PCH FileName  OPTIONAL,
    OUT PCH FunctionName  OPTIONAL)
{
    if (!KdbpSymbolsInitialized ||
        !RosSymInfo ||
        !RosSymGetAddressInformation(RosSymInfo, RelativeAddress, LineNumber, FileName, FunctionName))
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
static PROSSYM_INFO
KdbpSymFindCachedFile(
    IN PUNICODE_STRING FileName)
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
static VOID
KdbpSymAddCachedFile(
    IN PUNICODE_STRING FileName,
    IN PROSSYM_INFO RosSymInfo)
{
    PIMAGE_SYMBOL_INFO_CACHE CacheEntry;
    KIRQL Irql;

    DPRINT("Adding symbol file: RosSymInfo = %p\n", RosSymInfo);

    /* allocate entry */
    CacheEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof (IMAGE_SYMBOL_INFO_CACHE), TAG_KDBS);
    ASSERT(CacheEntry);
    RtlZeroMemory(CacheEntry, sizeof (IMAGE_SYMBOL_INFO_CACHE));

    /* fill entry */
    CacheEntry->FileName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                        FileName->Length,
                                                        TAG_KDBS);
    RtlCopyUnicodeString(&CacheEntry->FileName, FileName);
    ASSERT(CacheEntry->FileName.Buffer);
    CacheEntry->RefCount = 1;
    CacheEntry->RosSymInfo = RosSymInfo;
    KeAcquireSpinLock(&SymbolFileListLock, &Irql);
    InsertTailList(&SymbolFileListHead, &CacheEntry->ListEntry);
    KeReleaseSpinLock(&SymbolFileListLock, Irql);
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
static VOID
KdbpSymRemoveCachedFile(
    IN PROSSYM_INFO RosSymInfo)
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
static VOID
KdbpSymLoadModuleSymbols(
    IN PUNICODE_STRING FileName,
    OUT PROSSYM_INFO *RosSymInfo)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Allow KDB to break on module load */
    KdbModuleLoaded(FileName);

    if (!LoadSymbols)
    {
        *RosSymInfo = NULL;
        return;
    }

    /*  Try to find cached (already loaded) symbol file  */
    *RosSymInfo = KdbpSymFindCachedFile(FileName);
    if (*RosSymInfo)
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
                        FILE_READ_ACCESS | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not open image file: %wZ\n", FileName);
        return;
    }

    DPRINT("Loading symbols from %wZ...\n", FileName);

    if (!RosSymCreateFromFile(&FileHandle, RosSymInfo))
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

VOID
KdbSymProcessSymbols(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    if (!LoadSymbols)
    {
        LdrEntry->PatchInformation = NULL;
        return;
    }

    /* Remove symbol info if it already exists */
    if (LdrEntry->PatchInformation)
        KdbpSymRemoveCachedFile(LdrEntry->PatchInformation);

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

    DPRINT("Installed symbols: %wZ@%p-%p %p\n",
           &LdrEntry->BaseDllName,
           LdrEntry->DllBase,
           (PVOID)(LdrEntry->SizeOfImage + (ULONG_PTR)LdrEntry->DllBase),
           LdrEntry->PatchInformation);

}

VOID
NTAPI
KdbDebugPrint(
    PCH Message,
    ULONG Length)
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
KdbInitialize(
    PKD_DISPATCH_TABLE DispatchTable,
    ULONG BootPhase)
{
    PCHAR p1, p2;
    SHORT Found = FALSE;
    CHAR YesNo;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    DPRINT("KdbSymInit() BootPhase=%d\n", BootPhase);

    LoadSymbols = FALSE;

#if DBG
    /* Load symbols only if we have 96Mb of RAM or more */
    if (MmNumberOfPhysicalPages >= 0x6000)
        LoadSymbols = TRUE;
#endif

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
        /* Load symbols for NTOSKRNL.EXE.
           It is always the first module in PsLoadedModuleList. KeLoaderBlock can't be used here as its content is just temporary. */
        LdrEntry = CONTAINING_RECORD(PsLoadedModuleList.Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        KdbSymProcessSymbols(LdrEntry);

        /* Also load them for HAL.DLL. */
        LdrEntry = CONTAINING_RECORD(PsLoadedModuleList.Flink->Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        KdbSymProcessSymbols(LdrEntry);

        KdbpSymbolsInitialized = TRUE;
    }
}

/* EOF */

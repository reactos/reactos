/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb_symbols.c
 * PURPOSE:         Getting symbol information...
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Colin Finck (colin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <cache/section/newmm.h>
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

typedef struct _ROSSYM_KM_OWN_CONTEXT {
    LARGE_INTEGER FileOffset;
    PFILE_OBJECT FileObject;
} ROSSYM_KM_OWN_CONTEXT, *PROSSYM_KM_OWN_CONTEXT;

static BOOLEAN LoadSymbols;
static LIST_ENTRY SymbolFileListHead;
static KSPIN_LOCK SymbolFileListLock;
//static PROSSYM_INFO KdbpRosSymInfo;
//static ULONG_PTR KdbpImageBase;
BOOLEAN KdbpSymbolsInitialized = FALSE;

/* FUNCTIONS ****************************************************************/

static BOOLEAN
KdbpSeekSymFile(PVOID FileContext, ULONG_PTR Target)
{
    PROSSYM_KM_OWN_CONTEXT Context = (PROSSYM_KM_OWN_CONTEXT)FileContext;
    Context->FileOffset.QuadPart = Target;
    return TRUE;
}

static BOOLEAN
KdbpReadSymFile(PVOID FileContext, PVOID Buffer, ULONG Length)
{
    PROSSYM_KM_OWN_CONTEXT Context = (PROSSYM_KM_OWN_CONTEXT)FileContext;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status = MiSimpleRead
        (Context->FileObject,
         &Context->FileOffset,
         Buffer,
         Length,
         FALSE,
         &Iosb);
    return NT_SUCCESS(Status);
}

static PROSSYM_KM_OWN_CONTEXT
KdbpCaptureFileForSymbols(PFILE_OBJECT FileObject)
{
    PROSSYM_KM_OWN_CONTEXT Context = ExAllocatePool(NonPagedPool, sizeof(*Context));
    if (!Context) return NULL;
    ObReferenceObject(FileObject);
    Context->FileOffset.QuadPart = 0;
    Context->FileObject = FileObject;
    return Context;
}

static VOID
KdbpReleaseFileForSymbols(PROSSYM_KM_OWN_CONTEXT Context)
{
    ObDereferenceObject(Context->FileObject);
    ExFreePool(Context);
}

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
    int i;
	PMEMORY_AREA MemoryArea = NULL;
	PROS_SECTION_OBJECT SectionObject;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
#if 0
    PROSSYM_KM_OWN_CONTEXT FileContext;
#endif
    ULONG_PTR RelativeAddress;
    NTSTATUS Status;
	ROSSYM_LINEINFO LineInfo = {0};

    struct {
        enum _ROSSYM_REGNAME regname;
        size_t ctx_offset;
    } regmap[] = {
        { ROSSYM_X86_EDX, FIELD_OFFSET(KTRAP_FRAME, Edx) },
        { ROSSYM_X86_EAX, FIELD_OFFSET(KTRAP_FRAME, Eax) },
        { ROSSYM_X86_ECX, FIELD_OFFSET(KTRAP_FRAME, Ecx) },
        { ROSSYM_X86_EBX, FIELD_OFFSET(KTRAP_FRAME, Ebx) },
        { ROSSYM_X86_ESI, FIELD_OFFSET(KTRAP_FRAME, Esi) },
        { ROSSYM_X86_EDI, FIELD_OFFSET(KTRAP_FRAME, Edi) },
        { ROSSYM_X86_EBP, FIELD_OFFSET(KTRAP_FRAME, Ebp) },
        { ROSSYM_X86_ESP, FIELD_OFFSET(KTRAP_FRAME, HardwareEsp) }
    };

    if (Context)
    {
#if 0
        // Disable arguments for now
        DPRINT("Has Context %x (EBP %x)\n", Context, Context->Ebp);
        LineInfo.Flags = ROSSYM_LINEINFO_HAS_REGISTERS;
#endif

        for (i = 0; i < sizeof(regmap) / sizeof(regmap[0]); i++) {
            memcpy
                (&LineInfo.Registers.Registers[regmap[i].regname],
                 ((PCHAR)Context)+regmap[i].ctx_offset,
                 sizeof(ULONG_PTR));
            DPRINT("DWARF REG[%d] -> %x\n", regmap[i].regname, LineInfo.Registers.Registers[regmap[i].regname]);
        }
    }

    if (!KdbpSymbolsInitialized || !KdbpSymFindModule(Address, NULL, -1, &LdrEntry))
        return FALSE;

    RelativeAddress = (ULONG_PTR)Address - (ULONG_PTR)LdrEntry->DllBase;
    Status = KdbSymGetAddressInformation
		(LdrEntry->PatchInformation,
		 RelativeAddress,
		 &LineInfo);

    if (NT_SUCCESS(Status))
    {
        DbgPrint("<%wZ:%x (%s:%d (%s))>",
            &LdrEntry->BaseDllName, RelativeAddress, LineInfo.FileName, LineInfo.LineNumber, LineInfo.FunctionName);
        if (Context && LineInfo.NumParams)
        {
            int i;
            char *comma = "";
            DbgPrint("(");
            for (i = 0; i < LineInfo.NumParams; i++) {
                DbgPrint
                    ("%s%s=%llx",
                     comma,
                     LineInfo.Parameters[i].ValueName,
                     LineInfo.Parameters[i].Value);
                comma = ",";
            }
            DbgPrint(")");
        }

		return TRUE;
    }
	else if (Address < MmSystemRangeStart)
	{
		MemoryArea = MmLocateMemoryAreaByAddress(&PsGetCurrentProcess()->Vm, Address);
		if (!MemoryArea || MemoryArea->Type != MEMORY_AREA_SECTION_VIEW)
		{
			goto end;
		}

		SectionObject = MemoryArea->Data.SectionData.Section;
		if (!(SectionObject->AllocationAttributes & SEC_IMAGE)) goto end;
#if 0
		if (MemoryArea->StartingAddress != (PVOID)KdbpImageBase)
		{
			if (KdbpRosSymInfo)
			{
				RosSymDelete(KdbpRosSymInfo);
				KdbpRosSymInfo = NULL;
                KdbpImageBase = 0;
			}

            if ((FileContext = KdbpCaptureFileForSymbols(SectionObject->FileObject)))
			{
                if (RosSymCreateFromFile(FileContext, &KdbpRosSymInfo))
                    KdbpImageBase = (ULONG_PTR)MemoryArea->StartingAddress;

                KdbpReleaseFileForSymbols(FileContext);
			}
		}

		if (KdbpRosSymInfo)
		{
			RelativeAddress = (ULONG_PTR)Address - KdbpImageBase;
			RosSymFreeInfo(&LineInfo);
			Status = KdbSymGetAddressInformation
				(KdbpRosSymInfo,
				 RelativeAddress,
				 &LineInfo);
			if (NT_SUCCESS(Status))
			{
				DbgPrint
					("<%wZ:%x (%s:%d (%s))>",
					 &SectionObject->FileObject->FileName,
					 RelativeAddress,
					 LineInfo.FileName,
					 LineInfo.LineNumber,
					 LineInfo.FunctionName);

                if (Context && LineInfo.NumParams)
                {
                    int i;
                    char *comma = "";
                    DbgPrint("(");
                    for (i = 0; i < LineInfo.NumParams; i++) {
                        DbgPrint
                            ("%s%s=%llx",
                             comma,
                             LineInfo.Parameters[i].ValueName,
                             LineInfo.Parameters[i].Value);
                        comma = ",";
                    }
                    DbgPrint(")");
                }

				return TRUE;
			}
		}
#endif
	}

end:
	DbgPrint("<%wZ:%x>", &LdrEntry->BaseDllName, RelativeAddress);

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
	IN PROSSYM_LINEINFO LineInfo)
{
    if (!KdbpSymbolsInitialized ||
        !RosSymInfo ||
        !RosSymGetAddressInformation(RosSymInfo, RelativeAddress, LineInfo))
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
PROSSYM_INFO
KdbpSymFindCachedFile(
    IN PUNICODE_STRING FileName)
{
    PIMAGE_SYMBOL_INFO_CACHE Current;
    PLIST_ENTRY CurrentEntry;
    KIRQL Irql;

    KeAcquireSpinLock(&SymbolFileListLock, &Irql);

    CurrentEntry = SymbolFileListHead.Flink;
    while (CurrentEntry != (&SymbolFileListHead))
    {
        Current = CONTAINING_RECORD(CurrentEntry, IMAGE_SYMBOL_INFO_CACHE, ListEntry);

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

    DPRINT("Adding symbol file: %wZ RosSymInfo = %p\n", FileName, RosSymInfo);

    /* allocate entry */
    CacheEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof (IMAGE_SYMBOL_INFO_CACHE), TAG_KDBS);
    ASSERT(CacheEntry);
    RtlZeroMemory(CacheEntry, sizeof (IMAGE_SYMBOL_INFO_CACHE));

    /* fill entry */
    CacheEntry->FileName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                        FileName->Length,
                                                        TAG_KDBS);
    CacheEntry->FileName.MaximumLength = FileName->Length;
    RtlCopyUnicodeString(&CacheEntry->FileName, FileName);
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
}

/*! \brief Loads a symbol file.
 *
 * \param FileName    Filename of the symbol file to load.
 * \param RosSymInfo  Pointer to a ROSSYM_INFO which gets filled.
 *
 * \sa KdbpSymUnloadModuleSymbols
 */
VOID
KdbpSymLoadModuleSymbols(
    IN PUNICODE_STRING FileName,
    OUT PROSSYM_INFO *RosSymInfo)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_OBJECT FileObject;
    PROSSYM_KM_OWN_CONTEXT FileContext;

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
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    DPRINT("Attempting to open image: %wZ\n", FileName);

    Status = ZwOpenFile(&FileHandle,
                        FILE_READ_ACCESS | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not open image file(%x): %wZ\n", Status, FileName);
        return;
    }

    DPRINT("Loading symbols from %wZ...\n", FileName);

    Status = ObReferenceObjectByHandle
        (FileHandle,
         FILE_READ_DATA | SYNCHRONIZE,
         NULL,
         KernelMode,
         (PVOID*)&FileObject,
         NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not get the file object\n");
        ZwClose(FileHandle);
        return;
    }

    if ((FileContext = KdbpCaptureFileForSymbols(FileObject)))
    {
        if (RosSymCreateFromFile(FileContext, RosSymInfo))
        {
            /* add file to cache */
            int i;
            UNICODE_STRING TruncatedName = *FileName;
            for (i = (TruncatedName.Length / sizeof(WCHAR)) - 1; i >= 0; i--)
                if (TruncatedName.Buffer[i] == '\\') {
                    TruncatedName.Buffer += i+1;
                    TruncatedName.Length -= (i+1)*sizeof(WCHAR);
                    TruncatedName.MaximumLength -= (i+1)*sizeof(WCHAR);
                    break;
                }
            KdbpSymAddCachedFile(&TruncatedName, *RosSymInfo);
            DPRINT("Installed symbols: %wZ %p\n", &TruncatedName, *RosSymInfo);
        }
        KdbpReleaseFileForSymbols(FileContext);
    }

    ObDereferenceObject(FileObject);
    ZwClose(FileHandle);
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
    if (LdrEntry->PatchInformation) {
        KdbpSymRemoveCachedFile(LdrEntry->PatchInformation);
    }

	/* Error loading symbol info, try to load it from file */
	KdbpSymLoadModuleSymbols(&LdrEntry->FullDllName,
            (PROSSYM_INFO*)&LdrEntry->PatchInformation);

    if (!LdrEntry->PatchInformation) {
        // HACK: module dll names don't identify the real files
        UNICODE_STRING SystemRoot;
        UNICODE_STRING ModuleNameCopy;
        RtlInitUnicodeString(&SystemRoot, L"\\SystemRoot\\System32\\Drivers\\");
        ModuleNameCopy.Length = 0;
        ModuleNameCopy.MaximumLength =
            LdrEntry->BaseDllName.MaximumLength + SystemRoot.MaximumLength;
        ModuleNameCopy.Buffer = ExAllocatePool(NonPagedPool, SystemRoot.MaximumLength + LdrEntry->BaseDllName.MaximumLength);
        RtlCopyUnicodeString(&ModuleNameCopy, &SystemRoot);
        RtlCopyMemory
            (ModuleNameCopy.Buffer + ModuleNameCopy.Length / sizeof(WCHAR),
             LdrEntry->BaseDllName.Buffer,
             LdrEntry->BaseDllName.Length);
        ModuleNameCopy.Length += LdrEntry->BaseDllName.Length;
        KdbpSymLoadModuleSymbols(&ModuleNameCopy,
                                 (PROSSYM_INFO*)&LdrEntry->PatchInformation);
        if (!LdrEntry->PatchInformation) {
            SystemRoot.Length -= strlen("Drivers\\") * sizeof(WCHAR);
            RtlCopyUnicodeString(&ModuleNameCopy, &SystemRoot);
            RtlCopyMemory
                (ModuleNameCopy.Buffer + ModuleNameCopy.Length / sizeof(WCHAR),
                 LdrEntry->BaseDllName.Buffer,
                 LdrEntry->BaseDllName.Length);
            ModuleNameCopy.Length += LdrEntry->BaseDllName.Length;
            KdbpSymLoadModuleSymbols(&ModuleNameCopy,
                                     (PROSSYM_INFO*)&LdrEntry->PatchInformation);
        }
        RtlFreeUnicodeString(&ModuleNameCopy);
    }

	/* It already added symbols to cache */
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

static PVOID KdbpSymAllocMem(ULONG_PTR size)
{
	return ExAllocatePoolWithTag(NonPagedPool, size, 'RSYM');
}

static VOID KdbpSymFreeMem(PVOID Area)
{
	return ExFreePool(Area);
}

static BOOLEAN KdbpSymReadMem(PVOID FileContext, ULONG_PTR* TargetDebug, PVOID SourceMem, ULONG Size)
{
	return NT_SUCCESS(KdbpSafeReadMemory(TargetDebug, SourceMem, Size));
}

static ROSSYM_CALLBACKS KdbpRosSymCallbacks = {
	KdbpSymAllocMem, KdbpSymFreeMem,
	KdbpReadSymFile, KdbpSeekSymFile,
	KdbpSymReadMem
};

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

        RosSymInit(&KdbpRosSymCallbacks);
    }
    else if (BootPhase == 3)
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

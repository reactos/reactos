/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/utils.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Michael Martin
 */

/*
 * TODO:
 *      - Handle loading flags correctly
 *      - Handle errors correctly (unload dll's)
 *      - Implement a faster way to find modules (hash table)
 *      - any more ??
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

#define LDRP_PROCESS_CREATION_TIME 0xffff
#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))

/* GLOBALS *******************************************************************/

#ifdef NDEBUG
#define TRACE_LDR(...) if (RtlGetNtGlobalFlags() & FLG_SHOW_LDR_SNAPS) { DbgPrint("(LDR:%s:%d) ",__FILE__,__LINE__); DbgPrint(__VA_ARGS__); }
#endif

static BOOLEAN LdrpDllShutdownInProgress = FALSE;
extern HANDLE LdrpKnownDllObjectDirectory;
extern UNICODE_STRING LdrpKnownDllPath;
static PLDR_DATA_TABLE_ENTRY LdrpLastModule = NULL;
extern PLDR_DATA_TABLE_ENTRY LdrpImageEntry;

/* PROTOTYPES ****************************************************************/

static NTSTATUS LdrFindEntryForName(PUNICODE_STRING Name, PLDR_DATA_TABLE_ENTRY *Module, BOOLEAN Ref);
static PVOID LdrFixupForward(PCHAR ForwardName);
static PVOID LdrGetExportByName(PVOID BaseAddress, PUCHAR SymbolName, USHORT Hint);

NTSTATUS find_actctx_dll( LPCWSTR libname, WCHAR *fulldosname );
NTSTATUS create_module_activation_context( LDR_DATA_TABLE_ENTRY *module );

/* FUNCTIONS *****************************************************************/

static __inline LONG LdrpDecrementLoadCount(PLDR_DATA_TABLE_ENTRY Module, BOOLEAN Locked)
{
    LONG LoadCount;
    if (!Locked)
    {
        RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    }
    LoadCount = Module->LoadCount;
    if (Module->LoadCount > 0 && Module->LoadCount != LDRP_PROCESS_CREATION_TIME)
    {
        Module->LoadCount--;
    }
    if (!Locked)
    {
        RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
    }
    return LoadCount;
}

static __inline LONG LdrpIncrementLoadCount(PLDR_DATA_TABLE_ENTRY Module, BOOLEAN Locked)
{
    LONG LoadCount;
    if (!Locked)
    {
        RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    }
    LoadCount = Module->LoadCount;
    if (Module->LoadCount != LDRP_PROCESS_CREATION_TIME)
    {
        Module->LoadCount++;
    }
    if (!Locked)
    {
        RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
    }
    return LoadCount;
}

/***************************************************************************
 * NAME                                                         LOCAL
 *      LdrAdjustDllName
 *
 * DESCRIPTION
 *      Adjusts the name of a dll to a fully qualified name.
 *
 * ARGUMENTS
 *      FullDllName:    Pointer to caller supplied storage for the fully
 *                      qualified dll name.
 *      DllName:        Pointer to the dll name.
 *      BaseName:       TRUE:  Only the file name is passed to FullDllName
 *                      FALSE: The full path is preserved in FullDllName
 *
 * RETURN VALUE
 *      None
 *
 * REVISIONS
 *
 * NOTE
 *      A given path is not affected by the adjustment, but the file
 *      name only:
 *        ntdll      --> ntdll.dll
 *        ntdll.     --> ntdll
 *        ntdll.xyz  --> ntdll.xyz
 */
static VOID
LdrAdjustDllName (PUNICODE_STRING FullDllName,
                  PUNICODE_STRING DllName,
                  BOOLEAN BaseName)
{
    WCHAR Buffer[MAX_PATH];
    ULONG Length;
    PWCHAR Extension;
    PWCHAR Pointer;

    DPRINT1("\n");

    Length = DllName->Length / sizeof(WCHAR);

    if (BaseName)
    {
        /* get the base dll name */
        Pointer = DllName->Buffer + Length;
        Extension = Pointer;

        do
        {
            --Pointer;
        }
        while (Pointer >= DllName->Buffer && *Pointer != L'\\' && *Pointer != L'/');

        Pointer++;
        Length = Extension - Pointer;
        memmove (Buffer, Pointer, Length * sizeof(WCHAR));
        Buffer[Length] = L'\0';
    }
    else
    {
        /* get the full dll name */
        memmove (Buffer, DllName->Buffer, DllName->Length);
        Buffer[DllName->Length / sizeof(WCHAR)] = L'\0';
    }

    /* Build the DLL's absolute name */
    Extension = wcsrchr (Buffer, L'.');
    if ((Extension != NULL) && (*Extension == L'.'))
    {
        /* with extension - remove dot if it's the last character */
        if (Buffer[Length - 1] == L'.')
            Length--;
        Buffer[Length] = 0;
    }
    else
    {
        /* name without extension - assume that it is .dll */
        memmove (Buffer + Length, L".dll", 10);
    }

    RtlCreateUnicodeString(FullDllName, Buffer);
}

PLDR_DATA_TABLE_ENTRY
LdrAddModuleEntry(PVOID ImageBase,
                  PIMAGE_NT_HEADERS NTHeaders,
                  PWSTR FullDosName)
{
    PLDR_DATA_TABLE_ENTRY Module;

    DPRINT1("\n");

    Module = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof (LDR_DATA_TABLE_ENTRY));
    ASSERT(Module);
    memset(Module, 0, sizeof(LDR_DATA_TABLE_ENTRY));
    Module->DllBase = (PVOID)ImageBase;
    Module->EntryPoint = (PVOID)NTHeaders->OptionalHeader.AddressOfEntryPoint;
    if (Module->EntryPoint != 0)
        Module->EntryPoint = (PVOID)((ULONG_PTR)Module->EntryPoint + (ULONG_PTR)Module->DllBase);
    Module->SizeOfImage = LdrpGetResidentSize(NTHeaders);
    if (NtCurrentPeb()->Ldr->Initialized == TRUE)
    {
        /* loading while app is running */
        Module->LoadCount = 1;
    }
    else
    {
        /*
         * loading while app is initializing
         * dll must not be unloaded
         */
        Module->LoadCount = LDRP_PROCESS_CREATION_TIME;
    }

    Module->Flags = 0;
    Module->TlsIndex = -1;
    Module->CheckSum = NTHeaders->OptionalHeader.CheckSum;
    Module->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

    RtlCreateUnicodeString (&Module->FullDllName,
                            FullDosName);
    RtlCreateUnicodeString (&Module->BaseDllName,
                            wcsrchr(FullDosName, L'\\') + 1);
    DPRINT ("BaseDllName %wZ\n", &Module->BaseDllName);

    RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    InsertTailList(&NtCurrentPeb()->Ldr->InLoadOrderModuleList,
                   &Module->InLoadOrderLinks);
    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    return(Module);
}

/***************************************************************************
 * NAME                                                         LOCAL
 *      LdrFindEntryForName
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static NTSTATUS
LdrFindEntryForName(PUNICODE_STRING Name,
                    PLDR_DATA_TABLE_ENTRY *Module,
                    BOOLEAN Ref)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY ModulePtr;
    BOOLEAN ContainsPath;
    UNICODE_STRING AdjustedName;

    DPRINT("LdrFindEntryForName(Name %wZ)\n", Name);

    if (NtCurrentPeb()->Ldr == NULL)
        return(STATUS_NO_MORE_ENTRIES);

    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    if (Entry == ModuleListHead)
    {
        RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
        return(STATUS_NO_MORE_ENTRIES);
    }

    // NULL is the current process
    if (Name == NULL)
    {
        *Module = LdrpImageEntry;
        RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
        return(STATUS_SUCCESS);
    }

    ContainsPath = (Name->Length >= 2 * sizeof(WCHAR) && L':' == Name->Buffer[1]);
    LdrAdjustDllName (&AdjustedName, Name, !ContainsPath);

    if (LdrpLastModule)
    {
        if ((!ContainsPath &&
             0 == RtlCompareUnicodeString(&LdrpLastModule->BaseDllName, &AdjustedName, TRUE)) ||
            (ContainsPath &&
             0 == RtlCompareUnicodeString(&LdrpLastModule->FullDllName, &AdjustedName, TRUE)))
        {
            *Module = LdrpLastModule;
            if (Ref && (*Module)->LoadCount != LDRP_PROCESS_CREATION_TIME)
            {
                (*Module)->LoadCount++;
            }
            RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
            RtlFreeUnicodeString(&AdjustedName);
            return(STATUS_SUCCESS);
        }
    }
    while (Entry != ModuleListHead)
    {
        ModulePtr = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        DPRINT("Scanning %wZ %wZ\n", &ModulePtr->BaseDllName, &AdjustedName);

        if ((!ContainsPath &&
             0 == RtlCompareUnicodeString(&ModulePtr->BaseDllName, &AdjustedName, TRUE)) ||
            (ContainsPath &&
             0 == RtlCompareUnicodeString(&ModulePtr->FullDllName, &AdjustedName, TRUE)))
        {
            *Module = LdrpLastModule = ModulePtr;
            if (Ref && ModulePtr->LoadCount != LDRP_PROCESS_CREATION_TIME)
            {
                ModulePtr->LoadCount++;
            }
            RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
            RtlFreeUnicodeString(&AdjustedName);
            return(STATUS_SUCCESS);
        }

        Entry = Entry->Flink;
    }

    DPRINT("Failed to find dll %wZ\n", Name);
    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
    RtlFreeUnicodeString(&AdjustedName);
    return(STATUS_NO_MORE_ENTRIES);
}

/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrFixupForward
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static PVOID
LdrFixupForward(PCHAR ForwardName)
{
    CHAR NameBuffer[128];
    UNICODE_STRING DllName;
    NTSTATUS Status;
    PCHAR p;
    PLDR_DATA_TABLE_ENTRY Module;
    PVOID BaseAddress;

    DPRINT1("\n");

    strcpy(NameBuffer, ForwardName);
    p = strchr(NameBuffer, '.');
    if (p != NULL)
    {
        *p = 0;

        DPRINT("Dll: %s  Function: %s\n", NameBuffer, p+1);
        RtlCreateUnicodeStringFromAsciiz (&DllName,
                                          NameBuffer);

        Status = LdrFindEntryForName (&DllName, &Module, FALSE);
        /* FIXME:
         *   The caller (or the image) is responsible for loading of the dll, where the function is forwarded.
         */
        if (!NT_SUCCESS(Status))
        {
            Status = LdrLoadDll(NULL, NULL, &DllName, &BaseAddress);
            if (NT_SUCCESS(Status))
            {
                Status = LdrFindEntryForName (&DllName, &Module, FALSE);
            }
        }
        RtlFreeUnicodeString (&DllName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LdrFixupForward: failed to load %s\n", NameBuffer);
            return NULL;
        }

        DPRINT("BaseAddress: %p\n", Module->DllBase);

        return LdrGetExportByName(Module->DllBase, (PUCHAR)(p+1), -1);
    }

    return NULL;
}


/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrGetExportByName
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *  AddressOfNames and AddressOfNameOrdinals are paralell tables,
 *  both with NumberOfNames entries.
 *
 */
static PVOID
LdrGetExportByName(PVOID BaseAddress,
                   PUCHAR SymbolName,
                   WORD Hint)
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    PDWORD *ExFunctions;
    PDWORD *ExNames;
    USHORT *ExOrdinals;
    PVOID ExName;
    ULONG Ordinal;
    PVOID Function;
    LONG minn, maxn;
    ULONG ExportDirSize;

    DPRINT("LdrGetExportByName %p %s %hu\n", BaseAddress, SymbolName, Hint);

    ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData(BaseAddress,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_EXPORT,
                        &ExportDirSize);
    if (ExportDir == NULL)
    {
        DPRINT1("LdrGetExportByName(): no export directory, "
                "can't lookup %s/%hu!\n", SymbolName, Hint);
        return NULL;
    }


    //The symbol names may be missing entirely
    if (ExportDir->AddressOfNames == 0)
    {
        DPRINT("LdrGetExportByName(): symbol names missing entirely\n");
        return NULL;
    }

    /*
     * Get header pointers
     */
    ExNames = (PDWORD *)RVA(BaseAddress, ExportDir->AddressOfNames);
    ExOrdinals = (USHORT *)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
    ExFunctions = (PDWORD *)RVA(BaseAddress, ExportDir->AddressOfFunctions);

    /*
     * Check the hint first
     */
    if (Hint < ExportDir->NumberOfNames)
    {
        ExName = RVA(BaseAddress, ExNames[Hint]);
        if (strcmp(ExName, (PCHAR)SymbolName) == 0)
        {
            Ordinal = ExOrdinals[Hint];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if (((ULONG)Function >= (ULONG)ExportDir) &&
                    ((ULONG)Function < (ULONG)ExportDir + (ULONG)ExportDirSize))
            {
                DPRINT("Forward: %s\n", (PCHAR)Function);
                Function = LdrFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DPRINT1("LdrGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }
            if (Function != NULL)
                return Function;
        }
    }

    /*
     * Binary search
     */
    minn = 0;
    maxn = ExportDir->NumberOfNames - 1;
    while (minn <= maxn)
    {
        LONG mid;
        LONG res;

        mid = (minn + maxn) / 2;

        ExName = RVA(BaseAddress, ExNames[mid]);
        res = strcmp(ExName, (PCHAR)SymbolName);
        if (res == 0)
        {
            Ordinal = ExOrdinals[mid];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if (((ULONG)Function >= (ULONG)ExportDir) &&
                ((ULONG)Function < (ULONG)ExportDir + (ULONG)ExportDirSize))
            {
                DPRINT("Forward: %s\n", (PCHAR)Function);
                Function = LdrFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DPRINT1("LdrGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }
            if (Function != NULL)
                return Function;
        }
        else if (minn == maxn)
        {
            DPRINT("LdrGetExportByName(): binary search failed\n");
            break;
        }
        else if (res > 0)
        {
            maxn = mid - 1;
        }
        else
        {
            minn = mid + 1;
        }
    }

    DPRINT("LdrGetExportByName(): failed to find %s\n",SymbolName);
    return (PVOID)NULL;
}


/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrPerformRelocations
 *
 * DESCRIPTION
 *      Relocate a DLL's memory image.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
NTSTATUS
LdrPerformRelocations(PIMAGE_NT_HEADERS NTHeaders,
                      PVOID ImageBase)
{
    PIMAGE_DATA_DIRECTORY RelocationDDir;
    PIMAGE_BASE_RELOCATION RelocationDir, RelocationEnd;
    ULONG Count, ProtectSize, OldProtect, OldProtect2;
    PVOID Page, ProtectPage, ProtectPage2;
    PUSHORT TypeOffset;
    LONG_PTR Delta;
    NTSTATUS Status;

    DPRINT1("\n");

    if (NTHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
        return STATUS_SUCCESS;
    }

    RelocationDDir =
        &NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (RelocationDDir->VirtualAddress == 0 || RelocationDDir->Size == 0)
    {
        return STATUS_SUCCESS;
    }

    ProtectSize = PAGE_SIZE;
    Delta = (ULONG_PTR)ImageBase - NTHeaders->OptionalHeader.ImageBase;
    RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)ImageBase +
                    RelocationDDir->VirtualAddress);
    RelocationEnd = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)ImageBase +
                    RelocationDDir->VirtualAddress + RelocationDDir->Size);

    while (RelocationDir < RelocationEnd &&
            RelocationDir->SizeOfBlock > 0)
    {
        Count = (RelocationDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) /
                sizeof(USHORT);
        Page = (PVOID)((ULONG_PTR)ImageBase + (ULONG_PTR)RelocationDir->VirtualAddress);
        TypeOffset = (PUSHORT)(RelocationDir + 1);

        /* Unprotect the page(s) we're about to relocate. */
        ProtectPage = Page;
        Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                        &ProtectPage,
                                        &ProtectSize,
                                        PAGE_READWRITE,
                                        &OldProtect);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to unprotect relocation target.\n");
            return Status;
        }

        if (RelocationDir->VirtualAddress + PAGE_SIZE <
                NTHeaders->OptionalHeader.SizeOfImage)
        {
            ProtectPage2 = (PVOID)((ULONG_PTR)ProtectPage + PAGE_SIZE);
            Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                            &ProtectPage2,
                                            &ProtectSize,
                                            PAGE_READWRITE,
                                            &OldProtect2);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to unprotect relocation target (2).\n");
                NtProtectVirtualMemory(NtCurrentProcess(),
                                       &ProtectPage,
                                       &ProtectSize,
                                       OldProtect,
                                       &OldProtect);
                return Status;
            }
        }
        else
        {
            ProtectPage2 = NULL;
        }

        RelocationDir = LdrProcessRelocationBlock((ULONG_PTR)Page,
                                                  Count,
                                                  TypeOffset,
                                                  Delta);
        if (RelocationDir == NULL)
            return STATUS_UNSUCCESSFUL;

        /* Restore old page protection. */
        NtProtectVirtualMemory(NtCurrentProcess(),
                               &ProtectPage,
                               &ProtectSize,
                               OldProtect,
                               &OldProtect);

        if (ProtectPage2 != NULL)
        {
            NtProtectVirtualMemory(NtCurrentProcess(),
                                   &ProtectPage2,
                                   &ProtectSize,
                                   OldProtect2,
                                   &OldProtect2);
        }
    }

    return STATUS_SUCCESS;
}

void
RtlpRaiseImportNotFound(CHAR *FuncName, ULONG Ordinal, PUNICODE_STRING DllName)
{
    ULONG ErrorResponse;
    ULONG_PTR ErrorParameters[2];
    ANSI_STRING ProcNameAnsi;
    ANSI_STRING DllNameAnsi;
    CHAR Buffer[8];

    DPRINT1("\n");

    if (!FuncName)
    {
        _snprintf(Buffer, 8, "# %ld", Ordinal);
        FuncName = Buffer;
    }

    RtlInitAnsiString(&ProcNameAnsi, FuncName);
    RtlUnicodeStringToAnsiString(&DllNameAnsi, DllName, TRUE);
    ErrorParameters[0] = (ULONG_PTR)&ProcNameAnsi;
    ErrorParameters[1] = (ULONG_PTR)&DllNameAnsi;
    NtRaiseHardError(STATUS_ENTRYPOINT_NOT_FOUND,
                     2,
                     3,
                     ErrorParameters,
                     OptionOk,
                     &ErrorResponse);
    RtlFreeAnsiString(&DllNameAnsi);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    NTSTATUS Status;

    DPRINT("LdrDisableThreadCalloutsForDll (BaseAddress %p)\n", BaseAddress);

    Status = STATUS_DLL_NOT_FOUND;
    RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead)
    {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        DPRINT("BaseDllName %wZ BaseAddress %p\n", &Module->BaseDllName, Module->DllBase);

        if (Module->DllBase == BaseAddress)
        {
            if (Module->TlsIndex == 0xFFFF)
            {
                Module->Flags |= LDRP_DONT_CALL_FOR_THREADS;
                Status = STATUS_SUCCESS;
            }
            break;
        }
        Entry = Entry->Flink;
    }
    RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN NTAPI
RtlDllShutdownInProgress (VOID)
{
    DPRINT1("\n");
    return LdrpDllShutdownInProgress;
}

/*
 * Compute size of an image as it is actually present in virt memory
 * (i.e. excluding NEVER_LOAD sections)
 */
ULONG
LdrpGetResidentSize(PIMAGE_NT_HEADERS NTHeaders)
{
    PIMAGE_SECTION_HEADER SectionHeader;
    unsigned SectionIndex;
    ULONG ResidentSize;

    DPRINT1("\n");

    SectionHeader = (PIMAGE_SECTION_HEADER)((char *) &NTHeaders->OptionalHeader
                                            + NTHeaders->FileHeader.SizeOfOptionalHeader);
    ResidentSize = 0;
    for (SectionIndex = 0;
         SectionIndex < NTHeaders->FileHeader.NumberOfSections;
         SectionIndex++)
    {
        if (0 == (SectionHeader->Characteristics & IMAGE_SCN_LNK_REMOVE)
                && ResidentSize < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize)
        {
            ResidentSize = SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize;
        }
        SectionHeader++;
    }

    return ResidentSize;
}

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    IN ULONG_PTR Address,
    IN ULONG Count,
    IN PUSHORT TypeOffset,
    IN LONG_PTR Delta)
{
    DPRINT1("\n");
    return LdrProcessRelocationBlockLongLong(Address, Count, TypeOffset, Delta);
}

BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(IN PVOID BaseAddress)
{
    //FIXME: We don't implement alternate resources anyway, don't spam the log
    //UNIMPLEMENTED;
    return FALSE;
}

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
#include "kdb.h"

#define NDEBUG
#include "debug.h"

/* GLOBALS ******************************************************************/

typedef struct _IMAGE_SYMBOL_INFO_CACHE
{
    LIST_ENTRY ListEntry;
    ULONG RefCount;
    UNICODE_STRING FileName;
    PROSSYM_INFO RosSymInfo;
}
IMAGE_SYMBOL_INFO_CACHE, *PIMAGE_SYMBOL_INFO_CACHE;

#define KDB_SYM_TAG 'bSyK'

typedef struct _KDB_SYMBOL_WORK_ITEM
{
    LIST_ENTRY ListEntry;
    UNICODE_STRING BaseDllName;
    UNICODE_STRING FullDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
}
KDB_SYMBOL_WORK_ITEM, *PKDB_SYMBOL_WORK_ITEM;

static BOOLEAN LoadSymbols = FALSE;
static LIST_ENTRY SymbolsToLoad;
static KSPIN_LOCK SymbolsToLoadLock;
static KEVENT SymbolsToLoadEvent;

/* FUNCTIONS ****************************************************************/

static
BOOLEAN
KdbpSymDuplicateUnicodeString(
    _In_ PUNICODE_STRING Source,
    _Out_ PUNICODE_STRING Destination)
{
    Destination->Length = 0;
    Destination->MaximumLength = 0;
    Destination->Buffer = NULL;

    if (!Source || !Source->Length)
        return TRUE;

    Destination->MaximumLength = Source->Length + sizeof(WCHAR);
    Destination->Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                Destination->MaximumLength,
                                                KDB_SYM_TAG);
    if (!Destination->Buffer)
        return FALSE;

    RtlCopyMemory(Destination->Buffer, Source->Buffer, Source->Length);
    Destination->Buffer[Source->Length / sizeof(WCHAR)] = UNICODE_NULL;
    Destination->Length = Source->Length;
    return TRUE;
}

static
VOID
KdbpSymFreeUnicodeString(
    _Inout_ PUNICODE_STRING String)
{
    if (String->Buffer)
    {
        ExFreePoolWithTag(String->Buffer, KDB_SYM_TAG);
        RtlZeroMemory(String, sizeof(*String));
    }
}

static
VOID
KdbpSymFreeWorkItem(
    _Inout_ PKDB_SYMBOL_WORK_ITEM WorkItem)
{
    if (!WorkItem)
        return;

    KdbpSymFreeUnicodeString(&WorkItem->BaseDllName);
    KdbpSymFreeUnicodeString(&WorkItem->FullDllName);
    ExFreePoolWithTag(WorkItem, KDB_SYM_TAG);
}

static
BOOLEAN
KdbpSymStoreSymbolInfo(
    _In_ PUNICODE_STRING BaseDllName,
    _In_opt_ PUNICODE_STRING FullDllName,
    _In_ PVOID DllBase,
    _In_ ULONG SizeOfImage,
    _In_ PROSSYM_INFO RosSymInfo)
{
    PLIST_ENTRY CurrentEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    KIRQL OldIrql;
    BOOLEAN Stored = FALSE;

    if (!BaseDllName || !BaseDllName->Length)
        return FALSE;

    KeAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);

    CurrentEntry = PsLoadedModuleList.Flink;
    while (CurrentEntry != &PsLoadedModuleList)
    {
        LdrEntry = CONTAINING_RECORD(CurrentEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if ((!(FullDllName && FullDllName->Length) ||
             RtlEqualUnicodeString(&LdrEntry->FullDllName, FullDllName, TRUE)) &&
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, BaseDllName, TRUE) &&
            LdrEntry->DllBase == DllBase &&
            LdrEntry->SizeOfImage == SizeOfImage)
        {
            if (!LdrEntry->PatchInformation)
            {
                LdrEntry->PatchInformation = RosSymInfo;
                Stored = TRUE;
            }
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
    return Stored;
}

static
BOOLEAN
KdbpSymSearchModuleList(
    IN PLIST_ENTRY current_entry,
    IN PLIST_ENTRY end_entry,
    IN PLONG Count,
    IN PVOID Address,
    IN INT Index,
    OUT PLDR_DATA_TABLE_ENTRY* pLdrEntry)
{
    while (current_entry && current_entry != end_entry)
    {
        *pLdrEntry = CONTAINING_RECORD(current_entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if ((Address && Address >= (PVOID)(*pLdrEntry)->DllBase && Address < (PVOID)((ULONG_PTR)(*pLdrEntry)->DllBase + (*pLdrEntry)->SizeOfImage)) ||
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
    IN INT Index  OPTIONAL,
    OUT PLDR_DATA_TABLE_ENTRY* pLdrEntry)
{
    LONG Count = 0;
    PEPROCESS CurrentProcess;

    /* First try to look up the module in the kernel module list. */
    KeAcquireSpinLockAtDpcLevel(&PsLoadedModuleSpinLock);
    if(KdbpSymSearchModuleList(PsLoadedModuleList.Flink,
                               &PsLoadedModuleList,
                               &Count,
                               Address,
                               Index,
                               pLdrEntry))
    {
        KeReleaseSpinLockFromDpcLevel(&PsLoadedModuleSpinLock);
        return TRUE;
    }
    KeReleaseSpinLockFromDpcLevel(&PsLoadedModuleSpinLock);

    /* That didn't succeed. Try the module list of the current process now. */
    CurrentProcess = PsGetCurrentProcess();

    if(!CurrentProcess || !CurrentProcess->Peb || !CurrentProcess->Peb->Ldr)
        return FALSE;

    return KdbpSymSearchModuleList(CurrentProcess->Peb->Ldr->InLoadOrderModuleList.Flink,
                                   &CurrentProcess->Peb->Ldr->InLoadOrderModuleList,
                                   &Count,
                                   Address,
                                   Index,
                                   pLdrEntry);
}

/*! \brief Find symbol info by module name
 *
 * \param ModName   Pointer to a UNICODE_STRING containing the module name.
 *
 * \retval PROSSYM_INFO    Symbol info for the module, or NULL if not found.
 */
PROSSYM_INFO
KdbpSymFindCachedFile(
    IN PUNICODE_STRING ModName)
{
    PLIST_ENTRY CurrentEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    KeAcquireSpinLockAtDpcLevel(&PsLoadedModuleSpinLock);

    CurrentEntry = PsLoadedModuleList.Flink;
    while (CurrentEntry != &PsLoadedModuleList)
    {
        LdrEntry = CONTAINING_RECORD(CurrentEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, ModName, TRUE))
        {
            KeReleaseSpinLockFromDpcLevel(&PsLoadedModuleSpinLock);
            return (PROSSYM_INFO)LdrEntry->PatchInformation;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLockFromDpcLevel(&PsLoadedModuleSpinLock);
    return NULL;
}

static
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

static volatile LONG KdbSymLookupInProgress = 0;

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
    IN PCONTEXT Context)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG_PTR RelativeAddress;
    BOOLEAN Printed = FALSE;
    CHAR ModuleNameAnsi[64];

    if (!KdbpSymFindModule(Address, -1, &LdrEntry))
        return FALSE;

    RelativeAddress = (ULONG_PTR)Address - (ULONG_PTR)LdrEntry->DllBase;

    KdbpSymUnicodeToAnsi(&LdrEntry->BaseDllName,
                        ModuleNameAnsi,
                        sizeof(ModuleNameAnsi));

    if (LdrEntry->PatchInformation &&
        InterlockedCompareExchange(&KdbSymLookupInProgress, 1, 0) == 0)
    {
#ifdef __ROS_DWARF__
        ROSSYM_LINEINFO LineInfo = { 0 };

        _SEH2_TRY
        {
            if (RosSymGetAddressInformation(LdrEntry->PatchInformation,
                                            RelativeAddress,
                                            &LineInfo))
            {
                if (LineInfo.DirectoryName && LineInfo.DirectoryName[0])
                {
                    KdbPrintf("<%s:%x (%s/%s:%d (%s))>",
                              ModuleNameAnsi, RelativeAddress,
                              LineInfo.DirectoryName,
                              LineInfo.FileName ? LineInfo.FileName : "",
                              LineInfo.LineNumber,
                              LineInfo.FunctionName ? LineInfo.FunctionName : "");
                }
                else
                {
                    KdbPrintf("<%s:%x (%s:%d (%s))>",
                              ModuleNameAnsi, RelativeAddress,
                              LineInfo.FileName ? LineInfo.FileName : "",
                              LineInfo.LineNumber,
                              LineInfo.FunctionName ? LineInfo.FunctionName : "");
                }
                Printed = TRUE;
                RosSymFreeInfo(&LineInfo);
            }
        }
        _SEH2_FINALLY
        {
            InterlockedExchange(&KdbSymLookupInProgress, 0);
        }
        _SEH2_END;
#else
        ULONG LineNumber;
        CHAR FileName[256];
        CHAR FunctionName[256];

        _SEH2_TRY
        {
            if (RosSymGetAddressInformation(LdrEntry->PatchInformation,
                                            RelativeAddress,
                                            &LineNumber,
                                            FileName,
                                            FunctionName))
            {
                KdbPrintf("<%s:%x (%s:%d (%s))>",
                          ModuleNameAnsi, RelativeAddress,
                          FileName, LineNumber, FunctionName);
                Printed = TRUE;
            }
        }
        _SEH2_FINALLY
        {
            InterlockedExchange(&KdbSymLookupInProgress, 0);
        }
        _SEH2_END;
#endif
    }

    if (!Printed)
    {
        /* Just print module & address */
        KdbPrintf("<%s:%x>", ModuleNameAnsi, RelativeAddress);
    }

    return TRUE;
}

static KSTART_ROUTINE LoadSymbolsRoutine;
/*! \brief          The symbol loader thread routine.
 *                  This opens the image file for reading and loads the symbols
 *                  section from there.
 *
 * \note            We must do this because KdbSymProcessSymbols is
 *                  called at high IRQL and we can't set the event from here
 *
 * \param Context   Unused
 */
_Use_decl_annotations_
VOID
NTAPI
LoadSymbolsRoutine(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    while (TRUE)
    {
        PLIST_ENTRY ListEntry;
        NTSTATUS Status = KeWaitForSingleObject(&SymbolsToLoadEvent, WrKernel, KernelMode, FALSE, NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("KeWaitForSingleObject failed: 0x%08x\n", Status);
            LoadSymbols = FALSE;
            return;
        }

        while ((ListEntry = ExInterlockedRemoveHeadList(&SymbolsToLoad, &SymbolsToLoadLock)))
        {
            PKDB_SYMBOL_WORK_ITEM WorkItem = CONTAINING_RECORD(ListEntry, KDB_SYMBOL_WORK_ITEM, ListEntry);
            PROSSYM_INFO RosSymInfo = NULL;
            HANDLE FileHandle;
            OBJECT_ATTRIBUTES Attrib;
            IO_STATUS_BLOCK Iosb;

            Status = STATUS_UNSUCCESSFUL;
            if (WorkItem->FullDllName.Length)
            {
                InitializeObjectAttributes(&Attrib, &WorkItem->FullDllName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
                Status = ZwOpenFile(&FileHandle,
                                    FILE_READ_ACCESS | SYNCHRONIZE,
                                    &Attrib,
                                    &Iosb,
                                    FILE_SHARE_READ,
                                    FILE_SYNCHRONOUS_IO_NONALERT);
            }

            if (!NT_SUCCESS(Status) && WorkItem->BaseDllName.Length)
            {
                /* Try system paths */
                static const UNICODE_STRING System32Dir = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\");
                UNICODE_STRING ImagePath;
                WCHAR ImagePathBuffer[256];
                RtlInitEmptyUnicodeString(&ImagePath, ImagePathBuffer, sizeof(ImagePathBuffer));
                RtlCopyUnicodeString(&ImagePath, &System32Dir);
                RtlAppendUnicodeStringToString(&ImagePath, &WorkItem->BaseDllName);
                InitializeObjectAttributes(&Attrib, &ImagePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
                Status = ZwOpenFile(&FileHandle,
                                    FILE_READ_ACCESS | SYNCHRONIZE,
                                    &Attrib,
                                    &Iosb,
                                    FILE_SHARE_READ,
                                    FILE_SYNCHRONOUS_IO_NONALERT);
                if (!NT_SUCCESS(Status))
                {
                    static const UNICODE_STRING DriversDir= RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\drivers\\");

                    RtlInitEmptyUnicodeString(&ImagePath, ImagePathBuffer, sizeof(ImagePathBuffer));
                    RtlCopyUnicodeString(&ImagePath, &DriversDir);
                    RtlAppendUnicodeStringToString(&ImagePath, &WorkItem->BaseDllName);
                    InitializeObjectAttributes(&Attrib, &ImagePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
                    Status = ZwOpenFile(&FileHandle,
                                        FILE_READ_ACCESS | SYNCHRONIZE,
                                        &Attrib,
                                        &Iosb,
                                        FILE_SHARE_READ,
                                        FILE_SYNCHRONOUS_IO_NONALERT);
                }
            }

            if (!NT_SUCCESS(Status))
            {
                DPRINT("Failed opening file %wZ (%wZ) for symbols (0x%08x)\n",
                       &WorkItem->FullDllName,
                       &WorkItem->BaseDllName,
                       Status);
                KdbpSymFreeWorkItem(WorkItem);
                continue;
            }

            /* Hand it to Rossym */
            if (!RosSymCreateFromFile(&FileHandle, &RosSymInfo))
                RosSymInfo = NULL;

            /* We're done for this one. */
            NtClose(FileHandle);

            /* Attach only if the original module is still loaded and matches. */
            if (RosSymInfo &&
                KdbpSymStoreSymbolInfo(&WorkItem->BaseDllName,
                                       &WorkItem->FullDllName,
                                       WorkItem->DllBase,
                                       WorkItem->SizeOfImage,
                                       RosSymInfo))
            {
                RosSymInfo = NULL;
            }

            if (RosSymInfo)
                RosSymDelete(RosSymInfo);

            KdbpSymFreeWorkItem(WorkItem);
        }
    }
}

/*! \brief          Load symbols from image mapping. If this fails,
 *
 * \param LdrEntry  The entry to load symbols from
 */
VOID
KdbSymProcessSymbols(
    _Inout_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ BOOLEAN Load)
{
    if (!LoadSymbols)
        return;

    /* Check if this is unload */
    if (!Load)
    {
        /* Did we process it */
        if (LdrEntry->PatchInformation)
        {
            RosSymDelete(LdrEntry->PatchInformation);
            LdrEntry->PatchInformation = NULL;
        }
        return;
    }

    /*
     * For DWARF-based builds, RosSymCreateFromMem allocates memory and does
     * operations that require PASSIVE_LEVEL. Only try inline loading if we're
     * at PASSIVE_LEVEL, otherwise always defer to the worker thread.
     */
    if (KeGetCurrentIrql() == PASSIVE_LEVEL &&
        RosSymCreateFromMem(LdrEntry->DllBase, LdrEntry->SizeOfImage, (PROSSYM_INFO*)&LdrEntry->PatchInformation))
    {
        return;
    }

    /* Tell our worker thread to read from it */
    PKDB_SYMBOL_WORK_ITEM WorkItem = ExAllocatePoolWithTag(NonPagedPool,
                                                           sizeof(*WorkItem),
                                                           KDB_SYM_TAG);
    if (!WorkItem)
        return;

    RtlZeroMemory(WorkItem, sizeof(*WorkItem));
    /*
     * Copy the names so the worker doesn't dereference a potentially
     * unloaded LDR entry; we re-lookup under the module list lock before
     * attaching symbols, so unloadable drivers are not pinned.
     */
    if (!KdbpSymDuplicateUnicodeString(&LdrEntry->BaseDllName, &WorkItem->BaseDllName) ||
        !KdbpSymDuplicateUnicodeString(&LdrEntry->FullDllName, &WorkItem->FullDllName))
    {
        KdbpSymFreeWorkItem(WorkItem);
        return;
    }

    WorkItem->DllBase = LdrEntry->DllBase;
    WorkItem->SizeOfImage = LdrEntry->SizeOfImage;

    KeAcquireSpinLockAtDpcLevel(&SymbolsToLoadLock);
    InsertTailList(&SymbolsToLoad, &WorkItem->ListEntry);
    KeReleaseSpinLockFromDpcLevel(&SymbolsToLoadLock);

    KeSetEvent(&SymbolsToLoadEvent, IO_NO_INCREMENT, FALSE);
}


/**
 * @brief   Initializes the KDB symbols implementation.
 *
 * @param[in]   BootPhase
 * Phase of initialization.
 *
 * @return
 * TRUE if symbols are to be loaded at this given BootPhase; FALSE if not.
 **/
BOOLEAN
KdbSymInit(
    _In_ ULONG BootPhase)
{
#if 1 // FIXME: This is a workaround HACK!!
    static BOOLEAN OrigLoadSymbols = FALSE;
#endif

    DPRINT("KdbSymInit() BootPhase=%d\n", BootPhase);

#ifdef SEPARATE_DBG
    DPRINT("KDBG symbols disabled for SEPARATE_DBG builds\n");
    LoadSymbols = FALSE;
    return FALSE;
#endif

    if (BootPhase == 0)
    {
        PSTR CommandLine;
        SHORT Found = FALSE;
        CHAR YesNo;

        /* By default, load symbols in DBG builds on supported architectures */
#if DBG && (defined(_M_IX86) || defined(_M_AMD64))
        LoadSymbols = TRUE;
#else
        LoadSymbols = FALSE;
#endif

        /* Check the command line for LOADSYMBOLS, NOLOADSYMBOLS,
         * LOADSYMBOLS={YES|NO}, NOLOADSYMBOLS={YES|NO} */
        ASSERT(KeLoaderBlock);
        CommandLine = KeLoaderBlock->LoadOptions;
        while (*CommandLine)
        {
            /* Skip any whitespace */
            while (isspace(*CommandLine))
                ++CommandLine;

            Found = 0;
            if (_strnicmp(CommandLine, "LOADSYMBOLS", 11) == 0)
            {
                Found = +1;
                CommandLine += 11;
            }
            else if (_strnicmp(CommandLine, "NOLOADSYMBOLS", 13) == 0)
            {
                Found = -1;
                CommandLine += 13;
            }
            if (Found != 0)
            {
                if (*CommandLine == '=')
                {
                    ++CommandLine;
                    YesNo = toupper(*CommandLine);
                    if (YesNo == 'N' || YesNo == '0')
                    {
                        Found = -1 * Found;
                    }
                }
                LoadSymbols = (0 < Found);
            }

            /* Move on to the next option */
            while (*CommandLine && !isspace(*CommandLine))
                ++CommandLine;
        }

#if 1 // FIXME: This is a workaround HACK!!
// Save the actual value of LoadSymbols but disable it for BootPhase 0.
        OrigLoadSymbols = LoadSymbols;
        LoadSymbols = FALSE;
        return OrigLoadSymbols;
#endif
    }
    else if (BootPhase == 1)
    {
        HANDLE Thread;
        NTSTATUS Status;
        KIRQL OldIrql;
        PLIST_ENTRY ListEntry;

#if 1 // FIXME: This is a workaround HACK!!
// Now, restore the actual value of LoadSymbols.
        LoadSymbols = OrigLoadSymbols;
#endif

        /* Do not continue loading symbols if we have less than 96MB of RAM */
        if (MmNumberOfPhysicalPages < (96 * 1024 * 1024 / PAGE_SIZE))
            LoadSymbols = FALSE;

        /* Continue this phase only if we need to load symbols */
        if (!LoadSymbols)
            return LoadSymbols;

        /* Launch our worker thread */
        InitializeListHead(&SymbolsToLoad);
        KeInitializeSpinLock(&SymbolsToLoadLock);
        KeInitializeEvent(&SymbolsToLoadEvent, SynchronizationEvent, FALSE);

        Status = PsCreateSystemThread(&Thread,
                                      THREAD_ALL_ACCESS,
                                      NULL, NULL, NULL,
                                      LoadSymbolsRoutine,
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Failed starting symbols loader thread: 0x%08x\n", Status);
            LoadSymbols = FALSE;
            return LoadSymbols;
        }

        RosSymInitKernelMode();

        /*
         * For DWARF symbols, we need to process modules at PASSIVE_LEVEL.
         * Collect all boot modules first while holding the spinlock,
         * then process them after releasing it.
         */
        {
            PLDR_DATA_TABLE_ENTRY *ModuleArray;
            ULONG ModuleCount = 0, i;

            /* Count modules */
            KeAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);
            for (ListEntry = PsLoadedModuleList.Flink;
                 ListEntry != &PsLoadedModuleList;
                 ListEntry = ListEntry->Flink)
            {
                ModuleCount++;
            }
            KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);

            if (ModuleCount == 0)
                return LoadSymbols;

            /* Allocate array for module pointers */
            ModuleArray = ExAllocatePoolWithTag(NonPagedPool,
                                                ModuleCount * sizeof(PLDR_DATA_TABLE_ENTRY),
                                                'mySK');
            if (!ModuleArray)
            {
                DPRINT("Failed to allocate module array\n");
                return LoadSymbols;
            }

            /* Collect module pointers */
            KeAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);
            i = 0;
            for (ListEntry = PsLoadedModuleList.Flink;
                 ListEntry != &PsLoadedModuleList && i < ModuleCount;
                 ListEntry = ListEntry->Flink)
            {
                PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
                ModuleArray[i++] = LdrEntry;
            }
            ModuleCount = i;
            KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);

            /* Now process at PASSIVE_LEVEL */
            for (i = 0; i < ModuleCount; i++)
            {
                KdbSymProcessSymbols(ModuleArray[i], TRUE);
            }

            ExFreePoolWithTag(ModuleArray, 'mySK');
        }
    }

    return LoadSymbols;
}

/* EOF */

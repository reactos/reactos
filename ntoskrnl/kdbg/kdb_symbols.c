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

static BOOLEAN LoadSymbols = FALSE;
static BOOLEAN LoadSymbolsRequested;
static LIST_ENTRY SymbolsToLoad;
static KSPIN_LOCK SymbolsToLoadLock;
static KEVENT SymbolsToLoadEvent;

/* FUNCTIONS ****************************************************************/

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

/*! \brief Acquire the module list spin lock.
 *
 * \param OldIrql Receives the original IRQL.
 *
 * \retval TRUE The lock was acquired.
 * \retval FALSE The lock was busy and the original IRQL was restored.
 */
static
BOOLEAN
KdbpSymTryLockModuleList(
    _Out_ PKIRQL OldIrql)
{
    *OldIrql = KeGetCurrentIrql();

    if (*OldIrql < DISPATCH_LEVEL)
        KeRaiseIrqlToDpcLevel();

    /*
     * The debugger may have entered with the module list lock held by
     * the current processor. KeTryToAcquireSpinLockAtDpcLevel asserts
     * on recursive acquire, so test the spin lock first.
     */
    if (KeTestSpinLock(&PsLoadedModuleSpinLock) &&
        KeTryToAcquireSpinLockAtDpcLevel(&PsLoadedModuleSpinLock))
    {
        return TRUE;
    }

    if (*OldIrql < DISPATCH_LEVEL)
        KeLowerIrql(*OldIrql);

    return FALSE;
}

/*! \brief Release the loaded module list spin lock and restore IRQL.
 *
 * \param OldIrql The IRQL returned by KdbpSymTryLockModuleList.
 */
static
VOID
KdbpSymUnlockModuleList(
    _In_ KIRQL OldIrql)
{
    KeReleaseSpinLockFromDpcLevel(&PsLoadedModuleSpinLock);
    if (OldIrql < DISPATCH_LEVEL)
        KeLowerIrql(OldIrql);
}

/*! \brief Search the current process's user mode module list.
 *
 * \param Address Optional address to look up within a module.
 * \param Index Module index or a negative value to skip index matching.
 * \param Count Module count for index matching.
 * \param LdrEntry Receives the matching loader entry on success.
 *
 * \retval TRUE if a matching module was found.
 */
static
BOOLEAN
KdbpSymFindUserModule(
    _In_opt_ PVOID Address,
    _In_ INT Index,
    _Inout_ PLONG Count,
    _Out_ PLDR_DATA_TABLE_ENTRY* LdrEntry)
{
    PEPROCESS Process = PsGetCurrentProcess();

    if (!Process || !Process->Peb || !Process->Peb->Ldr)
        return FALSE;

    return KdbpSymSearchModuleList(Process->Peb->Ldr->InLoadOrderModuleList.Flink,
                                   &Process->Peb->Ldr->InLoadOrderModuleList,
                                   Count,
                                   Address,
                                   Index,
                                   LdrEntry);
}

/*! \brief Find a module for the debugger's module command.
 *
 * \param Address      If \a Address is not NULL the module containing \a Address
 *                     is searched.
 * \param Index        If \a Index is >= 0 the Index'th module will be returned.
 * \param pLdrEntry    Pointer to a PLDR_DATA_TABLE_ENTRY which is filled.
 *
 * \retval TRUE    Module was found, \a pLdrEntry was filled.
 * \retval FALSE   The list was busy or no module was found.
 *
 * \remark Other processors must be frozen while the caller accesses the returned entry.
 */
BOOLEAN
KdbpSymFindModule(
    IN PVOID Address  OPTIONAL,
    IN INT Index  OPTIONAL,
    OUT PLDR_DATA_TABLE_ENTRY* pLdrEntry)
{
    LONG Count = 0;
    KIRQL OldIrql;
    BOOLEAN Found;

    if (!KdbpSymTryLockModuleList(&OldIrql))
        return FALSE;

    Found = KdbpSymSearchModuleList(PsLoadedModuleList.Flink,
                                    &PsLoadedModuleList,
                                    &Count,
                                    Address,
                                    Index,
                                    pLdrEntry);

    KdbpSymUnlockModuleList(OldIrql);

    return Found || KdbpSymFindUserModule(Address, Index, &Count, pLdrEntry);
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
    LONG Count = 0;
    ULONG_PTR RelativeAddress;
    BOOLEAN HaveSymbols = FALSE;
    CHAR ModuleNameAnsi[64];
    ULONG LineNumber;
    CHAR FileName[256];
    CHAR FunctionName[256];

    UNREFERENCED_PARAMETER(Context);

    if (Address > MmHighestUserAddress)
    {
        KIRQL OldIrql;

        if (!KdbpSymTryLockModuleList(&OldIrql))
            return FALSE;

        if (!KdbpSymSearchModuleList(PsLoadedModuleList.Flink,
                                     &PsLoadedModuleList,
                                     &Count,
                                     Address,
                                     -1,
                                     &LdrEntry))
        {
            KdbpSymUnlockModuleList(OldIrql);
            return FALSE;
        }

        /* Resolve the address and copy the module name before releasing the lock */
        RelativeAddress = (ULONG_PTR)Address - (ULONG_PTR)LdrEntry->DllBase;
        KdbpSymUnicodeToAnsi(&LdrEntry->BaseDllName, ModuleNameAnsi, sizeof(ModuleNameAnsi));

        if (LdrEntry->PatchInformation)
        {
            HaveSymbols = RosSymGetAddressInformation(LdrEntry->PatchInformation,
                                                      RelativeAddress,
                                                      &LineNumber,
                                                      FileName,
                                                      FunctionName);
        }

        KdbpSymUnlockModuleList(OldIrql);
    }
    else
    {
        if (!KdbpSymFindUserModule(Address, -1, &Count, &LdrEntry))
            return FALSE;

        RelativeAddress = (ULONG_PTR)Address - (ULONG_PTR)LdrEntry->DllBase;
        KdbpSymUnicodeToAnsi(&LdrEntry->BaseDllName, ModuleNameAnsi, sizeof(ModuleNameAnsi));
    }

    /* Print the copied information without holding the module list lock */
    if (HaveSymbols)
    {
        KdbPrintf("<%s:%x (%s:%d (%s))>",
                  ModuleNameAnsi, RelativeAddress,
                  FileName, LineNumber, FunctionName);
    }
    else
    {
        /* Just print module & address */
        KdbPrintf("<%s:%x>", ModuleNameAnsi, RelativeAddress);
    }

    return TRUE;
}

/*! \brief Associate initialized symbol information with a loaded image.
 *
 * \param LdrEntry Loader entry that will own the symbol information.
 * \param SymbolInfo Initialized symbol information.
 */
static
VOID
KdbpSymPublish(
    _Inout_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ PROSSYM_INFO SymbolInfo)
{
    KIRQL OldIrql = KeAcquireSpinLockRaiseToSynch(&PsLoadedModuleSpinLock);
    ASSERT(LdrEntry->PatchInformation == NULL);
    LdrEntry->PatchInformation = SymbolInfo;
    KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
}

static KSTART_ROUTINE LoadSymbolsRoutine;
/*! \brief          The symbol loader thread routine.
 *                  This opens the image file for reading and loads the symbols
 *                  section from there.
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
        KeWaitForSingleObject(&SymbolsToLoadEvent, WrKernel, KernelMode, FALSE, NULL);

        while ((ListEntry = ExInterlockedRemoveHeadList(&SymbolsToLoad, &SymbolsToLoadLock)))
        {
            PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
            HANDLE FileHandle;
            OBJECT_ATTRIBUTES Attrib;
            IO_STATUS_BLOCK Iosb;
            PROSSYM_INFO SymbolInfo;
            NTSTATUS Status;

            InitializeObjectAttributes(&Attrib, &LdrEntry->FullDllName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
            DPRINT1("Trying %wZ\n", &LdrEntry->FullDllName);
            Status = ZwOpenFile(&FileHandle,
                                FILE_READ_ACCESS | SYNCHRONIZE,
                                &Attrib,
                                &Iosb,
                                FILE_SHARE_READ,
                                FILE_SYNCHRONOUS_IO_NONALERT);
            if (!NT_SUCCESS(Status))
            {
                /* Try system paths */
                static const UNICODE_STRING System32Dir = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\");
                UNICODE_STRING ImagePath;
                WCHAR ImagePathBuffer[256];
                RtlInitEmptyUnicodeString(&ImagePath, ImagePathBuffer, sizeof(ImagePathBuffer));
                RtlCopyUnicodeString(&ImagePath, &System32Dir);
                RtlAppendUnicodeStringToString(&ImagePath, &LdrEntry->BaseDllName);
                InitializeObjectAttributes(&Attrib, &ImagePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
                DPRINT1("Trying %wZ\n", &ImagePath);
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
                    RtlAppendUnicodeStringToString(&ImagePath, &LdrEntry->BaseDllName);
                    InitializeObjectAttributes(&Attrib, &ImagePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
                    DPRINT1("Trying %wZ\n", &ImagePath);
                    Status = ZwOpenFile(&FileHandle,
                                        FILE_READ_ACCESS | SYNCHRONIZE,
                                        &Attrib,
                                        &Iosb,
                                        FILE_SHARE_READ,
                                        FILE_SYNCHRONOUS_IO_NONALERT);
                }
            }

            if (NT_SUCCESS(Status))
            {
                if (RosSymCreateFromFile(&FileHandle, &SymbolInfo))
                    KdbpSymPublish(LdrEntry, SymbolInfo);

                NtClose(FileHandle);
            }
            else
            {
                DPRINT1("Failed opening file %wZ (%wZ) for reading symbols (0x%08x)\n", &LdrEntry->FullDllName, &LdrEntry->BaseDllName, Status);
            }

            /* Release the reference taken when the request was queued */
            MmUnloadSystemImage(LdrEntry);
        }
    }
}

/*! \brief Load symbols from a mapped image or queue a file read.
 *
 * \param LdrEntry  Loader entry for the mapped image.
 *
 * \remark The caller must hold MmSystemLoadLock at APC_LEVEL or below.
 */
VOID
KdbSymLoadModule(
    _Inout_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PROSSYM_INFO SymbolInfo;

    if (!LoadSymbols)
        return;

    PAGED_CODE();

    if (RosSymCreateFromMem(LdrEntry->DllBase, LdrEntry->SizeOfImage, &SymbolInfo))
    {
        KdbpSymPublish(LdrEntry, SymbolInfo);
        return;
    }

    /* The worker must be able to access the loader entry after MmSystemLoadLock release */
    MmReferenceSystemImage(LdrEntry);
    ExInterlockedInsertTailList(&SymbolsToLoad, &LdrEntry->InInitializationOrderLinks, &SymbolsToLoadLock);
    KeSetEvent(&SymbolsToLoadEvent, IO_NO_INCREMENT, FALSE);
}

/*! \brief Detach and free local symbols on final image unload.
 *
 * \param LdrEntry Loader entry for the image being unloaded.
 *
 * \remarks Call before unmapping the image with MmSystemLoadLock held.
 *          Call at <= APC_LEVEL.
 */
VOID
KdbSymUnloadModule(
    _Inout_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PROSSYM_INFO SymbolInfo;
    KIRQL OldIrql;

    PAGED_CODE();

    /* Ensure no readers are accessing the symbol data before clearing the pointer */
    OldIrql = KeAcquireSpinLockRaiseToSynch(&PsLoadedModuleSpinLock);
    SymbolInfo = LdrEntry->PatchInformation;
    LdrEntry->PatchInformation = NULL;
    KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);

    if (SymbolInfo)
        RosSymDelete(SymbolInfo);
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
    DPRINT("KdbSymInit() BootPhase=%d\n", BootPhase);

    if (BootPhase == 0)
    {
        PSTR CommandLine;
        SHORT Found = FALSE;
        CHAR YesNo;

        /* By default, load symbols in DBG builds, but not in REL builds
           or anything other than x86, because they only work on x86
           and can cause the system to hang on x64. */
#if DBG && defined(_M_IX86)
        LoadSymbolsRequested = TRUE;
#else
        LoadSymbolsRequested = FALSE;
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
                LoadSymbolsRequested = (0 < Found);
            }

            /* Move on to the next option */
            while (*CommandLine && !isspace(*CommandLine))
                ++CommandLine;
        }

        return LoadSymbolsRequested;
    }
    else if (BootPhase == 1)
    {
        HANDLE Thread;
        NTSTATUS Status;
        PLIST_ENTRY ListEntry;

        if (!LoadSymbolsRequested || MmNumberOfPhysicalPages < (96 * 1024 * 1024 / PAGE_SIZE))
            return FALSE;

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
            DPRINT1("Failed starting symbols loader thread: 0x%08x\n", Status);
            return FALSE;
        }
        NtClose(Thread);

        RosSymInitKernelMode();

        /*
         * Serialize with image loading and unloading here without raising IRQL (because
         * symbol extraction may fault or allocate).
         */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(&MmSystemLoadLock, Executive, KernelMode, FALSE, NULL);
        LoadSymbols = TRUE;

        for (ListEntry = PsLoadedModuleList.Flink;
             ListEntry != &PsLoadedModuleList;
             ListEntry = ListEntry->Flink)
        {
            PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            KdbSymLoadModule(LdrEntry);
        }

        KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);
        KeLeaveCriticalRegion();
    }

    return LoadSymbols;
}

/* EOF */

/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/sysldr.c
 * PURPOSE:         Contains the Kernel Loader (SYSLDR) for loading PE files.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY PsLoadedModuleList;
LIST_ENTRY MmLoadedUserImageList;
KSPIN_LOCK PsLoadedModuleSpinLock;
ERESOURCE PsLoadedModuleResource;
ULONG_PTR PsNtosImageBase;
KMUTANT MmSystemLoadLock;

PFN_NUMBER MmTotalSystemDriverPages;

PVOID MmUnloadedDrivers;
PVOID MmLastUnloadedDrivers;

BOOLEAN MmMakeLowMemory;
BOOLEAN MmEnforceWriteProtection = TRUE;

PMMPTE MiKernelResourceStartPte, MiKernelResourceEndPte;
ULONG_PTR ExPoolCodeStart, ExPoolCodeEnd, MmPoolCodeStart, MmPoolCodeEnd;
ULONG_PTR MmPteCodeStart, MmPteCodeEnd;

#ifdef _WIN64
#define COOKIE_MAX 0x0000FFFFFFFFFFFFll
#define DEFAULT_SECURITY_COOKIE 0x00002B992DDFA232ll
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif

/* FUNCTIONS ******************************************************************/

PVOID
NTAPI
MiCacheImageSymbols(IN PVOID BaseAddress)
{
    ULONG DebugSize;
    PVOID DebugDirectory = NULL;
    PAGED_CODE();

    /* Make sure it's safe to access the image */
    _SEH2_TRY
    {
        /* Get the debug directory */
        DebugDirectory = RtlImageDirectoryEntryToData(BaseAddress,
                                                      TRUE,
                                                      IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                      &DebugSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Nothing */
    }
    _SEH2_END;

    /* Return the directory */
    return DebugDirectory;
}

NTSTATUS
NTAPI
MiLoadImageSection(_Inout_ PSECTION *SectionPtr,
                   _Out_ PVOID *ImageBase,
                   _In_ PUNICODE_STRING FileName,
                   _In_ BOOLEAN SessionLoad,
                   _In_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PSECTION Section = *SectionPtr;
    NTSTATUS Status;
    PEPROCESS Process;
    PVOID Base = NULL;
    SIZE_T ViewSize = 0;
    KAPC_STATE ApcState;
    LARGE_INTEGER SectionOffset = {{0, 0}};
    BOOLEAN LoadSymbols = FALSE;
    PFN_COUNT PteCount;
    PMMPTE PointerPte, LastPte;
    PVOID DriverBase;
    MMPTE TempPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PAGED_CODE();

    /* Detect session load */
    if (SessionLoad)
    {
        /* Fail */
        UNIMPLEMENTED_DBGBREAK("Session loading not yet supported!\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Not session load, shouldn't have an entry */
    ASSERT(LdrEntry == NULL);

    /* Attach to the system process */
    KeStackAttachProcess(&PsInitialSystemProcess->Pcb, &ApcState);

    /* Check if we need to load symbols */
    if (NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD)
    {
        /* Yes we do */
        LoadSymbols = TRUE;
        NtGlobalFlag &= ~FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }

    /* Map the driver */
    Process = PsGetCurrentProcess();
    Status = MmMapViewOfSection(Section,
                                Process,
                                &Base,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_EXECUTE);

    /* Re-enable the flag */
    if (LoadSymbols) NtGlobalFlag |= FLG_ENABLE_KDEBUG_SYMBOL_LOAD;

    /* Check if we failed with distinguished status code */
    if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
    {
        /* Change it to something more generic */
        Status = STATUS_INVALID_IMAGE_FORMAT;
    }

    /* Now check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Detach and return */
        DPRINT1("MmMapViewOfSection failed with status 0x%x\n", Status);
        KeUnstackDetachProcess(&ApcState);
        return Status;
    }

    /* Reserve system PTEs needed */
    PteCount = ROUND_TO_PAGES(((PMM_IMAGE_SECTION_OBJECT)Section->Segment)->ImageInformation.ImageFileSize) >> PAGE_SHIFT;
    PointerPte = MiReserveSystemPtes(PteCount, SystemPteSpace);
    if (!PointerPte)
    {
        DPRINT1("MiReserveSystemPtes failed\n");
        KeUnstackDetachProcess(&ApcState);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* New driver base */
    LastPte = PointerPte + PteCount;
    DriverBase = MiPteToAddress(PointerPte);

    /* The driver is here */
    *ImageBase = DriverBase;
    DPRINT1("Loading: %wZ at %p with %lx pages\n", FileName, DriverBase, PteCount);

    /* Lock the PFN database */
    OldIrql = MiAcquirePfnLock();

    /* Loop the new driver PTEs */
    TempPte = ValidKernelPte;
    while (PointerPte < LastPte)
    {
        /* Make sure the PTE is not valid for whatever reason */
        ASSERT(PointerPte->u.Hard.Valid == 0);

        /* Some debug stuff */
        MI_SET_USAGE(MI_USAGE_DRIVER_PAGE);
#if MI_TRACE_PFNS
        MI_SET_PROCESS_USTR(FileName);
#endif

        /* Grab a page */
        PageFrameIndex = MiRemoveAnyPage(MI_GET_NEXT_COLOR());

        /* Initialize its PFN entry */
        MiInitializePfn(PageFrameIndex, PointerPte, TRUE);

        /* Write the PTE */
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE(PointerPte, TempPte);

        /* Move on */
        PointerPte++;
    }

    /* Release the PFN lock */
    MiReleasePfnLock(OldIrql);

    /* Copy the image */
    RtlCopyMemory(DriverBase, Base, PteCount << PAGE_SHIFT);

    /* Now unmap the view */
    Status = MmUnmapViewOfSection(Process, Base);
    ASSERT(NT_SUCCESS(Status));

    /* Detach and return status */
    KeUnstackDetachProcess(&ApcState);
    return Status;
}

#ifndef RVA
#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))
#endif

USHORT
NTAPI
NameToOrdinal(
    _In_ PCSTR ExportName,
    _In_ PVOID ImageBase,
    _In_ ULONG NumberOfNames,
    _In_ PULONG NameTable,
    _In_ PUSHORT OrdinalTable)
{
    LONG Low, Mid, High, Ret;

    /* Fail if no names */
    if (!NumberOfNames)
        return -1;

    /* Do a binary search */
    Low = Mid = 0;
    High = NumberOfNames - 1;
    while (High >= Low)
    {
        /* Get new middle value */
        Mid = (Low + High) >> 1;

        /* Compare name */
        Ret = strcmp(ExportName, (PCHAR)RVA(ImageBase, NameTable[Mid]));
        if (Ret < 0)
        {
            /* Update high */
            High = Mid - 1;
        }
        else if (Ret > 0)
        {
            /* Update low */
            Low = Mid + 1;
        }
        else
        {
            /* We got it */
            break;
        }
    }

    /* Check if we couldn't find it */
    if (High < Low)
        return -1;

    /* Otherwise, this is the ordinal */
    return OrdinalTable[Mid];
}

/**
 * @brief
 * ReactOS-only helper routine for RtlFindExportedRoutineByName(),
 * that provides a finer granularity regarding the nature of the
 * export, and the failure reasons.
 *
 * @param[in]   ImageBase
 * The base address of the loaded image.
 *
 * @param[in]   ExportName
 * The name of the export, given as an ANSI NULL-terminated string.
 *
 * @param[out]  Function
 * The address of the named exported routine, or NULL if not found.
 * If the export is a forwarder (see @p IsForwarder below), this
 * address points to the forwarder name.
 *
 * @param[out]  IsForwarder
 * An optional pointer to a BOOLEAN variable, that is set to TRUE
 * if the found export is a forwarder, and FALSE otherwise.
 *
 * @param[in]   NotFoundStatus
 * The status code to return in case the export could not be found
 * (examples: STATUS_ENTRYPOINT_NOT_FOUND, STATUS_PROCEDURE_NOT_FOUND).
 *
 * @return
 * A status code as follows:
 * - STATUS_SUCCESS if the named exported routine is found;
 * - The custom @p NotFoundStatus if the export could not be found;
 * - STATUS_INVALID_PARAMETER if the image is invalid or does not
 *   contain an Export Directory.
 *
 * @note
 * See RtlFindExportedRoutineByName() for more remarks.
 * Used by psmgr.c PspLookupSystemDllEntryPoint() as well.
 **/
NTSTATUS
NTAPI
RtlpFindExportedRoutineByName(
    _In_ PVOID ImageBase,
    _In_ PCSTR ExportName,
    _Out_ PVOID* Function,
    _Out_opt_ PBOOLEAN IsForwarder,
    _In_ NTSTATUS NotFoundStatus)
{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PULONG NameTable;
    PUSHORT OrdinalTable;
    ULONG ExportSize;
    USHORT Ordinal;
    PULONG ExportTable;
    ULONG_PTR FunctionAddress;

    PAGED_CODE();

    /* Get the export directory */
    ExportDirectory = RtlImageDirectoryEntryToData(ImageBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                   &ExportSize);
    if (!ExportDirectory)
        return STATUS_INVALID_PARAMETER;

    /* Setup name tables */
    NameTable = (PULONG)RVA(ImageBase, ExportDirectory->AddressOfNames);
    OrdinalTable = (PUSHORT)RVA(ImageBase, ExportDirectory->AddressOfNameOrdinals);

    /* Get the ordinal */
    Ordinal = NameToOrdinal(ExportName,
                            ImageBase,
                            ExportDirectory->NumberOfNames,
                            NameTable,
                            OrdinalTable);

    /* Check if we couldn't find it */
    if (Ordinal == -1)
        return NotFoundStatus;

    /* Validate the ordinal */
    if (Ordinal >= ExportDirectory->NumberOfFunctions)
        return NotFoundStatus;

    /* Resolve the function's address */
    ExportTable = (PULONG)RVA(ImageBase, ExportDirectory->AddressOfFunctions);
    FunctionAddress = (ULONG_PTR)RVA(ImageBase, ExportTable[Ordinal]);

    /* Check if the function is actually a forwarder */
    if (IsForwarder)
    {
        *IsForwarder = FALSE;
        if ((FunctionAddress > (ULONG_PTR)ExportDirectory) &&
            (FunctionAddress < (ULONG_PTR)ExportDirectory + ExportSize))
        {
            /* It is, and points to the forwarder name */
            *IsForwarder = TRUE;
        }
    }

    /* We've found it */
    *Function = (PVOID)FunctionAddress;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Finds the address of a given named exported routine in a loaded image.
 * Note that this function does not support forwarders.
 *
 * @param[in]   ImageBase
 * The base address of the loaded image.
 *
 * @param[in]   ExportName
 * The name of the export, given as an ANSI NULL-terminated string.
 *
 * @return
 * The address of the named exported routine, or NULL if not found.
 * If the export is a forwarder, this function returns NULL as well.
 *
 * @note
 * This routine was originally named MiLocateExportName(), with a separate
 * duplicated MiFindExportedRoutineByName() one (taking a PANSI_STRING)
 * on Windows <= 2003. Both routines have been then merged and renamed
 * to MiFindExportedRoutineByName() on Windows 8 (taking a PCSTR instead),
 * and finally renamed and exported as RtlFindExportedRoutineByName() on
 * Windows 10.
 *
 * @see https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/mm/sysload/mmgetsystemroutineaddress.htm
 **/
PVOID
NTAPI
RtlFindExportedRoutineByName(
    _In_ PVOID ImageBase,
    _In_ PCSTR ExportName)
{
    NTSTATUS Status;
    BOOLEAN IsForwarder = FALSE;
    PVOID Function;

    PAGED_CODE();

    /* Call the internal API */
    Status = RtlpFindExportedRoutineByName(ImageBase,
                                           ExportName,
                                           &Function,
                                           &IsForwarder,
                                           STATUS_ENTRYPOINT_NOT_FOUND);
    if (!NT_SUCCESS(Status))
        return NULL;

    /* If the export is actually a forwarder, log the error and fail */
    if (IsForwarder)
    {
        DPRINT1("RtlFindExportedRoutineByName does not support forwarders!\n", FALSE);
        return NULL;
    }

    /* We've found the export */
    return Function;
}

NTSTATUS
NTAPI
MmCallDllInitialize(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ PLIST_ENTRY ModuleListHead)
{
    UNICODE_STRING ServicesKeyName = RTL_CONSTANT_STRING(
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    PMM_DLL_INITIALIZE DllInit;
    UNICODE_STRING RegPath, ImportName;
    PCWCH Extension;
    NTSTATUS Status;

    PAGED_CODE();

    /* Try to see if the image exports a DllInitialize routine */
    DllInit = (PMM_DLL_INITIALIZE)
        RtlFindExportedRoutineByName(LdrEntry->DllBase, "DllInitialize");
    if (!DllInit)
        return STATUS_SUCCESS;

    /* Make a temporary copy of BaseDllName because we will alter its length */
    ImportName.Length = LdrEntry->BaseDllName.Length;
    ImportName.MaximumLength = LdrEntry->BaseDllName.MaximumLength;
    ImportName.Buffer = LdrEntry->BaseDllName.Buffer;

    /* Obtain the path to this dll's service in the registry */
    RegPath.MaximumLength = ServicesKeyName.Length +
        ImportName.Length + sizeof(UNICODE_NULL);
    RegPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                           RegPath.MaximumLength,
                                           TAG_LDR_WSTR);

    /* Check if this allocation was unsuccessful */
    if (!RegPath.Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Build and append the service name itself */
    RegPath.Length = ServicesKeyName.Length;
    RtlCopyMemory(RegPath.Buffer,
                  ServicesKeyName.Buffer,
                  ServicesKeyName.Length);

    /* If the filename has an extension, remove it */
    Extension = wcschr(ImportName.Buffer, L'.');
    if (Extension)
        ImportName.Length = (USHORT)(Extension - ImportName.Buffer) * sizeof(WCHAR);

    /* Append the service name (base name without extension) */
    RtlAppendUnicodeStringToString(&RegPath, &ImportName);

    /* Now call DllInitialize */
    DPRINT("Calling DllInit(%wZ)\n", &RegPath);
    Status = DllInit(&RegPath);

    /* Clean up */
    ExFreePoolWithTag(RegPath.Buffer, TAG_LDR_WSTR);

    // TODO: This is for Driver Verifier support.
    UNREFERENCED_PARAMETER(ModuleListHead);

    /* Return the DllInitialize status value */
    return Status;
}

BOOLEAN
MiCallDllUnloadAndUnloadDll(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    NTSTATUS Status;
    PMM_DLL_UNLOAD DllUnload;

    PAGED_CODE();

    /* Retrieve the DllUnload routine */
    DllUnload = (PMM_DLL_UNLOAD)
        RtlFindExportedRoutineByName(LdrEntry->DllBase, "DllUnload");
    if (!DllUnload)
        return FALSE;

    /* Call it and check for success */
    Status = DllUnload();
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* Lie about the load count so we can unload the image */
    ASSERT(LdrEntry->LoadCount == 0);
    LdrEntry->LoadCount = 1;

    /* Unload it */
    MmUnloadSystemImage(LdrEntry);
    return TRUE;
}

NTSTATUS
NTAPI
MiDereferenceImports(IN PLOAD_IMPORTS ImportList)
{
    SIZE_T i;
    LOAD_IMPORTS SingleEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PVOID CurrentImports;
    PAGED_CODE();

    /* Check if there's no imports or if we're a boot driver */
    if ((ImportList == MM_SYSLDR_NO_IMPORTS) ||
        (ImportList == MM_SYSLDR_BOOT_LOADED) ||
        (ImportList->Count == 0))
    {
        /* Then there's nothing to do */
        return STATUS_SUCCESS;
    }

    /* Check for single-entry */
    if ((ULONG_PTR)ImportList & MM_SYSLDR_SINGLE_ENTRY)
    {
        /* Set it up */
        SingleEntry.Count = 1;
        SingleEntry.Entry[0] = (PVOID)((ULONG_PTR)ImportList &~ MM_SYSLDR_SINGLE_ENTRY);

        /* Use this as the import list */
        ImportList = &SingleEntry;
    }

    /* Loop the import list */
    for (i = 0; (i < ImportList->Count) && (ImportList->Entry[i]); i++)
    {
        /* Get the entry */
        LdrEntry = ImportList->Entry[i];
        DPRINT1("%wZ <%wZ>\n", &LdrEntry->FullDllName, &LdrEntry->BaseDllName);

        /* Skip boot loaded images */
        if (LdrEntry->LoadedImports == MM_SYSLDR_BOOT_LOADED) continue;

        /* Dereference the entry */
        ASSERT(LdrEntry->LoadCount >= 1);
        if (!--LdrEntry->LoadCount)
        {
            /* Save the import data in case unload fails */
            CurrentImports = LdrEntry->LoadedImports;

            /* This is the last entry */
            LdrEntry->LoadedImports = MM_SYSLDR_NO_IMPORTS;
            if (MiCallDllUnloadAndUnloadDll(LdrEntry))
            {
                /* Unloading worked, parse this DLL's imports too */
                MiDereferenceImports(CurrentImports);

                /* Check if we had valid imports */
                if ((CurrentImports != MM_SYSLDR_BOOT_LOADED) &&
                    (CurrentImports != MM_SYSLDR_NO_IMPORTS) &&
                    !((ULONG_PTR)CurrentImports & MM_SYSLDR_SINGLE_ENTRY))
                {
                    /* Free them */
                    ExFreePoolWithTag(CurrentImports, TAG_LDR_IMPORTS);
                }
            }
            else
            {
                /* Unload failed, restore imports */
                LdrEntry->LoadedImports = CurrentImports;
            }
        }
    }

    /* Done */
    return STATUS_SUCCESS;
}

VOID
NTAPI
MiClearImports(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PAGED_CODE();

    /* Check if there's no imports or we're a boot driver or only one entry */
    if ((LdrEntry->LoadedImports == MM_SYSLDR_BOOT_LOADED) ||
        (LdrEntry->LoadedImports == MM_SYSLDR_NO_IMPORTS) ||
        ((ULONG_PTR)LdrEntry->LoadedImports & MM_SYSLDR_SINGLE_ENTRY))
    {
        /* Nothing to do */
        return;
    }

    /* Otherwise, free the import list */
    ExFreePoolWithTag(LdrEntry->LoadedImports, TAG_LDR_IMPORTS);
    LdrEntry->LoadedImports = MM_SYSLDR_BOOT_LOADED;
}

VOID
NTAPI
MiProcessLoaderEntry(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                     IN BOOLEAN Insert)
{
    KIRQL OldIrql;

    /* Acquire module list lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PsLoadedModuleResource, TRUE);

    /* Acquire the spinlock too as we will insert or remove the entry */
    OldIrql = KeAcquireSpinLockRaiseToSynch(&PsLoadedModuleSpinLock);

    /* Insert or remove from the list */
    if (Insert)
        InsertTailList(&PsLoadedModuleList, &LdrEntry->InLoadOrderLinks);
    else
        RemoveEntryList(&LdrEntry->InLoadOrderLinks);

    /* Release locks */
    KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
    ExReleaseResourceLite(&PsLoadedModuleResource);
    KeLeaveCriticalRegion();
}

CODE_SEG("INIT")
VOID
NTAPI
MiUpdateThunks(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
               IN PVOID OldBase,
               IN PVOID NewBase,
               IN ULONG Size)
{
    ULONG_PTR OldBaseTop, Delta;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY NextEntry;
    ULONG ImportSize;
    //
    // FIXME: MINGW-W64 must fix LD to generate drivers that Windows can load,
    // since a real version of Windows would fail at this point, but they seem
    // busy implementing features such as "HotPatch" support in GCC 4.6 instead,
    // a feature which isn't even used by Windows. Priorities, priorities...
    // Please note that Microsoft WDK EULA and license prohibits using
    // the information contained within it for the generation of "non-Windows"
    // drivers, which is precisely what LD will generate, since an LD driver
    // will not load on Windows.
    //
#ifdef _WORKING_LINKER_
    ULONG i;
#endif
    PULONG_PTR ImageThunk;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;

    /* Calculate the top and delta */
    OldBaseTop = (ULONG_PTR)OldBase + Size - 1;
    Delta = (ULONG_PTR)NewBase - (ULONG_PTR)OldBase;

    /* Loop the loader block */
    for (NextEntry = LoaderBlock->LoadOrderListHead.Flink;
         NextEntry != &LoaderBlock->LoadOrderListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the loader entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);
#ifdef _WORKING_LINKER_
        /* Get the IAT */
        ImageThunk = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                  TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_IAT,
                                                  &ImportSize);
        if (!ImageThunk) continue;

        /* Make sure we have an IAT */
        DPRINT("[Mm0]: Updating thunks in: %wZ\n", &LdrEntry->BaseDllName);
        for (i = 0; i < ImportSize; i++, ImageThunk++)
        {
            /* Check if it's within this module */
            if ((*ImageThunk >= (ULONG_PTR)OldBase) && (*ImageThunk <= OldBaseTop))
            {
                /* Relocate it */
                DPRINT("[Mm0]: Updating IAT at: %p. Old Entry: %p. New Entry: %p.\n",
                        ImageThunk, *ImageThunk, *ImageThunk + Delta);
                *ImageThunk += Delta;
            }
        }
#else
        /* Get the import table */
        ImportDescriptor = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                        TRUE,
                                                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                        &ImportSize);
        if (!ImportDescriptor) continue;

        /* Make sure we have an IAT */
        DPRINT("[Mm0]: Updating thunks in: %wZ\n", &LdrEntry->BaseDllName);
        while ((ImportDescriptor->Name) &&
               (ImportDescriptor->OriginalFirstThunk))
        {
            /* Get the image thunk */
            ImageThunk = (PVOID)((ULONG_PTR)LdrEntry->DllBase +
                                 ImportDescriptor->FirstThunk);
            while (*ImageThunk)
            {
                /* Check if it's within this module */
                if ((*ImageThunk >= (ULONG_PTR)OldBase) && (*ImageThunk <= OldBaseTop))
                {
                    /* Relocate it */
                    DPRINT("[Mm0]: Updating IAT at: %p. Old Entry: %p. New Entry: %p.\n",
                            ImageThunk, *ImageThunk, *ImageThunk + Delta);
                    *ImageThunk += Delta;
                }

                /* Go to the next thunk */
                ImageThunk++;
            }

            /* Go to the next import */
            ImportDescriptor++;
        }
#endif
    }
}

NTSTATUS
NTAPI
MiSnapThunk(IN PVOID DllBase,
            IN PVOID ImageBase,
            IN PIMAGE_THUNK_DATA Name,
            IN PIMAGE_THUNK_DATA Address,
            IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
            IN ULONG ExportSize,
            IN BOOLEAN SnapForwarder,
            OUT PCHAR *MissingApi)
{
    BOOLEAN IsOrdinal;
    USHORT Ordinal;
    PULONG NameTable;
    PUSHORT OrdinalTable;
    PIMAGE_IMPORT_BY_NAME NameImport;
    USHORT Hint;
    NTSTATUS Status;
    PCHAR MissingForwarder;
    CHAR NameBuffer[MAXIMUM_FILENAME_LENGTH];
    PULONG ExportTable;
    ANSI_STRING DllName;
    UNICODE_STRING ForwarderName;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG ForwardExportSize;
    PIMAGE_EXPORT_DIRECTORY ForwardExportDirectory;
    PIMAGE_IMPORT_BY_NAME ForwardName;
    SIZE_T ForwardLength;
    IMAGE_THUNK_DATA ForwardThunk;

    PAGED_CODE();

    /* Check if this is an ordinal */
    IsOrdinal = IMAGE_SNAP_BY_ORDINAL(Name->u1.Ordinal);
    if ((IsOrdinal) && !(SnapForwarder))
    {
        /* Get the ordinal number and set it as missing */
        Ordinal = (USHORT)(IMAGE_ORDINAL(Name->u1.Ordinal) -
                           ExportDirectory->Base);
        *MissingApi = (PCHAR)(ULONG_PTR)Ordinal;
    }
    else
    {
        /* Get the VA if we don't have to snap */
        if (!SnapForwarder) Name->u1.AddressOfData += (ULONG_PTR)ImageBase;
        NameImport = (PIMAGE_IMPORT_BY_NAME)Name->u1.AddressOfData;

        /* Copy the procedure name */
        RtlStringCbCopyA(*MissingApi,
                         MAXIMUM_FILENAME_LENGTH,
                         (PCHAR)NameImport->Name);

        /* Setup name tables */
        DPRINT("Import name: %s\n", NameImport->Name);
        NameTable = (PULONG)((ULONG_PTR)DllBase +
                             ExportDirectory->AddressOfNames);
        OrdinalTable = (PUSHORT)((ULONG_PTR)DllBase +
                                 ExportDirectory->AddressOfNameOrdinals);

        /* Get the hint and check if it's valid */
        Hint = NameImport->Hint;
        if ((Hint < ExportDirectory->NumberOfNames) &&
            !(strcmp((PCHAR)NameImport->Name, (PCHAR)DllBase + NameTable[Hint])))
        {
            /* We have a match, get the ordinal number from here */
            Ordinal = OrdinalTable[Hint];
        }
        else
        {
            /* Do a binary search */
            Ordinal = NameToOrdinal((PCHAR)NameImport->Name,
                                    DllBase,
                                    ExportDirectory->NumberOfNames,
                                    NameTable,
                                    OrdinalTable);

            /* Check if we couldn't find it */
            if (Ordinal == -1)
            {
                DPRINT1("Warning: Driver failed to load, %s not found\n", NameImport->Name);
                return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
            }
        }
    }

    /* Check if the ordinal is valid */
    if (Ordinal >= ExportDirectory->NumberOfFunctions)
    {
        /* It's not, fail */
        Status = STATUS_DRIVER_ORDINAL_NOT_FOUND;
    }
    else
    {
        /* In case the forwarder is missing */
        MissingForwarder = NameBuffer;

        /* Resolve the address and write it */
        ExportTable = (PULONG)((ULONG_PTR)DllBase +
                               ExportDirectory->AddressOfFunctions);
        Address->u1.Function = (ULONG_PTR)DllBase + ExportTable[Ordinal];

        /* Assume success from now on */
        Status = STATUS_SUCCESS;

        /* Check if the function is actually a forwarder */
        if ((Address->u1.Function > (ULONG_PTR)ExportDirectory) &&
            (Address->u1.Function < (ULONG_PTR)ExportDirectory + ExportSize))
        {
            /* Now assume failure in case the forwarder doesn't exist */
            Status = STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

            /* Build the forwarder name */
            DllName.Buffer = (PCHAR)Address->u1.Function;
            DllName.Length = (USHORT)(strchr(DllName.Buffer, '.') -
                                      DllName.Buffer) +
                                      sizeof(ANSI_NULL);
            DllName.MaximumLength = DllName.Length;

            /* Convert it */
            if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&ForwarderName,
                                                         &DllName,
                                                         TRUE)))
            {
                /* We failed, just return an error */
                return Status;
            }

            /* Loop the module list */
            NextEntry = PsLoadedModuleList.Flink;
            while (NextEntry != &PsLoadedModuleList)
            {
                /* Get the loader entry */
                LdrEntry = CONTAINING_RECORD(NextEntry,
                                             LDR_DATA_TABLE_ENTRY,
                                             InLoadOrderLinks);

                /* Check if it matches */
                if (RtlPrefixUnicodeString(&ForwarderName,
                                           &LdrEntry->BaseDllName,
                                           TRUE))
                {
                    /* Get the forwarder export directory */
                    ForwardExportDirectory =
                        RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                     TRUE,
                                                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                     &ForwardExportSize);
                    if (!ForwardExportDirectory) break;

                    /* Allocate a name entry */
                    ForwardLength = strlen(DllName.Buffer + DllName.Length) +
                                    sizeof(ANSI_NULL);
                    ForwardName = ExAllocatePoolWithTag(PagedPool,
                                                        sizeof(*ForwardName) +
                                                        ForwardLength,
                                                        TAG_LDR_WSTR);
                    if (!ForwardName) break;

                    /* Copy the data */
                    RtlCopyMemory(&ForwardName->Name[0],
                                  DllName.Buffer + DllName.Length,
                                  ForwardLength);
                    ForwardName->Hint = 0;

                    /* Set the new address */
                    ForwardThunk.u1.AddressOfData = (ULONG_PTR)ForwardName;

                    /* Snap the forwarder */
                    Status = MiSnapThunk(LdrEntry->DllBase,
                                         ImageBase,
                                         &ForwardThunk,
                                         &ForwardThunk,
                                         ForwardExportDirectory,
                                         ForwardExportSize,
                                         TRUE,
                                         &MissingForwarder);

                    /* Free the forwarder name and set the thunk */
                    ExFreePoolWithTag(ForwardName, TAG_LDR_WSTR);
                    Address->u1 = ForwardThunk.u1;
                    break;
                }

                /* Go to the next entry */
                NextEntry = NextEntry->Flink;
            }

            /* Free the name */
            RtlFreeUnicodeString(&ForwarderName);
        }
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
MmUnloadSystemImage(IN PVOID ImageHandle)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry = ImageHandle;
    PVOID BaseAddress = LdrEntry->DllBase;
    NTSTATUS Status;
    STRING TempName;
    BOOLEAN HadEntry = FALSE;

    /* Acquire the loader lock */
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Check if this driver was loaded at boot and didn't get imports parsed */
    if (LdrEntry->LoadedImports == MM_SYSLDR_BOOT_LOADED) goto Done;

    /* We should still be alive */
    ASSERT(LdrEntry->LoadCount != 0);
    LdrEntry->LoadCount--;

    /* Check if we're still loaded */
    if (LdrEntry->LoadCount) goto Done;

    /* We should cleanup... are symbols loaded */
    if (LdrEntry->Flags & LDRP_DEBUG_SYMBOLS_LOADED)
    {
        /* Create the ANSI name */
        Status = RtlUnicodeStringToAnsiString(&TempName,
                                              &LdrEntry->BaseDllName,
                                              TRUE);
        if (NT_SUCCESS(Status))
        {
            /* Unload the symbols */
            DbgUnLoadImageSymbols(&TempName,
                                  BaseAddress,
                                  (ULONG_PTR)PsGetCurrentProcessId());
            RtlFreeAnsiString(&TempName);
        }
    }

    /* FIXME: Free the driver */
    DPRINT1("Leaking driver: %wZ\n", &LdrEntry->BaseDllName);
    //MmFreeSection(LdrEntry->DllBase);

    /* Check if we're linked in */
    if (LdrEntry->InLoadOrderLinks.Flink)
    {
        /* Remove us */
        MiProcessLoaderEntry(LdrEntry, FALSE);
        HadEntry = TRUE;
    }

    /* Dereference and clear the imports */
    MiDereferenceImports(LdrEntry->LoadedImports);
    MiClearImports(LdrEntry);

    /* Check if the entry needs to go away */
    if (HadEntry)
    {
        /* Check if it had a name */
        if (LdrEntry->FullDllName.Buffer)
        {
            /* Free it */
            ExFreePoolWithTag(LdrEntry->FullDllName.Buffer, TAG_LDR_WSTR);
        }

        /* Check if we had a section */
        if (LdrEntry->SectionPointer)
        {
            /* Dereference it */
            ObDereferenceObject(LdrEntry->SectionPointer);
        }

        /* Free the entry */
        ExFreePoolWithTag(LdrEntry, TAG_MODULE_OBJECT);
    }

    /* Release the system lock and return */
Done:
    KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);
    KeLeaveCriticalRegion();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiResolveImageReferences(IN PVOID ImageBase,
                         IN PUNICODE_STRING ImageFileDirectory,
                         IN PUNICODE_STRING NamePrefix OPTIONAL,
                         OUT PCHAR *MissingApi,
                         OUT PWCHAR *MissingDriver,
                         OUT PLOAD_IMPORTS *LoadImports)
{
    static UNICODE_STRING DriversFolderName = RTL_CONSTANT_STRING(L"drivers\\");
    PCHAR MissingApiBuffer = *MissingApi, ImportName;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor, CurrentImport;
    ULONG ImportSize, ImportCount = 0, LoadedImportsSize, ExportSize;
    PLOAD_IMPORTS LoadedImports, NewImports;
    ULONG i;
    BOOLEAN GdiLink, NormalLink;
    BOOLEAN ReferenceNeeded, Loaded;
    ANSI_STRING TempString;
    UNICODE_STRING NameString, DllName;
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL, DllEntry, ImportEntry = NULL;
    PVOID ImportBase, DllBase;
    PLIST_ENTRY NextEntry;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    NTSTATUS Status;
    PIMAGE_THUNK_DATA OrigThunk, FirstThunk;

    PAGED_CODE();

    DPRINT("%s - ImageBase: %p. ImageFileDirectory: %wZ\n",
           __FUNCTION__, ImageBase, ImageFileDirectory);

    /* No name string buffer yet */
    NameString.Buffer = NULL;

    /* Assume no imports */
    *LoadImports = MM_SYSLDR_NO_IMPORTS;

    /* Get the import descriptor */
    ImportDescriptor = RtlImageDirectoryEntryToData(ImageBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                    &ImportSize);
    if (!ImportDescriptor) return STATUS_SUCCESS;

    /* Loop all imports to count them */
    for (CurrentImport = ImportDescriptor;
         (CurrentImport->Name) && (CurrentImport->OriginalFirstThunk);
         CurrentImport++)
    {
        /* One more */
        ImportCount++;
    }

    /* Make sure we have non-zero imports */
    if (ImportCount)
    {
        /* Calculate and allocate the list we'll need */
        LoadedImportsSize = ImportCount * sizeof(PVOID) + sizeof(SIZE_T);
        LoadedImports = ExAllocatePoolWithTag(PagedPool,
                                              LoadedImportsSize,
                                              TAG_LDR_IMPORTS);
        if (LoadedImports)
        {
            /* Zero it */
            RtlZeroMemory(LoadedImports, LoadedImportsSize);
            LoadedImports->Count = ImportCount;
        }
    }
    else
    {
        /* No table */
        LoadedImports = NULL;
    }

    /* Reset the import count and loop descriptors again */
    GdiLink = NormalLink = FALSE;
    ImportCount = 0;
    while ((ImportDescriptor->Name) && (ImportDescriptor->OriginalFirstThunk))
    {
        /* Get the name */
        ImportName = (PCHAR)((ULONG_PTR)ImageBase + ImportDescriptor->Name);

        /* Check if this is a GDI driver */
        GdiLink = GdiLink ||
                  !(_strnicmp(ImportName, "win32k", sizeof("win32k") - 1));

        /* We can also allow dxapi (for Windows compat, allow IRT and coverage) */
        NormalLink = NormalLink ||
                     ((_strnicmp(ImportName, "win32k", sizeof("win32k") - 1)) &&
                      (_strnicmp(ImportName, "dxapi", sizeof("dxapi") - 1)) &&
                      (_strnicmp(ImportName, "coverage", sizeof("coverage") - 1)) &&
                      (_strnicmp(ImportName, "irt", sizeof("irt") - 1)));

        /* Check if this is a valid GDI driver */
        if (GdiLink && NormalLink)
        {
            /* It's not, it's importing stuff it shouldn't be! */
            Status = STATUS_PROCEDURE_NOT_FOUND;
            goto Failure;
        }

        /* Check for user-mode printer or video card drivers, which don't belong */
        if (!(_strnicmp(ImportName, "ntdll", sizeof("ntdll") - 1)) ||
            !(_strnicmp(ImportName, "winsrv", sizeof("winsrv") - 1)) ||
            !(_strnicmp(ImportName, "advapi32", sizeof("advapi32") - 1)) ||
            !(_strnicmp(ImportName, "kernel32", sizeof("kernel32") - 1)) ||
            !(_strnicmp(ImportName, "user32", sizeof("user32") - 1)) ||
            !(_strnicmp(ImportName, "gdi32", sizeof("gdi32") - 1)))
        {
            /* This is not kernel code */
            Status = STATUS_PROCEDURE_NOT_FOUND;
            goto Failure;
        }

        /* Check if this is a "core" import, which doesn't get referenced */
        if (!(_strnicmp(ImportName, "ntoskrnl", sizeof("ntoskrnl") - 1)) ||
            !(_strnicmp(ImportName, "win32k", sizeof("win32k") - 1)) ||
            !(_strnicmp(ImportName, "hal", sizeof("hal") - 1)))
        {
            /* Don't reference this */
            ReferenceNeeded = FALSE;
        }
        else
        {
            /* Reference these modules */
            ReferenceNeeded = TRUE;
        }

        /* Now setup a unicode string for the import */
        RtlInitAnsiString(&TempString, ImportName);
        Status = RtlAnsiStringToUnicodeString(&NameString, &TempString, TRUE);
        if (!NT_SUCCESS(Status))
        {
            /* Failed */
            goto Failure;
        }

        /* We don't support name prefixes yet */
        if (NamePrefix) DPRINT1("Name Prefix not yet supported!\n");

        /* Remember that we haven't loaded the import at this point */
CheckDllState:
        Loaded = FALSE;
        ImportBase = NULL;

        /* Loop the driver list */
        NextEntry = PsLoadedModuleList.Flink;
        while (NextEntry != &PsLoadedModuleList)
        {
            /* Get the loader entry and compare the name */
            LdrEntry = CONTAINING_RECORD(NextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);
            if (RtlEqualUnicodeString(&NameString,
                                      &LdrEntry->BaseDllName,
                                      TRUE))
            {
                /* Get the base address */
                ImportBase = LdrEntry->DllBase;

                /* Check if we haven't loaded yet, and we need references */
                if (!(Loaded) && (ReferenceNeeded))
                {
                    /* Make sure we're not already loading */
                    if (!(LdrEntry->Flags & LDRP_LOAD_IN_PROGRESS))
                    {
                        /* Increase the load count */
                        LdrEntry->LoadCount++;
                    }
                }

                /* Done, break out */
                break;
            }

            /* Go to the next entry */
            NextEntry = NextEntry->Flink;
        }

        /* Check if we haven't loaded the import yet */
        if (!ImportBase)
        {
            /* Setup the import DLL name */
            DllName.MaximumLength = NameString.Length +
                                    ImageFileDirectory->Length +
                                    sizeof(UNICODE_NULL);
            DllName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                   DllName.MaximumLength,
                                                   TAG_LDR_WSTR);
            if (!DllName.Buffer)
            {
                /* We're out of resources */
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Failure;
            }

            /* Add the import name to the base directory */
            RtlCopyUnicodeString(&DllName, ImageFileDirectory);
            RtlAppendUnicodeStringToString(&DllName,
                                           &NameString);

            /* Load the image */
            Status = MmLoadSystemImage(&DllName,
                                       NamePrefix,
                                       NULL,
                                       FALSE,
                                       (PVOID *)&DllEntry,
                                       &DllBase);

            /* win32k / GDI drivers can also import from system32 folder */
            if ((Status == STATUS_OBJECT_NAME_NOT_FOUND) &&
                (MI_IS_SESSION_ADDRESS(ImageBase) || 1)) // HACK
            {
                /* Free the old name buffer */
                ExFreePoolWithTag(DllName.Buffer, TAG_LDR_WSTR);

                /* Calculate size for a string the adds 'drivers\' */
                DllName.MaximumLength += DriversFolderName.Length;

                /* Allocate the new buffer */
                DllName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                       DllName.MaximumLength,
                                                       TAG_LDR_WSTR);
                if (!DllName.Buffer)
                {
                    /* We're out of resources */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Failure;
                }

                /* Copy image directory and append 'drivers\' folder name */
                RtlCopyUnicodeString(&DllName, ImageFileDirectory);
                RtlAppendUnicodeStringToString(&DllName, &DriversFolderName);

                /* Now add the import name */
                RtlAppendUnicodeStringToString(&DllName, &NameString);

                /* Try once again to load the image */
                Status = MmLoadSystemImage(&DllName,
                                           NamePrefix,
                                           NULL,
                                           FALSE,
                                           (PVOID *)&DllEntry,
                                           &DllBase);
            }

            if (!NT_SUCCESS(Status))
            {
                /* Fill out the information for the error */
                *MissingDriver = DllName.Buffer;
                *(PULONG)MissingDriver |= 1;
                *MissingApi = NULL;

                DPRINT1("Failed to load dependency: %wZ\n", &DllName);

                /* Don't free the name */
                DllName.Buffer = NULL;

                /* Cleanup and return */
                goto Failure;
            }

            /* We can free the DLL Name */
            ExFreePoolWithTag(DllName.Buffer, TAG_LDR_WSTR);
            DllName.Buffer = NULL;

            /* We're now loaded */
            Loaded = TRUE;

            /* Sanity check */
            ASSERT(DllBase == DllEntry->DllBase);

            /* Call the initialization routines */
            Status = MmCallDllInitialize(DllEntry, &PsLoadedModuleList);
            if (!NT_SUCCESS(Status))
            {
                /* We failed, unload the image */
                MmUnloadSystemImage(DllEntry);
                ERROR_DBGBREAK("MmCallDllInitialize failed with status 0x%x\n", Status);
                Loaded = FALSE;
            }

            /* Loop again to make sure that everything is OK */
            goto CheckDllState;
        }

        /* Check if we're support to reference this import */
        if ((ReferenceNeeded) && (LoadedImports))
        {
            /* Make sure we're not already loading */
            if (!(LdrEntry->Flags & LDRP_LOAD_IN_PROGRESS))
            {
                /* Add the entry */
                LoadedImports->Entry[ImportCount] = LdrEntry;
                ImportCount++;
            }
        }

        /* Free the import name */
        RtlFreeUnicodeString(&NameString);

        /* Set the missing driver name and get the export directory */
        *MissingDriver = LdrEntry->BaseDllName.Buffer;
        ExportDirectory = RtlImageDirectoryEntryToData(ImportBase,
                                                       TRUE,
                                                       IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                       &ExportSize);
        if (!ExportDirectory)
        {
            /* Cleanup and return */
            DPRINT1("Warning: Driver failed to load, %S not found\n", *MissingDriver);
            Status = STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
            goto Failure;
        }

        /* Make sure we have an IAT */
        if (ImportDescriptor->OriginalFirstThunk)
        {
            /* Get the first thunks */
            OrigThunk = (PVOID)((ULONG_PTR)ImageBase +
                                ImportDescriptor->OriginalFirstThunk);
            FirstThunk = (PVOID)((ULONG_PTR)ImageBase +
                                 ImportDescriptor->FirstThunk);

            /* Loop the IAT */
            while (OrigThunk->u1.AddressOfData)
            {
                /* Snap thunk */
                Status = MiSnapThunk(ImportBase,
                                     ImageBase,
                                     OrigThunk++,
                                     FirstThunk++,
                                     ExportDirectory,
                                     ExportSize,
                                     FALSE,
                                     MissingApi);
                if (!NT_SUCCESS(Status))
                {
                    /* Cleanup and return */
                    goto Failure;
                }

                /* Reset the buffer */
                *MissingApi = MissingApiBuffer;
            }
        }

        /* Go to the next import */
        ImportDescriptor++;
    }

    /* Check if we have an import list */
    if (LoadedImports)
    {
        /* Reset the count again, and loop entries */
        ImportCount = 0;
        for (i = 0; i < LoadedImports->Count; i++)
        {
            if (LoadedImports->Entry[i])
            {
                /* Got an entry, OR it with 1 in case it's the single entry */
                ImportEntry = (PVOID)((ULONG_PTR)LoadedImports->Entry[i] |
                                      MM_SYSLDR_SINGLE_ENTRY);
                ImportCount++;
            }
        }

        /* Check if we had no imports */
        if (!ImportCount)
        {
            /* Free the list and set it to no imports */
            ExFreePoolWithTag(LoadedImports, TAG_LDR_IMPORTS);
            LoadedImports = MM_SYSLDR_NO_IMPORTS;
        }
        else if (ImportCount == 1)
        {
            /* Just one entry, we can free the table and only use our entry */
            ExFreePoolWithTag(LoadedImports, TAG_LDR_IMPORTS);
            LoadedImports = (PLOAD_IMPORTS)ImportEntry;
        }
        else if (ImportCount != LoadedImports->Count)
        {
            /* Allocate a new list */
            LoadedImportsSize = ImportCount * sizeof(PVOID) + sizeof(SIZE_T);
            NewImports = ExAllocatePoolWithTag(PagedPool,
                                               LoadedImportsSize,
                                               TAG_LDR_IMPORTS);
            if (NewImports)
            {
                /* Set count */
                NewImports->Count = 0;

                /* Loop all the imports */
                for (i = 0; i < LoadedImports->Count; i++)
                {
                    /* Make sure it's valid */
                    if (LoadedImports->Entry[i])
                    {
                        /* Copy it */
                        NewImports->Entry[NewImports->Count] = LoadedImports->Entry[i];
                        NewImports->Count++;
                    }
                }

                /* Free the old copy */
                ExFreePoolWithTag(LoadedImports, TAG_LDR_IMPORTS);
                LoadedImports = NewImports;
            }
        }

        /* Return the list */
        *LoadImports = LoadedImports;
    }

    /* Return success */
    return STATUS_SUCCESS;

Failure:

    /* Cleanup and return */
    RtlFreeUnicodeString(&NameString);

    if (LoadedImports)
    {
        MiDereferenceImports(LoadedImports);
        ExFreePoolWithTag(LoadedImports, TAG_LDR_IMPORTS);
    }

    return Status;
}

VOID
NTAPI
MiFreeInitializationCode(IN PVOID InitStart,
                         IN PVOID InitEnd)
{
    PMMPTE PointerPte;
    PFN_NUMBER PagesFreed;

    /* Get the start PTE */
    PointerPte = MiAddressToPte(InitStart);
    ASSERT(MI_IS_PHYSICAL_ADDRESS(InitStart) == FALSE);

    /*  Compute the number of pages we expect to free */
    PagesFreed = (PFN_NUMBER)(MiAddressToPte(InitEnd) - PointerPte);

    /* Try to actually free them */
    PagesFreed = MiDeleteSystemPageableVm(PointerPte,
                                          PagesFreed,
                                          0,
                                          NULL);
}

CODE_SEG("INIT")
VOID
NTAPI
MiFindInitializationCode(OUT PVOID *StartVa,
                         OUT PVOID *EndVa)
{
    ULONG Size, SectionCount, Alignment;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG_PTR DllBase, InitStart, InitEnd, ImageEnd, InitCode;
    PLIST_ENTRY NextEntry;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER Section, LastSection, InitSection;
    BOOLEAN InitFound;
    DBG_UNREFERENCED_LOCAL_VARIABLE(InitSection);

    /* So we don't free our own code yet */
    InitCode = (ULONG_PTR)&MiFindInitializationCode;

    /* Assume failure */
    *StartVa = NULL;

    /* Acquire the necessary locks while we loop the list */
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);
    ExAcquireResourceExclusiveLite(&PsLoadedModuleResource, TRUE);

    /* Loop all loaded modules */
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList)
    {
        /* Get the loader entry and its DLL base */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        DllBase = (ULONG_PTR)LdrEntry->DllBase;

        /* Only process boot loaded images. Other drivers are processed by
           MmFreeDriverInitialization */
        if (LdrEntry->Flags & LDRP_MM_LOADED)
        {
            /* Keep going */
            NextEntry = NextEntry->Flink;
            continue;
        }

        /* Get the NT header */
        NtHeader = RtlImageNtHeader((PVOID)DllBase);
        if (!NtHeader)
        {
            /* Keep going */
            NextEntry = NextEntry->Flink;
            continue;
        }

        /* Get the first section, the section count, and scan them all */
        Section = IMAGE_FIRST_SECTION(NtHeader);
        SectionCount = NtHeader->FileHeader.NumberOfSections;
        InitStart = 0;
        while (SectionCount > 0)
        {
            /* Assume failure */
            InitFound = FALSE;

            /* Is this the INIT section or a discardable section? */
            if ((strncmp((PCCH)Section->Name, "INIT", 5) == 0) ||
                ((Section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)))
            {
                /* Remember this */
                InitFound = TRUE;
                InitSection = Section;
            }

            if (InitFound)
            {
                /* Pick the biggest size -- either raw or virtual */
                Size = max(Section->SizeOfRawData, Section->Misc.VirtualSize);

                /* Read the section alignment */
                Alignment = NtHeader->OptionalHeader.SectionAlignment;

                /* Get the start and end addresses */
                InitStart = DllBase + Section->VirtualAddress;
                InitEnd = ALIGN_UP_BY(InitStart + Size, Alignment);

                /* Align the addresses to PAGE_SIZE */
                InitStart = ALIGN_UP_BY(InitStart, PAGE_SIZE);
                InitEnd = ALIGN_DOWN_BY(InitEnd, PAGE_SIZE);

                /* Have we reached the last section? */
                if (SectionCount == 1)
                {
                    /* Remember this */
                    LastSection = Section;
                }
                else
                {
                    /* We have not, loop all the sections */
                    LastSection = NULL;
                    do
                    {
                        /* Keep going until we find a non-discardable section range */
                        SectionCount--;
                        Section++;
                        if (Section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
                        {
                            /* Discardable, so record it, then keep going */
                            LastSection = Section;
                        }
                        else
                        {
                            /* Non-contigous discard flag, or no flag, break out */
                            break;
                        }
                    }
                    while (SectionCount > 1);
                }

                /* Have we found a discardable or init section? */
                if (LastSection)
                {
                    /* Pick the biggest size -- either raw or virtual */
                    Size = max(LastSection->SizeOfRawData, LastSection->Misc.VirtualSize);

                    /* Use this as the end of the section address */
                    InitEnd = DllBase + LastSection->VirtualAddress + Size;

                    /* Have we reached the last section yet? */
                    if (SectionCount != 1)
                    {
                        /* Then align this accross the session boundary */
                        InitEnd = ALIGN_UP_BY(InitEnd, Alignment);
                        InitEnd = ALIGN_DOWN_BY(InitEnd, PAGE_SIZE);
                    }
                }

                /* Make sure we don't let the init section go past the image */
                ImageEnd = DllBase + LdrEntry->SizeOfImage;
                if (InitEnd > ImageEnd) InitEnd = ALIGN_UP_BY(ImageEnd, PAGE_SIZE);

                /* Make sure we have a valid, non-zero init section */
                if (InitStart < InitEnd)
                {
                    /* Make sure we are not within this code itself */
                    if ((InitCode >= InitStart) && (InitCode < InitEnd))
                    {
                        /* Return it, we can't free ourselves now */
                        ASSERT(*StartVa == 0);
                        *StartVa = (PVOID)InitStart;
                        *EndVa = (PVOID)InitEnd;
                    }
                    else
                    {
                        /* This isn't us -- go ahead and free it */
                        ASSERT(MI_IS_PHYSICAL_ADDRESS((PVOID)InitStart) == FALSE);
                        DPRINT("Freeing init code: %p-%p ('%wZ' @%p : '%s')\n",
                               (PVOID)InitStart,
                               (PVOID)InitEnd,
                               &LdrEntry->BaseDllName,
                               LdrEntry->DllBase,
                               InitSection->Name);
                        MiFreeInitializationCode((PVOID)InitStart, (PVOID)InitEnd);
                    }
                }
            }

            /* Move to the next section */
            SectionCount--;
            Section++;
        }

        /* Move to the next module */
        NextEntry = NextEntry->Flink;
    }

    /* Release the locks and return */
    ExReleaseResourceLite(&PsLoadedModuleResource);
    KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);
    KeLeaveCriticalRegion();
}

/*
 * Note: This function assumes that all discardable sections are at the end of
 * the PE file. It searches backwards until it finds the non-discardable section
 */
VOID
NTAPI
MmFreeDriverInitialization(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PMMPTE StartPte, EndPte;
    PFN_NUMBER PageCount;
    PVOID DllBase;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER Section, DiscardSection;

    /* Get the base address and the page count */
    DllBase = LdrEntry->DllBase;
    PageCount = LdrEntry->SizeOfImage >> PAGE_SHIFT;

    /* Get the last PTE in this image */
    EndPte = MiAddressToPte(DllBase) + PageCount;

    /* Get the NT header */
    NtHeader = RtlImageNtHeader(DllBase);
    if (!NtHeader) return;

    /* Get the last section and loop each section backwards */
    Section = IMAGE_FIRST_SECTION(NtHeader) + NtHeader->FileHeader.NumberOfSections;
    DiscardSection = NULL;
    for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
    {
        /* Go back a section and check if it's discardable */
        Section--;
        if (Section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
        {
            /* It is, select it for freeing */
            DiscardSection = Section;
        }
        else
        {
            /* No more discardable sections exist, bail out */
            break;
        }
    }

    /* Bail out if there's nothing to free */
    if (!DiscardSection) return;

    /* Push the DLL base to the first disacrable section, and get its PTE */
    DllBase = (PVOID)ROUND_TO_PAGES((ULONG_PTR)DllBase + DiscardSection->VirtualAddress);
    ASSERT(MI_IS_PHYSICAL_ADDRESS(DllBase) == FALSE);
    StartPte = MiAddressToPte(DllBase);

    /* Check how many pages to free total */
    PageCount = (PFN_NUMBER)(EndPte - StartPte);
    if (!PageCount) return;

    /* Delete this many PTEs */
    MiDeleteSystemPageableVm(StartPte, PageCount, 0, NULL);
}

CODE_SEG("INIT")
VOID
NTAPI
MiReloadBootLoadedDrivers(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    ULONG i = 0;
    PIMAGE_NT_HEADERS NtHeader;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_FILE_HEADER FileHeader;
    BOOLEAN ValidRelocs;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    PVOID DllBase, NewImageAddress;
    NTSTATUS Status;
    PMMPTE PointerPte, StartPte, LastPte;
    PFN_COUNT PteCount;
    PMMPFN Pfn1;
    MMPTE TempPte, OldPte;

    /* Loop driver list */
    for (NextEntry = LoaderBlock->LoadOrderListHead.Flink;
         NextEntry != &LoaderBlock->LoadOrderListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the loader entry and NT header */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);
        NtHeader = RtlImageNtHeader(LdrEntry->DllBase);

        /* Debug info */
        DPRINT("[Mm0]: Driver at: %p ending at: %p for module: %wZ\n",
                LdrEntry->DllBase,
                (ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage,
                &LdrEntry->FullDllName);

        /* Get the first PTE and the number of PTEs we'll need */
        PointerPte = StartPte = MiAddressToPte(LdrEntry->DllBase);
        PteCount = ROUND_TO_PAGES(LdrEntry->SizeOfImage) >> PAGE_SHIFT;
        LastPte = StartPte + PteCount;

#if MI_TRACE_PFNS
        /* Loop the PTEs */
        while (PointerPte < LastPte)
        {
            ULONG len;
            ASSERT(PointerPte->u.Hard.Valid == 1);
            Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
            len = wcslen(LdrEntry->BaseDllName.Buffer) * sizeof(WCHAR);
            snprintf(Pfn1->ProcessName, min(16, len), "%S", LdrEntry->BaseDllName.Buffer);
            PointerPte++;
        }
#endif
        /* Skip kernel and HAL */
        /* ROS HACK: Skip BOOTVID/KDCOM too */
        i++;
        if (i <= 4) continue;

        /* Skip non-drivers */
        if (!NtHeader) continue;

        /* Get the file header and make sure we can relocate */
        FileHeader = &NtHeader->FileHeader;
        if (FileHeader->Characteristics & IMAGE_FILE_RELOCS_STRIPPED) continue;
        if (NtHeader->OptionalHeader.NumberOfRvaAndSizes <
            IMAGE_DIRECTORY_ENTRY_BASERELOC) continue;

        /* Everything made sense until now, check the relocation section too */
        DataDirectory = &NtHeader->OptionalHeader.
                        DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (!DataDirectory->VirtualAddress)
        {
            /* We don't really have relocations */
            ValidRelocs = FALSE;
        }
        else
        {
            /* Make sure the size is valid */
            if ((DataDirectory->VirtualAddress + DataDirectory->Size) >
                LdrEntry->SizeOfImage)
            {
                /* They're not, skip */
                continue;
            }

            /* We have relocations */
            ValidRelocs = TRUE;
        }

        /* Remember the original address */
        DllBase = LdrEntry->DllBase;

        /* Loop the PTEs */
        PointerPte = StartPte;
        while (PointerPte < LastPte)
        {
            /* Mark the page modified in the PFN database */
            ASSERT(PointerPte->u.Hard.Valid == 1);
            Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
            ASSERT(Pfn1->u3.e1.Rom == 0);
            Pfn1->u3.e1.Modified = TRUE;

            /* Next */
            PointerPte++;
        }

        /* Now reserve system PTEs for the image */
        PointerPte = MiReserveSystemPtes(PteCount, SystemPteSpace);
        if (!PointerPte)
        {
            /* Shouldn't happen */
            ERROR_FATAL("[Mm0]: Couldn't allocate driver section!\n");
            return;
        }

        /* This is the new virtual address for the module */
        LastPte = PointerPte + PteCount;
        NewImageAddress = MiPteToAddress(PointerPte);

        /* Sanity check */
        DPRINT("[Mm0]: Copying from: %p to: %p\n", DllBase, NewImageAddress);
        ASSERT(ExpInitializationPhase == 0);

        /* Loop the new driver PTEs */
        TempPte = ValidKernelPte;
        while (PointerPte < LastPte)
        {
            /* Copy the old data */
            OldPte = *StartPte;
            ASSERT(OldPte.u.Hard.Valid == 1);

            /* Set page number from the loader's memory */
            TempPte.u.Hard.PageFrameNumber = OldPte.u.Hard.PageFrameNumber;

            /* Write it */
            MI_WRITE_VALID_PTE(PointerPte, TempPte);

            /* Move on */
            PointerPte++;
            StartPte++;
        }

        /* Update position */
        PointerPte -= PteCount;

        /* Sanity check */
        ASSERT(*(PULONG)NewImageAddress == *(PULONG)DllBase);

        /* Set the image base to the address where the loader put it */
        NtHeader->OptionalHeader.ImageBase = (ULONG_PTR)DllBase;

        /* Check if we had relocations */
        if (ValidRelocs)
        {
            /* Relocate the image */
            Status = LdrRelocateImageWithBias(NewImageAddress,
                                              0,
                                              "SYSLDR",
                                              STATUS_SUCCESS,
                                              STATUS_CONFLICTING_ADDRESSES,
                                              STATUS_INVALID_IMAGE_FORMAT);
            if (!NT_SUCCESS(Status))
            {
                /* This shouldn't happen */
                ERROR_FATAL("Relocations failed!\n");
                return;
            }
        }

        /* Update the loader entry */
        LdrEntry->DllBase = NewImageAddress;

        /* Update the thunks */
        DPRINT("[Mm0]: Updating thunks to: %wZ\n", &LdrEntry->BaseDllName);
        MiUpdateThunks(LoaderBlock,
                       DllBase,
                       NewImageAddress,
                       LdrEntry->SizeOfImage);

        /* Update the loader entry */
        LdrEntry->Flags |= LDRP_SYSTEM_MAPPED;
        LdrEntry->EntryPoint = (PVOID)((ULONG_PTR)NewImageAddress +
                                NtHeader->OptionalHeader.AddressOfEntryPoint);
        LdrEntry->SizeOfImage = PteCount << PAGE_SHIFT;

        /* FIXME: We'll need to fixup the PFN linkage when switching to ARM3 */
    }
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
MiBuildImportsForBootDrivers(VOID)
{
    PLIST_ENTRY NextEntry, NextEntry2;
    PLDR_DATA_TABLE_ENTRY LdrEntry, KernelEntry, HalEntry, LdrEntry2, LastEntry;
    PLDR_DATA_TABLE_ENTRY* EntryArray;
    UNICODE_STRING KernelName = RTL_CONSTANT_STRING(L"ntoskrnl.exe");
    UNICODE_STRING HalName = RTL_CONSTANT_STRING(L"hal.dll");
    PLOAD_IMPORTS LoadedImports;
    ULONG LoadedImportsSize, ImportSize;
    PULONG_PTR ImageThunk;
    ULONG_PTR DllBase, DllEnd;
    ULONG Modules = 0, i, j = 0;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;

    /* Initialize variables */
    KernelEntry = HalEntry = LastEntry = NULL;

    /* Loop the loaded module list... we are early enough that no lock is needed */
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Check if it's the kernel or HAL */
        if (RtlEqualUnicodeString(&KernelName, &LdrEntry->BaseDllName, TRUE))
        {
            /* Found it */
            KernelEntry = LdrEntry;
        }
        else if (RtlEqualUnicodeString(&HalName, &LdrEntry->BaseDllName, TRUE))
        {
            /* Found it */
            HalEntry = LdrEntry;
        }

        /* Check if this is a driver DLL */
        if (LdrEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL)
        {
            /* Check if this is the HAL or kernel */
            if ((LdrEntry == HalEntry) || (LdrEntry == KernelEntry))
            {
                /* Add a reference */
                LdrEntry->LoadCount = 1;
            }
            else
            {
                /* No referencing needed */
                LdrEntry->LoadCount = 0;
            }
        }
        else
        {
            /* Add a reference for all other modules as well */
            LdrEntry->LoadCount = 1;
        }

        /* Remember this came from the loader */
        LdrEntry->LoadedImports = MM_SYSLDR_BOOT_LOADED;

        /* Keep looping */
        NextEntry = NextEntry->Flink;
        Modules++;
    }

    /* We must have at least found the kernel and HAL */
    if (!(HalEntry) || (!KernelEntry)) return STATUS_NOT_FOUND;

    /* Allocate the list */
    EntryArray = ExAllocatePoolWithTag(PagedPool, Modules * sizeof(PVOID), TAG_LDR_IMPORTS);
    if (!EntryArray) return STATUS_INSUFFICIENT_RESOURCES;

    /* Loop the loaded module list again */
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);
#ifdef _WORKING_LOADER_
        /* Get its imports */
        ImageThunk = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                  TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_IAT,
                                                  &ImportSize);
        if (!ImageThunk)
#else
        /* Get its imports */
        ImportDescriptor = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                        TRUE,
                                                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                        &ImportSize);
        if (!ImportDescriptor)
#endif
        {
            /* None present */
            LdrEntry->LoadedImports = MM_SYSLDR_NO_IMPORTS;
            NextEntry = NextEntry->Flink;
            continue;
        }

        /* Clear the list and count the number of IAT thunks */
        RtlZeroMemory(EntryArray, Modules * sizeof(PVOID));
#ifdef _WORKING_LOADER_
        ImportSize /= sizeof(ULONG_PTR);

        /* Scan the thunks */
        for (i = 0, DllBase = 0, DllEnd = 0; i < ImportSize; i++, ImageThunk++)
#else
        DllBase = DllEnd = i = 0;
        while ((ImportDescriptor->Name) &&
               (ImportDescriptor->OriginalFirstThunk))
        {
            /* Get the image thunk */
            ImageThunk = (PVOID)((ULONG_PTR)LdrEntry->DllBase +
                                 ImportDescriptor->FirstThunk);
            while (*ImageThunk)
#endif
            {
            /* Do we already have an address? */
            if (DllBase)
            {
                /* Is the thunk in the same address? */
                if ((*ImageThunk >= DllBase) && (*ImageThunk < DllEnd))
                {
                    /* Skip it, we already have a reference for it */
                    ASSERT(EntryArray[j]);
                    ImageThunk++;
                    continue;
                }
            }

            /* Loop the loaded module list to locate this address owner */
            j = 0;
            NextEntry2 = PsLoadedModuleList.Flink;
            while (NextEntry2 != &PsLoadedModuleList)
            {
                /* Get the entry */
                LdrEntry2 = CONTAINING_RECORD(NextEntry2,
                                              LDR_DATA_TABLE_ENTRY,
                                              InLoadOrderLinks);

                /* Get the address range for this module */
                DllBase = (ULONG_PTR)LdrEntry2->DllBase;
                DllEnd = DllBase + LdrEntry2->SizeOfImage;

                /* Check if this IAT entry matches it */
                if ((*ImageThunk >= DllBase) && (*ImageThunk < DllEnd))
                {
                    /* Save it */
                    //DPRINT1("Found imported dll: %wZ\n", &LdrEntry2->BaseDllName);
                    EntryArray[j] = LdrEntry2;
                    break;
                }

                /* Keep searching */
                NextEntry2 = NextEntry2->Flink;
                j++;
            }

            /* Do we have a thunk outside the range? */
            if ((*ImageThunk < DllBase) || (*ImageThunk >= DllEnd))
            {
                /* Could be 0... */
                if (*ImageThunk)
                {
                    /* Should not be happening */
                    ERROR_FATAL("Broken IAT entry for %p at %p (%lx)\n",
                                LdrEntry, ImageThunk, *ImageThunk);
                }

                /* Reset if we hit this */
                DllBase = 0;
            }
#ifndef _WORKING_LOADER_
            ImageThunk++;
            }

            i++;
            ImportDescriptor++;
#endif
        }

        /* Now scan how many imports we really have */
        for (i = 0, ImportSize = 0; i < Modules; i++)
        {
            /* Skip HAL and kernel */
            if ((EntryArray[i]) &&
                (EntryArray[i] != HalEntry) &&
                (EntryArray[i] != KernelEntry))
            {
                /* A valid reference */
                LastEntry = EntryArray[i];
                ImportSize++;
            }
        }

        /* Do we have any imports after all? */
        if (!ImportSize)
        {
            /* No */
            LdrEntry->LoadedImports = MM_SYSLDR_NO_IMPORTS;
        }
        else if (ImportSize == 1)
        {
            /* A single entry import */
            LdrEntry->LoadedImports = (PVOID)((ULONG_PTR)LastEntry | MM_SYSLDR_SINGLE_ENTRY);
            LastEntry->LoadCount++;
        }
        else
        {
            /* We need an import table */
            LoadedImportsSize = ImportSize * sizeof(PVOID) + sizeof(SIZE_T);
            LoadedImports = ExAllocatePoolWithTag(PagedPool,
                                                  LoadedImportsSize,
                                                  TAG_LDR_IMPORTS);
            ASSERT(LoadedImports);

            /* Save the count */
            LoadedImports->Count = ImportSize;

            /* Now copy all imports */
            for (i = 0, j = 0; i < Modules; i++)
            {
                /* Skip HAL and kernel */
                if ((EntryArray[i]) &&
                    (EntryArray[i] != HalEntry) &&
                    (EntryArray[i] != KernelEntry))
                {
                    /* A valid reference */
                    //DPRINT1("Found valid entry: %p\n", EntryArray[i]);
                    LoadedImports->Entry[j] = EntryArray[i];
                    EntryArray[i]->LoadCount++;
                    j++;
                }
            }

            /* Should had as many entries as we expected */
            ASSERT(j == ImportSize);
            LdrEntry->LoadedImports = LoadedImports;
        }

        /* Next */
        NextEntry = NextEntry->Flink;
    }

    /* Free the initial array */
    ExFreePoolWithTag(EntryArray, TAG_LDR_IMPORTS);

    /* FIXME: Might not need to keep the HAL/Kernel imports around */

    /* Kernel and HAL are loaded at boot */
    KernelEntry->LoadedImports = MM_SYSLDR_BOOT_LOADED;
    HalEntry->LoadedImports = MM_SYSLDR_BOOT_LOADED;

    /* All worked well */
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
VOID
NTAPI
MiLocateKernelSections(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG_PTR DllBase;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG Sections, Size;

    /* Get the kernel section header */
    DllBase = (ULONG_PTR)LdrEntry->DllBase;
    NtHeaders = RtlImageNtHeader((PVOID)DllBase);
    SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

    /* Loop all the sections */
    for (Sections = NtHeaders->FileHeader.NumberOfSections;
         Sections > 0; --Sections, ++SectionHeader)
    {
        /* Grab the size of the section */
        Size = max(SectionHeader->SizeOfRawData, SectionHeader->Misc.VirtualSize);

        /* Check for .RSRC section */
        if (*(PULONG)SectionHeader->Name == 'rsr.')
        {
            /* Remember the PTEs so we can modify them later */
            MiKernelResourceStartPte = MiAddressToPte(DllBase +
                                                      SectionHeader->VirtualAddress);
            MiKernelResourceEndPte = MiAddressToPte(ROUND_TO_PAGES(DllBase +
                                                    SectionHeader->VirtualAddress + Size));
        }
        else if (*(PULONG)SectionHeader->Name == 'LOOP')
        {
            /* POOLCODE vs. POOLMI */
            if (*(PULONG)&SectionHeader->Name[4] == 'EDOC')
            {
                /* Found Ex* Pool code */
                ExPoolCodeStart = DllBase + SectionHeader->VirtualAddress;
                ExPoolCodeEnd = ExPoolCodeStart + Size;
            }
            else if (*(PUSHORT)&SectionHeader->Name[4] == 'MI')
            {
                /* Found Mm* Pool code */
                MmPoolCodeStart = DllBase + SectionHeader->VirtualAddress;
                MmPoolCodeEnd = MmPoolCodeStart + Size;
            }
        }
        else if ((*(PULONG)SectionHeader->Name == 'YSIM') &&
                 (*(PULONG)&SectionHeader->Name[4] == 'ETPS'))
        {
            /* Found MISYSPTE (Mm System PTE code) */
            MmPteCodeStart = DllBase + SectionHeader->VirtualAddress;
            MmPteCodeEnd = MmPteCodeStart + Size;
        }
    }
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
MiInitializeLoadedModuleList(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry, NewEntry;
    PLIST_ENTRY ListHead, NextEntry;
    ULONG EntrySize;

    /* Setup the loaded module list and locks */
    ExInitializeResourceLite(&PsLoadedModuleResource);
    KeInitializeSpinLock(&PsLoadedModuleSpinLock);
    InitializeListHead(&PsLoadedModuleList);

    /* Get loop variables and the kernel entry */
    ListHead = &LoaderBlock->LoadOrderListHead;
    NextEntry = ListHead->Flink;
    LdrEntry = CONTAINING_RECORD(NextEntry,
                                 LDR_DATA_TABLE_ENTRY,
                                 InLoadOrderLinks);
    PsNtosImageBase = (ULONG_PTR)LdrEntry->DllBase;

    /* Locate resource section, pool code, and system pte code */
    MiLocateKernelSections(LdrEntry);

    /* Loop the loader block */
    while (NextEntry != ListHead)
    {
        /* Get the loader entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* FIXME: ROS HACK. Make sure this is a driver */
        if (!RtlImageNtHeader(LdrEntry->DllBase))
        {
            /* Skip this entry */
            NextEntry = NextEntry->Flink;
            continue;
        }

        /* Calculate the size we'll need and allocate a copy */
        EntrySize = sizeof(LDR_DATA_TABLE_ENTRY) +
                    LdrEntry->BaseDllName.MaximumLength +
                    sizeof(UNICODE_NULL);
        NewEntry = ExAllocatePoolWithTag(NonPagedPool, EntrySize, TAG_MODULE_OBJECT);
        if (!NewEntry) return FALSE;

        /* Copy the entry over */
        *NewEntry = *LdrEntry;

        /* Allocate the name */
        NewEntry->FullDllName.Buffer =
            ExAllocatePoolWithTag(PagedPool,
                                  LdrEntry->FullDllName.MaximumLength +
                                      sizeof(UNICODE_NULL),
                                  TAG_LDR_WSTR);
        if (!NewEntry->FullDllName.Buffer)
        {
            ExFreePoolWithTag(NewEntry, TAG_MODULE_OBJECT);
            return FALSE;
        }

        /* Set the base name */
        NewEntry->BaseDllName.Buffer = (PVOID)(NewEntry + 1);

        /* Copy the full and base name */
        RtlCopyMemory(NewEntry->FullDllName.Buffer,
                      LdrEntry->FullDllName.Buffer,
                      LdrEntry->FullDllName.MaximumLength);
        RtlCopyMemory(NewEntry->BaseDllName.Buffer,
                      LdrEntry->BaseDllName.Buffer,
                      LdrEntry->BaseDllName.MaximumLength);

        /* Null-terminate the base name */
        NewEntry->BaseDllName.Buffer[NewEntry->BaseDllName.Length /
                                     sizeof(WCHAR)] = UNICODE_NULL;

        /* Insert the entry into the list */
        InsertTailList(&PsLoadedModuleList, &NewEntry->InLoadOrderLinks);
        NextEntry = NextEntry->Flink;
    }

    /* Build the import lists for the boot drivers */
    MiBuildImportsForBootDrivers();

    /* We're done */
    return TRUE;
}

BOOLEAN
NTAPI
MmChangeKernelResourceSectionProtection(IN ULONG_PTR ProtectionMask)
{
    PMMPTE PointerPte;
    MMPTE TempPte;

    /* Don't do anything if the resource section is already writable */
    if (MiKernelResourceStartPte == NULL || MiKernelResourceEndPte == NULL)
        return FALSE;

    /* If the resource section is physical, we cannot change its protection */
    if (MI_IS_PHYSICAL_ADDRESS(MiPteToAddress(MiKernelResourceStartPte)))
        return FALSE;

    /* Loop the PTEs */
    for (PointerPte = MiKernelResourceStartPte; PointerPte < MiKernelResourceEndPte; ++PointerPte)
    {
        /* Read the PTE */
        TempPte = *PointerPte;

        /* Update the protection */
        MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, PointerPte, ProtectionMask, TempPte.u.Hard.PageFrameNumber);
        MI_UPDATE_VALID_PTE(PointerPte, TempPte);
    }

    /* Only flush the current processor's TLB */
    KeFlushCurrentTb();
    return TRUE;
}

VOID
NTAPI
MmMakeKernelResourceSectionWritable(VOID)
{
    /* Don't do anything if the resource section is already writable */
    if (MiKernelResourceStartPte == NULL || MiKernelResourceEndPte == NULL)
        return;

    /* If the resource section is physical, we cannot change its protection */
    if (MI_IS_PHYSICAL_ADDRESS(MiPteToAddress(MiKernelResourceStartPte)))
        return;

    if (MmChangeKernelResourceSectionProtection(MM_READWRITE))
    {
        /*
         * Invalidate the cached resource section PTEs
         * so as to not change its protection again later.
         */
        MiKernelResourceStartPte = NULL;
        MiKernelResourceEndPte = NULL;
    }
}

LOGICAL
NTAPI
MiUseLargeDriverPage(IN ULONG NumberOfPtes,
                     IN OUT PVOID *ImageBaseAddress,
                     IN PUNICODE_STRING BaseImageName,
                     IN BOOLEAN BootDriver)
{
    PLIST_ENTRY NextEntry;
    BOOLEAN DriverFound = FALSE;
    PMI_LARGE_PAGE_DRIVER_ENTRY LargePageDriverEntry;
    ASSERT(KeGetCurrentIrql () <= APC_LEVEL);
    ASSERT(*ImageBaseAddress >= MmSystemRangeStart);

#ifdef _X86_
    if (!(KeFeatureBits & KF_LARGE_PAGE)) return FALSE;
    if (!(__readcr4() & CR4_PSE)) return FALSE;
#endif

    /* Make sure there's enough system PTEs for a large page driver */
    if (MmTotalFreeSystemPtes[SystemPteSpace] < (16 * (PDE_MAPPED_VA >> PAGE_SHIFT)))
    {
        return FALSE;
    }

    /* This happens if the registry key had a "*" (wildcard) in it */
    if (MiLargePageAllDrivers == 0)
    {
        /* It didn't, so scan the list */
        NextEntry = MiLargePageDriverList.Flink;
        while (NextEntry != &MiLargePageDriverList)
        {
            /* Check if the driver name matches */
            LargePageDriverEntry = CONTAINING_RECORD(NextEntry,
                                                     MI_LARGE_PAGE_DRIVER_ENTRY,
                                                     Links);
            if (RtlEqualUnicodeString(BaseImageName,
                                      &LargePageDriverEntry->BaseName,
                                      TRUE))
            {
                /* Enable large pages for this driver */
                DriverFound = TRUE;
                break;
            }

            /* Keep trying */
            NextEntry = NextEntry->Flink;
        }

        /* If we didn't find the driver, it doesn't need large pages */
        if (DriverFound == FALSE) return FALSE;
    }

    /* Nothing to do yet */
    DPRINT1("Large pages not supported!\n");
    return FALSE;
}

VOID
NTAPI
MiSetSystemCodeProtection(
    _In_ PMMPTE FirstPte,
    _In_ PMMPTE LastPte,
    _In_ ULONG Protection)
{
    PMMPTE PointerPte;
    MMPTE TempPte;

    /* Loop the PTEs */
    for (PointerPte = FirstPte; PointerPte <= LastPte; PointerPte++)
    {
        /* Read the PTE */
        TempPte = *PointerPte;

        /* Make sure it's valid */
        if (TempPte.u.Hard.Valid != 1)
        {
            DPRINT1("CORE-16449: FirstPte=%p, LastPte=%p, Protection=%lx\n", FirstPte, LastPte, Protection);
            DPRINT1("CORE-16449: PointerPte=%p, TempPte=%lx\n", PointerPte, TempPte.u.Long);
            DPRINT1("CORE-16449: Please issue the 'mod' and 'bt' (KDBG) or 'lm' and 'kp' (WinDbg) commands. Then report this in Jira.\n");
            ASSERT(TempPte.u.Hard.Valid == 1);
            break;
        }

        /* Update the protection */
        TempPte.u.Hard.Write = BooleanFlagOn(Protection, IMAGE_SCN_MEM_WRITE);
#if _MI_HAS_NO_EXECUTE
        TempPte.u.Hard.NoExecute = !BooleanFlagOn(Protection, IMAGE_SCN_MEM_EXECUTE);
#endif

        MI_UPDATE_VALID_PTE(PointerPte, TempPte);
    }

    /* Flush it all */
    KeFlushEntireTb(TRUE, TRUE);
}

VOID
NTAPI
MiWriteProtectSystemImage(
    _In_ PVOID ImageBase)
{
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeaders, Section;
    ULONG i;
    PVOID SectionBase, SectionEnd;
    ULONG SectionSize;
    ULONG Protection;
    PMMPTE FirstPte, LastPte;

    /* Check if the registry setting is on or not */
    if (MmEnforceWriteProtection == FALSE)
    {
        /* Ignore section protection */
        return;
    }

    /* Large page mapped images are not supported */
    NT_ASSERT(!MI_IS_PHYSICAL_ADDRESS(ImageBase));

    /* Session images are not yet supported */
    NT_ASSERT(!MI_IS_SESSION_ADDRESS(ImageBase));

    /* Get the NT headers */
    NtHeaders = RtlImageNtHeader(ImageBase);
    if (NtHeaders == NULL)
    {
        DPRINT1("Failed to get NT headers for image @ %p\n", ImageBase);
        return;
    }

    /* Don't touch NT4 drivers */
    if ((NtHeaders->OptionalHeader.MajorOperatingSystemVersion < 5) ||
        (NtHeaders->OptionalHeader.MajorSubsystemVersion < 5))
    {
        DPRINT1("Skipping NT 4 driver @ %p\n", ImageBase);
        return;
    }

    /* Get the section headers */
    SectionHeaders = IMAGE_FIRST_SECTION(NtHeaders);

    /* Get the base address of the first section */
    SectionBase = Add2Ptr(ImageBase, SectionHeaders[0].VirtualAddress);

    /* Start protecting the image header as R/W */
    FirstPte = MiAddressToPte(ImageBase);
    LastPte = MiAddressToPte(SectionBase) - 1;
    Protection = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    if (LastPte >= FirstPte)
    {
        MiSetSystemCodeProtection(FirstPte, LastPte, Protection);
    }

    /* Loop the sections */
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
    {
        /* Get the section base address and size */
        Section = &SectionHeaders[i];
        SectionBase = Add2Ptr(ImageBase, Section->VirtualAddress);
        SectionSize = max(Section->SizeOfRawData, Section->Misc.VirtualSize);

        /* Get the first PTE of this section */
        FirstPte = MiAddressToPte(SectionBase);

        /* Check for overlap with the previous range */
        if (FirstPte == LastPte)
        {
            /* Combine the old and new protection by ORing them */
            Protection |= (Section->Characteristics & IMAGE_SCN_PROTECTION_MASK);

            /* Update the protection for this PTE */
            MiSetSystemCodeProtection(FirstPte, FirstPte, Protection);

            /* Skip this PTE */
            FirstPte++;
        }

        /* There can not be gaps! */
        NT_ASSERT(FirstPte == (LastPte + 1));

        /* Get the end of the section and the last PTE */
        SectionEnd = Add2Ptr(SectionBase, SectionSize - 1);
        NT_ASSERT(SectionEnd < Add2Ptr(ImageBase, NtHeaders->OptionalHeader.SizeOfImage));
        LastPte = MiAddressToPte(SectionEnd);

        /* If there are no more pages (after an overlap), skip this section */
        if (LastPte < FirstPte)
        {
            NT_ASSERT(FirstPte == (LastPte + 1));
            continue;
        }

        /* Get the section protection */
        Protection = (Section->Characteristics & IMAGE_SCN_PROTECTION_MASK);

        /* Update the protection for this section */
        MiSetSystemCodeProtection(FirstPte, LastPte, Protection);
    }

    /* Image should end with the last section */
    if (ALIGN_UP_POINTER_BY(SectionEnd, PAGE_SIZE) !=
        Add2Ptr(ImageBase, NtHeaders->OptionalHeader.SizeOfImage))
    {
        DPRINT1("ImageBase 0x%p ImageSize 0x%lx Section %u VA 0x%lx Raw 0x%lx virt 0x%lx\n",
            ImageBase,
            NtHeaders->OptionalHeader.SizeOfImage,
            i,
            Section->VirtualAddress,
            Section->SizeOfRawData,
            Section->Misc.VirtualSize);
    }
}

VOID
NTAPI
MiSetPagingOfDriver(IN PMMPTE PointerPte,
                    IN PMMPTE LastPte)
{
#ifdef ENABLE_MISETPAGINGOFDRIVER
    PVOID ImageBase;
    PETHREAD CurrentThread = PsGetCurrentThread();
    PFN_COUNT PageCount = 0;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
#endif // ENABLE_MISETPAGINGOFDRIVER

    PAGED_CODE();

#ifndef ENABLE_MISETPAGINGOFDRIVER
    /* The page fault handler is broken and doesn't page back in! */
    DPRINT1("WARNING: MiSetPagingOfDriver() called, but paging is broken! ignoring!\n");
#else  // ENABLE_MISETPAGINGOFDRIVER
    /* Get the driver's base address */
    ImageBase = MiPteToAddress(PointerPte);
    ASSERT(MI_IS_SESSION_IMAGE_ADDRESS(ImageBase) == FALSE);

    /* If this is a large page, it's stuck in physical memory */
    if (MI_IS_PHYSICAL_ADDRESS(ImageBase)) return;

    /* Lock the working set */
    MiLockWorkingSet(CurrentThread, &MmSystemCacheWs);

    /* Loop the PTEs */
    while (PointerPte <= LastPte)
    {
        /* Check for valid PTE */
        if (PointerPte->u.Hard.Valid == 1)
        {
            PageFrameIndex = PFN_FROM_PTE(PointerPte);
            Pfn1 = MiGetPfnEntry(PageFrameIndex);
            ASSERT(Pfn1->u2.ShareCount == 1);

            /* No working sets in ReactOS yet */
            PageCount++;
        }

        ImageBase = (PVOID)((ULONG_PTR)ImageBase + PAGE_SIZE);
        PointerPte++;
    }

    /* Release the working set */
    MiUnlockWorkingSet(CurrentThread, &MmSystemCacheWs);

    /* Do we have any driver pages? */
    if (PageCount)
    {
        /* Update counters */
        InterlockedExchangeAdd((PLONG)&MmTotalSystemDriverPages, PageCount);
    }
#endif // ENABLE_MISETPAGINGOFDRIVER
}

VOID
NTAPI
MiEnablePagingOfDriver(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG_PTR ImageBase;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG Sections, Alignment, Size;
    PIMAGE_SECTION_HEADER Section;
    PMMPTE PointerPte = NULL, LastPte = NULL;
    if (MmDisablePagingExecutive) return;

    /* Get the driver base address and its NT header */
    ImageBase = (ULONG_PTR)LdrEntry->DllBase;
    NtHeaders = RtlImageNtHeader((PVOID)ImageBase);
    if (!NtHeaders) return;

    /* Get the sections and their alignment */
    Sections = NtHeaders->FileHeader.NumberOfSections;
    Alignment = NtHeaders->OptionalHeader.SectionAlignment - 1;

    /* Loop each section */
    Section = IMAGE_FIRST_SECTION(NtHeaders);
    while (Sections)
    {
        /* Find PAGE or .edata */
        if ((*(PULONG)Section->Name == 'EGAP') ||
            (*(PULONG)Section->Name == 'ade.'))
        {
            /* Had we already done some work? */
            if (!PointerPte)
            {
                /* Nope, setup the first PTE address */
                PointerPte = MiAddressToPte(ROUND_TO_PAGES(ImageBase +
                                                           Section->VirtualAddress));
            }

            /* Compute the size */
            Size = max(Section->SizeOfRawData, Section->Misc.VirtualSize);

            /* Find the last PTE that maps this section */
            LastPte = MiAddressToPte(ImageBase +
                                     Section->VirtualAddress +
                                     Alignment + Size - PAGE_SIZE);
        }
        else
        {
            /* Had we found a section before? */
            if (PointerPte)
            {
                /* Mark it as pageable */
                MiSetPagingOfDriver(PointerPte, LastPte);
                PointerPte = NULL;
            }
        }

        /* Keep searching */
        Sections--;
        Section++;
    }

    /* Handle the straggler */
    if (PointerPte) MiSetPagingOfDriver(PointerPte, LastPte);
}

#ifdef CONFIG_SMP
FORCEINLINE
BOOLEAN
MiVerifyImageIsOkForMpUse(
    _In_ PIMAGE_NT_HEADERS NtHeaders)
{
    /* Fail if we have more than one CPU, but the image is only safe for UP */
    if ((KeNumberProcessors > 1) &&
        (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY))
    {
        return FALSE;
    }
    /* Otherwise, it's safe to use */
    return TRUE;
}

BOOLEAN
NTAPI
MmVerifyImageIsOkForMpUse(
    _In_ PVOID BaseAddress)
{
    PIMAGE_NT_HEADERS NtHeaders;
    PAGED_CODE();

    /* Get the NT headers. If none, suppose the image is safe
     * to use on an MP system, otherwise invoke the helper. */
    NtHeaders = RtlImageNtHeader(BaseAddress);
    if (!NtHeaders)
        return TRUE;
    return MiVerifyImageIsOkForMpUse(NtHeaders);
}
#endif // CONFIG_SMP

NTSTATUS
NTAPI
MmCheckSystemImage(
    _In_ HANDLE ImageHandle)
{
    NTSTATUS Status;
    HANDLE SectionHandle;
    PVOID ViewBase = NULL;
    SIZE_T ViewSize = 0;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    KAPC_STATE ApcState;
    PIMAGE_NT_HEADERS NtHeaders;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PAGED_CODE();

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Create a section for the DLL */
    Status = ZwCreateSection(&SectionHandle,
                             SECTION_MAP_EXECUTE,
                             &ObjectAttributes,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             ImageHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwCreateSection failed with status 0x%x\n", Status);
        return Status;
    }

    /* Make sure we're in the system process */
    KeStackAttachProcess(&PsInitialSystemProcess->Pcb, &ApcState);

    /* Map it */
    Status = ZwMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_EXECUTE);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, close the handle and return */
        DPRINT1("ZwMapViewOfSection failed with status 0x%x\n", Status);
        KeUnstackDetachProcess(&ApcState);
        ZwClose(SectionHandle);
        return Status;
    }

    /* Now query image information */
    Status = ZwQueryInformationFile(ImageHandle,
                                    &IoStatusBlock,
                                    &FileStandardInfo,
                                    sizeof(FileStandardInfo),
                                    FileStandardInformation);
    if (NT_SUCCESS(Status))
    {
        /* First, verify the checksum */
        if (!LdrVerifyMappedImageMatchesChecksum(ViewBase,
                                                 ViewSize,
                                                 FileStandardInfo.
                                                 EndOfFile.LowPart))
        {
            /* Set checksum failure */
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
            goto Fail;
        }

        /* Make sure it's a real image */
        NtHeaders = RtlImageNtHeader(ViewBase);
        if (!NtHeaders)
        {
            /* Set checksum failure */
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
            goto Fail;
        }

        /* Make sure it's for the correct architecture */
        if ((NtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_NATIVE) ||
            (NtHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
        {
            /* Set protection failure */
            Status = STATUS_INVALID_IMAGE_PROTECT;
            goto Fail;
        }

#ifdef CONFIG_SMP
        /* Check that the image is safe to use if we have more than one CPU */
        if (!MiVerifyImageIsOkForMpUse(NtHeaders))
        {
            /* Otherwise it's not the right image */
            Status = STATUS_IMAGE_MP_UP_MISMATCH;
        }
#endif // CONFIG_SMP
    }

    /* Unmap the section, close the handle, and return status */
Fail:
    ZwUnmapViewOfSection(NtCurrentProcess(), ViewBase);
    KeUnstackDetachProcess(&ApcState);
    ZwClose(SectionHandle);
    return Status;
}


PVOID
NTAPI
LdrpFetchAddressOfSecurityCookie(PVOID BaseAddress, ULONG SizeOfImage)
{
    PIMAGE_LOAD_CONFIG_DIRECTORY ConfigDir;
    ULONG DirSize;
    PULONG_PTR Cookie = NULL;

    /* Get the pointer to the config directory */
    ConfigDir = RtlImageDirectoryEntryToData(BaseAddress,
                                             TRUE,
                                             IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                             &DirSize);

    /* Check for sanity */
    if (!ConfigDir ||
        DirSize < RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, SecurityCookie))
    {
        /* Invalid directory*/
        return NULL;
    }

    /* Now get the cookie */
    Cookie = (PULONG_PTR)ConfigDir->SecurityCookie;

    /* Check this cookie */
    if ((PCHAR)Cookie <= (PCHAR)BaseAddress ||
        (PCHAR)Cookie >= (PCHAR)BaseAddress + SizeOfImage - sizeof(*Cookie))
    {
        Cookie = NULL;
    }

    /* Return validated security cookie */
    return Cookie;
}

PVOID
NTAPI
LdrpInitSecurityCookie(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PULONG_PTR Cookie;
    ULONG_PTR NewCookie;

    /* Fetch address of the cookie */
    Cookie = LdrpFetchAddressOfSecurityCookie(LdrEntry->DllBase, LdrEntry->SizeOfImage);

    if (!Cookie)
        return NULL;
    
    /* Check if it's a default one */
    if ((*Cookie == DEFAULT_SECURITY_COOKIE) ||
        (*Cookie == 0))
    {
        LARGE_INTEGER Counter = KeQueryPerformanceCounter(NULL);
        /* The address should be unique */
        NewCookie = (ULONG_PTR)Cookie;

        /* We just need a simple tick, don't care about precision and whatnot */
        NewCookie ^= (ULONG_PTR)Counter.LowPart;
#ifdef _WIN64
        /* Some images expect first 16 bits to be kept clean (like in default cookie) */
        if (NewCookie > COOKIE_MAX)
        {
            NewCookie >>= 16;
        }
#endif
        /* If the result is 0 or the same as we got, just add one to the default value */
        if ((NewCookie == 0) || (NewCookie == *Cookie))
        {
            NewCookie = DEFAULT_SECURITY_COOKIE + 1;
        }

        /* Set the new cookie value */
        *Cookie = NewCookie; 
    }

    return Cookie;
}

NTSTATUS
NTAPI
MmLoadSystemImage(IN PUNICODE_STRING FileName,
                  IN PUNICODE_STRING NamePrefix OPTIONAL,
                  IN PUNICODE_STRING LoadedName OPTIONAL,
                  IN ULONG Flags,
                  OUT PVOID *ModuleObject,
                  OUT PVOID *ImageBaseAddress)
{
    PVOID ModuleLoadBase = NULL;
    NTSTATUS Status;
    HANDLE FileHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING BaseName, BaseDirectory, PrefixName;
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
    ULONG EntrySize, DriverSize;
    PLOAD_IMPORTS LoadedImports = MM_SYSLDR_NO_IMPORTS;
    PCHAR MissingApiName, Buffer;
    PWCHAR MissingDriverName, PrefixedBuffer = NULL;
    HANDLE SectionHandle;
    ACCESS_MASK DesiredAccess;
    PSECTION Section = NULL;
    BOOLEAN LockOwned = FALSE;
    PLIST_ENTRY NextEntry;
    IMAGE_INFO ImageInfo;

    PAGED_CODE();

    /* Detect session-load */
    if (Flags)
    {
        /* Sanity checks */
        ASSERT(NamePrefix == NULL);
        ASSERT(LoadedName == NULL);

        /* Make sure the process is in session too */
        if (!PsGetCurrentProcess()->ProcessInSession) return STATUS_NO_MEMORY;
    }

    /* Allocate a buffer we'll use for names */
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   MAXIMUM_FILENAME_LENGTH,
                                   TAG_LDR_WSTR);
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    /* Check for a separator */
    if (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        PWCHAR p;
        ULONG BaseLength;

        /* Loop the path until we get to the base name */
        p = &FileName->Buffer[FileName->Length / sizeof(WCHAR)];
        while (*(p - 1) != OBJ_NAME_PATH_SEPARATOR) p--;

        /* Get the length */
        BaseLength = (ULONG)(&FileName->Buffer[FileName->Length / sizeof(WCHAR)] - p);
        BaseLength *= sizeof(WCHAR);

        /* Setup the string */
        BaseName.Length = (USHORT)BaseLength;
        BaseName.Buffer = p;
    }
    else
    {
        /* Otherwise, we already have a base name */
        BaseName.Length = FileName->Length;
        BaseName.Buffer = FileName->Buffer;
    }

    /* Setup the maximum length */
    BaseName.MaximumLength = BaseName.Length;

    /* Now compute the base directory */
    BaseDirectory = *FileName;
    BaseDirectory.Length -= BaseName.Length;
    BaseDirectory.MaximumLength = BaseDirectory.Length;

    /* And the prefix, which for now is just the name itself */
    PrefixName = *FileName;

    /* Check if we have a prefix */
    if (NamePrefix)
    {
        /* Check if "directory + prefix" is too long for the string */
        Status = RtlUShortAdd(BaseDirectory.Length,
                              NamePrefix->Length,
                              &PrefixName.MaximumLength);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        /* Check if "directory + prefix + basename" is too long for the string */
        Status = RtlUShortAdd(PrefixName.MaximumLength,
                              BaseName.Length,
                              &PrefixName.MaximumLength);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        /* Allocate the buffer exclusively used for prefixed name */
        PrefixedBuffer = ExAllocatePoolWithTag(PagedPool,
                                               PrefixName.MaximumLength,
                                               TAG_LDR_WSTR);
        if (!PrefixedBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        /* Clear out the prefixed name string */
        PrefixName.Buffer = PrefixedBuffer;
        PrefixName.Length = 0;

        /* Concatenate the strings */
        RtlAppendUnicodeStringToString(&PrefixName, &BaseDirectory);
        RtlAppendUnicodeStringToString(&PrefixName, NamePrefix);
        RtlAppendUnicodeStringToString(&PrefixName, &BaseName);

        /* Now the base name of the image becomes the prefixed version */
        BaseName.Buffer = &(PrefixName.Buffer[BaseDirectory.Length / sizeof(WCHAR)]);
        BaseName.Length += NamePrefix->Length;
        BaseName.MaximumLength = (PrefixName.MaximumLength - BaseDirectory.Length);
    }

    /* Check if we already have a name, use it instead */
    if (LoadedName) BaseName = *LoadedName;

    /* Check for loader snap debugging */
    if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS)
    {
        /* Print out standard string */
        DPRINT1("MM:SYSLDR Loading %wZ (%wZ) %s\n",
                &PrefixName, &BaseName, Flags ? "in session space" : "");
    }

    /* Acquire the load lock */
LoaderScan:
    ASSERT(LockOwned == FALSE);
    LockOwned = TRUE;
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Scan the module list */
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList)
    {
        /* Get the entry and compare the names */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);
        if (RtlEqualUnicodeString(&PrefixName, &LdrEntry->FullDllName, TRUE))
        {
            /* Found it, break out */
            break;
        }

        /* Keep scanning */
        NextEntry = NextEntry->Flink;
    }

    /* Check if we found the image */
    if (NextEntry != &PsLoadedModuleList)
    {
        /* Check if we had already mapped a section */
        if (Section)
        {
            /* Dereference and clear */
            ObDereferenceObject(Section);
            Section = NULL;
        }

        /* Check if this was supposed to be a session load */
        if (!Flags)
        {
            /* It wasn't, so just return the data */
            *ModuleObject = LdrEntry;
            *ImageBaseAddress = LdrEntry->DllBase;
            Status = STATUS_IMAGE_ALREADY_LOADED;
        }
        else
        {
            /* We don't support session loading yet */
            UNIMPLEMENTED_DBGBREAK("Unsupported Session-Load!\n");
            Status = STATUS_NOT_IMPLEMENTED;
        }

        /* Do cleanup */
        goto Quickie;
    }
    else if (!Section)
    {
        /* It wasn't loaded, and we didn't have a previous attempt */
        KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);
        KeLeaveCriticalRegion();
        LockOwned = FALSE;

        /* Check if KD is enabled */
        if ((KdDebuggerEnabled) && !(KdDebuggerNotPresent))
        {
            /* FIXME: Attempt to get image from KD */
        }

        /* We don't have a valid entry */
        LdrEntry = NULL;

        /* Setup image attributes */
        InitializeObjectAttributes(&ObjectAttributes,
                                   FileName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        /* Open the image */
        Status = ZwOpenFile(&FileHandle,
                            FILE_EXECUTE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwOpenFile failed for '%wZ' with status 0x%x\n",
                    FileName, Status);
            goto Quickie;
        }

        /* Validate it */
        Status = MmCheckSystemImage(FileHandle);
        if ((Status == STATUS_IMAGE_CHECKSUM_MISMATCH) ||
            (Status == STATUS_IMAGE_MP_UP_MISMATCH) ||
            (Status == STATUS_INVALID_IMAGE_PROTECT))
        {
            /* Fail loading */
            goto Quickie;
        }

        /* Check if this is a session-load */
        if (Flags)
        {
            /* Then we only need read and execute */
            DesiredAccess = SECTION_MAP_READ | SECTION_MAP_EXECUTE;
        }
        else
        {
            /* Otherwise, we can allow write access */
            DesiredAccess = SECTION_ALL_ACCESS;
        }

        /* Initialize the attributes for the section */
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        /* Create the section */
        Status = ZwCreateSection(&SectionHandle,
                                 DesiredAccess,
                                 &ObjectAttributes,
                                 NULL,
                                 PAGE_EXECUTE,
                                 SEC_IMAGE,
                                 FileHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwCreateSection failed with status 0x%x\n", Status);
            goto Quickie;
        }

        /* Now get the section pointer */
        Status = ObReferenceObjectByHandle(SectionHandle,
                                           SECTION_MAP_EXECUTE,
                                           MmSectionObjectType,
                                           KernelMode,
                                           (PVOID*)&Section,
                                           NULL);
        ZwClose(SectionHandle);
        if (!NT_SUCCESS(Status)) goto Quickie;

        /* Check if this was supposed to be a session-load */
        if (Flags)
        {
            /* We don't support session loading yet */
            UNIMPLEMENTED_DBGBREAK("Unsupported Session-Load!\n");
            goto Quickie;
        }

        /* Check the loader list again, we should end up in the path below */
        goto LoaderScan;
    }
    else
    {
        /* We don't have a valid entry */
        LdrEntry = NULL;
    }

    /* Load the image */
    Status = MiLoadImageSection(&Section,
                                &ModuleLoadBase,
                                FileName,
                                FALSE,
                                NULL);
    ASSERT(Status != STATUS_ALREADY_COMMITTED);

    /* Get the size of the driver */
    DriverSize = ((PMM_IMAGE_SECTION_OBJECT)Section->Segment)->ImageInformation.ImageFileSize;

    /* Make sure we're not being loaded into session space */
    if (!Flags)
    {
        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Support large pages for drivers */
            MiUseLargeDriverPage(DriverSize / PAGE_SIZE,
                                 &ModuleLoadBase,
                                 &BaseName,
                                 TRUE);
        }

        /* Dereference the section */
        ObDereferenceObject(Section);
        Section = NULL;
    }

    /* Check for failure of the load earlier */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MiLoadImageSection failed with status 0x%x\n", Status);
        goto Quickie;
    }

    /* Relocate the driver */
    Status = LdrRelocateImageWithBias(ModuleLoadBase,
                                      0,
                                      "SYSLDR",
                                      STATUS_SUCCESS,
                                      STATUS_CONFLICTING_ADDRESSES,
                                      STATUS_INVALID_IMAGE_FORMAT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LdrRelocateImageWithBias failed with status 0x%x\n", Status);
        goto Quickie;
    }

    /* Get the NT Header */
    NtHeader = RtlImageNtHeader(ModuleLoadBase);

    /* Calculate the size we'll need for the entry and allocate it */
    EntrySize = sizeof(LDR_DATA_TABLE_ENTRY) +
                BaseName.Length +
                sizeof(UNICODE_NULL);

    /* Allocate the entry */
    LdrEntry = ExAllocatePoolWithTag(NonPagedPool, EntrySize, TAG_MODULE_OBJECT);
    if (!LdrEntry)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Setup the entry */
    LdrEntry->Flags = LDRP_LOAD_IN_PROGRESS;
    LdrEntry->LoadCount = 1;
    LdrEntry->LoadedImports = LoadedImports;
    LdrEntry->PatchInformation = NULL;

    /* Check the version */
    if ((NtHeader->OptionalHeader.MajorOperatingSystemVersion >= 5) &&
        (NtHeader->OptionalHeader.MajorImageVersion >= 5))
    {
        /* Mark this image as a native image */
        LdrEntry->Flags |= LDRP_ENTRY_NATIVE;
    }

    /* Setup the rest of the entry */
    LdrEntry->DllBase = ModuleLoadBase;
    LdrEntry->EntryPoint = (PVOID)((ULONG_PTR)ModuleLoadBase +
                                   NtHeader->OptionalHeader.AddressOfEntryPoint);
    LdrEntry->SizeOfImage = DriverSize;
    LdrEntry->CheckSum = NtHeader->OptionalHeader.CheckSum;
    LdrEntry->SectionPointer = Section;

    /* Now write the DLL name */
    LdrEntry->BaseDllName.Buffer = (PVOID)(LdrEntry + 1);
    LdrEntry->BaseDllName.Length = BaseName.Length;
    LdrEntry->BaseDllName.MaximumLength = BaseName.Length;

    /* Copy and null-terminate it */
    RtlCopyMemory(LdrEntry->BaseDllName.Buffer,
                  BaseName.Buffer,
                  BaseName.Length);
    LdrEntry->BaseDllName.Buffer[BaseName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Now allocate the full name */
    LdrEntry->FullDllName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                         PrefixName.Length +
                                                         sizeof(UNICODE_NULL),
                                                         TAG_LDR_WSTR);
    if (!LdrEntry->FullDllName.Buffer)
    {
        /* Don't fail, just set it to zero */
        LdrEntry->FullDllName.Length = 0;
        LdrEntry->FullDllName.MaximumLength = 0;
    }
    else
    {
        /* Set it up */
        LdrEntry->FullDllName.Length = PrefixName.Length;
        LdrEntry->FullDllName.MaximumLength = PrefixName.Length;

        /* Copy and null-terminate */
        RtlCopyMemory(LdrEntry->FullDllName.Buffer,
                      PrefixName.Buffer,
                      PrefixName.Length);
        LdrEntry->FullDllName.Buffer[PrefixName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* Add the entry */
    MiProcessLoaderEntry(LdrEntry, TRUE);

    /* Resolve imports */
    MissingApiName = Buffer;
    MissingDriverName = NULL;
    Status = MiResolveImageReferences(ModuleLoadBase,
                                      &BaseDirectory,
                                      NULL,
                                      &MissingApiName,
                                      &MissingDriverName,
                                      &LoadedImports);
    if (!NT_SUCCESS(Status))
    {
        BOOLEAN NeedToFreeString = FALSE;

        /* If the lowest bit is set to 1, this is a hint that we need to free */
        if (*(ULONG_PTR*)&MissingDriverName & 1)
        {
            NeedToFreeString = TRUE;
            *(ULONG_PTR*)&MissingDriverName &= ~1;
        }

        DPRINT1("MiResolveImageReferences failed with status 0x%x\n", Status);
        DPRINT1(" Missing driver '%ls', missing API '%s'\n",
                MissingDriverName, MissingApiName);

        if (NeedToFreeString)
        {
            ExFreePoolWithTag(MissingDriverName, TAG_LDR_WSTR);
        }

        /* Fail */
        MiProcessLoaderEntry(LdrEntry, FALSE);

        /* Check if we need to free the name */
        if (LdrEntry->FullDllName.Buffer)
        {
            /* Free it */
            ExFreePoolWithTag(LdrEntry->FullDllName.Buffer, TAG_LDR_WSTR);
        }

        /* Free the entry itself */
        ExFreePoolWithTag(LdrEntry, TAG_MODULE_OBJECT);
        LdrEntry = NULL;
        goto Quickie;
    }

    /* Update the loader entry */
    LdrEntry->Flags |= (LDRP_SYSTEM_MAPPED |
                        LDRP_ENTRY_PROCESSED |
                        LDRP_MM_LOADED);
    LdrEntry->Flags &= ~LDRP_LOAD_IN_PROGRESS;
    LdrEntry->LoadedImports = LoadedImports;

    /* FIXME: Call driver verifier's loader function */

    /* Write-protect the system image */
    MiWriteProtectSystemImage(LdrEntry->DllBase);

    /* Initialize the security cookie (Win7 is not doing this yet!) */
    LdrpInitSecurityCookie(LdrEntry);

    /* Check if notifications are enabled */
    if (PsImageNotifyEnabled)
    {
        /* Fill out the notification data */
        ImageInfo.Properties = 0;
        ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        ImageInfo.SystemModeImage = TRUE;
        ImageInfo.ImageSize = LdrEntry->SizeOfImage;
        ImageInfo.ImageBase = LdrEntry->DllBase;
        ImageInfo.ImageSectionNumber = ImageInfo.ImageSelector = 0;

        /* Send the notification */
        PspRunLoadImageNotifyRoutines(FileName, NULL, &ImageInfo);
    }

#ifdef __ROS_ROSSYM__
    /* MiCacheImageSymbols doesn't detect rossym */
    if (TRUE)
#else
    /* Check if there's symbols */
    if (MiCacheImageSymbols(LdrEntry->DllBase))
#endif
    {
        UNICODE_STRING UnicodeTemp;
        STRING AnsiTemp;

        /* Check if the system root is present */
        if ((PrefixName.Length > (11 * sizeof(WCHAR))) &&
            !(_wcsnicmp(PrefixName.Buffer, L"\\SystemRoot", 11)))
        {
            /* Add the system root */
            UnicodeTemp = PrefixName;
            UnicodeTemp.Buffer += 11;
            UnicodeTemp.Length -= (11 * sizeof(WCHAR));
            RtlStringCbPrintfA(Buffer,
                               MAXIMUM_FILENAME_LENGTH,
                               "%ws%wZ",
                               &SharedUserData->NtSystemRoot[2],
                               &UnicodeTemp);
        }
        else
        {
            /* Build the name */
            RtlStringCbPrintfA(Buffer, MAXIMUM_FILENAME_LENGTH,
                               "%wZ", &BaseName);
        }

        /* Setup the ANSI string */
        RtlInitString(&AnsiTemp, Buffer);

        /* Notify the debugger */
        DbgLoadImageSymbols(&AnsiTemp,
                            LdrEntry->DllBase,
                            (ULONG_PTR)PsGetCurrentProcessId());
        LdrEntry->Flags |= LDRP_DEBUG_SYMBOLS_LOADED;
    }

    /* Page the driver */
    ASSERT(Section == NULL);
    MiEnablePagingOfDriver(LdrEntry);

    /* Return pointers */
    *ModuleObject = LdrEntry;
    *ImageBaseAddress = LdrEntry->DllBase;

Quickie:
    /* Check if we have the lock acquired */
    if (LockOwned)
    {
        /* Release the lock */
        KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);
        KeLeaveCriticalRegion();
        LockOwned = FALSE;
    }

    /* If we have a file handle, close it */
    if (FileHandle) ZwClose(FileHandle);

    /* If we have allocated a prefixed name buffer, free it */
    if (PrefixedBuffer) ExFreePoolWithTag(PrefixedBuffer, TAG_LDR_WSTR);

    /* Free the name buffer and return status */
    ExFreePoolWithTag(Buffer, TAG_LDR_WSTR);
    return Status;
}

PLDR_DATA_TABLE_ENTRY
NTAPI
MiLookupDataTableEntry(IN PVOID Address)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry, FoundEntry = NULL;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Loop entries */
    NextEntry = PsLoadedModuleList.Flink;
    do
    {
        /* Get the loader entry */
        LdrEntry =  CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

        /* Check if the address matches */
        if ((Address >= LdrEntry->DllBase) &&
            (Address < (PVOID)((ULONG_PTR)LdrEntry->DllBase +
                               LdrEntry->SizeOfImage)))
        {
            /* Found a match */
            FoundEntry = LdrEntry;
            break;
        }

        /* Move on */
        NextEntry = NextEntry->Flink;
    } while(NextEntry != &PsLoadedModuleList);

    /* Return the entry */
    return FoundEntry;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
MmPageEntireDriver(IN PVOID AddressWithinSection)
{
    PMMPTE StartPte, EndPte;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PAGED_CODE();

    /* Get the loader entry */
    LdrEntry = MiLookupDataTableEntry(AddressWithinSection);
    if (!LdrEntry) return NULL;

    /* Check if paging of kernel mode is disabled or if the driver is mapped as an image */
    if ((MmDisablePagingExecutive) || (LdrEntry->SectionPointer))
    {
        /* Don't do anything, just return the base address */
        return LdrEntry->DllBase;
    }

    /* Wait for active DPCs to finish before we page out the driver */
    KeFlushQueuedDpcs();

    /* Get the PTE range for the whole driver image */
    StartPte = MiAddressToPte((ULONG_PTR)LdrEntry->DllBase);
    EndPte = MiAddressToPte((ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage);

    /* Enable paging for the PTE range */
    ASSERT(MI_IS_SESSION_IMAGE_ADDRESS(AddressWithinSection) == FALSE);
    MiSetPagingOfDriver(StartPte, EndPte);

    /* Return the base address */
    return LdrEntry->DllBase;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmResetDriverPaging(IN PVOID AddressWithinSection)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
PVOID
NTAPI
MmGetSystemRoutineAddress(IN PUNICODE_STRING SystemRoutineName)
{
    PVOID ProcAddress = NULL;
    ANSI_STRING AnsiRoutineName;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    BOOLEAN Found = FALSE;
    UNICODE_STRING KernelName = RTL_CONSTANT_STRING(L"ntoskrnl.exe");
    UNICODE_STRING HalName = RTL_CONSTANT_STRING(L"hal.dll");
    ULONG Modules = 0;

    /* Convert routine to ANSI name */
    Status = RtlUnicodeStringToAnsiString(&AnsiRoutineName,
                                          SystemRoutineName,
                                          TRUE);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Lock the list */
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PsLoadedModuleResource, TRUE);

    /* Loop the loaded module list */
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Check if it's the kernel or HAL */
        if (RtlEqualUnicodeString(&KernelName, &LdrEntry->BaseDllName, TRUE))
        {
            /* Found it */
            Found = TRUE;
            Modules++;
        }
        else if (RtlEqualUnicodeString(&HalName, &LdrEntry->BaseDllName, TRUE))
        {
            /* Found it */
            Found = TRUE;
            Modules++;
        }

        /* Check if we found a valid binary */
        if (Found)
        {
            /* Find the procedure name */
            ProcAddress = RtlFindExportedRoutineByName(LdrEntry->DllBase,
                                                       AnsiRoutineName.Buffer);

            /* Break out if we found it or if we already tried both modules */
            if (ProcAddress) break;
            if (Modules == 2) break;
        }

        /* Keep looping */
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock */
    ExReleaseResourceLite(&PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    /* Free the string and return */
    RtlFreeAnsiString(&AnsiRoutineName);
    return ProcAddress;
}

/* EOF */

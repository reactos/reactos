/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/mm/sysldr.c
* PURPOSE:         Contains the Kernel Loader (SYSLDR) for loading PE files.
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GCC's incompetence strikes again */
VOID
sprintf_nt(IN PCHAR Buffer,
           IN PCHAR Format,
           IN ...)
{
    va_list ap;
    va_start(ap, Format);
    vsprintf(Buffer, Format, ap);
    va_end(ap);
}

/* GLOBALS *******************************************************************/

LIST_ENTRY PsLoadedModuleList;
KSPIN_LOCK PsLoadedModuleSpinLock;
ULONG_PTR PsNtosImageBase;
KMUTANT MmSystemLoadLock;
extern ULONG NtGlobalFlag;

/* FUNCTIONS *****************************************************************/

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

VOID
NTAPI
MiFreeBootDriverMemory(PVOID BaseAddress,
                       ULONG Length)
{
    ULONG i;

    /* Loop each page */
    for (i = 0; i < PAGE_ROUND_UP(Length) / PAGE_SIZE; i++)
    {
        /* Free the page */
        MmDeleteVirtualMapping(NULL,
                               (PVOID)((ULONG_PTR)BaseAddress + i * PAGE_SIZE),
                               TRUE,
                               NULL,
                               NULL);
    }
}

NTSTATUS
NTAPI
MiLoadImageSection(IN OUT PVOID *SectionPtr,
                   OUT PVOID *ImageBase,
                   IN PUNICODE_STRING FileName,
                   IN BOOLEAN SessionLoad,
                   IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PROS_SECTION_OBJECT Section = *SectionPtr;
    NTSTATUS Status;
    PEPROCESS Process;
    PVOID Base = NULL;
    SIZE_T ViewSize = 0;
    KAPC_STATE ApcState;
    LARGE_INTEGER SectionOffset = {{0, 0}};
    BOOLEAN LoadSymbols = FALSE;
    ULONG DriverSize;
    PVOID DriverBase;
    PAGED_CODE();

    /* Detect session load */
    if (SessionLoad)
    {
        /* Fail */
        DPRINT1("Session loading not yet supported!\n");
        while (TRUE);
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
        KeUnstackDetachProcess(&ApcState);
        return Status;
    }

    /* Get the driver size */
    DriverSize = Section->ImageSection->ImageSize;

    /*  Allocate a virtual section for the module  */
    DriverBase = MmAllocateSection(DriverSize, NULL);
    *ImageBase = DriverBase;

    /* Copy the image */
    RtlCopyMemory(DriverBase, Base, DriverSize);

    /* Now unmap the view */
    Status = MmUnmapViewOfSection(Process, Base);
    ASSERT(NT_SUCCESS(Status));

    /* Detach and return status */
    KeUnstackDetachProcess(&ApcState);
    return Status;
}

NTSTATUS
NTAPI
MiDereferenceImports(IN PLOAD_IMPORTS ImportList)
{
    SIZE_T i;

    /* Check if there's no imports or if we're a boot driver */
    if ((ImportList == (PVOID)-1) ||
        (ImportList == (PVOID)-2) ||
        (ImportList->Count == 0))
    {
        /* Then there's nothing to do */
        return STATUS_SUCCESS;
    }

    /* Otherwise, FIXME */
    DPRINT1("%u imports not dereferenced!\n", ImportList->Count);
    for (i = 0; i < ImportList->Count; i++)
    {
        DPRINT1("%wZ <%wZ>\n", &ImportList->Entry[i]->FullDllName, &ImportList->Entry[i]->BaseDllName);
    }
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
MiClearImports(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PAGED_CODE();

    /* Check if there's no imports or we're a boot driver or only one entry */
    if ((LdrEntry->LoadedImports == (PVOID)-1) ||
        (LdrEntry->LoadedImports == (PVOID)-2) ||
        ((ULONG_PTR)LdrEntry->LoadedImports & 1))
    {
        /* Nothing to do */
        return;
    }

    /* Otherwise, free the import list */
    ExFreePool(LdrEntry->LoadedImports);
}

PVOID
NTAPI
MiFindExportedRoutineByName(IN PVOID DllBase,
                            IN PANSI_STRING ExportName)
{
    PULONG NameTable;
    PUSHORT OrdinalTable;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    LONG Low = 0, Mid = 0, High, Ret;
    USHORT Ordinal;
    PVOID Function;
    ULONG ExportSize;
    PULONG ExportTable;
    PAGED_CODE();

    /* Get the export directory */
    ExportDirectory = RtlImageDirectoryEntryToData(DllBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                   &ExportSize);
    if (!ExportDirectory) return NULL;

    /* Setup name tables */
    NameTable = (PULONG)((ULONG_PTR)DllBase +
                         ExportDirectory->AddressOfNames);
    OrdinalTable = (PUSHORT)((ULONG_PTR)DllBase +
                             ExportDirectory->AddressOfNameOrdinals);

    /* Do a binary search */
    High = ExportDirectory->NumberOfNames - 1;
    while (High >= Low)
    {
        /* Get new middle value */
        Mid = (Low + High) >> 1;

        /* Compare name */
        Ret = strcmp(ExportName->Buffer, (PCHAR)DllBase + NameTable[Mid]);
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
    if (High < Low) return NULL;

    /* Otherwise, this is the ordinal */
    Ordinal = OrdinalTable[Mid];

    /* Resolve the address and write it */
    ExportTable = (PULONG)((ULONG_PTR)DllBase +
                           ExportDirectory->AddressOfFunctions);
    Function = (PVOID)((ULONG_PTR)DllBase + ExportTable[Ordinal]);

    /* We found it! */
    ASSERT(!(Function > (PVOID)ExportDirectory) &&
           (Function < (PVOID)((ULONG_PTR)ExportDirectory + ExportSize)));
    return Function;
}

PVOID
NTAPI
MiLocateExportName(IN PVOID DllBase,
                   IN PCHAR ExportName)
{
    PULONG NameTable;
    PUSHORT OrdinalTable;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    LONG Low = 0, Mid = 0, High, Ret;
    USHORT Ordinal;
    PVOID Function;
    ULONG ExportSize;
    PULONG ExportTable;
    PAGED_CODE();

    /* Get the export directory */
    ExportDirectory = RtlImageDirectoryEntryToData(DllBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                   &ExportSize);
    if (!ExportDirectory) return NULL;

    /* Setup name tables */
    NameTable = (PULONG)((ULONG_PTR)DllBase +
                         ExportDirectory->AddressOfNames);
    OrdinalTable = (PUSHORT)((ULONG_PTR)DllBase +
                             ExportDirectory->AddressOfNameOrdinals);

    /* Do a binary search */
    High = ExportDirectory->NumberOfNames - 1;
    while (High >= Low)
    {
        /* Get new middle value */
        Mid = (Low + High) >> 1;

        /* Compare name */
        Ret = strcmp(ExportName, (PCHAR)DllBase + NameTable[Mid]);
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
    if (High < Low) return NULL;

    /* Otherwise, this is the ordinal */
    Ordinal = OrdinalTable[Mid];

    /* Resolve the address and write it */
    ExportTable = (PULONG)((ULONG_PTR)DllBase +
                           ExportDirectory->AddressOfFunctions);
    Function = (PVOID)((ULONG_PTR)DllBase + ExportTable[Ordinal]);

    /* Check if the function is actually a forwarder */
    if (((ULONG_PTR)Function > (ULONG_PTR)ExportDirectory) &&
        ((ULONG_PTR)Function < ((ULONG_PTR)ExportDirectory + ExportSize)))
    {
        /* It is, fail */
        return NULL;
    }

    /* We found it */
    return Function;
}

NTSTATUS
NTAPI
MmCallDllInitialize(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                    IN PLIST_ENTRY ListHead)
{
    UNICODE_STRING ServicesKeyName = RTL_CONSTANT_STRING(
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    PMM_DLL_INITIALIZE DllInit;
    UNICODE_STRING RegPath, ImportName;
    NTSTATUS Status;

    /* Try to see if the image exports a DllInitialize routine */
    DllInit = (PMM_DLL_INITIALIZE)MiLocateExportName(LdrEntry->DllBase,
                                                     "DllInitialize");
    if (!DllInit) return STATUS_SUCCESS;

    /* Do a temporary copy of BaseDllName called ImportName
     * because we'll alter the length of the string
     */
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
    if (!RegPath.Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    /* Build and append the service name itself */
    RegPath.Length = ServicesKeyName.Length;
    RtlCopyMemory(RegPath.Buffer,
                  ServicesKeyName.Buffer,
                  ServicesKeyName.Length);

    /* Check if there is a dot in the filename */
    if (wcschr(ImportName.Buffer, L'.'))
    {
        /* Remove the extension */
        ImportName.Length = (wcschr(ImportName.Buffer, L'.') -
            ImportName.Buffer) * sizeof(WCHAR);
    }

    /* Append service name (the basename without extension) */
    RtlAppendUnicodeStringToString(&RegPath, &ImportName);

    /* Now call the DllInit func */
    DPRINT("Calling DllInit(%wZ)\n", &RegPath);
    Status = DllInit(&RegPath);

    /* Clean up */
    ExFreePool(RegPath.Buffer);

    /* Return status value which DllInitialize returned */
    return Status;
}

VOID
NTAPI
MiProcessLoaderEntry(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                     IN BOOLEAN Insert)
{
    KIRQL OldIrql;

    /* Acquire the lock */
    KeAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);

    /* Insert or remove from the list */
    Insert ? InsertTailList(&PsLoadedModuleList, &LdrEntry->InLoadOrderLinks) :
             RemoveEntryList(&LdrEntry->InLoadOrderLinks);

    /* Release the lock */
    KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
}

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
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PULONG ImageThunk;

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
    ULONG Low = 0, Mid = 0, High;
    LONG Ret;
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
    ULONG ForwardLength;
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
        strncpy(*MissingApi,
                (PCHAR)&NameImport->Name[0],
                MAXIMUM_FILENAME_LENGTH - 1);

        /* Setup name tables */
        DPRINT("Import name: %s\n", NameImport->Name);
        NameTable = (PULONG)((ULONG_PTR)DllBase +
                             ExportDirectory->AddressOfNames);
        OrdinalTable = (PUSHORT)((ULONG_PTR)DllBase +
                                 ExportDirectory->AddressOfNameOrdinals);

        /* Get the hint and check if it's valid */
        Hint = NameImport->Hint;
        if ((Hint < ExportDirectory->NumberOfNames) &&
            !(strcmp((PCHAR) NameImport->Name, (PCHAR)DllBase + NameTable[Hint])))
        {
            /* We have a match, get the ordinal number from here */
            Ordinal = OrdinalTable[Hint];
        }
        else
        {
            /* Do a binary search */
            High = ExportDirectory->NumberOfNames - 1;
            while (High >= Low)
            {
                /* Get new middle value */
                Mid = (Low + High) >> 1;

                /* Compare name */
                Ret = strcmp((PCHAR)NameImport->Name, (PCHAR)DllBase + NameTable[Mid]);
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
            if (High < Low) return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

            /* Otherwise, this is the ordinal */
            Ordinal = OrdinalTable[Mid];
        }
    }

    /* Check if the ordinal is invalid */
    if (Ordinal >= ExportDirectory->NumberOfFunctions)
    {
        /* Fail */
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
            (Address->u1.Function < ((ULONG_PTR)ExportDirectory + ExportSize)))
        {
            /* Now assume failure in case the forwarder doesn't exist */
            Status = STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

            /* Build the forwarder name */
            DllName.Buffer = (PCHAR)Address->u1.Function;
            DllName.Length = strchr(DllName.Buffer, '.') -
                             DllName.Buffer +
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
                if (RtlPrefixString((PSTRING)&ForwarderName,
                                    (PSTRING)&LdrEntry->BaseDllName,
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
    ANSI_STRING TempName;
    BOOLEAN HadEntry = FALSE;

    /* Acquire the loader lock */
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Check if this driver was loaded at boot and didn't get imports parsed */
    if (LdrEntry->LoadedImports == (PVOID)-1) goto Done;

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
            DbgUnLoadImageSymbols(&TempName, BaseAddress, -1);
            RtlFreeAnsiString(&TempName);
        }
    }

    /* FIXME: Free the driver */
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
            ExFreePool(LdrEntry->FullDllName.Buffer);
        }

        /* Check if we had a section */
        if (LdrEntry->SectionPointer)
        {
            /* Dereference it */
            ObDereferenceObject(LdrEntry->SectionPointer);
        }

        /* Free the entry */
        ExFreePool(LdrEntry);
    }

    /* Release the system lock and return */
Done:
    KeReleaseMutant(&MmSystemLoadLock, 1, FALSE, FALSE);
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
    PCHAR MissingApiBuffer = *MissingApi, ImportName;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor, CurrentImport;
    ULONG ImportSize, ImportCount = 0, LoadedImportsSize, ExportSize;
    PLOAD_IMPORTS LoadedImports, NewImports;
    ULONG GdiLink, NormalLink, i;
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

    /* Assume no imports */
    *LoadImports = (PVOID)-2;

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
                                              TAG_LDR_WSTR);
        if (LoadedImports)
        {
            /* Zero it */
            RtlZeroMemory(LoadedImports, LoadedImportsSize);
        }
    }
    else
    {
        /* No table */
        LoadedImports = NULL;
    }

    /* Reset the import count and loop descriptors again */
    ImportCount = GdiLink = NormalLink = 0;
    while ((ImportDescriptor->Name) && (ImportDescriptor->OriginalFirstThunk))
    {
        /* Get the name */
        ImportName = (PCHAR)((ULONG_PTR)ImageBase + ImportDescriptor->Name);

        /* Check if this is a GDI driver */
        GdiLink = GdiLink |
                  !(_strnicmp(ImportName, "win32k", sizeof("win32k") - 1));

        /* We can also allow dxapi */
        NormalLink = NormalLink |
                     ((_strnicmp(ImportName, "win32k", sizeof("win32k") - 1)) &&
                      (_strnicmp(ImportName, "dxapi", sizeof("dxapi") - 1)));

        /* Check if this is a valid GDI driver */
        if ((GdiLink) && (NormalLink))
        {
            /* It's not, it's importing stuff it shouldn't be! */
            MiDereferenceImports(LoadedImports);
            if (LoadedImports) ExFreePool(LoadedImports);
            return STATUS_PROCEDURE_NOT_FOUND;
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
            MiDereferenceImports(LoadedImports);
            if (LoadedImports) ExFreePoolWithTag(LoadedImports, TAG_LDR_WSTR);
            return Status;
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
            if (DllName.Buffer)
            {
                /* Setup the base length and copy it */
                DllName.Length = ImageFileDirectory->Length;
                RtlCopyMemory(DllName.Buffer,
                              ImageFileDirectory->Buffer,
                              ImageFileDirectory->Length);

                /* Now add the import name and null-terminate it */
                RtlAppendStringToString((PSTRING)&DllName,
                                        (PSTRING)&NameString);
                DllName.Buffer[(DllName.MaximumLength - 1) / 2] = UNICODE_NULL;

                /* Load the image */
                Status = MmLoadSystemImage(&DllName,
                                           NamePrefix,
                                           NULL,
                                           0,
                                           (PVOID)&DllEntry,
                                           &DllBase);
                if (NT_SUCCESS(Status))
                {
                    /* We can free the DLL Name */
                    ExFreePool(DllName.Buffer);
                }
                else
                {
                    /* Fill out the information for the error */
                    *MissingDriver = DllName.Buffer;
                    *(PULONG)MissingDriver |= 1;
                    *MissingApi = NULL;
                }
            }
            else
            {
                /* We're out of resources */
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Check if we're OK until now */
            if (NT_SUCCESS(Status))
            {
                /* We're now loaded */
                Loaded = TRUE;

                /* Sanity check */
                ASSERT(DllBase = DllEntry->DllBase);

                /* Call the initialization routines */
                Status = MmCallDllInitialize(DllEntry, &PsLoadedModuleList);
                if (!NT_SUCCESS(Status))
                {
                    /* We failed, unload the image */
                    MmUnloadSystemImage(DllEntry);
                    while (TRUE);
                    Loaded = FALSE;
                }
            }

            /* Check if we failed by here */
            if (!NT_SUCCESS(Status))
            {
                /* Cleanup and return */
                RtlFreeUnicodeString(&NameString);
                MiDereferenceImports(LoadedImports);
                if (LoadedImports) ExFreePoolWithTag(LoadedImports, TAG_LDR_WSTR);
                return Status;
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
                LoadedImports->Entry[LoadedImports->Count] = LdrEntry;
                LoadedImports->Count++;
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
            MiDereferenceImports(LoadedImports);
            if (LoadedImports) ExFreePoolWithTag(LoadedImports, TAG_LDR_WSTR);
            return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
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
                    MiDereferenceImports(LoadedImports);
                    if (LoadedImports) ExFreePoolWithTag(LoadedImports, TAG_LDR_WSTR);
                    return Status;
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
        /* Reset the count again, and loop entries*/
        ImportCount = 0;
        for (i = 0; i < LoadedImports->Count; i++)
        {
            if (LoadedImports->Entry[i])
            {
                /* Got an entry, OR it with 1 in case it's the single entry */
                ImportEntry = (PVOID)((ULONG_PTR)LoadedImports->Entry[i] | 1);
                ImportCount++;
            }
        }

        /* Check if we had no imports */
        if (!ImportCount)
        {
            /* Free the list and set it to no imports */
            ExFreePoolWithTag(LoadedImports, TAG_LDR_WSTR);
            LoadedImports = (PVOID)-2;
        }
        else if (ImportCount == 1)
        {
            /* Just one entry, we can free the table and only use our entry */
            ExFreePoolWithTag(LoadedImports, TAG_LDR_WSTR);
            LoadedImports = (PLOAD_IMPORTS)ImportEntry;
        }
        else if (ImportCount != LoadedImports->Count)
        {
            /* Allocate a new list */
            LoadedImportsSize = ImportCount * sizeof(PVOID) + sizeof(SIZE_T);
            NewImports = ExAllocatePoolWithTag(PagedPool,
                                               LoadedImportsSize,
                                               TAG_LDR_WSTR);
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
                ExFreePoolWithTag(LoadedImports, TAG_LDR_WSTR);
                LoadedImports = NewImports;
            }
        }

        /* Return the list */
        *LoadImports = LoadedImports;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

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
                (ULONG_PTR)LdrEntry->DllBase+ LdrEntry->SizeOfImage,
                &LdrEntry->FullDllName);

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

        /*  Allocate a virtual section for the module  */
        NewImageAddress = MmAllocateSection(LdrEntry->SizeOfImage, NULL);
        if (!NewImageAddress)
        {
            /* Shouldn't happen */
            DPRINT1("[Mm0]: Couldn't allocate driver section!\n");
            while (TRUE);
        }

        /* Sanity check */
        DPRINT("[Mm0]: Copying from: %p to: %p\n", DllBase, NewImageAddress);
        ASSERT(ExpInitializationPhase == 0);

        /* Now copy the entire driver over */
        RtlCopyMemory(NewImageAddress, DllBase, LdrEntry->SizeOfImage);

        /* Sanity check */
        ASSERT(*(PULONG)NewImageAddress == *(PULONG)DllBase);

        /* Set the image base to the address where the loader put it */
        NtHeader->OptionalHeader.ImageBase = (ULONG_PTR)DllBase;
        NtHeader = RtlImageNtHeader(NewImageAddress);
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
                DPRINT1("Relocations failed!\n");
                while (TRUE);
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
        LdrEntry->Flags |= 0x01000000;
        LdrEntry->EntryPoint = (PVOID)((ULONG_PTR)NewImageAddress +
                                NtHeader->OptionalHeader.AddressOfEntryPoint);
        LdrEntry->SizeOfImage = LdrEntry->SizeOfImage;

        /* Free the old copy */
        MiFreeBootDriverMemory(DllBase, LdrEntry->SizeOfImage);
    }
}

BOOLEAN
NTAPI
MiInitializeLoadedModuleList(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry, NewEntry;
    PLIST_ENTRY ListHead, NextEntry;
    ULONG EntrySize;

    /* Setup the loaded module list and lock */
    KeInitializeSpinLock(&PsLoadedModuleSpinLock);
    InitializeListHead(&PsLoadedModuleList);

    /* Get loop variables and the kernel entry */
    ListHead = &LoaderBlock->LoadOrderListHead;
    NextEntry = ListHead->Flink;
    LdrEntry = CONTAINING_RECORD(NextEntry,
                                 LDR_DATA_TABLE_ENTRY,
                                 InLoadOrderLinks);
    PsNtosImageBase = (ULONG_PTR)LdrEntry->DllBase;

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
            NextEntry= NextEntry->Flink;
            continue;
        }

        /* Calculate the size we'll need and allocate a copy */
        EntrySize = sizeof(LDR_DATA_TABLE_ENTRY) +
                    LdrEntry->BaseDllName.MaximumLength +
                    sizeof(UNICODE_NULL);
        NewEntry = ExAllocatePoolWithTag(NonPagedPool, EntrySize, TAG_LDR_WSTR);
        if (!NewEntry) return FALSE;

        /* Copy the entry over */
        *NewEntry = *LdrEntry;

        /* Allocate the name */
        NewEntry->FullDllName.Buffer =
            ExAllocatePoolWithTag(PagedPool,
                                  LdrEntry->FullDllName.MaximumLength +
                                  sizeof(UNICODE_NULL),
                                  TAG_LDR_WSTR);
        if (!NewEntry->FullDllName.Buffer) return FALSE;

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
    //MiBuildImportsForBootDrivers();

    /* We're done */
    return TRUE;
}

BOOLEAN
NTAPI
MmVerifyImageIsOkForMpUse(IN PVOID BaseAddress)
{
    PIMAGE_NT_HEADERS NtHeader;
    PAGED_CODE();

    /* Get NT Headers */
    NtHeader = RtlImageNtHeader(BaseAddress);
    if (NtHeader)
    {
        /* Check if this image is only safe for UP while we have 2+ CPUs */
        if ((KeNumberProcessors > 1) &&
            (NtHeader->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY))
        {
            /* Fail */
            return FALSE;
        }
    }

    /* Otherwise, it's safe */
    return TRUE;
}

NTSTATUS
NTAPI
MmCheckSystemImage(IN HANDLE ImageHandle,
                   IN BOOLEAN PurgeSection)
{
    NTSTATUS Status;
    HANDLE SectionHandle;
    PVOID ViewBase = NULL;
    SIZE_T ViewSize = 0;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    KAPC_STATE ApcState;
    PAGED_CODE();

    /* Create a section for the DLL */
    Status = ZwCreateSection(&SectionHandle,
                             SECTION_MAP_EXECUTE,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_COMMIT,
                             ImageHandle);
    if (!NT_SUCCESS(Status)) return Status;

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
    if ( NT_SUCCESS(Status) )
    {
        /* First, verify the checksum */
        if (!LdrVerifyMappedImageMatchesChecksum(ViewBase,
                                                 FileStandardInfo.
                                                 EndOfFile.LowPart,
                                                 FileStandardInfo.
                                                 EndOfFile.LowPart))
        {
            /* Set checksum failure */
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
        }

        /* Check that it's a valid SMP image if we have more then one CPU */
        if (!MmVerifyImageIsOkForMpUse(ViewBase))
        {
            /* Otherwise it's not the right image */
            Status = STATUS_IMAGE_MP_UP_MISMATCH;
        }
    }

    /* Unmap the section, close the handle, and return status */
    ZwUnmapViewOfSection(NtCurrentProcess(), ViewBase);
    KeUnstackDetachProcess(&ApcState);
    ZwClose(SectionHandle);
    return Status;
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
    UNICODE_STRING BaseName, BaseDirectory, PrefixName, UnicodeTemp;
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
    ULONG EntrySize, DriverSize;
    PLOAD_IMPORTS LoadedImports = (PVOID)-2;
    PCHAR MissingApiName, Buffer;
    PWCHAR MissingDriverName;
    HANDLE SectionHandle;
    ACCESS_MASK DesiredAccess;
    PVOID Section = NULL;
    BOOLEAN LockOwned = FALSE;
    PLIST_ENTRY NextEntry;
    IMAGE_INFO ImageInfo;
    ANSI_STRING AnsiTemp;
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

    if (ModuleObject) *ModuleObject = NULL;
    if (ImageBaseAddress) *ImageBaseAddress = NULL;

    /* Allocate a buffer we'll use for names */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, MAX_PATH, TAG_LDR_WSTR);
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
    if (NamePrefix) DPRINT1("Prefixed images are not yet supported!\n");

    /* Check if we already have a name, use it instead */
    if (LoadedName) BaseName = *LoadedName;

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
            if (ModuleObject) *ModuleObject = LdrEntry;
            if (ImageBaseAddress) *ImageBaseAddress = LdrEntry->DllBase;
            Status = STATUS_IMAGE_ALREADY_LOADED;
        }
        else
        {
            /* We don't support session loading yet */
            DPRINT1("Unsupported Session-Load!\n");
            while (TRUE);
        }

        /* Do cleanup */
        goto Quickie;
    }
    else if (!Section)
    {
        /* It wasn't loaded, and we didn't have a previous attempt */
        KeReleaseMutant(&MmSystemLoadLock, 1, FALSE, FALSE);
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
        if (!NT_SUCCESS(Status)) goto Quickie;

        /* Validate it */
        Status = MmCheckSystemImage(FileHandle, FALSE);
        if ((Status == STATUS_IMAGE_CHECKSUM_MISMATCH) ||
            (Status == STATUS_IMAGE_MP_UP_MISMATCH) ||
            (Status == STATUS_INVALID_IMAGE_FORMAT))
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
        if (!NT_SUCCESS(Status)) goto Quickie;

        /* Now get the section pointer */
        Status = ObReferenceObjectByHandle(SectionHandle,
                                           SECTION_MAP_EXECUTE,
                                           MmSectionObjectType,
                                           KernelMode,
                                           &Section,
                                           NULL);
        ZwClose(SectionHandle);
        if (!NT_SUCCESS(Status)) goto Quickie;

        /* Check if this was supposed to be a session-load */
        if (Flags)
        {
            /* We don't support session loading yet */
            DPRINT1("Unsupported Session-Load!\n");
            while (TRUE);
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
    DriverSize = ((PROS_SECTION_OBJECT)Section)->ImageSection->ImageSize;

    /* Make sure we're not being loaded into session space */
    if (!Flags)
    {
        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* FIXME: Support large pages for drivers */
        }

        /* Dereference the section */
        ObDereferenceObject(Section);
        Section = NULL;
    }

    /* Get the NT Header */
    NtHeader = RtlImageNtHeader(ModuleLoadBase);

    /* Relocate the driver */
    Status = LdrRelocateImageWithBias(ModuleLoadBase,
                                      0,
                                      "SYSLDR",
                                      STATUS_SUCCESS,
                                      STATUS_CONFLICTING_ADDRESSES,
                                      STATUS_INVALID_IMAGE_FORMAT);
    if (!NT_SUCCESS(Status)) goto Quickie;

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
        LdrEntry->Flags |= 0x80000000;
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
    LdrEntry->BaseDllName.Buffer[BaseName.Length / 2] = UNICODE_NULL;

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
        LdrEntry->FullDllName.Buffer[PrefixName.Length / 2] = UNICODE_NULL;
    }

    /* Add the entry */
    MiProcessLoaderEntry(LdrEntry, TRUE);

    /* Resolve imports */
    MissingApiName = Buffer;
    Status = MiResolveImageReferences(ModuleLoadBase,
                                      &BaseDirectory,
                                      NULL,
                                      &MissingApiName,
                                      &MissingDriverName,
                                      &LoadedImports);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        MiProcessLoaderEntry(LdrEntry, FALSE);

        /* Check if we need to free the name */
        if (LdrEntry->FullDllName.Buffer)
        {
            /* Free it */
            ExFreePool(LdrEntry->FullDllName.Buffer);
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

    /* FIXME: Apply driver verifier */

    /* FIXME: Write-protect the system image */

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

    /* Check if there's symbols */
#ifdef KDBG
    /* If KDBG is defined, then we always have symbols */
    if (TRUE)
#else
    if (MiCacheImageSymbols(LdrEntry->DllBase))
#endif
    {
        /* Check if the system root is present */
        if ((PrefixName.Length > (11 * sizeof(WCHAR))) &&
            !(_wcsnicmp(PrefixName.Buffer, L"\\SystemRoot", 11)))
        {
            /* Add the system root */
            UnicodeTemp = PrefixName;
            UnicodeTemp.Buffer += 11;
            UnicodeTemp.Length -= (11 * sizeof(WCHAR));
            sprintf_nt(Buffer,
                       "%ws%wZ",
                       &SharedUserData->NtSystemRoot[2],
                       &UnicodeTemp);
        }
        else
        {
            /* Build the name */
            sprintf_nt(Buffer, "%wZ", &BaseName);
        }

        /* Setup the ansi string */
        RtlInitString(&AnsiTemp, Buffer);

        /* Notify the debugger */
        DbgLoadImageSymbols(&AnsiTemp, LdrEntry->DllBase, -1);
        LdrEntry->Flags |= LDRP_DEBUG_SYMBOLS_LOADED;
    }

    /* FIXME: Page the driver */
    ASSERT(Section == NULL);

    /* Return pointers */
    if (ModuleObject) *ModuleObject = LdrEntry;
    if (ImageBaseAddress) *ImageBaseAddress = LdrEntry->DllBase;

Quickie:
    /* If we have a file handle, close it */
    if (FileHandle) ZwClose(FileHandle);

    /* Check if we have the lock acquired */
    if (LockOwned)
    {
        /* Release the lock */
        KeReleaseMutant(&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegion();
        LockOwned = FALSE;
    }

    /* Check if we had a prefix */
    if (NamePrefix) ExFreePool(PrefixName.Buffer);

    /* Free the name buffer and return status */
    ExFreePoolWithTag(Buffer, TAG_LDR_WSTR);
    return Status;
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
    extern LIST_ENTRY PsLoadedModuleList;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    BOOLEAN Found = FALSE;
    UNICODE_STRING KernelName = RTL_CONSTANT_STRING(L"ntoskrnl.exe");
    UNICODE_STRING HalName = RTL_CONSTANT_STRING(L"hal.dll");
    ULONG Modules = 0;

    /* Convert routine to ansi name */
    Status = RtlUnicodeStringToAnsiString(&AnsiRoutineName,
                                          SystemRoutineName,
                                          TRUE);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Lock the list */
    KeEnterCriticalRegion();

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
            ProcAddress = MiFindExportedRoutineByName(LdrEntry->DllBase,
                                                      &AnsiRoutineName);

            /* Break out if we found it or if we already tried both modules */
            if (ProcAddress) break;
            if (Modules == 2) break;
        }

        /* Keep looping */
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock */
    KeLeaveCriticalRegion();

    /* Free the string and return */
    RtlFreeAnsiString(&AnsiRoutineName);
    return ProcAddress;
}


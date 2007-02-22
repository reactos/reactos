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

/* GLOBALS *******************************************************************/

LIST_ENTRY PsLoadedModuleList;
KSPIN_LOCK PsLoadedModuleSpinLock;
PVOID PsNtosImageBase;
KMUTANT MmSystemLoadLock;

/* FUNCTIONS *****************************************************************/

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
                    *(PULONG)&ForwardThunk.u1.AddressOfData = (ULONG)ForwardName;

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
                    ExFreePool(ForwardName);
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
    PLOAD_IMPORTS LoadedImports;
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
            /* Zero it and set the count */
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
            DPRINT1("Invalid driver!\n");
            //MiDereferenceImports(LoadedImports);
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
            //MiDereferenceImports(LoadedImports);
            if (LoadedImports) ExFreePool(LoadedImports);
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
                    DPRINT1("Failed to import: %S\n", DllName.Buffer);
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
#if 0
                Status = MmCallDllInitialize(DllEntry, &PsLoadedModuleList);
                if (!NT_SUCCESS(Status))
                {
                    /* We failed, unload the image */
                    MmUnloadSystemImage(DllEntry);
                    while (TRUE);
                    Loaded = FALSE;
                }
#endif
            }

            /* Check if we failed by here */
            if (!NT_SUCCESS(Status))
            {
                /* Cleanup and return */
                DPRINT1("Failed loading import\n");
                RtlFreeUnicodeString(&NameString);
                //MiDereferenceImports(LoadedImports);
                if (LoadedImports) ExFreePool(LoadedImports);
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
            DPRINT1("Invalid driver: %wZ\n", &LdrEntry->BaseDllName);
            //MiDereferenceImports(LoadedImports);
            if (LoadedImports) ExFreePool(LoadedImports);
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
                    //MiDereferenceImports(LoadedImports);
                    if (LoadedImports) ExFreePool(LoadedImports);
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
            ExFreePool(LoadedImports);
            LoadedImports = (PVOID)-2;
        }
        else if (ImportCount == 1)
        {
            /* Just one entry, we can free the table and only use our entry */
            ExFreePool(LoadedImports);
            LoadedImports = (PLOAD_IMPORTS)ImportEntry;
        }
        else if (ImportCount != LoadedImports->Count)
        {
            /* FIXME: Can this happen? */
            DPRINT1("Unhandled scenario\n");
            while (TRUE);
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
    ULONG DriverSize = 0, Size;
    PIMAGE_SECTION_HEADER Section;

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

#if 1 // Disable for FreeLDR 2.5
        /*  Get header pointers  */
        Section = IMAGE_FIRST_SECTION(NtHeader);

        /*  Determine the size of the module  */
        for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
        {
            /* Skip this section if we're not supposed to load it */
            if (!(Section[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
            {
                /* Add the size of this section into the total size */
                Size = Section[i].VirtualAddress + Section[i].Misc.VirtualSize;
                DriverSize = max(DriverSize, Size);
            }
        }

        /* Round up the driver size to section alignment */
        DriverSize = ROUND_UP(DriverSize, NtHeader->OptionalHeader.SectionAlignment);
#endif

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
        NewImageAddress = MmAllocateSection(DriverSize, NULL);
        if (!NewImageAddress)
        {
            /* Shouldn't happen */
            DPRINT1("[Mm0]: Couldn't allocate driver section!\n");
            while (TRUE);
        }

        /* Sanity check */
        DPRINT("[Mm0]: Copying from: %p to: %p\n", DllBase, NewImageAddress);
        ASSERT(ExpInitializationPhase == 0);

#if 0 // Enable for FreeLDR 2.5
        /* Now copy the entire driver over */
        RtlCopyMemory(NewImageAddress, DllBase, DriverSize);
#else
        /* Copy headers over */
        RtlCopyMemory(NewImageAddress,
                      DllBase,
                      NtHeader->OptionalHeader.SizeOfHeaders);

        /*  Copy image sections into virtual section  */
        for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
        {
            /* Get the size of this section and check if it's valid and on-disk */
            Size = Section[i].VirtualAddress + Section[i].Misc.VirtualSize;
            if ((Size <= DriverSize) && (Section[i].SizeOfRawData))
            {
                /* Copy the data from the disk to the image */
                RtlCopyMemory((PVOID)((ULONG_PTR)NewImageAddress +
                                      Section[i].VirtualAddress),
                              (PVOID)((ULONG_PTR)DllBase +
                                      Section[i].PointerToRawData),
                              Section[i].Misc.VirtualSize >
                              Section[i].SizeOfRawData ?
                              Section[i].SizeOfRawData :
                              Section[i].Misc.VirtualSize);
            }
        }
#endif

        /* Sanity check */
        ASSERT(*(PULONG)NewImageAddress == *(PULONG)DllBase);

        /* Set the image base to the old address */
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
        LdrEntry->SizeOfImage = DriverSize;
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
    PsNtosImageBase = LdrEntry->DllBase;

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

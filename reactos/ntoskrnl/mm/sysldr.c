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

/* FUNCTIONS *****************************************************************/

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
            !(strcmp(NameImport->Name, (PCHAR)DllBase + NameTable[Hint])))
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
                Ret = strcmp(NameImport->Name, (PCHAR)DllBase + NameTable[Mid]);
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
                Status = LdrLoadModule(&DllName, &DllEntry);
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

                /* Get the base address and other information */
                DllBase = DllEntry->DllBase;
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


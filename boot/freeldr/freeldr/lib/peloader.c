/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides routines for loading PE files.
 *              (Deprecated remark) To be merged with arch/i386/loader.c in future.
 *
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2006-2019 Aleksey Bragin <aleksey@reactos.org>
 *
 * NOTES:       The source code in this file is based on the work of respective
 *              authors of PE loading code in ReactOS and Brian Palmer and
 *              Alex Ionescu's arch/i386/loader.c, and my research project
 *              (creating a native EFI loader for Windows).
 *
 *              This article was very handy during development:
 *              http://msdn.microsoft.com/msdnmag/issues/02/03/PE2/
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(PELOADER);

/* PRIVATE FUNCTIONS *********************************************************/

/* DllName - physical, UnicodeString->Buffer - virtual */
static BOOLEAN
PeLdrpCompareDllName(
    IN PCH DllName,
    IN PUNICODE_STRING UnicodeName)
{
    PWSTR Buffer;
    SIZE_T i, Length;

    /* First obvious check: for length of two names */
    Length = strlen(DllName);

#if DBG
    {
        UNICODE_STRING UnicodeNamePA;
        UnicodeNamePA.Length = UnicodeName->Length;
        UnicodeNamePA.MaximumLength = UnicodeName->MaximumLength;
        UnicodeNamePA.Buffer = VaToPa(UnicodeName->Buffer);
        TRACE("PeLdrpCompareDllName: %s and %wZ, Length = %d "
              "UN->Length %d\n", DllName, &UnicodeNamePA, Length, UnicodeName->Length);
    }
#endif

    if ((Length * sizeof(WCHAR)) > UnicodeName->Length)
        return FALSE;

    /* Store pointer to unicode string's buffer */
    Buffer = VaToPa(UnicodeName->Buffer);

    /* Loop character by character */
    for (i = 0; i < Length; i++)
    {
        /* Compare two characters, uppercasing them */
        if (toupper(*DllName) != toupper((CHAR)*Buffer))
            return FALSE;

        /* Move to the next character */
        DllName++;
        Buffer++;
    }

    /* Check, if strings either fully match, or match till the "." (w/o extension) */
    if ((UnicodeName->Length == Length * sizeof(WCHAR)) || (*Buffer == L'.'))
    {
        /* Yes they do */
        return TRUE;
    }

    /* Strings don't match, return FALSE */
    return FALSE;
}

static BOOLEAN
PeLdrpLoadAndScanReferencedDll(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCCH DirectoryPath,
    IN PCH ImportName,
    IN PLIST_ENTRY Parent OPTIONAL,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry);

static BOOLEAN
PeLdrpBindImportName(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA ThunkData,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN ProcessForwards,
    IN PCSTR DirectoryPath,
    IN PLIST_ENTRY Parent)
{
    ULONG Ordinal;
    PULONG NameTable, FunctionTable;
    PUSHORT OrdinalTable;
    LONG High, Low, Middle, Result;
    ULONG Hint;
    PIMAGE_IMPORT_BY_NAME ImportData;
    PCHAR ExportName, ForwarderName;
    BOOLEAN Success;

    //TRACE("PeLdrpBindImportName(): DllBase 0x%X, ImageBase 0x%X, ThunkData 0x%X, ExportDirectory 0x%X, ExportSize %d, ProcessForwards 0x%X\n",
    //      DllBase, ImageBase, ThunkData, ExportDirectory, ExportSize, ProcessForwards);

    /* Check passed DllBase param */
    if(DllBase == NULL)
    {
        WARN("DllBase == NULL!\n");
        return FALSE;
    }

    /* Convert all non-critical pointers to PA from VA */
    ThunkData = VaToPa(ThunkData);

    /* Is the reference by ordinal? */
    if (IMAGE_SNAP_BY_ORDINAL(ThunkData->u1.Ordinal) && !ProcessForwards)
    {
        /* Yes, calculate the ordinal */
        Ordinal = (ULONG)(IMAGE_ORDINAL(ThunkData->u1.Ordinal) - (UINT32)ExportDirectory->Base);
        //TRACE("PeLdrpBindImportName(): Ordinal %d\n", Ordinal);
    }
    else
    {
        /* It's reference by name, we have to look it up in the export directory */
        if (!ProcessForwards)
        {
            /* AddressOfData in thunk entry will become a virtual address (from relative) */
            //TRACE("PeLdrpBindImportName(): ThunkData->u1.AOD was %p\n", ThunkData->u1.AddressOfData);
            ThunkData->u1.AddressOfData =
                (ULONG_PTR)RVA(ImageBase, ThunkData->u1.AddressOfData);
            //TRACE("PeLdrpBindImportName(): ThunkData->u1.AOD became %p\n", ThunkData->u1.AddressOfData);
        }

        /* Get the import name */
        ImportData = VaToPa((PVOID)ThunkData->u1.AddressOfData);

        /* Get pointers to Name and Ordinal tables (RVA -> VA) */
        NameTable = VaToPa(RVA(DllBase, ExportDirectory->AddressOfNames));
        OrdinalTable = VaToPa(RVA(DllBase, ExportDirectory->AddressOfNameOrdinals));

        //TRACE("NameTable 0x%X, OrdinalTable 0x%X, ED->AddressOfNames 0x%X, ED->AOFO 0x%X\n",
        //      NameTable, OrdinalTable, ExportDirectory->AddressOfNames, ExportDirectory->AddressOfNameOrdinals);

        /* Get the hint, convert it to a physical pointer */
        Hint = ((PIMAGE_IMPORT_BY_NAME)VaToPa((PVOID)ThunkData->u1.AddressOfData))->Hint;
        //TRACE("HintIndex %d\n", Hint);

        /* Get the export name from the hint */
        ExportName = VaToPa(RVA(DllBase, NameTable[Hint]));

        /* If Hint is less than total number of entries in the export directory,
           and import name == export name, then we can just get it from the OrdinalTable */
        if ((Hint < ExportDirectory->NumberOfNames) &&
            (strcmp(ExportName, (PCHAR)ImportData->Name) == 0))
        {
            Ordinal = OrdinalTable[Hint];
            //TRACE("PeLdrpBindImportName(): Ordinal %d\n", Ordinal);
        }
        else
        {
            /* It's not the easy way, we have to lookup import name in the name table.
               Let's use a binary search for this task. */

            //TRACE("PeLdrpBindImportName() looking up the import name using binary search...\n");

            /* Low boundary is set to 0, and high boundary to the maximum index */
            Low = 0;
            High = ExportDirectory->NumberOfNames - 1;

            /* Perform a binary-search loop */
            while (High >= Low)
            {
                /* Divide by 2 by shifting to the right once */
                Middle = (Low + High) / 2;

                /* Get the name from the name table */
                ExportName = VaToPa(RVA(DllBase, NameTable[Middle]));

                /* Compare the names */
                Result = strcmp(ExportName, (PCHAR)ImportData->Name);

                // TRACE("Binary search: comparing Import '__', Export '%s'\n",
                      // VaToPa(&((PIMAGE_IMPORT_BY_NAME)VaToPa(ThunkData->u1.AddressOfData))->Name[0]),
                      // (PCHAR)VaToPa(RVA(DllBase, NameTable[Middle])));

                // TRACE("TE->u1.AOD %p, fulladdr %p\n",
                      // ThunkData->u1.AddressOfData,
                      // ((PIMAGE_IMPORT_BY_NAME)VaToPa(ThunkData->u1.AddressOfData))->Name );

                /* Depending on result of strcmp, perform different actions */
                if (Result > 0)
                {
                    /* Adjust top boundary */
                    High = Middle - 1;
                }
                else if (Result < 0)
                {
                    /* Adjust bottom boundary */
                    Low = Middle + 1;
                }
                else
                {
                    /* Yay, found it! */
                    break;
                }
            }

            /* If high boundary is less than low boundary, then no result found */
            if (High < Low)
            {
                ERR("Did not find export '%s'!\n", (PCHAR)ImportData->Name);
                return FALSE;
            }

            /* Everything alright, get the ordinal */
            Ordinal = OrdinalTable[Middle];

            //TRACE("PeLdrpBindImportName() found Ordinal %d\n", Ordinal);
        }
    }

    /* Check ordinal number for validity! */
    if (Ordinal >= ExportDirectory->NumberOfFunctions)
    {
        ERR("Ordinal number is invalid!\n");
        return FALSE;
    }

    /* Get a pointer to the function table */
    FunctionTable = (PULONG)VaToPa(RVA(DllBase, ExportDirectory->AddressOfFunctions));

    /* Save a pointer to the function */
    ThunkData->u1.Function = (ULONG_PTR)RVA(DllBase, FunctionTable[Ordinal]);

    /* Is it a forwarder? (function pointer is within the export directory) */
    ForwarderName = (PCHAR)VaToPa((PVOID)ThunkData->u1.Function);
    if (((ULONG_PTR)ForwarderName > (ULONG_PTR)ExportDirectory) &&
        ((ULONG_PTR)ForwarderName < ((ULONG_PTR)ExportDirectory + ExportSize)))
    {
        PLDR_DATA_TABLE_ENTRY DataTableEntry;
        CHAR ForwardDllName[255];
        PIMAGE_EXPORT_DIRECTORY RefExportDirectory;
        ULONG RefExportSize;

        TRACE("PeLdrpBindImportName(): ForwarderName %s\n", ForwarderName);

        /* Save the name of the forward dll */
        RtlCopyMemory(ForwardDllName, ForwarderName, sizeof(ForwardDllName));

        /* Strip out the symbol name */
        *strrchr(ForwardDllName,'.') = '\0';

        /* Check if the target image is already loaded */
        if (!PeLdrCheckForLoadedDll(ModuleListHead, ForwardDllName, &DataTableEntry))
        {
            /* Check if the forward dll name has an extension */
            if (strchr(ForwardDllName, '.') == NULL)
            {
                /* Name does not have an extension, append '.dll' */
                strcat(ForwardDllName, ".dll");
            }

            /* Now let's try to load it! */
            Success = PeLdrpLoadAndScanReferencedDll(ModuleListHead,
                                                     DirectoryPath,
                                                     ForwardDllName,
                                                     Parent,
                                                     &DataTableEntry);
            if (!Success)
            {
                ERR("PeLdrpLoadAndScanReferencedDll() failed to load forwarder dll.\n");
                return Success;
            }
        }

        /* Get pointer to the export directory of loaded DLL */
        RefExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
            RtlImageDirectoryEntryToData(VaToPa(DataTableEntry->DllBase),
            TRUE,
            IMAGE_DIRECTORY_ENTRY_EXPORT,
            &RefExportSize);

        /* Fail if it's NULL */
        if (RefExportDirectory)
        {
            UCHAR Buffer[128];
            IMAGE_THUNK_DATA RefThunkData;
            PIMAGE_IMPORT_BY_NAME ImportByName;
            PCHAR ImportName;

            /* Get pointer to the import name */
            ImportName = strrchr(ForwarderName, '.') + 1;

            /* Create a IMAGE_IMPORT_BY_NAME structure, pointing to the local Buffer */
            ImportByName = (PIMAGE_IMPORT_BY_NAME)Buffer;

            /* Fill the name with the import name */
            RtlCopyMemory(ImportByName->Name, ImportName, strlen(ImportName)+1);

            /* Set Hint to 0 */
            ImportByName->Hint = 0;

            /* And finally point ThunkData's AddressOfData to that structure */
            RefThunkData.u1.AddressOfData = (ULONG_PTR)ImportByName;

            /* And recursively call ourselves */
            Success = PeLdrpBindImportName(ModuleListHead,
                                           DataTableEntry->DllBase,
                                           ImageBase,
                                           &RefThunkData,
                                           RefExportDirectory,
                                           RefExportSize,
                                           TRUE,
                                           DirectoryPath,
                                           Parent);

            /* Fill out the ThunkData with data from RefThunkData */
            ThunkData->u1 = RefThunkData.u1;

            /* Return what we got from the recursive call */
            return Success;
        }
        else
        {
            /* Fail if ExportDirectory is NULL */
            return FALSE;
        }
    }

    /* Success! */
    return TRUE;
}

static BOOLEAN
PeLdrpLoadAndScanReferencedDll(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCCH DirectoryPath,
    IN PCH ImportName,
    IN PLIST_ENTRY Parent OPTIONAL,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry)
{
    CHAR FullDllName[256];
    BOOLEAN Success;
    PVOID BasePA = NULL;

    /* Prepare the full path to the file to be loaded */
    strcpy(FullDllName, DirectoryPath);
    strcat(FullDllName, ImportName);

    TRACE("Loading referenced DLL: %s\n", FullDllName);

    /* Load the image */
    Success = PeLdrLoadImage(FullDllName, LoaderBootDriver, &BasePA);
    if (!Success)
    {
        ERR("PeLdrLoadImage() failed\n");
        return Success;
    }

    /* Allocate DTE for newly loaded DLL */
    Success = PeLdrAllocateDataTableEntry(Parent ? Parent->Blink : ModuleListHead,
                                          ImportName,
                                          FullDllName,
                                          BasePA,
                                          DataTableEntry);
    if (!Success)
    {
        ERR("PeLdrAllocateDataTableEntry() failed\n");
        return Success;
    }

    (*DataTableEntry)->Flags |= LDRP_DRIVER_DEPENDENT_DLL;

    /* Scan its dependencies too */
    TRACE("PeLdrScanImportDescriptorTable() calling ourselves for %S\n",
          VaToPa((*DataTableEntry)->BaseDllName.Buffer));
    Success = PeLdrScanImportDescriptorTable(ModuleListHead, DirectoryPath, *DataTableEntry);
    if (!Success)
    {
        ERR("PeLdrScanImportDescriptorTable() failed\n");
        return Success;
    }

    return TRUE;
}

static BOOLEAN
PeLdrpScanImportAddressTable(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA ThunkData,
    IN PCSTR DirectoryPath,
    IN PLIST_ENTRY Parent)
{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory = NULL;
    BOOLEAN Success;
    ULONG ExportSize;

    TRACE("PeLdrpScanImportAddressTable(): DllBase 0x%X, "
          "ImageBase 0x%X, ThunkData 0x%X\n", DllBase, ImageBase, ThunkData);

    /* Obtain the export table from the DLL's base */
    if (DllBase == NULL)
    {
        ERR("Error, DllBase == NULL!\n");
        return FALSE;
    }
    else
    {
        ExportDirectory =
            (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(VaToPa(DllBase),
                TRUE,
                IMAGE_DIRECTORY_ENTRY_EXPORT,
                &ExportSize);
    }

    TRACE("PeLdrpScanImportAddressTable(): ExportDirectory 0x%X\n", ExportDirectory);

    /* If pointer to Export Directory is */
    if (ExportDirectory == NULL)
    {
        ERR("DllBase=%p(%p)\n", DllBase, VaToPa(DllBase));
        return FALSE;
    }

    /* Go through each entry in the thunk table and bind it */
    while (((PIMAGE_THUNK_DATA)VaToPa(ThunkData))->u1.AddressOfData != 0)
    {
        /* Bind it */
        Success = PeLdrpBindImportName(ModuleListHead,
                                       DllBase,
                                       ImageBase,
                                       ThunkData,
                                       ExportDirectory,
                                       ExportSize,
                                       FALSE,
                                       DirectoryPath,
                                       Parent);

        /* Move to the next entry */
        ThunkData++;

        /* Return error if binding was unsuccessful */
        if (!Success)
            return Success;
    }

    /* Return success */
    return TRUE;
}


/* FUNCTIONS *****************************************************************/

/* Returns TRUE if DLL has already been loaded - looks in LoadOrderList in LPB */
BOOLEAN
PeLdrCheckForLoadedDll(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCH DllName,
    OUT PLDR_DATA_TABLE_ENTRY *LoadedEntry)
{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    LIST_ENTRY *ModuleEntry;

    TRACE("PeLdrCheckForLoadedDll: DllName %s\n", DllName);

    /* Just go through each entry in the LoadOrderList and compare loaded module's
       name with a given name */
    ModuleEntry = ModuleListHead->Flink;
    while (ModuleEntry != ModuleListHead)
    {
        /* Get pointer to the current DTE */
        DataTableEntry = CONTAINING_RECORD(ModuleEntry,
            LDR_DATA_TABLE_ENTRY,
            InLoadOrderLinks);

        TRACE("PeLdrCheckForLoadedDll: DTE %p, EP %p, base %p name '%.*ws'\n",
              DataTableEntry, DataTableEntry->EntryPoint, DataTableEntry->DllBase,
              DataTableEntry->BaseDllName.Length / 2, VaToPa(DataTableEntry->BaseDllName.Buffer));

        /* Compare names */
        if (PeLdrpCompareDllName(DllName, &DataTableEntry->BaseDllName))
        {
            /* Yes, found it, report pointer to the loaded module's DTE
               to the caller and increase load count for it */
            *LoadedEntry = DataTableEntry;
            DataTableEntry->LoadCount++;
            TRACE("PeLdrCheckForLoadedDll: LoadedEntry %X\n", DataTableEntry);
            return TRUE;
        }

        /* Go to the next entry */
        ModuleEntry = ModuleEntry->Flink;
    }

    /* Nothing found */
    return FALSE;
}

BOOLEAN
PeLdrScanImportDescriptorTable(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCCH DirectoryPath,
    IN PLDR_DATA_TABLE_ENTRY ScanDTE)
{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PIMAGE_IMPORT_DESCRIPTOR ImportTable;
    ULONG ImportTableSize;
    PCH ImportName;
    BOOLEAN Success;

    /* Get a pointer to the import table of this image */
    ImportTable = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(VaToPa(ScanDTE->DllBase),
        TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImportTableSize);

#if DBG
    {
        UNICODE_STRING BaseName;
        BaseName.Buffer = VaToPa(ScanDTE->BaseDllName.Buffer);
        BaseName.MaximumLength = ScanDTE->BaseDllName.MaximumLength;
        BaseName.Length = ScanDTE->BaseDllName.Length;
        TRACE("PeLdrScanImportDescriptorTable(): %wZ ImportTable = 0x%X\n",
              &BaseName, ImportTable);
    }
#endif

    /* If image doesn't have any import directory - just return success */
    if (ImportTable == NULL)
        return TRUE;

    /* Loop through all entries */
    for (;(ImportTable->Name != 0) && (ImportTable->FirstThunk != 0);ImportTable++)
    {
        /* Get pointer to the name */
        ImportName = (PCH)VaToPa(RVA(ScanDTE->DllBase, ImportTable->Name));
        TRACE("PeLdrScanImportDescriptorTable(): Looking at %s\n", ImportName);

        /* In case we get a reference to ourselves - just skip it */
        if (PeLdrpCompareDllName(ImportName, &ScanDTE->BaseDllName))
            continue;

        /* Load the DLL if it is not already loaded */
        if (!PeLdrCheckForLoadedDll(ModuleListHead, ImportName, &DataTableEntry))
        {
            Success = PeLdrpLoadAndScanReferencedDll(ModuleListHead,
                                                     DirectoryPath,
                                                     ImportName,
                                                     &ScanDTE->InLoadOrderLinks,
                                                     &DataTableEntry);
            if (!Success)
            {
                ERR("PeLdrpLoadAndScanReferencedDll() failed\n");
                return Success;
            }
        }

        /* Scan its import address table */
        Success = PeLdrpScanImportAddressTable(ModuleListHead,
                                               DataTableEntry->DllBase,
                                               ScanDTE->DllBase,
                                               (PIMAGE_THUNK_DATA)RVA(ScanDTE->DllBase, ImportTable->FirstThunk),
                                               DirectoryPath,
                                               &ScanDTE->InLoadOrderLinks);

        if (!Success)
        {
            ERR("PeLdrpScanImportAddressTable() failed: ImportName = '%s', DirectoryPath = '%s'\n",
                ImportName, DirectoryPath);
            return Success;
        }
    }

    return TRUE;
}

BOOLEAN
PeLdrAllocateDataTableEntry(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCCH BaseDllName,
    IN PCCH FullDllName,
    IN PVOID BasePA,
    OUT PLDR_DATA_TABLE_ENTRY *NewEntry)
{
    PVOID BaseVA = PaToVa(BasePA);
    PWSTR BaseDllNameBuffer, Buffer;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PIMAGE_NT_HEADERS NtHeaders;
    USHORT Length;

    TRACE("PeLdrAllocateDataTableEntry(, '%s', '%s', %p)\n",
          BaseDllName, FullDllName, BasePA);

    /* Allocate memory for a data table entry, zero-initialize it */
    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)FrLdrHeapAlloc(sizeof(LDR_DATA_TABLE_ENTRY),
                                                           TAG_WLDR_DTE);
    if (DataTableEntry == NULL)
        return FALSE;

    /* Get NT headers from the image */
    NtHeaders = RtlImageNtHeader(BasePA);

    /* Initialize corresponding fields of DTE based on NT headers value */
    RtlZeroMemory(DataTableEntry, sizeof(LDR_DATA_TABLE_ENTRY));
    DataTableEntry->DllBase = BaseVA;
    DataTableEntry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
    DataTableEntry->EntryPoint = RVA(BaseVA, NtHeaders->OptionalHeader.AddressOfEntryPoint);
    DataTableEntry->SectionPointer = 0;
    DataTableEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;

    /* Initialize BaseDllName field (UNICODE_STRING) from the Ansi BaseDllName
       by simple conversion - copying each character */
    Length = (USHORT)(strlen(BaseDllName) * sizeof(WCHAR));
    Buffer = (PWSTR)FrLdrHeapAlloc(Length, TAG_WLDR_NAME);
    if (Buffer == NULL)
    {
        FrLdrHeapFree(DataTableEntry, TAG_WLDR_DTE);
        return FALSE;
    }

    /* Save Buffer, in case of later failure */
    BaseDllNameBuffer = Buffer;

    DataTableEntry->BaseDllName.Length = Length;
    DataTableEntry->BaseDllName.MaximumLength = Length;
    DataTableEntry->BaseDllName.Buffer = PaToVa(Buffer);

    RtlZeroMemory(Buffer, Length);
    Length /= sizeof(WCHAR);
    while (Length--)
    {
        *Buffer++ = *BaseDllName++;
    }

    /* Initialize FullDllName field (UNICODE_STRING) from the Ansi FullDllName
       using the same method */
    Length = (USHORT)(strlen(FullDllName) * sizeof(WCHAR));
    Buffer = (PWSTR)FrLdrHeapAlloc(Length, TAG_WLDR_NAME);
    if (Buffer == NULL)
    {
        FrLdrHeapFree(BaseDllNameBuffer, TAG_WLDR_NAME);
        FrLdrHeapFree(DataTableEntry, TAG_WLDR_DTE);
        return FALSE;
    }

    DataTableEntry->FullDllName.Length = Length;
    DataTableEntry->FullDllName.MaximumLength = Length;
    DataTableEntry->FullDllName.Buffer = PaToVa(Buffer);

    RtlZeroMemory(Buffer, Length);
    Length /= sizeof(WCHAR);
    while (Length--)
    {
        *Buffer++ = *FullDllName++;
    }

    /* Initialize what's left - LoadCount which is 1, and set Flags so that
       we know this entry is processed */
    DataTableEntry->Flags = LDRP_ENTRY_PROCESSED;
    DataTableEntry->LoadCount = 1;

    /* Honour the FORCE_INTEGRITY flag */
    if (NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY)
    {
        /*
         * On Vista and above, the LDRP_IMAGE_INTEGRITY_FORCED flag must be set
         * if IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY is set in the image header.
         * This is done after the image has been loaded and the digital signature
         * check has passed successfully. (We do not do it yet!)
         *
         * Several OS functionality depend on the presence of this flag.
         * For example, when using Object-Manager callbacks the latter will call
         * MmVerifyCallbackFunction() to verify whether the flag is present.
         * If not callbacks will not work.
         * (See Windows Internals Part 1, 6th edition, p. 176.)
         */
        DataTableEntry->Flags |= LDRP_IMAGE_INTEGRITY_FORCED;
    }

    /* Insert this DTE to a list in the LPB */
    InsertTailList(ModuleListHead, &DataTableEntry->InLoadOrderLinks);
    TRACE("Inserting DTE %p, name='%.*S' DllBase=%p\n", DataTableEntry,
          DataTableEntry->BaseDllName.Length / sizeof(WCHAR),
          VaToPa(DataTableEntry->BaseDllName.Buffer),
          DataTableEntry->DllBase);

    /* Save pointer to a newly allocated and initialized entry */
    *NewEntry = DataTableEntry;

    /* Return success */
    return TRUE;
}

/*
 * PeLdrLoadImage loads the specified image from the file (it doesn't
 * perform any additional operations on the filename, just directly
 * calls the file I/O routines), and relocates it so that it's ready
 * to be used when paging is enabled.
 * Addressing mode: physical
 */
BOOLEAN
PeLdrLoadImage(
    IN PCHAR FileName,
    IN TYPE_OF_MEMORY MemoryType,
    OUT PVOID *ImageBasePA)
{
    ULONG FileId;
    PVOID PhysicalBase;
    PVOID VirtualBase = NULL;
    UCHAR HeadersBuffer[SECTOR_SIZE * 2];
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG VirtualSize, SizeOfRawData, NumberOfSections;
    ARC_STATUS Status;
    LARGE_INTEGER Position;
    ULONG i, BytesRead;

    TRACE("PeLdrLoadImage(%s, %ld, *)\n", FileName, MemoryType);

    /* Open the image file */
    Status = ArcOpen((PSTR)FileName, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        WARN("ArcOpen(FileName: '%s') failed. Status: %u\n", FileName, Status);
        return FALSE;
    }

    /* Load the first 2 sectors of the image so we can read the PE header */
    Status = ArcRead(FileId, HeadersBuffer, SECTOR_SIZE * 2, &BytesRead);
    if (Status != ESUCCESS)
    {
        ERR("ArcRead(File: '%s') failed. Status: %u\n", FileName, Status);
        UiMessageBox("Error reading from file.");
        ArcClose(FileId);
        return FALSE;
    }

    /* Now read the MZ header to get the offset to the PE Header */
    NtHeaders = RtlImageNtHeader(HeadersBuffer);
    if (!NtHeaders)
    {
        ERR("No NT header found in \"%s\"\n", FileName);
        UiMessageBox("Error: No NT header found.");
        ArcClose(FileId);
        return FALSE;
    }

    /* Ensure this is executable image */
    if (((NtHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0))
    {
        ERR("Not an executable image \"%s\"\n", FileName);
        UiMessageBox("Not an executable image.");
        ArcClose(FileId);
        return FALSE;
    }

    /* Store number of sections to read and a pointer to the first section */
    NumberOfSections = NtHeaders->FileHeader.NumberOfSections;
    SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

    /* Try to allocate this memory, if fails - allocate somewhere else */
    PhysicalBase = MmAllocateMemoryAtAddress(NtHeaders->OptionalHeader.SizeOfImage,
                       (PVOID)((ULONG)NtHeaders->OptionalHeader.ImageBase & (KSEG0_BASE - 1)),
                       MemoryType);

    if (PhysicalBase == NULL)
    {
        /* It's ok, we don't panic - let's allocate again at any other "low" place */
        PhysicalBase = MmAllocateMemoryWithType(NtHeaders->OptionalHeader.SizeOfImage, MemoryType);

        if (PhysicalBase == NULL)
        {
            ERR("Failed to alloc %lu bytes for image %s\n", NtHeaders->OptionalHeader.SizeOfImage, FileName);
            UiMessageBox("Failed to alloc pages for image.");
            ArcClose(FileId);
            return FALSE;
        }
    }

    /* This is the real image base - in form of a virtual address */
    VirtualBase = PaToVa(PhysicalBase);

    TRACE("Base PA: 0x%X, VA: 0x%X\n", PhysicalBase, VirtualBase);

    /* Copy headers from already read data */
    RtlCopyMemory(PhysicalBase, HeadersBuffer, min(NtHeaders->OptionalHeader.SizeOfHeaders, sizeof(HeadersBuffer)));
    /* If headers are quite big, request next bytes from file */
    if (NtHeaders->OptionalHeader.SizeOfHeaders > sizeof(HeadersBuffer))
    {
        Status = ArcRead(FileId, (PUCHAR)PhysicalBase + sizeof(HeadersBuffer), NtHeaders->OptionalHeader.SizeOfHeaders - sizeof(HeadersBuffer), &BytesRead);
        if (Status != ESUCCESS)
        {
            ERR("ArcRead(File: '%s') failed. Status: %u\n", FileName, Status);
            UiMessageBox("Error reading headers.");
            ArcClose(FileId);
            return FALSE;
        }
    }

    /*
     * On Vista and above, a digital signature check is performed when the image
     * has the IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY flag set in its header.
     * (We of course do not perform this check yet!)
     */

    /* Reload the NT Header */
    NtHeaders = RtlImageNtHeader(PhysicalBase);

    /* Load the first section */
    SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

    /* Fill output parameters */
    *ImageBasePA = PhysicalBase;

    /* Walk through each section and read it (check/fix any possible
       bad situations, if they arise) */
    for (i = 0; i < NumberOfSections; i++)
    {
        VirtualSize = SectionHeader->Misc.VirtualSize;
        SizeOfRawData = SectionHeader->SizeOfRawData;

        /* Handle a case when VirtualSize equals 0 */
        if (VirtualSize == 0)
            VirtualSize = SizeOfRawData;

        /* If PointerToRawData is 0, then force its size to be also 0 */
        if (SectionHeader->PointerToRawData == 0)
        {
            SizeOfRawData = 0;
        }
        else
        {
            /* Cut the loaded size to the VirtualSize extents */
            if (SizeOfRawData > VirtualSize)
                SizeOfRawData = VirtualSize;
        }

        /* Actually read the section (if its size is not 0) */
        if (SizeOfRawData != 0)
        {
            /* Seek to the correct position */
            Position.QuadPart = SectionHeader->PointerToRawData;
            Status = ArcSeek(FileId, &Position, SeekAbsolute);

            TRACE("SH->VA: 0x%X\n", SectionHeader->VirtualAddress);

            /* Read this section from the file, size = SizeOfRawData */
            Status = ArcRead(FileId, (PUCHAR)PhysicalBase + SectionHeader->VirtualAddress, SizeOfRawData, &BytesRead);
            if (Status != ESUCCESS)
            {
                ERR("PeLdrLoadImage(): Error reading section from file!\n");
                break;
            }
        }

        /* Size of data is less than the virtual size - fill up the remainder with zeroes */
        if (SizeOfRawData < VirtualSize)
        {
            TRACE("PeLdrLoadImage(): SORD %d < VS %d\n", SizeOfRawData, VirtualSize);
            RtlZeroMemory((PVOID)(SectionHeader->VirtualAddress + (ULONG_PTR)PhysicalBase + SizeOfRawData), VirtualSize - SizeOfRawData);
        }

        SectionHeader++;
    }

    /* We are done with the file - close it */
    ArcClose(FileId);

    /* If loading failed - return right now */
    if (Status != ESUCCESS)
        return FALSE;

    /* Relocate the image, if it needs it */
    if (NtHeaders->OptionalHeader.ImageBase != (ULONG_PTR)VirtualBase)
    {
        WARN("Relocating %p -> %p\n", NtHeaders->OptionalHeader.ImageBase, VirtualBase);
        return (BOOLEAN)LdrRelocateImageWithBias(PhysicalBase,
                                                 (ULONG_PTR)VirtualBase - (ULONG_PTR)PhysicalBase,
                                                 "FreeLdr",
                                                 TRUE,
                                                 TRUE, /* in case of conflict still return success */
                                                 FALSE);
    }

    TRACE("PeLdrLoadImage() done, PA = %p\n", *ImageBasePA);
    return TRUE;
}

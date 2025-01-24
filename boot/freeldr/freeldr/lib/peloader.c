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
 *              https://web.archive.org/web/20131202091645/http://msdn.microsoft.com/en-us/magazine/cc301808.aspx
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(PELOADER);

/* GLOBALS *******************************************************************/

LIST_ENTRY FrLdrModuleList;

PELDR_IMPORTDLL_LOAD_CALLBACK PeLdrImportDllLoadCallback = NULL;

#ifdef _WIN64
#define COOKIE_MAX 0x0000FFFFFFFFFFFFll
#define DEFAULT_SECURITY_COOKIE 0x00002B992DDFA232ll
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif


/* PRIVATE FUNCTIONS *********************************************************/

static PVOID
PeLdrpFetchAddressOfSecurityCookie(PVOID BaseAddress, ULONG SizeOfImage)
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
    Cookie = VaToPa((PULONG_PTR)ConfigDir->SecurityCookie);

    /* Check this cookie */
    if ((PCHAR)Cookie <= (PCHAR)BaseAddress ||
        (PCHAR)Cookie >= (PCHAR)BaseAddress + SizeOfImage - sizeof(*Cookie))
    {
        Cookie = NULL;
    }

    /* Return validated security cookie */
    return Cookie;
}

/* DllName: physical, UnicodeString->Buffer: virtual */
static BOOLEAN
PeLdrpCompareDllName(
    _In_ PCSTR DllName,
    _In_ PCUNICODE_STRING UnicodeName)
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
        TRACE("PeLdrpCompareDllName: %s and %wZ, Length = %d, UN->Length %d\n",
              DllName, &UnicodeNamePA, Length, UnicodeName->Length);
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

    /* Strings don't match */
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
    _Inout_ PLIST_ENTRY ModuleListHead,
    _In_ PVOID DllBase,
    _In_ PVOID ImageBase,
    _In_ PIMAGE_THUNK_DATA ThunkName,
    _Inout_ PIMAGE_THUNK_DATA ThunkData,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_ ULONG ExportSize,
    _In_ BOOLEAN ProcessForwards,
    _In_ PCSTR DirectoryPath,
    _In_ PLIST_ENTRY Parent)
{
    ULONG Ordinal;
    PULONG NameTable, FunctionTable;
    PUSHORT OrdinalTable;
    LONG High, Low, Middle, Result;
    ULONG Hint;
    PIMAGE_IMPORT_BY_NAME ImportData;
    PCHAR ExportName, ForwarderName;
    BOOLEAN Success;

    //TRACE("PeLdrpBindImportName(): "
    //      "DllBase 0x%p, ImageBase 0x%p, ThunkName 0x%p, ThunkData 0x%p, ExportDirectory 0x%p, ExportSize %d, ProcessForwards 0x%X\n",
    //      DllBase, ImageBase, ThunkName, ThunkData, ExportDirectory, ExportSize, ProcessForwards);

    /* Check passed DllBase */
    if (!DllBase)
    {
        WARN("DllBase == NULL\n");
        return FALSE;
    }

    /* Convert all non-critical pointers to PA from VA */
    ThunkName = VaToPa(ThunkName);
    ThunkData = VaToPa(ThunkData);

    /* Is the reference by ordinal? */
    if (IMAGE_SNAP_BY_ORDINAL(ThunkName->u1.Ordinal) && !ProcessForwards)
    {
        /* Yes, calculate the ordinal */
        Ordinal = (ULONG)(IMAGE_ORDINAL(ThunkName->u1.Ordinal) - (UINT32)ExportDirectory->Base);
        //TRACE("PeLdrpBindImportName(): Ordinal %d\n", Ordinal);
    }
    else
    {
        /* It's reference by name, we have to look it up in the export directory */
        if (!ProcessForwards)
        {
            /* AddressOfData in thunk entry will become a virtual address (from relative) */
            //TRACE("PeLdrpBindImportName(): ThunkName->u1.AOD was %p\n", ThunkName->u1.AddressOfData);
            ThunkName->u1.AddressOfData =
                (ULONG_PTR)RVA(ImageBase, ThunkName->u1.AddressOfData);
            //TRACE("PeLdrpBindImportName(): ThunkName->u1.AOD became %p\n", ThunkName->u1.AddressOfData);
        }

        /* Get the import name, convert it to a physical pointer */
        ImportData = VaToPa((PVOID)ThunkName->u1.AddressOfData);

        /* Get pointers to Name and Ordinal tables (RVA -> VA) */
        NameTable = VaToPa(RVA(DllBase, ExportDirectory->AddressOfNames));
        OrdinalTable = VaToPa(RVA(DllBase, ExportDirectory->AddressOfNameOrdinals));

        //TRACE("NameTable 0x%X, OrdinalTable 0x%X, ED->AddressOfNames 0x%X, ED->AOFO 0x%X\n",
        //      NameTable, OrdinalTable, ExportDirectory->AddressOfNames, ExportDirectory->AddressOfNameOrdinals);

        /* Get the hint */
        Hint = ImportData->Hint;
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

                // TRACE("Binary search: comparing Import '%s', Export '%s'\n",
                      // (PCHAR)ImportData->Name, ExportName);

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
        PIMAGE_EXPORT_DIRECTORY RefExportDirectory;
        ULONG RefExportSize;
        CHAR ForwardDllName[256];

        TRACE("PeLdrpBindImportName(): ForwarderName %s\n", ForwarderName);

        /* Save the name of the forward dll */
        RtlCopyMemory(ForwardDllName, ForwarderName, sizeof(ForwardDllName));

        /* Strip out the symbol name */
        *strrchr(ForwardDllName, '.') = ANSI_NULL;

        /* Check if the target image is already loaded */
        if (!PeLdrCheckForLoadedDll(ModuleListHead, ForwardDllName, &DataTableEntry))
        {
            /* Check if the forward dll name has an extension */
            if (strchr(ForwardDllName, '.') == NULL)
            {
                /* Name does not have an extension, append '.dll' */
                RtlStringCbCatA(ForwardDllName, sizeof(ForwardDllName), ".dll");
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
    RtlStringCbCopyA(FullDllName, sizeof(FullDllName), DirectoryPath);
    RtlStringCbCatA(FullDllName, sizeof(FullDllName), ImportName);

    TRACE("Loading referenced DLL: %s\n", FullDllName);

    if (PeLdrImportDllLoadCallback)
        PeLdrImportDllLoadCallback(FullDllName);

    /* Load the image */
    Success = PeLdrLoadImage(FullDllName, LoaderBootDriver, &BasePA);
    if (!Success)
    {
        ERR("PeLdrLoadImage('%s') failed\n", FullDllName);
        return Success;
    }

    /* Allocate DTE for newly loaded DLL */
    Success = PeLdrAllocateDataTableEntry(Parent ? Parent->Blink : ModuleListHead,
                                          ImportName,
                                          FullDllName,
                                          PaToVa(BasePA),
                                          DataTableEntry);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("PeLdrAllocateDataTableEntry('%s') failed\n", FullDllName);
        MmFreeMemory(BasePA);
        return Success;
    }

    /* Init security cookie */
    PeLdrInitSecurityCookie(*DataTableEntry);

    (*DataTableEntry)->Flags |= LDRP_DRIVER_DEPENDENT_DLL;

    /* Scan its dependencies too */
    TRACE("PeLdrScanImportDescriptorTable() calling ourselves for '%.*S'\n",
          (*DataTableEntry)->BaseDllName.Length / sizeof(WCHAR),
          VaToPa((*DataTableEntry)->BaseDllName.Buffer));
    Success = PeLdrScanImportDescriptorTable(ModuleListHead, DirectoryPath, *DataTableEntry);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("PeLdrScanImportDescriptorTable() failed\n");
        PeLdrFreeDataTableEntry(*DataTableEntry);
        MmFreeMemory(BasePA);
        return Success;
    }

    return TRUE;
}

static BOOLEAN
PeLdrpScanImportAddressTable(
    _Inout_ PLIST_ENTRY ModuleListHead,
    _In_ PVOID DllBase,
    _In_ PVOID ImageBase,
    _In_ PIMAGE_THUNK_DATA ThunkName,
    _Inout_ PIMAGE_THUNK_DATA ThunkData,
    _In_ PCSTR DirectoryPath,
    _In_ PLIST_ENTRY Parent)
{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory = NULL;
    BOOLEAN Success;
    ULONG ExportSize;

    TRACE("PeLdrpScanImportAddressTable(): "
          "DllBase 0x%p, ImageBase 0x%p, ThunkName 0x%p, ThunkData 0x%p\n",
          DllBase, ImageBase, ThunkName, ThunkData);

    /* Obtain the export table from the DLL's base */
    if (!DllBase)
    {
        ERR("DllBase == NULL\n");
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
    TRACE("PeLdrpScanImportAddressTable(): ExportDirectory 0x%p\n", ExportDirectory);

    /* Fail if no export directory */
    if (!ExportDirectory)
    {
        ERR("No ExportDir, DllBase = %p (%p)\n", DllBase, VaToPa(DllBase));
        return FALSE;
    }

    /* Go through each thunk in the table and bind it */
    while (((PIMAGE_THUNK_DATA)VaToPa(ThunkName))->u1.AddressOfData != 0)
    {
        /* Bind it */
        Success = PeLdrpBindImportName(ModuleListHead,
                                       DllBase,
                                       ImageBase,
                                       ThunkName,
                                       ThunkData,
                                       ExportDirectory,
                                       ExportSize,
                                       FALSE,
                                       DirectoryPath,
                                       Parent);
        /* Fail if binding was unsuccessful */
        if (!Success)
            return Success;

        /* Move to the next thunk */
        ThunkName++;
        ThunkData++;
    }

    /* Return success */
    return TRUE;
}


/* FUNCTIONS *****************************************************************/

BOOLEAN
PeLdrInitializeModuleList(VOID)
{
    PLDR_DATA_TABLE_ENTRY FreeldrDTE;

    InitializeListHead(&FrLdrModuleList);

    /* Allocate a data table entry for freeldr.sys.
       The base name is scsiport.sys for imports from ntbootdd.sys */
    if (!PeLdrAllocateDataTableEntry(&FrLdrModuleList,
                                     "scsiport.sys",
                                     "freeldr.sys",
                                     &__ImageBase,
                                     &FreeldrDTE))
    {
        /* Cleanup and bail out */
        ERR("Failed to allocate DTE for freeldr\n");
        return FALSE;
    }

    return TRUE;
}

PVOID
PeLdrInitSecurityCookie(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PULONG_PTR Cookie;
    ULONG_PTR NewCookie;

    /* Fetch address of the cookie */
    Cookie = PeLdrpFetchAddressOfSecurityCookie(VaToPa(LdrEntry->DllBase), LdrEntry->SizeOfImage);

    if (!Cookie)
        return NULL;

    /* Check if it's a default one */
    if ((*Cookie == DEFAULT_SECURITY_COOKIE) ||
        (*Cookie == 0))
    {
        /* Generate new cookie using cookie address and time as seed */
        NewCookie = (ULONG_PTR)Cookie ^ (ULONG_PTR)ArcGetRelativeTime();
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

/* Returns TRUE if the DLL has already been loaded in the module list */
BOOLEAN
PeLdrCheckForLoadedDll(
    _Inout_ PLIST_ENTRY ModuleListHead,
    _In_ PCSTR DllName,
    _Out_ PLDR_DATA_TABLE_ENTRY* LoadedEntry)
{
    PLIST_ENTRY ModuleEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;

    TRACE("PeLdrCheckForLoadedDll: DllName %s\n", DllName);

    /* Go through each entry in the LoadOrderList and
     * compare the module's name with the given name */
    for (ModuleEntry = ModuleListHead->Flink;
         ModuleEntry != ModuleListHead;
         ModuleEntry = ModuleEntry->Flink)
    {
        /* Get a pointer to the current DTE */
        DataTableEntry = CONTAINING_RECORD(ModuleEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        TRACE("PeLdrCheckForLoadedDll: DTE %p, EP %p, Base %p, Name '%.*S'\n",
              DataTableEntry, DataTableEntry->EntryPoint, DataTableEntry->DllBase,
              DataTableEntry->BaseDllName.Length / sizeof(WCHAR),
              VaToPa(DataTableEntry->BaseDllName.Buffer));

        /* Compare names */
        if (PeLdrpCompareDllName(DllName, &DataTableEntry->BaseDllName))
        {
            /* Found it, return a pointer to the loaded module's
             * DTE to the caller and increase its load count */
            *LoadedEntry = DataTableEntry;
            DataTableEntry->LoadCount++;
            TRACE("PeLdrCheckForLoadedDll: LoadedEntry 0x%p\n", DataTableEntry);
            return TRUE;
        }
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
        TRACE("PeLdrScanImportDescriptorTable(): %wZ ImportTable = 0x%p\n",
              &BaseName, ImportTable);
    }
#endif

    /* If the image doesn't have any import directory, just return success */
    if (!ImportTable)
        return TRUE;

    /* Loop through all the entries */
    for (;(ImportTable->Name != 0) && (ImportTable->OriginalFirstThunk != 0);ImportTable++)
    {
        PIMAGE_THUNK_DATA ThunkName = RVA(ScanDTE->DllBase, ImportTable->OriginalFirstThunk);
        PIMAGE_THUNK_DATA ThunkData = RVA(ScanDTE->DllBase, ImportTable->FirstThunk);

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
                                               ThunkName,
                                               ThunkData,
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
    IN PVOID BaseVA,
    OUT PLDR_DATA_TABLE_ENTRY *NewEntry)
{
    PVOID BasePA = VaToPa(BaseVA);
    PWSTR BaseDllNameBuffer, Buffer;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PIMAGE_NT_HEADERS NtHeaders;
    USHORT Length;

    TRACE("PeLdrAllocateDataTableEntry('%s', '%s', %p)\n",
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

VOID
PeLdrFreeDataTableEntry(
    // _In_ PLIST_ENTRY ModuleListHead,
    _In_ PLDR_DATA_TABLE_ENTRY Entry)
{
    // ASSERT(ModuleListHead);
    ASSERT(Entry);

    RemoveEntryList(&Entry->InLoadOrderLinks);
    FrLdrHeapFree(VaToPa(Entry->FullDllName.Buffer), TAG_WLDR_NAME);
    FrLdrHeapFree(VaToPa(Entry->BaseDllName.Buffer), TAG_WLDR_NAME);
    FrLdrHeapFree(Entry, TAG_WLDR_DTE);
}

/**
 * @brief
 * Loads the specified image from the file.
 *
 * PeLdrLoadImage doesn't perform any additional operations on the file path,
 * it just directly calls the file I/O routines. It then relocates the image
 * so that it's ready to be used when paging is enabled.
 *
 * @note
 * Addressing mode: physical.
 **/
BOOLEAN
PeLdrLoadImageEx(
    _In_ PCSTR FilePath,
    _In_ TYPE_OF_MEMORY MemoryType,
    _Out_ PVOID* ImageBasePA,
    _In_ BOOLEAN KernelMapping)
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

    TRACE("PeLdrLoadImage('%s', %ld)\n", FilePath, MemoryType);

    /* Open the image file */
    Status = ArcOpen((PSTR)FilePath, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        WARN("ArcOpen('%s') failed. Status: %u\n", FilePath, Status);
        return FALSE;
    }

    /* Load the first 2 sectors of the image so we can read the PE header */
    Status = ArcRead(FileId, HeadersBuffer, SECTOR_SIZE * 2, &BytesRead);
    if (Status != ESUCCESS)
    {
        ERR("ArcRead('%s') failed. Status: %u\n", FilePath, Status);
        ArcClose(FileId);
        return FALSE;
    }

    /* Now read the MZ header to get the offset to the PE Header */
    NtHeaders = RtlImageNtHeader(HeadersBuffer);
    if (!NtHeaders)
    {
        ERR("No NT header found in \"%s\"\n", FilePath);
        ArcClose(FileId);
        return FALSE;
    }

    /* Ensure this is executable image */
    if (((NtHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0))
    {
        ERR("Not an executable image \"%s\"\n", FilePath);
        ArcClose(FileId);
        return FALSE;
    }

    /* Store number of sections to read and a pointer to the first section */
    NumberOfSections = NtHeaders->FileHeader.NumberOfSections;
    SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

    /* Try to allocate this memory; if it fails, allocate somewhere else */
    PhysicalBase = MmAllocateMemoryAtAddress(NtHeaders->OptionalHeader.SizeOfImage,
                       (PVOID)((ULONG)NtHeaders->OptionalHeader.ImageBase & (KSEG0_BASE - 1)),
                       MemoryType);

    if (PhysicalBase == NULL)
    {
        /* Don't fail, allocate again at any other "low" place */
        PhysicalBase = MmAllocateMemoryWithType(NtHeaders->OptionalHeader.SizeOfImage, MemoryType);

        if (PhysicalBase == NULL)
        {
            ERR("Failed to alloc %lu bytes for image %s\n", NtHeaders->OptionalHeader.SizeOfImage, FilePath);
            ArcClose(FileId);
            return FALSE;
        }
    }

    /* This is the real image base, in form of a virtual address */
    VirtualBase = KernelMapping ? PaToVa(PhysicalBase) : PhysicalBase;

    TRACE("Base PA: 0x%p, VA: 0x%p\n", PhysicalBase, VirtualBase);

    /* Copy headers from already read data */
    RtlCopyMemory(PhysicalBase, HeadersBuffer, min(NtHeaders->OptionalHeader.SizeOfHeaders, sizeof(HeadersBuffer)));
    /* If headers are quite big, request next bytes from file */
    if (NtHeaders->OptionalHeader.SizeOfHeaders > sizeof(HeadersBuffer))
    {
        Status = ArcRead(FileId, (PUCHAR)PhysicalBase + sizeof(HeadersBuffer), NtHeaders->OptionalHeader.SizeOfHeaders - sizeof(HeadersBuffer), &BytesRead);
        if (Status != ESUCCESS)
        {
            ERR("ArcRead('%s') failed. Status: %u\n", FilePath, Status);
            // UiMessageBox("Error reading headers.");
            ArcClose(FileId);
            goto Failure;
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

        /* Size of data is less than the virtual size: fill up the remainder with zeroes */
        if (SizeOfRawData < VirtualSize)
        {
            TRACE("PeLdrLoadImage(): SORD %d < VS %d\n", SizeOfRawData, VirtualSize);
            RtlZeroMemory((PVOID)(SectionHeader->VirtualAddress + (ULONG_PTR)PhysicalBase + SizeOfRawData), VirtualSize - SizeOfRawData);
        }

        SectionHeader++;
    }

    /* We are done with the file, close it */
    ArcClose(FileId);

    /* If loading failed, return right now */
    if (Status != ESUCCESS)
        goto Failure;

    /* Relocate the image, if it needs it */
    if (NtHeaders->OptionalHeader.ImageBase != (ULONG_PTR)VirtualBase)
    {
        WARN("Relocating %p -> %p\n", NtHeaders->OptionalHeader.ImageBase, VirtualBase);
        Status = LdrRelocateImageWithBias(PhysicalBase,
                                          (ULONG_PTR)VirtualBase - (ULONG_PTR)PhysicalBase,
                                          "FreeLdr",
                                          ESUCCESS,
                                          ESUCCESS, /* In case of conflict still return success */
                                          ENOEXEC);
        if (Status != ESUCCESS)
            goto Failure;
    }

    /* Fill output parameters */
    *ImageBasePA = PhysicalBase;

    TRACE("PeLdrLoadImage() done, PA = %p\n", *ImageBasePA);
    return TRUE;

Failure:
    /* Cleanup and bail out */
    MmFreeMemory(PhysicalBase);
    return FALSE;
}

BOOLEAN
PeLdrLoadImage(
    _In_ PCSTR FilePath,
    _In_ TYPE_OF_MEMORY MemoryType,
    _Out_ PVOID* ImageBasePA)
{
    return PeLdrLoadImageEx(FilePath, MemoryType, ImageBasePA, TRUE);
}

BOOLEAN
PeLdrLoadBootImage(
    _In_ PCSTR FilePath,
    _In_ PCSTR BaseDllName,
    _Out_ PVOID* ImageBase,
    _Out_ PLDR_DATA_TABLE_ENTRY* DataTableEntry)
{
    BOOLEAN Success;

    /* Load the image as a bootloader image */
    Success = PeLdrLoadImageEx(FilePath,
                               LoaderLoadedProgram,
                               ImageBase,
                               FALSE);
    if (!Success)
    {
        WARN("Failed to load boot image '%s'\n", FilePath);
        return FALSE;
    }

    /* Allocate a DTE */
    Success = PeLdrAllocateDataTableEntry(&FrLdrModuleList,
                                          BaseDllName,
                                          FilePath,
                                          *ImageBase,
                                          DataTableEntry);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("Failed to allocate DTE for '%s'\n", FilePath);
        MmFreeMemory(*ImageBase);
        return FALSE;
    }

    /* Resolve imports */
    Success = PeLdrScanImportDescriptorTable(&FrLdrModuleList, "", *DataTableEntry);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("Failed to resolve imports for '%s'\n", FilePath);
        PeLdrFreeDataTableEntry(*DataTableEntry);
        MmFreeMemory(*ImageBase);
        return FALSE;
    }

    return TRUE;
}

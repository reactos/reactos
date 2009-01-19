#include <freeldr.h>
#include <debug.h>

extern BOOLEAN FrLdrBootType;

ULONG_PTR NextModuleBase = KERNEL_BASE_PHYS;
PLOADER_MODULE CurrentModule = NULL;

PVOID
NTAPI
LdrPEGetExportByName(
    IN PVOID BaseAddress,
    IN PUCHAR SymbolName,
    IN USHORT Hint
);

extern BOOLEAN FrLdrLoadDriver(PCHAR szFileName, INT nPos);
ULONG Drivers;
PVOID AnsiData, OemData, UnicodeData, RegistryData, KernelData, HalData, DriverData[16];
ULONG RegistrySize, AnsiSize, OemSize, UnicodeSize, KernelSize, HalSize, DriverSize[16];
PCHAR DriverName[16];

/* MODULE MANAGEMENT **********************************************************/

PLOADER_MODULE
NTAPI
LdrGetModuleObject(IN PCHAR ModuleName)
{
    ULONG i;
    
    for (i = 0; i < LoaderBlock.ModsCount; i++)
    {
        if (strstr(_strupr((PCHAR)reactos_modules[i].String), _strupr(ModuleName)))
        {
            return &reactos_modules[i];
        }
    }
    
    return NULL;
}

NTSTATUS
NTAPI
LdrPEGetOrLoadModule(IN PCHAR ModuleName,
                     IN PCHAR ImportedName,
                     IN PLOADER_MODULE* ImportedModule)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    *ImportedModule = LdrGetModuleObject(ImportedName);
    if (*ImportedModule == NULL)
    {
        if (!FrLdrLoadDriver(ImportedName, 0))
        {
            return STATUS_UNSUCCESSFUL;
        }
        else
        {
            return LdrPEGetOrLoadModule(ModuleName, ImportedName, ImportedModule);
        }
    }
    
    return Status;
}

ULONG_PTR
NTAPI
FrLdrLoadModule(FILE *ModuleImage,
                LPCSTR ModuleName,
                PULONG ModuleSize)
{
    ULONG LocalModuleSize;
    PLOADER_MODULE ModuleData;
    LPSTR NameBuffer;
    LPSTR TempName;
    
    /* Get current module data structure and module name string array */
    ModuleData = &reactos_modules[LoaderBlock.ModsCount];
    
    /* Get only the Module Name */
    do {
        
        TempName = strchr(ModuleName, '\\');
        
        if(TempName) {
            ModuleName = TempName + 1;
        }
        
    } while(TempName);
    NameBuffer = reactos_module_strings[LoaderBlock.ModsCount];
    
    /* Get Module Size */
    LocalModuleSize = FsGetFileSize(ModuleImage);
    
    /* Fill out Module Data Structure */
    ModuleData->ModStart = NextModuleBase;
    ModuleData->ModEnd = NextModuleBase + LocalModuleSize;
    
    /* Save name */
    strcpy(NameBuffer, ModuleName);
    ModuleData->String = (ULONG_PTR)NameBuffer;
    
    /* NLS detection for NT Loader Block */
    if (!_stricmp(NameBuffer, "ansi.nls"))
    {
        AnsiData = (PVOID)NextModuleBase;
        AnsiSize = LocalModuleSize;
    }
    else if (!_stricmp(NameBuffer, "oem.nls"))
    {
        OemData = (PVOID)NextModuleBase;
        OemSize = LocalModuleSize;
    }
    else if (!_stricmp(NameBuffer, "casemap.nls"))
    {
        UnicodeData = (PVOID)NextModuleBase;
        UnicodeSize = LocalModuleSize;
    }
    else if (!(_stricmp(NameBuffer, "system")) ||
             !(_stricmp(NameBuffer, "system.hiv")))
    {
        RegistryData = (PVOID)NextModuleBase;
        RegistrySize = LocalModuleSize;
    }

    /* Load the file image */
    FsReadFile(ModuleImage, LocalModuleSize, NULL, (PVOID)NextModuleBase);
    
    /* Move to next memory block and increase Module Count */
    NextModuleBase = ROUND_UP(ModuleData->ModEnd, PAGE_SIZE);
    LoaderBlock.ModsCount++;
    //    DbgPrint("NextBase, ImageSize, ModStart, ModEnd %p %p %p %p\n",
    //           NextModuleBase, LocalModuleSize, ModuleData->ModStart, ModuleData->ModEnd);
    
    /* Return Module Size if required */
    if (ModuleSize != NULL) {
        *ModuleSize = LocalModuleSize;
    }
    
    return(ModuleData->ModStart);
}

ULONG_PTR
NTAPI
FrLdrCreateModule(LPCSTR ModuleName)
{
    PLOADER_MODULE ModuleData;
    LPSTR NameBuffer;
    
    /* Get current module data structure and module name string array */
    ModuleData = &reactos_modules[LoaderBlock.ModsCount];
    NameBuffer = reactos_module_strings[LoaderBlock.ModsCount];
    
    /* Set up the structure */
    ModuleData->ModStart = NextModuleBase;
    ModuleData->ModEnd = -1;
    
    /* Copy the name */
    strcpy(NameBuffer, ModuleName);
    ModuleData->String = (ULONG_PTR)NameBuffer;
    
    /* Set the current Module */
    CurrentModule = ModuleData;
    
    /* Return Module Base Address */
    return(ModuleData->ModStart);
}

BOOLEAN
NTAPI
FrLdrCloseModule(ULONG_PTR ModuleBase,
                 ULONG ModuleSize)
{
    PLOADER_MODULE ModuleData = CurrentModule;
    
    /* Make sure a module is opened */
    if (ModuleData) {
        
        /* Make sure this is the right module and that it hasn't been closed */
        if ((ModuleBase == ModuleData->ModStart) && (ModuleData->ModEnd == (ULONG_PTR)-1)) {
            
            /* Close the Module */
            ModuleData->ModEnd = ModuleData->ModStart + ModuleSize;
            
            /* Set the next Module Base and increase the number of modules */
            NextModuleBase = ROUND_UP(ModuleData->ModEnd, PAGE_SIZE);
            LoaderBlock.ModsCount++;
            
            /* Close the currently opened module */
            CurrentModule = NULL;
            
            /* Success */
            return(TRUE);
        }
    }
    
    /* Failure path */
    return(FALSE);
}

/* PE IMAGE LOADER  ***********************************************************/

PVOID
NTAPI
LdrPEFixupForward(IN PCHAR ForwardName)
{
    CHAR NameBuffer[128];
    PCHAR p;
    PLOADER_MODULE ModuleObject;
    
    strcpy(NameBuffer, ForwardName);
    p = strchr(NameBuffer, '.');
    if (p == NULL) return NULL;
    *p = 0;
    
    ModuleObject = LdrGetModuleObject(NameBuffer);
    if (!ModuleObject)
    {
        DbgPrint("LdrPEFixupForward: failed to find module %s\n", NameBuffer);
        return NULL;
    }
    
    return LdrPEGetExportByName((PVOID)ModuleObject->ModStart, (PUCHAR)(p + 1), 0xffff);
}

PVOID
NTAPI
LdrPEGetExportByName(PVOID BaseAddress,
                     PUCHAR SymbolName,
                     USHORT Hint)
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    ULONG * ExFunctions;
    ULONG * ExNames;
    USHORT * ExOrdinals;
    PVOID ExName;
    ULONG Ordinal;
    PVOID Function;
    LONG minn, maxn, mid, res;
    ULONG ExportDirSize;
    
    /* HAL and NTOS use a virtual address, switch it to physical mode */
    if ((ULONG_PTR)BaseAddress & KSEG0_BASE)
    {
        BaseAddress = RVA(BaseAddress, -KSEG0_BASE);
    }
    
    ExportDir = (PIMAGE_EXPORT_DIRECTORY)
    RtlImageDirectoryEntryToData(BaseAddress,
                                 TRUE,
                                 IMAGE_DIRECTORY_ENTRY_EXPORT,
                                 &ExportDirSize);
    if (!ExportDir)
    {
        DbgPrint("LdrPEGetExportByName(): no export directory!\n");
        return NULL;
    }
    
    /* The symbol names may be missing entirely */
    if (!ExportDir->AddressOfNames)
    {
        DbgPrint("LdrPEGetExportByName(): symbol names missing entirely\n");
        return NULL;
    }
    
    /*
     * Get header pointers
     */
    ExNames = (ULONG *)RVA(BaseAddress, ExportDir->AddressOfNames);
    ExOrdinals = (USHORT *)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
    ExFunctions = (ULONG *)RVA(BaseAddress, ExportDir->AddressOfFunctions);

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
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DbgPrint("LdrPEGetExportByName(): failed to find %s\n", Function);
                }
                return Function;
            }
            
            if (Function != NULL) return Function;
        }
    }
    
    /*
     * Binary search
     */
    minn = 0;
    maxn = ExportDir->NumberOfNames - 1;
    while (minn <= maxn)
    {
        mid = (minn + maxn) / 2;
        
        ExName = RVA(BaseAddress, ExNames[mid]);
        res = strcmp(ExName, (PCHAR)SymbolName);
        if (res == 0)
        {
            Ordinal = ExOrdinals[mid];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DbgPrint("1: failed to find %s\n", Function);
                }
                return Function;
            }
            if (Function != NULL)
            {
                return Function;
            }
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
    
    ExName = RVA(BaseAddress, ExNames[mid]);
    DbgPrint("2: failed to find %s\n",SymbolName);
    return (PVOID)NULL;
}

NTSTATUS
NTAPI
LdrPEProcessImportDirectoryEntry(PVOID DriverBase,
                                 PLOADER_MODULE LoaderModule,
                                 PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory)
{
    PVOID* ImportAddressList;
    PULONG_PTR FunctionNameList;
    
    if (ImportModuleDirectory == NULL || ImportModuleDirectory->Name == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    
    /* Get the import address list. */
    ImportAddressList = (PVOID*)RVA(DriverBase, ImportModuleDirectory->FirstThunk);
    
    /* Get the list of functions to import. */
    if (ImportModuleDirectory->OriginalFirstThunk != 0)
    {
        FunctionNameList = (PULONG_PTR)RVA(DriverBase, ImportModuleDirectory->OriginalFirstThunk);
    }
    else
    {
        FunctionNameList = (PULONG_PTR)RVA(DriverBase, ImportModuleDirectory->FirstThunk);
    }
    
    /* Walk through function list and fixup addresses. */
    while (*FunctionNameList != 0L)
    {
        if ((*FunctionNameList) & 0x80000000)
        {
            DbgPrint("Failed to import ordinal from %s\n", LoaderModule->String);
            return STATUS_UNSUCCESSFUL;
        }
        else
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = RVA(DriverBase, *FunctionNameList);
            *ImportAddressList = LdrPEGetExportByName((PVOID)LoaderModule->ModStart, pe_name->Name, pe_name->Hint);
            
            /* Fixup the address to be virtual */
            *ImportAddressList = RVA(*ImportAddressList, KSEG0_BASE);
            
            //DbgPrint("Looked for: %s and found: %p\n", pe_name->Name, *ImportAddressList);
            if ((*ImportAddressList) == NULL)
            {
                DbgPrint("Failed to import %s from %s\n", pe_name->Name, LoaderModule->String);
                return STATUS_UNSUCCESSFUL;
            }
        }
        ImportAddressList++;
        FunctionNameList++;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrPEFixupImports(IN PVOID DllBase,
                  IN PCHAR DllName)
{
    PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
    PCHAR ImportedName;
    NTSTATUS Status;
    PLOADER_MODULE ImportedModule;
    ULONG Size;
    
    /*  Process each import module  */
    ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
    RtlImageDirectoryEntryToData(DllBase,
                                 TRUE,
                                 IMAGE_DIRECTORY_ENTRY_IMPORT,
                                 &Size);
    while (ImportModuleDirectory && ImportModuleDirectory->Name)
    {
        /*  Check to make sure that import lib is kernel  */
        ImportedName = (PCHAR) DllBase + ImportModuleDirectory->Name;
        //DbgPrint("Processing imports for file: %s into file: %s\n", DllName, ImportedName);
        
        Status = LdrPEGetOrLoadModule(DllName, ImportedName, &ImportedModule);
        if (!NT_SUCCESS(Status)) return Status;
        
        Status = LdrPEProcessImportDirectoryEntry(DllBase, ImportedModule, ImportModuleDirectory);
        if (!NT_SUCCESS(Status)) return Status;
        
        //DbgPrint("Imports for file: %s into file: %s complete\n", DllName, ImportedName);
        ImportModuleDirectory++;
    }
    
    return STATUS_SUCCESS;
}

PVOID
NTAPI
FrLdrReadAndMapImage(IN FILE *Image,
                     IN PCHAR Name,
                     IN ULONG ImageType)
{
    PVOID ImageBase, LoadBase, ReadBuffer;
    ULONG ImageId = LoaderBlock.ModsCount;
    ULONG i, Size, ImageSize, SizeOfHeaders;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER Section;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Try to see, maybe it's loaded already */
    if (LdrGetModuleObject(Name) != NULL)
    {
        /* It's loaded, return NULL. It would be wise to return
         correct LoadBase, but it seems to be ignored almost everywhere */
        return NULL;
    }

    /* Set the virtual (image) and physical (load) addresses */
    LoadBase = (PVOID)NextModuleBase;
    ImageBase = RVA(LoadBase, KSEG0_BASE);

    /* Allocate a temporary buffer for the read */
    ReadBuffer = MmHeapAlloc(MM_PAGE_SIZE);
    if (!ReadBuffer)
    {
        /* Fail */
        DbgPrint("Failed to allocate a temporary buffer for the read\n");
        return NULL;
    }

    /* Set the file pointer to zero */
    FsSetFilePointer(Image, 0);

    /* Load first page of the file image */
    if (!FsReadFile(Image, MM_PAGE_SIZE, NULL, ReadBuffer))
    {
        /* Fail */
        DbgPrint("Failed to read image: %s\n", Name);
        return NULL;
    }

    /* Get image headers */
    NtHeader = RtlImageNtHeader(ReadBuffer);

    /* Allocate memory for the driver */
    ImageSize = NtHeader->OptionalHeader.SizeOfImage;
    LoadBase = MmAllocateMemoryAtAddress(ImageSize, LoadBase, LoaderSystemCode);
    ASSERT(LoadBase);

    /* Copy headers over */
    SizeOfHeaders = NtHeader->OptionalHeader.SizeOfHeaders;
    if (SizeOfHeaders < MM_PAGE_SIZE)
    {
        RtlMoveMemory(LoadBase, ReadBuffer, SizeOfHeaders);
    }
    else
    {
        RtlMoveMemory(LoadBase, ReadBuffer, MM_PAGE_SIZE);
        if (!FsReadFile(Image, SizeOfHeaders - MM_PAGE_SIZE, NULL,
                        (PVOID)((ULONG_PTR)LoadBase + MM_PAGE_SIZE)))
        {
            DbgPrint("Failed to read image: %s\n", Name);
            return NULL;
        }
    }

    /* Free the temporary buffer */
    MmHeapFree(ReadBuffer);

    /* Get the first section */
    NtHeader = RtlImageNtHeader(LoadBase);
    Section = IMAGE_FIRST_SECTION(NtHeader);

    /*  Read image sections into virtual section  */
    for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
    {
        /* Get the size of this section and check if it's valid */
        Size = Section[i].VirtualAddress + Section[i].Misc.VirtualSize;
        if (Size <= ImageSize)
        {
            if (Section[i].SizeOfRawData)
            {
                /* Copy the data from the disk to the image */
                FsSetFilePointer(Image, Section[i].PointerToRawData);
                if (!FsReadFile(Image,
                                Section[i].Misc.VirtualSize >
                                Section[i].SizeOfRawData ?
                                Section[i].SizeOfRawData :
                                Section[i].Misc.VirtualSize,
                                NULL,
                                (PVOID)((ULONG_PTR)LoadBase +
                                        Section[i].VirtualAddress)))
                {
                    DbgPrint("Failed to read image: %s\n", Name);
                    return NULL;
                }
            }
            else
            {
                /* Clear the BSS area */
                RtlZeroMemory((PVOID)((ULONG_PTR)LoadBase +
                                      Section[i].VirtualAddress),
                              Section[i].Misc.VirtualSize);
            }
        }
        else
        {
            DbgPrint("Section %s in %s doesn't fit: VA: %lx, Size: %lx\n", 
                      Section[i].Name, Name, Section[i].VirtualAddress,
                      Section[i].Misc.VirtualSize);
        }
    }

    /* Calculate Difference between Real Base and Compiled Base*/
    Status = LdrRelocateImageWithBias(LoadBase,
                                      (ULONG_PTR)ImageBase -
                                      (ULONG_PTR)LoadBase,
                                      "FreeLdr",
                                      STATUS_SUCCESS,
#ifdef _M_AMD64
                                      STATUS_SUCCESS, // allow stripped files
#else
                                      STATUS_UNSUCCESSFUL,
#endif
                                      STATUS_UNSUCCESSFUL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DbgPrint("Failed to relocate image: %s\n", Name);
        return NULL;
    }

    /* Fill out Module Data Structure */
    reactos_modules[ImageId].ModStart = (ULONG_PTR)ImageBase;
    reactos_modules[ImageId].ModEnd = (ULONG_PTR)ImageBase + ImageSize;
    strcpy(reactos_module_strings[ImageId], Name);
    reactos_modules[ImageId].String = (ULONG_PTR)reactos_module_strings[ImageId];
    LoaderBlock.ModsCount++;

    /* Detect kernel or HAL */
    if (!_stricmp(Name, "ntoskrnl.exe"))
    {
        KernelData = (PVOID)NextModuleBase;
        KernelSize = ImageSize;
    }
    else if (!_stricmp(Name, "hal.dll"))
    {
        HalData = (PVOID)NextModuleBase;
        HalSize = ImageSize;
    }
    else
    {
        DriverName[Drivers] = reactos_module_strings[ImageId];
        DriverData[Drivers] = (PVOID)NextModuleBase;
        DriverSize[Drivers] = ImageSize;
        Drivers++;
    }

    /* Increase the next Load Base */
    NextModuleBase = ROUND_UP(NextModuleBase + ImageSize, PAGE_SIZE);

    /* Perform import fixups */
    if (!NT_SUCCESS(LdrPEFixupImports(LoadBase, Name)))
    {
        /* Fixup failed, just don't include it in the list */
        // NextModuleBase = OldNextModuleBase;
        LoaderBlock.ModsCount = ImageId;
        return NULL;
    }

    /* Return the final mapped address */
    return LoadBase;
}

ULONG
NTAPI
FrLdrReMapImage(IN PVOID Base,
                IN PVOID LoadBase)
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER Section;
    ULONG i, Size, DriverSize = 0;
    
    /* Get the first section */
    NtHeader = RtlImageNtHeader(Base);
    Section = IMAGE_FIRST_SECTION(NtHeader);
    
    /* Allocate memory for the driver */
    DriverSize = NtHeader->OptionalHeader.SizeOfImage;
    LoadBase = MmAllocateMemoryAtAddress(DriverSize, LoadBase, LoaderSystemCode);
    ASSERT(LoadBase);
    
    /* Copy headers over */
    RtlMoveMemory(LoadBase, Base, NtHeader->OptionalHeader.SizeOfHeaders);
    
    /*  Copy image sections into virtual section  */
    for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
    {
        /* Get the size of this section and check if it's valid */
        Size = Section[i].VirtualAddress + Section[i].Misc.VirtualSize;
        if (Size <= DriverSize)
        {
            if (Section[i].SizeOfRawData)
            {
                /* Copy the data from the disk to the image */
                RtlCopyMemory((PVOID)((ULONG_PTR)LoadBase +
                                      Section[i].VirtualAddress),
                              (PVOID)((ULONG_PTR)Base +
                                      Section[i].PointerToRawData),
                              Section[i].Misc.VirtualSize >
                              Section[i].SizeOfRawData ?
                              Section[i].SizeOfRawData :
                              Section[i].Misc.VirtualSize);
            }
            else
            {
                /* Clear the BSS area */
                RtlZeroMemory((PVOID)((ULONG_PTR)LoadBase +
                                      Section[i].VirtualAddress),
                              Section[i].Misc.VirtualSize);
            }
        }
    }
    
    /* Return the size of the mapped driver */
    return DriverSize;
}

PVOID
NTAPI
FrLdrMapImage(IN FILE *Image,
              IN PCHAR Name,
              IN ULONG ImageType)
{
    PVOID ImageBase, LoadBase, ReadBuffer;
    ULONG ImageId = LoaderBlock.ModsCount;
    ULONG ImageSize;
    NTSTATUS Status = STATUS_SUCCESS;
    
    /* Try to see, maybe it's loaded already */
    if (LdrGetModuleObject(Name) != NULL)
    {
        /* It's loaded, return NULL. It would be wise to return
         correct LoadBase, but it seems to be ignored almost everywhere */
        return NULL;
    }
    
    /* Set the virtual (image) and physical (load) addresses */
    LoadBase = (PVOID)NextModuleBase;
    ImageBase = RVA(LoadBase, KSEG0_BASE);
    
    /* Save the Image Size */
    ImageSize = FsGetFileSize(Image);
    
    /* Set the file pointer to zero */
    FsSetFilePointer(Image, 0);
    
    /* Allocate a temporary buffer for the read */
    ReadBuffer = MmHeapAlloc(ImageSize);
    if (!ReadBuffer)
    {
        /* Fail */
        DbgPrint("Failed to allocate a temporary buffer for the read\n");
        return NULL;
    }

    /* Load the file image */
    if (!FsReadFile(Image, ImageSize, NULL, ReadBuffer))
    {
        /* Fail */
        DbgPrint("Failed to read image: %s\n", Name);
        return NULL;
    }

    /* Map it into virtual memory */
    ImageSize = FrLdrReMapImage(ReadBuffer, LoadBase);

    /* Free the temporary buffer */
    MmHeapFree(ReadBuffer);
    
    /* Calculate Difference between Real Base and Compiled Base*/
    Status = LdrRelocateImageWithBias(LoadBase,
                                      (ULONG_PTR)ImageBase -
                                      (ULONG_PTR)LoadBase,
                                      "FreeLdr",
                                      STATUS_SUCCESS,
                                      STATUS_UNSUCCESSFUL,
                                      STATUS_UNSUCCESSFUL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DbgPrint("Failed to relocate image: %s\n", Name);
        return NULL;
    }
    
    /* Fill out Module Data Structure */
    reactos_modules[ImageId].ModStart = (ULONG_PTR)ImageBase;
    reactos_modules[ImageId].ModEnd = (ULONG_PTR)ImageBase + ImageSize;
    strcpy(reactos_module_strings[ImageId], Name);
    reactos_modules[ImageId].String = (ULONG_PTR)reactos_module_strings[ImageId];
    LoaderBlock.ModsCount++;
    
    /* Detect kernel or HAL */
    if (!_stricmp(Name, "ntoskrnl.exe"))
    {
        KernelData = (PVOID)NextModuleBase;
        KernelSize = ImageSize;
    }
    else if (!_stricmp(Name, "hal.dll"))
    {
        HalData = (PVOID)NextModuleBase;
        HalSize = ImageSize;
    }
    else
    {
        DriverName[Drivers] = reactos_module_strings[ImageId];
        DriverData[Drivers] = (PVOID)NextModuleBase;
        DriverSize[Drivers] = ImageSize;
        Drivers++;
    }
    
    /* Increase the next Load Base */
    NextModuleBase = ROUND_UP(NextModuleBase + ImageSize, PAGE_SIZE);
    
    /* Perform import fixups */
    if (!NT_SUCCESS(LdrPEFixupImports(LoadBase, Name)))
    {
        /* Fixup failed, just don't include it in the list */
        // NextModuleBase = OldNextModuleBase;
        LoaderBlock.ModsCount = ImageId;
        return NULL;
    }
    
    /* Return the final mapped address */
    return LoadBase;
}

/* EOF */

/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 *
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Jason Filby (jasonfilby@yahoo.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */


/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY ModuleListHead;
KSPIN_LOCK ModuleListLock;
LDR_DATA_TABLE_ENTRY NtoskrnlModuleObject;
LDR_DATA_TABLE_ENTRY HalModuleObject;

/* FUNCTIONS *****************************************************************/

static PVOID
LdrPEGetExportByName (
                      PVOID BaseAddress,
                      PUCHAR SymbolName,
                      USHORT Hint );

static VOID
LdrpBuildModuleBaseName (
                         PUNICODE_STRING BaseName,
                         PUNICODE_STRING FullName )
{
    PWCHAR p;

    DPRINT("LdrpBuildModuleBaseName()\n");
    DPRINT("FullName %wZ\n", FullName);

    p = wcsrchr(FullName->Buffer, L'\\');
    if (p == NULL)
    {
        p = FullName->Buffer;
    }
    else
    {
        p++;
    }

    DPRINT("p %S\n", p);

    RtlInitUnicodeString(BaseName, p);
}


static LONG
LdrpCompareModuleNames (
                        IN PUNICODE_STRING String1,
                        IN PUNICODE_STRING String2 )
{
    ULONG len1, len2, i;
    PWCHAR s1, s2, p;
    WCHAR  c1, c2;

    if (String1 && String2)
    {
        /* Search String1 for last path component */
        len1 = String1->Length / sizeof(WCHAR);
        s1 = String1->Buffer;
        for (i = 0, p = String1->Buffer; i < String1->Length; i = i + sizeof(WCHAR), p++)
        {
            if (*p == L'\\')
            {
                if (i == String1->Length - sizeof(WCHAR))
                {
                    s1 = NULL;
                    len1 = 0;
                }
                else
                {
                    s1 = p + 1;
                    len1 = (String1->Length - i) / sizeof(WCHAR);
                }
            }
        }

        /* Search String2 for last path component */
        len2 = String2->Length / sizeof(WCHAR);
        s2 = String2->Buffer;
        for (i = 0, p = String2->Buffer; i < String2->Length; i = i + sizeof(WCHAR), p++)
        {
            if (*p == L'\\')
            {
                if (i == String2->Length - sizeof(WCHAR))
                {
                    s2 = NULL;
                    len2 = 0;
                }
                else
                {
                    s2 = p + 1;
                    len2 = (String2->Length - i) / sizeof(WCHAR);
                }
            }
        }

        /* Compare last path components */
        if (s1 && s2)
        {
            while (1)
            {
                c1 = len1-- ? RtlUpcaseUnicodeChar (*s1++) : 0;
                c2 = len2-- ? RtlUpcaseUnicodeChar (*s2++) : 0;
                if ((c1 == 0 && c2 == L'.') || (c1 == L'.' && c2 == 0))
                    return(0);
                if (!c1 || !c2 || c1 != c2)
                    return(c1 - c2);
            }
        }
    }

    return(0);
}

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

static NTSTATUS
LdrPEGetOrLoadModule (
                      PWCHAR ModuleName,
                      PCHAR ImportedName,
                      PLDR_DATA_TABLE_ENTRY* ImportedModule)
{
    UNICODE_STRING DriverName;
    UNICODE_STRING NameString;
    WCHAR  NameBuffer[PATH_MAX];
    NTSTATUS Status = STATUS_SUCCESS;

    RtlCreateUnicodeStringFromAsciiz (&DriverName, ImportedName);
    DPRINT("Import module: %wZ\n", &DriverName);

    *ImportedModule = LdrGetModuleObject(&DriverName);
    if (*ImportedModule == NULL)
    {
        PWCHAR PathEnd;
        ULONG PathLength;

        PathEnd = wcsrchr(ModuleName, L'\\');
        if (NULL != PathEnd)
        {
            PathLength = (PathEnd - ModuleName + 1) * sizeof(WCHAR);
            RtlCopyMemory(NameBuffer, ModuleName, PathLength);
            RtlCopyMemory(NameBuffer + (PathLength / sizeof(WCHAR)), DriverName.Buffer, DriverName.Length);
            NameString.Buffer = NameBuffer;
            NameString.MaximumLength = NameString.Length = (USHORT)PathLength + DriverName.Length;

            /* NULL-terminate */
            NameString.MaximumLength += sizeof(WCHAR);
            NameBuffer[NameString.Length / sizeof(WCHAR)] = 0;

            Status = LdrLoadModule(&NameString, ImportedModule);
        }
        else
        {
            DPRINT("Module '%wZ' not loaded yet\n", &DriverName);
            wcscpy(NameBuffer, L"\\SystemRoot\\system32\\drivers\\");
            wcsncat(NameBuffer, DriverName.Buffer, DriverName.Length / sizeof(WCHAR));
            RtlInitUnicodeString(&NameString, NameBuffer);
            Status = LdrLoadModule(&NameString, ImportedModule);
        }
        if (!NT_SUCCESS(Status))
        {
            wcscpy(NameBuffer, L"\\SystemRoot\\system32\\");
            wcsncat(NameBuffer, DriverName.Buffer, DriverName.Length / sizeof(WCHAR));
            RtlInitUnicodeString(&NameString, NameBuffer);
            Status = LdrLoadModule(&NameString, ImportedModule);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Unknown import module: %wZ (Status %lx)\n", &DriverName, Status);
            }
        }
    }
    RtlFreeUnicodeString(&DriverName);
    return Status;
}

static PVOID
LdrPEFixupForward ( PCHAR ForwardName )
{
    CHAR NameBuffer[128];
    UNICODE_STRING ModuleName;
    PCHAR p;
    PLDR_DATA_TABLE_ENTRY ModuleObject;

    DPRINT("LdrPEFixupForward (%s)\n", ForwardName);

    strcpy(NameBuffer, ForwardName);
    p = strchr(NameBuffer, '.');
    if (p == NULL)
    {
        return NULL;
    }

    *p = 0;

    DPRINT("Driver: %s  Function: %s\n", NameBuffer, p+1);

    RtlCreateUnicodeStringFromAsciiz(&ModuleName,
        NameBuffer);
    ModuleObject = LdrGetModuleObject(&ModuleName);
    RtlFreeUnicodeString(&ModuleName);

    DPRINT("ModuleObject: %p\n", ModuleObject);

    if (ModuleObject == NULL)
    {
        DPRINT("LdrPEFixupForward: failed to find module %s\n", NameBuffer);
        return NULL;
    }
    return LdrPEGetExportByName(ModuleObject->DllBase, (PUCHAR)(p+1), 0xffff);
}

static PVOID
LdrPEGetExportByOrdinal (
                         PVOID BaseAddress,
                         ULONG Ordinal )
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    ULONG ExportDirSize;
    PULONG * ExFunctions;
    PVOID Function;

    ExportDir = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData (
        BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &ExportDirSize);

    ExFunctions = (PULONG *)RVA(BaseAddress,
        ExportDir->AddressOfFunctions);
    DPRINT("LdrPEGetExportByOrdinal(Ordinal %d) = %x\n",
        Ordinal,
        RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base]));

    Function = 0 != ExFunctions[Ordinal - ExportDir->Base]
    ? RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] )
        : NULL;

    if (((ULONG_PTR)Function >= (ULONG_PTR)ExportDir) &&
        ((ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize))
    {
        DPRINT("Forward: %s\n", (PCHAR)Function);
        Function = LdrPEFixupForward((PCHAR)Function);
    }

    return Function;
}

static PVOID
LdrPEGetExportByName (
                      PVOID BaseAddress,
                      PUCHAR SymbolName,
                      USHORT Hint )
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    PULONG * ExFunctions;
    PULONG * ExNames;
    USHORT * ExOrdinals;
    PVOID ExName;
    ULONG Ordinal;
    PVOID Function;
    LONG minn, maxn, mid, res;
    ULONG ExportDirSize;

    DPRINT("LdrPEGetExportByName %x %s %hu\n", BaseAddress, SymbolName, Hint);

    ExportDir = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &ExportDirSize);
    if (ExportDir == NULL)
    {
        DPRINT1("LdrPEGetExportByName(): no export directory!\n");
        return NULL;
    }


    /* The symbol names may be missing entirely */
    if (ExportDir->AddressOfNames == 0)
    {
        DPRINT("LdrPEGetExportByName(): symbol names missing entirely\n");
        return NULL;
    }

    /*
    * Get header pointers
    */
    ExNames = (PULONG *)RVA(BaseAddress, ExportDir->AddressOfNames);
    ExOrdinals = (USHORT *)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
    ExFunctions = (PULONG *)RVA(BaseAddress, ExportDir->AddressOfFunctions);

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
                DPRINT("Forward: %s\n", (PCHAR)Function);
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DPRINT1("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }
            if (Function != NULL)
            {
                return Function;
            }
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
                DPRINT("Forward: %s\n", (PCHAR)Function);
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DPRINT1("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
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
    DPRINT1("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
    return (PVOID)NULL;
}

static NTSTATUS
LdrPEProcessImportDirectoryEntry(
                                 PVOID DriverBase,
                                 PLDR_DATA_TABLE_ENTRY ImportedModule,
                                 PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory )
{
    PVOID* ImportAddressList;
    PULONG FunctionNameList;
    ULONG Ordinal;

    if (ImportModuleDirectory == NULL || ImportModuleDirectory->Name == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the import address list. */
    ImportAddressList = (PVOID*)RVA(DriverBase, ImportModuleDirectory->FirstThunk);

    /* Get the list of functions to import. */
    if (ImportModuleDirectory->OriginalFirstThunk != 0)
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->OriginalFirstThunk);
    }
    else
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->FirstThunk);
    }

    /* Walk through function list and fixup addresses. */
    while (*FunctionNameList != 0L)
    {
        if ((*FunctionNameList) & 0x80000000)
        {
            Ordinal = (*FunctionNameList) & 0x7fffffff;
            *ImportAddressList = LdrPEGetExportByOrdinal(ImportedModule->DllBase, Ordinal);
            if ((*ImportAddressList) == NULL)
            {
                DPRINT1("Failed to import #%ld from %wZ\n", Ordinal, &ImportedModule->FullDllName);
                return STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = RVA(DriverBase, *FunctionNameList);
            *ImportAddressList = LdrPEGetExportByName(ImportedModule->DllBase, pe_name->Name, pe_name->Hint);
            if ((*ImportAddressList) == NULL)
            {
                DPRINT1("Failed to import %s from %wZ\n", pe_name->Name, &ImportedModule->FullDllName);
                return STATUS_UNSUCCESSFUL;
            }
        }
        ImportAddressList++;
        FunctionNameList++;
    }
    return STATUS_SUCCESS;
}

typedef struct _LOAD_IMPORTS
{
    SIZE_T Count;
    PLDR_DATA_TABLE_ENTRY Entry[1];
} LOAD_IMPORTS, *PLOAD_IMPORTS;

NTSTATUS
NTAPI
MiResolveImageReferences(IN PVOID ImageBase,
                         IN PUNICODE_STRING ImageFileDirectory,
                         IN PUNICODE_STRING NamePrefix OPTIONAL,
                         OUT PCHAR *MissingApi,
                         OUT PWCHAR *MissingDriver,
                         OUT PLOAD_IMPORTS *LoadImports)
{
    /* We don't do anything for now */
    return STATUS_SUCCESS;
}

static NTSTATUS
LdrPEFixupImports (IN PVOID DllBase,
                   IN PWCHAR DllName)
{
    PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
    PCHAR ImportedName;
    PLDR_DATA_TABLE_ENTRY ImportedModule;
    NTSTATUS Status;
    ULONG Size;

    /*  Process each import module  */
    ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
        RtlImageDirectoryEntryToData(DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_IMPORT,
        &Size);
    DPRINT("Processeing import directory at %p\n", ImportModuleDirectory);
    while (ImportModuleDirectory->Name)
    {
        /*  Check to make sure that import lib is kernel  */
        ImportedName = (PCHAR) DllBase + ImportModuleDirectory->Name;

        Status = LdrPEGetOrLoadModule(DllName, ImportedName, &ImportedModule);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        Status = LdrPEProcessImportDirectoryEntry(DllBase, ImportedModule, ImportModuleDirectory);
        if (!NT_SUCCESS(Status))
        {
            while (TRUE);
            return Status;
        }

        ImportModuleDirectory++;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
LdrPEProcessModule(
                   PVOID ModuleLoadBase,
                   PUNICODE_STRING FileName,
                   PLDR_DATA_TABLE_ENTRY *ModuleObject )
{
    unsigned int DriverSize, Idx;
    ULONG CurrentSize;
    PVOID DriverBase;
    PIMAGE_DOS_HEADER PEDosHeader;
    PIMAGE_NT_HEADERS PENtHeaders;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    NTSTATUS Status;
    KIRQL Irql;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING BaseName, BaseDirectory, PrefixName;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG EntrySize;
    PLOAD_IMPORTS LoadedImports = (PVOID)-2;
    PWSTR NameBuffer;
    PCHAR MissingApiName, Buffer;
    PWCHAR MissingDriverName;

    DPRINT("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

    /*  Get header pointers  */
    PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
    PENtHeaders = RtlImageNtHeader(ModuleLoadBase);
    NtHeader = PENtHeaders;
    PESectionHeaders = IMAGE_FIRST_SECTION(PENtHeaders);


    /*  Check file magic numbers  */
    if (PEDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        DPRINT("Incorrect MZ magic: %04x\n", PEDosHeader->e_magic);
        return STATUS_UNSUCCESSFUL;
    }
    if (PEDosHeader->e_lfanew == 0)
    {
        DPRINT("Invalid lfanew offset: %08x\n", PEDosHeader->e_lfanew);
        return STATUS_UNSUCCESSFUL;
    }
    if (PENtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        DPRINT("Incorrect PE magic: %08x\n", PENtHeaders->Signature);
        return STATUS_UNSUCCESSFUL;
    }
    if (PENtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        DPRINT("Incorrect Architechture: %04x\n", PENtHeaders->FileHeader.Machine);
        return STATUS_UNSUCCESSFUL;
    }


    /* FIXME: if image is fixed-address load, then fail  */

    /* FIXME: check/verify OS version number  */

    DPRINT("OptionalHdrMagic:%04x LinkVersion:%d.%d\n",
        PENtHeaders->OptionalHeader.Magic,
        PENtHeaders->OptionalHeader.MajorLinkerVersion,
        PENtHeaders->OptionalHeader.MinorLinkerVersion);
    DPRINT("Entry Point:%08lx\n", PENtHeaders->OptionalHeader.AddressOfEntryPoint);

    /*  Determine the size of the module  */
    DriverSize = 0;
    for (Idx = 0; Idx < PENtHeaders->FileHeader.NumberOfSections; Idx++)
    {
        if (!(PESectionHeaders[Idx].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
        {
            CurrentSize = PESectionHeaders[Idx].VirtualAddress + PESectionHeaders[Idx].Misc.VirtualSize;
            DriverSize = max(DriverSize, CurrentSize);
        }
    }
    DriverSize = ROUND_UP(DriverSize, PENtHeaders->OptionalHeader.SectionAlignment);
    DPRINT("DriverSize %x, SizeOfImage %x\n",DriverSize, PENtHeaders->OptionalHeader.SizeOfImage);

    /*  Allocate a virtual section for the module  */
    DriverBase = NULL;
    DriverBase = MmAllocateSection(DriverSize, DriverBase);
    if (DriverBase == 0)
    {
        DPRINT("Failed to allocate a virtual section for driver\n");
        return STATUS_UNSUCCESSFUL;
    }
    DPRINT("DriverBase for %wZ: %x\n", FileName, DriverBase);

    /*  Copy headers over */
    memcpy(DriverBase, ModuleLoadBase, PENtHeaders->OptionalHeader.SizeOfHeaders);

    /*  Copy image sections into virtual section  */
    for (Idx = 0; Idx < PENtHeaders->FileHeader.NumberOfSections; Idx++)
    {
        CurrentSize = PESectionHeaders[Idx].VirtualAddress + PESectionHeaders[Idx].Misc.VirtualSize;
        /* Copy current section into current offset of virtual section */
        if (CurrentSize <= DriverSize &&
            PESectionHeaders[Idx].SizeOfRawData)
        {
            DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
                PESectionHeaders[Idx].VirtualAddress + (ULONG_PTR)DriverBase);
            memcpy((PVOID)((ULONG_PTR)DriverBase + PESectionHeaders[Idx].VirtualAddress),
                (PVOID)((ULONG_PTR)ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
                PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData
                ? PESectionHeaders[Idx].SizeOfRawData : PESectionHeaders[Idx].Misc.VirtualSize );
        }
    }

    /* Relocate the driver */
    Status = LdrRelocateImageWithBias(DriverBase,
                                      0,
                                      "SYSLDR",
                                      STATUS_SUCCESS,
                                      STATUS_CONFLICTING_ADDRESSES,
                                      STATUS_INVALID_IMAGE_FORMAT);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        return Status;
    }

    /* Allocate a buffer we'll use for names */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, MAX_PATH, TAG_LDR_WSTR);
    if (!Buffer)
    {
        /* Fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

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
        BaseName.Length = BaseLength;
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

    /* And the prefix */
    PrefixName = *FileName;

    /* Calculate the size we'll need for the entry and allocate it */
    EntrySize = sizeof(LDR_DATA_TABLE_ENTRY) +
                BaseName.Length +
                sizeof(UNICODE_NULL);

    /* Allocate the entry */
    LdrEntry = ExAllocatePoolWithTag(NonPagedPool, EntrySize, TAG_MODULE_OBJECT);
    if (!LdrEntry)
    {
        /* Fail */
        return STATUS_INSUFFICIENT_RESOURCES;
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

    /* We'll put the name after the entry */
    NameBuffer = (PVOID)(LdrEntry + 1);

    /* Setup the rest of the entry */
    LdrEntry->DllBase = DriverBase;
    LdrEntry->EntryPoint = (PVOID)((ULONG_PTR)DriverBase +
                                   NtHeader->OptionalHeader.AddressOfEntryPoint);
    LdrEntry->SizeOfImage = DriverSize;
    LdrEntry->CheckSum = NtHeader->OptionalHeader.CheckSum;
    LdrEntry->SectionPointer = NULL; // FIXME

    /* Now write the DLL name */
    LdrEntry->BaseDllName.Buffer = NameBuffer;
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

    /* Insert the entry */
    KeAcquireSpinLock(&ModuleListLock, &Irql);
    InsertTailList(&ModuleListHead, &LdrEntry->InLoadOrderLinks);
    KeReleaseSpinLock(&ModuleListLock, Irql);

    /* Resolve imports */
    MissingApiName = Buffer;
    Status = MiResolveImageReferences(DriverBase,
                                      &BaseDirectory,
                                      NULL,
                                      &MissingApiName,
                                      &MissingDriverName,
                                      &LoadedImports);

    /* Resolve imports */
    Status = LdrPEFixupImports(LdrEntry->DllBase,
                               LdrEntry->FullDllName.Buffer);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ExFreePool(LdrEntry->FullDllName.Buffer);
        ExFreePool(LdrEntry);
        return Status;
    }

    /* Return */
    *ModuleObject = LdrEntry;
    return STATUS_SUCCESS;
}

VOID
INIT_FUNCTION
NTAPI
LdrInit1(VOID)
{
    PLDR_DATA_TABLE_ENTRY HalModuleObject, NtoskrnlModuleObject, LdrEntry;

    /* Initialize the module list and spinlock */
    InitializeListHead(&ModuleListHead);
    KeInitializeSpinLock(&ModuleListLock);

    /* Get the NTOSKRNL Entry from the loader */
    LdrEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    /* Initialize ModuleObject for NTOSKRNL */
    NtoskrnlModuleObject = ExAllocatePoolWithTag(PagedPool,
                                                 sizeof(LDR_DATA_TABLE_ENTRY),
                                                 TAG('M', 'm', 'L', 'd'));
    NtoskrnlModuleObject->DllBase = LdrEntry->DllBase;
    RtlInitUnicodeString(&NtoskrnlModuleObject->FullDllName, KERNEL_MODULE_NAME);
    LdrpBuildModuleBaseName(&NtoskrnlModuleObject->BaseDllName, &NtoskrnlModuleObject->FullDllName);
    NtoskrnlModuleObject->EntryPoint = LdrEntry->EntryPoint;
    NtoskrnlModuleObject->SizeOfImage = LdrEntry->SizeOfImage;

    /* Insert it into the list */
    InsertTailList(&ModuleListHead, &NtoskrnlModuleObject->InLoadOrderLinks);

    /* Get the HAL Entry from the loader */
    LdrEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink->Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    /* Initialize ModuleObject for HAL */
    HalModuleObject = ExAllocatePoolWithTag(PagedPool,
                                                 sizeof(LDR_DATA_TABLE_ENTRY),
                                                 TAG('M', 'm', 'L', 'd'));
    HalModuleObject->DllBase = LdrEntry->DllBase;
    RtlInitUnicodeString(&HalModuleObject->FullDllName, HAL_MODULE_NAME);
    LdrpBuildModuleBaseName(&HalModuleObject->BaseDllName, &HalModuleObject->FullDllName);
    HalModuleObject->EntryPoint = LdrEntry->EntryPoint;
    HalModuleObject->SizeOfImage = LdrEntry->SizeOfImage;

    /* Insert it into the list */
    InsertTailList(&ModuleListHead, &HalModuleObject->InLoadOrderLinks);

    /* Hook for KDB on initialization of the loader. */
    KDB_LOADERINIT_HOOK(NtoskrnlModuleObject, HalModuleObject);
}

//
// Used for checking if a module is already in the module list.
// Used during loading/unloading drivers.
//
PLDR_DATA_TABLE_ENTRY
NTAPI
LdrGetModuleObject ( PUNICODE_STRING ModuleName )
{
    PLDR_DATA_TABLE_ENTRY Module;
    PLIST_ENTRY Entry;
    KIRQL Irql;

    DPRINT("LdrGetModuleObject(%wZ) called\n", ModuleName);

    KeAcquireSpinLock(&ModuleListLock,&Irql);

    Entry = ModuleListHead.Flink;
    while (Entry != &ModuleListHead)
    {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        DPRINT("Comparing %wZ and %wZ\n",
            &Module->BaseDllName,
            ModuleName);

        if (!LdrpCompareModuleNames(&Module->BaseDllName, ModuleName))
        {
            DPRINT("Module %wZ\n", &Module->BaseDllName);
            KeReleaseSpinLock(&ModuleListLock, Irql);
            return(Module);
        }

        Entry = Entry->Flink;
    }

    KeReleaseSpinLock(&ModuleListLock, Irql);

    DPRINT("Could not find module '%wZ'\n", ModuleName);

    return(NULL);
}

//
// Used when unloading drivers
//
NTSTATUS
NTAPI
LdrUnloadModule ( PLDR_DATA_TABLE_ENTRY ModuleObject )
{
    KIRQL Irql;

    /* Remove the module from the module list */
    KeAcquireSpinLock(&ModuleListLock,&Irql);
    RemoveEntryList(&ModuleObject->InLoadOrderLinks);
    KeReleaseSpinLock(&ModuleListLock, Irql);

    /* Hook for KDB on unloading a driver. */
    KDB_UNLOADDRIVER_HOOK(ModuleObject);

    /* Free module section */
    //  MmFreeSection(ModuleObject->DllBase);

    ExFreePool(ModuleObject->FullDllName.Buffer);
    ExFreePool(ModuleObject);

    return(STATUS_SUCCESS);
}

//
// Used for images already loaded (boot drivers)
//
NTSTATUS
LdrProcessModule(
    PVOID ModuleLoadBase,
    PUNICODE_STRING ModuleName,
    PLDR_DATA_TABLE_ENTRY *ModuleObject )
{
    PIMAGE_DOS_HEADER PEDosHeader;

    /*  If MZ header exists  */
    PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
    if (PEDosHeader->e_magic == IMAGE_DOS_SIGNATURE && PEDosHeader->e_lfanew != 0L)
    {
        return LdrPEProcessModule(ModuleLoadBase,
            ModuleName,
            ModuleObject);
    }

    DPRINT("Module wasn't PE\n");
    return STATUS_UNSUCCESSFUL;
}

//
// Used by NtLoadDriver/IoMgr
//
NTSTATUS
NTAPI
LdrLoadModule(
              PUNICODE_STRING Filename,
              PLDR_DATA_TABLE_ENTRY *ModuleObject )
{
    PVOID ModuleLoadBase;
    NTSTATUS Status;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PLDR_DATA_TABLE_ENTRY Module;
    FILE_STANDARD_INFORMATION FileStdInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    *ModuleObject = NULL;

    DPRINT("Loading Module %wZ...\n", Filename);

    /*  Open the Module  */
    InitializeObjectAttributes(&ObjectAttributes,
        Filename,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = ZwOpenFile(&FileHandle,
        GENERIC_READ,
        &ObjectAttributes,
        &IoStatusBlock,
        FILE_SHARE_READ,
        FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not open module file: %wZ (Status 0x%08lx)\n", Filename, Status);
        return(Status);
    }


    /*  Get the size of the file  */
    Status = ZwQueryInformationFile(FileHandle,
        &IoStatusBlock,
        &FileStdInfo,
        sizeof(FileStdInfo),
        FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not get file size\n");
        NtClose(FileHandle);
        return(Status);
    }


    /*  Allocate nonpageable memory for driver  */
    ModuleLoadBase = ExAllocatePoolWithTag(NonPagedPool,
        FileStdInfo.EndOfFile.u.LowPart,
        TAG_DRIVER_MEM);
    if (ModuleLoadBase == NULL)
    {
        DPRINT("Could not allocate memory for module");
        NtClose(FileHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    /*  Load driver into memory chunk  */
    Status = ZwReadFile(FileHandle,
        0, 0, 0,
        &IoStatusBlock,
        ModuleLoadBase,
        FileStdInfo.EndOfFile.u.LowPart,
        0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not read module file into memory");
        ExFreePool(ModuleLoadBase);
        NtClose(FileHandle);
        return(Status);
    }


    ZwClose(FileHandle);

    Status = LdrProcessModule(ModuleLoadBase,
        Filename,
        &Module);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not process module\n");
        ExFreePool(ModuleLoadBase);
        return(Status);
    }

    /*  Cleanup  */
    ExFreePool(ModuleLoadBase);

    *ModuleObject = Module;

    /* Hook for KDB on loading a driver. */
    KDB_LOADDRIVER_HOOK(Filename, Module);

    return(STATUS_SUCCESS);
}

/* EOF */

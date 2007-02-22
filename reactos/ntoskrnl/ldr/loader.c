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

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
MiResolveImageReferences(IN PVOID ImageBase,
                         IN PUNICODE_STRING ImageFileDirectory,
                         IN PUNICODE_STRING NamePrefix OPTIONAL,
                         OUT PCHAR *MissingApi,
                         OUT PWCHAR *MissingDriver,
                         OUT PLOAD_IMPORTS *LoadImports);

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

extern KSPIN_LOCK PsLoadedModuleSpinLock;

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

    KeAcquireSpinLock(&PsLoadedModuleSpinLock,&Irql);

    Entry = PsLoadedModuleList.Flink;
    while (Entry != &PsLoadedModuleList)
    {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        DPRINT("Comparing %wZ and %wZ\n",
            &Module->BaseDllName,
            ModuleName);

        if (!LdrpCompareModuleNames(&Module->BaseDllName, ModuleName))
        {
            DPRINT("Module %wZ\n", &Module->BaseDllName);
            KeReleaseSpinLock(&PsLoadedModuleSpinLock, Irql);
            return(Module);
        }

        Entry = Entry->Flink;
    }

    KeReleaseSpinLock(&PsLoadedModuleSpinLock, Irql);

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
    KeAcquireSpinLock(&PsLoadedModuleSpinLock,&Irql);
    RemoveEntryList(&ModuleObject->InLoadOrderLinks);
    KeReleaseSpinLock(&PsLoadedModuleSpinLock, Irql);

    /* Hook for KDB on unloading a driver. */
    KDB_UNLOADDRIVER_HOOK(ModuleObject);

    /* Free module section */
    //  MmFreeSection(ModuleObject->DllBase);

    ExFreePool(ModuleObject->FullDllName.Buffer);
    ExFreePool(ModuleObject);

    return(STATUS_SUCCESS);
}

//
// Used by NtLoadDriver/IoMgr
//
NTSTATUS
NTAPI
MmLoadSystemImage(IN PUNICODE_STRING FileName,
                  IN PUNICODE_STRING NamePrefix OPTIONAL,
                  IN PUNICODE_STRING LoadedName OPTIONAL,
                  IN ULONG Flags,
                  OUT PVOID *ModuleObject,
                  OUT PVOID *ImageBaseAddress)
{
    PVOID ModuleLoadBase;
    NTSTATUS Status;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PLDR_DATA_TABLE_ENTRY Module;
    FILE_STANDARD_INFORMATION FileStdInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    unsigned int DriverSize, Idx;
    ULONG CurrentSize;
    PVOID DriverBase;
    PIMAGE_DOS_HEADER PEDosHeader;
    PIMAGE_NT_HEADERS PENtHeaders;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    KIRQL Irql;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING BaseName, BaseDirectory, PrefixName;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG EntrySize;
    PLOAD_IMPORTS LoadedImports = (PVOID)-2;
    PCHAR MissingApiName, Buffer;
    PWCHAR MissingDriverName;

    if (ModuleObject) *ModuleObject = NULL;
    if (ImageBaseAddress) *ImageBaseAddress = NULL;

    DPRINT("Loading Module %wZ...\n", FileName);

    /*  Open the Module  */
    InitializeObjectAttributes(&ObjectAttributes,
        FileName,
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
        DPRINT("Could not open module file: %wZ (Status 0x%08lx)\n", FileName, Status);
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

    /* Setup the rest of the entry */
    LdrEntry->DllBase = DriverBase;
    LdrEntry->EntryPoint = (PVOID)((ULONG_PTR)DriverBase +
                                   NtHeader->OptionalHeader.AddressOfEntryPoint);
    LdrEntry->SizeOfImage = DriverSize;
    LdrEntry->CheckSum = NtHeader->OptionalHeader.CheckSum;
    LdrEntry->SectionPointer = LdrEntry;

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

    /* Insert the entry */
    KeAcquireSpinLock(&PsLoadedModuleSpinLock, &Irql);
    InsertTailList(&PsLoadedModuleList, &LdrEntry->InLoadOrderLinks);
    KeReleaseSpinLock(&PsLoadedModuleSpinLock, Irql);

    /* Resolve imports */
    MissingApiName = Buffer;
    Status = MiResolveImageReferences(DriverBase,
                                      &BaseDirectory,
                                      NULL,
                                      &MissingApiName,
                                      &MissingDriverName,
                                      &LoadedImports);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ExFreePool(LdrEntry->FullDllName.Buffer);
        ExFreePool(LdrEntry);
        return Status;
    }

    /* Return */
    Module = LdrEntry;

    /*  Cleanup  */
    ExFreePool(ModuleLoadBase);

    if (ModuleObject) *ModuleObject = Module;
    if (ImageBaseAddress) *ImageBaseAddress = Module->DllBase;

    /* Hook for KDB on loading a driver. */
    KDB_LOADDRIVER_HOOK(FileName, Module);

    return(STATUS_SUCCESS);
}

/* EOF */

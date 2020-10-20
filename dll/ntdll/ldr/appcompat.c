/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS NT User-Mode Library
* FILE:            dll/ntdll/ldr/appcompat.c
* PURPOSE:         
* PROGRAMMERS:     Timo Kreuzer <timo.kreuzer@reactos.org>
*/

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#include <compat_undoc.h>
#include <compatguid_undoc.h>
#include <roscompat.h>

#define NDEBUG
#include <debug.h>

static DWORD g_CompatVersion = REACTOS_COMPATVERSION_UNINITIALIZED;


DWORD
NTAPI
RosGetProcessCompatVersion(VOID)
{
    if (g_CompatVersion == REACTOS_COMPATVERSION_UNINITIALIZED)
    {
        ReactOS_ShimData* pShimData = (ReactOS_ShimData*)NtCurrentPeb()->pShimData;
        if (pShimData && pShimData->dwMagic == REACTOS_SHIMDATA_MAGIC &&
            pShimData->dwSize == sizeof(ReactOS_ShimData))
        {
            g_CompatVersion = pShimData->dwRosProcessCompatVersion;
            DPRINT("roscompat: Initializing version to 0x%x\n", g_CompatVersion);
        }
        else
        {
            g_CompatVersion = 0;
            DPRINT("roscompat: No version set\n");
        }
    }
    return g_CompatVersion < REACTOS_COMPATVERSION_UNINITIALIZED ? g_CompatVersion : 0;
}

static
ROSCOMPAT_VERSION_BIT
TranslateAppcompatVersionToVersionBit(
    _In_ DWORD AppcompatVersion)
{
    switch (AppcompatVersion)
    {
        case _WIN32_WINNT_NT4:
            return ROSCOMPAT_VERSION_BIT_NT4;
        case _WIN32_WINNT_WIN2K:
            return ROSCOMPAT_VERSION_BIT_WIN2K;
        case _WIN32_WINNT_WINXP:
            return ROSCOMPAT_VERSION_BIT_WINXP;
        case _WIN32_WINNT_WS03:
            return ROSCOMPAT_VERSION_BIT_WS03;
        case _WIN32_WINNT_VISTA:
            return ROSCOMPAT_VERSION_BIT_VISTA;
        case _WIN32_WINNT_WIN7:
            return ROSCOMPAT_VERSION_BIT_WIN7;
        case _WIN32_WINNT_WIN8:
            return ROSCOMPAT_VERSION_BIT_WIN8;
        case _WIN32_WINNT_WINBLUE:
            return ROSCOMPAT_VERSION_BIT_WIN81;
        case _WIN32_WINNT_WIN10:
            return ROSCOMPAT_VERSION_BIT_WIN10;
        default:
            ASSERT(FALSE);
    }

    return ROSCOMPAT_VERSION_BIT_WS03;
}

PIMAGE_SECTION_HEADER
LdrpFindSectionByName(
    PVOID ImageBase,
    PSTR SectionName)
{
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeaders;
    SIZE_T NameLength;
    ULONG i;

    /* Get the NT headers */
    NtHeaders = RtlImageNtHeader(ImageBase);
    if (NtHeaders == NULL)
    {
        return NULL;
    }

    SectionHeaders = IMAGE_FIRST_SECTION(NtHeaders);

    /* Check for long section name */
    NameLength = strlen(SectionName);
    if (NameLength <= IMAGE_SIZEOF_SHORT_NAME)
    {
        /* Loop the sections until we found the requested one */
        for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
        {
            /* Directly compare names */
            if (strncmp((char*)SectionHeaders[i].Name, SectionName, IMAGE_SIZEOF_SHORT_NAME) == 0)
            {
                return &SectionHeaders[i];
            }
        }
    }
    else
    {
        // FIXME: support long section names
        ASSERT(FALSE);
    }

    return NULL;
}

PVOID
LdrpSectionHeaderToVAAndSize(
    _In_ PVOID ImageBase,
    _In_ PIMAGE_SECTION_HEADER SectionHeader,
    _Out_ PSIZE_T SectionSize)
{
    *SectionSize = SectionHeader->SizeOfRawData;
    return (PVOID)((PUCHAR)ImageBase + SectionHeader->VirtualAddress);
}

PROSCOMPAT_DESCRIPTOR
FindRosCompatDescriptor(
    _In_ PVOID ImageBase)
{
    PIMAGE_SECTION_HEADER SectionHeader;

    SectionHeader = LdrpFindSectionByName(ImageBase, ".expvers");
    if (SectionHeader == NULL)
    {
        return NULL;
    }

    return (PROSCOMPAT_DESCRIPTOR)((PUCHAR)ImageBase + SectionHeader->VirtualAddress);
}

#if 0
static
NTSTATUS
GetExportNameVersionTable(
    _In_ PVOID ImageBase,
    _Out_ PULONG *Table,
    _Out_ PULONG NumberOfEntries)
{
    PROSCOMPAT_DESCRIPTOR RosCompatDescriptor;

    RosCompatDescriptor = FindRosCompatDescriptor(ImageBase);
    if (RosCompatDescriptor == NULL)
    {
        return STATUS_NOT_FOUND;
    }

    *NumberOfEntries = RosCompatDescriptor->NumberOfExportNames;
    *Table = RosCompatDescriptor->ExportNameMasks;

    return STATUS_SUCCESS;
}
#endif

__inline
BOOLEAN IsPowerOfTwo(unsigned int x)
{
    return ((x != 0) && !(x & (x - 1)));
}

static
NTSTATUS
LdrpPatchExportNameTable(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ PROSCOMPAT_DESCRIPTOR RosCompatDescriptor,
    _Inout_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_ DWORD AppCompatVersion)
{
    ULONG VersionMask;
    PULONG NameTable, OrgNameTable;
    PUSHORT OrdinalTable, OrgOrdinalTable;
    SIZE_T Size;
    ULONG i, j, k;

    /* Translate to version mask */
    VersionMask = 1 << TranslateAppcompatVersionToVersionBit(AppCompatVersion);
    ASSERT(IsPowerOfTwo(VersionMask));

    /* Get the VA of the name table */
    NameTable = (PULONG)((ULONG_PTR)LdrEntry->DllBase +
        (ULONG_PTR)ExportDirectory->AddressOfNames);

    OrdinalTable = (PUSHORT)((ULONG_PTR)LdrEntry->DllBase +
        (ULONG_PTR)ExportDirectory->AddressOfNameOrdinals);

    /* Allocate a temp buffer for the exports */
    Size = ExportDirectory->NumberOfNames * (sizeof(ULONG) + sizeof(USHORT));
    OrgNameTable = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (OrgNameTable == NULL)
    {
        DPRINT1("ERROR: LdrpPatchExportNameTable: Unable to allocate %u bytes\n", Size);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    OrgOrdinalTable = (PUSHORT)&OrgNameTable[ExportDirectory->NumberOfNames];

    /* Make a copy of the original exports */
    RtlCopyMemory(OrgNameTable, NameTable, ExportDirectory->NumberOfNames * sizeof(ULONG));
    RtlCopyMemory(OrgOrdinalTable, OrdinalTable, ExportDirectory->NumberOfNames * sizeof(USHORT));

    /* Strip unused entries from the name and ordinal table */
    for (i = 0, j = 0, k = 0; i < ExportDirectory->NumberOfNames; i++)
    {
        if (RosCompatDescriptor->ExportNameMasks[i] & VersionMask)
        {
            /* Put public functions into the export table */
            NameTable[j] = OrgNameTable[i];
            OrdinalTable[j] = OrgOrdinalTable[i];
            j++;
        }
        else
        {
            /* Move private functions to the start of the temp buffer */
            OrgNameTable[k] = OrgNameTable[i];
            OrgOrdinalTable[k] = OrgOrdinalTable[i];
            k++;
        }
    }

    /* Finally patch the number of exports */
    DPRINT("roscompat: %wZ from %u to %u exports\n", &LdrEntry->BaseDllName, ExportDirectory->NumberOfNames, j);
    ExportDirectory->NumberOfNames = j;

    /* Copy the private functions back */
    for (i = 0; j < RosCompatDescriptor->NumberOfExportNames; i++, j++)
    {
        NameTable[j] = OrgNameTable[i];
        OrdinalTable[j] = OrgOrdinalTable[i];
    }

    /* Free the copy */
    RtlFreeHeap(RtlGetProcessHeap(), 0, OrgNameTable);

    return STATUS_SUCCESS;
}

static
NTSTATUS
LdrpPatchExportTable(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ PROSCOMPAT_DESCRIPTOR RosCompatDescriptor,
    _In_ DWORD AppCompatVersion
    )
{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory, MagicExportDir;
    ULONG ExportDirectorySize, OldProtect;
    PVOID ProtectAddress;
    SIZE_T ProtectSize;
    NTSTATUS Status;

    /* Get export directory */
    ExportDirectory = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                   &ExportDirectorySize);
    if (ExportDirectory == NULL)
    {
        DPRINT("roscompat: No ExportDirectory for %wZ\n", &LdrEntry->BaseDllName);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    ASSERT(RosCompatDescriptor->NumberOfExportNames == ExportDirectory->NumberOfNames);

    /* Unprotect the export directory */
    ProtectAddress = ExportDirectory;
    ProtectSize = ExportDirectorySize;
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &ProtectAddress,
                                    &ProtectSize,
                                    PAGE_READWRITE,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("roscompat: Unable to unprotect EAT for %wZ (Status 0x%x)\n", &LdrEntry->BaseDllName, Status);
        return Status;
    }

    /* Patch the export name and ordinal table */
    Status = LdrpPatchExportNameTable(LdrEntry,
                                      RosCompatDescriptor,
                                      ExportDirectory,
                                      AppCompatVersion);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("roscompat: Unable to patch EAT, 0x%x\n", Status);
        return Status;
    }

    /* Unprotect the export directory */
    ProtectAddress = ExportDirectory;
    ProtectSize = ExportDirectorySize;
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &ProtectAddress,
                                    &ProtectSize,
                                    OldProtect,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("roscompat: Unable to reprotect EAT for %wZ (Status 0x%x)\n", &LdrEntry->BaseDllName, Status);
        return Status;
    }

    /* Also flush out the cache */
    NtFlushInstructionCache(NtCurrentProcess(), ExportDirectory, ProtectSize);

    /* Setup an export directory for "magic" (i.e. private) exports */
    MagicExportDir = RosCompatDescriptor->MagicExportDir;
    MagicExportDir->Characteristics = 0;
    MagicExportDir->TimeDateStamp = 0xFFFFFFFF;
    MagicExportDir->MajorVersion = AppCompatVersion >> 8;
    MagicExportDir->MinorVersion = AppCompatVersion & 0xFF;
    MagicExportDir->Name = ExportDirectory->Name;
    MagicExportDir->Base = ExportDirectory->Base;
    MagicExportDir->NumberOfFunctions = ExportDirectory->NumberOfFunctions;
    MagicExportDir->NumberOfNames = RosCompatDescriptor->NumberOfExportNames -
        ExportDirectory->NumberOfNames;
    MagicExportDir->AddressOfFunctions = ExportDirectory->AddressOfFunctions;
    MagicExportDir->AddressOfNames = ExportDirectory->AddressOfNames +
        ExportDirectory->NumberOfNames * sizeof(ULONG);
    MagicExportDir->AddressOfNameOrdinals = ExportDirectory->AddressOfNameOrdinals +
        ExportDirectory->NumberOfNames * sizeof(ULONG);

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
LdrpApplyRosCompatMagic(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    DWORD AppCompatVersion;
    PROSCOMPAT_DESCRIPTOR RosCompatDescriptor;
    NTSTATUS Status;

    /* Make sure we have a data table entry */
    ASSERT(LdrEntry != NULL);

    /* Ensure that this field is not used */
    ASSERT(LdrEntry->PatchInformation == NULL);

    /* Get the appcompat descriptor */
    RosCompatDescriptor = FindRosCompatDescriptor(LdrEntry->DllBase);
    if (RosCompatDescriptor == NULL)
    {
        DPRINT("roscompat: No descriptor in %wZ\n", &LdrEntry->BaseDllName);
        return FALSE;
    }

    /* Get the AppCompat version */
    AppCompatVersion = RosGetProcessCompatVersion();
    if (AppCompatVersion == 0)
    {
        /* Default to WS 2003 */
        AppCompatVersion = _WIN32_WINNT_WS03;
    }
    else
    {
        /* Patch the PEB */
        PPEB Peb = NtCurrentPeb();
        Peb->OSMajorVersion = AppCompatVersion >> 8;
        Peb->OSMinorVersion = AppCompatVersion & 0xFF;
    }

    DPRINT("roscompat: Patching eat of %wZ for 0x%x\n", &LdrEntry->BaseDllName, AppCompatVersion);

    /* Save the descriptor in the PatchInformation field, which is otherwise
       unused in user-mode */
    LdrEntry->PatchInformation = RosCompatDescriptor;

    /* Now patch the export table */
    Status = LdrpPatchExportTable(LdrEntry,
                                  RosCompatDescriptor,
                                  AppCompatVersion);
    if (!NT_SUCCESS(Status))
    {
        __debugbreak();
        return FALSE;
    }

    return TRUE;
}

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

static USHORT ExportVersionMask;

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

VOID
NTAPI
LdrpApplyDllExportVersioning(
    _Inout_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PROSCOMPAT_DESCRIPTOR RosCompatDescriptor;

    /* Make sure we have a data table entry */
    ASSERT(LdrEntry != NULL);

    /* Ensure that this field is not used */
    ASSERT(LdrEntry->PatchInformation == NULL);

    /* Get the appcompat descriptor */
    RosCompatDescriptor = FindRosCompatDescriptor(LdrEntry->DllBase);
    if (RosCompatDescriptor == NULL)
    {
        DPRINT1("roscompat: No descriptor in %wZ\n", &LdrEntry->BaseDllName);
        return;
    }

    /* Save the descriptor */
    LdrEntry->PatchInformation = RosCompatDescriptor;

    /* Check for dummy descriptor */
    if (RosCompatDescriptor->ExportMasks == NULL)
    {
        *RosCompatDescriptor->NumberOfValidExports = MAXULONG;
        return;
    }

    /* Count the number of valid (public) exports */
    ULONG NumberOfValidExports = 0;
    for (ULONG i = 0; i < RosCompatDescriptor->NumberOfOrdinals; i++)
    {
        if (RosCompatDescriptor->ExportMasks[i] & ExportVersionMask)
        {
            NumberOfValidExports++;
        }
    }

    *RosCompatDescriptor->NumberOfValidExports = NumberOfValidExports;

    DPRINT1("roscompat: Applied export version info for '%wZ'\n",
           &LdrEntry->BaseDllName);
}

VOID
NTAPI
LdrpInitializeExportVersioning(
    _Inout_ PLDR_DATA_TABLE_ENTRY ImageLdrEntry,
    _Inout_ PLDR_DATA_TABLE_ENTRY NtdllLdrEntry)
{
    DWORD AppCompatVersion;

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

        DPRINT1("roscompat: Using AppCompat version %lu for image %wZ\n",
                AppCompatVersion,
                &ImageLdrEntry->BaseDllName);
    }

    /* Set the global export version mask */
    ExportVersionMask = 1 << TranslateAppcompatVersionToVersionBit(AppCompatVersion);

    LdrpApplyDllExportVersioning(ImageLdrEntry);
    LdrpApplyDllExportVersioning(NtdllLdrEntry);
}

BOOLEAN
NTAPI
LdrpValidateVersionedExport(
    _In_ const LDR_DATA_TABLE_ENTRY* LdrEntry,
    _In_ ULONG OrdinalIndex)
{
    PROSCOMPAT_DESCRIPTOR RosCompatDescriptor = LdrEntry->PatchInformation;

    /* Check for dummy descriptor */
    if (RosCompatDescriptor->ExportMasks == NULL)
    {
        return TRUE;
    }

    if (OrdinalIndex >= RosCompatDescriptor->NumberOfOrdinals)
    {
        DPRINT1("roscompat: Invalid ordinal %lu for '%wZ'\n",
                OrdinalIndex,
                &LdrEntry->BaseDllName);
        return FALSE;
    }

    if ((RosCompatDescriptor->ExportMasks[OrdinalIndex] & ExportVersionMask) == 0)
    {
        DPRINT1("roscompat: Export %lu is not valid for '%wZ'\n",
                OrdinalIndex,
                &LdrEntry->BaseDllName);
        return FALSE;
    }

    return TRUE;
}

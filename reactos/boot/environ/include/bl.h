/*
* COPYRIGHT:       See COPYING.ARM in the top level directory
* PROJECT:         ReactOS UEFI Boot Library
* FILE:            boot/environ/include/bl.h
* PURPOSE:         Main Boot Library Header
* PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

#ifndef _BL_H
#define _BL_H

/* DATA STRUCTURES ***********************************************************/

typedef struct _BL_LIBRARY_PARAMETERS
{
    ULONG LibraryFlags;
    ULONG TranslationType;
    ULONG MinimumAllocationCount;
    ULONG MinimumHeapSize;
    ULONG HeapAllocationAttributes;
    PWCHAR ApplicationBaseDirectory;
    ULONG DescriptorCount;
} BL_LIBRARY_PARAMETERS, *PBL_LIBRARY_PARAMETERS;

/* This should eventually go into a more public header */
typedef struct _BOOT_APPLICATION_PARAMETER_BLOCK
{
    ULONG Signature[2];
    ULONG Version;
    ULONG Size;
    ULONG ImageType;
    ULONG MemoryTranslationType;
    ULONGLONG ImageBase;
    ULONG ImageSize;
    ULONG MemorySettingsOffset;
    ULONG AppEntryOffset;
    ULONG BootDeviceOffset;
    ULONG FirmwareParametersOffset;
    ULONG FlagOffset;
} BOOT_APPLICATION_PARAMETER_BLOCK, *PBOOT_APPLICATION_PARAMETER_BLOCK;

/* INITIALIZATION ROUTINES ***************************************************/

NTSTATUS
BlInitializeLibrary(
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootAppParameters,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    );

/* UTILITY ROUTINES **********************************************************/

EFI_STATUS
EfiGetEfiStatusCode(
    _In_ NTSTATUS Status
    );

#endif

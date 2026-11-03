/*
 * PROJECT:     ReactOS NT User-Mode Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Support for .NET (CLR) images in the loader
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 *              Copyright 2026 Abdias Joel Moya Perez <abdi.moya@gmail.com>
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

/* Optional MSCOREE handoff */
static PVOID LdrpCorMscoreeHandle;
static PVOID LdrpCorEncodedDllMain;
static PVOID LdrpCorEncodedImageUnloading;
static BOOLEAN LdrpCorMscoreeAttempted;

typedef VOID (WINAPI *PFN_CorImageUnloading)(PVOID);

/* Start of the metadata that the COM descriptor points to. The version string follows it. */
typedef struct _LDRP_COR_METADATA_ROOT
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Reserved;
    ULONG VersionLength;
} LDRP_COR_METADATA_ROOT, *PLDRP_COR_METADATA_ROOT;
C_ASSERT(sizeof(LDRP_COR_METADATA_ROOT) == 16);

#define LDRP_COR_METADATA_SIGNATURE 0x424A5342 /* "BSJB" */

/* FUNCTIONS ****************************************************************/

/**
 * @brief
 * Looks up an export of mscoree.dll and encodes its address for storage.
 *
 * @param[in] Base
 * Base address of mscoree.dll.
 *
 * @param[in] ExportName
 * Name of the export.
 *
 * @return
 * The encoded address of the export, or NULL if mscoree.dll does not export it.
 */
static
PVOID
LdrpCorGetEncodedExport(
    _In_ PVOID Base,
    _In_ PCSTR ExportName)
{
    ANSI_STRING Name;
    PVOID Address;

    RtlInitAnsiString(&Name, ExportName);
    if (!NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &Address)))
        return NULL;

    return RtlEncodeSystemPointer(Address);
}

/**
 * @brief
 * Loads mscoree.dll from the system directory on the first call and resolves the CLR entry
 * points that the loader uses.
 *
 * @return
 * STATUS_SUCCESS if mscoree.dll is loaded, an error status if it could not be loaded.
 * A failed attempt is remembered and not repeated.
 */
NTSTATUS
NTAPI
LdrpCorEnsureMscoreeLoaded(VOID)
{
    WCHAR DllPathBuffer[MAX_PATH + 10];
    UNICODE_STRING DllPath, Mscoree;
    PVOID Base = NULL;
    NTSTATUS Status;

    if (LdrpCorMscoreeAttempted)
        return LdrpCorMscoreeHandle ? STATUS_SUCCESS : STATUS_DLL_NOT_FOUND;

    LdrpCorMscoreeAttempted = TRUE;

    /* Search the system directory only, not the directories that the application controls */
    RtlInitEmptyUnicodeString(&DllPath, DllPathBuffer, sizeof(DllPathBuffer));
    Status = RtlAppendUnicodeToString(&DllPath, SharedUserData->NtSystemRoot);
    if (NT_SUCCESS(Status))
        Status = RtlAppendUnicodeToString(&DllPath, L"\\System32");
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&Mscoree, L"mscoree.dll");
    Status = LdrLoadDll(DllPath.Buffer, NULL, &Mscoree, &Base);
    if (!NT_SUCCESS(Status))
        return Status;

    LdrpCorMscoreeHandle = Base;

    /* Resolve optional exports */
    LdrpCorEncodedDllMain = LdrpCorGetEncodedExport(Base, "_CorDllMain");
    LdrpCorEncodedImageUnloading = LdrpCorGetEncodedExport(Base, "_CorImageUnloading");

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Returns mscoree's _CorDllMain, which is the entry point of IL-only images.
 *
 * @return
 * The address of _CorDllMain, or NULL if mscoree.dll is not loaded or does not export it.
 */
PVOID
NTAPI
LdrpCorGetCorDllMain(VOID)
{
    if (!LdrpCorMscoreeHandle || !LdrpCorEncodedDllMain)
        return NULL;

    return RtlDecodeSystemPointer(LdrpCorEncodedDllMain);
}

/**
 * @brief
 * Tells the CLR that a .NET image is about to be unmapped.
 *
 * This calls mscoree's _CorImageUnloading, but only if mscoree.dll is already loaded, so that
 * unloading an image never loads it.
 *
 * @param[in] ImageBase
 * Base address of the image that is being unloaded.
 */
VOID
NTAPI
LdrpCorImageUnloading(
    _In_ PVOID ImageBase)
{
    PFN_CorImageUnloading ImageUnloading;

    if (!LdrpCorMscoreeHandle || !LdrpCorEncodedImageUnloading)
        return;

    ImageUnloading = (PFN_CorImageUnloading)RtlDecodeSystemPointer(LdrpCorEncodedImageUnloading);
    ImageUnloading(ImageBase);
}

/**
 * @brief
 * Tells whether a CLR image contains only IL and no native code.
 *
 * @param[in] BaseAddress
 * Base address of the mapped image. It must have a COM descriptor.
 *
 * @return
 * TRUE if the COR header has the IL-only flag set, FALSE otherwise.
 */
BOOLEAN
NTAPI
LdrpIsILOnlyImage(
    _In_ PVOID BaseAddress)
{
    ULONG Cor20HeaderSize;
    PIMAGE_COR20_HEADER Cor20Header;

    Cor20Header = RtlImageDirectoryEntryToData(BaseAddress,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                               &Cor20HeaderSize);

    return Cor20Header != NULL &&
           Cor20HeaderSize >= sizeof(IMAGE_COR20_HEADER) &&
           (Cor20Header->Flags & COMIMAGE_FLAGS_ILONLY) != 0;
}

/**
 * @brief
 * Tells whether a range lies inside an image.
 *
 * @param[in] Rva
 * Relative virtual address of the start of the range.
 *
 * @param[in] Size
 * Size of the range in bytes.
 *
 * @param[in] ImageSize
 * Size of the image in bytes.
 *
 * @return
 * TRUE if the range from Rva to Rva + Size is inside the image, FALSE otherwise.
 * The sum is done in 64 bits, so it cannot overflow.
 */
static
BOOLEAN
LdrpCorRangeInImage(
    _In_ ULONG Rva,
    _In_ ULONG Size,
    _In_ ULONG ImageSize)
{
    return ((ULONG64)Rva + Size) <= ImageSize;
}

/**
 * @brief
 * Validates an image with a COM descriptor without help from mscoree.
 *
 * The image is not trusted, so every RVA is checked against SizeOfImage before it is read.
 * Images with a TLS directory are refused. The COM descriptor must lie inside the image, be at
 * least as large as IMAGE_COR20_HEADER and declare runtime version 2 or later. The metadata must
 * start with the BSJB signature and have a version string that fits.
 *
 * @param[in] ImageBase
 * Base address of the mapped image. It must have a COM descriptor.
 *
 * @param[in] CorHeader
 * The COM descriptor of the image, as RtlImageDirectoryEntryToData returns it.
 *
 * @param[in] CorHeaderSize
 * The size of the COM descriptor, as RtlImageDirectoryEntryToData returns it.
 *
 * @return
 * STATUS_SUCCESS if the image is acceptable, STATUS_INVALID_IMAGE_FORMAT otherwise.
 */
NTSTATUS
NTAPI
LdrpCorValidateImage(
    _In_ PVOID ImageBase,
    _In_ PIMAGE_COR20_HEADER CorHeader,
    _In_ ULONG CorHeaderSize)
{
    PLDRP_COR_METADATA_ROOT Root;
    ULONG CorHeaderRva, ImageSize, Size;

    if (RtlImageDirectoryEntryToData(ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &Size))
        return STATUS_INVALID_IMAGE_FORMAT;

    CorHeaderRva = (ULONG)((PUCHAR)CorHeader - (PUCHAR)ImageBase);
    ImageSize = RtlImageNtHeader(ImageBase)->OptionalHeader.SizeOfImage;
    if (CorHeaderSize < sizeof(IMAGE_COR20_HEADER) ||
        !LdrpCorRangeInImage(CorHeaderRva, CorHeaderSize, ImageSize))
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (CorHeader->cb < sizeof(IMAGE_COR20_HEADER) ||
        CorHeader->MajorRuntimeVersion < COR_VERSION_MAJOR_V2)
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    /* The metadata root, its version string, and 2 bytes each of flags and stream count */
    if (CorHeader->MetaData.Size < sizeof(*Root) + 4 ||
        !LdrpCorRangeInImage(CorHeader->MetaData.VirtualAddress,
                             CorHeader->MetaData.Size,
                             ImageSize))
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    Root = (PLDRP_COR_METADATA_ROOT)((PUCHAR)ImageBase + CorHeader->MetaData.VirtualAddress);
    if (Root->Signature != LDRP_COR_METADATA_SIGNATURE ||
        Root->VersionLength > CorHeader->MetaData.Size - sizeof(*Root) - 4)
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    return STATUS_SUCCESS;
}

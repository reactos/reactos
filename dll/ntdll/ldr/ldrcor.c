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
static PVOID LdrpCorEncodedValidateImage;
static PVOID LdrpCorEncodedDllMain;
static PVOID LdrpCorEncodedImageUnloading;
static BOOLEAN LdrpCorMscoreeAttempted;

typedef HRESULT (WINAPI *PFN_CorValidateImage)(PVOID* ImageBase, LPCWSTR ImageName);
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
 * Loads mscoree.dll on the first call and resolves the CLR entry points that the loader uses.
 *
 * @return
 * STATUS_SUCCESS if mscoree.dll is loaded, an error status if it could not be loaded.
 * A failed attempt is remembered and not repeated.
 */
static
NTSTATUS
LdrpCorEnsureMscoreeLoadedInternal(VOID)
{
    UNICODE_STRING Mscoree;
    ANSI_STRING Name;
    PVOID Base = NULL;
    NTSTATUS Status;

    if (LdrpCorMscoreeAttempted)
        return LdrpCorMscoreeHandle ? STATUS_SUCCESS : STATUS_DLL_NOT_FOUND;

    LdrpCorMscoreeAttempted = TRUE;

    RtlInitUnicodeString(&Mscoree, L"mscoree.dll");
    Status = LdrLoadDll(NULL, NULL, &Mscoree, &Base);
    if (!NT_SUCCESS(Status))
        return Status;

    LdrpCorMscoreeHandle = Base;

    /* Resolve optional exports */
    RtlInitAnsiString(&Name, "_CorValidateImage");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &LdrpCorEncodedValidateImage)))
        LdrpCorEncodedValidateImage = RtlEncodeSystemPointer(LdrpCorEncodedValidateImage);
    else
        LdrpCorEncodedValidateImage = NULL;

    RtlInitAnsiString(&Name, "_CorDllMain");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &LdrpCorEncodedDllMain)))
        LdrpCorEncodedDllMain = RtlEncodeSystemPointer(LdrpCorEncodedDllMain);
    else
        LdrpCorEncodedDllMain = NULL;

    RtlInitAnsiString(&Name, "_CorImageUnloading");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &LdrpCorEncodedImageUnloading)))
        LdrpCorEncodedImageUnloading = RtlEncodeSystemPointer(LdrpCorEncodedImageUnloading);
    else
        LdrpCorEncodedImageUnloading = NULL;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Loads mscoree.dll if that has not been tried yet.
 *
 * @return
 * STATUS_SUCCESS if mscoree.dll is loaded, an error status otherwise.
 */
NTSTATUS
NTAPI
LdrpCorEnsureMscoreeLoaded(VOID)
{
    return LdrpCorEnsureMscoreeLoadedInternal();
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
 * Asks mscoree's _CorValidateImage whether an image with a COM descriptor is acceptable.
 *
 * @param[in,out] ImageBase
 * Pointer to the base address of the mapped image, passed on to _CorValidateImage.
 *
 * @param[in] FileName
 * Name of the image file.
 *
 * @param[out] StatusOptional
 * Optional. Receives STATUS_SUCCESS if mscoree accepted the image, an error status otherwise.
 * Only valid if the function returns TRUE.
 *
 * @return
 * TRUE if mscoree could be asked, FALSE if mscoree.dll or _CorValidateImage is not available.
 */
BOOLEAN
NTAPI
LdrpCorTryValidateViaMscoree(
    _Inout_ PVOID* ImageBase,
    _In_ LPCWSTR FileName,
    _Out_opt_ NTSTATUS* StatusOptional)
{
    NTSTATUS LoadStatus;
    PFN_CorValidateImage ValidateImage;
    HRESULT Result;

    LoadStatus = LdrpCorEnsureMscoreeLoadedInternal();
    if (!NT_SUCCESS(LoadStatus) || !LdrpCorEncodedValidateImage)
        return FALSE;

    ValidateImage = (PFN_CorValidateImage)RtlDecodeSystemPointer(LdrpCorEncodedValidateImage);
    Result = ValidateImage(ImageBase, FileName);
    if (StatusOptional)
        *StatusOptional = SUCCEEDED(Result) ? STATUS_SUCCESS : (NTSTATUS)Result;

    return TRUE;
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

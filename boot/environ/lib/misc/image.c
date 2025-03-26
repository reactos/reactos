/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/image.c
 * PURPOSE:         Boot Library Image Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include <bcd.h>

/* DATA VARIABLES ************************************************************/

ULONG IapAllocatedTableEntries;
ULONG IapTableEntries;
PVOID* IapImageTable;

#ifndef _M_ARM
KDESCRIPTOR GdtRegister;
KDESCRIPTOR IdtRegister;
KDESCRIPTOR BootAppGdtRegister;
KDESCRIPTOR BootAppIdtRegister;
PVOID BootApp32EntryRoutine;
PBOOT_APPLICATION_PARAMETER_BLOCK BootApp32Parameters;
PVOID BootApp32Stack;
#endif

/* FUNCTIONS *****************************************************************/

NTSTATUS
ImgpGetFileSize (
    _In_ PBL_IMG_FILE File,
    _Out_ PULONG FileSize
    )
{
    NTSTATUS Status;
    ULONG Size;
    BL_FILE_INFORMATION FileInformation;

    /* Check if the file was memory mapped */
    if (File->Flags & BL_IMG_MEMORY_FILE)
    {
        /* Just read the size of the mapping */
        Size = File->FileSize;
    }
    else
    {
        /* Do file I/O to get the file size */
        Status = BlFileGetInformation(File->FileId,
                                      &FileInformation);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* We only support files less than 4GB in the Image Mapped */
        Size = FileInformation.Size;
        if (FileInformation.Size > ULONG_MAX)
        {
            return STATUS_NOT_SUPPORTED;
        }
    }

    /* Return the size and success */
    *FileSize = Size;
    return STATUS_SUCCESS;
}

NTSTATUS
ImgpReadAtFileOffset (
    _In_ PBL_IMG_FILE File,
    _In_ ULONG Size,
    _In_ ULONGLONG ByteOffset,
    _In_ PVOID Buffer,
    _Out_ PULONG BytesReturned
    )
{
    NTSTATUS Status;

    /* Check what if this is a mapped file or not */
    if (File->Flags & BL_IMG_MEMORY_FILE)
    {
        /* Check if the boundaries are within the file size */
        if ((ByteOffset + Size) <= File->FileSize)
        {
            /* Yep, copy into the caller-supplied buffer */
            RtlCopyMemory(Buffer,
                          (PVOID)((ULONG_PTR)File->BaseAddress + (ULONG_PTR)ByteOffset),
                          Size);

            /* If caller wanted to know, return the size copied */
            if (BytesReturned)
            {
                *BytesReturned = Size;
            }

            /* All good */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Doesn't fit */
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Issue the file I/O instead */
        Status = BlFileReadAtOffsetEx(File->FileId,
                                      Size,
                                      ByteOffset,
                                      Buffer,
                                      BytesReturned,
                                      0);
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
ImgpOpenFile (
    _In_ ULONG DeviceId,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _Out_ PBL_IMG_FILE NewFile
    )
{
    NTSTATUS Status;
    ULONG FileSize;
    ULONGLONG RemoteFileSize;
    PVOID RemoteFileAddress;
    ULONG FileId;

    /* First, try to see if BD has this file remotely */
    Status = BlBdPullRemoteFile(FileName,
                                &RemoteFileAddress,
                                &RemoteFileSize);
    if (NT_SUCCESS(Status))
    {
        /* Yep, get the file size and make sure it's < 4GB */
        FileSize = RemoteFileSize;
        if (RemoteFileSize <= ULONG_MAX)
        {
            /* Remember this is a memory mapped remote file */
            NewFile->Flags |= (BL_IMG_MEMORY_FILE | BL_IMG_REMOTE_FILE);
            NewFile->FileSize = FileSize;
            NewFile->BaseAddress = RemoteFileAddress;
            goto Quickie;
        }
    }

    /* Use File I/O instead */
    Status = BlFileOpen(DeviceId,
                        FileName,
                        BL_FILE_READ_ACCESS,
                        &FileId);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        return Status;
    }

    /* Make sure nobody thinks this is a memory file */
    NewFile->Flags &= ~BL_IMG_MEMORY_FILE;
    NewFile->FileId = FileId;

Quickie:
    /* Set common data for both memory and I/O based file */
    NewFile->Flags |= BL_IMG_VALID_FILE;
    NewFile->FileName = FileName;
    return Status;
}

NTSTATUS
ImgpCloseFile (
    _In_ PBL_IMG_FILE File
    )
{
    NTSTATUS Status;

    /* Make sure this is a valid file, otherwise no-op */
    Status = STATUS_SUCCESS;
    if (File->Flags & BL_IMG_VALID_FILE)
    {
        /* Is this a memory mapped file? */
        if (!(File->Flags & BL_IMG_MEMORY_FILE))
        {
            /* Nope, close the file handle */
            return BlFileClose(File->FileId);
        }

        /* Is this a remote file? */
        if (File->Flags & BL_IMG_REMOTE_FILE)
        {
            /* Then only free the memory in that scenario */
            return MmPapFreePages(File->BaseAddress, BL_MM_INCLUDE_MAPPED_ALLOCATED);
        }
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
BlImgUnallocateImageBuffer (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ ULONG ImageFlags
    )
{
    PHYSICAL_ADDRESS PhysicalAddress;
    NTSTATUS Status;

    /* Make sure required parameters are present */
    if (!(ImageBase) || !(ImageSize))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if this was a physical allocation */
    if (!(ImageFlags & BL_LOAD_IMG_VIRTUAL_BUFFER))
    {
        return MmPapFreePages(ImageBase, BL_MM_INCLUDE_MAPPED_ALLOCATED);
    }

    /* It's virtual, so translate it first */
    if (!BlMmTranslateVirtualAddress(ImageBase, &PhysicalAddress))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Unmap the virtual mapping */
    Status = BlMmUnmapVirtualAddressEx(ImageBase, ROUND_TO_PAGES(ImageSize));
    if (NT_SUCCESS(Status))
    {
        /* Now free the physical pages */
        Status = BlMmFreePhysicalPages(PhysicalAddress);
    }

    /* All done */
    return Status;
}

NTSTATUS
BlImgAllocateImageBuffer (
    _Inout_ PVOID* ImageBuffer,
    _In_ ULONG MemoryType,
    _In_ ULONGLONG ImageSize,
    _In_ ULONG Flags
    )
{
    ULONG Attributes;
    ULONGLONG Pages, Size;
    PVOID MappedBase, CurrentBuffer;
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Read and reset the current buffer address */
    CurrentBuffer = *ImageBuffer;
    *ImageBuffer = NULL;

    /* Align the image size to page */
    Size = ROUND_TO_PAGES(ImageSize);

    /* Not sure what this attribute does yet */
    Attributes = 0;
    if (Flags & BL_LOAD_IMG_UNKNOWN_BUFFER_FLAG)
    {
        Attributes = 0x10000;
    }

    /* Check if the caller wants a virtual buffer */
    if (Flags & BL_LOAD_IMG_VIRTUAL_BUFFER)
    {
        /* Set the physical address to the current buffer */
        PhysicalAddress.QuadPart = (ULONG_PTR)CurrentBuffer;
        Pages = Size >> PAGE_SHIFT;

        /* Allocate the physical pages */
        Status = BlMmAllocatePhysicalPages(&PhysicalAddress,
                                           Pages,
                                           MemoryType,
                                           Attributes,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            /* If that failed, remove allocation attributes */
            PhysicalAddress.QuadPart = 0;
            Attributes &= ~BlMemoryValidAllocationAttributeMask,
            Status = BlMmAllocatePhysicalPages(&PhysicalAddress,
                                               Pages,
                                               MemoryType,
                                               Attributes,
                                               0);
        }

        /* Check if either attempts succeeded */
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Now map the physical buffer at the address requested */
        MappedBase = PhysicalAddressToPtr(PhysicalAddress);
        Status = BlMmMapPhysicalAddressEx(&MappedBase,
                                          BlMemoryFixed,
                                          Size,
                                          PhysicalAddress);
        if (!NT_SUCCESS(Status))
        {
            /* Free on failure if needed */
            BlMmFreePhysicalPages(PhysicalAddress);
            return Status;
        }
    }
    else
    {
        /* Otherwise, allocate raw physical pages */
        MappedBase = CurrentBuffer;
        Pages = Size >> PAGE_SHIFT;
        Status = MmPapAllocatePagesInRange(&MappedBase,
                                           MemoryType,
                                           Pages,
                                           Attributes,
                                           0,
                                           NULL,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            /* If that failed, try without allocation attributes */
            MappedBase = NULL;
            Attributes &= ~BlMemoryValidAllocationAttributeMask,
            Status = MmPapAllocatePagesInRange(&MappedBase,
                                               MemoryType,
                                               Pages,
                                               Attributes,
                                               0,
                                               NULL,
                                               0);
        }

        /* Check if either attempts succeeded */
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Success path, returned allocated address */
    *ImageBuffer = MappedBase;
    return STATUS_SUCCESS;
}

NTSTATUS
BlImgLoadImageWithProgress2 (
    _In_ ULONG DeviceId,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ PWCHAR FileName,
    _Inout_ PVOID* MappedBase,
    _Inout_ PULONG MappedSize,
    _In_ ULONG ImageFlags,
    _In_ BOOLEAN ShowProgress,
    _Out_opt_ PUCHAR* HashBuffer,
    _Out_opt_ PULONG HashSize
    )
{
    NTSTATUS Status;
    PVOID BaseAddress, Buffer;
    ULONG RemainingLength, CurrentSize, ImageSize, ReadSize;
    BOOLEAN ComputeSignature, ComputeHash, Completed;
    BL_IMG_FILE FileHandle;
    ULONGLONG ByteOffset;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Initialize variables */
    BaseAddress = 0;
    ImageSize = 0;
    Completed = FALSE;
    RtlZeroMemory(&FileHandle, sizeof(FileHandle));

    /* Check for missing parameters */
    if (!MappedBase)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }
    if (!FileName)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }
    if (!MappedSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check if the image buffer is being provided */
    if (ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* An existing base must already exist */
        if (!(*MappedBase))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }

    /* Check of a hash is being requested */
    if (ImageFlags & BL_LOAD_IMG_COMPUTE_HASH)
    {
        /* Make sure we can return the hash */
        if (!HashBuffer)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
        if (!HashSize)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }

    /* Check for invalid combination of parameters */
    if ((ImageFlags & BL_LOAD_IMG_COMPUTE_HASH) && (ImageFlags & 0x270))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Initialize hash if requested by caller */
    if (HashBuffer)
    {
        *HashBuffer = 0;
    }

    /* Do the same for the hash size */
    if (HashSize)
    {
        *HashSize = 0;
    }

    /* Open the image file */
    Status = ImgpOpenFile(DeviceId, FileName, DeviceId, &FileHandle);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Error opening file: %lx\r\n", Status);
        goto Quickie;
    }

    /* Get the size of the image */
    Status = ImgpGetFileSize(&FileHandle, &ImageSize);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Error getting file size: %lx\r\n", Status);
        goto Quickie;
    }

    /* Read the current base address */
    BaseAddress = *MappedBase;
    if (ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* Check if the current buffer is too small */
        if (*MappedSize < ImageSize)
        {
            /* Return the required size of the buffer */
            *MappedSize = ImageSize;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        /* A buffer was not provided, allocate one ourselves */
        Status = BlImgAllocateImageBuffer(&BaseAddress,
                                          MemoryType,
                                          ImageSize,
                                          ImageFlags);
    }

    /* Bail out if allocation failed */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Set the initial byte offset and length to read */
    RemainingLength = ImageSize;
    ByteOffset = 0;
    Buffer = BaseAddress;

    /* Update the initial progress */
    Completed = FALSE;
    if (ShowProgress)
    {
        BlUtlUpdateProgress(0, &Completed);
        ShowProgress &= (Completed != 0) - 1;
    }

    /* Set the chunk size for each read */
    ReadSize = 0x100000;
    if (ReadSize > ImageSize)
    {
        ReadSize = ImageSize;
    }

    /* Check if we should compute hash and/or signatures */
    ComputeSignature = ImageFlags & BL_LOAD_IMG_COMPUTE_SIGNATURE;
    ComputeHash = FALSE;
    if ((ComputeSignature) || (ImageFlags & BL_LOAD_IMG_COMPUTE_HASH))
    {
        ComputeHash = TRUE;
        // todo: crypto is hard
    }

    /* Begin the read loop */
    while (RemainingLength)
    {
        /* Check if we've got more than a chunk left to read */
        if (RemainingLength > ReadSize)
        {
            /* Read a chunk*/
            CurrentSize = ReadSize;
        }
        else
        {
            /* Read only what's left */
            CurrentSize = RemainingLength;
        }

        /* Read the chunk */
        Status = ImgpReadAtFileOffset(&FileHandle,
                                      CurrentSize,
                                      ByteOffset,
                                      Buffer,
                                      0);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }

        /* Check if we need to compute the hash of this chunk */
        if (ComputeHash)
        {
            // todo: crypto is hard
        }

        /* Update our position and read information */
        Buffer = (PVOID)((ULONG_PTR)Buffer + CurrentSize);
        RemainingLength -= CurrentSize;
        ByteOffset += CurrentSize;

        /* Check if we should update the progress bar */
        if (ShowProgress)
        {
            /* Compute new percentage completed, check if we're done */
            BlUtlUpdateProgress(100 - 100 * RemainingLength / ImageSize,
                                &Completed);
            ShowProgress &= (Completed != 0) - 1;
        }
    }

    /* Is the read fully complete? We need to finalize the hash if requested */
    if (ComputeHash)
    {
        // todo: CRYPTO IS HARD
    }

    /* Success path, return back the buffer and the size of the image */
    *MappedBase = BaseAddress;
    *MappedSize = ImageSize;

Quickie:
    /* Close the file handle */
    ImgpCloseFile(&FileHandle);

    /* Check if we failed and had allocated a buffer */
    if (!(NT_SUCCESS(Status)) &&
        (BaseAddress) &&
        !(ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER))
    {
        /* Check what kind of buffer we had allocated */
        if (ImageFlags & BL_LOAD_IMG_VIRTUAL_BUFFER)
        {
            /* Unmap and free the virtual buffer */
            PhysicalAddress.QuadPart = (ULONG_PTR)BaseAddress;
            BlMmUnmapVirtualAddressEx(BaseAddress, ImageSize);
            BlMmFreePhysicalPages(PhysicalAddress);
        }
        else
        {
            /* Free the physical buffer */
            MmPapFreePages(BaseAddress, BL_MM_INCLUDE_MAPPED_ALLOCATED);
        }
    }

    /* If we hadn't gotten to 100% yet, do it now */
    if (ShowProgress)
    {
        BlUtlUpdateProgress(100, &Completed);
    }

    /* Return the final status */
    return Status;
}

PIMAGE_SECTION_HEADER
BlImgFindSection (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize
    )
{
    PIMAGE_SECTION_HEADER FoundSection;
    ULONG i;
    PIMAGE_SECTION_HEADER SectionHeader;
    PIMAGE_NT_HEADERS NtHeader;
    NTSTATUS Status;

    /* Assume failure */
    FoundSection = NULL;

    /* Make sure the image is valid */
    Status = RtlImageNtHeaderEx(0, ImageBase, ImageSize, &NtHeader);
    if (NT_SUCCESS(Status))
    {
        /* Get the first section and loop through them all */
        SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
        for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
        {
            /* Check if this is the resource section */
            if (!_stricmp((PCCH)SectionHeader->Name, ".rsrc"))
            {
                /* Yep, we're done */
                FoundSection = SectionHeader;
                break;
            }

            /* Nope, keep going */
            SectionHeader++;
        }
    }

    /* Return the matching section */
    return FoundSection;
}

VOID
BlImgQueryCodeIntegrityBootOptions (
    _In_ PBL_LOADED_APPLICATION_ENTRY ApplicationEntry,
    _Out_ PBOOLEAN IntegrityChecksDisabled,
    _Out_ PBOOLEAN TestSigning
    )
{

    NTSTATUS Status;
    BOOLEAN Value;

    /* Check if /DISABLEINTEGRITYCHECKS is on */
    Status = BlGetBootOptionBoolean(ApplicationEntry->BcdData,
                                    BcdLibraryBoolean_DisableIntegrityChecks,
                                    &Value);
    *IntegrityChecksDisabled = NT_SUCCESS(Status) && (Value);

    /* Check if /TESTSIGNING is on */
    Status = BlGetBootOptionBoolean(ApplicationEntry->BcdData,
                                    BcdLibraryBoolean_AllowPrereleaseSignatures,
                                    &Value);
    *TestSigning = NT_SUCCESS(Status) && (Value);
}

NTSTATUS
BlImgUnLoadImage (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ ULONG ImageFlags
    )
{
    /* Check for missing parameters */
    if (!(ImageSize) || !(ImageBase))
    {
        /* Bail out */
        return STATUS_INVALID_PARAMETER;
    }

    /* Unallocate the image buffer */
    return BlImgUnallocateImageBuffer(ImageBase, ImageSize, ImageFlags);
}

NTSTATUS
ImgpLoadPEImage (
    _In_ PBL_IMG_FILE ImageFile,
    _In_ BL_MEMORY_TYPE MemoryType,
    _Inout_ PVOID* ImageBase,
    _Out_opt_ PULONG ImageSize,
    _Inout_opt_ PVOID Hash,
    _In_ ULONG Flags
    )
{
    NTSTATUS Status;
    ULONG FileSize, HeaderSize;
    BL_IMG_FILE LocalFileBuffer;
    PBL_IMG_FILE LocalFile;
    PVOID VirtualAddress, PreferredBase, ImageBuffer, CertBuffer, HashBuffer;
    ULONGLONG VirtualSize;
    PIMAGE_DATA_DIRECTORY CertDirectory;
    PHYSICAL_ADDRESS PhysicalAddress;
    PIMAGE_NT_HEADERS NtHeaders;
    USHORT SectionCount, CheckSum, PartialSum, FinalSum;
    PIMAGE_SECTION_HEADER Section;
    ULONG_PTR EndOfHeaders, SectionStart, Slack, SectionEnd;
    ULONG i, SectionSize, RawSize, BytesRead, RemainingLength, Offset, AlignSize;
    BOOLEAN First, ImageHashValid;
    UCHAR LocalBuffer[1024];
    UCHAR TrustedBootInformation[52];
    ULONG WorkaroundForBinutils;

    /* Initialize locals */
    WorkaroundForBinutils = 0;
    LocalFile = NULL;
    ImageBuffer = NULL;
    FileSize = 0;
    First = FALSE;
    VirtualAddress = NULL;
    CertBuffer = NULL;
    CertDirectory = NULL;
    HashBuffer = NULL;
    Offset = 0;
    VirtualSize = 0;
    ImageHashValid = FALSE;
    RtlZeroMemory(&TrustedBootInformation, sizeof(TrustedBootInformation));

    /* Get the size of the image */
    Status = ImgpGetFileSize(ImageFile, &FileSize);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_FILE_INVALID;
    }

    /* Allocate a flat buffer for it */
    Status = BlImgAllocateImageBuffer(&ImageBuffer, BlLoaderData, FileSize, 0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Read the whole file flat for now */
    Status = ImgpReadAtFileOffset(ImageFile, FileSize, 0, ImageBuffer, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Build a local file handle */
    LocalFile = &LocalFileBuffer;
    LocalFileBuffer.FileName = ImageFile->FileName;
    LocalFileBuffer.Flags = BL_IMG_MEMORY_FILE | BL_IMG_VALID_FILE;
    LocalFileBuffer.BaseAddress = ImageBuffer;
    LocalFileBuffer.FileSize = FileSize;

    /* Get the NT headers of the file */
    Status = RtlImageNtHeaderEx(0, ImageBuffer, FileSize, &NtHeaders);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Check if we should validate the machine type */
    if (Flags & BL_LOAD_PE_IMG_CHECK_MACHINE)
    {
        /* Is it different than our current machine type? */
#if _M_AMD64
        if (NtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
#else
        if (NtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
#endif
        {
            /* Is it x86 (implying we are x64) ? */
            if (NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
            {
                /* Return special error code */
                Status = STATUS_INVALID_IMAGE_WIN_32;
            }
            else if (NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
            {
                /* Otherwise, it's x64 but we are x86 */
                Status = STATUS_INVALID_IMAGE_WIN_64;
            }
            else
            {
                /* Or it's ARM or something... */
                Status = STATUS_INVALID_IMAGE_FORMAT;
            }

            /* Return with the distinguished error code */
            goto Quickie;
        }
    }

    /* Check if we should validate the subsystem */
    if (Flags & BL_LOAD_PE_IMG_CHECK_SUBSYSTEM)
    {
        /* It must be a Windows boot Application */
        if (NtHeaders->OptionalHeader.Subsystem !=
            IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION)
        {
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }
    }

    /* Check if we should validate the /INTEGRITYCHECK flag */
    if (Flags & BL_LOAD_PE_IMG_CHECK_FORCED_INTEGRITY)
    {
        /* Check if it's there */
        if (!(NtHeaders->OptionalHeader.DllCharacteristics &
              IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY))
        {
            /* Nope, fail otherwise */
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }
    }

    /* Check if we should compute the image hash */
    if ((Flags & BL_LOAD_PE_IMG_COMPUTE_HASH) || (Hash))
    {
        EfiPrintf(L"No hash support\r\n");
    }

    /* Read the current base address, if any */
    VirtualAddress = *ImageBase;

    /* Get the virtual size of the image */
    VirtualSize = NtHeaders->OptionalHeader.SizeOfImage;

    /* Safely align the virtual size to a page */
    Status = RtlULongLongAdd(VirtualSize,
                             PAGE_SIZE - 1,
                             &VirtualSize);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
    VirtualSize = ALIGN_DOWN_BY(VirtualSize, PAGE_SIZE);

    /* Make sure the image isn't larger than 4GB */
    if (VirtualSize > ULONG_MAX)
    {
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto Quickie;
    }

    /* Check if we have a buffer already */
    if (Flags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* Check if it's too small */
        if (*ImageSize < VirtualSize)
        {
            /* Fail, letting the caller know how big to make it */
            *ImageSize = VirtualSize;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        /* Allocate the buffer with the flags and type the caller wants */
        Status = BlImgAllocateImageBuffer(&VirtualAddress,
                                          MemoryType,
                                          VirtualSize,
                                          Flags);
    }

    /* Bail out if allocation failed, or existing buffer is too small */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Read the size of the headers */
    HeaderSize = NtHeaders->OptionalHeader.SizeOfHeaders;
    if (VirtualSize < HeaderSize)
    {
        /* Bail out if they're bigger than the image! */
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto Quickie;
    }

    /* Now read the header into the buffer */
    Status = ImgpReadAtFileOffset(LocalFile, HeaderSize, 0, VirtualAddress, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Get the NT headers of the file */
    Status = RtlImageNtHeaderEx(0, VirtualAddress, HeaderSize, &NtHeaders);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    First = FALSE;

    /* Record how many sections we have */
    SectionCount = NtHeaders->FileHeader.NumberOfSections;

    /* Capture the current checksum and reset it */
    CheckSum = NtHeaders->OptionalHeader.CheckSum;
    NtHeaders->OptionalHeader.CheckSum = 0;

    /* Calculate the checksum of the header, and restore the original one */
    PartialSum = BlUtlCheckSum(0,
                               VirtualAddress,
                               HeaderSize,
                               BL_UTL_CHECKSUM_COMPLEMENT |
                               BL_UTL_CHECKSUM_USHORT_BUFFER);
    NtHeaders->OptionalHeader.CheckSum = CheckSum;

    /* Record our current position (right after the headers) */
    EndOfHeaders = (ULONG_PTR)VirtualAddress + HeaderSize;

    /* Get the first section and iterate through each one */
    Section = IMAGE_FIRST_SECTION(NtHeaders);
    for (i = 0; i < SectionCount; i++)
    {
        /* Compute where this section starts */
        SectionStart = (ULONG_PTR)VirtualAddress + Section->VirtualAddress;

        /* Make sure that the section fits within the image */
        if ((VirtualSize < Section->VirtualAddress) ||
            ((PVOID)SectionStart < VirtualAddress))
        {
            EfiPrintf(L"fail 1\r\n");
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }

        /* Check if there's slack space between header end and the section */
        if (!(First) && (EndOfHeaders < SectionStart))
        {
            /* Zero it out */
            Slack = SectionStart - EndOfHeaders;
            RtlZeroMemory((PVOID)EndOfHeaders, Slack);
        }

        /* Get the section virtual size and the raw size */
        SectionSize = Section->Misc.VirtualSize;
        RawSize = Section->SizeOfRawData;

        /* Safely align the raw size by 2 */
        Status = RtlULongAdd(RawSize, 1, &AlignSize);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }
        AlignSize = ALIGN_DOWN_BY(AlignSize, 2);

        /* IF we don't have a virtual size, use the raw size */
        if (!SectionSize)
        {
            SectionSize = RawSize;
        }

        /* If we don't have raw data, ignore the raw size */
        if (!Section->PointerToRawData)
        {
            RawSize = 0;
        }
        else if (SectionSize < RawSize)
        {
            /* And if the virtual size is smaller, use it as the final size */
            RawSize = SectionSize;
        }

        /* Make sure that the section doesn't overflow in memory */
        Status = RtlULongPtrAdd(Section->VirtualAddress,
                                SectionSize,
                                &SectionEnd);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"fail 21\r\n");
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }

        /* Make sure that it fits within the image */
        if (VirtualSize < SectionEnd)
        {
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }

        /* Make sure it doesn't overflow on disk */
        Status = RtlULongPtrAdd(Section->VirtualAddress,
                                AlignSize,
                                &SectionEnd);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"fail 31\r\n");
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }

        /* Make sure that it fits within the disk image as well */
        if (VirtualSize < SectionEnd)
        {
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }

        /* So does this section have a valid size after all? */
        if (RawSize)
        {
            /* Are we in the first iteration? */
            if (!First)
            {
                /* FUCK YOU BINUTILS */
                if (NtHeaders->OptionalHeader.MajorLinkerVersion < 7)
                {
                    if ((*(PULONG)&Section->Name == 'ler.') && (RawSize < AlignSize))
                    {
                        /* Piece of shit won't build relocations when you tell it to,
                         * either by using --emit-relocs or --dynamicbase. People online
                         * have found out that by using -pie-executable you can get this
                         * to happen, but then it turns out that the .reloc section is
                         * incorrectly sized, and results in a corrupt PE. However, they
                         * still compute the checksum using the correct value. What idiots.
                         */
                        WorkaroundForBinutils = AlignSize - RawSize;
                        AlignSize -= WorkaroundForBinutils;
                    }
                }

                /* Yes, read the section data */
                Status = ImgpReadAtFileOffset(LocalFile,
                                              AlignSize,
                                              Section->PointerToRawData,
                                              (PVOID)SectionStart,
                                              NULL);
                if (!NT_SUCCESS(Status))
                {
                    goto Quickie;
                }

                /* Update our current offset */
                Offset = AlignSize + Section->PointerToRawData;

                /* Update the checksum to include this section */
                PartialSum = BlUtlCheckSum(PartialSum,
                                           (PUCHAR)SectionStart,
                                           AlignSize,
                                           BL_UTL_CHECKSUM_COMPLEMENT |
                                           BL_UTL_CHECKSUM_USHORT_BUFFER);
                AlignSize += WorkaroundForBinutils;
            }
        }

        /* Are we in the first iteration? */
        if (!First)
        {
            /* Is there space at the end of the section? */
            if (RawSize < SectionSize)
            {
                /* Zero out the slack space that's there */
                Slack = SectionSize - RawSize;
                RtlZeroMemory((PVOID)(SectionStart + RawSize), Slack);
            }

            /* Update our tail offset */
            EndOfHeaders = SectionStart + SectionSize;
        }

        /* Move to the next section */
        Section++;
    }

    /* Are we in the first iteration? */
    if (!First)
    {
        /* Go to the end of the file */
        SectionStart = (ULONG_PTR)VirtualAddress + VirtualSize;

        /* Is there still some slack space left? */
        if (EndOfHeaders < SectionStart)
        {
            /* Zero it out */
            Slack = SectionStart - EndOfHeaders;
            RtlZeroMemory((PVOID)EndOfHeaders, Slack);
        }
    }

    /* Did the first iteration complete OK? */
    if ((NT_SUCCESS(Status)) && !(First))
    {
        /* Check how many non-image bytes are left in the file */
        RemainingLength = FileSize - Offset;
        while (RemainingLength)
        {
            /* See if the read will fit into our local buffer */
            if (RemainingLength >= sizeof(LocalBuffer))
            {
                /* Nope, cap it */
                BytesRead = sizeof(LocalBuffer);
            }
            else
            {
                /* Yes, but there's less to read */
                BytesRead = RemainingLength;
            }

            /* Read 1024 bytes into the local buffer */
            Status = ImgpReadAtFileOffset(LocalFile,
                                          BytesRead,
                                          Offset,
                                          LocalBuffer,
                                          &BytesRead);
            if (!(NT_SUCCESS(Status)) || !(BytesRead))
            {
                Status = STATUS_FILE_INVALID;
                goto Quickie;
            }

            /* Advance the offset and reduce the length */
            RemainingLength -= BytesRead;
            Offset += BytesRead;

            /* Compute the checksum of this leftover space */
            PartialSum = BlUtlCheckSum(PartialSum,
                                       LocalBuffer,
                                       BytesRead,
                                       BL_UTL_CHECKSUM_COMPLEMENT |
                                       BL_UTL_CHECKSUM_USHORT_BUFFER);
        }

        /* Finally, calculate the final checksum and compare it */
        FinalSum = FileSize + PartialSum + WorkaroundForBinutils;
        if ((FinalSum != CheckSum) && (PartialSum == 0xFFFF))
        {
            /* It hit overflow, so set it to the file size */
            FinalSum = FileSize;
        }

        /* If the checksum doesn't match, and caller is enforcing, bail out */
        if ((FinalSum != CheckSum) &&
            !(Flags & BL_LOAD_PE_IMG_IGNORE_CHECKSUM_MISMATCH))
        {
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
            goto Quickie;
        }
    }

    /* Check if the .rsrc section should be checked with the filename */
    if (Flags & BL_LOAD_PE_IMG_VALIDATE_ORIGINAL_FILENAME)
    {
        EfiPrintf(L"Not yet supported\r\n");
        Status = 0xC0430007; // STATUS_SECUREBOOT_FILE_REPLACED
        goto Quickie;
    }

    /* Check if we should relocate */
    if (!(Flags & BL_LOAD_PE_IMG_SKIP_RELOCATIONS))
    {
        /* Check if we loaded at a different address */
        PreferredBase = (PVOID)NtHeaders->OptionalHeader.ImageBase;
        if (VirtualAddress != PreferredBase)
        {
            /* Yep -- do relocations */
            Status = LdrRelocateImage(VirtualAddress,
                                      "Boot Environment Library",
                                      STATUS_SUCCESS,
                                      STATUS_UNSUCCESSFUL,
                                      STATUS_INVALID_IMAGE_FORMAT);
            if (!NT_SUCCESS(Status))
            {
                /* That's bad */
                goto Quickie;
            }
        }
    }

#if BL_TPM_SUPPORT
    /* Check if the image hash  was valid */
    if (!ImageHashValid)
    {
        /* Send a TPM/SI notification without a context */
        BlEnNotifyEvent(0x10000002, NULL);
    }

    /* Now send a TPM/SI notification with the hash of the loaded image */
    BlMmTranslateVirtualAddress(VirtualAddress, &Context.ImageBase);
    Context.HashAlgorithm = HashAlgorithm;
    Context.HashSize = HashSize;
    Context.FileName = ImageFile->FileName;
    Context.ImageSize = VirtualSize;
    Context.HashValid = ImageHashValid;
    Context.Hash = Hash;
    BlEnNotifyEvent(0x10000002, &Context);
#endif

    /* Return the loaded address to the caller */
    *ImageBase = VirtualAddress;

    /* If the caller wanted the image size, return it too */
    if (ImageSize)
    {
        *ImageSize = VirtualSize;
    }

Quickie:
    /* Check if we computed the image hash OK */
    if (ImageHashValid)
    {
        /* Then free the information that ImgpValidateImageHash set up */
        EfiPrintf(L"leadking trusted boot\r\n");
        //ImgpDestroyTrustedBootInformation(&TrustedBootInformation);
    }

    /* Check if we had a hash buffer */
    if (HashBuffer)
    {
        /* Free it */
        MmPapFreePages(HashBuffer, BL_MM_INCLUDE_MAPPED_ALLOCATED);
    }

    /* Check if we have a certificate directory */
    if ((CertBuffer) && (CertDirectory))
    {
        /* Free it */
        BlImgUnallocateImageBuffer(CertBuffer, CertDirectory->Size, 0);
    }

    /* Check if we had an image buffer allocated */
    if ((ImageBuffer) && (FileSize))
    {
        /* Free it */
        BlImgUnallocateImageBuffer(ImageBuffer, FileSize, 0);
    }

    /* Check if we had a local file handle */
    if (LocalFile)
    {
        /* Close it */
        ImgpCloseFile(LocalFile);
    }

    /* Check if this is the failure path */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we had started mapping in the image already */
        if ((VirtualAddress) && !(Flags & BL_LOAD_PE_IMG_EXISTING_BUFFER))
        {
            /* Into a virtual buffer? */
            if (Flags & BL_LOAD_PE_IMG_VIRTUAL_BUFFER)
            {
                /* Unmap and free it */
                BlMmUnmapVirtualAddressEx(VirtualAddress, VirtualSize);
                PhysicalAddress.QuadPart = (ULONG_PTR)VirtualAddress;
                BlMmFreePhysicalPages(PhysicalAddress);
            }
            else
            {
                /* Into a physical buffer -- free it */
                MmPapFreePages(VirtualAddress, BL_MM_INCLUDE_MAPPED_ALLOCATED);
            }
        }
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
BlImgLoadPEImageEx (
    _In_ ULONG DeviceId,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ PWCHAR Path,
    _Out_ PVOID* ImageBase,
    _Out_ PULONG ImageSize,
    _Out_ PVOID Hash,
    _In_ ULONG Flags
    )
{
    BL_IMG_FILE ImageFile;
    NTSTATUS Status;

    /* Initialize the image file structure */
    ImageFile.Flags = 0;
    ImageFile.FileName = NULL;

    /* Check if the required parameter are missing */
    if (!(ImageBase) || !(Path))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If we are loading a pre-allocated image, make sure we have it */
    if ((Flags & BL_LOAD_IMG_EXISTING_BUFFER) && (!(*ImageBase) || !(ImageSize)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Load the file from disk */
    Status = ImgpOpenFile(DeviceId, Path, 0, &ImageFile);
    if (NT_SUCCESS(Status))
    {
        /* If that worked, do the PE parsing */
        Status = ImgpLoadPEImage(&ImageFile,
                                 MemoryType,
                                 ImageBase,
                                 ImageSize,
                                 Hash,
                                 Flags);
    }

    /* Close the image file and return back to caller */
    ImgpCloseFile(&ImageFile);
    return Status;
}

NTSTATUS
BlImgLoadBootApplication (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _Out_ PULONG AppHandle
    )
{
    NTSTATUS Status;
    PULONGLONG AllowedList;
    ULONGLONG AllowedCount;
    ULONG i, DeviceId, ImageSize, Flags, ListSize;
    LARGE_INTEGER Frequency;
    PVOID UnlockCode, ImageBase;
    PBL_DEVICE_DESCRIPTOR Device, BitLockerDevice;
    PWCHAR Path;
    PBL_APPLICATION_ENTRY AppEntry;
    PBL_IMG_FILE ImageFile;
    BOOLEAN DisableIntegrity, TestSigning;
    UCHAR Hash[64];
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;

    /* Initialize all locals */
    BitLockerDevice = NULL;
    UnlockCode = NULL;
    ImageFile = NULL;
    DeviceId = -1;
    Device = NULL;
    ImageAppEntry = NULL;
    AppEntry = NULL;
    Path = NULL;
    ImageSize = 0;
    ImageBase = NULL;

    /* Check for "allowed in-memory settings" */
    Status = BlpGetBootOptionIntegerList(BootEntry->BcdData,
                                         BcdLibraryIntegerList_AllowedInMemorySettings,
                                         &AllowedList,
                                         &AllowedCount,
                                         TRUE);
    if (Status == STATUS_SUCCESS)
    {
        /* Loop through the list of allowed setting */
        for (i = 0; i < AllowedCount; i++)
        {
            /* Find the super undocumented one */
            if (AllowedList[i] == BcdLibraryInteger_UndocumentedMagic)
            {
                /* If it's present, append the current perf frequence to it */
                BlTimeQueryPerformanceCounter(&Frequency);
                BlAppendBootOptionInteger(BootEntry,
                                          BcdLibraryInteger_UndocumentedMagic,
                                          Frequency.QuadPart);
            }
        }
    }

#if BL_BITLOCKER_SUPPORT
    /* Do bitlocker stuff */
    Status = BlFveSecureBootUnlockBootDevice(BootEntry, &BitLockerDevice, &UnlockCode);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
#endif

    /* Get the device on which this application is on*/
    Status = BlGetBootOptionDevice(BootEntry->BcdData,
                                   BcdLibraryDevice_ApplicationDevice,
                                   &Device,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Get the path of the application */
    Status = BlGetBootOptionString(BootEntry->BcdData,
                                   BcdLibraryString_ApplicationPath,
                                   &Path);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Open the device */
    Status = BlpDeviceOpen(Device,
                           BL_DEVICE_READ_ACCESS,
                           0,
                           &DeviceId);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Check for integrity BCD options */
    BlImgQueryCodeIntegrityBootOptions(BootEntry,
                                       &DisableIntegrity,
                                       &TestSigning);

#if BL_TPM_SUPPORT
    RtlZeroMemory(&Context, sizeof(Context);
    Context.BootEntry = BootEntry;
    BlEnNotifyEvent(0x10000003, &Context);
#endif

    /* Enable signing and hashing checks if integrity is enabled */
    Flags = 0;
    if (!DisableIntegrity)
    {
        Flags = 0x8070;
    }

    /* Now call the PE loader to load the image */
    Status = BlImgLoadPEImageEx(DeviceId,
                                BlLoaderMemory,
                                Path,
                                &ImageBase,
                                &ImageSize,
                                Hash,
                                Flags);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

#if BL_KD_SUPPORT
    /* Check if we should notify the debugger of load */
    if (BdDebugTransitions)
    {
        /* Initialize it */
        BdForceDebug = 1;
        Status = BlBdInitialize();
        if (NT_SUCCESS(Status))
        {
            /* Check if it's enabled */
            if (BlBdDebuggerEnabled())
            {
                /* Send it an image load notification */
                BdDebuggerNotPresent = FALSE;
                RtlInitUnicodeString(&PathString, Path);
                BlBdLoadImageSymbols(&PathString, ImageBase);
            }
        }
    }
#endif

#if BL_BITLOCKER_SUPPORT
    /* Do bitlocker stuff */
    Status = BlSecureBootCheckPolicyOnFveDevice(BitLockerDevice);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
#endif

#if BL_BITLOCKER_SUPPORT
    /* Do bitlocker stuff */
    Status = BlFveSecureBootCheckpointBootApp(BootEntry, BitLockerDevice, Hash, UnlockCode);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
#endif

    /* Get the BCD option size */
    ListSize = BlGetBootOptionListSize(BootEntry->BcdData);

    /* Allocate an entry with all the BCD options */
    AppEntry = BlMmAllocateHeap(ListSize + sizeof(*AppEntry));
    if (!AppEntry)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Zero it out */
    RtlZeroMemory(AppEntry, sizeof(*AppEntry));

    /* Initialize it */
    strcpy(AppEntry->Signature, "BTAPENT");
    AppEntry->Guid = BootEntry->Guid;
    AppEntry->Flags = BootEntry->Flags;

    /* Copy the BCD options */
    RtlCopyMemory(&AppEntry->BcdData, BootEntry->BcdData, ListSize);

    /* Allocate the image entry */
    ImageAppEntry = BlMmAllocateHeap(sizeof(*ImageAppEntry));
    if (!ImageAppEntry)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Initialize it */
    ImageAppEntry->ImageBase = ImageBase;
    ImageAppEntry->ImageSize = ImageSize;
    ImageAppEntry->AppEntry = AppEntry;

    /* Check if this is the first entry */
    if (!IapTableEntries)
    {
        /* Allocate two entries */
        IapAllocatedTableEntries = 0;
        IapTableEntries = 2;
        IapImageTable = BlMmAllocateHeap(IapTableEntries * sizeof(PVOID));
        if (!IapImageTable)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Zero out the entries for now */
        RtlZeroMemory(IapImageTable, IapTableEntries * sizeof(PVOID));
    }

    /* Set this entry into the table */
    Status = BlTblSetEntry(&IapImageTable,
                           &IapTableEntries,
                           ImageAppEntry,
                           AppHandle,
                           TblDoNotPurgeEntry);

Quickie:
    /* Is the device open? Close it if so */
    if (DeviceId != 1)
    {
        BlDeviceClose(DeviceId);
    }

    /* Is there an allocated device? Free it */
    if (Device)
    {
        BlMmFreeHeap(Device);
    }

    /* Is there an allocated path? Free it */
    if (Path)
    {
        BlMmFreeHeap(Path);
    }

    /* Is there a bitlocker device? Free it */
    if (BitLockerDevice)
    {
        BlMmFreeHeap(BitLockerDevice);
    }

    /* Is there a bitlocker unlock code? Free it */
    if (UnlockCode)
    {
        BlMmFreeHeap(UnlockCode);
    }

    /* Did we succeed in creating an entry? */
    if (NT_SUCCESS(Status))
    {
        /* Remember there's one more in the table */
        IapAllocatedTableEntries++;

        /* Return success */
        return Status;
    }

    /* Did we load an image after all? */
    if (ImageBase)
    {
        /* Unload it */
        BlImgUnLoadImage(ImageBase, ImageSize, 0);
    }

    /* Did we allocate an app entry? Free it */
    if (AppEntry)
    {
        BlMmFreeHeap(AppEntry);
    }

    /* Do we have an image file entry?  Free it */
    if (ImageFile)
    {
        BlMmFreeHeap(ImageFile);
    }

    /* Do we no longer have a single entry in the table? */
    if (!(IapAllocatedTableEntries) && (IapImageTable))
    {
        /* Free and destroy the table */
        BlMmFreeHeap(IapImageTable);
        IapTableEntries = 0;
        IapImageTable = NULL;
    }

    /* Return the failure code */
    return Status;
}

NTSTATUS
BlpPdParseReturnArguments (
    _In_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
    /* Check if any custom data was returned */
    if (ReturnArguments->DataPage == 0)
    {
        /* Nope, nothing to do */
        return STATUS_SUCCESS;
    }

    /* Yes, we have to parse it */
    EfiPrintf(L"Return arguments not supported\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ImgpCopyApplicationBootDevice (
    __in PBL_DEVICE_DESCRIPTOR DestinationDevice,
    __in PBL_DEVICE_DESCRIPTOR SourceDevice
    )
{
    /* Is this a partition device? */
    if (SourceDevice->DeviceType != PartitionDevice)
    {
        /* It's not -- a simple copy will do */
        RtlCopyMemory(DestinationDevice, SourceDevice, SourceDevice->Size);
        return STATUS_SUCCESS;
    }

    /* TODO */
    EfiPrintf(L"Partition copy not supported\r\n");
    return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
ImgpInitializeBootApplicationParameters (
    _In_ PBL_BUFFER_DESCRIPTOR ImageParameters,
    _In_ PBL_APPLICATION_ENTRY AppEntry,
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize
    )
{
    NTSTATUS Status;
    PIMAGE_NT_HEADERS NtHeaders;
    BL_BUFFER_DESCRIPTOR MemoryParameters;
    LIST_ENTRY MemoryList;
    PBL_FIRMWARE_DESCRIPTOR FirmwareParameters;
    PBL_DEVICE_DESCRIPTOR BootDevice;
    PBL_MEMORY_DATA MemoryData;
    PBL_APPLICATION_ENTRY BootAppEntry;
    PBL_RETURN_ARGUMENTS ReturnArguments;
    PBOOT_APPLICATION_PARAMETER_BLOCK ParameterBlock;
    ULONG EntrySize, BufferSize;

    /* Get the image headers and validate it */
    Status = RtlImageNtHeaderEx(0, ImageBase, ImageSize, &NtHeaders);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get the size of the entire non-firmware, allocated, memory map */
    MemoryParameters.BufferSize = 0;
    Status = BlMmGetMemoryMap(&MemoryList,
                              &MemoryParameters,
                              BL_MM_INCLUDE_PERSISTENT_MEMORY |
                              BL_MM_INCLUDE_MAPPED_ALLOCATED |
                              BL_MM_INCLUDE_MAPPED_UNALLOCATED |
                              BL_MM_INCLUDE_UNMAPPED_ALLOCATED |
                              BL_MM_INCLUDE_RESERVED_ALLOCATED,
                              0);
    if ((Status != STATUS_BUFFER_TOO_SMALL) && (Status != STATUS_SUCCESS))
    {
        /* We failed due to an unknown reason -- bail out */
        return Status;
    }

    /* Compute the list of the BCD plus the application entry */
    EntrySize = BlGetBootOptionListSize(&AppEntry->BcdData) +
                FIELD_OFFSET(BL_APPLICATION_ENTRY, BcdData);

    /* Compute the total size required for the entire structure */
    BufferSize = EntrySize +
                 BlpBootDevice->Size +
                 MemoryParameters.BufferSize +
                 sizeof(*ReturnArguments) +
                 sizeof(*MemoryData) +
                 sizeof(*FirmwareParameters) +
                 sizeof(*ParameterBlock);

    /* Check if this gives us enough space */
    if (ImageParameters->BufferSize < BufferSize)
    {
        /* It does not -- free the existing buffer */
        if (ImageParameters->BufferSize)
        {
            BlMmFreeHeap(ImageParameters->Buffer);
        }

        /* Allocate a new buffer of sufficient size */
        ImageParameters->BufferSize = BufferSize;
        ImageParameters->Buffer = BlMmAllocateHeap(BufferSize);
        if (!ImageParameters->Buffer)
        {
            /* Bail out if we couldn't allocate it */
            return STATUS_NO_MEMORY;
        }
    }

    /* Zero out the parameter block */
    ParameterBlock = (PBOOT_APPLICATION_PARAMETER_BLOCK)ImageParameters->Buffer;
    RtlZeroMemory(ParameterBlock, BufferSize);

    /* Initialize it */
    ParameterBlock->Version = BOOT_APPLICATION_VERSION;
    ParameterBlock->Size = BufferSize;
    ParameterBlock->Signature[0] = BOOT_APPLICATION_SIGNATURE_1;
    ParameterBlock->Signature[1] = BOOT_APPLICATION_SIGNATURE_2;
    ParameterBlock->MemoryTranslationType = MmTranslationType;
    ParameterBlock->ImageType = IMAGE_FILE_MACHINE_I386;
    ParameterBlock->ImageBase = (ULONG_PTR)ImageBase;
    ParameterBlock->ImageSize = NtHeaders->OptionalHeader.SizeOfImage;

    /* Get the offset to the memory data */
    ParameterBlock->MemoryDataOffset = sizeof(*ParameterBlock);

    /* Fill it out */
    MemoryData = (PBL_MEMORY_DATA)((ULONG_PTR)ParameterBlock +
                                   ParameterBlock->MemoryDataOffset);
    MemoryData->Version = BL_MEMORY_DATA_VERSION;
    MemoryData->MdListOffset = sizeof(*MemoryData);
    MemoryData->DescriptorSize = sizeof(BL_MEMORY_DESCRIPTOR);
    MemoryData->DescriptorOffset = FIELD_OFFSET(BL_MEMORY_DESCRIPTOR, BasePage);

    /* And populate the memory map */
    MemoryParameters.Buffer = MemoryData + 1;
    Status = BlMmGetMemoryMap(&MemoryList,
                              &MemoryParameters,
                              BL_MM_INCLUDE_PERSISTENT_MEMORY |
                              BL_MM_INCLUDE_MAPPED_ALLOCATED |
                              BL_MM_INCLUDE_MAPPED_UNALLOCATED |
                              BL_MM_INCLUDE_UNMAPPED_ALLOCATED |
                              BL_MM_INCLUDE_RESERVED_ALLOCATED,
                              0);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Now that we have the map, indicate the number of descriptors */
    MemoryData->DescriptorCount = MemoryParameters.ActualSize /
                                  MemoryData->DescriptorSize;

    /* Get the offset to the application entry */
    ParameterBlock->AppEntryOffset = ParameterBlock->MemoryDataOffset +
                                     MemoryData->MdListOffset +
                                     MemoryParameters.BufferSize;

    /* Fill it out */
    BootAppEntry = (PBL_APPLICATION_ENTRY)((ULONG_PTR)ParameterBlock +
                                           ParameterBlock->AppEntryOffset);
    RtlCopyMemory(BootAppEntry, AppEntry, EntrySize);

    /* Get the offset to the boot device */
    ParameterBlock->BootDeviceOffset = ParameterBlock->AppEntryOffset +
                                       EntrySize;

    /* Fill it out */
    BootDevice = (PBL_DEVICE_DESCRIPTOR)((ULONG_PTR)ParameterBlock +
                                         ParameterBlock->BootDeviceOffset);
    Status = ImgpCopyApplicationBootDevice(BootDevice, BlpBootDevice);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get the offset to the firmware data */
    ParameterBlock->FirmwareParametersOffset = ParameterBlock->BootDeviceOffset +
                                               BootDevice->Size;

    /* Fill it out */
    FirmwareParameters = (PBL_FIRMWARE_DESCRIPTOR)((ULONG_PTR)ParameterBlock +
                                                   ParameterBlock->
                                                   FirmwareParametersOffset);
    Status = BlFwGetParameters(FirmwareParameters);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get the offset to the return arguments */
    ParameterBlock->ReturnArgumentsOffset = ParameterBlock->FirmwareParametersOffset +
                                            sizeof(BL_FIRMWARE_DESCRIPTOR);

    /* Fill them out */
    ReturnArguments = (PBL_RETURN_ARGUMENTS)((ULONG_PTR)ParameterBlock +
                                             ParameterBlock->
                                             ReturnArgumentsOffset);
    ReturnArguments->Version = BL_RETURN_ARGUMENTS_VERSION;
    ReturnArguments->DataPage = 0;
    ReturnArguments->DataSize = 0;

    /* Structure complete */
    ImageParameters->ActualSize = ParameterBlock->ReturnArgumentsOffset +
                                  sizeof(*ReturnArguments);
    return STATUS_SUCCESS;
}

NTSTATUS
ImgArchEfiStartBootApplication (
    _In_ PBL_APPLICATION_ENTRY AppEntry,
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
#ifndef _M_ARM
    KDESCRIPTOR Gdt, Idt;
    ULONG BootSizeNeeded;
    NTSTATUS Status;
    PVOID BootData;
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID NewStack, NewGdt, NewIdt;
    BL_BUFFER_DESCRIPTOR Parameters;

    /* Read the current IDT and GDT */
    _sgdt(&Gdt.Limit);
    __sidt(&Idt.Limit);

    /* Allocate space for the IDT, GDT, and 24 pages of stack */
    BootSizeNeeded = (ULONG_PTR)PAGE_ALIGN(Idt.Limit + Gdt.Limit + 1 + 25 * PAGE_SIZE);
    Status = MmPapAllocatePagesInRange(&BootData,
                                       BlLoaderArchData,
                                       BootSizeNeeded >> PAGE_SHIFT,
                                       0,
                                       0,
                                       NULL,
                                       0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Zero the boot data */
    RtlZeroMemory(BootData, BootSizeNeeded);

    /* Set the new stack, GDT and IDT */
    NewStack = (PVOID)((ULONG_PTR)BootData + (24 * PAGE_SIZE) - 8);
    NewGdt = (PVOID)((ULONG_PTR)BootData + (24 * PAGE_SIZE));
    NewIdt = (PVOID)((ULONG_PTR)BootData + (24 * PAGE_SIZE) + Gdt.Limit + 1);

    /* Copy the current (firmware) GDT and IDT */
    RtlCopyMemory(NewGdt, (PVOID)Gdt.Base, Gdt.Limit + 1);
    RtlCopyMemory(NewIdt, (PVOID)Idt.Base, Idt.Limit + 1);

    /* Read the NT headers so that we can get the entrypoint later on */
    RtlImageNtHeaderEx(0, ImageBase, ImageSize, &NtHeaders);

    /* Prepare the application parameters */
    RtlZeroMemory(&Parameters, sizeof(Parameters));
    Status = ImgpInitializeBootApplicationParameters(&Parameters,
                                                     AppEntry,
                                                     ImageBase,
                                                     ImageSize);
    if (NT_SUCCESS(Status))
    {
        /* Set the firmware GDT/IDT as the one the application will use */
        BootAppGdtRegister = Gdt;
        BootAppIdtRegister = Idt;

        /* Set the entrypoint, parameters, and stack */
        BootApp32EntryRoutine = (PVOID)((ULONG_PTR)ImageBase +
                                        NtHeaders->OptionalHeader.
                                        AddressOfEntryPoint);
        BootApp32Parameters = Parameters.Buffer;
        BootApp32Stack = NewStack;

#if BL_KD_SUPPORT
        /* Disable the kernel debugger */
        BlBdStop();
#endif
        /* Make it so */
        Archx86TransferTo32BitApplicationAsm();

        /* Not yet implemented. This is the last step! */
        EfiPrintf(L"EFI APPLICATION RETURNED!!!\r\n");
        EfiStall(100000000);

#if BL_KD_SUPPORT
        /* Re-enable the kernel debugger */
        BlBdStart();
#endif
    }

Quickie:
    /* Check if we had boot data allocated */
    if (BootData)
    {
        /* Free it */
        MmPapFreePages(BootData, BL_MM_INCLUDE_MAPPED_ALLOCATED);
    }
#else
    EfiPrintf(L"ImgArchEfiStartBootApplication not implemented for this platform.\r\n");
#endif

    /* All done */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlImgStartBootApplication (
    _In_ ULONG AppHandle,
    _Inout_opt_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;
    BL_RETURN_ARGUMENTS LocalReturnArgs;
    PBL_FILE_SYSTEM_ENTRY FileSystem;
    PLIST_ENTRY NextEntry, ListHead;
    NTSTATUS Status;

    /* Check if we don't have an argument structure */
    if (!ReturnArguments)
    {
        /* Initialize a local copy and use it instead */
        LocalReturnArgs.Version = BL_RETURN_ARGUMENTS_VERSION;
        LocalReturnArgs.Status = STATUS_SUCCESS;
        LocalReturnArgs.Flags = 0;
        LocalReturnArgs.DataPage = 0;
        LocalReturnArgs.DataSize = 0;
        ReturnArguments = &LocalReturnArgs;
    }

    /* Make sure the handle index is valid */
    if (IapTableEntries <= AppHandle)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the entry for this handle, making sure it exists */
    ImageAppEntry = IapImageTable[AppHandle];
    if (!ImageAppEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Loop the registered file systems */
    ListHead = &RegisteredFileSystems;
    NextEntry = RegisteredFileSystems.Flink;
    while (NextEntry != ListHead)
    {
        /* Get the filesystem entry */
        FileSystem = CONTAINING_RECORD(NextEntry,
                                       BL_FILE_SYSTEM_ENTRY,
                                       ListEntry);

        /* See if it has a purge callback */
        if (FileSystem->PurgeCallback)
        {
            /* Call it */
            FileSystem->PurgeCallback();
        }

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* TODO  -- flush the block I/O cache too */
    //BlockIoPurgeCache();

    /* Call into EFI land to start the boot application */
    Status = ImgArchEfiStartBootApplication(ImageAppEntry->AppEntry,
                                            ImageAppEntry->ImageBase,
                                            ImageAppEntry->ImageSize,
                                            ReturnArguments);

    /* Parse any arguments we got on the way back */
    BlpPdParseReturnArguments(ReturnArguments);

#if BL_BITLOCKER_SUPPORT
    /* Bitlocker stuff */
    FvebpCheckAllPartitions(TRUE);
#endif

#if BL_TPM_SUPPORT
    /* Notify a TPM/SI event */
    BlEnNotifyEvent(0x10000005, NULL);
#endif

    /* Reset the display */
    BlpDisplayReinitialize();

    /* TODO -- reset ETW */
    //BlpLogInitialize();

    /* All done */
    return Status;
}

NTSTATUS
BlImgUnloadBootApplication (
    _In_ ULONG AppHandle
    )
{
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;
    NTSTATUS Status;

    /* Make sure the handle index is valid */
    if (IapTableEntries <= AppHandle)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the entry for this handle, making sure it exists */
    ImageAppEntry = IapImageTable[AppHandle];
    if (!ImageAppEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Unload the image */
    Status = BlImgUnLoadImage(ImageAppEntry->ImageBase,
                              ImageAppEntry->ImageSize,
                              0);
    if (NT_SUCCESS(Status))
    {
        /* Normalize the success code */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Normalize the failure code */
        Status = STATUS_MEMORY_NOT_ALLOCATED;
    }

    /* Free the entry and the image entry as well */
    BlMmFreeHeap(ImageAppEntry->AppEntry);
    BlMmFreeHeap(ImageAppEntry);

    /* Clear the handle */
    IapImageTable[AppHandle] = NULL;

    /* Free one entry */
    if (!(--IapAllocatedTableEntries))
    {
        /* There are no more, so get rid of the table itself */
        BlMmFreeHeap(IapImageTable);
        IapImageTable = NULL;
        IapTableEntries = 0;
    }

    /* All good */
    return Status;
}

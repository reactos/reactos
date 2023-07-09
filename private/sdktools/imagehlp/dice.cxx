/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dice.cxx

Abstract:

    This file implements the Image Integrity API's.

Author:

    Bryan Tuttle (bryant) 7-Dec-1995

Environment:

    User Mode

--*/

#include <private.h>

BOOL
FindCertificate(
    IN PLOADED_IMAGE    LoadedImage,
    IN DWORD            Index,
    LPWIN_CERTIFICATE * Certificate
    )
{
    PIMAGE_DATA_DIRECTORY pDataDir;
    DWORD_PTR CurrentCert;
    BOOL rc;

    if (LoadedImage->fDOSImage) {
        // No way this could have a certificate;
        return(FALSE);
    }

    rc = FALSE;

    __try {
        if (LoadedImage->FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            pDataDir = &((PIMAGE_NT_HEADERS32)(LoadedImage->FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else if (LoadedImage->FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            pDataDir = &((PIMAGE_NT_HEADERS64)(LoadedImage->FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else {
            __leave;     // Not an interesting file type.
        }

        // Check if the cert pointer is at least reasonable.
        if (!pDataDir->VirtualAddress ||
            !pDataDir->Size ||
            (pDataDir->VirtualAddress + pDataDir->Size > LoadedImage->SizeOfImage))
        {
            __leave;
        }

        // We're not looking at an empty security slot or an invalid (past the image boundary) value.
        // Let's see if we can find it.

        DWORD CurrentIdx = 0;
        DWORD_PTR LastCert;

        CurrentCert = (DWORD_PTR)(LoadedImage->MappedAddress) + pDataDir->VirtualAddress;
        LastCert = CurrentCert + pDataDir->Size;

        while (CurrentCert < LastCert ) {
            if (CurrentIdx == Index) {
                rc = TRUE;
                __leave;
            }
            CurrentIdx++;
            CurrentCert += ((LPWIN_CERTIFICATE)CurrentCert)->dwLength;
            CurrentCert = (CurrentCert + 7) & ~7;   // align it.
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    if (rc == TRUE) {
        *Certificate = (LPWIN_CERTIFICATE)CurrentCert;
    }

    return(rc);
}

typedef struct _EXCLUDE_RANGE {
    PBYTE Offset;
    DWORD Size;
    struct _EXCLUDE_RANGE *Next;
} EXCLUDE_RANGE;

typedef enum {
    Raw,
    Virtual
} ADDRTYPE;

class EXCLUDE_LIST
{
    public:
        EXCLUDE_LIST() {
            m_Image = NULL;
            m_ExRange = (EXCLUDE_RANGE *)MemAlloc(sizeof(EXCLUDE_RANGE));
        }

        ~EXCLUDE_LIST() {
            EXCLUDE_RANGE *pTmp;
            pTmp = m_ExRange->Next;
            while (pTmp) {
                MemFree(m_ExRange);
                m_ExRange = pTmp;
                pTmp = m_ExRange->Next;
            }
            MemFree(m_ExRange);
        }

        void Init(LOADED_IMAGE * Image, DIGEST_FUNCTION pFunc, DIGEST_HANDLE dh) {
            m_Image = Image;
            m_ExRange->Offset = NULL;
            m_ExRange->Size = 0;
            m_pFunc = pFunc;
            m_dh = dh;
            return;
        }

        void Add(DWORD_PTR Offset, DWORD Size, ADDRTYPE AddrType);

        BOOL Emit(PBYTE Offset, DWORD Size);

    private:
        LOADED_IMAGE  * m_Image;
        EXCLUDE_RANGE * m_ExRange;
        DIGEST_FUNCTION m_pFunc;
        DIGEST_HANDLE m_dh;
};

void
EXCLUDE_LIST::Add(
    DWORD_PTR Offset,
    DWORD Size,
    ADDRTYPE AddrType
    )
{
    if (AddrType == Virtual) {
        // Always save raw offsets
        DWORD_PTR RawOffset;

        // Note: it's O.K. to cast down to a dword here.  Offset is really a Rva from the start
        // of the image (always limited to 4G).

        RawOffset = (DWORD_PTR)ImageRvaToVa((PIMAGE_NT_HEADERS)m_Image->FileHeader, m_Image->MappedAddress, (DWORD)Offset, NULL);
        Offset = RawOffset;
    }

    EXCLUDE_RANGE *pTmp, *pExRange;

    pExRange = m_ExRange;

    while (pExRange->Next && (pExRange->Next->Offset < (PBYTE)Offset)) {
        pExRange = pExRange->Next;
    }

    pTmp = (EXCLUDE_RANGE *) MemAlloc(sizeof(EXCLUDE_RANGE));
    pTmp->Next = pExRange->Next;
    pTmp->Offset = (PBYTE)Offset;
    pTmp->Size = Size;
    pExRange->Next = pTmp;

    return;
}


BOOL
EXCLUDE_LIST::Emit(
    PBYTE Offset,
    DWORD Size
    )
{
    BOOL rc;

    EXCLUDE_RANGE *pExRange;
    DWORD EmitSize, ExcludeSize;

    pExRange = m_ExRange->Next;

    while (pExRange && (Size > 0)) {
        if (pExRange->Offset >= Offset) {
            // Emit what's before the exclude list.
            EmitSize = __min((DWORD)(pExRange->Offset - Offset), Size);
            if (EmitSize) {
                rc = (*m_pFunc)(m_dh, Offset, EmitSize);
                Size -= EmitSize;
                Offset += EmitSize;
            }
        }

        if (Size) {
            if (pExRange->Offset + pExRange->Size >= Offset) {
                // Skip over what's in the exclude list.
                ExcludeSize = __min(Size, (DWORD)(pExRange->Offset + pExRange->Size - Offset));
                Size -= ExcludeSize;
                Offset += ExcludeSize;
            }
        }

        pExRange = pExRange->Next;
    }

    // Emit what's left.
    if (Size) {
        rc = (*m_pFunc)(m_dh, Offset, Size);
    }
    return rc;
}


BOOL
IMAGEAPI
ImageGetDigestStream(
    IN HANDLE           FileHandle,
    IN DWORD            DigestLevel,
    IN DIGEST_FUNCTION  DigestFunction,
    IN DIGEST_HANDLE    DigestHandle
    )

/*++

Routine Description:

    Given an image, return the bytes necessary to construct a certificate.
    Only PE images are supported at this time.

Arguments:

    FileHandle  -   Handle to the file in question.  The file should be opened
                    with at least GENERIC_READ access.

    DigestLevel -   Indicates what data will be included in the returned buffer.
                    Valid values are:

                        CERT_PE_IMAGE_DIGEST_DEBUG_INFO - Include Debug symbolic (if mapped)
                        CERT_PE_IMAGE_DIGEST_RESOURCES  - Include Resource info
                        CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO - Include ALL the import information

                    By default, neither Debug Symbolic, Resources, nor import information affected
                    by binding are returned.

    DigestFunction - User supplied routine that will process the data.

    DigestHandle -  User supplied handle to identify the digest.  Passed as the first
                    argument to the DigestFunction.

Return Value:

    TRUE         - Success.

    FALSE        - There was some error.  Call GetLastError for more information.  Possible
                   values are ERROR_INVALID_PARAMETER or ERROR_OPERATION_ABORTED.

--*/

{
    LOADED_IMAGE    LoadedImage;
    BOOL            rc, fAddThisSection, fDebugAdded;
    DWORD           i;
    EXCLUDE_LIST    ExList;
    PIMAGE_SECTION_HEADER SectionHeaders;
    ULONG ResourceOffset, ResourceSize, DebugOffset, DebugSize, RelocOffset, RelocSize, SectionHeaderSize;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_DATA_DIRECTORY pDataDir;
    INT RelocHdr;
    union {
        IMAGE_NT_HEADERS32 PE32;
        IMAGE_NT_HEADERS64 PE64;
    }Hdr;
    BOOL f32;

    if (MapIt(FileHandle, &LoadedImage, MAP_READONLY) == FALSE) {
        // Unable to map the image.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = ERROR_INVALID_PARAMETER;

    __try {

        if (LoadedImage.fDOSImage) {
            __leave;
        }

        if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            f32 = TRUE;
        } else if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            f32 = FALSE;
        } else {
            __leave;
        }

        ExList.Init(&LoadedImage, DigestFunction, DigestHandle);

        // Return all the interesting stuff from the image.  First, the common stuff.

        // 1. Add the DOS stub (if it exists).

        if ((ULONG_PTR)LoadedImage.FileHeader - (ULONG_PTR) LoadedImage.MappedAddress) {
            if (!ExList.Emit((PBYTE) LoadedImage.MappedAddress,
                             (DWORD)((ULONG_PTR) LoadedImage.FileHeader - (ULONG_PTR) LoadedImage.MappedAddress)))
            {
                rc = ERROR_OPERATION_ABORTED;
                __leave;
            }
        }

        // Add the headers, but not the checksum and not the security Data directory entry.

        if (f32) {
            Hdr.PE32 = *((PIMAGE_NT_HEADERS32)LoadedImage.FileHeader);
            pDataDir = &Hdr.PE32.OptionalHeader.DataDirectory[0];
            Hdr.PE32.OptionalHeader.CheckSum = 0;
        } else {
            Hdr.PE64 = *((PIMAGE_NT_HEADERS64)LoadedImage.FileHeader);
            pDataDir = &Hdr.PE64.OptionalHeader.DataDirectory[0];
            Hdr.PE64.OptionalHeader.CheckSum = 0;
        }

        pDataDir[IMAGE_DIRECTORY_ENTRY_SECURITY].Size = 0;
        pDataDir[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0;

        SectionHeaderSize = sizeof(IMAGE_SECTION_HEADER) * LoadedImage.NumberOfSections;
        SectionHeaders = (PIMAGE_SECTION_HEADER) MemAlloc(SectionHeaderSize);

        ResourceOffset = pDataDir[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
        ResourceSize = pDataDir[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
        RelocOffset = pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
        RelocSize = pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

        fDebugAdded = TRUE;
        DebugOffset = 0xFFFFFFFF;
        RelocHdr = -1;

        for (i = 0; i < LoadedImage.NumberOfSections; i++) {
            SectionHeaders[i] = LoadedImage.Sections[i];

            // Keep track of the reloc section header.  We may need to adjust it later.

            if (RelocSize &&
                ((LoadedImage.Sections[i].VirtualAddress <= RelocOffset) &&
                 (LoadedImage.Sections[i].VirtualAddress +
                    LoadedImage.Sections[i].Misc.VirtualSize >= RelocOffset + RelocSize))
                )
            {
                RelocHdr = i;
            }

            // If resources aren't in the digest, we need to clear the resource section header

            if (ResourceSize && !(DigestLevel & CERT_PE_IMAGE_DIGEST_RESOURCES)) {

                if (((LoadedImage.Sections[i].VirtualAddress <= ResourceOffset) &&
                     (LoadedImage.Sections[i].VirtualAddress +
                        LoadedImage.Sections[i].Misc.VirtualSize >= ResourceOffset + ResourceSize))
                    )
                {
                    // Found the resource section header.  Zero it out.
                    SectionHeaders[i].Misc.VirtualSize = 0;
                    SectionHeaders[i].VirtualAddress = 0;
                    SectionHeaders[i].SizeOfRawData = 0;
                    SectionHeaders[i].PointerToRawData = 0;
                }
            }

            if (!(DigestLevel & CERT_PE_IMAGE_DIGEST_DEBUG_INFO)) {
                // Same with mapped debug info.
                if (!strncmp((char *)LoadedImage.Sections[i].Name, ".debug", sizeof(".debug"))) {

                    DebugOffset = SectionHeaders[i].VirtualAddress;
                    DebugSize = SectionHeaders[i].SizeOfRawData;
                    ExList.Add(SectionHeaders[i].PointerToRawData + (DWORD_PTR) LoadedImage.MappedAddress, DebugSize, Raw);

                    SectionHeaders[i].Misc.VirtualSize = 0;
                    SectionHeaders[i].VirtualAddress = 0;
                    SectionHeaders[i].SizeOfRawData = 0;
                    SectionHeaders[i].PointerToRawData = 0;
                    fDebugAdded = FALSE;
                }
            }
        }

        // The first pass on the section headers is finished.  See it we need to adjust the
        // reloc dir or the image headers.

        if (!(DigestLevel & CERT_PE_IMAGE_DIGEST_RESOURCES)) {
            // If the resources aren't in the digest, don't add the base reloc address or the
            // resource address/size to the digest.  This allows subsequent tools to add/subtract
            // resource info w/o effecting the digest.

            if ((ResourceOffset < RelocOffset) && (RelocHdr != -1))
            {
                pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
                SectionHeaders[RelocHdr].PointerToRawData = 0;
                SectionHeaders[RelocHdr].VirtualAddress = 0;
            }
            pDataDir[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = 0;
            pDataDir[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = 0;
            if (f32) {
                Hdr.PE32.OptionalHeader.SizeOfImage = 0;
                Hdr.PE32.OptionalHeader.SizeOfInitializedData = 0;
            } else {
                Hdr.PE64.OptionalHeader.SizeOfImage = 0;
                Hdr.PE64.OptionalHeader.SizeOfInitializedData = 0;
            }
            ExList.Add(ResourceOffset, ResourceSize, Virtual);
        }

        if (!(DigestLevel & CERT_PE_IMAGE_DIGEST_DEBUG_INFO) &&
            (fDebugAdded == FALSE))
        {
            // Debug wasn't added to the image and IS mapped in.  Allow these to grow also.
            if (f32) {
                Hdr.PE32.OptionalHeader.SizeOfImage = 0;
                Hdr.PE32.OptionalHeader.SizeOfInitializedData = 0;
            } else {
                Hdr.PE64.OptionalHeader.SizeOfImage = 0;
                Hdr.PE64.OptionalHeader.SizeOfInitializedData = 0;
            }
            if ((DebugOffset < RelocOffset) && (RelocHdr != -1))
            {
                pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
                SectionHeaders[RelocHdr].PointerToRawData = 0;
                SectionHeaders[RelocHdr].VirtualAddress = 0;
            }
        }

        // Looks good.  Send the headers to the digest function.

        if (f32) {
            if (!ExList.Emit((PBYTE) &Hdr.PE32, sizeof(Hdr.PE32))) {
                rc = ERROR_OPERATION_ABORTED;
                __leave;
            }
        } else {
            if (!ExList.Emit((PBYTE) &Hdr.PE64, sizeof(Hdr.PE64))) {
                rc = ERROR_OPERATION_ABORTED;
                __leave;
            }
        }

        // Then the section headers.

        if (!ExList.Emit((PBYTE) SectionHeaders, SectionHeaderSize)) {
            rc = ERROR_OPERATION_ABORTED;
            __leave;
        }

        MemFree(SectionHeaders);

        // The headers are done.  Now let's see what we need to do with the import information.

        if (!(DigestLevel & CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO)) {
            // The user didn't explicitly ask for all import info.
            // Add the info modified by bind to the exclude list.

            PIMAGE_IMPORT_DESCRIPTOR ImportDesc;
            DWORD ImportDescSize, IATSize;
            PVOID IAT;

            ImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ImageDirectoryEntryToData(
                                LoadedImage.MappedAddress,
                                FALSE,
                                IMAGE_DIRECTORY_ENTRY_IMPORT,
                                &ImportDescSize);
            if (ImportDescSize) {

                IAT = ImageDirectoryEntryToData(LoadedImage.MappedAddress,
                                                FALSE,
                                                IMAGE_DIRECTORY_ENTRY_IAT,
                                                &IATSize);

                if (IAT) {
                    // Easy case.  All the IATs are grouped together.
                    ExList.Add((DWORD_PTR) IAT, IATSize, Raw);

                    // Add the TimeDateStamp and ForwarderChain fields in the Import Descriptors

                    while (ImportDesc->Characteristics) {
                        ExList.Add((DWORD_PTR) &ImportDesc->TimeDateStamp, 8, Raw);
                        ImportDesc++;
                    }

                } else {
                    // Not so easy.  Need to walk each Import descriptor to find the bounds of the IAT
                    //  (note, there's no requirement that all the IAT's for all descriptors be contiguous).


                    while (ImportDesc->Characteristics) {
                        PIMAGE_THUNK_DATA ThunkStart;
                        ExList.Add((DWORD_PTR)&ImportDesc->TimeDateStamp, 8, Raw);
                        ThunkStart = (PIMAGE_THUNK_DATA) ImageRvaToVa((PIMAGE_NT_HEADERS)LoadedImage.FileHeader,
                                                                      LoadedImage.MappedAddress,
                                                                      (ULONG) ImportDesc->OriginalFirstThunk,
                                                                      NULL);
                        if (f32) {
                            PIMAGE_THUNK_DATA32 Thunk = (PIMAGE_THUNK_DATA32)ThunkStart;
                            while (Thunk->u1.AddressOfData) {
                                Thunk++;
                            }
                            ExList.Add( (DWORD)ImportDesc->FirstThunk,
                                        (DWORD)((DWORD_PTR)Thunk - (DWORD_PTR) ThunkStart + sizeof(IMAGE_THUNK_DATA32)), Virtual);
                        } else {
                            PIMAGE_THUNK_DATA64 Thunk = (PIMAGE_THUNK_DATA64)ThunkStart;
                            while (Thunk->u1.AddressOfData) {
                                Thunk++;
                            }
                            ExList.Add( (DWORD)ImportDesc->FirstThunk,
                                        (DWORD)((DWORD_PTR)Thunk - (DWORD_PTR) ThunkStart + sizeof(IMAGE_THUNK_DATA64)), Virtual);
                        }
                        ImportDesc++;
                    }
                }
            }
        }

        // Add each section header followed by the data from that section.

        for (i = 0; i < LoadedImage.NumberOfSections; i++) {
            if (!ExList.Emit((PBYTE) (LoadedImage.MappedAddress + LoadedImage.Sections[i].PointerToRawData),
                             LoadedImage.Sections[i].SizeOfRawData))
            {
                rc = ERROR_OPERATION_ABORTED;
                __leave;
            }
        }
        rc = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    UnMapIt(&LoadedImage);

    SetLastError(rc);

    return(rc == ERROR_SUCCESS ? TRUE : FALSE);
}



BOOL
IMAGEAPI
ImageAddCertificate(
    IN HANDLE               FileHandle,
    IN LPWIN_CERTIFICATE    Certificate,
    OUT PDWORD              Index
    )

/*++

Routine Description:
    Add a certificate to the image.  There is no checking to ensure there are no
    duplicate types.

Arguments:

    FileHandle      -   Handle to the file in question.  The file should be opened
                        with at least GENERIC_WRITE access.

    Certificate     -   Pointer to a WIN_CERTIFICATE structure.

    Index           -   After adding the Certificate to the image, this is the index
                        you can use for later references to that certificate.

Return Value:

    TRUE    - Success
    FALSE   - There was some error.  Call GetLastError() for more information.

--*/

{
    LOADED_IMAGE        LoadedImage;
    DWORD               rc;
    LPWIN_CERTIFICATE   pCert;
    DWORD               OnDiskCertLength;
    DWORD_PTR           NewCertLocation;
    DWORD               OriginalImageSize;
    PIMAGE_DATA_DIRECTORY  pDataDir;
    BOOL                f32;

    if (MapIt(FileHandle, &LoadedImage, MAP_READWRITE) == FALSE) {
        // Unable to map the image.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = ERROR_INVALID_PARAMETER;

    __try {

        if (LoadedImage.fDOSImage) {
            __leave;
        }

        if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            f32 = TRUE;
            pDataDir = &((PIMAGE_NT_HEADERS32)(LoadedImage.FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            f32 = FALSE;
            pDataDir = &((PIMAGE_NT_HEADERS64)(LoadedImage.FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else {
            __leave;
        }

        pCert = (LPWIN_CERTIFICATE) Certificate;

        // Test the output parameter and the the cert.

        *Index = (DWORD) -1;
        OnDiskCertLength = pCert->dwLength;
        OnDiskCertLength = (OnDiskCertLength + 7) & ~7;        // Round the size of cert.

        // Grow the image.

        OriginalImageSize = LoadedImage.SizeOfImage;
        OriginalImageSize = (OriginalImageSize + 7) & ~7;      // Round the size of Image.

        // Check if the cert pointer is at least reasonable.
        if (pDataDir->VirtualAddress &&
            (pDataDir->VirtualAddress + pDataDir->Size) > LoadedImage.SizeOfImage)
        {
            __leave;
        }

        // Looks good now.

        *Index = 0;
        if (pDataDir->VirtualAddress == 0) {
            pDataDir->VirtualAddress = OriginalImageSize;
            pDataDir->Size = 0;
            NewCertLocation = OriginalImageSize;
        } else {
            LPWIN_CERTIFICATE CurrentCert;

            NewCertLocation = pDataDir->VirtualAddress + pDataDir->Size + (DWORD_PTR) LoadedImage.MappedAddress;
            CurrentCert = (LPWIN_CERTIFICATE) (LoadedImage.MappedAddress + pDataDir->VirtualAddress);
            while (((DWORD_PTR)CurrentCert) < NewCertLocation) {
                if (CurrentCert->dwLength == 0) {
                    __leave;
                }
                CurrentCert = (LPWIN_CERTIFICATE)(((DWORD_PTR)CurrentCert + CurrentCert->dwLength + 7) & ~7);
                (*Index)++;
            }
            NewCertLocation -= (DWORD_PTR) LoadedImage.MappedAddress;
        }

        if (!GrowMap (&LoadedImage, OnDiskCertLength + (OriginalImageSize - LoadedImage.SizeOfImage))) {
             __leave;
        }

        if (NewCertLocation < OriginalImageSize) {
            // There's data after the current security data.  Move it down.
            memmove(LoadedImage.MappedAddress + NewCertLocation + pCert->dwLength,
                    LoadedImage.MappedAddress + NewCertLocation,
                    (unsigned) (OriginalImageSize - NewCertLocation));
        }

        memmove(LoadedImage.MappedAddress + NewCertLocation,
                pCert,
                pCert->dwLength);

        // GrowMap may have moved the dirs.
        if (f32) {
            pDataDir = &((PIMAGE_NT_HEADERS32)(LoadedImage.FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else {
            pDataDir = &((PIMAGE_NT_HEADERS64)(LoadedImage.FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        }

        pDataDir->Size += OnDiskCertLength;
        rc = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    UnMapIt(&LoadedImage);

    SetLastError(rc);

    return(rc == ERROR_SUCCESS ? TRUE : FALSE);
}


BOOL
IMAGEAPI
ImageRemoveCertificate(
    IN HANDLE       FileHandle,
    IN DWORD        Index
    )

/*++

Routine Description:

    Remove a certificate from an image.

Arguments:

    FileHandle  -   Handle to the file in question.  The file should be opened
                    with at least GENERIC_WRITE access.

    Index       -   The index to remove from the image.

Return Value:

    TRUE    - Successful

    FALSE   - There was some error.  Call GetLastError() for more information.

--*/

{
    LOADED_IMAGE    LoadedImage;
    LPWIN_CERTIFICATE CurrentCert;
    DWORD           rc;
    DWORD           OldCertLength;

    if (MapIt(FileHandle, &LoadedImage, MAP_READWRITE) == FALSE) {
        // Unable to map the image.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = ERROR_INVALID_PARAMETER;

    __try {

        if (FindCertificate(&LoadedImage, Index, &CurrentCert) == FALSE) {
            __leave;
        }

        OldCertLength = CurrentCert->dwLength;
        OldCertLength = (OldCertLength + 7) & ~7;           // The disk size is actually a multiple of 8

        memmove(CurrentCert,
                ((PCHAR)CurrentCert) + OldCertLength,
                (size_t)(LoadedImage.SizeOfImage - (((DWORD_PTR)CurrentCert) - (DWORD_PTR)LoadedImage.MappedAddress) - OldCertLength));

        if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            ((PIMAGE_NT_HEADERS32)LoadedImage.FileHeader)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size -= OldCertLength;
        } else {
            ((PIMAGE_NT_HEADERS64)LoadedImage.FileHeader)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size -= OldCertLength;
        }

        LoadedImage.SizeOfImage -= OldCertLength;

        rc = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    UnMapIt(&LoadedImage);

    SetLastError(rc);

    return(rc == ERROR_SUCCESS ? TRUE : FALSE);
}


BOOL
IMAGEAPI
ImageEnumerateCertificates(
    IN  HANDLE      FileHandle,
    IN  WORD        TypeFilter,
    OUT PDWORD      CertificateCount,
    IN OUT PDWORD   Indices OPTIONAL,
    IN  DWORD       IndexCount  OPTIONAL
    )

/*++

Routine Description:

    Enumerate the certificates in an image.

Arguments:

    FileHandle          -   Handle to the file in question.  The file should be opened
                            with at least GENERIC_READ access.

    TypeFilter          -   The filter to apply when enumertating the certificates.
                            Valid values are:

                                CERT_SECTION_TYPE_ANY - Enumerate all certificate types
                                                        in the image.

    CertificateCount    -   How many certificates are in the image.

    Indices             -   An array of indexes that match the filter type.

    IndexCount          -   The number of indexes in the indices array.

Return Value:

    TRUE    - Successful

    FALSE   - There was some error.  Call GetLastError() for more information.

--*/

{
    LOADED_IMAGE LoadedImage;
    BOOL    rc;
    PIMAGE_DATA_DIRECTORY pDataDir;
    LPWIN_CERTIFICATE CurrentCert, LastCert;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;

    if (MapIt(FileHandle, &LoadedImage, MAP_READONLY) == FALSE) {
        // Unable to map the image.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = ERROR_INVALID_PARAMETER;
    __try {
        if (LoadedImage.fDOSImage) {
            __leave;
        }

        if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            pDataDir = &((PIMAGE_NT_HEADERS32)(LoadedImage.FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            pDataDir = &((PIMAGE_NT_HEADERS64)(LoadedImage.FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else {
            __leave;
        }

        if ((pDataDir->VirtualAddress + pDataDir->Size) > LoadedImage.SizeOfImage) {
            *CertificateCount = 0;
            __leave;
        }

        if (!pDataDir->VirtualAddress || !pDataDir->Size) {
            *CertificateCount = 0;
        } else {

            DWORD MatchedIndex = 0;
            DWORD ActualIndex = 0;

            CurrentCert = (LPWIN_CERTIFICATE)((DWORD_PTR)LoadedImage.MappedAddress + pDataDir->VirtualAddress);
            LastCert = (LPWIN_CERTIFICATE)((DWORD_PTR)CurrentCert + pDataDir->Size);

            while (CurrentCert < LastCert ) {
                if ((TypeFilter == CERT_SECTION_TYPE_ANY) || (TypeFilter == CurrentCert->wCertificateType)) {
                    if (Indices && (MatchedIndex < IndexCount)) {
                        Indices[MatchedIndex] = ActualIndex;
                    }
                    MatchedIndex++;
                }

                ActualIndex++;
                CurrentCert = (LPWIN_CERTIFICATE)((((DWORD_PTR)CurrentCert + CurrentCert->dwLength) +7) & ~7);
            }

            *CertificateCount = MatchedIndex;
        }

        rc = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    UnMapIt(&LoadedImage);

    SetLastError(rc);

    return(rc == ERROR_SUCCESS ? TRUE : FALSE);
}


BOOL
IMAGEAPI
ImageGetCertificateData(
    IN  HANDLE              FileHandle,
    IN  DWORD               CertificateIndex,
    OUT LPWIN_CERTIFICATE   Certificate,
    IN OUT PDWORD           RequiredLength
    )

/*++

Routine Description:

    Given a specific certificate index, retrieve the certificate data.

Arguments:

    FileHandle          -   Handle to the file in question.  The file should be opened
                            with at least GENERIC_READ access.

    CertificateIndex    -   Index to retrieve

    Certificate         -   Output buffer where the certificate is to be stored.

    RequiredLength      -   Size of the certificate buffer (input).  On return, is
                            set to the actual certificate length.  NULL can be used
                            to determine the size of a certificate.

Return Value:

    TRUE    - Successful

    FALSE   - There was some error.  Call GetLastError() for more information.

--*/

{
    LOADED_IMAGE LoadedImage;
    DWORD   ErrorCode;

    LPWIN_CERTIFICATE ImageCert;

    if (MapIt(FileHandle, &LoadedImage, MAP_READONLY) == FALSE) {
        // Unable to map the image.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    ErrorCode = ERROR_INVALID_PARAMETER;
    __try {
        if (FindCertificate(&LoadedImage, CertificateIndex, &ImageCert) == FALSE) {
            __leave;
        }

        if (*RequiredLength < ImageCert->dwLength) {
            *RequiredLength = ImageCert->dwLength;
            ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        } else {
            memcpy(Certificate, (PUCHAR)ImageCert, ImageCert->dwLength);
            ErrorCode = ERROR_SUCCESS;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    UnMapIt(&LoadedImage);

    SetLastError(ErrorCode);
    return(ErrorCode == ERROR_SUCCESS ? TRUE: FALSE);
}


BOOL
IMAGEAPI
ImageGetCertificateHeader(
    IN      HANDLE              FileHandle,
    IN      DWORD               CertificateIndex,
    IN OUT  LPWIN_CERTIFICATE   CertificateHeader
    )

/*++

Routine Description:

    Given a specific certificate index, retrieve the certificate data.

Arguments:

    FileHandle          -   Handle to the file in question.  The file should be opened
                            with at least GENERIC_READ access.

    CertificateIndex    -   Index to retrieve.

    CertificateHeader   -   Pointer to a WIN_CERTIFICATE to fill in.

Return Value:

    TRUE    - Success

    FALSE   - There was some error.  Call GetLastError() for more information.

--*/

{
    LOADED_IMAGE LoadedImage;
    LPWIN_CERTIFICATE ImageCert;
    BOOL    rc;

    if (MapIt(FileHandle, &LoadedImage, MAP_READONLY) == FALSE) {
        // Unable to map the image.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (FindCertificate(&LoadedImage, CertificateIndex, &ImageCert) == FALSE) {
        rc = FALSE;
        goto Exit;
    }

    __try {
        memcpy(CertificateHeader, ImageCert, sizeof(WIN_CERTIFICATE));
        rc = TRUE;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        rc = FALSE;
    }
Exit:

    UnMapIt(&LoadedImage);

    if (rc == FALSE) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return(rc);
}

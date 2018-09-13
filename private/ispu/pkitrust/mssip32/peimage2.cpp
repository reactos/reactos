//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       peimage2.cpp
//
//  Contents:   Microsoft SIP Provider
//
//  History:    14-Mar-1997 pberkman   created
//
//--------------------------------------------------------------------------


#include    "global.hxx"

__inline DWORD AlignIt (DWORD Value, DWORD Alignment) { return (Value + (Alignment - 1)) & ~(Alignment -1); }

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define MAP_READONLY  TRUE
#define MAP_READWRITE FALSE

BOOL
CalculateImagePtrs(
    PLOADED_IMAGE LoadedImage
    )
{
    PIMAGE_DOS_HEADER DosHeader;
    BOOL fRC = FALSE;

    // Everything is mapped. Now check the image and find nt image headers

    __try {
        DosHeader = (PIMAGE_DOS_HEADER)LoadedImage->MappedAddress;

        if ((DosHeader->e_magic != IMAGE_DOS_SIGNATURE) &&
            (DosHeader->e_magic != IMAGE_NT_SIGNATURE)) {
            __leave;
        }

        if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
            if (DosHeader->e_lfanew == 0) {
                __leave;
            }
            LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

            if (
                // If IMAGE_NT_HEADERS would extend past the end of file...
                (PBYTE)LoadedImage->FileHeader + sizeof(IMAGE_NT_HEADERS) >
                    (PBYTE)LoadedImage->MappedAddress + LoadedImage->SizeOfImage ||

                // ..or if it would begin in, or before the IMAGE_DOS_HEADER...
                (PBYTE)LoadedImage->FileHeader <
                    (PBYTE)LoadedImage->MappedAddress + sizeof(IMAGE_DOS_HEADER)  )
            {
                // ...then e_lfanew is not as expected.
                // (Several Win95 files are in this category.)
                __leave;
            }
        } else {

            // No DOS header indicates an image built w/o a dos stub

            LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)DosHeader;
        }

        if ( LoadedImage->FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
            __leave;
        }

        // No optional header indicates an object...

        if ( !LoadedImage->FileHeader->FileHeader.SizeOfOptionalHeader ) {
            __leave;
        }

        // Check for versions < 2.50

        if ( LoadedImage->FileHeader->OptionalHeader.MajorLinkerVersion < 3 &&
             LoadedImage->FileHeader->OptionalHeader.MinorLinkerVersion < 5 ) {
            __leave;
        }

        InitializeListHead( &LoadedImage->Links );
        LoadedImage->NumberOfSections = LoadedImage->FileHeader->FileHeader.NumberOfSections;
        LoadedImage->Sections = IMAGE_FIRST_SECTION(LoadedImage->FileHeader);
        fRC = TRUE;

    } __except ( EXCEPTION_EXECUTE_HANDLER ) { }

    return fRC;
}

BOOL
MapIt(
    HANDLE hFile,
    PLOADED_IMAGE LoadedImage
    )
{
    HANDLE hMappedFile;

    hMappedFile = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );
    if ( !hMappedFile ) {
        return FALSE;
    }

    LoadedImage->MappedAddress = (PUCHAR) MapViewOfFile(
                                    hMappedFile,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    0
                                    );

    CloseHandle(hMappedFile);

    LoadedImage->SizeOfImage = GetFileSize(hFile, NULL);

    if (!LoadedImage->MappedAddress) {
        return (FALSE);
    }

    if (!CalculateImagePtrs(LoadedImage)) {
        UnmapViewOfFile(LoadedImage->MappedAddress);
        return(FALSE);
    }

    LoadedImage->hFile = INVALID_HANDLE_VALUE;

    return(TRUE);
}

typedef struct _EXCLUDE_RANGE {
    PBYTE Offset;
    DWORD Size;
    struct _EXCLUDE_RANGE *Next;
} EXCLUDE_RANGE;

class EXCLUDE_LIST
{
    public:
        EXCLUDE_LIST() {
            m_Image = NULL;
            m_ExRange = new EXCLUDE_RANGE;

            if(m_ExRange)
                memset(m_ExRange, 0x00, sizeof(EXCLUDE_RANGE));
        }

        ~EXCLUDE_LIST() {
            EXCLUDE_RANGE *pTmp;
            pTmp = m_ExRange->Next;
            while (pTmp)
            {
                DELETE_OBJECT(m_ExRange);
                m_ExRange = pTmp;
                pTmp = m_ExRange->Next;
            }
            DELETE_OBJECT(m_ExRange);
        }

        void Init(LOADED_IMAGE * Image, DIGEST_FUNCTION pFunc, DIGEST_HANDLE dh) {
            m_Image = Image;
            m_ExRange->Offset = NULL;
            m_ExRange->Size = 0;
            m_pFunc = pFunc;
            m_dh = dh;
            return;
        }

        void Add(DWORD_PTR Offset, DWORD Size);

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
    DWORD Size
    )
{
    EXCLUDE_RANGE *pTmp, *pExRange;

    pExRange = m_ExRange;

    while (pExRange->Next && (pExRange->Next->Offset < (PBYTE)Offset)) {
        pExRange = pExRange->Next;
    }

    pTmp = new EXCLUDE_RANGE;

    if(pTmp)
    {
        pTmp->Next = pExRange->Next;
        pTmp->Offset = (PBYTE)Offset;
        pTmp->Size = Size;
        pExRange->Next = pTmp;
    }

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
            EmitSize = min((DWORD)(pExRange->Offset - Offset), Size);
            if (EmitSize) {
                rc = (*m_pFunc)(m_dh, Offset, EmitSize);
                Size -= EmitSize;
                Offset += EmitSize;
            }
        }

        if (Size) {
            if (pExRange->Offset + pExRange->Size >= Offset) {
                // Skip over what's in the exclude list.
                ExcludeSize = min(Size, (DWORD)(pExRange->Offset + pExRange->Size - Offset));
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
imagehack_IsImagePEOnly(
    IN HANDLE           FileHandle
    )
/*
   What we're looking for here is if there's data outside the exe.
   To do so, find the highest section header offset.  To that, find the
   highest debug directory offset.  Finally, round up to the file alignment
   size, add in the cert size, and compare to the reported image size...
*/
{
    LOADED_IMAGE    LoadedImage;
    DWORD HighOffset;
    DWORD i, Offset, Size;
    LONG DebugDirectorySize, CertSize;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    PVOID CertDir;
    BOOL rc;
    DWORD FileAlignment;
    DWORD NumberOfSections;

    if (MapIt(FileHandle, &LoadedImage) == FALSE) {
        return(FALSE);
    }

    rc = FALSE;

    __try {
        if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            FileAlignment = ((PIMAGE_NT_HEADERS32)LoadedImage.FileHeader)->OptionalHeader.FileAlignment;
        } else if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            FileAlignment = ((PIMAGE_NT_HEADERS64)LoadedImage.FileHeader)->OptionalHeader.FileAlignment;
        } else {
            __leave;
        }

        NumberOfSections =  LoadedImage.FileHeader->FileHeader.NumberOfSections;
        HighOffset = 0;

        for (i = 0; i < NumberOfSections; i++) {
            Offset = LoadedImage.Sections[i].PointerToRawData;
            Size = LoadedImage.Sections[i].SizeOfRawData;
            HighOffset = max(HighOffset, (Offset + Size));
        }

        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
                          ImageDirectoryEntryToData(
                            LoadedImage.MappedAddress,
                            FALSE,
                            IMAGE_DIRECTORY_ENTRY_DEBUG,
                            (ULONG *) &DebugDirectorySize
                          );

        while (DebugDirectorySize > 0) {
            Offset = DebugDirectory->PointerToRawData;
            Size = DebugDirectory->SizeOfData;
            HighOffset = max(HighOffset, (Offset + Size));
            DebugDirectorySize -= sizeof(IMAGE_DEBUG_DIRECTORY);
            DebugDirectory++;
        }

        HighOffset = AlignIt(HighOffset, FileAlignment);

        CertDir = (PVOID) ImageDirectoryEntryToData(
                            LoadedImage.MappedAddress,
                            FALSE,
                            IMAGE_DIRECTORY_ENTRY_SECURITY,
                            (ULONG *) &CertSize
                          );

        if (LoadedImage.SizeOfImage <= (HighOffset + CertSize)) {
            rc = TRUE;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    UnmapViewOfFile(LoadedImage.MappedAddress);

    return(rc);
}

BOOL
imagehack_AuImageGetDigestStream(
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

                        CERT_PE_IMAGE_DIGEST_ALL_BUT_CERTS - Include data outside the PE image itself
                                                              (may include non-mapped debug symbolic)

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
    BOOL            rc;
    EXCLUDE_LIST    ExList;

    if (MapIt(FileHandle, &LoadedImage) == FALSE) {
        // Unable to map the image or invalid digest level.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = ERROR_INVALID_PARAMETER;
    __try {

        if ((LoadedImage.FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) &&
            (LoadedImage.FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC))
        {
            __leave;
        }

        ExList.Init(&LoadedImage, DigestFunction, DigestHandle);

        if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            PIMAGE_NT_HEADERS32 NtHeader32 = (PIMAGE_NT_HEADERS32)(LoadedImage.FileHeader);
            // Exclude the checksum.
            ExList.Add(((DWORD_PTR) &NtHeader32->OptionalHeader.CheckSum),
                       sizeof(NtHeader32->OptionalHeader.CheckSum));

            // Exclude the Security directory.
            ExList.Add(((DWORD_PTR) &NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]),
                       sizeof(NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]));

            // Exclude the certs.
            ExList.Add((DWORD_PTR)NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress +
                                (DWORD_PTR)LoadedImage.MappedAddress,
                       NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
        } else {
            PIMAGE_NT_HEADERS64 NtHeader64 = (PIMAGE_NT_HEADERS64)(LoadedImage.FileHeader);
            // Exclude the checksum.
            ExList.Add(((DWORD_PTR) &NtHeader64->OptionalHeader.CheckSum),
                       sizeof(NtHeader64->OptionalHeader.CheckSum));

            // Exclude the Security directory.
            ExList.Add(((DWORD_PTR) &NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]),
                       sizeof(NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]));

            // Exclude the certs.
            ExList.Add((DWORD_PTR)NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress +
                                (DWORD_PTR)LoadedImage.MappedAddress,
                       NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
        }

        ExList.Emit((PBYTE) (LoadedImage.MappedAddress), LoadedImage.SizeOfImage);
        rc = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    UnmapViewOfFile(LoadedImage.MappedAddress);

    SetLastError(rc);

    return(rc == ERROR_SUCCESS ? TRUE : FALSE);
}

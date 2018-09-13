#include <private.h>

// Random debug info modification functions for imagehlp


BOOL
IMAGEAPI
UpdateDebugInfoFile(
    LPSTR ImageFileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath,
    PIMAGE_NT_HEADERS32 NtHeaders
    )
{
    return UpdateDebugInfoFileEx(
                ImageFileName,
                SymbolPath,
                DebugFilePath,
                NtHeaders,
                NtHeaders->OptionalHeader.CheckSum);
}

BOOL
IMAGEAPI
UpdateDebugInfoFileEx(
    LPSTR ImageFileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath,
    PIMAGE_NT_HEADERS32 NtHeaders,
    DWORD OldCheckSum
    )
{
    // UnSafe...

    HANDLE hDebugFile, hMappedFile;
    PVOID MappedAddress;
    PIMAGE_SEPARATE_DEBUG_HEADER DbgFileHeader;
    DWORD dwError = ERROR_SUCCESS;

#ifdef _BUILDING_UPDDBG_
    OSVERSIONINFO OSVerInfo;
    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OSVerInfo);
#endif

    hDebugFile = FindDebugInfoFile(
                    ImageFileName,
                    SymbolPath,
                    DebugFilePath
                    );
    if ( hDebugFile == NULL ) {
        return FALSE;
    }
    CloseHandle(hDebugFile);

    hDebugFile = CreateFile( DebugFilePath,
                             GENERIC_READ | GENERIC_WRITE,
                             OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : (FILE_SHARE_READ | FILE_SHARE_WRITE),
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );
    if ( hDebugFile == INVALID_HANDLE_VALUE ) {
        return FALSE;
    }

    hMappedFile = CreateFileMapping(
                    hDebugFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL
                    );
    if ( !hMappedFile ) {
        CloseHandle(hDebugFile);
        return FALSE;
    }

    MappedAddress = MapViewOfFile(hMappedFile,
                        FILE_MAP_WRITE,
                        0,
                        0,
                        0
                        );
    CloseHandle(hMappedFile);
    if ( !MappedAddress ) {
        CloseHandle(hDebugFile);
        return FALSE;
    }

    DbgFileHeader = (PIMAGE_SEPARATE_DEBUG_HEADER)MappedAddress;
    if (DbgFileHeader->ImageBase != NtHeaders->OptionalHeader.ImageBase ||
        DbgFileHeader->CheckSum != NtHeaders->OptionalHeader.CheckSum   ||
        DbgFileHeader->SizeOfImage != NtHeaders->OptionalHeader.SizeOfImage
       ) {
        DbgFileHeader->ImageBase = NtHeaders->OptionalHeader.ImageBase;
        if (OldCheckSum != DbgFileHeader->CheckSum) {
            DbgFileHeader->Flags |= IMAGE_SEPARATE_DEBUG_MISMATCH;
            dwError = ERROR_INVALID_DATA;
        }
        DbgFileHeader->CheckSum = NtHeaders->OptionalHeader.CheckSum;
        DbgFileHeader->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
        DbgFileHeader->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
        FlushViewOfFile(MappedAddress,0);
        UnmapViewOfFile(MappedAddress);
        TouchFileTimes(hDebugFile,NULL);
        CloseHandle(hDebugFile);
        SetLastError(dwError);
        return TRUE;
    } else {
        FlushViewOfFile(MappedAddress,0);
        UnmapViewOfFile(MappedAddress);
        CloseHandle(hDebugFile);
        return FALSE;
    }
}
#ifndef _BUILDING_UPDDBG_

BOOL
IMAGEAPI
RemovePrivateCvSymbolic(
    PCHAR   DebugData,
    PCHAR * NewDebugData,
    ULONG * NewDebugSize
    )
{
    BOOL rc;
    rc = RemovePrivateCvSymbolicEx(DebugData, 0, NewDebugData, NewDebugSize);
    if (rc && (*NewDebugSize == 0) && (*NewDebugData == DebugData)) {
        *NewDebugData = NULL;
    }
    return(rc);
}

BOOL
IMAGEAPI
RemovePrivateCvSymbolicEx(
    PCHAR   DebugData,
    ULONG   DebugSize,
    PCHAR * NewDebugData,
    ULONG * NewDebugSize
    )
{
    OMFSignature       *CvDebugData, *NewStartCvSig, *NewEndCvSig;
    OMFDirEntry        *CvDebugDirEntry;
    OMFDirHeader       *CvDebugDirHead;
    unsigned int        i, j;
    PCHAR               NewCvData;
    ULONG               NewCvSize = 0, NewCvOffset;
    BOOL                RC = FALSE;

    __try {
        CvDebugDirHead  = NULL;
        CvDebugDirEntry = NULL;
        CvDebugData = (OMFSignature *)DebugData;

        if ((((*(PULONG)(CvDebugData->Signature)) == '90BN') ||
             ((*(PULONG)(CvDebugData->Signature)) == '80BN') ||
             ((*(PULONG)(CvDebugData->Signature)) == '11BN'))  &&
            ((CvDebugDirHead = (OMFDirHeader *)((PUCHAR) CvDebugData + CvDebugData->filepos)) != NULL) &&
            ((CvDebugDirEntry = (OMFDirEntry *)((PUCHAR) CvDebugDirHead + CvDebugDirHead->cbDirHeader)) != NULL)) {

            // Walk the directory.  Keep what we want, zero out the rest.

            for (i=0, j=0; i < CvDebugDirHead->cDir; i++) {
                switch (CvDebugDirEntry[i].SubSection) {
                    case sstSegMap:
                    case sstSegName:
                    case sstOffsetMap16:
                    case sstOffsetMap32:
                    case sstModule:
                    case SSTMODULE:
                    case SSTPUBLIC:
                    case sstPublic:
                    case sstPublicSym:
                    case sstGlobalPub:
                        CvDebugDirEntry[j] = CvDebugDirEntry[i];
                        NewCvSize += CvDebugDirEntry[j].cb;
                        NewCvSize = (NewCvSize + 3) & ~3;
                        if (i != j++) {
                            // Clear the old entry.
                            RtlZeroMemory(&CvDebugDirEntry[i], CvDebugDirHead->cbDirEntry);
                        }
                        break;

                    default:
                        RC = TRUE;
                        RtlZeroMemory(CvDebugDirEntry[i].lfo + (PUCHAR) CvDebugData, CvDebugDirEntry[i].cb);
                        RtlZeroMemory(&CvDebugDirEntry[i], CvDebugDirHead->cbDirEntry);
                        break;
                }
            }

            // Now, allocate the new cv data.

            CvDebugDirHead->cDir = j;

            NewCvSize += (j * CvDebugDirHead->cbDirEntry) + // The directory itself
                            CvDebugDirHead->cbDirHeader +   // The directory header
                            (sizeof(OMFSignature) * 2);     // The signature/offset pairs at each end.

            NewCvData = (PCHAR) MemAlloc( NewCvSize );

            // And move the stuff we kept into the new section.

            NewCvOffset = sizeof(OMFSignature);

            RtlCopyMemory(NewCvData + NewCvOffset, CvDebugDirHead, CvDebugDirHead->cbDirHeader);

            CvDebugDirHead = (OMFDirHeader *) (NewCvData + NewCvOffset);

            NewCvOffset += CvDebugDirHead->cbDirHeader;

            RtlCopyMemory(NewCvData + NewCvOffset,
                        CvDebugDirEntry,
                        CvDebugDirHead->cDir * CvDebugDirHead->cbDirEntry);

            CvDebugDirEntry = (OMFDirEntry *)(NewCvData + NewCvOffset);

            NewCvOffset += (CvDebugDirHead->cbDirEntry * CvDebugDirHead->cDir);

            for (i=0; i < CvDebugDirHead->cDir; i++) {
                RtlCopyMemory(NewCvData + NewCvOffset,
                            CvDebugDirEntry[i].lfo + (PCHAR) CvDebugData,
                            CvDebugDirEntry[i].cb);
                CvDebugDirEntry[i].lfo = NewCvOffset;
                NewCvOffset += (CvDebugDirEntry[i].cb + 3) & ~3;
            }


            // Re-do the start/end signatures

            NewStartCvSig = (OMFSignature *) NewCvData;
            NewEndCvSig   = (OMFSignature *) ((PCHAR)NewCvData + NewCvOffset);
            *(PULONG)(NewStartCvSig->Signature) = *(PULONG)(CvDebugData->Signature);
            NewStartCvSig->filepos = (int)((PCHAR)CvDebugDirHead - (PCHAR)NewStartCvSig);
            *(PULONG)(NewEndCvSig->Signature) = *(PULONG)(CvDebugData->Signature);
            NewCvOffset += sizeof(OMFSignature);
            NewEndCvSig->filepos = (LONG)NewCvOffset;

            // Set the return values appropriately

            *NewDebugData = NewCvData;
            *NewDebugSize = NewCvSize;

        } else {
            if (*(PULONG)(CvDebugData->Signature) == '01BN') {
                *NewDebugData = DebugData;
                *NewDebugSize = DebugSize;
                RC = TRUE;
            } else {
                // Not NB10, NB09 or NB08.  Forget we ever heard of it.
                *NewDebugData = DebugData;
                *NewDebugSize = 0;
                RC = TRUE;
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        RC = FALSE;
    }

    return(RC);
}

#include <copypdb.c>

#endif

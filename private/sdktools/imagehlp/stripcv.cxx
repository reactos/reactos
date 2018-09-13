#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <imagehlp.h>
#include <cvinfo.h>
#include <private.h>

extern "C"
BOOL
IMAGEAPI
CopyPdb(
    CHAR const * szSrcPdb,
    CHAR const * szDestPdb,
    BOOL StripPrivate
    );

BOOL
StripCv(
    PSZ szImage
    );

void __cdecl main(int argc, char *argv[]);
void Usage(void);

void
Usage (void)
{
    puts("USAGE: StripCV <imagename>\n"
         "\tRemove non-public CV info from an image.\n");
}

void __cdecl
main(
    int argc,
    char *argv[])
{
    int i;

    if (argc < 2) {
        Usage();
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        StripCv(argv[i]);
    }
}

BOOL
StripCv(
    PSZ szImage
    )
{
    PCHAR               CvDebugData;
    unsigned int        i, NumberOfDebugDirectories;
    ULONG               DebugDirectorySize, NewCvSize = 0, CvDebugSize;
    PCHAR               NewCvData;
    HANDLE              FileHandle, hMappedFile;
    PVOID               DebugDirectories, ImageBase;
    PIMAGE_NT_HEADERS   NtHeaders;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    BOOL                fRemoveCV = FALSE;
    BOOL                fDbgFile = FALSE;
    ULONG              *pDebugSize, FileSize;
    BOOL                RC = TRUE;

    FileHandle = CreateFile(
                     szImage,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL );

    if (FileHandle == INVALID_HANDLE_VALUE) {
        printf("Error: Unable to open %s - rc: %d\n", szImage, GetLastError());
        RC = FALSE;
        goto cleanup1;
    }

    FileSize = GetFileSize(FileHandle, NULL);

    hMappedFile = CreateFileMapping( FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL );
    if (!hMappedFile) {
        printf("Error: Unable to create read/write map on %s - rc: %d\n", szImage, GetLastError());
        RC = FALSE;
        goto cleanup2;
    }

    ImageBase = MapViewOfFile( hMappedFile, FILE_MAP_WRITE, 0, 0, 0 );
    if (!ImageBase) {
        printf("Error: Unable to Map view of file %s - rc: %d\n", szImage, GetLastError());
        RC = FALSE;
        goto cleanup3;
    }

    if (*(USHORT *)ImageBase == IMAGE_SEPARATE_DEBUG_SIGNATURE) {

        fDbgFile = TRUE;

        PIMAGE_SEPARATE_DEBUG_HEADER DbgFile = (PIMAGE_SEPARATE_DEBUG_HEADER) ImageBase;

        DebugDirectories = (PIMAGE_DEBUG_DIRECTORY)((PUCHAR)ImageBase +
                                    sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
                                    (DbgFile->NumberOfSections * sizeof(IMAGE_SECTION_HEADER)) +
                                    DbgFile->ExportedNamesSize );
        pDebugSize = &(DbgFile->DebugDirectorySize);

        DebugDirectorySize = DbgFile->DebugDirectorySize;

    } else {

        NtHeaders = ImageNtHeader( ImageBase );
        if (NtHeaders == NULL) {
            printf("Error: %s is not an NT image\n", szImage);
            RC = FALSE;
            goto cleanup4;
        }

        DebugDirectories =
            ImageDirectoryEntryToData(
                ImageBase,
                FALSE,
                IMAGE_DIRECTORY_ENTRY_DEBUG,
                &DebugDirectorySize );
        pDebugSize = &(NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    }

    if (DebugDirectories == NULL || DebugDirectorySize == 0)
    {
        printf("Warning: No debug info found on %s\n", szImage);
        RC = FALSE;
        goto cleanup4;
    }

    DebugDirectory = (PIMAGE_DEBUG_DIRECTORY) DebugDirectories;

    NumberOfDebugDirectories = DebugDirectorySize / sizeof( IMAGE_DEBUG_DIRECTORY );

    CvDebugData = NULL;

    for (i=0; i < NumberOfDebugDirectories; i++) {
        if (DebugDirectory->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            CvDebugData = (PCHAR)ImageBase + DebugDirectory->PointerToRawData;
            CvDebugSize = DebugDirectory->SizeOfData;
            break;
        }

        // Zero out the Fixup data

        if (DebugDirectory->Type == IMAGE_DEBUG_TYPE_FIXUP) {
            printf("Info: Removing Fixup data from %s\n", szImage);
            RtlZeroMemory(((PUCHAR)ImageBase + DebugDirectory->PointerToRawData),
                          DebugDirectory->SizeOfData);
            DebugDirectory = DebugDirectory+1;
            *pDebugSize -= sizeof(IMAGE_DEBUG_DIRECTORY);
        } else {
            DebugDirectory++;
        }
    }

    if (CvDebugData == NULL) {
        printf("Info: No CV Debug found on %s\n", szImage);
        RC = FALSE;
        goto cleanup5;
    }

    if (RemovePrivateCvSymbolicEx(CvDebugData, CvDebugSize, &NewCvData, &NewCvSize)) {
        // No CV debug in new size.  Let's check for PDB's.  If so, copy the pdb to
        // <filename>pub

        typedef struct NB10I {                 // NB10 debug info
            DWORD   nb10;                      // NB10
            DWORD   off;                       // offset, always 0
            DWORD   sig;
            DWORD   age;
        } NB10I;

        NB10I *NB10Data = (NB10I *)CvDebugData;
        PCHAR  szPrivatePdb;
        CHAR  szPublicPdb[_MAX_PATH];

        if (NB10Data->nb10 == '01BN') {
            // It has a NB10 signature.  Get the name.
            szPrivatePdb = CvDebugData + sizeof(NB10I);
            strcpy(szPublicPdb, szPrivatePdb);
            strcat(szPublicPdb, "pub");
            CopyPdb(szPrivatePdb, szPublicPdb, TRUE);
        } else {
            printf("Info: CV types info stripped from %s\n", szImage);
        }
    }

    RtlCopyMemory(CvDebugData, NewCvData, NewCvSize);

    DebugDirectory = (PIMAGE_DEBUG_DIRECTORY) DebugDirectories;

    for (i=0; i < NumberOfDebugDirectories; i++) {
        if (DebugDirectory->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            if (i+1 == NumberOfDebugDirectories) {
                // This is the simple case.  cv is the last entry.  Simply truncate the image.
                FileSize += NewCvSize - DebugDirectory->SizeOfData;
            }
            DebugDirectory->SizeOfData = NewCvSize;
            break;
        }

        DebugDirectory++;
    }

cleanup5:

    // All done.

    if (!fDbgFile) {
        PIMAGE_NT_HEADERS pHdr = NULL;
        DWORD SumHeader;
        DWORD SumTotal;

        pHdr = CheckSumMappedFile(ImageBase, FileSize, &SumHeader, &SumTotal);
        if (pHdr != NULL) {
            pHdr->OptionalHeader.CheckSum = SumTotal;
        }
    }

    FlushViewOfFile(ImageBase, NULL);
cleanup4:
    UnmapViewOfFile(ImageBase);
cleanup3:
    CloseHandle(hMappedFile);
cleanup2:
    SetFilePointer(FileHandle, FileSize, NULL, FILE_BEGIN);
    SetEndOfFile(FileHandle);
    CloseHandle(FileHandle);
cleanup1:
    return(RC);
}


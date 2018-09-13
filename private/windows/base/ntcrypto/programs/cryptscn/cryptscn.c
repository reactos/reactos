#include <windows.h>
#include <stdio.h>

#include "sha.h"

int
UpdateChanges(
    LPSTR szScanFile
    );

int
ScanChanges(
    LPSTR szScanFile
    );

BOOL
CheckHash(
    IN LPSTR    szFileToCheck,
    IN          BYTE HashExpected[A_SHA_DIGEST_LEN],
    IN OUT      BOOL *pfMatch
    );

BOOL
GetHash(
    IN LPSTR    szFileToHash,
    IN          BYTE HashCurrent[A_SHA_DIGEST_LEN]
    );

BOOL
HashDiskImage(
    IN  HANDLE hFile,                   // handle of file to hash
    IN  BYTE FileHash[A_SHA_DIGEST_LEN] // on success, buffer contains file hash
    );

BOOL
TranslateToMapped(
    IN      LPSTR szFileCandidate,
    IN OUT  LPSTR szFileMapped
    );

#define RTN_OK 0
#define RTN_USAGE 1
#define RTN_ERROR 13
#define RTN_CHANGED 14


//
// default file path that defines the changes to check/update
//

#define SCANFILE_DEFAULT    "\\nt\\private\\windows\\base\\ntcrypto\\cryptscn.txt"

//
// environment variable name that provides a mechanism for scanning against
// a remote source tree shared out from the \nt directory.
// eg: \\x86fre\sources is mapped to d:\nt on the source machine.
//

#define MAPPED_SRC_ENV      "CRYPT_MAP_SRC"

//
// variables that indicate if and where the mapped source is
//

BOOL g_fMappedSource = FALSE;
CHAR g_szMappedSource[ MAX_PATH + 1];

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    LPSTR szScanFile = SCANFILE_DEFAULT;
    BOOL fUpdate = FALSE;
    DWORD cchBuf;
    DWORD cchEnv;
    int i;

    if( argc == 2 && (argv[1][0] == '-' || argv[1][0] == '/') && argv[1][1] == '?' ) {
        fprintf(stderr, "Usage: %s [-u] [ScanFilePath]\n", argv[0]);
        fprintf(stderr, " -u Update the default or specified ScanFilePath\n");
        fprintf(stderr, "    ScanFilePath is alternate input file.  Default is:\n");
        fprintf(stderr, " %s\n", SCANFILE_DEFAULT);
        fprintf(stderr, "    Env. Var %s may point to remote sources, eg: \\\\x86fre\\sources\n", MAPPED_SRC_ENV);
        return RTN_USAGE;
    }

    for( i=1 ; i < argc ; i++) {
        if(argv[i][0] == '-' && argv[i][1] == 'u')
            fUpdate = TRUE;
        else
            szScanFile = argv[i];
    }

    //
    // if the MAPPED_SRC_ENV var is set, enable \nt -> MAPPED_SRC_ENV translation
    // otherwise, make current directory %_NTDRIVE% if it is set.
    //


    cchBuf = sizeof(g_szMappedSource) / sizeof(CHAR);
    cchEnv = GetEnvironmentVariableA( MAPPED_SRC_ENV, g_szMappedSource, cchBuf );

    if( cchEnv > 0 && cchEnv <= cchBuf ) {
        g_fMappedSource = TRUE;
    } else {
        CHAR szNTDrive[ MAX_PATH + 1 ];

        cchBuf = sizeof(szNTDrive) / sizeof(CHAR);
        cchEnv = GetEnvironmentVariableA( "_NTDRIVE", szNTDrive, cchBuf );

        if(cchEnv > 0 && cchEnv <= cchBuf)
            SetCurrentDirectoryA( szNTDrive );
    }

    if( fUpdate )
        return UpdateChanges( szScanFile );
    else
        return ScanChanges( szScanFile );
}

int
UpdateChanges(
    LPSTR szScanFile
    )
{
    FILE *streamTemp = NULL;
    FILE *stream = NULL;
    FILE *streamHashes = NULL;
    CHAR szTempFile[ MAX_PATH + 1 ];

    CHAR LineBuf[ 512 ];
    int iRet = RTN_OK;

    //
    // get a temporary file name for the new output file...
    //

    if(GetTempFileNameA( ".", "cry", 0, szTempFile ) == 0)
        return RTN_ERROR;

    //
    // ... and open it for write binary mode.
    //

    streamTemp = fopen( szTempFile, "w+b" );
    if(streamTemp == NULL) {
        iRet = RTN_ERROR;
        goto cleanup;
    }

    //
    // create a temporary intermediate file containing updated hash values.
    // this is done so we can just append all updated hashes to the end
    // of the filtered input file.
    //

    streamHashes = tmpfile();
    if(streamHashes == NULL) {
        iRet = RTN_ERROR;
        goto cleanup;
    }

    //
    // open the input file that defines what files get hashed.
    //

    stream = fopen( szScanFile, "r" );
    if(stream == NULL) {
        iRet = RTN_ERROR;
        goto cleanup;
    }

    while( fgets(LineBuf, sizeof(LineBuf), stream) ) {
        BYTE HashExpected[A_SHA_DIGEST_LEN];

        CHAR szFileCandidate[ MAX_PATH + 1 ];
        CHAR szFileToRead[ MAX_PATH + 1 ];

        int iScanned;

        //
        // just copy comment block
        //

        if( LineBuf[0] == '#' ) {
            fprintf(streamTemp, LineBuf);
            continue;
        }

        //
        // ignore existing hash entries.
        //

        iScanned = sscanf(LineBuf, "%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx %s",
                    &HashExpected[0], &HashExpected[1], &HashExpected[2], &HashExpected[3],
                    &HashExpected[4], &HashExpected[5], &HashExpected[6], &HashExpected[7],
                    &HashExpected[8], &HashExpected[9], &HashExpected[10], &HashExpected[11],
                    &HashExpected[12], &HashExpected[13], &HashExpected[14], &HashExpected[15],
                    &HashExpected[16], &HashExpected[17], &HashExpected[18], &HashExpected[19],
                    szFileCandidate
                    );

        if( iScanned == 21 )
            continue;

        //
        // write out anything that isn't a file specifier.
        //

        fprintf(streamTemp, LineBuf);

        iScanned = sscanf(LineBuf, "%s", szFileCandidate);

        if( iScanned != 1 )
            continue;

        if( szFileCandidate[0] == '\0' )
            continue;

        TranslateToMapped(szFileCandidate, szFileToRead);

        //
        // get current hash and output the result to file.
        //


        if(!GetHash( szFileToRead, HashExpected )) {
            fprintf(stderr, "%s: GetHash failed! (rc=%lu)\n", szFileToRead, GetLastError());
            iRet = RTN_ERROR;
            break;
        }

        //
        // write the updated hash entry out to the intermediate hash file
        //

        fprintf(streamHashes, "%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx%.2lx %s\n",
                    HashExpected[0], HashExpected[1], HashExpected[2], HashExpected[3],
                    HashExpected[4], HashExpected[5], HashExpected[6], HashExpected[7],
                    HashExpected[8], HashExpected[9], HashExpected[10], HashExpected[11],
                    HashExpected[12], HashExpected[13], HashExpected[14], HashExpected[15],
                    HashExpected[16], HashExpected[17], HashExpected[18], HashExpected[19],
                    szFileCandidate
                    );

    }

    //
    // if everything succeeded, append the intermediate hash entry file
    // to the intermediate temporary file.
    //

    if( iRet == RTN_OK ) {

        rewind( streamHashes );

        while( fgets( LineBuf, sizeof(LineBuf), streamHashes ) )
            fprintf(streamTemp, LineBuf);

    }

cleanup:

    if(stream)
        fclose(stream);

    if(streamTemp)
        fclose(streamTemp);

    if(streamHashes)
        fclose(streamHashes);

    //
    // update the original file.
    //

    if( iRet == RTN_OK ) {
        if(!CopyFileA(szTempFile, szScanFile, FALSE)) {
            iRet = RTN_ERROR;
        } else {
            printf("%s: update successful\n", szScanFile);
        }
    }

    DeleteFileA(szTempFile);

    return iRet;
}

int
ScanChanges(
    LPSTR szScanFile
    )
{
    FILE *stream = NULL;
    CHAR LineBuf[ 512 ];
    int iRet = RTN_OK;

    stream = fopen( szScanFile, "r" );
    if(stream == NULL)
        return RTN_ERROR;

    while( fgets(LineBuf, sizeof(LineBuf), stream) ) {
        BYTE HashExpected[A_SHA_DIGEST_LEN];

        CHAR szFileCandidate[ MAX_PATH + 1 ];
        CHAR szFileToRead[ MAX_PATH + 1 ];

        BOOL fMatch = FALSE;
        int iScanned = RTN_OK;

        //
        // ignore # comment block
        //

        if( LineBuf[0] == '#' )
            continue;

        iScanned = sscanf(LineBuf, "%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx%2lx %s",
                    &HashExpected[0], &HashExpected[1], &HashExpected[2], &HashExpected[3],
                    &HashExpected[4], &HashExpected[5], &HashExpected[6], &HashExpected[7],
                    &HashExpected[8], &HashExpected[9], &HashExpected[10], &HashExpected[11],
                    &HashExpected[12], &HashExpected[13], &HashExpected[14], &HashExpected[15],
                    &HashExpected[16], &HashExpected[17], &HashExpected[18], &HashExpected[19],
                    szFileCandidate
                    );

        if(iScanned != 21)
            continue;

        TranslateToMapped(szFileCandidate, szFileToRead);

        if(!CheckHash( szFileToRead, HashExpected, &fMatch )) {
            fprintf(stderr, "%s: CheckHash failed! (rc=%lu)\n", szFileToRead, GetLastError());
            iRet = RTN_CHANGED; // assume the worst: a change occurred.
            continue;
        }

        if(!fMatch) {
            fprintf(stderr, "%s: file has changed\n", szFileToRead);
            iRet = RTN_CHANGED;
        } else {
            printf("%s: no change\n", szFileToRead);
        }

    }

    fclose(stream);

    return iRet;
}

BOOL
TranslateToMapped(
    IN      LPSTR szFileCandidate,
    IN OUT  LPSTR szFileMapped
    )
{
    DWORD cchFileCandidate = lstrlenA(szFileCandidate);

    if( g_fMappedSource && cchFileCandidate > 3 ) {
        CHAR charReplaced = szFileCandidate[3];
        LPSTR szNTDir = "\\nt";
        int icmp;

        szFileCandidate[3] = '\0';

        icmp = lstrcmpiA( szFileCandidate, szNTDir );

        szFileCandidate[3] = charReplaced;

        if( icmp == 0 ) {
            lstrcpyA(szFileMapped, g_szMappedSource);
            lstrcatA(szFileMapped, &szFileCandidate[3]);
            return TRUE;
        }
    }

    CopyMemory( szFileMapped, szFileCandidate, (cchFileCandidate+1) * sizeof(CHAR) );

    return TRUE;
}

BOOL
CheckHash(
    IN LPSTR    szFileToCheck,
    IN          BYTE HashExpected[A_SHA_DIGEST_LEN],
    IN OUT      BOOL *pfMatch
    )
{
    BYTE HashCurrent[A_SHA_DIGEST_LEN];
    HANDLE hFile;
    BOOL fSuccess;

    *pfMatch = FALSE;

    hFile = CreateFileA(
                    szFileToCheck,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

    if(hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    fSuccess = HashDiskImage( hFile, HashCurrent );

    CloseHandle( hFile );

    if( fSuccess ) {
        if(memcmp(HashExpected, HashCurrent, sizeof(HashCurrent)) == 0)
            *pfMatch = TRUE;
    }

    return fSuccess;
}

BOOL
GetHash(
    IN LPSTR    szFileToHash,
    IN          BYTE HashCurrent[A_SHA_DIGEST_LEN]
    )
{
    HANDLE hFile;
    BOOL fSuccess;

    hFile = CreateFileA(
                    szFileToHash,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

    if(hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    fSuccess = HashDiskImage( hFile, HashCurrent );

    CloseHandle( hFile );

    return fSuccess;
}


BOOL
HashDiskImage(
    IN  HANDLE hFile,                   // handle of file to hash
    IN  BYTE FileHash[A_SHA_DIGEST_LEN] // on success, buffer contains file hash
    )
/*++

    This function hashes the file associated with the file specified by the
    hFile parameter.  If the function succeeds, the return value is TRUE,
    and the buffer specified by the FileHash parameter is filled with the
    hash of the file.

    The hashing performed by this function begins at the current file position
    associated with the hFile parameter, and continues until EOF or and error
    occurs.  The caller should make no assumptions about the file position
    upon return of this call.  For that reason, the caller should preserve
    and restore file position associated with hFile parameter if necessary.

    This function should not be called from outside this module, unless
    absolutely necessary, because no caching or other performance improvements
    take place.

    The buffer specified by the FileHash parameter should be
    A_SHA_DIGEST_LEN length (20).

--*/
{
    #define DISK_BUF_SIZE 4096

    BYTE DiskBuffer[DISK_BUF_SIZE];
    A_SHA_CTX context;
    DWORD dwBytesToRead;
    DWORD dwBytesRead;
    BOOL bSuccess = FALSE;

    A_SHAInit(&context);

    dwBytesToRead = sizeof(DiskBuffer);

    do {
        bSuccess = ReadFile(hFile, DiskBuffer, dwBytesToRead, &dwBytesRead, NULL);
        if( !bSuccess )
            break;

        A_SHAUpdate(&context, DiskBuffer, dwBytesRead);
    } while ( dwBytesRead );

    A_SHAFinal(&context, FileHash);

    return bSuccess;
}


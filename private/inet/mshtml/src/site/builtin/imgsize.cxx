//+------------------------------------------------------------------------
//
//  File:       imgsize.cxx
//
//  Contents:   Image size cache
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#include "shfolder.h"

#ifdef UNIX
#ifndef X_MAINWIN_H_
#define X_MAINWIN_H_
#include <mainwin.h>
#endif
#endif

#ifdef _MAC
#include <folders.h>
extern "C" {
Boolean _CopySzToSt(const char* sz, StringPtr st, short cchSt);
void _FSpFormat(const FSSpec* pfss, char* szBuffer);
}
#endif

struct CACHE_ENTRY
{
    DWORD dwHash;
    WORD  wWidth;
    WORD  wHeight;
};

struct CACHE_FILE
{
    DWORD dwMagic;
    DWORD dwVersion;
    DWORD dwHit;
    DWORD dwMiss;
    CACHE_ENTRY aCacheEntry[2046];
};
 
static HANDLE s_hCacheFileMapping;
static CACHE_FILE *s_pCacheFile;
static BOOL   s_fInitializationTried;

#define MAGIC       0xCAC8EF17
#define OBJ_NAME(x) "MSIMGSIZECache" #x
#define FILE_NAME   "MSIMGSIZ.DAT"

//+------------------------------------------------------------------------
//
//  VERSION     Increment this number with each file format change.
//
//-------------------------------------------------------------------------

#define VERSION 1

//+------------------------------------------------------------------------
//
//  Function:   InitImageSizeCache
//
//  Synopsis:   Open the image size cache file.  Create and initialize it
//              if required. 
//
//-------------------------------------------------------------------------

BOOL
InitImageSizeCache()
{
    BOOL   fInitialize = FALSE;
    HANDLE hMutex = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    CACHE_FILE * pCacheFile;

    if (s_fInitializationTried)
        return s_pCacheFile != NULL;

    s_fInitializationTried = TRUE;

#ifdef WIN16
//    MessageBox(NULL, "Need to implement InitImageSizeCache", "BUGWIN16", MB_OK);
#else
    hMutex = CreateMutexA(NULL, FALSE, OBJ_NAME(Mutex));
    if (!hMutex)
        goto Cleanup;

    if (WaitForSingleObject(hMutex, 4000) != WAIT_OBJECT_0)
        goto Cleanup;

    // Did another thread initialize when we were not looking?

    if (s_pCacheFile)
        goto Cleanup;

    // Open or create new file mapping.

    s_hCacheFileMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, OBJ_NAME(Map));
    if (s_hCacheFileMapping)
    {
        BOOL    fVersionMismatch = FALSE;
        pCacheFile = (CACHE_FILE *)MapViewOfFile(
                    s_hCacheFileMapping, 
                    FILE_MAP_ALL_ACCESS,            // access
                    0,                              // offset low
                    0,                              // offset high
                    sizeof(CACHE_FILE));            // number of bytes to map

        if (!pCacheFile)
            goto Cleanup;

        // Punt if some other version of Trident has the file open.

        __try
        {
            fVersionMismatch =      pCacheFile->dwMagic != MAGIC
                                ||  pCacheFile->dwVersion != VERSION;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            goto Cleanup;
        }

        if (fVersionMismatch)
            goto Cleanup;
    }
    else
    {
        // File mapping does not exist.  Open or create file.

        HRESULT hr = S_OK;
        char achBuf[MAX_PATH + sizeof(FILE_NAME) +1];
        int  cch;
#ifdef _MAC
        long    foundDirID;
        short   foundVRefNum;
        FSSpec  FileSpec;

        FindFolder(kOnSystemDisk, kSystemFolderType, false, &foundVRefNum, &foundDirID);
        FileSpec.vRefNum = foundVRefNum;
        FileSpec.parID = foundDirID;
        _CopySzToSt(FILE_NAME, FileSpec.name, 64);

        _FSpFormat(&FileSpec, achBuf);
#else
#ifdef UNIX
        cch = MwGetUserWindowsDirectoryA(achBuf, MAX_PATH);
#else
        hr = SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, achBuf);
        if (hr)
            goto Cleanup;

        strcat(achBuf, "\\Microsoft");
        if (!PathFileExistsA(achBuf) && !CreateDirectoryA(achBuf, NULL))
        {
            goto Cleanup;
        }
        strcat(achBuf, "\\Internet Explorer");
        if (!PathFileExistsA(achBuf) && !CreateDirectoryA(achBuf, NULL))
        {
            goto Cleanup;
        }

        cch = strlen(achBuf);
#endif
        if (cch == 0)
            goto Cleanup;

        if (achBuf[cch - 1] != FILENAME_SEPARATOR)
            achBuf[cch++] = FILENAME_SEPARATOR;
        strcpy(&achBuf[cch], FILE_NAME);
#endif // _MAC

        hFile = CreateFileA(
            achBuf,
            GENERIC_READ | GENERIC_WRITE,       // access mode
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
            NULL,                               // security
            OPEN_ALWAYS,                        // disposition
            0,                                  // flags and attributes
            NULL);                              // template file

        if (hFile == INVALID_HANDLE_VALUE)
            goto Cleanup;

        // Do we need to initialize the file?

        if (GetLastError() != ERROR_ALREADY_EXISTS)
            fInitialize = TRUE;
      
        if (GetFileSize(hFile, NULL) != sizeof(CACHE_FILE))
        {
            fInitialize = TRUE;
            SetFilePointer(hFile, sizeof(CACHE_FILE), NULL, FILE_BEGIN);
            SetEndOfFile(hFile);
        }

        // Create the mapping.

        s_hCacheFileMapping = CreateFileMappingA(
                hFile,                          // file
                NULL,                           // security
                PAGE_READWRITE,                 // protect
                0,                              // size low
                0,                              // size high
                OBJ_NAME(Map));                 // name

        if (!s_hCacheFileMapping)
            goto Cleanup;

        // Map view of file.

        pCacheFile = (CACHE_FILE *)MapViewOfFile(
                    s_hCacheFileMapping, 
                    FILE_MAP_ALL_ACCESS,            // access
                    0,                              // offset low
                    0,                              // offset high
                    sizeof(CACHE_FILE));            // number of bytes to map

        if (!pCacheFile)
            goto Cleanup;

        // Reset the contents of the file if we are opening it for the
        // first time or if the file contents do not look right.

        __try
        {
            if (fInitialize ||
                pCacheFile->dwMagic != MAGIC ||
                pCacheFile->dwVersion != VERSION)
            {
                memset(pCacheFile, 0, sizeof(CACHE_FILE));
                pCacheFile->dwMagic = MAGIC;
                pCacheFile->dwVersion = VERSION;
                FlushViewOfFile(pCacheFile, sizeof(CACHE_FILE));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }

    }
    
    s_pCacheFile = pCacheFile;

Cleanup:
    if (!s_pCacheFile && s_hCacheFileMapping)
    {
        CloseHandle(s_hCacheFileMapping);
        s_hCacheFileMapping = NULL;
    }

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }  

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }
#endif // ndef WIN16

    return s_pCacheFile != NULL;
}

//+------------------------------------------------------------------------
//
//  Function:   DeinitImageSizeCache
//
//  Synopsis:   Undo action of InitImageSizeCache 
//
//-------------------------------------------------------------------------

void
DeinitImageSizeCache()
{
#ifndef WIN16
    if (s_pCacheFile)
    {
        FlushViewOfFile(s_pCacheFile, sizeof(CACHE_FILE));
    }
#endif // ndef WIN16
    if (s_hCacheFileMapping)
    {
        CloseHandle(s_hCacheFileMapping);
    }

    s_pCacheFile = NULL;
    s_hCacheFileMapping = NULL;
}

//+------------------------------------------------------------------------
//
//  Function:   HashData
//
//  Synopsis:   Compute hash of bytes.
//
//-------------------------------------------------------------------------

// BUGBUG (garybu) Hash function is duplicated. Should export common fn from shlwapi.

const static BYTE Translate[256] =
{
    1, 14,110, 25, 97,174,132,119,138,170,125,118, 27,233,140, 51,
    87,197,177,107,234,169, 56, 68, 30,  7,173, 73,188, 40, 36, 65,
    49,213,104,190, 57,211,148,223, 48,115, 15,  2, 67,186,210, 28,
    12,181,103, 70, 22, 58, 75, 78,183,167,238,157,124,147,172,144,
    176,161,141, 86, 60, 66,128, 83,156,241, 79, 46,168,198, 41,254,
    178, 85,253,237,250,154,133, 88, 35,206, 95,116,252,192, 54,221,
    102,218,255,240, 82,106,158,201, 61,  3, 89,  9, 42,155,159, 93,
    166, 80, 50, 34,175,195,100, 99, 26,150, 16,145,  4, 33,  8,189,
    121, 64, 77, 72,208,245,130,122,143, 55,105,134, 29,164,185,194,
    193,239,101,242,  5,171,126, 11, 74, 59,137,228,108,191,232,139,
    6, 24, 81, 20,127, 17, 91, 92,251,151,225,207, 21, 98,113,112,
    84,226, 18,214,199,187, 13, 32, 94,220,224,212,247,204,196, 43,
    249,236, 45,244,111,182,153,136,129, 90,217,202, 19,165,231, 71,
    230,142, 96,227, 62,179,246,114,162, 53,160,215,205,180, 47,109,
    44, 38, 31,149,135,  0,216, 52, 63, 23, 37, 69, 39,117,146,184,
    163,200,222,235,248,243,219, 10,152,131,123,229,203, 76,120,209
};

#define HashData BUGBUGHashData // avoid conflict with shlwapi
void HashData(LPBYTE pbData, DWORD cbData, LPBYTE pbHash, DWORD cbHash)
{
    DWORD i, j;
    //  seed the hash
    for (i = cbHash; i-- > 0;)
        pbHash[i] = (BYTE) i;

    //  do the hash
    for (j = cbData; j-- > 0;)
    {
        for (i = cbHash; i-- > 0;)
            pbHash[i] = Translate[pbHash[i] ^ pbData[j]];
    }
}

//+------------------------------------------------------------------------
//
//  Function:   GetCachedImageSize
//
//  Synopsis:   Get cached image size, returns FALSE if not found. 
//
//-------------------------------------------------------------------------

BOOL
GetCachedImageSize(LPCTSTR pchURL, SIZE *psize)
{
    CACHE_ENTRY *pCacheEntry;

    struct { DWORD dw; WORD  w; } Hash;

    if (!InitImageSizeCache())
        return FALSE;

    HashData((BYTE *)pchURL, _tcslen(pchURL) * sizeof(TCHAR), (BYTE *)&Hash, sizeof(Hash));
    if (Hash.dw == 0) Hash.dw = 1;

    pCacheEntry = &s_pCacheFile->aCacheEntry[Hash.w % ARRAY_SIZE(s_pCacheFile->aCacheEntry)];

    // The following chunk of code can read bad values because the cache memory 
    // is shared by multiple processes.  We don't bother with the expense
    // of a mutex because we can tolerate fetching a bad image size.

    //
    // read / write to/from Maped file can raise exceptions
    //
    __try
    {
        if (pCacheEntry->dwHash == Hash.dw)
        {
            psize->cx = pCacheEntry->wWidth;
            psize->cy = pCacheEntry->wHeight;
            #if DBG==1
                s_pCacheFile->dwHit += 1;
            #endif
            return TRUE;
        }
    #if DBG==1
        s_pCacheFile->dwMiss += 1;
    #endif
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return FALSE;
}


//+------------------------------------------------------------------------
//
//  Function:   SetCachedImageSize
//
//  Synopsis:   Save image size for future use. 
//
//-------------------------------------------------------------------------

void
SetCachedImageSize(LPCTSTR pchURL, SIZE size)
{
    CACHE_ENTRY *pCacheEntry;

    struct { DWORD dw; WORD  w; } Hash;

    if (!InitImageSizeCache())
        return;

    HashData((BYTE *)pchURL, _tcslen(pchURL) * sizeof(TCHAR), (BYTE *)&Hash, sizeof(Hash));
    if (Hash.dw == 0) Hash.dw = 1;
    pCacheEntry = &s_pCacheFile->aCacheEntry[Hash.w % ARRAY_SIZE(s_pCacheFile->aCacheEntry)];

    // The following chunk of code can confuse GetCachedImageSize because
    // the cache memory is shared by multiple processes.  We don't 
    // bother with the expense of a mutex because we can tolerate fetching a 
    // bad image size.

    //
    // read / write to/from Maped file can raise exceptions
    //

    __try
    {
        pCacheEntry->wWidth = size.cx;
        pCacheEntry->wHeight = size.cy;
        pCacheEntry->dwHash = Hash.dw;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

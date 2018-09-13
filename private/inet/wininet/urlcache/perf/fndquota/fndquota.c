/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    fndquota.c

Abstract:

    Test program to fill you cache to just under your quota.

Author:

    Vince Roggero (vincentr)  27-Jun-1997


Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <wininet.h>
#include <winineti.h>

//=================================================================================
#define MAX_COMMAND_ARGS    32
#define DEFAULT_BUFFER_SIZE 1024    // 1k
#define URL_NAME_SIZE   (16 + 1)

#define CACHE_ENTRY_BUFFER_SIZE (1024 * 5)
#define CACHE_DATA_BUFFER_SIZE 1024

#define CACHE_HEADER_INFO_SIZE  2048
#define CACHE_HEADER_INFO_SIZE_NORMAL_MAX   256
#define CACHE_HEADER_INFO_SIZE_BIG_MAX      512

//=================================================================================
BYTE GlobalCacheEntryInfoBuffer[CACHE_ENTRY_BUFFER_SIZE];
BYTE GlobalCacheDataBuffer[CACHE_DATA_BUFFER_SIZE];
BYTE GlobalCacheHeaderInfo[CACHE_HEADER_INFO_SIZE];
DWORD g_dwFileSize = 16384;
DWORD g_dwNumEntries = 1;
DWORD g_dwInitEntries = 0;
BOOL g_bVerbose = FALSE;

//=================================================================================
DWORD SetFileSizeByName(LPCTSTR FileName, DWORD FileSize)
/*++

Routine Description:
    Set the size of the specified file.

Arguments:
    FileName : full path name of the file whose size is asked for.
    FileSize : new size of the file.

Return Value:
    Windows Error Code.

--*/
{
    HANDLE FileHandle;
    DWORD FilePointer;
    DWORD Error = ERROR_SUCCESS;
    DWORD dwFlags = 0;
    DWORD dwCreate;
    BOOL BoolError;

    //
    // get the size of the file being cached.
    //
    dwFlags = 0;
    dwCreate = CREATE_ALWAYS;

    FileHandle = CreateFile(
                    FileName,
                    GENERIC_WRITE,
                    0,   //FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    dwCreate,
                    FILE_ATTRIBUTE_NORMAL | dwFlags,
                    NULL );

    if( FileHandle == INVALID_HANDLE_VALUE ) {
        return( GetLastError() );
    }

    FilePointer = SetFilePointer(FileHandle, FileSize, NULL, FILE_BEGIN );

    if( FilePointer != 0xFFFFFFFF )
    {
        if(!SetEndOfFile( FileHandle ))
            Error = GetLastError();
    }
    else
    {
        Error = GetLastError();
    }

    CloseHandle( FileHandle );
    return( Error );
}

//=================================================================================
FILETIME
GetGmtTime(
    VOID
    )
{
    SYSTEMTIME SystemTime;
    FILETIME Time;

    GetSystemTime( &SystemTime );
    SystemTimeToFileTime( &SystemTime, &Time );

    return( Time );
}


//=================================================================================
DWORD EnumUrlCacheEntries(DWORD *pdwTotal)
{
    DWORD BufferSize, dwSmall=0, dwLarge=0;
    HANDLE EnumHandle;
    DWORD Index = 1, len;
    DWORD dwTotal = 0;
    LPINTERNET_CACHE_ENTRY_INFO lpCEI = (LPINTERNET_CACHE_ENTRY_INFO)GlobalCacheEntryInfoBuffer;
    BOOL bRC;
    char Str[256];
 
    //
    // start enum.
    //
    memset(GlobalCacheEntryInfoBuffer, 0, CACHE_ENTRY_BUFFER_SIZE);

    BufferSize = CACHE_ENTRY_BUFFER_SIZE;
    EnumHandle = FindFirstUrlCacheEntryEx (
        NULL,         // search pattern
        0,            // flags
        0xffffffff,   // filter
        0,            // groupid
        (LPINTERNET_CACHE_ENTRY_INFO)GlobalCacheEntryInfoBuffer,
        &BufferSize,
        NULL,
        NULL,
        NULL
    );

    if( EnumHandle == NULL ) 
    {
        return( GetLastError() );
    }

    ++dwTotal;

    //
    // get more entries.
    //
    for ( ;; )
    {
        memset(GlobalCacheEntryInfoBuffer, 0, CACHE_ENTRY_BUFFER_SIZE);
        BufferSize = CACHE_ENTRY_BUFFER_SIZE;
        if( !FindNextUrlCacheEntryEx(
                EnumHandle,
                (LPINTERNET_CACHE_ENTRY_INFO)GlobalCacheEntryInfoBuffer,
                &BufferSize, NULL, NULL, NULL))
        {
            DWORD Error;

            Error = GetLastError();
            if( Error != ERROR_NO_MORE_ITEMS ) {
                return( Error );
            }

            break;
        }

        ++dwTotal;
    }

    *pdwTotal = dwTotal;
    
    FindCloseUrlCache(EnumHandle);

    return(ERROR_SUCCESS);
}

//=================================================================================
DWORD ProcessSimulateCache(DWORD dwNumUrls)
{
    static DWORD dwUrlNum = 0;
    DWORD Error;
    DWORD i, j;
    CHAR UrlName[ URL_NAME_SIZE ];
    TCHAR LocalFileName[MAX_PATH];
    DWORD FileSize;
    LONGLONG ExpireTime;
    FILETIME LastModTime;
    CHAR TimeBuffer[MAX_PATH];
    DWORD UrlLife;
    DWORD BufferSize;
    DWORD CacheHeaderInfoSize;

    for( i = dwUrlNum; i < (dwUrlNum + dwNumUrls); i++ ) 
    {
        //
        // make a new url name.
        //
        sprintf(UrlName, "http://serv/URL%ld", i);

        //
        // create url file.
        //
        if( !CreateUrlCacheEntry(UrlName, 0, "tmp", LocalFileName, 0 ) ) 
        {
            Error = GetLastError();
            printf( "CreateUrlFile call failed, %ld.\n", Error );
            return( Error );
        }

        //
        // set file size.
        //
        Error = SetFileSizeByName(LocalFileName, g_dwFileSize);
        if( Error != ERROR_SUCCESS ) 
        {
            printf( "SetFileSizeByName call failed, %ld.\n", Error );
            return( Error );
        }

        UrlLife = rand() % 48;

        ExpireTime = (LONGLONG)UrlLife * (LONGLONG)36000000000;
        // in 100 of nano seconds.

        LastModTime = GetGmtTime();
        ExpireTime += *((LONGLONG *)&LastModTime);

        CacheHeaderInfoSize = CACHE_HEADER_INFO_SIZE_NORMAL_MAX;

        //
        // cache this file.
        //
        if( !CommitUrlCacheEntryA(
                        UrlName,
                        LocalFileName,
                        *((FILETIME *)&ExpireTime),
                        LastModTime,
                        NORMAL_CACHE_ENTRY,
                        (LPBYTE)GlobalCacheHeaderInfo,
                        CacheHeaderInfoSize,
                        TEXT("tst"),
                        0 ) ) {
            Error = GetLastError();
            printf( "CreateUrlFile call failed, %ld.\n", Error );
            return( Error );
        }

    }
    dwUrlNum = i;   // Save last for next call

    return( ERROR_SUCCESS );
}

//---------------------------------------------------------------------
void Display_Usage(const char *szApp)
{
    printf("Usage: %s [Options]\r\n\n", szApp);
    printf("Options:\r\n");
    printf("\t-f#   File size of cache entries in bytes.\r\n");
    printf("\t-i#   Initial number of entries to create\r\n");
    printf("\t-n#   Number of entries to create before checking total.\r\n");
    printf("\t-v    Turn on verbose output.\r\n");
}

//---------------------------------------------------------------------
BOOL ParseCommandLine(int argcIn, char *argvIn[])
{
    BOOL bRC = TRUE;
    int argc = argcIn;
    char **argv = argvIn;

    argv++; argc--;
    while( argc > 0 && argv[0][0] == '-' )  
    {
        switch (argv[0][1]) 
        {
            case 'f':
                g_dwFileSize = atoi(&argv[0][2]);
                break;
            case 'i':
                g_dwInitEntries= atoi(&argv[0][2]);
                break;
            case 'n':
                g_dwNumEntries = atoi(&argv[0][2]);
                break;
            case 'v':
                g_bVerbose = TRUE;
                break;
            case '?':
                bRC = FALSE;
                break;
            default:
                bRC = FALSE;
                break;
        }
        argv++; argc--;
    }

    if(bRC == FALSE)
    {
        Display_Usage(argvIn[0]);
        bRC = FALSE;
    }

    return(bRC);

}

//=================================================================================
void __cdecl main(int argc,char *argv[])
{

    DWORD Error;
    DWORD i;
    DWORD dwEntries = 0;
    DWORD dwOldEntries = 0;

    if(!ParseCommandLine(argc, argv))
        return;
    
    //
    // init GlobalCacheHeaderInfo buffer.
    //
    for( i = 0; i < CACHE_HEADER_INFO_SIZE; i++) {
        GlobalCacheHeaderInfo[i] = (BYTE)((DWORD)'0' + i % 10);
    }

    if(g_bVerbose)
        printf("FileSize=%d InitEntries=%d NumEntries=%d\r\n", g_dwFileSize, g_dwInitEntries, g_dwNumEntries);

    if(g_dwInitEntries)
        ProcessSimulateCache(g_dwInitEntries);
        
    while(TRUE)
    {
        ProcessSimulateCache(g_dwNumEntries);
        
        dwOldEntries = dwEntries;
        EnumUrlCacheEntries(&dwEntries);
        if(dwEntries < dwOldEntries)    // Quota has been exceeded
            break;

        if(g_bVerbose)
            printf("Entries=%d\r\n", dwEntries);
    }

    printf("setperfmode on\n");
    printf("setquietmode on\n");
    printf("setfilesize %d\n", g_dwFileSize);
    printf("simcache %d\n", dwOldEntries - 4);
    
    return;
}


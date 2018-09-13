/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tfile.c

Abstract:

    Test program for Win32 Base File API calls

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <memory.h>

#define xassert ASSERT

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    HANDLE iFile,oFile,fFile,mFile,n1,o1;
    DWORD n,n2;
    LONG Size;
    BYTE buff[512],cbuff[512];
    DWORD attr;
    FILETIME Mtime, Mtime0, Mtime1;
    PWIN32_FIND_DATA FindFileData;
    BOOL b;
    WORD w1,w2;
    DWORD dw,spc,bps,fc,tc;
    LPSTR base;
    LPSTR l;
    OFSTRUCT reopen1, reopen2;
    DWORD volsn[2];

    printf("Drives = 0x%lx\n",GetLogicalDrives());
    n = GetLogicalDriveStrings(511,buff);
    if ( n < 512 ) {
        l = buff;
        while ( *l ) {
            printf("%s\n",l);
            n2 = strlen(l);
            l += (n2+1);;
            }
        }
    n = GetLogicalDriveStringsW(511,buff);
    xassert(!SetCurrentDirectory("X:\\"));

    FindFileData = (PWIN32_FIND_DATA)buff;
    fFile =  FindFirstFile(
                "e:\\nt\\dll\\nul",
                FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    FindClose(fFile);

    FindFileData = (PWIN32_FIND_DATA)buff;
    fFile =  FindFirstFile(
                "e:\\",
                FindFileData
                );
    xassert(fFile == INVALID_HANDLE_VALUE);

    b = GetVolumeInformation("",NULL,0,volsn,NULL,NULL,NULL,0);
    assert(!b);

    b = GetVolumeInformation("e:\\",NULL,0,volsn,NULL,NULL,NULL,0);
    assert(b);

    w2 = GetTempFileName("e:\\zzzxtmp","zz",0,buff);
    xassert(w2 == 0 );

    b = GetDiskFreeSpace(NULL,&spc,&bps,&fc,&tc);
    xassert(b && spc && (bps == 512) && fc && tc);
    printf("SPC = %d, BPS = %d, FC = %d, TC = %d\n",spc,bps,fc,tc);

    SearchPath(
        ".;e:\\nt;e:\\nt\\dll",
        "base",
        ".dll",
        512,
        cbuff,
        NULL
        );

    SearchPath(
        NULL,
        "win386.exe",
        ".EXe",
        512,
        cbuff,
        NULL
        );

    (VOID)argc;
    (VOID)argv;
    (VOID)envp;

#if 0
    CreateDirectory("e:\\zyx",NULL);
    xassert(SetCurrentDirectory("e:\\zyx"));
    *buff = '\0';
    xassert(GetTempPath(8,buff) == 7 && !_strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(7,buff) == 7 && !_strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(6,buff) == 7 && _strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(5,buff) == 7 && _strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(4,buff) == 7 && _strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(3,buff) == 7 && _strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(2,buff) == 7 && _strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(1,buff) == 7 && _strcmpi(buff,"e:\\zyx\\") );
    *buff = '\0';
    xassert(GetTempPath(0,buff) == 7 && _strcmpi(buff,"e:\\zyx\\") );
#endif
    xassert(SetCurrentDirectory("e:\\nt"));
    b = SetEnvironmentVariable("TMP","d:\\tmp");
    xassert(b);
#if 0
    *buff = '\0';
    xassert(GetTempPath(8,buff) == 7 && !_strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(7,buff) == 7 && !_strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(6,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(5,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(4,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(3,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(2,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(1,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(0,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
#endif
    b = SetEnvironmentVariable("TMP","d:\\tmp\\");
    xassert(b);
#if 0
    *buff = '\0';
    xassert(GetTempPath(8,buff) == 7 && !_strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(7,buff) == 7 && !_strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(6,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(5,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(4,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(3,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(2,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(1,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
    *buff = '\0';
    xassert(GetTempPath(0,buff) == 7 && _strcmpi(buff,"d:\\tmp\\") );
#endif
    w1 = GetTempFileName("e:\\tmp\\","foo",0,buff);
    iFile = CreateFile(
                buff,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(iFile != INVALID_HANDLE_VALUE);
    w2 = GetTempFileName("e:\\tmp","foobar",w1,buff);
    xassert(w1 == w2);
    oFile = CreateFile(
                buff,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(oFile == INVALID_HANDLE_VALUE);

    CloseHandle(iFile);
    xassert(CopyFile("e:\\nt\\dll\\usersrv.dll",buff,FALSE));
    DeleteFile("xyzzy");
    xassert(MoveFile(buff,"xyzzy"));
    xassert(!DeleteFile(buff));
    DeleteFile("rrxyzzy");
    xassert(MoveFile("xyzzy","rrxyzzy"));

    oFile = CreateFile(
                "rrxyzzy",
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(oFile != INVALID_HANDLE_VALUE);

    iFile = CreateFile(
                "e:\\nt\\dll\\usersrv.dll",
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(iFile != INVALID_HANDLE_VALUE);

    fFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,8192,NULL);
    xassert(fFile);
    base = MapViewOfFile(fFile,FILE_MAP_WRITE,0,0,0);
    xassert(base);
    strcpy(base,"Hello World\n");
    printf("%s",base);
    xassert(UnmapViewOfFile(base));
    CloseHandle(fFile);
    fFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READONLY,0,8192,NULL);
    xassert(fFile);
    CloseHandle(fFile);
    fFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READONLY,0,0,NULL);
    xassert(!fFile);

    mFile = CreateFileMapping(oFile,NULL,PAGE_READONLY,0,0,NULL);
    xassert(mFile);
    n1 = CreateFileMapping(oFile,NULL,PAGE_READONLY,0,0,"named-map");
    xassert(n1);
    o1 = OpenFileMapping(FILE_MAP_WRITE,FALSE,"named-map");
    xassert(o1);
    base = MapViewOfFile(o1,FILE_MAP_WRITE,0,0,0);
    xassert(!base);
    base = MapViewOfFile(n1,FILE_MAP_READ,0,0,0);
    xassert(base);

    FindFileData = (PWIN32_FIND_DATA)buff;
    fFile =  FindFirstFile(
                "e:\\nt\\dll\\usersrv.dll",
                FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);

    l = LocalAlloc(LMEM_ZEROINIT,FindFileData->nFileSizeLow);
    xassert(l);
    b = ReadFile(iFile,l,FindFileData->nFileSizeLow,&n,NULL);
    xassert(b);
    xassert(n == FindFileData->nFileSizeLow);
    CloseHandle(oFile);
    xassert(memcmp(l,base,FindFileData->nFileSizeLow) == 0 );

    xassert(DeleteFile("rrxyzzy"));

    w2 = GetTempFileName("e:\\tmp","foobar",(WORD)0xf7ff,buff);
    xassert(w2 == 0xf7ff);
    oFile = CreateFile(
                buff,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(oFile == INVALID_HANDLE_VALUE);

    w2 = GetTempFileName("e:\\xtmp","foobar",0,buff);
    xassert(w2 == 0);

    xassert(!RemoveDirectory("e:\\"));

    xassert(SetCurrentDirectory("e:\\nt"));
    xassert(!RemoveDirectory("."));
    n = GetCurrentDirectory(0,buff);
    xassert(n == 5);
    n = GetCurrentDirectory(4,buff);
    xassert(n == 5);
    *buff = '\0';
    n = GetCurrentDirectory(5,buff);
    xassert(n == 5 && strcmp(buff,"e:\\nt") == 0 );
    *buff = '\0';
    n = GetCurrentDirectory(500,buff);
    xassert(n == 5 && strcmp(buff,"e:\\nt") == 0 );

    xassert(SetCurrentDirectory("e:\\"));
    *buff = '\0';
    n = GetCurrentDirectory(500,buff);
    xassert(n == 3 && strcmp(buff,"e:\\") == 0 );
    *buff = '\0';
    n = GetCurrentDirectory(3,buff);
    xassert(n == 3 && strcmp(buff,"e:\\") == 0 );
    *buff = '\0';
    n = GetCurrentDirectory(2,buff);
    xassert(n == 3 && strcmp(buff,"e:\\") != 0 );
    xassert(SetCurrentDirectory("e:\\nt"));
    *buff = '\0';
    n = GetCurrentDirectory(500,buff);
    xassert(n == 5 && strcmp(buff,"e:\\nt") == 0 );
    xassert(SetCurrentDirectory("."));
    *buff = '\0';
    n = GetCurrentDirectory(500,buff);
    xassert(n == 5 && strcmp(buff,"e:\\nt") == 0 );

    xassert(GetDriveType("Z:\\") == 1);
    xassert(GetDriveType(".") == 1);
    xassert(GetDriveType("e:\\NT") == 1);
    xassert(GetDriveType(NULL) == DRIVE_FIXED);
    xassert(GetDriveType("e:\\") == DRIVE_FIXED);
    xassert(GetDriveType("A:\\") == DRIVE_REMOVABLE);


    xassert(!CreateDirectory(".",NULL));
    xassert(CreateDirectory("xyzzy",NULL));
    FindFileData = (PWIN32_FIND_DATA)buff;
    fFile =  FindFirstFile(
                "xyzzy\\*.*",
                FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    b = TRUE;
    while(b) {
        printf("0x%08x %08d %s\n",
            FindFileData->dwFileAttributes,
            FindFileData->nFileSizeLow,
            FindFileData->cFileName
            );
        b = FindNextFile(fFile,FindFileData);
        }
    FindClose(fFile);
    xassert(!FindClose(fFile));

    xassert(RemoveDirectory("xyzzy"));

    iFile = CreateFile(
                "",
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(iFile == INVALID_HANDLE_VALUE);

    iFile = CreateFile(
                "nt.cfg",
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(iFile != INVALID_HANDLE_VALUE);
    xassert(!FlushFileBuffers(iFile));

    xassert(LockFile(iFile,0,0,10,0));
    xassert(LockFile(iFile,10,0,10,0));
    xassert(!LockFile(iFile,1,0,1,0));
    xassert(!LockFile(iFile,0,0,11,0));
    xassert(!LockFile(iFile,0,0,20,0));
    xassert(!UnlockFile(iFile,1,0,1,0));
    xassert(!UnlockFile(iFile,0,0,11,0));
    xassert(!UnlockFile(iFile,0,0,20,0));
    xassert(UnlockFile(iFile,0,0,10,0));
    xassert(UnlockFile(iFile,10,0,10,0));
    xassert(LockFile(iFile,0,0,10,0));
    xassert(LockFile(iFile,10,0,10,0));
    fFile = CreateFile(
                "nt.cfg",
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    b = ReadFile(fFile,&buff,1, &n, NULL);
    xassert(!b);
    xassert(SetFilePointer(INVALID_HANDLE_VALUE,0,NULL,FILE_BEGIN) == 0xffffffff);
    b = ReadFile(fFile,&buff,11, &n, NULL);
    xassert(SetFilePointer(fFile,0,NULL,FILE_BEGIN) == 0);
    b = ReadFile(fFile,&buff,11, &n, NULL);
    xassert(!b);
    xassert(SetFilePointer(fFile,10,NULL,FILE_BEGIN) == 10);
    b = ReadFile(fFile,&buff,1,&n,NULL);
    xassert(!b);
    xassert(SetFilePointer(fFile,10,NULL,FILE_BEGIN) == 10);
    b = ReadFile(fFile,&buff,1,&n,NULL);
    xassert(!b);
    xassert(SetFilePointer(fFile,20,NULL,FILE_BEGIN) == 20);
    b = ReadFile(fFile,&buff,1, &n, NULL);
    xassert(b && n == 1);
    xassert(UnlockFile(iFile,0,0,10,0));
    xassert(UnlockFile(iFile,10,0,10,0));
    CloseHandle(fFile);
    SetHandleCount(255);

    oFile = CreateFile(
            "ntos3.cfg",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_HIDDEN,
            NULL
            );
    xassert(oFile != INVALID_HANDLE_VALUE);
    xassert(FlushFileBuffers(oFile));

    //
    // Copy iFile to oFile
    //

    Size = 0;
    b = ReadFile(iFile,&buff,512, &n, NULL);
    while(b && n){
        Size += n;
        b = WriteFile(oFile,&buff,n, &n, NULL);
        xassert(b && n);
        b = ReadFile(iFile,&buff,512, &n, NULL);
        }

    //
    // Make sure that we can not truncate a read only file
    //

    xassert(!SetEndOfFile(iFile));

    //
    // Go back to beginning of the iFile
    //

    xassert(SetFilePointer(iFile,0,NULL,FILE_BEGIN) == 0);
    xassert(SetFilePointer(oFile,-Size,NULL,FILE_CURRENT) == 0);

    b = ReadFile(iFile,&buff,512, &n, NULL);
    xassert(b && n);
    b = ReadFile(oFile,&cbuff,n, &n2, NULL);
    xassert(b && (n2 == n) );
    while(n && (n == n2) ){
        xassert(memcmp(&buff,&cbuff,n) == 0);
        ReadFile(iFile,&buff,512, &n, NULL);
        ReadFile(oFile,&cbuff,n, &n2, NULL);
        xassert(n2 == n);
        }
    printf("End of Loop. n2 = %ld GLE = 0x%lx\n",n2,GetLastError());

    //
    // Truncate the file to 128 bytes
    //

    xassert(SetFilePointer(oFile,128,NULL,FILE_BEGIN) == 128);
    xassert(SetEndOfFile(oFile));

    xassert(SetFilePointer(oFile,0,NULL,FILE_BEGIN) == 0);
    xassert(SetFilePointer(oFile,0,NULL,FILE_END) != -1);
    xassert(SetFilePointer(oFile,0,NULL,FILE_BEGIN) == 0);
    ReadFile(oFile,&cbuff,512, &n2, NULL);
    xassert(n2 == 128);

    CloseHandle(iFile);
    CloseHandle(oFile);

    oFile = CreateFile(
            "ntos3.cfg",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
    xassert(oFile == INVALID_HANDLE_VALUE);

    attr = GetFileAttributes("ntos3.cfg");
    xassert(attr == (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));

    xassert(SetFileAttributes("ntos3.cfg", FILE_ATTRIBUTE_NORMAL));

    attr = GetFileAttributes("ntos3.cfg");
    xassert(attr == FILE_ATTRIBUTE_NORMAL);

    oFile = CreateFile(
                "ntos3.cfg",
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(oFile != INVALID_HANDLE_VALUE);


    xassert(!DeleteFile("ntos3.cfg"));
    CloseHandle(oFile);

    xassert(DeleteFile("ntos3.cfg"));
    oFile = CreateFile(
                "ntos3.cfg",
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    xassert(oFile == INVALID_HANDLE_VALUE);

    FindFileData = (PWIN32_FIND_DATA)buff;
    fFile =  FindFirstFile(
                "dll\\*.*",
                FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    b = TRUE;
    while(b) {
        printf("0x%08x %08d %s\n",
            FindFileData->dwFileAttributes,
            FindFileData->nFileSizeLow,
            FindFileData->cFileName
            );
        b = FindNextFile(fFile,FindFileData);
        }
    FindClose(fFile);
    xassert(!FindClose(fFile));

    n = GetWindowsDirectory(cbuff,512);
    printf("Windows directory %s length %d\n",cbuff,n);
    strcat(cbuff,"\\*.*");
    fFile =  FindFirstFile(
                cbuff,
                FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    b = TRUE;
    while(b) {
        printf("0x%08x %08d %s\n",
            FindFileData->dwFileAttributes,
            FindFileData->nFileSizeLow,
            FindFileData->cFileName
            );
        b = FindNextFile(fFile,FindFileData);
        }
    FindClose(fFile);
    xassert(!FindClose(fFile));

    GetSystemDirectory(cbuff,512);
    strcat(cbuff,"\\*.*");
    fFile =  FindFirstFile(
                cbuff,
                FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    b = TRUE;
    while(b) {
        printf("0x%08x %08d %s\n",
            FindFileData->dwFileAttributes,
            FindFileData->nFileSizeLow,
            FindFileData->cFileName
            );
        b = FindNextFile(fFile,FindFileData);
        }
    FindClose(fFile);
    xassert(!FindClose(fFile));

    iFile = (HANDLE)OpenFile("xyzzy.ggg",&reopen1, OF_PARSE);
    xassert(iFile == 0);
    dw = GetFullPathName("xyzzy.ggg",512,buff,&l);
    xassert(!_stricmp(buff,&reopen1.szPathName[0]));

    iFile = (HANDLE)OpenFile("",&reopen1, OF_READ);
    xassert(iFile == INVALID_HANDLE_VALUE);

    iFile = (HANDLE)OpenFile("ls.exe",&reopen1, OF_READ);
    xassert(iFile && iFile != INVALID_HANDLE_VALUE);
    xassert(!_stricmp("e:\\nt\\bin\\ls.exe",&reopen1.szPathName[0]));
    CloseHandle(iFile);
    reopen2 = reopen1;
    iFile = (HANDLE)OpenFile("ls.exe",&reopen1, OF_VERIFY | OF_READ);
    xassert(iFile && iFile != INVALID_HANDLE_VALUE);
    xassert(!_stricmp("e:\\nt\\bin\\ls.exe",&reopen1.szPathName[0]));
    CloseHandle(iFile);
    iFile = (HANDLE)OpenFile("",&reopen1, OF_REOPEN | OF_READ);
    xassert(iFile && iFile != INVALID_HANDLE_VALUE);
    xassert(!_stricmp("e:\\nt\\bin\\ls.exe",&reopen1.szPathName[0]));
    CloseHandle(iFile);
    iFile = (HANDLE)OpenFile("ls.exe",&reopen2, OF_VERIFY | OF_READ);
    xassert(iFile && iFile != INVALID_HANDLE_VALUE);
    xassert(!_stricmp("e:\\nt\\bin\\ls.exe",&reopen2.szPathName[0]));
    CloseHandle(iFile);
    iFile = (HANDLE)OpenFile("ls.exe",&reopen2, OF_EXIST );
    xassert(!CloseHandle(iFile));
    xassert(CopyFile("e:\\nt\\bin\\ls.exe","xxx.xxx",FALSE));
    iFile = (HANDLE)OpenFile("xxx.xxx",&reopen1, OF_EXIST );
    xassert(!CloseHandle(iFile));
    iFile = (HANDLE)OpenFile("xxx.xxx",&reopen1, OF_READWRITE );
    xassert(iFile && iFile != INVALID_HANDLE_VALUE);
    Sleep(3000);
    b = WriteFile(iFile,"Hello World",12,&n,NULL);
    xassert(b && n == 12);
    CloseHandle(iFile);
    iFile = (HANDLE)OpenFile("xxx.xxx",&reopen1, OF_VERIFY | OF_READ);
    xassert(iFile == INVALID_HANDLE_VALUE);
    iFile = (HANDLE)OpenFile("xxx.xxx",&reopen1, OF_DELETE );
    xassert(!CloseHandle(iFile));
    iFile = (HANDLE)OpenFile("xxx.xxx",&reopen1, OF_EXIST );
    printf("iFile %lx nErrCode %d\n",iFile,reopen1.nErrCode);
    xassert(iFile == INVALID_HANDLE_VALUE && reopen1.nErrCode == ERROR_FILE_NOT_FOUND);

    fFile =  FindFirstFile(
                "e:\\nt\\nt.cfg",
                FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    iFile = (HANDLE)OpenFile("e:\\nt\\nt.cfg",&reopen1, OF_READ);
    xassert(iFile && iFile != INVALID_HANDLE_VALUE);
    n = GetFileSize(iFile,NULL);
    xassert(n != -1);
    xassert(n==FindFileData->nFileSizeLow);
    FindClose(fFile);
    CloseHandle(iFile);

    xassert(CopyFile("e:\\nt\\bin\\ls.exe","xxx.xxx",FALSE));
    iFile = _lopen("xxx.xxx",OF_READ);
    xassert(iFile != INVALID_HANDLE_VALUE);
    oFile = _lcreat("xxx.zzz",0);
    xassert(oFile != INVALID_HANDLE_VALUE);

    Size = 0;
    b = ReadFile(iFile,&buff,512, &n, NULL);
    while(b && n){
        Size += n;
        b = WriteFile(oFile,&buff,n, &n, NULL);
        xassert(b && n);
        b = ReadFile(iFile,&buff,512, &n, NULL);
        }
    CloseHandle(iFile);
    CloseHandle(oFile);

    iFile = _lopen("xxx.xxx",OF_WRITE);
    xassert(iFile != INVALID_HANDLE_VALUE);
    CloseHandle(iFile);
    iFile = _lopen("xxx.xxx",OF_READ);
    b = ReadFile(iFile,&buff,512, &n, NULL);
    xassert(b && n == 0 && GetLastError() == 0);

    return 1;

}

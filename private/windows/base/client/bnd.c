//
// Program to test boundry conditions of APIs which take a string buffer.
// Conditions tested are strlen-1, strlen, and strlen+1
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
//
// Define these to match your machine.  If these are not set properly, its
// very likely the tests will pass when they didn't deserve to. 
//
                                      // List of all logical drives

#define LOGICAL_DRIVES "a:\\\0c:\\\0"
#define LOGICAL_DRIVES_W L"a:\\\0c:\\\0"
#define LEN_LOGICAL_DRIVES 8
#define SYSTEM_DIR  "c:\\winnt\\system32"  // Where is system dir
#define SYSTEM_DIR_W  L"c:\\winnt\\system32"  // Where is system dir
#define WINDOWS_DIR "c:\\winnt"          // Where is windows dir
#define WINDOWS_DIR_W L"c:\\winnt"          // Where is windows dir
#define TEMP_DIRA   "c:\\tmp"         // For GetTempPath, env var tmp or temp
#define TEMP_DIR_WA   L"c:\\tmp"         // For GetTempPath, env var tmp or temp
#define TEMP_DIRB   "c:\\tmp\\"         // For GetTempPath, env var tmp or temp
#define TEMP_DIR_WB   L"c:\\tmp\\"         // For GetTempPath, env var tmp or temp
#define TEMP_DIRC   "c:\\"         // For GetTempPath, env var tmp or temp
#define TEMP_DIR_WC   L"c:\\"         // For GetTempPath, env var tmp or temp
#define VOLUME_NAME "MARKLTST433"        // Name of volume where c: partition is
#define VOLUME_NAME_W L"MARKLTST433"        // Name of volume where c: partition is
#define FILE_SYSTEM_NAME "FAT"        // Name of file system on c: partition
#define FILE_SYSTEM_NAME_W L"FAT"        // Name of file system on c: partition
                                      // { FAT, HPFS, NTFS }

//
// Global vars, constants and prototypes
//

#define SIGNATURE 0xf
#define SIGNATURE_W 0xaaff
#define SIZE_BUFF 128

void init_buff();
void init_buffw();
void check(DWORD retcode, DWORD size_passed, int expected_ret,BOOL shouldxfer);
void checkw(DWORD retcode, DWORD size_passed, int expected_ret,BOOL shouldxfer);

char buff[SIZE_BUFF], *cur_test;
wchar_t buffw[SIZE_BUFF];
LPSTR exp_val;
LPWSTR exp_valw;

//
// main
//

void _cdecl main(void)
{
    int   exp_len;
    int   wexp_len;
    LPSTR lpJunk;
    LPWSTR lpwJunk;
    BOOL  bRet;

    //
    // GetSystemDirectory
    //

    cur_test = "GetSystemDirectory";
    exp_len  = strlen(SYSTEM_DIR);
    exp_val = SYSTEM_DIR;
    exp_valw = SYSTEM_DIR_W;

    printf("\nGetSystemDirectory: Expected string is <%s>\n", SYSTEM_DIR);

    init_buff();
    check(GetSystemDirectory(buff, exp_len-1), exp_len-1, exp_len+1,FALSE);
    init_buff();
    check(GetSystemDirectory(buff, exp_len),   exp_len,   exp_len+1,FALSE);
    init_buff();
    check(GetSystemDirectory(buff, exp_len+1), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetSystemDirectoryW(buffw, exp_len-1), exp_len-1, exp_len+1,FALSE);
    init_buffw();
    checkw(GetSystemDirectoryW(buffw, exp_len),   exp_len,   exp_len+1,FALSE);
    init_buffw();
    checkw(GetSystemDirectoryW(buffw, exp_len+1), exp_len+1, exp_len,TRUE);

    //
    // GetWindowsDirectory
    //

    cur_test = "GetWindowsDirectory";
    exp_len  = strlen(WINDOWS_DIR);
    exp_val = WINDOWS_DIR;
    exp_valw = WINDOWS_DIR_W;

    printf("\nGetWindowsDirectory: Expected string is <%s>\n", WINDOWS_DIR);

    init_buff();
    check(GetWindowsDirectory(buff, exp_len-1), exp_len-1, exp_len+1,FALSE);
    init_buff();
    check(GetWindowsDirectory(buff, exp_len),   exp_len,   exp_len+1,FALSE);
    init_buff();
    check(GetWindowsDirectory(buff, exp_len+1), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetWindowsDirectoryW(buffw, exp_len-1), exp_len-1, exp_len+1,FALSE);
    init_buffw();
    checkw(GetWindowsDirectoryW(buffw, exp_len),   exp_len,   exp_len+1,FALSE);
    init_buffw();
    checkw(GetWindowsDirectoryW(buffw, exp_len+1), exp_len+1, exp_len,TRUE);

    //
    // GetLogicalDriveStrings 
    //

    cur_test = "GetLogicalDriveStrings";
    exp_len  = LEN_LOGICAL_DRIVES;
    exp_val = LOGICAL_DRIVES;
    exp_valw = LOGICAL_DRIVES_W;

    printf("\nGetLogicalDriveStrings: Expected string is <%s>\n",
           LOGICAL_DRIVES);

    init_buff();
    check(GetLogicalDriveStrings(exp_len-1, buff), exp_len-1, exp_len+1,FALSE);
    init_buff();
    check(GetLogicalDriveStrings(exp_len,   buff), exp_len,   exp_len+1,FALSE);
    init_buff();
    check(GetLogicalDriveStrings(exp_len+1, buff), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetLogicalDriveStringsW(exp_len-1, buffw), exp_len-1, exp_len+1,FALSE);
    init_buffw();
    checkw(GetLogicalDriveStringsW(exp_len,   buffw), exp_len,   exp_len+1,FALSE);
    init_buffw();
    checkw(GetLogicalDriveStringsW(exp_len+1, buffw), exp_len+1, exp_len,TRUE);

    //
    // GetVolumeInformation: Be sure that does not stomp on VolumeName buffer,
    // and be sure it returns FALSE when the volume name would not fit into
    // the provided buffer
    //

    cur_test = "GetVolumeInformation 1";
    exp_len  = strlen(VOLUME_NAME);
    exp_val = VOLUME_NAME;
    exp_valw = VOLUME_NAME_W;

    printf("\nGetVolumeInformation 1: Expected string is <%s>\n", VOLUME_NAME);

    init_buff();
    bRet = GetVolumeInformation("c:\\",
                                buff,      // volume name buffer
                                exp_len-1,
                                NULL, NULL, NULL, NULL, 0);

    check(bRet, exp_len-1, FALSE,FALSE);

    init_buff();
    bRet = GetVolumeInformation("c:\\",
                                buff,      // volume name buffer
                                exp_len,
                                NULL, NULL, NULL, NULL, 0);

    check(bRet, exp_len, FALSE,FALSE);

    init_buff();
    bRet = GetVolumeInformation("c:\\",
                                buff,      // volume name buffer
                                exp_len+1,
                                NULL, NULL, NULL, NULL, 0);
    check(bRet, exp_len+1, TRUE,TRUE);

    init_buffw();
    bRet = GetVolumeInformationW(L"c:\\",
                                buffw,      // volume name buffer
                                exp_len-1,
                                NULL, NULL, NULL, NULL, 0);

    checkw(bRet, exp_len-1, FALSE,FALSE);

    init_buffw();
    bRet = GetVolumeInformationW(L"c:\\",
                                buffw,      // volume name buffer
                                exp_len,
                                NULL, NULL, NULL, NULL, 0);

    checkw(bRet, exp_len, FALSE,FALSE);

    init_buffw();
    bRet = GetVolumeInformationW(L"c:\\",
                                buffw,      // volume name buffer
                                exp_len+1,
                                NULL, NULL, NULL, NULL, 0);
    checkw(bRet, exp_len+1, TRUE,TRUE);

    //
    // GetVolumeInformation: Be sure that does not stomp on the File system
    // name buffer this time, and be sure returns FALSE when not enough room.
    //

    cur_test = "GetVolumeInformation 2";
    exp_len  = strlen(FILE_SYSTEM_NAME);
    exp_val = FILE_SYSTEM_NAME;
    exp_valw = FILE_SYSTEM_NAME_W;

    printf("\nGetVolumeInformation 2: Expected string is <%s>\n",
            FILE_SYSTEM_NAME);

    init_buff();
    bRet = GetVolumeInformation("c:\\",
                                NULL, 0, NULL, NULL, NULL,
                                buff,      // file system name buffer, ex: FAT
                                exp_len-1);

    check(bRet, exp_len-1, FALSE,FALSE);

    init_buff();
    bRet = GetVolumeInformation("c:\\",
                                NULL, 0, NULL, NULL, NULL,
                                buff,      // file system name buffer, ex: FAT
                                exp_len);

    check(bRet, exp_len, FALSE,FALSE);

    init_buff();
    bRet = GetVolumeInformation("c:\\",
                                NULL, 0, NULL, NULL, NULL,
                                buff,      // file system name buffer, ex: FAT
                                exp_len+1);

    check(bRet, exp_len+1, TRUE,TRUE);

    init_buffw();
    bRet = GetVolumeInformationW(L"c:\\",
                                NULL, 0, NULL, NULL, NULL,
                                buffw,      // file system name buffer, ex: FAT
                                exp_len-1);

    checkw(bRet, exp_len-1, FALSE,FALSE);

    init_buffw();
    bRet = GetVolumeInformationW(L"c:\\",
                                NULL, 0, NULL, NULL, NULL,
                                buffw,      // file system name buffer, ex: FAT
                                exp_len);

    checkw(bRet, exp_len, FALSE,FALSE);

    init_buffw();
    bRet = GetVolumeInformationW(L"c:\\",
                                NULL, 0, NULL, NULL, NULL,
                                buffw,      // file system name buffer, ex: FAT
                                exp_len+1);

    checkw(bRet, exp_len+1, TRUE,TRUE);

    //
    // GetEnvironmentVariable: set variable JUNK=junk and check
    //

    cur_test = "GetEnvironmentVariable";
    exp_val = "junk";
    exp_valw = L"junk";

    printf("\nGetEnvironmentVariable: Expected string is <%s>\n", "junk");

    if ( ! SetEnvironmentVariable("JUNK", "junk") ) {
        printf("Error setting environment variable\n");
        exit(1);
    }

    init_buff();
    check(GetEnvironmentVariable("JUNK", buff, 3), 3, 5,FALSE);
    init_buff();
    check(GetEnvironmentVariable("JUNK", buff, 4), 4, 5,FALSE);
    init_buff();
    check(GetEnvironmentVariable("JUNK", buff, 5), 5, 4,TRUE);

    init_buffw();
    checkw(GetEnvironmentVariableW(L"JUNK", buffw, 3), 3, 5,FALSE);
    init_buffw();
    checkw(GetEnvironmentVariableW(L"JUNK", buffw, 4), 4, 5,FALSE);
    init_buffw();
    checkw(GetEnvironmentVariableW(L"JUNK", buffw, 5), 5, 4,TRUE);


    //
    // GetCurrentDirectory: set to c:\ and check
    //

    cur_test = "GetCurrentDirectory";
    exp_val = "c:\\winnt";
    exp_valw = L"c:\\winnt";
    exp_len = strlen(exp_val);

    printf("\nGetCurrentDirectory: Expected string is <%s>\n", "c:\\winnt");

    if ( ! SetCurrentDirectory("c:\\winnt") ) {
        printf("Error setting cur dir\n");
        exit(1);
    }

    init_buff();
    check(GetCurrentDirectory(exp_len-1, buff), exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetCurrentDirectory(exp_len, buff), exp_len, exp_len+1,FALSE);
    init_buff();
    check(GetCurrentDirectory(exp_len+1, buff), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetCurrentDirectoryW(exp_len-1, buffw), exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetCurrentDirectoryW(exp_len, buffw), exp_len, exp_len+1,FALSE);
    init_buffw();
    checkw(GetCurrentDirectoryW(exp_len+1, buffw), exp_len+1, exp_len,TRUE);

    cur_test = "GetCurrentDirectory";
    exp_val = "c:\\";
    exp_valw = L"c:\\";
    exp_len = strlen(exp_val);

    printf("\nGetCurrentDirectory: Expected string is <%s>\n", "c:\\");

    if ( ! SetCurrentDirectory("c:\\") ) {
        printf("Error setting cur dir\n");
        exit(1);
    }

    init_buff();
    check(GetCurrentDirectory(exp_len-1, buff), exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetCurrentDirectory(exp_len, buff), exp_len, exp_len+1,FALSE);
    init_buff();
    check(GetCurrentDirectory(exp_len+1, buff), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetCurrentDirectoryW(exp_len-1, buffw), exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetCurrentDirectoryW(exp_len, buffw), exp_len, exp_len+1,FALSE);
    init_buffw();
    checkw(GetCurrentDirectoryW(exp_len+1, buffw), exp_len+1, exp_len,TRUE);


    //
    // GetFullPathName: this assumes that the current directory is c:\, which
    // was set earlier as part of the GetCurrentDirectory test.  c:\junk.txt
    // is a total of 11 chars.
    //

    cur_test = "GetFullPathName";
    exp_val = "c:\\junk.txt";
    exp_valw = L"c:\\junk.txt";
    exp_len  = strlen(exp_val);

    printf("\nGetFullPathName: Expected string is <%s>\n", exp_val);

    init_buff();
    check(GetFullPathName(exp_val, exp_len-1, buff, &lpJunk), exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName(exp_val, exp_len, buff, &lpJunk), exp_len,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName(exp_val, exp_len+1, buff, &lpJunk), exp_len+1,exp_len,TRUE);

    init_buffw();
    checkw(GetFullPathNameW(exp_valw, exp_len-1, buffw, &lpwJunk), exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(exp_valw, exp_len, buffw, &lpwJunk), exp_len,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(exp_valw, exp_len+1, buffw, &lpwJunk), exp_len+1,exp_len,TRUE);

    cur_test = "GetFullPathName";
    exp_val = "\\\\.\\lpt1";
    exp_valw = L"\\\\.\\lpt1";
    exp_len  = strlen(exp_val);

    printf("\nGetFullPathName: Expected string is <%s>\n", exp_val);

    init_buff();
    check(GetFullPathName("lpt1", exp_len-1, buff, &lpJunk), exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName("lpt1", exp_len, buff, &lpJunk), exp_len,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName("lpt1", exp_len+1, buff, &lpJunk), exp_len+1,exp_len,TRUE);

    init_buffw();
    checkw(GetFullPathNameW(L"lpt1", exp_len-1, buffw, &lpwJunk), exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L"lpt1", exp_len, buffw, &lpwJunk), exp_len,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L"lpt1", exp_len+1, buffw, &lpwJunk), exp_len+1,exp_len,TRUE);

    if ( ! SetCurrentDirectory("c:\\winnt\\dump") ) {
        printf("Error setting cur dir\n");
        exit(1);
    }

    cur_test = "GetFullPathName";
    exp_val = "c:\\winnt\\dump";
    exp_valw = L"c:\\winnt\\dump";
    exp_len  = strlen(exp_val);

    printf("\nGetFullPathName: Expected string is <%s>\n", exp_val);

    init_buff();
    check(GetFullPathName(".", exp_len-1, buff, &lpJunk), exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName(".", exp_len, buff, &lpJunk), exp_len,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName(".", exp_len+1, buff, &lpJunk), exp_len+1,exp_len,TRUE);

    init_buffw();
    checkw(GetFullPathNameW(L".", exp_len-1, buffw, &lpwJunk), exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L".", exp_len, buffw, &lpwJunk), exp_len,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L".", exp_len+1, buffw, &lpwJunk), exp_len+1,exp_len,TRUE);

    if ( ! SetCurrentDirectory("c:\\winnt") ) {
        printf("Error setting cur dir\n");
        exit(1);
    }

    cur_test = "GetFullPathName";
    exp_val = "c:\\winnt\\dump";
    exp_valw = L"c:\\winnt\\dump";
    exp_len  = strlen(exp_val);

    printf("\nGetFullPathName: Expected string is <%s>\n", exp_val);

    init_buff();
    check(GetFullPathName("c:dump", exp_len-1, buff, &lpJunk), exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName("c:dump", exp_len, buff, &lpJunk), exp_len,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName("c:dump", exp_len+1, buff, &lpJunk), exp_len+1,exp_len,TRUE);
    init_buffw();
    checkw(GetFullPathNameW(L"c:dump", exp_len-1, buffw, &lpwJunk), exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L"c:dump", exp_len, buffw, &lpwJunk), exp_len,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L"c:dump", exp_len+1, buffw, &lpwJunk), exp_len+1,exp_len,TRUE);

    if ( ! SetCurrentDirectory("c:\\") ) {
        printf("Error setting cur dir\n");
        exit(1);
    }

    cur_test = "GetFullPathName";
    exp_val = "c:\\";
    exp_valw = L"c:\\";
    exp_len  = strlen(exp_val);

    printf("\nGetFullPathName: Expected string is <%s>\n", exp_val);

    init_buff();
    check(GetFullPathName(".", exp_len-1, buff, &lpJunk), exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName(".", exp_len, buff, &lpJunk), exp_len,exp_len+1,FALSE);
    init_buff();
    check(GetFullPathName(".", exp_len+1, buff, &lpJunk), exp_len+1,exp_len,TRUE);

    init_buffw();
    checkw(GetFullPathNameW(L".", exp_len-1, buffw, &lpwJunk), exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L".", exp_len, buffw, &lpwJunk), exp_len,exp_len+1,FALSE);
    init_buffw();
    checkw(GetFullPathNameW(L".", exp_len+1, buffw, &lpwJunk), exp_len+1,exp_len,TRUE);


    //
    // GetTempPath
    //

    cur_test = "GetTempPath";
    exp_len  = strlen(TEMP_DIRB);
    exp_val = TEMP_DIRB;
    exp_valw = TEMP_DIR_WB;
    if ( ! SetEnvironmentVariable("TMP", TEMP_DIRA)) {
        printf("Error setting tmp environment variable\n");
        exit(1);
    }

    printf("\nGetTempPath: Expected string is <%s>\n", TEMP_DIRB);

    init_buff();
    check(GetTempPath(exp_len-1, buff), exp_len-1, exp_len+1,FALSE);
    init_buff();
    check(GetTempPath(exp_len,   buff), exp_len,   exp_len+1,FALSE);
    init_buff();
    check(GetTempPath(exp_len+1, buff), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetTempPathW(exp_len-1, buffw), exp_len-1, exp_len+1,FALSE);
    init_buffw();
    checkw(GetTempPathW(exp_len,   buffw), exp_len,   exp_len+1,FALSE);
    init_buffw();
    checkw(GetTempPathW(exp_len+1, buffw), exp_len+1, exp_len,TRUE);

    cur_test = "GetTempPath";
    exp_len  = strlen(TEMP_DIRB);
    exp_val = TEMP_DIRB;
    exp_valw = TEMP_DIR_WB;
    if ( ! SetEnvironmentVariable("TMP", TEMP_DIRB)) {
        printf("Error setting tmp environment variable\n");
        exit(1);
    }

    printf("\nGetTempPath: Expected string is <%s>\n", TEMP_DIRB);

    init_buff();
    check(GetTempPath(exp_len-1, buff), exp_len-1, exp_len+1,FALSE);
    init_buff();
    check(GetTempPath(exp_len,   buff), exp_len,   exp_len+1,FALSE);
    init_buff();
    check(GetTempPath(exp_len+1, buff), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetTempPathW(exp_len-1, buffw), exp_len-1, exp_len+1,FALSE);
    init_buffw();
    checkw(GetTempPathW(exp_len,   buffw), exp_len,   exp_len+1,FALSE);
    init_buffw();
    checkw(GetTempPathW(exp_len+1, buffw), exp_len+1, exp_len,TRUE);

    cur_test = "GetTempPath";
    exp_len  = strlen(TEMP_DIRC);
    exp_val = TEMP_DIRC;
    exp_valw = TEMP_DIR_WC;
    if ( ! SetEnvironmentVariable("TMP", TEMP_DIRC)) {
        printf("Error setting tmp environment variable\n");
        exit(1);
    }

    printf("\nGetTempPath: Expected string is <%s>\n", TEMP_DIRC);

    init_buff();
    check(GetTempPath(exp_len-1, buff), exp_len-1, exp_len+1,FALSE);
    init_buff();
    check(GetTempPath(exp_len,   buff), exp_len,   exp_len+1,FALSE);
    init_buff();
    check(GetTempPath(exp_len+1, buff), exp_len+1, exp_len,TRUE);

    init_buffw();
    checkw(GetTempPathW(exp_len-1, buffw), exp_len-1, exp_len+1,FALSE);
    init_buffw();
    checkw(GetTempPathW(exp_len,   buffw), exp_len,   exp_len+1,FALSE);
    init_buffw();
    checkw(GetTempPathW(exp_len+1, buffw), exp_len+1, exp_len,TRUE);


    cur_test = "SearchPath";
    exp_len  = strlen("c:\\winnt\\system32\\kernel32.dll");
    exp_val = "c:\\winnt\\system32\\kernel32.dll";
    exp_valw = L"c:\\winnt\\system32\\kernel32.dll";

    printf("\nSearchPath: Expected string is <%s>\n", exp_val);

    init_buff();
    check(SearchPath(NULL,"kernel32",".dll",exp_len-1,buff,&lpJunk),exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(SearchPath(NULL,"kernel32",".dll",exp_len,buff,&lpJunk),exp_len,exp_len+1,FALSE);
    init_buff();
    check(SearchPath(NULL,"kernel32",".dll",exp_len+1,buff,&lpJunk),exp_len+1,exp_len,TRUE);

    init_buffw();
    checkw(SearchPathW(NULL,L"kernel32",L".dll",exp_len-1,buffw,&lpwJunk),exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(SearchPathW(NULL,L"kernel32",L".dll",exp_len,buffw,&lpwJunk),exp_len,exp_len+1,FALSE);
    init_buffw();
    checkw(SearchPathW(NULL,L"kernel32",L".dll",exp_len+1,buffw,&lpwJunk),exp_len+1,exp_len,TRUE);

    cur_test = "GetModuleFileName";
    exp_len  = strlen("c:\\winnt\\system32\\bnd.exe");
    exp_val = "c:\\winnt\\system32\\bnd.exe";
    exp_valw = L"c:\\winnt\\system32\\bnd.exe";

    printf("\nGetModuleFileName: Expected string is <%s>\n", exp_val);

    init_buff();
    check(GetModuleFileName(NULL,buff,exp_len-1),exp_len-1,exp_len+1,FALSE);
    init_buff();
    check(GetModuleFileName(NULL,buff,exp_len),exp_len,exp_len+1,FALSE);
    init_buff();
    check(GetModuleFileName(NULL,buff,exp_len+1),exp_len+1,exp_len,TRUE);

    init_buffw();
    checkw(GetModuleFileNameW(NULL,buffw,exp_len-1),exp_len-1,exp_len+1,FALSE);
    init_buffw();
    checkw(GetModuleFileNameW(NULL,buffw,exp_len),exp_len,exp_len+1,FALSE);
    init_buffw();
    checkw(GetModuleFileNameW(NULL,buffw,exp_len+1),exp_len+1,exp_len,TRUE);


}

//
// Function to init the global buffer to the SIGNATURE
//

void init_buff()
{
    int i;

    for ( i=0; i<SIZE_BUFF; i++ )
        buff[i] = SIGNATURE;

    buff[SIZE_BUFF-1] = '\0'; // to ensure its null-terminated somewhere
                              // (for debugging)
}

void init_buffw()
{
    int i;

    for ( i=0; i<SIZE_BUFF; i++ )
        buffw[i] = SIGNATURE_W;

    buffw[SIZE_BUFF-1] = 0;

}

//
// Function to check whether the API wrote past the end of the buffer, and
// whether it returned what was expected
//
// It assumes that the global vars cur_test is set and buff was used
//

void check(DWORD retcode, DWORD size_passed, int expected_ret, BOOL shouldxfer)
{

    int i;

    if ( buff[size_passed] != SIGNATURE ) {
        printf("\nFAILURE: %s wrote past end of buffer, when passed %d\n",
                cur_test, size_passed);
        return;
    }

    if ( retcode != (DWORD)expected_ret ) {
        printf("\nFAILURE: %s returned %d, expected %d, when passed %d\n",
               cur_test, retcode, expected_ret, size_passed);
        return;
    }

    if ( shouldxfer ) {
        for (i=0;i<expected_ret;i++){
            if ( toupper(buff[i]) != toupper(exp_val[i]) ) {
                printf("\nFAILURE: %s mismatch at %d",cur_test, i);
                return;
            }
        }
    }
    printf("\nPASSED:  %s when passed %d\n", cur_test, size_passed);
}


void checkw(DWORD retcode, DWORD size_passed, int expected_ret,BOOL shouldxfer)
{
    int i;
    if ( buffw[size_passed] != SIGNATURE_W ) {
        printf("\nFAILURE: %s wrote past end of buffer, when passed %d\n",
                cur_test, size_passed);
        return;
    }

    if ( retcode != (DWORD)expected_ret ) {
        printf("\nFAILURE: %s returned %d, expected %d, when passed %d\n",
               cur_test, retcode, expected_ret, size_passed);
        return;
    }

    if (shouldxfer) {
        for (i=0;i<expected_ret;i++){
            if ( towupper(buffw[i]) != towupper(exp_valw[i]) ) {
                printf("\nFAILURE: %s mismatch at %d",cur_test, i);
                return;
            }
        }
    }
    printf("\nPASSED:  %s when passed %d\n", cur_test, size_passed);
}

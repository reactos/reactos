/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tbase.c

Abstract:

    Skeleton for a Win32 Base API Test Program

Author:

    Steve Wood (stevewo) 26-Oct-1990

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <windows.h>

LPSTR Inserts[ 9 ] = {
    "Insert 1",
    "Insert 2",
    "Insert 3",
    "Insert 4",
    "Insert 5",
    "Insert 6",
    "Insert 7",
    "Insert 8",
    "Insert 9"
};

void
TestEnvironment( void );

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    int i;
    HANDLE hMod,x;
    CHAR Buff[256];
    PCHAR s;
    FARPROC f;
    DWORD Version;
    HANDLE Handle;
    DWORD  rc;
    STARTUPINFO StartupInfo;


    GetStartupInfo(&StartupInfo);
    printf("Title %s\n",StartupInfo.lpTitle);


    printf( "TBASE: Entering Test Program\n" );

    assert(GetModuleFileName(0,Buff,256) < 255);
    printf("Image Name %s\n",Buff);
#if 0
    printf( "argc: %ld\n", argc );
    for (i=0; i<argc; i++) {
        printf( "argv[ %3ld ]: '%s'\n", i, argv[ i ] );
        }

    for (i=0; envp[ i ]; i++) {
        printf( "envp[ %3ld ]: %s\n", i, envp[ i ] );
        }

    DbgBreakPoint();

    s = "ync ""Yes or No""";
    printf( "Invoking: '%s'\nResult: %d\n", s, system(s) );

    TestEnvironment();
    for (i=1; i<=256; i++) {
        rc = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            NULL,
                            i, NULL, sizeof( Buff ), (va_list *)Inserts );
        if (rc != 0) {
            rc = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL,
                                i, Buff, rc, (va_list *)Inserts );
            if (rc != 0) {
                cprintf( "SYS%05u: %s\r\n", i, Buff );
                }
            }
        }

    Handle = CreateFile( "\\config.sys",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL
                       );
    if (Handle != INVALID_HANDLE_VALUE) {
        printf( "CreateFile successful, handle = %lX\n", Handle );
        }
    else {
        printf( "CreateFile failed\n" );
        }

    rc = SetFilePointer(Handle, 0, NULL, FILE_END);
    if (rc != -1) {
        printf( "File size = %ld\n", rc );
        }
    else {
        printf( "SetFilePointer failed\n" );
        }

    Version = GetVersion();
    assert( (Version & 0x0000ffff) == 4);
    assert( (Version >> 16) == 0);
#endif
    hMod = LoadLibrary("dbgdll");
    assert(hMod);
    assert(hMod == GetModuleHandle("dbgdll"));

    hMod = LoadLibrary("c:\\nt\\dll\\csr.dll");
    assert(hMod);

    assert(GetModuleFileName(hMod,Buff,256) == strlen("c:\\nt\\dll\\csr.dll")+1);
    assert(_strcmpi(Buff,"c:\\nt\\dll\\csr.dll") == 0 );

    hMod = LoadLibrary("nt\\dll\\csrrtl.dll");
    assert(hMod);

    x = LoadLibrary("csrrtl");
    assert( x && x == hMod);
    assert(FreeLibrary(x));
    assert(FreeLibrary(x));
    hMod = GetModuleHandle("csrrtl");
    assert(hMod == NULL);
    x = LoadLibrary("csrrtl");
    assert( x );
    assert(FreeLibrary(x));

    hMod = LoadLibrary("kernel32");
    assert(hMod);

    f = GetProcAddress(hMod,"GetProcAddress");
    assert(f);
    assert(f == (f)(hMod,"GetProcAddress"));
    assert(f == MakeProcInstance(f,hMod));
    FreeProcInstance(f);
    DebugBreak();
    assert(FreeLibrary(hMod));

//    hMod = LoadLibrary("baddll");
//    assert(!hMod);

    printf( "TBASE: Exiting Test Program\n" );

    return 0;
}

VOID
DumpEnvironment( VOID )
{
    PCHAR s;

    s = (PCHAR)GetEnvironmentStrings();
    while (*s) {
        printf( "%s\n", s );
        while (*s++) {
            }
        }
}

VOID
SetEnvironment(
    PCHAR Name,
    PCHAR Value
    );

VOID
SetEnvironment(
    PCHAR Name,
    PCHAR Value
    )
{
    BOOL Success;

    Success = SetEnvironmentVariable( Name, Value );
    if (Value != NULL) {
        printf( "TENV: set variable %s=%s", Name, Value );
        }
    else {
        printf( "TENV: delete variable %Z", Name );
        }

    if (Success) {
        printf( "\n" );
        }
    else {
        printf( " - failed\n" );
        }

    DumpEnvironment();
    printf( "\n" );
}


void
TestEnvironment( void )
{
    DumpEnvironment();
    SetEnvironment( "aaaa", "12345" );
    SetEnvironment( "aaaa", "1234567890" );
    SetEnvironment( "aaaa", "1" );
    SetEnvironment( "aaaa", "" );
    SetEnvironment( "aaaa", NULL );
    SetEnvironment( "AAAA", "12345" );
    SetEnvironment( "AAAA", "1234567890" );
    SetEnvironment( "AAAA", "1" );
    SetEnvironment( "AAAA", "" );
    SetEnvironment( "AAAA", NULL );
    SetEnvironment( "MMMM", "12345" );
    SetEnvironment( "MMMM", "1234567890" );
    SetEnvironment( "MMMM", "1" );
    SetEnvironment( "MMMM", "" );
    SetEnvironment( "MMMM", NULL );
    SetEnvironment( "ZZZZ", "12345" );
    SetEnvironment( "ZZZZ", "1234567890" );
    SetEnvironment( "ZZZZ", "1" );
    SetEnvironment( "ZZZZ", "" );
    SetEnvironment( "ZZZZ", NULL );
    return;
}

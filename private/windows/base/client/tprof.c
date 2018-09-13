/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tprof.c

Abstract:

    Win32 Base API Test Program for Profile File Management calls

Author:

    Steve Wood (stevewo) 26-Oct-1990

Revision History:

--*/

#include <stdio.h>
#include <string.h>

#define WIN32_CONSOLE_APP
#include <windows.h>


void
DumpProfile(
    LPTSTR ProfileFileName
    )
{
    LPTSTR Sections, Section;
    LPTSTR Keywords, Keyword;
    LPTSTR KeyValue;

    Sections = LocalAlloc( 0, 4096 * sizeof( *Sections ) );
    Keywords = LocalAlloc( 0, 4096 * sizeof( *Keywords ) );
    KeyValue = LocalAlloc( 0, 1024 * sizeof( *KeyValue ) );

#ifdef UNICODE
    printf( "\nDump of %ws\n",
#else
    printf( "\nDump of %s\n",
#endif
            ProfileFileName ? ProfileFileName : TEXT("win.ini")
          );
    *Sections = TEXT('\0');
    if (!GetPrivateProfileString( NULL, NULL, NULL,
                                  Sections, 4096 * sizeof( *Sections ),
                                  ProfileFileName
                                )
       ) {
        printf( "*** Unable to read - rc == %d\n", GetLastError() );
        }

    Section = Sections;
    while (*Section) {
#ifdef UNICODE
        printf( "[%ws]\n",
#else
        printf( "[%s]\n",
#endif
                Section
              );
        *Keywords = TEXT('\0');
        GetPrivateProfileString( Section, NULL, NULL,
                                 Keywords, 4096 * sizeof( *Keywords ),
                                 ProfileFileName
                               );
        Keyword = Keywords;
        while (*Keyword) {
            GetPrivateProfileString( Section, Keyword, NULL,
                                     KeyValue, 1024 * sizeof( *KeyValue ),
                                     ProfileFileName
                                   );
#ifdef UNICODE
            printf( "    %ws=%ws\n",
#else
            printf( "    %s=%s\n",
#endif
                    Keyword, KeyValue
                  );

            while (*Keyword++) {
                }
            }

        while (*Section++) {
            }
        }

    LocalFree( Sections );
    LocalFree( Keywords );
    LocalFree( KeyValue );

    return;
}

void
DumpSection(
    LPTSTR ProfileFileName,
    LPTSTR SectionName
    )
{
    LPTSTR SectionValue;
    LPTSTR s;

    SectionValue = LocalAlloc( 0, 4096 * sizeof( TCHAR ) );

#ifdef UNICODE
    printf( "\nDump of Section %ws in %ws\n",
#else
    printf( "\nDump of Section %s in %s\n",
#endif
              SectionName,
              ProfileFileName ? ProfileFileName : TEXT("win.ini")
            );

    *SectionValue = TEXT('\0');
    GetPrivateProfileSection( SectionName,
                              SectionValue, 4096 * sizeof( TCHAR ),
                              ProfileFileName
                            );
#ifdef UNICODE
    printf( "[%ws]\n",
#else
    printf( "[%s]\n",
#endif
            SectionName
          );
    s = SectionValue;
    while (*s) {
#ifdef UNICODE
        printf( "    %ws\n", s );
#else
        printf( "    %s\n", s );
#endif

        while (*s++) {
            }
        }

    LocalFree( SectionValue );
    return;
}

#define MAX_SECTIONS 32
#define MAX_KEYWORDS 32

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    ULONG n;
    int SectionNumber, KeyNumber;
    LPTSTR SectionValue, KeyValue, Keyword;
    TCHAR SectionName[ 32 ], KeyName[ 32 ], KeyValueBuffer[ 32 ], Buffer[ 32 ];
    FILE *fh;

    printf( "TPROF: Entering Test Program\n" );

    SectionValue = LocalAlloc( 0, 4096 * sizeof( TCHAR ) );
    KeyValue = LocalAlloc( 0, 1024 * sizeof( TCHAR ) );

#if 0
    for (SectionNumber=0; SectionNumber<MAX_SECTIONS; SectionNumber++) {
        sprintf( SectionName, "Section%02u", SectionNumber );
        for (KeyNumber=0; KeyNumber<MAX_KEYWORDS; KeyNumber++) {
            sprintf( KeyName, "KeyName%02u", KeyNumber );
            sprintf( KeyValueBuffer, "KeyValue%02u_%02u", SectionNumber, KeyNumber );
            if (!WritePrivateProfileString( SectionName, KeyName, KeyValueBuffer, "foo.ini"  )) {
                fprintf( stderr, "WriteProfileString( test.ini, [%s].%s=%s )  errno == %u\n",
                         SectionName, KeyName, KeyValueBuffer, GetLastError()
                       );
                }

            GetPrivateProfileString( SectionName, KeyName, "bogus", Buffer, sizeof( Buffer ), "foo.ini" );

            if (strcmp( Buffer, KeyValueBuffer )) {
                fprintf( stderr, "Write: %s != %s, errno == %u\n", KeyValueBuffer, Buffer, GetLastError() );
                }
            }
        }

    for (SectionNumber=0; SectionNumber<MAX_SECTIONS; SectionNumber++) {
        sprintf( SectionName, "Section%02u", SectionNumber );
        for (KeyNumber=0; KeyNumber<MAX_KEYWORDS; KeyNumber++) {
            sprintf( KeyName, "KeyName%02u", KeyNumber );
            sprintf( KeyValueBuffer, "KeyValue%02u_%02u", SectionNumber, KeyNumber );

            GetPrivateProfileString( SectionName, KeyName, "bogus", Buffer, sizeof( Buffer ), "foo.ini" );

            if (strcmp( Buffer, KeyValueBuffer )) {
                fprintf( stderr, "Read: %s != %s, errno == %u\n", KeyValueBuffer, Buffer, GetLastError() );
                }
            }
        }

    for (SectionNumber=0; SectionNumber<MAX_SECTIONS; SectionNumber++) {
        sprintf( SectionName, "Section%02u", SectionNumber );
        for (KeyNumber=0; KeyNumber<MAX_KEYWORDS; KeyNumber++) {
            sprintf( KeyName, "KeyName%02u", KeyNumber );
            sprintf( KeyValueBuffer, "KeyValue%02u_%02u", SectionNumber, KeyNumber );

            if (!WritePrivateProfileString( SectionName, KeyName, NULL, "foo.ini"  )) {
                fprintf( stderr, "WriteProfileString( test.ini, [%s].%s (delete) )  errno == %u\n",
                         SectionName, KeyName, GetLastError()
                       );
                }

            GetPrivateProfileString( SectionName, KeyName, "bogus", Buffer, sizeof( Buffer ), "foo.ini" );

            if (strcmp( Buffer, "bogus" )) {
                fprintf( stderr, "Delete: bogus != %s, errno == %u\n", Buffer, GetLastError() );
                }
            }
        }

    exit( 0 );
#endif

    WriteProfileString( TEXT("TESTINI"), TEXT("Key1"), TEXT("100abc") );
    n = GetProfileString( TEXT("ports"), NULL, TEXT(""), SectionValue, 4096 * sizeof( TCHAR ) );
    Keyword = SectionValue;
    printf( "Keywords in win.ini[ports]\n" );
    while (*Keyword) {
#ifdef UNICODE
        printf( "    %ws\n", Keyword );
#else
        printf( "    %s\n", Keyword );
#endif
        while (*Keyword++) {
            }
        }

    n = GetProfileString( TEXT("ports"), NULL, NULL, SectionValue, 4096 );
    Keyword = SectionValue;
    printf( "Keywords in win.ini[ports]\n" );
    while (*Keyword) {
#ifdef UNICODE
        printf( "    %ws\n", Keyword );
#else
        printf( "    %s\n", Keyword );
#endif
        while (*Keyword++) {
            }
        }

    DeleteFile( TEXT("\\nt\\windows\\test.ini") );
    fh = fopen( "\\nt\\windows\\test.ini", "w" );
    fclose( fh );

    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("StrApp"), TEXT("StrKey"), TEXT("StrVal"), TEXT("test.ini"));
    DumpProfile( TEXT("test.ini") );


    DeleteFile( TEXT("test.ini") );
    fh = fopen( "\\nt\\test.ini", "w" );
    fprintf( fh, "[IncompleteSectionWithoutTrailingBracket\n\n" );
    fprintf( fh, "[StrApp]\n" );
    fprintf( fh, "StrKey=xxxxxx\n" );
    fclose( fh );

    DumpProfile( TEXT("test.ini") );

    if (!WritePrivateProfileString( TEXT("StrApp"), TEXT("StrKey"), TEXT("StrVal"), TEXT("test.ini"))) {
        printf( "*** Write failed - rc == %d\n", GetLastError() );
        }
    else {
        DumpProfile( TEXT("test.ini") );
        }

    DeleteFile( "test.ini" );

    fh = fopen( "\\nt\\windows\\test.ini", "w" );
    fprintf( fh, "[a]\n" );
    fprintf( fh, "a1=b1\n" );
    fprintf( fh, "[b]\n" );
    fprintf( fh, "a2=b2\n" );
    fclose( fh );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileSection( TEXT("Section2"),
        TEXT("Keyword21=Value21\0Keyword22=Value22\0Keyword23=Value23\0Keyword24=\0"),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    n = GetPrivateProfileString( TEXT("a"), TEXT("\0"), TEXT("Default"),
                                 KeyValue, 1024 * sizeof( TCHAR ),
                                 TEXT("test.ini")
                               );
#ifdef UNICODE
    printf( "GetPrivateProfileString( a, \\0, Default ) == %ld '%ws'\n", n, KeyValue );
#else
    printf( "GetPrivateProfileString( a, \\0, Default ) == %ld '%s'\n", n, KeyValue );
#endif

    n = GetPrivateProfileInt( TEXT("a"), TEXT("\0"), 123, TEXT("test.ini") );
    printf( "GetPrivateProfileString( a, \\0, 123 ) == %ld\n", n );

    WritePrivateProfileString( TEXT("a"), NULL, NULL, TEXT("test.ini") );

    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("TESTINI"), TEXT("Key1"), TEXT("100abc"), TEXT("test.ini") );

    WritePrivateProfileString( TEXT("    TESTINI    "),
        TEXT("    Key1    "),
        TEXT("  Val1   "),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    printf( "GetProfileInt( 123 ) == %ld\n",
            GetProfileInt( TEXT("AAAAA"), TEXT("XXXXX"), 123 )
          );

    printf( "GetProfileInt( -123 ) == %ld\n",
            GetProfileInt( TEXT("AAAAA"), TEXT("XXXXX"), -123 )
          );

    WritePrivateProfileString( TEXT("TESTINI"),
        TEXT("Key1"),
        NULL,
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("TESTINI"),
        TEXT("Key2"),
        TEXT("Val2"),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("TESTINI"),
        TEXT("Key2"),
        TEXT(""),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("TESTINI"),
        TEXT("Key2"),
        NULL,
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("TESTINI"),
        TEXT("Key3"),
        TEXT("Val3"),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("TESTINI"),
        NULL,
        TEXT("Something"),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("Section1"),
        TEXT("Keyword11"),
        TEXT("Value11"),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileSection( TEXT("Section2"),
        TEXT("Keyword21=Value21\0Keyword22=Value22\0Keyword23=Value23\0Keyword24=\0"),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    WritePrivateProfileString( TEXT("Section1"),
        TEXT("Keyword12"),
        TEXT("Value12"),
        TEXT("test.ini")
        );
    DumpProfile( TEXT("test.ini") );

    n = GetPrivateProfileSection( TEXT("Section1"),
                                  SectionValue, 4096 * sizeof( TCHAR ),
                                  TEXT("test.ini")
                                );
#if 0
    if (n != 36 ||
        strcmp( SectionValue, "Keyword11=Value11" ) ||
        strcmp( SectionValue+18, "Keyword12=Value12" )
       ) {
        printf( "*** test.ini[ Section1 ] is incorrect (Length == %d)\n", n );
        DumpSection( "test.ini", "Section1"  );
        }

    n = GetPrivateProfileString( "Section2", "Keyword25", "Default25",
                                 KeyValue, 1024,
                                 "test.ini"
                               );
    if (n != 9 || strcmp( KeyValue, "Default25" )) {
        printf( "*** test.ini[ Section2 ].Keyword25 is incorrect (Length == %d)\n", n );
        DumpSection( "test.ini", "Section2"  );
        }

    n = GetPrivateProfileString( "Section2", "Keyword24", NULL,
                                 KeyValue, 1024,
                                 "test.ini"
                               );
    if (n || strcmp( KeyValue, "" )) {
        printf( "*** test.ini[ Section2 ].Keyword24 is incorrect (Length == %d)\n", n );
        DumpSection( "test.ini", "Section2"  );
        }

    n = GetPrivateProfileString( "Section2", "Keyword23", NULL,
                                 KeyValue, 1024,
                                 "test.ini"
                               );
    if (n != 7 || strcmp( KeyValue, "Value23" )) {
        printf( "*** test.ini[ Section2 ].Keyword23 is incorrect (Length == %d)\n", n );
        DumpSection( "test.ini", "Section2"  );
        }

    n = GetPrivateProfileString( "Section2", "Keyword22", NULL,
                                 KeyValue, 1024,
                                 "test.ini"
                               );
    if (n != 7 || strcmp( KeyValue, "Value22" )) {
        printf( "*** test.ini[ Section2 ].Keyword22 is incorrect (Length == %d)\n", n );
        DumpSection( "test.ini", "Section2"  );
        }

    n = GetPrivateProfileString( "Section2", "Keyword21", NULL,
                                 KeyValue, 1024,
                                 "test.ini"
                               );
    if (n != 7 || strcmp( KeyValue, "Value21" )) {
        printf( "*** test.ini[ Section2 ].Keyword21 is incorrect (Length == %d)\n", n );
        DumpSection( "test.ini", "Section2"  );
        }

    DumpProfile( "test.ini" );

    printf( "Deleting [Section1]Keyword11\n" );
    WritePrivateProfileString( "Section1",
        "Keyword11",
        NULL,
        "test.ini"
        );
    DumpProfile( "test.ini" );

    printf( "Deleting all keywords in [Section1]\n" );
    WritePrivateProfileSection( "Section1",
        "",
        "test.ini"
        );
    DumpProfile( "test.ini" );

    printf( "Deleting [Section1]\n" );
    WritePrivateProfileString( "Section1",
        NULL,
        NULL,
        "test.ini"
        );
    DumpProfile( "test.ini" );

    printf( "Setting [Section2]Keyword21=\n" );
    WritePrivateProfileString( "Section2",
        "Keyword21",
        "",
        "test.ini"
        );
#endif
    DumpProfile( TEXT("test.ini") );
    DumpProfile( NULL );
    DumpSection( NULL, TEXT("Extensions")  );

    printf( "TPROF: Exiting Test Program\n" );

    return 0;
}

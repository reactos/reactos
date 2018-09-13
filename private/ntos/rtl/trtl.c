/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    trtl.c

Abstract:

    Test program for the NT OS Runtime Library (RTL)

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include <os2.h>
#include <stdio.h>
#include <process.h>
#include "nt.h"
#include "ntrtl.h"

char *TestMemoryStrings[] = {
    "",
    "1",
    "12",
    "123",
    "1234",
    "12345",
    "123456",
    "1234567",
    "12345678",
    "123456789",
    "123456789A",
    NULL
};


BOOLEAN
StringCompare(
    IN PSTRING String1,
    IN PSTRING String2,
    IN BOOLEAN CaseInSensitive,
    IN LONG ExpectedResult
    )
{
    LONG Result = RtlCompareString( String1, String2, CaseInSensitive );

    if (Result < 0) {
        Result = -1L;
        }
    else {
        if (Result > 0) {
            Result = 1L;
            }
        }

    if (Result != ExpectedResult) {
        DbgPrint( "RtlCompareString( \"%.*s\", \"%.*s\", %d ) == %ld (%ld)\n",
                String1->Length, String1->Buffer,
                String2->Length, String2->Buffer,
                CaseInSensitive,
                Result, ExpectedResult
                );
        return( FALSE );
        }
    else {
        return( TRUE );
        }
}

BOOLEAN
StringEqual(
    IN PSTRING String1,
    IN PSTRING String2,
    IN BOOLEAN CaseInSensitive,
    IN BOOLEAN ExpectedResult
    )
{
    BOOLEAN Result = RtlEqualString( String1, String2, CaseInSensitive );

    if (Result != ExpectedResult) {
        DbgPrint( "RtlEqualString( \"%.*s\", \"%.*s\", %d ) == %d (%d)\n",
                String1->Length, String1->Buffer,
                String2->Length, String2->Buffer,
                CaseInSensitive,
                Result, ExpectedResult );
        return( FALSE );
        }
    else {
        return( TRUE );
        }
}

VOID
DumpString(
    IN PCH StringTitle,
    IN PSTRING String
    )
{
    DbgPrint( "%s: (%d, %d) \"%.*s\"\n", StringTitle,
                                       String->MaximumLength,
                                       String->Length,
                                       String->Length,
                                       String->Buffer );
}


BOOLEAN
TestString( void )
{
    BOOLEAN Result;
    char buffer5[ 80 ], buffer6[ 15 ], buffer7[ 3 ];
    STRING String1, String2, String3, String4;
    STRING String5, String6, String7, String8;
                            //         1         2
                            //12345678901234567890
                            //
    RtlInitString( &String1, " One" );
    RtlInitString( &String2, " Two" );
    RtlInitString( &String3, " Three" );
    RtlInitString( &String4, " Four" );
    String5.Buffer = buffer5;
    String5.MaximumLength = sizeof( buffer5 );
    String5.Length = 0;
    String6.Buffer = buffer6;
    String6.MaximumLength = sizeof( buffer6 );
    String6.Length = 0;
    String7.Buffer = buffer7;
    String7.MaximumLength = sizeof( buffer7 );
    String7.Length = 0;
    String8.Buffer = NULL;
    String8.MaximumLength = 0;
    String8.Length = 0;
    RtlCopyString( &String5, &String1 );
    RtlCopyString( &String6, &String2 );
    RtlCopyString( &String7, &String3 );
    RtlCopyString( &String8, &String4 );

    DumpString( "String1", &String1 );
    DumpString( "String2", &String2 );
    DumpString( "String3", &String3 );
    DumpString( "String4", &String4 );
    DumpString( "String5", &String5 );
    DumpString( "String6", &String6 );
    DumpString( "String7", &String7 );
    DumpString( "String8", &String8 );

    Result = TRUE;
    Result &= StringCompare( &String1, &String1, FALSE, 0L );
    Result &= StringCompare( &String1, &String2, FALSE, -1L);
    Result &= StringCompare( &String1, &String3, FALSE, -1L);
    Result &= StringCompare( &String1, &String4, FALSE, 1L );
    Result &= StringCompare( &String1, &String5, FALSE, 0L );
    Result &= StringCompare( &String1, &String6, FALSE, -1L);
    Result &= StringCompare( &String1, &String7, FALSE, -1L);
    Result &= StringCompare( &String1, &String8, FALSE, 1L );

    Result &= StringEqual( &String1, &String1, FALSE, 1 );
    Result &= StringEqual( &String1, &String2, FALSE, 0 );
    Result &= StringEqual( &String1, &String3, FALSE, 0 );
    Result &= StringEqual( &String1, &String4, FALSE, 0 );
    Result &= StringEqual( &String1, &String5, FALSE, 1 );
    Result &= StringEqual( &String1, &String6, FALSE, 0 );
    Result &= StringEqual( &String1, &String7, FALSE, 0 );
    Result &= StringEqual( &String1, &String8, FALSE, 0 );

    return( Result );
}


int
_CDECL
main(
    int argc,
    char *argv[]
    )
{
    if (!TestString()) {
        DbgPrint( "TRTL: TestString failed\n" );
        exit( 1 );
        }

    exit( 0 );
    return( 0 );
}

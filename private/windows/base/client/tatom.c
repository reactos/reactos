/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tatom.c

Abstract:

    Win32 Base API Test Program for Atom Manager calls

Author:

    Steve Wood (stevewo) 26-Oct-1990

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <windows.h>

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    ATOM Atom1, Atom2, Atom3;
    BOOL Atom1Found, Atom2Found, Atom3Found;
    DWORD Atom1Length, Atom2Length, Atom3Length;
    TCHAR NameBuffer[ 128 ];

    printf( "TATOM: Entering Test Program\n" );

    Atom1 = AddAtom( TEXT("Atom1") );
    Atom2 = AddAtom( TEXT("#4095") );
    Atom3 = AddAtom( (LPTSTR)0x1234 );

    printf( "AddAtom( ""Atom1"" ) == %X\n", Atom1 );
    printf( "AddAtom( ""#4095"" ) == %X\n", Atom2 );
    printf( "AddAtom( 0x1234 ) == %X\n", Atom3 );

    Atom1 = AddAtom( TEXT("Atom1") );
    Atom2 = AddAtom( TEXT("#4095") );
    Atom3 = AddAtom( (LPTSTR)0x1234 );

    printf( "AddAtom( ""Atom1"" ) == %X\n", Atom1 );
    printf( "AddAtom( ""#4095"" ) == %X\n", Atom2 );
    printf( "AddAtom( 0x1234 ) == %X\n", Atom3 );

    assert( Atom1Found = (Atom1 == FindAtom( TEXT("Atom1") )) );
    assert( Atom2Found = (Atom2 == FindAtom( TEXT("#4095") )) );
    assert( Atom3Found = (Atom3 == FindAtom( (LPTSTR)0x1234 )) );

    printf( "FindAtom( ""Atom1"" ) == %X\n", Atom1 );
    printf( "FindAtom( ""#4095"" ) == %X\n", Atom2 );
    printf( "FindAtom( 0x1234 ) == %X\n", Atom3 );

    Atom1Length = GetAtomName( Atom1, NameBuffer, sizeof( NameBuffer ));
#ifdef UNICODE
    printf( "GetAtomName( %X ) == %ws\n", Atom1, NameBuffer );
#else
    printf( "GetAtomName( %X ) == %s\n", Atom1, NameBuffer );
#endif
    Atom2Length = GetAtomName( Atom2, NameBuffer, sizeof( NameBuffer ));
#ifdef UNICODE
    printf( "GetAtomName( %X ) == %ws\n", Atom2, NameBuffer );
#else
    printf( "GetAtomName( %X ) == %s\n", Atom2, NameBuffer );
#endif
    Atom3Length = GetAtomName( Atom3, NameBuffer, sizeof( NameBuffer ));
#ifdef UNICODE
    printf( "GetAtomName( %X ) == %ws\n", Atom3, NameBuffer );
#else
    printf( "GetAtomName( %X ) == %s\n", Atom3, NameBuffer );
#endif

    printf( "DeleteAtom( %X ) == %X\n", Atom1, DeleteAtom( Atom1 ) );
    printf( "DeleteAtom( %X ) == %X\n", Atom1, DeleteAtom( Atom1 ) );
    printf( "DeleteAtom( %X ) == %X\n", Atom1, DeleteAtom( Atom1 ) );
    printf( "DeleteAtom( %X ) == %X\n", Atom1, DeleteAtom( Atom1 ) );
    printf( "DeleteAtom( %X ) == %X\n", Atom2, DeleteAtom( Atom2 ) );
    printf( "DeleteAtom( %X ) == %X\n", Atom3, DeleteAtom( Atom3 ) );

    Atom1 = GlobalAddAtom( TEXT("Atom1") );
    Atom2 = GlobalAddAtom( TEXT("#4095") );
    Atom3 = GlobalAddAtom( (LPTSTR)0x1234 );

    printf( "GlobalAddAtom( ""Atom1"" ) == %X\n", Atom1 );
    printf( "GlobalAddAtom( ""#4095"" ) == %X\n", Atom2 );
    printf( "GlobalAddAtom( 0x1234 ) == %X\n", Atom3 );

    Atom1 = GlobalAddAtom( TEXT("Atom1") );
    Atom2 = GlobalAddAtom( TEXT("#4095") );
    Atom3 = GlobalAddAtom( (LPTSTR)0x1234 );

    printf( "GlobalAddAtom( ""Atom1"" ) == %X\n", Atom1 );
    printf( "GlobalAddAtom( ""#4095"" ) == %X\n", Atom2 );
    printf( "GlobalAddAtom( 0x1234 ) == %X\n", Atom3 );

    assert( Atom1Found = (Atom1 == GlobalFindAtom( TEXT("Atom1") )) );
    assert( Atom2Found = (Atom2 == GlobalFindAtom( TEXT("#4095") )) );
    assert( Atom3Found = (Atom3 == GlobalFindAtom( (LPTSTR)0x1234 )) );

    printf( "GlobalFindAtom( ""Atom1"" ) == %X\n", Atom1 );
    printf( "GlobalFindAtom( ""#4095"" ) == %X\n", Atom2 );
    printf( "GlobalFindAtom( 0x1234 ) == %X\n", Atom3 );

    Atom1Length = GlobalGetAtomName( Atom1, NameBuffer, sizeof( NameBuffer ));
#ifdef UNICODE
    printf( "GlobalGetAtomName( %X ) == %ws\n", Atom1, NameBuffer );
#else
    printf( "GlobalGetAtomName( %X ) == %s\n", Atom1, NameBuffer );
#endif
    Atom2Length = GlobalGetAtomName( Atom2, NameBuffer, sizeof( NameBuffer ));
#ifdef UNICODE
    printf( "GlobalGetAtomName( %X ) == %ws\n", Atom2, NameBuffer );
#else
    printf( "GlobalGetAtomName( %X ) == %s\n", Atom2, NameBuffer );
#endif
    Atom3Length = GlobalGetAtomName( Atom3, NameBuffer, sizeof( NameBuffer ));
#ifdef UNICODE
    printf( "GlobalGetAtomName( %X ) == %ws\n", Atom3, NameBuffer );
#else
    printf( "GlobalGetAtomName( %X ) == %s\n", Atom3, NameBuffer );
#endif

    printf( "GlobalDeleteAtom( %X ) == %X\n", Atom1, GlobalDeleteAtom( Atom1 ) );
    printf( "GlobalDeleteAtom( %X ) == %X\n", Atom1, GlobalDeleteAtom( Atom1 ) );
    printf( "GlobalDeleteAtom( %X ) == %X\n", Atom1, GlobalDeleteAtom( Atom1 ) );
    printf( "GlobalDeleteAtom( %X ) == %X\n", Atom1, GlobalDeleteAtom( Atom1 ) );
    printf( "GlobalDeleteAtom( %X ) == %X\n", Atom2, GlobalDeleteAtom( Atom2 ) );
    printf( "GlobalDeleteAtom( %X ) == %X\n", Atom3, GlobalDeleteAtom( Atom3 ) );

    printf( "TATOM: Exiting Test Program\n" );

    return 0;
}

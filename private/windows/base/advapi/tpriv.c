/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tuser.c

Abstract:

    This module tests windows LookupPrivilegeNameX for a specific
    condition that required a Daytona hotfix.

Author:

    Jim Kelly (JimK) July-27-1994


Revision History:

--*/

#include <windows.h>
#include <stdio.h>



VOID
DoLookup( DWORD L );

CHAR BufferA[0x11000];
CHAR BufferA1[0x11000];
WCHAR BufferW[11000];

int
main (void)
{

    //
    // Lookup privilege names with certain special buffer lengths.


    DoLookup( 0xFFFF );
    DoLookup( 0xFFFC );
    DoLookup( 0x45 );
    DoLookup( 0x10000 );
    DoLookup( 0x10010 );
    DoLookup( 0x2 );

    return(0);
}



VOID
DoLookup( DWORD L )
{

    DWORD
        Language,
        Length;

    LUID
        PrivilegeId;

    Language = 0;
    

    Length = L;
    PrivilegeId.HighPart = 0;
    PrivilegeId.LowPart  = 7;   //SE_TCB_PRIVILEGE;

    printf("length %d\n", Length);
    printf("  LookupPrivilegeNameA:");
    if (LookupPrivilegeNameA( "", &PrivilegeId, &BufferA[0], &Length)) {
        printf("success  (%s)\n", &BufferA[0] );
    } else {
        printf("failed ****\n");
    }

    printf("  LookupPrivilegeDisplayNameA:");
    if (LookupPrivilegeDisplayNameA( "", &BufferA[0], &BufferA1[0], &Length, &Language)) {
        printf("success  (%s)\n", &BufferA1[0] );
    } else {
        printf("failed ****\n");
    }

    return;
}

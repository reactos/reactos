/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    undname.c

Abstract:

    This is the main source file for the UNDNAME utility program.
    This is a simple command line utility for undecorating C++ symbol
    names.

Author:

    Weslwy Witt (wesw) 09-June-1993

Revision History:

--*/

#include <private.h>
#include <ntverp.h>
#include <common.ver>


void
Usage( void )
{
    fprintf( stderr,
             "usage: UNDNAME [-f] decorated-names...\n"
             "       -f Undecorate fully.  Default is to only undecorate the class::member\n");
    exit( 1 );
}

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char UnDecoratedName[4000];
    DWORD Flags;

    fputs(VER_PRODUCTNAME_STR "\nUNDNAME Version " VER_PRODUCTVERSION_STR, stderr );
    fputs(VER_LEGALCOPYRIGHT_STR "\n\n", stderr);

    if (argc <= 1) {
        Usage();
    }

    if ((argv[1][0] == '-') && (argv[1][1] == 'f')) {
        Flags = UNDNAME_COMPLETE;
        argc--;
        argv++;
    } else {
        Flags = UNDNAME_NAME_ONLY;
    }

    if (argc <= 1) {
        Usage();
    }

    while (--argc) {
        UnDecorateSymbolName( *++argv, UnDecoratedName, sizeof(UnDecoratedName), Flags );
        printf( ">> %s == %s\n", *argv, UnDecoratedName );
    }

    exit( 0 );
    return 0;
}

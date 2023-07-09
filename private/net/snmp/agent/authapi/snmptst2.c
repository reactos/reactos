/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmptst2.c

Abstract:

    Component test engine.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

#include <stdio.h>

int bertest( void );
int pdutest( void );
int authtest( void );


int __cdecl main ()

{
   bertest();

   pdutest();

   authtest();

   return 0;
}

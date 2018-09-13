/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msvars.c

Abstract:

   This module contains variables used within the msv1_0 authentication
   package.

Author:

    Jim Kelly (JimK) 11-Apr-1991

Environment:

    User mode - msv1_0 authentication package DLL

Revision History:
    Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\msvars.c

--*/

#include "msp.h"



////////////////////////////////////////////////////////////////////////
//                                                                    //
//                   READ ONLY  Variables                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////



//
// msv1_0 private heap.
//

PVOID MspHeap;


//
// package ID assigned to msv1_0 by the LSA.
//

ULONG MspAuthenticationPackageId;


//
// dispatch table of (public) LSA service routines.
//

LSA_DISPATCH_TABLE Lsa;


//
// dispatch table of (Private) LSA service routines.
//

LSAP_PRIVATE_LSA_SERVICES Lsap;



////////////////////////////////////////////////////////////////////////
//                                                                    //
//                   READ/WRITE Variables                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////





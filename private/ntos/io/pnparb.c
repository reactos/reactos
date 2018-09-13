/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pnparb.c

Abstract:

    This module contains support routines for the Pnp resource arbiters.

Author:

    Andrew Thornton (andrewth) 1-April-1997
    
    
Environment:

    Kernel mode

--*/

#include "iop.h"
#pragma hdrstop

//
// Debugging support
//

//
// Always enable the debug stuff at the moment
// BUGBUG(andrewth) - this should go away before we ship
//
#undef DBG
#define DBG 1
#if DBG

//
// Debug print level:
//    -1 = no messages
//     0 = vital messages only
//     1 = call trace
//     2 = verbose messages
//

LONG IoArbiterDebugLevel = -1;

#define DEBUG_PRINT(Level, Message) \
    if (Level <= IoArbiterDebugLevel) DbgPrint Message

#else
    
#define DEBUG_PRINT(Level, Message) 

#endif // DBG


#define ARBITER_CONTEXT_TO_INSTANCE(x)      (x)

//
// Include code from pnp
// This is a cpp style symbolic link
//


#include "..\pnp\arbiter.c"


/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ketypes.h (ARCH)

Abstract:

    Portability file to choose the correct Architecture-specific file.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _ARCH_KETYPES_H
#define _ARCH_KETYPES_H

//
// FIXME: Find a solution to take these out of here!
//
typedef struct _KDPC_DATA
{
    LIST_ENTRY DpcListHead;
    ULONG DpcLock;
    ULONG DpcQueueDepth;
    ULONG DpcCount;
} KDPC_DATA, *PKDPC_DATA;

typedef struct _PP_LOOKASIDE_LIST
{
    struct _GENERAL_LOOKASIDE *P;
    struct _GENERAL_LOOKASIDE *L;
} PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;

//
// Include the right file for this architecture.
//
#ifdef _M_IX86
#include <i386/ketypes.h>
#else
#error "Unknown processor"
#endif

#endif

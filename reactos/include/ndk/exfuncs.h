/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    exfuncs.h

Abstract:

    Function definitions for the Executive.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _EXFUNCS_H
#define _EXFUNCS_H

//
// Dependencies
//

//
// Fast Mutex functions
//
VOID
FASTCALL
ExEnterCriticalRegionAndAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex);

VOID
FASTCALL
ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(PFAST_MUTEX FastMutex);

#endif

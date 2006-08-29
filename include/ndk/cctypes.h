/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    cctypes.h

Abstract:

    Type definitions for the Cache Controller.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _CCTYPES_H
#define _CCTYPES_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef NTOS_MODE_USER

//
// Kernel Exported CcData
//
extern ULONG NTSYSAPI CcFastReadNotPossible;
extern ULONG NTSYSAPI CcFastReadWait;

#endif
#endif // _CCTYPES_H


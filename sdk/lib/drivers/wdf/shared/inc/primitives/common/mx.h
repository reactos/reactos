/*++

Copyright (c) Microsoft Corporation

ModuleName:

    Mx.h

Abstract:

    This file includes MxUm.h/MxKm.h based on the mode

    Shared code can use this header to have appropriate
    mode headers pulled in

Author:



Revision History:



--*/

#pragma once

#include "mxmacros.h"

//
// Greater than 64 logical processors support: direct DDK to include the new
// routines to handle groups and exclude any potentially dangerous, old
// interfaces.
//
#define NT_PROCESSOR_GROUPS 1

#define WDF_VIOLATION                    ((ULONG)0x0000010DL)

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "mxum.h"
#elif (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#include "mxkm.h"
#endif


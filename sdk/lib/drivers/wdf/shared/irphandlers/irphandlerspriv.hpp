/*++

Copyright (c) Microsoft Corporation

Module Name:

    irphandlerspriv.hpp

Abstract:

    Private header for irphandlers.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

extern "C" {
#include "mx.h"
}

#include "fxmin.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "wdmdefs.h"
#include "fxirpum.hpp"
#else
#include "fxirpkm.hpp"
#endif

#include "fxirp.hpp"

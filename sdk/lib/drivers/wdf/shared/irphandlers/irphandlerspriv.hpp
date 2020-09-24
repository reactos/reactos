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

#include "FxMin.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "wdmdefs.h"
#include "FxIrpUm.hpp"
#else
#include "FxIrpKm.hpp"
#endif

#include "FxIrp.hpp"

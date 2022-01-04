/*++

Copyright (c) Microsoft Corporation

Module Name:

    fxtypedefs.hpp

Abstract:

    Contains defines for types that are different in KMDF and UMDF
    Respective defines are in km\fxtypedefsKm.hpp and um\fxtypedefsUm.hpp

    For example CfxDevice is defined as
        FxDevice in km
        AWDFDevice in um

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#pragma once

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)

#include "fxtypedefskm.hpp"

#else

#include "fxtypedefsum.hpp"

#endif


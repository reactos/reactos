/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

   fxsupportpch.h

Abstract:

    This module contains header definitions and include files needed by all
    modules in fx\support

--*/

#ifndef __FX_SUPPORT_PCH_HPP__
#define __FX_SUPPORT_PCH_HPP__

#if FX_CORE_MODE == FX_CORE_USER_MODE
#include "um/fxsupportpchum.hpp"
#elif FX_CORE_MODE == FX_CORE_KERNEL_MODE
#include "km/fxsupportpchkm.hpp"
#endif

#endif // __FX_SUPPORT_PCH_HPP__

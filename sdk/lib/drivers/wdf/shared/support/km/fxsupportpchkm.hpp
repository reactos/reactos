/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

   fxsupportpchkm.h

Abstract:

    This module contains header definitions and include files needed by all
    modules in fx\support

Author:

Environment:
   Kernel mode only

Revision History:

--*/

#ifndef __FX_SUPPORT_PCH_KM_HPP__
#define __FX_SUPPORT_PCH_KM_HPP__

extern "C" {
#include <mx.h>
}

#include <fxmin.hpp>

#include "fxcollection.hpp"
#include "stringutil.hpp"
#include "fxstring.hpp"
#include "fxdevicetext.hpp"
#include "fxwaitlock.hpp"

#include <wdfresource.h>
#include <fxresource.hpp>

#endif // __FX_SUPPORT_PCH_KM_HPP__

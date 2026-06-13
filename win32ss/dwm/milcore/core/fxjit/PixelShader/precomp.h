// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------
//

//
//  Precompiled header file 
//
//------------------------------------------------------------------------

#include <wpfsdl.h>
#include <sal.h>
#include <salextra.h>

//----------------------------------------------------------------------------
//
// This is done so that other users of this library can include windows.h here
// and have no dependencies on the Jolt headers
//

#include "windowsshim.h"

// End of Jolt specific headers 
//----------------------------------------------------------------------------

//
// Useful macro headers
//

#include "macros.h"

//
// Basic d3d and refrast headers
//

#include "d3d9types.h"
#include "d3d9caps.h"
#include "warpplatform.h"
#include "d3dhal.h"
#include "pstrans.hpp"
#include "rdpstrans.h"

//
// Disable warnings needed for headers to compile
//

#pragma warning(disable:4291) //no matching operator delete found

//
// JIT  headers
//

#include "JitterAccess.h"
#include "JitterSupport.h"
#include "SIMDJit.h"

#include "FlushMemory.h"
#include "Register.h"
#include "Operator.h"
#include "Locator.h"
#include "Program.h"

//
// Pixel shader headers
//

#include "effectparams.h"
#include "shaderreg.h"
#include "pshader.h"


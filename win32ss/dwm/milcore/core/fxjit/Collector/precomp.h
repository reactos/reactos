// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------
//

//
//  Precompiled header file for SIMDJit\Collector.lib
//
//------------------------------------------------------------------------

#include <wpfsdl.h>
#include <sal.h>
#include <salextra.h>

#include "types.h"
#include "warpplatform.h"

#include "SIMDJit.h"

#include "FlushMemory.h"
#include "Register.h"
#include "Operator.h"
#include "Locator.h"
#include "Program.h"
#include "Coder86.h"
#include "Assemble.h"
#include "ShuffleRegs.h"
#include "BitArray.h"
#include "Mapper.h"


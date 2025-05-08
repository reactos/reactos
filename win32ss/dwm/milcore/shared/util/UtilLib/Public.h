// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------
#pragma once

// Needed by DynCast.h for DBG - include always for consistency
#include <strsafe.h>
#include <wgx_error.h>

#include "DebugBreak.h"
#include "Instrumentation.h"
#include "InstrumentationConfig.h"
#include "InstrumentationApi.h"
#include "MemUtils.h"
#include "Locks.h"
#include "StrUtil.h"
#include "UtilMisc.h"
#include "DynCast.h"    // Needs UtilMisc.h and StrSafe.h for DBG
#include "List.h"
#include "HiResTimer.h"
#include "Registry.h"
#include "WPFEventTrace.h"




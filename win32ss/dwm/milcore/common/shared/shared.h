// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.



#include <intsafe.h>
#pragma warning (push)
#pragma warning (disable: 4201) // nonstandard extension used : nameless struct/union
#include <mmsystem.h>
// winnt.h and the VC8 emmintrin.h define this function differently, and compilation fails.
#define _mm_clflush _mm_clflush__dupe_renamed
#if !defined(_ARM_) && !defined(_ARM64_)
#include <emmintrin.h>
#endif
#pragma warning (pop)
#include "milcom.h"
#include "basetypes.h"
#include "utc.h"
#include "utils.h"
#include "processorfeatures.h"
#include "real.h"
#include "pixelformatutils.h"
#include "mem.h"
#include "engine.h"
#include "List.h"
#include "MILInterlocked.h"
#include "assertentry.h"
#include "refcountbase.h"
#include "arithmetic.h"
#include "dynarrayimpl.h"
#include "dynarray.h"
#include "heap.h"
#include "resourcecache.h"
#include "generictablemap.h"
#include "internalGuids.h"
#include "xmm.h"
#include "rgnutils.h"
#include "etwtrace.h"

#include "MILRectF_WH.h"
#include "MILRect.h"

#include <vector>
#include <map>


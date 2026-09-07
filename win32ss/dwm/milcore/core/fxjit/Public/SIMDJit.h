// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#if !defined(WPFGFX_FXJIT_X86)
#if defined(_X86_) || defined(_ARM_)
#define WPFGFX_FXJIT_X86 1
#endif
#endif

#include "Types.h"
#include "JitterSupport.h"
#include "JitterAccess.h"
#include "Operations.h"
#include "Variable.h"
#include "C_u32.h"
#include "C_s32.h"

#if WPFGFX_FXJIT_X86
#include "MmValue.h"
#include "C_u64x1.h"
#include "C_u32x2.h"
#include "C_u16x4.h"
#include "C_u8x8.h"
#else //_AMD64_
#include "C_u64.h"
#endif WPFGFX_FXJIT_X86

#include "XmmValue.h"
#include "C_u128x1.h"
#include "C_u64x2.h"
#include "C_u32x4.h"
#include "C_u16x8.h"
#include "C_u8x16.h"
#include "C_s16x8.h"
#include "C_f32x1.h"
#include "C_f32x4.h"
#include "C_s32x4.h"
#include "C_LazyVar.h"
#include "Branch.h"
#include "PVoid.h"
#include "P_u8.h"
#include "P_u16.h"
#include "P_u32.h"

#if WPFGFX_FXJIT_X86
#include "P_u64x1.h"
#include "P_u32x2.h"
#include "P_u16x4.h"
#include "P_u8x8.h"
#endif //WPFGFX_FXJIT_X86

#include "P_u128x1.h"
#include "P_u64x2.h"
#include "P_u32x4.h"
#include "P_s32x4.h"
#include "P_u16x8.h"
#include "P_u8x16.h"
#include "P_f32x1.h"
#include "P_f32x4.h"


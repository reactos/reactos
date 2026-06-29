/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Intriniscs used by the C++ Standard Library
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#define __INTRIN0_INL_H_

#include <vcruntime.h>

#if defined(__cplusplus)
extern "C" {
#endif

__int8 __iso_volatile_load8(const volatile __int8 *);
__int16 __iso_volatile_load16(const volatile __int16 *);
__int32 __iso_volatile_load32(const volatile __int32 *);
__int64 __iso_volatile_load64(const volatile __int64 *);
void __iso_volatile_store8(volatile __int8*, __int8);
void __iso_volatile_store16(volatile __int16*, __int16);
void __iso_volatile_store32(volatile __int32*, __int32);
void __iso_volatile_store64(volatile __int64*, __int64);

#if defined(__cplusplus)
} // extern "C"
#endif

/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Intrinsics for the SSE3 instruction set
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#define _INCLUDED_PMM

#include <emmintrin.h>

#define _MM_DENORMALS_ZERO_MASK 0x0040
#define _MM_DENORMALS_ZERO_ON   0x0040
#define _MM_DENORMALS_ZERO_OFF  0x0000

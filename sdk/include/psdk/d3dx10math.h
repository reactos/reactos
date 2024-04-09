/*
 * Copyright (C) 2016 Alistair Leslie-Hughes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "d3dx10.h"

/* This guard is the same as D3DX9 to prevent double-inclusion */
#ifndef __D3DX9MATH_H__
#define __D3DX9MATH_H__

#include <math.h>

typedef enum _D3DX_CPU_OPTIMIZATION
{
    D3DX_NOT_OPTIMIZED,
    D3DX_3DNOW_OPTIMIZED,
    D3DX_SSE2_OPTIMIZED,
    D3DX_SSE_OPTIMIZED
} D3DX_CPU_OPTIMIZATION;

#ifdef __cplusplus
extern "C" {
#endif

D3DX_CPU_OPTIMIZATION WINAPI D3DXCpuOptimizations(BOOL enable);

#ifdef __cplusplus
}
#endif

#endif /* __D3DX9MATH_H__ */

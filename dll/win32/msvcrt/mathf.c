/*
 * msvcrt float functions
 *
 * Copyright 2019 Jacek Caban for CodeWeavers
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

/* this function is part of the import lib to provide floating */
#if 0
#pragma makedep implib
#endif

#include <corecrt.h>
#include <wine/asm.h>

double __cdecl sin(double);
double __cdecl cos(double);
double __cdecl tan(double);
double __cdecl atan2(double, double);
double __cdecl exp(double);
double __cdecl log(double);
double __cdecl pow(double, double);
double __cdecl sqrt(double);
double __cdecl floor(double);
double __cdecl ceil(double);
float __cdecl powf(float, float);

#if defined(__i386__) || (_MSVCR_VER > 0 && _MSVCR_VER < 80)
float sinf(float x) { return sin(x); }
float cosf(float x) { return cos(x); }
float tanf(float x) { return tan(x); }
float atan2f(float x, float y) { return atan2(x, y); }
float expf(float x) { return exp(x); }
float logf(float x) { return log(x); }
float sqrtf(float x) { return sqrt(x); }
float floorf(float x) { return floor(x); }
float ceilf(float x) { return ceil(x); }
__ASM_GLOBAL_IMPORT(sinf)
__ASM_GLOBAL_IMPORT(cosf)
__ASM_GLOBAL_IMPORT(tanf)
__ASM_GLOBAL_IMPORT(atan2f)
__ASM_GLOBAL_IMPORT(expf)
__ASM_GLOBAL_IMPORT(logf)
__ASM_GLOBAL_IMPORT(sqrtf)
__ASM_GLOBAL_IMPORT(floorf)
__ASM_GLOBAL_IMPORT(ceilf)

#if _MSVCR_VER < 140
float powf(float x, float y) { return pow(x, y); }
__ASM_GLOBAL_IMPORT(powf)
#endif
#endif

#if _MSVCR_VER < 120
double exp2(double x) { return pow(2.0, x); }
float exp2f(float x) { return powf(2.0f, x); }
__ASM_GLOBAL_IMPORT(exp2)
__ASM_GLOBAL_IMPORT(exp2f)
#endif

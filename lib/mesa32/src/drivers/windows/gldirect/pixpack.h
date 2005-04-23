/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x (Win32)
*
* Description:  Pixel packing functions.
*
****************************************************************************/

#ifndef __PIXPACK_H
#define __PIXPACK_H

#include <GL\gl.h>
#include <ddraw.h>

#include "ddlog.h"

/*---------------------- Macros and type definitions ----------------------*/

#define PXAPI

// Typedef that can be used for pixel packing function pointers.
#define PX_PACK_FUNC(a) void PXAPI (a)(unsigned char *pixdata, void *dst, GLenum Format, const LPDDSURFACEDESC2 lpDDSD2)
typedef void (PXAPI *PX_packFunc)(unsigned char *pixdata, void *dst, GLenum Format, const LPDDSURFACEDESC2 lpDDSD2);

// Typedef that can be used for pixel unpacking function pointers.
#define PX_UNPACK_FUNC(a) void PXAPI (a)(unsigned char *pixdata, void *src, GLenum Format, const LPDDSURFACEDESC2 lpDDSD2)
typedef void (PXAPI *PX_unpackFunc)(unsigned char *pixdata, void *src, GLenum Format, const LPDDSURFACEDESC2 lpDDSD2);

// Typedef that can be used for pixel span packing function pointers.
#define PX_PACK_SPAN_FUNC(a) void PXAPI (a)(GLuint n, unsigned char *pixdata, unsigned char *dst, GLenum Format, const LPDDSURFACEDESC2 lpDDSD2)
typedef void (PXAPI *PX_packSpanFunc)(GLuint n, unsigned char *pixdata, unsigned char *dst, GLenum Format, const LPDDSURFACEDESC2 lpDDSD2);

/*------------------------- Function Prototypes ---------------------------*/

#ifdef  __cplusplus
extern "C" {
#endif

// Function that examines a pixel format and returns the relevent
// pixel-packing function
void PXAPI pxClassifyPixelFormat(const LPDDPIXELFORMAT lpddpf, PX_packFunc *lpPackFn ,PX_unpackFunc *lpUnpackFn, PX_packSpanFunc *lpPackSpanFn);

// Packing functions
PX_PACK_FUNC(pxPackGeneric);
PX_PACK_FUNC(pxPackRGB555);
PX_PACK_FUNC(pxPackARGB4444);
PX_PACK_FUNC(pxPackARGB1555);
PX_PACK_FUNC(pxPackRGB565);
PX_PACK_FUNC(pxPackRGB332);
PX_PACK_FUNC(pxPackRGB888);
PX_PACK_FUNC(pxPackARGB8888);
PX_PACK_FUNC(pxPackPAL8);

// Unpacking functions
PX_UNPACK_FUNC(pxUnpackGeneric);
PX_UNPACK_FUNC(pxUnpackRGB555);
PX_UNPACK_FUNC(pxUnpackARGB4444);
PX_UNPACK_FUNC(pxUnpackARGB1555);
PX_UNPACK_FUNC(pxUnpackRGB565);
PX_UNPACK_FUNC(pxUnpackRGB332);
PX_UNPACK_FUNC(pxUnpackRGB888);
PX_UNPACK_FUNC(pxUnpackARGB8888);
PX_UNPACK_FUNC(pxUnpackPAL8);

// Span Packing functions
PX_PACK_SPAN_FUNC(pxPackSpanGeneric);
PX_PACK_SPAN_FUNC(pxPackSpanRGB555);
PX_PACK_SPAN_FUNC(pxPackSpanARGB4444);
PX_PACK_SPAN_FUNC(pxPackSpanARGB1555);
PX_PACK_SPAN_FUNC(pxPackSpanRGB565);
PX_PACK_SPAN_FUNC(pxPackSpanRGB332);
PX_PACK_SPAN_FUNC(pxPackSpanRGB888);
PX_PACK_SPAN_FUNC(pxPackSpanARGB8888);
PX_PACK_SPAN_FUNC(pxPackSpanPAL8);

#ifdef  __cplusplus
}
#endif

#endif

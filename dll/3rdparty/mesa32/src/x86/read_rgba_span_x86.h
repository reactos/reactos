/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
/**
 * \file read_rgba_span_x86.h
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#ifndef READ_RGBA_SPAN_X86_H
#define READ_RGBA_SPAN_X86_H

#if defined(USE_SSE_ASM) || defined(USE_MMX_ASM)
#include "x86/common_x86_asm.h"
#endif

#if defined(USE_SSE_ASM)
extern void _generic_read_RGBA_span_BGRA8888_REV_SSE2( const unsigned char *,
    unsigned char *, unsigned );
#endif

#if defined(USE_SSE_ASM)
extern void _generic_read_RGBA_span_BGRA8888_REV_SSE( const unsigned char *,
    unsigned char *, unsigned );
#endif

#if defined(USE_MMX_ASM)
extern void _generic_read_RGBA_span_BGRA8888_REV_MMX( const unsigned char *,
    unsigned char *, unsigned );

extern void _generic_read_RGBA_span_RGB565_MMX( const unsigned char *,
    unsigned char *, unsigned );
#endif

#endif /* READ_RGBA_SPAN_X86_H */

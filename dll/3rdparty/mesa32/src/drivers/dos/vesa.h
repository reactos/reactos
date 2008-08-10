/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef VESA_H_included
#define VESA_H_included

#include "internal.h"

extern void *vesa_swbank;

extern void vesa_b_dump_virtual (void);
extern void vesa_l_dump_virtual (void);
extern void vesa_l_dump_virtual_mmx (void);

extern void vesa_l_dump_32_to_24 (void);
extern void vesa_l_dump_32_to_16 (void);
extern void vesa_l_dump_32_to_15 (void);
extern void vesa_l_dump_32_to_8 (void);
extern void vesa_l_dump_24_to_32 (void);
extern void vesa_l_dump_24_to_8 (void);
extern void vesa_l_dump_16_to_15 (void);
extern void vesa_l_dump_16_to_8 (void);

extern void vesa_b_dump_32_to_24 (void);
extern void vesa_b_dump_32_to_16 (void);
extern void vesa_b_dump_32_to_15 (void);
extern void vesa_b_dump_32_to_8 (void);
extern void vesa_b_dump_24_to_32 (void);
extern void vesa_b_dump_24_to_8 (void);
extern void vesa_b_dump_16_to_15 (void);
extern void vesa_b_dump_16_to_8 (void);

extern vl_driver VESA;

#endif

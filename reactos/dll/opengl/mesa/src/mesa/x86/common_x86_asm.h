
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 * Check CPU capabilities & initialize optimized funtions for this particular
 * processor.
 *
 * Written by Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 * Changed by Andre Werthmann <wertmann@cs.uni-potsdam.de> for using the
 * new SSE functions
 *
 * Reimplemented by Gareth Hughes in a more
 * future-proof manner, based on code in the Linux kernel.
 */

#ifndef __COMMON_X86_ASM_H__
#define __COMMON_X86_ASM_H__

/* Do not reference mtypes.h from this file.
 */
#include "common_x86_features.h"

extern int _mesa_x86_cpu_features;

extern void _mesa_get_x86_features(void);

extern void _mesa_check_os_sse_support(void);

extern void _mesa_init_all_x86_transform_asm( void );

#endif

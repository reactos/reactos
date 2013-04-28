/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_config.h"
#include "rtasm_cpu.h"

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)

#include "util/u_debug.h"
#include "util/u_cpu_detect.h"

DEBUG_GET_ONCE_BOOL_OPTION(nosse, "GALLIUM_NOSSE", FALSE);

static struct util_cpu_caps *get_cpu_caps(void)
{
   util_cpu_detect();
   return &util_cpu_caps;
}

int rtasm_cpu_has_sse(void)
{
   return !debug_get_option_nosse() && get_cpu_caps()->has_sse;
}

int rtasm_cpu_has_sse2(void) 
{
   return !debug_get_option_nosse() && get_cpu_caps()->has_sse2;
}


#else

int rtasm_cpu_has_sse(void)
{
   return 0;
}

int rtasm_cpu_has_sse2(void)
{
   return 0;
}

#endif

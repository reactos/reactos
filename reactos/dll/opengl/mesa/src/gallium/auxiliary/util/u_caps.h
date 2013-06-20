/**************************************************************************
 *
 * Copyright 2010 Vmware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef U_CAPS_H
#define U_CAPS_H

#include "pipe/p_compiler.h"

struct pipe_screen;

enum u_caps_check_enum {
   UTIL_CAPS_CHECK_TERMINATE = 0,
   UTIL_CAPS_CHECK_CAP,
   UTIL_CAPS_CHECK_INT,
   UTIL_CAPS_CHECK_FLOAT,
   UTIL_CAPS_CHECK_FORMAT,
   UTIL_CAPS_CHECK_SHADER,
   UTIL_CAPS_CHECK_UNIMPLEMENTED,
};

#define UTIL_CHECK_CAP(cap) \
   UTIL_CAPS_CHECK_CAP, PIPE_CAP_##cap

#define UTIL_CHECK_INT(cap, higher) \
   UTIL_CAPS_CHECK_INT, PIPE_CAP_##cap, (unsigned)(higher)

/* Floats currently lose precision */
#define UTIL_CHECK_FLOAT(cap, higher) \
   UTIL_CAPS_CHECK_FLOAT, PIPE_CAPF_##cap, (unsigned)(int)(higher)

#define UTIL_CHECK_FORMAT(format) \
   UTIL_CAPS_CHECK_FORMAT, PIPE_FORMAT_##format

#define UTIL_CHECK_SHADER(shader, cap, higher) \
   UTIL_CAPS_CHECK_SHADER, (PIPE_SHADER_##shader << 24) | PIPE_SHADER_CAP_##cap, (unsigned)(higher)

#define UTIL_CHECK_UNIMPLEMENTED \
   UTIL_CAPS_CHECK_UNIMPLEMENTED

#define UTIL_CHECK_TERMINATE \
   UTIL_CAPS_CHECK_TERMINATE

boolean util_check_caps(struct pipe_screen *screen, const unsigned *list);
boolean util_check_caps_out(struct pipe_screen *screen, const unsigned *list, int *out);
void util_caps_demo_print(struct pipe_screen *screen);

#endif

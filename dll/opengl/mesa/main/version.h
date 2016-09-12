/*
 * Mesa 3-D graphics library
 * Version:  7.11
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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


#ifndef VERSION_H
#define VERSION_H


struct gl_context;


/* Mesa version */
#define MESA_MAJOR 8
#define MESA_MINOR 0
#define MESA_PATCH 4
#define MESA_VERSION_STRING "8.0.4"

/* To make version comparison easy */
#define MESA_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#define MESA_VERSION_CODE MESA_VERSION(MESA_MAJOR, MESA_MINOR, MESA_PATCH)


extern void
_mesa_compute_version(struct gl_context *ctx);

#endif /* VERSION_H */

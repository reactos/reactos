/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


/* Mesa version */
#define MESA_MAJOR 7
#define MESA_MINOR 2
#define MESA_PATCH 0
#define MESA_VERSION_STRING "7.2"

/* To make version comparison easy */
#define MESA_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#define MESA_VERSION_CODE MESA_VERSION(MESA_MAJOR, MESA_MINOR, MESA_PATCH)


/* OpenGL API version */
#define OPENGL_MAJOR 2
#define OPENGL_MINOR 1
#define OPENGL_PATCH 0
#define OPENGL_VERSION_STRING "2.1"

/* To make version comparison easy */
#define OPENGL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#define OPENGL_VERSION_CODE OPENGL_VERSION(OPENGL_MAJOR, OPENGL_MINOR, OPENGL_PATCH)


#endif /* VERSION_H */

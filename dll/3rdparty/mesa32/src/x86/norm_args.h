/* $Id: norm_args.h,v 1.4 2003/11/26 08:32:36 dborca Exp $ */

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
 * Normal transform function interface for assembly code.  Simply define
 * FRAME_OFFSET to the number of bytes pushed onto the stack before
 * using the ARG_* argument macros.
 *
 * Gareth Hughes
 */

#ifndef __NORM_ARGS_H__
#define __NORM_ARGS_H__

/* Offsets for normal_func arguments
 *
 * typedef void (*normal_func)( CONST GLmatrix *mat,
 *                              GLfloat scale,
 *                              CONST GLvector4f *in,
 *                              CONST GLfloat lengths[],
 *                              GLvector4f *dest );
 */
#define OFFSET_MAT	4
#define OFFSET_SCALE	8
#define OFFSET_IN	12
#define OFFSET_LENGTHS	16
#define OFFSET_DEST	20

#define ARG_MAT         REGOFF(FRAME_OFFSET+OFFSET_MAT, ESP)
#define ARG_SCALE       REGOFF(FRAME_OFFSET+OFFSET_SCALE, ESP)
#define ARG_IN          REGOFF(FRAME_OFFSET+OFFSET_IN, ESP)
#define ARG_LENGTHS     REGOFF(FRAME_OFFSET+OFFSET_LENGTHS, ESP)
#define ARG_DEST        REGOFF(FRAME_OFFSET+OFFSET_DEST, ESP)

#endif

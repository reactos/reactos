
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
 * Clip test function interface for assembly code.  Simply define
 * FRAME_OFFSET to the number of bytes pushed onto the stack before
 * using the ARG_* argument macros.
 *
 * Gareth Hughes
 */

#ifndef __CLIP_ARGS_H__
#define __CLIP_ARGS_H__

/*
 * Offsets for clip_func arguments
 *
 * typedef GLvector4f *(*clip_func)( GLvector4f *clip_vec,
 *	                             GLvector4f *proj_vec,
 *	                             GLubyte clipMask[],
 *	                             GLubyte *orMask,
 *	                             GLubyte *andMask );
 */

#define OFFSET_SOURCE	4
#define OFFSET_DEST	8
#define OFFSET_CLIP	12
#define OFFSET_OR	16
#define OFFSET_AND	20

#define ARG_SOURCE	REGOFF(FRAME_OFFSET+OFFSET_SOURCE, ESP)
#define ARG_DEST	REGOFF(FRAME_OFFSET+OFFSET_DEST, ESP)
#define ARG_CLIP	REGOFF(FRAME_OFFSET+OFFSET_CLIP, ESP)
#define ARG_OR		REGOFF(FRAME_OFFSET+OFFSET_OR, ESP)
#define ARG_AND		REGOFF(FRAME_OFFSET+OFFSET_AND, ESP)

#endif

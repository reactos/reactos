/**
 * \file extensions.h
 * Extension handling.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include "glheader.h"
#include "mfeatures.h"

struct gl_context;

#if _HAVE_FULL_GL

extern void _mesa_enable_sw_extensions(struct gl_context *ctx);

extern void _mesa_enable_1_3_extensions(struct gl_context *ctx);

extern void _mesa_enable_1_4_extensions(struct gl_context *ctx);

extern void _mesa_enable_1_5_extensions(struct gl_context *ctx);

extern void _mesa_enable_2_0_extensions(struct gl_context *ctx);

extern void _mesa_enable_2_1_extensions(struct gl_context *ctx);

extern void _mesa_enable_extension(struct gl_context *ctx, const char *name);

extern void _mesa_disable_extension(struct gl_context *ctx, const char *name);

extern GLboolean _mesa_extension_is_enabled(struct gl_context *ctx, const char *name);

extern void _mesa_init_extensions(struct gl_context *ctx);

extern GLubyte *_mesa_make_extension_string(struct gl_context *ctx);

extern GLuint
_mesa_get_extension_count(struct gl_context *ctx);

extern const GLubyte *
_mesa_get_enabled_extension(struct gl_context *ctx, GLuint index);


#else

/** No-op */
#define _mesa_extensions_dtr( ctx ) ((void)0)

/** No-op */
#define _mesa_extensions_ctr( ctx ) ((void)0)

/** No-op */
#define _mesa_extensions_get_string( ctx ) "GL_EXT_texture_object"

/** No-op */
#define _mesa_enable_extension( c, n ) ((void)0)

#endif

#endif

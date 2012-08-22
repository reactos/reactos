/*
 * Mesa 3-D graphics library
 * Version:  7.1
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


#ifndef _GLAPI_PRIV_H
#define _GLAPI_PRIV_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include "glapi/mesa.h"
#else /* HAVE_DIX_CONFIG_H */
#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "GL/glext.h"

/* The define of GL_COVERAGE_SAMPLES_NV in gl2ext.h is guarded by a different
 * extension (GL_NV_coverage_sample) than in glext.h
 * (GL_NV_multisample_coverage).  Just undefine it to avoid spurious compiler
 * warnings.
 */
#undef GL_COVERAGE_SAMPLES_NV

#include "GLES2/gl2platform.h"
#include "GLES2/gl2ext.h"

#ifndef GL_OES_fixed_point
typedef int GLfixed;
typedef int GLclampx;
#endif

#ifndef GL_OES_EGL_image
typedef void *GLeglImageOES;
#endif

#endif /* HAVE_DIX_CONFIG_H */

#include "glapi/glapi.h"


/* getproc */

extern void
_glapi_check_table_not_null(const struct _glapi_table *table);


extern void
_glapi_check_table(const struct _glapi_table *table);


/* entrypoint */

extern void
init_glapi_relocs_once(void);


extern _glapi_proc
generate_entrypoint(unsigned int functionOffset);


extern void
fill_in_entrypoint_offset(_glapi_proc entrypoint, unsigned int offset);


extern _glapi_proc
get_entrypoint_address(unsigned int functionOffset);


/**
 * Size (in bytes) of dispatch function (entrypoint).
 */
#if defined(USE_X86_ASM)
# if defined(GLX_USE_TLS)
#  define DISPATCH_FUNCTION_SIZE  16
# elif defined(THREADS)
#  define DISPATCH_FUNCTION_SIZE  32
# else
#  define DISPATCH_FUNCTION_SIZE  16
# endif
#endif

#if defined(USE_X64_64_ASM)
# if defined(GLX_USE_TLS)
#  define DISPATCH_FUNCTION_SIZE  16
# endif
#endif


/**
 * Number of extension functions which we can dynamically add at runtime.
 *
 * Number of extension functions is also subject to the size of backing exec
 * mem we allocate. For the common case of dispatch stubs with size 16 bytes,
 * the two limits will be hit simultaneously. For larger dispatch function
 * sizes, MAX_EXTENSION_FUNCS is effectively reduced.
 */
#define MAX_EXTENSION_FUNCS 256


#endif

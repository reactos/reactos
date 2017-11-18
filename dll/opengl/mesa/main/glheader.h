/*
 * Mesa 3-D graphics library
 * Version:  7.5
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


/**
 * \file glheader.h
 * Wrapper for GL/gl.h and GL/glext.h
 */


#ifndef GLHEADER_H
#define GLHEADER_H


#ifdef WGLAPI
#undef WGLAPI
#endif


#if !defined(OPENSTEP) && (defined(__WIN32__) && !defined(__CYGWIN__)) && !defined(BUILD_FOR_SNAP)
#  if (defined(_MSC_VER) || defined(__MINGW32__)) && defined(BUILD_GL32) /* tag specify we're building mesa as a DLL */
#    define WGLAPI __declspec(dllexport)
#  elif (defined(_MSC_VER) || defined(__MINGW32__)) && defined(_DLL) /* tag specifying we're building for DLL runtime support */
#    define WGLAPI __declspec(dllimport)
#  else /* for use with static link lib build of Win32 edition only */
#    define WGLAPI __declspec(dllimport)
#  endif /* _STATIC_MESA support */
#endif /* WIN32 / CYGWIN bracket */

#define WIN32_NO_STATUS

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

/* Threading glue for WINAPI */
#include <stdarg.h>
#include <winbase.h>
typedef CRITICAL_SECTION _glthread_Mutex;
#define _glthread_DECLARE_STATIC_MUTEX(name) static _glthread_Mutex name = {(PCRITICAL_SECTION_DEBUG)-1, -1, 0, 0, 0, 0}
#define _glthread_INIT_MUTEX(name)    InitializeCriticalSection(&name)
#define _glthread_DESTROY_MUTEX(name) DeleteCriticalSection(&name)
#define _glthread_LOCK_MUTEX(name)    EnterCriticalSection(&name)
#define _glthread_UNLOCK_MUTEX(name)  LeaveCriticalSection(&name)

/* Context glue with opengl32 */
#include "../opengl32/opengl32.h"
#define GET_CURRENT_CONTEXT(__ctx__) struct gl_context* __ctx__ = (struct gl_context*)IntGetCurrentICDPrivate()
#define GET_DISPATCH() ((struct _glapi_table*)IntGetCurrentDispatchTable())
#define _glapi_set_dispatch(table) IntSetCurrentDispatchTable((const GLDISPATCHTABLE*)(table))
#define _glapi_get_dispatch_table_size() (OPENGL_VERSION_110_ENTRIES)
#define _glapi_get_context() ((void*)IntGetCurrentICDPrivate())
#define _glapi_set_context(__ctx__) IntSetCurrentICDPrivate((void*)(__ctx__))


#ifdef __cplusplus
extern "C" {
#endif


/**
 * GL_FIXED is defined in glext.h version 64 but these typedefs aren't (yet).
 */
typedef int GLfixed;
typedef int GLclampx;


#ifndef GL_OES_EGL_image
typedef void *GLeglImageOES;
#endif


#ifndef GL_OES_EGL_image_external
#define GL_TEXTURE_EXTERNAL_OES                                 0x8D65
#define GL_SAMPLER_EXTERNAL_OES                                 0x8D66
#define GL_TEXTURE_BINDING_EXTERNAL_OES                         0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES                     0x8D68
#endif


#ifndef GL_OES_point_size_array
#define GL_POINT_SIZE_ARRAY_OES                                 0x8B9C
#define GL_POINT_SIZE_ARRAY_TYPE_OES                            0x898A
#define GL_POINT_SIZE_ARRAY_STRIDE_OES                          0x898B
#define GL_POINT_SIZE_ARRAY_POINTER_OES                         0x898C
#define GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES                  0x8B9F
#endif


#ifndef GL_OES_draw_texture
#define GL_TEXTURE_CROP_RECT_OES  0x8B9D
#endif


#ifndef GL_PROGRAM_BINARY_LENGTH_OES
#define GL_PROGRAM_BINARY_LENGTH_OES 0x8741
#endif

/* GLES 2.0 tokens */
#ifndef GL_RGB565
#define GL_RGB565 0x8D62
#endif

#ifndef GL_TEXTURE_GEN_STR_OES
#define GL_TEXTURE_GEN_STR_OES                                  0x8D60
#endif

#ifndef GL_OES_compressed_paletted_texture
#define GL_PALETTE4_RGB8_OES                                    0x8B90
#define GL_PALETTE4_RGBA8_OES                                   0x8B91
#define GL_PALETTE4_R5_G6_B5_OES                                0x8B92
#define GL_PALETTE4_RGBA4_OES                                   0x8B93
#define GL_PALETTE4_RGB5_A1_OES                                 0x8B94
#define GL_PALETTE8_RGB8_OES                                    0x8B95
#define GL_PALETTE8_RGBA8_OES                                   0x8B96
#define GL_PALETTE8_R5_G6_B5_OES                                0x8B97
#define GL_PALETTE8_RGBA4_OES                                   0x8B98
#define GL_PALETTE8_RGB5_A1_OES                                 0x8B99
#endif

#ifndef GL_OES_matrix_get
#define GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES               0x898D
#define GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES              0x898E
#define GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES                 0x898F
#endif

#ifndef GL_ES_VERSION_2_0
#define GL_SHADER_BINARY_FORMATS            0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS        0x8DF9
#define GL_SHADER_COMPILER                  0x8DFA
#define GL_MAX_VERTEX_UNIFORM_VECTORS       0x8DFB
#define GL_MAX_VARYING_VECTORS              0x8DFC
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS     0x8DFD
#endif


/**
 * Internal token to represent a GLSL shader program (a collection of
 * one or more shaders that get linked together).  Note that GLSL
 * shaders and shader programs share one name space (one hash table)
 * so we need a value that's different from any of the
 * GL_VERTEX/FRAGMENT/GEOMETRY_PROGRAM tokens.
 */
#define GL_SHADER_PROGRAM_MESA 0x9999

/* Several fields of struct gl_config can take these as values.  Since
 * GLX header files may not be available everywhere they need to be used,
 * redefine them here.
 */
#define GLX_NONE                           0x8000
#define GLX_SLOW_CONFIG                    0x8001
#define GLX_TRUE_COLOR                     0x8002
#define GLX_DIRECT_COLOR                   0x8003
#define GLX_PSEUDO_COLOR                   0x8004
#define GLX_STATIC_COLOR                   0x8005
#define GLX_GRAY_SCALE                     0x8006
#define GLX_STATIC_GRAY                    0x8007
#define GLX_TRANSPARENT_RGB                0x8008
#define GLX_TRANSPARENT_INDEX              0x8009
#define GLX_NON_CONFORMANT_CONFIG          0x800D
#define GLX_SWAP_EXCHANGE_OML              0x8061
#define GLX_SWAP_COPY_OML                  0x8062
#define GLX_SWAP_UNDEFINED_OML             0x8063

#define GLX_DONT_CARE                      0xFFFFFFFF


#ifdef __cplusplus
}
#endif

#endif /* GLHEADER_H */

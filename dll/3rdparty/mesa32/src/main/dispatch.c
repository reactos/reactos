/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * \file dispatch.c
 *
 * This file generates all the gl* function entrypoints.  This code is not
 * used if optimized assembly stubs are available (e.g., using x86/glapi_x86.S
 * on IA32 or sparc/glapi_sparc.S on SPARC).
 *
 * \note
 * This file is also used to build the client-side libGL that loads DRI-based
 * device drivers.  At build-time it is symlinked to src/glx/x11.
 *
 * \author Brian Paul <brian@precisioninsight.com>
 */

#ifndef GLX_USE_APPLEGL

#include "glheader.h"
#include "glapi.h"
#include "glapitable.h"
#include "glthread.h"


#if !(defined(USE_X86_ASM) || defined(USE_X86_64_ASM) || defined(USE_SPARC_ASM))

#if defined(WIN32)
#define KEYWORD1 GLAPI
#else
#define KEYWORD1 PUBLIC
#endif

#define KEYWORD2 GLAPIENTRY

#if defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#if 0  /* Use this to log GL calls to stdout (for DEBUG only!) */

#define F stdout
#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   fprintf MESSAGE;				\
   CALL_ ## FUNC(GET_DISPATCH(), ARGS);

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   fprintf MESSAGE;				\
   return CALL_ ## FUNC(GET_DISPATCH(), ARGS);

#else

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   CALL_ ## FUNC(GET_DISPATCH(), ARGS);

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   return CALL_ ## FUNC(GET_DISPATCH(), ARGS);

#endif /* logging */


#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#include "dispatch.h"
#include "glapitemp.h"

#endif /* USE_X86_ASM */

#endif /* !GLX_USE_APPLEGL */

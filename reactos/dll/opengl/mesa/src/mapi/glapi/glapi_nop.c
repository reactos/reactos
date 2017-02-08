/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * No-op dispatch table.
 *
 * This file defines a special dispatch table which is loaded with no-op
 * functions.
 *
 * When there's no current rendering context, calling a GL function like
 * glBegin() is a no-op.  Apps should never normally do this.  So as a
 * debugging aid, each of the no-op functions will emit a warning to
 * stderr if the MESA_DEBUG or LIBGL_DEBUG env var is set.
 */



#include "glapi/glapi_priv.h"


void
_glapi_noop_enable_warnings(unsigned char enable)
{
}

void
_glapi_set_warning_func(_glapi_proc func)
{
}

/*
 * When GLAPIENTRY is __stdcall (i.e. Windows), the stack is popped by the
 * callee making the number/type of arguments significant.
 */
#if defined(_WIN32) || defined(DEBUG)

/**
 * Called by each of the no-op GL entrypoints.
 */
static int
Warn(const char *func)
{
#if defined(DEBUG) && !defined(_WIN32_WCE)
   if (getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG")) {
      fprintf(stderr, "GL User Error: gl%s called without a rendering context\n",
              func);
   }
#endif
   return 0;
}


/**
 * This is called if the user somehow calls an unassigned GL dispatch function.
 */
static GLint
NoOpUnused(void)
{
   return Warn(" function");
}

/*
 * Defines for the glapitemp.h functions.
 */
#define KEYWORD1 static
#define KEYWORD1_ALT static
#define KEYWORD2 GLAPIENTRY
#define NAME(func)  NoOp##func
#define DISPATCH(func, args, msg)  Warn(#func);
#define RETURN_DISPATCH(func, args, msg)  Warn(#func); return 0


/*
 * Defines for the table of no-op entry points.
 */
#define TABLE_ENTRY(name) (_glapi_proc) NoOp##name

#else

static int
NoOpGeneric(void)
{
#if !defined(_WIN32_WCE)
   if (getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG")) {
      fprintf(stderr, "GL User Error: calling GL function without a rendering context\n");
   }
#endif
   return 0;
}

#define TABLE_ENTRY(name) (_glapi_proc) NoOpGeneric

#endif

#define DISPATCH_TABLE_NAME __glapi_noop_table
#define UNUSED_TABLE_NAME __unused_noop_functions

#include "glapi/glapitemp.h"

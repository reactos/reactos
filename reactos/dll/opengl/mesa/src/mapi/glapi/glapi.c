/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "glapi/glapi.h"
#include "mapi/u_current.h"

/*
 * Global variables, _glapi_get_context, and _glapi_get_dispatch are defined in
 * u_current.c.
 */

#ifdef GLX_USE_TLS
/* not used, but defined for compatibility */
const struct _glapi_table *_glapi_Dispatch;
const void *_glapi_Context;
#endif /* GLX_USE_TLS */

void
_glapi_destroy_multithread(void)
{
   u_current_destroy();
}

void
_glapi_check_multithread(void)
{
   u_current_init();
}

void
_glapi_set_context(void *context)
{
   u_current_set_user((const void *) context);
}

void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
   u_current_set((const struct mapi_table *) dispatch);
}

/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

#include <string.h>
#include "glapi/glapi.h"
#include "mapi/u_current.h"
#include "mapi/table.h" /* for MAPI_TABLE_NUM_SLOTS */
#include "mapi/stub.h"

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

/**
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
unsigned int
_glapi_get_dispatch_table_size(void)
{
   return MAPI_TABLE_NUM_SLOTS;
}

/**
 * Fill-in the dispatch stub for the named function.
 *
 * This function is intended to be called by a hardware driver.  When called,
 * a dispatch stub may be created created for the function.  A pointer to this
 * dispatch function will be returned by glXGetProcAddress.
 *
 * \param function_names       Array of pointers to function names that should
 *                             share a common dispatch offset.
 * \param parameter_signature  String representing the types of the parameters
 *                             passed to the named function.  Parameter types
 *                             are converted to characters using the following
 *                             rules:
 *                               - 'i' for \c GLint, \c GLuint, and \c GLenum
 *                               - 'p' for any pointer type
 *                               - 'f' for \c GLfloat and \c GLclampf
 *                               - 'd' for \c GLdouble and \c GLclampd
 *
 * \returns
 * The offset in the dispatch table of the named function.  A pointer to the
 * driver's implementation of the named function should be stored at
 * \c dispatch_table[\c offset].  Return -1 if error/problem.
 *
 * \sa glXGetProcAddress
 *
 * \warning
 * This function can only handle up to 8 names at a time.  As far as I know,
 * the maximum number of names ever associated with an existing GL function is
 * 4 (\c glPointParameterfSGIS, \c glPointParameterfEXT,
 * \c glPointParameterfARB, and \c glPointParameterf), so this should not be
 * too painful of a limitation.
 *
 * \todo
 * Check parameter_signature.
 */
int
_glapi_add_dispatch( const char * const * function_names,
		     const char * parameter_signature )
{
   const struct mapi_stub *function_stubs[8];
   const struct mapi_stub *alias = NULL;
   unsigned i;

   (void) memset(function_stubs, 0, sizeof(function_stubs));

   /* find the missing stubs, and decide the alias */
   for (i = 0; function_names[i] != NULL && i < 8; i++) {
      const char * funcName = function_names[i];
      const struct mapi_stub *stub;
      int slot;

      if (!funcName || funcName[0] != 'g' || funcName[1] != 'l')
         return -1;
      funcName += 2;

      stub = stub_find_public(funcName);
      if (!stub)
         stub = stub_find_dynamic(funcName, 0);

      slot = (stub) ? stub_get_slot(stub) : -1;
      if (slot >= 0) {
         if (alias && stub_get_slot(alias) != slot)
            return -1;
         /* use the first existing stub as the alias */
         if (!alias)
            alias = stub;

         function_stubs[i] = stub;
      }
   }

   /* generate missing stubs */
   for (i = 0; function_names[i] != NULL && i < 8; i++) {
      const char * funcName = function_names[i] + 2;
      struct mapi_stub *stub;

      if (function_stubs[i])
         continue;

      stub = stub_find_dynamic(funcName, 1);
      if (!stub)
         return -1;

      stub_fix_dynamic(stub, alias);
      if (!alias)
         alias = stub;
   }

   return (alias) ? stub_get_slot(alias) : -1;
}

static const struct mapi_stub *
_glapi_get_stub(const char *name, int generate)
{
   const struct mapi_stub *stub;

#ifdef USE_MGL_NAMESPACE
   if (name)
      name++;
#endif

   if (!name || name[0] != 'g' || name[1] != 'l')
      return NULL;
   name += 2;

   stub = stub_find_public(name);
   if (!stub)
      stub = stub_find_dynamic(name, generate);

   return stub;
}

/**
 * Return offset of entrypoint for named function within dispatch table.
 */
int
_glapi_get_proc_offset(const char *funcName)
{
   const struct mapi_stub *stub = _glapi_get_stub(funcName, 0);
   return (stub) ? stub_get_slot(stub) : -1;
}

/**
 * Return pointer to the named function.  If the function name isn't found
 * in the name of static functions, try generating a new API entrypoint on
 * the fly with assembly language.
 */
_glapi_proc
_glapi_get_proc_address(const char *funcName)
{
   const struct mapi_stub *stub = _glapi_get_stub(funcName, 1);
   return (stub) ? (_glapi_proc) stub_get_addr(stub) : NULL;
}

/**
 * Return the name of the function at the given dispatch offset.
 * This is only intended for debugging.
 */
const char *
_glapi_get_proc_name(unsigned int offset)
{
   /* not implemented */
   return NULL;
}

unsigned long
_glthread_GetID(void)
{
   return u_thread_self();
}

void
_glapi_noop_enable_warnings(unsigned char enable)
{
}

void
_glapi_set_warning_func(_glapi_proc func)
{
}

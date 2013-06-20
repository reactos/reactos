/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
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
 */


/**
 * \file remap.c
 * Remap table management.
 *
 * Entries in the dispatch table are either static or dynamic.  The
 * dispatch table is shared by mesa core and glapi.  When they are
 * built separately, it is possible that a static entry in mesa core
 * is dynamic, or assigned a different static offset, in glapi.  The
 * remap table is in charge of mapping a static entry in mesa core to
 * a dynamic entry, or the corresponding static entry, in glapi.
 */

#include "mfeatures.h"

#if FEATURE_remap_table

#include "remap.h"
#include "imports.h"
#include "glapi/glapi.h"

#define MAX_ENTRY_POINTS 16

#define need_MESA_remap_table
#include "main/remap_helper.h"


/* this is global for quick access */
int driDispatchRemapTable[driDispatchRemapTable_size];


/**
 * Return the spec string associated with the given function index.
 * The index is available from including remap_helper.h.
 *
 * \param func_index an opaque function index.
 *
 * \return the spec string associated with the function index, or NULL.
 */
const char *
_mesa_get_function_spec(GLint func_index)
{
   if (func_index < Elements(_mesa_function_pool))
      return _mesa_function_pool + func_index;
   else
      return NULL;
}


/**
 * Map a function by its spec.  The function will be added to glapi,
 * and the dispatch offset will be returned.
 *
 * \param spec a '\0'-separated string array specifying a function.
 *        It begins with the parameter signature of the function,
 *        followed by the names of the entry points.  An empty entry
 *        point name terminates the array.
 *
 * \return the offset of the (re-)mapped function in the dispatch
 *         table, or -1.
 */
GLint
_mesa_map_function_spec(const char *spec)
{
   const char *signature;
   const char *names[MAX_ENTRY_POINTS + 1];
   GLint num_names = 0;

   if (!spec)
      return -1;

   signature = spec;
   spec += strlen(spec) + 1;

   /* spec is terminated by an empty string */
   while (*spec) {
      names[num_names] = spec;
      num_names++;
      if (num_names >= MAX_ENTRY_POINTS)
         break;
      spec += strlen(spec) + 1;
   }
   if (!num_names)
      return -1;

   names[num_names] = NULL;

   /* add the entry points to the dispatch table */
   return _glapi_add_dispatch(names, signature);
}


/**
 * Map an array of functions.  This is a convenient function for
 * use with arrays available from including remap_helper.h.
 *
 * Note that the dispatch offsets of the functions are not returned.
 * If they are needed, _mesa_map_function_spec() should be used.
 *
 * \param func_array an array of function remaps.
 */
void
_mesa_map_function_array(const struct gl_function_remap *func_array)
{
   GLint i;

   if (!func_array)
      return;

   for (i = 0; func_array[i].func_index != -1; i++) {
      const char *spec;
      GLint offset;

      spec = _mesa_get_function_spec(func_array[i].func_index);
      if (!spec) {
         _mesa_problem(NULL, "invalid function index %d",
                       func_array[i].func_index);
         continue;
      }

      offset = _mesa_map_function_spec(spec);
      /* error checks */
      if (offset < 0) {
         const char *name = spec + strlen(spec) + 1;
         _mesa_warning(NULL, "failed to remap %s", name);
      }
      else if (func_array[i].dispatch_offset >= 0 &&
               offset != func_array[i].dispatch_offset) {
         const char *name = spec + strlen(spec) + 1;
         _mesa_problem(NULL, "%s should be mapped to %d, not %d",
                       name, func_array[i].dispatch_offset, offset);
      }
   }
}


/**
 * Map the functions which are already static.
 *
 * When a extension function are incorporated into the ABI, the
 * extension suffix is usually stripped.  Mapping such functions
 * makes sure the alternative names are available.
 *
 * Note that functions mapped by _mesa_init_remap_table() are
 * excluded.
 */
void
_mesa_map_static_functions(void)
{
   /* Remap static functions which have alternative names and are in the ABI.
    * This is to be on the safe side.  glapi should have defined those names.
    */
   _mesa_map_function_array(MESA_alt_functions);
}


/**
 * Initialize the remap table.  This is called in one_time_init().
 * The remap table needs to be initialized before calling the
 * CALL/GET/SET macros defined in main/dispatch.h.
 */
static void
_mesa_do_init_remap_table(const char *pool,
			  int size,
			  const struct gl_function_pool_remap *remap)
{
   static GLboolean initialized = GL_FALSE;
   GLint i;

   if (initialized)
      return;
   initialized = GL_TRUE;

   /* initialize the remap table */
   for (i = 0; i < size; i++) {
      GLint offset;
      const char *spec;

      /* sanity check */
      ASSERT(i == remap[i].remap_index);
      spec = _mesa_function_pool + remap[i].pool_index;

      offset = _mesa_map_function_spec(spec);
      /* store the dispatch offset in the remap table */
      driDispatchRemapTable[i] = offset;
      if (offset < 0)
         _mesa_warning(NULL, "failed to remap index %d", i);
   }
}


void
_mesa_init_remap_table(void)
{
   _mesa_do_init_remap_table(_mesa_function_pool,
			     driDispatchRemapTable_size,
			     MESA_remap_table_functions);
}


#endif /* FEATURE_remap_table */

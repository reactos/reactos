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

#include <stdlib.h>
#include <string.h>

#include "u_current.h"
#include "u_thread.h"
#include "mapi.h"
#include "stub.h"
#include "table.h"

/* dynamic stubs will run out before this array */
static const struct mapi_stub *mapi_stub_map[MAPI_TABLE_NUM_SLOTS];
static int mapi_num_stubs;

static const struct mapi_stub *
get_stub(const char *name, const struct mapi_stub *alias)
{
   const struct mapi_stub *stub;

   stub = stub_find_public(name);
   if (!stub) {
      struct mapi_stub *dyn = stub_find_dynamic(name, 1);
      if (dyn) {
         stub_fix_dynamic(dyn, alias);
         stub = dyn;
      }
   }

   return stub;
}

/**
 * Initialize mapi.  spec consists of NULL-separated strings.  The first string
 * denotes the version.  It is followed by variable numbers of entries.  Each
 * entry can have multiple names.  An empty name terminates an entry.  An empty
 * entry terminates the spec.  A spec of two entries, Foo and Bar, is as
 * follows
 *
 *   "1\0"
 *   "Foo\0"
 *   "FooEXT\0"
 *   "\0"
 *   "Bar\0"
 *   "\0"
 */
void
mapi_init(const char *spec)
{
   u_mutex_declare_static(mutex);
   const char *p;
   int ver, count;

   u_mutex_lock(mutex);

   /* already initialized */
   if (mapi_num_stubs) {
      u_mutex_unlock(mutex);
      return;
   }

   count = 0;
   p = spec;

   /* parse version string */
   ver = atoi(p);
   if (ver != 1) {
      u_mutex_unlock(mutex);
      return;
   }
   p += strlen(p) + 1;

   while (*p) {
      const struct mapi_stub *stub;

      stub = get_stub(p, NULL);
      /* out of dynamic entries */
      if (!stub)
         break;
      p += strlen(p) + 1;

      while (*p) {
         get_stub(p, stub);
         p += strlen(p) + 1;
      }

      mapi_stub_map[count++] = stub;
      p++;
   }

   mapi_num_stubs = count;

   u_mutex_unlock(mutex);
}

/**
 * Return the address of an entry.  Optionally generate the entry if it does
 * not exist.
 */
mapi_proc
mapi_get_proc_address(const char *name)
{
   const struct mapi_stub *stub;

   stub = stub_find_public(name);
   if (!stub)
      stub = stub_find_dynamic(name, 0);

   return (stub) ? (mapi_proc) stub_get_addr(stub) : NULL;
}

/**
 * Create a dispatch table.
 */
struct mapi_table *
mapi_table_create(void)
{
   const struct mapi_table *noop = table_get_noop();
   struct mapi_table *tbl;

   tbl = malloc(MAPI_TABLE_SIZE);
   if (tbl)
      memcpy(tbl, noop, MAPI_TABLE_SIZE);

   return tbl;
}

/**
 * Destroy a dispatch table.
 */
void
mapi_table_destroy(struct mapi_table *tbl)
{
   free(tbl);
}

/**
 * Fill a dispatch table.  The order of the procs is determined when mapi_init
 * is called.
 */
void
mapi_table_fill(struct mapi_table *tbl, const mapi_proc *procs)
{
   const struct mapi_table *noop = table_get_noop();
   int i;

   for (i = 0; i < mapi_num_stubs; i++) {
      const struct mapi_stub *stub = mapi_stub_map[i];
      int slot = stub_get_slot(stub);
      mapi_func func = (mapi_func) procs[i];

      if (!func)
         func = table_get_func(noop, slot);
      table_set_func(tbl, slot, func);
   }
}

/**
 * Make a dispatch table current.
 */
void
mapi_table_make_current(const struct mapi_table *tbl)
{
   u_current_set(tbl);
}

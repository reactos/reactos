/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#if defined(DEBUG)

/* see http://www.mozilla.org/performance/refcnt-balancer.html for what do with the output
 * on Linux, use tools/addr2line.sh to postprocess it before anything else
 **/

#include <stdio.h>

#include "util/u_debug.h"
#include "util/u_debug_refcnt.h"
#include "util/u_debug_stack.h"
#include "util/u_debug_symbol.h"
#include "util/u_string.h"
#include "util/u_hash_table.h"
#include "os/os_thread.h"

int debug_refcnt_state;

FILE* stream;

/* TODO: maybe move this serial machinery to a stand-alone module and expose it? */
pipe_static_mutex(serials_mutex);

static struct util_hash_table* serials_hash;
static unsigned serials_last;

static unsigned hash_ptr(void* p)
{
   return (unsigned)(uintptr_t)p;
}

static int compare_ptr(void* a, void* b)
{
   if(a == b)
      return 0;
   else if(a < b)
      return -1;
   else
      return 1;
}

static boolean debug_serial(void* p, unsigned* pserial)
{
   unsigned serial;
   boolean found = TRUE;
#ifdef PIPE_SUBSYSTEM_WINDOWS_USER
   static boolean first = TRUE;

   if (first) {
      pipe_mutex_init(serials_mutex);
      first = FALSE;
   }
#endif

   pipe_mutex_lock(serials_mutex);
   if(!serials_hash)
      serials_hash = util_hash_table_create(hash_ptr, compare_ptr);
   serial = (unsigned)(uintptr_t)util_hash_table_get(serials_hash, p);
   if(!serial)
   {
      /* time to stop logging... (you'll have a 100 GB logfile at least at this point)
       * TODO: avoid this
       */
      serial = ++serials_last;
      if(!serial)
      {
         debug_error("More than 2^32 objects detected, aborting.\n");
         os_abort();
      }

      util_hash_table_set(serials_hash, p, (void*)(uintptr_t)serial);
      found = FALSE;
   }
   pipe_mutex_unlock(serials_mutex);
   *pserial = serial;
   return found;
}

static void debug_serial_delete(void* p)
{
   pipe_mutex_lock(serials_mutex);
   util_hash_table_remove(serials_hash, p);
   pipe_mutex_unlock(serials_mutex);
}

#define STACK_LEN 64

static void dump_stack(const char* symbols[STACK_LEN])
{
   unsigned i;
   for(i = 0; i < STACK_LEN; ++i)
   {
      if(symbols[i])
         fprintf(stream, "%s\n", symbols[i]);
   }
   fprintf(stream, "\n");
}

void debug_reference_slowpath(const struct pipe_reference* p, debug_reference_descriptor get_desc, int change)
{
   if(debug_refcnt_state < 0)
      return;

   if(!debug_refcnt_state)
   {
      const char* filename = debug_get_option("GALLIUM_REFCNT_LOG", NULL);
      if(filename && filename[0])
         stream = fopen(filename, "wt");

      if(stream)
         debug_refcnt_state = 1;
      else
         debug_refcnt_state = -1;
   }

   if(debug_refcnt_state > 0)
   {
      struct debug_stack_frame frames[STACK_LEN];
      const char* symbols[STACK_LEN];
      char buf[1024];

      unsigned i;
      unsigned refcnt = p->count;
      unsigned serial;
      boolean existing = debug_serial((void*)p, &serial);

      debug_backtrace_capture(frames, 1, STACK_LEN);
      for(i = 0; i < STACK_LEN; ++i)
      {
         if(frames[i].function)
            symbols[i] = debug_symbol_name_cached(frames[i].function);
         else
            symbols[i] = 0;
      }

      get_desc(buf, p);

      if(!existing)
      {
         fprintf(stream, "<%s> %p %u Create\n", buf, (void *) p, serial);
         dump_stack(symbols);

         /* this is there to provide a gradual change even if we don't see the initialization */
         for(i = 1; i <= refcnt - change; ++i)
         {
            fprintf(stream, "<%s> %p %u AddRef %u\n", buf, (void *) p,
                    serial, i);
            dump_stack(symbols);
         }
      }

      if(change)
      {
         fprintf(stream, "<%s> %p %u %s %u\n", buf, (void *) p, serial,
                 change > 0 ? "AddRef" : "Release", refcnt);
         dump_stack(symbols);
      }

      if(!refcnt)
      {
         debug_serial_delete((void*)p);
         fprintf(stream, "<%s> %p %u Destroy\n", buf, (void *) p, serial);
         dump_stack(symbols);
      }

      fflush(stream);
   }
}
#endif

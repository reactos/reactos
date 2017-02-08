/* -*- c++ -*- */
/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "glsl_symbol_table.h"

class symbol_table_entry {
public:
   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *entry = ralloc_size(ctx, size);
      assert(entry != NULL);
      return entry;
   }

   /* If the user *does* call delete, that's OK, we will just ralloc_free. */
   static void operator delete(void *entry)
   {
      ralloc_free(entry);
   }

   symbol_table_entry(ir_variable *v)                     : v(v), f(0), t(0) {}
   symbol_table_entry(ir_function *f)                     : v(0), f(f), t(0) {}
   symbol_table_entry(const glsl_type *t)                 : v(0), f(0), t(t) {}

   ir_variable *v;
   ir_function *f;
   const glsl_type *t;
};

glsl_symbol_table::glsl_symbol_table()
{
   this->language_version = 120;
   this->table = _mesa_symbol_table_ctor();
   this->mem_ctx = ralloc_context(NULL);
}

glsl_symbol_table::~glsl_symbol_table()
{
   _mesa_symbol_table_dtor(table);
   ralloc_free(mem_ctx);
}

void glsl_symbol_table::push_scope()
{
   _mesa_symbol_table_push_scope(table);
}

void glsl_symbol_table::pop_scope()
{
   _mesa_symbol_table_pop_scope(table);
}

bool glsl_symbol_table::name_declared_this_scope(const char *name)
{
   return _mesa_symbol_table_symbol_scope(table, -1, name) == 0;
}

bool glsl_symbol_table::add_variable(ir_variable *v)
{
   if (this->language_version == 110) {
      /* In 1.10, functions and variables have separate namespaces. */
      symbol_table_entry *existing = get_entry(v->name);
      if (name_declared_this_scope(v->name)) {
	 /* If there's already an existing function (not a constructor!) in
	  * the current scope, just update the existing entry to include 'v'.
	  */
	 if (existing->v == NULL && existing->t == NULL) {
	    existing->v = v;
	    return true;
	 }
      } else {
	 /* If not declared at this scope, add a new entry.  But if an existing
	  * entry includes a function, propagate that to this block - otherwise
	  * the new variable declaration would shadow the function.
	  */
	 symbol_table_entry *entry = new(mem_ctx) symbol_table_entry(v);
	 if (existing != NULL)
	    entry->f = existing->f;
	 int added = _mesa_symbol_table_add_symbol(table, -1, v->name, entry);
	 assert(added == 0);
	 (void)added;
	 return true;
      }
      return false;
   }

   /* 1.20+ rules: */
   symbol_table_entry *entry = new(mem_ctx) symbol_table_entry(v);
   return _mesa_symbol_table_add_symbol(table, -1, v->name, entry) == 0;
}

bool glsl_symbol_table::add_type(const char *name, const glsl_type *t)
{
   symbol_table_entry *entry = new(mem_ctx) symbol_table_entry(t);
   return _mesa_symbol_table_add_symbol(table, -1, name, entry) == 0;
}

bool glsl_symbol_table::add_function(ir_function *f)
{
   if (this->language_version == 110 && name_declared_this_scope(f->name)) {
      /* In 1.10, functions and variables have separate namespaces. */
      symbol_table_entry *existing = get_entry(f->name);
      if ((existing->f == NULL) && (existing->t == NULL)) {
	 existing->f = f;
	 return true;
      }
   }
   symbol_table_entry *entry = new(mem_ctx) symbol_table_entry(f);
   return _mesa_symbol_table_add_symbol(table, -1, f->name, entry) == 0;
}

void glsl_symbol_table::add_global_function(ir_function *f)
{
   symbol_table_entry *entry = new(mem_ctx) symbol_table_entry(f);
   int added = _mesa_symbol_table_add_global_symbol(table, -1, f->name, entry);
   assert(added == 0);
   (void)added;
}

ir_variable *glsl_symbol_table::get_variable(const char *name)
{
   symbol_table_entry *entry = get_entry(name);
   return entry != NULL ? entry->v : NULL;
}

const glsl_type *glsl_symbol_table::get_type(const char *name)
{
   symbol_table_entry *entry = get_entry(name);
   return entry != NULL ? entry->t : NULL;
}

ir_function *glsl_symbol_table::get_function(const char *name)
{
   symbol_table_entry *entry = get_entry(name);
   return entry != NULL ? entry->f : NULL;
}

symbol_table_entry *glsl_symbol_table::get_entry(const char *name)
{
   return (symbol_table_entry *)
      _mesa_symbol_table_find_symbol(table, -1, name);
}

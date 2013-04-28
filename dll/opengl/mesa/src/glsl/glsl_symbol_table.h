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

#pragma once
#ifndef GLSL_SYMBOL_TABLE
#define GLSL_SYMBOL_TABLE

#include <new>

extern "C" {
#include "program/symbol_table.h"
}
#include "ir.h"
#include "glsl_types.h"

class symbol_table_entry;

/**
 * Facade class for _mesa_symbol_table
 *
 * Wraps the existing \c _mesa_symbol_table data structure to enforce some
 * type safe and some symbol table invariants.
 */
struct glsl_symbol_table {
private:
   static void
   _glsl_symbol_table_destructor (glsl_symbol_table *table)
   {
      table->~glsl_symbol_table();
   }

public:
   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *table;

      table = ralloc_size(ctx, size);
      assert(table != NULL);

      ralloc_set_destructor(table, (void (*)(void*)) _glsl_symbol_table_destructor);

      return table;
   }

   /* If the user *does* call delete, that's OK, we will just
    * ralloc_free in that case. Here, C++ will have already called the
    * destructor so tell ralloc not to do that again. */
   static void operator delete(void *table)
   {
      ralloc_set_destructor(table, NULL);
      ralloc_free(table);
   }
   
   glsl_symbol_table();
   ~glsl_symbol_table();

   unsigned int language_version;

   void push_scope();
   void pop_scope();

   /**
    * Determine whether a name was declared at the current scope
    */
   bool name_declared_this_scope(const char *name);

   /**
    * \name Methods to add symbols to the table
    *
    * There is some temptation to rename all these functions to \c add_symbol
    * or similar.  However, this breaks symmetry with the getter functions and
    * reduces the clarity of the intention of code that uses these methods.
    */
   /*@{*/
   bool add_variable(ir_variable *v);
   bool add_type(const char *name, const glsl_type *t);
   bool add_function(ir_function *f);
   /*@}*/

   /**
    * Add an function at global scope without checking for scoping conflicts.
    */
   void add_global_function(ir_function *f);

   /**
    * \name Methods to get symbols from the table
    */
   /*@{*/
   ir_variable *get_variable(const char *name);
   const glsl_type *get_type(const char *name);
   ir_function *get_function(const char *name);
   /*@}*/

private:
   symbol_table_entry *get_entry(const char *name);

   struct _mesa_symbol_table *table;
   void *mem_ctx;
};

#endif /* GLSL_SYMBOL_TABLE */

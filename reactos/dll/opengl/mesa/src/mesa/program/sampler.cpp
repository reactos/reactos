/*
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  VMware, Inc.   All Rights Reserved.
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

#include "ir.h"
#include "glsl_types.h"
#include "ir_visitor.h"
#include "../glsl/program.h"
#include "program/hash_table.h"
#include "ir_uniform.h"

extern "C" {
#include "main/compiler.h"
#include "main/mtypes.h"
#include "program/prog_parameter.h"
}

class get_sampler_name : public ir_hierarchical_visitor
{
public:
   get_sampler_name(ir_dereference *last,
		    struct gl_shader_program *shader_program)
   {
      this->mem_ctx = ralloc_context(NULL);
      this->shader_program = shader_program;
      this->name = NULL;
      this->offset = 0;
      this->last = last;
   }

   ~get_sampler_name()
   {
      ralloc_free(this->mem_ctx);
   }

   virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      this->name = ir->var->name;
      return visit_continue;
   }

   virtual ir_visitor_status visit_leave(ir_dereference_record *ir)
   {
      this->name = ralloc_asprintf(mem_ctx, "%s.%s", name, ir->field);
      return visit_continue;
   }

   virtual ir_visitor_status visit_leave(ir_dereference_array *ir)
   {
      ir_constant *index = ir->array_index->as_constant();
      int i;

      if (index) {
	 i = index->value.i[0];
      } else {
	 /* GLSL 1.10 and 1.20 allowed variable sampler array indices,
	  * while GLSL 1.30 requires that the array indices be
	  * constant integer expressions.  We don't expect any driver
	  * to actually work with a really variable array index, so
	  * all that would work would be an unrolled loop counter that ends
	  * up being constant above.
	  */
	 ralloc_strcat(&shader_program->InfoLog,
		       "warning: Variable sampler array index unsupported.\n"
		       "This feature of the language was removed in GLSL 1.20 "
		       "and is unlikely to be supported for 1.10 in Mesa.\n");
	 i = 0;
      }
      if (ir != last) {
	 this->name = ralloc_asprintf(mem_ctx, "%s[%d]", name, i);
      } else {
	 offset = i;
      }
      return visit_continue;
   }

   struct gl_shader_program *shader_program;
   const char *name;
   void *mem_ctx;
   int offset;
   ir_dereference *last;
};

extern "C" {
int
_mesa_get_sampler_uniform_value(class ir_dereference *sampler,
				struct gl_shader_program *shader_program,
				const struct gl_program *prog)
{
   get_sampler_name getname(sampler, shader_program);

   sampler->accept(&getname);

   unsigned location;
   if (!shader_program->UniformHash->get(location, getname.name)) {
      linker_error(shader_program,
		   "failed to find sampler named %s.\n", getname.name);
      return 0;
   }

   return shader_program->UniformStorage[location].sampler + getname.offset;
}
}

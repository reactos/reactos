/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
  

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_eu.h"



/* How does predicate control work when execution_size != 8?  Do I
 * need to test/set for 0xffff when execution_size is 16?
 */
void brw_set_predicate_control_flag_value( struct brw_compile *p, GLuint value )
{
   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   if (value != 0xff) {
      if (value != p->flag_value) {
	 brw_push_insn_state(p);
	 brw_MOV(p, brw_flag_reg(), brw_imm_uw(value));
	 p->flag_value = value;
	 brw_pop_insn_state(p);
      }

      p->current->header.predicate_control = BRW_PREDICATE_NORMAL;
   }   
}

void brw_set_predicate_control( struct brw_compile *p, GLuint pc )
{
   p->current->header.predicate_control = pc;
}

void brw_set_conditionalmod( struct brw_compile *p, GLuint conditional )
{
   p->current->header.destreg__conditonalmod = conditional;
}

void brw_set_access_mode( struct brw_compile *p, GLuint access_mode )
{
   p->current->header.access_mode = access_mode;
}

void brw_set_compression_control( struct brw_compile *p, GLboolean compression_control )
{
   p->current->header.compression_control = compression_control;
}

void brw_set_mask_control( struct brw_compile *p, GLuint value )
{
   p->current->header.mask_control = value;
}

void brw_set_saturate( struct brw_compile *p, GLuint value )
{
   p->current->header.saturate = value;
}

void brw_push_insn_state( struct brw_compile *p )
{
   assert(p->current != &p->stack[BRW_EU_MAX_INSN_STACK-1]);
   memcpy(p->current+1, p->current, sizeof(struct brw_instruction));
   p->current++;   
}

void brw_pop_insn_state( struct brw_compile *p )
{
   assert(p->current != p->stack);
   p->current--;
}


/***********************************************************************
 */
void brw_init_compile( struct brw_compile *p )
{
   p->nr_insn = 0;
   p->current = p->stack;
   memset(p->current, 0, sizeof(p->current[0]));

   /* Some defaults?
    */
   brw_set_mask_control(p, BRW_MASK_ENABLE); /* what does this do? */
   brw_set_saturate(p, 0);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_predicate_control_flag_value(p, 0xff); 
}


const GLuint *brw_get_program( struct brw_compile *p,
			       GLuint *sz )
{
   GLuint i;

   for (i = 0; i < 8; i++)
      brw_NOP(p);

   *sz = p->nr_insn * sizeof(struct brw_instruction);
   return (const GLuint *)p->store;
}


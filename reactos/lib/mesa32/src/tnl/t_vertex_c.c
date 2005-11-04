/*
 * Copyright 2003 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "colormac.h"

#include "t_context.h"
#include "t_vertex.h"

#include "simple_list.h"

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

/* A version of code generation for t_clipspace_codegen.c which prints
 * out 'c' code to implement the generated function.  A useful
 * debugging tool, and in concert with something like tcc or a
 * library-ized gcc, could do the whole job.
 */


static GLboolean emit( struct tnl_clipspace_codegen *p,
		  const char *fmt,
		  ... )
{
   if (p->buf_used < p->buf_size) {
      va_list ap;
      va_start( ap, fmt );  
      p->buf_used += vsnprintf( p->buf + p->buf_used, 
				p->buf_size - p->buf_used,
				fmt, ap );   
      va_end( ap );
   }
   
   return p->buf_used < p->buf_size;
}


static GLboolean print_header( struct tnl_clipspace_codegen *p,
			  struct tnl_clipspace *vtx )
{
   p->buf_used = 0;
   p->out_offset = 0;

   return 
      emit(p,
	   "struct tnl_clipspace_attr\n"
	   "{\n"
	   "   unsigned int attrib;          \n"
	   "   unsigned int format;\n"
	   "   unsigned int vertoffset;      \n"
	   "   unsigned int vertattrsize;    \n"
	   "   char *inputptr;\n"
	   "   unsigned int inputstride;\n"
	   "   void *insert;\n"
	   "   void *emit;\n"
	   "   void * extract;\n"
	   "   const float *vp;   \n"
	   "};\n"
	   "\n"
	 ) && 
      emit(p, 
	   "void emit_vertices( int start, int end, char *dest, \n"
	   "                    struct tnl_clipspace_attr *a) \n"
	   "{\n"
	   "   int i;"
	   "   for (i = start ; i < end ; i++, dest += %d) {\n",
	   vtx->vertex_size);

}

static GLboolean print_footer( struct tnl_clipspace_codegen *p )
{
   return 
      emit(p, 
	   "   }\n"
	   "}\n"
	 );
}

static GLboolean emit_reg( struct tnl_clipspace_codegen *p, GLint reg )
{
   int idx = reg & REG_OFFSET_MASK;

   switch (reg & REG_MASK) {
   case REG_IN: return emit(p, "in[%d]", idx);
   case REG_VP: return emit(p, "vp[%d]", idx);
   case REG_TMP: return emit(p, "temp[%d]", idx); /* not used? */
   case REG_OUT: return emit(p, "out[%d]", idx);
   }
   
   return GL_FALSE;
}

static GLboolean print_mov( struct tnl_clipspace_codegen *p, GLint dest, GLint src )
{
   return 
      emit(p, "         ") &&
      emit_reg(p, dest) &&
      emit(p, " = ") &&
      emit_reg(p, src) &&
      emit(p, ";\n");
}


static GLboolean print_const( struct tnl_clipspace_codegen *p, 
			 GLint dest, GLfloat c )
{
   return 
      emit(p, "         ") &&
      emit_reg(p, dest) &&
      emit(p, " = %g;\n", c);
}

static GLboolean print_const_chan( struct tnl_clipspace_codegen *p, 
			      GLint dest, GLchan c )
{
   return 
      emit(p, "         ") &&
      emit_reg(p, dest) &&
      emit(p, " = ") &&
#if CHAN_TYPE == GL_FLOAT
      emit(p, "%f", c) &&
#else
      emit(p, "%d", c) &&
#endif
      emit(p, ";\n");
}

static GLboolean print_const_ubyte( struct tnl_clipspace_codegen *p, 
			       GLint dest, GLubyte c )
{
   return 
      emit(p, "         ") &&
      emit_reg(p, dest) &&
      emit(p, " = %x;\n", c);
}

static GLboolean print_mad( struct tnl_clipspace_codegen *p,
		       GLint dest, GLint src0, GLint src1, GLint src2 )
{
   return 
      emit(p, "         ") &&
      emit_reg(p, dest) &&
      emit(p, " = ") &&
      emit_reg(p, src0) &&
      emit(p, " * ") &&
      emit_reg(p, src1) &&
      emit(p, " + ") &&
      emit_reg(p, src2) &&
      emit(p, ";\n");
}

static GLboolean print_float_to_ubyte( struct tnl_clipspace_codegen *p,
				  GLint dest, GLint src )
{
   return 
      emit(p, "         ") &&
      emit(p, "UNCLAMPED_FLOAT_TO_UBYTE(") &&
      emit_reg(p, dest) &&
      emit(p, ", ") &&
      emit_reg(p, src) &&
      emit(p, ");\n");
}

static GLboolean print_float_to_chan( struct tnl_clipspace_codegen *p, 
				 GLint dest, GLint src )
{
   return 
      emit(p, "         ") &&
      emit(p, "UNCLAMPED_FLOAT_TO_CHAN(") &&
      emit_reg(p, dest) &&
      emit(p, ", ") &&
      emit_reg(p, src) &&
      emit(p, ");\n");
}


static GLboolean print_attr_header( struct tnl_clipspace_codegen *p, 
			       struct tnl_clipspace_attr *a, 
			       GLint j,
			       GLenum out_type, 
			       GLboolean need_vp)
{
   char *out_type_str = "void";

   switch(out_type) {
   case GL_FLOAT: out_type_str = "float"; break;
   case GL_UNSIGNED_BYTE: out_type_str = "unsigned char"; break;
   case GL_UNSIGNED_SHORT: out_type_str = "unsigned short"; break;
   }

   return 
      emit(p, "      {\n") &&
      (need_vp ? emit(p, "         const float *vp = a[%d].vp;\n", j) : 1) &&
      emit(p, "         %s *out = (%s *)(dest + %d);\n", 
	   out_type_str, out_type_str, a[j].vertoffset) &&
      emit(p, "         const float *in = (const float *)a[%d].inputptr;\n", 
	   j) &&
      emit(p, "         a[%d].inputptr += a[%d].inputstride;\n", j, j);
}

static GLboolean print_attr_footer( struct tnl_clipspace_codegen *p )
{
   return emit(p, "      }\n");
}

static tnl_emit_func print_store_func( struct tnl_clipspace_codegen *p ) 
{
   fprintf(stderr, "print_store_func: emitted:\n%s\n", p->buf);
   return 0;
}

void _tnl_init_c_codegen( struct tnl_clipspace_codegen *p ) 
{
   p->emit_header = print_header;
   p->emit_footer = print_footer;
   p->emit_attr_header = print_attr_header;
   p->emit_attr_footer = print_attr_footer;
   p->emit_mov = print_mov;
   p->emit_const = print_const;
   p->emit_mad = print_mad;
   p->emit_float_to_chan = print_float_to_chan;
   p->emit_const_chan = print_const_chan;
   p->emit_float_to_ubyte = print_float_to_ubyte;
   p->emit_const_ubyte = print_const_ubyte;
   p->emit_store_func = print_store_func;
   
   make_empty_list(&p->codegen_list);

   p->buf_size = 2048;
   p->buf = MALLOC(p->buf_size);
}

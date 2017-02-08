/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/condrender.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/enums.h"

#include "t_context.h"
#include "tnl.h"



static GLubyte *get_space(struct gl_context *ctx, GLuint bytes)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLubyte *space = malloc(bytes);
   
   tnl->block[tnl->nr_blocks++] = space;
   return space;
}


static void free_space(struct gl_context *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < tnl->nr_blocks; i++)
      free(tnl->block[i]);
   tnl->nr_blocks = 0;
}


/* Convert the incoming array to GLfloats.  Understands the
 * array->Normalized flag and selects the correct conversion method.
 */
#define CONVERT( TYPE, MACRO ) do {		\
   GLuint i, j;					\
   if (input->Normalized) {			\
      for (i = 0; i < count; i++) {		\
	 const TYPE *in = (TYPE *)ptr;		\
	 for (j = 0; j < sz; j++) {		\
	    *fptr++ = MACRO(*in);		\
	    in++;				\
	 }					\
	 ptr += input->StrideB;			\
      }						\
   } else {					\
      for (i = 0; i < count; i++) {		\
	 const TYPE *in = (TYPE *)ptr;		\
	 for (j = 0; j < sz; j++) {		\
	    *fptr++ = (GLfloat)(*in);		\
	    in++;				\
	 }					\
	 ptr += input->StrideB;			\
      }						\
   }						\
} while (0)


/**
 * Convert array of BGRA/GLubyte[4] values to RGBA/float[4]
 * \param ptr  input/ubyte array
 * \param fptr  output/float array
 */
static void
convert_bgra_to_float(const struct gl_client_array *input,
                      const GLubyte *ptr, GLfloat *fptr,
                      GLuint count )
{
   GLuint i;
   assert(input->Normalized);
   assert(input->Size == 4);
   for (i = 0; i < count; i++) {
      const GLubyte *in = (GLubyte *) ptr;  /* in is in BGRA order */
      *fptr++ = UBYTE_TO_FLOAT(in[2]);  /* red */
      *fptr++ = UBYTE_TO_FLOAT(in[1]);  /* green */
      *fptr++ = UBYTE_TO_FLOAT(in[0]);  /* blue */
      *fptr++ = UBYTE_TO_FLOAT(in[3]);  /* alpha */
      ptr += input->StrideB;
   }
}

static void
convert_half_to_float(const struct gl_client_array *input,
		      const GLubyte *ptr, GLfloat *fptr,
		      GLuint count, GLuint sz)
{
   GLuint i, j;

   for (i = 0; i < count; i++) {
      GLhalfARB *in = (GLhalfARB *)ptr;

      for (j = 0; j < sz; j++) {
	 *fptr++ = _mesa_half_to_float(in[j]);
      }
      ptr += input->StrideB;
   }
}

/**
 * \brief Convert fixed-point to floating-point.
 *
 * In OpenGL, a fixed-point number is a "signed 2's complement 16.16 scaled
 * integer" (Table 2.2 of the OpenGL ES 2.0 spec).
 *
 * If the buffer has the \c normalized flag set, the formula
 *     \code normalize(x) := (2*x + 1) / (2^16 - 1) \endcode
 * is used to map the fixed-point numbers into the range [-1, 1].
 */
static void
convert_fixed_to_float(const struct gl_client_array *input,
                       const GLubyte *ptr, GLfloat *fptr,
                       GLuint count)
{
   GLuint i, j;
   const GLint size = input->Size;

   if (input->Normalized) {
      for (i = 0; i < count; ++i) {
         const GLfixed *in = (GLfixed *) ptr;
         for (j = 0; j < size; ++j) {
            *fptr++ = (GLfloat) (2 * in[j] + 1) / (GLfloat) ((1 << 16) - 1);
         }
         ptr += input->StrideB;
      }
   } else {
      for (i = 0; i < count; ++i) {
         const GLfixed *in = (GLfixed *) ptr;
         for (j = 0; j < size; ++j) {
            *fptr++ = in[j] / (GLfloat) (1 << 16);
         }
         ptr += input->StrideB;
      }
   }
}

/* Adjust pointer to point at first requested element, convert to
 * floating point, populate VB->AttribPtr[].
 */
static void _tnl_import_array( struct gl_context *ctx,
			       GLuint attrib,
			       GLuint count,
			       const struct gl_client_array *input,
			       const GLubyte *ptr )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint stride = input->StrideB;

   if (input->Type != GL_FLOAT) {
      const GLuint sz = input->Size;
      GLubyte *buf = get_space(ctx, count * sz * sizeof(GLfloat));
      GLfloat *fptr = (GLfloat *)buf;

      switch (input->Type) {
      case GL_BYTE: 
	 CONVERT(GLbyte, BYTE_TO_FLOAT); 
	 break;
      case GL_UNSIGNED_BYTE: 
         if (input->Format == GL_BGRA) {
            /* See GL_EXT_vertex_array_bgra */
            convert_bgra_to_float(input, ptr, fptr, count);
         }
         else {
            CONVERT(GLubyte, UBYTE_TO_FLOAT); 
         }
	 break;
      case GL_SHORT: 
	 CONVERT(GLshort, SHORT_TO_FLOAT); 
	 break;
      case GL_UNSIGNED_SHORT: 
	 CONVERT(GLushort, USHORT_TO_FLOAT); 
	 break;
      case GL_INT: 
	 CONVERT(GLint, INT_TO_FLOAT); 
	 break;
      case GL_UNSIGNED_INT: 
	 CONVERT(GLuint, UINT_TO_FLOAT); 
	 break;
      case GL_DOUBLE: 
	 CONVERT(GLdouble, (GLfloat)); 
	 break;
      case GL_HALF_FLOAT:
	 convert_half_to_float(input, ptr, fptr, count, sz);
	 break;
      case GL_FIXED:
         convert_fixed_to_float(input, ptr, fptr, count);
         break;
      default:
	 assert(0);
	 break;
      }

      ptr = buf;
      stride = sz * sizeof(GLfloat);
   }

   VB->AttribPtr[attrib] = &tnl->tmp_inputs[attrib];
   VB->AttribPtr[attrib]->data = (GLfloat (*)[4])ptr;
   VB->AttribPtr[attrib]->start = (GLfloat *)ptr;
   VB->AttribPtr[attrib]->count = count;
   VB->AttribPtr[attrib]->stride = stride;
   VB->AttribPtr[attrib]->size = input->Size;

   /* This should die, but so should the whole GLvector4f concept: 
    */
   VB->AttribPtr[attrib]->flags = (((1<<input->Size)-1) | 
				   VEC_NOT_WRITEABLE |
				   (stride == 4*sizeof(GLfloat) ? 0 : VEC_BAD_STRIDE));
   
   VB->AttribPtr[attrib]->storage = NULL;
}

#define CLIPVERTS  ((6 + MAX_CLIP_PLANES) * 2)


static GLboolean *_tnl_import_edgeflag( struct gl_context *ctx,
					const GLvector4f *input,
					GLuint count)
{
   const GLubyte *ptr = (const GLubyte *)input->data;
   const GLuint stride = input->stride;
   GLboolean *space = (GLboolean *)get_space(ctx, count + CLIPVERTS);
   GLboolean *bptr = space;
   GLuint i;

   for (i = 0; i < count; i++) {
      *bptr++ = ((GLfloat *)ptr)[0] == 1.0;
      ptr += stride;
   }

   return space;
}


static void bind_inputs( struct gl_context *ctx, 
			 const struct gl_client_array *inputs[],
			 GLint count,
			 struct gl_buffer_object **bo,
			 GLuint *nr_bo )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   /* Map all the VBOs
    */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      const void *ptr;

      if (inputs[i]->BufferObj->Name) { 
	 if (!inputs[i]->BufferObj->Pointer) {
	    bo[*nr_bo] = inputs[i]->BufferObj;
	    (*nr_bo)++;
	    ctx->Driver.MapBufferRange(ctx, 0, inputs[i]->BufferObj->Size,
				       GL_MAP_READ_BIT,
				       inputs[i]->BufferObj);
	    
	    assert(inputs[i]->BufferObj->Pointer);
	 }
	 
	 ptr = ADD_POINTERS(inputs[i]->BufferObj->Pointer,
			    inputs[i]->Ptr);
      }
      else
	 ptr = inputs[i]->Ptr;

      /* Just make sure the array is floating point, otherwise convert to
       * temporary storage.  
       *
       * XXX: remove the GLvector4f type at some stage and just use
       * client arrays.
       */
      _tnl_import_array(ctx, i, count, inputs[i], ptr);
   }

   /* We process only the vertices between min & max index:
    */
   VB->Count = count;

   /* These should perhaps be part of _TNL_ATTRIB_* */
   VB->BackfaceColorPtr = NULL;
   VB->BackfaceIndexPtr = NULL;
   VB->BackfaceSecondaryColorPtr = NULL;

   /* Clipping and drawing code still requires this to be a packed
    * array of ubytes which can be written into.  TODO: Fix and
    * remove.
    */
   if (ctx->Polygon.FrontMode != GL_FILL ||
       ctx->Polygon.BackMode != GL_FILL)
   {
      VB->EdgeFlag = _tnl_import_edgeflag( ctx, 
					   VB->AttribPtr[_TNL_ATTRIB_EDGEFLAG],
					   VB->Count );
   }
   else {
      /* the data previously pointed to by EdgeFlag may have been freed */
      VB->EdgeFlag = NULL;
   }
}


/* Translate indices to GLuints and store in VB->Elts.
 */
static void bind_indices( struct gl_context *ctx,
			  const struct _mesa_index_buffer *ib,
			  struct gl_buffer_object **bo,
			  GLuint *nr_bo)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   const void *ptr;

   if (!ib) {
      VB->Elts = NULL;
      return;
   }

   if (_mesa_is_bufferobj(ib->obj) && !_mesa_bufferobj_mapped(ib->obj)) {
      /* if the buffer object isn't mapped yet, map it now */
      bo[*nr_bo] = ib->obj;
      (*nr_bo)++;
      ptr = ctx->Driver.MapBufferRange(ctx, (GLsizeiptr) ib->ptr,
                                       ib->count * vbo_sizeof_ib_type(ib->type),
				       GL_MAP_READ_BIT, ib->obj);
      assert(ib->obj->Pointer);
   } else {
      /* user-space elements, or buffer already mapped */
      ptr = ADD_POINTERS(ib->obj->Pointer, ib->ptr);
   }

   if (ib->type == GL_UNSIGNED_INT && VB->Primitive[0].basevertex == 0) {
      VB->Elts = (GLuint *) ptr;
   }
   else {
      GLuint *elts = (GLuint *)get_space(ctx, ib->count * sizeof(GLuint));
      VB->Elts = elts;

      if (ib->type == GL_UNSIGNED_INT) {
	 const GLuint *in = (GLuint *)ptr;
	 for (i = 0; i < ib->count; i++)
	    *elts++ = (GLuint)(*in++) + VB->Primitive[0].basevertex;
      }
      else if (ib->type == GL_UNSIGNED_SHORT) {
	 const GLushort *in = (GLushort *)ptr;
	 for (i = 0; i < ib->count; i++) 
	    *elts++ = (GLuint)(*in++) + VB->Primitive[0].basevertex;
      }
      else {
	 const GLubyte *in = (GLubyte *)ptr;
	 for (i = 0; i < ib->count; i++) 
	    *elts++ = (GLuint)(*in++) + VB->Primitive[0].basevertex;
      }
   }
}

static void bind_prims( struct gl_context *ctx,
			const struct _mesa_prim *prim,
			GLuint nr_prims )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   VB->Primitive = prim;
   VB->PrimitiveCount = nr_prims;
}

static void unmap_vbos( struct gl_context *ctx,
			struct gl_buffer_object **bo,
			GLuint nr_bo )
{
   GLuint i;
   for (i = 0; i < nr_bo; i++) { 
      ctx->Driver.UnmapBuffer(ctx, bo[i]);
   }
}


void _tnl_vbo_draw_prims(struct gl_context *ctx,
			 const struct gl_client_array *arrays[],
			 const struct _mesa_prim *prim,
			 GLuint nr_prims,
			 const struct _mesa_index_buffer *ib,
			 GLboolean index_bounds_valid,
			 GLuint min_index,
			 GLuint max_index,
			 struct gl_transform_feedback_object *tfb_vertcount)
{
   if (!index_bounds_valid)
      vbo_get_minmax_index(ctx, prim, ib, &min_index, &max_index);

   _tnl_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
}

/* This is the main entrypoint into the slimmed-down software tnl
 * module.  In a regular swtnl driver, this can be plugged straight
 * into the vbo->Driver.DrawPrims() callback.
 */
void _tnl_draw_prims( struct gl_context *ctx,
		      const struct gl_client_array *arrays[],
		      const struct _mesa_prim *prim,
		      GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLuint min_index,
		      GLuint max_index)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   const GLuint TEST_SPLIT = 0;
   const GLint max = TEST_SPLIT ? 8 : tnl->vb.Size - MAX_CLIPPED_VERTICES;
   GLint max_basevertex = prim->basevertex;
   GLuint i;

   /* Mesa core state should have been validated already */
   assert(ctx->NewState == 0x0);

   if (!_mesa_check_conditional_render(ctx))
      return; /* don't draw */

   for (i = 1; i < nr_prims; i++)
      max_basevertex = MAX2(max_basevertex, prim[i].basevertex);

   if (0)
   {
      printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);
      for (i = 0; i < nr_prims; i++)
	 printf("prim %d: %s start %d count %d\n", i, 
		_mesa_lookup_enum_by_nr(prim[i].mode),
		prim[i].start,
		prim[i].count);
   }

   if (min_index) {
      /* We always translate away calls with min_index != 0. 
       */
      vbo_rebase_prims( ctx, arrays, prim, nr_prims, ib, 
			min_index, max_index,
			_tnl_vbo_draw_prims );
      return;
   }
   else if ((GLint)max_index + max_basevertex > max) {
      /* The software TNL pipeline has a fixed amount of storage for
       * vertices and it is necessary to split incoming drawing commands
       * if they exceed that limit.
       */
      struct split_limits limits;
      limits.max_verts = max;
      limits.max_vb_size = ~0;
      limits.max_indices = ~0;

      /* This will split the buffers one way or another and
       * recursively call back into this function.
       */
      vbo_split_prims( ctx, arrays, prim, nr_prims, ib, 
		       0, max_index + prim->basevertex,
		       _tnl_vbo_draw_prims,
		       &limits );
   }
   else {
      /* May need to map a vertex buffer object for every attribute plus
       * one for the index buffer.
       */
      struct gl_buffer_object *bo[VERT_ATTRIB_MAX + 1];
      GLuint nr_bo = 0;
      GLuint inst;

      for (i = 0; i < nr_prims;) {
	 GLuint this_nr_prims;

	 /* Our SW TNL pipeline doesn't handle basevertex yet, so bind_indices
	  * will rebase the elements to the basevertex, and we'll only
	  * emit strings of prims with the same basevertex in one draw call.
	  */
	 for (this_nr_prims = 1; i + this_nr_prims < nr_prims;
	      this_nr_prims++) {
	    if (prim[i].basevertex != prim[i + this_nr_prims].basevertex)
	       break;
	 }

         assert(prim[i].num_instances > 0);

	 /* Binding inputs may imply mapping some vertex buffer objects.
	  * They will need to be unmapped below.
	  */
         for (inst = 0; inst < prim[i].num_instances; inst++) {

            bind_prims(ctx, &prim[i], this_nr_prims);
            bind_inputs(ctx, arrays, max_index + prim[i].basevertex + 1,
                        bo, &nr_bo);
            bind_indices(ctx, ib, bo, &nr_bo);

            tnl->CurInstance = inst;
            TNL_CONTEXT(ctx)->Driver.RunPipeline(ctx);

            unmap_vbos(ctx, bo, nr_bo);
            free_space(ctx);
         }

	 i += this_nr_prims;
      }
   }
}


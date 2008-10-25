/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <stdlib.h>

#include "glheader.h"
#include "context.h"
#include "state.h"
#include "api_validate.h"
#include "enums.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_aub.h"
#include "brw_state.h"
#include "brw_fallback.h"

#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"


struct brw_array_state {
   union header_union header;

   struct {
      union {
	 struct {
	    GLuint pitch:11; 
	    GLuint pad:15;
	    GLuint access_type:1; 
	    GLuint vb_index:5; 
	 } bits;
	 GLuint dword;
      } vb0;
   
      struct buffer *buffer;
      GLuint offset;

      GLuint max_index;   
      GLuint instance_data_step_rate;

   } vb[BRW_VBP_MAX];
};


static struct buffer *array_buffer( const struct gl_client_array *array )
{
   return intel_bufferobj_buffer(intel_buffer_object(array->BufferObj));
}

static GLuint double_types[5] = {
   0,
   BRW_SURFACEFORMAT_R64_FLOAT,
   BRW_SURFACEFORMAT_R64G64_FLOAT,
   BRW_SURFACEFORMAT_R64G64B64_FLOAT,
   BRW_SURFACEFORMAT_R64G64B64A64_FLOAT
};

static GLuint float_types[5] = {
   0,
   BRW_SURFACEFORMAT_R32_FLOAT,
   BRW_SURFACEFORMAT_R32G32_FLOAT,
   BRW_SURFACEFORMAT_R32G32B32_FLOAT,
   BRW_SURFACEFORMAT_R32G32B32A32_FLOAT
};

static GLuint uint_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R32_UNORM,
   BRW_SURFACEFORMAT_R32G32_UNORM,
   BRW_SURFACEFORMAT_R32G32B32_UNORM,
   BRW_SURFACEFORMAT_R32G32B32A32_UNORM
};

static GLuint uint_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R32_USCALED,
   BRW_SURFACEFORMAT_R32G32_USCALED,
   BRW_SURFACEFORMAT_R32G32B32_USCALED,
   BRW_SURFACEFORMAT_R32G32B32A32_USCALED
};

static GLuint int_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SNORM,
   BRW_SURFACEFORMAT_R32G32_SNORM,
   BRW_SURFACEFORMAT_R32G32B32_SNORM,
   BRW_SURFACEFORMAT_R32G32B32A32_SNORM
};

static GLuint int_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SSCALED,
   BRW_SURFACEFORMAT_R32G32_SSCALED,
   BRW_SURFACEFORMAT_R32G32B32_SSCALED,
   BRW_SURFACEFORMAT_R32G32B32A32_SSCALED
};

static GLuint ushort_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R16_UNORM,
   BRW_SURFACEFORMAT_R16G16_UNORM,
   BRW_SURFACEFORMAT_R16G16B16_UNORM,
   BRW_SURFACEFORMAT_R16G16B16A16_UNORM
};

static GLuint ushort_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R16_USCALED,
   BRW_SURFACEFORMAT_R16G16_USCALED,
   BRW_SURFACEFORMAT_R16G16B16_USCALED,
   BRW_SURFACEFORMAT_R16G16B16A16_USCALED
};

static GLuint short_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SNORM,
   BRW_SURFACEFORMAT_R16G16_SNORM,
   BRW_SURFACEFORMAT_R16G16B16_SNORM,
   BRW_SURFACEFORMAT_R16G16B16A16_SNORM
};

static GLuint short_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SSCALED,
   BRW_SURFACEFORMAT_R16G16_SSCALED,
   BRW_SURFACEFORMAT_R16G16B16_SSCALED,
   BRW_SURFACEFORMAT_R16G16B16A16_SSCALED
};

static GLuint ubyte_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R8_UNORM,
   BRW_SURFACEFORMAT_R8G8_UNORM,
   BRW_SURFACEFORMAT_R8G8B8_UNORM,
   BRW_SURFACEFORMAT_R8G8B8A8_UNORM
};

static GLuint ubyte_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R8_USCALED,
   BRW_SURFACEFORMAT_R8G8_USCALED,
   BRW_SURFACEFORMAT_R8G8B8_USCALED,
   BRW_SURFACEFORMAT_R8G8B8A8_USCALED
};

static GLuint byte_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SNORM,
   BRW_SURFACEFORMAT_R8G8_SNORM,
   BRW_SURFACEFORMAT_R8G8B8_SNORM,
   BRW_SURFACEFORMAT_R8G8B8A8_SNORM
};

static GLuint byte_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SSCALED,
   BRW_SURFACEFORMAT_R8G8_SSCALED,
   BRW_SURFACEFORMAT_R8G8B8_SSCALED,
   BRW_SURFACEFORMAT_R8G8B8A8_SSCALED
};


static GLuint get_surface_type( GLenum type, GLuint size, GLboolean normalized )
{
   if (INTEL_DEBUG & DEBUG_VERTS)
      _mesa_printf("type %s size %d normalized %d\n", 
		   _mesa_lookup_enum_by_nr(type), size, normalized);

   if (normalized) {
      switch (type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_INT: return int_types_norm[size];
      case GL_SHORT: return short_types_norm[size];
      case GL_BYTE: return byte_types_norm[size];
      case GL_UNSIGNED_INT: return uint_types_norm[size];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_norm[size];
      default: assert(0); return 0;
      }      
   }
   else {
      switch (type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_INT: return int_types_scale[size];
      case GL_SHORT: return short_types_scale[size];
      case GL_BYTE: return byte_types_scale[size];
      case GL_UNSIGNED_INT: return uint_types_scale[size];
      case GL_UNSIGNED_SHORT: return ushort_types_scale[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_scale[size];
      default: assert(0); return 0;
      }      
   }
}


static GLuint get_size( GLenum type )
{
   switch (type) {
   case GL_DOUBLE: return sizeof(GLdouble);
   case GL_FLOAT: return sizeof(GLfloat);
   case GL_INT: return sizeof(GLint);
   case GL_SHORT: return sizeof(GLshort);
   case GL_BYTE: return sizeof(GLbyte);
   case GL_UNSIGNED_INT: return sizeof(GLuint);
   case GL_UNSIGNED_SHORT: return sizeof(GLushort);
   case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
   default: return 0;
   }      
}

static GLuint get_index_type(GLenum type) 
{
   switch (type) {
   case GL_UNSIGNED_BYTE:  return BRW_INDEX_BYTE;
   case GL_UNSIGNED_SHORT: return BRW_INDEX_WORD;
   case GL_UNSIGNED_INT:   return BRW_INDEX_DWORD;
   default: assert(0); return 0;
   }
}

static void copy_strided_array( GLubyte *dest, 
				const GLubyte *src, 
				GLuint size, 
				GLuint stride,
				GLuint count )
{
   if (size == stride) 
      do_memcpy(dest, src, count * size);
   else {
      GLuint i,j;
   
      for (i = 0; i < count; i++) {
	 for (j = 0; j < size; j++)
	    *dest++ = *src++;
	 src += (stride - size);
      }
   }
}

static void wrap_buffers( struct brw_context *brw,
			  GLuint size )
{
   GLcontext *ctx = &brw->intel.ctx;

   if (size < BRW_UPLOAD_INIT_SIZE)
      size = BRW_UPLOAD_INIT_SIZE;

   brw->vb.upload.buf++;
   brw->vb.upload.buf %= BRW_NR_UPLOAD_BUFS;
   brw->vb.upload.offset = 0;

   ctx->Driver.BufferData(ctx,
			  GL_ARRAY_BUFFER_ARB,
			  size,
			  NULL,
			  GL_DYNAMIC_DRAW_ARB,
			  brw->vb.upload.vbo[brw->vb.upload.buf]);
}

static void get_space( struct brw_context *brw,
		       GLuint size,
		       struct gl_buffer_object **vbo_return,
		       GLuint *offset_return )
{
   size = (size + 63) & ~63;
   
   if (brw->vb.upload.offset + size > BRW_UPLOAD_INIT_SIZE)
      wrap_buffers(brw, size);

   *vbo_return = brw->vb.upload.vbo[brw->vb.upload.buf];
   *offset_return = brw->vb.upload.offset;

   brw->vb.upload.offset += size;
}



static struct gl_client_array *
copy_array_to_vbo_array( struct brw_context *brw,
			 GLuint i,
			 const struct gl_client_array *array,
			 GLuint element_size,
			 GLuint count)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct gl_client_array *vbo_array = &brw->vb.vbo_array[i];
   GLuint size = count * element_size;
   struct gl_buffer_object *vbo;
   GLuint offset;
   GLuint new_stride;

   get_space(brw, size, &vbo, &offset);

   if (array->StrideB == 0) {
      assert(count == 1);
      new_stride = 0;
   }
   else 
      new_stride = element_size;

   vbo_array->Size = array->Size;
   vbo_array->Type = array->Type;
   vbo_array->Stride = new_stride;
   vbo_array->StrideB = new_stride;   
   vbo_array->Ptr = (const void *)offset;
   vbo_array->Enabled = 1;
   vbo_array->Normalized = array->Normalized;
   vbo_array->_MaxElement = array->_MaxElement;	/* ? */
   vbo_array->BufferObj = vbo;

   {
      GLubyte *map = ctx->Driver.MapBuffer(ctx,
					   GL_ARRAY_BUFFER_ARB,
					   GL_DYNAMIC_DRAW_ARB,
					   vbo);
   
      map += offset;

      copy_strided_array( map, 
			  array->Ptr,
			  element_size,
			  array->StrideB,
			  count);

      ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER_ARB, vbo_array->BufferObj);
   }

   return vbo_array;
}



static struct gl_client_array *
interleaved_vbo_array( struct brw_context *brw,
		       GLuint i,
		       const struct gl_client_array *uploaded_array,
		       const struct gl_client_array *array,
		       const char *ptr)
{
   struct gl_client_array *vbo_array = &brw->vb.vbo_array[i];

   vbo_array->Size = array->Size;
   vbo_array->Type = array->Type;
   vbo_array->Stride = array->Stride;
   vbo_array->StrideB = array->StrideB;   
   vbo_array->Ptr = (const void *)((const char *)uploaded_array->Ptr + 
				   ((const char *)array->Ptr - ptr));
   vbo_array->Enabled = 1;
   vbo_array->Normalized = array->Normalized;
   vbo_array->_MaxElement = array->_MaxElement;	
   vbo_array->BufferObj = uploaded_array->BufferObj;

   return vbo_array;
}


GLboolean brw_upload_vertices( struct brw_context *brw,
			       GLuint min_index,
			       GLuint max_index )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   GLuint tmp = brw->vs.prog_data->inputs_read; 
   struct brw_vertex_element_packet vep;
   struct brw_array_state vbp;
   GLuint i;
   const void *ptr = NULL;
   GLuint interleave = 0;

   struct brw_vertex_element *enabled[VERT_ATTRIB_MAX];
   GLuint nr_enabled = 0;

   struct brw_vertex_element *upload[VERT_ATTRIB_MAX];
   GLuint nr_uploads = 0;
   

   memset(&vbp, 0, sizeof(vbp));
   memset(&vep, 0, sizeof(vep));

   /* First build an array of pointers to ve's in vb.inputs_read
    */
   if (0)
      _mesa_printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);
   
   while (tmp) {
      GLuint i = _mesa_ffsll(tmp)-1;
      struct brw_vertex_element *input = &brw->vb.inputs[i];

      tmp &= ~(1<<i);
      enabled[nr_enabled++] = input;

      input->index = i;
      input->element_size = get_size(input->glarray->Type) * input->glarray->Size;
      input->count = input->glarray->StrideB ? max_index + 1 - min_index : 1;

      if (!input->glarray->BufferObj->Name) {
	 if (i == 0) {
	    /* Position array not properly enabled:
	     */
	    if (input->glarray->StrideB == 0)
	       return GL_FALSE;

	    interleave = input->glarray->StrideB;
	    ptr = input->glarray->Ptr;
	 }
	 else if (interleave != input->glarray->StrideB ||
		  (const char *)input->glarray->Ptr - (const char *)ptr < 0 ||
		  (const char *)input->glarray->Ptr - (const char *)ptr > interleave) {
	    interleave = 0;
	 }

	 upload[nr_uploads++] = input;
	 
	 /* We rebase drawing to start at element zero only when
	  * varyings are not in vbos, which means we can end up
	  * uploading non-varying arrays (stride != 0) when min_index
	  * is zero.  This doesn't matter as the amount to upload is
	  * the same for these arrays whether the draw call is rebased
	  * or not - we just have to upload the one element.
	  */
	 assert(min_index == 0 || input->glarray->StrideB == 0);
      }
   }

   /* Upload interleaved arrays if all uploads are interleaved
    */
   if (nr_uploads > 1 && 
       interleave && 
       interleave <= 256) {
      struct brw_vertex_element *input0 = upload[0];

      input0->glarray = copy_array_to_vbo_array(brw, 0,
						input0->glarray, 
						interleave,
						input0->count);

      for (i = 1; i < nr_uploads; i++) {
	 upload[i]->glarray = interleaved_vbo_array(brw,
						    i,
						    input0->glarray,
						    upload[i]->glarray,
						    ptr);
      }
   }
   else {
      for (i = 0; i < nr_uploads; i++) {
	 struct brw_vertex_element *input = upload[i];

	 input->glarray = copy_array_to_vbo_array(brw, i, 
						  input->glarray,
						  input->element_size,
						  input->count);

      }
   }

   /* XXX: In the rare cases where this happens we fallback all
    * the way to software rasterization, although a tnl fallback
    * would be sufficient.  I don't know of *any* real world
    * cases with > 17 vertex attributes enabled, so it probably
    * isn't an issue at this point.
    */
   if (nr_enabled >= BRW_VEP_MAX)
	 return GL_FALSE;

   /* This still defines a hardware VB for each input, even if they
    * are interleaved or from the same VBO.  TBD if this makes a
    * performance difference.
    */
   for (i = 0; i < nr_enabled; i++) {
      struct brw_vertex_element *input = enabled[i];

      input->vep = &vep.ve[i];
      input->vep->ve0.src_format = get_surface_type(input->glarray->Type, 
						    input->glarray->Size,
						    input->glarray->Normalized);
      input->vep->ve0.valid = 1;
      input->vep->ve1.dst_offset = (i) * 4;
      input->vep->ve1.vfcomponent3 = BRW_VFCOMPONENT_STORE_SRC;
      input->vep->ve1.vfcomponent2 = BRW_VFCOMPONENT_STORE_SRC;
      input->vep->ve1.vfcomponent1 = BRW_VFCOMPONENT_STORE_SRC;
      input->vep->ve1.vfcomponent0 = BRW_VFCOMPONENT_STORE_SRC;

      switch (input->glarray->Size) {
      case 0: input->vep->ve1.vfcomponent0 = BRW_VFCOMPONENT_STORE_0;
      case 1: input->vep->ve1.vfcomponent1 = BRW_VFCOMPONENT_STORE_0;
      case 2: input->vep->ve1.vfcomponent2 = BRW_VFCOMPONENT_STORE_0;
      case 3: input->vep->ve1.vfcomponent3 = BRW_VFCOMPONENT_STORE_1_FLT;
	 break;
      }

      input->vep->ve0.vertex_buffer_index = i;
      input->vep->ve0.src_offset = 0;

      vbp.vb[i].vb0.bits.pitch = input->glarray->StrideB;
      vbp.vb[i].vb0.bits.pad = 0;
      vbp.vb[i].vb0.bits.access_type = BRW_VERTEXBUFFER_ACCESS_VERTEXDATA;
      vbp.vb[i].vb0.bits.vb_index = i;
      vbp.vb[i].offset = (GLuint)input->glarray->Ptr;
      vbp.vb[i].buffer = array_buffer(input->glarray);
      vbp.vb[i].max_index = max_index;
   }



   /* Now emit VB and VEP state packets:
    */
   vbp.header.bits.length = (1 + nr_enabled * 4) - 2;
   vbp.header.bits.opcode = CMD_VERTEX_BUFFER;

   BEGIN_BATCH(vbp.header.bits.length+2, 0);
   OUT_BATCH( vbp.header.dword );
   
   for (i = 0; i < nr_enabled; i++) {
      OUT_BATCH( vbp.vb[i].vb0.dword );
      OUT_BATCH( bmBufferOffset(&brw->intel, vbp.vb[i].buffer) + vbp.vb[i].offset);
      OUT_BATCH( vbp.vb[i].max_index );
      OUT_BATCH( vbp.vb[i].instance_data_step_rate );
   }
   ADVANCE_BATCH();

   vep.header.length = (1 + nr_enabled * sizeof(vep.ve[0])/4) - 2;
   vep.header.opcode = CMD_VERTEX_ELEMENT;
   brw_cached_batch_struct(brw, &vep, 4 + nr_enabled * sizeof(vep.ve[0]));

   return GL_TRUE;
}


static GLuint element_size( GLenum type )
{
   switch(type) {
   case GL_UNSIGNED_INT: return 4;
   case GL_UNSIGNED_SHORT: return 2;
   case GL_UNSIGNED_BYTE: return 1;
   default: assert(0); return 0;
   }
}



void brw_upload_indices( struct brw_context *brw,
			 const struct _mesa_index_buffer *index_buffer )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   GLuint ib_size = get_size(index_buffer->type) * index_buffer->count;
   struct gl_buffer_object *bufferobj = index_buffer->obj;
   GLuint offset = (GLuint)index_buffer->ptr;

   /* Turn into a proper VBO:
    */
   if (!bufferobj->Name) {
     
      /* Get new bufferobj, offset:
       */
      get_space(brw, ib_size, &bufferobj, &offset);

      /* Straight upload
       */
      ctx->Driver.BufferSubData( ctx,
				 GL_ELEMENT_ARRAY_BUFFER_ARB,
				 offset, 
				 ib_size,
				 index_buffer->ptr,
				 bufferobj);
   }

   /* Emit the indexbuffer packet:
    */
   {
      struct brw_indexbuffer ib;
      struct buffer *buffer = intel_bufferobj_buffer(intel_buffer_object(bufferobj));

      memset(&ib, 0, sizeof(ib));
   
      ib.header.bits.opcode = CMD_INDEX_BUFFER;
      ib.header.bits.length = sizeof(ib)/4 - 2;
      ib.header.bits.index_format = get_index_type(index_buffer->type);
      ib.header.bits.cut_index_enable = 0;
   

      BEGIN_BATCH(4, 0);
      OUT_BATCH( ib.header.dword );
      OUT_BATCH( bmBufferOffset(intel, buffer) + offset );
      OUT_BATCH( bmBufferOffset(intel, buffer) + offset + ib_size );
      OUT_BATCH( 0 );
      ADVANCE_BATCH();
   }
}

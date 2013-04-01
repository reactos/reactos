/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 */


#include "glheader.h"
#include "imports.h"
#include "bufferobj.h"
#include "context.h"
#include "enable.h"
#include "enums.h"
#include "hash.h"
#include "image.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "varray.h"
#include "arrayobj.h"
#include "main/dispatch.h"


/** Used to do error checking for GL_EXT_vertex_array_bgra */
#define BGRA_OR_4  5


/** Used to indicate which GL datatypes are accepted by each of the
 * glVertex/Color/Attrib/EtcPointer() functions.
 */
#define BOOL_BIT             0x1
#define BYTE_BIT             0x2
#define UNSIGNED_BYTE_BIT    0x4
#define SHORT_BIT            0x8
#define UNSIGNED_SHORT_BIT   0x10
#define INT_BIT              0x20
#define UNSIGNED_INT_BIT     0x40
#define HALF_BIT             0x80
#define FLOAT_BIT            0x100
#define DOUBLE_BIT           0x200
#define FIXED_ES_BIT         0x400
#define FIXED_GL_BIT         0x800
#define UNSIGNED_INT_2_10_10_10_REV_BIT 0x1000
#define INT_2_10_10_10_REV_BIT 0x2000


/** Convert GL datatype enum into a <type>_BIT value seen above */
static GLbitfield
type_to_bit(const struct gl_context *ctx, GLenum type)
{
   switch (type) {
   case GL_BOOL:
      return BOOL_BIT;
   case GL_BYTE:
      return BYTE_BIT;
   case GL_UNSIGNED_BYTE:
      return UNSIGNED_BYTE_BIT;
   case GL_SHORT:
      return SHORT_BIT;
   case GL_UNSIGNED_SHORT:
      return UNSIGNED_SHORT_BIT;
   case GL_INT:
      return INT_BIT;
   case GL_UNSIGNED_INT:
      return UNSIGNED_INT_BIT;
   case GL_HALF_FLOAT:
      if (ctx->Extensions.ARB_half_float_vertex)
         return HALF_BIT;
      else
         return 0x0;
   case GL_FLOAT:
      return FLOAT_BIT;
   case GL_DOUBLE:
      return DOUBLE_BIT;
   case GL_FIXED:
      return ctx->API == API_OPENGL ? FIXED_GL_BIT : FIXED_ES_BIT;
   case GL_UNSIGNED_INT_2_10_10_10_REV:
      return UNSIGNED_INT_2_10_10_10_REV_BIT;
   case GL_INT_2_10_10_10_REV:
      return INT_2_10_10_10_REV_BIT;
   default:
      return 0;
   }
}


/**
 * Do error checking and update state for glVertex/Color/TexCoord/...Pointer
 * functions.
 *
 * \param func  name of calling function used for error reporting
 * \param attrib  the attribute array index to update
 * \param legalTypes  bitmask of *_BIT above indicating legal datatypes
 * \param sizeMin  min allowable size value
 * \param sizeMax  max allowable size value (may also be BGRA_OR_4)
 * \param size  components per element (1, 2, 3 or 4)
 * \param type  datatype of each component (GL_FLOAT, GL_INT, etc)
 * \param stride  stride between elements, in elements
 * \param normalized  are integer types converted to floats in [-1, 1]?
 * \param integer  integer-valued values (will not be normalized to [-1,1])
 * \param ptr  the address (or offset inside VBO) of the array data
 */
static void
update_array(struct gl_context *ctx,
             const char *func,
             GLuint attrib, GLbitfield legalTypesMask,
             GLint sizeMin, GLint sizeMax,
             GLint size, GLenum type, GLsizei stride,
             GLboolean normalized, GLboolean integer,
             const GLvoid *ptr)
{
   struct gl_client_array *array;
   GLbitfield typeBit;
   GLsizei elementSize;
   GLenum format = GL_RGBA;

   if (ctx->API != API_OPENGLES && ctx->API != API_OPENGLES2) {
      /* fixed point arrays / data is only allowed with OpenGL ES 1.x/2.0 */
      legalTypesMask &= ~FIXED_ES_BIT;
   }
   if (!ctx->Extensions.ARB_ES2_compatibility) {
      legalTypesMask &= ~FIXED_GL_BIT;
   }
   if (!ctx->Extensions.ARB_vertex_type_2_10_10_10_rev) {
      legalTypesMask &= ~(UNSIGNED_INT_2_10_10_10_REV_BIT |
                          INT_2_10_10_10_REV_BIT);
   }

   typeBit = type_to_bit(ctx, type);
   if (typeBit == 0x0 || (typeBit & legalTypesMask) == 0x0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(type = %s)",
                  func, _mesa_lookup_enum_by_nr(type));
      return;
   }

   /* Do size parameter checking.
    * If sizeMax = BGRA_OR_4 it means that size = GL_BGRA is legal and
    * must be handled specially.
    */
   if (ctx->Extensions.EXT_vertex_array_bgra &&
       sizeMax == BGRA_OR_4 &&
       size == GL_BGRA) {
      GLboolean bgra_error = GL_FALSE;

      if (ctx->Extensions.ARB_vertex_type_2_10_10_10_rev) {
         if (type != GL_UNSIGNED_INT_2_10_10_10_REV &&
             type != GL_INT_2_10_10_10_REV &&
             type != GL_UNSIGNED_BYTE)
            bgra_error = GL_TRUE;
      } else if (type != GL_UNSIGNED_BYTE)
         bgra_error = GL_TRUE;

      if (bgra_error) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s(GL_BGRA/GLubyte)", func);
         return;
      }
      format = GL_BGRA;
      size = 4;
   }
   else if (size < sizeMin || size > sizeMax || size > 4) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size=%d)", func, size);
      return;
   }

   if (ctx->Extensions.ARB_vertex_type_2_10_10_10_rev &&
       (type == GL_UNSIGNED_INT_2_10_10_10_REV ||
        type == GL_INT_2_10_10_10_REV) && size != 4) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(size=%d)", func, size);
      return;
   }

   ASSERT(size <= 4);

   if (stride < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "%s(stride=%d)", func, stride );
      return;
   }

   if (ctx->Array.ArrayObj->ARBsemantics &&
       !_mesa_is_bufferobj(ctx->Array.ArrayBufferObj)) {
      /* GL_ARB_vertex_array_object requires that all arrays reside in VBOs.
       * Generate GL_INVALID_OPERATION if that's not true.
       */
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(non-VBO array)", func);
      return;
   }

   elementSize = _mesa_sizeof_type(type) * size;

   array = &ctx->Array.ArrayObj->VertexAttrib[attrib];
   array->Size = size;
   array->Type = type;
   array->Format = format;
   array->Stride = stride;
   array->StrideB = stride ? stride : elementSize;
   array->Normalized = normalized;
   array->Integer = integer;
   array->Ptr = (const GLubyte *) ptr;
   array->_ElementSize = elementSize;

   _mesa_reference_buffer_object(ctx, &array->BufferObj,
                                 ctx->Array.ArrayBufferObj);

   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= VERT_BIT(attrib);
}


void GLAPIENTRY
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GLbitfield legalTypes = (SHORT_BIT | INT_BIT | FLOAT_BIT |
                            DOUBLE_BIT | HALF_BIT | FIXED_ES_BIT |
                            UNSIGNED_INT_2_10_10_10_REV_BIT |
                            INT_2_10_10_10_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->API == API_OPENGLES)
      legalTypes |= BYTE_BIT;

   update_array(ctx, "glVertexPointer", VERT_ATTRIB_POS,
                legalTypes, 2, 4,
                size, type, stride, GL_FALSE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr )
{
   const GLbitfield legalTypes = (BYTE_BIT | SHORT_BIT | INT_BIT |
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                                  FIXED_ES_BIT |
                                  UNSIGNED_INT_2_10_10_10_REV_BIT |
                                  INT_2_10_10_10_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glNormalPointer", VERT_ATTRIB_NORMAL,
                legalTypes, 3, 3,
                3, type, stride, GL_TRUE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT |
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                                  FIXED_ES_BIT |
                                  UNSIGNED_INT_2_10_10_10_REV_BIT |
                                  INT_2_10_10_10_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glColorPointer", VERT_ATTRIB_COLOR0,
                legalTypes, 3, BGRA_OR_4,
                size, type, stride, GL_TRUE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_FogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (HALF_BIT | FLOAT_BIT | DOUBLE_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glFogCoordPointer", VERT_ATTRIB_FOG,
                legalTypes, 1, 1,
                1, type, stride, GL_FALSE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (UNSIGNED_BYTE_BIT | SHORT_BIT | INT_BIT |
                                  FLOAT_BIT | DOUBLE_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glIndexPointer", VERT_ATTRIB_COLOR_INDEX,
                legalTypes, 1, 1,
                1, type, stride, GL_FALSE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_SecondaryColorPointerEXT(GLint size, GLenum type,
			       GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT |
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                                  UNSIGNED_INT_2_10_10_10_REV_BIT |
                                  INT_2_10_10_10_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glSecondaryColorPointer", VERT_ATTRIB_COLOR1,
                legalTypes, 3, BGRA_OR_4,
                size, type, stride, GL_TRUE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr)
{
   GLbitfield legalTypes = (SHORT_BIT | INT_BIT |
                            HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                            FIXED_ES_BIT |
                            UNSIGNED_INT_2_10_10_10_REV_BIT |
                            INT_2_10_10_10_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);
   const GLuint unit = ctx->Array.ActiveTexture;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->API == API_OPENGLES)
      legalTypes |= BYTE_BIT;

   update_array(ctx, "glTexCoordPointer", VERT_ATTRIB_TEX(unit),
                legalTypes, 1, 4,
                size, type, stride, GL_FALSE, GL_FALSE,
                ptr);
}


void GLAPIENTRY
_mesa_EdgeFlagPointer(GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = UNSIGNED_BYTE_BIT;
   /* see table 2.4 edits in GL_EXT_gpu_shader4 spec: */
   const GLboolean integer = GL_TRUE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glEdgeFlagPointer", VERT_ATTRIB_EDGEFLAG,
                legalTypes, 1, 1,
                1, GL_UNSIGNED_BYTE, stride, GL_FALSE, integer, ptr);
}


void GLAPIENTRY
_mesa_PointSizePointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (FLOAT_BIT | FIXED_ES_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->API != API_OPENGLES) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glPointSizePointer(ES 1.x only)");
      return;
   }
      
   update_array(ctx, "glPointSizePointer", VERT_ATTRIB_POINT_SIZE,
                legalTypes, 1, 1,
                1, type, stride, GL_FALSE, GL_FALSE, ptr);
}


#if FEATURE_NV_vertex_program
/**
 * Set a vertex attribute array.
 * Note that these arrays DO alias the conventional GL vertex arrays
 * (position, normal, color, fog, texcoord, etc).
 * The generic attribute slots at #16 and above are not touched.
 */
void GLAPIENTRY
_mesa_VertexAttribPointerNV(GLuint index, GLint size, GLenum type,
                            GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (UNSIGNED_BYTE_BIT | SHORT_BIT |
                                  FLOAT_BIT | DOUBLE_BIT);
   GLboolean normalized = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glVertexAttribPointerNV(index)");
      return;
   }

   if (type == GL_UNSIGNED_BYTE && size != 4) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glVertexAttribPointerNV(size!=4)");
      return;
   }

   update_array(ctx, "glVertexAttribPointerNV", VERT_ATTRIB_GENERIC(index),
                legalTypes, 1, BGRA_OR_4,
                size, type, stride, normalized, GL_FALSE, ptr);
}
#endif


#if FEATURE_ARB_vertex_program
/**
 * Set a generic vertex attribute array.
 * Note that these arrays DO NOT alias the conventional GL vertex arrays
 * (position, normal, color, fog, texcoord, etc).
 */
void GLAPIENTRY
_mesa_VertexAttribPointerARB(GLuint index, GLint size, GLenum type,
                             GLboolean normalized,
                             GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT |
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                                  FIXED_ES_BIT | FIXED_GL_BIT |
                                  UNSIGNED_INT_2_10_10_10_REV_BIT |
                                  INT_2_10_10_10_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glVertexAttribPointerARB(index)");
      return;
   }

   update_array(ctx, "glVertexAttribPointer", VERT_ATTRIB_GENERIC(index),
                legalTypes, 1, BGRA_OR_4,
                size, type, stride, normalized, GL_FALSE, ptr);
}
#endif


/**
 * GL_EXT_gpu_shader4 / GL 3.0.
 * Set an integer-valued vertex attribute array.
 * Note that these arrays DO NOT alias the conventional GL vertex arrays
 * (position, normal, color, fog, texcoord, etc).
 */
void GLAPIENTRY
_mesa_VertexAttribIPointer(GLuint index, GLint size, GLenum type,
                           GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT);
   const GLboolean normalized = GL_FALSE;
   const GLboolean integer = GL_TRUE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glVertexAttribIPointer(index)");
      return;
   }

   update_array(ctx, "glVertexAttribIPointer", VERT_ATTRIB_GENERIC(index),
                legalTypes, 1, 4,
                size, type, stride, normalized, integer, ptr);
}



void GLAPIENTRY
_mesa_EnableVertexAttribArrayARB(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.ArrayObj->VertexAttrib));

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Enabled = GL_TRUE;
   ctx->Array.ArrayObj->_Enabled |= VERT_BIT_GENERIC(index);
   ctx->Array.NewState |= VERT_BIT_GENERIC(index);
}


void GLAPIENTRY
_mesa_DisableVertexAttribArrayARB(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDisableVertexAttribArrayARB(index)");
      return;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.ArrayObj->VertexAttrib));

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Enabled = GL_FALSE;
   ctx->Array.ArrayObj->_Enabled &= ~VERT_BIT_GENERIC(index);
   ctx->Array.NewState |= VERT_BIT_GENERIC(index);
}


/**
 * Return info for a vertex attribute array (no alias with legacy
 * vertex attributes (pos, normal, color, etc)).  This function does
 * not handle the 4-element GL_CURRENT_VERTEX_ATTRIB_ARB query.
 */
static GLuint
get_vertex_array_attrib(struct gl_context *ctx, GLuint index, GLenum pname,
                  const char *caller)
{
   const struct gl_client_array *array;

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%u)", caller, index);
      return 0;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.ArrayObj->VertexAttrib));

   array = &ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(index)];

   switch (pname) {
   case GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB:
      return array->Enabled;
   case GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB:
      return array->Size;
   case GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB:
      return array->Stride;
   case GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB:
      return array->Type;
   case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB:
      return array->Normalized;
   case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB:
      return array->BufferObj->Name;
   case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
      if (ctx->VersionMajor >= 3 || ctx->Extensions.EXT_gpu_shader4) {
         return array->Integer;
      }
      goto error;
   case GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ARB:
      if (ctx->Extensions.ARB_instanced_arrays) {
         return array->InstanceDivisor;
      }
      goto error;
   default:
      ; /* fall-through */
   }

error:
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=0x%x)", caller, pname);
   return 0;
}


static const GLfloat *
get_current_attrib(struct gl_context *ctx, GLuint index, const char *function)
{
   if (index == 0) {
      if (ctx->API != API_OPENGLES2) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "%s(index==0)", function);
	 return NULL;
      }
   }
   else if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "%s(index>=GL_MAX_VERTEX_ATTRIBS)", function);
      return NULL;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.ArrayObj->VertexAttrib));

   FLUSH_CURRENT(ctx, 0);
   return ctx->Current.Attrib[VERT_ATTRIB_GENERIC(index)];
}

void GLAPIENTRY
_mesa_GetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v = get_current_attrib(ctx, index, "glGetVertexAttribfv");
      if (v != NULL) {
         COPY_4V(params, v);
      }
   }
   else {
      params[0] = (GLfloat) get_vertex_array_attrib(ctx, index, pname,
                                                    "glGetVertexAttribfv");
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v = get_current_attrib(ctx, index, "glGetVertexAttribdv");
      if (v != NULL) {
         params[0] = (GLdouble) v[0];
         params[1] = (GLdouble) v[1];
         params[2] = (GLdouble) v[2];
         params[3] = (GLdouble) v[3];
      }
   }
   else {
      params[0] = (GLdouble) get_vertex_array_attrib(ctx, index, pname,
                                                     "glGetVertexAttribdv");
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribivARB(GLuint index, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v = get_current_attrib(ctx, index, "glGetVertexAttribiv");
      if (v != NULL) {
         /* XXX should floats in[0,1] be scaled to full int range? */
         params[0] = (GLint) v[0];
         params[1] = (GLint) v[1];
         params[2] = (GLint) v[2];
         params[3] = (GLint) v[3];
      }
   }
   else {
      params[0] = (GLint) get_vertex_array_attrib(ctx, index, pname,
                                                  "glGetVertexAttribiv");
   }
}


/** GL 3.0 */
void GLAPIENTRY
_mesa_GetVertexAttribIiv(GLuint index, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v =
	 get_current_attrib(ctx, index, "glGetVertexAttribIiv");
      if (v != NULL) {
         /* XXX we don't have true integer-valued vertex attribs yet */
         params[0] = (GLint) v[0];
         params[1] = (GLint) v[1];
         params[2] = (GLint) v[2];
         params[3] = (GLint) v[3];
      }
   }
   else {
      params[0] = (GLint) get_vertex_array_attrib(ctx, index, pname,
                                                  "glGetVertexAttribIiv");
   }
}


/** GL 3.0 */
void GLAPIENTRY
_mesa_GetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v =
	 get_current_attrib(ctx, index, "glGetVertexAttribIuiv");
      if (v != NULL) {
         /* XXX we don't have true integer-valued vertex attribs yet */
         params[0] = (GLuint) v[0];
         params[1] = (GLuint) v[1];
         params[2] = (GLuint) v[2];
         params[3] = (GLuint) v[3];
      }
   }
   else {
      params[0] = get_vertex_array_attrib(ctx, index, pname,
                                          "glGetVertexAttribIuiv");
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribPointervARB(GLuint index, GLenum pname, GLvoid **pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribPointerARB(index)");
      return;
   }

   if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribPointerARB(pname)");
      return;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.ArrayObj->VertexAttrib));

   *pointer = (GLvoid *) ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Ptr;
}


void GLAPIENTRY
_mesa_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                       GLsizei count, const GLvoid *ptr)
{
   (void) count;
   _mesa_VertexPointer(size, type, stride, ptr);
}


void GLAPIENTRY
_mesa_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                       const GLvoid *ptr)
{
   (void) count;
   _mesa_NormalPointer(type, stride, ptr);
}


void GLAPIENTRY
_mesa_ColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   (void) count;
   _mesa_ColorPointer(size, type, stride, ptr);
}


void GLAPIENTRY
_mesa_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   (void) count;
   _mesa_IndexPointer(type, stride, ptr);
}


void GLAPIENTRY
_mesa_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
                         GLsizei count, const GLvoid *ptr)
{
   (void) count;
   _mesa_TexCoordPointer(size, type, stride, ptr);
}


void GLAPIENTRY
_mesa_EdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr)
{
   (void) count;
   _mesa_EdgeFlagPointer(stride, ptr);
}


void GLAPIENTRY
_mesa_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean tflag, cflag, nflag;  /* enable/disable flags */
   GLint tcomps, ccomps, vcomps;   /* components per texcoord, color, vertex */
   GLenum ctype = 0;               /* color type */
   GLint coffset = 0, noffset = 0, voffset;/* color, normal, vertex offsets */
   const GLint toffset = 0;        /* always zero */
   GLint defstride;                /* default stride */
   GLint c, f;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   f = sizeof(GLfloat);
   c = f * ((4 * sizeof(GLubyte) + (f - 1)) / f);

   if (stride < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glInterleavedArrays(stride)" );
      return;
   }

   switch (format) {
      case GL_V2F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 2;
         voffset = 0;
         defstride = 2*f;
         break;
      case GL_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         voffset = 0;
         defstride = 3*f;
         break;
      case GL_C4UB_V2F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 2;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 2*f;
         break;
      case GL_C4UB_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 3*f;
         break;
      case GL_C3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         noffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_C4F_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         noffset = 4*f;
         voffset = 7*f;
         defstride = 10*f;
         break;
      case GL_T2F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         voffset = 2*f;
         defstride = 5*f;
         break;
      case GL_T4F_V4F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 4;  ccomps = 0;  vcomps = 4;
         voffset = 4*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4UB_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 2*f;
         voffset = c+2*f;
         defstride = c+5*f;
         break;
      case GL_T2F_C3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         noffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         noffset = 6*f;
         voffset = 9*f;
         defstride = 12*f;
         break;
      case GL_T4F_C4F_N3F_V4F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 4;  ccomps = 4;  vcomps = 4;
         ctype = GL_FLOAT;
         coffset = 4*f;
         noffset = 8*f;
         voffset = 11*f;
         defstride = 15*f;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glInterleavedArrays(format)" );
         return;
   }

   if (stride==0) {
      stride = defstride;
   }

   _mesa_DisableClientState( GL_EDGE_FLAG_ARRAY );
   _mesa_DisableClientState( GL_INDEX_ARRAY );
   /* XXX also disable secondary color and generic arrays? */

   /* Texcoords */
   if (tflag) {
      _mesa_EnableClientState( GL_TEXTURE_COORD_ARRAY );
      _mesa_TexCoordPointer( tcomps, GL_FLOAT, stride,
                             (GLubyte *) pointer + toffset );
   }
   else {
      _mesa_DisableClientState( GL_TEXTURE_COORD_ARRAY );
   }

   /* Color */
   if (cflag) {
      _mesa_EnableClientState( GL_COLOR_ARRAY );
      _mesa_ColorPointer( ccomps, ctype, stride,
			  (GLubyte *) pointer + coffset );
   }
   else {
      _mesa_DisableClientState( GL_COLOR_ARRAY );
   }


   /* Normals */
   if (nflag) {
      _mesa_EnableClientState( GL_NORMAL_ARRAY );
      _mesa_NormalPointer( GL_FLOAT, stride, (GLubyte *) pointer + noffset );
   }
   else {
      _mesa_DisableClientState( GL_NORMAL_ARRAY );
   }

   /* Vertices */
   _mesa_EnableClientState( GL_VERTEX_ARRAY );
   _mesa_VertexPointer( vcomps, GL_FLOAT, stride,
			(GLubyte *) pointer + voffset );
}


void GLAPIENTRY
_mesa_LockArraysEXT(GLint first, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glLockArrays %d %d\n", first, count);

   if (first < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glLockArraysEXT(first)" );
      return;
   }
   if (count <= 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glLockArraysEXT(count)" );
      return;
   }
   if (ctx->Array.LockCount != 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glLockArraysEXT(reentry)" );
      return;
   }

   ctx->Array.LockFirst = first;
   ctx->Array.LockCount = count;

   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= VERT_BIT_ALL;
}


void GLAPIENTRY
_mesa_UnlockArraysEXT( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glUnlockArrays\n");

   if (ctx->Array.LockCount == 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glUnlockArraysEXT(reexit)" );
      return;
   }

   ctx->Array.LockFirst = 0;
   ctx->Array.LockCount = 0;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= VERT_BIT_ALL;
}


/* GL_EXT_multi_draw_arrays */
void GLAPIENTRY
_mesa_MultiDrawArraysEXT( GLenum mode, const GLint *first,
                          const GLsizei *count, GLsizei primcount )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         CALL_DrawArrays(ctx->Exec, (mode, first[i], count[i]));
      }
   }
}


/* GL_IBM_multimode_draw_arrays */
void GLAPIENTRY
_mesa_MultiModeDrawArraysIBM( const GLenum * mode, const GLint * first,
			      const GLsizei * count,
			      GLsizei primcount, GLint modestride )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   for ( i = 0 ; i < primcount ; i++ ) {
      if ( count[i] > 0 ) {
         GLenum m = *((GLenum *) ((GLubyte *) mode + i * modestride));
	 CALL_DrawArrays(ctx->Exec, ( m, first[i], count[i] ));
      }
   }
}


/* GL_IBM_multimode_draw_arrays */
void GLAPIENTRY
_mesa_MultiModeDrawElementsIBM( const GLenum * mode, const GLsizei * count,
				GLenum type, const GLvoid * const * indices,
				GLsizei primcount, GLint modestride )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   /* XXX not sure about ARB_vertex_buffer_object handling here */

   for ( i = 0 ; i < primcount ; i++ ) {
      if ( count[i] > 0 ) {
         GLenum m = *((GLenum *) ((GLubyte *) mode + i * modestride));
	 CALL_DrawElements(ctx->Exec, ( m, count[i], type, indices[i] ));
      }
   }
}


/**
 * GL_NV_primitive_restart and GL 3.1
 */
void GLAPIENTRY
_mesa_PrimitiveRestartIndex(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.NV_primitive_restart &&
       ctx->VersionMajor * 10 + ctx->VersionMinor < 31) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPrimitiveRestartIndexNV()");
      return;
   }

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_VERTICES(ctx, _NEW_TRANSFORM);

   ctx->Array.RestartIndex = index;
}


/**
 * See GL_ARB_instanced_arrays.
 * Note that the instance divisor only applies to generic arrays, not
 * the legacy vertex arrays.
 */
void GLAPIENTRY
_mesa_VertexAttribDivisor(GLuint index, GLuint divisor)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (!ctx->Extensions.ARB_instanced_arrays) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glVertexAttribDivisor()");
      return;
   }

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glVertexAttribDivisor(index = %u)",
                  index);
      return;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.ArrayObj->VertexAttrib));

   ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(index)].InstanceDivisor = divisor;
}



/**
 * Copy one client vertex array to another.
 */
void
_mesa_copy_client_array(struct gl_context *ctx,
                        struct gl_client_array *dst,
                        struct gl_client_array *src)
{
   dst->Size = src->Size;
   dst->Type = src->Type;
   dst->Format = src->Format;
   dst->Stride = src->Stride;
   dst->StrideB = src->StrideB;
   dst->Ptr = src->Ptr;
   dst->Enabled = src->Enabled;
   dst->Normalized = src->Normalized;
   dst->Integer = src->Integer;
   dst->InstanceDivisor = src->InstanceDivisor;
   dst->_ElementSize = src->_ElementSize;
   _mesa_reference_buffer_object(ctx, &dst->BufferObj, src->BufferObj);
   dst->_MaxElement = src->_MaxElement;
}



/**
 * Print vertex array's fields.
 */
static void
print_array(const char *name, GLint index, const struct gl_client_array *array)
{
   if (index >= 0)
      printf("  %s[%d]: ", name, index);
   else
      printf("  %s: ", name);
   printf("Ptr=%p, Type=0x%x, Size=%d, ElemSize=%u, Stride=%d, Buffer=%u(Size %lu), MaxElem=%u\n",
	  array->Ptr, array->Type, array->Size,
	  array->_ElementSize, array->StrideB,
	  array->BufferObj->Name, (unsigned long) array->BufferObj->Size,
	  array->_MaxElement);
}


/**
 * Print current vertex object/array info.  For debug.
 */
void
_mesa_print_arrays(struct gl_context *ctx)
{
   struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
   GLuint i;

   _mesa_update_array_object_max_element(ctx, arrayObj);

   printf("Array Object %u\n", arrayObj->Name);
   if (arrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled)
      print_array("Vertex", -1, &arrayObj->VertexAttrib[VERT_ATTRIB_POS]);
   if (arrayObj->VertexAttrib[VERT_ATTRIB_NORMAL].Enabled)
      print_array("Normal", -1, &arrayObj->VertexAttrib[VERT_ATTRIB_NORMAL]);
   if (arrayObj->VertexAttrib[VERT_ATTRIB_COLOR0].Enabled)
      print_array("Color", -1, &arrayObj->VertexAttrib[VERT_ATTRIB_COLOR0]);
   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++)
      if (arrayObj->VertexAttrib[VERT_ATTRIB_TEX(i)].Enabled)
         print_array("TexCoord", i, &arrayObj->VertexAttrib[VERT_ATTRIB_TEX(i)]);
   for (i = 0; i < VERT_ATTRIB_GENERIC_MAX; i++)
      if (arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(i)].Enabled)
         print_array("Attrib", i, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(i)]);
   printf("  _MaxElement = %u\n", arrayObj->_MaxElement);
}


/**
 * Initialize vertex array state for given context.
 */
void 
_mesa_init_varray(struct gl_context *ctx)
{
   ctx->Array.DefaultArrayObj = _mesa_new_array_object(ctx, 0);
   _mesa_reference_array_object(ctx, &ctx->Array.ArrayObj,
                                ctx->Array.DefaultArrayObj);
   ctx->Array.ActiveTexture = 0;   /* GL_ARB_multitexture */

   ctx->Array.Objects = _mesa_NewHashTable();
}


/**
 * Callback for deleting an array object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_arrayobj_cb(GLuint id, void *data, void *userData)
{
   struct gl_array_object *arrayObj = (struct gl_array_object *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
   _mesa_delete_array_object(ctx, arrayObj);
}


/**
 * Free vertex array state for given context.
 */
void 
_mesa_free_varray_data(struct gl_context *ctx)
{
   _mesa_HashDeleteAll(ctx->Array.Objects, delete_arrayobj_cb, ctx);
   _mesa_DeleteHashTable(ctx->Array.Objects);
}

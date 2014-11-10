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

#include <precomp.h>

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
   case GL_FLOAT:
      return FLOAT_BIT;
   case GL_DOUBLE:
      return DOUBLE_BIT;
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
 * \param sizeMax  max allowable size value
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

   typeBit = type_to_bit(ctx, type);
   if (typeBit == 0x0 || (typeBit & legalTypesMask) == 0x0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(type = %s)",
                  func, _mesa_lookup_enum_by_nr(type));
      return;
   }

   /* Do size parameter checking. */
   if (size < sizeMin || size > sizeMax || size > 4) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size=%d)", func, size);
      return;
   }

   ASSERT(size <= 4);

   if (stride < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "%s(stride=%d)", func, stride );
      return;
   }

   elementSize = _mesa_sizeof_type(type) * size;

   array = &ctx->Array.VertexAttrib[attrib];
   array->Size = size;
   array->Type = type;
   array->Stride = stride;
   array->StrideB = stride ? stride : elementSize;
   array->Normalized = normalized;
   array->Integer = integer;
   array->Ptr = (const GLubyte *) ptr;
   array->_ElementSize = elementSize;

   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= VERT_BIT(attrib);
}


void GLAPIENTRY
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GLbitfield legalTypes = (SHORT_BIT | INT_BIT | FLOAT_BIT |
                            DOUBLE_BIT | HALF_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glVertexPointer", VERT_ATTRIB_POS,
                legalTypes, 2, 4,
                size, type, stride, GL_FALSE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr )
{
   const GLbitfield legalTypes = (BYTE_BIT | SHORT_BIT | INT_BIT |
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT);
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
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glColorPointer", VERT_ATTRIB_COLOR,
                legalTypes, 3, 4,
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
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr)
{
   GLbitfield legalTypes = (SHORT_BIT | INT_BIT |
                            HALF_BIT | FLOAT_BIT | DOUBLE_BIT);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   update_array(ctx, "glTexCoordPointer", VERT_ATTRIB_TEX,
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
 * Copy one client vertex array to another.
 */
void
_mesa_copy_client_array(struct gl_context *ctx,
                        struct gl_client_array *dst,
                        struct gl_client_array *src)
{
   dst->Size = src->Size;
   dst->Type = src->Type;
   dst->Stride = src->Stride;
   dst->StrideB = src->StrideB;
   dst->Ptr = src->Ptr;
   dst->Enabled = src->Enabled;
   dst->Normalized = src->Normalized;
   dst->Integer = src->Integer;
   dst->_ElementSize = src->_ElementSize;
   _mesa_reference_buffer_object(ctx, &dst->BufferObj, src->BufferObj);
   dst->_MaxElement = src->_MaxElement;
}

static void
init_array(struct gl_context *ctx,
           struct gl_client_array *array, GLint size, GLint type)
{
   array->Size = size;
   array->Type = type;
   array->Stride = 0;
   array->StrideB = 0;
   array->Ptr = NULL;
   array->Enabled = GL_FALSE;
   array->Normalized = GL_FALSE;
   array->Integer = GL_FALSE;
   array->_ElementSize = size * _mesa_sizeof_type(type);
   /* Vertex array buffers */
   _mesa_reference_buffer_object(ctx, &array->BufferObj,
                                 ctx->Shared->NullBufferObj);
}


/**
 * Initialize vertex array state for given context.
 */
void 
_mesa_init_varray(struct gl_context *ctx, struct gl_array_attrib *array)
{
    GLuint i;

    /* Init the individual arrays */
    for (i = 0; i < Elements(array->VertexAttrib); i++) {
       switch (i) {
       case VERT_ATTRIB_WEIGHT:
          init_array(ctx, &array->VertexAttrib[VERT_ATTRIB_WEIGHT], 1, GL_FLOAT);
          break;
       case VERT_ATTRIB_NORMAL:
          init_array(ctx, &array->VertexAttrib[VERT_ATTRIB_NORMAL], 3, GL_FLOAT);
          break;
       case VERT_ATTRIB_FOG:
          init_array(ctx, &array->VertexAttrib[VERT_ATTRIB_FOG], 1, GL_FLOAT);
          break;
       case VERT_ATTRIB_COLOR_INDEX:
          init_array(ctx, &array->VertexAttrib[VERT_ATTRIB_COLOR_INDEX], 1, GL_FLOAT);
          break;
       case VERT_ATTRIB_EDGEFLAG:
          init_array(ctx, &array->VertexAttrib[VERT_ATTRIB_EDGEFLAG], 1, GL_BOOL);
          break;
       default:
          init_array(ctx, &array->VertexAttrib[i], 4, GL_FLOAT);
          break;
       }
    }
}


/**
 * Free vertex array state for given context.
 */
void 
_mesa_free_varray_data(struct gl_context *ctx, struct gl_array_attrib* array)
{
    GLuint i;

    /* Uninit the individual arrays */
    for (i = 0; i < Elements(array->VertexAttrib); i++)
    {
        _mesa_reference_buffer_object(ctx, &array->VertexAttrib[i].BufferObj, NULL);
        memset(&array->VertexAttrib[i], 0, sizeof(struct gl_client_array));
    }
}

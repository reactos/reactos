/* $Id: varray.c,v 1.17 1998/02/03 01:40:45 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: varray.c,v $
 * Revision 1.17  1998/02/03 01:40:45  brianp
 * fixed a few expressions for Amiga compilation (Sam Jordan)
 *
 * Revision 1.16  1997/08/14 01:15:47  brianp
 * gl_ArrayElement()'s assignment of current normal was wrong
 *
 * Revision 1.15  1997/07/24 01:25:54  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.14  1997/06/20 02:50:19  brianp
 * replaced Current.IntColor with Current.ByteColor
 *
 * Revision 1.13  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.12  1997/05/24 12:07:43  brianp
 * colors weren't being used in gl_ArrayElement()
 *
 * Revision 1.11  1997/04/20 20:29:11  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.10  1997/04/02 03:12:31  brianp
 * misc changes for new vertex buffer organization
 *
 * Revision 1.9  1997/01/30 21:05:03  brianp
 * moved gl_GetPointerv() to get.c
 *
 * Revision 1.8  1996/12/18 20:00:57  brianp
 * gl_set_material() now takes a bitmask instead of face and pname
 *
 * Revision 1.7  1996/12/07 10:21:28  brianp
 * call gl_set_material() instead of gl_Materialfv()
 *
 * Revision 1.6  1996/11/09 03:12:34  brianp
 * now call gl_render_vb() after gl_transform_vb_part2() call
 *
 * Revision 1.5  1996/10/08 00:05:06  brianp
 * added some missing stuff to gl_ArrayElement()
 *
 * Revision 1.4  1996/10/04 02:37:06  brianp
 * gl_ArrayElement() wasn't always initializing vertex Z and W values
 *
 * Revision 1.3  1996/10/03 00:47:56  brianp
 * added #include <stdlib.h> for abort()
 *
 * Revision 1.2  1996/09/27 01:33:07  brianp
 * added missing default cases to switches
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */



/*
 * NOTE:  At this time, only three vertex array configurations are optimized:
 *  1.  glVertex3fv(), zero stride
 *  2.  glNormal3fv() with glVertex3fv(), zero stride
 *  3.  glNormal3fv() with glVertex4fv(), zero stride
 *
 * More optimized array configurations can be added.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "enable.h"
#include "dlist.h"
#include "light.h"
#include "macros.h"
#include "types.h"
#include "varray.h"
#include "vb.h"
#include "vbfill.h"
#include "vbrender.h"
#include "vbxform.h"
#include "xform.h"
#endif


void gl_VertexPointer( GLcontext *ctx,
                       GLint size, GLenum type, GLsizei stride,
                       const GLvoid *ptr )
{
   if (size<2 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glVertexPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glVertexPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_SHORT:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glVertexPointer(type)" );
         return;
   }
   ctx->Array.VertexSize = size;
   ctx->Array.VertexType = type;
   ctx->Array.VertexStride = stride;
   ctx->Array.VertexPtr = (void *) ptr;
}




void gl_NormalPointer( GLcontext *ctx,
                       GLenum type, GLsizei stride, const GLvoid *ptr )
{
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glNormalPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_BYTE:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLbyte);
         break;
      case GL_SHORT:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glNormalPointer(type)" );
         return;
   }
   ctx->Array.NormalType = type;
   ctx->Array.NormalStride = stride;
   ctx->Array.NormalPtr = (void *) ptr;
}



void gl_ColorPointer( GLcontext *ctx,
                      GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr )
{
   if (size<3 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_BYTE:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLbyte);
         break;
      case GL_UNSIGNED_BYTE:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLubyte);
         break;
      case GL_SHORT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLshort);
         break;
      case GL_UNSIGNED_SHORT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLushort);
         break;
      case GL_INT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLint);
         break;
      case GL_UNSIGNED_INT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLuint);
         break;
      case GL_FLOAT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glColorPointer(type)" );
         return;
   }
   ctx->Array.ColorSize = size;
   ctx->Array.ColorType = type;
   ctx->Array.ColorStride = stride;
   ctx->Array.ColorPtr = (void *) ptr;
}



void gl_IndexPointer( GLcontext *ctx,
                      GLenum type, GLsizei stride, const GLvoid *ptr )
{
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glIndexPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_SHORT:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLbyte);
         break;
      case GL_INT:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glIndexPointer(type)" );
         return;
   }
   ctx->Array.IndexType = type;
   ctx->Array.IndexStride = stride;
   ctx->Array.IndexPtr = (void *) ptr;
}



void gl_TexCoordPointer( GLcontext *ctx,
                         GLint size, GLenum type, GLsizei stride,
                         const GLvoid *ptr )
{
   if (size<1 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexCoordPointer(size)" );
      return;
   }
   switch (type) {
      case GL_SHORT:
         ctx->Array.TexCoordStrideB = stride ? stride : size*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.TexCoordStrideB = stride ? stride : size*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.TexCoordStrideB = stride ? stride : size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.TexCoordStrideB = stride ? stride : size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexCoordPointer(type)" );
         return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexCoordPointer(stride)" );
      return;
   }
   ctx->Array.TexCoordSize = size;
   ctx->Array.TexCoordType = type;
   ctx->Array.TexCoordStride = stride;
   ctx->Array.TexCoordPtr = (void *) ptr;
}



void gl_EdgeFlagPointer( GLcontext *ctx,
                         GLsizei stride, const GLboolean *ptr )
{
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glEdgeFlagPointer(stride)" );
      return;
   }
   ctx->Array.EdgeFlagStride = stride;
   ctx->Array.EdgeFlagStrideB = stride ? stride : sizeof(GLboolean);
   ctx->Array.EdgeFlagPtr = (GLboolean *) ptr;
}



/*
 * Execute
 */
void gl_ArrayElement( GLcontext *ctx, GLint i )
{
   struct vertex_buffer *VB = ctx->VB;
   GLint count = VB->Count;

   /* copy vertex data into the Vertex Buffer */

   if (ctx->Array.NormalEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.NormalPtr
                  + i * ctx->Array.NormalStrideB;
      switch (ctx->Array.NormalType) {
         case GL_BYTE:
            VB->Normal[count][0] = BYTE_TO_FLOAT( p[0] );
            VB->Normal[count][1] = BYTE_TO_FLOAT( p[1] );
            VB->Normal[count][2] = BYTE_TO_FLOAT( p[2] );
            break;
         case GL_SHORT:
            VB->Normal[count][0] = SHORT_TO_FLOAT( ((GLshort*)p)[0] );
            VB->Normal[count][1] = SHORT_TO_FLOAT( ((GLshort*)p)[1] );
            VB->Normal[count][2] = SHORT_TO_FLOAT( ((GLshort*)p)[2] );
            break;
         case GL_INT:
            VB->Normal[count][0] = INT_TO_FLOAT( ((GLint*)p)[0] );
            VB->Normal[count][1] = INT_TO_FLOAT( ((GLint*)p)[1] );
            VB->Normal[count][2] = INT_TO_FLOAT( ((GLint*)p)[2] );
            break;
         case GL_FLOAT:
            VB->Normal[count][0] = ((GLfloat*)p)[0];
            VB->Normal[count][1] = ((GLfloat*)p)[1];
            VB->Normal[count][2] = ((GLfloat*)p)[2];
            break;
         case GL_DOUBLE:
            VB->Normal[count][0] = ((GLdouble*)p)[0];
            VB->Normal[count][1] = ((GLdouble*)p)[1];
            VB->Normal[count][2] = ((GLdouble*)p)[2];
            break;
         default:
            gl_problem(ctx, "Bad normal type in gl_ArrayElement");
            return;
      }
      VB->MonoNormal = GL_FALSE;
   }
   else {
      VB->Normal[count][0] = ctx->Current.Normal[0];
      VB->Normal[count][1] = ctx->Current.Normal[1];
      VB->Normal[count][2] = ctx->Current.Normal[2];
   } 

   /* TODO: directly set VB->Fcolor instead of calling a glColor command */
   if (ctx->Array.ColorEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.ColorPtr + i * ctx->Array.ColorStrideB;
      switch (ctx->Array.ColorType) {
         case GL_BYTE:
            switch (ctx->Array.ColorSize) {
               case 4:   glColor4bv( (GLbyte*) p );   break;
               case 3:   glColor3bv( (GLbyte*) p );   break;
            }
            break;
         case GL_UNSIGNED_BYTE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3ubv( (GLubyte*) p );   break;
               case 4:   glColor4ubv( (GLubyte*) p );   break;
            }
            break;
         case GL_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3sv( (GLshort*) p );   break;
               case 4:   glColor4sv( (GLshort*) p );   break;
            }
            break;
         case GL_UNSIGNED_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3usv( (GLushort*) p );   break;
               case 4:   glColor4usv( (GLushort*) p );   break;
            }
            break;
         case GL_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3iv( (GLint*) p );   break;
               case 4:   glColor4iv( (GLint*) p );   break;
            }
            break;
         case GL_UNSIGNED_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3uiv( (GLuint*) p );   break;
               case 4:   glColor4uiv( (GLuint*) p );   break;
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3fv( (GLfloat*) p );   break;
               case 4:   glColor4fv( (GLfloat*) p );   break;
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3dv( (GLdouble*) p );   break;
               case 4:   glColor4dv( (GLdouble*) p );   break;
            }
            break;
         default:
            gl_problem(ctx, "Bad color type in gl_ArrayElement");
            return;
      }
      ctx->VB->MonoColor = GL_FALSE;
   }

   /* current color has been updated. store in vertex buffer now */
   {
      COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
      if (ctx->Light.ColorMaterialEnabled) {
         GLfloat color[4];
         color[0] = ctx->Current.ByteColor[0] * ctx->Visual->InvRedScale;
         color[1] = ctx->Current.ByteColor[1] * ctx->Visual->InvGreenScale;
         color[2] = ctx->Current.ByteColor[2] * ctx->Visual->InvBlueScale;
         color[3] = ctx->Current.ByteColor[3] * ctx->Visual->InvAlphaScale;
         gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
      }
   }

   if (ctx->Array.IndexEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.IndexPtr + i * ctx->Array.IndexStrideB;
      switch (ctx->Array.IndexType) {
         case GL_SHORT:
            VB->Findex[count] = (GLuint) (*((GLshort*) p));
            break;
         case GL_INT:
            VB->Findex[count] = (GLuint) (*((GLint*) p));
            break;
         case GL_FLOAT:
            VB->Findex[count] = (GLuint) (*((GLfloat*) p));
            break;
         case GL_DOUBLE:
            VB->Findex[count] = (GLuint) (*((GLdouble*) p));
            break;
         default:
            gl_problem(ctx, "Bad index type in gl_ArrayElement");
            return;
      }
      ctx->VB->MonoColor = GL_FALSE;
   }
   else {
      VB->Findex[count] = ctx->Current.Index;
   }

   if (ctx->Array.TexCoordEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.TexCoordPtr
                  + i * ctx->Array.TexCoordStrideB;
      VB->TexCoord[count][1] = 0.0F;
      VB->TexCoord[count][2] = 0.0F;
      VB->TexCoord[count][3] = 1.0F;
      switch (ctx->Array.TexCoordType) {
         case GL_SHORT:
            switch (ctx->Array.TexCoordSize) {
               /* FALL THROUGH! */
               case 4:   VB->TexCoord[count][3] = ((GLshort*) p)[3];
               case 3:   VB->TexCoord[count][2] = ((GLshort*) p)[2];
               case 2:   VB->TexCoord[count][1] = ((GLshort*) p)[1];
               case 1:   VB->TexCoord[count][0] = ((GLshort*) p)[0];
            }
            break;
         case GL_INT:
            switch (ctx->Array.TexCoordSize) {
               /* FALL THROUGH! */
               case 4:   VB->TexCoord[count][3] = ((GLint*) p)[3];
               case 3:   VB->TexCoord[count][2] = ((GLint*) p)[2];
               case 2:   VB->TexCoord[count][1] = ((GLint*) p)[1];
               case 1:   VB->TexCoord[count][0] = ((GLint*) p)[0];
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.TexCoordSize) {
               /* FALL THROUGH! */
               case 4:   VB->TexCoord[count][3] = ((GLfloat*) p)[3];
               case 3:   VB->TexCoord[count][2] = ((GLfloat*) p)[2];
               case 2:   VB->TexCoord[count][1] = ((GLfloat*) p)[1];
               case 1:   VB->TexCoord[count][0] = ((GLfloat*) p)[0];
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.TexCoordSize) {
               /* FALL THROUGH! */
               case 4:   VB->TexCoord[count][3] = ((GLdouble*) p)[3];
               case 3:   VB->TexCoord[count][2] = ((GLdouble*) p)[2];
               case 2:   VB->TexCoord[count][1] = ((GLdouble*) p)[1];
               case 1:   VB->TexCoord[count][0] = ((GLdouble*) p)[0];
            }
            break;
         default:
            gl_problem(ctx, "Bad texcoord type in gl_ArrayElement");
            return;
      }
   }
   else {
      COPY_4V( VB->TexCoord[count], ctx->Current.TexCoord );
   }

   if (ctx->Array.EdgeFlagEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.EdgeFlagPtr
                  + i * ctx->Array.EdgeFlagStrideB;
      VB->Edgeflag[count] = *((GLboolean*) b);
   }
   else {
      VB->Edgeflag[count] = ctx->Current.EdgeFlag;
   }

   if (ctx->Array.VertexEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.VertexPtr
                  + i * ctx->Array.VertexStrideB;
      VB->Obj[count][2] = 0.0F;
      VB->Obj[count][3] = 1.0F;
      switch (ctx->Array.VertexType) {
         case GL_SHORT:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLshort*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLshort*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLshort*) b)[1];
                         VB->Obj[count][0] = ((GLshort*) b)[0];
            }
            break;
         case GL_INT:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLint*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLint*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLint*) b)[1];
                         VB->Obj[count][0] = ((GLint*) b)[0];
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLfloat*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLfloat*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLfloat*) b)[1];
                         VB->Obj[count][0] = ((GLfloat*) b)[0];
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLdouble*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLdouble*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLdouble*) b)[1];
                         VB->Obj[count][0] = ((GLdouble*) b)[0];
            }
            break;
         default:
            gl_problem(ctx, "Bad vertex type in gl_ArrayElement");
            return;
      }

      /* Only store vertex if Vertex array pointer is enabled */
      count++;
      VB->Count = count;
      if (count==VB_MAX) {
         gl_transform_vb_part1( ctx, GL_FALSE );
      }

   }
   else {
      /* vertex array pointer not enabled: no vertex to process */
   }
}




/*
 * Save into display list
 * Use external API entry points since speed isn't too important here
 * and makes the code simpler.  Also, if GL_COMPILE_AND_EXECUTE then
 * execute will happen too.
 */
void gl_save_ArrayElement( GLcontext *ctx, GLint i )
{
   if (ctx->Array.NormalEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.NormalPtr
                  + i * ctx->Array.NormalStrideB;
      switch (ctx->Array.NormalType) {
         case GL_BYTE:
            glNormal3bv( (GLbyte*) p );
            break;
         case GL_SHORT:
            glNormal3sv( (GLshort*) p );
            break;
         case GL_INT:
            glNormal3iv( (GLint*) p );
            break;
         case GL_FLOAT:
            glNormal3fv( (GLfloat*) p );
            break;
         case GL_DOUBLE:
            glNormal3dv( (GLdouble*) p );
            break;
         default:
            gl_problem(ctx, "Bad normal type in gl_save_ArrayElement");
            return;
      }
   }

   if (ctx->Array.ColorEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.ColorPtr + i * ctx->Array.ColorStrideB;
      switch (ctx->Array.ColorType) {
         case GL_BYTE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3bv( (GLbyte*) p );   break;
               case 4:   glColor4bv( (GLbyte*) p );   break;
            }
            break;
         case GL_UNSIGNED_BYTE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3ubv( (GLubyte*) p );   break;
               case 4:   glColor4ubv( (GLubyte*) p );   break;
            }
            break;
         case GL_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3sv( (GLshort*) p );   break;
               case 4:   glColor4sv( (GLshort*) p );   break;
            }
            break;
         case GL_UNSIGNED_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3usv( (GLushort*) p );   break;
               case 4:   glColor4usv( (GLushort*) p );   break;
            }
            break;
         case GL_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3iv( (GLint*) p );   break;
               case 4:   glColor4iv( (GLint*) p );   break;
            }
            break;
         case GL_UNSIGNED_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3uiv( (GLuint*) p );   break;
               case 4:   glColor4uiv( (GLuint*) p );   break;
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3fv( (GLfloat*) p );   break;
               case 4:   glColor4fv( (GLfloat*) p );   break;
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3dv( (GLdouble*) p );   break;
               case 4:   glColor4dv( (GLdouble*) p );   break;
            }
            break;
         default:
            gl_problem(ctx, "Bad color type in gl_save_ArrayElement");
            return;
      }
   }

   if (ctx->Array.IndexEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.IndexPtr + i * ctx->Array.IndexStrideB;
      switch (ctx->Array.IndexType) {
         case GL_SHORT:
            glIndexsv( (GLshort*) p );
            break;
         case GL_INT:
            glIndexiv( (GLint*) p );
            break;
         case GL_FLOAT:
            glIndexfv( (GLfloat*) p );
            break;
         case GL_DOUBLE:
            glIndexdv( (GLdouble*) p );
            break;
         default:
            gl_problem(ctx, "Bad index type in gl_save_ArrayElement");
            return;
      }
   }

   if (ctx->Array.TexCoordEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.TexCoordPtr
                  + i * ctx->Array.TexCoordStrideB;
      switch (ctx->Array.TexCoordType) {
         case GL_SHORT:
            switch (ctx->Array.TexCoordSize) {
               case 1:   glTexCoord1sv( (GLshort*) p );   break;
               case 2:   glTexCoord2sv( (GLshort*) p );   break;
               case 3:   glTexCoord3sv( (GLshort*) p );   break;
               case 4:   glTexCoord4sv( (GLshort*) p );   break;
            }
            break;
         case GL_INT:
            switch (ctx->Array.TexCoordSize) {
               case 1:   glTexCoord1iv( (GLint*) p );   break;
               case 2:   glTexCoord2iv( (GLint*) p );   break;
               case 3:   glTexCoord3iv( (GLint*) p );   break;
               case 4:   glTexCoord4iv( (GLint*) p );   break;
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.TexCoordSize) {
               case 1:   glTexCoord1fv( (GLfloat*) p );   break;
               case 2:   glTexCoord2fv( (GLfloat*) p );   break;
               case 3:   glTexCoord3fv( (GLfloat*) p );   break;
               case 4:   glTexCoord4fv( (GLfloat*) p );   break;
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.TexCoordSize) {
               case 1:   glTexCoord1dv( (GLdouble*) p );   break;
               case 2:   glTexCoord2dv( (GLdouble*) p );   break;
               case 3:   glTexCoord3dv( (GLdouble*) p );   break;
               case 4:   glTexCoord4dv( (GLdouble*) p );   break;
            }
            break;
         default:
            gl_problem(ctx, "Bad texcoord type in gl_save_ArrayElement");
            return;
      }
   }

   if (ctx->Array.EdgeFlagEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.EdgeFlagPtr + i * ctx->Array.EdgeFlagStrideB;
      glEdgeFlagv( (GLboolean*) b );
   }

   if (ctx->Array.VertexEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.VertexPtr
                  + i * ctx->Array.VertexStrideB;
      switch (ctx->Array.VertexType) {
         case GL_SHORT:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2sv( (GLshort*) b );   break;
               case 3:   glVertex3sv( (GLshort*) b );   break;
               case 4:   glVertex4sv( (GLshort*) b );   break;
            }
            break;
         case GL_INT:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2iv( (GLint*) b );   break;
               case 3:   glVertex3iv( (GLint*) b );   break;
               case 4:   glVertex4iv( (GLint*) b );   break;
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2fv( (GLfloat*) b );   break;
               case 3:   glVertex3fv( (GLfloat*) b );   break;
               case 4:   glVertex4fv( (GLfloat*) b );   break;
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2dv( (GLdouble*) b );   break;
               case 3:   glVertex3dv( (GLdouble*) b );   break;
               case 4:   glVertex4dv( (GLdouble*) b );   break;
            }
            break;
         default:
            gl_problem(ctx, "Bad vertex type in gl_save_ArrayElement");
            return;
      }
   }
}



/*
 * Execute
 */
void gl_DrawArrays( GLcontext *ctx,
                    GLenum mode, GLint first, GLsizei count )
{
   struct vertex_buffer* VB = ctx->VB;

   GLint i;
   GLboolean need_edges;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawArrays" );
      return;
   }
   if (count<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return;
   }

   if (ctx->Primitive==GL_TRIANGLES || ctx->Primitive==GL_QUADS
       || ctx->Primitive==GL_POLYGON) {
      need_edges = GL_TRUE;
   }
   else {
      need_edges = GL_FALSE;
   }

   if (!ctx->Light.Enabled
       && !ctx->Texture.Enabled
       && ctx->Array.VertexEnabled && ctx->Array.VertexType==GL_FLOAT
       && ctx->Array.VertexStride==0 && ctx->Array.VertexSize==3
       && !ctx->Array.NormalEnabled
       && !ctx->Array.ColorEnabled
       && !ctx->Array.IndexEnabled
       && !ctx->Array.TexCoordEnabled
       && !ctx->Array.EdgeFlagEnabled) {
      /*
       * SPECIAL CASE:  glVertex3fv() with no lighting
       */
      GLfloat (*vptr)[3];
      GLint remaining;

      gl_Begin( ctx, mode );

      remaining = count;
      vptr = (GLfloat (*)[3]) ctx->Array.VertexPtr;
      vptr += 3 * first;
      while (remaining>0) {
         GLint vbspace, n;

         vbspace = VB_MAX - VB->Start;
         n = MIN2( vbspace, remaining );

         gl_xform_points_3fv( n, VB->Eye+VB->Start, ctx->ModelViewMatrix, vptr );

         /* assign vertex colors */
         {
            GLint i, start = VB->Start;
            for (i=0;i<n;i++) {
               COPY_4UBV( VB->Fcolor[start+i], ctx->Current.ByteColor );
            }
         }

         /* assign polygon edgeflags */
         if (need_edges) {
            GLint i;
            for (i=0;i<n;i++) {
               VB->Edgeflag[VB->Start+i] = ctx->Current.EdgeFlag;
            }
         }

         remaining -= n;

         VB->MonoNormal = GL_FALSE;
         VB->Count = VB->Start + n;
         gl_transform_vb_part2( ctx, remaining==0 ? GL_TRUE : GL_FALSE );

         vptr += n;
      }

      gl_End( ctx );
   }
   else if (!ctx->CompileFlag
       && ctx->Light.Enabled
       && !ctx->Texture.Enabled
       && ctx->Array.VertexEnabled && ctx->Array.VertexType==GL_FLOAT
       && ctx->Array.VertexStride==0 && ctx->Array.VertexSize==4
       && ctx->Array.NormalEnabled && ctx->Array.NormalType==GL_FLOAT
       && ctx->Array.NormalStride==0
       && !ctx->Array.ColorEnabled
       && !ctx->Array.IndexEnabled
       && !ctx->Array.TexCoordEnabled
       && !ctx->Array.EdgeFlagEnabled) {
      /*
       * SPECIAL CASE:  glNormal3fv();  glVertex4fv();  with lighting
       */
      GLfloat (*vptr)[4], (*nptr)[3];
      GLint remaining;

      gl_Begin( ctx, mode );

      remaining = count;
      vptr = (GLfloat (*)[4]) ctx->Array.VertexPtr;
      vptr += 4 * first;
      nptr = (GLfloat (*)[3]) ctx->Array.NormalPtr;
      nptr += 3 * first;
      while (remaining>0) {
         GLint vbspace, n;

         vbspace = VB_MAX - VB->Start;
         n = MIN2( vbspace, remaining );

         gl_xform_points_4fv( n, VB->Eye+VB->Start, ctx->ModelViewMatrix, vptr );
         gl_xform_normals_3fv( n, VB->Normal+VB->Start, ctx->ModelViewInv, nptr,
                               ctx->Transform.Normalize );

         /* assign polygon edgeflags */
         if (need_edges) {
            GLint i;
            for (i=0;i<n;i++) {
               VB->Edgeflag[VB->Start+i] = ctx->Current.EdgeFlag;
            }
         }

         remaining -= n;

         VB->MonoNormal = GL_FALSE;
         VB->Count = VB->Start + n;
         gl_transform_vb_part2( ctx, remaining==0 ? GL_TRUE : GL_FALSE );

         vptr += n;
         nptr += n;
      }

      gl_End( ctx );
   }
   else if (!ctx->CompileFlag
       && ctx->Light.Enabled
       && !ctx->Texture.Enabled
       && ctx->Array.VertexEnabled && ctx->Array.VertexType==GL_FLOAT
       && ctx->Array.VertexStride==0 && ctx->Array.VertexSize==3
       && ctx->Array.NormalEnabled && ctx->Array.NormalType==GL_FLOAT
       && ctx->Array.NormalStride==0
       && !ctx->Array.ColorEnabled
       && !ctx->Array.IndexEnabled
       && !ctx->Array.TexCoordEnabled
       && !ctx->Array.EdgeFlagEnabled) {
      /*
       * SPECIAL CASE:  glNormal3fv();  glVertex3fv();  with lighting
       */
      GLfloat (*vptr)[3], (*nptr)[3];
      GLint remaining;

      gl_Begin( ctx, mode );

      remaining = count;
      vptr = (GLfloat (*)[3]) ctx->Array.VertexPtr;
      vptr += 3 * first;
      nptr = (GLfloat (*)[3]) ctx->Array.NormalPtr;
      nptr += 3 * first;
      while (remaining>0) {
         GLint vbspace, n;

         vbspace = VB_MAX - VB->Start;
         n = MIN2( vbspace, remaining );

         gl_xform_points_3fv( n, VB->Eye+VB->Start, ctx->ModelViewMatrix, vptr );
         gl_xform_normals_3fv( n, VB->Normal+VB->Start, ctx->ModelViewInv, nptr,
                               ctx->Transform.Normalize );

         /* assign polygon edgeflags */
         if (need_edges) {
            GLint i;
            for (i=0;i<n;i++) {
               VB->Edgeflag[VB->Start+i] = ctx->Current.EdgeFlag;
            }
         }

         remaining -= n;

         VB->MonoNormal = GL_FALSE;
         VB->Count = VB->Start + n;
         gl_transform_vb_part2( ctx, remaining==0 ? GL_TRUE : GL_FALSE );

         vptr += n;
         nptr += n;
      }

      gl_End( ctx );
   }
   else {
      /*
       * GENERAL CASE:
       */
      gl_Begin( ctx, mode );
      for (i=0;i<count;i++) {
         gl_ArrayElement( ctx, first+i );
      }
      gl_End( ctx );
   }
}



/*
 * Save into a display list
 */
void gl_save_DrawArrays( GLcontext *ctx,
                         GLenum mode, GLint first, GLsizei count )
{
   GLint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawArrays" );
      return;
   }
   if (count<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return;
   }
   switch (mode) {
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
      case GL_QUADS:
      case GL_QUAD_STRIP:
      case GL_POLYGON:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
         return;
   }


   /* Note: this will do compile AND execute if needed */
   gl_save_Begin( ctx, mode );
   for (i=0;i<count;i++) {
      gl_save_ArrayElement( ctx, first+i );
   }
   gl_save_End( ctx );
}




/*
 * Execute only
 */
void gl_DrawElements( GLcontext *ctx,
                      GLenum mode, GLsizei count,
                      GLenum type, const GLvoid *indices )
{
   
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawElements" );
      return;
   }
   if (count<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDrawElements(count)" );
      return;
   }
   switch (mode) {
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
      case GL_QUADS:
      case GL_QUAD_STRIP:
      case GL_POLYGON:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
         return;
   }
   switch (type) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *ub_indices = (GLubyte *) indices;
            GLint i;
            gl_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_ArrayElement( ctx, (GLint) ub_indices[i] );
            }
            gl_End( ctx );
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *us_indices = (GLushort *) indices;
            GLint i;
            gl_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_ArrayElement( ctx, (GLint) us_indices[i] );
            }
            gl_End( ctx );
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *ui_indices = (GLuint *) indices;
            GLint i;
            gl_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_ArrayElement( ctx, (GLint) ui_indices[i] );
            }
            gl_End( ctx );
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
         return;
   }
}




/*
 * Save (and perhaps execute)
 */
void gl_save_DrawElements( GLcontext *ctx,
                           GLenum mode, GLsizei count,
                           GLenum type, const GLvoid *indices )
{
   switch (type) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *ub_indices = (GLubyte *) indices;
            GLint i;
            gl_save_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_save_ArrayElement( ctx, (GLint) ub_indices[i] );
            }
            gl_save_End( ctx );
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *us_indices = (GLushort *) indices;
            GLint i;
            gl_save_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_save_ArrayElement( ctx, (GLint) us_indices[i] );
            }
            gl_save_End( ctx );
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *ui_indices = (GLuint *) indices;
            GLint i;
            gl_save_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_save_ArrayElement( ctx, (GLint) ui_indices[i] );
            }
            gl_save_End( ctx );
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
         return;
   }
}




void gl_InterleavedArrays( GLcontext *ctx,
                           GLenum format, GLsizei stride,
                           const GLvoid *pointer )
{
   GLboolean tflag, cflag, nflag;  /* enable/disable flags */
   GLint tcomps, ccomps, vcomps;   /* components per texcoord, color, vertex */
   GLenum ctype;                   /* color type */
   GLint coffset, noffset, voffset;/* color, normal, vertex offsets */
   GLint defstride;                /* default stride */
   GLint c, f;

   f = sizeof(GLfloat);
   c = f * ((4*sizeof(GLubyte) + (f-1)) / f);

   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glInterleavedArrays(stride)" );
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
         gl_error( ctx, GL_INVALID_ENUM, "glInterleavedArrays(format)" );
         return;
   }

   if (stride==0) {
      stride = defstride;
   }

   gl_DisableClientState( ctx, GL_EDGE_FLAG_ARRAY );
   gl_DisableClientState( ctx, GL_INDEX_ARRAY );

   if (tflag) {
      gl_EnableClientState( ctx, GL_TEXTURE_COORD_ARRAY );
      gl_TexCoordPointer( ctx, tcomps, GL_FLOAT, stride, pointer );
   }
   else {
      gl_DisableClientState( ctx, GL_TEXTURE_COORD_ARRAY );
   }

   if (cflag) {
      gl_EnableClientState( ctx, GL_COLOR_ARRAY );
      gl_ColorPointer( ctx, ccomps, ctype, stride,
                       (GLubyte*) pointer + coffset );
   }
   else {
      gl_DisableClientState( ctx, GL_COLOR_ARRAY );
   }

   if (nflag) {
      gl_EnableClientState( ctx, GL_NORMAL_ARRAY );
      gl_NormalPointer( ctx, GL_FLOAT, stride,
                        (GLubyte*) pointer + noffset );
   }
   else {
      gl_DisableClientState( ctx, GL_NORMAL_ARRAY );
   }

   gl_EnableClientState( ctx, GL_VERTEX_ARRAY );
   gl_VertexPointer( ctx, vcomps, GL_FLOAT, stride,
                     (GLubyte *) pointer + voffset );
}



void gl_save_InterleavedArrays( GLcontext *ctx,
                                GLenum format, GLsizei stride,
                                const GLvoid *pointer )
{
   /* Just execute since client-side state changes aren't put in
    * display lists.
    */
   gl_InterleavedArrays( ctx, format, stride, pointer );
}


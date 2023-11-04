/* $Id: types.h,v 1.50 1998/02/03 23:45:36 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
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
 * $Log: types.h,v $
 * Revision 1.50  1998/02/03 23:45:36  brianp
 * added space parameter to clip interpolation functions
 *
 * Revision 1.49  1998/01/06 02:40:52  brianp
 * added DavidB's clipping interpolation optimization
 *
 * Revision 1.48  1997/12/31 06:10:03  brianp
 * added Henk Kok's texture validation optimization (AnyDirty flag)
 *
 * Revision 1.47  1997/12/06 18:06:50  brianp
 * moved several static display list vars into GLcontext
 *
 * Revision 1.46  1997/10/29 01:33:29  brianp
 * added DriverMgrCtx field to gl_context struct
 *
 * Revision 1.45  1997/10/29 01:29:09  brianp
 * added GL_EXT_point_parameters extension from Daniel Barrero
 *
 * Revision 1.44  1997/10/16 01:59:08  brianp
 * added GL_EXT_shared_texture_palette extension
 *
 * Revision 1.43  1997/10/16 01:13:03  brianp
 * removed teximage Dirty, DirtyPalette flags, mondello code
 *
 * Revision 1.42  1997/09/29 23:27:08  brianp
 * added Dirty and DriverData fields for texturing
 *
 * Revision 1.41  1997/09/27 00:13:44  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.40  1997/09/23 00:57:38  brianp
 * now using hash table for texture objects
 *
 * Revision 1.39  1997/09/22 02:33:58  brianp
 * display lists now implemented with hash table
 *
 * Revision 1.38  1997/08/13 01:30:55  brianp
 * LightTwoSide is now a GLboolean
 *
 * Revision 1.37  1997/08/01 02:23:27  brianp
 * removed unused gl_list_group struct
 *
 * Revision 1.36  1997/06/20 02:07:50  brianp
 * replaced Current.IntColor with Current.ByteColor
 * removed ColorShift field
 *
 * Revision 1.35  1997/06/20 02:02:37  brianp
 * added Color4ubv API pointer
 *
 * Revision 1.34  1997/05/31 16:57:14  brianp
 * added MESA_NO_RASTER env var support
 *
 * Revision 1.33  1997/05/28 04:06:03  brianp
 * implemented projection near/far value stack for Driver.NearFar() function
 *
 * Revision 1.32  1997/05/26 21:15:13  brianp
 * added Red/Green/Blue/AlphaBits to GLvisual struct
 *
 * Revision 1.31  1997/05/03 00:49:31  brianp
 * added SampleFunc, Current, and Dirty to gl_texture_object
 *
 * Revision 1.30  1997/05/01 02:07:35  brianp
 * added shared state variable NextFreeTextureName
 *
 * Revision 1.29  1997/04/24 01:50:53  brianp
 * optimized glColor3f, glColor3fv, glColor4fv
 *
 * Revision 1.28  1997/04/21 01:20:41  brianp
 * added MATRIX_2D_NO_ROT
 *
 * Revision 1.27  1997/04/20 16:18:15  brianp
 * added glOrtho and glFrustum API pointers
 *
 * Revision 1.26  1997/04/16 23:55:33  brianp
 * added optimized glTexCoord2f code
 *
 * Revision 1.25  1997/04/14 22:18:23  brianp
 * added optimized glVertex3fv code
 *
 * Revision 1.24  1997/04/14 02:03:05  brianp
 * added MinMagThresh to texture object
 *
 * Revision 1.23  1997/04/12 16:54:05  brianp
 * new NEW_POLYGON state flag
 *
 * Revision 1.22  1997/04/12 12:26:39  brianp
 * added quad_func and rect_func, removed polygon_func
 *
 * Revision 1.21  1997/04/07 02:57:13  brianp
 * added API.Vertex[23] functions
 *
 * Revision 1.20  1997/04/01 04:19:37  brianp
 * added API pointer for glLoadIdentity
 * added matrix type constants and fields
 *
 * Revision 1.19  1997/03/13 03:05:54  brianp
 * changed AlphaRefInt to AlphaRefUbyte
 *
 * Revision 1.18  1997/03/04 19:17:38  brianp
 * added a few members to gl_texture_image struct
 *
 * Revision 1.17  1997/02/10 19:49:29  brianp
 * added glResizeBuffersMESA() code
 *
 * Revision 1.16  1997/02/09 19:53:43  brianp
 * now use TEXTURE_xD enable constants
 *
 * Revision 1.15  1997/02/09 18:42:41  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.14  1997/01/28 22:14:36  brianp
 * now there's separate state for CI and RGBA logic op enabled
 *
 * Revision 1.13  1997/01/21 03:09:41  brianp
 * added definition:  union node;
 *
 * Revision 1.12  1997/01/09 21:26:23  brianp
 * added RefCount to gl_image struct
 *
 * Revision 1.11  1996/12/18 20:01:39  brianp
 * added material bitmask constants and ColorMaterialBitmask
 *
 * Revision 1.10  1996/12/11 20:16:30  brianp
 * changed GLaccum types to signed
 *
 * Revision 1.9  1996/11/08 02:21:04  brianp
 * added NoRaster flag to context
 *
 * Revision 1.8  1996/11/07 04:11:34  brianp
 * now pass gl_image struct pointer to glTexImage[12]D()
 *
 * Revision 1.7  1996/11/06 04:21:58  brianp
 * added Format to struct gl_image
 *
 * Revision 1.6  1996/10/11 03:41:12  brianp
 * removed OffsetBias field
 *
 * Revision 1.5  1996/10/11 00:23:04  brianp
 * changed Polygon/Line/PointZoffset to GLfloat
 *
 * Revision 1.4  1996/09/25 03:22:53  brianp
 * added NO_DRAW_BIT for glDrawBuffer(GL_NONE)
 *
 * Revision 1.3  1996/09/19 03:17:56  brianp
 * removed Window field from struct gl_frame_buffer
 *
 * Revision 1.2  1996/09/15 14:20:54  brianp
 * added GLframebuffer and GLvisual datatypes
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef TYPES_H
#define TYPES_H


#include "GL/gl.h"
#include "config.h"


struct HashTable;


/*
 * Accumulation buffer data type:
 */
#if ACCUM_BITS==8
   typedef GLbyte GLaccum;
#elif ACCUM_BITS==16
   typedef GLshort GLaccum;
#else
   illegal number of accumulation bits
#endif


/*
 * Stencil buffer data type:
 */
#if STENCIL_BITS==8
   typedef GLubyte GLstencil;
#else
   illegal number of stencil bits
#endif



/*
 * Depth buffer data type:
 */
typedef GLint GLdepth;



#include "fixed.h"



typedef struct gl_visual GLvisual;

typedef struct gl_context GLcontext;

typedef struct gl_frame_buffer GLframebuffer;



/*
 * Point, line, triangle, quadrilateral and rectangle rasterizer functions:
 */
typedef void (*points_func)( GLcontext *ctx, GLuint first, GLuint last );

typedef void (*line_func)( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv );

typedef void (*triangle_func)( GLcontext *ctx,
                               GLuint v1, GLuint v2, GLuint v3, GLuint pv );

typedef void (*quad_func)( GLcontext *ctx, GLuint v1, GLuint v2,
                           GLuint v3, GLuint v4, GLuint pv );

typedef void (*rect_func)( GLcontext *ctx, GLint x, GLint y,
                           GLint width, GLint height );

/*
 * The auxiliary interpolation function for clipping
 */
typedef void (* interp_func)(GLcontext *, GLuint, GLuint, GLfloat, GLuint, GLuint);


/*
 * For texture sampling:
 */
struct gl_texture_object;

typedef void (*TextureSampleFunc)( const struct gl_texture_object *tObj,
                                   GLuint n,
                                   const GLfloat s[], const GLfloat t[],
                                   const GLfloat u[], const GLfloat lambda[],
                                   GLubyte r[], GLubyte g[],
                                   GLubyte b[],GLubyte a[] );


/* Generic internal image format */
struct gl_image {
	GLint Width;
	GLint Height;
	GLint Components;	/* 1, 2, 3 or 4 */
        GLenum Format;		/* GL_COLOR_INDEX, GL_RED, GL_RGB, etc */
	GLenum Type;		/* GL_UNSIGNED_BYTE or GL_FLOAT or GL_BITMAP */
	GLvoid *Data;
	GLboolean Interleaved;  /* If TRUE and Format==GL_RGB, GL_RGBA or
                                 * GL_LUMINANCE_ALPHA then each row is
                                 * stored as RRR..RGGG..GBBB.B instead of
                                 * RGBRGBRGBRGB..RGB.  This is only used
                                 * for glDrawPixels.
                                 */
	GLint RefCount;
};



/* Texture image record */
struct gl_texture_image {
	GLenum Format;		/* GL_ALPHA, GL_LUMINANCE, GL_LUMINANCE_ALPHA,
				 * GL_INTENSITY, GL_RGB, GL_RGBA, or
				 * GL_COLOR_INDEX
				 */
	GLenum IntFormat;	/* Internal format as given by the user */
	GLuint Border;		/* 0 or 1 */
	GLuint Width;		/* = 2^WidthLog2 + 2*Border */
	GLuint Height;		/* = 2^HeightLog2 + 2*Border */
	GLuint Width2;		/* = Width - 2*Border */
	GLuint Height2;		/* = Height - 2*Border */
	GLuint WidthLog2;	/* = log2(Width2) */
	GLuint HeightLog2;	/* = log2(Height2) */
	GLuint MaxLog2;		/* = MAX(WidthLog2, HeightLog2) */
	GLubyte *Data;		/* Image data as unsigned bytes */

	/* For device driver: */
	void *DriverData;	/* Arbitrary device driver data */
};



/*
 * All gl* API functions in api.c jump through pointers in this struct.
 */
struct gl_api_table {
   void (*Accum)( GLcontext *, GLenum, GLfloat );
   void (*AlphaFunc)( GLcontext *, GLenum, GLclampf );
   GLboolean (*AreTexturesResident)( GLcontext *, GLsizei,
                                     const GLuint *, GLboolean * );
   void (*ArrayElement)( GLcontext *, GLint );
   void (*Begin)( GLcontext *, GLenum );
   void (*BindTexture)( GLcontext *, GLenum, GLuint );
   void (*Bitmap)( GLcontext *, GLsizei, GLsizei, GLfloat, GLfloat,
		     GLfloat, GLfloat, const struct gl_image *bitmap );
   void (*BlendFunc)( GLcontext *, GLenum, GLenum );
   void (*CallList)( GLcontext *, GLuint list );
   void (*CallLists)( GLcontext *, GLsizei, GLenum, const GLvoid * );
   void (*Clear)( GLcontext *, GLbitfield );
   void (*ClearAccum)( GLcontext *, GLfloat, GLfloat, GLfloat, GLfloat );
   void (*ClearColor)( GLcontext *, GLclampf, GLclampf, GLclampf, GLclampf );
   void (*ClearDepth)( GLcontext *, GLclampd );
   void (*ClearIndex)( GLcontext *, GLfloat );
   void (*ClearStencil)( GLcontext *, GLint );
   void (*ClipPlane)( GLcontext *, GLenum, const GLfloat * );
   void (*Color3f)( GLcontext *, GLfloat, GLfloat, GLfloat );
   void (*Color3fv)( GLcontext *, const GLfloat * );
   void (*Color4f)( GLcontext *, GLfloat, GLfloat, GLfloat, GLfloat );
   void (*Color4fv)( GLcontext *, const GLfloat * );
   void (*Color4ub)( GLcontext *, GLubyte, GLubyte, GLubyte, GLubyte );
   void (*Color4ubv)( GLcontext *, const GLubyte * );
   void (*ColorMask)( GLcontext *,
			GLboolean, GLboolean, GLboolean, GLboolean );
   void (*ColorMaterial)( GLcontext *, GLenum, GLenum );
   void (*ColorPointer)( GLcontext *, GLint, GLenum, GLsizei, const GLvoid * );
   void (*ColorTable)( GLcontext *, GLenum, GLenum, struct gl_image * );
   void (*ColorSubTable)( GLcontext *, GLenum, GLsizei, struct gl_image * );
   void (*CopyPixels)( GLcontext *, GLint, GLint, GLsizei, GLsizei, GLenum );
   void (*CopyTexImage1D)( GLcontext *, GLenum, GLint, GLenum,
                           GLint, GLint, GLsizei, GLint );
   void (*CopyTexImage2D)( GLcontext *, GLenum, GLint, GLenum,
                           GLint, GLint, GLsizei, GLsizei, GLint );
   void (*CopyTexSubImage1D)( GLcontext *, GLenum, GLint, GLint,
                              GLint, GLint, GLsizei );
   void (*CopyTexSubImage2D)( GLcontext *, GLenum, GLint, GLint, GLint,
                              GLint, GLint, GLsizei, GLsizei );
   void (*CullFace)( GLcontext *, GLenum );
   void (*DeleteLists)( GLcontext *, GLuint, GLsizei );
   void (*DeleteTextures)( GLcontext *, GLsizei, const GLuint *);
   void (*DepthFunc)( GLcontext *, GLenum );
   void (*DepthMask)( GLcontext *, GLboolean );
   void (*DepthRange)( GLcontext *, GLclampd, GLclampd );
   void (*Disable)( GLcontext *, GLenum );
   void (*DisableClientState)( GLcontext *, GLenum );
   void (*DrawArrays)( GLcontext *, GLenum, GLint, GLsizei );
   void (*DrawBuffer)( GLcontext *, GLenum );
   void (*DrawElements)( GLcontext *, GLenum, GLsizei, GLenum, const GLvoid *);
   void (*DrawPixels)( GLcontext *,
                       GLsizei, GLsizei, GLenum, GLenum, const GLvoid * );
   void (*EdgeFlag)( GLcontext *, GLboolean );
   void (*EdgeFlagPointer)( GLcontext *, GLsizei, const GLboolean * );
   void (*Enable)( GLcontext *, GLenum );
   void (*EnableClientState)( GLcontext *, GLenum );
   void (*End)( GLcontext * );
   void (*EndList)( GLcontext * );
   void (*EvalCoord1f)( GLcontext *, GLfloat );
   void (*EvalCoord2f)( GLcontext *, GLfloat , GLfloat );
   void (*EvalMesh1)( GLcontext *, GLenum, GLint, GLint );
   void (*EvalMesh2)( GLcontext *, GLenum, GLint, GLint, GLint, GLint );
   void (*EvalPoint1)( GLcontext *, GLint );
   void (*EvalPoint2)( GLcontext *, GLint, GLint );
   void (*FeedbackBuffer)( GLcontext *, GLsizei, GLenum, GLfloat * );
   void (*Finish)( GLcontext * );
   void (*Flush)( GLcontext * );
   void (*Fogfv)( GLcontext *, GLenum, const GLfloat * );
   void (*FrontFace)( GLcontext *, GLenum );
   void (*Frustum)( GLcontext *, GLdouble, GLdouble, GLdouble, GLdouble,
                    GLdouble, GLdouble );
   GLuint (*GenLists)( GLcontext *, GLsizei );
   void (*GenTextures)( GLcontext *, GLsizei, GLuint * );
   void (*GetBooleanv)( GLcontext *, GLenum, GLboolean * );
   void (*GetClipPlane)( GLcontext *, GLenum, GLdouble * );
   void (*GetColorTable)( GLcontext *, GLenum, GLenum, GLenum, GLvoid *);
   void (*GetColorTableParameteriv)( GLcontext *, GLenum, GLenum, GLint *);
   void (*GetDoublev)( GLcontext *, GLenum, GLdouble * );
   GLenum (*GetError)( GLcontext * );
   void (*GetFloatv)( GLcontext *, GLenum, GLfloat * );
   void (*GetIntegerv)( GLcontext *, GLenum, GLint * );
   const GLubyte* (*GetString)( GLcontext *, GLenum name );
   void (*GetLightfv)( GLcontext *, GLenum light, GLenum, GLfloat * );
   void (*GetLightiv)( GLcontext *, GLenum light, GLenum, GLint * );
   void (*GetMapdv)( GLcontext *, GLenum, GLenum, GLdouble * );
   void (*GetMapfv)( GLcontext *, GLenum, GLenum, GLfloat * );
   void (*GetMapiv)( GLcontext *, GLenum, GLenum, GLint * );
   void (*GetMaterialfv)( GLcontext *, GLenum, GLenum, GLfloat * );
   void (*GetMaterialiv)( GLcontext *, GLenum, GLenum, GLint * );
   void (*GetPixelMapfv)( GLcontext *, GLenum, GLfloat * );
   void (*GetPixelMapuiv)( GLcontext *, GLenum, GLuint * );
   void (*GetPixelMapusv)( GLcontext *, GLenum, GLushort * );
   void (*GetPointerv)( GLcontext *, GLenum, GLvoid ** );
   void (*GetPolygonStipple)( GLcontext *, GLubyte * );
   void (*PrioritizeTextures)( GLcontext *, GLsizei, const GLuint *,
                               const GLclampf * );
   void (*GetTexEnvfv)( GLcontext *, GLenum, GLenum, GLfloat * );
   void (*GetTexEnviv)( GLcontext *, GLenum, GLenum, GLint * );
   void (*GetTexGendv)( GLcontext *, GLenum coord, GLenum, GLdouble * );
   void (*GetTexGenfv)( GLcontext *, GLenum coord, GLenum, GLfloat * );
   void (*GetTexGeniv)( GLcontext *, GLenum coord, GLenum, GLint * );
   void (*GetTexImage)( GLcontext *, GLenum, GLint level, GLenum, GLenum,
                        GLvoid * );
   void (*GetTexLevelParameterfv)( GLcontext *,
				     GLenum, GLint, GLenum, GLfloat * );
   void (*GetTexLevelParameteriv)( GLcontext *,
				     GLenum, GLint, GLenum, GLint * );
   void (*GetTexParameterfv)( GLcontext *, GLenum, GLenum, GLfloat *);
   void (*GetTexParameteriv)( GLcontext *, GLenum, GLenum, GLint * );
   void (*Hint)( GLcontext *, GLenum, GLenum );
   void (*IndexMask)( GLcontext *, GLuint );
   void (*Indexf)( GLcontext *, GLfloat c );
   void (*Indexi)( GLcontext *, GLint c );
   void (*IndexPointer)( GLcontext *, GLenum, GLsizei, const GLvoid * );
   void (*InitNames)( GLcontext * );
   void (*InterleavedArrays)( GLcontext *, GLenum, GLsizei, const GLvoid * );
   GLboolean (*IsEnabled)( GLcontext *, GLenum );
   GLboolean (*IsList)( GLcontext *, GLuint );
   GLboolean (*IsTexture)( GLcontext *, GLuint );
   void (*LightModelfv)( GLcontext *, GLenum, const GLfloat * );
   void (*Lightfv)( GLcontext *, GLenum light, GLenum, const GLfloat *, GLint);
   void (*LineStipple)( GLcontext *, GLint factor, GLushort );
   void (*LineWidth)( GLcontext *, GLfloat );
   void (*ListBase)( GLcontext *, GLuint );
   void (*LoadIdentity)( GLcontext * );
   /* LoadMatrixd implemented with glLoadMatrixf */
   void (*LoadMatrixf)( GLcontext *, const GLfloat * );
   void (*LoadName)( GLcontext *, GLuint );
   void (*LogicOp)( GLcontext *, GLenum );
   void (*Map1f)( GLcontext *, GLenum, GLfloat, GLfloat, GLint, GLint,
		  const GLfloat *, GLboolean );
   void (*Map2f)( GLcontext *, GLenum, GLfloat, GLfloat, GLint, GLint,
		  GLfloat, GLfloat, GLint, GLint, const GLfloat *,
		  GLboolean );
   void (*MapGrid1f)( GLcontext *, GLint, GLfloat, GLfloat );
   void (*MapGrid2f)( GLcontext *, GLint, GLfloat, GLfloat,
			GLint, GLfloat, GLfloat );
   void (*Materialfv)( GLcontext *, GLenum, GLenum, const GLfloat * );
   void (*MatrixMode)( GLcontext *, GLenum );
   /* MultMatrixd implemented with glMultMatrixf */
   void (*MultMatrixf)( GLcontext *, const GLfloat * );
   void (*NewList)( GLcontext *, GLuint list, GLenum );
   void (*Normal3f)( GLcontext *, GLfloat, GLfloat, GLfloat );
   void (*Normal3fv)( GLcontext *, const GLfloat * );
   void (*NormalPointer)( GLcontext *, GLenum, GLsizei, const GLvoid * );
   void (*Ortho)( GLcontext *, GLdouble, GLdouble, GLdouble, GLdouble,
                  GLdouble, GLdouble );
   void (*PassThrough)( GLcontext *, GLfloat );
   void (*PixelMapfv)( GLcontext *, GLenum, GLint, const GLfloat * );
   void (*PixelStorei)( GLcontext *, GLenum, GLint );
   void (*PixelTransferf)( GLcontext *, GLenum, GLfloat );
   void (*PixelZoom)( GLcontext *, GLfloat, GLfloat );
   void (*PointSize)( GLcontext *, GLfloat );
   void (*PolygonMode)( GLcontext *, GLenum, GLenum );
   void (*PolygonOffset)( GLcontext *, GLfloat, GLfloat );
   void (*PolygonStipple)( GLcontext *, const GLubyte * );
   void (*PopAttrib)( GLcontext * );
   void (*PopClientAttrib)( GLcontext * );
   void (*PopMatrix)( GLcontext * );
   void (*PopName)( GLcontext * );
   void (*PushAttrib)( GLcontext *, GLbitfield );
   void (*PushClientAttrib)( GLcontext *, GLbitfield );
   void (*PushMatrix)( GLcontext * );
   void (*PushName)( GLcontext *, GLuint );
   void (*RasterPos4f)( GLcontext *,
                        GLfloat x, GLfloat y, GLfloat z, GLfloat w );
   void (*ReadBuffer)( GLcontext *, GLenum );
   void (*ReadPixels)( GLcontext *, GLint, GLint, GLsizei, GLsizei, GLenum,
			 GLenum, GLvoid * );
   void (*Rectf)( GLcontext *, GLfloat, GLfloat, GLfloat, GLfloat );
   GLint (*RenderMode)( GLcontext *, GLenum );
   void (*Rotatef)( GLcontext *, GLfloat, GLfloat, GLfloat, GLfloat );
   void (*Scalef)( GLcontext *, GLfloat, GLfloat, GLfloat );
   void (*Scissor)( GLcontext *, GLint, GLint, GLsizei, GLsizei);
   void (*SelectBuffer)( GLcontext *, GLsizei, GLuint * );
   void (*ShadeModel)( GLcontext *, GLenum );
   void (*StencilFunc)( GLcontext *, GLenum, GLint, GLuint );
   void (*StencilMask)( GLcontext *, GLuint );
   void (*StencilOp)( GLcontext *, GLenum, GLenum, GLenum );
   void (*TexCoord2f)( GLcontext *, GLfloat, GLfloat );
   void (*TexCoord4f)( GLcontext *, GLfloat, GLfloat, GLfloat, GLfloat );
   void (*TexCoordPointer)( GLcontext *, GLint, GLenum, GLsizei,
                            const GLvoid *);
   void (*TexEnvfv)( GLcontext *, GLenum, GLenum, const GLfloat * );
   void (*TexGenfv)( GLcontext *, GLenum coord, GLenum, const GLfloat * );
   void (*TexImage1D)( GLcontext *, GLenum, GLint, GLint, GLsizei,
                       GLint, GLenum, GLenum, struct gl_image * );
   void (*TexImage2D)( GLcontext *, GLenum, GLint, GLint, GLsizei, GLsizei,
                       GLint, GLenum, GLenum, struct gl_image * );
   void (*TexSubImage1D)( GLcontext *, GLenum, GLint, GLint, GLsizei,
                          GLenum, GLenum, struct gl_image * );
   void (*TexSubImage2D)( GLcontext *, GLenum, GLint, GLint, GLint,
                          GLsizei, GLsizei, GLenum, GLenum,
                          struct gl_image * );
   void (*TexParameterfv)( GLcontext *, GLenum, GLenum, const GLfloat * );
   /* Translated implemented by Translatef */
   void (*Translatef)( GLcontext *, GLfloat, GLfloat, GLfloat );
   void (*Vertex2f)( GLcontext *, GLfloat, GLfloat );
   void (*Vertex3f)( GLcontext *, GLfloat, GLfloat, GLfloat );
   void (*Vertex4f)( GLcontext *, GLfloat, GLfloat, GLfloat, GLfloat );
   void (*Vertex3fv)( GLcontext *, const GLfloat * );
   void (*VertexPointer)( GLcontext *, GLint, GLenum, GLsizei, const GLvoid *);
   void (*Viewport)( GLcontext *, GLint, GLint, GLsizei, GLsizei );
};



#include "dd.h"


/*
 * Bit flags used for updating material values.
 */
#define FRONT_AMBIENT_BIT     0x1
#define BACK_AMBIENT_BIT      0x2
#define FRONT_DIFFUSE_BIT     0x4
#define BACK_DIFFUSE_BIT      0x8
#define FRONT_SPECULAR_BIT   0x10
#define BACK_SPECULAR_BIT    0x20
#define FRONT_EMISSION_BIT   0x40
#define BACK_EMISSION_BIT    0x80
#define FRONT_SHININESS_BIT 0x100
#define BACK_SHININESS_BIT  0x200
#define FRONT_INDEXES_BIT   0x400
#define BACK_INDEXES_BIT    0x800

#define FRONT_MATERIAL_BITS	(FRONT_EMISSION_BIT | FRONT_AMBIENT_BIT | \
				 FRONT_DIFFUSE_BIT | FRONT_SPECULAR_BIT | \
				 FRONT_SHININESS_BIT | FRONT_INDEXES_BIT)

#define BACK_MATERIAL_BITS	(BACK_EMISSION_BIT | BACK_AMBIENT_BIT | \
				 BACK_DIFFUSE_BIT | BACK_SPECULAR_BIT | \
				 BACK_SHININESS_BIT | BACK_INDEXES_BIT)

#define ALL_MATERIAL_BITS	(FRONT_MATERIAL_BITS | BACK_MATERIAL_BITS)



/*
 * Specular exponent and material shininess lookup table sizes:
 */
#define EXP_TABLE_SIZE 512
#define SHINE_TABLE_SIZE 200

struct gl_light {
	GLfloat Ambient[4];		/* ambient color */
	GLfloat Diffuse[4];		/* diffuse color */
	GLfloat Specular[4];		/* specular color */
	GLfloat Position[4];		/* position in eye coordinates */
	GLfloat Direction[4];		/* spotlight dir in eye coordinates */
	GLfloat SpotExponent;
	GLfloat SpotCutoff;		/* in degress */
        GLfloat CosCutoff;		/* = cos(SpotCutoff) */
	GLfloat ConstantAttenuation;
	GLfloat LinearAttenuation;
	GLfloat QuadraticAttenuation;
	GLboolean Enabled;		/* On/off flag */

	struct gl_light *NextEnabled;	/* Ptr to next enabled light or NULL */

	/* Derived fields */
	GLfloat VP_inf_norm[3];		/* Norm direction to infinite light */
	GLfloat h_inf_norm[3];		/* Norm( VP_inf_norm + <0,0,1> ) */
        GLfloat NormDirection[3];	/* normalized spotlight direction */
        GLfloat SpotExpTable[EXP_TABLE_SIZE][2];  /* to replace a pow() call */
	GLfloat MatAmbient[2][3];	/* material ambient * light ambient */
	GLfloat MatDiffuse[2][3];	/* material diffuse * light diffuse */
	GLfloat MatSpecular[2][3];	/* material spec * light specular */
	GLfloat dli;			/* CI diffuse light intensity */
	GLfloat sli;			/* CI specular light intensity */
};


struct gl_lightmodel {
	GLfloat Ambient[4];		/* ambient color */
	GLboolean LocalViewer;		/* Local (or infinite) view point? */
	GLboolean TwoSide;		/* Two (or one) sided lighting? */
};


struct gl_material {
	GLfloat Ambient[4];
	GLfloat Diffuse[4];
	GLfloat Specular[4];
	GLfloat Emission[4];
	GLfloat Shininess;
	GLfloat AmbientIndex;	/* for color index lighting */
	GLfloat DiffuseIndex;	/* for color index lighting */
	GLfloat SpecularIndex;	/* for color index lighting */
        GLfloat ShineTable[SHINE_TABLE_SIZE];  /* to replace a pow() call */
};



/*
 * Attribute structures:
 *    We define a struct for each attribute group to make pushing and
 *    popping attributes easy.  Also it's a good organization.
 */


struct gl_accum_attrib {
	GLfloat ClearColor[4];	/* Accumulation buffer clear color */
};


struct gl_colorbuffer_attrib {
	GLuint ClearIndex;		/* Index to use for glClear */
	GLfloat ClearColor[4];		/* Color to use for glClear */

	GLuint IndexMask;		/* Color index write mask */
	GLuint ColorMask;		/* bit 3=red,2=green,1=blue,0=alpha*/
        GLboolean SWmasking;		/* Do color/CI masking in software? */

	GLenum DrawBuffer;		/* Which buffer to draw into */

	/* alpha testing */
	GLboolean AlphaEnabled;		/* Alpha test enabled flag */
	GLenum AlphaFunc;		/* Alpha test function */
	GLfloat AlphaRef;		/* Alpha reference value */
	GLubyte AlphaRefUbyte;		/* AlphaRef scaled to an integer */

	/* blending */
	GLboolean BlendEnabled;		/* Blending enabled flag */
	GLenum BlendSrc;		/* Blending source operator */
	GLenum BlendDst;		/* Blending destination operator */

	/* logic op */
	GLenum LogicOp;			/* Logic operator */
	GLboolean IndexLogicOpEnabled;	/* Color index logic op enabled flag */
	GLboolean ColorLogicOpEnabled;	/* RGBA logic op enabled flag */
	GLboolean SWLogicOpEnabled;	/* Do logic ops in software? */

	GLboolean DitherFlag;		/* Dither enable flag */
};


struct gl_current_attrib {
	GLubyte ByteColor[4];		/* Current RGBA color */
	GLuint Index;			/* Current color index */
	GLfloat Normal[3];		/* Current normal vector */
	GLfloat TexCoord[4];		/* Current texture coordinate */
	GLfloat RasterPos[4];		/* Current raster position */
	GLfloat RasterDistance;		/* Current raster distance */
	GLfloat RasterColor[4];		/* Current raster color */
	GLuint RasterIndex;		/* Current raster index */
	GLfloat RasterTexCoord[4];	/* Current raster texture coord */
	GLboolean RasterPosValid;	/* Raster position valid flag */
	GLboolean EdgeFlag;		/* Current edge flag */
};


struct gl_depthbuffer_attrib {
	GLenum Func;		/* Function for depth buffer compare */
	GLfloat Clear;		/* Value to clear depth buffer to */
	GLboolean Test;		/* Depth buffering enabled flag */
	GLboolean Mask;		/* Depth buffer writable? */
};


struct gl_enable_attrib {
	GLboolean AlphaTest;
	GLboolean AutoNormal;
	GLboolean Blend;
	GLboolean ClipPlane[MAX_CLIP_PLANES];
	GLboolean ColorMaterial;
	GLboolean CullFace;
	GLboolean DepthTest;
	GLboolean Dither;
	GLboolean Fog;
	GLboolean Light[MAX_LIGHTS];
	GLboolean Lighting;
	GLboolean LineSmooth;
	GLboolean LineStipple;
	GLboolean IndexLogicOp;
	GLboolean ColorLogicOp;
	GLboolean Map1Color4;
	GLboolean Map1Index;
	GLboolean Map1Normal;
	GLboolean Map1TextureCoord1;
	GLboolean Map1TextureCoord2;
	GLboolean Map1TextureCoord3;
	GLboolean Map1TextureCoord4;
	GLboolean Map1Vertex3;
	GLboolean Map1Vertex4;
	GLboolean Map2Color4;
	GLboolean Map2Index;
	GLboolean Map2Normal;
	GLboolean Map2TextureCoord1;
	GLboolean Map2TextureCoord2;
	GLboolean Map2TextureCoord3;
	GLboolean Map2TextureCoord4;
	GLboolean Map2Vertex3;
	GLboolean Map2Vertex4;
	GLboolean Normalize;
	GLboolean PointSmooth;
	GLboolean PolygonOffsetPoint;
	GLboolean PolygonOffsetLine;
	GLboolean PolygonOffsetFill;
	GLboolean PolygonSmooth;
	GLboolean PolygonStipple;
	GLboolean Scissor;
	GLboolean Stencil;
	GLuint Texture;
	GLuint TexGen;
};


struct gl_eval_attrib {
	/* Enable bits */
	GLboolean Map1Color4;
	GLboolean Map1Index;
	GLboolean Map1Normal;
	GLboolean Map1TextureCoord1;
	GLboolean Map1TextureCoord2;
	GLboolean Map1TextureCoord3;
	GLboolean Map1TextureCoord4;
	GLboolean Map1Vertex3;
	GLboolean Map1Vertex4;
	GLboolean Map2Color4;
	GLboolean Map2Index;
	GLboolean Map2Normal;
	GLboolean Map2TextureCoord1;
	GLboolean Map2TextureCoord2;
	GLboolean Map2TextureCoord3;
	GLboolean Map2TextureCoord4;
	GLboolean Map2Vertex3;
	GLboolean Map2Vertex4;
	GLboolean AutoNormal;
	/* Map Grid endpoints and divisions */
	GLuint MapGrid1un;
	GLfloat MapGrid1u1, MapGrid1u2;
	GLuint MapGrid2un, MapGrid2vn;
	GLfloat MapGrid2u1, MapGrid2u2;
	GLfloat MapGrid2v1, MapGrid2v2;
};


struct gl_fog_attrib {
	GLboolean Enabled;		/* Fog enabled flag */
	GLfloat Color[4];		/* Fog color */
	GLfloat Density;		/* Density >= 0.0 */
	GLfloat Start;			/* Start distance in eye coords */
	GLfloat End;			/* End distance in eye coords */
	GLfloat Index;			/* Fog index */
	GLenum Mode;			/* Fog mode */
};


struct gl_hint_attrib {
	/* always one of GL_FASTEST, GL_NICEST, or GL_DONT_CARE */
	GLenum PerspectiveCorrection;
	GLenum PointSmooth;
	GLenum LineSmooth;
	GLenum PolygonSmooth;
	GLenum Fog;
};


struct gl_light_attrib {
   struct gl_light Light[MAX_LIGHTS];	/* Array of lights */
   struct gl_lightmodel Model;		/* Lighting model */
   struct gl_material Material[2];	/* Material 0=front, 1=back */
   GLboolean Enabled;			/* Lighting enabled flag */
   GLenum ShadeModel;			/* GL_FLAT or GL_SMOOTH */
   GLenum ColorMaterialFace;		/* GL_FRONT, BACK or FRONT_AND_BACK */
   GLenum ColorMaterialMode;		/* GL_AMBIENT, GL_DIFFUSE, etc */
   GLuint ColorMaterialBitmask;		/* bitmask formed from Face and Mode */
   GLboolean ColorMaterialEnabled;

   /* Derived for optimizations: */
   struct gl_light *FirstEnabled;	/* Ptr to 1st enabled light */
   GLboolean Fast;			/* Use fast shader? */
   GLfloat BaseColor[2][4];
};


struct gl_line_attrib {
	GLboolean SmoothFlag;		/* GL_LINE_SMOOTH enabled? */
	GLboolean StippleFlag;		/* GL_LINE_STIPPLE enabled? */
	GLushort StipplePattern;	/* Stipple pattern */
	GLint StippleFactor;		/* Stipple repeat factor */
	GLfloat Width;			/* Line width */
};


struct gl_list_attrib {
	GLuint ListBase;
};


struct gl_pixel_attrib {
	GLenum ReadBuffer;
	GLfloat RedBias, RedScale;	/* Pixel xfer bias & scale values */
	GLfloat GreenBias, GreenScale;
	GLfloat BlueBias, BlueScale;
	GLfloat AlphaBias, AlphaScale;
	GLfloat DepthBias, DepthScale;
	GLint IndexShift;
	GLint IndexOffset;
	GLboolean MapColorFlag;
	GLboolean MapStencilFlag;
	GLfloat ZoomX;			/* Pixel zoom X factor */
	GLfloat ZoomY;			/* Pixel zoom Y factor */
	/* TODO: Do the following belong here??? */
	GLint MapStoSsize;			/* Size of each pixel map */
	GLint MapItoIsize;
	GLint MapItoRsize;
	GLint MapItoGsize;
	GLint MapItoBsize;
	GLint MapItoAsize;
	GLint MapRtoRsize;
	GLint MapGtoGsize;
	GLint MapBtoBsize;
	GLint MapAtoAsize;
	GLint MapStoS[MAX_PIXEL_MAP_TABLE];	/* Pixel map tables */
	GLint MapItoI[MAX_PIXEL_MAP_TABLE];
	GLfloat MapItoR[MAX_PIXEL_MAP_TABLE];
	GLfloat MapItoG[MAX_PIXEL_MAP_TABLE];
	GLfloat MapItoB[MAX_PIXEL_MAP_TABLE];
	GLfloat MapItoA[MAX_PIXEL_MAP_TABLE];
	GLfloat MapRtoR[MAX_PIXEL_MAP_TABLE];
	GLfloat MapGtoG[MAX_PIXEL_MAP_TABLE];
	GLfloat MapBtoB[MAX_PIXEL_MAP_TABLE];
	GLfloat MapAtoA[MAX_PIXEL_MAP_TABLE];
};


struct gl_point_attrib {
	GLboolean SmoothFlag;	/* True if GL_POINT_SMOOTH is enabled */
	GLfloat Size;		/* Point size */
};


struct gl_polygon_attrib {
	GLenum FrontFace;	/* Either GL_CW or GL_CCW */
	GLenum FrontMode;	/* Either GL_POINT, GL_LINE or GL_FILL */
	GLenum BackMode;	/* Either GL_POINT, GL_LINE or GL_FILL */
	GLboolean Unfilled;	/* True if back or front mode is not GL_FILL */
	GLboolean CullFlag;	/* Culling on/off flag */
	GLenum CullFaceMode;	/* Culling mode GL_FRONT or GL_BACK */
        GLuint CullBits;	/* Used for cull testing */
	GLboolean SmoothFlag;	/* True if GL_POLYGON_SMOOTH is enabled */
	GLboolean StippleFlag;	/* True if GL_POLYGON_STIPPLE is enabled */
        GLfloat OffsetFactor;	/* Polygon offset factor */
        GLfloat OffsetUnits;	/* Polygon offset units */
        GLboolean OffsetPoint;	/* Offset in GL_POINT mode? */
        GLboolean OffsetLine;	/* Offset in GL_LINE mode? */
        GLboolean OffsetFill;	/* Offset in GL_FILL mode? */
        GLboolean OffsetAny;	/* OR of OffsetPoint, OffsetLine, OffsetFill */
};


struct gl_scissor_attrib {
	GLboolean	Enabled;		/* Scissor test enabled? */
	GLint		X, Y;			/* Lower left corner of box */
	GLsizei		Width, Height;		/* Size of box */
};


struct gl_stencil_attrib {
	GLboolean	Enabled;	/* Enabled flag */
	GLenum		Function;	/* Stencil function */
	GLenum		FailFunc;	/* Fail function */
	GLenum		ZPassFunc;	/* Depth buffer pass function */
	GLenum		ZFailFunc;	/* Depth buffer fail function */
	GLstencil	Ref;		/* Reference value */
	GLstencil	ValueMask;	/* Value mask */
	GLstencil	Clear;		/* Clear value */
	GLstencil	WriteMask;	/* Write mask */
};


#define Q_BIT 1
#define R_BIT 2
#define S_BIT 4
#define T_BIT 8

#define TEXTURE_1D 1
#define TEXTURE_2D 2


struct gl_texture_attrib {
	GLuint Enabled;			/* Bitwise-OR of TEXTURE_XD values */
	GLenum EnvMode;			/* GL_MODULATE, GL_DECAL, GL_BLEND */
	GLfloat EnvColor[4];
	GLuint TexGenEnabled;		/* Bitwise-OR of [QRST]_BIT values */
	GLenum GenModeS;		/* Tex coord generation mode, either */
	GLenum GenModeT;		/*	GL_OBJECT_LINEAR, or */
	GLenum GenModeR;		/*	GL_EYE_LINEAR, or    */
	GLenum GenModeQ;		/*      GL_SPHERE_MAP        */
	GLfloat ObjectPlaneS[4];
	GLfloat ObjectPlaneT[4];
	GLfloat ObjectPlaneR[4];
	GLfloat ObjectPlaneQ[4];
	GLfloat EyePlaneS[4];
	GLfloat EyePlaneT[4];
	GLfloat EyePlaneR[4];
	GLfloat EyePlaneQ[4];
	struct gl_texture_object *Current1D;
	struct gl_texture_object *Current2D;
	struct gl_texture_object *Current;  /* = Current1D, 2D or NULL */
#ifdef GL_VERSION_1_1
	struct gl_texture_object *Proxy1D;
	struct gl_texture_object *Proxy2D;
#endif
	GLboolean AnyDirty;
};


struct gl_transform_attrib {
	GLenum MatrixMode;			/* Matrix mode */
	GLfloat ClipEquation[MAX_CLIP_PLANES][4];
	GLboolean ClipEnabled[MAX_CLIP_PLANES];
	GLboolean AnyClip;			/* Any ClipEnabled[] true? */
	GLboolean Normalize;			/* Normalize all normals? */
};


struct gl_viewport_attrib {
	GLint X, Y;		/* position */
	GLsizei Width, Height;	/* size */
	GLfloat Near, Far;	/* Depth buffer range */
	GLfloat Sx, Sy, Sz;	/* NDC to WinCoord scaling */
	GLfloat Tx, Ty, Tz;	/* NDC to WinCoord translation */
};


/* For the attribute stack: */
struct gl_attrib_node {
	GLbitfield kind;
	void *data;
	struct gl_attrib_node *next;
};



/*
 * Client pixel packing/unpacking attributes
 */
struct gl_pixelstore_attrib {
	GLint Alignment;
	GLint RowLength;
	GLint SkipPixels;
	GLint SkipRows;
	GLboolean SwapBytes;
	GLboolean LsbFirst;
};


/*
 * Client vertex array attributes
 */
struct gl_array_attrib {
	GLint VertexSize;
	GLenum VertexType;
	GLsizei VertexStride;		/* user-specified stride */
	GLsizei VertexStrideB;		/* actual stride in bytes */
	void *VertexPtr;
	GLboolean VertexEnabled;

	GLenum NormalType;
	GLsizei NormalStride;		/* user-specified stride */
	GLsizei NormalStrideB;		/* actual stride in bytes */
	void *NormalPtr;
	GLboolean NormalEnabled;

	GLint ColorSize;
	GLenum ColorType;
	GLsizei ColorStride;		/* user-specified stride */
	GLsizei ColorStrideB;		/* actual stride in bytes */
	void *ColorPtr;
	GLboolean ColorEnabled;

	GLenum IndexType;
	GLsizei IndexStride;		/* user-specified stride */
	GLsizei IndexStrideB;		/* actual stride in bytes */
	void *IndexPtr;
	GLboolean IndexEnabled;

	GLint TexCoordSize;
	GLenum TexCoordType;
	GLsizei TexCoordStride;		/* user-specified stride */
	GLsizei TexCoordStrideB;	/* actual stride in bytes */
	void *TexCoordPtr;
	GLboolean TexCoordEnabled;

	GLsizei EdgeFlagStride;		/* user-specified stride */
	GLsizei EdgeFlagStrideB;	/* actual stride in bytes */
	GLboolean *EdgeFlagPtr;
	GLboolean EdgeFlagEnabled;
};



struct gl_feedback {
	GLenum Type;
	GLuint Mask;
	GLfloat *Buffer;
	GLuint BufferSize;
	GLuint Count;
};



struct gl_selection {
	GLuint *Buffer;
	GLuint BufferSize;	/* size of SelectBuffer */
	GLuint BufferCount;	/* number of values in SelectBuffer */
	GLuint Hits;		/* number of records in SelectBuffer */
	GLuint NameStackDepth;
	GLuint NameStack[MAX_NAME_STACK_DEPTH];
	GLboolean HitFlag;
	GLfloat HitMinZ, HitMaxZ;
};



/*
 * 1-D Evaluator control points
 */
struct gl_1d_map {
	GLuint Order;		/* Number of control points */
	GLfloat u1, u2;
	GLfloat *Points;	/* Points to contiguous control points */
	GLboolean Retain;	/* Reference counter */
};
	

/*
 * 2-D Evaluator control points
 */
struct gl_2d_map {
	GLuint Uorder;		/* Number of control points in U dimension */
	GLuint Vorder;		/* Number of control points in V dimension */
	GLfloat u1, u2;
	GLfloat v1, v2;
	GLfloat *Points;	/* Points to contiguous control points */
	GLboolean Retain;	/* Reference counter */
};


/*
 * All evalutator control points
 */
struct gl_evaluators {
	/* 1-D maps */
	struct gl_1d_map Map1Vertex3;
	struct gl_1d_map Map1Vertex4;
	struct gl_1d_map Map1Index;
	struct gl_1d_map Map1Color4;
	struct gl_1d_map Map1Normal;
	struct gl_1d_map Map1Texture1;
	struct gl_1d_map Map1Texture2;
	struct gl_1d_map Map1Texture3;
	struct gl_1d_map Map1Texture4;

	/* 2-D maps */
	struct gl_2d_map Map2Vertex3;
	struct gl_2d_map Map2Vertex4;
	struct gl_2d_map Map2Index;
	struct gl_2d_map Map2Color4;
	struct gl_2d_map Map2Normal;
	struct gl_2d_map Map2Texture1;
	struct gl_2d_map Map2Texture2;
	struct gl_2d_map Map2Texture3;
	struct gl_2d_map Map2Texture4;
};



/* Texture object record */
struct gl_texture_object {
	GLint RefCount;			/* reference count */
	GLuint Name;			/* an unsigned integer */
	GLuint Dimensions;		/* 1 or 2 or 3 */
	GLfloat Priority;		/* in [0,1] */
	GLint BorderColor[4];		/* as integers in [0,255] */
	GLenum WrapS;			/* GL_CLAMP or GL_REPEAT */
	GLenum WrapT;			/* GL_CLAMP or GL_REPEAT */
	GLenum WrapR;			/* GL_CLAMP or GL_REPEAT */
	GLenum MinFilter;		/* minification filter */
	GLenum MagFilter;		/* magnification filter */
	GLfloat MinMagThresh;		/* min/mag threshold */
	struct gl_texture_image *Image[MAX_TEXTURE_LEVELS];

	/* GL_EXT_paletted_texture */
	GLubyte Palette[MAX_TEXTURE_PALETTE_SIZE*4];
	GLuint PaletteSize;
	GLenum PaletteIntFormat;
	GLenum PaletteFormat;

	/* For device driver: */
	GLboolean Dirty;	/* Set when any texobj state changes */
	void *DriverData;	/* Arbitrary device driver data */

	GLboolean Complete;		/* Complete set of images? */
	TextureSampleFunc SampleFunc;
	struct gl_texture_object *Next;	/* Next in linked list */
};



/*
 * State which can be shared by multiple contexts:
 */
struct gl_shared_state {
   GLint RefCount;			/* Reference count */
   struct HashTable *DisplayList;	/* Display lists hash table */
   struct HashTable *TexObjects;	/* Texture objects hash table */
   struct gl_texture_object *TexObjectList;/* Linked list of texture objects */
   struct gl_texture_object *Default1D;	/* Default texture objects */
   struct gl_texture_object *Default2D;
};



/*
 * Describes the color, depth, stencil and accum buffer parameters.
 */
struct gl_visual {
	GLboolean RGBAflag;	/* Is frame buffer in RGBA mode, not CI? */
	GLboolean DBflag;	/* Is color buffer double buffered? */

	GLfloat RedScale;	/* These values are used to scale color */
	GLfloat GreenScale;	/* components from the range [0,1] to */
	GLfloat BlueScale;	/* integer values.  It should be the case */
	GLfloat AlphaScale;	/* that scale = 2^bits - 1 where bits is */
				/* the number of bits for the component */
				/* in the frame buffer. */
	GLboolean EightBitColor;/* TRUE if all the above scales are 255.0 */

        GLfloat InvRedScale;	/* = 1 / RedScale */
        GLfloat InvGreenScale;	/* = 1 / GreenScale */
        GLfloat InvBlueScale;	/* = 1 / BlueScale */
        GLfloat InvAlphaScale;	/* = 1 / AlphaScale */

	GLint RedBits;		/* Bits per color component */
	GLint GreenBits;
	GLint BlueBits;
	GLint AlphaBits;

	GLint IndexBits;	/* Bits/pixel if in color index mode */

	GLint AccumBits;	/* Number of bits per color channel, or 0 */
	GLint DepthBits;	/* Number of bits in depth buffer, or 0 */
	GLint StencilBits;	/* Number of bits in stencil buffer, or 0 */

	/* Software alpha planes: */
	GLboolean FrontAlphaEnabled;
	GLboolean BackAlphaEnabled;
};



/*
 * A "frame buffer" is a color buffer and its optional ancillary buffers:
 * depth, accum, stencil, and software-simulated alpha buffers.
 */
struct gl_frame_buffer {
	GLvisual *Visual;	/* The corresponding visual */

	GLint Width;		/* Width of frame buffer in pixels */
	GLint Height;		/* Height of frame buffer in pixels */

	GLdepth *Depth;		/* array [Width*Height] of GLdepth values */

	/* Stencil buffer */
	GLstencil *Stencil;	/* array [Width*Height] of GLstencil values */

	/* Accumulation buffer */
	GLaccum *Accum;		/* array [4*Width*Height] of GLaccum values */

	/* Software alpha planes: */
	GLubyte *FrontAlpha;	/* array [Width*Height] of GLubyte */
	GLubyte *BackAlpha;	/* array [Width*Height] of GLubyte */
	GLubyte *Alpha;		/* Points to front or back alpha buffer */

	/* Drawing bounds: intersection of window size and scissor box */
	GLint Xmin, Xmax, Ymin, Ymax;
};



/*
 * Bitmasks to indicate what auxillary information must be interpolated
 * when clipping (ClipMask).
 */
#define CLIP_FCOLOR_BIT		0x01
#define CLIP_BCOLOR_BIT		0x02
#define CLIP_FINDEX_BIT		0x04
#define CLIP_BINDEX_BIT		0x08
#define CLIP_TEXTURE_BIT	0x10



/*
 * Bitmasks to indicate which rasterization options are enabled (RasterMask)
 */
#define ALPHATEST_BIT		0x001	/* Alpha-test pixels */
#define BLEND_BIT		0x002	/* Blend pixels */
#define DEPTH_BIT		0x004	/* Depth-test pixels */
#define FOG_BIT			0x008	/* Per-pixel fog */
#define LOGIC_OP_BIT		0x010	/* Apply logic op in software */
#define SCISSOR_BIT		0x020	/* Scissor pixels */
#define STENCIL_BIT		0x040	/* Stencil pixels */
#define MASKING_BIT		0x080	/* Do glColorMask() or glIndexMask() */
#define ALPHABUF_BIT		0x100	/* Using software alpha buffer */
#define WINCLIP_BIT		0x200	/* Clip pixels/primitives to window */
#define FRONT_AND_BACK_BIT	0x400	/* Write to front and back buffers */
#define NO_DRAW_BIT		0x800	/* Don't write any pixels */


/*
 * Bits to indicate what state has to be updated (NewState)
 */
#define NEW_LIGHTING	0x1
#define NEW_RASTER_OPS	0x2
#define NEW_TEXTURING	0x4
#define NEW_POLYGON	0x8
#define NEW_ALL		0xf


/*
 * Different kinds of 4x4 transformation matrices:
 */
#define MATRIX_GENERAL		0	/* general 4x4 matrix */
#define MATRIX_IDENTITY		1	/* identity matrix */
#define MATRIX_ORTHO		2	/* orthographic projection matrix */
#define MATRIX_PERSPECTIVE	3	/* perspective projection matrix */
#define MATRIX_2D		4	/* 2-D transformation */
#define MATRIX_2D_NO_ROT	5	/* 2-D scale & translate only */
#define MATRIX_3D		6	/* 3-D transformation */


/*
 * Forward declaration of display list datatypes:
 */
union node;
typedef union node Node;



/*
 * The library context: 
 */

struct gl_context {
	/* State possibly shared with other contexts in the address space */
	struct gl_shared_state *Shared;

	/* API function pointer tables */
	struct gl_api_table API;		/* For api.c */
	struct gl_api_table Save;		/* Display list save funcs */
	struct gl_api_table Exec;		/* Execute funcs */

        GLvisual *Visual;
        GLframebuffer *Buffer;

	/* Driver function pointer table */
	struct dd_function_table Driver;

	void *DriverCtx;	/* Points to device driver context/state */
	void *DriverMgrCtx;	/* Points to device driver manager (optional)*/

	/* Modelview matrix and stack */
	GLboolean NewModelViewMatrix;
	GLuint ModelViewMatrixType;	/* = one of MATRIX_* values */
	GLfloat ModelViewMatrix[16];
	GLfloat ModelViewInv[16];	/* Inverse of ModelViewMatrix */
	GLuint ModelViewStackDepth;
	GLfloat ModelViewStack[MAX_MODELVIEW_STACK_DEPTH][16];

	/* Projection matrix and stack */
	GLboolean NewProjectionMatrix;
	GLuint ProjectionMatrixType;	/* = one of MATRIX_* values */
	GLfloat ProjectionMatrix[16];
	GLuint ProjectionStackDepth;
	GLfloat ProjectionStack[MAX_PROJECTION_STACK_DEPTH][16];
	GLfloat NearFarStack[MAX_PROJECTION_STACK_DEPTH][2];

	/* Texture matrix and stack */
	GLboolean NewTextureMatrix;
	GLuint TextureMatrixType;	/* = one of MATRIX_* values */
	GLfloat TextureMatrix[16];
	GLuint TextureStackDepth;
	GLfloat TextureStack[MAX_TEXTURE_STACK_DEPTH][16];

	/* Display lists */
	GLuint CallDepth;	/* Current recursion calling depth */
	GLboolean ExecuteFlag;	/* Execute GL commands? */
	GLboolean CompileFlag;	/* Compile GL commands into display list? */
	Node *CurrentListPtr;	/* Head of list being compiled */
	GLuint CurrentListNum;	/* Number of the list being compiled */
	Node *CurrentBlock;	/* Pointer to current block of nodes */
	GLuint CurrentPos;	/* Index into current block of nodes */

	/* Renderer attribute stack */
	GLuint AttribStackDepth;
	struct gl_attrib_node *AttribStack[MAX_ATTRIB_STACK_DEPTH];

	/* Renderer attribute groups */
	struct gl_accum_attrib		Accum;
	struct gl_colorbuffer_attrib	Color;
	struct gl_current_attrib	Current;
	struct gl_depthbuffer_attrib	Depth;
	struct gl_eval_attrib		Eval;
	struct gl_fog_attrib		Fog;
	struct gl_hint_attrib		Hint;
	struct gl_light_attrib		Light;
	struct gl_line_attrib		Line;
	struct gl_list_attrib		List;
	struct gl_pixel_attrib		Pixel;
	struct gl_point_attrib		Point;
	struct gl_polygon_attrib	Polygon;
	GLuint PolygonStipple[32];
	struct gl_scissor_attrib	Scissor;
	struct gl_stencil_attrib	Stencil;
	struct gl_texture_attrib	Texture;
	struct gl_transform_attrib	Transform;
	struct gl_viewport_attrib	Viewport;

	/* Client attribute stack */
	GLuint ClientAttribStackDepth;
	struct gl_attrib_node *ClientAttribStack[MAX_CLIENT_ATTRIB_STACK_DEPTH];
	/* Client attribute groups */
	struct gl_array_attrib		Array;	/* Vertex arrays */
	struct gl_pixelstore_attrib	Pack;	/* Pixel packing */
	struct gl_pixelstore_attrib	Unpack;	/* Pixel unpacking */

	struct gl_evaluators EvalMap;	/* All evaluators */
	struct gl_feedback Feedback;	/* Feedback */
	struct gl_selection Select;	/* Selection */

	GLenum ErrorValue;		/* Last error code */

	GLboolean DirectContext;	/* Important for real GLX */

	/* Miscellaneous */
        GLuint NewState;        /* bitwise OR of NEW_* flags */
	GLenum RenderMode;	/* either GL_RENDER, GL_SELECT, GL_FEEDBACK */
	GLenum Primitive;	/* glBegin primitive or GL_BITMAP */
	GLuint StippleCounter;	/* Line stipple counter */
	GLuint ClipMask;	/* OR of CLIP_* values from above */
        interp_func ClipInterpAuxFunc; /* The auxiliary interpolation function */
	GLuint RasterMask;	/* OR of rasterization flags */
	GLboolean LightTwoSide;	/* Compute two-sided lighting? */
	GLboolean DirectTriangles;/* Directly call (*ctx->TriangleFunc) ? */
	GLfloat PolygonZoffset;	/* Z offset for GL_FILL polygons */
	GLfloat LineZoffset;	/* Z offset for GL_LINE polygons */
	GLfloat PointZoffset;	/* Z offset for GL_POINT polygons */
	GLboolean NeedNormals;	 /* Are vertex normal vectors needed? */
        GLboolean FastDrawPixels;/* Use optimized glDrawPixels? */
        GLboolean MutablePixels; /* Can rasterization change pixel's color? */
        GLboolean MonoPixels;    /* Are all pixels likely to be same color? */

	/* Current Primitive functions */
        points_func PointsFunc;
        line_func LineFunc;
        triangle_func TriangleFunc;
        quad_func QuadFunc;
        rect_func RectFunc;

        /* The vertex buffer being used by this context */
        struct vertex_buffer* VB;

        /* The pixel buffer being used by this context */
        struct pixel_buffer* PB;

#ifdef PROFILE
        /* Performance measurements */
        GLuint BeginEndCount;	/* number of glBegin/glEnd pairs */
        GLdouble BeginEndTime;	/* seconds spent between glBegin/glEnd */
        GLuint VertexCount;	/* number of vertices processed */
        GLdouble VertexTime;	/* total time in seconds */
        GLuint PointCount;	/* number of points rendered */
        GLdouble PointTime;	/* total time in seconds */
        GLuint LineCount;	/* number of lines rendered */
        GLdouble LineTime;	/* total time in seconds */
        GLuint PolygonCount;	/* number of polygons rendered */
        GLdouble PolygonTime;	/* total time in seconds */
        GLuint ClearCount;	/* number of glClear calls */
        GLdouble ClearTime;	/* seconds spent in glClear */
        GLuint SwapCount;	/* number of swap-buffer calls */
        GLdouble SwapTime;	/* seconds spent in swap-buffers */
#endif

        /* For debugging/development only */
        GLboolean NoRaster;

        /* Dither disable via MESA_NO_DITHER env var */
        GLboolean NoDither;
};


#endif

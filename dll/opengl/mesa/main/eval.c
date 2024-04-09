
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

/*
 * eval.c was written by
 * Bernd Barsuhn (bdbarsuh@cip.informatik.uni-erlangen.de) and
 * Volker Weiss (vrweiss@cip.informatik.uni-erlangen.de).
 *
 * My original implementation of evaluators was simplistic and didn't
 * compute surface normal vectors properly.  Bernd and Volker applied
 * used more sophisticated methods to get better results.
 *
 * Thanks guys!
 */

#include <precomp.h>

#if FEATURE_evaluators


/*
 * Return the number of components per control point for any type of
 * evaluator.  Return 0 if bad target.
 * See table 5.1 in the OpenGL 1.2 spec.
 */
GLuint _mesa_evaluator_components( GLenum target )
{
   switch (target) {
      case GL_MAP1_VERTEX_3:		return 3;
      case GL_MAP1_VERTEX_4:		return 4;
      case GL_MAP1_INDEX:		return 1;
      case GL_MAP1_COLOR_4:		return 4;
      case GL_MAP1_NORMAL:		return 3;
      case GL_MAP1_TEXTURE_COORD_1:	return 1;
      case GL_MAP1_TEXTURE_COORD_2:	return 2;
      case GL_MAP1_TEXTURE_COORD_3:	return 3;
      case GL_MAP1_TEXTURE_COORD_4:	return 4;
      case GL_MAP2_VERTEX_3:		return 3;
      case GL_MAP2_VERTEX_4:		return 4;
      case GL_MAP2_INDEX:		return 1;
      case GL_MAP2_COLOR_4:		return 4;
      case GL_MAP2_NORMAL:		return 3;
      case GL_MAP2_TEXTURE_COORD_1:	return 1;
      case GL_MAP2_TEXTURE_COORD_2:	return 2;
      case GL_MAP2_TEXTURE_COORD_3:	return 3;
      case GL_MAP2_TEXTURE_COORD_4:	return 4;
      default:				break;
   }

   /* XXX need to check for the vertex program extension
   if (!ctx->Extensions.NV_vertex_program)
      return 0;
   */

   if (target >= GL_MAP1_VERTEX_ATTRIB0_4_NV &&
       target <= GL_MAP1_VERTEX_ATTRIB15_4_NV)
      return 4;

   if (target >= GL_MAP2_VERTEX_ATTRIB0_4_NV &&
       target <= GL_MAP2_VERTEX_ATTRIB15_4_NV)
      return 4;

   return 0;
}


/*
 * Return pointer to the gl_1d_map struct for the named target.
 */
static struct gl_1d_map *
get_1d_map( struct gl_context *ctx, GLenum target )
{
   switch (target) {
      case GL_MAP1_VERTEX_3:
         return &ctx->EvalMap.Map1Vertex3;
      case GL_MAP1_VERTEX_4:
         return &ctx->EvalMap.Map1Vertex4;
      case GL_MAP1_INDEX:
         return &ctx->EvalMap.Map1Index;
      case GL_MAP1_COLOR_4:
         return &ctx->EvalMap.Map1Color4;
      case GL_MAP1_NORMAL:
         return &ctx->EvalMap.Map1Normal;
      case GL_MAP1_TEXTURE_COORD_1:
         return &ctx->EvalMap.Map1Texture1;
      case GL_MAP1_TEXTURE_COORD_2:
         return &ctx->EvalMap.Map1Texture2;
      case GL_MAP1_TEXTURE_COORD_3:
         return &ctx->EvalMap.Map1Texture3;
      case GL_MAP1_TEXTURE_COORD_4:
         return &ctx->EvalMap.Map1Texture4;
      default:
         return NULL;
   }
}


/*
 * Return pointer to the gl_2d_map struct for the named target.
 */
static struct gl_2d_map *
get_2d_map( struct gl_context *ctx, GLenum target )
{
   switch (target) {
      case GL_MAP2_VERTEX_3:
         return &ctx->EvalMap.Map2Vertex3;
      case GL_MAP2_VERTEX_4:
         return &ctx->EvalMap.Map2Vertex4;
      case GL_MAP2_INDEX:
         return &ctx->EvalMap.Map2Index;
      case GL_MAP2_COLOR_4:
         return &ctx->EvalMap.Map2Color4;
      case GL_MAP2_NORMAL:
         return &ctx->EvalMap.Map2Normal;
      case GL_MAP2_TEXTURE_COORD_1:
         return &ctx->EvalMap.Map2Texture1;
      case GL_MAP2_TEXTURE_COORD_2:
         return &ctx->EvalMap.Map2Texture2;
      case GL_MAP2_TEXTURE_COORD_3:
         return &ctx->EvalMap.Map2Texture3;
      case GL_MAP2_TEXTURE_COORD_4:
         return &ctx->EvalMap.Map2Texture4;
      default:
         return NULL;
   }
}


/**********************************************************************/
/***            Copy and deallocate control points                  ***/
/**********************************************************************/


/*
 * Copy 1-parametric evaluator control points from user-specified
 * memory space to a buffer of contiguous control points.
 * \param see glMap1f for details
 * \return pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
GLfloat *_mesa_copy_map_points1f( GLenum target, GLint ustride, GLint uorder,
                                  const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLint i, k, size = _mesa_evaluator_components(target);

   if (!points || !size)
      return NULL;

   buffer = (GLfloat *) MALLOC(uorder * size * sizeof(GLfloat));

   if (buffer)
      for (i = 0, p = buffer; i < uorder; i++, points += ustride)
	for (k = 0; k < size; k++)
	  *p++ = points[k];

   return buffer;
}



/*
 * Same as above but convert doubles to floats.
 */
GLfloat *_mesa_copy_map_points1d( GLenum target, GLint ustride, GLint uorder,
                                  const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLint i, k, size = _mesa_evaluator_components(target);

   if (!points || !size)
      return NULL;

   buffer = (GLfloat *) MALLOC(uorder * size * sizeof(GLfloat));

   if (buffer)
      for (i = 0, p = buffer; i < uorder; i++, points += ustride)
	for (k = 0; k < size; k++)
	  *p++ = (GLfloat) points[k];

   return buffer;
}



/*
 * Copy 2-parametric evaluator control points from user-specified
 * memory space to a buffer of contiguous control points.
 * Additional memory is allocated to be used by the horner and
 * de Casteljau evaluation schemes.
 *
 * \param see glMap2f for details
 * \return pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
GLfloat *_mesa_copy_map_points2f( GLenum target,
                                  GLint ustride, GLint uorder,
                                  GLint vstride, GLint vorder,
                                  const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLint i, j, k, size, dsize, hsize;
   GLint uinc;

   size = _mesa_evaluator_components(target);

   if (!points || size==0) {
      return NULL;
   }

   /* max(uorder, vorder) additional points are used in      */
   /* horner evaluation and uorder*vorder additional */
   /* values are needed for de Casteljau                     */
   dsize = (uorder == 2 && vorder == 2)? 0 : uorder*vorder;
   hsize = (uorder > vorder ? uorder : vorder)*size;

   if(hsize>dsize)
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+hsize)*sizeof(GLfloat));
   else
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+dsize)*sizeof(GLfloat));

   /* compute the increment value for the u-loop */
   uinc = ustride - vorder*vstride;

   if (buffer)
      for (i=0, p=buffer; i<uorder; i++, points += uinc)
	 for (j=0; j<vorder; j++, points += vstride)
	    for (k=0; k<size; k++)
	       *p++ = points[k];

   return buffer;
}



/*
 * Same as above but convert doubles to floats.
 */
GLfloat *_mesa_copy_map_points2d(GLenum target,
                                 GLint ustride, GLint uorder,
                                 GLint vstride, GLint vorder,
                                 const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLint i, j, k, size, hsize, dsize;
   GLint uinc;

   size = _mesa_evaluator_components(target);

   if (!points || size==0) {
      return NULL;
   }

   /* max(uorder, vorder) additional points are used in      */
   /* horner evaluation and uorder*vorder additional */
   /* values are needed for de Casteljau                     */
   dsize = (uorder == 2 && vorder == 2)? 0 : uorder*vorder;
   hsize = (uorder > vorder ? uorder : vorder)*size;

   if(hsize>dsize)
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+hsize)*sizeof(GLfloat));
   else
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+dsize)*sizeof(GLfloat));

   /* compute the increment value for the u-loop */
   uinc = ustride - vorder*vstride;

   if (buffer)
      for (i=0, p=buffer; i<uorder; i++, points += uinc)
	 for (j=0; j<vorder; j++, points += vstride)
	    for (k=0; k<size; k++)
	       *p++ = (GLfloat) points[k];

   return buffer;
}




/**********************************************************************/
/***                      API entry points                          ***/
/**********************************************************************/


/*
 * This does the work of glMap1[fd].
 */
static void
map1(GLenum target, GLfloat u1, GLfloat u2, GLint ustride,
     GLint uorder, const GLvoid *points, GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint k;
   GLfloat *pnts;
   struct gl_1d_map *map = NULL;

   ASSERT_OUTSIDE_BEGIN_END(ctx);
   ASSERT(type == GL_FLOAT || type == GL_DOUBLE);

   if (u1 == u2) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(u1,u2)" );
      return;
   }
   if (uorder < 1 || uorder > MAX_EVAL_ORDER) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(order)" );
      return;
   }
   if (!points) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(points)" );
      return;
   }

   k = _mesa_evaluator_components( target );
   if (k == 0) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
   }

   if (ustride < k) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(stride)" );
      return;
   }

   map = get_1d_map(ctx, target);
   if (!map) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
      return;
   }

   /* make copy of the control points */
   if (type == GL_FLOAT)
      pnts = _mesa_copy_map_points1f(target, ustride, uorder, (GLfloat*) points);
   else
      pnts = _mesa_copy_map_points1d(target, ustride, uorder, (GLdouble*) points);


   FLUSH_VERTICES(ctx, _NEW_EVAL);
   map->Order = uorder;
   map->u1 = u1;
   map->u2 = u2;
   map->du = 1.0F / (u2 - u1);
   if (map->Points)
      FREE( map->Points );
   map->Points = pnts;
}



static void GLAPIENTRY
_mesa_Map1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride,
             GLint order, const GLfloat *points )
{
   map1(target, u1, u2, stride, order, points, GL_FLOAT);
}


static void GLAPIENTRY
_mesa_Map1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride,
             GLint order, const GLdouble *points )
{
   map1(target, (GLfloat) u1, (GLfloat) u2, stride, order, points, GL_DOUBLE);
}


static void
map2( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
      GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
      const GLvoid *points, GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint k;
   GLfloat *pnts;
   struct gl_2d_map *map = NULL;

   ASSERT_OUTSIDE_BEGIN_END(ctx);
   ASSERT(type == GL_FLOAT || type == GL_DOUBLE);

   if (u1==u2) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(u1,u2)" );
      return;
   }

   if (v1==v2) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(v1,v2)" );
      return;
   }

   if (uorder<1 || uorder>MAX_EVAL_ORDER) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(uorder)" );
      return;
   }

   if (vorder<1 || vorder>MAX_EVAL_ORDER) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(vorder)" );
      return;
   }

   k = _mesa_evaluator_components( target );
   if (k==0) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
   }

   if (ustride < k) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(ustride)" );
      return;
   }
   if (vstride < k) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(vstride)" );
      return;
   }

   map = get_2d_map(ctx, target);
   if (!map) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
      return;
   }

   /* make copy of the control points */
   if (type == GL_FLOAT)
      pnts = _mesa_copy_map_points2f(target, ustride, uorder,
                                  vstride, vorder, (GLfloat*) points);
   else
      pnts = _mesa_copy_map_points2d(target, ustride, uorder,
                                  vstride, vorder, (GLdouble*) points);


   FLUSH_VERTICES(ctx, _NEW_EVAL);
   map->Uorder = uorder;
   map->u1 = u1;
   map->u2 = u2;
   map->du = 1.0F / (u2 - u1);
   map->Vorder = vorder;
   map->v1 = v1;
   map->v2 = v2;
   map->dv = 1.0F / (v2 - v1);
   if (map->Points)
      FREE( map->Points );
   map->Points = pnts;
}


static void GLAPIENTRY
_mesa_Map2f( GLenum target,
             GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
             GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
             const GLfloat *points)
{
   map2(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder,
        points, GL_FLOAT);
}


static void GLAPIENTRY
_mesa_Map2d( GLenum target,
             GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
             GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
             const GLdouble *points )
{
   map2(target, (GLfloat) u1, (GLfloat) u2, ustride, uorder, 
	(GLfloat) v1, (GLfloat) v2, vstride, vorder, points, GL_DOUBLE);
}



static void GLAPIENTRY
_mesa_GetMapdv( GLenum target, GLenum query, GLdouble *v )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_1d_map *map1d;
   struct gl_2d_map *map2d;
   GLint i, n;
   GLfloat *data;
   GLuint comps;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   comps = _mesa_evaluator_components(target);
   if (!comps) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
      return;
   }

   map1d = get_1d_map(ctx, target);
   map2d = get_2d_map(ctx, target);
   ASSERT(map1d || map2d);

   switch (query) {
      case GL_COEFF:
         if (map1d) {
            data = map1d->Points;
            n = map1d->Order * comps;
         }
         else {
            data = map2d->Points;
            n = map2d->Uorder * map2d->Vorder * comps;
         }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = data[i];
	    }
	 }
         break;
      case GL_ORDER:
         if (map1d) {
            v[0] = (GLdouble) map1d->Order;
         }
         else {
            v[0] = (GLdouble) map2d->Uorder;
            v[1] = (GLdouble) map2d->Vorder;
         }
         break;
      case GL_DOMAIN:
         if (map1d) {
            v[0] = (GLdouble) map1d->u1;
            v[1] = (GLdouble) map1d->u2;
         }
         else {
            v[0] = (GLdouble) map2d->u1;
            v[1] = (GLdouble) map2d->u2;
            v[2] = (GLdouble) map2d->v1;
            v[3] = (GLdouble) map2d->v2;
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapdv(query)" );
   }
   return;
}

static void GLAPIENTRY
_mesa_GetMapfv( GLenum target, GLenum query, GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_1d_map *map1d;
   struct gl_2d_map *map2d;
   GLint i, n;
   GLfloat *data;
   GLuint comps;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   comps = _mesa_evaluator_components(target);
   if (!comps) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
      return;
   }

   map1d = get_1d_map(ctx, target);
   map2d = get_2d_map(ctx, target);
   ASSERT(map1d || map2d);

   switch (query) {
      case GL_COEFF:
         if (map1d) {
            data = map1d->Points;
            n = map1d->Order * comps;
         }
         else {
            data = map2d->Points;
            n = map2d->Uorder * map2d->Vorder * comps;
         }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = data[i];
	    }
	 }
         break;
      case GL_ORDER:
         if (map1d) {
            v[0] = (GLfloat) map1d->Order;
         }
         else {
            v[0] = (GLfloat) map2d->Uorder;
            v[1] = (GLfloat) map2d->Vorder;
         }
         break;
      case GL_DOMAIN:
         if (map1d) {
            v[0] = map1d->u1;
            v[1] = map1d->u2;
         }
         else {
            v[0] = map2d->u1;
            v[1] = map2d->u2;
            v[2] = map2d->v1;
            v[3] = map2d->v2;
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapfv(query)" );
   }
   return;
}


static void GLAPIENTRY
_mesa_GetMapiv( GLenum target, GLenum query, GLint *v )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_1d_map *map1d;
   struct gl_2d_map *map2d;
   GLuint i, n;
   GLfloat *data;
   GLuint comps;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   comps = _mesa_evaluator_components(target);
   if (!comps) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
      return;
   }

   map1d = get_1d_map(ctx, target);
   map2d = get_2d_map(ctx, target);
   ASSERT(map1d || map2d);

   switch (query) {
      case GL_COEFF:
         if (map1d) {
            data = map1d->Points;
            n = map1d->Order * comps;
         }
         else {
            data = map2d->Points;
            n = map2d->Uorder * map2d->Vorder * comps;
         }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = IROUND(data[i]);
	    }
	 }
         break;
      case GL_ORDER:
         if (map1d) {
            v[0] = map1d->Order;
         }
         else {
            v[0] = map2d->Uorder;
            v[1] = map2d->Vorder;
         }
         break;
      case GL_DOMAIN:
         if (map1d) {
            v[0] = IROUND(map1d->u1);
            v[1] = IROUND(map1d->u2);
         }
         else {
            v[0] = IROUND(map2d->u1);
            v[1] = IROUND(map2d->u2);
            v[2] = IROUND(map2d->v1);
            v[3] = IROUND(map2d->v2);
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapiv(query)" );
   }
   return;
}


static void GLAPIENTRY
_mesa_MapGrid1f( GLint un, GLfloat u1, GLfloat u2 )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (un<1) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMapGrid1f" );
      return;
   }
   FLUSH_VERTICES(ctx, _NEW_EVAL);
   ctx->Eval.MapGrid1un = un;
   ctx->Eval.MapGrid1u1 = u1;
   ctx->Eval.MapGrid1u2 = u2;
   ctx->Eval.MapGrid1du = (u2 - u1) / (GLfloat) un;
}


static void GLAPIENTRY
_mesa_MapGrid1d( GLint un, GLdouble u1, GLdouble u2 )
{
   _mesa_MapGrid1f( un, (GLfloat) u1, (GLfloat) u2 );
}


static void GLAPIENTRY
_mesa_MapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                 GLint vn, GLfloat v1, GLfloat v2 )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (un<1) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(un)" );
      return;
   }
   if (vn<1) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(vn)" );
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_EVAL);
   ctx->Eval.MapGrid2un = un;
   ctx->Eval.MapGrid2u1 = u1;
   ctx->Eval.MapGrid2u2 = u2;
   ctx->Eval.MapGrid2du = (u2 - u1) / (GLfloat) un;
   ctx->Eval.MapGrid2vn = vn;
   ctx->Eval.MapGrid2v1 = v1;
   ctx->Eval.MapGrid2v2 = v2;
   ctx->Eval.MapGrid2dv = (v2 - v1) / (GLfloat) vn;
}


static void GLAPIENTRY
_mesa_MapGrid2d( GLint un, GLdouble u1, GLdouble u2,
                 GLint vn, GLdouble v1, GLdouble v2 )
{
   _mesa_MapGrid2f( un, (GLfloat) u1, (GLfloat) u2, 
		    vn, (GLfloat) v1, (GLfloat) v2 );
}


void
_mesa_install_eval_vtxfmt(struct _glapi_table *disp,
                          const GLvertexformat *vfmt)
{
   SET_EvalCoord1f(disp, vfmt->EvalCoord1f);
   SET_EvalCoord1fv(disp, vfmt->EvalCoord1fv);
   SET_EvalCoord2f(disp, vfmt->EvalCoord2f);
   SET_EvalCoord2fv(disp, vfmt->EvalCoord2fv);
   SET_EvalPoint1(disp, vfmt->EvalPoint1);
   SET_EvalPoint2(disp, vfmt->EvalPoint2);

   SET_EvalMesh1(disp, vfmt->EvalMesh1);
   SET_EvalMesh2(disp, vfmt->EvalMesh2);
}


void
_mesa_init_eval_dispatch(struct _glapi_table *disp)
{
   SET_GetMapdv(disp, _mesa_GetMapdv);
   SET_GetMapfv(disp, _mesa_GetMapfv);
   SET_GetMapiv(disp, _mesa_GetMapiv);
   SET_Map1d(disp, _mesa_Map1d);
   SET_Map1f(disp, _mesa_Map1f);
   SET_Map2d(disp, _mesa_Map2d);
   SET_Map2f(disp, _mesa_Map2f);
   SET_MapGrid1d(disp, _mesa_MapGrid1d);
   SET_MapGrid1f(disp, _mesa_MapGrid1f);
   SET_MapGrid2d(disp, _mesa_MapGrid2d);
   SET_MapGrid2f(disp, _mesa_MapGrid2f);
}


#endif /* FEATURE_evaluators */


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

/**
 * Initialize a 1-D evaluator map.
 */
static void
init_1d_map( struct gl_1d_map *map, int n, const float *initial )
{
   map->Order = 1;
   map->u1 = 0.0;
   map->u2 = 1.0;
   map->Points = (GLfloat *) MALLOC(n * sizeof(GLfloat));
   if (map->Points) {
      GLint i;
      for (i=0;i<n;i++)
         map->Points[i] = initial[i];
   }
}


/**
 * Initialize a 2-D evaluator map 
 */
static void
init_2d_map( struct gl_2d_map *map, int n, const float *initial )
{
   map->Uorder = 1;
   map->Vorder = 1;
   map->u1 = 0.0;
   map->u2 = 1.0;
   map->v1 = 0.0;
   map->v2 = 1.0;
   map->Points = (GLfloat *) MALLOC(n * sizeof(GLfloat));
   if (map->Points) {
      GLint i;
      for (i=0;i<n;i++)
         map->Points[i] = initial[i];
   }
}


void _mesa_init_eval( struct gl_context *ctx )
{

   /* Evaluators group */
   ctx->Eval.Map1Color4 = GL_FALSE;
   ctx->Eval.Map1Index = GL_FALSE;
   ctx->Eval.Map1Normal = GL_FALSE;
   ctx->Eval.Map1TextureCoord1 = GL_FALSE;
   ctx->Eval.Map1TextureCoord2 = GL_FALSE;
   ctx->Eval.Map1TextureCoord3 = GL_FALSE;
   ctx->Eval.Map1TextureCoord4 = GL_FALSE;
   ctx->Eval.Map1Vertex3 = GL_FALSE;
   ctx->Eval.Map1Vertex4 = GL_FALSE;
   ctx->Eval.Map2Color4 = GL_FALSE;
   ctx->Eval.Map2Index = GL_FALSE;
   ctx->Eval.Map2Normal = GL_FALSE;
   ctx->Eval.Map2TextureCoord1 = GL_FALSE;
   ctx->Eval.Map2TextureCoord2 = GL_FALSE;
   ctx->Eval.Map2TextureCoord3 = GL_FALSE;
   ctx->Eval.Map2TextureCoord4 = GL_FALSE;
   ctx->Eval.Map2Vertex3 = GL_FALSE;
   ctx->Eval.Map2Vertex4 = GL_FALSE;
   ctx->Eval.AutoNormal = GL_FALSE;
   ctx->Eval.MapGrid1un = 1;
   ctx->Eval.MapGrid1u1 = 0.0;
   ctx->Eval.MapGrid1u2 = 1.0;
   ctx->Eval.MapGrid2un = 1;
   ctx->Eval.MapGrid2vn = 1;
   ctx->Eval.MapGrid2u1 = 0.0;
   ctx->Eval.MapGrid2u2 = 1.0;
   ctx->Eval.MapGrid2v1 = 0.0;
   ctx->Eval.MapGrid2v2 = 1.0;

   /* Evaluator data */
   {
      static GLfloat vertex[4] = { 0.0, 0.0, 0.0, 1.0 };
      static GLfloat normal[3] = { 0.0, 0.0, 1.0 };
      static GLfloat index[1] = { 1.0 };
      static GLfloat color[4] = { 1.0, 1.0, 1.0, 1.0 };
      static GLfloat texcoord[4] = { 0.0, 0.0, 0.0, 1.0 };

      init_1d_map( &ctx->EvalMap.Map1Vertex3, 3, vertex );
      init_1d_map( &ctx->EvalMap.Map1Vertex4, 4, vertex );
      init_1d_map( &ctx->EvalMap.Map1Index, 1, index );
      init_1d_map( &ctx->EvalMap.Map1Color4, 4, color );
      init_1d_map( &ctx->EvalMap.Map1Normal, 3, normal );
      init_1d_map( &ctx->EvalMap.Map1Texture1, 1, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture2, 2, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture3, 3, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture4, 4, texcoord );

      init_2d_map( &ctx->EvalMap.Map2Vertex3, 3, vertex );
      init_2d_map( &ctx->EvalMap.Map2Vertex4, 4, vertex );
      init_2d_map( &ctx->EvalMap.Map2Index, 1, index );
      init_2d_map( &ctx->EvalMap.Map2Color4, 4, color );
      init_2d_map( &ctx->EvalMap.Map2Normal, 3, normal );
      init_2d_map( &ctx->EvalMap.Map2Texture1, 1, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture2, 2, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture3, 3, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture4, 4, texcoord );
   }
}


void _mesa_free_eval_data( struct gl_context *ctx )
{
   /* Free evaluator data */
   if (ctx->EvalMap.Map1Vertex3.Points)
      FREE( ctx->EvalMap.Map1Vertex3.Points );
   if (ctx->EvalMap.Map1Vertex4.Points)
      FREE( ctx->EvalMap.Map1Vertex4.Points );
   if (ctx->EvalMap.Map1Index.Points)
      FREE( ctx->EvalMap.Map1Index.Points );
   if (ctx->EvalMap.Map1Color4.Points)
      FREE( ctx->EvalMap.Map1Color4.Points );
   if (ctx->EvalMap.Map1Normal.Points)
      FREE( ctx->EvalMap.Map1Normal.Points );
   if (ctx->EvalMap.Map1Texture1.Points)
      FREE( ctx->EvalMap.Map1Texture1.Points );
   if (ctx->EvalMap.Map1Texture2.Points)
      FREE( ctx->EvalMap.Map1Texture2.Points );
   if (ctx->EvalMap.Map1Texture3.Points)
      FREE( ctx->EvalMap.Map1Texture3.Points );
   if (ctx->EvalMap.Map1Texture4.Points)
      FREE( ctx->EvalMap.Map1Texture4.Points );

   if (ctx->EvalMap.Map2Vertex3.Points)
      FREE( ctx->EvalMap.Map2Vertex3.Points );
   if (ctx->EvalMap.Map2Vertex4.Points)
      FREE( ctx->EvalMap.Map2Vertex4.Points );
   if (ctx->EvalMap.Map2Index.Points)
      FREE( ctx->EvalMap.Map2Index.Points );
   if (ctx->EvalMap.Map2Color4.Points)
      FREE( ctx->EvalMap.Map2Color4.Points );
   if (ctx->EvalMap.Map2Normal.Points)
      FREE( ctx->EvalMap.Map2Normal.Points );
   if (ctx->EvalMap.Map2Texture1.Points)
      FREE( ctx->EvalMap.Map2Texture1.Points );
   if (ctx->EvalMap.Map2Texture2.Points)
      FREE( ctx->EvalMap.Map2Texture2.Points );
   if (ctx->EvalMap.Map2Texture3.Points)
      FREE( ctx->EvalMap.Map2Texture3.Points );
   if (ctx->EvalMap.Map2Texture4.Points)
      FREE( ctx->EvalMap.Map2Texture4.Points );
}

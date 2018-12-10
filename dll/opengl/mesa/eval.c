/* $Id: eval.c,v 1.9 1998/02/03 00:53:51 brianp Exp $ */

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
 * $Log: eval.c,v $
 * Revision 1.9  1998/02/03 00:53:51  brianp
 * fixed bug in gl_copy_map_points*() functions (Sam Jordan)
 *
 * Revision 1.8  1997/07/24 01:25:01  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.7  1997/06/20 01:58:47  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.6  1997/05/28 03:24:22  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.5  1997/05/14 03:27:04  brianp
 * removed context argument from gl_init_eval()
 *
 * Revision 1.4  1997/05/01 01:38:38  brianp
 * use NORMALIZE_3FV() from mmath.h instead of private NORMALIZE() macro
 *
 * Revision 1.3  1997/04/02 03:10:58  brianp
 * changed some #include's
 *
 * Revision 1.2  1996/09/15 14:17:30  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
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


#ifdef PC_HEADER
#include "all.h"
#else
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "eval.h"
#include "dlist.h"
#include "macros.h"
#include "mmath.h"
#include "types.h"
#include "vbfill.h"
#endif



/*
 * Do one-time initialization for evaluators.
 */
void gl_init_eval( void )
{
  static int init_flag = 0;

  /* Compute a table of nCr (combination) values used by the
   * Bernstein polynomial generator.
   */

  if (init_flag==0) 
  { /* no initialization needed */ 
  }

  init_flag = 1;
}



/*
 * Horner scheme for Bezier curves
 * 
 * Bezier curves can be computed via a Horner scheme.
 * Horner is numerically less stable than the de Casteljau
 * algorithm, but it is faster. For curves of degree n 
 * the complexity of Horner is O(n) and de Casteljau is O(n^2).
 * Since stability is not important for displaying curve 
 * points I decided to use the Horner scheme.
 *
 * A cubic Bezier curve with control points b0, b1, b2, b3 can be 
 * written as
 *
 *        (([3]        [3]     )     [3]       )     [3]
 * c(t) = (([0]*s*b0 + [1]*t*b1)*s + [2]*t^2*b2)*s + [3]*t^2*b3
 *
 *                                           [n]
 * where s=1-t and the binomial coefficients [i]. These can 
 * be computed iteratively using the identity:
 *
 * [n]               [n  ]             [n]
 * [i] = (n-i+1)/i * [i-1]     and     [0] = 1
 */

static void
horner_bezier_curve(GLfloat *cp, GLfloat *out, GLfloat t,
                    GLuint dim, GLuint order)
{
  GLfloat s, powert;
  GLuint i, k, bincoeff;

  if(order >= 2)
  { 
    bincoeff = order-1;
    s = 1.0-t;

    for(k=0; k<dim; k++)
      out[k] = s*cp[k] + bincoeff*t*cp[dim+k];

    for(i=2, cp+=2*dim, powert=t*t; i<order; i++, powert*=t, cp +=dim)
    {
      bincoeff *= order-i;
      bincoeff /= i;

      for(k=0; k<dim; k++)
        out[k] = s*out[k] + bincoeff*powert*cp[k];
    }
  }
  else /* order=1 -> constant curve */
  { 
    for(k=0; k<dim; k++)
      out[k] = cp[k];
  } 
}

/*
 * Tensor product Bezier surfaces
 *
 * Again the Horner scheme is used to compute a point on a 
 * TP Bezier surface. First a control polygon for a curve
 * on the surface in one parameter direction is computed,
 * then the point on the curve for the other parameter 
 * direction is evaluated.
 *
 * To store the curve control polygon additional storage
 * for max(uorder,vorder) points is needed in the 
 * control net cn.
 */

static void
horner_bezier_surf(GLfloat *cn, GLfloat *out, GLfloat u, GLfloat v,
                   GLuint dim, GLuint uorder, GLuint vorder)
{
  GLfloat *cp = cn + uorder*vorder*dim;
  GLuint i, uinc = vorder*dim;

  if(vorder > uorder)
  {
    if(uorder >= 2)
    { 
      GLfloat s, poweru;
      GLuint j, k, bincoeff;

      /* Compute the control polygon for the surface-curve in u-direction */
      for(j=0; j<vorder; j++)
      {
        GLfloat *ucp = &cn[j*dim];

        /* Each control point is the point for parameter u on a */ 
        /* curve defined by the control polygons in u-direction */
	bincoeff = uorder-1;
	s = 1.0-u;

	for(k=0; k<dim; k++)
	  cp[j*dim+k] = s*ucp[k] + bincoeff*u*ucp[uinc+k];

	for(i=2, ucp+=2*uinc, poweru=u*u; i<uorder; 
            i++, poweru*=u, ucp +=uinc)
	{
	  bincoeff *= uorder-i;
          bincoeff /= i;

	  for(k=0; k<dim; k++)
	    cp[j*dim+k] = s*cp[j*dim+k] + bincoeff*poweru*ucp[k];
	}
      }
        
      /* Evaluate curve point in v */
      horner_bezier_curve(cp, out, v, dim, vorder);
    }
    else /* uorder=1 -> cn defines a curve in v */
      horner_bezier_curve(cn, out, v, dim, vorder);
  }
  else /* vorder <= uorder */
  {
    if(vorder > 1)
    {
      GLuint i;

      /* Compute the control polygon for the surface-curve in u-direction */
      for(i=0; i<uorder; i++, cn += uinc)
      {
	/* For constant i all cn[i][j] (j=0..vorder) are located */
	/* on consecutive memory locations, so we can use        */
	/* horner_bezier_curve to compute the control points     */

	horner_bezier_curve(cn, &cp[i*dim], v, dim, vorder);
      }

      /* Evaluate curve point in u */
      horner_bezier_curve(cp, out, u, dim, uorder);
    }
    else  /* vorder=1 -> cn defines a curve in u */
      horner_bezier_curve(cn, out, u, dim, uorder);
  }
}

/*
 * The direct de Casteljau algorithm is used when a point on the
 * surface and the tangent directions spanning the tangent plane
 * should be computed (this is needed to compute normals to the
 * surface). In this case the de Casteljau algorithm approach is
 * nicer because a point and the partial derivatives can be computed 
 * at the same time. To get the correct tangent length du and dv
 * must be multiplied with the (u2-u1)/uorder-1 and (v2-v1)/vorder-1. 
 * Since only the directions are needed, this scaling step is omitted.
 *
 * De Casteljau needs additional storage for uorder*vorder
 * values in the control net cn.
 */

static void
de_casteljau_surf(GLfloat *cn, GLfloat *out, GLfloat *du, GLfloat *dv,
                  GLfloat u, GLfloat v, GLuint dim, 
                  GLuint uorder, GLuint vorder)
{
  GLfloat *dcn = cn + uorder*vorder*dim;
  GLfloat us = 1.0-u, vs = 1.0-v;
  GLuint h, i, j, k;
  GLuint minorder = uorder < vorder ? uorder : vorder;
  GLuint uinc = vorder*dim;
  GLuint dcuinc = vorder;
 
  /* Each component is evaluated separately to save buffer space  */
  /* This does not drasticaly decrease the performance of the     */
  /* algorithm. If additional storage for (uorder-1)*(vorder-1)   */
  /* points would be available, the components could be accessed  */
  /* in the innermost loop which could lead to less cache misses. */

#define CN(I,J,K) cn[(I)*uinc+(J)*dim+(K)] 
#define DCN(I, J) dcn[(I)*dcuinc+(J)]
  if(minorder < 3)
  {
    if(uorder==vorder)
    {
      for(k=0; k<dim; k++)
      {
	/* Derivative direction in u */
	du[k] = vs*(CN(1,0,k) - CN(0,0,k)) +
	         v*(CN(1,1,k) - CN(0,1,k));

	/* Derivative direction in v */
	dv[k] = us*(CN(0,1,k) - CN(0,0,k)) + 
	         u*(CN(1,1,k) - CN(1,0,k));

	/* bilinear de Casteljau step */
        out[k] =  us*(vs*CN(0,0,k) + v*CN(0,1,k)) +
	           u*(vs*CN(1,0,k) + v*CN(1,1,k));
      }
    }
    else if(minorder == uorder)
    {
      for(k=0; k<dim; k++)
      {
	/* bilinear de Casteljau step */
	DCN(1,0) =    CN(1,0,k) -   CN(0,0,k);
	DCN(0,0) = us*CN(0,0,k) + u*CN(1,0,k);

	for(j=0; j<vorder-1; j++)
	{
	  /* for the derivative in u */
	  DCN(1,j+1) =    CN(1,j+1,k) -   CN(0,j+1,k);
	  DCN(1,j)   = vs*DCN(1,j)    + v*DCN(1,j+1);

	  /* for the `point' */
	  DCN(0,j+1) = us*CN(0,j+1,k) + u*CN(1,j+1,k);
	  DCN(0,j)   = vs*DCN(0,j)    + v*DCN(0,j+1);
	}
        
	/* remaining linear de Casteljau steps until the second last step */
	for(h=minorder; h<vorder-1; h++)
	  for(j=0; j<vorder-h; j++)
	  {
	    /* for the derivative in u */
	    DCN(1,j) = vs*DCN(1,j) + v*DCN(1,j+1);

	    /* for the `point' */
	    DCN(0,j) = vs*DCN(0,j) + v*DCN(0,j+1);
	  }

	/* derivative direction in v */
	dv[k] = DCN(0,1) - DCN(0,0);

	/* derivative direction in u */
	du[k] =   vs*DCN(1,0) + v*DCN(1,1);

	/* last linear de Casteljau step */
	out[k] =  vs*DCN(0,0) + v*DCN(0,1);
      }
    }
    else /* minorder == vorder */
    {
      for(k=0; k<dim; k++)
      {
	/* bilinear de Casteljau step */
	DCN(0,1) =    CN(0,1,k) -   CN(0,0,k);
	DCN(0,0) = vs*CN(0,0,k) + v*CN(0,1,k);
	for(i=0; i<uorder-1; i++)
	{
	  /* for the derivative in v */
	  DCN(i+1,1) =    CN(i+1,1,k) -   CN(i+1,0,k);
	  DCN(i,1)   = us*DCN(i,1)    + u*DCN(i+1,1);

	  /* for the `point' */
	  DCN(i+1,0) = vs*CN(i+1,0,k) + v*CN(i+1,1,k);
	  DCN(i,0)   = us*DCN(i,0)    + u*DCN(i+1,0);
	}
        
	/* remaining linear de Casteljau steps until the second last step */
	for(h=minorder; h<uorder-1; h++)
	  for(i=0; i<uorder-h; i++)
	  {
	    /* for the derivative in v */
	    DCN(i,1) = us*DCN(i,1) + u*DCN(i+1,1);

	    /* for the `point' */
	    DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  }

	/* derivative direction in u */
	du[k] = DCN(1,0) - DCN(0,0);

	/* derivative direction in v */
	dv[k] =   us*DCN(0,1) + u*DCN(1,1);

	/* last linear de Casteljau step */
	out[k] =  us*DCN(0,0) + u*DCN(1,0);
      }
    }
  }
  else if(uorder == vorder)
  {
    for(k=0; k<dim; k++)
    {
      /* first bilinear de Casteljau step */
      for(i=0; i<uorder-1; i++)
      {
	DCN(i,0) = us*CN(i,0,k) + u*CN(i+1,0,k);
	for(j=0; j<vorder-1; j++)
	{
	  DCN(i,j+1) = us*CN(i,j+1,k) + u*CN(i+1,j+1,k);
	  DCN(i,j)   = vs*DCN(i,j)    + v*DCN(i,j+1);
	}
      }

      /* remaining bilinear de Casteljau steps until the second last step */
      for(h=2; h<minorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  for(j=0; j<vorder-h; j++)
	  {
	    DCN(i,j+1) = us*DCN(i,j+1) + u*DCN(i+1,j+1);
	    DCN(i,j)   = vs*DCN(i,j)   + v*DCN(i,j+1);
	  }
	}

      /* derivative direction in u */
      du[k] = vs*(DCN(1,0) - DCN(0,0)) +
	       v*(DCN(1,1) - DCN(0,1));

      /* derivative direction in v */
      dv[k] = us*(DCN(0,1) - DCN(0,0)) + 
	       u*(DCN(1,1) - DCN(1,0));

      /* last bilinear de Casteljau step */
      out[k] =  us*(vs*DCN(0,0) + v*DCN(0,1)) +
	         u*(vs*DCN(1,0) + v*DCN(1,1));
    }
  }
  else if(minorder == uorder)
  {
    for(k=0; k<dim; k++)
    {
      /* first bilinear de Casteljau step */
      for(i=0; i<uorder-1; i++)
      {
	DCN(i,0) = us*CN(i,0,k) + u*CN(i+1,0,k);
	for(j=0; j<vorder-1; j++)
	{
	  DCN(i,j+1) = us*CN(i,j+1,k) + u*CN(i+1,j+1,k);
	  DCN(i,j)   = vs*DCN(i,j)    + v*DCN(i,j+1);
	}
      }

      /* remaining bilinear de Casteljau steps until the second last step */
      for(h=2; h<minorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  for(j=0; j<vorder-h; j++)
	  {
	    DCN(i,j+1) = us*DCN(i,j+1) + u*DCN(i+1,j+1);
	    DCN(i,j)   = vs*DCN(i,j)   + v*DCN(i,j+1);
	  }
	}

      /* last bilinear de Casteljau step */
      DCN(2,0) =    DCN(1,0) -   DCN(0,0);
      DCN(0,0) = us*DCN(0,0) + u*DCN(1,0);
      for(j=0; j<vorder-1; j++)
      {
	/* for the derivative in u */
	DCN(2,j+1) =    DCN(1,j+1) -    DCN(0,j+1);
	DCN(2,j)   = vs*DCN(2,j)    + v*DCN(2,j+1);
	
	/* for the `point' */
	DCN(0,j+1) = us*DCN(0,j+1 ) + u*DCN(1,j+1);
	DCN(0,j)   = vs*DCN(0,j)    + v*DCN(0,j+1);
      }
        
      /* remaining linear de Casteljau steps until the second last step */
      for(h=minorder; h<vorder-1; h++)
	for(j=0; j<vorder-h; j++)
	{
	  /* for the derivative in u */
	  DCN(2,j) = vs*DCN(2,j) + v*DCN(2,j+1);
	  
	  /* for the `point' */
	  DCN(0,j) = vs*DCN(0,j) + v*DCN(0,j+1);
	}
      
      /* derivative direction in v */
      dv[k] = DCN(0,1) - DCN(0,0);
      
      /* derivative direction in u */
      du[k] =   vs*DCN(2,0) + v*DCN(2,1);
      
      /* last linear de Casteljau step */
      out[k] =  vs*DCN(0,0) + v*DCN(0,1);
    }
  }
  else /* minorder == vorder */
  {
    for(k=0; k<dim; k++)
    {
      /* first bilinear de Casteljau step */
      for(i=0; i<uorder-1; i++)
      {
	DCN(i,0) = us*CN(i,0,k) + u*CN(i+1,0,k);
	for(j=0; j<vorder-1; j++)
	{
	  DCN(i,j+1) = us*CN(i,j+1,k) + u*CN(i+1,j+1,k);
	  DCN(i,j)   = vs*DCN(i,j)    + v*DCN(i,j+1);
	}
      }

      /* remaining bilinear de Casteljau steps until the second last step */
      for(h=2; h<minorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  for(j=0; j<vorder-h; j++)
	  {
	    DCN(i,j+1) = us*DCN(i,j+1) + u*DCN(i+1,j+1);
	    DCN(i,j)   = vs*DCN(i,j)   + v*DCN(i,j+1);
	  }
	}

      /* last bilinear de Casteljau step */
      DCN(0,2) =    DCN(0,1) -   DCN(0,0);
      DCN(0,0) = vs*DCN(0,0) + v*DCN(0,1);
      for(i=0; i<uorder-1; i++)
      {
	/* for the derivative in v */
	DCN(i+1,2) =    DCN(i+1,1)  -   DCN(i+1,0);
	DCN(i,2)   = us*DCN(i,2)    + u*DCN(i+1,2);
	
	/* for the `point' */
	DCN(i+1,0) = vs*DCN(i+1,0)  + v*DCN(i+1,1);
	DCN(i,0)   = us*DCN(i,0)    + u*DCN(i+1,0);
      }
      
      /* remaining linear de Casteljau steps until the second last step */
      for(h=minorder; h<uorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  /* for the derivative in v */
	  DCN(i,2) = us*DCN(i,2) + u*DCN(i+1,2);
	  
	  /* for the `point' */
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	}
      
      /* derivative direction in u */
      du[k] = DCN(1,0) - DCN(0,0);
      
      /* derivative direction in v */
      dv[k] =   us*DCN(0,2) + u*DCN(1,2);
      
      /* last linear de Casteljau step */
      out[k] =  us*DCN(0,0) + u*DCN(1,0);
    }
  }
#undef DCN
#undef CN
}

/*
 * Return the number of components per control point for any type of
 * evaluator.  Return 0 if bad target.
 */

static GLint components( GLenum target )
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
      default:				return 0;
   }
}


/**********************************************************************/
/***            Copy and deallocate control points                  ***/
/**********************************************************************/


/*
 * Copy 1-parametric evaluator control points from user-specified 
 * memory space to a buffer of contiguous control points.
 * Input:  see glMap1f for details
 * Return:  pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
GLfloat *gl_copy_map_points1f( GLenum target,
                               GLint ustride, GLint uorder,
                               const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLuint i, k, size = components(target);

   if (!points || size==0) {
      return NULL;
   }

   buffer = (GLfloat *) malloc(uorder * size * sizeof(GLfloat));

   if(buffer) 
      for(i=0, p=buffer; i<uorder; i++, points+=ustride)
	for(k=0; k<size; k++)
	  *p++ = points[k];

   return buffer;
}



/*
 * Same as above but convert doubles to floats.
 */
GLfloat *gl_copy_map_points1d( GLenum target,
			        GLint ustride, GLint uorder,
			        const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLuint i, k, size = components(target);

   if (!points || size==0) {
      return NULL;
   }

   buffer = (GLfloat *) malloc(uorder * size * sizeof(GLfloat));

   if(buffer)
      for(i=0, p=buffer; i<uorder; i++, points+=ustride)
	for(k=0; k<size; k++)
	  *p++ = (GLfloat) points[k];

   return buffer;
}



/*
 * Copy 2-parametric evaluator control points from user-specified 
 * memory space to a buffer of contiguous control points.
 * Additional memory is allocated to be used by the horner and
 * de Casteljau evaluation schemes.
 *
 * Input:  see glMap2f for details
 * Return:  pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
GLfloat *gl_copy_map_points2f( GLenum target,
			        GLint ustride, GLint uorder,
			        GLint vstride, GLint vorder,
			        const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLuint i, j, k, size, dsize, hsize;
   GLint uinc;

   size = components(target);

   if (!points || size==0) {
      return NULL;
   }

   /* max(uorder, vorder) additional points are used in      */
   /* horner evaluation and uorder*vorder additional */
   /* values are needed for de Casteljau                     */
   dsize = (uorder == 2 && vorder == 2)? 0 : uorder*vorder;
   hsize = (uorder > vorder ? uorder : vorder)*size;

   if(hsize>dsize)
     buffer = (GLfloat *) malloc((uorder*vorder*size+hsize)*sizeof(GLfloat));
   else
     buffer = (GLfloat *) malloc((uorder*vorder*size+dsize)*sizeof(GLfloat));

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
GLfloat *gl_copy_map_points2d(GLenum target,
                              GLint ustride, GLint uorder,
                              GLint vstride, GLint vorder,
                              const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLuint i, j, k, size, hsize, dsize;
   GLint uinc;

   size = components(target);

   if (!points || size==0) {
      return NULL;
   }

   /* max(uorder, vorder) additional points are used in      */
   /* horner evaluation and uorder*vorder additional */
   /* values are needed for de Casteljau                     */
   dsize = (uorder == 2 && vorder == 2)? 0 : uorder*vorder;
   hsize = (uorder > vorder ? uorder : vorder)*size;

   if(hsize>dsize)
     buffer = (GLfloat *) malloc((uorder*vorder*size+hsize)*sizeof(GLfloat));
   else
     buffer = (GLfloat *) malloc((uorder*vorder*size+dsize)*sizeof(GLfloat));

   /* compute the increment value for the u-loop */
   uinc = ustride - vorder*vstride;

   if (buffer) 
      for (i=0, p=buffer; i<uorder; i++, points += uinc)
	 for (j=0; j<vorder; j++, points += vstride)
	    for (k=0; k<size; k++)
	       *p++ = (GLfloat) points[k];

   return buffer;
}


/*
 * This function is called by the display list deallocator function to
 * specify that a given set of control points are no longer needed.
 */
void gl_free_control_points( GLcontext* ctx, GLenum target, GLfloat *data )
{
   struct gl_1d_map *map1 = NULL;
   struct gl_2d_map *map2 = NULL;

   switch (target) {
      case GL_MAP1_VERTEX_3:
         map1 = &ctx->EvalMap.Map1Vertex3;
         break;
      case GL_MAP1_VERTEX_4:
         map1 = &ctx->EvalMap.Map1Vertex4;
	 break;
      case GL_MAP1_INDEX:
         map1 = &ctx->EvalMap.Map1Index;
         break;
      case GL_MAP1_COLOR_4:
         map1 = &ctx->EvalMap.Map1Color4;
         break;
      case GL_MAP1_NORMAL:
         map1 = &ctx->EvalMap.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
         map1 = &ctx->EvalMap.Map1Texture1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
         map1 = &ctx->EvalMap.Map1Texture2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
         map1 = &ctx->EvalMap.Map1Texture3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
         map1 = &ctx->EvalMap.Map1Texture4;
	 break;
      case GL_MAP2_VERTEX_3:
         map2 = &ctx->EvalMap.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
         map2 = &ctx->EvalMap.Map2Vertex4;
	 break;
      case GL_MAP2_INDEX:
         map2 = &ctx->EvalMap.Map2Index;
	 break;
      case GL_MAP2_COLOR_4:
         map2 = &ctx->EvalMap.Map2Color4;
         break;
      case GL_MAP2_NORMAL:
         map2 = &ctx->EvalMap.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
         map2 = &ctx->EvalMap.Map2Texture1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
         map2 = &ctx->EvalMap.Map2Texture2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
         map2 = &ctx->EvalMap.Map2Texture3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
         map2 = &ctx->EvalMap.Map2Texture4;
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "gl_free_control_points" );
         return;
   }

   if (map1) {
      if (data==map1->Points) {
         /* The control points in the display list are currently */
         /* being used so we can mark them as discard-able. */
         map1->Retain = GL_FALSE;
      }
      else {
         /* The control points in the display list are not currently */
         /* being used. */
         free( data );
      }
   }
   if (map2) {
      if (data==map2->Points) {
         /* The control points in the display list are currently */
         /* being used so we can mark them as discard-able. */
         map2->Retain = GL_FALSE;
      }
      else {
         /* The control points in the display list are not currently */
         /* being used. */
         free( data );
      }
   }

}



/**********************************************************************/
/***                      API entry points                          ***/
/**********************************************************************/


/*
 * Note that the array of control points must be 'unpacked' at this time.
 * Input:  retain - if TRUE, this control point data is also in a display
 *                  list and can't be freed until the list is freed.
 */
void gl_Map1f( GLcontext* ctx, GLenum target,
               GLfloat u1, GLfloat u2, GLint stride,
               GLint order, const GLfloat *points, GLboolean retain )
{
   GLuint k;

   if (!points) {
      gl_error( ctx, GL_OUT_OF_MEMORY, "glMap1f" );
      return;
   }

   /* may be a new stride after copying control points */
   stride = components( target );

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glMap1" );
      return;
   }

   if (u1==u2) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap1(u1,u2)" );
      return;
   }

   if (order<1 || order>MAX_EVAL_ORDER) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap1(order)" );
      return;
   }

   k = components( target );
   if (k==0) {
      gl_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
   }

   if (stride < k) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap1(stride)" );
      return;
   }

   switch (target) {
      case GL_MAP1_VERTEX_3:
         ctx->EvalMap.Map1Vertex3.Order = order;
	 ctx->EvalMap.Map1Vertex3.u1 = u1;
	 ctx->EvalMap.Map1Vertex3.u2 = u2;
	 if (ctx->EvalMap.Map1Vertex3.Points
             && !ctx->EvalMap.Map1Vertex3.Retain) {
	    free( ctx->EvalMap.Map1Vertex3.Points );
	 }
	 ctx->EvalMap.Map1Vertex3.Points = (GLfloat *) points;
         ctx->EvalMap.Map1Vertex3.Retain = retain;
	 break;
      case GL_MAP1_VERTEX_4:
         ctx->EvalMap.Map1Vertex4.Order = order;
	 ctx->EvalMap.Map1Vertex4.u1 = u1;
	 ctx->EvalMap.Map1Vertex4.u2 = u2;
	 if (ctx->EvalMap.Map1Vertex4.Points
             && !ctx->EvalMap.Map1Vertex4.Retain) {
	    free( ctx->EvalMap.Map1Vertex4.Points );
	 }
	 ctx->EvalMap.Map1Vertex4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Vertex4.Retain = retain;
	 break;
      case GL_MAP1_INDEX:
         ctx->EvalMap.Map1Index.Order = order;
	 ctx->EvalMap.Map1Index.u1 = u1;
	 ctx->EvalMap.Map1Index.u2 = u2;
	 if (ctx->EvalMap.Map1Index.Points
             && !ctx->EvalMap.Map1Index.Retain) {
	    free( ctx->EvalMap.Map1Index.Points );
	 }
	 ctx->EvalMap.Map1Index.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Index.Retain = retain;
	 break;
      case GL_MAP1_COLOR_4:
         ctx->EvalMap.Map1Color4.Order = order;
	 ctx->EvalMap.Map1Color4.u1 = u1;
	 ctx->EvalMap.Map1Color4.u2 = u2;
	 if (ctx->EvalMap.Map1Color4.Points
             && !ctx->EvalMap.Map1Color4.Retain) {
	    free( ctx->EvalMap.Map1Color4.Points );
	 }
	 ctx->EvalMap.Map1Color4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Color4.Retain = retain;
	 break;
      case GL_MAP1_NORMAL:
         ctx->EvalMap.Map1Normal.Order = order;
	 ctx->EvalMap.Map1Normal.u1 = u1;
	 ctx->EvalMap.Map1Normal.u2 = u2;
	 if (ctx->EvalMap.Map1Normal.Points
             && !ctx->EvalMap.Map1Normal.Retain) {
	    free( ctx->EvalMap.Map1Normal.Points );
	 }
	 ctx->EvalMap.Map1Normal.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Normal.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
         ctx->EvalMap.Map1Texture1.Order = order;
	 ctx->EvalMap.Map1Texture1.u1 = u1;
	 ctx->EvalMap.Map1Texture1.u2 = u2;
	 if (ctx->EvalMap.Map1Texture1.Points
             && !ctx->EvalMap.Map1Texture1.Retain) {
	    free( ctx->EvalMap.Map1Texture1.Points );
	 }
	 ctx->EvalMap.Map1Texture1.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture1.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
         ctx->EvalMap.Map1Texture2.Order = order;
	 ctx->EvalMap.Map1Texture2.u1 = u1;
	 ctx->EvalMap.Map1Texture2.u2 = u2;
	 if (ctx->EvalMap.Map1Texture2.Points
             && !ctx->EvalMap.Map1Texture2.Retain) {
	    free( ctx->EvalMap.Map1Texture2.Points );
	 }
	 ctx->EvalMap.Map1Texture2.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture2.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
         ctx->EvalMap.Map1Texture3.Order = order;
	 ctx->EvalMap.Map1Texture3.u1 = u1;
	 ctx->EvalMap.Map1Texture3.u2 = u2;
	 if (ctx->EvalMap.Map1Texture3.Points
             && !ctx->EvalMap.Map1Texture3.Retain) {
	    free( ctx->EvalMap.Map1Texture3.Points );
	 }
	 ctx->EvalMap.Map1Texture3.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture3.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
         ctx->EvalMap.Map1Texture4.Order = order;
	 ctx->EvalMap.Map1Texture4.u1 = u1;
	 ctx->EvalMap.Map1Texture4.u2 = u2;
	 if (ctx->EvalMap.Map1Texture4.Points
             && !ctx->EvalMap.Map1Texture4.Retain) {
	    free( ctx->EvalMap.Map1Texture4.Points );
	 }
	 ctx->EvalMap.Map1Texture4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture4.Retain = retain;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
   }
}




/*
 * Note that the array of control points must be 'unpacked' at this time.
 * Input:  retain - if TRUE, this control point data is also in a display
 *                  list and can't be freed until the list is freed.
 */
void gl_Map2f( GLcontext* ctx, GLenum target,
	      GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
	      GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
	      const GLfloat *points, GLboolean retain )
{
   GLuint k;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glMap2" );
      return;
   }

   if (u1==u2) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(u1,u2)" );
      return;
   }

   if (v1==v2) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(v1,v2)" );
      return;
   }

   if (uorder<1 || uorder>MAX_EVAL_ORDER) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(uorder)" );
      return;
   }

   if (vorder<1 || vorder>MAX_EVAL_ORDER) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(vorder)" );
      return;
   }

   k = components( target );
   if (k==0) {
      gl_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
   }

   if (ustride < k) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(ustride)" );
      return;
   }
   if (vstride < k) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(vstride)" );
      return;
   }

   switch (target) {
      case GL_MAP2_VERTEX_3:
         ctx->EvalMap.Map2Vertex3.Uorder = uorder;
	 ctx->EvalMap.Map2Vertex3.u1 = u1;
	 ctx->EvalMap.Map2Vertex3.u2 = u2;
         ctx->EvalMap.Map2Vertex3.Vorder = vorder;
	 ctx->EvalMap.Map2Vertex3.v1 = v1;
	 ctx->EvalMap.Map2Vertex3.v2 = v2;
	 if (ctx->EvalMap.Map2Vertex3.Points
             && !ctx->EvalMap.Map2Vertex3.Retain) {
	    free( ctx->EvalMap.Map2Vertex3.Points );
	 }
	 ctx->EvalMap.Map2Vertex3.Retain = retain;
	 ctx->EvalMap.Map2Vertex3.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_VERTEX_4:
         ctx->EvalMap.Map2Vertex4.Uorder = uorder;
	 ctx->EvalMap.Map2Vertex4.u1 = u1;
	 ctx->EvalMap.Map2Vertex4.u2 = u2;
         ctx->EvalMap.Map2Vertex4.Vorder = vorder;
	 ctx->EvalMap.Map2Vertex4.v1 = v1;
	 ctx->EvalMap.Map2Vertex4.v2 = v2;
	 if (ctx->EvalMap.Map2Vertex4.Points
             && !ctx->EvalMap.Map2Vertex4.Retain) {
	    free( ctx->EvalMap.Map2Vertex4.Points );
	 }
	 ctx->EvalMap.Map2Vertex4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map2Vertex4.Retain = retain;
	 break;
      case GL_MAP2_INDEX:
         ctx->EvalMap.Map2Index.Uorder = uorder;
	 ctx->EvalMap.Map2Index.u1 = u1;
	 ctx->EvalMap.Map2Index.u2 = u2;
         ctx->EvalMap.Map2Index.Vorder = vorder;
	 ctx->EvalMap.Map2Index.v1 = v1;
	 ctx->EvalMap.Map2Index.v2 = v2;
	 if (ctx->EvalMap.Map2Index.Points
             && !ctx->EvalMap.Map2Index.Retain) {
	    free( ctx->EvalMap.Map2Index.Points );
	 }
	 ctx->EvalMap.Map2Index.Retain = retain;
	 ctx->EvalMap.Map2Index.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_COLOR_4:
         ctx->EvalMap.Map2Color4.Uorder = uorder;
	 ctx->EvalMap.Map2Color4.u1 = u1;
	 ctx->EvalMap.Map2Color4.u2 = u2;
         ctx->EvalMap.Map2Color4.Vorder = vorder;
	 ctx->EvalMap.Map2Color4.v1 = v1;
	 ctx->EvalMap.Map2Color4.v2 = v2;
	 if (ctx->EvalMap.Map2Color4.Points
             && !ctx->EvalMap.Map2Color4.Retain) {
	    free( ctx->EvalMap.Map2Color4.Points );
	 }
	 ctx->EvalMap.Map2Color4.Retain = retain;
	 ctx->EvalMap.Map2Color4.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_NORMAL:
         ctx->EvalMap.Map2Normal.Uorder = uorder;
	 ctx->EvalMap.Map2Normal.u1 = u1;
	 ctx->EvalMap.Map2Normal.u2 = u2;
         ctx->EvalMap.Map2Normal.Vorder = vorder;
	 ctx->EvalMap.Map2Normal.v1 = v1;
	 ctx->EvalMap.Map2Normal.v2 = v2;
	 if (ctx->EvalMap.Map2Normal.Points
             && !ctx->EvalMap.Map2Normal.Retain) {
	    free( ctx->EvalMap.Map2Normal.Points );
	 }
	 ctx->EvalMap.Map2Normal.Retain = retain;
	 ctx->EvalMap.Map2Normal.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
         ctx->EvalMap.Map2Texture1.Uorder = uorder;
	 ctx->EvalMap.Map2Texture1.u1 = u1;
	 ctx->EvalMap.Map2Texture1.u2 = u2;
         ctx->EvalMap.Map2Texture1.Vorder = vorder;
	 ctx->EvalMap.Map2Texture1.v1 = v1;
	 ctx->EvalMap.Map2Texture1.v2 = v2;
	 if (ctx->EvalMap.Map2Texture1.Points
             && !ctx->EvalMap.Map2Texture1.Retain) {
	    free( ctx->EvalMap.Map2Texture1.Points );
	 }
	 ctx->EvalMap.Map2Texture1.Retain = retain;
	 ctx->EvalMap.Map2Texture1.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
         ctx->EvalMap.Map2Texture2.Uorder = uorder;
	 ctx->EvalMap.Map2Texture2.u1 = u1;
	 ctx->EvalMap.Map2Texture2.u2 = u2;
         ctx->EvalMap.Map2Texture2.Vorder = vorder;
	 ctx->EvalMap.Map2Texture2.v1 = v1;
	 ctx->EvalMap.Map2Texture2.v2 = v2;
	 if (ctx->EvalMap.Map2Texture2.Points
             && !ctx->EvalMap.Map2Texture2.Retain) {
	    free( ctx->EvalMap.Map2Texture2.Points );
	 }
	 ctx->EvalMap.Map2Texture2.Retain = retain;
	 ctx->EvalMap.Map2Texture2.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
         ctx->EvalMap.Map2Texture3.Uorder = uorder;
	 ctx->EvalMap.Map2Texture3.u1 = u1;
	 ctx->EvalMap.Map2Texture3.u2 = u2;
         ctx->EvalMap.Map2Texture3.Vorder = vorder;
	 ctx->EvalMap.Map2Texture3.v1 = v1;
	 ctx->EvalMap.Map2Texture3.v2 = v2;
	 if (ctx->EvalMap.Map2Texture3.Points
             && !ctx->EvalMap.Map2Texture3.Retain) {
	    free( ctx->EvalMap.Map2Texture3.Points );
	 }
	 ctx->EvalMap.Map2Texture3.Retain = retain;
	 ctx->EvalMap.Map2Texture3.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
         ctx->EvalMap.Map2Texture4.Uorder = uorder;
	 ctx->EvalMap.Map2Texture4.u1 = u1;
	 ctx->EvalMap.Map2Texture4.u2 = u2;
         ctx->EvalMap.Map2Texture4.Vorder = vorder;
	 ctx->EvalMap.Map2Texture4.v1 = v1;
	 ctx->EvalMap.Map2Texture4.v2 = v2;
	 if (ctx->EvalMap.Map2Texture4.Points
             && !ctx->EvalMap.Map2Texture4.Retain) {
	    free( ctx->EvalMap.Map2Texture4.Points );
	 }
	 ctx->EvalMap.Map2Texture4.Retain = retain;
	 ctx->EvalMap.Map2Texture4.Points = (GLfloat *) points;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
   }
}


   


void gl_GetMapdv( GLcontext* ctx, GLenum target, GLenum query, GLdouble *v )
{
   GLuint i, n;
   GLfloat *data;

   switch (query) {
      case GL_COEFF:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       data = ctx->EvalMap.Map1Color4.Points;
	       n = ctx->EvalMap.Map1Color4.Order * 4;
	       break;
	    case GL_MAP1_INDEX:
	       data = ctx->EvalMap.Map1Index.Points;
	       n = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       data = ctx->EvalMap.Map1Normal.Points;
	       n = ctx->EvalMap.Map1Normal.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map1Texture1.Points;
	       n = ctx->EvalMap.Map1Texture1.Order * 1;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map1Texture2.Points;
	       n = ctx->EvalMap.Map1Texture2.Order * 2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map1Texture3.Points;
	       n = ctx->EvalMap.Map1Texture3.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map1Texture4.Points;
	       n = ctx->EvalMap.Map1Texture4.Order * 4;
	       break;
	    case GL_MAP1_VERTEX_3:
	       data = ctx->EvalMap.Map1Vertex3.Points;
	       n = ctx->EvalMap.Map1Vertex3.Order * 3;
	       break;
	    case GL_MAP1_VERTEX_4:
	       data = ctx->EvalMap.Map1Vertex4.Points;
	       n = ctx->EvalMap.Map1Vertex4.Order * 4;
	       break;
	    case GL_MAP2_COLOR_4:
	       data = ctx->EvalMap.Map2Color4.Points;
	       n = ctx->EvalMap.Map2Color4.Uorder
                 * ctx->EvalMap.Map2Color4.Vorder * 4;
	       break;
	    case GL_MAP2_INDEX:
	       data = ctx->EvalMap.Map2Index.Points;
	       n = ctx->EvalMap.Map2Index.Uorder
                 * ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       data = ctx->EvalMap.Map2Normal.Points;
	       n = ctx->EvalMap.Map2Normal.Uorder
                 * ctx->EvalMap.Map2Normal.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map2Texture1.Points;
	       n = ctx->EvalMap.Map2Texture1.Uorder
                 * ctx->EvalMap.Map2Texture1.Vorder * 1;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map2Texture2.Points;
	       n = ctx->EvalMap.Map2Texture2.Uorder
                 * ctx->EvalMap.Map2Texture2.Vorder * 2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map2Texture3.Points;
	       n = ctx->EvalMap.Map2Texture3.Uorder
                 * ctx->EvalMap.Map2Texture3.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map2Texture4.Points;
	       n = ctx->EvalMap.Map2Texture4.Uorder
                 * ctx->EvalMap.Map2Texture4.Vorder * 4;
	       break;
	    case GL_MAP2_VERTEX_3:
	       data = ctx->EvalMap.Map2Vertex3.Points;
	       n = ctx->EvalMap.Map2Vertex3.Uorder
                 * ctx->EvalMap.Map2Vertex3.Vorder * 3;
	       break;
	    case GL_MAP2_VERTEX_4:
	       data = ctx->EvalMap.Map2Vertex4.Points;
	       n = ctx->EvalMap.Map2Vertex4.Uorder
                 * ctx->EvalMap.Map2Vertex4.Vorder * 4;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
	 }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = data[i];
	    }
	 }
         break;
      case GL_ORDER:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       *v = ctx->EvalMap.Map1Color4.Order;
	       break;
	    case GL_MAP1_INDEX:
	       *v = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       *v = ctx->EvalMap.Map1Normal.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       *v = ctx->EvalMap.Map1Texture1.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       *v = ctx->EvalMap.Map1Texture2.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       *v = ctx->EvalMap.Map1Texture3.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       *v = ctx->EvalMap.Map1Texture4.Order;
	       break;
	    case GL_MAP1_VERTEX_3:
	       *v = ctx->EvalMap.Map1Vertex3.Order;
	       break;
	    case GL_MAP1_VERTEX_4:
	       *v = ctx->EvalMap.Map1Vertex4.Order;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.Uorder;
	       v[1] = ctx->EvalMap.Map2Color4.Vorder;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.Uorder;
	       v[1] = ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.Uorder;
	       v[1] = ctx->EvalMap.Map2Normal.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture1.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture2.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture3.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture4.Vorder;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex3.Vorder;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex4.Vorder;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
	 }
         break;
      case GL_DOMAIN:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       v[0] = ctx->EvalMap.Map1Color4.u1;
	       v[1] = ctx->EvalMap.Map1Color4.u2;
	       break;
	    case GL_MAP1_INDEX:
	       v[0] = ctx->EvalMap.Map1Index.u1;
	       v[1] = ctx->EvalMap.Map1Index.u2;
	       break;
	    case GL_MAP1_NORMAL:
	       v[0] = ctx->EvalMap.Map1Normal.u1;
	       v[1] = ctx->EvalMap.Map1Normal.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map1Texture1.u1;
	       v[1] = ctx->EvalMap.Map1Texture1.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map1Texture2.u1;
	       v[1] = ctx->EvalMap.Map1Texture2.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map1Texture3.u1;
	       v[1] = ctx->EvalMap.Map1Texture3.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map1Texture4.u1;
	       v[1] = ctx->EvalMap.Map1Texture4.u2;
	       break;
	    case GL_MAP1_VERTEX_3:
	       v[0] = ctx->EvalMap.Map1Vertex3.u1;
	       v[1] = ctx->EvalMap.Map1Vertex3.u2;
	       break;
	    case GL_MAP1_VERTEX_4:
	       v[0] = ctx->EvalMap.Map1Vertex4.u1;
	       v[1] = ctx->EvalMap.Map1Vertex4.u2;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.u1;
	       v[1] = ctx->EvalMap.Map2Color4.u2;
	       v[2] = ctx->EvalMap.Map2Color4.v1;
	       v[3] = ctx->EvalMap.Map2Color4.v2;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.u1;
	       v[1] = ctx->EvalMap.Map2Index.u2;
	       v[2] = ctx->EvalMap.Map2Index.v1;
	       v[3] = ctx->EvalMap.Map2Index.v2;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.u1;
	       v[1] = ctx->EvalMap.Map2Normal.u2;
	       v[2] = ctx->EvalMap.Map2Normal.v1;
	       v[3] = ctx->EvalMap.Map2Normal.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.u1;
	       v[1] = ctx->EvalMap.Map2Texture1.u2;
	       v[2] = ctx->EvalMap.Map2Texture1.v1;
	       v[3] = ctx->EvalMap.Map2Texture1.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.u1;
	       v[1] = ctx->EvalMap.Map2Texture2.u2;
	       v[2] = ctx->EvalMap.Map2Texture2.v1;
	       v[3] = ctx->EvalMap.Map2Texture2.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.u1;
	       v[1] = ctx->EvalMap.Map2Texture3.u2;
	       v[2] = ctx->EvalMap.Map2Texture3.v1;
	       v[3] = ctx->EvalMap.Map2Texture3.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.u1;
	       v[1] = ctx->EvalMap.Map2Texture4.u2;
	       v[2] = ctx->EvalMap.Map2Texture4.v1;
	       v[3] = ctx->EvalMap.Map2Texture4.v2;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.u1;
	       v[1] = ctx->EvalMap.Map2Vertex3.u2;
	       v[2] = ctx->EvalMap.Map2Vertex3.v1;
	       v[3] = ctx->EvalMap.Map2Vertex3.v2;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.u1;
	       v[1] = ctx->EvalMap.Map2Vertex4.u2;
	       v[2] = ctx->EvalMap.Map2Vertex4.v1;
	       v[3] = ctx->EvalMap.Map2Vertex4.v2;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(query)" );
   }
}


void gl_GetMapfv( GLcontext* ctx, GLenum target, GLenum query, GLfloat *v )
{
   GLuint i, n;
   GLfloat *data;

   switch (query) {
      case GL_COEFF:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       data = ctx->EvalMap.Map1Color4.Points;
	       n = ctx->EvalMap.Map1Color4.Order * 4;
	       break;
	    case GL_MAP1_INDEX:
	       data = ctx->EvalMap.Map1Index.Points;
	       n = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       data = ctx->EvalMap.Map1Normal.Points;
	       n = ctx->EvalMap.Map1Normal.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map1Texture1.Points;
	       n = ctx->EvalMap.Map1Texture1.Order * 1;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map1Texture2.Points;
	       n = ctx->EvalMap.Map1Texture2.Order * 2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map1Texture3.Points;
	       n = ctx->EvalMap.Map1Texture3.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map1Texture4.Points;
	       n = ctx->EvalMap.Map1Texture4.Order * 4;
	       break;
	    case GL_MAP1_VERTEX_3:
	       data = ctx->EvalMap.Map1Vertex3.Points;
	       n = ctx->EvalMap.Map1Vertex3.Order * 3;
	       break;
	    case GL_MAP1_VERTEX_4:
	       data = ctx->EvalMap.Map1Vertex4.Points;
	       n = ctx->EvalMap.Map1Vertex4.Order * 4;
	       break;
	    case GL_MAP2_COLOR_4:
	       data = ctx->EvalMap.Map2Color4.Points;
	       n = ctx->EvalMap.Map2Color4.Uorder
                 * ctx->EvalMap.Map2Color4.Vorder * 4;
	       break;
	    case GL_MAP2_INDEX:
	       data = ctx->EvalMap.Map2Index.Points;
	       n = ctx->EvalMap.Map2Index.Uorder
                 * ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       data = ctx->EvalMap.Map2Normal.Points;
	       n = ctx->EvalMap.Map2Normal.Uorder
                 * ctx->EvalMap.Map2Normal.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map2Texture1.Points;
	       n = ctx->EvalMap.Map2Texture1.Uorder
                 * ctx->EvalMap.Map2Texture1.Vorder * 1;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map2Texture2.Points;
	       n = ctx->EvalMap.Map2Texture2.Uorder
                 * ctx->EvalMap.Map2Texture2.Vorder * 2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map2Texture3.Points;
	       n = ctx->EvalMap.Map2Texture3.Uorder
                 * ctx->EvalMap.Map2Texture3.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map2Texture4.Points;
	       n = ctx->EvalMap.Map2Texture4.Uorder
                 * ctx->EvalMap.Map2Texture4.Vorder * 4;
	       break;
	    case GL_MAP2_VERTEX_3:
	       data = ctx->EvalMap.Map2Vertex3.Points;
	       n = ctx->EvalMap.Map2Vertex3.Uorder
                 * ctx->EvalMap.Map2Vertex3.Vorder * 3;
	       break;
	    case GL_MAP2_VERTEX_4:
	       data = ctx->EvalMap.Map2Vertex4.Points;
	       n = ctx->EvalMap.Map2Vertex4.Uorder
                 * ctx->EvalMap.Map2Vertex4.Vorder * 4;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
	 }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = data[i];
	    }
	 }
         break;
      case GL_ORDER:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       *v = ctx->EvalMap.Map1Color4.Order;
	       break;
	    case GL_MAP1_INDEX:
	       *v = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       *v = ctx->EvalMap.Map1Normal.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       *v = ctx->EvalMap.Map1Texture1.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       *v = ctx->EvalMap.Map1Texture2.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       *v = ctx->EvalMap.Map1Texture3.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       *v = ctx->EvalMap.Map1Texture4.Order;
	       break;
	    case GL_MAP1_VERTEX_3:
	       *v = ctx->EvalMap.Map1Vertex3.Order;
	       break;
	    case GL_MAP1_VERTEX_4:
	       *v = ctx->EvalMap.Map1Vertex4.Order;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.Uorder;
	       v[1] = ctx->EvalMap.Map2Color4.Vorder;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.Uorder;
	       v[1] = ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.Uorder;
	       v[1] = ctx->EvalMap.Map2Normal.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture1.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture2.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture3.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture4.Vorder;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex3.Vorder;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex4.Vorder;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
	 }
         break;
      case GL_DOMAIN:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       v[0] = ctx->EvalMap.Map1Color4.u1;
	       v[1] = ctx->EvalMap.Map1Color4.u2;
	       break;
	    case GL_MAP1_INDEX:
	       v[0] = ctx->EvalMap.Map1Index.u1;
	       v[1] = ctx->EvalMap.Map1Index.u2;
	       break;
	    case GL_MAP1_NORMAL:
	       v[0] = ctx->EvalMap.Map1Normal.u1;
	       v[1] = ctx->EvalMap.Map1Normal.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map1Texture1.u1;
	       v[1] = ctx->EvalMap.Map1Texture1.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map1Texture2.u1;
	       v[1] = ctx->EvalMap.Map1Texture2.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map1Texture3.u1;
	       v[1] = ctx->EvalMap.Map1Texture3.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map1Texture4.u1;
	       v[1] = ctx->EvalMap.Map1Texture4.u2;
	       break;
	    case GL_MAP1_VERTEX_3:
	       v[0] = ctx->EvalMap.Map1Vertex3.u1;
	       v[1] = ctx->EvalMap.Map1Vertex3.u2;
	       break;
	    case GL_MAP1_VERTEX_4:
	       v[0] = ctx->EvalMap.Map1Vertex4.u1;
	       v[1] = ctx->EvalMap.Map1Vertex4.u2;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.u1;
	       v[1] = ctx->EvalMap.Map2Color4.u2;
	       v[2] = ctx->EvalMap.Map2Color4.v1;
	       v[3] = ctx->EvalMap.Map2Color4.v2;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.u1;
	       v[1] = ctx->EvalMap.Map2Index.u2;
	       v[2] = ctx->EvalMap.Map2Index.v1;
	       v[3] = ctx->EvalMap.Map2Index.v2;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.u1;
	       v[1] = ctx->EvalMap.Map2Normal.u2;
	       v[2] = ctx->EvalMap.Map2Normal.v1;
	       v[3] = ctx->EvalMap.Map2Normal.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.u1;
	       v[1] = ctx->EvalMap.Map2Texture1.u2;
	       v[2] = ctx->EvalMap.Map2Texture1.v1;
	       v[3] = ctx->EvalMap.Map2Texture1.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.u1;
	       v[1] = ctx->EvalMap.Map2Texture2.u2;
	       v[2] = ctx->EvalMap.Map2Texture2.v1;
	       v[3] = ctx->EvalMap.Map2Texture2.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.u1;
	       v[1] = ctx->EvalMap.Map2Texture3.u2;
	       v[2] = ctx->EvalMap.Map2Texture3.v1;
	       v[3] = ctx->EvalMap.Map2Texture3.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.u1;
	       v[1] = ctx->EvalMap.Map2Texture4.u2;
	       v[2] = ctx->EvalMap.Map2Texture4.v1;
	       v[3] = ctx->EvalMap.Map2Texture4.v2;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.u1;
	       v[1] = ctx->EvalMap.Map2Vertex3.u2;
	       v[2] = ctx->EvalMap.Map2Vertex3.v1;
	       v[3] = ctx->EvalMap.Map2Vertex3.v2;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.u1;
	       v[1] = ctx->EvalMap.Map2Vertex4.u2;
	       v[2] = ctx->EvalMap.Map2Vertex4.v1;
	       v[3] = ctx->EvalMap.Map2Vertex4.v2;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(query)" );
   }
}


void gl_GetMapiv( GLcontext* ctx, GLenum target, GLenum query, GLint *v )
{
   GLuint i, n;
   GLfloat *data;

   switch (query) {
      case GL_COEFF:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       data = ctx->EvalMap.Map1Color4.Points;
	       n = ctx->EvalMap.Map1Color4.Order * 4;
	       break;
	    case GL_MAP1_INDEX:
	       data = ctx->EvalMap.Map1Index.Points;
	       n = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       data = ctx->EvalMap.Map1Normal.Points;
	       n = ctx->EvalMap.Map1Normal.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map1Texture1.Points;
	       n = ctx->EvalMap.Map1Texture1.Order * 1;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map1Texture2.Points;
	       n = ctx->EvalMap.Map1Texture2.Order * 2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map1Texture3.Points;
	       n = ctx->EvalMap.Map1Texture3.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map1Texture4.Points;
	       n = ctx->EvalMap.Map1Texture4.Order * 4;
	       break;
	    case GL_MAP1_VERTEX_3:
	       data = ctx->EvalMap.Map1Vertex3.Points;
	       n = ctx->EvalMap.Map1Vertex3.Order * 3;
	       break;
	    case GL_MAP1_VERTEX_4:
	       data = ctx->EvalMap.Map1Vertex4.Points;
	       n = ctx->EvalMap.Map1Vertex4.Order * 4;
	       break;
	    case GL_MAP2_COLOR_4:
	       data = ctx->EvalMap.Map2Color4.Points;
	       n = ctx->EvalMap.Map2Color4.Uorder
                 * ctx->EvalMap.Map2Color4.Vorder * 4;
	       break;
	    case GL_MAP2_INDEX:
	       data = ctx->EvalMap.Map2Index.Points;
	       n = ctx->EvalMap.Map2Index.Uorder
                 * ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       data = ctx->EvalMap.Map2Normal.Points;
	       n = ctx->EvalMap.Map2Normal.Uorder
                 * ctx->EvalMap.Map2Normal.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map2Texture1.Points;
	       n = ctx->EvalMap.Map2Texture1.Uorder
                 * ctx->EvalMap.Map2Texture1.Vorder * 1;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map2Texture2.Points;
	       n = ctx->EvalMap.Map2Texture2.Uorder
                 * ctx->EvalMap.Map2Texture2.Vorder * 2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map2Texture3.Points;
	       n = ctx->EvalMap.Map2Texture3.Uorder
                 * ctx->EvalMap.Map2Texture3.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map2Texture4.Points;
	       n = ctx->EvalMap.Map2Texture4.Uorder
                 * ctx->EvalMap.Map2Texture4.Vorder * 4;
	       break;
	    case GL_MAP2_VERTEX_3:
	       data = ctx->EvalMap.Map2Vertex3.Points;
	       n = ctx->EvalMap.Map2Vertex3.Uorder
                 * ctx->EvalMap.Map2Vertex3.Vorder * 3;
	       break;
	    case GL_MAP2_VERTEX_4:
	       data = ctx->EvalMap.Map2Vertex4.Points;
	       n = ctx->EvalMap.Map2Vertex4.Uorder
                 * ctx->EvalMap.Map2Vertex4.Vorder * 4;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
	 }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = ROUNDF(data[i]);
	    }
	 }
         break;
      case GL_ORDER:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       *v = ctx->EvalMap.Map1Color4.Order;
	       break;
	    case GL_MAP1_INDEX:
	       *v = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       *v = ctx->EvalMap.Map1Normal.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       *v = ctx->EvalMap.Map1Texture1.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       *v = ctx->EvalMap.Map1Texture2.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       *v = ctx->EvalMap.Map1Texture3.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       *v = ctx->EvalMap.Map1Texture4.Order;
	       break;
	    case GL_MAP1_VERTEX_3:
	       *v = ctx->EvalMap.Map1Vertex3.Order;
	       break;
	    case GL_MAP1_VERTEX_4:
	       *v = ctx->EvalMap.Map1Vertex4.Order;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.Uorder;
	       v[1] = ctx->EvalMap.Map2Color4.Vorder;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.Uorder;
	       v[1] = ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.Uorder;
	       v[1] = ctx->EvalMap.Map2Normal.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture1.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture2.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture3.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture4.Vorder;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex3.Vorder;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex4.Vorder;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
	 }
         break;
      case GL_DOMAIN:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Color4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Color4.u2);
	       break;
	    case GL_MAP1_INDEX:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Index.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Index.u2);
	       break;
	    case GL_MAP1_NORMAL:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Normal.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Normal.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture1.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture1.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture2.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture2.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture3.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture4.u2);
	       break;
	    case GL_MAP1_VERTEX_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Vertex3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Vertex3.u2);
	       break;
	    case GL_MAP1_VERTEX_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Vertex4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Vertex4.u2);
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Color4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Color4.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Color4.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Color4.v2);
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Index.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Index.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Index.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Index.v2);
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Normal.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Normal.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Normal.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Normal.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture1.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture1.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture1.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture1.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture2.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture2.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture2.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture2.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture3.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture3.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture3.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture4.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture4.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture4.v2);
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Vertex3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Vertex3.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Vertex3.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Vertex3.v2);
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Vertex4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Vertex4.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Vertex4.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Vertex4.v2);
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(query)" );
   }
}



void gl_EvalCoord1f(GLcontext* ctx, GLfloat u)
{
  GLfloat vertex[4];
  GLfloat normal[3];
  GLfloat fcolor[4];
  GLubyte icolor[4];
  GLubyte *colorptr;
  GLfloat texcoord[4];
  GLuint index;
  register GLfloat uu;

  /** Vertex **/
  if (ctx->Eval.Map1Vertex4) 
  {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Vertex4;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, vertex, uu, 4, map->Order);
  }
  else if (ctx->Eval.Map1Vertex3) 
  {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Vertex3;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, vertex, uu, 3, map->Order);
     vertex[3] = 1.0;
  }

  /** Color Index **/
  if (ctx->Eval.Map1Index) 
  {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Index;
     GLfloat findex;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, &findex, uu, 1, map->Order);
     index = (GLuint) (GLint) findex;
  }
  else {
     index = ctx->Current.Index;
  }

  /** Color **/
  if (ctx->Eval.Map1Color4) {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Color4;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, fcolor, uu, 4, map->Order);
     icolor[0] = (GLint) (fcolor[0] * ctx->Visual->RedScale);
     icolor[1] = (GLint) (fcolor[1] * ctx->Visual->GreenScale);
     icolor[2] = (GLint) (fcolor[2] * ctx->Visual->BlueScale);
     icolor[3] = (GLint) (fcolor[3] * ctx->Visual->AlphaScale);
     colorptr = icolor;
  }
  else {
     GLubyte col[4];
     COPY_4V(col, ctx->Current.ByteColor );
     colorptr = col;
  }

  /** Normal Vector **/
  if (ctx->Eval.Map1Normal) {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Normal;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, normal, uu, 3, map->Order);
  }
  else {
    normal[0] = ctx->Current.Normal[0];
    normal[1] = ctx->Current.Normal[1];
    normal[2] = ctx->Current.Normal[2];
  }

  /** Texture Coordinates **/
  if (ctx->Eval.Map1TextureCoord4) {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Texture4;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, texcoord, uu, 4, map->Order);
  }
  else if (ctx->Eval.Map1TextureCoord3) {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Texture3;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, texcoord, uu, 3, map->Order);
     texcoord[3] = 1.0;
  }
  else if (ctx->Eval.Map1TextureCoord2) {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Texture2;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, texcoord, uu, 2, map->Order);
     texcoord[2] = 0.0;
     texcoord[3] = 1.0;
  }
  else if (ctx->Eval.Map1TextureCoord1) {
     struct gl_1d_map *map = &ctx->EvalMap.Map1Texture1;
     uu = (u - map->u1) / (map->u2 - map->u1);
     horner_bezier_curve(map->Points, texcoord, uu, 1, map->Order);
     texcoord[1] = 0.0;
     texcoord[2] = 0.0;
     texcoord[3] = 1.0;
  }
  else {
     texcoord[0] = ctx->Current.TexCoord[0];
     texcoord[1] = ctx->Current.TexCoord[1];
     texcoord[2] = ctx->Current.TexCoord[2];
     texcoord[3] = ctx->Current.TexCoord[3];
  }
  
  gl_eval_vertex( ctx, vertex, normal, colorptr, index, texcoord );
}


void gl_EvalCoord2f( GLcontext* ctx, GLfloat u, GLfloat v )
{
   GLfloat vertex[4];
   GLfloat normal[3];
   GLfloat fcolor[4];
   GLubyte icolor[4];
   GLubyte *colorptr;
   GLfloat texcoord[4];
   GLuint index;
   register GLfloat uu, vv;

#define CROSS_PROD(n, u, v) \
  (n)[0] = (u)[1]*(v)[2] - (u)[2]*(v)[1]; \
  (n)[1] = (u)[2]*(v)[0] - (u)[0]*(v)[2]; \
  (n)[2] = (u)[0]*(v)[1] - (u)[1]*(v)[0]

   /** Vertex **/
   if(ctx->Eval.Map2Vertex4) {
      struct gl_2d_map *map = &ctx->EvalMap.Map2Vertex4;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);

      if (ctx->Eval.AutoNormal) {
         GLfloat du[4], dv[4];

         de_casteljau_surf(map->Points, vertex, du, dv, uu, vv, 4,
                           map->Uorder, map->Vorder);

         CROSS_PROD(normal, du, dv);
         NORMALIZE_3FV(normal);
      }
      else {
         horner_bezier_surf(map->Points, vertex, uu, vv, 4,
                            map->Uorder, map->Vorder);
      }
   }
   else if (ctx->Eval.Map2Vertex3) {
      struct gl_2d_map *map = &ctx->EvalMap.Map2Vertex3;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);
      if (ctx->Eval.AutoNormal) {
         GLfloat du[3], dv[3];
         de_casteljau_surf(map->Points, vertex, du, dv, uu, vv, 3,
                           map->Uorder, map->Vorder);
         CROSS_PROD(normal, du, dv);
         NORMALIZE_3FV(normal);
      }
      else {
         horner_bezier_surf(map->Points, vertex, uu, vv, 3,
                            map->Uorder, map->Vorder);
      }
      vertex[3] = 1.0;
   }
#undef CROSS_PROD
   
   /** Color Index **/
   if (ctx->Eval.Map2Index) {
      GLfloat findex;
      struct gl_2d_map *map = &ctx->EvalMap.Map2Index;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);
      horner_bezier_surf(map->Points, &findex, uu, vv, 1,
                         map->Uorder, map->Vorder);
      index = (GLuint) (GLint) findex;
   }
   else {
      index = ctx->Current.Index;
   }

   /** Color **/
   if (ctx->Eval.Map2Color4) {
      struct gl_2d_map *map = &ctx->EvalMap.Map2Color4;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);
      horner_bezier_surf(map->Points, fcolor, uu, vv, 4,
                         map->Uorder, map->Vorder);
      icolor[0] = (GLint) (fcolor[0] * ctx->Visual->RedScale);
      icolor[1] = (GLint) (fcolor[1] * ctx->Visual->GreenScale);
      icolor[2] = (GLint) (fcolor[2] * ctx->Visual->BlueScale);
      icolor[3] = (GLint) (fcolor[3] * ctx->Visual->AlphaScale);
      colorptr = icolor;
   }
   else {
     GLubyte col[4];
     COPY_4V(col, ctx->Current.ByteColor );
     colorptr = col;
   }

   /** Normal **/
   if (!ctx->Eval.AutoNormal
       || (!ctx->Eval.Map2Vertex3 && !ctx->Eval.Map2Vertex4)) {
      if (ctx->Eval.Map2Normal) {
         struct gl_2d_map *map = &ctx->EvalMap.Map2Normal;
         uu = (u - map->u1) / (map->u2 - map->u1);
         vv = (v - map->v1) / (map->v2 - map->v1);
         horner_bezier_surf(map->Points, normal, uu, vv, 3,
                            map->Uorder, map->Vorder);
      }
      else {
         normal[0] = ctx->Current.Normal[0];
         normal[1] = ctx->Current.Normal[1];
         normal[2] = ctx->Current.Normal[2];
      }
   }

   /** Texture Coordinates **/
   if (ctx->Eval.Map2TextureCoord4) {
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture4;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);
      horner_bezier_surf(map->Points, texcoord, uu, vv, 4,
                         map->Uorder, map->Vorder);
   }
   else if (ctx->Eval.Map2TextureCoord3) {
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture3;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);
      horner_bezier_surf(map->Points, texcoord, uu, vv, 3,
                         map->Uorder, map->Vorder);
     texcoord[3] = 1.0;
   }
   else if (ctx->Eval.Map2TextureCoord2) {
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture2;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);
      horner_bezier_surf(map->Points, texcoord, uu, vv, 2,
                         map->Uorder, map->Vorder);
     texcoord[2] = 0.0;
     texcoord[3] = 1.0;
   }
   else if (ctx->Eval.Map2TextureCoord1) {
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture1;
      uu = (u - map->u1) / (map->u2 - map->u1);
      vv = (v - map->v1) / (map->v2 - map->v1);
      horner_bezier_surf(map->Points, texcoord, uu, vv, 1,
                         map->Uorder, map->Vorder);
     texcoord[1] = 0.0;
     texcoord[2] = 0.0;
     texcoord[3] = 1.0;
   }
   else 
   {
     texcoord[0] = ctx->Current.TexCoord[0];
     texcoord[1] = ctx->Current.TexCoord[1];
     texcoord[2] = ctx->Current.TexCoord[2];
     texcoord[3] = ctx->Current.TexCoord[3];
   }

   gl_eval_vertex( ctx, vertex, normal, colorptr, index, texcoord );
}


void gl_MapGrid1f( GLcontext* ctx, GLint un, GLfloat u1, GLfloat u2 )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glMapGrid1f" );
      return;
   }
   if (un<1) {
      gl_error( ctx, GL_INVALID_VALUE, "glMapGrid1f" );
      return;
   }
   ctx->Eval.MapGrid1un = un;
   ctx->Eval.MapGrid1u1 = u1;
   ctx->Eval.MapGrid1u2 = u2;
}


void gl_MapGrid2f( GLcontext* ctx, GLint un, GLfloat u1, GLfloat u2,
		  GLint vn, GLfloat v1, GLfloat v2 )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glMapGrid2f" );
      return;
   }
   if (un<1) {
      gl_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(un)" );
      return;
   }
   if (vn<1) {
      gl_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(vn)" );
      return;
   }
   ctx->Eval.MapGrid2un = un;
   ctx->Eval.MapGrid2u1 = u1;
   ctx->Eval.MapGrid2u2 = u2;
   ctx->Eval.MapGrid2vn = vn;
   ctx->Eval.MapGrid2v1 = v1;
   ctx->Eval.MapGrid2v2 = v2;
}


void gl_EvalPoint1( GLcontext* ctx, GLint i )
{
	GLfloat u, du;

	if (i==0) {
		u = ctx->Eval.MapGrid1u1;
	}
	else if (i==ctx->Eval.MapGrid1un) {
		u = ctx->Eval.MapGrid1u2;
	}
	else {
		du = (ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1)
			/ (GLfloat) ctx->Eval.MapGrid1un;
		u = i * du + ctx->Eval.MapGrid1u1;
	}
	gl_EvalCoord1f( ctx, u );
}



void gl_EvalPoint2( GLcontext* ctx, GLint i, GLint j )
{
	GLfloat u, du;
	GLfloat v, dv;

	if (i==0) {
		u = ctx->Eval.MapGrid2u1;
	}
	else if (i==ctx->Eval.MapGrid2un) {
		u = ctx->Eval.MapGrid2u2;
	}
	else {
		du = (ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1)
			/ (GLfloat) ctx->Eval.MapGrid2un;
		u = i * du + ctx->Eval.MapGrid2u1;
	}

	if (j==0) {
		v = ctx->Eval.MapGrid2v1;
	}
	else if (j==ctx->Eval.MapGrid2vn) {
		v = ctx->Eval.MapGrid2v2;
	}
	else {
		dv = (ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1)
			/ (GLfloat) ctx->Eval.MapGrid2vn;
		v = j * dv + ctx->Eval.MapGrid2v1;
	}

	gl_EvalCoord2f( ctx, u, v );
}



void gl_EvalMesh1( GLcontext* ctx, GLenum mode, GLint i1, GLint i2 )
{
   GLint i;
   GLfloat u, du;
   GLenum prim;

	if (INSIDE_BEGIN_END(ctx)) {
		gl_error( ctx, GL_INVALID_OPERATION, "glEvalMesh1" );
		return;
	}

	switch (mode) {
	case GL_POINT:
		prim = GL_POINTS;
		break;
	case GL_LINE:
		prim = GL_LINE_STRIP;
		break;
	default:
		gl_error( ctx, GL_INVALID_ENUM, "glEvalMesh1(mode)" );
		return;
	}

	du = (ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1)
		/ (GLfloat) ctx->Eval.MapGrid1un;

	gl_Begin( ctx, prim );
	for (i=i1;i<=i2;i++) {
		if (i==0) {
			u = ctx->Eval.MapGrid1u1;
		}
		else if (i==ctx->Eval.MapGrid1un) {
			u = ctx->Eval.MapGrid1u2;
		}
		else {
			u = i * du + ctx->Eval.MapGrid1u1;
		}
		gl_EvalCoord1f( ctx, u );
	}
	gl_End(ctx);
}



void gl_EvalMesh2( GLcontext* ctx, GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
	GLint i, j;
	GLfloat u, du, v, dv, v1, v2;

	if (INSIDE_BEGIN_END(ctx)) {
		gl_error( ctx, GL_INVALID_OPERATION, "glEvalMesh2" );
		return;
	}

	du = (ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1)
		/ (GLfloat) ctx->Eval.MapGrid2un;
	dv = (ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1)
		/ (GLfloat) ctx->Eval.MapGrid2vn;

#define I_TO_U( I, U )				\
	if ((I)==0) {		       	\
		U = ctx->Eval.MapGrid2u1;		\
	}					\
	else if ((I)==ctx->Eval.MapGrid2un) {	\
		U = ctx->Eval.MapGrid2u2;		\
	}					\
	else {				\
		U = (I) * du + ctx->Eval.MapGrid2u1;\
	}

#define J_TO_V( J, V )				\
	if ((J)==0) {			\
		V = ctx->Eval.MapGrid2v1;		\
	}					\
	else if ((J)==ctx->Eval.MapGrid2vn) {	\
		V = ctx->Eval.MapGrid2v2;		\
	}					\
	else {				\
		V = (J) * dv + ctx->Eval.MapGrid2v1;\
	}

	switch (mode) {
	case GL_POINT:
		gl_Begin( ctx, GL_POINTS );
		for (j=j1;j<=j2;j++) {
			J_TO_V( j, v );
			for (i=i1;i<=i2;i++) {
				I_TO_U( i, u );
				gl_EvalCoord2f( ctx, u, v );
			}
		}
		gl_End(ctx);
		break;
	case GL_LINE:
		for (j=j1;j<=j2;j++) {
			J_TO_V( j, v );
			gl_Begin( ctx, GL_LINE_STRIP );
			for (i=i1;i<=i2;i++) {
				I_TO_U( i, u );
				gl_EvalCoord2f( ctx, u, v );
			}
			gl_End(ctx);
		}
		for (i=i1;i<=i2;i++) {
			I_TO_U( i, u );
			gl_Begin( ctx, GL_LINE_STRIP );
			for (j=j1;j<=j2;j++) {
				J_TO_V( j, v );
				gl_EvalCoord2f( ctx, u, v );
			}
			gl_End(ctx);
		}
		break;
	case GL_FILL:
		for (j=j1;j<j2;j++) {
			/* NOTE: a quad strip can't be used because the four */
			/* can't be guaranteed to be coplanar! */
			gl_Begin( ctx, GL_TRIANGLE_STRIP );
			J_TO_V( j, v1 );
			J_TO_V( j+1, v2 );
			for (i=i1;i<=i2;i++) {
				I_TO_U( i, u );
				gl_EvalCoord2f( ctx, u, v1 );
				gl_EvalCoord2f( ctx, u, v2 );
			}
			gl_End(ctx);
		}
		break;
	default:
		gl_error( ctx, GL_INVALID_ENUM, "glEvalMesh2(mode)" );
		return;
	}

#undef I_TO_U
#undef J_TO_V
}


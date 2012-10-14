/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
*/

/*
 * glsurfeval.c++
 *
 */

/* Polynomial Evaluator Interface */
#include "gluos.h"
#include <stdio.h>
#include "glimports.h"
#include "glrenderer.h"
#include "glsurfeval.h"
#include "nurbsconsts.h"
#include "bezierPatchMesh.h"


//extern int surfcount;
//int surfcount=0;

/*#define USE_INTERNAL_EVAL*/ //use internal evaluator

/*whether do evaluation or not*/
/*#define NO_EVALUATION*/

//#define USE_LOD //for LOD test, have to turn on USE_LOD in insurfeval.c++ too

/*for statistics*/
//#define STATISTICS
#ifdef STATISTICS
static int STAT_num_of_triangles=0;
static int STAT_num_of_eval_vertices=0;
static int STAT_num_of_quad_strips=0;
#endif

/*for output triangles*/
/*#define OUTPUT_TRIANGLES*/


/*#define FOR_CHRIS*/
#ifdef FOR_CHRIS
extern "C"  {  void                evalUStripExt(int n_upper, REAL v_upper, REAL* upper_val,
				   int n_lower, REAL v_lower, REAL* lower_val);}

extern "C" {   void                evalVStripExt(int n_left, REAL u_left, REAL* left_val,
				   int n_right, REAL u_right, REAL* right_val);
	     }
#endif


/**************begin for LOD_eval_list***********/
void OpenGLSurfaceEvaluator::LOD_eval_list(int level)
{
  if(level == 0)
    LOD_eval_level = 1;
  else if(level == 1)
    LOD_eval_level = 2;
  else if(level == 2)
    LOD_eval_level = 4;
  else
    LOD_eval_level = 8;

  inBPMListEvalEM(global_bpm);
}


OpenGLSurfaceEvaluator::OpenGLSurfaceEvaluator()
{
    int i;

    for (i=0; i<VERTEX_CACHE_SIZE; i++) {
	vertexCache[i] = new StoredVertex;
    }
    tmeshing = 0;
    which = 0;
    vcount = 0;

    global_uorder = 0;
    global_vorder = 0;
    global_uprime = -1.0;
    global_vprime = -1.0;
    global_vprime_BV = -1.0;
    global_uprime_BU = -1.0;
    global_uorder_BU = 0;
    global_vorder_BU = 0;
    global_uorder_BV = 0;
    global_vorder_BV = 0;
    global_baseData = NULL;
        
    global_bpm = NULL;
    output_triangles = 0; //don't output triangles by default

    //no default callback functions
    beginCallBackN = NULL;
    endCallBackN = NULL;
    vertexCallBackN = NULL;
    normalCallBackN = NULL;
    colorCallBackN = NULL;
    texcoordCallBackN = NULL;
    beginCallBackData = NULL;
    endCallBackData = NULL;
    vertexCallBackData = NULL;
    normalCallBackData = NULL;
    colorCallBackData = NULL;
    texcoordCallBackData = NULL;

    userData = NULL;

    auto_normal_flag = 0;
    callback_auto_normal = 0; //default of GLU_CALLBACK_AUTO_NORMAL is 0
    vertex_flag = 0;
    normal_flag = 0;
    color_flag = 0;
    texcoord_flag = 0;

    em_vertex.uprime = -1.0;
    em_vertex.vprime = -1.0;
    em_normal.uprime = -1.0;
    em_normal.vprime = -1.0;
    em_color.uprime = -1.0;
    em_color.vprime = -1.0;
    em_texcoord.uprime = -1.0;
    em_texcoord.vprime = -1.0;

#ifdef USE_LOD
    LOD_eval_level = 1;
#endif
}

OpenGLSurfaceEvaluator::~OpenGLSurfaceEvaluator()
{
   for (int ii= 0; ii< VERTEX_CACHE_SIZE; ii++) {
      delete vertexCache[ii];
      vertexCache[ii]= 0;
   }
}

/*---------------------------------------------------------------------------
 * disable - turn off a map
 *---------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::disable(long type)
{
    glDisable((GLenum) type);
}

/*---------------------------------------------------------------------------
 * enable - turn on a map
 *---------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::enable(long type)
{
    glEnable((GLenum) type);
}

/*-------------------------------------------------------------------------
 * mapgrid2f - define a lattice of points with origin and offset
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::mapgrid2f(long nu, REAL u0, REAL u1, long nv, REAL v0, REAL v1)
{
#ifdef USE_INTERNAL_EVAL
  inMapGrid2f((int) nu, (REAL) u0, (REAL) u1, (int) nv,
	      (REAL) v0, (REAL) v1);
#else

  if(output_triangles)  
    {
      global_grid_u0 = u0;
      global_grid_u1 = u1;
      global_grid_nu = nu;
      global_grid_v0 = v0;
      global_grid_v1 = v1;
      global_grid_nv = nv;
    }
  else
    glMapGrid2d((GLint) nu, (GLdouble) u0, (GLdouble) u1, (GLint) nv,
	    (GLdouble) v0, (GLdouble) v1);

#endif
}

void
OpenGLSurfaceEvaluator::polymode(long style)
{
  if(! output_triangles)
    {
      switch(style) {
      default:
      case N_MESHFILL:
        
	glPolygonMode((GLenum) GL_FRONT_AND_BACK, (GLenum) GL_FILL);
	break;
      case N_MESHLINE:
	glPolygonMode((GLenum) GL_FRONT_AND_BACK, (GLenum) GL_LINE);
	break;
      case N_MESHPOINT:
	glPolygonMode((GLenum) GL_FRONT_AND_BACK, (GLenum) GL_POINT);
	break;
      }
    }
}

void
OpenGLSurfaceEvaluator::bgnline(void)
{
  if(output_triangles)
    bezierPatchMeshBeginStrip(global_bpm, GL_LINE_STRIP);
  else
    glBegin((GLenum) GL_LINE_STRIP);
}

void
OpenGLSurfaceEvaluator::endline(void)
{
  if(output_triangles)
    bezierPatchMeshEndStrip(global_bpm);
  else
    glEnd();
}

void
OpenGLSurfaceEvaluator::range2f(long type, REAL *from, REAL *to)
{
}

void
OpenGLSurfaceEvaluator::domain2f(REAL ulo, REAL uhi, REAL vlo, REAL vhi)
{
}

void
OpenGLSurfaceEvaluator::bgnclosedline(void)
{
  if(output_triangles)
    bezierPatchMeshBeginStrip(global_bpm, GL_LINE_LOOP);
  else
    glBegin((GLenum) GL_LINE_LOOP);
}

void
OpenGLSurfaceEvaluator::endclosedline(void)
{
  if(output_triangles)
    bezierPatchMeshEndStrip(global_bpm);
  else
    glEnd();
}





void
OpenGLSurfaceEvaluator::bgntmesh(void)
{

    tmeshing = 1;
    which = 0;
    vcount = 0;

    if(output_triangles)
      bezierPatchMeshBeginStrip(global_bpm, GL_TRIANGLES);
    else
      glBegin((GLenum) GL_TRIANGLES);

}

void
OpenGLSurfaceEvaluator::swaptmesh(void)
{
    which = 1 - which;

}

void
OpenGLSurfaceEvaluator::endtmesh(void)
{
    tmeshing = 0;


    if(output_triangles)
      bezierPatchMeshEndStrip(global_bpm);
    else
      glEnd();
}

void
OpenGLSurfaceEvaluator::bgntfan(void)
{

  if(output_triangles)
    bezierPatchMeshBeginStrip(global_bpm, GL_TRIANGLE_FAN);
  else
    glBegin((GLenum) GL_TRIANGLE_FAN);

}
void
OpenGLSurfaceEvaluator::endtfan(void)
{
  if(output_triangles)
	bezierPatchMeshEndStrip(global_bpm);
  else
    glEnd();
}

void
OpenGLSurfaceEvaluator::evalUStrip(int n_upper, REAL v_upper, REAL* upper_val, int n_lower, REAL v_lower, REAL* lower_val)
{
#ifdef USE_INTERNAL_EVAL
  inEvalUStrip(n_upper, v_upper, upper_val,
	n_lower, v_lower, lower_val);
#else

#ifdef FOR_CHRIS
  evalUStripExt(n_upper, v_upper, upper_val,
		 n_lower, v_lower, lower_val);
  return;

#endif
  int i,j,k,l;
  REAL leftMostV[2];

  /*
   *the algorithm works by scanning from left to right.
   *leftMostV: the left most of the remaining verteces (on both upper and lower).
   *	       it could an element of upperVerts or lowerVerts.
   *i: upperVerts[i] is the first vertex to the right of leftMostV on upper line
   *j: lowerVerts[j] is the first vertex to the right of leftMostV on lower line
   */

  /*initialize i,j,and leftMostV
   */
  if(upper_val[0] <= lower_val[0])
    {
      i=1;
      j=0;

      leftMostV[0] = upper_val[0];
      leftMostV[1] = v_upper;
    }
  else
    {
      i=0;
      j=1;

      leftMostV[0] = lower_val[0];
      leftMostV[1] = v_lower;

    }

  /*the main loop.
   *the invariance is that:
   *at the beginning of each loop, the meaning of i,j,and leftMostV are
   *maintained
   */
  while(1)
    {
      if(i >= n_upper) /*case1: no more in upper*/
	{
	  if(j<n_lower-1) /*at least two vertices in lower*/
	    {
	      bgntfan();
	      coord2f(leftMostV[0], leftMostV[1]);
//	      glNormal3fv(leftMostNormal);
//		glVertex3fv(leftMostXYZ);

	      while(j<n_lower){
		coord2f(lower_val[j], v_lower);
//		glNormal3fv(lowerNormal[j]);
//		glVertex3fv(lowerXYZ[j]);
		j++;

	      }
	      endtfan();
	    }
	  break; /*exit the main loop*/
	}
      else if(j>= n_lower) /*case2: no more in lower*/
	{
	  if(i<n_upper-1) /*at least two vertices in upper*/
	    {
	      bgntfan();
	      coord2f(leftMostV[0], leftMostV[1]);
//	      glNormal3fv(leftMostNormal);
//	      glVertex3fv(leftMostXYZ);
        
	      for(k=n_upper-1; k>=i; k--) /*reverse order for two-side lighting*/
		{
		  coord2f(upper_val[k], v_upper);
//		  glNormal3fv(upperNormal[k]);
//		  glVertex3fv(upperXYZ[k]);
		}

	      endtfan();
	    }
	  break; /*exit the main loop*/
	}
      else /* case3: neither is empty, plus the leftMostV, there is at least one triangle to output*/
	{
	  if(upper_val[i] <= lower_val[j])
	    {
	      bgntfan();
	      coord2f(lower_val[j], v_lower);
//	      glNormal3fv(lowerNormal[j]);
//	      glVertex3fv(lowerXYZ[j]);

	      /*find the last k>=i such that
	       *upperverts[k][0] <= lowerverts[j][0]
	       */
	      k=i;

	      while(k<n_upper)
		{
		  if(upper_val[k] > lower_val[j])
		    break;
		  k++;

		}
	      k--;


	      for(l=k; l>=i; l--)/*the reverse is for two-side lighting*/
		{
		  coord2f(upper_val[l], v_upper);
//		  glNormal3fv(upperNormal[l]);
//		  glVertex3fv(upperXYZ[l]);

		}
	      coord2f(leftMostV[0], leftMostV[1]);
//	      glNormal3fv(leftMostNormal);
//	      glVertex3fv(leftMostXYZ);

	      endtfan();

	      /*update i and leftMostV for next loop
	       */
	      i = k+1;

	      leftMostV[0] = upper_val[k];
	      leftMostV[1] = v_upper;
//	      leftMostNormal = upperNormal[k];
//	      leftMostXYZ = upperXYZ[k];
	    }
	  else /*upperVerts[i][0] > lowerVerts[j][0]*/
	    {
	      bgntfan();
	      coord2f(upper_val[i], v_upper);
//	      glNormal3fv(upperNormal[i]);
//	      glVertex3fv(upperXYZ[i]);
        
	      coord2f(leftMostV[0], leftMostV[1]);
//		glNormal3fv(leftMostNormal);
//	      glVertex3fv(leftMostXYZ);
        

	      /*find the last k>=j such that
	       *lowerverts[k][0] < upperverts[i][0]
	       */
	      k=j;
	      while(k< n_lower)
		{
		  if(lower_val[k] >= upper_val[i])
		    break;
		  coord2f(lower_val[k], v_lower);
//		  glNormal3fv(lowerNormal[k]);
//		  glVertex3fv(lowerXYZ[k]);

		  k++;
		}
	      endtfan();

	      /*update j and leftMostV for next loop
	       */
	      j=k;
	      leftMostV[0] = lower_val[j-1];
	      leftMostV[1] = v_lower;

//	      leftMostNormal = lowerNormal[j-1];
//	      leftMostXYZ = lowerXYZ[j-1];
	    }
	}
    }
  //clean up
//  free(upperXYZ);
//  free(lowerXYZ);
//  free(upperNormal);
//  free(lowerNormal);
#endif

}


void
OpenGLSurfaceEvaluator::evalVStrip(int n_left, REAL u_left, REAL* left_val, int n_right, REAL u_right, REAL* right_val)
{
#ifdef USE_INTERNAL_EVAL
	inEvalVStrip(n_left, u_left, left_val,
	n_right, u_right, right_val);
#else

#ifdef FOR_CHRIS
	evalVStripExt(n_left, u_left, left_val,
		      n_right, u_right, right_val);
	return;

#endif

  int i,j,k,l;
  REAL botMostV[2];
  /*
   *the algorithm works by scanning from bot to top.
   *botMostV: the bot most of the remaining verteces (on both left and right).
   *	       it could an element of leftVerts or rightVerts.
   *i: leftVerts[i] is the first vertex to the top of botMostV on left line
   *j: rightVerts[j] is the first vertex to the top of botMostV on rightline
   */

  /*initialize i,j,and botMostV
   */
  if(left_val[0] <= right_val[0])
    {
      i=1;
      j=0;

      botMostV[0] = u_left;
      botMostV[1] = left_val[0];
    }
  else
    {
      i=0;
      j=1;

      botMostV[0] = u_right;
      botMostV[1] = right_val[0];
    }

  /*the main loop.
   *the invariance is that:
   *at the beginning of each loop, the meaning of i,j,and botMostV are
   *maintained
   */
  while(1)
    {
      if(i >= n_left) /*case1: no more in left*/
	{
	  if(j<n_right-1) /*at least two vertices in right*/
	    {
	      bgntfan();
	      coord2f(botMostV[0], botMostV[1]);
	      while(j<n_right){
		coord2f(u_right, right_val[j]);
//		glNormal3fv(rightNormal[j]);
//		glVertex3fv(rightXYZ[j]);
		j++;

	      }
	      endtfan();
	    }
	  break; /*exit the main loop*/
	}
      else if(j>= n_right) /*case2: no more in right*/
	{
	  if(i<n_left-1) /*at least two vertices in left*/
	    {
	      bgntfan();
	      coord2f(botMostV[0], botMostV[1]);
//	      glNormal3fv(botMostNormal);
//	      glVertex3fv(botMostXYZ);
        
	      for(k=n_left-1; k>=i; k--) /*reverse order for two-side lighting*/
		{
		  coord2f(u_left, left_val[k]);
//		  glNormal3fv(leftNormal[k]);
//		  glVertex3fv(leftXYZ[k]);
		}

	      endtfan();
	    }
	  break; /*exit the main loop*/
	}
      else /* case3: neither is empty, plus the botMostV, there is at least one triangle to output*/
	{
	  if(left_val[i] <= right_val[j])
	    {
	      bgntfan();
	      coord2f(u_right, right_val[j]);
//	      glNormal3fv(rightNormal[j]);
//	      glVertex3fv(rightXYZ[j]);

	      /*find the last k>=i such that
	       *leftverts[k][0] <= rightverts[j][0]
	       */
	      k=i;

	      while(k<n_left)
		{
		  if(left_val[k] > right_val[j])
		    break;
		  k++;

		}
	      k--;


	      for(l=k; l>=i; l--)/*the reverse is for two-side lighting*/
		{
		  coord2f(u_left, left_val[l]);
//		  glNormal3fv(leftNormal[l]);
//		  glVertex3fv(leftXYZ[l]);

		}
	      coord2f(botMostV[0], botMostV[1]);
//	      glNormal3fv(botMostNormal);
//	      glVertex3fv(botMostXYZ);

	      endtfan();

	      /*update i and botMostV for next loop
	       */
	      i = k+1;

	      botMostV[0] = u_left;
	      botMostV[1] = left_val[k];
//	      botMostNormal = leftNormal[k];
//	      botMostXYZ = leftXYZ[k];
	    }
	  else /*left_val[i] > right_val[j])*/
	    {
	      bgntfan();
	      coord2f(u_left, left_val[i]);
//	      glNormal3fv(leftNormal[i]);
//	      glVertex3fv(leftXYZ[i]);
        
	      coord2f(botMostV[0], botMostV[1]);
//	      glNormal3fv(botMostNormal);
//	      glVertex3fv(botMostXYZ);
        

	      /*find the last k>=j such that
	       *rightverts[k][0] < leftverts[i][0]
	       */
	      k=j;
	      while(k< n_right)
		{
		  if(right_val[k] >= left_val[i])
		    break;
		  coord2f(u_right, right_val[k]);
//		  glNormal3fv(rightNormal[k]);
//		  glVertex3fv(rightXYZ[k]);

		  k++;
		}
	      endtfan();

	      /*update j and botMostV for next loop
	       */
	      j=k;
	      botMostV[0] = u_right;
	      botMostV[1] = right_val[j-1];

//	      botMostNormal = rightNormal[j-1];
//	      botMostXYZ = rightXYZ[j-1];
	    }
	}
    }
  //clean up
//  free(leftXYZ);
//  free(leftNormal);
//  free(rightXYZ);
//  free(rightNormal);
#endif
}


void
OpenGLSurfaceEvaluator::bgnqstrip(void)
{
  if(output_triangles)
    bezierPatchMeshBeginStrip(global_bpm, GL_QUAD_STRIP);
  else
    glBegin((GLenum) GL_QUAD_STRIP);

#ifdef STATISTICS
	STAT_num_of_quad_strips++;
#endif
}

void
OpenGLSurfaceEvaluator::endqstrip(void)
{
  if(output_triangles)
    bezierPatchMeshEndStrip(global_bpm);
  else
    glEnd();

}

/*-------------------------------------------------------------------------
 * bgnmap2f - preamble to surface definition and evaluations
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::bgnmap2f(long)
{
  if(output_triangles)
    {
      /*deallocate the space which may has been
       *allocated by global_bpm previously
       */
      if(global_bpm != NULL) {
	bezierPatchMeshListDelete(global_bpm);
	global_bpm = NULL;
      }


      /*
	auto_normal_flag = 1; //always output normal in callback mode.
			      //we could have used the following code,
			      //but Inspector doesn't have gl context
			      //before it calls tessellator.
			      //this way is temporary.
	*/
      //NEWCALLBACK
      //if one of the two normal callback functions are set,
      //then set
      if(normalCallBackN != NULL ||
	 normalCallBackData != NULL)
	auto_normal_flag = 1;
      else
	auto_normal_flag = 0;

      //initialize so that no maps initially
      vertex_flag = 0;
      normal_flag = 0;
      color_flag = 0;
      texcoord_flag = 0;

      /*
      if(glIsEnabled(GL_AUTO_NORMAL) == GL_TRUE)
	auto_normal_flag = 1;
      else if (callback_auto_normal == 1)
	auto_normal_flag = 1;
      else
	auto_normal_flag = 0;
	*/
	  glPushAttrib((GLbitfield) GL_EVAL_BIT);

    }
  else
    {
      glPushAttrib((GLbitfield) GL_EVAL_BIT);

      /*to avoid side effect, we restor the opengl state for GL_POLYGON_MODE
       */       
      glGetIntegerv(GL_POLYGON_MODE, gl_polygon_mode);
    }

}

/*-------------------------------------------------------------------------
 * endmap2f - postamble to a map
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::endmap2f(void)
{

  if(output_triangles)
    {
      //bezierPatchMeshListDelDeg(global_bpm);

      //    bezierPatchMeshListEval(global_bpm);

      //surfcount++;
      //printf("surfcount=%i\n", surfcount);
      //if(surfcount == 8) exit(0);

      inBPMListEvalEM(global_bpm);



/*
    global_bpm = bezierPatchMeshListReverse(global_bpm);
    {
      float *vertex_array;
      float *normal_array;
      int *length_array;
      int *type_array;
      int num_strips;
      bezierPatchMeshListCollect(global_bpm, &vertex_array, &normal_array, &length_array, &type_array, &num_strips);
      drawStrips(vertex_array, normal_array, length_array, type_array, num_strips);
      free(vertex_array);
      free(normal_array);
      free(length_array);
      free(type_array);
    }
*/

    //bezierPatchMeshListPrint(global_bpm);
    //bezierPatchMeshListDraw(global_bpm);

//	  printf("num triangles=%i\n", bezierPatchMeshListNumTriangles(global_bpm));

#ifdef USE_LOD
#else
    bezierPatchMeshListDelete(global_bpm);
    global_bpm = NULL;
#endif
	glPopAttrib();
  }
else
  {
#ifndef USE_LOD
    glPopAttrib();
#endif

#ifdef STATISTICS
    fprintf(stderr, "num_vertices=%i,num_triangles=%i,num_quads_strips=%i\n", STAT_num_of_eval_vertices,STAT_num_of_triangles,STAT_num_of_quad_strips);
#endif

    /*to restore the gl_polygon_mode
     */
#ifndef USE_LOD
    glPolygonMode( GL_FRONT, (GLenum) gl_polygon_mode[0]);
    glPolygonMode( GL_BACK,  (GLenum) gl_polygon_mode[1]);
#endif
}

}

/*-------------------------------------------------------------------------
 * map2f - pass a desription of a surface map
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::map2f(
    long _type,
    REAL _ulower,	/* u lower domain coord 	*/
    REAL _uupper,	/* u upper domain coord 	*/
    long _ustride,	/* interpoint distance		*/
    long _uorder,	/* parametric order		*/
    REAL _vlower,	/* v lower domain coord 	*/
    REAL _vupper,	/* v upper domain coord 	*/
    long _vstride,	/* interpoint distance		*/
    long _vorder,	/* parametric order		*/
    REAL *pts)	/* control points		*/
{
#ifdef USE_INTERNAL_EVAL
   inMap2f((int) _type, (REAL) _ulower, (REAL) _uupper,
	    (int) _ustride, (int) _uorder, (REAL) _vlower,
	    (REAL) _vupper, (int) _vstride, (int) _vorder,
	    (REAL *) pts);
#else



   if(output_triangles)
     {
       if(global_bpm == NULL)
	 global_bpm = bezierPatchMeshMake2(10,10);
       if(
	  (global_bpm->bpatch == NULL &&
	  (_type == GL_MAP2_VERTEX_3 || _type == GL_MAP2_VERTEX_4))
	  ||
	  (global_bpm->bpatch_normal == NULL &&
	   (_type == GL_MAP2_NORMAL))
	  ||
	  (global_bpm->bpatch_color == NULL &&
	   (_type == GL_MAP2_INDEX || _type == GL_MAP2_COLOR_4))
	  ||
	  (global_bpm->bpatch_texcoord == NULL &&
	   (_type == GL_MAP2_TEXTURE_COORD_1 ||
	    _type == GL_MAP2_TEXTURE_COORD_2 ||
	    _type == GL_MAP2_TEXTURE_COORD_3 ||
	    _type == GL_MAP2_TEXTURE_COORD_4 )
	   ))
	 {
	   bezierPatchMeshPutPatch(global_bpm, (int) _type, _ulower, _uupper,(int)  _ustride,(int) _uorder,_vlower, _vupper, (int) _vstride, (int) _vorder, pts);
	 }
       else /*new surface patch (with multiple maps) starts*/
	 {
	   bezierPatchMesh *temp = bezierPatchMeshMake2(10,10);
	   bezierPatchMeshPutPatch(temp, (int) _type, _ulower, _uupper,(int)  _ustride,(int) _uorder,_vlower, _vupper, (int) _vstride, (int) _vorder, pts);
	   global_bpm = bezierPatchMeshListInsert(global_bpm, temp);

	   /*
	   global_bpm = bezierPatchMeshListInsert(global_bpm,
						  bezierPatchMeshMake(
								      (int) _type, _ulower, _uupper,(int)  _ustride, (int) _uorder, _vlower, _vupper, (int) _vstride, (int) _vorder, pts, 10, 10));
	   */
	 }
     }
   else /*not output triangles*/
     {
       glMap2f((GLenum) _type, (GLfloat) _ulower, (GLfloat) _uupper,
	       (GLint) _ustride, (GLint) _uorder, (GLfloat) _vlower,
	       (GLfloat) _vupper, (GLint) _vstride, (GLint) _vorder,
	       (const GLfloat *) pts);
     }

#endif
}


/*-------------------------------------------------------------------------
 * mapmesh2f - evaluate a mesh of points on lattice
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::mapmesh2f(long style, long umin, long umax, long vmin, long vmax)
{
#ifdef NO_EVALUATION
return;
#endif

#ifdef USE_INTERNAL_EVAL
    inEvalMesh2((int)umin, (int)vmin, (int)umax, (int)vmax);
#else



if(output_triangles)
{
#ifdef USE_LOD
  bezierPatchMeshBeginStrip(global_bpm, GL_POLYGON);
  bezierPatchMeshInsertUV(global_bpm, global_grid_u0, global_grid_v0);
  bezierPatchMeshInsertUV(global_bpm, global_grid_u1, global_grid_v1);
  bezierPatchMeshInsertUV(global_bpm, (REAL)global_grid_nu, (REAL)global_grid_nv);
  bezierPatchMeshInsertUV(global_bpm, (REAL)umin, (REAL)vmin);
  bezierPatchMeshInsertUV(global_bpm, (REAL)umax, (REAL)vmax);
  bezierPatchMeshEndStrip(global_bpm);

#else

  REAL du, dv;
  long i,j;
  if(global_grid_nu == 0 || global_grid_nv == 0)
    return; /*no points need to be output*/
  du = (global_grid_u1 - global_grid_u0) / (REAL)global_grid_nu;
  dv = (global_grid_v1 - global_grid_v0) / (REAL)global_grid_nv;

  if(global_grid_nu >= global_grid_nv){

    for(i=umin; i<umax; i++){
      REAL u1 = (i==global_grid_nu)? global_grid_u1:(global_grid_u0 + i*du);
      REAL u2 = ((i+1) == global_grid_nu)? global_grid_u1: (global_grid_u0+(i+1)*du);

      bgnqstrip();
      for(j=vmax; j>=vmin; j--){
	REAL v1 = (j == global_grid_nv)? global_grid_v1: (global_grid_v0 +j*dv);
        
	coord2f(u1, v1);
	coord2f(u2, v1);
      }
      endqstrip();
    }
  }
  else{

    for(i=vmin; i<vmax; i++){
      REAL v1 = (i==global_grid_nv)? global_grid_v1:(global_grid_v0 + i*dv);
      REAL v2 = ((i+1) == global_grid_nv)? global_grid_v1: (global_grid_v0+(i+1)*dv);

      bgnqstrip();
      for(j=umax; j>=umin; j--){
	REAL u1 = (j == global_grid_nu)? global_grid_u1: (global_grid_u0 +j*du);        
	coord2f(u1, v2);
	coord2f(u1, v1);
      }
      endqstrip();
    }
  }
#endif
}
else
{
    switch(style) {
    default:
    case N_MESHFILL:
	glEvalMesh2((GLenum) GL_FILL, (GLint) umin, (GLint) umax,
		(GLint) vmin, (GLint) vmax);
	break;
    case N_MESHLINE:
	glEvalMesh2((GLenum) GL_LINE, (GLint) umin, (GLint) umax,
		(GLint) vmin, (GLint) vmax);
	break;
    case N_MESHPOINT:
	glEvalMesh2((GLenum) GL_POINT, (GLint) umin, (GLint) umax,
		(GLint) vmin, (GLint) vmax);
	break;
    }
  }

#endif

#ifdef STATISTICS
	STAT_num_of_quad_strips += (umax-umin)*(vmax-vmin);
#endif
}

/*-------------------------------------------------------------------------
 * evalcoord2f - evaluate a point on a surface
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::evalcoord2f(long, REAL u, REAL v)
{


#ifdef NO_EVALUATION
return;
#endif


    newtmeshvert(u, v);
}

/*-------------------------------------------------------------------------
 * evalpoint2i - evaluate a grid point
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::evalpoint2i(long u, long v)
{
#ifdef NO_EVALUATION
return;
#endif

    newtmeshvert(u, v);
}

void
OpenGLSurfaceEvaluator::point2i( long u, long v )
{
#ifdef NO_EVALUATION
return;
#else

#ifdef USE_INTERNAL_EVAL
    inEvalPoint2( (int)u,  (int)v);
#else


if(output_triangles)
{

  REAL du, dv;
  REAL fu,fv;
  du = (global_grid_u1 - global_grid_u0) / (REAL)global_grid_nu;
  dv = (global_grid_v1 - global_grid_v0) / (REAL)global_grid_nv;
  fu = (u==global_grid_nu)? global_grid_u1:(global_grid_u0 + u*du);
  fv = (v == global_grid_nv)? global_grid_v1: (global_grid_v0 +v*dv);
  coord2f(fu,fv);
}
else
    glEvalPoint2((GLint) u, (GLint) v);


#endif

#ifdef STATISTICS
  STAT_num_of_eval_vertices++;
#endif

#endif

}

void
OpenGLSurfaceEvaluator::coord2f( REAL u, REAL v )
{
#ifdef NO_EVALUATION
return;
#else

#ifdef USE_INTERNAL_EVAL
    inEvalCoord2f( u, v);
#else


if(output_triangles)
    bezierPatchMeshInsertUV(global_bpm, u,v);
else
    glEvalCoord2f((GLfloat) u, (GLfloat) v);


#endif


#ifdef STATISTICS
  STAT_num_of_eval_vertices++;
#endif

#endif
}

void
OpenGLSurfaceEvaluator::newtmeshvert( long u, long v )
{
#ifdef NO_EVALUATION
return;
#else

    if (tmeshing) {

	if (vcount == 2) {
	    vertexCache[0]->invoke(this);
	    vertexCache[1]->invoke(this);
	    point2i( u,  v);

	} else {
	    vcount++;
	}

	vertexCache[which]->saveEvalPoint(u, v);
	which = 1 - which;
    } else {
	point2i( u,  v);
    }
#endif
}

void
OpenGLSurfaceEvaluator::newtmeshvert( REAL u, REAL v )
{
#ifdef NO_EVALUATION
return;
#else
    if (tmeshing) {


	if (vcount == 2) {
	    vertexCache[0]->invoke(this);
	    vertexCache[1]->invoke(this);
	    coord2f(u,v);

	} else {
	    vcount++;
	}

	vertexCache[which]->saveEvalCoord(u, v);
	which = 1 - which;
    } else {

	coord2f( u,  v);
    }
#endif

}

#ifdef _WIN32
void OpenGLSurfaceEvaluator::putCallBack(GLenum which, void (GLAPIENTRY *fn)() )
#else
void OpenGLSurfaceEvaluator::putCallBack(GLenum which, _GLUfuncptr fn )
#endif
{
  switch(which)
    {
    case GLU_NURBS_BEGIN:
      beginCallBackN = (void (GLAPIENTRY *) (GLenum)) fn;
      break;
    case GLU_NURBS_END:
      endCallBackN = (void (GLAPIENTRY *) (void)) fn;
      break;
    case GLU_NURBS_VERTEX:
      vertexCallBackN = (void (GLAPIENTRY *) (const GLfloat*)) fn;
      break;
    case GLU_NURBS_NORMAL:
      normalCallBackN = (void (GLAPIENTRY *) (const GLfloat*)) fn;
      break;
    case GLU_NURBS_COLOR:
      colorCallBackN = (void (GLAPIENTRY *) (const GLfloat*)) fn;
      break;
    case GLU_NURBS_TEXTURE_COORD:
      texcoordCallBackN = (void (GLAPIENTRY *) (const GLfloat*)) fn;
      break;
    case GLU_NURBS_BEGIN_DATA:
      beginCallBackData = (void (GLAPIENTRY *) (GLenum, void*)) fn;
      break;
    case GLU_NURBS_END_DATA:
      endCallBackData = (void (GLAPIENTRY *) (void*)) fn;
      break;
    case GLU_NURBS_VERTEX_DATA:
      vertexCallBackData = (void (GLAPIENTRY *) (const GLfloat*, void*)) fn;
      break;
    case GLU_NURBS_NORMAL_DATA:
      normalCallBackData = (void (GLAPIENTRY *) (const GLfloat*, void*)) fn;
      break;
    case GLU_NURBS_COLOR_DATA:
      colorCallBackData = (void (GLAPIENTRY *) (const GLfloat*, void*)) fn;
      break;
    case GLU_NURBS_TEXTURE_COORD_DATA:
      texcoordCallBackData = (void (GLAPIENTRY *) (const GLfloat*, void*)) fn;
      break;

    }
}


void
OpenGLSurfaceEvaluator::beginCallBack(GLenum which, void *data)
{
  if(beginCallBackData)
    beginCallBackData(which, data);
  else if(beginCallBackN)
    beginCallBackN(which);
}

void
OpenGLSurfaceEvaluator::endCallBack(void *data)
{
  if(endCallBackData)
    endCallBackData(data);
  else if(endCallBackN)
    endCallBackN();
}

void
OpenGLSurfaceEvaluator::vertexCallBack(const GLfloat *vert, void* data)
{
  if(vertexCallBackData)
    vertexCallBackData(vert, data);
  else if(vertexCallBackN)
    vertexCallBackN(vert);
}


void
OpenGLSurfaceEvaluator::normalCallBack(const GLfloat *normal, void* data)
{
  if(normalCallBackData)
    normalCallBackData(normal, data);
  else if(normalCallBackN)
    normalCallBackN(normal);
}

void
OpenGLSurfaceEvaluator::colorCallBack(const GLfloat *color, void* data)
{
  if(colorCallBackData)
    colorCallBackData(color, data);
  else if(colorCallBackN)
    colorCallBackN(color);
}

void
OpenGLSurfaceEvaluator::texcoordCallBack(const GLfloat *texcoord, void* data)
{
  if(texcoordCallBackData)
    texcoordCallBackData(texcoord, data);
  else if(texcoordCallBackN)
    texcoordCallBackN(texcoord);
}





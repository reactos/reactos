/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/*
 * glsurfeval.h
 *
 */

#ifndef __gluglsurfeval_h_
#define __gluglsurfeval_h_

#include "basicsurfeval.h"
#include "bezierPatchMesh.h" //in case output triangles
#include <GL/gl.h>
#include <GL/glu.h>

class SurfaceMap;
class OpenGLSurfaceEvaluator;
class StoredVertex;

#define TYPECOORD	1
#define TYPEPOINT	2

/* Cache up to 3 vertices from tmeshes */
#define VERTEX_CACHE_SIZE	3

/*for internal evaluator callback stuff*/
#ifndef IN_MAX_BEZIER_ORDER
#define IN_MAX_BEZIER_ORDER 40 /*XXX should be bigger than machine order*/
#endif
			
#ifndef IN_MAX_DIMENSION
#define IN_MAX_DIMENSION 4 
#endif

typedef struct surfEvalMachine{
  REAL uprime;//cached previusly evaluated uprime.
  REAL vprime;
  int k; /*the dimension*/
  REAL u1;
  REAL u2;
  int ustride;
  int uorder;
  REAL v1;
  REAL v2;
  int vstride;
  int vorder;
  REAL ctlPoints[IN_MAX_BEZIER_ORDER*IN_MAX_BEZIER_ORDER*IN_MAX_DIMENSION];
  REAL ucoeff[IN_MAX_BEZIER_ORDER]; /*cache the polynomial values*/
  REAL vcoeff[IN_MAX_BEZIER_ORDER];
  REAL ucoeffDeriv[IN_MAX_BEZIER_ORDER]; /*cache the polynomial derivatives*/
  REAL vcoeffDeriv[IN_MAX_BEZIER_ORDER];
} surfEvalMachine;
  
  

class StoredVertex {
public:
    		StoredVertex() { type = 0; coord[0] = 0; coord[1] = 0; point[0] = 0; point[1] = 0; }
		~StoredVertex(void) {}
    void	saveEvalCoord(REAL x, REAL y) 
		    {coord[0] = x; coord[1] = y; type = TYPECOORD; }
    void	saveEvalPoint(long x, long y)
		    {point[0] = x; point[1] = y; type = TYPEPOINT; }
    void	invoke(OpenGLSurfaceEvaluator *eval);

private:
    int		type;
    REAL	coord[2];
    long	point[2];
};

class OpenGLSurfaceEvaluator : public BasicSurfaceEvaluator {
public:
			OpenGLSurfaceEvaluator();
    			virtual ~OpenGLSurfaceEvaluator( void );
    void		polymode( long style );
    void		range2f( long, REAL *, REAL * );
    void		domain2f( REAL, REAL, REAL, REAL );
    void		addMap( SurfaceMap * ) { }

    void		enable( long );
    void		disable( long );
    void		bgnmap2f( long );
    void		map2f( long, REAL, REAL, long, long, 
				     REAL, REAL, long, long, REAL * );
    void		mapgrid2f( long, REAL, REAL, long, REAL, REAL );
    void		mapmesh2f( long, long, long, long, long );
    void		evalcoord2f( long, REAL, REAL );
    void		evalpoint2i( long, long );
    void		endmap2f( void );

    void	 	bgnline( void );
    void	 	endline( void );
    void	 	bgnclosedline( void );
    void	 	endclosedline( void );
    void	 	bgntmesh( void );
    void	 	swaptmesh( void );
    void	 	endtmesh( void );
    void	 	bgnqstrip( void );
    void	 	endqstrip( void );

    void                bgntfan( void );
    void                endtfan( void );
    void                evalUStrip(int n_upper, REAL v_upper, REAL* upper_val,
                                   int n_lower, REAL v_lower, REAL* lower_val);
    void                evalVStrip(int n_left, REAL u_left, REAL* left_val,
                                   int n_right, REAL u_right, REAL* right_val);

    void		coord2f( REAL, REAL );
    void		point2i( long, long );

    void		newtmeshvert( REAL, REAL );
    void		newtmeshvert( long, long );

#ifdef _WIN32
    void 	        putCallBack(GLenum which, void (GLAPIENTRY *fn)() );
#else
    void 	        putCallBack(GLenum which, _GLUfuncptr fn );
#endif

    int                 get_vertices_call_back()
      {
	return output_triangles;
      }
    void                put_vertices_call_back(int flag)
      {
	output_triangles = flag;
      }

    void                 put_callback_auto_normal(int flag)
      {
        callback_auto_normal = flag;
      } 

   int                   get_callback_auto_normal()
     {
        return callback_auto_normal;
      }

   void                  set_callback_userData(void* data)
     {
       userData = data;
     }

    /**************begin for LOD_eval_list***********/
    void LOD_eval_list(int level);



   
private:
    StoredVertex	*vertexCache[VERTEX_CACHE_SIZE];
    int			tmeshing;
    int			which;
    int			vcount;

    GLint              gl_polygon_mode[2];/*to save and restore so that
					 *no side effect
					 */
    bezierPatchMesh        *global_bpm; //for output triangles
    int                output_triangles; //true 1 or false 0



    void (GLAPIENTRY *beginCallBackN) (GLenum type);
    void (GLAPIENTRY *endCallBackN)   (void);
    void (GLAPIENTRY *vertexCallBackN) (const GLfloat *vert);
    void (GLAPIENTRY *normalCallBackN) (const GLfloat *normal);
    void (GLAPIENTRY *colorCallBackN) (const GLfloat *color);
    void (GLAPIENTRY *texcoordCallBackN) (const GLfloat *texcoord);

    void (GLAPIENTRY *beginCallBackData) (GLenum type, void* data);
    void (GLAPIENTRY *endCallBackData)   (void* data);
    void (GLAPIENTRY *vertexCallBackData) (const GLfloat *vert, void* data);
    void (GLAPIENTRY *normalCallBackData) (const GLfloat *normal, void* data);
    void (GLAPIENTRY *colorCallBackData) (const GLfloat *color, void* data);
    void (GLAPIENTRY *texcoordCallBackData) (const GLfloat *texcoord, void* data);

    void               beginCallBack (GLenum type, void* data);
    void               endCallBack   (void* data);
    void               vertexCallBack (const GLfloat *vert, void* data);
    void               normalCallBack (const GLfloat *normal, void* data);
    void               colorCallBack (const GLfloat *color, void* data);
    void               texcoordCallBack (const GLfloat *texcoord, void* data);


    void* userData; //the opaque pointer for Data callback functions.

   /*LOD evaluation*/
   void LOD_triangle(REAL A[2], REAL B[2], REAL C[2],
		     int level);
   void LOD_eval(int num_vert, REAL* verts, int type, int level);
		     
  int LOD_eval_level; //set by LOD_eval_list()

   /*************begin for internal evaluators*****************/
			
 /*the following global variables are only defined in this file. 
 *They are used to cache the precomputed Bezier polynomial values.
 *These calues may be used consecutively in which case we don't have 
 *recompute these values again.
 */
 int global_uorder; /*store the uorder in the previous evaluation*/
 int global_vorder; /*store the vorder in the previous evaluation*/
 REAL global_uprime;
 REAL global_vprime;
 REAL global_vprime_BV;
 REAL global_uprime_BU;
 int global_uorder_BV; /*store the uorder in the previous evaluation*/
 int global_vorder_BV; /*store the vorder in the previous evaluation*/
 int global_uorder_BU; /*store the uorder in the previous evaluation*/
 int global_vorder_BU; /*store the vorder in the previous evaluation*/

 REAL global_ucoeff[IN_MAX_BEZIER_ORDER]; /*cache the polynomial values*/
 REAL global_vcoeff[IN_MAX_BEZIER_ORDER];
 REAL global_ucoeffDeriv[IN_MAX_BEZIER_ORDER]; /*cache the polynomial derivatives*/
 REAL global_vcoeffDeriv[IN_MAX_BEZIER_ORDER];

 REAL global_BV[IN_MAX_BEZIER_ORDER][IN_MAX_DIMENSION];
 REAL global_PBV[IN_MAX_BEZIER_ORDER][IN_MAX_DIMENSION];
 REAL global_BU[IN_MAX_BEZIER_ORDER][IN_MAX_DIMENSION];
 REAL global_PBU[IN_MAX_BEZIER_ORDER][IN_MAX_DIMENSION];
 REAL* global_baseData;

 int    global_ev_k; /*the dimension*/
 REAL global_ev_u1;
 REAL global_ev_u2;
 int    global_ev_ustride;
 int    global_ev_uorder;
 REAL global_ev_v1;
 REAL global_ev_v2;
 int    global_ev_vstride;
 int    global_ev_vorder;
 REAL global_ev_ctlPoints[IN_MAX_BEZIER_ORDER*IN_MAX_BEZIER_ORDER*IN_MAX_DIMENSION];

 REAL  global_grid_u0;
 REAL  global_grid_u1;
 int     global_grid_nu;
 REAL  global_grid_v0;
 REAL  global_grid_v1;
 int     global_grid_nv;

/*functions*/
 void inDoDomain2WithDerivs(int k, REAL u, REAL v, 
				REAL u1, REAL u2, int uorder, 
				REAL v1,  REAL v2, int vorder, 
				REAL *baseData,
				REAL *retPoint, REAL *retdu, REAL *retdv);
 void inPreEvaluate(int order, REAL vprime, REAL *coeff);
 void inPreEvaluateWithDeriv(int order, REAL vprime, REAL *coeff, REAL *coeffDeriv);
 void inComputeFirstPartials(REAL *p, REAL *pu, REAL *pv);
 void inComputeNormal2(REAL *pu, REAL *pv, REAL *n);
 void inDoEvalCoord2(REAL u, REAL v,
		     REAL *retPoint, REAL *retNormal);
 void inDoEvalCoord2NOGE(REAL u, REAL v,
		     REAL *retPoint, REAL *retNormal);
 void inMap2f(int k,
	      REAL ulower,
	      REAL uupper,
	      int ustride,
	      int uorder,
	      REAL vlower,
	      REAL vupper,
	      int vstride,
	      int vorder,
	      REAL *ctlPoints);

 void inMapGrid2f(int nu, REAL u0, REAL u1, 
		  int nv, REAL v0, REAL v1);

 void inEvalMesh2(int lowU, int lowV, int highU, int highV);
 void inEvalPoint2(int i, int j);
 void inEvalCoord2f(REAL u, REAL v);

void inEvalULine(int n_points, REAL v, REAL* u_vals, 
	int stride, REAL ret_points[][3], REAL ret_normals[][3]);

void inEvalVLine(int n_points, REAL u, REAL* v_vals, 
	int stride, REAL ret_points[][3], REAL ret_normals[][3]);

void inEvalUStrip(int n_upper, REAL v_upper, REAL* upper_val, 
                       int n_lower, REAL v_lower, REAL* lower_val
                       );
void inEvalVStrip(int n_left, REAL u_left, REAL* left_val, int n_right, REAL u_right, REAL* right_val);

void inPreEvaluateBV(int k, int uorder, int vorder, REAL vprime, REAL *baseData);
void inPreEvaluateBU(int k, int uorder, int vorder, REAL uprime, REAL *baseData);
void inPreEvaluateBV_intfac(REAL v )
  {
   inPreEvaluateBV(global_ev_k, global_ev_uorder, global_ev_vorder, (v-global_ev_v1)/(global_ev_v2-global_ev_v1), global_ev_ctlPoints);
  }

void inPreEvaluateBU_intfac(REAL u)
  {
    inPreEvaluateBU(global_ev_k, global_ev_uorder, global_ev_vorder, (u-global_ev_u1)/(global_ev_u2-global_ev_u1), global_ev_ctlPoints); 
  }

void inDoDomain2WithDerivsBV(int k, REAL u, REAL v,
			     REAL u1, REAL u2, int uorder,
			     REAL v1, REAL v2, int vorder,
			     REAL *baseData,
			     REAL *retPoint, REAL* retdu, REAL *retdv);

void inDoDomain2WithDerivsBU(int k, REAL u, REAL v,
			     REAL u1, REAL u2, int uorder,
			     REAL v1, REAL v2, int vorder,
			     REAL *baseData,
			     REAL *retPoint, REAL* retdu, REAL *retdv);


void inDoEvalCoord2NOGE_BV(REAL u, REAL v,
			   REAL *retPoint, REAL *retNormal);

void inDoEvalCoord2NOGE_BU(REAL u, REAL v,
			   REAL *retPoint, REAL *retNormal);

void inBPMEval(bezierPatchMesh* bpm);
void inBPMListEval(bezierPatchMesh* list);

/*-------------begin for surfEvalMachine -------------*/
surfEvalMachine em_vertex;
surfEvalMachine em_normal;
surfEvalMachine em_color;
surfEvalMachine em_texcoord;

int auto_normal_flag; //whether to output normla or not in callback
                      //determined by GL_AUTO_NORMAL and callback_auto_normal
int callback_auto_normal; //GLU_CALLBACK_AUTO_NORMAL_EXT
int vertex_flag;
int normal_flag;
int color_flag;
int texcoord_flag;

void inMap2fEM(int which, //0:vert,1:norm,2:color,3:tex
	       int dimension,
	      REAL ulower,
	      REAL uupper,
	      int ustride,
	      int uorder,
	      REAL vlower,
	      REAL vupper,
	      int vstride,
	      int vorder,
	      REAL *ctlPoints);

void inDoDomain2WithDerivsEM(surfEvalMachine *em, REAL u, REAL v, 
				REAL *retPoint, REAL *retdu, REAL *retdv);
void inDoDomain2EM(surfEvalMachine *em, REAL u, REAL v, 
				REAL *retPoint);
 void inDoEvalCoord2EM(REAL u, REAL v);

void inBPMEvalEM(bezierPatchMesh* bpm);
void inBPMListEvalEM(bezierPatchMesh* list);

/*-------------end for surfEvalMachine -------------*/


   /*************end for internal evaluators*****************/
		       
};

inline void StoredVertex::invoke(OpenGLSurfaceEvaluator *eval)
{
    switch(type) {
      case TYPECOORD:
	eval->coord2f(coord[0], coord[1]);
	break;
      case TYPEPOINT:
	eval->point2i(point[0], point[1]);
	break;
      default:
	break;
    }
}

#endif /* __gluglsurfeval_h_ */

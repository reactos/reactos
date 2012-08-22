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
 * slicer.c++
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "bufpool.h"
#include "slicer.h"
#include "backend.h"
#include "arc.h"
#include "gridtrimvertex.h"
#include "simplemath.h"
#include "trimvertex.h"
#include "varray.h"

#include "polyUtil.h" //for area()

//static int count=0;

/*USE_OPTTT is initiated in trimvertex.h*/

#ifdef USE_OPTTT
	#include <GL/gl.h>
#endif

//#define USE_READ_FLAG //whether to use new or old tesselator
                          //if defined, it reads "flagFile", 
                          // if the number is 1, then use new tess
                          // otherwise, use the old tess.
                         //if not defined, then use new tess.
#ifdef USE_READ_FLAG
static Int read_flag(char* name);
Int newtess_flag = read_flag("flagFile");
#endif

//#define COUNT_TRIANGLES
#ifdef COUNT_TRIANGLES
Int num_triangles = 0;
Int num_quads = 0;
#endif

#define max(a,b) ((a>b)? a:b)
#define ZERO 0.00001 /*determing whether a loop is a rectngle or not*/
#define equalRect(a,b) ((glu_abs(a-b) <= ZERO)? 1:0) //only used in tessellating a rectangle

#if 0 // UNUSED
static Int is_Convex(Arc_ptr loop)
{
  if(area(loop->tail(), loop->head(), loop->next->head()) <0 )
    return 0;
  for(Arc_ptr jarc = loop->next; jarc != loop; jarc = jarc->next)
    {
      if(area(jarc->tail(), jarc->head(), jarc->next->head()) < 0)
	return 0;
    }
  return 1;
}
#endif

/******triangulate a monotone polygon**************/
#include "monoTriangulation.h"
#if 0 // UNUSED
static int is_U_monotone(Arc_ptr loop)
{
  int n_changes=0;
  int prev_sign;
  int cur_sign;
  Arc_ptr temp;

  cur_sign = compV2InX(loop->head(), loop->tail());

  n_changes  = (compV2InX(loop->prev->head(), loop->prev->tail())
		!= cur_sign);

  for(temp=loop->next; temp != loop; temp = temp->next)
    {
      prev_sign = cur_sign;
      cur_sign = compV2InX(temp->head(), temp->tail());
      if(cur_sign != prev_sign)
       {
#ifdef DEBUG
	 printf("***change signe\n");
#endif
	 n_changes++;
       }
    }
  if(n_changes == 2) return 1;
  else
    return 0;
}
#endif

inline int compInY(REAL a[2], REAL b[2])
{
  if(a[1] < b[1])
    return -1;
  else if (a[1] > b[1])
    return 1;
  else if(a[0] > b[0])
    return 1;
  else return -1;
}

void monoTriangulationLoop(Arc_ptr loop, Backend& backend, primStream* pStream)
{
  int i;
  //find the top, bottom, increasing and decreasing chain
  //then call monoTrianulation
  Arc_ptr jarc, temp;
  Arc_ptr top;
  Arc_ptr bot;
  top = bot = loop;
  if(compInY(loop->tail(), loop->prev->tail()) < 0)
    {
      //first find bot
      for(temp = loop->next; temp != loop; temp = temp->next)
	{
	  if(compInY(temp->tail(), temp->prev->tail()) > 0)
	    break;
	}
      bot = temp->prev;
      //then find top
      for(temp=loop->prev; temp != loop; temp = temp->prev)
	{
	  if(compInY(temp->tail(), temp->prev->tail()) > 0)
	    break;
	}
      top = temp;
    }
  else //loop > loop->prev
    {
      for(temp=loop->next; temp != loop; temp = temp->next)
	{
	  if(compInY(temp->tail(), temp->prev->tail()) < 0)
	    break;
	}
      top = temp->prev;
      for(temp=loop->prev; temp != loop; temp = temp->prev)
	{
	  if(compInY(temp->tail(), temp->prev->tail()) < 0)
	    break;
	}
      bot = temp;
    }
  //creat increase and decrease chains
  vertexArray inc_chain(50); //this is a dynamci array
  for(i=1; i<=top->pwlArc->npts-2; i++) 
    {
      //the first vertex is the top which doesn't below to inc_chain
      inc_chain.appendVertex(top->pwlArc->pts[i].param);
    }
  for(jarc=top->next; jarc != bot; jarc = jarc->next)
    {
      for(i=0; i<=jarc->pwlArc->npts-2; i++)
	{
	  inc_chain.appendVertex(jarc->pwlArc->pts[i].param);
	}
      
    }
  vertexArray dec_chain(50);
  for(jarc = top->prev; jarc != bot; jarc = jarc->prev)
    {
      for(i=jarc->pwlArc->npts-2; i>=0; i--)
	{
	  dec_chain.appendVertex(jarc->pwlArc->pts[i].param);
	}
    }
  for(i=bot->pwlArc->npts-2; i>=1; i--)
    {
      dec_chain.appendVertex(jarc->pwlArc->pts[i].param);
    }	  

  monoTriangulationRec(top->tail(), bot->tail(), &inc_chain, 0,
		       &dec_chain, 0, &backend);

}

/********tesselate a rectanlge (OPTIMIZATION**************/
static void triangulateRectGen(Arc_ptr loop, int n_ulines, int n_vlines, Backend& backend);

static Int is_rect(Arc_ptr loop)
{
  Int nlines =1;
  for(Arc_ptr jarc = loop->next; jarc != loop; jarc = jarc->next)
    {
      nlines++;
      if(nlines == 5)
	break;
    }
  if(nlines != 4)
    return 0;


/*
printf("here1\n");
printf("loop->tail=(%f,%f)\n", loop->tail()[0], loop->tail()[1]);
printf("loop->head=(%f,%f)\n", loop->head()[0], loop->head()[1]);
printf("loop->next->tail=(%f,%f)\n", loop->next->tail()[0], loop->next->tail()[1]);
printf("loop->next->head=(%f,%f)\n", loop->next->head()[0], loop->next->head()[1]);
if(fglu_abs(loop->tail()[0] - loop->head()[0])<0.000001)
	printf("equal 1\n");
if(loop->next->tail()[1] == loop->next->head()[1])
	printf("equal 2\n");
*/

  if( (glu_abs(loop->tail()[0] - loop->head()[0])<=ZERO) && 
      (glu_abs(loop->next->tail()[1] - loop->next->head()[1])<=ZERO) &&
      (glu_abs(loop->prev->tail()[1] - loop->prev->head()[1])<=ZERO) &&
      (glu_abs(loop->prev->prev->tail()[0] - loop->prev->prev->head()[0])<=ZERO)
     )
    return 1;
  else if
    ( (glu_abs(loop->tail()[1] - loop->head()[1]) <= ZERO) && 
      (glu_abs(loop->next->tail()[0] - loop->next->head()[0]) <= ZERO) &&
      (glu_abs(loop->prev->tail()[0] - loop->prev->head()[0]) <= ZERO) &&
      (glu_abs(loop->prev->prev->tail()[1] - loop->prev->prev->head()[1]) <= ZERO)
     )
      return 1;
  else
    return 0;
}


//a line with the same u for opt
#ifdef USE_OPTTT
static void evalLineNOGE_BU(TrimVertex *verts, int n, Backend& backend)
{
  int i;
  backend.preEvaluateBU(verts[0].param[0]);
  for(i=0; i<n; i++)
    backend.tmeshvertNOGE_BU(&verts[i]);
}
#endif

//a line with the same v for opt
#ifdef USE_OPTTT
static void evalLineNOGE_BV(TrimVertex *verts, int n, Backend& backend)
{
  int i;
  backend.preEvaluateBV(verts[0].param[1]);

  for(i=0; i<n; i++)
    backend.tmeshvertNOGE_BV(&verts[i]);
}
#endif

#ifdef USE_OPTTT
static void evalLineNOGE(TrimVertex *verts, int n, Backend& backend)
{

  if(verts[0].param[0] == verts[n-1].param[0]) //all u;s are equal
    evalLineNOGE_BU(verts, n, backend);
  else if(verts[0].param[1] == verts[n-1].param[1]) //all v's are equal
    evalLineNOGE_BV(verts, n, backend);
  else
    {
      int i;
      for(i=0; i<n; i++)
	backend.tmeshvertNOGE(&verts[i]);
    }
}
#endif

inline void  OPT_OUTVERT(TrimVertex& vv, Backend& backend) 
{

#ifdef USE_OPTTT
  glNormal3fv(vv.cache_normal);                         
  glVertex3fv(vv.cache_point);
#else

  backend.tmeshvert(&vv);

#endif

}

static void triangulateRectAux(PwlArc* top, PwlArc* bot, PwlArc* left, PwlArc* right, Backend& backend);

static void triangulateRect(Arc_ptr loop, Backend& backend, int TB_or_LR, int ulinear, int vlinear)
{
  //we know the loop is a rectangle, but not sure which is top
  Arc_ptr top, bot, left, right;
  if(loop->tail()[1] == loop->head()[1])
    {
      if(loop->tail()[1] > loop->prev->prev->tail()[1])
	{

	top = loop;
	}
      else{

	top = loop->prev->prev;
	}
    }
  else 
    {
      if(loop->tail()[0] > loop->prev->prev->tail()[0])
	{
	  //loop is the right arc

	  top = loop->next;
	}
      else
	{

	  top = loop->prev;
	}
    }
  left = top->next;
  bot  = left->next;
  right= bot->next;

  //if u, v are both nonlinear, then if the
  //boundary is tessellated dense, we also
  //sample the inside to get a better tesslletant.
  if( (!ulinear) && (!vlinear))
    {
      int nu = top->pwlArc->npts;
      if(nu < bot->pwlArc->npts)
	nu = bot->pwlArc->npts;
      int nv = left->pwlArc->npts;
      if(nv < right->pwlArc->npts)
	nv = right->pwlArc->npts;
/*
      if(nu > 2 && nv > 2)
	{
	  triangulateRectGen(top, nu-2,  nv-2, backend);
	  return;
	}
*/
    }

  if(TB_or_LR == 1)
    triangulateRectAux(top->pwlArc, bot->pwlArc, left->pwlArc, right->pwlArc, backend);
  else if(TB_or_LR == -1)
    triangulateRectAux(left->pwlArc, right->pwlArc, bot->pwlArc, top->pwlArc, backend);    
  else
    {
      Int maxPointsTB = top->pwlArc->npts + bot->pwlArc->npts;
      Int maxPointsLR = left->pwlArc->npts + right->pwlArc->npts;
      
      if(maxPointsTB < maxPointsLR)
	triangulateRectAux(left->pwlArc, right->pwlArc, bot->pwlArc, top->pwlArc, backend);    
      else
	triangulateRectAux(top->pwlArc, bot->pwlArc, left->pwlArc, right->pwlArc, backend);
    }
}

static void triangulateRectAux(PwlArc* top, PwlArc* bot, PwlArc* left, PwlArc* right, Backend& backend)
{ //if(maxPointsTB >= maxPointsLR)
    {

      Int d, topd_left, topd_right, botd_left, botd_right, i,j;
      d = left->npts /2;

#ifdef USE_OPTTT
      evalLineNOGE(top->pts, top->npts, backend);
      evalLineNOGE(bot->pts, bot->npts, backend);
      evalLineNOGE(left->pts, left->npts, backend);
      evalLineNOGE(right->pts, right->npts, backend);
#endif

      if(top->npts == 2) {
	backend.bgntfan();
	OPT_OUTVERT(top->pts[0], backend);//the root
	for(i=0; i<left->npts; i++){
	  OPT_OUTVERT(left->pts[i], backend);
	}
	for(i=1; i<= bot->npts-2; i++){
	  OPT_OUTVERT(bot->pts[i], backend);
	}
	backend.endtfan();
	
	backend.bgntfan();
	OPT_OUTVERT(bot->pts[bot->npts-2], backend);
	for(i=0; i<right->npts; i++){
	  OPT_OUTVERT(right->pts[i], backend);
	}
	backend.endtfan();
      }
      else if(bot->npts == 2) {
	backend.bgntfan();
	OPT_OUTVERT(bot->pts[0], backend);//the root
	for(i=0; i<right->npts; i++){
	  OPT_OUTVERT(right->pts[i], backend);
	}
	for(i=1; i<= top->npts-2; i++){
	  OPT_OUTVERT(top->pts[i], backend);
	}
	backend.endtfan();
	
	backend.bgntfan();
	OPT_OUTVERT(top->pts[top->npts-2], backend);
	for(i=0; i<left->npts; i++){
	  OPT_OUTVERT(left->pts[i], backend);
	}
	backend.endtfan();
      }
      else { //both top and bot have >=3 points
	
	backend.bgntfan();
	
	OPT_OUTVERT(top->pts[top->npts-2], backend);
	
	for(i=0; i<=d; i++)
	  {
	    OPT_OUTVERT(left->pts[i], backend);	  
	  }
	backend.endtfan();
	
	backend.bgntfan();
	
	OPT_OUTVERT(bot->pts[1], backend);
	
	OPT_OUTVERT(top->pts[top->npts-2], backend);
	
	for(i=d; i< left->npts; i++)
	  {      
	    OPT_OUTVERT(left->pts[i], backend);      
	  }
	backend.endtfan();

	d = right->npts/2;
	//output only when d<right->npts-1 and
	//
	if(d<right->npts-1)
	  {	
	    backend.bgntfan();
	    //      backend.tmeshvert(& top->pts[1]);
	    OPT_OUTVERT(top->pts[1], backend);
	    for(i=d; i< right->npts; i++)
	      {
		//	  backend.tmeshvert(& right->pts[i]);
		
		OPT_OUTVERT(right->pts[i], backend);
		
	      }
	    backend.endtfan();
	  }
	
	backend.bgntfan();
	//      backend.tmeshvert(& bot->pts[bot->npts-2]);
	OPT_OUTVERT( bot->pts[bot->npts-2], backend);
	for(i=0; i<=d; i++)
	  {
	    //	  backend.tmeshvert(& right->pts[i]);
	    OPT_OUTVERT(right->pts[i], backend);
	    
	  }
	
	//      backend.tmeshvert(& top->pts[1]);
	OPT_OUTVERT(top->pts[1], backend);      
	
	backend.endtfan();


	topd_left = top->npts-2;
	topd_right = 1; //topd_left>= topd_right

	botd_left = 1;
	botd_right = bot->npts-2; //botd_left<= bot_dright

	
	if(top->npts < bot->npts)
	  {
	    int delta=bot->npts - top->npts;
	    int u = delta/2;
	    botd_left = 1+ u;
	    botd_right = bot->npts-2-( delta-u);	    
	
	    if(botd_left >1)
	      {
		backend.bgntfan();
		//	  backend.tmeshvert(& top->pts[top->npts-2]);
		OPT_OUTVERT(top->pts[top->npts-2], backend);
		for(i=1; i<= botd_left; i++)
		  {
		    //	      backend.tmeshvert(& bot->pts[i]);
		    OPT_OUTVERT(bot->pts[i] , backend);
		  }
		backend.endtfan();
	      }
	    if(botd_right < bot->npts-2)
	      {
		backend.bgntfan();
		OPT_OUTVERT(top->pts[1], backend);
		for(i=botd_right; i<= bot->npts-2; i++)
		  OPT_OUTVERT(bot->pts[i], backend);
		backend.endtfan();
	      }
	  }
	else if(top->npts> bot->npts)
	  {
	    int delta=top->npts-bot->npts;
	    int u = delta/2;
	    topd_left = top->npts-2 - u;
	    topd_right = 1+delta-u;
	    
	    if(topd_left < top->npts-2)
	      {
		backend.bgntfan();
		//	  backend.tmeshvert(& bot->pts[1]);
		OPT_OUTVERT(bot->pts[1], backend);
		for(i=topd_left; i<= top->npts-2; i++)
		  {
		    //	      backend.tmeshvert(& top->pts[i]);
		    OPT_OUTVERT(top->pts[i], backend);
		  }
		backend.endtfan();
	      }
	    if(topd_right > 1)
	      {
		backend.bgntfan();
		OPT_OUTVERT(bot->pts[bot->npts-2], backend);
		for(i=1; i<= topd_right; i++)
		  OPT_OUTVERT(top->pts[i], backend);
		backend.endtfan();
	      }
	  }
	
	if(topd_left <= topd_right) 
	  return;

	backend.bgnqstrip();
	for(j=botd_left, i=topd_left; i>=topd_right; i--,j++)
	  {
	    //	  backend.tmeshvert(& top->pts[i]);
	    //	  backend.tmeshvert(& bot->pts[j]);
	    OPT_OUTVERT(top->pts[i], backend);
	    OPT_OUTVERT(bot->pts[j], backend);
	  }
	backend.endqstrip();
	
      }
    }
}

  
static void triangulateRectCenter(int n_ulines, REAL* u_val, 
				  int n_vlines, REAL* v_val,
				  Backend& backend)
{

  // XXX this code was patched by Diego Santa Cruz <Diego.SantaCruz@epfl.ch>
  // to fix a problem in which glMapGrid2f() was called with bad parameters.
  // This has beens submitted to SGI but not integrated as of May 1, 2001.
  if(n_ulines>1 && n_vlines>1) {
    backend.surfgrid(u_val[0], u_val[n_ulines-1], n_ulines-1, 
                     v_val[n_vlines-1], v_val[0], n_vlines-1);
    backend.surfmesh(0,0,n_ulines-1,n_vlines-1);
  }

  return;

  /*
  for(i=0; i<n_vlines-1; i++)
    {

      backend.bgnqstrip();
      for(j=0; j<n_ulines; j++)
	{
	  trimVert.param[0] = u_val[j];
	  trimVert.param[1] = v_val[i+1];
	  backend.tmeshvert(& trimVert);	  

	  trimVert.param[1] = v_val[i];
	  backend.tmeshvert(& trimVert);	  
	}
      backend.endqstrip();

    }
    */
}

//it works for top, bot, left ad right, you need ot select correct arguments
static void triangulateRectTopGen(Arc_ptr arc, int n_ulines, REAL* u_val, Real v, int dir, int is_u, Backend& backend)
{

  if(is_u)
    {
      int i,k;
      REAL* upper_val = (REAL*) malloc(sizeof(REAL) * arc->pwlArc->npts);
      assert(upper_val);
      if(dir)
	{
	  for(k=0,i=arc->pwlArc->npts-1; i>=0; i--,k++)
	    {
	      upper_val[k] = arc->pwlArc->pts[i].param[0];
	    }	
	  backend.evalUStrip(arc->pwlArc->npts, arc->pwlArc->pts[0].param[1],
			     upper_val,
			     n_ulines, v, u_val);
	}
      else
	{
	  for(k=0,i=0;  i<arc->pwlArc->npts; i++,k++)
	    {
	      upper_val[k] = arc->pwlArc->pts[i].param[0];

	    }		  

	  backend.evalUStrip(
			     n_ulines, v, u_val,
			     arc->pwlArc->npts, arc->pwlArc->pts[0].param[1], upper_val
			     );	 
	}

      free(upper_val);
      return;
    }
  else //is_v
    {
      int i,k;
      REAL* left_val = (REAL*) malloc(sizeof(REAL) * arc->pwlArc->npts);
      assert(left_val);   
      if(dir)
	{
	  for(k=0,i=arc->pwlArc->npts-1; i>=0; i--,k++)
	    {
	      left_val[k] = arc->pwlArc->pts[i].param[1];
	    }
	  backend.evalVStrip(arc->pwlArc->npts, arc->pwlArc->pts[0].param[0],
			     left_val,
			     n_ulines, v, u_val);
	}
      else 
	{
	  for(k=0,i=0;  i<arc->pwlArc->npts; i++,k++)
	    {
	      left_val[k] = arc->pwlArc->pts[i].param[1];
	    }
	   backend.evalVStrip(
			     n_ulines, v, u_val,
			     arc->pwlArc->npts, arc->pwlArc->pts[0].param[0], left_val
			     );	
	}
      free(left_val);
      return;
    }
	
  //the following is a different version of the above code. If you comment
  //the above code, the following code will still work. The reason to leave
  //the folliwng code here is purely for testing purpose.
  /*
  int i,j;
  PwlArc* parc = arc->pwlArc;
  int d1 = parc->npts-1;
  int d2 = 0;
  TrimVertex trimVert;
  trimVert.nuid = 0;//????
  REAL* temp_u_val = u_val;
  if(dir ==0) //have to reverse u_val
    {
      temp_u_val = (REAL*) malloc(sizeof(REAL) * n_ulines);
      assert(temp_u_val);
      for(i=0; i<n_ulines; i++)
	temp_u_val[i] = u_val[n_ulines-1-i];
    }
  u_val = temp_u_val;

  if(parc->npts > n_ulines)
    {
      d1 = n_ulines-1;

      backend.bgntfan();
      if(is_u){
	trimVert.param[0] = u_val[0];
	trimVert.param[1] = v;
      }
      else
	{
	trimVert.param[1] = u_val[0];
	trimVert.param[0] = v;
      }
	  
      backend.tmeshvert(& trimVert);
      for(i=d1; i< parc->npts; i++)
	backend.tmeshvert(& parc->pts[i]);
      backend.endtfan();


    }
  else if(parc->npts < n_ulines)
    {
      d2 = n_ulines-parc->npts;


      backend.bgntfan();
      backend.tmeshvert(& parc->pts[parc->npts-1]);
      for(i=0; i<= d2; i++)
	{
	  if(is_u){
	    trimVert.param[0] = u_val[i];
	    trimVert.param[1] = v;
	  }
	  else
	    {
	      trimVert.param[1] = u_val[i];
	      trimVert.param[0] = v;
	    }
	  backend.tmeshvert(&trimVert);
	}
      backend.endtfan();

    }
  if(d1>0){


    backend.bgnqstrip();
    for(i=d1, j=d2; i>=0; i--, j++)
      {
	backend.tmeshvert(& parc->pts[i]);

	if(is_u){
	  trimVert.param[0] = u_val[j];
	  trimVert.param[1] = v;
	}
	else{
	  trimVert.param[1] = u_val[j];
	  trimVert.param[0] = v;
	}
	backend.tmeshvert(&trimVert);


	
      }
    backend.endqstrip();


  }
  if(dir == 0)  //temp_u_val was mallocated
    free(temp_u_val);
 */
}

//n_ulines is the number of ulines inside, and n_vlines is the number of vlines
//inside, different from meanings elsewhere!!!
static void triangulateRectGen(Arc_ptr loop, int n_ulines, int n_vlines, Backend& backend)
{

  int i;
  //we know the loop is a rectangle, but not sure which is top
  Arc_ptr top, bot, left, right;

  if(equalRect(loop->tail()[1] ,  loop->head()[1]))
    {

      if(loop->tail()[1] > loop->prev->prev->tail()[1])
	{

	top = loop;
	}
      else{

	top = loop->prev->prev;
	}
    }
  else 
    {
      if(loop->tail()[0] > loop->prev->prev->tail()[0])
	{
	  //loop is the right arc

	  top = loop->next;
	}
      else
	{

	  top = loop->prev;
	}
    }

  left = top->next;
  bot  = left->next;
  right= bot->next;

#ifdef COUNT_TRIANGLES
  num_triangles += loop->pwlArc->npts + 
                 left->pwlArc->npts + 
                 bot->pwlArc->npts + 
		  right->pwlArc->npts 
		      + 2*n_ulines + 2*n_vlines 
			-8;
  num_quads += (n_ulines-1)*(n_vlines-1);
#endif
/*
  backend.surfgrid(left->tail()[0], right->tail()[0], n_ulines+1, 
		   top->tail()[1], bot->tail()[1], n_vlines+1);
//  if(n_ulines>1 && n_vlines>1)
    backend.surfmesh(0,0,n_ulines+1,n_vlines+1);
return;
*/
  REAL* u_val=(REAL*) malloc(sizeof(REAL)*n_ulines);
  assert(u_val);
  REAL* v_val=(REAL*)malloc(sizeof(REAL) * n_vlines);
  assert(v_val);
  REAL u_stepsize = (right->tail()[0] - left->tail()[0])/( (REAL) n_ulines+1);
  REAL v_stepsize = (top->tail()[1] - bot->tail()[1])/( (REAL) n_vlines+1);
  Real temp=left->tail()[0]+u_stepsize;
  for(i=0; i<n_ulines; i++)
    {
      u_val[i] = temp;
      temp += u_stepsize;
    }
  temp = bot->tail()[1] + v_stepsize;
  for(i=0; i<n_vlines; i++)
    {
      v_val[i] = temp;
      temp += v_stepsize;
    }

  triangulateRectTopGen(top, n_ulines, u_val, v_val[n_vlines-1], 1,1, backend);
  triangulateRectTopGen(bot, n_ulines, u_val, v_val[0], 0, 1, backend);
  triangulateRectTopGen(left, n_vlines, v_val, u_val[0], 1, 0, backend);
  triangulateRectTopGen(right, n_vlines, v_val, u_val[n_ulines-1], 0,0, backend);




  //triangulate the center
  triangulateRectCenter(n_ulines, u_val, n_vlines, v_val, backend);

  free(u_val);
  free(v_val);
  
}


  

/**********for reading newtess_flag from a file**********/
#ifdef USE_READ_FLAG
static Int read_flag(char* name)
{
  Int ret;
  FILE* fp = fopen(name, "r");
  if(fp == NULL)
    {
      fprintf(stderr, "can't open file %s\n", name);
      exit(1);
    }
  fscanf(fp, "%i", &ret);
  fclose(fp);
  return ret;
}
#endif  

/***********nextgen tess****************/
#include "sampleMonoPoly.h"
directedLine* arcToDLine(Arc_ptr arc)
{
  int i;
  Real vert[2];
  directedLine* ret;
  sampledLine* sline = new sampledLine(arc->pwlArc->npts);
  for(i=0; i<arc->pwlArc->npts; i++)
    {
      vert[0] = arc->pwlArc->pts[i].param[0];
      vert[1] = arc->pwlArc->pts[i].param[1];
      sline->setPoint(i, vert);
    }
  ret = new directedLine(INCREASING, sline);
  return ret;
}
 
/*an pwlArc may not be a straight line*/
directedLine* arcToMultDLines(directedLine* original, Arc_ptr arc)
{
  directedLine* ret = original;
  int is_linear = 0;
  if(arc->pwlArc->npts == 2 )
    is_linear = 1;
  else if(area(arc->pwlArc->pts[0].param, arc->pwlArc->pts[1].param, arc->pwlArc->pts[arc->pwlArc->npts-1].param) == 0.0)
    is_linear = 1;
  
  if(is_linear)
    {
      directedLine *dline = arcToDLine(arc);
      if(ret == NULL)
	ret = dline;
      else
	ret->insert(dline);
      return ret;
    }
  else /*not linear*/
    {
      for(Int i=0; i<arc->pwlArc->npts-1; i++)
	{
	  Real vert[2][2];
	  vert[0][0] = arc->pwlArc->pts[i].param[0];
	  vert[0][1] = arc->pwlArc->pts[i].param[1];
	  vert[1][0] = arc->pwlArc->pts[i+1].param[0];
	  vert[1][1] = arc->pwlArc->pts[i+1].param[1];
	  
	  sampledLine *sline = new sampledLine(2, vert);
	  directedLine *dline = new directedLine(INCREASING, sline);
	  if(ret == NULL)
	    ret = dline;
	  else
	    ret->insert(dline);
	}
      return ret;
    }
}
  
     
	
directedLine* arcLoopToDLineLoop(Arc_ptr loop)
{
  directedLine* ret;

  if(loop == NULL)
    return NULL;
  ret = arcToMultDLines(NULL, loop);
//ret->printSingle();
  for(Arc_ptr temp = loop->next; temp != loop; temp = temp->next){
    ret = arcToMultDLines(ret, temp);
//ret->printSingle();
  }

  return ret;
}

/*
void Slicer::evalRBArray(rectBlockArray* rbArray, gridWrap* grid)
{
  TrimVertex *trimVert = (TrimVertex*)malloc(sizeof(TrimVertex));
  trimVert -> nuid = 0;//????

  Real* u_values = grid->get_u_values();
  Real* v_values = grid->get_v_values();

  Int i,j,k,l;

  for(l=0; l<rbArray->get_n_elements(); l++)
    {
      rectBlock* block = rbArray->get_element(l);
      for(k=0, i=block->get_upGridLineIndex(); i>block->get_lowGridLineIndex(); i--, k++)
	{

	  backend.bgnqstrip();
	  for(j=block->get_leftIndices()[k+1]; j<= block->get_rightIndices()[k+1]; j++)
	    {
	      trimVert->param[0] = u_values[j];
	      trimVert->param[1] = v_values[i];
	      backend.tmeshvert(trimVert);

	      trimVert->param[1] = v_values[i-1];
	      backend.tmeshvert(trimVert);

	    }
	  backend.endqstrip();

	}
    }
  
  free(trimVert);
}
*/

void Slicer::evalRBArray(rectBlockArray* rbArray, gridWrap* grid)
{
  Int i,j,k;
  
  Int n_vlines=grid->get_n_vlines();
  //the reason to switch the position of v_max and v_min is because of the
  //the orientation problem. glEvalMesh generates quad_strip clockwise, but
  //we need counter-clockwise.
  backend.surfgrid(grid->get_u_min(), grid->get_u_max(), grid->get_n_ulines()-1,
		   grid->get_v_max(), grid->get_v_min(), n_vlines-1);


  for(j=0; j<rbArray->get_n_elements(); j++)
    {
      rectBlock* block = rbArray->get_element(j);
      Int low = block->get_lowGridLineIndex();
      Int high = block->get_upGridLineIndex();

      for(k=0, i=high; i>low; i--, k++)
	{
	  backend.surfmesh(block->get_leftIndices()[k+1], n_vlines-1-i, block->get_rightIndices()[k+1]-block->get_leftIndices()[k+1], 1);
	}
    }
}


void Slicer::evalStream(primStream* pStream)
{
  Int i,j,k;
  k=0;
/*  TrimVertex X;*/
  TrimVertex *trimVert =/*&X*/  (TrimVertex*)malloc(sizeof(TrimVertex));
  trimVert -> nuid = 0;//???
  Real* vertices = pStream->get_vertices(); //for efficiency
  for(i=0; i<pStream->get_n_prims(); i++)
    {

     //ith primitive  has #vertices = lengths[i], type=types[i]
      switch(pStream->get_type(i)){
      case PRIMITIVE_STREAM_FAN:

	backend.bgntfan();

	for(j=0; j<pStream->get_length(i); j++)
	  {	    
	    trimVert->param[0] = vertices[k];
	    trimVert->param[1] = vertices[k+1];
	    backend.tmeshvert(trimVert);	   
	    
//	    backend.tmeshvert(vertices[k], vertices[k+1]);
	    k += 2;
	  }
	backend.endtfan();
	break;
	
      default:
	fprintf(stderr, "evalStream: not implemented yet\n");
	exit(1);

      }
    }
  free(trimVert);
}
	   
	   
	    

void Slicer::slice_new(Arc_ptr loop)
{
//count++;
//if(count == 78) count=1;
//printf("count=%i\n", count);
//if( ! (4<= count && count <=4)) return;


  Int num_ulines;
  Int num_vlines;
  Real uMin, uMax, vMin, vMax;
  Real mydu, mydv;
  uMin = uMax = loop->tail()[0];
  vMin = vMax = loop->tail()[1];
  mydu = (du>0)? du: -du;
  mydv = (dv>0)? dv: -dv;

  for(Arc_ptr jarc=loop->next; jarc != loop; jarc = jarc->next)
   {

     if(jarc->tail()[0] < uMin)
       uMin = jarc->tail()[0];
     if(jarc->tail()[0] > uMax)
       uMax = jarc->tail()[0];
     if(jarc->tail()[1] < vMin)
       vMin = jarc->tail()[1];
     if(jarc->tail()[1] > vMax)
       vMax = jarc->tail()[1];
   }

  if (uMax == uMin)
    return; // prevent divide-by-zero.  Jon Perry.  17 June 2002

  if(mydu > uMax - uMin)
    num_ulines = 2;
  else
    {
      num_ulines = 3 + (Int) ((uMax-uMin)/mydu);
    }
  if(mydv>=vMax-vMin)
    num_vlines = 2;
  else
    {
      num_vlines = 2+(Int)((vMax-vMin)/mydv);
    }

  Int isRect = is_rect(loop);

  if(isRect && (num_ulines<=2 || num_vlines<=2))
    {
      if(vlinear)
	triangulateRect(loop, backend, 1, ulinear, vlinear);
      else if(ulinear)
	triangulateRect(loop, backend, -1, ulinear, vlinear);	
      else
	triangulateRect(loop, backend, 0, ulinear, vlinear);		
    }

   else if(isRect)
    {
      triangulateRectGen(loop, num_ulines-2, num_vlines-2, backend); 
    }
  else if( (num_ulines<=2 || num_vlines <=2) && ulinear)
    {
      monoTriangulationFunBackend(loop, compV2InY, &backend);
    }
  else if( (!ulinear) && (!vlinear) && (num_ulines == 2) && (num_vlines > 2))
    {
      monoTriangulationFunBackend(loop, compV2InY, &backend);
    }
  else 
    {
      directedLine* poly = arcLoopToDLineLoop(loop);

      gridWrap grid(num_ulines, num_vlines, uMin, uMax, vMin, vMax);
      primStream pStream(20, 20);
      rectBlockArray rbArray(20);

      sampleMonoPoly(poly, &grid, ulinear, vlinear, &pStream, &rbArray);

      evalStream(&pStream);

      evalRBArray(&rbArray, &grid);
      
#ifdef COUNT_TRIANGLES
      num_triangles += pStream.num_triangles();
      num_quads += rbArray.num_quads();
#endif
      poly->deleteSinglePolygonWithSline();      
    }
  
#ifdef COUNT_TRIANGLES
  printf("num_triangles=%i\n", num_triangles);
  printf("num_quads = %i\n", num_quads);
#endif
}

void Slicer::slice(Arc_ptr loop)
{
#ifdef USE_READ_FLAG
  if(read_flag("flagFile"))
    slice_new(loop);
  else
    slice_old(loop);

#else
    slice_new(loop);
#endif

}

  

Slicer::Slicer( Backend &b ) 
	: CoveAndTiler( b ), Mesher( b ), backend( b )
{
    oneOverDu = 0;
    du = 0;
    dv = 0;
    isolines = 0;
    ulinear = 0;
    vlinear = 0;
}

Slicer::~Slicer()
{
}

void
Slicer::setisolines( int x )
{
    isolines = x;
}

void
Slicer::setstriptessellation( REAL x, REAL y )
{
    assert(x > 0 && y > 0);
    du = x;
    dv = y;
    setDu( du );
}

void
Slicer::slice_old( Arc_ptr loop )
{
    loop->markverts();

    Arc_ptr extrema[4];
    loop->getextrema( extrema );

    unsigned int npts = loop->numpts();
    TrimRegion::init( npts, extrema[0] );

    Mesher::init( npts );

    long ulines = uarray.init( du, extrema[1], extrema[3] );
//printf("ulines = %i\n", ulines);
    Varray varray;
    long vlines = varray.init( dv, extrema[0], extrema[2] );
//printf("vlines = %i\n", vlines);
    long botv = 0;
    long topv;
    TrimRegion::init( varray.varray[botv] );
    getGridExtent( &extrema[0]->pwlArc->pts[0], &extrema[0]->pwlArc->pts[0] );

    for( long quad=0; quad<varray.numquads; quad++ ) {
	backend.surfgrid( uarray.uarray[0], 
		       uarray.uarray[ulines-1], 
	 	       ulines-1, 
		       varray.vval[quad], 
		       varray.vval[quad+1], 
		       varray.voffset[quad+1] - varray.voffset[quad] );

	for( long i=varray.voffset[quad]+1; i <= varray.voffset[quad+1]; i++ ) {
    	    topv = botv++;
    	    advance( topv - varray.voffset[quad], 
		     botv - varray.voffset[quad], 
		     varray.varray[botv] );
	    if( i == vlines )
		getPts( extrema[2] );
	    else
		getPts( backend );
	    getGridExtent();
            if( isolines ) {
	        outline();
	    } else {
		if( canTile() ) 
		    coveAndTile();
		else
		    mesh();
	    }
        }
   }
}


void
Slicer::outline( void )
{
    GridTrimVertex upper, lower;
    Hull::init( );

    backend.bgnoutline();
    while( (nextupper( &upper )) ) {
	if( upper.isGridVert() )
	    backend.linevert( upper.g );
	else
	    backend.linevert( upper.t );
    }
    backend.endoutline();

    backend.bgnoutline();
    while( (nextlower( &lower )) ) {
	if( lower.isGridVert() )
	    backend.linevert( lower.g );
	else
	    backend.linevert( lower.t );
    }
    backend.endoutline();
}


void
Slicer::outline( Arc_ptr jarc )
{
    jarc->markverts();

    if( jarc->pwlArc->npts >= 2 ) {
	backend.bgnoutline();
	for( int j = jarc->pwlArc->npts-1; j >= 0; j--  )
	    backend.linevert( &(jarc->pwlArc->pts[j]) );
	backend.endoutline();
    }
}



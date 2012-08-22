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
**
*/
/*
*/

#include "gluos.h"
#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>
#include "zlassert.h"
#include "gridWrap.h"


/*******************grid structure****************************/
void gridWrap::print()
{
  printf("n_ulines = %i\n", n_ulines);
  printf("n_vlines = %i\n", n_vlines);
  printf("u_min=%f, umax=%f, vmin=%f, vmax=%f\n", u_min, u_max, v_min, v_max);
}

gridWrap::gridWrap(Int nUlines, Real* uvals,
		   Int nVlines, Real* vvals)
{
  assert(nUlines>=2);
  assert(nVlines>=2);

  is_uniform = 0;
  n_ulines = nUlines;
  n_vlines = nVlines;
  u_min = uvals[0];
  u_max = uvals[nUlines-1];
  v_min = vvals[0];
  v_max = vvals[nVlines-1];
  u_values = (Real*) malloc(sizeof(Real) * n_ulines);
  assert(u_values);
  v_values = (Real*) malloc(sizeof(Real) * n_vlines);
  assert(v_values);  
  
  Int i;
  for(i=0; i<n_ulines; i++)
    u_values[i] = uvals[i];
  for(i=0; i<n_vlines; i++)
    v_values[i] = vvals[i];
}
  
gridWrap::gridWrap(Int nUlines, Int nVlines,
		   Real uMin, Real uMax,
		   Real vMin, Real vMax
		   )
{
  is_uniform = 1;
  n_ulines = nUlines;
  n_vlines = nVlines;
  u_min = uMin;
  u_max = uMax;
  v_min = vMin;
  v_max = vMax;
  u_values = (Real*) malloc(sizeof(Real) * n_ulines);
  assert(u_values);
  v_values = (Real*) malloc(sizeof(Real) * n_vlines);
  assert(v_values);
  
  Int i;
  assert(nUlines>=2);
  assert(nVlines>=2);
  Real du = (uMax-uMin)/(nUlines-1);
  Real dv = (vMax-vMin)/(nVlines-1);

  float tempu=uMin;
  u_values[0] = tempu;
  for(i=1; i<nUlines; i++)
    {
      tempu += du;
      u_values[i] = tempu;
    }
  u_values[nUlines-1] = uMax;

  float tempv=vMin;
  v_values[0] = tempv;
  for(i=1; i<nVlines; i++)
    {
      tempv += dv;
      v_values[i] = tempv;
    }
  v_values[nVlines-1] = vMax;
}

gridWrap::~gridWrap()
{
  free(u_values);
  free(v_values);
}

void gridWrap::draw()
{
  int i,j;
  glBegin(GL_POINTS);
  for(i=0; i<n_ulines; i++)
    for(j=0; j<n_vlines; j++)
      glVertex2f(get_u_value(i), get_v_value(j));
  glEnd();
}

void gridWrap::outputFanWithPoint(Int v, Int uleft, Int uright, Real vert[2], primStream* pStream)
{
  Int i;
  if(uleft >= uright) 
    return; //no triangles to output.
    
  pStream->begin();
  pStream->insert(vert);

  assert(vert[1] != v_values[v]); //don't output degenerate triangles

  if(vert[1] > v_values[v]) //vertex is above this grid line: notice the orientation
    {
      for(i=uleft; i<=uright; i++)
	pStream->insert(u_values[i], v_values[v]);
    }
  else //vertex is below the grid line
    {
      for(i=uright; i>= uleft; i--)
	pStream->insert(u_values[i], v_values[v]);
    }	
  
  pStream->end(PRIMITIVE_STREAM_FAN);
}



/*each chain stores a number of consecutive 
 *V-lines within a grid. 
 *There is one grid vertex on each V-line.
 * The total number of V-lines is:
 *   nVlines.
 * with respect to the grid, the index of the first V-line is 
 *   firstVlineIndex.
 * So with respect to the grid, the index of the ith V-line is 
 *   firstVlineIndex-i.
 * the grid-index of the uline at the ith vline (recall that each vline has one grid point)
 * is ulineIndices[i]. The u_value is cached in ulineValues[i], that is,
 *   ulineValues[i] = grid->get_u_value(ulineIndices[i]) 
 */
gridBoundaryChain::gridBoundaryChain(
				     gridWrap* gr, 
				     Int first_vline_index, 
				     Int n_vlines, 
				     Int* uline_indices,
				     Int* inner_indices
				     )
: grid(gr), firstVlineIndex(first_vline_index), nVlines(n_vlines)
{
  ulineIndices = (Int*) malloc(sizeof(Int) * n_vlines);
  assert(ulineIndices);

  innerIndices = (Int*) malloc(sizeof(Int) * n_vlines);
  assert(innerIndices);

  vertices = (Real2*) malloc(sizeof(Real2) * n_vlines);
  assert(vertices);
  


  Int i;
  for(i=0; i<n_vlines; i++){
    ulineIndices[i] = uline_indices[i];
    innerIndices[i] = inner_indices[i];
  }
  
  for(i=0; i<n_vlines; i++){
    vertices[i][0] = gr->get_u_value(ulineIndices[i]);
    vertices[i][1] = gr->get_v_value(first_vline_index-i);
  }
}
  
void gridBoundaryChain::draw()
{
  Int i;
  glBegin(GL_LINE_STRIP);
  for(i=0; i<nVlines; i++)
    {
      glVertex2fv(vertices[i]);
    }
  glEnd();
}

void gridBoundaryChain::drawInner()
{
  Int i;
  for(i=1; i<nVlines; i++)
    {
      glBegin(GL_LINE_STRIP);
      glVertex2f(grid->get_u_value(innerIndices[i]), get_v_value(i-1) );
      glVertex2f(grid->get_u_value(innerIndices[i]), get_v_value(i) );
      glEnd();
    }
}
  
Int gridBoundaryChain::lookfor(Real v, Int i1, Int i2)
{
  Int mid;
  while(i1 < i2-1)
    {
      mid = (i1+i2)/2;
      if(v > vertices[mid][1])
	{
	  i2 = mid;
	}
      else
	i1 = mid;
    }
  return i1;
}

/*output the fan of the right end between grid line i-1 and grid line i*/
void gridBoundaryChain::rightEndFan(Int i, primStream* pStream)
{
  Int j;
  if(getUlineIndex(i) > getUlineIndex(i-1))
    {
      pStream->begin();
      pStream->insert(get_vertex(i-1));
      for(j=getUlineIndex(i-1); j<= getUlineIndex(i); j++)
	pStream->insert(grid->get_u_value(j), get_v_value(i));
      pStream->end(PRIMITIVE_STREAM_FAN);
    }
  else if(getUlineIndex(i) < getUlineIndex(i-1))
    {
      pStream->begin();
      pStream->insert(get_vertex(i));
      for(j=getUlineIndex(i-1); j>= getUlineIndex(i); j--)
	pStream->insert(grid->get_u_value(j), get_v_value(i-1));
      pStream->end(PRIMITIVE_STREAM_FAN);
    }
  //otherside, the two are equal, so there is no fan to output
}
		      

/*output the fan of the left end between grid line i-1 and grid line i*/
void gridBoundaryChain::leftEndFan(Int i, primStream* pStream)
{
  Int j;
  if(getUlineIndex(i) < getUlineIndex(i-1))
    {
      pStream->begin();
      pStream->insert(get_vertex(i-1));
      for(j=getUlineIndex(i); j<= getUlineIndex(i-1); j++)
	pStream->insert(grid->get_u_value(j), get_v_value(i));
      pStream->end(PRIMITIVE_STREAM_FAN);
    }
  else if(getUlineIndex(i) > getUlineIndex(i-1))
    {
      pStream->begin();
      pStream->insert(get_vertex(i));
      for(j=getUlineIndex(i); j>= getUlineIndex(i-1); j--)
	pStream->insert(grid->get_u_value(j), get_v_value(i-1));
      pStream->end(PRIMITIVE_STREAM_FAN);
    }
  /*otherwisem, the two are equal, so there is no fan to outout*/	  
}

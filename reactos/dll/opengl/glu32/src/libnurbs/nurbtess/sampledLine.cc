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

#include <stdlib.h>
#include <stdio.h>
#include <math.h> //for fabs()
#include "glimports.h"
#include "zlassert.h"
#include "sampledLine.h"

void sampledLine::setPoint(Int i, Real p[2]) 
{
  points[i][0]=p[0];  
  points[i][1]=p[1];
}


/*insert this single line in front of the oldList*/
sampledLine* sampledLine::insert(sampledLine *oldList)
{
  next = oldList;
  return this;
}

void sampledLine::deleteList()
{
  sampledLine *temp, *tempNext;
  for(temp = this; temp != NULL; temp = tempNext)
    {
      tempNext = temp->next;
      delete temp;
    }
}
 

/*space of points[][2] is allocated*/
sampledLine::sampledLine(Int n_points)
{
  npoints = n_points;
  points = (Real2*) malloc(sizeof(Real2) * n_points);
  assert(points);
  next = NULL;
}

/*space of points[][2] is allocated and 
 *points are copied
 */
sampledLine::sampledLine(Int n_points, Real2 pts[])
{
  int i;
  npoints = n_points;
  points = (Real2*) malloc(sizeof(Real2) * n_points);
  assert(points);
  for(i=0; i<npoints; i++) {
    points[i][0] = pts[i][0];
    points[i][1] = pts[i][1];
  }
  next = NULL;
}

sampledLine::sampledLine(Real pt1[2], Real pt2[2])
{
  npoints = 2;
  points = (Real2*) malloc(sizeof(Real2) * 2);
  assert(points);
  points[0][0] = pt1[0];
  points[0][1] = pt1[1];
  points[1][0] = pt2[0];
  points[1][1] = pt2[1];
  next = NULL;
}

//needs tp call init to setup
sampledLine::sampledLine()
{
  npoints = 0;
  points = NULL;
  next = NULL;
}

//warning: ONLY pointer is copies!!!
void sampledLine::init(Int n_points, Real2 *pts)
{
  npoints = n_points;
  points = pts;
}

/*points[] is dealocated
 */
sampledLine::~sampledLine()
{
  free(points);
}

void sampledLine::print()
{
  int i;
  printf("npoints=%i\n", npoints);

  for(i=0; i<npoints; i++){
    printf("(%f,%f)\n", points[i][0], points[i][1]);
  }

}

void sampledLine::tessellate(Real u_reso, Real v_reso)
{
  int i;

  Int nu, nv, n;
  nu = 1+(Int) (fabs((points[npoints-1][0] - points[0][0])) * u_reso);
  nv = 1+(Int) (fabs((points[npoints-1][1] - points[0][1])) * v_reso);

  if(nu > nv) n = nu;
  else 
    n = nv;
  if(n<1)
    n = 1;
  //du dv could be negative  
  Real du = (points[npoints-1][0] - points[0][0])/n;
  Real dv = (points[npoints-1][1] - points[0][1])/n;
  Real2 *temp = (Real2*) malloc(sizeof(Real2) * (n+1));
  assert(temp);

  Real u,v;
  for(i=0, u=points[0][0], v=points[0][1]; i<n; i++, u+=du, v+=dv)
    {
      temp[i][0] = u;
      temp[i][1] = v;
    }
  temp[n][0] = points[npoints-1][0];
  temp[n][1] = points[npoints-1][1];

  free(points);

  npoints = n+1;
  points = temp;

}

void sampledLine::tessellateAll(Real u_reso, Real v_reso)
{
  sampledLine* temp;
  for(temp = this; temp != NULL; temp = temp->next)
    {
      temp->tessellate(u_reso, v_reso);
    }
}

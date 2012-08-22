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

#include "partitionX.h"

#define CONCAVITY_ZERO 1.0e-6 //this number is used to test whether a vertex is concave (refelx) 
                              //or not. The test needs to compute the area of the three adjacent 
                              //vertices to see if the are is positive or negative. 

Int isCuspX(directedLine *v)
{
  //if v->prev <= v && v->next <= v
  //|| v->prev >= v && v->next >= v
  Real* T = v->head();
  Real* P = v->getPrev()->head();
  Real* N = v->getNext()->head();
  if(
     (compV2InX(T,P) != -1 && 
      compV2InX(T,N) != -1
      ) ||
     (compV2InX(T,P) != 1 && 
      compV2InX(T,N) != 1
      )
     )
    return 1;
  else
    return 0;
}

Int isReflexX(directedLine* v)
{
  Real* A = v->getPrev()->head();
  Real* B = v->head();
  Real* C = v->tail();
  Real Bx,By, Cx, Cy;
  //scale them in case they are too small
  Bx = 10*(B[0] - A[0]);
  By = 10*(B[1] - A[1]);
  Cx = 10*(C[0] - A[0]);
  Cy = 10*(C[1] - A[1]);

  if(Bx*Cy - Cx*By < -CONCAVITY_ZERO) return 1;
  else return 0;
}


/*return 
 *0: not-cusp
 *1: interior cusp
 *2: exterior cusp
 */
Int cuspTypeX(directedLine *v)
{
  if(! isCuspX(v)) return 0;
  else 
    {
//printf("isCusp,%f,%f\n", v->head()[0], v->head()[1]);
      if(isReflexX(v)) 
	{
//	  printf("isReflex\n");
	  return 1;
	}
      else
	{
//	  printf("not isReflex\n");
	  return 2;
	}
    }
}

Int numInteriorCuspsX(directedLine *polygon)
{
  directedLine *temp;
  int ret = 0;
  if(cuspTypeX(polygon) == 1)
    ret++;
  for(temp = polygon->getNext(); temp != polygon; temp = temp->getNext())
    if(cuspTypeX(temp) == 1)
      ret++;
  return ret;
}
  

void findInteriorCuspsX(directedLine *polygon, Int& ret_n_interior_cusps,
			directedLine** ret_interior_cusps)
{
  directedLine *temp;
  ret_n_interior_cusps = 0;
  if(cuspTypeX(polygon) == 1)
    {
      ret_interior_cusps[ret_n_interior_cusps++] = polygon;
    }
  for(temp = polygon->getNext(); temp != polygon; temp = temp->getNext())    
    if(cuspTypeX(temp) == 1)
      {
	ret_interior_cusps[ret_n_interior_cusps++] = temp;    
      }
}

directedLine* findDiagonal_singleCuspX(directedLine* cusp)
{
  directedLine* temp;
  Int is_minimal = ((compV2InX(cusp->head(), cusp->tail()) == -1)? 1:0);

  if(is_minimal)
    for(temp = cusp->getNext(); temp != cusp; temp = temp->getNext())
      {
	if(compV2InX(cusp->head(), temp->head()) == 1)
	  {	   
	    return temp;
	  }
      }
  else //is maxmal 
    for(temp = cusp->getNext(); temp != cusp; temp = temp->getNext())
      {
	if(compV2InX(cusp->head(), temp->head()) == -1)
	  {	   
	    return temp;
	  }
      }
  return NULL;
}
      

     

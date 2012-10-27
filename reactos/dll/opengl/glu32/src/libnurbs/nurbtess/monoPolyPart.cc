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
 *monoPolyPart.C
 *
 *To partition a v-monotone polygon into some uv-monotone polygons.
 *The algorithm is different from the general monotone partition algorithm.
 *while the general monotone partition algorithm works for this special case,
 *but it is more expensive (O(nlogn)). The algorithm implemented here takes
 *advantage of the fact that the input is a v-monotone polygon and it is
 *conceptually simpler and  computationally cheaper (a linear time algorithm).
 *The algorithm is described in Zicheng Liu's  paper
 * "Quality-Oriented Linear Time Tessellation".
 */

#include <stdlib.h>
#include <stdio.h>
#include "directedLine.h"
#include "monoPolyPart.h"

/*a vertex is u_maximal if both of its two neightbors are to the left of this 
 *vertex
 */
static Int is_u_maximal(directedLine* v)
{
  if (compV2InX(v->getPrev()->head(), v->head()) == -1 &&
      compV2InX(v->getNext()->head(), v->head()) == -1)
    return 1;
  else
    return 0;
}

/*a vertex is u_minimal if both of its two neightbors are to the right of this 
 *vertex
 */
static Int is_u_minimal(directedLine* v)
{
  if (compV2InX(v->getPrev()->head(), v->head()) == 1 &&
      compV2InX(v->getNext()->head(), v->head()) == 1)
    return 1;
  else
    return 0;
}

/*poly: a v-monotone polygon
 *return: a linked list of uv-monotone polygons.
 */
directedLine* monoPolyPart(directedLine* polygon)
{
  //handle special cases:
  if(polygon == NULL)
    return NULL;
  if(polygon->getPrev() == polygon)
    return polygon;
  if(polygon->getPrev() == polygon->getNext())
    return polygon;
  if(polygon->getPrev()->getPrev() == polygon->getNext())
    return polygon;

  //find the top and bottom vertexes
  directedLine *tempV, *topV, *botV;
  topV = botV = polygon;
  for(tempV = polygon->getNext(); tempV != polygon; tempV = tempV->getNext())
    {
      if(compV2InY(topV->head(), tempV->head())<0) {
	topV = tempV;
      }
      if(compV2InY(botV->head(), tempV->head())>0) {
	botV = tempV;
      }
    }

  //initilization
  directedLine *A, *B, *C, *D, *G, *H;
  //find A:the first u_maximal vertex on the left chain
  //and C: the left most vertex between top and A
  A = NULL;
  C = topV;
  for(tempV=topV->getNext(); tempV != botV; tempV = tempV->getNext())
    {
      if(tempV->head()[0] < C->head()[0])
	C = tempV;

      if(is_u_maximal(tempV))
	{
	  A = tempV;
	  break;
	}
    }
  if(A == NULL)
    {
      A = botV;
      if(A->head()[0] < C->head()[0])
	C = A;
    }
      
  //find B: the first u_minimal vertex on the right chain
  //and  D: the right most vertex between top and B
  B = NULL;
  D = topV;
  for(tempV=topV->getPrev(); tempV != botV; tempV = tempV->getPrev())
    {
      if(tempV->head()[0] > D->head()[0])
	D = tempV;
      if(is_u_minimal(tempV))
	{
	  B = tempV;
	  break;
	}
    }
  if(B == NULL)
    {
      B = botV;
      if(B->head()[0] > D->head()[0])
	D = B;
    }

  //error checking XXX
  if(C->head()[0] >= D->head()[0])
    return polygon;

  //find G on the left chain that is right above B
  for(tempV=topV; compV2InY(tempV->head(), B->head()) == 1;  tempV=tempV->getNext());
  G = tempV->getPrev();
  //find H on the right chain that is right above A
  for(tempV=topV; compV2InY(tempV->head(), A->head()) == 1; tempV = tempV->getPrev());
  H = tempV->getNext();
  
  //Main Loop
  directedLine* ret = NULL;
  directedLine* currentPolygon = polygon;
  while(1)
    {
      //if both B and D are equal to botV, then this polygon is already 
      //u-monotone
      if(A == botV && B == botV)
	{
	  ret = currentPolygon->insertPolygon(ret);
	  return ret;
	}
      else //not u-monotone
	{
	  directedLine *ret_p1, *ret_p2;
	  if(compV2InY(A->head(),B->head()) == 1) //A is above B
	    {
	      directedLine* E = NULL;
	      for(tempV = C; tempV != D; tempV = tempV->getPrev())
		{
		  if(tempV->head()[0] >= A->head()[0])
		    {
		      E = tempV;
		      break;
		    }
		}

	      if(E == NULL)
		E = D;
	      if(E->head()[0]> H->head()[0])
		E = H;
	      //connect AE and output polygon ECA
	      polygon->connectDiagonal_2slines(A, E,
					       &ret_p1,
					       &ret_p2,
					       NULL);
	      ret = ret_p2->insertPolygon(ret);
	      currentPolygon = ret_p1;

	      if(E == D)
		D = ret_p1;
	      if(E == H)
		H = ret_p1;
              if(G->head()[1] >= A->head()[1])
		G = A;
	      //update A to be the next u-maxiaml vertex on left chain
	      //and C the leftmost vertex between the old A and the new A
	      C = A;
	      for(tempV = A->getNext(); tempV != botV; tempV = tempV->getNext())
		{

		  if(tempV->head()[0] < C->head()[0])
		    C = tempV;
		  if(is_u_maximal(tempV))
		    {
		      A = tempV;
		      break;
		    }
		}

	      if(tempV == botV)
		{
		  A = botV;
		  if(botV->head()[0] < C->head()[0])
		    C = botV;
		}
	      //update H

              if(A == botV)
		H = botV;
              else
		{
		  for(tempV = H; compV2InY(tempV->head(), A->head()) == 1; tempV = tempV->getPrev());
		  H = tempV->getNext();
		}

	    }
	  else //A is below B
	    {

	      directedLine* F = NULL;
	      for(tempV = D; tempV != C; tempV = tempV->getNext())
		{
		  if(tempV->head()[0] <= B->head()[0])
		    {
		      F = tempV;
		      break;
		    }
		}
	      if(F == NULL)
		F = C;
	      if(F->head()[0] < G->head()[0])
		F = G;

	      //connect FB
	      polygon->connectDiagonal_2slines(F, B, 
					       &ret_p1,
					       &ret_p2,
					       NULL);
	      ret = ret_p2->insertPolygon(ret);
	      currentPolygon = ret_p1;
	      B = ret_p1;
	      if(H ->head()[1] >= B->head()[1])
		H = ret_p1;

	      //update B to be the next u-minimal vertex on right chain
	      //and D the rightmost vertex between the old B and the new B
	      D = B;
	      for(tempV = B->getPrev(); tempV != botV; tempV = tempV->getPrev())
		{
		  if(tempV->head()[0] > D->head()[0])
		    D = tempV;
		  if(is_u_minimal(tempV))
		    {
		      B = tempV;
		      break;
		    }
		}
	      if(tempV == botV)
		{
		  B = botV;
		  if(botV->head()[0] > D->head()[0])
		    D = botV;
		}
	      //update G
              if(B == botV)
		G = botV; 
              else
		{
		  for(tempV = G; compV2InY(tempV->head(), B->head()) == 1;  tempV = tempV->getNext());
		  G = tempV->getPrev();
		}
	    } //end of A is below B
	} //end not u-monotone	
    } //end of main loop
}




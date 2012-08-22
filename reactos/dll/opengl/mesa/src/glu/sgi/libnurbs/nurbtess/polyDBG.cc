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
#include <math.h>
#include "zlassert.h"
#include "polyDBG.h"

#ifdef __WATCOMC__
#pragma warning 14  10
#pragma warning 391 10
#pragma warning 726 10
#endif

static Real area(Real A[2], Real B[2], Real C[2])
{
  Real Bx, By, Cx, Cy;
  Bx = B[0] - A[0];
  By = B[1] - A[1];
  Cx = C[0] - A[0];
  Cy = C[1] - A[1];
  return Bx*Cy - Cx*By;
}

Int DBG_isConvex(directedLine *poly)
{
  directedLine* temp;
  if(area(poly->head(), poly->tail(), poly->getNext()->tail()) < 0.00000)
    return 0;
  for(temp = poly->getNext(); temp != poly; temp = temp->getNext())
    {
      if(area(temp->head(), temp->tail(), temp->getNext()->tail()) < 0.00000)
	return 0;
    }
  return 1;
}

Int DBG_is_U_monotone(directedLine* poly)
{
  Int n_changes = 0;
  Int prev_sign;
  Int cur_sign;
   directedLine* temp;
  cur_sign = compV2InX(poly->tail(), poly->head());

  n_changes = (compV2InX(poly->getPrev()->tail(), poly->getPrev()->head())
	       != cur_sign);

  for(temp = poly->getNext(); temp != poly; temp = temp->getNext())
    {
      prev_sign = cur_sign;
      cur_sign = compV2InX(temp->tail(), temp->head());

      if(cur_sign != prev_sign)
	n_changes++;
    }

  if(n_changes ==2) return 1;
  else return 0;
}

/*if u-monotone, and there is a long horizontal edge*/
Int DBG_is_U_direction(directedLine* poly)
{
/*
  if(! DBG_is_U_monotone(poly))
    return 0;
*/
  Int V_count = 0;
  Int U_count = 0;
  directedLine* temp;
  if( fabs(poly->head()[0] - poly->tail()[0]) <= fabs(poly->head()[1]-poly->tail()[1]))
    V_count += poly->get_npoints();
  else
    U_count += poly->get_npoints();
  /*
  else if(poly->head()[1] == poly->tail()[1])
    U_count += poly->get_npoints();
    */
  for(temp = poly->getNext(); temp != poly; temp = temp->getNext())
    {
      if( fabs(temp->head()[0] - temp->tail()[0]) <= fabs(temp->head()[1]-temp->tail()[1]))
	V_count += temp->get_npoints();
      else
	U_count += temp->get_npoints();
      /*
      if(temp->head()[0] == temp->tail()[0])
	V_count += temp->get_npoints();
      else if(temp->head()[1] == temp->tail()[1])
	U_count += temp->get_npoints();
	*/
    }

  if(U_count > V_count) return 1;
  else return 0;
}

/*given two line segments, determine whether
 *they intersect each other or not.
 *return 1 if they do,
 *return 0 otherwise
 */
Int DBG_edgesIntersect(directedLine* l1, directedLine* l2)
{
  if(l1->getNext() == l2)
    {
      if(area(l1->head(), l1->tail(), l2->tail()) == 0) //colinear
	{
	  if( (l1->tail()[0] - l1->head()[0])*(l2->tail()[0]-l2->head()[0]) +
	     (l1->tail()[1] - l1->head()[1])*(l2->tail()[1]-l2->head()[1]) >=0)
	    return 0; //not intersect
	  else
	    return 1;
	}
      //else we use the normal code
    }
  else if(l1->getPrev() == l2)
    {
      if(area(l2->head(), l2->tail(), l1->tail()) == 0) //colinear
	{
	  if( (l2->tail()[0] - l2->head()[0])*(l1->tail()[0]-l1->head()[0]) +
	     (l2->tail()[1] - l2->head()[1])*(l1->tail()[1]-l1->head()[1]) >=0)
	    return 0; //not intersect
	  else
	    return 1;
	}
      //else we use the normal code
    }
  else //the two edges are not connected
    {
      if((l1->head()[0] == l2->head()[0] &&
	 l1->head()[1] == l2->head()[1]) ||
	 (l1->tail()[0] == l2->tail()[0] &&
	 l1->tail()[1] == l2->tail()[1]))
	return 1;

    }


  if(
     (
      area(l1->head(), l1->tail(), l2->head())
      *
      area(l1->head(), l1->tail(), l2->tail())
      < 0
      )
     &&
     (
      area(l2->head(), l2->tail(), l1->head())
      *area(l2->head(), l2->tail(), l1->tail())
      < 0
      )
     )
    return 1;
  else
    return 0;
}

/*whether AB and CD intersect
 *return 1 if they do
 *retur 0 otheriwse
 */
Int DBG_edgesIntersectGen(Real A[2], Real B[2], Real C[2], Real D[2])
{
  if(
     (
      area(A, B, C) * area(A,B,D) <0
      )
     &&
     (
      area(C,D,A) * area(C,D,B) < 0
      )
     )
    return 1;
  else
    return 0;
}

/*determien whether    (A,B) interesect chain[start] to [end]
 */
Int DBG_intersectChain(vertexArray* chain, Int start, Int end, Real A[2], Real B[2])
{
  Int i;
  for(i=start; i<=end-2; i++)
    if(DBG_edgesIntersectGen(chain->getVertex(i), chain->getVertex(i+1), A, B))
      return 1;

  return 0;
}

/*determine whether a polygon intersect itself or not
 *return 1 is it does,
 *	 0 otherwise
 */
Int DBG_polygonSelfIntersect(directedLine* poly)
{
  directedLine* temp1;
  directedLine* temp2;
  temp1=poly;
  for(temp2=temp1->getNext(); temp2 != temp1; temp2=temp2->getNext())
    {
      if(DBG_edgesIntersect(temp1, temp2))
	{
	  return 1;
	}

    }

  for(temp1=poly->getNext(); temp1 != poly; temp1 = temp1->getNext())
    for(temp2=temp1->getNext(); temp2 != temp1; temp2=temp2->getNext())
      {
	if(DBG_edgesIntersect(temp1, temp2))
	  {
	    return 1;
	  }
      }
  return 0;
}

/*check whether a line segment intersects a  polygon
 */
Int DBG_edgeIntersectPoly(directedLine* edge, directedLine* poly)
{
  directedLine* temp;
  if(DBG_edgesIntersect(edge, poly))
    return 1;
  for(temp=poly->getNext(); temp != poly; temp=temp->getNext())
    if(DBG_edgesIntersect(edge, temp))
      return 1;
  return 0;
}

/*check whether two polygons intersect
 */
Int DBG_polygonsIntersect(directedLine* p1, directedLine* p2)
{
  directedLine* temp;
  if(DBG_edgeIntersectPoly(p1, p2))
    return 1;
  for(temp=p1->getNext(); temp!= p1; temp = temp->getNext())
    if(DBG_edgeIntersectPoly(temp, p2))
      return 1;
  return 0;
}

/*check whether there are polygons intersecting each other in
 *a list of polygons
 */
Int DBG_polygonListIntersect(directedLine* pList)
{
  directedLine *temp;
  for(temp=pList; temp != NULL; temp = temp->getNextPolygon())
    if(DBG_polygonSelfIntersect(temp))
      return 1;
  directedLine* temp2;
  for(temp=pList; temp!=NULL; temp=temp->getNextPolygon())
    {
      for(temp2=temp->getNextPolygon(); temp2 != NULL; temp2=temp2->getNextPolygon())
	if(DBG_polygonsIntersect(temp, temp2))
	  return 1;
    }

  return 0;
}


Int DBG_isCounterclockwise(directedLine* poly)
{
  return (poly->polyArea() > 0);
}

/*ray: v0 with direction (dx,dy).
 *edge: v1-v2.
 * the extra point v10[2] is given for the information at
 *v1. Basically this edge is connectd to edge
 * v10-v1. If v1 is on the ray,
 * then we need v10  to determine whether this ray intersects
 * the edge or not (that is, return 1 or return 0).
 * If v1 is on the ray, then if v2 and v10 are on the same side of the ray,
 * we return 0, otherwise return 1.
 *For v2, if v2 is on the ray, we always return 0.
 *Notice that v1 and v2 are not symmetric. So the edge is directed!!!
 * The purpose for this convention is such that: a point is inside a polygon
 * if and only if it intersets with odd number of edges.
 */
Int DBG_rayIntersectEdge(Real v0[2], Real dx, Real dy, Real v10[2], Real v1[2], Real v2[2])
{
/*
if( (v1[1] >= v0[1] && v2[1]<= v0[1] )
  ||(v2[1] >= v0[1] && v1[1]<= v0[1] )
   )
  printf("rayIntersectEdge, *********\n");
*/

  Real denom = (v2[0]-v1[0])*(-dy) - (v2[1]-v1[1]) * (-dx);
  Real nomRay = (v2[0]-v1[0]) * (v0[1] - v1[1]) - (v2[1]-v1[1])*(v0[0]-v1[0]);
  Real nomEdge = (v0[0]-v1[0]) * (-dy) - (v0[1]-v1[1])*(-dx);


  /*if the ray is parallel to the edge, return 0: not intersect*/
  if(denom == 0.0)
    return 0;

  /*if v0 is on the edge, return 0: not intersect*/
  if(nomRay == 0.0)
    return 0;

  /*if v1 is on the positive ray, and the neighbor of v1 crosses the ray
   *return 1: intersect
   */
  if(nomEdge == 0)
    { /*v1 is on the positive or negative ray*/

/*
      printf("v1 is on the ray\n");
*/

      if(dx*(v1[0]-v0[0])>=0 && dy*(v1[1]-v0[1])>=0) /*v1 on positive ray*/
	{
	  if(area(v0, v1, v10) * area(v0, v1, v2) >0)
	    return 0;
	  else
	    return 1;
	}
      else /*v1 on negative ray*/
	return 0;
    }

  /*if v2 is on the ray, always return 0: not intersect*/
  if(nomEdge == denom) {
/*    printf("v2 is on the ray\n");*/
    return 0;
  }

  /*finally */
  if(denom*nomRay>0 && denom*nomEdge>0 && nomEdge/denom <=1.0)
    return 1;
  return 0;
}


/*return the number of intersections*/
Int DBG_rayIntersectPoly(Real v0[2], Real dx, Real dy, directedLine* poly)
{
  directedLine* temp;
  Int count=0;
  if(DBG_rayIntersectEdge(v0, dx, dy, poly->getPrev()->head(), poly->head(), poly->tail()))
    count++;

  for(temp=poly->getNext(); temp != poly; temp = temp->getNext())
    if(DBG_rayIntersectEdge(v0, dx, dy, temp->getPrev()->head(), temp->head(), temp->tail()))
      count++;
/*printf("ray intersect poly: count=%i\n", count);*/
  return count;
}

Int DBG_pointInsidePoly(Real v[2], directedLine* poly)
{
/*
printf("enter pointInsidePoly , v=(%f,%f)\n", v[0], v[1]);
printf("the polygon is\n");
poly->printList();
*/
  /*for debug purpose*/
  assert( (DBG_rayIntersectPoly(v,1,0,poly) % 2 )
	 == (DBG_rayIntersectPoly(v,1,Real(0.1234), poly) % 2 )
	 );
  if(DBG_rayIntersectPoly(v, 1, 0, poly) % 2 == 1)
    return 1;
  else
    return 0;
}

/*return the number of polygons which contain thie polygon
 * as a subset
 */
Int DBG_enclosingPolygons(directedLine* poly, directedLine* list)
{
  directedLine* temp;
  Int count=0;
/*
printf("%i\n", DBG_pointInsidePoly(poly->head(),
				   list->getNextPolygon()
				   ->getNextPolygon()
				   ->getNextPolygon()
				   ->getNextPolygon()
));
*/

  for(temp = list; temp != NULL; temp = temp->getNextPolygon())
    {
      if(poly != temp)
	if(DBG_pointInsidePoly(poly->head(), temp))
	  count++;
/*	printf("count=%i\n", count);*/
    }
  return count;
}

void  DBG_reverse(directedLine* poly)
{
  if(poly->getDirection() == INCREASING)
    poly->putDirection(DECREASING);
  else
    poly->putDirection(INCREASING);

  directedLine* oldNext = poly->getNext();
  poly->putNext(poly->getPrev());
  poly->putPrev(oldNext);

  directedLine* temp;
  for(temp=oldNext; temp!=poly; temp = oldNext)
    {
      if(temp->getDirection() == INCREASING)
	temp->putDirection(DECREASING);
      else
	temp->putDirection(INCREASING);

      oldNext = temp->getNext();
      temp->putNext(temp->getPrev());
      temp->putPrev(oldNext);
    }
  printf("reverse done\n");
}

Int DBG_checkConnectivity(directedLine *polygon)
{
  if(polygon == NULL) return 1;
  directedLine* temp;
  if(polygon->head()[0] != polygon->getPrev()->tail()[0] ||
     polygon->head()[1] != polygon->getPrev()->tail()[1])
    return 0;
  for(temp=polygon->getNext(); temp != polygon; temp=temp->getNext())
    {
      if(temp->head()[0] != temp->getPrev()->tail()[0] ||
	 temp->head()[1] != temp->getPrev()->tail()[1])
	return 0;
    }
  return 1;
}

/*print out error message.
 *If it cannot modify the polygon list to make it satify the
 *requirements, return 1.
 *otherwise modify the polygon list, and return 0
 */
Int DBG_check(directedLine *polyList)
{
  directedLine* temp;
  if(polyList == NULL) return 0;

  /*if there are intersections, print out error message
   */
  if(DBG_polygonListIntersect(polyList))
    {
      fprintf(stderr, "DBG_check: there are self intersections, don't know to modify the polygons\n");
    return 1;
    }

  /*check the connectivity of each polygon*/
  for(temp = polyList; temp!= NULL; temp = temp ->getNextPolygon())
    {
      if(! DBG_checkConnectivity(temp))
	{
	  fprintf(stderr, "DBG_check, polygon not connected\n");
	  return 1;
	}
    }

  /*check the orientation of each polygon*/
  for(temp = polyList; temp!= NULL; temp = temp ->getNextPolygon())
    {


      Int correctDir;

      if( DBG_enclosingPolygons(temp, polyList) % 2 == 0)
	correctDir = 1; /*counterclockwise*/
      else
	correctDir = 0; /*clockwise*/

      Int actualDir = DBG_isCounterclockwise(temp);

      if(correctDir != actualDir)
	{
	  fprintf(stderr, "DBG_check: polygon with incorrect orientations. reversed\n");

	  DBG_reverse(temp);
	}

    }
  return 0;
}

/**************handle self intersections*****************/
//determine whether e interects [begin, end] or not
static directedLine* DBG_edgeIntersectChainD(directedLine *e,
			       directedLine *begin, directedLine *end)
{
  directedLine *temp;
  for(temp=begin; temp != end; temp = temp->getNext())
    {
      if(DBG_edgesIntersect(e, temp))
	 return temp;
    }
  if(DBG_edgesIntersect(e, end))
    return end;
  return NULL;
}

//given a polygon, cut the edges off and finally obtain a
//a polygon without intersections. The cut-off edges are
//dealloated. The new polygon is returned.
directedLine* DBG_cutIntersectionPoly(directedLine *polygon, int& cutOccur)
{
  directedLine *begin, *end, *next;
  begin = polygon;
  end = polygon;
  cutOccur = 0;
  while( (next = end->getNext()) != begin)
    {
      directedLine *interc = NULL;
      if( (interc = DBG_edgeIntersectChainD(next, begin, end)))
	{
	  int fixed = 0;
	  if(DBG_edgesIntersect(next, interc->getNext()))
	     {
	       //trying to fix it
	       Real buf[2];
	       int i;
	       Int n=5;
	       buf[0] = interc->tail()[0];
	       buf[1] = interc->tail()[1];

	       for(i=1; i<n; i++)
		 {
		   Real r = ((Real)i) / ((Real) n);
		   Real u = (1-r) * interc->head()[0] + r * interc->tail()[0];
		   Real v = (1-r) * interc->head()[1] + r * interc->tail()[1];
		   interc->tail()[0] = interc->getNext()->head()[0] = u;
		   interc->tail()[1] = interc->getNext()->head()[1] = v;
		   if( (! DBG_edgesIntersect(next, interc)) &&
		      (! DBG_edgesIntersect(next, interc->getNext())))
		     break; //we fixed it
		 }
	       if(i==n) // we didn't fix it
		 {
		   fixed = 0;
		   //back to original
		   interc->tail()[0] = interc->getNext()->head()[0] = buf[0];
		   interc->tail()[1] = interc->getNext()->head()[1] = buf[1];
		 }
	       else
		 {
		   fixed = 1;
		 }
	     }
	  if(fixed == 0)
	    {
	      cutOccur = 1;
	      begin->deleteSingleLine(next);

	      if(begin != end)
		{
		  if(DBG_polygonSelfIntersect(begin))
		    {
		      directedLine* newEnd = end->getPrev();
		      begin->deleteSingleLine(end);
		      end = newEnd;
		    }
		}
	    }
	  else
	    {
	      end = end->getNext();
	    }
	}
      else
	{
	  end = end->getNext();
	}
    }
  return begin;
}

//given a polygon, cut the edges off and finally obtain a
//a polygon without intersections. The cut-off edges are
//dealloated. The new polygon is returned.
#if 0 // UNUSED
static directedLine* DBG_cutIntersectionPoly_notwork(directedLine *polygon)
{
  directedLine *crt;//current polygon
  directedLine *begin;
  directedLine *end;
  directedLine *temp;
  crt = polygon;
  int find=0;
  while(1)
    {
//printf("loop\n");
      //if there are less than 3 edges, we should stop
      if(crt->getPrev()->getPrev() == crt)
	return NULL;

      if(DBG_edgesIntersect(crt, crt->getNext()) ||
	(crt->head()[0] == crt->getNext()->tail()[0] &&
	crt->head()[1] == crt->getNext()->tail()[1])
	 )
	{
	  find = 1;
	  crt=crt->deleteChain(crt, crt->getNext());
	}
      else
	{
	  //now we know crt and crt->getNext do not intersect
	  begin = crt;
	  end = crt->getNext();
//printf("begin=(%f,%f)\n", begin->head()[0], begin->head()[1]);
//printf("end=(%f,%f)\n", end->head()[0], end->head()[1]);
	  for(temp=end->getNext(); temp!=begin; temp= temp->getNext())
	    {
//printf("temp=(%f,%f)\n", temp->head()[0], temp->head()[1]);
	       directedLine *intersect = DBG_edgeIntersectChainD(temp, begin, end);
	       if(intersect != NULL)
		{
		  crt = crt->deleteChain(intersect, temp);
		  find=1;
		  break; //the for loop
		}
	      else
		{
		  end = temp;
		}
	    }
	}
      if(find == 0)
	return crt;
      else
	find = 0;    //go to next loop
}
}
#endif

directedLine* DBG_cutIntersectionAllPoly(directedLine* list)
{
  directedLine* temp;
  directedLine* tempNext=NULL;
  directedLine* ret = NULL;
  int cutOccur=0;
  for(temp=list; temp != NULL; temp = tempNext)
    {
      directedLine *left;
      tempNext = temp->getNextPolygon();

      left = DBG_cutIntersectionPoly(temp, cutOccur);
      if(left != NULL)
	ret=left->insertPolygon(ret);
    }
  return ret;
}

sampledLine*  DBG_collectSampledLinesAllPoly(directedLine *polygonList)
{
  directedLine *temp;
  sampledLine* tempHead = NULL;
  sampledLine* tempTail = NULL;
  sampledLine* cHead = NULL;
  sampledLine* cTail = NULL;

  if(polygonList == NULL)
    return NULL;

  DBG_collectSampledLinesPoly(polygonList, cHead, cTail);

  assert(cHead);
  assert(cTail);
  for(temp = polygonList->getNextPolygon(); temp != NULL; temp = temp->getNextPolygon())
    {
      DBG_collectSampledLinesPoly(temp, tempHead, tempTail);
      cTail->insert(tempHead);
      cTail = tempTail;
    }
  return cHead;
}

void  DBG_collectSampledLinesPoly(directedLine *polygon, sampledLine*& retHead, sampledLine*& retTail)
{
  directedLine *temp;
  retHead = NULL;
  retTail = NULL;
  if(polygon == NULL)
    return;

  retHead = retTail = polygon->getSampledLine();
  for(temp = polygon->getNext(); temp != polygon; temp=temp->getNext())
    {
      retHead = temp->getSampledLine()->insert(retHead);
    }
}

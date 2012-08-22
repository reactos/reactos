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
#include <math.h>

#ifndef max
#define max(a,b) ((a>b)? a:b)
#endif
#ifndef min
#define min(a,b) ((a>b)? b:a)
#endif

#include <GL/gl.h>

#include "glimports.h"
#include "zlassert.h"
#include "sampleMonoPoly.h"
#include "sampleComp.h"
#include "polyDBG.h"
#include "partitionX.h"


#define ZERO 0.00001

//#define  MYDEBUG

//#define SHORTEN_GRID_LINE
//see work/newtess/internal/test/problems


/*split a polygon so that each vertex correcpond to one edge
 *the head of the first edge of the returned plygon must be the head of the first
 *edge of the origianl polygon. This is crucial for the code in sampleMonoPoly function
 */
 directedLine*  polygonConvert(directedLine* polygon)
{
  int i;
  directedLine* ret;
  sampledLine* sline;
  sline = new sampledLine(2);
  sline->setPoint(0, polygon->getVertex(0));
  sline->setPoint(1, polygon->getVertex(1));
  ret=new directedLine(INCREASING, sline);
  for(i=1; i<= polygon->get_npoints()-2; i++)
    {
      sline = new sampledLine(2);
      sline->setPoint(0, polygon->getVertex(i));
      sline->setPoint(1, polygon->getVertex(i+1));
      ret->insert(new directedLine(INCREASING, sline));
    }

  for(directedLine *temp = polygon->getNext(); temp != polygon; temp = temp->getNext())
    {
      for(i=0; i<= temp->get_npoints()-2; i++)
	{
	  sline = new sampledLine(2);
	  sline->setPoint(0, temp->getVertex(i));
	  sline->setPoint(1, temp->getVertex(i+1));
	  ret->insert(new directedLine(INCREASING, sline));
	}
    }
  return ret;
}

void triangulateConvexPolyVertical(directedLine* topV, directedLine* botV, primStream *pStream)
{
  Int i,j;
  Int n_leftVerts;
  Int n_rightVerts;
  Real** leftVerts;
  Real** rightVerts;
  directedLine* tempV;
  n_leftVerts = 0;
  for(tempV = topV; tempV != botV; tempV = tempV->getNext())
    {
      n_leftVerts += tempV->get_npoints();
    }
  n_rightVerts=0;
  for(tempV = botV; tempV != topV; tempV = tempV->getNext())
    {
      n_rightVerts += tempV->get_npoints();
    }

  Real2* temp_leftVerts = (Real2 *) malloc(sizeof(Real2) * n_leftVerts);
  assert(temp_leftVerts);
  Real2* temp_rightVerts = (Real2 *) malloc(sizeof(Real2) * n_rightVerts);
  assert(temp_rightVerts);

  leftVerts = (Real**) malloc(sizeof(Real2*) * n_leftVerts);
  assert(leftVerts);
  rightVerts = (Real**) malloc(sizeof(Real2*) * n_rightVerts);
  assert(rightVerts);
  for(i=0; i<n_leftVerts; i++)
    leftVerts[i] = temp_leftVerts[i];
  for(i=0; i<n_rightVerts; i++)
    rightVerts[i] = temp_rightVerts[i];

  i=0;
  for(tempV = topV; tempV != botV; tempV = tempV->getNext())
    {
      for(j=1; j<tempV->get_npoints(); j++)
	{
	  leftVerts[i][0] = tempV->getVertex(j)[0];
	  leftVerts[i][1] = tempV->getVertex(j)[1];
	  i++;
	}
    }
  n_leftVerts = i;
  i=0;
  for(tempV = topV->getPrev(); tempV != botV->getPrev(); tempV = tempV->getPrev())
    {
      for(j=tempV->get_npoints()-1; j>=1; j--)
	{
	  rightVerts[i][0] = tempV->getVertex(j)[0];
	  rightVerts[i][1] = tempV->getVertex(j)[1];
	  i++;
	}
    }
  n_rightVerts = i;
  triangulateXYMonoTB(n_leftVerts, leftVerts, n_rightVerts, rightVerts, pStream);
  free(leftVerts);
  free(rightVerts);
  free(temp_leftVerts);
  free(temp_rightVerts);
}  

void triangulateConvexPolyHoriz(directedLine* leftV, directedLine* rightV, primStream *pStream)
{
  Int i,j;
  Int n_lowerVerts;
  Int n_upperVerts;
  Real2 *lowerVerts;
  Real2 *upperVerts;
  directedLine* tempV;
  n_lowerVerts=0;
  for(tempV = leftV; tempV != rightV; tempV = tempV->getNext())
    {
      n_lowerVerts += tempV->get_npoints();
    }
  n_upperVerts=0;
  for(tempV = rightV; tempV != leftV; tempV = tempV->getNext())
    {
      n_upperVerts += tempV->get_npoints();
    }
  lowerVerts = (Real2 *) malloc(sizeof(Real2) * n_lowerVerts);
  assert(n_lowerVerts);
  upperVerts = (Real2 *) malloc(sizeof(Real2) * n_upperVerts);
  assert(n_upperVerts);
  i=0;
  for(tempV = leftV; tempV != rightV; tempV = tempV->getNext())
    {
      for(j=0; j<tempV->get_npoints(); j++)
	{
	  lowerVerts[i][0] = tempV->getVertex(j)[0];
	  lowerVerts[i][1] = tempV->getVertex(j)[1];
	  i++;
	}
    }
  i=0;
  for(tempV = leftV->getPrev(); tempV != rightV->getPrev(); tempV = tempV->getPrev())
    {
      for(j=tempV->get_npoints()-1; j>=0; j--)
	{
	  upperVerts[i][0] = tempV->getVertex(j)[0];
	  upperVerts[i][1] = tempV->getVertex(j)[1];
	  i++;
	}
    }
  triangulateXYMono(n_upperVerts, upperVerts, n_lowerVerts, lowerVerts, pStream);
  free(lowerVerts);
  free(upperVerts);
}  
void triangulateConvexPoly(directedLine* polygon, Int ulinear, Int vlinear, primStream* pStream)
{
  /*find left, right, top , bot
    */
  directedLine* tempV;
  directedLine* topV;
  directedLine* botV;
  directedLine* leftV;
  directedLine* rightV;
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
  //find leftV
  for(tempV = topV; tempV != botV; tempV = tempV->getNext())
    {
      if(tempV->tail()[0] >= tempV->head()[0])
	break;
    }
  leftV = tempV;
  //find rightV
  for(tempV = botV; tempV != topV; tempV = tempV->getNext())
    {
      if(tempV->tail()[0] <= tempV->head()[0])
	break;
    }
  rightV = tempV;
  if(vlinear)
    {
      triangulateConvexPolyHoriz( leftV, rightV, pStream);
    }
  else if(ulinear)
    {
      triangulateConvexPolyVertical(topV, botV, pStream);
    }
  else
    {
      if(DBG_is_U_direction(polygon))
	{
	  triangulateConvexPolyHoriz( leftV, rightV, pStream);
	}
      else
	triangulateConvexPolyVertical(topV, botV, pStream);
    }
}	      

/*for debug purpose*/
void drawCorners(
		 Real* topV, Real* botV,		 
		 vertexArray* leftChain,
		 vertexArray* rightChain,
		 gridBoundaryChain* leftGridChain,
		 gridBoundaryChain* rightGridChain,
		 Int gridIndex1,
		 Int gridIndex2,
		 Int leftCornerWhere,
		 Int leftCornerIndex,
		 Int rightCornerWhere,
		 Int rightCornerIndex,
		 Int bot_leftCornerWhere,
		 Int bot_leftCornerIndex,
		 Int bot_rightCornerWhere,
		 Int bot_rightCornerIndex)
{
  Real* leftCornerV;
  Real* rightCornerV;
  Real* bot_leftCornerV;
  Real* bot_rightCornerV;

  if(leftCornerWhere == 1)
    leftCornerV = topV;
  else if(leftCornerWhere == 0)
    leftCornerV = leftChain->getVertex(leftCornerIndex);
  else
    leftCornerV = rightChain->getVertex(leftCornerIndex);

  if(rightCornerWhere == 1)
    rightCornerV = topV;
  else if(rightCornerWhere == 0)
    rightCornerV = leftChain->getVertex(rightCornerIndex);
  else
    rightCornerV = rightChain->getVertex(rightCornerIndex);

  if(bot_leftCornerWhere == 1)
    bot_leftCornerV = botV;
  else if(bot_leftCornerWhere == 0)
    bot_leftCornerV = leftChain->getVertex(bot_leftCornerIndex);
  else
    bot_leftCornerV = rightChain->getVertex(bot_leftCornerIndex);

  if(bot_rightCornerWhere == 1)
    bot_rightCornerV = botV;
  else if(bot_rightCornerWhere == 0)
    bot_rightCornerV = leftChain->getVertex(bot_rightCornerIndex);
  else
    bot_rightCornerV = rightChain->getVertex(bot_rightCornerIndex);

  Real topGridV = leftGridChain->get_v_value(gridIndex1);
  Real topGridU1 = leftGridChain->get_u_value(gridIndex1);
  Real topGridU2 = rightGridChain->get_u_value(gridIndex1);
  Real botGridV = leftGridChain->get_v_value(gridIndex2);
  Real botGridU1 = leftGridChain->get_u_value(gridIndex2);
  Real botGridU2 = rightGridChain->get_u_value(gridIndex2);
  
  glBegin(GL_LINE_STRIP);
  glVertex2fv(leftCornerV);
  glVertex2f(topGridU1, topGridV);
  glEnd();

  glBegin(GL_LINE_STRIP);
  glVertex2fv(rightCornerV);
  glVertex2f(topGridU2, topGridV);
  glEnd();

  glBegin(GL_LINE_STRIP);
  glVertex2fv(bot_leftCornerV);
  glVertex2f(botGridU1, botGridV);
  glEnd();

  glBegin(GL_LINE_STRIP);
  glVertex2fv(bot_rightCornerV);
  glVertex2f(botGridU2, botGridV);
  glEnd();


}
		 
void toVertexArrays(directedLine* topV, directedLine* botV, vertexArray& leftChain, vertexArray& rightChain)
{  
  Int i;
  directedLine* tempV;
  for(i=1; i<=topV->get_npoints()-2; i++) { /*the first vertex is the top vertex which doesn't belong to inc_chain*/
    leftChain.appendVertex(topV->getVertex(i));
  }
  for(tempV = topV->getNext(); tempV != botV; tempV = tempV->getNext())
    {
      for(i=0; i<=tempV->get_npoints()-2; i++){
	leftChain.appendVertex(tempV->getVertex(i));
      }
    }  

  for(tempV = topV->getPrev(); tempV != botV; tempV = tempV->getPrev())
    {
      for(i=tempV->get_npoints()-2; i>=0; i--){
	rightChain.appendVertex(tempV->getVertex(i));
      }
    }
  for(i=botV->get_npoints()-2; i>=1; i--){ 
    rightChain.appendVertex(tempV->getVertex(i));
  }
}


void findTopAndBot(directedLine* polygon, directedLine*& topV, directedLine*& botV)
{
  assert(polygon);
  directedLine* tempV;
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
}
   
void findGridChains(directedLine* topV, directedLine* botV, 
		    gridWrap* grid,
		    gridBoundaryChain*& leftGridChain,
		    gridBoundaryChain*& rightGridChain)
{
  /*find the first(top) and the last (bottom) grid line which intersect the
   *this polygon
   */
  Int firstGridIndex; /*the index in the grid*/
  Int lastGridIndex;

  firstGridIndex = (Int) ((topV->head()[1] - grid->get_v_min()) / (grid->get_v_max() - grid->get_v_min()) * (grid->get_n_vlines()-1));

  if(botV->head()[1] < grid->get_v_min())
    lastGridIndex = 0;
  else
    lastGridIndex  = (Int) ((botV->head()[1] - grid->get_v_min()) / (grid->get_v_max() - grid->get_v_min()) * (grid->get_n_vlines()-1)) + 1;

  /*find the interval inside  the polygon for each gridline*/
  Int *leftGridIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(leftGridIndices);
  Int *rightGridIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(rightGridIndices);
  Int *leftGridInnerIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(leftGridInnerIndices);
  Int *rightGridInnerIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(rightGridInnerIndices);

  findLeftGridIndices(topV, firstGridIndex, lastGridIndex, grid,  leftGridIndices, leftGridInnerIndices);

  findRightGridIndices(topV, firstGridIndex, lastGridIndex, grid,  rightGridIndices, rightGridInnerIndices);

  leftGridChain =  new gridBoundaryChain(grid, firstGridIndex, firstGridIndex-lastGridIndex+1, leftGridIndices, leftGridInnerIndices);

  rightGridChain = new gridBoundaryChain(grid, firstGridIndex, firstGridIndex-lastGridIndex+1, rightGridIndices, rightGridInnerIndices);

  free(leftGridIndices);
  free(rightGridIndices);
  free(leftGridInnerIndices);
  free(rightGridInnerIndices);
}

void findDownCorners(Real *botVertex, 
		   vertexArray *leftChain, Int leftChainStartIndex, Int leftChainEndIndex,
		   vertexArray *rightChain, Int rightChainStartIndex, Int rightChainEndIndex,
		   Real v,
		   Real uleft,
		   Real uright,
		   Int& ret_leftCornerWhere, /*0: left chain, 1: topvertex, 2: rightchain*/
		   Int& ret_leftCornerIndex, /*useful when ret_leftCornerWhere == 0 or 2*/
		   Int& ret_rightCornerWhere, /*0: left chain, 1: topvertex, 2: rightchain*/
		   Int& ret_rightCornerIndex /*useful when ret_leftCornerWhere == 0 or 2*/
		   )
{
#ifdef MYDEBUG
printf("*************enter find donw corner\n");
printf("finddownCorner: v=%f, uleft=%f, uright=%f\n", v, uleft, uright);
printf("(%i,%i,%i,%i)\n", leftChainStartIndex, leftChainEndIndex,rightChainStartIndex, rightChainEndIndex); 
printf("left chain is\n");
leftChain->print();
printf("right chain is\n");
rightChain->print();
#endif

  assert(v > botVertex[1]);
  Real leftGridPoint[2];
  leftGridPoint[0] = uleft;
  leftGridPoint[1] = v;
  Real rightGridPoint[2];
  rightGridPoint[0] = uright;
  rightGridPoint[1] = v;

  Int i;
  Int index1, index2;

  index1 = leftChain->findIndexBelowGen(v, leftChainStartIndex, leftChainEndIndex);
  index2 = rightChain->findIndexBelowGen(v, rightChainStartIndex, rightChainEndIndex);

  if(index2 <= rightChainEndIndex) //index2 was found above
    index2 = rightChain->skipEqualityFromStart(v, index2, rightChainEndIndex);

  if(index1>leftChainEndIndex && index2 > rightChainEndIndex) /*no point below v on left chain or right chain*/
    {

      /*the botVertex is the only vertex below v*/
      ret_leftCornerWhere = 1;
      ret_rightCornerWhere = 1;
    }
  else if(index1>leftChainEndIndex ) /*index2 <= rightChainEndIndex*/
    {

      ret_rightCornerWhere = 2; /*on right chain*/
      ret_rightCornerIndex = index2;


      Real tempMin = rightChain->getVertex(index2)[0];
      Int tempI = index2;
      for(i=index2+1; i<= rightChainEndIndex; i++)
	if(rightChain->getVertex(i)[0] < tempMin)
	  {
	    tempI = i;
	    tempMin = rightChain->getVertex(i)[0];
	  }


      //we consider whether we can use botVertex as left corner. First check 
      //if (leftGirdPoint, botVertex) interesects right chian or not.
     if(DBG_intersectChain(rightChain, rightChainStartIndex,rightChainEndIndex,
				    leftGridPoint, botVertex))
       {
	 ret_leftCornerWhere = 2;//right
	 ret_leftCornerIndex = index2; //should use tempI???
       }
     else if(botVertex[0] < tempMin)
       ret_leftCornerWhere = 1; //bot
     else
       {
	 ret_leftCornerWhere = 2; //right
	 ret_leftCornerIndex = tempI;
       }
    }
  else if(index2> rightChainEndIndex) /*index1<=leftChainEndIndex*/
    {
      ret_leftCornerWhere = 0; /*left chain*/
      ret_leftCornerIndex = index1;
      
      /*find the vertex on the left chain with the maximum u,
       *either this vertex or the botvertex can be used as the right corner
       */

      Int tempI;
      //skip those points which are equal to v. (avoid degeneratcy)
      for(tempI = index1; tempI <= leftChainEndIndex; tempI++)
	if(leftChain->getVertex(tempI)[1] < v) 
	  break;
      if(tempI > leftChainEndIndex)
	ret_rightCornerWhere = 1;
      else
	{
	  Real tempMax = leftChain->getVertex(tempI)[0];
	  for(i=tempI; i<= leftChainEndIndex; i++)
	    if(leftChain->getVertex(i)[0] > tempMax)
	      {
		tempI = i;
		tempMax = leftChain->getVertex(i)[0];
	      }



	  //we consider whether we can use botVertex as a corner. So first we check 
	  //whether (rightGridPoint, botVertex) interescts the left chain or not.
	  if(DBG_intersectChain(leftChain, leftChainStartIndex,leftChainEndIndex,
				    rightGridPoint, botVertex))
	    {
	      ret_rightCornerWhere = 0;
	      ret_rightCornerIndex = index1; //should use tempI???
	    }
	  else if(botVertex[0] > tempMax)
	    {
		      
              ret_rightCornerWhere = 1;
	    }
	  else
	    {
	      ret_rightCornerWhere = 0;
	      ret_rightCornerIndex = tempI;
	    }
	}
      
    }
  else /*index1<=leftChainEndIndex and index2 <=rightChainEndIndex*/
    {
      if(leftChain->getVertex(index1)[1] >= rightChain->getVertex(index2)[1]) /*left point above right point*/
	{
	  ret_leftCornerWhere = 0; /*on left chain*/
	  ret_leftCornerIndex = index1;

	  Real tempMax;
	  Int tempI;

	  tempI = index1;
	  tempMax = leftChain->getVertex(index1)[0];

	  /*find the maximum u for all the points on the left above the right point index2*/
	  for(i=index1+1; i<= leftChainEndIndex; i++)
	    {
	      if(leftChain->getVertex(i)[1] < rightChain->getVertex(index2)[1])
		break;

	      if(leftChain->getVertex(i)[0]>tempMax)
		{
		  tempI = i;
		  tempMax = leftChain->getVertex(i)[0];
		}
	    }
	  //we consider if we can use rightChain(index2) as right corner
	  //we check if (rightChain(index2), rightGidPoint) intersecs left chain or not.
	  if(DBG_intersectChain(leftChain, leftChainStartIndex,leftChainEndIndex, rightGridPoint, rightChain->getVertex(index2)))
	    {
	      ret_rightCornerWhere = 0;
	      ret_rightCornerIndex = index1; //should use tempI???
	    }
	  else if(tempMax >= rightChain->getVertex(index2)[0] ||
	     tempMax >= uright
	     )
	    {

	      ret_rightCornerWhere = 0; /*on left Chain*/
	      ret_rightCornerIndex = tempI;
	    }
	  else
	    {
	      ret_rightCornerWhere = 2; /*on right chain*/
	      ret_rightCornerIndex = index2;
	    }
	}
      else /*left below right*/
	{
	  ret_rightCornerWhere = 2; /*on the right*/
	  ret_rightCornerIndex = index2;
	  
	  Real tempMin;
	  Int tempI;
	  
	  tempI = index2;
	  tempMin = rightChain->getVertex(index2)[0];
	  
	  /*find the minimum u for all the points on the right above the left poitn index1*/
	  for(i=index2+1; i<= rightChainEndIndex; i++)
	    {
	      if( rightChain->getVertex(i)[1] < leftChain->getVertex(index1)[1])
		break;
	      if(rightChain->getVertex(i)[0] < tempMin)
		{
		  tempI = i;
		  tempMin = rightChain->getVertex(i)[0];
		}
	    }

	  //we consider if we can use leftchain(index1) as left corner. 
	  //we check if (leftChain(index1) intersects right chian or not
	  if(DBG_intersectChain(rightChain, rightChainStartIndex, rightChainEndIndex, leftGridPoint, leftChain->getVertex(index1)))
	    {
	      ret_leftCornerWhere = 2;
	      ret_leftCornerIndex = index2; //should use tempI???
	      }
	  else if(tempMin <= leftChain->getVertex(index1)[0] ||
	     tempMin <= uleft)				
	    {
	      ret_leftCornerWhere = 2; /* on right chain*/
	      ret_leftCornerIndex = tempI;
	    }
	  else
	    {
	      ret_leftCornerWhere = 0; /*on left chain*/
	      ret_leftCornerIndex = index1;
	    }
	}
    }

}


void findUpCorners(Real *topVertex, 
		   vertexArray *leftChain, Int leftChainStartIndex, Int leftChainEndIndex,
		   vertexArray *rightChain, Int rightChainStartIndex, Int rightChainEndIndex,
		   Real v,
		   Real uleft,
		   Real uright,
		   Int& ret_leftCornerWhere, /*0: left chain, 1: topvertex, 2: rightchain*/
		   Int& ret_leftCornerIndex, /*useful when ret_leftCornerWhere == 0 or 2*/
		   Int& ret_rightCornerWhere, /*0: left chain, 1: topvertex, 2: rightchain*/
		   Int& ret_rightCornerIndex /*useful when ret_leftCornerWhere == 0 or 2*/
		   )
{
#ifdef MYDEBUG
printf("***********enter findUpCorners\n");
#endif

  assert(v < topVertex[1]);
  Real leftGridPoint[2];
  leftGridPoint[0] = uleft;
  leftGridPoint[1] = v;
  Real rightGridPoint[2];
  rightGridPoint[0] = uright;
  rightGridPoint[1] = v;

  Int i;
  Int index1, index2;

  index1 = leftChain->findIndexFirstAboveEqualGen(v, leftChainStartIndex, leftChainEndIndex);


  index2 = rightChain->findIndexFirstAboveEqualGen(v, rightChainStartIndex, rightChainEndIndex);

  if(index2>= leftChainStartIndex)  //index2 was found above  
    index2 = rightChain->skipEqualityFromStart(v, index2, rightChainEndIndex);

  if(index1<leftChainStartIndex && index2 <rightChainStartIndex) /*no point above v on left chain or right chain*/
    {
      /*the topVertex is the only vertex above v*/
      ret_leftCornerWhere = 1;
      ret_rightCornerWhere = 1;
    }
  else if(index1<leftChainStartIndex ) /*index2 >= rightChainStartIndex*/
    {
      ret_rightCornerWhere = 2; /*on right chain*/
      ret_rightCornerIndex = index2;

      //find the minimum u on right top, either that, or top, or right[index2] is the left corner
      Real tempMin = rightChain->getVertex(index2)[0];
      Int tempI = index2;
      for(i=index2-1; i>=rightChainStartIndex; i--)
	if(rightChain->getVertex(i)[0] < tempMin)
	  {
	    tempMin = rightChain->getVertex(i)[0];
	    tempI = i;
	  }
      //chech whether (leftGridPoint, top) intersects rightchai,
      //if yes, use right corner as left corner
      //if not, use top or right[tempI] as left corner
      if(DBG_intersectChain(rightChain, rightChainStartIndex, rightChainEndIndex,
			leftGridPoint, topVertex))
	{
	  ret_leftCornerWhere = 2; //rightChain
	  ret_leftCornerIndex = index2; 
	}
      else if(topVertex[0] < tempMin)
	ret_leftCornerWhere = 1; /*topvertex*/
      else
	{
	  ret_leftCornerWhere = 2; //right chain
	  ret_leftCornerIndex = tempI;
	}
	      
    }
  else if(index2< rightChainStartIndex) /*index1>=leftChainStartIndex*/
    {
      ret_leftCornerWhere = 0; /*left chain*/
      ret_leftCornerIndex = index1;
       
      //find the maximum u on the left top section. either that or topvertex, or left[index1]  is the right corner
      Real tempMax = leftChain->getVertex(index1)[0];
      Int tempI = index1;

      for(i=index1-1; i>=leftChainStartIndex; i--){

	if(leftChain->getVertex(i)[0] > tempMax)
	  {

	    tempMax = leftChain->getVertex(i)[0];
	    tempI = i;
	  }
      }
      //check whether (rightGridPoint, top) intersects leftChain or not
      //if yes, we use leftCorner as the right corner
      //if not, we use either top or left[tempI] as the right corner
      if(DBG_intersectChain(leftChain, leftChainStartIndex,leftChainEndIndex,
			    rightGridPoint, topVertex))
	 {
	   ret_rightCornerWhere = 0; //left chan
	   ret_rightCornerIndex = index1;
	 }
      else if(topVertex[0] > tempMax)
	ret_rightCornerWhere = 1;//topVertex
      else
	{
	  ret_rightCornerWhere = 0;//left chain
	  ret_rightCornerIndex = tempI;
	}
    }
  else /*index1>=leftChainStartIndex and index2 >=rightChainStartIndex*/
    {
      if(leftChain->getVertex(index1)[1] <= rightChain->getVertex(index2)[1]) /*left point below right point*/
	{
	  ret_leftCornerWhere = 0; /*on left chain*/
	  ret_leftCornerIndex = index1;

	  Real tempMax;
	  Int tempI;

	  tempI = index1;
	  tempMax = leftChain->getVertex(index1)[0];

	  /*find the maximum u for all the points on the left below the right point index2*/
	  for(i=index1-1; i>= leftChainStartIndex; i--)
	    {
	      if(leftChain->getVertex(i)[1] > rightChain->getVertex(index2)[1])
		break;

	      if(leftChain->getVertex(i)[0]>tempMax)
		{
		  tempI = i;
		  tempMax = leftChain->getVertex(i)[0];
		}
	    }
	  //chek whether (rightChain(index2), rightGridPoint) intersects leftchian or not
	  if(DBG_intersectChain(leftChain, leftChainStartIndex, leftChainEndIndex, rightGridPoint, rightChain->getVertex(index2)))
	     {
	       ret_rightCornerWhere = 0;
	       ret_rightCornerIndex = index1;
	     }
	  else if(tempMax >= rightChain->getVertex(index2)[0] ||
	     tempMax >= uright)
	    {
	      ret_rightCornerWhere = 0; /*on left Chain*/
	      ret_rightCornerIndex = tempI;
	    }
	  else
	    {
	      ret_rightCornerWhere = 2; /*on right chain*/
	      ret_rightCornerIndex = index2;
	    }
	}
      else /*left above right*/
	{
	  ret_rightCornerWhere = 2; /*on the right*/
	  ret_rightCornerIndex = index2;
	  
	  Real tempMin;
	  Int tempI;
	  
	  tempI = index2;
	  tempMin = rightChain->getVertex(index2)[0];
	  
	  /*find the minimum u for all the points on the right below the left poitn index1*/
	  for(i=index2-1; i>= rightChainStartIndex; i--)
	    {
	      if( rightChain->getVertex(i)[1] > leftChain->getVertex(index1)[1])
		break;
	      if(rightChain->getVertex(i)[0] < tempMin)
		{
		  tempI = i;
		  tempMin = rightChain->getVertex(i)[0];
		}
	    }
          //check whether (leftGRidPoint,left(index1)) interesect right chain 
	  if(DBG_intersectChain(rightChain, rightChainStartIndex, rightChainEndIndex,
				leftGridPoint, leftChain->getVertex(index1)))
	    {
	      ret_leftCornerWhere = 2; //right
	      ret_leftCornerIndex = index2;
	    }
	  else if(tempMin <= leftChain->getVertex(index1)[0] ||
	     tempMin <= uleft)
	    {
	      ret_leftCornerWhere = 2; /* on right chain*/
	      ret_leftCornerIndex = tempI;
	    }
	  else
	    {
	      ret_leftCornerWhere = 0; /*on left chain*/
	      ret_leftCornerIndex = index1;
	    }
	}
    }
#ifdef MYDEBUG
printf("***********leave findUpCorners\n");
#endif
}

//return 1 if neck exists, 0 othewise
Int findNeckF(vertexArray *leftChain, Int botLeftIndex,
	      vertexArray *rightChain, Int botRightIndex,
	      gridBoundaryChain* leftGridChain,
	      gridBoundaryChain* rightGridChain,
	      Int gridStartIndex,
	      Int& neckLeft, 
	      Int& neckRight)
{
/*
printf("enter findNeckF, botleft, botright=%i,%i,gstartindex=%i\n",botLeftIndex,botRightIndex,gridStartIndex);
printf("leftChain is\n");
leftChain->print();
printf("rightChain is\n");
rightChain->print();
*/

  Int lowerGridIndex; //the grid below leftChain and rightChian vertices
  Int i;
  Int n_vlines = leftGridChain->get_nVlines();
  Real v;
  if(botLeftIndex >= leftChain->getNumElements() ||
     botRightIndex >= rightChain->getNumElements())
    return 0; //no neck exists
     
  v=min(leftChain->getVertex(botLeftIndex)[1], rightChain->getVertex(botRightIndex)[1]);  

 


  for(i=gridStartIndex; i<n_vlines; i++)
    if(leftGridChain->get_v_value(i) <= v && 
       leftGridChain->getUlineIndex(i)<= rightGridChain->getUlineIndex(i))
      break;
  
  lowerGridIndex = i;

  if(lowerGridIndex == n_vlines) //the two trm vertex are higher than all gridlines
    return 0;
  else 
    {
      Int botLeft2, botRight2;
/*
printf("leftGridChain->get_v_)value=%f\n",leftGridChain->get_v_value(lowerGridIndex), botLeftIndex); 
printf("leftChain->get_vertex(0)=(%f,%f)\n", leftChain->getVertex(0)[0],leftChain->getVertex(0)[1]);
printf("leftChain->get_vertex(1)=(%f,%f)\n", leftChain->getVertex(1)[0],leftChain->getVertex(1)[1]);
printf("leftChain->get_vertex(2)=(%f,%f)\n", leftChain->getVertex(2)[0],leftChain->getVertex(2)[1]);
*/
      botLeft2 = leftChain->findIndexFirstAboveEqualGen(leftGridChain->get_v_value(lowerGridIndex), botLeftIndex, leftChain->getNumElements()-1) -1 ;

/*
printf("botLeft2=%i\n", botLeft2);
printf("leftChain->getNumElements=%i\n", leftChain->getNumElements());
*/

      botRight2 = rightChain->findIndexFirstAboveEqualGen(leftGridChain->get_v_value(lowerGridIndex), botRightIndex, rightChain->getNumElements()-1) -1;
      if(botRight2 < botRightIndex) botRight2=botRightIndex;

      if(botLeft2 < botLeftIndex) botLeft2 = botLeftIndex;

      assert(botLeft2 >= botLeftIndex);
      assert(botRight2 >= botRightIndex);
      //find nectLeft so that it is th erightmost vertex on letChain

      Int tempI = botLeftIndex;
      Real temp = leftChain->getVertex(tempI)[0];
      for(i=botLeftIndex+1; i<= botLeft2; i++)
	if(leftChain->getVertex(i)[0] > temp)
	  {
	    temp = leftChain->getVertex(i)[0];
	    tempI = i;
	  }
      neckLeft = tempI;

      tempI = botRightIndex;
      temp = rightChain->getVertex(tempI)[0];
      for(i=botRightIndex+1; i<= botRight2; i++)
	if(rightChain->getVertex(i)[0] < temp)
	  {
	    temp = rightChain->getVertex(i)[0];
	    tempI = i;
	  }
      neckRight = tempI;
      return 1;
    }
}
							
  
	          
/*find i>=botLeftIndex,j>=botRightIndex so that
 *(leftChain[i], rightChain[j]) is a neck.
 */
void findNeck(vertexArray *leftChain, Int botLeftIndex, 
	      vertexArray *rightChain, Int botRightIndex, 
	      Int& leftLastIndex, /*left point of the neck*/
	      Int& rightLastIndex /*right point of the neck*/
	      )
{
  assert(botLeftIndex < leftChain->getNumElements() &&
     botRightIndex < rightChain->getNumElements());
     
  /*now the neck exists for sure*/

  if(leftChain->getVertex(botLeftIndex)[1] <= rightChain->getVertex(botRightIndex)[1]) //left below right
    {

      leftLastIndex = botLeftIndex;
      
      /*find i so that rightChain[i][1] >= leftchainbotverte[1], and i+1<
       */
      rightLastIndex=rightChain->findIndexAboveGen(leftChain->getVertex(botLeftIndex)[1], botRightIndex+1, rightChain->getNumElements()-1);    
    }
  else  //left above right
    {

      rightLastIndex = botRightIndex;
     
      leftLastIndex = leftChain->findIndexAboveGen(rightChain->getVertex(botRightIndex)[1], 
						  botLeftIndex+1,
						  leftChain->getNumElements()-1);
    }
}
      
      

void findLeftGridIndices(directedLine* topEdge, Int firstGridIndex, Int lastGridIndex, gridWrap* grid,  Int* ret_indices, Int* ret_innerIndices)
{

  Int i,k,isHoriz = 0;
  Int n_ulines = grid->get_n_ulines();
  Real uMin = grid->get_u_min();
  Real uMax = grid->get_u_max();
  /*
  Real vMin = grid->get_v_min();
  Real vMax = grid->get_v_max();
  */
  Real slop = 0.0, uinterc;

#ifdef SHORTEN_GRID_LINE
  //uintercBuf stores all the interction u value for each grid line
  //notice that lastGridIndex<= firstGridIndex
  Real *uintercBuf = (Real *) malloc (sizeof(Real) * (firstGridIndex-lastGridIndex+1));
  assert(uintercBuf);
#endif

  /*initialization to make vtail bigger than grid->...*/
  directedLine* dLine = topEdge;
  Real vtail = grid->get_v_value(firstGridIndex) + 1.0; 
  Real tempMaxU = grid->get_u_min();


  /*for each grid line*/
  for(k=0, i=firstGridIndex; i>=lastGridIndex; i--, k++)
    {

      Real grid_v_value = grid->get_v_value(i);

      /*check whether this grid line is below the current trim edge.*/
      if(vtail > grid_v_value)
	{
	  /*since the grid line is below the trim edge, we 
	   *find the trim edge which will contain the trim line
	   */
	  while( (vtail=dLine->tail()[1]) > grid_v_value){

	    tempMaxU = max(tempMaxU, dLine->tail()[0]);
	    dLine = dLine -> getNext();
	  }

	  if( fabs(dLine->head()[1] - vtail) < ZERO)
	    isHoriz = 1;
	  else
	    {
	      isHoriz = 0;
	      slop = (dLine->head()[0] - dLine->tail()[0]) / (dLine->head()[1]-vtail);
	    }
	}

      if(isHoriz)
	{
	  uinterc = max(dLine->head()[0], dLine->tail()[0]);
	}
      else
	{
	  uinterc = slop * (grid_v_value - vtail) + dLine->tail()[0];
	}
      
      tempMaxU = max(tempMaxU, uinterc);

      if(uinterc < uMin && uinterc >= uMin - ZERO)
	uinterc = uMin;
      if(uinterc > uMax && uinterc <= uMax + ZERO)
	uinterc = uMax;

#ifdef SHORTEN_GRID_LINE
      uintercBuf[k] = uinterc;
#endif

      assert(uinterc >= uMin && uinterc <= uMax);
       if(uinterc == uMax)
         ret_indices[k] = n_ulines-1;
       else
	 ret_indices[k] = (Int)(((uinterc-uMin)/(uMax - uMin)) * (n_ulines-1)) + 1;
      if(ret_indices[k] >= n_ulines)
	ret_indices[k] = n_ulines-1;


      ret_innerIndices[k] = (Int)(((tempMaxU-uMin)/(uMax - uMin)) * (n_ulines-1)) + 1;

      /*reinitialize tempMaxU for next grdiLine*/
      tempMaxU = uinterc;
    }
#ifdef SHORTEN_GRID_LINE
  //for each grid line, compare the left grid point with the 
  //intersection point. If the two points are too close, then
  //we should move the grid point one grid to the right
  //and accordingly we should update the inner index.
  for(k=0, i=firstGridIndex; i>=lastGridIndex; i--, k++)
    {
      //check gridLine i
      //check ret_indices[k]
      Real a = grid->get_u_value(ret_indices[k]-1);
      Real b = grid->get_u_value(ret_indices[k]);
      assert(uintercBuf[k] >= a && uintercBuf < b);
      if( (b-uintercBuf[k]) <= 0.2 * (b-a)) //interc is very close to b
	{
	  ret_indices[k]++;
	}

      //check ret_innerIndices[k]
      if(k>0)
	{
	  if(ret_innerIndices[k] < ret_indices[k-1])
	    ret_innerIndices[k] = ret_indices[k-1];
	  if(ret_innerIndices[k] < ret_indices[k])
	    ret_innerIndices[k] = ret_indices[k];
	}
    }
  //clean up
  free(uintercBuf);
#endif
}

void findRightGridIndices(directedLine* topEdge, Int firstGridIndex, Int lastGridIndex, gridWrap* grid,  Int* ret_indices, Int* ret_innerIndices)
{

  Int i,k;
  Int n_ulines = grid->get_n_ulines();
  Real uMin = grid->get_u_min();
  Real uMax = grid->get_u_max();
  /*
  Real vMin = grid->get_v_min();
  Real vMax = grid->get_v_max();
  */
  Real slop = 0.0, uinterc;

#ifdef SHORTEN_GRID_LINE
  //uintercBuf stores all the interction u value for each grid line
  //notice that firstGridIndex >= lastGridIndex
  Real *uintercBuf = (Real *) malloc (sizeof(Real) * (firstGridIndex-lastGridIndex+1));
  assert(uintercBuf);
#endif

  /*initialization to make vhead bigger than grid->v_value...*/
  directedLine* dLine = topEdge->getPrev();
  Real vhead = dLine->tail()[1];
  Real tempMinU = grid->get_u_max();

  /*for each grid line*/
  for(k=0, i=firstGridIndex; i>=lastGridIndex; i--, k++)
    {

      Real grid_v_value = grid->get_v_value(i);


      /*check whether this grid line is below the current trim edge.*/
      if(vhead >= grid_v_value)
	{
	  /*since the grid line is below the tail of the trim edge, we 
	   *find the trim edge which will contain the trim line
	   */
	  while( (vhead=dLine->head()[1]) > grid_v_value){
	    tempMinU = min(tempMinU, dLine->head()[0]);
	    dLine = dLine -> getPrev();
	  }

	  /*skip the equality in the case of degenerat case: horizontal */
	  while(dLine->head()[1] == grid_v_value)
	    dLine = dLine->getPrev();
	    
	  assert( dLine->tail()[1] != dLine->head()[1]);
	  slop = (dLine->tail()[0] - dLine->head()[0]) / (dLine->tail()[1]-dLine->head()[1]);
	  /*
	   if(dLine->tail()[1] == vhead)
	     isHoriz = 1;
	     else
	    {
	      isHoriz = 0;
	      slop = (dLine->tail()[0] - dLine->head()[0]) / (dLine->tail()[1]-vhead);
	    }
	    */
	}
	uinterc = slop * (grid_v_value - dLine->head()[1]) + dLine->head()[0];

      //in case unterc is outside of the grid due to floating point
      if(uinterc < uMin)
	uinterc = uMin;
      else if(uinterc > uMax)
	uinterc = uMax;

#ifdef SHORTEN_GRID_LINE
      uintercBuf[k] = uinterc;
#endif      

      tempMinU = min(tempMinU, uinterc);

      assert(uinterc >= uMin && uinterc <= uMax);      

      if(uinterc == uMin)
	ret_indices[k] = 0;
      else
	ret_indices[k] = (int)ceil((((uinterc-uMin)/(uMax - uMin)) * (n_ulines-1))) -1;
/*
if(ret_indices[k] >= grid->get_n_ulines())
  {
  printf("ERROR3\n");
  exit(0);
}
if(ret_indices[k] < 0)    
  {
  printf("ERROR4\n");
  exit(0);
}
*/
      ret_innerIndices[k] = (int)ceil ((((tempMinU-uMin)/(uMax - uMin)) * (n_ulines-1))) -1;

      tempMinU = uinterc;
    }
#ifdef SHORTEN_GRID_LINE
  //for each grid line, compare the left grid point with the 
  //intersection point. If the two points are too close, then
  //we should move the grid point one grid to the right
  //and accordingly we should update the inner index.
  for(k=0, i=firstGridIndex; i>=lastGridIndex; i--, k++)
    {
      //check gridLine i
      //check ret_indices[k]
      Real a = grid->get_u_value(ret_indices[k]);
      Real b = grid->get_u_value(ret_indices[k]+1);
      assert(uintercBuf[k] > a && uintercBuf <= b);
      if( (uintercBuf[k]-a) <= 0.2 * (b-a)) //interc is very close to a
	{
	  ret_indices[k]--;
	}

      //check ret_innerIndices[k]
      if(k>0)
	{
	  if(ret_innerIndices[k] > ret_indices[k-1])
	    ret_innerIndices[k] = ret_indices[k-1];
	  if(ret_innerIndices[k] > ret_indices[k])
	    ret_innerIndices[k] = ret_indices[k];
	}
    }
  //clean up
  free(uintercBuf);
#endif
}


void sampleMonoPoly(directedLine* polygon, gridWrap* grid, Int ulinear, Int vlinear, primStream* pStream, rectBlockArray* rbArray)
{
/*
{
grid->print();
polygon->writeAllPolygons("zloutputFile");
exit(0);
}
*/

if(grid->get_n_ulines() == 2 ||
   grid->get_n_vlines() == 2)
{ 
  if(ulinear && grid->get_n_ulines() == 2)
    {
      monoTriangulationFun(polygon, compV2InY, pStream);   
      return;
    }
  else if(DBG_isConvex(polygon) && polygon->numEdges() >=4)
     {
       triangulateConvexPoly(polygon, ulinear, vlinear, pStream);
       return;
     }
  else if(vlinear || DBG_is_U_direction(polygon))
    {
      Int n_cusps;//num interior cusps
      Int n_edges = polygon->numEdges();
      directedLine** cusps = (directedLine**) malloc(sizeof(directedLine*) * n_edges);
      assert(cusps);
      findInteriorCuspsX(polygon, n_cusps, cusps);

      if(n_cusps == 0) //u_monotone
	{

	  monoTriangulationFun(polygon, compV2InX, pStream);

          free(cusps);
          return;          
	}
      else if(n_cusps == 1) //one interior cusp
	{

	  directedLine* new_polygon = polygonConvert(cusps[0]);

	  directedLine* other = findDiagonal_singleCuspX( new_polygon);



	  //<other> should NOT be null unless there are self-intersecting
          //trim curves. In that case, we don't want to core dump, instead,
          //we triangulate anyway, and print out error message.
	  if(other == NULL)
	    {
	      monoTriangulationFun(polygon, compV2InX, pStream);
	      free(cusps);
	      return;
	    }

	  directedLine* ret_p1;
	  directedLine* ret_p2;

	  new_polygon->connectDiagonal_2slines(new_polygon, other, 
					   &ret_p1,
					   &ret_p2,
					   new_polygon);

	  monoTriangulationFun(ret_p1, compV2InX, pStream);
	  monoTriangulationFun(ret_p2, compV2InX, pStream);

	  ret_p1->deleteSinglePolygonWithSline();	      
	  ret_p2->deleteSinglePolygonWithSline();	  

          free(cusps);
          return;
         }
     free(cusps);
     }
}

  /*find the top and bottom of the polygon. It is supposed to be
   *a V-monotone polygon
   */

  directedLine* tempV;
  directedLine* topV;
  directedLine* botV;
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
  
  /*find the first(top) and the last (bottom) grid line which intersect the
   *this polygon
   */
  Int firstGridIndex; /*the index in the grid*/
  Int lastGridIndex;
  firstGridIndex = (Int) ((topV->head()[1] - grid->get_v_min()) / (grid->get_v_max() - grid->get_v_min()) * (grid->get_n_vlines()-1));
  lastGridIndex  = (Int) ((botV->head()[1] - grid->get_v_min()) / (grid->get_v_max() - grid->get_v_min()) * (grid->get_n_vlines()-1)) + 1;


  /*find the interval inside  the polygon for each gridline*/
  Int *leftGridIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(leftGridIndices);
  Int *rightGridIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(rightGridIndices);
  Int *leftGridInnerIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(leftGridInnerIndices);
  Int *rightGridInnerIndices = (Int*) malloc(sizeof(Int) * (firstGridIndex - lastGridIndex +1));
  assert(rightGridInnerIndices);

  findLeftGridIndices(topV, firstGridIndex, lastGridIndex, grid,  leftGridIndices, leftGridInnerIndices);

  findRightGridIndices(topV, firstGridIndex, lastGridIndex, grid,  rightGridIndices, rightGridInnerIndices);

  gridBoundaryChain leftGridChain(grid, firstGridIndex, firstGridIndex-lastGridIndex+1, leftGridIndices, leftGridInnerIndices);

  gridBoundaryChain rightGridChain(grid, firstGridIndex, firstGridIndex-lastGridIndex+1, rightGridIndices, rightGridInnerIndices);



//  leftGridChain.draw();
//  leftGridChain.drawInner();
//  rightGridChain.draw();
//  rightGridChain.drawInner();
  /*(1) determine the grid boundaries (left and right).
   *(2) process polygon  into two monotone chaines: use vertexArray
   *(3) call sampleMonoPolyRec
   */

  /*copy the two chains into vertexArray datastructure*/
  Int i;
  vertexArray leftChain(20); /*this is a dynamic array*/
  for(i=1; i<=topV->get_npoints()-2; i++) { /*the first vertex is the top vertex which doesn't belong to inc_chain*/
    leftChain.appendVertex(topV->getVertex(i));
  }
  for(tempV = topV->getNext(); tempV != botV; tempV = tempV->getNext())
    {
      for(i=0; i<=tempV->get_npoints()-2; i++){
	leftChain.appendVertex(tempV->getVertex(i));
      }
    }
  
  vertexArray rightChain(20);
  for(tempV = topV->getPrev(); tempV != botV; tempV = tempV->getPrev())
    {
      for(i=tempV->get_npoints()-2; i>=0; i--){
	rightChain.appendVertex(tempV->getVertex(i));
      }
    }
  for(i=botV->get_npoints()-2; i>=1; i--){ 
    rightChain.appendVertex(tempV->getVertex(i));
  }

  sampleMonoPolyRec(topV->head(),
		    botV->head(),
		    &leftChain,
		    0,
		    &rightChain,
		    0,
		    &leftGridChain,
		    &rightGridChain,
		    0,
		    pStream,
		    rbArray);


  /*cleanup space*/
  free(leftGridIndices);
  free(rightGridIndices);
  free(leftGridInnerIndices);
  free(rightGridInnerIndices);
}

void sampleMonoPolyRec(
		       Real* topVertex,
		       Real* botVertex,
		       vertexArray* leftChain, 
		       Int leftStartIndex,
		       vertexArray* rightChain,
		       Int rightStartIndex,
		       gridBoundaryChain* leftGridChain, 
		       gridBoundaryChain* rightGridChain, 
		       Int gridStartIndex,
		       primStream* pStream,
		       rectBlockArray* rbArray)
{

  /*find the first connected component, and the four corners.
   */
  Int index1, index2; /*the first and last grid line of the first connected component*/

  if(topVertex[1] <= botVertex[1])
    return;
    
  /*find i so that the grid line is below the top vertex*/
  Int i=gridStartIndex;
  while (i < leftGridChain->get_nVlines())
    {
      if(leftGridChain->get_v_value(i) < topVertex[1])
	break;
      i++;
    }

  /*find the first connected component starting with i*/
  /*find index1 so that left_uline_index <= right_uline_index, that is, this
   *grid line contains at least one inner grid point
   */
  index1=i;
  int num_skipped_grid_lines=0;
  while(index1 < leftGridChain->get_nVlines())
    {
      if(leftGridChain->getUlineIndex(index1) <= rightGridChain->getUlineIndex(index1))
	break;
      num_skipped_grid_lines++;
      index1++;
    }



  if(index1 >= leftGridChain->get_nVlines()) /*no grid line exists which has inner point*/
    {
      /*stop recursion, ...*/
      /*monotone triangulate it...*/
//      printf("no grid line exists\n");      
/*
      monoTriangulationRecOpt(topVertex, botVertex, leftChain, leftStartIndex,
			   rightChain, rightStartIndex, pStream);
*/

if(num_skipped_grid_lines <2)
  {
    monoTriangulationRecGenOpt(topVertex, botVertex, leftChain, leftStartIndex,
			       leftChain->getNumElements()-1,
			       rightChain, rightStartIndex, 
			       rightChain->getNumElements()-1,
			       pStream);
  }
else
  {
    //the optimum way to triangulate is top-down since this polygon
    //is narrow-long.
    monoTriangulationRec(topVertex, botVertex, leftChain, leftStartIndex,
			 rightChain, rightStartIndex, pStream);
  }

/*
      monoTriangulationRec(topVertex, botVertex, leftChain, leftStartIndex,
			   rightChain, rightStartIndex, pStream);
*/

/*      monoTriangulationRecGenTBOpt(topVertex, botVertex, 
				   leftChain, leftStartIndex, leftChain->getNumElements()-1,
				   rightChain, rightStartIndex, rightChain->getNumElements()-1, 
				   pStream);*/
      


    }
  else
    {

      /*find index2 so that left_inner_index <= right_inner_index holds until index2*/
      index2=index1+1;
      if(index2 < leftGridChain->get_nVlines())
	while(leftGridChain->getInnerIndex(index2) <= rightGridChain->getInnerIndex(index2))
	  {
	    index2++;
	    if(index2 >= leftGridChain->get_nVlines())
	      break;
	  }
      
      index2--;
      

      
      /*the neck*/
      Int neckLeftIndex;
      Int neckRightIndex;

      /*the four corners*/
      Int up_leftCornerWhere;
      Int up_leftCornerIndex;
      Int up_rightCornerWhere;
      Int up_rightCornerIndex;
      Int down_leftCornerWhere;
      Int down_leftCornerIndex;
      Int down_rightCornerWhere;
      Int down_rightCornerIndex;

      Real* tempBotVertex; /*the bottom vertex for this component*/
      Real* nextTopVertex=NULL; /*for the recursion*/
      Int nextLeftStartIndex=0;
      Int nextRightStartIndex=0;

      /*find the points below the grid line index2 on both chains*/
      Int botLeftIndex = leftChain->findIndexStrictBelowGen(
						      leftGridChain->get_v_value(index2),
						      leftStartIndex,
						      leftChain->getNumElements()-1);
      Int botRightIndex = rightChain->findIndexStrictBelowGen(
							rightGridChain->get_v_value(index2),
							rightStartIndex,
							rightChain->getNumElements()-1);
      /*if either botLeftIndex>= numelements,
       *        or botRightIndex >= numelemnet,
       *then there is no neck exists. the bottom vertex is botVertex, 
       */
      if(! findNeckF(leftChain, botLeftIndex, rightChain, botRightIndex,
		   leftGridChain, rightGridChain, index2, neckLeftIndex, neckRightIndex))
	 /*
      if(botLeftIndex == leftChain->getNumElements() ||
	 botRightIndex == rightChain->getNumElements())
	 */
	{
#ifdef MYDEBUG
	  printf("neck NOT exists, botRightIndex=%i\n", botRightIndex);
#endif

	  tempBotVertex =  botVertex;
	  nextTopVertex = botVertex;
	  botLeftIndex = leftChain->getNumElements()-1;
	  botRightIndex = rightChain->getNumElements()-1;
	}
      else /*neck exists*/
	{
#ifdef MYDEBUG
	  printf("neck exists\n");
#endif

          /*
	  findNeck(leftChain, botLeftIndex,
		   rightChain, botRightIndex,
		   neckLeftIndex,
		   neckRightIndex);
		   */
#ifdef MYDEBUG
printf("neck is found, neckLeftIndex=%i, neckRightIndex=%i\n", neckLeftIndex, neckRightIndex);
glBegin(GL_LINES);
glVertex2fv(leftChain->getVertex(neckLeftIndex));
glVertex2fv(rightChain->getVertex(neckRightIndex));
glEnd();
#endif

	  if(leftChain->getVertex(neckLeftIndex)[1] <= rightChain->getVertex(neckRightIndex)[1])
	    {
	      tempBotVertex = leftChain->getVertex(neckLeftIndex);
	      botLeftIndex = neckLeftIndex-1;
	      botRightIndex = neckRightIndex;
	      nextTopVertex = rightChain->getVertex(neckRightIndex);
	      nextLeftStartIndex = neckLeftIndex;
	      nextRightStartIndex = neckRightIndex+1;
	    }
	  else
	    {
	      tempBotVertex = rightChain->getVertex(neckRightIndex);
	      botLeftIndex = neckLeftIndex;
	      botRightIndex = neckRightIndex-1;		  
	      nextTopVertex = leftChain->getVertex(neckLeftIndex);
	      nextLeftStartIndex = neckLeftIndex+1;
	      nextRightStartIndex = neckRightIndex;
	    }
	}

      findUpCorners(topVertex,
		    leftChain,
		    leftStartIndex, botLeftIndex,
		    rightChain,
		    rightStartIndex, botRightIndex,
		    leftGridChain->get_v_value(index1),
		    leftGridChain->get_u_value(index1),
		    rightGridChain->get_u_value(index1),
		    up_leftCornerWhere,
		    up_leftCornerIndex,
		    up_rightCornerWhere,
		    up_rightCornerIndex);

      findDownCorners(tempBotVertex,
		      leftChain,
		      leftStartIndex, botLeftIndex,
		      rightChain,
		      rightStartIndex, botRightIndex,
		      leftGridChain->get_v_value(index2),
		      leftGridChain->get_u_value(index2),
		      rightGridChain->get_u_value(index2),
		      down_leftCornerWhere,
		      down_leftCornerIndex,
		      down_rightCornerWhere,
		      down_rightCornerIndex);	      
#ifdef MYDEBUG
      printf("find corners done, down_leftwhere=%i, down_righwhere=%i,\n",down_leftCornerWhere, down_rightCornerWhere );
      printf("find corners done, up_leftwhere=%i, up_righwhere=%i,\n",up_leftCornerWhere, up_rightCornerWhere );
      printf("find corners done, up_leftindex=%i, up_righindex=%i,\n",up_leftCornerIndex, up_rightCornerIndex );
      printf("find corners done, down_leftindex=%i, down_righindex=%i,\n",down_leftCornerIndex, down_rightCornerIndex );
#endif

/*
      drawCorners(topVertex,
		  tempBotVertex,
		  leftChain,
		  rightChain,
		  leftGridChain,
		  rightGridChain,
		  index1,
		  index2,
		  up_leftCornerWhere,
		  up_leftCornerIndex,
		  up_rightCornerWhere,
		  up_rightCornerIndex,
		  down_leftCornerWhere,
		  down_leftCornerIndex,
		  down_rightCornerWhere,
		  down_rightCornerIndex);
*/


      sampleConnectedComp(topVertex, tempBotVertex,
			  leftChain, 
			  leftStartIndex, botLeftIndex,
			  rightChain,
			  rightStartIndex, botRightIndex,
			  leftGridChain,
			  rightGridChain,
			  index1, index2,
			  up_leftCornerWhere,
			  up_leftCornerIndex,
			  up_rightCornerWhere,
			  up_rightCornerIndex,
			  down_leftCornerWhere,
			  down_leftCornerIndex,
			  down_rightCornerWhere,
			  down_rightCornerIndex,
			  pStream,
			  rbArray
			  );

      /*recursion*/

      sampleMonoPolyRec(
			nextTopVertex,
			botVertex,
			leftChain, 
			nextLeftStartIndex,
			rightChain,
			nextRightStartIndex,
			leftGridChain, 
			rightGridChain, 
			index2+1,
			pStream, rbArray);
			

    }

}

void sampleLeftStrip(vertexArray* leftChain,
		     Int topLeftIndex,
		     Int botLeftIndex,
		     gridBoundaryChain* leftGridChain,
		     Int leftGridChainStartIndex,
		     Int leftGridChainEndIndex,
		     primStream* pStream
		     )
{
  assert(leftChain->getVertex(topLeftIndex)[1] > leftGridChain->get_v_value(leftGridChainStartIndex));
  assert(leftChain->getVertex(topLeftIndex+1)[1] <= leftGridChain->get_v_value(leftGridChainStartIndex));
  assert(leftChain->getVertex(botLeftIndex)[1] <= leftGridChain->get_v_value(leftGridChainEndIndex));
  assert(leftChain->getVertex(botLeftIndex-1)[1] > leftGridChain->get_v_value(leftGridChainEndIndex));

  /*
   *(1)find the last grid line which doesn'; pass below
   * this first edge, sample this region: one trim edge and 
   * possily multiple grid lines.
   */
  Real *upperVert, *lowerVert; /*the end points of the first trim edge*/
  upperVert = leftChain->getVertex(topLeftIndex);
  lowerVert = leftChain->getVertex(topLeftIndex+1);
  
  Int index = leftGridChainStartIndex;
  while(leftGridChain->get_v_value(index) >= lowerVert[1]){
    index++;
    if(index > leftGridChainEndIndex) 
      break;
  }
  index--;
  
  sampleLeftSingleTrimEdgeRegion(upperVert, lowerVert,
				 leftGridChain,
				 leftGridChainStartIndex,
				 index,
				 pStream);
  sampleLeftStripRec(leftChain, topLeftIndex+1, botLeftIndex,
		     leftGridChain, index, leftGridChainEndIndex,
		     pStream);

}

void sampleLeftStripRec(vertexArray* leftChain,
		     Int topLeftIndex,
		     Int botLeftIndex,
		     gridBoundaryChain* leftGridChain,
		     Int leftGridChainStartIndex,
		     Int leftGridChainEndIndex,
			primStream* pStream
		     )
{
  /*now top left trim vertex is below the top grid line.
   */
  /*stop condition: if topLeftIndex >= botLeftIndex, then stop.
   */
  if(topLeftIndex >= botLeftIndex) 
    return;

  /*find the last trim vertex which is above the second top grid line:
   * index1.
   *and sampleLeftOneGridStep(leftchain, topLeftIndex, index1, leftGridChain,
   * leftGridChainStartIndex).
   * index1 could be equal to topLeftIndex.
   */
  Real secondGridChainV = leftGridChain->get_v_value(leftGridChainStartIndex+1);
  assert(leftGridChainStartIndex < leftGridChainEndIndex);
  Int index1 = topLeftIndex;
  while(leftChain->getVertex(index1)[1] > secondGridChainV)
    index1++;
  index1--;
  
  sampleLeftOneGridStep(leftChain, topLeftIndex, index1, leftGridChain, leftGridChainStartIndex, pStream);


  /* 
   * Let the next trim vertex be nextTrimVertIndex (which should be  
   *  below the second grid line).
   * Find the last grid line index2 which is above nextTrimVert.
   * sampleLeftSingleTrimEdgeRegion(uppervert[2], lowervert[2], 
   *                      leftGridChain, leftGridChainStartIndex+1, index2).
   */
  Real *uppervert, *lowervert;
  uppervert = leftChain->getVertex(index1);
  lowervert = leftChain->getVertex(index1+1);
  Int index2 = leftGridChainStartIndex+1;

  while(leftGridChain->get_v_value(index2) >= lowervert[1])
    {
      index2++;
      if(index2 > leftGridChainEndIndex)
	break;
    }
  index2--;
  sampleLeftSingleTrimEdgeRegion(uppervert, lowervert, leftGridChain, leftGridChainStartIndex+1, index2,  pStream);

   /* sampleLeftStripRec(leftChain, 
                        nextTrimVertIndex,
                        botLeftIndex,
                        leftGridChain,
			index2,
                        leftGridChainEndIndex
			)
   *
   */
  sampleLeftStripRec(leftChain, index1+1, botLeftIndex, leftGridChain, index2, leftGridChainEndIndex, pStream);
  
}


/***************begin RecF***********************/
/* the gridlines from leftGridChainStartIndex to 
 * leftGridChainEndIndex are assumed to form a
 * connected component.
 * the trim vertex of topLeftIndex is assumed to
 * be below the first gridline, and the tim vertex
 * of botLeftIndex is assumed to be above the last
 * grid line.
 * If botLeftIndex < topLeftIndex, then no connected componeent exists, and this funcion returns without
 * outputing any triangles.
 * Otherwise botLeftIndex >= topLeftIndex, there is at least one triangle to output.
 */
void sampleLeftStripRecF(vertexArray* leftChain,
		     Int topLeftIndex,
		     Int botLeftIndex,
		     gridBoundaryChain* leftGridChain,
		     Int leftGridChainStartIndex,
		     Int leftGridChainEndIndex,
			primStream* pStream
		     )
{
  /*now top left trim vertex is below the top grid line.
   */
  /*stop condition: if topLeftIndex > botLeftIndex, then stop.
   */
  if(topLeftIndex > botLeftIndex) 
    return;

  /*if there is only one grid Line, return.*/

  if(leftGridChainStartIndex>=leftGridChainEndIndex)
    return;


  assert(leftChain->getVertex(topLeftIndex)[1] <= leftGridChain->get_v_value(leftGridChainStartIndex) &&
	 leftChain->getVertex(botLeftIndex)[1] >= leftGridChain->get_v_value(leftGridChainEndIndex));

  /*firs find the first trim vertex which is below or equal to the second top grid line:
   * index1.
   */
  Real secondGridChainV = leftGridChain->get_v_value(leftGridChainStartIndex+1);


  Int index1 = topLeftIndex;

  while(leftChain->getVertex(index1)[1] > secondGridChainV){
    index1++;
    if(index1>botLeftIndex)
      break;
  }

  /*now leftChain->getVertex(index-1)[1] > secondGridChainV and
   *    leftChain->getVertex(index)[1] <= secondGridChainV
   *If equality holds, then we should include the vertex index1, otherwise we include only index1-1, to
   *perform sampleOneGridStep.
   */
  if(index1>botLeftIndex) 
    index1--;
  else if(leftChain->getVertex(index1)[1] < secondGridChainV)
    index1--;

  /*now we have leftChain->getVertex(index1)[1] >= secondGridChainV, and 
   *  leftChain->getVertex(index1+1)[1] <= secondGridChainV
   */


  sampleLeftOneGridStep(leftChain, topLeftIndex, index1, leftGridChain, leftGridChainStartIndex, pStream);


  /*if leftChain->getVertex(index1)[1] == secondGridChainV, then we can recursively do the rest.
   */
  if(leftChain->getVertex(index1)[1] == secondGridChainV)
    {

      sampleLeftStripRecF(leftChain, index1, botLeftIndex,leftGridChain, leftGridChainStartIndex+1, leftGridChainEndIndex, pStream);
    }
  else if(index1 < botLeftIndex)
    {

      /* Otherwise, we have leftChain->getVertex(index1)[1] > secondGridChainV,
       * let the next trim vertex be nextTrimVertIndex (which should be  strictly
       *  below the second grid line).
       * Find the last grid line index2 which is above nextTrimVert.
       * sampleLeftSingleTrimEdgeRegion(uppervert[2], lowervert[2], 
       *                      leftGridChain, leftGridChainStartIndex+1, index2).
       */
      Real *uppervert, *lowervert;
      uppervert = leftChain->getVertex(index1);
      lowervert = leftChain->getVertex(index1+1); //okay since index1<botLeftIndex
      Int index2 = leftGridChainStartIndex+1;

      
      while(leftGridChain->get_v_value(index2) >= lowervert[1])
	{
	  index2++;
	  if(index2 > leftGridChainEndIndex)
	    break;
	}
      index2--;

      
      sampleLeftSingleTrimEdgeRegion(uppervert, lowervert, leftGridChain, leftGridChainStartIndex+1, index2,  pStream);
      
      /*recursion*/

      sampleLeftStripRecF(leftChain, index1+1, botLeftIndex, leftGridChain, index2, leftGridChainEndIndex, pStream);
    }
  
}

/***************End RecF***********************/

/*sample the left area in between one trim edge and multiple grid lines.
 * all the grid lines should be in between the two end poins of the
 *trim edge.
 */
void sampleLeftSingleTrimEdgeRegion(Real upperVert[2], Real lowerVert[2],
				    gridBoundaryChain* gridChain,
				    Int beginIndex,
				    Int endIndex,
				    primStream* pStream)
{
  Int i,j,k;

  vertexArray vArray(endIndex-beginIndex+1);  
  vArray.appendVertex(gridChain->get_vertex(beginIndex));

  for(k=1, i=beginIndex+1; i<=endIndex; i++, k++)
    {
      vArray.appendVertex(gridChain->get_vertex(i));

      /*output the fan of the grid points of the (i)th and (i-1)th grid line.
       */
      if(gridChain->getUlineIndex(i) < gridChain->getUlineIndex(i-1))
	{
	  pStream->begin();	  
	  pStream->insert(gridChain->get_vertex(i-1));
	  for(j=gridChain->getUlineIndex(i); j<= gridChain->getUlineIndex(i-1); j++)
	    pStream->insert(gridChain->getGrid()->get_u_value(j), gridChain->get_v_value(i));
	  pStream->end(PRIMITIVE_STREAM_FAN);
	}
      else if(gridChain->getUlineIndex(i) > gridChain->getUlineIndex(i-1))
	{
	  pStream->begin();
	  pStream->insert(gridChain->get_vertex(i));
	  for(j=gridChain->getUlineIndex(i); j>= gridChain->getUlineIndex(i-1); j--)
	    pStream->insert(gridChain->getGrid()->get_u_value(j), gridChain->get_v_value(i-1));
	  pStream->end(PRIMITIVE_STREAM_FAN);
	}
      /*otherwisem, the two are equal, so there is no fan to outout*/	  
    }
  
  monoTriangulation2(upperVert, lowerVert, &vArray, 0, endIndex-beginIndex, 
		     0, /*decreasing chain*/
		     pStream);
}
  
/*return i, such that from begin to i-1 the chain is strictly u-monotone. 
 */				 
Int findIncreaseChainFromBegin(vertexArray* chain, Int begin ,Int end)
{
  Int i=begin;
  Real prevU = chain->getVertex(i)[0];
  Real thisU;
  for(i=begin+1; i<=end; i++){
    thisU = chain->getVertex(i)[0];

    if(prevU < thisU){
      prevU = thisU;
    }
    else
      break;
  }
  return i;
}
  
/*check whether there is a vertex whose v value is strictly 
 *inbetween vup vbelow
 *if no middle exists return -1, else return the idnex.
 */
Int checkMiddle(vertexArray* chain, Int begin, Int end, 
		Real vup, Real vbelow)
{
  Int i;
  for(i=begin; i<=end; i++)
    {
      if(chain->getVertex(i)[1] < vup && chain->getVertex(i)[1]>vbelow)
	return i;
    }
  return -1;
}
  
/*the degenerat case of sampleLeftOneGridStep*/
void sampleLeftOneGridStepNoMiddle(vertexArray* leftChain,
				   Int beginLeftIndex,
				   Int endLeftIndex,
				   gridBoundaryChain* leftGridChain,
				   Int leftGridChainStartIndex,
				   primStream* pStream)
{
  /*since there is no middle, there is at most one point which is on the 
   *second grid line, there could be multiple points on the first (top)
   *grid line.
   */

  leftGridChain->leftEndFan(leftGridChainStartIndex+1, pStream);

  monoTriangulation2(leftGridChain->get_vertex(leftGridChainStartIndex),
		     leftGridChain->get_vertex(leftGridChainStartIndex+1),
		     leftChain,
		     beginLeftIndex,
		     endLeftIndex,
		     1, //is increase chain.
		     pStream);
}



/*sampling the left area in between two grid lines.
 */
void sampleLeftOneGridStep(vertexArray* leftChain,
		  Int beginLeftIndex,
		  Int endLeftIndex,
		  gridBoundaryChain* leftGridChain,
		  Int leftGridChainStartIndex,
		  primStream* pStream
		  )
{
  if(checkMiddle(leftChain, beginLeftIndex, endLeftIndex, 
		 leftGridChain->get_v_value(leftGridChainStartIndex),
		 leftGridChain->get_v_value(leftGridChainStartIndex+1))<0)
    
    {
      
      sampleLeftOneGridStepNoMiddle(leftChain, beginLeftIndex, endLeftIndex, leftGridChain, leftGridChainStartIndex, pStream);
      return;
    }
  
  //copy into a polygon
  {
    directedLine* poly = NULL;
    sampledLine* sline;
    directedLine* dline;
    gridWrap* grid = leftGridChain->getGrid();
    Real vert1[2];
    Real vert2[2];
    Int i;
    
    Int innerInd = leftGridChain->getInnerIndex(leftGridChainStartIndex+1);
    Int upperInd = leftGridChain->getUlineIndex(leftGridChainStartIndex);
    Int lowerInd = leftGridChain->getUlineIndex(leftGridChainStartIndex+1);
    Real upperV = leftGridChain->get_v_value(leftGridChainStartIndex);
    Real lowerV = leftGridChain->get_v_value(leftGridChainStartIndex+1);

    //the upper gridline
    vert1[1] = vert2[1] = upperV;
    for(i=innerInd;	i>upperInd;	i--)
      {
	vert1[0]=grid->get_u_value(i);
	vert2[0]=grid->get_u_value(i-1);
	sline = new sampledLine(vert1, vert2);
	dline = new directedLine(INCREASING, sline);
	if(poly == NULL)
	  poly = dline;
	else
	  poly->insert(dline);
      }

    //the edge connecting upper grid with left chain
    vert1[0] = grid->get_u_value(upperInd);
    vert1[1] = upperV;
    sline = new sampledLine(vert1, leftChain->getVertex(beginLeftIndex));
    dline = new directedLine(INCREASING, sline);
    if(poly == NULL)
      poly = dline;
    else
      poly->insert(dline);    
 
    //the left chain
    for(i=beginLeftIndex; i<endLeftIndex; i++)
      {
	sline = new sampledLine(leftChain->getVertex(i), leftChain->getVertex(i+1));
	dline = new directedLine(INCREASING, sline);
	poly->insert(dline);
      }

    //the edge connecting left chain with lower gridline
    vert2[0] = grid->get_u_value(lowerInd);
    vert2[1] = lowerV;
    sline = new sampledLine(leftChain->getVertex(endLeftIndex), vert2);
    dline = new directedLine(INCREASING, sline);
    poly->insert(dline);

    //the lower grid line
    vert1[1] = vert2[1] = lowerV;
    for(i=lowerInd; i<innerInd; i++)
      {
	vert1[0] = grid->get_u_value(i);
	vert2[0] = grid->get_u_value(i+1);
	sline = new sampledLine(vert1, vert2);
	dline = new directedLine(INCREASING, sline);
	poly->insert(dline);       
      }

    //the vertical grid line segement
    vert1[0]=vert2[0] = grid->get_u_value(innerInd);
    vert2[1]=upperV;
    vert1[1]=lowerV;
    sline=new sampledLine(vert1, vert2);
    dline=new directedLine(INCREASING, sline);
    poly->insert(dline);
    monoTriangulationOpt(poly, pStream);
    //cleanup
    poly->deleteSinglePolygonWithSline();
    return;
  }
 


 

  Int i;
  if(1/*leftGridChain->getUlineIndex(leftGridChainStartIndex) >= 
     leftGridChain->getUlineIndex(leftGridChainStartIndex+1)*/
     ) /*the second grid line is beyond the first one to the left*/
    {
      /*find the maximal U-monotone chain 
       * of endLeftIndex, endLeftIndex-1, ..., 
       */
      i=endLeftIndex;
      Real prevU = leftChain->getVertex(i)[0];
      for(i=endLeftIndex-1; i>=beginLeftIndex; i--){
	Real thisU = leftChain->getVertex(i)[0];
	if( prevU < thisU){
	  prevU = thisU;
	}
	else 
	  break;
      }
      /*from endLeftIndex to i+1 is strictly U- monotone */
      /*if i+1==endLeftIndex and the vertex and leftchain is on the second gridline, then
       *we should use 2 vertices on the leftchain. If we only use one (endLeftIndex), then we
       *we would output degenerate triangles
       */
      if(i+1 == endLeftIndex && leftChain->getVertex(endLeftIndex)[1] == leftGridChain->get_v_value(1+leftGridChainStartIndex))
	i--;

      Int j = beginLeftIndex/*endLeftIndex*/+1;


      if(leftGridChain->getInnerIndex(leftGridChainStartIndex+1) > leftGridChain->getUlineIndex(leftGridChainStartIndex))
	{
	  j = findIncreaseChainFromBegin(leftChain, beginLeftIndex, i+1/*endLeftIndex*/);

	  Int temp = beginLeftIndex;
	  /*now from begin to j-1 is strictly u-monotone*/
	  /*if j-1 is on the first grid line, then we want to skip to the vertex which is strictly
	   *below the grid line. This vertexmust exist since there is a 'corner turn' inbetween the two grid lines
	   */
	  if(j-1 == beginLeftIndex)
	    {
	      while(leftChain->getVertex(j-1)[1] == leftGridChain->get_v_value(leftGridChainStartIndex))
		j++;	        

	      Real vert[2];
	      vert[0] = leftGridChain->get_u_value(leftGridChainStartIndex);
	      vert[1] = leftGridChain->get_v_value(leftGridChainStartIndex);

	      monoTriangulation2(
				 vert/*leftChain->getVertex(beginLeftIndex)*/,
				 leftChain->getVertex(j-1),
				 leftChain,
				 beginLeftIndex,
				 j-2,
				 1,
				 pStream //increase chain
				 );
				 
	      temp = j-1;
	    }
				 
	  stripOfFanLeft(leftChain, j-1, temp/*beginLeftIndex*/, leftGridChain->getGrid(),
			 leftGridChain->getVlineIndex(leftGridChainStartIndex),
			 leftGridChain->getUlineIndex(leftGridChainStartIndex),
			 leftGridChain->getInnerIndex(leftGridChainStartIndex+1),
			 pStream,
			 1 /*the grid line is above the trim line*/
			 );
	}
      
      stripOfFanLeft(leftChain, endLeftIndex, i+1, leftGridChain->getGrid(), 
		     leftGridChain->getVlineIndex(leftGridChainStartIndex+1),
		     leftGridChain->getUlineIndex(leftGridChainStartIndex+1),
		     leftGridChain->getInnerIndex(leftGridChainStartIndex+1),
		     pStream,
		     0 /*the grid line is below the trim lines*/
		     );
      
      /*monotone triangulate the remaining left chain togther with the
       *two vertices on the two grid v-lines.
       */
      Real vert[2][2];
      vert[0][0]=vert[1][0] = leftGridChain->getInner_u_value(leftGridChainStartIndex+1);
      vert[0][1] = leftGridChain->get_v_value(leftGridChainStartIndex);      
      vert[1][1] = leftGridChain->get_v_value(leftGridChainStartIndex+1);

//      vertexArray right(vert, 2);

      monoTriangulation2(
			 &vert[0][0], /*top vertex */
			 &vert[1][0], /*bottom vertex*/
			 leftChain, 
			 /*beginLeftIndex*/j-1,
			 i+1,
			 1, /*an increasing chain*/
			 pStream);
    }
  else /*the second one is shorter than the first one to the left*/
    {
      /*find the maximal U-monotone chain of beginLeftIndex, beginLeftIndex+1,...,
       */
      i=beginLeftIndex;
      Real prevU = leftChain->getVertex(i)[0];
      for(i=beginLeftIndex+1; i<=endLeftIndex; i++){
	Real thisU = leftChain->getVertex(i)[0];

	if(prevU < thisU){
	  prevU = thisU;
	}
	else
	  break;
      }
      /*from beginLeftIndex to i-1 is strictly U-monotone*/


      stripOfFanLeft(leftChain, i-1, beginLeftIndex, leftGridChain->getGrid(),
			 leftGridChain->getVlineIndex(leftGridChainStartIndex),
			 leftGridChain->getUlineIndex(leftGridChainStartIndex),
			 leftGridChain->getUlineIndex(leftGridChainStartIndex+1),
			 pStream,
		         1 /*the grid line is above the trim lines*/
			 );
      /*monotone triangulate the remaining left chain together with the 
       *two vertices on the two grid v-lines.
       */
      Real vert[2][2];
      vert[0][0]=vert[1][0] = leftGridChain->get_u_value(leftGridChainStartIndex+1);
      vert[0][1] = leftGridChain->get_v_value(leftGridChainStartIndex);      
      vert[1][1] = leftGridChain->get_v_value(leftGridChainStartIndex+1);

      vertexArray right(vert, 2);

      monoTriangulation2(
			 &vert[0][0], //top vertex 
			 &vert[1][0], //bottom vertex
			 leftChain, 
			 i-1,
			 endLeftIndex,
			 1, /*an increase chain*/
			 pStream);

    }
}
  
/*n_upper>=1
 *n_lower>=1
 */
void triangulateXYMono(Int n_upper, Real upperVerts[][2],
		       Int n_lower, Real lowerVerts[][2],
		       primStream* pStream)
{
  Int i,j,k,l;
  Real* leftMostV;

  assert(n_upper>=1 && n_lower>=1);
  if(upperVerts[0][0] <= lowerVerts[0][0])
    {
      i=1;
      j=0;
      leftMostV = upperVerts[0];
    }
  else
    {
      i=0;
      j=1;
      leftMostV = lowerVerts[0];
    }
  
  while(1)
    {
      if(i >= n_upper) /*case1: no more in upper*/
	{

	  if(j<n_lower-1) /*at least two vertices in lower*/
	    {
	      pStream->begin();
	      pStream->insert(leftMostV);
	      while(j<n_lower){
		pStream->insert(lowerVerts[j]);
		j++;
	      }
	      pStream->end(PRIMITIVE_STREAM_FAN);
	    }

	  break;	
	}
      else if(j>= n_lower) /*case2: no more in lower*/
	{

	  if(i<n_upper-1) /*at least two vertices in upper*/
	    {
	      pStream->begin();
	      pStream->insert(leftMostV);

	      for(k=n_upper-1; k>=i; k--)
		pStream->insert(upperVerts[k]);

	      pStream->end(PRIMITIVE_STREAM_FAN);
	    }

	  break;
	}
      else /* case3: neither is empty, plus the leftMostV, there is at least one triangle to output*/
	{

	  if(upperVerts[i][0] <= lowerVerts[j][0])
	    {
	      pStream->begin();
	      pStream->insert(lowerVerts[j]); /*the origin of this fan*/

	      /*find the last k>=i such that 
	       *upperverts[k][0] <= lowerverts[j][0]
	       */
	      k=i;
	      while(k<n_upper)
		{
		  if(upperVerts[k][0] > lowerVerts[j][0])
		    break;
		  k++;
		}
	      k--;
	      for(l=k; l>=i; l--)/*the reverse is for two-face lighting*/
		{
		  pStream->insert(upperVerts[l]);
		}
	      pStream->insert(leftMostV);

	      pStream->end(PRIMITIVE_STREAM_FAN);
	      //update i for next loop
	      i = k+1;
	      leftMostV = upperVerts[k];

	    }
	  else /*upperVerts[i][0] > lowerVerts[j][0]*/
	    {
	      pStream->begin();
	      pStream->insert(upperVerts[i]);/*the origion of this fan*/
	      pStream->insert(leftMostV);
	      /*find the last k>=j such that
	       *lowerverts[k][0] < upperverts[i][0]*/
	      k=j;
	      while(k< n_lower)
		{
		  if(lowerVerts[k][0] >= upperVerts[i][0])
		    break;
		  pStream->insert(lowerVerts[k]);
		  k++;
		}
	      pStream->end(PRIMITIVE_STREAM_FAN);
	      j=k;
	      leftMostV = lowerVerts[j-1];
	    }	  
	}
    }
}
		       

void stripOfFanLeft(vertexArray* leftChain, 
		    Int largeIndex,
		    Int smallIndex,
		    gridWrap* grid,
		    Int vlineIndex,
		    Int ulineSmallIndex,
		    Int ulineLargeIndex,
		    primStream* pStream,
		    Int gridLineUp /*1 if the grid line is above the trim lines*/
		     )
{
  assert(largeIndex >= smallIndex);

  Real grid_v_value;
  grid_v_value = grid->get_v_value(vlineIndex);

  Real2* trimVerts=(Real2*) malloc(sizeof(Real2)* (largeIndex-smallIndex+1));
  assert(trimVerts);


  Real2* gridVerts=(Real2*) malloc(sizeof(Real2)* (ulineLargeIndex-ulineSmallIndex+1));
  assert(gridVerts);

  Int k,i;
  if(gridLineUp) /*trim line is below grid line, so trim vertices are going right when index increases*/
    for(k=0, i=smallIndex; i<=largeIndex; i++, k++)
      {
      trimVerts[k][0] = leftChain->getVertex(i)[0];
      trimVerts[k][1] = leftChain->getVertex(i)[1];
    }
  else
    for(k=0, i=largeIndex; i>=smallIndex; i--, k++)
      {
	trimVerts[k][0] = leftChain->getVertex(i)[0];
	trimVerts[k][1] = leftChain->getVertex(i)[1];
      }

  for(k=0, i=ulineSmallIndex; i<= ulineLargeIndex; i++, k++)
    {
      gridVerts[k][0] = grid->get_u_value(i);
      gridVerts[k][1] = grid_v_value;
    }

  if(gridLineUp)
    triangulateXYMono(
		      ulineLargeIndex-ulineSmallIndex+1, gridVerts,
		      largeIndex-smallIndex+1, trimVerts,
		      pStream);
  else
    triangulateXYMono(largeIndex-smallIndex+1, trimVerts,
		      ulineLargeIndex-ulineSmallIndex+1, gridVerts,
		      pStream);
  free(trimVerts);
  free(gridVerts);
}

  




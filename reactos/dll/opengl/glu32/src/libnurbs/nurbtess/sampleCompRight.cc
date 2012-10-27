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
#include "gluos.h"
#include "glimports.h"
#include "zlassert.h"
#include "sampleCompRight.h"

#define max(a,b) ((a>b)? a:b)
#define min(a,b) ((a>b)? b:a)



#ifdef NOT_TAKEOUT

/*notice that we need leftChain because the 
 *corners could be on the leftChain.
 */
void sampleCompRight(Real* topVertex, Real* botVertex,
		    vertexArray* leftChain,
		    Int leftStartIndex, Int leftEndIndex,
		    vertexArray* rightChain,
		    Int rightStartIndex, Int rightEndIndex,
		    gridBoundaryChain* rightGridChain,
		    Int gridIndex1, Int gridIndex2,
		    Int up_rightCornerWhere,
		    Int up_rightCornerIndex,
		    Int down_rightCornerWhere,
		    Int down_rightCornerIndex,
		    primStream* pStream)
{
  /*find out whether there is a trim vertex  which is
   *inbetween the top and bot grid lines or not.
   */
  Int midIndex1;
  Int midIndex2;
  Int gridMidIndex1 = 0, gridMidIndex2 = 0;
  //midIndex1: array[i] <= v, array[i+1] > v
  //midIndex2: array[i] >= v,  array[i+1] < v
  midIndex1 = rightChain->findIndexBelowGen(rightGridChain->get_v_value(gridIndex1),
					    rightStartIndex,
					    rightEndIndex);
  midIndex2 = -1; //initilization
  if(midIndex1 <= rightEndIndex && gridIndex1 < gridIndex2)
    if(rightChain->getVertex(midIndex1)[1] >= rightGridChain->get_v_value(gridIndex2))
      {
	//midIndex2 must exist:
	midIndex2 = rightChain->findIndexAboveGen(rightGridChain->get_v_value(gridIndex2),
						  midIndex1, //midIndex1<=midIndex2
						  rightEndIndex);
	//find gridMidIndex1 so that either it=gridIndex1 when the gridline is 
	// at the same height as trim vertex midIndex1, or it is the last one 
	//which is strictly above midIndex1.
	{
	  Real temp = rightChain->getVertex(midIndex1)[1];
	  if(rightGridChain->get_v_value(gridIndex1) == temp)
	    gridMidIndex1 = gridIndex1;
	  else
	    {
	    gridMidIndex1 = gridIndex1;
	    while(rightGridChain->get_v_value(gridMidIndex1) > temp)
	      gridMidIndex1++;
	    gridMidIndex1--;
	    }
	}//end of find gridMindIndex1
	//find gridMidIndex2 so that it is the (first one below or equal 
	//midIndex) last one above or equal midIndex2
	{
	  Real temp = rightChain->getVertex(midIndex2)[1];
	  for(gridMidIndex2 = gridMidIndex1+1; gridMidIndex2 <= gridIndex2; gridMidIndex2++)
	    if(rightGridChain->get_v_value(gridMidIndex2) <= temp)
	      break;

	  assert(gridMidIndex2 <= gridIndex2);
	}//end of find gridMidIndex2
      }


  
  //to interprete the corner information
  Real* cornerTop;
  Real* cornerBot;
  Int cornerRightStart;
  Int cornerRightEnd;
  Int cornerLeftUpEnd;
  Int cornerLeftDownStart;
  if(up_rightCornerWhere == 2) //right corner is on right chain
    {
      cornerTop = rightChain->getVertex(up_rightCornerIndex);
      cornerRightStart = up_rightCornerIndex+1;
      cornerLeftUpEnd = -1; //no left
    }
  else if(up_rightCornerWhere == 1) //right corner is on top
    {
      cornerTop = topVertex;
      cornerRightStart = rightStartIndex;
      cornerLeftUpEnd = -1; //no left
    }
  else //right corner is on left chain
    {
      cornerTop = topVertex;
      cornerRightStart = rightStartIndex;
      cornerLeftUpEnd = up_rightCornerIndex;
    }
  
  if(down_rightCornerWhere == 2) //right corner is on right chan
    {
      cornerBot = rightChain->getVertex(down_rightCornerIndex);
      cornerRightEnd = down_rightCornerIndex-1;
      cornerLeftDownStart = leftEndIndex+1; //no left
    }
  else if (down_rightCornerWhere == 1) //right corner is at bot
    {
      cornerBot = botVertex;
      cornerRightEnd = rightEndIndex;
      cornerLeftDownStart = leftEndIndex+1; //no left     
    }
  else //right corner is on the left chain
    {
      cornerBot = botVertex;
      cornerRightEnd = rightEndIndex;
      cornerLeftDownStart = down_rightCornerIndex;
    }

  //sample
  if(midIndex2 >= 0) //there is a trm point between grid lines
    {

      sampleRightSingleTrimEdgeRegionGen(cornerTop, rightChain->getVertex(midIndex1),
					 rightChain,
					 cornerRightStart,
					 midIndex1-1,
					 rightGridChain,
					 gridIndex1,
					 gridMidIndex1,
					 leftChain,
					 leftStartIndex,
					 cornerLeftUpEnd,
					 0, //no left down section,
					 -1,
					 pStream);

      sampleRightSingleTrimEdgeRegionGen(rightChain->getVertex(midIndex2),
					 cornerBot,
					 rightChain,
					 midIndex2+1,
					 cornerRightEnd,
					 rightGridChain,
					 gridMidIndex2,
					 gridIndex2,
					 leftChain,
					 0, //no left up section
					 -1,
					 cornerLeftDownStart,
					 leftEndIndex,
					 pStream);

      sampleRightStripRecF(rightChain,
			   midIndex1,
			   midIndex2,
			   rightGridChain,
			   gridMidIndex1,
			   gridMidIndex2,
			   pStream);

    }
  else
    {
      sampleRightSingleTrimEdgeRegionGen(cornerTop, cornerBot,
					 rightChain,
					 cornerRightStart,
					 cornerRightEnd,
					 rightGridChain,
					 gridIndex1,
					 gridIndex2,
					 leftChain,
					 leftStartIndex,
					 cornerLeftUpEnd,
					 cornerLeftDownStart,
					 leftEndIndex,
					 pStream);
    }
}

void sampleRightSingleTrimEdgeRegionGen(Real topVertex[2], Real botVertex[2],
					 vertexArray* rightChain,
					 Int rightStart,
					 Int rightEnd,
					 gridBoundaryChain* gridChain,
					 Int gridBeginIndex,
					 Int gridEndIndex,
					 vertexArray* leftChain,
					 Int leftUpBegin,
					 Int leftUpEnd,
					 Int leftDownBegin,
					 Int leftDownEnd,
					 primStream* pStream)
{
  Int i,k;
   /*creat an array to store all the up and down secments of the left chain,
   *and the right end grid points
   *
   *although vertex array is a dynamic array, but to gain efficiency,
   *it is better to initiliza the exact array size
   */
  vertexArray vArray(gridEndIndex-gridBeginIndex+1 +
		     max(0,leftUpEnd - leftUpBegin+1)+
		     max(0,leftDownEnd - leftDownBegin+1));
  //append the vertices on the up section of the left chain
  for(i=leftUpBegin; i<= leftUpEnd; i++)
    vArray.appendVertex(leftChain->getVertex(i));
  
  //append the vertices of the right extremal grid points,
  //and at the same time, perform triangulation for the stair cases
  vArray.appendVertex(gridChain->get_vertex(gridBeginIndex));
  
  for(k=1, i=gridBeginIndex+1; i<= gridEndIndex; i++, k++)
    {
      vArray.appendVertex(gridChain->get_vertex(i));
      
      //output the fan of the grid points of the (i)th and (i-1)th grid line.
      gridChain->rightEndFan(i, pStream);
    }
  
  //append all the vertices on the down section of the left chain
  for(i=leftDownBegin; i<= leftDownEnd; i++)
    vArray.appendVertex(leftChain->getVertex(i));
  monoTriangulationRecGen(topVertex, botVertex,
			  &vArray, 0, vArray.getNumElements()-1,
			  rightChain, rightStart, rightEnd,
			  pStream);
}
  
void sampleRightSingleTrimEdgeRegion(Real upperVert[2], Real lowerVert[2],
				     gridBoundaryChain* gridChain,
				     Int beginIndex,
				     Int endIndex,
				     primStream* pStream)
{
  Int i,k;
  vertexArray vArray(endIndex-beginIndex+1);
  vArray.appendVertex(gridChain->get_vertex(beginIndex));
  for(k=1, i=beginIndex+1; i<= endIndex; i++, k++)
    {
      vArray.appendVertex(gridChain->get_vertex(i));
      //output the fan of the grid points of the (i)_th and i-1th gridLine
      gridChain->rightEndFan(i, pStream);
    }
  monoTriangulation2(upperVert, lowerVert, &vArray, 0, endIndex-beginIndex, 
		     1, //increase chain (to the left)
		     pStream);
}
		      

/*the gridlines from rightGridChainStartIndex to 
 *rightGridChainEndIndex are assumed to form a 
 *connected componenet
 *the trm vertex of topRightIndex is assumed to be below
 *or equal the first gridLine, and the trm vertex of
 *botRightIndex is assumed to be above or equal the last gridline
 **there could be multipe trm vertices equal to the last gridline, but
 **only one could be equal to top gridline. shape: ____| (recall that
 **for left chain recF, we allow shape: |----
 *if botRightIndex<topRightIndex, then no connected componenet exists, and 
 *no triangles are generated.
 *Othewise, botRightIndex>= topRightIndex, there is at least one triangles to 
 *output
 */
void sampleRightStripRecF(vertexArray* rightChain,
		     Int topRightIndex,
		     Int botRightIndex,
		     gridBoundaryChain* rightGridChain,
		     Int rightGridChainStartIndex,
		     Int rightGridChainEndIndex,	
		     primStream* pStream
		     )
{

  //sstop conditionL: if topRightIndex > botRightIndex, then stop
  if(topRightIndex > botRightIndex)
    return;
  
  //if there is only one grid line, return
  if(rightGridChainStartIndex >= rightGridChainEndIndex)
    return;


  assert(rightChain->getVertex(topRightIndex)[1] <= rightGridChain->get_v_value(rightGridChainStartIndex) &&
	 rightChain->getVertex(botRightIndex)[1] >= rightGridChain->get_v_value(rightGridChainEndIndex));

  //firstfind the first trim vertex which is strictly below the second top
  //grid line: index1. 
  Real secondGridChainV = rightGridChain->get_v_value(rightGridChainStartIndex+1);
  Int index1 = topRightIndex;
  while(rightChain->getVertex(index1)[1] >= secondGridChainV){
    index1++;
    if(index1 >  botRightIndex)
      break;
  }
  //now rightChain->getVertex(index1-1)[1] >= secondGridChainV and
  //rightChain->getVertex(index1)[1] < secondGridChainV and
  //we should include index1-1 to perform a gridStep
    index1--;

  //now we have rightChain->getVertex(index1)[1] >= secondGridChainV, and
  //rightChain->getVertex(index1+1)[1] < secondGridChainV
  sampleRightOneGridStep(rightChain, topRightIndex, index1, rightGridChain, rightGridChainStartIndex, pStream);

  //if rightChain->getVertex(index1)[1] ==secondGridChainV then we can 
  //recurvesively to the rest
  if(rightChain->getVertex(index1)[1] == secondGridChainV)
    {

      
      sampleRightStripRecF(rightChain, index1, botRightIndex, rightGridChain, rightGridChainStartIndex+1, rightGridChainEndIndex, pStream);
    }
  else if(index1 < botRightIndex)
    {
      //otherwise, we have rightChain->getVertex(index1)[1] > secondV
      //let the next trim vertex be nextTrimVertex, (which should be strictly
      //below the second grid line). Find the last grid line index2 which is STRICTLY ABOVE
      //nextTrimVertex.
      //sample one trm edge region.
      Real *uppervert, *lowervert;
      uppervert = rightChain->getVertex(index1);
      lowervert = rightChain->getVertex(index1+1); //okay since index1<botRightindex
      Int index2 = rightGridChainStartIndex+1;
      while(rightGridChain->get_v_value(index2) > lowervert[1])
	{
	  index2++;
	  if(index2 > rightGridChainEndIndex)
	    break;
	}
      index2--;
      
      sampleRightSingleTrimEdgeRegion(uppervert, lowervert, rightGridChain, rightGridChainStartIndex+1, index2, pStream);
      
      //recursion
      sampleRightStripRecF(rightChain, index1+1, botRightIndex, rightGridChain, index2, rightGridChainEndIndex, pStream);
    }
}

//the degenerate case of sampleRightOneGridStep
void sampleRightOneGridStepNoMiddle(vertexArray* rightChain,
				    Int beginRightIndex,
				    Int endRightIndex,
				    gridBoundaryChain* rightGridChain,
				    Int rightGridChainStartIndex,
				    primStream* pStream)
{
  /*since there is no middle, there is at most one point which is on the 
   *second grid line, there could be multiple points on the first (top)
   *grid line.
   */
  rightGridChain->rightEndFan(rightGridChainStartIndex+1, pStream);
  monoTriangulation2(rightGridChain->get_vertex(rightGridChainStartIndex),
		     rightGridChain->get_vertex(rightGridChainStartIndex+1),
		     rightChain,
		     beginRightIndex,
		     endRightIndex,
		     0, //decrease chain
		     pStream);
}

//sampling the right area in between two grid lines
//shape: _________|
void sampleRightOneGridStep(vertexArray* rightChain, 
			    Int beginRightIndex,
			    Int endRightIndex,
			    gridBoundaryChain* rightGridChain,
			    Int rightGridChainStartIndex,
			    primStream* pStream)
{
  if(checkMiddle(rightChain, beginRightIndex, endRightIndex,
		 rightGridChain->get_v_value(rightGridChainStartIndex),
		 rightGridChain->get_v_value(rightGridChainStartIndex+1))<0)
    {
      sampleRightOneGridStepNoMiddle(rightChain, beginRightIndex, endRightIndex, rightGridChain, rightGridChainStartIndex, pStream);
      return;
    }

  //copy into a polygn
  {
    directedLine* poly = NULL;
    sampledLine* sline;
    directedLine* dline;
    gridWrap* grid = rightGridChain->getGrid();
    float vert1[2];
    float vert2[2];
    Int i;
    
    Int innerInd = rightGridChain->getInnerIndex(rightGridChainStartIndex+1);
    Int upperInd = rightGridChain->getUlineIndex(rightGridChainStartIndex);
    Int lowerInd = rightGridChain->getUlineIndex(rightGridChainStartIndex+1);
    Real upperV = rightGridChain->get_v_value(rightGridChainStartIndex);
    Real lowerV = rightGridChain->get_v_value(rightGridChainStartIndex+1);
    
    //the upper gridline
    vert1[1]=vert2[1]=upperV;
    for(i=upperInd;
	i>innerInd;
	i--)
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
    
    //the vertical grid line segment
    vert1[0]=vert2[0] = grid->get_u_value(innerInd);
    vert1[1]=upperV;
    vert2[1]=lowerV;
    sline=new sampledLine(vert1, vert2);
    dline=new directedLine(INCREASING, sline);
    if(poly == NULL)
      poly = dline;
    else
      poly->insert(dline);
    
    //the lower grid line
    vert1[1]=vert2[1]=lowerV;
    for(i=innerInd; i<lowerInd; i++)
      {
	vert1[0] = grid->get_u_value(i);
	vert2[0] = grid->get_u_value(i+1);
	sline = new sampledLine(vert1, vert2);
	dline = new directedLine(INCREASING, sline);
	poly->insert(dline);       
      }

    //the edge connecting lower grid to right chain
    vert1[0]=grid->get_u_value(lowerInd);
    sline = new sampledLine(vert1, rightChain->getVertex(endRightIndex));
    dline = new directedLine(INCREASING, sline);
    poly->insert(dline);
    
    
    //the right Chain
    for(i=endRightIndex; i>beginRightIndex; i--)
      {
	sline = new sampledLine(rightChain->getVertex(i), rightChain->getVertex(i-1));
	dline = new directedLine(INCREASING, sline);
	poly->insert(dline);
      }

    //the edge connecting right chain with upper grid
    vert2[1]=upperV;
    vert2[0]=grid->get_u_value(upperInd);
    sline = new sampledLine(rightChain->getVertex(beginRightIndex), vert2);
    dline = new directedLine(INCREASING, sline);
    poly->insert(dline);    
    monoTriangulationOpt(poly, pStream);
    //clean up
    poly->deleteSinglePolygonWithSline();

    return;
  }
	   
  //this following code cannot be reached, but leave it for debuggig purpose.
  Int i;
  //find the maximal U-monotone chain of beginRightIndex, beginRightIndex+1,...
  i=beginRightIndex;
  Real prevU = rightChain->getVertex(i)[0];
  for(i=beginRightIndex+1; i<= endRightIndex; i++){
    Real thisU = rightChain->getVertex(i)[0];
    if(thisU < prevU)
      prevU = thisU;
    else
      break;
  }
  //from beginRightIndex to i-1 is strictly U-monotne
  //if(i-1==beginRightIndex and the vertex of rightchain is on the first 
  //gridline, then we should use 2 vertices  on the right chain. Of we only 
  //use one (begin), we would output degenrate triangles.
  if(i-1 == beginRightIndex && rightChain->getVertex(beginRightIndex)[1] == rightGridChain->get_v_value(rightGridChainStartIndex))
    i++;
  
  Int j = endRightIndex -1;
  if(rightGridChain->getInnerIndex(rightGridChainStartIndex+1) < rightGridChain->getUlineIndex(rightGridChainStartIndex+1))
    {
      j = rightChain->findDecreaseChainFromEnd(i-1/*beginRightIndex*/, endRightIndex);
      Int temp = endRightIndex;
      //now from j+1 to end is strictly U-monotone.
      //if j+1 is on the last grid line, then we wat to skip to the vertex   
      //whcih is strictly above the second grid line. This vertex must exist 
      //since there is a middle vertex
      if(j+1 == endRightIndex)
	{
	  while(rightChain->getVertex(j+1)[1] == rightGridChain->get_v_value(rightGridChainStartIndex+1))
	    j--;

	  monoTriangulation2(rightChain->getVertex(j+1),
			     rightGridChain->get_vertex(rightGridChainStartIndex+1),
			     rightChain,
			     j+2,
			     endRightIndex,
			     0, //a decrease chain
			     pStream);

	  temp = j+1;
	}

      stripOfFanRight(rightChain, temp, j+1, rightGridChain->getGrid(),
		      rightGridChain->getVlineIndex(rightGridChainStartIndex+1),
		      rightGridChain->getInnerIndex(rightGridChainStartIndex+1),
		      rightGridChain->getUlineIndex(rightGridChainStartIndex+1),
		      pStream,
		      0 //the grid line is below the trim line
		      );

    }


  stripOfFanRight(rightChain, i-1, beginRightIndex, rightGridChain->getGrid(),
		  rightGridChain->getVlineIndex(rightGridChainStartIndex),
		  rightGridChain->getInnerIndex(rightGridChainStartIndex+1),
		  rightGridChain->getUlineIndex(rightGridChainStartIndex),
		  pStream,
		  1 //the grid line is above the trm lines
		  );

  //monotone triangulate the remaining rightchain together with the
  //two vertices on the two grid v-lines
  Real vert[2][2];
  vert[0][0] = vert[1][0] = rightGridChain->getInner_u_value(rightGridChainStartIndex+1);
  vert[0][1] = rightGridChain->get_v_value(rightGridChainStartIndex);
  vert[1][1] = rightGridChain->get_v_value(rightGridChainStartIndex+1);

  monoTriangulation2(&vert[0][0], 
		     &vert[1][0],
		     rightChain,
		     i-1,
		     j+1,
		     0, ///a decreae chain
		     pStream);
}
		  
#endif    

void stripOfFanRight(vertexArray* rightChain, 
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
  if(! gridLineUp) /*trim line is above grid line, so trim vertices are going right when index increases*/
    for(k=0, i=smallIndex; i<=largeIndex; i++, k++)
      {
      trimVerts[k][0] = rightChain->getVertex(i)[0];
      trimVerts[k][1] = rightChain->getVertex(i)[1];
    }
  else
    for(k=0, i=largeIndex; i>=smallIndex; i--, k++)
      {
	trimVerts[k][0] = rightChain->getVertex(i)[0];
	trimVerts[k][1] = rightChain->getVertex(i)[1];
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










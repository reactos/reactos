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
#include "glimports.h"
#include "sampleComp.h"
#include "sampleCompTop.h"
#include "sampleCompBot.h"
#include "sampleCompRight.h"



#define max(a,b) ((a>b)? a:b)
#define min(a,b) ((a>b)? b:a)

void sampleConnectedComp(Real* topVertex, Real* botVertex,
		    vertexArray* leftChain, 
		    Int leftStartIndex, Int leftEndIndex,
		    vertexArray* rightChain,
		    Int rightStartIndex, Int rightEndIndex,
		    gridBoundaryChain* leftGridChain,
		    gridBoundaryChain* rightGridChain,
		    Int gridIndex1, Int gridIndex2,
		    Int up_leftCornerWhere,
		    Int up_leftCornerIndex,
		    Int up_rightCornerWhere,
		    Int up_rightCornerIndex,
		    Int down_leftCornerWhere,
		    Int down_leftCornerIndex,
		    Int down_rightCornerWhere,
		    Int down_rightCornerIndex,
		    primStream* pStream,
		    rectBlockArray* rbArray
		    )
{

 sampleCompLeft(topVertex, botVertex,
		leftChain,
		leftStartIndex,	leftEndIndex,
		rightChain,
		rightStartIndex, rightEndIndex,
		leftGridChain,
		gridIndex1,
		gridIndex2,
		up_leftCornerWhere,
		up_leftCornerIndex,
		down_leftCornerWhere,
		down_leftCornerIndex,
		pStream);


 sampleCompRight(topVertex, botVertex,
		 leftChain,
		 leftStartIndex, leftEndIndex,
		 rightChain,
		 rightStartIndex,
		 rightEndIndex,
		 rightGridChain,
		 gridIndex1, gridIndex2,
		 up_rightCornerWhere,
		 up_rightCornerIndex,
		 down_rightCornerWhere,
		 down_rightCornerIndex,
		 pStream);
		 

 sampleCompTop(topVertex,
		     leftChain,
		     leftStartIndex,
		     rightChain,
		     rightStartIndex,
		     leftGridChain,
		     rightGridChain,
		     gridIndex1,
		     up_leftCornerWhere,
		     up_leftCornerIndex,
		     up_rightCornerWhere,
		     up_rightCornerIndex,
		     pStream);

 sampleCompBot(botVertex,
		     leftChain,
		     leftEndIndex,
		     rightChain,
		     rightEndIndex,
		     leftGridChain,
		     rightGridChain,
		     gridIndex2,
		     down_leftCornerWhere,
		     down_leftCornerIndex,
		     down_rightCornerWhere,
		     down_rightCornerIndex,
		     pStream);


 //the center

 rbArray->insert(new rectBlock(leftGridChain, rightGridChain, gridIndex1, gridIndex2));

	
}

/*notice that we need rightChain because the 
 *corners could be on the rightChain.
 *here comp means component.
 */
void sampleCompLeft(Real* topVertex, Real* botVertex,
		    vertexArray* leftChain,
		    Int leftStartIndex, Int leftEndIndex,
		    vertexArray* rightChain,
		    Int rightStartIndex, Int rightEndIndex,
		    gridBoundaryChain* leftGridChain,
		    Int gridIndex1, Int gridIndex2,
		    Int up_leftCornerWhere,
		    Int up_leftCornerIndex,
		    Int down_leftCornerWhere,
		    Int down_leftCornerIndex,
		    primStream* pStream)
{
  /*find out whether there is a trim vertex which is
   *inbetween the top and bot grid lines or not.
   */
  Int midIndex1;
  Int midIndex2;
  Int gridMidIndex1 = 0, gridMidIndex2 = 0;
  //midIndex1: array[i] <= v, array[i-1] > v
  //midIndex2: array[i] >= v, array[i+1] < v
  // v(gridMidIndex1) >= v(midindex1) > v(gridMidIndex1+1)
  // v(gridMidIndex2-1) >= v(midIndex2) > v(gridMidIndex2) ??
  midIndex1 = leftChain->findIndexBelowGen(
					   leftGridChain->get_v_value(gridIndex1),
					   leftStartIndex,
					   leftEndIndex);

  midIndex2 = -1; /*initilization*/
  if(midIndex1<= leftEndIndex && gridIndex1<gridIndex2)
    if(leftChain->getVertex(midIndex1)[1] >= leftGridChain->get_v_value(gridIndex2))
      {
	midIndex2 = leftChain->findIndexAboveGen(
						 leftGridChain->get_v_value(gridIndex2),
						 midIndex1, //midIndex1 <= midIndex2.
						 leftEndIndex);
	gridMidIndex1  = leftGridChain->lookfor(leftChain->getVertex(midIndex1)[1],
						gridIndex1, gridIndex2);
	gridMidIndex2 = 1+leftGridChain->lookfor(leftChain->getVertex(midIndex2)[1],
					       gridMidIndex1, gridIndex2);	
      }


  /*to interprete the corner information*/
  Real* cornerTop;
  Real* cornerBot;
  Int cornerLeftStart;
  Int cornerLeftEnd;
  Int cornerRightUpEnd;
  Int cornerRightDownStart;
  if(up_leftCornerWhere == 0) /*left corner is on left chain*/
    {
      cornerTop = leftChain->getVertex(up_leftCornerIndex);
      cornerLeftStart = up_leftCornerIndex+1;
      cornerRightUpEnd = -1; /*no right*/
    }
  else if(up_leftCornerWhere == 1) /*left corner is on top*/
    {
      cornerTop = topVertex;
      cornerLeftStart = leftStartIndex;
      cornerRightUpEnd = -1; /*no right*/
    }
  else /*left corner is on right chain*/
    {
      cornerTop = topVertex;
      cornerLeftStart = leftStartIndex;
      cornerRightUpEnd = up_leftCornerIndex;
    }
  
  if(down_leftCornerWhere == 0) /*left corner is on left chain*/
    {
      cornerBot = leftChain->getVertex(down_leftCornerIndex);
      cornerLeftEnd = down_leftCornerIndex-1;
      cornerRightDownStart = rightEndIndex+1; /*no right*/
    }
  else if(down_leftCornerWhere == 1) /*left corner is on bot*/
    {
      cornerBot = botVertex;
      cornerLeftEnd = leftEndIndex;
      cornerRightDownStart = rightEndIndex+1; /*no right*/
    }
  else /*left corner is on the right chian*/
    {
      cornerBot = botVertex;
      cornerLeftEnd = leftEndIndex;
      cornerRightDownStart = down_leftCornerIndex;
    }
      



  /*sample*/
  if(midIndex2 >= 0) /*there is a trim point inbewteen grid lines*/
    {

      sampleLeftSingleTrimEdgeRegionGen(cornerTop, leftChain->getVertex(midIndex1),
					leftChain,
					cornerLeftStart,
					midIndex1-1,
					leftGridChain,
					gridIndex1,
					gridMidIndex1,
					rightChain,
					rightStartIndex,
					cornerRightUpEnd,
					0, //no right down section
					-1,
					pStream);

      sampleLeftSingleTrimEdgeRegionGen(leftChain->getVertex(midIndex2),
					cornerBot,
					leftChain,
					midIndex2+1,
					cornerLeftEnd,
					leftGridChain,
					gridMidIndex2,
					gridIndex2,
					rightChain,
					0, //no right up section
					-1,
					cornerRightDownStart,
					rightEndIndex,
					pStream);


      sampleLeftStripRecF(leftChain,
			  midIndex1,
			  midIndex2,
			  leftGridChain,
			  gridMidIndex1,
			  gridMidIndex2,
			  pStream);
    }
  else
    {
      sampleLeftSingleTrimEdgeRegionGen(cornerTop, cornerBot,
					leftChain,
					cornerLeftStart,
					cornerLeftEnd,
					leftGridChain,
					gridIndex1,
					gridIndex2,
					rightChain,
					rightStartIndex,
					cornerRightUpEnd,
					cornerRightDownStart,
					rightEndIndex,
					pStream);
    }					 
}
		    
void sampleLeftSingleTrimEdgeRegionGen(Real topVert[2], Real botVert[2],
				       vertexArray* leftChain,
				       Int leftStart,
				       Int leftEnd,
				       gridBoundaryChain* gridChain,
				       Int gridBeginIndex,
				       Int gridEndIndex,
				       vertexArray* rightChain,
				       Int rightUpBegin,
				       Int rightUpEnd,
				       Int rightDownBegin,
				       Int rightDownEnd,
				       primStream* pStream)
{
  Int i,j,k;

  /*creat an array to store all the up and down secments of the right chain,
   *and the left end grid points
   *
   *although vertex array is a dynamic array, but to gain efficiency,
   *it is better to initiliza the exact array size
   */
  vertexArray vArray(gridEndIndex-gridBeginIndex+1 +
		     max(0,rightUpEnd - rightUpBegin+1)+
		     max(0,rightDownEnd - rightDownBegin+1));
  
  /*append the vertices on the up section of thr right chain*/
  for(i=rightUpBegin; i<= rightUpEnd; i++)
    vArray.appendVertex(rightChain->getVertex(i));
  
  /*append the vertices of the left extremal grid points,
   *and at the same time, perform triangulation for the stair cases
   */
  vArray.appendVertex(gridChain->get_vertex(gridBeginIndex));

  for(k=1, i=gridBeginIndex+1; i<=gridEndIndex; i++, k++)
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

  /*then append all the vertices on the down section of the right chain*/
  for(i=rightDownBegin; i<= rightDownEnd; i++)
    vArray.appendVertex(rightChain->getVertex(i));  

  monoTriangulationRecGen(topVert, botVert,
			  leftChain, leftStart, leftEnd,
			  &vArray, 0, vArray.getNumElements()-1,
			  pStream);
		     
}










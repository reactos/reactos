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
#include "zlassert.h"
#include "sampleCompBot.h"
#include "sampleCompRight.h"

#define max(a,b) ((a>b)? a:b)

//return: index_mono, index_pass
//from [pass, mono] is strictly U-monotone
//from [corner, pass] is <u
// vertex[pass][0] >= u
//if everybost is <u, then pass = end+1.
//otherwise both mono and pass are meanng full and we have corner<=pass<=mono<=end
void findBotLeftSegment(vertexArray* leftChain, 
			Int leftEnd,
			Int leftCorner,
			Real u,
			Int& ret_index_mono,
			Int& ret_index_pass)
{
  Int i;

  assert(leftCorner <= leftEnd);
  for(i=leftCorner; i<= leftEnd; i++)
    if(leftChain->getVertex(i)[0] >= u)
      break;
  ret_index_pass = i;
  if(ret_index_pass <= leftEnd)
    {
      for(i=ret_index_pass; i< leftEnd; i++)
	{
	  if(leftChain->getVertex(i+1)[0] <= leftChain->getVertex(i)[0])
	    break;
	}
      ret_index_mono = i;
    }

}

void findBotRightSegment(vertexArray* rightChain, 
			 Int rightEnd,
			 Int rightCorner,
			 Real u,
			 Int& ret_index_mono,
			 Int& ret_index_pass)
{
  Int i;
  assert(rightCorner <= rightEnd);
  for(i=rightCorner; i<= rightEnd; i++)
    if(rightChain->getVertex(i)[0] <= u)
      break;



  ret_index_pass = i;

  if(ret_index_pass <= rightEnd)
    {
      for(i=ret_index_pass; i< rightEnd; i++)
	{
	  if(rightChain->getVertex(i+1)[0] >= rightChain->getVertex(i)[0])
	    break;
	}
      ret_index_mono = i;
    }
}


void sampleBotRightWithGridLinePost(Real* botVertex,
				    vertexArray* rightChain,
				    Int rightEnd,
				    Int segIndexMono,
				    Int segIndexPass,
				    Int rightCorner,
				    gridWrap* grid,
				    Int gridV,
				    Int leftU,
				    Int rightU,
				    primStream* pStream)
{
  //the possible section which is to the right of rightU
  if(segIndexPass > rightCorner) //from corner to pass-1 is > u.
    {
      Real *tempBot;
      if(segIndexPass <= rightEnd) //there is a point to the left of u
	tempBot = rightChain->getVertex(segIndexPass);
      else   //nothing is to the left of u.
	tempBot = botVertex; 
      Real tempTop[2];
      tempTop[0] = grid->get_u_value(rightU);
      tempTop[1] = grid->get_v_value(gridV);
      
      monoTriangulation2(tempTop, tempBot,
			 rightChain,
			 rightCorner,
			 segIndexPass-1,
			 0, // a decrease chain
			 pStream);
    }

  //the possible section which is strictly Umonotone
  if(segIndexPass <= rightEnd) //segIndex pass and mono exist
    {
      //if there are grid points which are to the left of botVertex
      //then we should use botVertex to form a fan with these points to
      //optimize the triangulation
      int do_optimize = 1;
      if(botVertex[0] <= grid->get_u_value(leftU))
	do_optimize = 0;
      else
	{
	  //we also have to make sure that botVertex is the left most vertex on the chain
	  int i;
	  for(i=segIndexMono; i<=rightEnd; i++)
	    if(rightChain->getVertex(i)[0] <= botVertex[0])
	      {
		do_optimize = 0;
		break;
	      }
	}

      if(do_optimize)
	{
	  //find midU so that grid->get_u_value(midU) <= botVertex[0]
	  //and               grid->get_u_value(midU) >  botVertex[0]
	  int midU = leftU;
	  while(grid->get_u_value(midU) <= botVertex[0])
	    {
	      midU++;
	      if(midU > rightU)
		break;
	    }
	  midU--;

	  grid->outputFanWithPoint(gridV, leftU, midU, botVertex, pStream);
	  stripOfFanRight(rightChain, segIndexMono, segIndexPass, grid, gridV, midU, rightU, pStream, 1);	  
	  Real tempTop[2];
	  tempTop[0] = grid->get_u_value(midU);
	  tempTop[1] = grid->get_v_value(gridV);
	  monoTriangulation2(tempTop, botVertex, rightChain, segIndexMono, rightEnd, 0, pStream);
	}
      else //not optimize
	{
	  stripOfFanRight(rightChain, segIndexMono, segIndexPass, grid, gridV, leftU, rightU, pStream, 1);
	  Real tempTop[2];
	  tempTop[0] = grid->get_u_value(leftU);
	  tempTop[1] = grid->get_v_value(gridV);
	  monoTriangulation2(tempTop, botVertex, rightChain, segIndexMono, rightEnd, 0, pStream);
	}
    }
  else //the botVertex forms a fan witht eh grid points
    grid->outputFanWithPoint(gridV, leftU, rightU, botVertex, pStream);
}
      
void sampleBotRightWithGridLine(Real* botVertex, 
				vertexArray* rightChain,
				Int rightEnd,
				Int rightCorner,
				gridWrap* grid,
				Int gridV,
				Int leftU,
				Int rightU,
				primStream* pStream)
{
  //if right chaain is empty, then there is only one bot vertex with
  //one grid line
  if(rightEnd<rightCorner){
    grid->outputFanWithPoint(gridV, leftU, rightU, botVertex, pStream);
    return;
  }

  Int segIndexMono = 0, segIndexPass;
  findBotRightSegment(rightChain,
		      rightEnd,
		      rightCorner,
		      grid->get_u_value(rightU),
		      segIndexMono,
		      segIndexPass);

  sampleBotRightWithGridLinePost(botVertex, 
				 rightChain,
				 rightEnd,
				 segIndexMono,
				 segIndexPass,
				 rightCorner,
				 grid,
				 gridV,
				 leftU,
				 rightU,
				 pStream);
}
  
 
void sampleBotLeftWithGridLinePost(Real* botVertex,
				   vertexArray* leftChain,
				   Int leftEnd,
				   Int segIndexMono,
				   Int segIndexPass,
				   Int leftCorner,
				   gridWrap* grid,
				   Int gridV,
				   Int leftU, 
				   Int rightU,
				   primStream* pStream)
{

  //the possible section which is to the left of leftU
  if(segIndexPass > leftCorner) //at least leftCorner is to the left of leftU
    {
      Real *tempBot; 
      if(segIndexPass <= leftEnd) //from corner to pass-1 is <u
	tempBot = leftChain->getVertex(segIndexPass);
      else //nothing is to the rigth of u
	tempBot = botVertex;
      Real tempTop[2];
      tempTop[0] = grid->get_u_value(leftU);
      tempTop[1] = grid->get_v_value(gridV);
      monoTriangulation2(tempTop, tempBot, leftChain, leftCorner, segIndexPass-1,
			 1, //a increase chain,
			 pStream);
    }
  //the possible section which is strictly Umonotone
  if(segIndexPass <= leftEnd) //segIndexpass and mono exist
    {
      stripOfFanLeft(leftChain, segIndexMono, segIndexPass, grid, gridV, leftU, rightU, pStream, 1);
      Real tempTop[2];
      tempTop[0] = grid->get_u_value(rightU);
      tempTop[1] = grid->get_v_value(gridV);

      monoTriangulation2(tempTop, botVertex, leftChain, segIndexMono, leftEnd, 
			 1,  //increase chain
			 pStream);
    }
  else //the botVertex forms a fan with the grid points
    {
      grid->outputFanWithPoint(gridV, leftU, rightU, botVertex, pStream);
    }

}

void sampleBotLeftWithGridLine(Real* botVertex,
			       vertexArray* leftChain,
			       Int leftEnd,
			       Int leftCorner,
			       gridWrap* grid,
			       Int gridV,
			       Int leftU,
			       Int rightU,
			       primStream* pStream)
{

  //if leftChain is empty, then there is only one botVertex with one grid line
  if(leftEnd< leftCorner){
    grid->outputFanWithPoint(gridV, leftU, rightU, botVertex, pStream);
    return;
  }

  Int segIndexPass, segIndexMono = 0;
  findBotLeftSegment(leftChain, leftEnd, leftCorner, grid->get_u_value(leftU), segIndexMono, segIndexPass);

  sampleBotLeftWithGridLinePost(botVertex,
			    leftChain,
			    leftEnd,
			    segIndexMono,
			    segIndexPass,
			    leftCorner,
			    grid,
			    gridV,
			    leftU, rightU, pStream);
}

//return 1 if separator exists, 0 otherwise
Int findBotSeparator(vertexArray* leftChain,
		     Int leftEnd,
		     Int leftCorner,
		     vertexArray* rightChain,
		     Int rightEnd,
		     Int rightCorner,
		     Int& ret_sep_left,
		     Int& ret_sep_right)
{
  Int oldLeftI, oldRightI, newLeftI, newRightI;
  Int i,j,k;
  Real leftMax /*= leftChain->getVertex(leftCorner)[0]*/;
  Real rightMin /*= rightChain->getVertex(rightCorner)[0]*/;
  if(leftChain->getVertex(leftCorner)[1] < rightChain->getVertex(rightCorner)[1])//leftlower
    {
      oldLeftI = leftCorner-1;
      oldRightI = rightCorner;
      leftMax = leftChain->getVertex(leftCorner)[0] - Real(1.0) ; //initilize to be left of leftCorner
      rightMin = rightChain->getVertex(rightCorner)[0]; 
    }
  else //rightlower
    {
      oldLeftI = leftCorner;
      oldRightI = rightCorner-1;
      leftMax = leftChain->getVertex(leftCorner)[0];
      rightMin = rightChain->getVertex(rightCorner)[0] + Real(1.0);
    }

  //i: the current working leftChain Index
  //j: the curent working right chian index
  //if(left(i) is lower than right(j), then the two chains above right(j) are separated.
  //else the two chains below left(i) are separated.
  i = leftCorner;
  j = rightCorner;
  while(1)
    {
      newLeftI = oldLeftI;
      newRightI = oldRightI;
      if(i> leftEnd) //left chain is doen , go through remaining right chain
	{
	  for(k=j+1; k<= rightEnd; k++)
	    {
	      if(rightChain->getVertex(k)[0] > leftMax) //no conflict
		{
		  //update oldRightI if necessary
		  if(rightChain->getVertex(k)[0] < rightMin)
		    {
		      rightMin = rightChain->getVertex(k)[0];
		      oldRightI = k;
		    }
		}
	      else //there is a conflict
		break; //the for-loop, above right(k+1) is separated: oldLeftI, oldRightI
	    }
	  break; //the while loop
	}
      else if(j > rightEnd) //right Chain is doen
	{
	  for(k=i+1; k<= leftEnd; k++)
	    {
	      if(leftChain->getVertex(k)[0] < rightMin) //no conflict
		{
		  //update oldLeftI if necessary
		  if(leftChain->getVertex(k)[0] > leftMax)
		    {
		      leftMax =  leftChain->getVertex(k)[0];
		      oldLeftI = k;
		    }
		}
	      else //there is a conflict
		break; //the for-loop, above left(k+1) is separated: oldLeftI, oldRightI
	    }
	  break; //the while loop
	}
      else if(leftChain->getVertex(i)[1] < rightChain->getVertex(j)[1]) //left lower
	{

	  if(leftChain->getVertex(i)[0] > leftMax) //update leftMax amd newLeftI
	    {
	      leftMax = leftChain->getVertex(i)[0];
	      newLeftI = i;
	    }
	  for(k=j+1; k<= rightEnd; k++) //update rightMin and newRightI;
	    {
	      if(rightChain->getVertex(k)[1] < leftChain->getVertex(i)[1]) //right gets lower
		break;
	      if(rightChain->getVertex(k)[0] < rightMin)
		{
		  rightMin = rightChain->getVertex(k)[0];
		  newRightI = k;
		}
	    }
	  j = k; //next working j, since j will he lower than i in next loop
	  if(leftMax >= rightMin) //there is a conflict
	    break;
	  else //still no conflict
	    {
	      oldLeftI = newLeftI;
	      oldRightI = newRightI;

	    }
	}
      else //right lower
	{
	  if(rightChain->getVertex(j)[0] < rightMin)
	    {
	      rightMin = rightChain->getVertex(j)[0];
	      newRightI = j;
	    }
	  for(k=i+1; k<= leftEnd; k++)
	    {
	      if(leftChain->getVertex(k)[1] < rightChain->getVertex(j)[1])
		break;
	      if(leftChain->getVertex(k)[0] > leftMax)
		{
		  leftMax = leftChain->getVertex(k)[0];
		  newLeftI = k;
		}
	    }
	  i=k; //nexct working i, since i will be lower than j next loop
	  if(leftMax >= rightMin) //there is conflict
	    break;
	  else //still no conflict
	    {
	      oldLeftI = newLeftI;
	      oldRightI = newRightI;
	    }
	}
    }//end of while loop
  //now oldLeftI and oldRight I are the desired separator index notice that they are not 
  //necessarily valid 
  if(oldLeftI < leftCorner || oldRightI < rightCorner)
    return 0; //no separator
  else
    {
      ret_sep_left = oldLeftI;
      ret_sep_right = oldRightI;
      return 1;
    }
}

void sampleCompBot(Real* botVertex,
		   vertexArray* leftChain,
		   Int leftEnd,
		   vertexArray* rightChain,
		   Int rightEnd,
		   gridBoundaryChain* leftGridChain,
		   gridBoundaryChain* rightGridChain,
		   Int gridIndex,
		   Int down_leftCornerWhere,
		   Int down_leftCornerIndex,
		   Int down_rightCornerWhere,
		   Int down_rightCornerIndex,
		   primStream* pStream)
{

  if(down_leftCornerWhere == 1 && down_rightCornerWhere == 1) //the bot is botVertex with possible grid points
    {

      leftGridChain->getGrid()->outputFanWithPoint(leftGridChain->getVlineIndex(gridIndex),
						   leftGridChain->getUlineIndex(gridIndex),
						  rightGridChain->getUlineIndex(gridIndex),
						   botVertex,
						   pStream);
      return;
    }
  else if(down_leftCornerWhere != 0)
    {

      Real* tempBot;
      Int tempRightEnd;
      if(down_leftCornerWhere == 1){
	tempRightEnd = rightEnd;
	tempBot = botVertex;
      }
      else
	{
	  tempRightEnd = down_leftCornerIndex-1;
	  tempBot = rightChain->getVertex(down_leftCornerIndex);
	}

      sampleBotRightWithGridLine(tempBot,
				 rightChain, 
				 tempRightEnd,
				 down_rightCornerIndex,
				 rightGridChain->getGrid(),
				 leftGridChain->getVlineIndex(gridIndex),
				 leftGridChain->getUlineIndex(gridIndex),
				 rightGridChain->getUlineIndex(gridIndex),
				 pStream);
    }
  else if(down_rightCornerWhere != 2)
    {

      Real* tempBot;
      Int tempLeftEnd;
      if(down_rightCornerWhere == 1){
	tempLeftEnd = leftEnd;
	tempBot = botVertex;
      }
      else //right corner is on left chain
	{
	  tempLeftEnd = down_rightCornerIndex-1;
	  tempBot = leftChain->getVertex(down_rightCornerIndex);	  
	}


      sampleBotLeftWithGridLine(tempBot, leftChain, tempLeftEnd, down_leftCornerIndex, 
				leftGridChain->getGrid(),
				leftGridChain->getVlineIndex(gridIndex),
				leftGridChain->getUlineIndex(gridIndex),
				rightGridChain->getUlineIndex(gridIndex),
				pStream);

    }
  else //down_leftCornereWhere == 0, down_rightCornerwhere == 2
    {
      sampleCompBotSimple(botVertex, 
			  leftChain,
			  leftEnd,
			  rightChain,
			  rightEnd,
			  leftGridChain,
			  rightGridChain,
			  gridIndex,
			  down_leftCornerWhere,
			  down_leftCornerIndex,
			  down_rightCornerWhere,
			  down_rightCornerIndex,
			  pStream);
      
      return;

#ifdef NOT_REACHABLE
      //the following code is trying to do some optimization, but not quite working. so it is not reachable, but leave it here for reference
      Int sep_left, sep_right;
      if(findBotSeparator(leftChain, leftEnd, down_leftCornerIndex,
			  rightChain, rightEnd, down_rightCornerIndex,
			  sep_left, sep_right)
	 )//separator exiosts
	{

	  if(leftChain->getVertex(sep_left)[0] >= leftGridChain->get_u_value(gridIndex) &&
	     rightChain->getVertex(sep_right)[0] <= rightGridChain->get_u_value(gridIndex))
	    {
	      Int gridSep;
	      Int segLeftMono, segLeftPass, segRightMono, segRightPass;
	      findBotLeftSegment(leftChain,
				 sep_left,
				 down_leftCornerIndex,
				 leftGridChain->get_u_value(gridIndex),
				 segLeftMono,
				 segLeftPass);
	      findBotRightSegment(rightChain,
				  sep_right,
				  down_rightCornerIndex,
				  rightGridChain->get_u_value(gridIndex),
				  segRightMono,
				  segRightPass);
	      if(leftChain->getVertex(segLeftMono)[1] <= rightChain->getVertex(segRightMono)[1])
		{
		  gridSep = rightGridChain->getUlineIndex(gridIndex);
		  while(leftGridChain->getGrid()->get_u_value(gridSep) > leftChain->getVertex(segLeftMono)[0])
		    gridSep--;
		}
	      else 
		{
		  gridSep = leftGridChain->getUlineIndex(gridIndex);
		  while(leftGridChain->getGrid()->get_u_value(gridSep) < rightChain->getVertex(segRightMono)[0])
		    gridSep++;
		}

	      sampleBotLeftWithGridLinePost(leftChain->getVertex(segLeftMono),
					    leftChain,
					    segLeftMono-1,
					    segLeftMono-1,
					    segLeftPass,
					    down_leftCornerIndex,
					    leftGridChain->getGrid(),
					    leftGridChain->getVlineIndex(gridIndex),
					    leftGridChain->getUlineIndex(gridIndex),
					    gridSep,
					    pStream);
	      sampleBotRightWithGridLinePost(rightChain->getVertex(segRightMono),
					     rightChain,
					     segRightMono-1,
					     segRightMono-1,
					     segRightPass,
					     down_rightCornerIndex,
					     rightGridChain->getGrid(),
					     rightGridChain->getVlineIndex(gridIndex),
					     gridSep,
					     rightGridChain->getUlineIndex(gridIndex),
					     pStream);
	      Real tempTop[2];
	      tempTop[0] = leftGridChain->getGrid()->get_u_value(gridSep);
	      tempTop[1] = leftGridChain->get_v_value(gridIndex);
	      monoTriangulationRecGen(tempTop, botVertex,
				      leftChain, segLeftMono, leftEnd,
				      rightChain, segRightMono, rightEnd,
				      pStream);
	    }//end if both sides have vertices inside the gridboundary points
	  else if(leftChain->getVertex(sep_left)[0] >= leftGridChain->get_u_value(gridIndex)) //left n right out
	    
	    {
	      Int segLeftMono, segLeftPass;
	      findBotLeftSegment(leftChain,
				 sep_left,
				 down_leftCornerIndex,
				 leftGridChain->get_u_value(gridIndex),
				 segLeftMono,
				 segLeftPass);				 
              assert(segLeftPass <= sep_left); //make sure there is a point to the right of u.
              monoTriangulation2(leftGridChain->get_vertex(gridIndex),
				 leftChain->getVertex(segLeftPass),
				 leftChain,
				 down_leftCornerIndex,
				 segLeftPass-1,
				 1, //a increase chain
				 pStream);
              stripOfFanLeft(leftChain, segLeftMono, segLeftPass, 
			     leftGridChain->getGrid(),
			     leftGridChain->getVlineIndex(gridIndex),
			     leftGridChain->getUlineIndex(gridIndex),
			     rightGridChain->getUlineIndex(gridIndex),
			     pStream,1 );
/*
	      sampleBotLeftWithGridLinePost(leftChain->getVertex(segLeftMono),
					    leftChain,
					    segLeftMono-1,
					    segLeftMono-1,
					    segLeftPass,
					    down_leftCornerIndex,
					    leftGridChain->getGrid(),
					    leftGridChain->getVlineIndex(gridIndex),
					    leftGridChain->getUlineIndex(gridIndex),
					    rightGridChain->getUlineIndex(gridIndex),
					    pStream);					    
*/
	      
	      monoTriangulationRecGen(rightGridChain->get_vertex(gridIndex),
				      botVertex,
				      leftChain, segLeftMono, leftEnd,
				      rightChain, down_rightCornerIndex, rightEnd,
				      pStream);
	    }//end left in right out
	  else if(rightChain->getVertex(sep_right)[0] <= rightGridChain->get_u_value(gridIndex))//left out right in
	    {	      
	      Int segRightMono, segRightPass;
	      findBotRightSegment(rightChain, sep_right, down_rightCornerIndex,
				  rightGridChain->get_u_value(gridIndex),
				  segRightMono,
				  segRightPass);

              assert(segRightPass <= sep_right); //make sure there is a point to the left of u.
              monoTriangulation2(rightGridChain->get_vertex(gridIndex),
				 rightChain->getVertex(segRightPass),
				 rightChain,
				 down_rightCornerIndex,
				 segRightPass-1,
				 0, // a decrease chain
				 pStream);

              stripOfFanRight(rightChain, segRightMono, segRightPass, 
			      rightGridChain->getGrid(),
			      rightGridChain->getVlineIndex(gridIndex),
			      leftGridChain->getUlineIndex(gridIndex),
			      rightGridChain->getUlineIndex(gridIndex),     
			      pStream, 1);


	      monoTriangulationRecGen(leftGridChain->get_vertex(gridIndex),
				      botVertex,
				      leftChain, down_leftCornerIndex, leftEnd,
				      rightChain, segRightMono, rightEnd,
				      pStream);	    	

	    }//end left out right in
	  else //left out, right out
	    {
	      sampleCompBotSimple(botVertex, 
				 leftChain,
				 leftEnd,
				 rightChain,
				 rightEnd,
				 leftGridChain,
				 rightGridChain,
				 gridIndex,
				 down_leftCornerWhere,
				 down_leftCornerIndex,
				 down_rightCornerWhere,
				 down_rightCornerIndex,
				 pStream);
	    
	    }//end leftout right out
	}//end if separator exists
      else //no separator
	{

	  sampleCompBotSimple(botVertex, 
			     leftChain,
			     leftEnd,
			     rightChain,
			     rightEnd,
			     leftGridChain,
			     rightGridChain,
			     gridIndex,
			     down_leftCornerWhere,
			     down_leftCornerIndex,
			     down_rightCornerWhere,
			     down_rightCornerIndex,
			     pStream);
	}
#endif
    }//end id 0 2
}//end if the functin

				 
void sampleCompBotSimple(Real* botVertex,
		   vertexArray* leftChain,
		   Int leftEnd,
		   vertexArray* rightChain,
		   Int rightEnd,
		   gridBoundaryChain* leftGridChain,
		   gridBoundaryChain* rightGridChain,
		   Int gridIndex,
		   Int down_leftCornerWhere,
		   Int down_leftCornerIndex,
		   Int down_rightCornerWhere,
		   Int down_rightCornerIndex,
		   primStream* pStream)  
{
  //the plan is to use monotriangulation algorithm.
  Int i,k;
  Real* ActualTop;
  Real* ActualBot;
  Int ActualLeftStart, ActualLeftEnd;
  Int ActualRightStart, ActualRightEnd;
  
  //creat an array to store the points on the grid line
  gridWrap* grid = leftGridChain->getGrid();
  Int gridV = leftGridChain->getVlineIndex(gridIndex);
  Int gridLeftU = leftGridChain->getUlineIndex(gridIndex);
  Int gridRightU = rightGridChain->getUlineIndex(gridIndex);
  Real2* gridPoints = (Real2*) malloc(sizeof(Real2) * (gridRightU - gridLeftU +1));
  assert(gridPoints);

  for(k=0, i=gridRightU; i>= gridLeftU; i--, k++)
    {
      gridPoints[k][0] = grid->get_u_value(i);
      gridPoints[k][1] = grid->get_v_value(gridV);
    }

  if(down_rightCornerWhere != 0) //rightCorner is not on lef
    ActualLeftEnd = leftEnd;
  else
    ActualLeftEnd = down_rightCornerIndex-1; //down_rightCornerIndex will be th actualBot
  
  if(down_leftCornerWhere != 0) //left corner is not on let chian
    ActualLeftStart = leftEnd+1; //meaning that there is no actual left section
  else
    ActualLeftStart = down_leftCornerIndex;
  
  vertexArray ActualLeftChain(max(0, ActualLeftEnd - ActualLeftStart +1) + gridRightU - gridLeftU +1);
  
  for(i=0; i<gridRightU - gridLeftU +1 ; i++)
    ActualLeftChain.appendVertex(gridPoints[i]);
  for(i=ActualLeftStart; i<= ActualLeftEnd; i++)
    ActualLeftChain.appendVertex(leftChain->getVertex(i));
  
  //determine ActualRightStart
  if(down_rightCornerWhere != 2) //right is not on right
    ActualRightStart = rightEnd +1; //meaning no section on right
  else
    ActualRightStart = down_rightCornerIndex;
  
  //determine actualrightEnd
  if(down_leftCornerWhere != 2) //left is not on right
    {

      ActualRightEnd = rightEnd;
    }
  else //left corner is on right 
    {
      ActualRightEnd = down_leftCornerIndex-1; //down_leftCornerIndex will be the bot

    }

  //actual bot
  if(down_rightCornerWhere == 2) 
    {
      if(down_leftCornerWhere == 2)
	ActualBot = rightChain->getVertex(down_leftCornerIndex);
      else
	ActualBot = botVertex;
    }
  else if(down_rightCornerWhere == 1) //right corner bot
    ActualBot = botVertex;
  else //down_rightCornerWhere == 0
    ActualBot = leftChain->getVertex(down_rightCornerIndex);
  
  ActualTop = gridPoints[0];
/*
printf("in bot simple, actual leftChain is \n");
ActualLeftChain.print();
printf("Actual Top = %f,%f\n", ActualTop[0],ActualTop[1]);
printf("Actual Bot = %f,%f\n", ActualBot[0],ActualBot[1]);
printf("Actual right start = %i, end=%i\n",ActualRightStart,   ActualRightEnd);
*/
  if(rightChain->getVertex(ActualRightStart)[1] == ActualTop[1])
    monoTriangulationRecGenOpt(rightChain->getVertex(ActualRightStart),
			    ActualBot,
			    &ActualLeftChain,
			    0, 
			    ActualLeftChain.getNumElements()-1,
			    rightChain,
			    ActualRightStart+1,
			    ActualRightEnd,
			    pStream);
  else
    monoTriangulationRecGenOpt(ActualTop, ActualBot, 
			  &ActualLeftChain,
			  1, //the first one is the top vertex
			  ActualLeftChain.getNumElements()-1,
			  rightChain,
			  ActualRightStart,
			  ActualRightEnd,
			  pStream);
  free(gridPoints);
}
  
  
				 


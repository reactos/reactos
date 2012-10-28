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
#include "sampleCompTop.h"
#include "sampleCompRight.h"

#define max(a,b) ((a>b)? a:b)

//return : index_small, and index_large,
//from [small, large] is strictly U-monotne,
//from [large+1, end] is <u
//and vertex[large][0] is >= u
//if eveybody is <u, the large = start-1.
//otherwise both large and small are meaningful and we have start<=small<=large<=end
void findTopLeftSegment(vertexArray* leftChain,
                        Int leftStart,
                        Int leftEnd,
                        Real u,
                        Int& ret_index_small,
                        Int& ret_index_large
                        )
{
  Int i;
  assert(leftStart <= leftEnd);
  for(i=leftEnd; i>= leftStart; i--)
    {
      if(leftChain->getVertex(i)[0] >= u)
        break;
    }
  ret_index_large = i;
  if(ret_index_large >= leftStart)
    {
      for(i=ret_index_large; i>leftStart; i--)
        {
          if(leftChain->getVertex(i-1)[0] <= leftChain->getVertex(i)[0])
            break;
        }
      ret_index_small = i;
    }
}

void findTopRightSegment(vertexArray* rightChain,
                         Int rightStart,
                         Int rightEnd,
                         Real u,
                         Int& ret_index_small,
                         Int& ret_index_large)
{
  Int i;
  assert(rightStart<=rightEnd);
  for(i=rightEnd; i>=rightStart; i--)
    {
      if(rightChain->getVertex(i)[0] <= u)
        break;
    }
  ret_index_large = i;
  if(ret_index_large >= rightStart)
    {
      for(i=ret_index_large; i>rightStart;i--)
        {
          if(rightChain->getVertex(i-1)[0] >= rightChain->getVertex(i)[0])           
	    break;
        }
      ret_index_small = i;
    }
}


void sampleTopRightWithGridLinePost(Real* topVertex,
				   vertexArray* rightChain,
				   Int rightStart,
				   Int segIndexSmall,
				   Int segIndexLarge,
				   Int rightEnd,
				   gridWrap* grid,
				   Int gridV,
				   Int leftU,
				   Int rightU,
				   primStream* pStream)
{
  //the possible section which is to the right of rightU
  if(segIndexLarge < rightEnd)
    {
      Real *tempTop;
      if(segIndexLarge >= rightStart)
        tempTop = rightChain->getVertex(segIndexLarge);
      else
        tempTop = topVertex;
      Real tempBot[2];
      tempBot[0] = grid->get_u_value(rightU);
      tempBot[1] = grid->get_v_value(gridV);
monoTriangulationRecGenOpt(tempTop, tempBot,
			   NULL, 1,0,
			   rightChain, segIndexLarge+1, rightEnd,
			   pStream);
/*
      monoTriangulation2(tempTop, tempBot,
                         rightChain,
                         segIndexLarge+1,
                         rightEnd,
                         0, //a decrease  chian
                         pStream);
*/

    }
  
  //the possible section which is strictly Umonotone
  if(segIndexLarge >= rightStart)
    {
      stripOfFanRight(rightChain, segIndexLarge, segIndexSmall, grid, gridV, leftU, rightU, pStream, 0);
      Real tempBot[2];
      tempBot[0] = grid->get_u_value(leftU);
      tempBot[1] = grid->get_v_value(gridV);
      monoTriangulation2(topVertex, tempBot, rightChain, rightStart, segIndexSmall, 0, pStream);
    }
  else //the topVertex forms a fan with the grid points
    grid->outputFanWithPoint(gridV, leftU, rightU, topVertex, pStream);
}

void sampleTopRightWithGridLine(Real* topVertex,
                                vertexArray* rightChain,
                                Int rightStart,
                                Int rightEnd,
                                gridWrap* grid,
                                Int gridV,
                                Int leftU,
                                Int rightU,
                                primStream* pStream
                                )
{
  //if right chian is empty, then there is only one topVertex with one grid line
  if(rightEnd < rightStart){
    grid->outputFanWithPoint(gridV, leftU, rightU, topVertex, pStream);
    return;
  }

  Int segIndexSmall = 0, segIndexLarge;
  findTopRightSegment(rightChain,
                      rightStart,
                      rightEnd,
                      grid->get_u_value(rightU),
                      segIndexSmall,
                      segIndexLarge
                      );
  sampleTopRightWithGridLinePost(topVertex, rightChain,
				 rightStart,
				 segIndexSmall,
				 segIndexLarge,
				 rightEnd,
				 grid,
				 gridV,
				 leftU,
				 rightU,
				 pStream);
}


void sampleTopLeftWithGridLinePost(Real* topVertex,
				   vertexArray* leftChain,
				   Int leftStart,
				   Int segIndexSmall,
				   Int segIndexLarge,
				   Int leftEnd,
				   gridWrap* grid,
				   Int gridV,
				   Int leftU,
				   Int rightU,
				   primStream* pStream)
{
  //the possible section which is to the left of leftU

  if(segIndexLarge < leftEnd)
    {
      Real *tempTop;
      if(segIndexLarge >= leftStart)
        tempTop = leftChain->getVertex(segIndexLarge);
      else
        tempTop = topVertex;
      Real tempBot[2];
      tempBot[0] = grid->get_u_value(leftU);
      tempBot[1] = grid->get_v_value(gridV);

      monoTriangulation2(tempTop, tempBot,
                         leftChain,
                         segIndexLarge+1,
                         leftEnd,
                         1, //a increase  chian
                         pStream);
    }

  //the possible section which is strictly Umonotone
  if(segIndexLarge >= leftStart)
    {
      //if there are grid points which are to the right of topV,
      //then we should use topVertex to form a fan with these points to
      //optimize the triangualtion
      int do_optimize=1;
      if(topVertex[0] >= grid->get_u_value(rightU))
	do_optimize = 0;
      else
	{
	  //we also have to make sure that topVertex are the right most vertex
          //on the chain.
	  int i;
	  for(i=leftStart; i<=segIndexSmall; i++)
	    if(leftChain->getVertex(i)[0] >= topVertex[0])
	      {
		do_optimize = 0;
		break;
	      }
	}

      if(do_optimize)
	{
	  //find midU so that grid->get_u_value(midU) >= topVertex[0]
	  //and               grid->get_u_value(midU-1) < topVertex[0]
	  int midU=rightU;
	  while(grid->get_u_value(midU) >= topVertex[0])
	    {
	      midU--;
	      if(midU < leftU)
		break;
	    }
	  midU++;

	  grid->outputFanWithPoint(gridV, midU, rightU, topVertex, pStream);
	  stripOfFanLeft(leftChain, segIndexLarge, segIndexSmall, grid, gridV, leftU, midU, pStream, 0);
	  Real tempBot[2];
	  tempBot[0] = grid->get_u_value(midU);
	  tempBot[1] = grid->get_v_value(gridV);
	  monoTriangulation2(topVertex, tempBot, leftChain, leftStart, segIndexSmall, 1, pStream);      
	}
      else //not optimize
	{

	  stripOfFanLeft(leftChain, segIndexLarge, segIndexSmall, grid, gridV, leftU, rightU, pStream, 0);
	  Real tempBot[2];
	  tempBot[0] = grid->get_u_value(rightU);
	  tempBot[1] = grid->get_v_value(gridV);
	  monoTriangulation2(topVertex, tempBot, leftChain, leftStart, segIndexSmall, 1, pStream);
	}
    }
  else //the topVertex forms a fan with the grid points
    grid->outputFanWithPoint(gridV, leftU, rightU, topVertex, pStream);  
}				   
				   

void sampleTopLeftWithGridLine(Real* topVertex,
                                vertexArray* leftChain,
                                Int leftStart,
                                Int leftEnd,
                                gridWrap* grid,
                                Int gridV,
                                Int leftU,
                                Int rightU,
                                primStream* pStream
                                )
{
  Int segIndexSmall = 0, segIndexLarge;
  //if left chain is empty, then there is only one top vertex with one grid 
  //  line
  if(leftEnd < leftStart) {
    grid->outputFanWithPoint(gridV, leftU, rightU, topVertex, pStream);
    return;
  }
  findTopLeftSegment(leftChain,
                      leftStart,
                      leftEnd,
                      grid->get_u_value(leftU),
                      segIndexSmall,
                      segIndexLarge
                      );
  sampleTopLeftWithGridLinePost(topVertex,
				leftChain,
				leftStart,
				segIndexSmall,
				segIndexLarge,
				leftEnd,
				grid,
				gridV,
			        leftU,
				rightU,
				pStream);   
}
 
                
//return 1 if saprator exits, 0 otherwise
Int findTopSeparator(vertexArray* leftChain,
		     Int leftStartIndex,
		     Int leftEndIndex,
		     vertexArray* rightChain,
		     Int rightStartIndex,
		     Int rightEndIndex,
		     Int& ret_sep_left,
		     Int& ret_sep_right)
{
  
  Int oldLeftI, oldRightI, newLeftI, newRightI;
  Int i,j,k;
  Real leftMax /*= leftChain->getVertex(leftEndIndex)[0]*/;
  Real rightMin /*= rightChain->getVertex(rightEndIndex)[0]*/;
  if(leftChain->getVertex(leftEndIndex)[1] > rightChain->getVertex(rightEndIndex)[1]) //left higher
    {
      oldLeftI = leftEndIndex+1;
      oldRightI = rightEndIndex;
      leftMax =  leftChain->getVertex(leftEndIndex)[0] - Real(1.0); //initilza to left of leftU
      rightMin = rightChain->getVertex(rightEndIndex)[0];
    }
  else
    {
      oldLeftI = leftEndIndex;
      oldRightI = rightEndIndex+1;
      leftMax =  leftChain->getVertex(leftEndIndex)[0]; 
      rightMin = rightChain->getVertex(rightEndIndex)[0] + Real(1.0);      
    }
  
  //i: the current working leftChain index, 
  //j: the current working rightChain index,
  //if left(i) is higher than right(j), then the two chains beloew right(j) are separated.
  //else the two chains below left(i) are separeated.
  i=leftEndIndex; 
  j=rightEndIndex;
  while(1)
    {
      newLeftI = oldLeftI;
      newRightI = oldRightI;

      if(i<leftStartIndex) //left chain is done, go through remining right chain.
	{
	  for(k=j-1; k>= rightStartIndex; k--)
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
	      else  //there is a conflict
		break; //the for-loop. below right(k-1) is seperated: oldLeftI, oldRightI.
	    }
	  break; //the while loop
	}
      else if(j<rightStartIndex) //rightChain is done
	{
	  for(k=i-1; k>= leftStartIndex; k--)
	    {
	      if(leftChain->getVertex(k)[0] < rightMin) //no conflict
		{
		  //update oldLeftI if necessary
		  if(leftChain->getVertex(k)[0] > leftMax)
		    {
		      leftMax = leftChain->getVertex(k)[0];
		      oldLeftI = k;
		    }
		}
	      else //there is a conflict
		break; //the for loop
	    }
	  break; //the while loop
	}
      else if(leftChain->getVertex(i)[1] > rightChain->getVertex(j)[1]) //left hgiher
	{
	  if(leftChain->getVertex(i)[0] > leftMax) //update leftMax and newLeftI.
	    {
	      leftMax = leftChain->getVertex(i)[0];	     
	      newLeftI = i;
	    }
	  for(k=j-1; k>= rightStartIndex; k--) //update rightMin and newRightI.
	    {
	      if(rightChain->getVertex(k)[1] > leftChain->getVertex(i)[1])
		break;
	      if(rightChain->getVertex(k)[0] < rightMin)
		{
		  rightMin = rightChain->getVertex(k)[0];
		  newRightI = k;
		}
	    }
	  j = k; //next working j, since j will be higher than i in next loop
	  if(leftMax >= rightMin) //there is a conflict
	    break;
	  else //still no conflict
	    {
	      oldLeftI = newLeftI;
	      oldRightI = newRightI;
	    }
	}
      else //right higher
	{
	  if(rightChain->getVertex(j)[0] < rightMin)
	    {
	      rightMin = rightChain->getVertex(j)[0];
	      newRightI = j;
	    }
	  for(k=i-1; k>= leftStartIndex; k--)
	    {
	      if(leftChain->getVertex(k)[1] > rightChain->getVertex(j)[1])
		break;
	      if(leftChain->getVertex(k)[0] > leftMax)
		{
		  leftMax = leftChain->getVertex(k)[0];
		  newLeftI = k;
		}
	    }
	  i = k; //next working i, since i will be higher than j next loop
	  
	  if(leftMax >= rightMin) //there is a conflict
	    break;
	  else //still no conflict
	    {
	      oldLeftI = newLeftI;
	      oldRightI = newRightI;
	    }
	}
    }//end of while loop
  //now oldLeftI and oldRightI are the desired separeator index, notice that there are not necessarily valid
  if(oldLeftI > leftEndIndex || oldRightI > rightEndIndex)
    return 0;
  else
    {
      ret_sep_left = oldLeftI;
      ret_sep_right = oldRightI;
      return 1;
    }
}

        
void sampleCompTop(Real* topVertex,
                   vertexArray* leftChain,
                   Int leftStartIndex,
                   vertexArray* rightChain,
                   Int rightStartIndex,
                   gridBoundaryChain* leftGridChain,
                   gridBoundaryChain* rightGridChain,
                   Int gridIndex1,
                   Int up_leftCornerWhere,
                   Int up_leftCornerIndex,
                   Int up_rightCornerWhere,
                   Int up_rightCornerIndex,
                   primStream* pStream)
{
  if(up_leftCornerWhere == 1 && up_rightCornerWhere == 1) //the top is topVertex with possible grid points
    {
      leftGridChain->getGrid()->outputFanWithPoint(leftGridChain->getVlineIndex(gridIndex1),
						   leftGridChain->getUlineIndex(gridIndex1),
						   rightGridChain->getUlineIndex(gridIndex1),
						   topVertex,
						   pStream);
      return;
    }

  else if(up_leftCornerWhere != 0)
    {
      Real* tempTop;
      Int tempRightStart;
      if(up_leftCornerWhere == 1){
	tempRightStart = rightStartIndex;
	tempTop = topVertex;
      }
      else
	{
	  tempRightStart = up_leftCornerIndex+1;
	  tempTop = rightChain->getVertex(up_leftCornerIndex);
	}
      sampleTopRightWithGridLine(tempTop, rightChain, tempRightStart, up_rightCornerIndex,
				 rightGridChain->getGrid(),
				 leftGridChain->getVlineIndex(gridIndex1),
				 leftGridChain->getUlineIndex(gridIndex1),
				 rightGridChain->getUlineIndex(gridIndex1),
				 pStream);
    }
  else if(up_rightCornerWhere != 2)
    {
      Real* tempTop;
      Int tempLeftStart;
      if(up_rightCornerWhere == 1)
	{
	  tempLeftStart = leftStartIndex;
	  tempTop = topVertex;
	}
      else //0
	{
	  tempLeftStart = up_rightCornerIndex+1;
	  tempTop = leftChain->getVertex(up_rightCornerIndex);
	}
/*
      sampleTopLeftWithGridLine(tempTop, leftChain, tempLeftStart, up_leftCornerIndex,
				leftGridChain->getGrid(),
				 leftGridChain->getVlineIndex(gridIndex1),
				 leftGridChain->getUlineIndex(gridIndex1),
				 rightGridChain->getUlineIndex(gridIndex1),
				 pStream);
*/
      sampleCompTopSimple(topVertex,
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
    }
  else //up_leftCornerWhere == 0, up_rightCornerWhere == 2.
    {
      sampleCompTopSimple(topVertex,
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
      return;
#ifdef NOT_REACHABLE //code is not reachable, for test purpose only
      //the following code is trying to do some optimization, but not quite working, also see sampleCompBot.C:
      Int sep_left, sep_right;
      if(findTopSeparator(leftChain,
			  leftStartIndex,
			  up_leftCornerIndex,
			  rightChain,
			  rightStartIndex,
			  up_rightCornerIndex,
			  sep_left,
			  sep_right)
	 ) //separator exists
	{

	  if( leftChain->getVertex(sep_left)[0] >= leftGridChain->get_u_value(gridIndex1) &&
	     rightChain->getVertex(sep_right)[0] <= rightGridChain->get_u_value(gridIndex1))
	    {
	      Int gridSep;
	      Int segLeftSmall, segLeftLarge, segRightSmall, segRightLarge;
	      Int valid=1; //whether the gridStep is valid or not.
	      findTopLeftSegment(leftChain,
				 sep_left,
				 up_leftCornerIndex,
				 leftGridChain->get_u_value(gridIndex1),
				 segLeftSmall,
				 segLeftLarge);
	      findTopRightSegment(rightChain,
				 sep_right,
				 up_rightCornerIndex,
				 rightGridChain->get_u_value(gridIndex1),
				 segRightSmall,
				 segRightLarge);
	      if(leftChain->getVertex(segLeftSmall)[1] >= rightChain->getVertex(segRightSmall)[1])
		{
		  gridSep = rightGridChain->getUlineIndex(gridIndex1);
		  while(leftGridChain->getGrid()->get_u_value(gridSep) > leftChain->getVertex(segLeftSmall)[0])
		    gridSep--;
		  if(segLeftSmall<segLeftLarge)
		    if(leftGridChain->getGrid()->get_u_value(gridSep) < leftChain->getVertex(segLeftSmall+1)[0])
		      {
			valid = 0;
		      }
		}
	      else
		{
		  gridSep = leftGridChain->getUlineIndex(gridIndex1);
		  while(leftGridChain->getGrid()->get_u_value(gridSep) < rightChain->getVertex(segRightSmall)[0])
		    gridSep++;
		  if(segRightSmall<segRightLarge)
		    if(leftGridChain->getGrid()->get_u_value(gridSep) > rightChain->getVertex(segRightSmall+1)[0])
		      {
			valid = 0;
		      }
		}		
		  
	      if(! valid)
		{
		  sampleCompTopSimple(topVertex,
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
		}
	      else
		{
		  sampleTopLeftWithGridLinePost(leftChain->getVertex(segLeftSmall),
						leftChain,
						segLeftSmall+1,
						segLeftSmall+1,
						segLeftLarge,
						up_leftCornerIndex,
						leftGridChain->getGrid(),
						leftGridChain->getVlineIndex(gridIndex1),
						leftGridChain->getUlineIndex(gridIndex1),
						gridSep,
						pStream);
		  sampleTopRightWithGridLinePost(rightChain->getVertex(segRightSmall),
						 rightChain,
						 segRightSmall+1,
						 segRightSmall+1,
						 segRightLarge,
						 up_rightCornerIndex,
						 leftGridChain->getGrid(),
						 leftGridChain->getVlineIndex(gridIndex1),
						 gridSep,
						 rightGridChain->getUlineIndex(gridIndex1),
						 pStream);
		  Real tempBot[2];
		  tempBot[0] = leftGridChain->getGrid()->get_u_value(gridSep);
		  tempBot[1] = leftGridChain->get_v_value(gridIndex1);
		  monoTriangulationRecGen(topVertex, tempBot,
					  leftChain, leftStartIndex, segLeftSmall,
					  rightChain, rightStartIndex, segRightSmall,
					  pStream);
		}
	    }//end if both sides have vetices inside the gridboundary points
	  else if(leftChain->getVertex(sep_left)[0] >= leftGridChain->get_u_value(gridIndex1)) //left is in, right is nout
	    {

	      Int segLeftSmall, segLeftLarge;
	      findTopLeftSegment(leftChain,
				 sep_left,
				 up_leftCornerIndex,
				 leftGridChain->get_u_value(gridIndex1),
				 segLeftSmall,
				 segLeftLarge);	      
	      assert(segLeftLarge >= sep_left); 
              monoTriangulation2(leftChain->getVertex(segLeftLarge),
				 leftGridChain->get_vertex(gridIndex1),
				 leftChain,
				 segLeftLarge+1,
				 up_leftCornerIndex,
				 1, //a increase chain,
				 pStream);

	      stripOfFanLeft(leftChain, segLeftLarge, segLeftSmall, 
			     leftGridChain->getGrid(),
			     leftGridChain->getVlineIndex(gridIndex1),
			     leftGridChain->getUlineIndex(gridIndex1),
			     rightGridChain->getUlineIndex(gridIndex1),
			     pStream, 0);


	      monoTriangulationRecGen(topVertex, rightGridChain->get_vertex(gridIndex1),
				      leftChain, leftStartIndex, segLeftSmall,
				      rightChain, rightStartIndex, up_rightCornerIndex,
				      pStream);	    
	    }//end left in right out
	  else if(rightChain->getVertex(sep_right)[0] <= rightGridChain->get_u_value(gridIndex1))
	    {
	      Int segRightSmall, segRightLarge;
	      findTopRightSegment(rightChain,
				 sep_right,
				 up_rightCornerIndex,
				 rightGridChain->get_u_value(gridIndex1),
				 segRightSmall,
				 segRightLarge);
	      assert(segRightLarge>=sep_right);
	      monoTriangulation2(rightChain->getVertex(segRightLarge),
				 rightGridChain->get_vertex(gridIndex1),
				 rightChain,
				 segRightLarge+1,
				 up_rightCornerIndex,
				 0, //a decrease chain
				 pStream);
	      stripOfFanRight(rightChain, segRightLarge, segRightSmall,
			      rightGridChain->getGrid(),
			      rightGridChain->getVlineIndex(gridIndex1),
			      leftGridChain->getUlineIndex(gridIndex1),
			      rightGridChain->getUlineIndex(gridIndex1),
			      pStream, 0);


	      monoTriangulationRecGen(topVertex, leftGridChain->get_vertex(gridIndex1),
				      leftChain, leftStartIndex, up_leftCornerIndex,
				      rightChain, rightStartIndex,segRightSmall,
				      pStream);

	    }//end left out rigth in
	  else //left out , right out
	    {

	      sampleCompTopSimple(topVertex,
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
	    }//end leftout, right out
	}//end if separator exixts.
      else //no separator
	{

	  sampleCompTopSimple(topVertex,
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
	}
#endif
    }//end if 0,2
}//end if the function

		   
static void sampleCompTopSimpleOpt(gridWrap* grid,
				   Int gridV,
				   Real* topVertex, Real* botVertex,
				   vertexArray* inc_chain, Int inc_current, Int inc_end,
				   vertexArray* dec_chain, Int dec_current, Int dec_end,
				   primStream* pStream)
{
  if(gridV <= 0 || dec_end<dec_current || inc_end <inc_current)
    {
      monoTriangulationRecGenOpt(topVertex, botVertex,
				 inc_chain, inc_current, inc_end,
				 dec_chain, dec_current, dec_end,
				 pStream);
      return;
    }
  if(grid->get_v_value(gridV+1) >= topVertex[1])
    {
      monoTriangulationRecGenOpt(topVertex, botVertex,
				 inc_chain, inc_current, inc_end,
				 dec_chain, dec_current, dec_end,
				 pStream);
      return;
    }      
 Int i,j,k;
  Real currentV = grid->get_v_value(gridV+1);
  if(inc_chain->getVertex(inc_end)[1] <= currentV &&
     dec_chain->getVertex(dec_end)[1] < currentV)
    {
      //find i bottom up so that inc_chain[i]<= curentV and inc_chain[i-1] > currentV, 
      //find j botom up so that dec_chain[j] < currentV and dec_chain[j-1] >= currentV
      for(i=inc_end; i >= inc_current; i--) 
	{
	  if(inc_chain->getVertex(i)[1] > currentV)
	    break;
	}
      i++;
      for(j=dec_end; j >= dec_current; j--)
	{
	  if(dec_chain->getVertex(j)[1] >= currentV)
	    break;
	}
      j++;
     if(inc_chain->getVertex(i)[1] <= dec_chain->getVertex(j)[1])
       {
	 //find the k so that dec_chain[k][1] < inc_chain[i][1]
	 for(k=j; k<=dec_end; k++)
	   {
	     if(dec_chain->getVertex(k)[1] < inc_chain->getVertex(i)[1])
	       break;
	   }
         //we know that dec_chain[j][1] >= inc_chian[i][1]
	 //we know that dec_chain[k-1][1]>=inc_chain[i][1]
         //we know that dec_chian[k][1] < inc_chain[i][1]
         //find l in [j, k-1] so that dec_chain[l][0] 0 is closest to
         // inc_chain[i]
         int l;
         Real tempI = Real(j);
         Real tempMin = (Real)fabs(inc_chain->getVertex(i)[0] - dec_chain->getVertex(j)[0]);
         for(l=j+1; l<= k-1; l++)
	   {
	     if(fabs(inc_chain->getVertex(i)[0] - dec_chain->getVertex(l)[0])
		<= tempMin)
	       {
		 tempMin = (Real)fabs(inc_chain->getVertex(i)[0] - dec_chain->getVertex(l)[0]);
		 tempI = (Real)l;
	       }
	   }
	 //inc_chain[i] and dec_chain[tempI] are connected.
	 monoTriangulationRecGenOpt(dec_chain->getVertex((int)tempI),
				    botVertex,
				    inc_chain, i, inc_end,
				    dec_chain, (int)(tempI+1), dec_end,
				    pStream);
	 //recursively do the rest
	 sampleCompTopSimpleOpt(grid,
				gridV+1,
				topVertex, inc_chain->getVertex(i),
				inc_chain, inc_current, i-1,
				dec_chain, dec_current, (int)tempI,
				pStream);
       }
      else
	{
	  //find the k so that inc_chain[k][1] <= dec_chain[j][1]
	  for(k=i; k<=inc_end; k++)
	    {
	      if(inc_chain->getVertex(k)[1] <= dec_chain->getVertex(j)[1])
		break;
	    }
	  //we know that inc_chain[i] > dec_chain[j]
	  //we know that inc_chain[k-1][1] > dec_chain[j][1]
	  //we know that inc_chain[k][1] <= dec_chain[j][1]
	  //so we find l between [i,k-1] so that 
	  //inc_chain[l][0] is the closet to dec_chain[j][0]
	  int tempI = i;
	  int l;
	  Real tempMin = (Real)fabs(inc_chain->getVertex(i)[0] - dec_chain->getVertex(j)[0]);
	  for(l=i+1; l<=k-1; l++)
	    {
	      if(fabs(inc_chain->getVertex(l)[0] - dec_chain->getVertex(j)[0]) <= tempMin)
		{
		  tempMin = (Real)fabs(inc_chain->getVertex(l)[0] - dec_chain->getVertex(j)[0]);
		  tempI = l;
		}
	    }				 	      

	  //inc_chain[tempI] and dec_chain[j] are connected

	  monoTriangulationRecGenOpt(inc_chain->getVertex(tempI),
				     botVertex,
				     inc_chain, tempI+1, inc_end,
				     dec_chain, j, dec_end,
				     pStream);

	  //recurvesily do the rest
	  sampleCompTopSimpleOpt(grid, gridV+1,
				 topVertex, dec_chain->getVertex(j),
				 inc_chain, inc_current, tempI,
				 dec_chain, dec_current, j-1,
				 pStream);
	}	    				   	       
    }
  else //go to the next higher gridV
    {
      sampleCompTopSimpleOpt(grid,
			     gridV+1,
			     topVertex, botVertex,
			     inc_chain, inc_current, inc_end,
			     dec_chain, dec_current, dec_end,
			     pStream);
    }
}
			  
void sampleCompTopSimple(Real* topVertex,
                   vertexArray* leftChain,
                   Int leftStartIndex,
                   vertexArray* rightChain,
                   Int rightStartIndex,
                   gridBoundaryChain* leftGridChain,
                   gridBoundaryChain* rightGridChain,
                   Int gridIndex1,
                   Int up_leftCornerWhere,
                   Int up_leftCornerIndex,
                   Int up_rightCornerWhere,
                   Int up_rightCornerIndex,
                   primStream* pStream)
{
  //the plan is to use monotriangulation algortihm.
  Int i,k;
  Real* ActualTop;
  Real* ActualBot;
  Int ActualLeftStart, ActualLeftEnd;
  Int ActualRightStart, ActualRightEnd;
  
  //creat an array to store the points on the grid line
  gridWrap* grid = leftGridChain->getGrid();
  Int gridV = leftGridChain->getVlineIndex(gridIndex1);
  Int gridLeftU = leftGridChain->getUlineIndex(gridIndex1);
  Int gridRightU = rightGridChain->getUlineIndex(gridIndex1);

  Real2* gridPoints = (Real2*) malloc(sizeof(Real2) * (gridRightU - gridLeftU +1));
  assert(gridPoints);
  
  for(k=0, i=gridRightU; i>= gridLeftU; i--, k++)
    {
      gridPoints[k][0] = grid->get_u_value(i);
      gridPoints[k][1] = grid->get_v_value(gridV);
    }

  if(up_leftCornerWhere != 2)
    ActualRightStart = rightStartIndex;
  else
    ActualRightStart = up_leftCornerIndex+1; //up_leftCornerIndex will be the ActualTop
  
  if(up_rightCornerWhere != 2) //right corner is not on right chain
    ActualRightEnd = rightStartIndex-1; //meaning that there is no actual rigth section
  else
    ActualRightEnd = up_rightCornerIndex;
  
  vertexArray ActualRightChain(max(0, ActualRightEnd-ActualRightStart+1) + gridRightU-gridLeftU+1);

  for(i=ActualRightStart; i<= ActualRightEnd; i++)
    ActualRightChain.appendVertex(rightChain->getVertex(i));
  for(i=0; i<gridRightU-gridLeftU+1; i++)
    ActualRightChain.appendVertex(gridPoints[i]);    

  //determine ActualLeftEnd
  if(up_leftCornerWhere != 0)
    ActualLeftEnd = leftStartIndex-1;
  else
    ActualLeftEnd = up_leftCornerIndex;
  
  if(up_rightCornerWhere != 0)
    ActualLeftStart = leftStartIndex;
  else
    ActualLeftStart = up_rightCornerIndex+1; //up_rightCornerIndex will be the actual top
  
  if(up_leftCornerWhere == 0) 
    {
      if(up_rightCornerWhere == 0)
	ActualTop = leftChain->getVertex(up_rightCornerIndex);
      else
	ActualTop = topVertex;
    }
  else if(up_leftCornerWhere == 1) 
    ActualTop = topVertex;
  else  //up_leftCornerWhere == 2
    ActualTop = rightChain->getVertex(up_leftCornerIndex);
  
  ActualBot = gridPoints[gridRightU - gridLeftU];
  



  if(leftChain->getVertex(ActualLeftEnd)[1] == ActualBot[1])
    {
/*
    monoTriangulationRecGenOpt(ActualTop, leftChain->getVertex(ActualLeftEnd),
			    leftChain,
			    ActualLeftStart, ActualLeftEnd-1,
			    &ActualRightChain,
			    0,
			    ActualRightChain.getNumElements()-1,
			    pStream);
*/
   
    sampleCompTopSimpleOpt(grid, gridV,
			   ActualTop, leftChain->getVertex(ActualLeftEnd),
			    leftChain,
			    ActualLeftStart, ActualLeftEnd-1,
			    &ActualRightChain,
			    0,
			    ActualRightChain.getNumElements()-1,
			    pStream);
    
  }
  else
    {
/*
    monoTriangulationRecGenOpt(ActualTop, ActualBot, leftChain,
			  ActualLeftStart, ActualLeftEnd,
			  &ActualRightChain,
			  0, ActualRightChain.getNumElements()-2, //the last is the bot.
			  pStream);
*/
   	  
    sampleCompTopSimpleOpt(grid, gridV,
			   ActualTop, ActualBot, leftChain,
			  ActualLeftStart, ActualLeftEnd,
			  &ActualRightChain,
			  0, ActualRightChain.getNumElements()-2, //the last is the bot.
			  pStream);

   
  }

  free(gridPoints);
      
}		  
						   

/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
*/

#ifndef _SAMPLEMONOPOLY_H
#define _SAMPLEMONOPOLY_H

#include "monoTriangulation.h"
#include "gridWrap.h"
#include "rectBlock.h"


void  triangulateXYMono(Int n_upper, Real upperVerts[][2],
		       Int n_lower, Real lowerVerts[][2],
		       primStream* pStream);

void stripOfFanLeft(vertexArray* leftChain, 
		     Int largeIndex,
		     Int smallIndex,
		     gridWrap* grid,
		     Int vlineIndex,
		     Int ulineSmallIndex,
		     Int ulineLargeIndex,
		     primStream* pStream,
		    Int gridLineUp
		     );
void sampleLeftOneGridStep(vertexArray* leftChain,
		  Int beginLeftIndex,
		  Int endLeftIndex,
		  gridBoundaryChain* leftGridChain,
		  Int leftGridChainStartIndex,
		  primStream* pStream
		  );

void sampleLeftSingleTrimEdgeRegion(Real upperVert[2], Real lowerVert[2],
				    gridBoundaryChain* gridChain,
				    Int beginIndex,
				    Int endIndex,
				    primStream* pStream);

void sampleLeftStripRec(vertexArray* leftChain,
		     Int topLeftIndex,
		     Int botLeftIndex,
		     gridBoundaryChain* leftGridChain,
		     Int leftGridChainStartIndex,
		     Int leftGridChainEndIndex,
			primStream* pStream
		     );

void sampleLeftStrip(vertexArray* leftChain,
		     Int topLeftIndex,
		     Int botLeftIndex,
		     gridBoundaryChain* leftGridChain,
		     Int leftGridChainStartIndex,
		     Int leftGridChainEndIndex,
		     primStream* pStream
		     );

void findLeftGridIndices(directedLine* topEdge, Int firstGridIndex, Int lastGridIndex, gridWrap* grid,  Int* ret_indices, Int* ret_inner);

void findRightGridIndices(directedLine* topEdge, Int firstGridIndex, Int lastGridIndex, gridWrap* grid,  Int* ret_indices, Int* ret_inner);

void sampleMonoPoly(directedLine* polygon, gridWrap* grid, Int ulinear, Int vlinear, primStream *pStream, rectBlockArray* rbArray);

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
		       rectBlockArray* rbArray
		       );

void sampleLeftStripRecF(vertexArray* leftChain,
		     Int topLeftIndex,
		     Int botLeftIndex,
		     gridBoundaryChain* leftGridChain,
		     Int leftGridChainStartIndex,
		     Int leftGridChainEndIndex,
			primStream* pStream
		     );

void findUpCorners(Real *topVertex, 
		   vertexArray *leftChain, 
		   Int leftChainStartIndex, Int leftChainEndIndex,
		   vertexArray *rightChain, 
		   Int rightChainStartIndex, Int rightChainEndIndex,
		   Real v,
		   Real uleft,
		   Real uright,
		   Int& ret_leftCornerWhere, 
		   Int& ret_leftCornerIndex,
		   Int& ret_rightCornerWhere,
		   Int& ret_rightCornerIndex
		   );
void findDownCorners(Real *botVertex, 
		   vertexArray *leftChain, Int leftChainStartIndex, Int leftChainEndIndex,
		   vertexArray *rightChain, Int rightChainStartIndex, Int rightChainEndIndex,
		   Real v,
		   Real uleft,
		   Real uright,
		   Int& ret_leftCornerWhere,
		   Int& ret_leftCornerIndex,
		   Int& ret_rightCornerWhere,
		   Int& ret_rightCornerIndex
		   );
void findNeck(vertexArray *leftChain, Int botLeftIndex, 
	      vertexArray *rightChain, Int botRightIndex, 
	      Int& leftLastIndex, /*left point of the neck*/
	      Int& rightLastIndex /*right point of the neck*/
	      );

Int findNeckF(vertexArray *leftChain, Int botLeftIndex,
	       vertexArray *rightChain, Int botRightIndex,
	       gridBoundaryChain* leftGridChain,
	       gridBoundaryChain* rightGridChain,
	       Int gridStartIndex,
	       Int& neckLeft, 
	       Int& neckRight);

void findTopAndBot(directedLine* polygon, 
		   directedLine*& topV, 
		   directedLine*& botV);
void findGridChains(directedLine* top, directedLine* bot, 
		    gridWrap* grid,
		    gridBoundaryChain*& leftGridChain,
		    gridBoundaryChain*& rightGridChain);
void toVertexArrays(directedLine* topV, directedLine* botV, vertexArray& leftChain, vertexArray& rightChain);

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
		 Int bot_rightCornerIndex);

Int checkMiddle(vertexArray* chain, Int begin, Int end, 
		Real vup, Real vbelow);

#endif


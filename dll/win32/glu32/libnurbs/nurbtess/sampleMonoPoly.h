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
** $Date$ $Revision: 1.1 $
*/
/*
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/sampleMonoPoly.h,v 1.1 2004/02/02 16:39:15 navaraf Exp $
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


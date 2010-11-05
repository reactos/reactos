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
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/sampleCompRight.h,v 1.1 2004/02/02 16:39:14 navaraf Exp $
*/

#ifndef _SAMPLECOMPRIGHT_H
#define _SAMPLECOMPRIGHT_H

#define NOT_TAKEOUT

#include "sampleMonoPoly.h"
void stripOfFanRight(vertexArray* rightChain,
		    Int largeIndex,
		    Int smallIndex,
		    gridWrap* grid,
		    Int vlineIndex,
		    Int ulineSmallIndex,
		    Int ulineLargeIndex,
		    primStream* pStream,
		    Int gridLineUp /*1 if grid line is above the trim lines */
		     );

#ifdef NOT_TAKEOUT
void sampleRightStripRecF(vertexArray* rightChain,
		     Int topRightIndex,
		     Int botRightIndex,
		     gridBoundaryChain* rightGridChain,
		     Int rightGridChainStartIndex,
		     Int rightGridChainEndIndex,
		     primStream* pStream
		     );
//the degenerate case of sampleRightOneGridStep
void sampleRightOneGridStepNoMiddle(vertexArray* rightChain,
				    Int beginRightIndex,
				    Int endRightIndex,
				    gridBoundaryChain* rightGridChain,
				    Int rightGridChainStartIndex,
				    primStream* pStream);
//sampling the right area in between two grid lines
//shape: _________|
void sampleRightOneGridStep(vertexArray* rightChain,
			    Int beginRightIndex,
			    Int endRightIndex,
			    gridBoundaryChain* rightGridChain,
			    Int rightGridChainStartIndex,
			    primStream* pStream);
void sampleRightSingleTrimEdgeRegion(Real upperVert[2], Real lowerVert[2],
				     gridBoundaryChain* gridChain,
				     Int beginIndex,
				     Int endIndex,
				     primStream* pStream);
//the degenerate case of sampleRightOneGridStep
void sampleRightOneGridStepNoMiddle(vertexArray* rightChain,
				    Int beginRightIndex,
				    Int endRightIndex,
				    gridBoundaryChain* rightGridChain,
				    Int rightGridChainStartIndex,
				    primStream* pStream);

void sampleCompRight(Real* topVertex, Real* botVertex,
                    vertexArray* leftChain,
                    Int leftStartIndex, Int leftEndIndex,
                    vertexArray* rightChain,
                    Int rightStartIndex, Int rightEndIndex,
                    gridBoundaryChain* rightGridChain,
                    Int gridIndex1, Int gridIndex2,
                    Int up_leftCornerWhere,
                    Int up_leftCornerIndex,
                    Int down_leftCornerWhere,
                    Int down_leftCornerIndex,
                    primStream* pStream);

void sampleRightSingleTrimEdgeRegionGen(Real topVert[2], Real botVert[2],
                                       vertexArray* rightChain,
                                       Int rightStart,
                                       Int rightEnd,
                                       gridBoundaryChain* gridChain,
                                       Int gridBegindex,
                                       Int gridEndIndex,
                                       vertexArray* leftChain,
                                       Int leftUpBegin,
                                       Int leftUpEnd,
                                       Int leftDownBegin,
                                       Int leftDownEnd,
                                       primStream* pStream);
#endif

#endif



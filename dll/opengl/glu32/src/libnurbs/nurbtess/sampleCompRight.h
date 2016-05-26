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



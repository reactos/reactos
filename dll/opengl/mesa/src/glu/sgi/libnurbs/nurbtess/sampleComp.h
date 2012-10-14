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

#ifndef _SAMPLECOMP_H
#define _SAMPLECOMP_H

#include "sampleMonoPoly.h"
#include "rectBlock.h"

void sampleConnectedComp(Real* topVertex, Real* botVertex,
		    vertexArray* leftChain, 
		    Int leftStartIndex, Int botLeftIndex,
		    vertexArray* rightChain,
		    Int rightStartIndex, Int botRightIndex,
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
		    );

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
		    primStream* pStream);

void sampleLeftSingleTrimEdgeRegionGen(Real topVert[2], Real botVert[2],
				       vertexArray* leftChain,
				       Int leftStart,
				       Int leftEnd,
				       gridBoundaryChain* gridChain,
				       Int gridBegindex,
				       Int gridEndIndex,
				       vertexArray* rightChain,
				       Int rightUpBegin,
				       Int rightUpEnd,
				       Int rightDownBegin,
				       Int rightDownEnd,
				       primStream* pStream);
		    
#endif

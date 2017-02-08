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
 *partitionY.h:
 *partition a polygon into a Y-monotone polygon:
 * A polygon is Y-monotone if the boundary can be split into two polygon chains
 *A and B such that each chain is Y-monotonic that is the intersection of any
 *horizontal line intersects each chain has at most one connected componenets
 * (empty, single point or a single line).
 * 
 * A vertex is a cusp if both its ajacent vertices are either at or above v, 
 *or both at or below v. In addition, at least one of the ajacent verteces is
 *strictly below or above v. 
 * A vertex is a relex vertex if the internals angle is strictly greater than 
 *180. In other words, if the signed area is negative:
 *(x1, y1), (x2, y2), (x3, y3) are the three vertices along a polygon, the 
 *order is such that left hand side is inside the polygon. Then (x2,y2) is
 *reflex if: 
 *  (x2-x1, y2-y1) cross (x3-x1, y3-y1) <0.
 *A vertex is an interior cusp if it is a cusp and a reflex.
 *A vertex is an exterior cusp if it is a cusp but not a reflex.
 *
 */

#ifndef _PARTITIONY_H
#define _PARTITIONY_H

#include "directedLine.h"

/*whether an edge is below a vertex*/
Int isBelow(directedLine *v, directedLine *e);

/*whether an edge is above a vertex*/
Int isAbove(directedLine *v, directedLine *e);

/*not-cusp,
 *inerior cusp
 *exterior cusp
 */
Int cuspType(directedLine *v);

/*used in trapezoidalization*/
typedef struct sweepRange{
  directedLine *left;
  Int leftType; /*either a vertex (leftType=0) or an edge (leftType =1) */
  directedLine *right;
  Int rightType; /*either a vertex (rightType=0) or an edge (rightType =1) */  
} sweepRange;

sweepRange* sweepRangeMake(directedLine* left, Int leftType,
			   directedLine* right, Int rightType);

void sweepRangeDelete(sweepRange* range);
Int sweepRangeEqual(sweepRange* sr1, sweepRange* sr2);

/*given a set of simple polygons where the interior 
 *is decided by left-hand principle,
 *return a range (sight) for each vertex. This is called
 *Trapezoidalization.
 */
void sweepY(Int nVertices, directedLine **sortedVerteces, sweepRange** ret_ranges);


directedLine* partitionY(directedLine *polygons, sampledLine **retSampledLines);

void findDiagonals(Int total_num_edges, directedLine** sortedVertices, sweepRange** ranges, Int& num_diagonals, directedLine** diagonal_vertices);

directedLine** DBGfindDiagonals(directedLine *polygons, Int& num_diagonals);

#endif

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
 *180. In other words, if the the signed area is negative:
 *(x1, y1), (x2, y2), (x3, y3) are the three vertices along a polygon, the 
 *order is such that left hand side is inside the polygon. Then (x2,y2) is
 *reflex if: 
 *  (x2-x1, y2-y1) cross (x3-x1, y3-y1) <0.
 *A vertex is an interior cusp if it is a cusp and a reflex.
 *A vertex is an exterior cusp if it is a cusp but not a reflex.
 *
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/partitionY.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
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

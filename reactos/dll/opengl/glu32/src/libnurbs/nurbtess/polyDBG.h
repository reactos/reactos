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

#ifndef _POLYDBG_H
#define _POLYDBG_H

#include "definitions.h"
#include "directedLine.h"
#include "monoTriangulation.h"

Int DBG_edgesIntersectGen(Real A[2], Real B[2], Real C[2], Real D[2]);
Int DBG_intersectChain(vertexArray* chain, Int start, Int end, Real A[2], Real B[2]);
  

Int DBG_edgesIntersect(directedLine* l1, directedLine* l2);
Int DBG_polygonSelfIntersect(directedLine* poly);
Int DBG_edgeIntersectPoly(directedLine* edge, directedLine* poly);
Int DBG_polygonsIntersect(directedLine* p1, directedLine* p2);
Int DBG_polygonListIntersect(directedLine* pList);

Int DBG_isCounterclockwise(directedLine* poly);
Int DBG_rayIntersectEdge(Real v0[2], Real dx, Real dy, Real v10[2], Real v1[2], Real v2[2]);
Int DBG_pointInsidePoly(Real v[2], directedLine* poly);
Int DBG_enclosingPolygons(directedLine* poly, directedLine* list);
void  DBG_reverse(directedLine* poly);
Int DBG_check(directedLine *polyList);

Int DBG_isConvex(directedLine *poly);
Int DBG_is_U_direction(directedLine *poly);
Int DBG_is_U_monotone(directedLine* poly);

directedLine* DBG_cutIntersectionAllPoly(directedLine* list);
directedLine* DBG_cutIntersectionPoly(directedLine *polygon, int& cutOccur);

sampledLine*  DBG_collectSampledLinesAllPoly(directedLine *polygonList);

void  DBG_collectSampledLinesPoly(directedLine *polygon, sampledLine*& retHead, sampledLine*& retTail);

#endif

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
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/monoTriangulation.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
*/

#ifndef _MONO_TRIANGULATION_H
#define _MONO_TRIANGULATION_H

#include "definitions.h"
#include "primitiveStream.h"
#include "directedLine.h"

class Backend;
class Arc;
typedef Arc *Arc_ptr;

class reflexChain{
  Real2 *queue; 
  /*the order of the polygon vertices: either q[0],q[1].., or 
   * q[n-1], q[n-2], ..., q[0]
   *this order determines the interior of the polygon, so it
   *also used to determines whether a chain is reflex or convex
   */
  Int isIncreasing; 
  Int index_queue;
  Int size_queue; /*allocated size*/
  
public:
  reflexChain(Int size, Int isIncreasing);
  ~reflexChain();
  
  void insert(Real u, Real v);
  void insert(Real v[2]);
  
  void processNewVertex(Real v[2], primStream* pStream);
  void outputFan(Real v[2], primStream* pStream);

  void processNewVertex(Real v[2], Backend* backend);
  void outputFan(Real v[2], Backend* backend);

  void print();
};

/*dynamic array of pointers to reals.
 *Intended to store an array of (u,v).
 *Notice that it doesn't allocate or dealocate the space
 *for the (u,v) themselfs. So it assums that someone else 
 *is taking care of them, while this class only plays with
 *the pointers.
 */
class vertexArray{
  Real** array;
  Int index;
  Int size;
public:
  vertexArray(Int s);
  vertexArray(Real vertices[][2], Int nVertices);
  ~vertexArray();
  void appendVertex(Real* ptr); /*the content (pointed by ptr is NOT copied*/
  Real* getVertex(Int i) {return array[i];}
  Real** getArray() {return array;}
  Int getNumElements() {return index;}
  Int findIndexAbove(Real v);
  Int findIndexAboveGen(Real v, Int startIndex, Int EndIndex);
  Int findIndexBelowGen(Real v, Int startIndex, Int EndIndex);
  Int findIndexStrictBelowGen(Real v, Int startIndex, Int EndIndex);
  Int findIndexFirstAboveEqualGen(Real v, Int startIndex, Int endIndex);
  Int skipEqualityFromStart(Real v, Int start, Int end);
  //return i such that fron [i+1, end] is strictly U-monotone (left to right
  Int findDecreaseChainFromEnd(Int begin, Int end);
  void print();
};

void monoTriangulation(directedLine* monoPolygon, primStream* pStream);

void monoTriangulationRec(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			  primStream* pStream);

void monoTriangulationRec(directedLine* inc_chain, Int inc_index, 
			  directedLine* dec_chain, Int dec_index, 
			  directedLine* topVertex, Int top_index,
			  directedLine* botVertex,
			  primStream* pStream);

/*the chain could be increasing or decreasing, although we use the
 * name inc_chain.
 *the argument  is_increase_chain indicates whether this chain
 *is increasing (left chain in V-monotone case) or decreaing (right chain
 *in V-monotone case).
 */
void monoTriangulation2(Real* topVertex, Real* botVertex, 
			vertexArray* inc_chain, Int inc_smallIndex,
			Int inc_largeIndex,
			Int is_increase_chain,
			primStream* pStream);
void monoTriangulationRecGen(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current, Int inc_end,
			  vertexArray* dec_chain, Int dec_current, Int dec_end,
			  primStream* pStream); 

void monoTriangulationRecGenOpt(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current, Int inc_end,
			  vertexArray* dec_chain, Int dec_current, Int dec_end,
			  primStream* pStream); 

void triangulateXYMonoTB(Int n_left, Real** leftVerts,
		       Int n_right, Real** rightVerts,
		       primStream* pStream);

void monoTriangulationRecGenTBOpt(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current, Int inc_end,
			  vertexArray* dec_chain, Int dec_current, Int dec_end,
			  primStream* pStream);

void monoTriangulationRecOpt(Real* topVertex, Real* botVertex, 
			     vertexArray* left_chain, Int left_current,
			     vertexArray* right_chain, Int right_current,
			     primStream* pStream);

void monoTriangulationRecFunGen(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current, Int inc_end,
			  vertexArray* dec_chain, Int dec_current, Int dec_end,
			  Int  (*compFun)(Real*, Real*),
			  primStream* pStream);

void monoTriangulationRecFun(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			   Int (*compFun)(Real*, Real*),
			  primStream* pStream);
void monoTriangulationFun(directedLine* monoPolygon, 
			  Int (*compFun)(Real*, Real*), primStream* pStream);




void monoTriangulationRec(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			  Backend* backend);

void monoTriangulationFunBackend(Arc_ptr loop, Int (*compFun)(Real*, Real*), Backend* backend);

void monoTriangulationRecFunBackend(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			  Int  (*compFun)(Real*, Real*),
			  Backend* backend);

void monoTriangulationOpt(directedLine* poly, primStream* pStream);

#endif





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

#ifndef _MONO_TRIANGULATION_H
#define _MONO_TRIANGULATION_H

#include "definitions.h"
#include "primitiveStream.h"
#include "directedLine.h"
#include "arc.h"

class Backend;

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





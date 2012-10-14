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

#ifndef _MONO_CHAIN_H
#define _MONO_CHAIN_H

#include "directedLine.h"
#include "partitionY.h"

class monoChain;

class monoChain{
  directedLine* chainHead;
  directedLine* chainTail;
  monoChain* next;
  monoChain* prev;
  monoChain* nextPolygon; //a list of polygons
  
  //cached informatin
  //bounding box
  Real minX, maxX, minY, maxY;
  Int isIncrease;
  
  //for efficiently comparing two chains

  directedLine* current;
  
public:
  monoChain(directedLine* cHead, directedLine* cTail);
  ~monoChain() {}
  
  inline  void setNext(monoChain* n) {next = n;}
  inline void setPrev(monoChain* p) {prev = p;}
  inline void setNextPolygon(monoChain* np) {nextPolygon = np;}
  inline monoChain* getNext() {return next;}
  inline monoChain* getPrev() {return prev;}
  inline directedLine* getHead() {return chainHead;}
  inline directedLine* getTail() {return chainTail;}
  
  inline void resetCurrent() { current = ((isIncrease==1)? chainHead:chainTail);}

  void deleteLoop();
  void deleteLoopList();

  //insert a new chain between prev and this
  void insert(monoChain* nc);

  Int numChainsSingleLoop();
  Int numChainsAllLoops();
  monoChain** toArrayAllLoops(Int& num_chains);
  Int toArraySingleLoop(monoChain** array, Int index);

  Int isKey;
  Real keyY; //the current horizotal line  
  Real chainIntersectHoriz(Real y); //updates current incrementally for efficiency
  directedLine* find(Real y);//find dline so that y intersects dline.

  void printOneChain();
  void printChainLoop();
  void printAllLoops();
    
};

monoChain* directedLineLoopToMonoChainLoop(directedLine* loop);
monoChain* directedLineLoopListToMonoChainLoopList(directedLine* list);
Int MC_sweepY(Int nVertices, monoChain** sortedVertices, sweepRange** ret_ranges);

void MC_findDiagonals(Int total_num_edges, monoChain** sortedVertices,
		   sweepRange** ranges, Int& num_diagonals, 
		   directedLine** diagonal_vertices);

directedLine* MC_partitionY(directedLine *polygons, sampledLine **retSampledLines);

#endif

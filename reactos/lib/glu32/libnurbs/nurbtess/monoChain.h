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
** $Date: 2004/02/02 16:39:13 $ $Revision: 1.1 $
*/
/*
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/monoChain.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
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

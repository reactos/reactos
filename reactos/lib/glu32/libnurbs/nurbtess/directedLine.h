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
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/directedLine.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
*/

#ifndef _DIRECTEDLINE_H
#define _DIRECTEDLINE_H

#include "definitions.h"
#include "sampledLine.h"

enum {INCREASING, DECREASING};

class directedLine {
  short direction; /*INCREASING or DECREASING*/
  sampledLine* sline;
  directedLine* next; /*double linked list*/
  directedLine* prev; /*double linked list*/

  /*in case we need a list of polygons each 
   *consisting of a double linked list
   */
  directedLine* nextPolygon; 

  /*optimization make cutoff polygon faster*/
/*  directedLine* prevPolygon;*/

  Int rootBit; /*1 if this is a root of the polygon, set by setRootBit*/
               /*and reset by resetRootBit()*/

  directedLine* rootLink; /*fast root-finding*/



public:
  directedLine(short dir, sampledLine* sl);
  directedLine();
  ~directedLine();

  void init(short dir, sampledLine* sl);
  
  Real* head(); /*points[0] if INCREASING, points[n-1] otherwise*/
  Real* tail(); /*points[n-1] if INCREASING, points[0] otherwise*/
  Real* getVertex(Int i); /*points[i] if INCREASING, points[n-1-i] otherwise*/
  Int get_npoints() {return sline->get_npoints();}
  directedLine* getPrev() {return prev;}
  directedLine* getNext() {return next;}
  directedLine* getNextPolygon()  {return nextPolygon;}
  sampledLine*  getSampledLine()  {return sline;}

  short getDirection(){return direction;}
  void putDirection(short dir) {direction = dir;}
  void putPrev(directedLine *p) {prev = p;}
  void putNext(directedLine *p) {next = p;}

  /*insert a new line between prev and this*/
  void insert(directedLine* nl);

  /*delete all the polygons following the link: nextPolygon.
   *notice that sampledLine is not deleted. The caller is
   *responsible for that
   */
  void  deletePolygonList();
  void  deleteSinglePolygon();

  void  deleteSinglePolygonWithSline(); //also delete sanmpled line
  void  deletePolygonListWithSline(); //also delete sanmpled line

  void deleteSingleLine(directedLine* dline);
  directedLine* deleteDegenerateLines();
  directedLine* deleteDegenerateLinesAllPolygons();
  directedLine* cutIntersectionAllPoly(int& cutOccur);

  /*check to see if the list forms a closed polygon
   *return 1 if yes
   */
  short isPolygon(); 
  
  Int compInY(directedLine* nl);
  Int compInX(directedLine* nl);

  /*return an array of pointers.
   *the 
   */
  directedLine** sortAllPolygons();

  Int numEdges();
  Int numEdgesAllPolygons();
  Int numPolygons();

  /*check if the head of this edge is connected to 
   *the tail of the prev
   */
  short isConnected();

  Real polyArea();

  void printSingle();
  void printList();
  void printAllPolygons();
  void writeAllPolygons(char* filename);
  

  /*insert a polygon: using nextPolygon*/
  directedLine* insertPolygon(directedLine* newpolygon);
  directedLine* cutoffPolygon(directedLine *p);

  Int toArraySinglePolygon(directedLine** array, Int index);
  directedLine** toArrayAllPolygons(Int& total_num_edges);

  void connectDiagonal(directedLine* v1, directedLine* v2, 
			   directedLine** ret_p1, 
			   directedLine** ret_p2,
			   sampledLine** generatedLine, directedLine* list);

  /*generate two slines
   */
  void connectDiagonal_2slines(directedLine* v1, directedLine* v2, 
			   directedLine** ret_p1, 
			   directedLine** ret_p2,
			   directedLine* list);

  Int samePolygon(directedLine* v1, directedLine* v2);
  void setRootBit() {rootBit = 1;}
  void resetRootBit() {rootBit = 0;}
  directedLine* findRoot();

  void rootLinkSet(directedLine* r) {rootLink = r;}
  directedLine* rootLinkFindRoot();

  //the chain from begin to end is deleted (the space is deallocated)
  //and a new edge(which connectes the head of begin and the tail of end)
  // is inserted. The new polygon is returned.
  //notice that "this" is arbitrary
  directedLine* deleteChain(directedLine* begin, directedLine* end);
};

directedLine*  readAllPolygons(char* filename);

extern Int compV2InY(Real A[2], Real B[2]);
extern Int compV2InX(Real A[2], Real B[2]);

#endif


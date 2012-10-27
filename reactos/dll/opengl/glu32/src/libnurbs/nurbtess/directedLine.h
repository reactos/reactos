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


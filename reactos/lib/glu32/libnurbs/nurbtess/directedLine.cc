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
*/
/*
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "glimports.h"
#include "zlassert.h"

#include "quicksort.h"
#include "directedLine.h"
#include "polyDBG.h"

#ifdef __WATCOMC__
#pragma warning 726 10
#endif

//we must return the newLine
directedLine* directedLine::deleteChain(directedLine* begin, directedLine* end)
{
  if(begin->head()[0] ==  end->tail()[0] &&
     begin->head()[1] ==  end->tail()[1]
     )
    {
      directedLine *ret = begin->prev;
      begin->prev->next = end->next;
      end->next->prev = begin->prev;
      delete begin->sline;
      delete end->sline;
      delete begin;
      delete end;

      return ret;
    }

  directedLine* newLine;
  sampledLine* sline = new sampledLine(begin->head(), end->tail());
  newLine =  new directedLine(INCREASING, sline);
  directedLine *p = begin->prev;
  directedLine *n = end->next;
  p->next = newLine;
  n->prev = newLine;
  newLine->prev = p;
  newLine->next = n;

  delete begin->sline;
  delete end->sline;
  delete begin;
  delete end;
  return newLine;
}


void directedLine::deleteSingleLine(directedLine* dline)
{
  //make sure that dline->prev->tail is the same as
  //dline->next->head. This is for numerical erros.
  //for example, if we delete a line which is almost degeneate
  //within (epsilon), then we want to make that the polygon after deletion
  //is still a valid polygon

  dline->next->head()[0] =  dline->prev->tail()[0];
  dline->next->head()[1] =  dline->prev->tail()[1];

  dline->prev->next = dline->next;
  dline->next->prev = dline->prev;

  delete dline;

}

static Int myequal(Real a[2], Real b[2])
{
  /*
  if(a[0]==b[0] && a[1] == b[1])
    return 1;
  else
    return 0;
    */


  if(fabs(a[0]-b[0]) < 0.00001 &&
     fabs(a[1]-b[1]) < 0.00001)
    return 1;
  else
    return 0;

}

directedLine* directedLine::deleteDegenerateLines()
{
  //if there is only one edge or two edges, don't do anything
  if(this->next == this)
    return this;
  if(this->next == this->prev)
    return this;

  //find a nondegenerate line
  directedLine* temp;
  directedLine* first = NULL;
  if(! myequal(head(), tail()))
    /*
  if(head()[0] != tail()[0] ||
  head()[1] != tail()[1])
  */
    first = this;
  else
    {
      for(temp = this->next; temp != this; temp = temp->next)
	{
	  /*
	  if(temp->head()[0] != temp->tail()[0] ||
	     temp->head()[1] != temp->tail()[1])
	     */
	  if(! myequal(temp->head(), temp->tail()))
	    {
	      first = temp;
	      break;
	    }
        
	}
    }

  //if there are no non-degenerate lines, then we simply return NULL.
  if(first == NULL)
    {
      deleteSinglePolygonWithSline();
      return NULL;
    }

  directedLine* tempNext = NULL;
  for(temp =first->next; temp != first; temp = tempNext)
    {
      tempNext = temp->getNext();
/*
      if(temp->head()[0] == temp->tail()[0] &&
	 temp->head()[1] == temp->tail()[1])
*/      

      if(myequal(temp->head(), temp->tail()))
	deleteSingleLine(temp);
    }   
  return first;
}

directedLine* directedLine::deleteDegenerateLinesAllPolygons()
{
  directedLine* temp;
  directedLine *tempNext = NULL;
  directedLine* ret= NULL;
  directedLine* retEnd = NULL;
  for(temp=this; temp != NULL; temp = tempNext)
    {
      tempNext = temp->nextPolygon;
      temp->nextPolygon = NULL;
      if(ret == NULL)
	{
	  ret = retEnd = temp->deleteDegenerateLines();
        
	}
      else
	{
	  directedLine *newPolygon = temp->deleteDegenerateLines();
	  if(newPolygon != NULL)
	    {
	  retEnd->nextPolygon = temp->deleteDegenerateLines();
	  retEnd = retEnd->nextPolygon;
	}
    }
    }
  return ret;
}

directedLine* directedLine::cutIntersectionAllPoly(int &cutOccur)
{
  directedLine* temp;
  directedLine *tempNext = NULL;
  directedLine* ret= NULL;
  directedLine* retEnd = NULL;
  cutOccur = 0;
  for(temp=this; temp != NULL; temp = tempNext)
    {
      int eachCutOccur=0;
      tempNext = temp->nextPolygon;
      temp->nextPolygon = NULL;
      if(ret == NULL)
	{

	  ret = retEnd = DBG_cutIntersectionPoly(temp, eachCutOccur);
	  if(eachCutOccur)
	    cutOccur = 1;
	}
      else
	{

	  retEnd->nextPolygon = DBG_cutIntersectionPoly(temp, eachCutOccur);
	  retEnd = retEnd->nextPolygon;
	  if(eachCutOccur)
	    cutOccur = 1;
	}
    }
  return ret;
}


void directedLine::deleteSinglePolygonWithSline()
{
  directedLine *temp, *tempNext;
  prev->next = NULL;
  for(temp=this; temp != NULL; temp = tempNext)
    {
      tempNext = temp->next;
      delete temp->sline;
      delete temp;
    }
}

void directedLine::deletePolygonListWithSline()
{
  directedLine *temp, *tempNext;
  for(temp=this; temp != NULL; temp=tempNext)
    {
      tempNext = temp->nextPolygon;
      temp->deleteSinglePolygonWithSline();
    }
}

void directedLine::deleteSinglePolygon()
{
  directedLine *temp, *tempNext;
  prev->next = NULL;
  for(temp=this; temp != NULL; temp = tempNext)
    {
      tempNext = temp->next;
      delete temp;
    }
}

void directedLine::deletePolygonList()
{
  directedLine *temp, *tempNext;
  for(temp=this; temp != NULL; temp=tempNext)
    {
      tempNext = temp->nextPolygon;
      temp->deleteSinglePolygon();
    }
}


/*a loop by itself*/
directedLine::directedLine(short dir, sampledLine* sl)
{
  direction = dir;
  sline = sl;
  next = this;
  prev = this;
  nextPolygon = NULL;
//  prevPolygon = NULL;
  rootBit = 0;/*important to initilzae to 0 meaning not root yet*/

  rootLink = NULL;

}

void directedLine::init(short dir, sampledLine* sl)
{
  direction = dir;
  sline = sl;
}

directedLine::directedLine()
{
  next = this;
  prev = this;
  nextPolygon = NULL;
  rootBit = 0;/*important to initilzae to 0 meaning not root yet*/
  rootLink = NULL;
}

directedLine::~directedLine()
{
}

Real* directedLine::head()
{

  return (direction==INCREASING)? (sline->get_points())[0] : (sline->get_points())[sline->get_npoints()-1];
}

/*inline*/ Real* directedLine::getVertex(Int i)
{
  return (direction==INCREASING)? (sline->get_points())[i] : (sline->get_points())[sline->get_npoints() - 1 -i];
}

Real* directedLine::tail()
{
  return (direction==DECREASING)? (sline->get_points())[0] : (sline->get_points())[sline->get_npoints()-1];
}

 /*insert a new line between prev and this*/
void directedLine::insert(directedLine* nl)
{
  nl->next = this;
  nl->prev = prev;
  prev->next = nl;
  prev = nl;
  nl->rootLink = this; /*assuming that 'this' is the root!!!*/
}

Int directedLine::numEdges()
{
  Int ret=0;
  directedLine* temp;
  if(next == this) return 1;

  ret = 1;
  for(temp = next; temp != this; temp = temp->next)
    ret++;
  return ret;
}

Int directedLine::numEdgesAllPolygons()
{
  Int ret=0;
  directedLine* temp;
  for(temp=this; temp!= NULL; temp=temp->nextPolygon)
    {
      ret += temp->numEdges();
    }
  return ret;
}

/*return 1 if the double linked list forms a polygon.
 */
short directedLine::isPolygon()
{
  directedLine* temp;

  /*a polygon contains at least 3 edges*/
  if(numEdges() <=2) return 0;

  /*check this edge*/
  if(! isConnected()) return 0;

  /*check all other edges*/
  for(temp=next; temp != this; temp = temp->next){
    if(!isConnected()) return 0;
  }
  return 1;
}

/*check if the head of this edge is connected to
 *the tail of the prev
 */
short directedLine::isConnected()
{
  if( (head()[0] == prev->tail()[0]) && (head()[1] == prev->tail()[1]))
    return 1;
  else
    return 0;
}

Int compV2InY(Real A[2], Real B[2])
{
  if(A[1] < B[1]) return -1;
  if(A[1] == B[1] && A[0] < B[0]) return -1;
  if(A[1] == B[1] && A[0] == B[0]) return 0;
  return 1;
}

Int compV2InX(Real A[2], Real B[2])
{
  if(A[0] < B[0]) return -1;
  if(A[0] == B[0] && A[1] < B[1]) return -1;
  if(A[0] == B[0] && A[1] == B[1]) return 0;
  return 1;
}

/*compare two vertices NOT lines!
 *A vertex is the head of a directed line.
 *(x_1, y_1) <= (x_2, y_2) if
 *either y_1 < y_2
 *or	 y_1 == y_2 && x_1 < x_2.
 *return -1 if this->head() <= nl->head(),
 *return  1 otherwise
 */
Int directedLine::compInY(directedLine* nl)
{
  if(head()[1] < nl->head()[1]) return -1;
  if(head()[1] == nl->head()[1] && head()[0] < nl->head()[0]) return -1;
  return 1;
}

/*compare two vertices NOT lines!
 *A vertex is the head of a directed line.
 *(x_1, y_1) <= (x_2, y_2) if
 *either x_1 < x_2
 *or	 x_1 == x_2 && y_1 < y_2.
 *return -1 if this->head() <= nl->head(),
 *return  1 otherwise
 */
Int directedLine::compInX(directedLine* nl)
{
  if(head()[0] < nl->head()[0]) return -1;
  if(head()[0] == nl->head()[0] && head()[1] < nl->head()[1]) return -1;
  return 1;
}

/*used by sort precedures
 */
static Int compInY2(directedLine* v1, directedLine* v2)
{
  return v1->compInY(v2);
}
#ifdef NOT_USED
static Int compInX(directedLine* v1, directedLine* v2)
{
  return v1->compInX(v2);
}
#endif

/*sort all the vertices NOT the lines!
 *a vertex is the head of a directed line
 */
directedLine** directedLine::sortAllPolygons()
{
  Int total_num_edges = 0;
  directedLine** array = toArrayAllPolygons(total_num_edges);
  quicksort( (void**)array, 0, total_num_edges-1, (Int (*)(void *, void *)) compInY2);

  return array;
}

void directedLine::printSingle()
{
  if(direction == INCREASING)
    printf("direction is INCREASING\n");
  else
    printf("direction is DECREASING\n");
  printf("head=%f,%f)\n", head()[0], head()[1]);
  sline->print();
}

/*print one polygon*/
void directedLine::printList()
{
  directedLine* temp;
  printSingle();
  for(temp = next; temp!=this; temp=temp->next)
    temp->printSingle();
}

/*print all the polygons*/
void directedLine::printAllPolygons()
{
  directedLine *temp;
  for(temp = this; temp!=NULL; temp = temp->nextPolygon)
    {
      printf("polygon:\n");
      temp->printList();
    }
}

/*insert this polygon into the head of the old polygon List*/
directedLine* directedLine::insertPolygon(directedLine* oldList)
{
  /*this polygon is a root*/
  setRootBit();
  if(oldList == NULL) return this;
  nextPolygon = oldList;
/*  oldList->prevPolygon = this;*/
  return this;
}

/*cutoff means delete. but we don't deallocate any space,
 *so we use cutoff instead of delete
 */
directedLine* directedLine::cutoffPolygon(directedLine *p)
{
  directedLine* temp;
  directedLine* prev_polygon  = NULL;
  if(p == NULL) return this;

  for(temp=this; temp != p; temp = temp->nextPolygon)
    {
      if(temp == NULL)
	{
	  fprintf(stderr, "in cutoffPolygon, not found\n");
	  exit(1);
	}
      prev_polygon = temp;
    }

/*  prev_polygon = p->prevPolygon;*/

  p->resetRootBit();
  if(prev_polygon == NULL) /*this is the one to cutoff*/
    return nextPolygon;
  else {
    prev_polygon->nextPolygon = p->nextPolygon;
    return this;
  }
}

Int directedLine::numPolygons()
{
  if(nextPolygon == NULL) return 1;
  else return 1+nextPolygon->numPolygons();
}


/*let  array[index ...] denote
 *all the edges in this polygon
 *return the next available index of array.
 */
Int directedLine::toArraySinglePolygon(directedLine** array, Int index)
{
  directedLine *temp;
  array[index++] = this;
  for(temp = next; temp != this; temp = temp->next)
    {
      array[index++] = temp;
    }
  return index;
}

/*the space is allocated. The caller is responsible for
 *deallocate the space.
 *total_num_edges is set to be the total number of edges of all polygons
 */
directedLine** directedLine::toArrayAllPolygons(Int& total_num_edges)
{
  total_num_edges=numEdgesAllPolygons();
  directedLine** ret = (directedLine**) malloc(sizeof(directedLine*) * total_num_edges);
  assert(ret);

  directedLine *temp;
  Int index = 0;
  for(temp=this; temp != NULL; temp=temp->nextPolygon) {
    index = temp->toArraySinglePolygon(ret, index);
  }
  return ret;
}

/*assume the polygon is a simple polygon, return
 *the area enclosed by it.
 *if thee order is counterclock wise, the area is positive.
 */
Real directedLine::polyArea()
{
  directedLine* temp;
  Real ret=0.0;
  Real x1,y1,x2,y2;
  x1 = this->head()[0];
  y1 = this->head()[1];
  x2 = this->next->head()[0];
  y2 = this->next->head()[1];
  ret = -(x2*y1-x1*y2);
  for(temp=this->next; temp!=this; temp = temp->next)
    {
      x1 = temp->head()[0];
      y1 = temp->head()[1];
      x2 = temp->next->head()[0];
      y2 = temp->next->head()[1];
      ret += -( x2*y1-x1*y2);
    }
  return Real(0.5)*ret;
}

/*******************split or combine polygons begin********************/
/*conect a diagonal of a single simple polygon or  two simple polygons.
 *If the two vertices v1 (head) and v2 (head) are in the same simple polygon,
 *then we actually split the simple polygon into two polygons.
 *If instead two vertices velong to two difference polygons,
 *then we combine the  two polygons into one polygon.
 *It is upto the caller to decide whether this is a split or a
 *combination.
 *
 *Case Split:
 *split a single simple polygon into two simple polygons by
 *connecting a diagonal (two vertices).
 *v1, v2: the two vertices are the head() of the two directedLines.
 *  this routine generates one new sampledLine which is returned in
 *generatedLine,
 *and it generates two directedLines returned in ret_p1 and ret_p2.
 *ret_p1 and ret_p2 are used as the entry to the two new polygons.
 *Notice the caller should not deallocate the space of v2 and v2 after
 *calling this function, since all of the edges are connected to
 *ret_p1 or ret_p2.
 *
 *combine:
 *combine two simpolygons into one by connecting one diagonal.
 *the returned polygon is returned in ret_p1.
 */
/*ARGSUSED*/
void directedLine::connectDiagonal(directedLine* v1, directedLine* v2,
			   directedLine** ret_p1,
			   directedLine** ret_p2,
			   sampledLine** generatedLine,
			   directedLine* polygonList								   )
{
  sampledLine *nsline  = new sampledLine(2);



  nsline->setPoint(0, v1->head());
  nsline->setPoint(1, v2->head());



  /*the increasing line is from v1 head to v2 head*/
  directedLine* newLineInc = new directedLine(INCREASING, nsline);



  directedLine* newLineDec = new directedLine(DECREASING, nsline);


  directedLine* v1Prev = v1->prev;
  directedLine* v2Prev = v2->prev;

  v1	    ->prev = newLineDec;
  v2Prev    ->next = newLineDec;
  newLineDec->next = v1;
  newLineDec->prev = v2Prev;

  v2	    ->prev = newLineInc;
  v1Prev    ->next = newLineInc;
  newLineInc->next = v2;
  newLineInc->prev = v1Prev;

  *ret_p1 = newLineDec;
  *ret_p2 = newLineInc;
  *generatedLine = nsline;
}

//see the function connectDiangle
/*ARGSUSED*/
void directedLine::connectDiagonal_2slines(directedLine* v1, directedLine* v2,
			   directedLine** ret_p1,
			   directedLine** ret_p2,
			   directedLine* polygonList								   )
{
  sampledLine *nsline  = new sampledLine(2);
  sampledLine *nsline2	= new sampledLine(2);

  nsline->setPoint(0, v1->head());
  nsline->setPoint(1, v2->head());
  nsline2->setPoint(0, v1->head());
  nsline2->setPoint(1, v2->head());

  /*the increasing line is from v1 head to v2 head*/
  directedLine* newLineInc = new directedLine(INCREASING, nsline);

  directedLine* newLineDec = new directedLine(DECREASING, nsline2);

  directedLine* v1Prev = v1->prev;
  directedLine* v2Prev = v2->prev;

  v1	    ->prev = newLineDec;
  v2Prev    ->next = newLineDec;
  newLineDec->next = v1;
  newLineDec->prev = v2Prev;

  v2	    ->prev = newLineInc;
  v1Prev    ->next = newLineInc;
  newLineInc->next = v2;
  newLineInc->prev = v1Prev;

  *ret_p1 = newLineDec;
  *ret_p2 = newLineInc;

}

Int directedLine::samePolygon(directedLine* v1, directedLine* v2)
{
  if(v1 == v2) return 1;
  directedLine *temp;
  for(temp = v1->next; temp != v1; temp = temp->next)
    {
      if(temp == v2) return 1;
    }
  return 0;
}

directedLine* directedLine::findRoot()
{
  if(rootBit) return this;
  directedLine* temp;
  for(temp = next; temp != this; temp = temp->next)
    if(temp -> rootBit ) return temp;
  return NULL; /*should not happen*/
}

directedLine* directedLine::rootLinkFindRoot()
{
  directedLine* tempRoot;
  directedLine* tempLink;
  tempRoot = this;
  tempLink = rootLink;
  while(tempLink != NULL){
    tempRoot = tempLink;
    tempLink = tempRoot->rootLink;
  }
  return tempRoot;
}

/*******************split or combine polygons end********************/

/*****************IO stuff begin*******************/

/*format:
 *#polygons
 * #vertices
 *  vertices
 * #vertices
 *  vertices
 *...
 */
void directedLine::writeAllPolygons(char* filename)
{
  FILE* fp = fopen(filename, "w");
  assert(fp);
  Int nPolygons = numPolygons();
  directedLine *root;
  fprintf(fp, "%i\n", nPolygons);
  for(root = this; root != NULL; root = root->nextPolygon)
    {
      directedLine *temp;
      Int npoints=0;
      npoints = root->get_npoints()-1;
      for(temp = root->next; temp != root; temp=temp->next)
	npoints += temp->get_npoints()-1;
      fprintf(fp, "%i\n", npoints/*root->numEdges()*/);


      for(Int i=0; i<root->get_npoints()-1; i++){
	fprintf(fp, "%f ", root->getVertex(i)[0]);
	fprintf(fp, "%f ", root->getVertex(i)[1]);
      }

      for(temp=root->next; temp != root; temp = temp->next)
	{
	  for(Int i=0; i<temp->get_npoints()-1; i++){

	    fprintf(fp, "%f ", temp->getVertex(i)[0]);
	    fprintf(fp, "%f ", temp->getVertex(i)[1]);
	  }
	  fprintf(fp,"\n");
	}
      fprintf(fp, "\n");
    }
  fclose(fp);
}

directedLine* readAllPolygons(char* filename)
{
  Int i,j;
  FILE* fp = fopen(filename, "r");
  assert(fp);
  Int nPolygons;
  fscanf(fp, "%i", &nPolygons);
  directedLine *ret = NULL;

  for(i=0; i<nPolygons; i++)
    {
      Int nEdges;
      fscanf(fp, "%i", &nEdges);
      Real vert[2][2];
      Real VV[2][2];
      /*the first two vertices*/
      fscanf(fp, "%f", &(vert[0][0]));
      fscanf(fp, "%f", &(vert[0][1]));
      fscanf(fp, "%f", &(vert[1][0]));
      fscanf(fp, "%f", &(vert[1][1]));
      VV[1][0] = vert[0][0];
      VV[1][1] = vert[0][1];
      sampledLine *sLine = new sampledLine(2, vert);
      directedLine *thisPoly = new directedLine(INCREASING, sLine);
thisPoly->rootLinkSet(NULL);

      directedLine *dLine;
      for(j=2; j<nEdges; j++)
	{
	  vert[0][0]=vert[1][0];
	  vert[0][1]=vert[1][1];
	  fscanf(fp, "%f", &(vert[1][0]));
	  fscanf(fp, "%f", &(vert[1][1]));
	  sLine = new sampledLine(2,vert);
	  dLine = new directedLine(INCREASING, sLine);
dLine->rootLinkSet(thisPoly);
	  thisPoly->insert(dLine);
	}

      VV[0][0]=vert[1][0];
      VV[0][1]=vert[1][1];
      sLine = new sampledLine(2,VV);
      dLine = new directedLine(INCREASING, sLine);
dLine->rootLinkSet(thisPoly);
      thisPoly->insert(dLine);

      ret = thisPoly->insertPolygon(ret);
    }
  fclose(fp);
  return ret;
}









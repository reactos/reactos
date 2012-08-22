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
#include <time.h>

#include "zlassert.h"
#include "partitionY.h"
#include "searchTree.h"
#include "quicksort.h"
#include "polyUtil.h"


#define max(a,b) ((a>b)? a:b)
#define min(a,b) ((a>b)? b:a)


/*retrurn 
 *-1: if A < B (Ya<Yb) || (Ya==Yb)
 * 0: if A == B
 * 1: if A>B
 */
static Int compVertInY(Real A[2], Real B[2])
{
  if( (A[1] < B[1]) || (A[1]==B[1] && A[0]<B[0])) 
    return -1;
  else if
    ( A[1] == B[1] && A[0] == B[0]) return 0;
  else
    return 1;
}

/*v is a vertex: the head of en edge,
 *e is an edge,
 *return 1 if e is below v: assume v1 and v2 are the two endpoints of e:
 * v1<= v, v2<=v.
 */
Int isBelow(directedLine *v, directedLine *e)
{
  Real* vert = v->head();
  if(   compVertInY(e->head(), vert) != 1 
     && compVertInY(e->tail(), vert) != 1
     )
    return 1;
  else
    return 0;
}

/*v is a vertex: the head of en edge,
 *e is an edge,
 *return 1 if e is below v: assume v1 and v2 are the two endpoints of e:
 * v1>= v, v2>=v.
 */
Int isAbove(directedLine *v, directedLine *e)
{
  Real* vert = v->head();
  if(   compVertInY(e->head(), vert) != -1 
     && compVertInY(e->tail(), vert) != -1
     )
    return 1;
  else
    return 0;
}

Int isCusp(directedLine *v)
{
  Real *A=v->getPrev()->head();
  Real *B=v->head();
  Real *C=v->tail();
  if(A[1] < B[1] && B[1] < C[1])
    return 0;
  else if(A[1] > B[1] && B[1] > C[1])
    return 0;
  else if(A[1] < B[1] && C[1] < B[1])
    return 1;
  else if(A[1] > B[1] && C[1] > B[1])
    return 1;

  if((isAbove(v, v) && isAbove(v, v->getPrev())) ||
     (isBelow(v, v) && isBelow(v, v->getPrev())))
    return 1;
  else
    return 0;
}

/*crossproduct is strictly less than 0*/
Int isReflex(directedLine *v)
{
  Real* A = v->getPrev()->head();
  Real* B = v->head();
  Real* C = v->tail();
  Real Bx,By, Cx, Cy;
  Bx = B[0] - A[0];
  By = B[1] - A[1];
  Cx = C[0] - A[0];
  Cy = C[1] - A[1];

  if(Bx*Cy - Cx*By < 0) return 1;
  else return 0;
}

 /*return 
 *0: not-cusp
 *1: interior cusp
 *2: exterior cusp
 */
Int cuspType(directedLine *v)
{
  if(! isCusp(v)) return 0;
  else if(isReflex(v)) return 1;
  else
    return 2;
}

sweepRange* sweepRangeMake(directedLine* left, Int leftType,
			   directedLine* right, Int rightType)
{
  sweepRange* ret = (sweepRange*)malloc(sizeof(sweepRange));
  assert(ret);
  ret->left = left;
  ret->leftType = leftType;
  ret->right = right;
  ret->rightType = rightType;
  return ret;
}

void sweepRangeDelete(sweepRange* range)
{
  free(range);
}

Int sweepRangeEqual(sweepRange* src1, sweepRange* src2)
{
  Int leftEqual;
  Int rightEqual;
  
  
  /*The case when both are vertices should not happen*/
  assert(! (src1->leftType == 0 && src2->leftType == 0));    
  if(src1->leftType == 0 && src2->leftType == 1){
    if(src1->left == src2->left ||
       src1->left->getPrev() == src2->left
       )
      leftEqual = 1;
    else
      leftEqual = 0;
  }
  else if(src1->leftType == 1 && src2->leftType == 1){
    if(src1->left == src2->left)
      leftEqual = 1;
    else
      leftEqual = 0;
  }
  else /*src1->leftType == 1 && src2->leftType == 0*/{
    if(src1->left == src2->left ||
       src1->left == src2->left->getPrev()
       )
      leftEqual = 1;
    else 
      leftEqual = 0;
  }

  /*the same thing for right*/
  /*The case when both are vertices should not happen*/
  assert(! (src1->rightType == 0 && src2->rightType == 0));    
  if(src1->rightType == 0 && src2->rightType == 1){
    if(src1->right == src2->right ||
       src1->right->getPrev() == src2->right
       )
      rightEqual = 1;
    else
      rightEqual = 0;
  }
  else if(src1->rightType == 1 && src2->rightType == 1){
    if(src1->right == src2->right)
      rightEqual = 1;
    else
      rightEqual = 0;
  }
  else /*src1->rightType == 1 && src2->rightType == 0*/{
    if(src1->right == src2->right ||
       src1->right == src2->right->getPrev()
       )
      rightEqual = 1;
    else 
      rightEqual = 0;
  }
  
  return (leftEqual == 1 || rightEqual == 1);
}

/*given (x_1, y_1) and (x_2, y_2), and y
 *return x such that (x,y) is on the line
 */
inline/*static*/ Real intersectHoriz(Real x1, Real y1, Real x2, Real y2, Real y)
{
  return ((y2==y1)? (x1+x2)*Real(0.5) : x1 + ((y-y1)/(y2-y1)) * (x2-x1));
/*
  if(y2 == y1) return (x1+x2)*0.5;
  else return x1 + ((y-y1)/(y2-y1)) * (x2-x1);
*/
}

/*compare two edges of a polygon.
 *edge A < edge B if there is a horizontal line so that the intersection
 *with A is to the left of the intersection with B.
 *This function is used in sweepY for the dynamic search tree insertion to
 *order the edges.
 * Implementation: (x_1,y_1) and (x_2, y_2)
 */
static Int compEdges(directedLine *e1, directedLine *e2)
{
  Real* head1 = e1->head();
  Real* tail1 = e1->tail();
  Real* head2 = e2->head();
  Real* tail2 = e2->tail();
/*
  Real h10 = head1[0];
  Real h11 = head1[1];
  Real t10 = tail1[0];
  Real t11 = tail1[1];
  Real h20 = head2[0];
  Real h21 = head2[1];
  Real t20 = tail2[0];
  Real t21 = tail2[1];
*/
  Real e1_Ymax, e1_Ymin, e2_Ymax, e2_Ymin;
/*
  if(h11>t11) {
    e1_Ymax= h11;
    e1_Ymin= t11;
  }
  else{
    e1_Ymax = t11;
    e1_Ymin = h11;
  }

  if(h21>t21) {
    e2_Ymax= h21;
    e2_Ymin= t21;
  }
  else{
    e2_Ymax = t21;
    e2_Ymin = h21;
  }
*/
 
  if(head1[1]>tail1[1]) {
    e1_Ymax= head1[1];
    e1_Ymin= tail1[1];
  }
  else{
    e1_Ymax = tail1[1];
    e1_Ymin = head1[1];
  }

  if(head2[1]>tail2[1]) {
    e2_Ymax= head2[1];
    e2_Ymin= tail2[1];
  }
  else{
    e2_Ymax = tail2[1];
    e2_Ymin = head2[1];
  }

  
  /*Real e1_Ymax = max(head1[1], tail1[1]);*/ /*max(e1->head()[1], e1->tail()[1]);*/
  /*Real e1_Ymin = min(head1[1], tail1[1]);*/ /*min(e1->head()[1], e1->tail()[1]);*/
  /*Real e2_Ymax = max(head2[1], tail2[1]);*/ /*max(e2->head()[1], e2->tail()[1]);*/
  /*Real e2_Ymin = min(head2[1], tail2[1]);*/ /*min(e2->head()[1], e2->tail()[1]);*/

  Real Ymax = min(e1_Ymax, e2_Ymax);
  Real Ymin = max(e1_Ymin, e2_Ymin);
    
  Real y = Real(0.5)*(Ymax + Ymin);

/*  Real x1 = intersectHoriz(e1->head()[0], e1->head()[1], e1->tail()[0], e1->tail()[1], y);
  Real x2 = intersectHoriz(e2->head()[0], e2->head()[1], e2->tail()[0], e2->tail()[1], y);
*/
/*
  Real x1 = intersectHoriz(h10, h11, t10, t11, y);
  Real x2 = intersectHoriz(h20, h21, t20, t21, y);
*/
  Real x1 = intersectHoriz(head1[0], head1[1], tail1[0], tail1[1], y);
  Real x2 = intersectHoriz(head2[0], head2[1], tail2[0], tail2[1], y);

  if(x1<= x2) return -1;
  else return 1;
}
  
/*used by sort precedures
 */
static Int compInY(directedLine* v1, directedLine* v2)
{
  return v1->compInY(v2);
}

void findDiagonals(Int total_num_edges, directedLine** sortedVertices, sweepRange** ranges, Int& num_diagonals, directedLine** diagonal_vertices)
{
  Int i,j,k;

  k=0;

  for(i=0; i<total_num_edges; i++)
    {
      directedLine* vert =sortedVertices[i];
      directedLine* thisEdge = vert;
      directedLine* prevEdge = vert->getPrev();
/*
printf("find i=%i\n", i);            
printf("the vertex is\n");
vert->printSingle();
*/
      if(isBelow(vert, thisEdge) && isBelow(vert, prevEdge) && compEdges(prevEdge, thisEdge)<0)
	{
	  /*this is an upward interior cusp*/
	  diagonal_vertices[k++] = vert;

	  for(j=i+1; j<total_num_edges; j++)
	    if(sweepRangeEqual(ranges[i], ranges[j]))
	      {
		diagonal_vertices[k++] = sortedVertices[j];
		break;
	      }
	  assert(j<total_num_edges);


	}
      else if(isAbove(vert, thisEdge) && isAbove(vert, prevEdge) && compEdges(prevEdge, thisEdge)>0)
	{
	  /*this is an downward interior cusp*/
	  diagonal_vertices[k++] = vert;
	  for(j=i-1; j>=0; j--)
	    if(sweepRangeEqual(ranges[i], ranges[j]))
	      {
		diagonal_vertices[k++] = sortedVertices[j];
		break;
	      }
/*	  printf("j=%i\n", j);*/
	  assert(j>=0);



	}
    }
  num_diagonals = k/2;
}

/*get rid of repeated diagonlas so that each diagonal appears only once in the array
 */
Int deleteRepeatDiagonals(Int num_diagonals, directedLine** diagonal_vertices, directedLine** new_vertices)
{
  Int i,k;
  Int j,l;
  Int index;
  index=0;
  for(i=0,k=0; i<num_diagonals; i++, k+=2)
    {
      Int isRepeated=0;
      /*check the diagonla (diagonal_vertice[k], diagonal_vertices[k+1])
       *is repeated or not
       */
      for(j=0,l=0; j<index; j++, l+=2)
	{
	  if(
	     (diagonal_vertices[k] == new_vertices[l] && 
	      diagonal_vertices[k+1] == new_vertices[l+1]
	      )
	     ||
	     (
	      diagonal_vertices[k] == new_vertices[l+1] && 
	      diagonal_vertices[k+1] == new_vertices[l]
	      )
	     )
	    {
	      isRepeated=1;
	      break;
	    }
	}
      if(! isRepeated)
	{
	  new_vertices[index+index] = diagonal_vertices[k];
	  new_vertices[index+index+1] = diagonal_vertices[k+1];	  
	  index++;
	}
    }
  return index;
}

/*for debug only*/	  
directedLine** DBGfindDiagonals(directedLine *polygons, Int& num_diagonals)
{
  Int total_num_edges = 0;
  directedLine** array = polygons->toArrayAllPolygons(total_num_edges);
  quicksort( (void**)array, 0, total_num_edges-1, (Int (*)(void*, void*)) compInY);
  sweepRange** ranges = (sweepRange**) malloc(sizeof(sweepRange*) * total_num_edges);
  assert(ranges);

  sweepY(total_num_edges, array, ranges);

 directedLine** diagonal_vertices = (directedLine**) malloc(sizeof(directedLine*) * total_num_edges);
  assert(diagonal_vertices);
  findDiagonals(total_num_edges, array, ranges, num_diagonals, diagonal_vertices);

  num_diagonals=deleteRepeatDiagonals(num_diagonals, diagonal_vertices, diagonal_vertices);
  return diagonal_vertices;

}


/*partition into Y-monotone polygons*/
directedLine* partitionY(directedLine *polygons, sampledLine **retSampledLines)
{
  Int total_num_edges = 0;
  directedLine** array = polygons->toArrayAllPolygons(total_num_edges);

  quicksort( (void**)array, 0, total_num_edges-1, (Int (*)(void*, void*)) compInY);

  sweepRange** ranges = (sweepRange**) malloc(sizeof(sweepRange*) * (total_num_edges));
  assert(ranges);



  sweepY(total_num_edges, array, ranges);



  /*the diagonal vertices are stored as:
   *v0-v1: 1st diagonal
   *v2-v3: 2nd diagonal
   *v5-v5: 3rd diagonal
   *...
   */


  Int num_diagonals;
  /*number diagonals is < total_num_edges*total_num_edges*/
  directedLine** diagonal_vertices = (directedLine**) malloc(sizeof(directedLine*) * total_num_edges*2/*total_num_edges*/);
  assert(diagonal_vertices);



  findDiagonals(total_num_edges, array, ranges, num_diagonals, diagonal_vertices);



  directedLine* ret_polygons = polygons;
  sampledLine* newSampledLines = NULL;
  Int i,k;

num_diagonals=deleteRepeatDiagonals(num_diagonals, diagonal_vertices, diagonal_vertices);



  Int *removedDiagonals=(Int*)malloc(sizeof(Int) * num_diagonals);
  for(i=0; i<num_diagonals; i++)
    removedDiagonals[i] = 0;





  for(i=0,k=0; i<num_diagonals; i++,k+=2)
    {


      directedLine* v1=diagonal_vertices[k];
      directedLine* v2=diagonal_vertices[k+1];
      directedLine* ret_p1;
      directedLine* ret_p2;
      
      /*we ahve to determine whether v1 and v2 belong to the same polygon before
       *their structure are modified by connectDiagonal().
       */
/*
      directedLine *root1 = v1->findRoot();
      directedLine *root2 = v2->findRoot();
      assert(root1);      
      assert(root2);
*/

directedLine*  root1 = v1->rootLinkFindRoot();
directedLine*  root2 = v2->rootLinkFindRoot();

      if(root1 != root2)
	{

	  removedDiagonals[i] = 1;
	  sampledLine* generatedLine;



	  v1->connectDiagonal(v1,v2, &ret_p1, &ret_p2, &generatedLine, ret_polygons);



	  newSampledLines = generatedLine->insert(newSampledLines);
/*
	  ret_polygons = ret_polygons->cutoffPolygon(root1);

	  ret_polygons = ret_polygons->cutoffPolygon(root2);
	  ret_polygons = ret_p1->insertPolygon(ret_polygons);
root1->rootLinkSet(ret_p1);
root2->rootLinkSet(ret_p1);
ret_p1->rootLinkSet(NULL);
ret_p2->rootLinkSet(ret_p1);
*/
	  ret_polygons = ret_polygons->cutoffPolygon(root2);



root2->rootLinkSet(root1);
ret_p1->rootLinkSet(root1);
ret_p2->rootLinkSet(root1);

       /*now that we have connected the diagonal v1 and v2, 
        *we have to check those unprocessed diagonals which 
        *have v1 or v2 as an end point. Notice that the head of v1
        *has the same coodinates as the head of v2->prev, and the head of
        *v2 has the same coordinate as the head of v1->prev. 
        *Suppose these is a diagonal (v1, x). If (v1,x) is still a valid
        *diagonal, then x should be on the left hand side of the directed line:        *v1->prev->head -- v1->head -- v1->tail. Otherwise, (v1,x) should be  
        *replaced by (v2->prev, x), that is, x is on the left of 
        * v2->prev->prev->head, v2->prev->head, v2->prev->tail.
        */
        Int ii, kk;
        for(ii=0, kk=0; ii<num_diagonals; ii++, kk+=2)
	  if( removedDiagonals[ii]==0)
	    {
	      directedLine* d1=diagonal_vertices[kk];
	      directedLine* d2=diagonal_vertices[kk+1];
	      /*check d1, and replace diagonal_vertices[kk] if necessary*/
	      if(d1 == v1) {
		/*check if d2 is to left of v1->prev->head:v1->head:v1->tail*/
		if(! pointLeft2Lines(v1->getPrev()->head(), 
				     v1->head(), v1->tail(), d2->head()))
		  {
/*
		    assert(pointLeft2Lines(v2->getPrev()->getPrev()->head(),
					   v2->getPrev()->head(), 
					   v2->getPrev()->tail(), d2->head()));
*/
		    diagonal_vertices[kk] = v2->getPrev();
		  }
	      }
	      if(d1 == v2) {
		/*check if d2 is to left of v2->prev->head:v2->head:v2->tail*/
		if(! pointLeft2Lines(v2->getPrev()->head(),
				     v2->head(), v2->tail(), d2->head()))
		  {
/*
		    assert(pointLeft2Lines(v1->getPrev()->getPrev()->head(),
					   v1->getPrev()->head(),
					   v1->getPrev()->tail(), d2->head()));
*/
		    diagonal_vertices[kk] = v1->getPrev();
		  }
	      }
	      /*check d2 and replace diagonal_vertices[k+1] if necessary*/
	      if(d2 == v1) {
		/*check if d1 is to left of v1->prev->head:v1->head:v1->tail*/
		if(! pointLeft2Lines(v1->getPrev()->head(), 
				     v1->head(), v1->tail(), d1->head()))
		  {
/*		    assert(pointLeft2Lines(v2->getPrev()->getPrev()->head(),
					   v2->getPrev()->head(), 
					   v2->getPrev()->tail(), d1->head()));
*/
		    diagonal_vertices[kk+1] = v2->getPrev();
		  }
	      }
	      if(d2 == v2) {
		/*check if d1 is to left of v2->prev->head:v2->head:v2->tail*/
		if(! pointLeft2Lines(v2->getPrev()->head(),
				     v2->head(), v2->tail(), d1->head()))
		  {
/*		    assert(pointLeft2Lines(v1->getPrev()->getPrev()->head(),
					   v1->getPrev()->head(),
					   v1->getPrev()->tail(), d1->head()));
*/
		    diagonal_vertices[kk+1] = v1->getPrev();
		  }
	      }
	    }					    	       
}/*end if (root1 not equal to root 2)*/
}

  /*second pass,  now all diagoals should belong to the same polygon*/



  for(i=0,k=0; i<num_diagonals; i++, k += 2)
    if(removedDiagonals[i] == 0) 
      {


	directedLine* v1=diagonal_vertices[k];
	directedLine* v2=diagonal_vertices[k+1];



	directedLine* ret_p1;
	directedLine* ret_p2;

	/*we ahve to determine whether v1 and v2 belong to the same polygon before
	 *their structure are modified by connectDiagonal().
	 */
	directedLine *root1 = v1->findRoot();
/*
	directedLine *root2 = v2->findRoot();



	assert(root1);      
	assert(root2);      
	assert(root1 == root2);
  */    
	sampledLine* generatedLine;



	v1->connectDiagonal(v1,v2, &ret_p1, &ret_p2, &generatedLine, ret_polygons);
	newSampledLines = generatedLine->insert(newSampledLines);

	ret_polygons = ret_polygons->cutoffPolygon(root1);

	ret_polygons = ret_p1->insertPolygon(ret_polygons);

	ret_polygons = ret_p2->insertPolygon(ret_polygons);



	for(Int j=i+1; j<num_diagonals; j++) 
	  {
	    if(removedDiagonals[j] ==0)
	      {

		directedLine* temp1=diagonal_vertices[2*j];
		directedLine* temp2=diagonal_vertices[2*j+1];
               if(temp1==v1 || temp1==v2 || temp2==v1 || temp2==v2)
		if(! temp1->samePolygon(temp1, temp2))
		  {
		    /*if temp1 and temp2 are in different polygons, 
		     *then one of them must be v1 or v2.
		     */



		    assert(temp1==v1 || temp1 == v2 || temp2==v1 || temp2 ==v2);
		    if(temp1==v1) 
		      {
			diagonal_vertices[2*j] = v2->getPrev();
		      }
		    if(temp2==v1)
		      {
			diagonal_vertices[2*j+1] = v2->getPrev();
		      }
		    if(temp1==v2)
		      {
			diagonal_vertices[2*j] = v1->getPrev();		      
		      }
		    if(temp2==v2)
		      {
			diagonal_vertices[2*j+1] = v1->getPrev();
		      }
		  }
	      }
	  }      

      }

  /*clean up spaces*/
  free(array);
  free(ranges);
  free(diagonal_vertices);
  free(removedDiagonals);

  *retSampledLines = newSampledLines;
  return ret_polygons;
}
	
/*given a set of simple polygons where the interior 
 *is decided by left-hand principle,
 *return a range (sight) for each vertex. This is called
 *Trapezoidalization.
 */ 
void sweepY(Int nVertices, directedLine** sortedVertices, sweepRange** ret_ranges)
{
  Int i;
  /*for each vertex in the sorted list, update the binary search tree.
   *and store the range information for each vertex.
   */
  treeNode* searchTree = NULL;
  for(i=0; i<nVertices;i++)
    {

      directedLine* vert = sortedVertices[i];

      directedLine* thisEdge = vert;
      directedLine* prevEdge = vert->getPrev();
      
      if(isBelow(vert, thisEdge) && isAbove(vert, prevEdge))
	{

	  /*case 1: this < v < prev
	   *the polygon is going down at v, the interior is to 
	   *the right hand side.
	   * find the edge to the right of thisEdge for right range.
           * delete thisEdge
           * insert prevEdge
	   */
	  treeNode* thisNode = TreeNodeFind(searchTree, thisEdge, ( Int (*) (void *, void *))compEdges);
	  assert(thisNode);

	  treeNode* succ = TreeNodeSuccessor(thisNode);
	  assert(succ);
	  searchTree = TreeNodeDeleteSingleNode(searchTree, thisNode);
	  searchTree = TreeNodeInsert(searchTree, TreeNodeMake(prevEdge), ( Int (*) (void *, void *))compEdges);


	  ret_ranges[i] = sweepRangeMake(vert, 0, (directedLine*) (succ->key), 1);

	}
      else if(isAbove(vert, thisEdge) && isBelow(vert, prevEdge))
	{

	  /*case 2: this > v > prev
	   *the polygon is going up at v, the interior is to 
	   *the left hand side.
	   * find the edge to the left of thisEdge for left range.
           * delete prevEdge
           * insert thisEdge
	   */	  
	  treeNode* prevNode = TreeNodeFind(searchTree, prevEdge, ( Int (*) (void *, void *))compEdges);
	  assert(prevNode);
	  treeNode* pred = TreeNodePredecessor(prevNode);
	  searchTree = TreeNodeDeleteSingleNode(searchTree, prevNode);
	  searchTree = TreeNodeInsert(searchTree, TreeNodeMake(thisEdge), ( Int (*) (void *, void *))compEdges);
	  ret_ranges[i] = sweepRangeMake((directedLine*)(pred->key), 1, vert, 0);
	}
      else if(isAbove(vert, thisEdge) && isAbove(vert, prevEdge))
	{

	  /*case 3: insert both edges*/
	  treeNode* thisNode = TreeNodeMake(thisEdge);
	  treeNode* prevNode = TreeNodeMake(prevEdge);	  
	  searchTree = TreeNodeInsert(searchTree, thisNode, ( Int (*) (void *, void *))compEdges);
	  searchTree = TreeNodeInsert(searchTree, prevNode, ( Int (*) (void *, void *))compEdges);	  
	  if(compEdges(thisEdge, prevEdge)<0)  /*interior cusp*/
	    {

	      treeNode* leftEdge = TreeNodePredecessor(thisNode);
	      treeNode* rightEdge = TreeNodeSuccessor(prevNode);
	      ret_ranges[i] = sweepRangeMake( (directedLine*) leftEdge->key, 1, 
					     (directedLine*) rightEdge->key, 1
					     );
	    }
	  else /*exterior cusp*/
	    {

	      ret_ranges[i] = sweepRangeMake( prevEdge, 1, thisEdge, 1);
	    }
	}
      else if(isBelow(vert, thisEdge) && isBelow(vert, prevEdge))
	{

	  /*case 4: delete both edges*/
	  treeNode* thisNode = TreeNodeFind(searchTree, thisEdge, ( Int (*) (void *, void *))compEdges);
	  treeNode* prevNode = TreeNodeFind(searchTree, prevEdge, ( Int (*) (void *, void *))compEdges);
	  if(compEdges(thisEdge, prevEdge)>0) /*interior cusp*/
	    {
	      treeNode* leftEdge = TreeNodePredecessor(prevNode);
	      treeNode* rightEdge = TreeNodeSuccessor(thisNode);
	      ret_ranges[i] = sweepRangeMake( (directedLine*) leftEdge->key, 1, 
					     (directedLine*) rightEdge->key, 1
					     );
	    }
	  else /*exterior cusp*/
	    {
	      ret_ranges[i] = sweepRangeMake( thisEdge, 1, prevEdge, 1);
	    }
	  searchTree = TreeNodeDeleteSingleNode(searchTree, thisNode);
	  searchTree = TreeNodeDeleteSingleNode(searchTree, prevNode);
	}
      else
	{
	  fprintf(stderr,"error in partitionY.C, invalid case\n");
	  printf("vert is\n");
	  vert->printSingle();
	  printf("thisEdge is\n");
	  thisEdge->printSingle();
	  printf("prevEdge is\n");
	  prevEdge->printSingle();
	  
	  exit(1);
	}
    }

  /*finaly clean up space: delete the search tree*/
  TreeNodeDeleteWholeTree(searchTree);
}

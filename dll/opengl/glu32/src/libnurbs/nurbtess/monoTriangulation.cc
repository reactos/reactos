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
#include "gluos.h"
#include "glimports.h"
#include "zlassert.h"

#include "monoTriangulation.h"
#include "polyUtil.h" /*for area*/
#include "partitionX.h"
#include "monoPolyPart.h"



extern  directedLine*  polygonConvert(directedLine* polygon);

/*poly is NOT deleted
 */
void monoTriangulationOpt(directedLine* poly, primStream* pStream)
{
  Int n_cusps;
  Int n_edges = poly->numEdges();
  directedLine** cusps = (directedLine**) malloc(sizeof(directedLine*)*n_edges);
  assert(cusps);
  findInteriorCuspsX(poly, n_cusps, cusps);
  if(n_cusps ==0) //u monotine
    {
      monoTriangulationFun(poly, compV2InX, pStream);
    }
  else if(n_cusps == 1) // one interior cusp
    {
      directedLine* new_polygon = polygonConvert(cusps[0]);
      directedLine* other = findDiagonal_singleCuspX(new_polygon);
      //<other> should NOT be null unless there are self-intersecting
      //trim curves. In that case, we don't want to core dump, instead,
      //we triangulate anyway, and print out error message.
      if(other == NULL)
	{
	  monoTriangulationFun(poly, compV2InX, pStream);
	}
      else
	{
	  directedLine* ret_p1;
	  directedLine* ret_p2;
	  
	  new_polygon->connectDiagonal_2slines(new_polygon, other, 
					       &ret_p1,
					       &ret_p2,
					       new_polygon);
	  
	  monoTriangulationFun(ret_p1, compV2InX, pStream);
	  monoTriangulationFun(ret_p2, compV2InX, pStream);
	  
	  ret_p1->deleteSinglePolygonWithSline();	      
	  ret_p2->deleteSinglePolygonWithSline();	  
	}
    }
  else
    {
      //we need a general partitionX funtion (supposed to be in partitionX.C,
      //not implemented yet. XXX
      monoTriangulationFun(poly, compV2InY, pStream);    
    }
  
  free(cusps);
}

void monoTriangulationRecOpt(Real* topVertex, Real* botVertex, 
			     vertexArray* left_chain, Int left_current,
			     vertexArray* right_chain, Int right_current,
			     primStream* pStream)
{
  Int i,j;
  Int n_left = left_chain->getNumElements();
  Int n_right = right_chain->getNumElements();
  if(left_current>= n_left-1 ||
     right_current>= n_right-1)
    {
      monoTriangulationRec(topVertex, botVertex, left_chain, left_current,
			   right_chain, right_current, pStream);
      return;
    }
  //now both left and right  have at least two vertices each.
  Real left_v = left_chain->getVertex(left_current)[1];
  Real right_v = right_chain->getVertex(right_current)[1];
 
  if(left_v <= right_v) //first left vertex is below right
    {
      //find the last vertex of right which is above or equal to left
      for(j=right_current; j<=n_right-1; j++)
	{
	  if(right_chain->getVertex(j)[1] < left_v)
	    break;
	}
      monoTriangulationRecGen(topVertex, left_chain->getVertex(left_current),
			      left_chain, left_current, left_current,
			      right_chain, right_current, j-1,
			      pStream);
      monoTriangulationRecOpt(right_chain->getVertex(j-1),
			      botVertex,
			      left_chain, left_current,
			      right_chain, j,
			      pStream);
    }
  else //first right vertex is strictly below left
    {
      //find the last vertex of left which is strictly above right
      for(i=left_current; i<=n_left-1; i++)
	{
	  if(left_chain->getVertex(i)[1] <= right_v)
	    break;
	}
      monoTriangulationRecGen(topVertex, right_chain->getVertex(right_current),
			      left_chain, left_current, i-1,
			      right_chain, right_current, right_current,
			      pStream);
      monoTriangulationRecOpt(left_chain->getVertex(i-1),
			      botVertex,
			      left_chain, i,
			      right_chain, right_current,
			      pStream);
    }
}


void monoTriangulationRecGenTBOpt(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current, Int inc_end,
			  vertexArray* dec_chain, Int dec_current, Int dec_end,
			  primStream* pStream)
{
  pStream->triangle(topVertex, inc_chain->getVertex(inc_current), dec_chain->getVertex(dec_current));
  
/*printf("**(%f,%f)\n", inc_chain->getArray()[0][0],inc_chain->getArray()[0][1]);*/
  triangulateXYMonoTB(inc_end-inc_current+1, inc_chain->getArray()+inc_current,  dec_end-dec_current+1,  dec_chain->getArray()+dec_current, pStream);

  pStream->triangle(botVertex, dec_chain->getVertex(dec_end), inc_chain->getVertex(inc_end));
}
  

/*n_left>=1
 *n_right>=1
 *the strip is going top to bottom. compared to the funtion
 * triangulateXYmono()
 */
void triangulateXYMonoTB(Int n_left, Real** leftVerts,
		       Int n_right, Real** rightVerts,
		       primStream* pStream)
{


  Int i,j,k,l;
  Real* topMostV;

  assert(n_left>=1 && n_right>=1);
  if(leftVerts[0][1] >= rightVerts[0][1])
    {
      i=1;
      j=0;
      topMostV = leftVerts[0];
    }
  else
    {
      i=0;
      j=1;
      topMostV = rightVerts[0];
    }
  
  while(1)
    {
      if(i >= n_left) /*case1: no more in left*/
	{

	  if(j<n_right-1) /*at least two vertices in right*/
	    {
	      pStream->begin();
	      pStream->insert(topMostV);
	      for(k=n_right-1; k>=j; k--)		
		pStream->insert(rightVerts[j]);

	      pStream->end(PRIMITIVE_STREAM_FAN);
	      
	    }

	  break;	
	}
      else if(j>= n_right) /*case2: no more in right*/
	{

	  if(i<n_left-1) /*at least two vertices in left*/
	    {
	      pStream->begin();
	      pStream->insert(topMostV);

	      for(k=i; k<n_left; k++)
		pStream->insert(leftVerts[k]);

	      pStream->end(PRIMITIVE_STREAM_FAN);	      
	    }

	  break;
	}
      else /* case3: neither is empty, plus the topMostV, there is at least one triangle to output*/
	{

	  if(leftVerts[i][1] >=  rightVerts[j][1])
	    {
	      pStream->begin();
	      pStream->insert(rightVerts[j]); /*the origin of this fan*/

	      pStream->insert(topMostV);

	      /*find the last k>=i such that 
	       *leftverts[k][1] >= rightverts[j][1]
	       */
	      k=i;
	      while(k<n_left)
		{
		  if(leftVerts[k][1] < rightVerts[j][1])
		    break;
		  k++;
		}
	      k--;
	      for(l=i; l<=k; l++)
		{
		  pStream->insert(leftVerts[l]);
		}

	      pStream->end(PRIMITIVE_STREAM_FAN);
	      //update i for next loop
	      i = k+1;
	      topMostV = leftVerts[k];

	    }
	  else /*leftVerts[i][1] < rightVerts[j][1]*/
	    {
	      pStream->begin();
	      pStream->insert(leftVerts[i]);/*the origion of this fan*/

	      /*find the last k>=j such that
	       *rightverts[k][1] > leftverts[i][1]*/
	      k=j;
	      while(k< n_right)
		{
		  if(rightVerts[k][1] <= leftVerts[i][1])
		    break;
		  k++;
		}
	      k--;

	      for(l=k; l>= j; l--)
		pStream->insert(rightVerts[l]);

	      pStream->insert(topMostV);
	      pStream->end(PRIMITIVE_STREAM_FAN);
	      j=k+1;
	      topMostV = rightVerts[j-1];
	    }	  
	}
    }
}

static int chainConvex(vertexArray* inc_chain, Int inc_current, Int inc_end)
{
  Int i;
  //if there are no more than 2 vertices, return 1
  if(inc_current >= inc_end-1) return 1;
  for(i=inc_current; i<= inc_end-2; i++)
    {
      if(area(inc_chain->getVertex(i), inc_chain->getVertex(i+1), inc_chain->getVertex(i+2)) <0)
	return 0;
    }
  return 1;
}

static int chainConcave(vertexArray* dec_chain, Int dec_current, Int dec_end)
{
  Int i;
  //if there are no more than 2 vertices, return 1
  if(dec_current >= dec_end -1) return 1;
  for(i=dec_current; i<=dec_end-2; i++)
    {
      if(area(dec_chain->getVertex(i), dec_chain->getVertex(i+1), dec_chain->getVertex(i+2)) >0)
	return 0;
    }
  return 1;
}
 
void monoTriangulationRecGenInU(Real* topVertex, Real* botVertex,
				vertexArray* inc_chain, Int inc_current, Int inc_end,
				vertexArray* dec_chain, Int dec_current, Int dec_end,
				primStream* pStream)
{
  
}

void  monoTriangulationRecGenOpt(Real* topVertex, Real* botVertex,
				 vertexArray* inc_chain, Int inc_current, Int inc_end,
				 vertexArray* dec_chain, Int dec_current, Int dec_end,
				 primStream* pStream)
{
  Int i;
  //copy this to a polygon: directedLine Lioop
  sampledLine* sline;
  directedLine* dline;
  directedLine* poly;

  if(inc_current <= inc_end) //at least one vertex in inc_chain
    {      
      sline = new sampledLine(topVertex, inc_chain->getVertex(inc_current));
      poly = new directedLine(INCREASING, sline);
      for(i=inc_current; i<=inc_end-1; i++)
	{
	  sline = new sampledLine(inc_chain->getVertex(i), inc_chain->getVertex(i+1));
	  dline = new directedLine(INCREASING, sline);
	  poly->insert(dline);
	}
      sline = new sampledLine(inc_chain->getVertex(inc_end), botVertex);
      dline = new directedLine(INCREASING, sline);
      poly->insert(dline);
    }
  else //inc_chian is empty
    {
      sline = new sampledLine(topVertex, botVertex);
      dline = new directedLine(INCREASING, sline);
      poly = dline;
    }
  
  assert(poly != NULL);

  if(dec_current <= dec_end) //at least on vertex in dec_Chain
    {
      sline = new sampledLine(botVertex, dec_chain->getVertex(dec_end));
      dline = new directedLine(INCREASING, sline);
      poly->insert(dline);
      for(i=dec_end; i>dec_current; i--)
	{
	  sline = new sampledLine(dec_chain->getVertex(i), dec_chain->getVertex(i-1));
	  dline = new directedLine(INCREASING, sline);
	  poly->insert(dline);
	}
      sline = new sampledLine(dec_chain->getVertex(dec_current), topVertex);
      dline = new directedLine(INCREASING, sline);
      poly->insert(dline);      
    }
  else //dec_chain  is empty
    {
      sline = new sampledLine(botVertex, topVertex);
      dline = new directedLine(INCREASING, sline);
      poly->insert(dline);
    }

  {
    Int n_cusps;
    Int n_edges = poly->numEdges();
    directedLine** cusps = (directedLine**) malloc(sizeof(directedLine*)*n_edges);
    assert(cusps);
    findInteriorCuspsX(poly, n_cusps, cusps);

    if(n_cusps ==0) //u monotine
      {
	monoTriangulationFun(poly, compV2InX, pStream);
      }
    else if(n_cusps == 1) // one interior cusp
      {
	directedLine* new_polygon = polygonConvert(cusps[0]);
	directedLine* other = findDiagonal_singleCuspX(new_polygon);
	  //<other> should NOT be null unless there are self-intersecting
          //trim curves. In that case, we don't want to core dump, instead,
          //we triangulate anyway, and print out error message.
	  if(other == NULL)
	    {
	      monoTriangulationFun(poly, compV2InX, pStream);
	    }
	  else
	    {
	      directedLine* ret_p1;
	      directedLine* ret_p2;
	      
	      new_polygon->connectDiagonal_2slines(new_polygon, other, 
						   &ret_p1,
						   &ret_p2,
						   new_polygon);
	      
	      monoTriangulationFun(ret_p1, compV2InX, pStream);
	      monoTriangulationFun(ret_p2, compV2InX, pStream);

	      ret_p1->deleteSinglePolygonWithSline();	      
	      ret_p2->deleteSinglePolygonWithSline();	  
	    }
      }
    else
      {
	//we need a general partitionX funtion (supposed to be in partitionX.C,
	//not implemented yet. XXX
	//monoTriangulationFun(poly, compV2InY, pStream);    
	
	directedLine* new_polygon = polygonConvert(poly);
	directedLine* list = monoPolyPart(new_polygon);
	for(directedLine* temp = list; temp != NULL; temp = temp->getNextPolygon())
	  { 
	    monoTriangulationFun(temp, compV2InX, pStream);
	  }
	//clean up
	list->deletePolygonListWithSline();
        
      }

    free(cusps);
    /*
      if(numInteriorCuspsX(poly) == 0) //is u monotone
	monoTriangulationFun(poly, compV2InX, pStream);
      else //it is not u motone
	monoTriangulationFun(poly, compV2InY, pStream);    
	*/
    //clean up space
    poly->deleteSinglePolygonWithSline();
    return;
  }
      
  //apparently the following code is not reachable, 
  //it is for test purpose
  if(inc_current > inc_end || dec_current>dec_end)
    {
    monoTriangulationRecGen(topVertex, botVertex, inc_chain, inc_current, inc_end,
			    dec_chain, dec_current, dec_end,
			    pStream);    
    return;
  }
  

  if(
     area(dec_chain->getVertex(dec_current),
	  topVertex,
	  inc_chain->getVertex(inc_current)) >=0
     && chainConvex(inc_chain, inc_current, inc_end)
     && chainConcave(dec_chain, dec_current, dec_end)
     && area(inc_chain->getVertex(inc_end), botVertex, dec_chain->getVertex(dec_end)) >=0
     )
    {
      monoTriangulationRecFunGen(topVertex, botVertex, 
				 inc_chain, inc_current, inc_end,
				 dec_chain, dec_current, dec_end, 
				 compV2InX, pStream);
    }
  else
    {
      monoTriangulationRecGen(topVertex, botVertex, inc_chain, inc_current, inc_end,
			      dec_chain, dec_current, dec_end,
			      pStream);
    }
}

/*if inc_current>inc_end, then inc_chain has no points to be considered
 *same for dec_chain
 */
void monoTriangulationRecGen(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current, Int inc_end,
			  vertexArray* dec_chain, Int dec_current, Int dec_end,
			  primStream* pStream)
{
  Real** inc_array ;
  Real** dec_array ;
  Int i;

  if(inc_current > inc_end && dec_current>dec_end)
    return;
  else if(inc_current>inc_end) /*no more vertices on inc_chain*/
    {
      dec_array = dec_chain->getArray();
      reflexChain rChain(100,0);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the dec_chain*/
      for(i=dec_current; i<=dec_end; i++){
	rChain.processNewVertex(dec_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);
    }
  else if(dec_current> dec_end) /*no more vertices on dec_chain*/
    {
      inc_array = inc_chain->getArray();

      reflexChain rChain(100,1);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the inc_chain*/
      for(i=inc_current; i<=inc_end; i++){
	rChain.processNewVertex(inc_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);
    }
  else /*neither chain is empty*/
    {
      inc_array = inc_chain -> getArray();
      dec_array = dec_chain -> getArray();

      /*if top of inc_chain is 'lower' than top of dec_chain, process all the 
       *vertices on the dec_chain which are higher than top of inc_chain
       */
      if(compV2InY(inc_array[inc_current], dec_array[dec_current]) <= 0)
	{

	  reflexChain rChain(100, 0);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=dec_current; i<=dec_end; i++)
	    {
	      if(compV2InY(inc_array[inc_current], dec_array[i]) <= 0)
		rChain.processNewVertex(dec_array[i], pStream);
	      else 
		break;
	    }
	  rChain.outputFan(inc_array[inc_current], pStream);
	  monoTriangulationRecGen(dec_array[i-1], botVertex, 
			       inc_chain, inc_current, inc_end,
			       dec_chain, i, dec_end,
			       pStream);
	}
      else /*compV2InY(inc_array[inc_current], dec_array[dec_current]) > 0*/
	{

	  reflexChain rChain(100, 1);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=inc_current; i<=inc_end; i++)
	    {
	      if(compV2InY(inc_array[i], dec_array[dec_current]) >0)		
		rChain.processNewVertex(inc_array[i], pStream);	      
	      else
		break;
	    }
	  rChain.outputFan(dec_array[dec_current], pStream);
	  monoTriangulationRecGen(inc_array[i-1], botVertex, 
			       inc_chain, i, inc_end,
			       dec_chain, dec_current,dec_end,
			       pStream);
	}
    }/*end case neither is empty*/
}

void monoTriangulationFun(directedLine* monoPolygon, Int (*compFun)(Real*, Real*), primStream* pStream)
{
  Int i;
  /*find the top vertex, bottom vertex, inccreasing chain, and decreasing chain,
   *then call monoTriangulationRec
   */
  directedLine* tempV;
  directedLine* topV;
  directedLine* botV;
  topV = botV = monoPolygon;
  for(tempV = monoPolygon->getNext(); tempV != monoPolygon; tempV = tempV->getNext())
    {
      if(compFun(topV->head(), tempV->head())<0) {
	topV = tempV;
      }
      if(compFun(botV->head(), tempV->head())>0) {
	botV = tempV;
      }
    }

  /*creat increase and decrease chains*/
  vertexArray inc_chain(20); /*this is a dynamic array*/
  for(i=1; i<=topV->get_npoints()-2; i++) { /*the first vertex is the top vertex which doesn't belong to inc_chain*/
    inc_chain.appendVertex(topV->getVertex(i));
  }
  for(tempV = topV->getNext(); tempV != botV; tempV = tempV->getNext())
    {
      for(i=0; i<=tempV->get_npoints()-2; i++){
	inc_chain.appendVertex(tempV->getVertex(i));
      }
    }
  
  vertexArray dec_chain(20);
  for(tempV = topV->getPrev(); tempV != botV; tempV = tempV->getPrev())
    {
      for(i=tempV->get_npoints()-2; i>=0; i--){
	dec_chain.appendVertex(tempV->getVertex(i));
      }
    }
  for(i=botV->get_npoints()-2; i>=1; i--){ 
    dec_chain.appendVertex(tempV->getVertex(i));
  }
  
  if (!(0 == inc_chain.getNumElements() && 0 == dec_chain.getNumElements())) {
     monoTriangulationRecFun(topV->head(), botV->head(), &inc_chain, 0,
                             &dec_chain, 0, compFun, pStream);
  }
}  

void monoTriangulation(directedLine* monoPolygon, primStream* pStream)
{
  Int i;
  /*find the top vertex, bottom vertex, inccreasing chain, and decreasing chain,
   *then call monoTriangulationRec
   */
  directedLine* tempV;
  directedLine* topV;
  directedLine* botV;
  topV = botV = monoPolygon;
  for(tempV = monoPolygon->getNext(); tempV != monoPolygon; tempV = tempV->getNext())
    {
      if(compV2InY(topV->head(), tempV->head())<0) {
	topV = tempV;
      }
      if(compV2InY(botV->head(), tempV->head())>0) {
	botV = tempV;
      }
    }
  /*creat increase and decrease chains*/
  vertexArray inc_chain(20); /*this is a dynamic array*/
  for(i=1; i<=topV->get_npoints()-2; i++) { /*the first vertex is the top vertex which doesn't belong to inc_chain*/
    inc_chain.appendVertex(topV->getVertex(i));
  }
  for(tempV = topV->getNext(); tempV != botV; tempV = tempV->getNext())
    {
      for(i=0; i<=tempV->get_npoints()-2; i++){
	inc_chain.appendVertex(tempV->getVertex(i));
      }
    }
  
  vertexArray dec_chain(20);
  for(tempV = topV->getPrev(); tempV != botV; tempV = tempV->getPrev())
    {
      for(i=tempV->get_npoints()-2; i>=0; i--){
	dec_chain.appendVertex(tempV->getVertex(i));
      }
    }
  for(i=botV->get_npoints()-2; i>=1; i--){ 
    dec_chain.appendVertex(tempV->getVertex(i));
  }
  
  monoTriangulationRec(topV->head(), botV->head(), &inc_chain, 0, &dec_chain, 0, pStream);

}

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
			primStream* pStream)
{
  assert( inc_chain != NULL);
  Real** inc_array ;

  if(inc_smallIndex > inc_largeIndex)
    return; //no triangles 
  if(inc_smallIndex == inc_largeIndex)
    {
      if(is_increase_chain)
	pStream->triangle(inc_chain->getVertex(inc_smallIndex), botVertex, topVertex);
      else
	pStream->triangle(inc_chain->getVertex(inc_smallIndex), topVertex, botVertex);	
      return;
    }
  Int i;

  if(is_increase_chain && botVertex[1] == inc_chain->getVertex(inc_largeIndex)[1])
    {
      pStream->triangle(botVertex, inc_chain->getVertex(inc_largeIndex-1),
			inc_chain->getVertex(inc_largeIndex));
      monoTriangulation2(topVertex, botVertex, inc_chain, inc_smallIndex,
			 inc_largeIndex-1,
			 is_increase_chain,
			 pStream);
      return;
    }
  else if( (!is_increase_chain) && topVertex[1] == inc_chain->getVertex(inc_smallIndex)[1])
    {
      pStream->triangle(topVertex, inc_chain->getVertex(inc_smallIndex+1),
			inc_chain->getVertex(inc_smallIndex));
      monoTriangulation2(topVertex, botVertex, inc_chain, inc_smallIndex+1,
			 inc_largeIndex, is_increase_chain, pStream);
      return ;
    }		           

  inc_array = inc_chain->getArray();

  reflexChain rChain(20,is_increase_chain); /*1 means the chain is increasing*/

  rChain.processNewVertex(topVertex, pStream);

  for(i=inc_smallIndex; i<=inc_largeIndex; i++){
    rChain.processNewVertex(inc_array[i], pStream);
  }
  rChain.processNewVertex(botVertex, pStream);

}
 
/*if compFun == compV2InY, top to bottom: V-monotone
 *if compFun == compV2InX, right to left: U-monotone
 */
void monoTriangulationRecFunGen(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current, Int inc_end,
			  vertexArray* dec_chain, Int dec_current, Int dec_end,
			  Int  (*compFun)(Real*, Real*),
			  primStream* pStream)
{
  assert( inc_chain != NULL && dec_chain != NULL);
  assert( ! (inc_current> inc_end &&
	     dec_current> dec_end));
  /*
  Int inc_nVertices;
  Int dec_nVertices;
  */
  Real** inc_array ;
  Real** dec_array ;
  Int i;
  assert( ! ( (inc_chain==NULL) && (dec_chain==NULL)));

  if(inc_current> inc_end) /*no more vertices on inc_chain*/
    {

      dec_array = dec_chain->getArray();
      reflexChain rChain(20,0);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the dec_chain*/
      for(i=dec_current; i<=dec_end; i++){
	rChain.processNewVertex(dec_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);

    }
  else if(dec_current> dec_end) /*no more vertices on dec_chain*/
    {
      inc_array = inc_chain->getArray();
      reflexChain rChain(20,1);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the inc_chain*/
      for(i=inc_current; i<=inc_end; i++){
	rChain.processNewVertex(inc_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);
    }
  else /*neither chain is empty*/
    {
      inc_array = inc_chain -> getArray();
      dec_array = dec_chain -> getArray();

      /*if top of inc_chain is 'lower' than top of dec_chain, process all the 
       *vertices on the dec_chain which are higher than top of inc_chain
       */
      if(compFun(inc_array[inc_current], dec_array[dec_current]) <= 0)
	{

	  reflexChain rChain(20, 0);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=dec_current; i<=dec_end; i++)
	    {
	      if(compFun(inc_array[inc_current], dec_array[i]) <= 0)
		rChain.processNewVertex(dec_array[i], pStream);
	      else 
		break;
	    }
	  rChain.outputFan(inc_array[inc_current], pStream);
	  monoTriangulationRecFunGen(dec_array[i-1], botVertex, 
			       inc_chain, inc_current, inc_end,
			       dec_chain, i, dec_end,
			       compFun,
			       pStream);
	}
      else /*compFun(inc_array[inc_current], dec_array[dec_current]) > 0*/
	{

	  reflexChain rChain(20, 1);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=inc_current; i<=inc_end; i++)
	    {
	      if(compFun(inc_array[i], dec_array[dec_current]) >0)		
		rChain.processNewVertex(inc_array[i], pStream);	      
	      else
		break;
	    }
	  rChain.outputFan(dec_array[dec_current], pStream);
	  monoTriangulationRecFunGen(inc_array[i-1], botVertex, 
			       inc_chain, i,inc_end,
			       dec_chain, dec_current,dec_end,
			       compFun,
			       pStream);
	}
    }/*end case neither is empty*/
}
   
/*if compFun == compV2InY, top to bottom: V-monotone
 *if compFun == compV2InX, right to left: U-monotone
 */
void monoTriangulationRecFun(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			  Int  (*compFun)(Real*, Real*),
			  primStream* pStream)
{
  assert( inc_chain != NULL && dec_chain != NULL);
  assert( ! (inc_current>=inc_chain->getNumElements() &&
	     dec_current>=dec_chain->getNumElements()));
  Int inc_nVertices;
  Int dec_nVertices;
  Real** inc_array ;
  Real** dec_array ;
  Int i;
  assert( ! ( (inc_chain==NULL) && (dec_chain==NULL)));

  if(inc_current>=inc_chain->getNumElements()) /*no more vertices on inc_chain*/
    {

      dec_array = dec_chain->getArray();
      dec_nVertices = dec_chain->getNumElements();      
      reflexChain rChain(20,0);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the dec_chain*/
      for(i=dec_current; i<dec_nVertices; i++){
	rChain.processNewVertex(dec_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);

    }
  else if(dec_current>= dec_chain->getNumElements()) /*no more vertices on dec_chain*/
    {
      inc_array = inc_chain->getArray();
      inc_nVertices= inc_chain->getNumElements();
      reflexChain rChain(20,1);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the inc_chain*/
      for(i=inc_current; i<inc_nVertices; i++){
	rChain.processNewVertex(inc_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);
    }
  else /*neither chain is empty*/
    {
      inc_array = inc_chain -> getArray();
      dec_array = dec_chain -> getArray();
      inc_nVertices= inc_chain->getNumElements();
      dec_nVertices= dec_chain->getNumElements();
      /*if top of inc_chain is 'lower' than top of dec_chain, process all the 
       *vertices on the dec_chain which are higher than top of inc_chain
       */
      if(compFun(inc_array[inc_current], dec_array[dec_current]) <= 0)
	{

	  reflexChain rChain(20, 0);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=dec_current; i<dec_nVertices; i++)
	    {
	      if(compFun(inc_array[inc_current], dec_array[i]) <= 0)
		rChain.processNewVertex(dec_array[i], pStream);
	      else 
		break;
	    }
	  rChain.outputFan(inc_array[inc_current], pStream);
	  monoTriangulationRecFun(dec_array[i-1], botVertex, 
			       inc_chain, inc_current,
			       dec_chain, i,
			       compFun,
			       pStream);
	}
      else /*compFun(inc_array[inc_current], dec_array[dec_current]) > 0*/
	{

	  reflexChain rChain(20, 1);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=inc_current; i<inc_nVertices; i++)
	    {
	      if(compFun(inc_array[i], dec_array[dec_current]) >0)		
		rChain.processNewVertex(inc_array[i], pStream);	      
	      else
		break;
	    }
	  rChain.outputFan(dec_array[dec_current], pStream);
	  monoTriangulationRecFun(inc_array[i-1], botVertex, 
			       inc_chain, i,
			       dec_chain, dec_current,
			       compFun,
			       pStream);
	}
    }/*end case neither is empty*/
}


void monoTriangulationRec(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			  primStream* pStream)
{
  assert( inc_chain != NULL && dec_chain != NULL);
  assert( ! (inc_current>=inc_chain->getNumElements() &&
	     dec_current>=dec_chain->getNumElements()));
  Int inc_nVertices;
  Int dec_nVertices;
  Real** inc_array ;
  Real** dec_array ;
  Int i;
  assert( ! ( (inc_chain==NULL) && (dec_chain==NULL)));

  if(inc_current>=inc_chain->getNumElements()) /*no more vertices on inc_chain*/
    {

      dec_array = dec_chain->getArray();
      dec_nVertices = dec_chain->getNumElements();      
      reflexChain rChain(20,0);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the dec_chain*/
      for(i=dec_current; i<dec_nVertices; i++){
	rChain.processNewVertex(dec_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);

    }
  else if(dec_current>= dec_chain->getNumElements()) /*no more vertices on dec_chain*/
    {
      inc_array = inc_chain->getArray();
      inc_nVertices= inc_chain->getNumElements();
      reflexChain rChain(20,1);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, pStream);
      /*process all the vertices on the inc_chain*/
      for(i=inc_current; i<inc_nVertices; i++){
	rChain.processNewVertex(inc_array[i], pStream);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, pStream);
    }
  else /*neither chain is empty*/
    {
      inc_array = inc_chain -> getArray();
      dec_array = dec_chain -> getArray();
      inc_nVertices= inc_chain->getNumElements();
      dec_nVertices= dec_chain->getNumElements();
      /*if top of inc_chain is 'lower' than top of dec_chain, process all the 
       *vertices on the dec_chain which are higher than top of inc_chain
       */
      if(compV2InY(inc_array[inc_current], dec_array[dec_current]) <= 0)
	{

	  reflexChain rChain(20, 0);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=dec_current; i<dec_nVertices; i++)
	    {
	      if(compV2InY(inc_array[inc_current], dec_array[i]) <= 0)
		rChain.processNewVertex(dec_array[i], pStream);
	      else 
		break;
	    }
	  rChain.outputFan(inc_array[inc_current], pStream);
	  monoTriangulationRec(dec_array[i-1], botVertex, 
			       inc_chain, inc_current,
			       dec_chain, i,
			       pStream);
	}
      else /*compV2InY(inc_array[inc_current], dec_array[dec_current]) > 0*/
	{

	  reflexChain rChain(20, 1);
	  rChain.processNewVertex(topVertex, pStream);
	  for(i=inc_current; i<inc_nVertices; i++)
	    {
	      if(compV2InY(inc_array[i], dec_array[dec_current]) >0)		
		rChain.processNewVertex(inc_array[i], pStream);	      
	      else
		break;
	    }
	  rChain.outputFan(dec_array[dec_current], pStream);
	  monoTriangulationRec(inc_array[i-1], botVertex, 
			       inc_chain, i,
			       dec_chain, dec_current,
			       pStream);
	}
    }/*end case neither is empty*/
}
    


/* the name here assumes that the polygon is Y-monotone, but 
 *this function also works for X-monotone polygons.
 * a monotne polygon consists of two extrem verteices: topVertex and botVertex, and
 *two monotone chains: inc_chain, and dec_chain. The edges of the increasing chain (inc_chain)
 *is ordered by following pointer: next, while  the edges of the decreasing chain (dec_chain)
 *is ordered by following pointer: prev
 * inc_index index the vertex which is the toppest of the inc_chain which we are handling currently.
 * dec_index index the vertex which is the toppest of the dec_chain which we are handling currently.
 */
void monoTriangulationRec(directedLine* inc_chain, Int inc_index, 
			  directedLine* dec_chain, Int dec_index, 
			  directedLine* topVertex, Int top_index,
			  directedLine* botVertex,
			  primStream* pStream)
{
  Int i;
  directedLine *temp, *oldtemp = NULL;
  Int tempIndex, oldtempIndex = 0;
  
  assert(inc_chain != NULL && dec_chain != NULL);
  
  if(inc_chain == botVertex) {
    reflexChain rChain(20, 0);
    rChain.processNewVertex(topVertex->getVertex(top_index), pStream);
    for(i=dec_index; i< dec_chain->get_npoints(); i++){
      rChain.processNewVertex(dec_chain->getVertex(i), pStream);
    }
    for(temp = dec_chain->getPrev(); temp != botVertex; temp = temp->getPrev())
      {
	for(i=0; i<temp->get_npoints(); i++){
	  rChain.processNewVertex(temp->getVertex(i), pStream);
	}
      }
  }
  else if(dec_chain==botVertex) {
    reflexChain rChain(20, 1);
    rChain.processNewVertex(topVertex->getVertex(top_index), pStream);
    for(i=inc_index; i< inc_chain->get_npoints(); i++){
      rChain.processNewVertex(inc_chain->getVertex(i), pStream);
    }
    for(temp = inc_chain->getPrev(); temp != botVertex; temp = temp->getNext())
      {
	for(i=0; i<temp->get_npoints(); i++){
	  rChain.processNewVertex(temp->getVertex(i), pStream);
	}
      }
  }
  else /*neither reached the bottom*/{
    if(compV2InY(inc_chain->getVertex(inc_index), dec_chain->getVertex(dec_index)) <=0) {
      reflexChain rChain(20, 0);
      rChain.processNewVertex(topVertex -> getVertex(top_index), pStream);
      temp = dec_chain;
      tempIndex = dec_index;
      while( compV2InY(inc_chain->getVertex(inc_index), temp->getVertex(tempIndex))<=0) {
	oldtemp = temp;
	oldtempIndex = tempIndex;
	rChain.processNewVertex(temp->getVertex(tempIndex), pStream);
	
	if(tempIndex == temp->get_npoints()-1){
	  tempIndex = 0;
	  temp = temp->getPrev();
	}
	else{
	  tempIndex++;
	}
      }
      rChain.outputFan(inc_chain->getVertex(inc_index), pStream);
      monoTriangulationRec(inc_chain, inc_index, temp, tempIndex, oldtemp, oldtempIndex, botVertex, pStream);
    }
    else /* >0*/ {
      reflexChain rChain(20, 1);
      rChain.processNewVertex(topVertex -> getVertex(top_index), pStream);
      temp = inc_chain;
      tempIndex = inc_index;
      while( compV2InY(temp->getVertex(tempIndex), dec_chain->getVertex(dec_index))>0){
	oldtemp = temp;
	oldtempIndex = tempIndex;
	rChain.processNewVertex(temp->getVertex(tempIndex), pStream);
	
	if(tempIndex == temp->get_npoints()-1){
	  tempIndex = 0;
	  temp = temp->getNext();
	}
	else{
	  tempIndex++;
	}
      }
      rChain.outputFan(dec_chain->getVertex(dec_index), pStream);
      monoTriangulationRec(temp, tempIndex, dec_chain, dec_index, oldtemp, oldtempIndex, botVertex, pStream); 
    }
  } /*end case neither reached the bottom*/
}

/***************************vertexArray begin here**********************************/
vertexArray::vertexArray(Real2* vertices, Int nVertices)
{
  Int i;
  size = index = nVertices;
  array = (Real**) malloc(sizeof(Real*) * nVertices);
  assert(array);
  for(i=0; i<nVertices; i++)
    {
      array[i] = vertices[i];
      array[i] = vertices[i];
    }
}

vertexArray::vertexArray(Int s)
{
  size = s;
  array = (Real**) malloc(sizeof(Real*) * s);
  assert(array);
  index = 0;
}

vertexArray::~vertexArray()
{
  free(array);
}

void vertexArray::appendVertex(Real* ptr)
{
  Int i;
  if(index >= size){
    Real** temp = (Real**) malloc(sizeof(Real*) * (2*size +1));
    assert(temp);
    for(i=0; i<index; i++)
      temp[i] = array[i];
    free(array);
    array = temp;
    size = 2*size+1;
  }
  array[index++] = ptr;
}

void vertexArray::print()
{
  printf("vertex Array:index=%i, size=%i\n", index, size);
  for(Int i=0; i<index; i++)
    {
      printf("(%f,%f) ", array[i][0], array[i][1]);
    }
  printf("\n");
}

/*find the first i such that array[i][1] >= v
 * and array[i+1][1] <v
 * if index == 0 (the array is empty, return -1.
 * if v is above all, return -1.
 * if v is below all, return index-1.
 */
Int vertexArray::findIndexAbove(Real v)
{
  Int i;
  if(index == 0) 
    return -1;
  else if(array[0][1] < v)
    return -1;
  else
    {
      for(i=1; i<index; i++)
	{
	  if(array[i][1] < v)
	    break;
	}
      return i-1;
    }
}

/*find the first i<=endIndex such that array[i][1] <= v
 * and array[i-1][1] > v
 *if sartIndex>endIndex, then return endIndex+1.
 *otherwise, startIndex<=endIndex, it is assumed that
 * 0<=startIndex<=endIndex<index.
 * if v is below all, return endIndex+1
 * if v is above all, return startIndex.
 */
Int vertexArray::findIndexBelowGen(Real v, Int startIndex, Int endIndex)
{
  Int i;
  if(startIndex > endIndex) 
    return endIndex+1;
  else if(array[endIndex][1] >  v)
    return endIndex+1;
  else //now array[endIndex][1] <= v
    {
      for(i=endIndex-1; i>=startIndex; i--)
	{
	  if(array[i][1] > v)
	    break;
	}
      return i+1;
    }
}

/*find the first i<=endIndex such that array[i-1][1] >= v
 * and array[i][1] < v
 *if sartIndex>endIndex, then return endIndex+1.
 *otherwise, startIndex<=endIndex, it is assumed that
 * 0<=startIndex<=endIndex<index.
 * if v is below or equal to all, return endIndex+1
 * if v is strictly above all, return startIndex.
 */
Int vertexArray::findIndexStrictBelowGen(Real v, Int startIndex, Int endIndex)
{
  Int i;
  if(startIndex > endIndex) 
    return endIndex+1;
  else if(array[endIndex][1] >=  v)
    return endIndex+1;
  else //now array[endIndex][1] < v
    {
      for(i=endIndex-1; i>=startIndex; i--)
	{
	  if(array[i][1] >= v)
	    break;
	}
      return i+1;
    }
}

/*find the first i>startIndex such that array[i-1][1] > v
 * and array[i][1] >=v
 *if sartIndex>endIndex, then return startIndex-1.
 *otherwise, startIndex<=endIndex, it is assumed that
 * 0<=startIndex<=endIndex<index.
 * if v is strictly above all, return startIndex-1
 * if v is strictly below all, return endIndex.
 */
Int vertexArray::findIndexFirstAboveEqualGen(Real v, Int startIndex, Int endIndex)
{

  Int i;
  if(startIndex > endIndex) 
    return startIndex-1;
  else if(array[startIndex][1] < v)
    return startIndex-1;
  else //now array[startIndex][1] >= v
    {

      for(i=startIndex; i<=endIndex; i++)
	{
	  if(array[i][1] <= v)
	    break;
	}
      if(i>endIndex) // v is strictly below all
	return endIndex;
      else if(array[i][1] == v)
	return i;
      else
	return i-1;
    }

}


/*find the first i>=startIndex such that array[i][1] >= v
 * and array[i+1][1] <v
 *if sartIndex>endIndex, then return startIndex-1.
 *otherwise, startIndex<=endIndex, it is assumed that
 * 0<=startIndex<=endIndex<index.
 * if v is above all, return startIndex-1
 * if v is below all, return endIndex.
 */
Int vertexArray::findIndexAboveGen(Real v, Int startIndex, Int endIndex)
{
  Int i;
  if(startIndex > endIndex) 
    return startIndex-1;
  else if(array[startIndex][1] < v)
    return startIndex-1;
  else //now array[startIndex][1] >= v
    {
      for(i=startIndex+1; i<=endIndex; i++)
	{
	  if(array[i][1] < v)
	    break;
	}
      return i-1;
    }
}

Int vertexArray::findDecreaseChainFromEnd(Int begin, Int end)
{
  Int i = end;
  Real prevU = array[i][0];
  Real thisU;
  for(i=end-1; i>=begin; i--){
    thisU = array[i][0];
    if(thisU < prevU)
      prevU = thisU;
    else
      break;
  }
  return i;
}

//if(V(start) == v, return start, other wise return the
//last i so that V(i)==v
Int vertexArray::skipEqualityFromStart(Real v, Int start, Int end)
{
  Int i;
  if(array[start][1] != v)
    return start;
  //now array[start][1] == v
  for(i=start+1; i<= end; i++)
    if(array[i][1] != v) 
      break;
  return i-1;
}
  

/***************************vertexArray end****************************************/



/***************************relfex chain stuff begin here*****************************/

reflexChain::reflexChain(Int size, Int is_increasing)
{
  queue = (Real2*) malloc(sizeof(Real2) * size);
  assert(queue);
  index_queue = 0;
  size_queue = size;
  isIncreasing = is_increasing;
}

reflexChain::~reflexChain()
{
  free(queue);
}

/*put (u,v) at the end of the queue
 *pay attention to space
 */
void reflexChain::insert(Real u, Real v)
{
  Int i;
  if(index_queue >= size_queue) {
    Real2 *temp = (Real2*) malloc(sizeof(Real2) * (2*size_queue+1));
    assert(temp);

    /*copy*/
    for(i=0; i<index_queue; i++){
      temp[i][0] = queue[i][0];
      temp[i][1] = queue[i][1];
    }
    
    free(queue);
    queue = temp;
    size_queue = 2*size_queue + 1;
  }

  queue[index_queue][0] = u;
  queue[index_queue][1] = v;
  index_queue ++;
}

void reflexChain::insert(Real v[2])
{
  insert(v[0], v[1]);
}

/*
static Real area(Real A[2], Real B[2], Real C[2])
{
  Real Bx, By, Cx, Cy;
  Bx = B[0] - A[0];
  By = B[1] - A[1];
  Cx = C[0] - A[0];
  Cy = C[1] - A[1];
  return Bx*Cy - Cx*By;
}
*/

/*the chain is reflex, and the vertex v is
 *on the other side of the chain, so that
 *we can outout the fan with v as the
 *the center
 */
void reflexChain::outputFan(Real v[2], primStream* pStream)
{
  Int i;
  pStream->begin();
  pStream->insert(v);
  if(isIncreasing) {
    for(i=0; i<index_queue; i++)
      pStream->insert(queue[i]);
  }
  else {
    for(i=index_queue-1; i>=0; i--)
      pStream->insert(queue[i]);    
  }
  pStream->end(PRIMITIVE_STREAM_FAN);
}

void reflexChain::processNewVertex(Real v[2], primStream* pStream)
{
  Int i,j,k;
  Int isReflex;
  /*if there are at most one vertex in the queue, then simply insert
   */
  if(index_queue <=1){
    insert(v);
    return;
  }
  
  /*there are at least two vertices in the queue*/
  j=index_queue-1;
  
  for(i=j; i>=1; i--) {
    if(isIncreasing) {
      isReflex = (area(queue[i-1], queue[i], v) <= 0.0);
    }
    else /*decreasing*/{
      isReflex = (area(v, queue[i], queue[i-1]) <= 0.0);	  
    }
    if(isReflex) {
      break;
    }
  }

  /*
   *if i<j then vertices: i+1--j are convex
   * output triangle fan: 
   *  v, and queue[i], i+1, ..., j
   */
  if(i<j) 
    {
      pStream->begin();
      pStream->insert(v);
      if(isIncreasing) {
	for(k=i; k<=j; k++)
	  pStream->insert(queue[k]);
      }
      else {
	for(k=j; k>=i; k--)
	  pStream->insert(queue[k]);
      }
      
      pStream->end(PRIMITIVE_STREAM_FAN);
    }

  /*delete vertices i+1--j from the queue*/
  index_queue = i+1;
  /*finally insert v at the end of the queue*/
  insert(v);

}

void reflexChain::print()
{
  Int i;
  printf("reflex chain: isIncreasing=%i\n", isIncreasing);
  for(i=0; i<index_queue; i++) {
    printf("(%f,%f) ", queue[i][0], queue[i][1]);
  }
  printf("\n");
}

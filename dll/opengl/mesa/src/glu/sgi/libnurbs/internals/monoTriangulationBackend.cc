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

#include "monoTriangulation.h"
#include "polyUtil.h"
#include "backend.h"
#include "arc.h"

void reflexChain::outputFan(Real v[2], Backend* backend)
{
  Int i;
  /*
  TrimVertex trimVert;
  */
  backend->bgntfan();

  /*
  trimVert.param[0]=v[0];
  trimVert.param[1]=v[1];
  backend->tmeshvert(&trimVert);
  */
  backend->tmeshvert(v[0], v[1]);

  if(isIncreasing) {
    for(i=0; i<index_queue; i++)
      {
	/*
	trimVert.param[0]=queue[i][0];
	trimVert.param[1]=queue[i][1];
	backend->tmeshvert(&trimVert);
	*/
	backend->tmeshvert(queue[i][0], queue[i][1]);
      }
  }
  else {
    for(i=index_queue-1; i>=0; i--)
      {
	/*
	trimVert.param[0]=queue[i][0];
	trimVert.param[1]=queue[i][1];
	backend->tmeshvert(&trimVert);
	*/
	backend->tmeshvert(queue[i][0], queue[i][1]);
      }
  }
  backend->endtfan();
}

void reflexChain::processNewVertex(Real v[2], Backend* backend)
{
  Int i,j,k;
  Int isReflex;
  /*TrimVertex trimVert;*/
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
      backend->bgntfan();
      /*
      trimVert.param[0]=v[0];
      trimVert.param[1]=v[1];
      backend->tmeshvert(& trimVert);
      */
      backend->tmeshvert(v[0], v[1]);

      if(isIncreasing) {
	for(k=i; k<=j; k++)
	  {
	    /*
	    trimVert.param[0]=queue[k][0];
	    trimVert.param[1]=queue[k][1];
	    backend->tmeshvert(& trimVert);
	    */
	    backend->tmeshvert(queue[k][0], queue[k][1]);
	  }
      }
      else {
	for(k=j; k>=i; k--)
	  {
	    /*
	    trimVert.param[0]=queue[k][0];
	    trimVert.param[1]=queue[k][1];
	    backend->tmeshvert(& trimVert);
	    */
	    backend->tmeshvert(queue[k][0], queue[k][1]);
	  }
      }
      
      backend->endtfan();
    }

  /*delete vertices i+1--j from the queue*/
  index_queue = i+1;
  /*finally insert v at the end of the queue*/
  insert(v);

}


void monoTriangulationRec(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			  Backend* backend)
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
      rChain.processNewVertex(topVertex, backend);
      /*process all the vertices on the dec_chain*/
      for(i=dec_current; i<dec_nVertices; i++){
	rChain.processNewVertex(dec_array[i], backend);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, backend);

    }
  else if(dec_current>= dec_chain->getNumElements()) /*no more vertices on dec_chain*/
    {
      inc_array = inc_chain->getArray();
      inc_nVertices= inc_chain->getNumElements();
      reflexChain rChain(20,1);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, backend);
      /*process all the vertices on the inc_chain*/
      for(i=inc_current; i<inc_nVertices; i++){
	rChain.processNewVertex(inc_array[i], backend);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, backend);
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
	  rChain.processNewVertex(topVertex, backend);
	  for(i=dec_current; i<dec_nVertices; i++)
	    {
	      if(compV2InY(inc_array[inc_current], dec_array[i]) <= 0)
		rChain.processNewVertex(dec_array[i], backend);
	      else 
		break;
	    }
	  rChain.outputFan(inc_array[inc_current], backend);
	  monoTriangulationRec(dec_array[i-1], botVertex, 
			       inc_chain, inc_current,
			       dec_chain, i,
			       backend);
	}
      else /*compV2InY(inc_array[inc_current], dec_array[dec_current]) > 0*/
	{

	  reflexChain rChain(20, 1);
	  rChain.processNewVertex(topVertex, backend);
	  for(i=inc_current; i<inc_nVertices; i++)
	    {
	      if(compV2InY(inc_array[i], dec_array[dec_current]) >0)		
		rChain.processNewVertex(inc_array[i], backend);	      
	      else
		break;
	    }
	  rChain.outputFan(dec_array[dec_current], backend);
	  monoTriangulationRec(inc_array[i-1], botVertex, 
			       inc_chain, i,
			       dec_chain, dec_current,
			       backend);
	}
    }/*end case neither is empty*/
}


void monoTriangulationFunBackend(Arc_ptr loop, Int (*compFun)(Real*, Real*), Backend* backend)
{
  Int i;
  /*find the top vertex, bottom vertex, inccreasing chain, and decreasing chain,
   *then call monoTriangulationRec
   */
  Arc_ptr tempV;
  Arc_ptr topV;
  Arc_ptr botV;
  topV = botV = loop;
  for(tempV = loop->next; tempV != loop; tempV = tempV->next)
    {
      if(compFun(topV->tail(), tempV->tail())<0) {
	topV = tempV;
      }
      if(compFun(botV->tail(), tempV->tail())>0) {
	botV = tempV;
      }
    }

  /*creat increase and decrease chains*/
  vertexArray inc_chain(20); /*this is a dynamic array*/
  for(i=1; i<=topV->pwlArc->npts-2; i++) { /*the first vertex is the top vertex which doesn't belong to inc_chain*/
    inc_chain.appendVertex(topV->pwlArc->pts[i].param);
  }
  for(tempV = topV->next; tempV != botV; tempV = tempV->next)
    {
      for(i=0; i<=tempV->pwlArc->npts-2; i++){
	inc_chain.appendVertex(tempV->pwlArc->pts[i].param);
      }
    }
  
  vertexArray dec_chain(20);
  for(tempV = topV->prev; tempV != botV; tempV = tempV->prev)
    {
      for(i=tempV->pwlArc->npts-2; i>=0; i--){
	dec_chain.appendVertex(tempV->pwlArc->pts[i].param);
      }
    }
  for(i=botV->pwlArc->npts-2; i>=1; i--){ 
    dec_chain.appendVertex(tempV->pwlArc->pts[i].param);
  }
  
  monoTriangulationRecFunBackend(topV->tail(), botV->tail(), &inc_chain, 0, &dec_chain, 0, compFun, backend);

}  

/*if compFun == compV2InY, top to bottom: V-monotone
 *if compFun == compV2InX, right to left: U-monotone
 */
void monoTriangulationRecFunBackend(Real* topVertex, Real* botVertex, 
			  vertexArray* inc_chain, Int inc_current,
			  vertexArray* dec_chain, Int dec_current,
			  Int  (*compFun)(Real*, Real*),
			  Backend* backend)
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
      rChain.processNewVertex(topVertex, backend);
      /*process all the vertices on the dec_chain*/
      for(i=dec_current; i<dec_nVertices; i++){
	rChain.processNewVertex(dec_array[i], backend);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, backend);

    }
  else if(dec_current>= dec_chain->getNumElements()) /*no more vertices on dec_chain*/
    {
      inc_array = inc_chain->getArray();
      inc_nVertices= inc_chain->getNumElements();
      reflexChain rChain(20,1);
      /*put the top vertex into the reflex chain*/
      rChain.processNewVertex(topVertex, backend);
      /*process all the vertices on the inc_chain*/
      for(i=inc_current; i<inc_nVertices; i++){
	rChain.processNewVertex(inc_array[i], backend);
      }
      /*process the bottom vertex*/
      rChain.processNewVertex(botVertex, backend);
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
	  rChain.processNewVertex(topVertex, backend);
	  for(i=dec_current; i<dec_nVertices; i++)
	    {
	      if(compFun(inc_array[inc_current], dec_array[i]) <= 0)
		rChain.processNewVertex(dec_array[i], backend);
	      else 
		break;
	    }
	  rChain.outputFan(inc_array[inc_current], backend);
	  monoTriangulationRecFunBackend(dec_array[i-1], botVertex, 
			       inc_chain, inc_current,
			       dec_chain, i,
			       compFun,
			       backend);
	}
      else /*compFun(inc_array[inc_current], dec_array[dec_current]) > 0*/
	{

	  reflexChain rChain(20, 1);
	  rChain.processNewVertex(topVertex, backend);
	  for(i=inc_current; i<inc_nVertices; i++)
	    {
	      if(compFun(inc_array[i], dec_array[dec_current]) >0)		
		rChain.processNewVertex(inc_array[i], backend);	      
	      else
		break;
	    }
	  rChain.outputFan(dec_array[dec_current], backend);
	  monoTriangulationRecFunBackend(inc_array[i-1], botVertex, 
			       inc_chain, i,
			       dec_chain, dec_current,
			       compFun,
			       backend);
	}
    }/*end case neither is empty*/
}

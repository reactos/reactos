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
*/

/*
 * quilt.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "quilt.h"
#include "backend.h"
#include "mapdesc.h"
#include "flist.h"
#include "patchlist.h"
#include "simplemath.h" //min()

/* local preprocessor definitions */
#define DEF_PATCH_STEPSIZE	.4
#define fsizeof(x)		(sizeof(x)/sizeof(REAL))


Quilt::Quilt( Mapdesc *_mapdesc )
{
    mapdesc = _mapdesc;
}
 
void
Quilt::deleteMe( Pool& p )
{
    for( Quiltspec *q=qspec; q != eqspec; q++ ) {
#if 1
	if( q->breakpoints) delete[] q->breakpoints;  q->breakpoints = 0;
#else
	if( q->breakpoints) {
	   delete[] q->breakpoints;  
	   q->breakpoints = 0;
printf("in here\n");
        }
#endif
    }
    if( cpts ) delete[] cpts;  
    cpts = 0;
    PooledObj::deleteMe( p );
}

void
Quilt::show( void )
{
#ifndef NDEBUG
    int nc = mapdesc->getNcoords();
    REAL *ps  = cpts; 
    ps += qspec[0].offset;
    ps += qspec[1].offset;
    for( int i=0; i!= qspec[0].order * qspec[0].width; i++ ) {
	for( int j = 0; j!= qspec[1].order * qspec[1].width; j++ ) {
	    for( int k=0; k < nc; k++ )
		_glu_dprintf(  "%g ", ps[i*qspec[0].stride + j*qspec[1].stride + k] );
	    _glu_dprintf(  "\n" );
	}
	_glu_dprintf(  "\n" );
    }
    _glu_dprintf( "\n" );
#endif
}

/*--------------------------------------------------------------------------
 * Quilt::select - find which map in each quilt contains the points
 *			pta and ptb with pta[i] < ptb[i]
 *--------------------------------------------------------------------------
 */

void
Quilt::select( REAL *pta, REAL *ptb )
{
    int dim = eqspec - qspec; 
    int i, j;
    for( i=0; i<dim; i++) {
	for( j=qspec[i].width-1; j>=0; j-- )
	    if( (qspec[i].breakpoints[j] <= pta[i]   ) &&
	    	(ptb[i] <= qspec[i].breakpoints[j+1] ) ) 
		 break;
	assert( j != -1 ); 
	qspec[i].index = j;
    }
}

void 
Quilt::download( Backend &backend )
{
    if( getDimension() == 2 ) {
	REAL *ps  = cpts; 
	ps += qspec[0].offset;
	ps += qspec[1].offset;
	ps += qspec[0].index * qspec[0].order * qspec[0].stride;
	ps += qspec[1].index * qspec[1].order * qspec[1].stride;
	backend.surfpts( mapdesc->getType(), ps, 
		  qspec[0].stride,
		  qspec[1].stride,
		  qspec[0].order,
		  qspec[1].order,
		  qspec[0].breakpoints[qspec[0].index],
		  qspec[0].breakpoints[qspec[0].index+1],
		  qspec[1].breakpoints[qspec[1].index],
		  qspec[1].breakpoints[qspec[1].index+1] );
    } else {
	REAL *ps  = cpts; 
	ps += qspec[0].offset;
	ps += qspec[0].index * qspec[0].order * qspec[0].stride;
	backend.curvpts( mapdesc->getType(), ps, 
		      qspec[0].stride,
		      qspec[0].order,
		      qspec[0].breakpoints[qspec[0].index],
		      qspec[0].breakpoints[qspec[0].index+1] );
    }
}

/*--------------------------------------------------------------------------
 * Quilt::downloadAll - download each map that contains the current patch
 *--------------------------------------------------------------------------
 */

void
Quilt::downloadAll( REAL *pta, REAL *ptb, Backend &backend )
{
    for( Quilt *m = this; m; m=m->next ) {
	m->select( pta, ptb );
	m->download( backend );
    }
}

/*--------------------------------------------------------------------------
  * Quilt::isCulled - determine if an entire quilt is trivially rejected.
 *--------------------------------------------------------------------------
 */

int
Quilt::isCulled( void )
{
    if( mapdesc->isCulling() )
	return mapdesc->xformAndCullCheck( cpts + qspec[0].offset + qspec[1].offset,
			qspec[0].order * qspec[0].width, qspec[0].stride, 
			qspec[1].order * qspec[1].width, qspec[1].stride );
    else
	return CULL_ACCEPT;
}

/*---------------------------------------------------------------------------
 * Quilt::getRange - retrieve the valid paramater range of a set of quilts
 *---------------------------------------------------------------------------
 */
void
Quilt::getRange( REAL *from, REAL *to, Flist& slist, Flist &tlist )
{
    getRange( from, to, 0, slist );
    getRange( from, to, 1, tlist );
}

/*---------------------------------------------------------------------------
 * Quilt::getRange - retrieve the valid paramater range of a set of quilts
 *---------------------------------------------------------------------------
 */
void
Quilt::getRange( REAL *from, REAL *to, int i, Flist &list )
{
    Quilt *maps = this;
    from[i] = maps->qspec[i].breakpoints[0];
    to[i]   = maps->qspec[i].breakpoints[maps->qspec[i].width];
    int maxpts = 0;
    Quilt_ptr m;    
    for( m=maps; m; m=m->next ) {
	if( m->qspec[i].breakpoints[0] > from[i] ) 
	    from[i] = m->qspec[i].breakpoints[0]; 
	if( m->qspec[i].breakpoints[m->qspec[i].width] < to[i] ) 
	    to[i] = m->qspec[i].breakpoints[m->qspec[i].width]; 
	maxpts += m->qspec[i].width + 1;
    }

    list.grow( maxpts );

    for( m=maps; m; m=m->next )
	for( int j=0; j<=m->qspec[i].width; j++ ) {
	    list.add( m->qspec[i].breakpoints[j] );
	}

    list.filter( );
    list.taper( from[i], to[i] );
}

void
Quilt::getRange( REAL *from, REAL *to, Flist& slist )
{
    getRange( from, to, 0, slist );
}

void
Quilt::findRates( Flist& slist, Flist& tlist, REAL rate[2] )
{
    findSampleRates( slist, tlist );
    rate[0] = qspec[0].step_size;
    rate[1] = qspec[1].step_size;

    for( Quilt *q = next; q; q = q->next ) {
	q->findSampleRates( slist, tlist );
	if( q->qspec[0].step_size < rate[0] ) 
	    rate[0] = q->qspec[0].step_size;
	if( q->qspec[1].step_size < rate[1] ) 
	    rate[1] = q->qspec[1].step_size;
    }
}

void
Quilt::findSampleRates( Flist& slist, Flist& tlist )
{
    qspec[0].step_size = DEF_PATCH_STEPSIZE * 
	    (qspec[0].breakpoints[qspec[0].width] - qspec[0].breakpoints[0]); 
    qspec[1].step_size = DEF_PATCH_STEPSIZE * 
	    (qspec[1].breakpoints[qspec[1].width] - qspec[1].breakpoints[0]); 
 
    for( int i = slist.start; i < slist.end-1; i++ ) {
	for( int j = tlist.start; j < tlist.end-1; j++ ) {

	    REAL pta[2], ptb[2];
	    pta[0] = slist.pts[i];
	    ptb[0] = slist.pts[i+1];
	    pta[1] = tlist.pts[j];
	    ptb[1] = tlist.pts[j+1];
	    Patchlist patchlist( this, pta, ptb );
	    patchlist.getstepsize();
	    
	    {
	    float edge_len_s = min(glu_abs(ptb[0]-pta[0]),1.0);
	    float edge_len_t = min(glu_abs(ptb[1]-pta[1]),1.0);

	    if( patchlist.getStepsize(0)/edge_len_s < qspec[0].step_size ) 
	       qspec[0].step_size = patchlist.getStepsize(0)/edge_len_s;
	    if( patchlist.getStepsize(1)/edge_len_t < qspec[1].step_size ) 
	      qspec[1].step_size = patchlist.getStepsize(1)/edge_len_t;
	    }
	}
    }
}

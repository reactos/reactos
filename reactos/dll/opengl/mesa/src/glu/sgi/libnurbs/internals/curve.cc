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
 * curve.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "mymath.h"
#include "curve.h"
#include "mapdesc.h"
#include "types.h"
#include "quilt.h"
#include "nurbsconsts.h"

/*--------------------------------------------------------------------------
 * Curve::Curve - copy curve from quilt and transform control points
 *--------------------------------------------------------------------------
 */

Curve::Curve( Quilt_ptr geo, REAL pta, REAL ptb, Curve *c )
{
    mapdesc = geo->mapdesc;
    next = c;
    needsSampling = mapdesc->isRangeSampling() ? 1 : 0;
    cullval = mapdesc->isCulling() ? CULL_ACCEPT : CULL_TRIVIAL_ACCEPT;
    order = geo->qspec[0].order;
    stride = MAXCOORDS;
    for( int i = 0; i < MAXORDER * MAXCOORDS; i++ ) {
        cpts[i] = 0;
        spts[i] = 0;
    }
    stepsize = 0;
    minstepsize = 0;

    REAL *ps  = geo->cpts; 
    Quiltspec_ptr qs = geo->qspec;
    ps += qs->offset;
    ps += qs->index * qs->order * qs->stride;

    if( needsSampling )
	mapdesc->xformSampling( ps, qs->order, qs->stride, spts, stride );
	
    if( cullval == CULL_ACCEPT )
	mapdesc->xformCulling(  ps, qs->order, qs->stride, cpts, stride );

    /* set untrimmed curve range */
    range[0] = qs->breakpoints[qs->index];
    range[1] = qs->breakpoints[qs->index+1];
    range[2] = range[1] - range[0];

    if( range[0] != pta ) {
	Curve lower( *this, pta, 0 );
	lower.next = next;
	*this = lower;
    }
    if( range[1] != ptb ) {
	Curve lower( *this, ptb, 0 );
    }
}

/*--------------------------------------------------------------------------
 * Curve::Curve - subdivide a curve along an isoparametric line
 *--------------------------------------------------------------------------
 */

Curve::Curve( Curve& upper, REAL value, Curve *c )
{
    Curve &lower = *this;

    lower.next = c;
    lower.mapdesc = upper.mapdesc;
    lower.needsSampling = upper.needsSampling;
    lower.order = upper.order;
    lower.stride = upper.stride;
    lower.cullval = upper.cullval;

    REAL d = (value - upper.range[0]) / upper.range[2];

    if( needsSampling )
        mapdesc->subdivide( upper.spts, lower.spts, d, upper.stride, upper.order );

    if( cullval == CULL_ACCEPT ) 
        mapdesc->subdivide( upper.cpts, lower.cpts, d, upper.stride, upper.order );

    lower.range[0] = upper.range[0];
    lower.range[1] = value;
    lower.range[2] = value - upper.range[0];
    upper.range[0] = value;
    upper.range[2] = upper.range[1] - value;
}


/*--------------------------------------------------------------------------
 * Curve::clamp - clamp the sampling rate to a given maximum
 *--------------------------------------------------------------------------
 */

void
Curve::clamp( void )
{
    if( stepsize < minstepsize )
        stepsize = mapdesc->clampfactor * minstepsize;
}

void
Curve::setstepsize( REAL max )
{
    stepsize = ( max >= 1.0 ) ? (range[2] / max) : range[2];
    minstepsize = stepsize;
}

void
Curve::getstepsize( void )
{
    minstepsize= 0;

    if( mapdesc->isConstantSampling() ) {
	// fixed number of samples per patch in each direction
	// maxrate is number of s samples per patch
        setstepsize( mapdesc->maxrate );
    } else if( mapdesc->isDomainSampling() ) {
	// maxrate is number of s samples per unit s length of domain
        setstepsize( mapdesc->maxrate * range[2] );
    } else {
	// upper bound on path length between sample points

	assert( order <= MAXORDER );
    
	/* points have been transformed, therefore they are homogeneous */
        REAL tmp[MAXORDER][MAXCOORDS];
	const int tstride = sizeof(tmp[0]) / sizeof(REAL);
	int val = mapdesc->project( spts, stride, &tmp[0][0], tstride,  order ); 

        if( val == 0 ) {
	    // control points cross infinity, therefore derivatives are undefined
            setstepsize( mapdesc->maxrate );
        } else {
            REAL t = mapdesc->getProperty( N_PIXEL_TOLERANCE );
	    if( mapdesc->isParametricDistanceSampling() ) {
		REAL d = mapdesc->calcPartialVelocity( &tmp[0][0], tstride, order, 2, range[2] );
		stepsize = (d > 0.0) ? sqrtf( 8.0 * t / d ) : range[2];
		minstepsize = ( mapdesc->maxrate > 0.0 ) ? (range[2] / mapdesc->maxrate) : 0.0;
	    } else if( mapdesc->isPathLengthSampling() ) {
		// t is upper bound on path (arc) length
		REAL d = mapdesc->calcPartialVelocity( &tmp[0][0], tstride, order, 1, range[2] );
		stepsize = ( d > 0.0 ) ? (t / d) : range[2];
		minstepsize = ( mapdesc->maxrate > 0.0 ) ? (range[2] / mapdesc->maxrate) : 0.0;
	    } else {
		// control points cross infinity, therefore partials are undefined
		setstepsize( mapdesc->maxrate );
	    }
	}
    }
}

int
Curve::needsSamplingSubdivision( void )
{
    return ( stepsize < minstepsize )  ? 1 : 0;
}

int
Curve::cullCheck( void )
{
    if( cullval == CULL_ACCEPT ) 
	cullval = mapdesc->cullCheck( cpts, order, stride );
    return cullval;
}


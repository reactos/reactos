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
 * mapdesc.c++
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/mapdesc.cc,v 1.1 2004/02/02 16:39:11 navaraf Exp $
 */

#include <stdio.h>
#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "mystring.h"
#include "mymath.h"
#include "backend.h"
#include "nurbsconsts.h"
#include "mapdesc.h"

Mapdesc::Mapdesc( long _type, int _israt, int _ncoords, Backend& b ) 
    : backend( b )
{
    type 		= _type;
    isrational 		= _israt;
    ncoords 		= _ncoords;
    hcoords		= _ncoords + (_israt ? 0 : 1 );
    inhcoords		= _ncoords - (_israt ? 1 : 0 );
    mask 		= ((1<<(inhcoords*2))-1);
    next		= 0;

    assert( hcoords <= MAXCOORDS );
    assert( inhcoords >= 1 );

    pixel_tolerance 	= 1.0;
    error_tolerance	= 1.0;
    bbox_subdividing	= N_NOBBOXSUBDIVISION;
    culling_method 	= N_NOCULLING;		
    sampling_method 	= N_NOSAMPLING;
    clampfactor 	= N_NOCLAMPING;
    minsavings 		= N_NOSAVINGSSUBDIVISION;
    s_steps  		= 0.0;
    t_steps 		= 0.0;
    maxrate 		= ( s_steps < 0.0 ) ? 0.0 : s_steps;
    maxsrate 		= ( s_steps < 0.0 ) ? 0.0 : s_steps;
    maxtrate 		= ( t_steps < 0.0 ) ? 0.0 : t_steps;
    identify( bmat );
    identify( cmat );
    identify( smat );
    for( int i = 0; i != inhcoords; i++ )
	bboxsize[i] = 1.0;
}

void
Mapdesc::setBboxsize( INREAL *mat )
{
    for( int i = 0; i != inhcoords; i++ )
	bboxsize[i] = (REAL) mat[i];
}

void
Mapdesc::identify( REAL dest[MAXCOORDS][MAXCOORDS] )
{
    memset( dest, 0, sizeof( dest ) );
    for( int i=0; i != hcoords; i++ )
	dest[i][i] = 1.0;
}

void
Mapdesc::surfbbox( REAL bb[2][MAXCOORDS] )
{
    backend.surfbbox( type, bb[0], bb[1] );
}

void 
Mapdesc::copy( REAL dest[MAXCOORDS][MAXCOORDS], long n, INREAL *src,
	long rstride, long cstride )
{
    assert( n >= 0 );
    for( int i=0; i != n; i++ )
        for( int j=0; j != n; j++ )
	    dest[i][j] = src[i*rstride + j*cstride];
}

/*--------------------------------------------------------------------------
 * copyPt - copy a homogeneous point
 *--------------------------------------------------------------------------
 */
void
Mapdesc::copyPt( REAL *d, REAL *s )
{
    assert( hcoords > 0 );
    switch( hcoords  ) {
	case 4: 
	    d[3] = s[3];
	    d[2] = s[2];
	    d[1] = s[1];
	    d[0] = s[0];
	    break;
	case 3: 
	    d[2] = s[2];
	    d[1] = s[1];
	    d[0] = s[0];
	    break;
	case 2: 
	    d[1] = s[1];
	    d[0] = s[0];
	    break;
	case 1: 
	    d[0] = s[0];
	    break;
	case 5: 
	    d[4] = s[4];
	    d[3] = s[3];
	    d[2] = s[2];
	    d[1] = s[1];
	    d[0] = s[0];
	    break;
	default:
	    memcpy( d, s, hcoords * sizeof( REAL ) );
	    break;
    }
}

/*--------------------------------------------------------------------------
 * sumPt - compute affine combination of two homogeneous points
 *--------------------------------------------------------------------------
 */
void
Mapdesc::sumPt( REAL *dst, REAL *src1, REAL *src2, register REAL alpha, register REAL beta )
{
    assert( hcoords > 0 );
    switch( hcoords  ) {
	case 4: 
	    dst[3] = src1[3] * alpha + src2[3] * beta;
	    dst[2] = src1[2] * alpha + src2[2] * beta;
	    dst[1] = src1[1] * alpha + src2[1] * beta;
	    dst[0] = src1[0] * alpha + src2[0] * beta;
	    break;
	case 3: 
	    dst[2] = src1[2] * alpha + src2[2] * beta;
	    dst[1] = src1[1] * alpha + src2[1] * beta;
	    dst[0] = src1[0] * alpha + src2[0] * beta;
	    break;
	case 2: 
	    dst[1] = src1[1] * alpha + src2[1] * beta;
	    dst[0] = src1[0] * alpha + src2[0] * beta;
	    break;
	case 1: 
	    dst[0] = src1[0] * alpha + src2[0] * beta;
	    break;
	case 5: 
	    dst[4] = src1[4] * alpha + src2[4] * beta;
	    dst[3] = src1[3] * alpha + src2[3] * beta;
	    dst[2] = src1[2] * alpha + src2[2] * beta;
	    dst[1] = src1[1] * alpha + src2[1] * beta;
	    dst[0] = src1[0] * alpha + src2[0] * beta;
	    break;
	default: {
		for( int i = 0; i != hcoords; i++ )
		    dst[i] = src1[i] * alpha + src2[i] * beta;
            }
	    break;
    }
}

/*--------------------------------------------------------------------------
 * clipbits - compute bit-vector indicating point/window position
 *		       of a (transformed) homogeneous point
 *--------------------------------------------------------------------------
 */
unsigned int
Mapdesc::clipbits( REAL *p )
{
    assert( inhcoords >= 0 );
    assert( inhcoords <= 3 );

    register int nc = inhcoords;
    register REAL pw = p[nc];
    register REAL nw = -pw;
    register unsigned int bits = 0;

    if( pw == 0.0 ) return mask;

    if( pw > 0.0 ) {
	switch( nc ) {
	case 3:
	    if( p[2] <= pw ) bits |= (1<<5);
	    if( p[2] >= nw ) bits |= (1<<4);
	    if( p[1] <= pw ) bits |= (1<<3);
	    if( p[1] >= nw ) bits |= (1<<2);
	    if( p[0] <= pw ) bits |= (1<<1);
	    if( p[0] >= nw ) bits |= (1<<0);
            return bits;
	case 2:
	    if( p[1] <= pw ) bits |= (1<<3);
	    if( p[1] >= nw ) bits |= (1<<2);
	    if( p[0] <= pw ) bits |= (1<<1);
	    if( p[0] >= nw ) bits |= (1<<0);
            return bits;
	case 1:
	    if( p[0] <= pw ) bits |= (1<<1);
	    if( p[0] >= nw ) bits |= (1<<0);
            return bits;
	default: {
		int bit = 1;
		for( int i=0; i<nc; i++ ) {
		    if( p[i] >= nw ) bits |= bit; 
		    bit <<= 1;
		    if( p[i] <= pw ) bits |= bit; 
		    bit <<= 1;
		}
		abort();
		break;
	    }
	}
    } else { 
	switch( nc ) {
	case 3:
	    if( p[2] <= nw ) bits |= (1<<5);
	    if( p[2] >= pw ) bits |= (1<<4);
	    if( p[1] <= nw ) bits |= (1<<3);
	    if( p[1] >= pw ) bits |= (1<<2);
	    if( p[0] <= nw ) bits |= (1<<1);
	    if( p[0] >= pw ) bits |= (1<<0);
            return bits;
	case 2:
	    if( p[1] <= nw ) bits |= (1<<3);
	    if( p[1] >= pw ) bits |= (1<<2);
	    if( p[0] <= nw ) bits |= (1<<1);
	    if( p[0] >= pw ) bits |= (1<<0);
            return bits;
	case 1:
	    if( p[0] <= nw ) bits |= (1<<1);
	    if( p[0] >= pw ) bits |= (1<<0);
            return bits;
	default: {
		int bit = 1; 
		for( int i=0; i<nc; i++ ) {
		    if( p[i] >= pw ) bits |= bit; 
		    bit <<= 1;
		    if( p[i] <= nw ) bits |= bit; 
		    bit <<= 1;
		}
		abort();
		break;
	    }
	}
    }
    return bits;
}

/*--------------------------------------------------------------------------
 * xformRational - transform a homogeneous point
 *--------------------------------------------------------------------------
 */
void
Mapdesc::xformRational( Maxmatrix mat, REAL *d, REAL *s )
{
    assert( hcoords >= 0 );

    if( hcoords == 3 ) {
	REAL x = s[0];
	REAL y = s[1];
	REAL z = s[2];
	d[0] = x*mat[0][0]+y*mat[1][0]+z*mat[2][0];
	d[1] = x*mat[0][1]+y*mat[1][1]+z*mat[2][1];
	d[2] = x*mat[0][2]+y*mat[1][2]+z*mat[2][2];
    } else if( hcoords == 4 ) {
	REAL x = s[0];
	REAL y = s[1];
	REAL z = s[2];
	REAL w = s[3];
	d[0] = x*mat[0][0]+y*mat[1][0]+z*mat[2][0]+w*mat[3][0];
	d[1] = x*mat[0][1]+y*mat[1][1]+z*mat[2][1]+w*mat[3][1];
	d[2] = x*mat[0][2]+y*mat[1][2]+z*mat[2][2]+w*mat[3][2];
	d[3] = x*mat[0][3]+y*mat[1][3]+z*mat[2][3]+w*mat[3][3];
    } else {
	for( int i=0; i != hcoords; i++ ) {
	    d[i] = 0;
	    for( int j = 0; j != hcoords; j++ )
		d[i] += s[j] * mat[j][i];
	}
    }
}

/*--------------------------------------------------------------------------
 * xformNonrational - transform a inhomogeneous point to a homogeneous point
 *--------------------------------------------------------------------------
 */
void
Mapdesc::xformNonrational( Maxmatrix mat, REAL *d, REAL *s )
{
    if( inhcoords == 2 ) {
	REAL x = s[0];
	REAL y = s[1];
	d[0] = x*mat[0][0]+y*mat[1][0]+mat[2][0];
	d[1] = x*mat[0][1]+y*mat[1][1]+mat[2][1];
	d[2] = x*mat[0][2]+y*mat[1][2]+mat[2][2];
    } else if( inhcoords == 3 ) {
	REAL x = s[0];
	REAL y = s[1];
	REAL z = s[2];
	d[0] = x*mat[0][0]+y*mat[1][0]+z*mat[2][0]+mat[3][0];
	d[1] = x*mat[0][1]+y*mat[1][1]+z*mat[2][1]+mat[3][1];
	d[2] = x*mat[0][2]+y*mat[1][2]+z*mat[2][2]+mat[3][2];
	d[3] = x*mat[0][3]+y*mat[1][3]+z*mat[2][3]+mat[3][3];
    } else {
        assert( inhcoords >= 0 );
	for( int i=0; i != hcoords; i++ ) {
	    d[i] = mat[inhcoords][i];
	    for( int j = 0; j < inhcoords; j++ )
		d[i] += s[j] * mat[j][i];
	}
    }
}

/*--------------------------------------------------------------------------
 * xformAndCullCheck - transform a set of points that may be EITHER 
 *	homogeneous or inhomogeneous depending on the map description and
 *	check if they are either completely inside, completely outside, 
 *	or intersecting the viewing frustrum.
 *--------------------------------------------------------------------------
 */
int
Mapdesc::xformAndCullCheck( 
    REAL *pts, int uorder, int ustride, int vorder, int vstride )
{
    assert( uorder > 0 );
    assert( vorder > 0 );

    unsigned int inbits = mask;
    unsigned int outbits = 0;

    REAL *p = pts;
    for( REAL *pend = p + uorder * ustride; p != pend; p += ustride ) {
        REAL *q = p;
        for( REAL *qend = q + vorder * vstride; q != qend; q += vstride ) {
    	    REAL cpts[MAXCOORDS];
	    xformCulling( cpts, q );
	    unsigned int bits = clipbits( cpts );
	    outbits |= bits;
	    inbits &= bits;
	    if( ( outbits == (unsigned int)mask ) && ( inbits != (unsigned int)mask ) ) return CULL_ACCEPT;
	} 
    }

    if( outbits != (unsigned int)mask ) {
	return CULL_TRIVIAL_REJECT;
    } else if( inbits == (unsigned int)mask ) {
	return CULL_TRIVIAL_ACCEPT;
    } else {
	return CULL_ACCEPT;
    }
}

/*--------------------------------------------------------------------------
 * cullCheck - check if a set of homogeneous transformed points are 
 *	either completely inside, completely outside, 
 *	or intersecting the viewing frustrum.
 *--------------------------------------------------------------------------
 */
int
Mapdesc::cullCheck( REAL *pts, int uorder, int ustride, int vorder, int vstride )
{
    unsigned int inbits = mask;
    unsigned int outbits  = 0;

    REAL *p = pts;
    for( REAL *pend = p + uorder * ustride; p != pend; p += ustride ) {
        REAL *q = p;
        for( REAL *qend = q + vorder * vstride; q != qend; q += vstride ) {
	    unsigned int bits = clipbits( q );
	    outbits |= bits;
	    inbits &= bits;
	    if( ( outbits == (unsigned int)mask ) && ( inbits != (unsigned int)mask ) ) return CULL_ACCEPT;
	} 
    }

    if( outbits != (unsigned int)mask ) {
	return CULL_TRIVIAL_REJECT;
    } else if( inbits == (unsigned int)mask ) {
	return CULL_TRIVIAL_ACCEPT;
    } else {
	return CULL_ACCEPT;
    }
}

/*--------------------------------------------------------------------------
 * cullCheck - check if a set of homogeneous transformed points are 
 *	either completely inside, completely outside, 
 *	or intersecting the viewing frustrum.
 *--------------------------------------------------------------------------
 */
int
Mapdesc::cullCheck( REAL *pts, int order, int stride )
{
    unsigned int inbits = mask;
    unsigned int outbits  = 0;

    REAL *p = pts;
    for( REAL *pend = p + order * stride; p != pend; p += stride ) {
	unsigned int bits = clipbits( p );
	outbits |= bits;
	inbits &= bits;
	if( ( outbits == (unsigned int)mask ) && ( inbits != (unsigned int)mask ) ) return CULL_ACCEPT;
    }

    if( outbits != (unsigned int)mask ) {
	return CULL_TRIVIAL_REJECT;
    } else if( inbits == (unsigned int)mask ) {
	return CULL_TRIVIAL_ACCEPT;
    } else {
	return CULL_ACCEPT;
    }
}

/*--------------------------------------------------------------------------
 * xformSampling - transform a set of points that may be EITHER 
 *	homogeneous or inhomogeneous depending on the map description 
 *	into sampling space 
 *--------------------------------------------------------------------------
 */
void
Mapdesc::xformSampling( REAL *pts, int order, int stride, REAL *sp, int outstride )
{
    xformMat( smat, pts, order, stride, sp, outstride );
}

void
Mapdesc::xformBounding( REAL *pts, int order, int stride, REAL *sp, int outstride )
{
    xformMat( bmat, pts, order, stride, sp, outstride );
}

/*--------------------------------------------------------------------------
 * xformCulling - transform a set of points that may be EITHER 
 *	homogeneous or inhomogeneous depending on the map description 
 *	into culling space 
 *--------------------------------------------------------------------------
 */
void
Mapdesc::xformCulling( REAL *pts, int order, int stride, REAL *cp, int outstride )
{
    xformMat( cmat, pts, order, stride, cp, outstride );
}

/*--------------------------------------------------------------------------
 * xformCulling - transform a set of points that may be EITHER 
 *	homogeneous or inhomogeneous depending on the map description 
 *	into culling space 
 *--------------------------------------------------------------------------
 */
void
Mapdesc::xformCulling( REAL *pts, 
    int uorder, int ustride,
    int vorder, int vstride, 
    REAL *cp, int outustride, int outvstride )
{
    xformMat( cmat, pts, uorder, ustride, vorder, vstride, cp, outustride, outvstride );
}

/*--------------------------------------------------------------------------
 * xformSampling - transform a set of points that may be EITHER 
 *	homogeneous or inhomogeneous depending on the map description 
 *	into sampling space 
 *--------------------------------------------------------------------------
 */
void
Mapdesc::xformSampling( REAL *pts, 
    int uorder, int ustride, 
    int vorder, int vstride, 
    REAL *sp, int outustride, int outvstride )
{
    xformMat( smat, pts, uorder, ustride, vorder, vstride, sp, outustride, outvstride );
}

void
Mapdesc::xformBounding( REAL *pts, 
    int uorder, int ustride, 
    int vorder, int vstride, 
    REAL *sp, int outustride, int outvstride )
{
    xformMat( bmat, pts, uorder, ustride, vorder, vstride, sp, outustride, outvstride );
}

void
Mapdesc::xformMat( 
    Maxmatrix	mat, 
    REAL *	pts, 
    int 	order, 
    int 	stride,
    REAL *	cp, 
    int 	outstride )
{
    if( isrational ) {
	REAL *pend = pts + order * stride;
	for( REAL *p = pts ; p != pend; p += stride ) {
	    xformRational( mat, cp, p );
	    cp += outstride;
	}       
    } else {
	REAL *pend = pts + order * stride;
	for( REAL *p = pts ; p != pend; p += stride ) {
	    xformNonrational( mat, cp, p );
	    cp += outstride;
	}	
    }
}

void
Mapdesc::xformMat( Maxmatrix mat, REAL *pts, 
    int uorder, int ustride, 
    int vorder, int vstride, 
    REAL *cp, int outustride, int outvstride )
{
    if( isrational ) {
	REAL *pend = pts + uorder * ustride;
	for( REAL *p = pts ; p != pend; p += ustride ) {
	    REAL *cpts2 = cp;
	    REAL *qend = p + vorder * vstride;
	    for( REAL *q = p; q != qend; q += vstride ) {
		xformRational( mat, cpts2, q );
		cpts2 += outvstride;
	    } 
	    cp += outustride;
	}
    } else {
	REAL *pend = pts + uorder * ustride;
	for( REAL *p = pts ; p != pend; p += ustride ) {
	    REAL *cpts2 = cp;
	    REAL *qend = p + vorder * vstride;
	    for( REAL *q = p; q != qend; q += vstride ) {
		xformNonrational( mat, cpts2, q );
		cpts2 += outvstride;
	    } 
	    cp += outustride;
	}
    }
}

/*--------------------------------------------------------------------------
 * subdivide - subdivide a curve along an isoparametric line
 *--------------------------------------------------------------------------
 */

void
Mapdesc::subdivide( REAL *src, REAL *dst, REAL v, int stride, int order )
{
    REAL mv = 1.0 - v;

    for( REAL *send=src+stride*order; src!=send; send-=stride, dst+=stride ) {
	copyPt( dst, src );
	REAL *qpnt = src + stride;
	for( REAL *qp=src; qpnt!=send; qp=qpnt, qpnt+=stride )
	    sumPt( qp, qp, qpnt, mv, v );
    }
}

/*--------------------------------------------------------------------------
 * subdivide - subdivide a patch along an isoparametric line
 *--------------------------------------------------------------------------
 */

void
Mapdesc::subdivide( REAL *src, REAL *dst, REAL v, 
    int so, int ss, int to, int ts  )
{
    REAL mv = 1.0 - v;

    for( REAL *slast = src+ss*so; src != slast; src += ss, dst += ss ) {
	REAL *sp = src;
	REAL *dp = dst;
        for( REAL *send = src+ts*to; sp != send; send -= ts, dp += ts ) {
	    copyPt( dp, sp );
	    REAL *qp = sp;
	    for( REAL *qpnt = sp+ts; qpnt != send; qp = qpnt, qpnt += ts )
	        sumPt( qp, qp, qpnt, mv, v );
	}
    }
}


#define sign(x)	((x > 0) ? 1 : ((x < 0.0) ? -1 : 0))

/*--------------------------------------------------------------------------
 * project - project a set of homogeneous coordinates into inhomogeneous ones
 *--------------------------------------------------------------------------
 */
int
Mapdesc::project( REAL *src, int rstride, int cstride, 
	          REAL *dest, int trstride, int tcstride,
		  int nrows, int ncols )
{
    int s = sign( src[inhcoords] );
    REAL *rlast = src + nrows * rstride;
    REAL *trptr = dest;
    for( REAL *rptr=src; rptr != rlast; rptr+=rstride, trptr+=trstride ) {
	REAL *clast = rptr + ncols * cstride;
	REAL *tcptr = trptr;
	for( REAL *cptr = rptr; cptr != clast; cptr+=cstride, tcptr+=tcstride ) {
	    REAL *coordlast = cptr + inhcoords;
	    if( sign( *coordlast ) != s ) return 0;
	    REAL *tcoord = tcptr;
	    for( REAL *coord = cptr; coord != coordlast; coord++, tcoord++ ) {
		*tcoord = *coord / *coordlast;
	    }
	}
    }
    return 1;
}

/*--------------------------------------------------------------------------
 * project - project a set of homogeneous coordinates into inhomogeneous ones
 *--------------------------------------------------------------------------
 */
int
Mapdesc::project( REAL *src, int stride, REAL *dest, int tstride, int ncols )
{
    int s = sign( src[inhcoords] );

    REAL *clast = src + ncols * stride;
    for( REAL *cptr = src, *tcptr = dest; cptr != clast; cptr+=stride, tcptr+=tstride ) {
	REAL *coordlast = cptr + inhcoords;
	if( sign( *coordlast ) != s ) return 0;
	for( REAL *coord = cptr, *tcoord = tcptr; coord != coordlast; coord++, tcoord++ ) 
	    *tcoord = *coord / *coordlast;
    }

    return 1;
}

int
Mapdesc::bboxTooBig( 
    REAL *p, 
    int	 rstride,
    int	 cstride,
    int	 nrows,
    int	 ncols,
    REAL bb[2][MAXCOORDS] )
{
    REAL bbpts[MAXORDER][MAXORDER][MAXCOORDS];
    const int trstride = sizeof(bbpts[0]) / sizeof(REAL);
    const int tcstride = sizeof(bbpts[0][0]) / sizeof(REAL); 

    // points have been transformed, therefore they are homogeneous
    // project points
    int val = project( p, rstride, cstride, 
	     &bbpts[0][0][0], trstride, tcstride, nrows, ncols );
    if( val == 0 ) return -1;

    // compute bounding box
    bbox( bb, &bbpts[0][0][0], trstride, tcstride, nrows, ncols );

    // find out if bounding box can't fit in unit cube
    if( bbox_subdividing == N_BBOXROUND ) {
	for( int k=0; k != inhcoords; k++ )
	    if( ceilf(bb[1][k]) - floorf(bb[0][k]) > bboxsize[k] ) return 1;
    } else {
	for( int k=0; k != inhcoords; k++ )
	    if( bb[1][k] - bb[0][k] > bboxsize[k] ) return 1;
    }
    return 0;  
}

void
Mapdesc::bbox( 
    REAL bb[2][MAXCOORDS], 
    REAL *p, 
    int	 rstride,
    int	 cstride,
    int	 nrows,
    int	 ncols )
{
    int k;
    for( k=0; k != inhcoords; k++ )
	 bb[0][k] = bb[1][k] = p[k];

    for( int i=0; i != nrows; i++ ) 
	for( int j=0; j != ncols; j++ ) 
	    for( k=0; k != inhcoords; k++ ) {
		REAL x = p[i*rstride + j*cstride + k];
		if(  x < bb[0][k] ) bb[0][k] = x;
		else if( x > bb[1][k] ) bb[1][k] = x;
	    }
}

/*--------------------------------------------------------------------------
 * calcVelocityRational - calculate upper bound on first partial derivative 
 *	of a homogeneous set of points and bounds on each row of points.
 *--------------------------------------------------------------------------
 */
REAL
Mapdesc::calcVelocityRational( REAL *p, int stride, int ncols )
{
    REAL tmp[MAXORDER][MAXCOORDS];

    assert( ncols <= MAXORDER );

    const int tstride = sizeof(tmp[0]) / sizeof(REAL); 

    if( project( p, stride, &tmp[0][0], tstride, ncols ) ) {
	return calcPartialVelocity( &tmp[0][0], tstride, ncols, 1, 1.0 );
    } else { /* XXX */
	return calcPartialVelocity( &tmp[0][0], tstride, ncols, 1, 1.0 );
    }
}

/*--------------------------------------------------------------------------
 * calcVelocityNonrational - calculate upper bound on  first partial 
 *	derivative of a inhomogeneous set of points.
 *--------------------------------------------------------------------------
 */
REAL
Mapdesc::calcVelocityNonrational( REAL *pts, int stride, int ncols )
{
    return calcPartialVelocity( pts, stride, ncols, 1, 1.0 );
}

int
Mapdesc::isProperty( long property )
{
    switch ( property ) {
	case N_PIXEL_TOLERANCE:
	case N_ERROR_TOLERANCE:
	case N_CULLING:
	case N_BBOX_SUBDIVIDING:
	case N_S_STEPS:
	case N_T_STEPS:
        case N_SAMPLINGMETHOD:
        case N_CLAMPFACTOR:
        case N_MINSAVINGS:
	    return 1;
	default:
	    return 0;
    }
}

REAL
Mapdesc::getProperty( long property )
{
    switch ( property ) {
	case N_PIXEL_TOLERANCE:
	    return pixel_tolerance;
	case N_ERROR_TOLERANCE:
	    return error_tolerance;
	case N_CULLING:
	    return culling_method;
	case N_BBOX_SUBDIVIDING:
	    return bbox_subdividing;
	case N_S_STEPS:
	    return s_steps;
	case N_T_STEPS:
	    return t_steps;
        case N_SAMPLINGMETHOD:
	    return sampling_method;
        case N_CLAMPFACTOR:
	    return clampfactor;
        case N_MINSAVINGS:
	    return minsavings;
	default:
	    abort();
	    return -1; //not necessary, needed to shut up compiler
    }
}

void
Mapdesc::setProperty( long property, REAL value )
{

    switch ( property ) {
	case N_PIXEL_TOLERANCE:
	    pixel_tolerance = value;
	    break;
	case N_ERROR_TOLERANCE:
	    error_tolerance = value;
	    break;
	case N_CULLING:
	    culling_method = value;
	    break;
	case N_BBOX_SUBDIVIDING:
	    if( value <= 0.0 ) value = N_NOBBOXSUBDIVISION;
	    bbox_subdividing = value;
	    break;
	case N_S_STEPS:
	    if( value < 0.0 ) value = 0.0;
	    s_steps = value;
	    maxrate = ( value < 0.0 ) ? 0.0 : value;
	    maxsrate = ( value < 0.0 ) ? 0.0 : value;
	    break;
	case N_T_STEPS:
	    if( value < 0.0 ) value = 0.0;
	    t_steps = value;
	    maxtrate = ( value < 0.0 ) ? 0.0 : value;
	    break;
	case N_SAMPLINGMETHOD:
	    sampling_method = value;
	    break;
	case N_CLAMPFACTOR:
	    if( value <= 0.0 ) value = N_NOCLAMPING;
	    clampfactor = value;
	    break;
	case N_MINSAVINGS:
	    if( value <= 0.0 ) value = N_NOSAVINGSSUBDIVISION;
	    minsavings = value;
	    break;
	default:
	    abort();
	    break;
    }
}


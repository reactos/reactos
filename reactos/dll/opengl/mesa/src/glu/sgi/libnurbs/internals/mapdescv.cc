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
 * mapdescv.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "mystring.h"
#include "mymath.h"
#include "nurbsconsts.h"
#include "mapdesc.h"

/*--------------------------------------------------------------------------
 * calcPartialVelocity - calculate maximum magnitude of a given partial
 * derivative
 *--------------------------------------------------------------------------
 */
REAL
Mapdesc::calcPartialVelocity (
    REAL *p,
    int	 stride,
    int	 ncols,
    int  partial,
    REAL range )
{
    REAL tmp[MAXORDER][MAXCOORDS];
    REAL mag[MAXORDER];

    assert( ncols <= MAXORDER );

    int j, k, t;
    // copy inhomogeneous control points into temporary array
    for( j=0; j != ncols; j++ ) 
	for( k=0; k != inhcoords; k++ )
	    tmp[j][k] = p[j*stride + k];

    for( t=0; t != partial; t++ ) 
	for( j=0; j != ncols-t-1; j++ ) 
	    for( k=0; k != inhcoords; k++ ) 
		tmp[j][k] = tmp[j+1][k] - tmp[j][k];

    // compute magnitude and store in mag array
    for( j=0; j != ncols-partial; j++ ) {
	mag[j] = 0.0;
	for( k=0; k != inhcoords; k++ )
	    mag[j] += tmp[j][k] * tmp[j][k];
    }

    // compute scale factor
    REAL fac = 1;
    REAL invt = 1.0 / range;
    for( t = ncols-1; t != ncols-1-partial; t-- ) 
	fac *= t * invt;

    // compute max magnitude of all entries in array
    REAL max = 0.0;
    for( j=0; j != ncols-partial; j++ )
	if( mag[j] > max ) max = mag[j];
    max = fac * sqrtf( (float) max );

    return max;
}

/*--------------------------------------------------------------------------
 * calcPartialVelocity - calculate maximum magnitude of a given partial
 * derivative
 *--------------------------------------------------------------------------
 */
REAL
Mapdesc::calcPartialVelocity (
    REAL *dist,
    REAL *p,
    int	 rstride,
    int	 cstride,
    int	 nrows,
    int	 ncols,
    int  spartial,
    int  tpartial,
    REAL srange, 
    REAL trange,
    int  side )
{
    REAL tmp[MAXORDER][MAXORDER][MAXCOORDS];
    REAL mag[MAXORDER][MAXORDER];

    assert( nrows <= MAXORDER );
    assert( ncols <= MAXORDER );

    REAL *tp = &tmp[0][0][0];
    REAL *mp = &mag[0][0];
    const int istride = sizeof( tmp[0]) / sizeof( tmp[0][0][0] );
    const int jstride = sizeof( tmp[0][0]) / sizeof( tmp[0][0][0] );
    /*
    const int kstride = sizeof( tmp[0][0][0]) / sizeof( tmp[0][0][0] );
    */
    const int mistride = sizeof( mag[0]) / sizeof( mag[0][0] );
    const int mjstride = sizeof( mag[0][0]) / sizeof( mag[0][0] );
    const int idist = nrows * istride;
    const int jdist = ncols * jstride;
    /*
    const int kdist = inhcoords * kstride;
    */
    const int id = idist - spartial * istride;
    const int jd = jdist - tpartial * jstride;

    {
	// copy control points
	REAL *ti = tp;
	REAL *qi = p;
	REAL *til = tp + idist;
	for( ; ti != til; ) {
	    REAL *tj = ti;
	    REAL *qj = qi;
	    REAL *tjl = ti + jdist;
	    for( ; tj != tjl;  ) {
		for( int k=0; k != inhcoords; k++ ) {
		    tj[k] = qj[k];
		}
		tj += jstride;
		qj += cstride;
	    }
	    ti += istride;
	    qi += rstride; 
	}
    }

    {
        // compute (s)-partial derivative control points
	REAL *til = tp + idist - istride;
	const REAL *till = til - ( spartial * istride );
	for( ; til != till; til -= istride )
	    for( REAL *ti = tp; ti != til; ti += istride )
		for( REAL *tj = ti, *tjl = tj + jdist; tj != tjl; tj += jstride )
		    for( int k=0; k != inhcoords; k++ )
			tj[k] = tj[k+istride] - tj[k];
    }

    {
        // compute (s,t)-partial derivative control points
	REAL *tjl = tp + jdist - jstride;
	const REAL *tjll = tjl - ( tpartial * jstride );
	for( ; tjl != tjll; tjl -= jstride )
	    for( REAL *tj = tp; tj != tjl; tj += jstride )
		for( REAL *ti = tj, *til = ti + id; ti != til; ti += istride )
		    for( int k=0; k != inhcoords; k++ ) 
			ti[k] = ti[k+jstride] - ti[k];

    }

    REAL max = 0.0;
    {
	// compute magnitude and store in mag array
	memset( (void *) mp, 0, sizeof( mag ) );
	for( REAL *ti = tp, *mi = mp, *til = tp + id; ti != til; ti += istride, mi += mistride )
	    for( REAL *tj = ti, *mj = mi, *tjl = ti + jd; tj != tjl; tj += jstride, mj += mjstride ) {
		for( int k=0; k != inhcoords; k++ )
		   *mj += tj[k] * tj[k];
		if( *mj > max ) max = *mj;
	    }

    }

    int i, j;

    // compute scale factor
    REAL fac = 1.0;
    {
	REAL invs = 1.0 / srange;
	REAL invt = 1.0 / trange;
	for( int s = nrows-1, slast = s-spartial; s != slast; s-- ) 
	    fac *= s * invs;
	for( int t = ncols-1, tlast = t-tpartial; t != tlast; t-- ) 
	    fac *= t * invt;
    }

    if( side == 0 ) {
	// compute max magnitude of first and last column
	dist[0] = 0.0;
	dist[1] = 0.0;
	for( i=0; i != nrows-spartial; i++ ) {
	    j = 0;
	    if( mag[i][j] > dist[0] ) dist[0] = mag[i][j];
    
	    j = ncols-tpartial-1;
	    if( mag[i][j] > dist[1] ) dist[1] = mag[i][j];
	}
	dist[0] = fac * sqrtf( dist[0] );
	dist[1] = fac * sqrtf( dist[1] );
    } else if( side == 1 ) {
	// compute max magnitude of first and last row
	dist[0] = 0.0;
	dist[1] = 0.0;
	for( j=0; j != ncols-tpartial; j++ ) {
	    i = 0;
	    if( mag[i][j] > dist[0] ) dist[0] = mag[i][j];
    
	    i = nrows-spartial-1;
	    if( mag[i][j] > dist[1] ) dist[1] = mag[i][j];
	}
	dist[0] = fac * sqrtf( dist[0] );
	dist[1] = fac * sqrtf( dist[1] );
    }

    max = fac * sqrtf( (float) max );

    return max;
}


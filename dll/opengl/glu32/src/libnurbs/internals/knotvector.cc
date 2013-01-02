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
 * knotvector.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "knotvector.h"
#include "defines.h"

#ifdef __WATCOMC__
#pragma warning 726 10
#endif

void Knotvector::init( long _knotcount, long _stride, long _order, INREAL *_knotlist )
{
    knotcount = _knotcount;
    stride = _stride;
    order = _order;
    knotlist = new Knot[_knotcount];
    assert( knotlist != 0 );

    for( int i = 0; i != _knotcount; i++ )
	knotlist[i] = (Knot) _knotlist[i];
}

Knotvector::Knotvector( void )
{
    knotcount = 0;
    stride = 0;
    order = 0;
    knotlist = 0;
}

Knotvector::~Knotvector( void )
{
    if( knotlist ) delete[] knotlist;
}

int Knotvector::validate( void )
{
   /* kindex is used as an array index so subtract one first,
     * this propagates throughout the code so study carefully */
    long	kindex = knotcount-1;

    if( order < 1 || order > MAXORDER ) {
	// spline order un-supported
	return( 1 );
    }

    if( knotcount < (2 * order) ) {
	// too few knots
	return( 2 );
    }

    if( identical( knotlist[kindex-(order-1)], knotlist[order-1]) ) {
	// valid knot range is empty
	return( 3 );
    }

    for( long i = 0; i < kindex; i++)
	if( knotlist[i] > knotlist[i+1] ) {
	    // decreasing knot sequence
	    return( 4 );
	}
        
    /* check for valid multiplicity */

    /*	kindex is currently the index of the last knot.
     *	In the next loop  it is decremented to ignore the last knot
     *	and the loop stops when kindex	is 2 so as to ignore the first
     *	knot as well.  These knots are not used in computing
     *	knot multiplicities.
     */

    long multi = 1;
    for( ; kindex >= 1; kindex-- ) {
	if( knotlist[kindex] - knotlist[kindex-1] < TOLERANCE ) {
	    multi++;
	    continue;
	}
	if ( multi > order ) {
	    // knot multiplicity greater than order of spline
	    return( 5 );
	}
	multi = 1;
    }

    if ( multi > order ) {
	// knot multiplicity greater than order of spline
	return( 5 );
    }

    return 0;
}

void Knotvector::show( const char *msg )
{
#ifndef NDEBUG
    _glu_dprintf( "%s\n", msg );
    _glu_dprintf( "order = %ld, count = %ld\n", order, knotcount );

    for( int i=0; i<knotcount; i++ )
	_glu_dprintf( "knot[%d] = %g\n", i, knotlist[i] );
#endif
}


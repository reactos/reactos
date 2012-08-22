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
 * flist.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "flist.h"

/*----------------------------------------------------------------------------
 * Flist::Flist - initialize a REAL number array
 *----------------------------------------------------------------------------
 */
Flist::Flist( void )
{
    npts = 0;
    pts = 0;
    start = end = 0;
}

/*----------------------------------------------------------------------------
 * Flist::~Flist - free a REAL number array
 *----------------------------------------------------------------------------
 */
Flist::~Flist( void )
{
    if( npts ) delete[] pts;
}

void
Flist::add( REAL x )
{
    pts[end++] = x;
    assert( end <= npts );
}

/*----------------------------------------------------------------------------
 * Flist::filter - remove duplicate numbers from array
 *----------------------------------------------------------------------------
 */
void Flist::filter( void )
{
    sorter.qsort( pts, end );
    start = 0;

    int j = 0;
    for( int i = 1; i < end; i++ ) { 
	if( pts[i] == pts[i-j-1] )
	    j++;
	pts[i-j] = pts[i];
    }
    end -= j;
}

/*----------------------------------------------------------------------------
 * Flist::grow - ensure that array is large enough
 *----------------------------------------------------------------------------
 */
void Flist::grow( int maxpts )
{
    if( npts < maxpts ) {
	if( npts ) delete[] pts;
	npts = 2 * maxpts; 
	pts = new REAL[npts];
	assert( pts != 0 );
    }
    start = end = 0;
}

/*----------------------------------------------------------------------------
 * Flist::taper - ignore head and tail of array
 *----------------------------------------------------------------------------
 */
void Flist::taper( REAL from, REAL to )
{
    while( pts[start] != from )
	start++;

    while( pts[end-1] != to )
	end--;
}



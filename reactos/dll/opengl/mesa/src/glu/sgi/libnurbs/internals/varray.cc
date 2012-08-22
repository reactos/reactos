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
 * varray.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "varray.h"
#include "arc.h"
#include "simplemath.h"         // glu_abs()

#define TINY 0.0001
inline long sgn( REAL x ) 
{
    return (x < -TINY) ? -1 :  ((x > TINY) ? 1 : 0 );
}


Varray::Varray( void )
{
    int i;

    varray = 0;
    size = 0;
    numquads = 0;

    for (i = 0; i < 1000; i++) {
        vval[i] = 0;
        voffset[i] = 0;
    }
}

Varray::~Varray( void )
{
    if( varray ) delete[] varray; 
}

inline void
Varray::update( Arc_ptr arc, long dir[2], REAL val )
{
    register long ds = sgn(arc->tail()[0] - arc->prev->tail()[0]);
    register long dt = sgn(arc->tail()[1] - arc->prev->tail()[1]);

    if( dir[0] != ds || dir[1] != dt ) {
	dir[0] = ds;
	dir[1] = dt;
	append( val );
    }
}

void
Varray::grow( long guess )
{
    if( size < guess ) {
	size = guess * 2;
	if( varray ) delete[] varray; 
	varray = new REAL[size];
	assert( varray != 0 );
    }
}

long
Varray::init( REAL delta, Arc_ptr toparc, Arc_ptr botarc )
{
    Arc_ptr left = toparc->next;
    Arc_ptr right = toparc;
    long ldir[2], rdir[2];
    
    ldir[0] = sgn( left->tail()[0] - left->prev->tail()[0] );
    ldir[1] = sgn( left->tail()[1] - left->prev->tail()[1] );
    rdir[0] = sgn( right->tail()[0] - right->prev->tail()[0] );
    rdir[1] = sgn( right->tail()[1] - right->prev->tail()[1] );

    vval[0] = toparc->tail()[1];
    numquads = 0;

    while( 1 ) {
	switch( sgn( left->tail()[1] - right->prev->tail()[1] ) ) {
	case 1:
	    left = left->next;
	    update( left, ldir, left->prev->tail()[1] );
	    break;
	case -1: 
	    right = right->prev;
	    update( right, rdir, right->tail()[1] );
	    break;
	case 0:
	    if( glu_abs(left->tail()[1] - botarc->tail()[1]) < TINY) goto end;
            if( glu_abs(left->tail()[0]-right->prev->tail()[0]) < TINY &&
                glu_abs(left->tail()[1]-right->prev->tail()[1]) < TINY) goto end;
	    left = left->next;
	    break;
 	}
    }

end:
    append( botarc->tail()[1] );

    grow( ((long) ((vval[0] - vval[numquads])/delta)) + numquads + 2 );

    long i, index = 0;
    for( i=0; i<numquads; i++ ) {
	voffset[i] = index;
        varray[index++] = vval[i];
	REAL dist = vval[i] - vval[i+1];
	if( dist > delta ) {
	    long steps = ((long) (dist/delta)) +1;
	    float deltav = - dist / (REAL) steps;
	    for( long j=1; j<steps; j++ ) 
		varray[index++] = vval[i] + j * deltav; 
	}
    }
    voffset[i] = index;
    varray[index] = vval[i];
    return index;
}


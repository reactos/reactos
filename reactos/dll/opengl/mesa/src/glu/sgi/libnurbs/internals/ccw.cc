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
 * ccw.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "subdivider.h"
#include "types.h"
#include "arc.h"
#include "trimvertex.h"
#include "simplemath.h"

inline int 
Subdivider::bbox( TrimVertex *a, TrimVertex *b, TrimVertex *c, int p )
{
    return bbox( a->param[p], b->param[p], c->param[p], 
	         a->param[1-p], b->param[1-p], c->param[1-p] ); 
}

int
Subdivider::ccwTurn_sr( Arc_ptr j1, Arc_ptr j2 ) // dir = 1
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    _glu_dprintf( "arc_ccw_turn, p = %d\n", 0 );
#endif

    // the arcs lie on the line (0 == v1->param[0])
    if( v1->param[0] == v1next->param[0] && v2->param[0] == v2next->param[0] )
	return 0;

    if( v2next->param[0] < v2->param[0] || v1next->param[0] < v1->param[0] )
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[1] < v2->param[1] )
	return 0;
    else if( v1->param[1] > v2->param[1] )
	return 1;

    while( 1 ) {
	if( v1next->param[0] < v2next->param[0] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case a\n" );
#endif
	    assert( v1->param[0] <= v1next->param[0] );
	    assert( v2->param[0] <= v1next->param[0] );
	    switch( bbox( v2, v2next, v1next, 1 ) ) {
		case -1:
		    return 0;
		case 0:
		   sgn = ccw( v1next, v2, v2next );
		   if( sgn != -1 ) {
			return sgn;
		   } else {
#ifdef DEBUG
			_glu_dprintf( "decr\n" );
#endif
			v1 = v1next--;
			if( v1 == v1last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else if( v1next->param[0] > v2next->param[0] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case b\n" );
#endif
	    assert( v1->param[0] <= v2next->param[0] );
	    assert( v2->param[0] <= v2next->param[0] );
	    switch( bbox( v1, v1next, v2next, 1 ) ) {
		case -1:
		    return 1;
		case 0:
		   sgn = ccw( v1next, v1, v2next );
		   if( sgn != -1 ) { 
			return sgn;
		   } else {
#ifdef DEBUG
			_glu_dprintf( "incr\n" );
#endif
			v2 = v2next++;
			if( v2 == v2last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else {
#ifndef NDEBUG
	    _glu_dprintf( "case ab\n" );
#endif
	    if( v1next->param[1] < v2next->param[1] )
		return 0;
	    else if( v1next->param[1] > v2next->param[1] )
		return 1;
	    else {
#ifdef DEBUG
		_glu_dprintf( "incr\n" );
#endif
		v2 = v2next++;
		if( v2 == v2last ) {
#ifdef DEBUG
		    _glu_dprintf( "no good results\n" );
#endif
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}

int
Subdivider::ccwTurn_sl( Arc_ptr j1, Arc_ptr j2 ) // dir = 0
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    _glu_dprintf( "arc_ccw_turn, p = %d\n", 0 );
#endif

    // the arcs lie on the line (0 == v1->param[0])
    if( v1->param[0] == v1next->param[0] && v2->param[0] == v2next->param[0] )
	return 0;

    if( v2next->param[0] > v2->param[0] || v1next->param[0] > v1->param[0] ) 
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[1] < v2->param[1] )
	return 1;
    else if( v1->param[1] > v2->param[1] )
	return 0;

    while( 1 ) {
	if( v1next->param[0] > v2next->param[0] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case c\n" );
#endif
	    assert( v1->param[0] >= v1next->param[0] );
	    assert( v2->param[0] >= v1next->param[0] );
	    switch( bbox( v2next, v2, v1next, 1 ) ) {
		case -1:
		    return 1;
		case 0:
		    sgn = ccw( v1next, v2, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v1 = v1next--;
#ifdef DEBUG
			_glu_dprintf( "decr\n" );
#endif
			if( v1 == v1last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else if( v1next->param[0] < v2next->param[0] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case d\n" );
#endif
	    assert( v1->param[0] >= v2next->param[0] );
	    assert( v2->param[0] >= v2next->param[0] );
	    switch( bbox( v1next, v1, v2next, 1 ) ) {
		case -1:
		    return 0;
		case 0:
		    sgn = ccw( v1next, v1, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v2 = v2next++;
#ifdef DEBUG
			_glu_dprintf( "incr\n" );
#endif
			if( v2 == v2last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else {
#ifdef DEBUG
	    _glu_dprintf( "case cd\n" );
#endif
	    if( v1next->param[1] < v2next->param[1] )
		return 1;
	    else if( v1next->param[1] > v2next->param[1] )
		return 0;
	    else {
		v2 = v2next++;
#ifdef DEBUG
		_glu_dprintf( "incr\n" );
#endif
		if( v2 == v2last ) {
#ifdef DEBUG
		    _glu_dprintf( "no good results\n" );
#endif
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}

int
Subdivider::ccwTurn_tr( Arc_ptr j1, Arc_ptr j2 ) // dir = 1
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    _glu_dprintf( "arc_ccw_turn, p = %d\n", 1 );
#endif

    // the arcs lie on the line (1 == v1->param[1])
    if( v1->param[1] == v1next->param[1] && v2->param[1] == v2next->param[1] )
	return 0;

    if( v2next->param[1] < v2->param[1] || v1next->param[1] < v1->param[1] )
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[0] < v2->param[0] )
	return 1;
    else if( v1->param[0] > v2->param[0] )
	return 0;

    while( 1 ) {
	if( v1next->param[1] < v2next->param[1] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case a\n" );
#endif
	    assert( v1->param[1] <= v1next->param[1] );
	    assert( v2->param[1] <= v1next->param[1] );
	    switch( bbox( v2, v2next, v1next, 0 ) ) {
		case -1:
		    return 1;
		case 0:
		   sgn = ccw( v1next, v2, v2next );
		   if( sgn != -1 ) {
			return sgn;
		   } else {
#ifdef DEBUG
			_glu_dprintf( "decr\n" );
#endif
			v1 = v1next--;
			if( v1 == v1last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else if( v1next->param[1] > v2next->param[1] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case b\n" );
#endif
	    assert( v1->param[1] <= v2next->param[1] );
	    assert( v2->param[1] <= v2next->param[1] );
	    switch( bbox( v1, v1next, v2next, 0 ) ) {
		case -1:
		    return 0;
		case 0:
		   sgn = ccw( v1next, v1, v2next );
		   if( sgn != -1 ) { 
			return sgn;
		   } else {
#ifdef DEBUG
			_glu_dprintf( "incr\n" );
#endif
			v2 = v2next++;
			if( v2 == v2last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else {
#ifdef DEBUG
	    _glu_dprintf( "case ab\n" );
#endif
	    if( v1next->param[0] < v2next->param[0] )
		return 1;
	    else if( v1next->param[0] > v2next->param[0] )
		return 0;
	    else {
#ifdef DEBUG
		_glu_dprintf( "incr\n" );
#endif
		v2 = v2next++;
		if( v2 == v2last ) {
#ifdef DEBUG
		    _glu_dprintf( "no good results\n" );
#endif
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}

int
Subdivider::ccwTurn_tl( Arc_ptr j1, Arc_ptr j2 )
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    _glu_dprintf( "arc_ccw_turn, p = %d\n", 1 );
#endif

    // the arcs lie on the line (1 == v1->param[1])
    if( v1->param[1] == v1next->param[1] && v2->param[1] == v2next->param[1] )
	return 0;

    if( v2next->param[1] > v2->param[1] || v1next->param[1] > v1->param[1] ) 
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[0] < v2->param[0] )
	return 0;
    else if( v1->param[0] > v2->param[0] )
	return 1;

    while( 1 ) {
	if( v1next->param[1] > v2next->param[1] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case c\n" );
#endif
	    assert( v1->param[1] >= v1next->param[1] );
	    assert( v2->param[1] >= v1next->param[1] );
	    switch( bbox( v2next, v2, v1next, 0 ) ) {
		case -1:
		    return 0;
		case 0:
		    sgn = ccw( v1next, v2, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v1 = v1next--;
#ifdef DEBUG
			_glu_dprintf( "decr\n" );
#endif
			if( v1 == v1last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else if( v1next->param[1] < v2next->param[1] ) {
#ifndef NDEBUG
	    _glu_dprintf( "case d\n" );
	    assert( v1->param[1] >= v2next->param[1] );
	    assert( v2->param[1] >= v2next->param[1] );
#endif
	    switch( bbox( v1next, v1, v2next, 0 ) ) {
		case -1:
		    return 1;
		case 0:
		    sgn = ccw( v1next, v1, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v2 = v2next++;
#ifdef DEBUG
			_glu_dprintf( "incr\n" );
#endif
			if( v2 == v2last ) {
#ifdef DEBUG
			    _glu_dprintf( "no good results\n" );
#endif
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else {
#ifdef DEBUG
	    _glu_dprintf( "case cd\n" );
#endif
	    if( v1next->param[0] < v2next->param[0] )
		return 0;
	    else if( v1next->param[0] > v2next->param[0] )
		return 1;
	    else {
		v2 = v2next++;
#ifdef DEBUG
		_glu_dprintf( "incr\n" );
#endif
		if( v2 == v2last ) {
#ifdef DEBUG
		    _glu_dprintf( "no good results\n" );
#endif
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}


#ifndef NDEBUG
int
Subdivider::bbox( register REAL sa, register REAL sb, register REAL sc,
      register REAL ta, register REAL tb, register REAL tc )
#else
int
Subdivider::bbox( register REAL sa, register REAL sb, register REAL sc,
      register REAL   , register REAL   , register REAL    )
#endif
{
#ifndef NDEBUG
    assert( tc >= ta );
    assert( tc <= tb );
#endif

    if( sa < sb ) {
	if( sc <= sa ) {
	    return -1;
	} else if( sb <= sc ) {
	    return 1;
	} else {
	    return 0;
	}
    } else if( sa > sb ) {
	if( sc >= sa ) {
	    return 1;
	} else if( sb >= sc ) {
	    return -1;
	} else {
	    return 0;
	}
    } else {
	if( sc > sa ) {
	    return 1;
	} else if( sb > sc ) {
	    return -1;
	} else {
	    return 0;
	}
    }
}

/*----------------------------------------------------------------------------
 * ccw - determine how three points are oriented by computing their
 *	 determinant.  
 *	 Return 1 if the vertices are ccw oriented, 
 *		0 if they are cw oriented, or 
 *		-1 if the computation is ill-conditioned.
 *----------------------------------------------------------------------------
 */
int
Subdivider::ccw( TrimVertex *a, TrimVertex *b, TrimVertex *c )
{
    REAL d = det3( a, b, c );
    if( glu_abs(d) < 0.0001 ) return -1;
    return (d < 0.0) ? 0 : 1;
}

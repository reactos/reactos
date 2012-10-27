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
 * splitarcs.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mysetjmp.h"
#include "mystdio.h"
#include "subdivider.h"
#include "arcsorter.h"
#include "arc.h"
#include "bin.h"

/* local preprocessor definitions */
#define MAXARCS	10

/*----------------------------------------------------------------------------
 * Subdivider::split - split trim regions in source bin by line (param == value). 
 *----------------------------------------------------------------------------
 */

void
Subdivider::split( Bin& bin, Bin& left, Bin& right, int param, REAL value )
{
    Bin	intersections, unknown; 

    partition( bin, left, intersections, right, unknown, param, value );

    int	count = intersections.numarcs();
    if( count % 2 ) {
#ifndef NDEBUG
	left.show( "left" );
	intersections.show( "intersections" );
	right.show( "right" );
#endif
	::mylongjmp( jumpbuffer, 29 );
    }

    Arc_ptr arclist[MAXARCS], *list;
    if( count >= MAXARCS ) {
	list = new Arc_ptr[count];
    } else {
	list = arclist;
    }

    Arc_ptr jarc, *last, *lptr;
    for( last = list; (jarc=intersections.removearc()) != NULL; last++ )
	*last = jarc;

    if( param == 0 ) { /* sort into increasing t order */
	ArcSdirSorter sorter(*this);
	sorter.qsort( list, count );
	
        //::qsort ((void *)list, count, sizeof(Arc_ptr), (cmpfunc)compare_s);
	for( lptr=list; lptr<last; lptr+=2 )
	    check_s ( lptr[0], lptr[1] );
	for( lptr=list; lptr<last; lptr+=2 )
	    join_s ( left, right, lptr[0], lptr[1] );
	for( lptr=list; lptr != last; lptr++ ) {
	    if( ((*lptr)->head()[0] <= value) && ((*lptr)->tail()[0] <= value) )
		left.addarc( *lptr  );
	    else
		right.addarc( *lptr  );
	}
    } else { /* sort into decreasing s order */
	ArcTdirSorter sorter(*this);
	sorter.qsort( list, count );
        //::qsort ((void *)list, count, sizeof(Arc_ptr), (cmpfunc)compare_t);
	for( lptr=list; lptr<last; lptr+=2 )
	    check_t ( lptr[0], lptr[1] );
	for( lptr=list; lptr<last; lptr+=2 )
	    join_t ( left, right, lptr[0], lptr[1] );
	for( lptr=list; lptr != last; lptr++ ) {
	    if( ((*lptr)->head()[1] <= value) && ((*lptr)->tail()[1] <= value) )
		left.addarc( *lptr  );
	    else
		right.addarc( *lptr  );
	}
    }

    if( list != arclist ) delete[] list;
    unknown.adopt(); 
}


void
Subdivider::check_s( Arc_ptr jarc1, Arc_ptr jarc2 )
{
    assert( jarc1->check( ) != 0 );
    assert( jarc2->check( ) != 0 );
    assert( jarc1->next->check( ) != 0 );
    assert( jarc2->next->check( ) != 0 );
    assert( jarc1 != jarc2 );

    /* XXX - if these assertions fail, it is due to user error or
	     undersampling */
    if( ! ( jarc1->tail()[0] < (jarc1)->head()[0] ) ) {
#ifndef NDEBUG
	_glu_dprintf( "s difference %f\n",  (jarc1)->tail()[0] - (jarc1)->head()[0] );
#endif
	::mylongjmp( jumpbuffer, 28 );
    }

    if( ! ( jarc2->tail()[0] > (jarc2)->head()[0] ) ) { 
#ifndef NDEBUG
	_glu_dprintf( "s difference %f\n",  (jarc2)->tail()[0] - (jarc2)->head()[0] );
#endif
	::mylongjmp( jumpbuffer, 28 );
    }
}

inline void
Subdivider::link( Arc_ptr jarc1, Arc_ptr jarc2, Arc_ptr up, Arc_ptr down )
{
    up->nuid = down->nuid = 0;		// XXX

    up->next = jarc2;
    down->next = jarc1;
    up->prev = jarc1->prev;
    down->prev = jarc2->prev;

    down->next->prev = down;
    up->next->prev = up;
    down->prev->next = down;
    up->prev->next = up;
}

inline void 
Subdivider::simple_link( Arc_ptr jarc1, Arc_ptr jarc2 )
{
    Arc_ptr tmp = jarc2->prev;
    jarc2->prev = jarc1->prev;
    jarc1->prev = tmp;
    jarc2->prev->next = jarc2;
    jarc1->prev->next = jarc1;
}


/*----------------------------------------------------------------------------
 * join - add a pair of oppositely directed jordan arcs between two arcs
 *----------------------------------------------------------------------------
 */

void
Subdivider::join_s( Bin& left, Bin& right, Arc_ptr jarc1, Arc_ptr jarc2 )
{
    assert( jarc1->check( ) != 0);
    assert( jarc2->check( ) != 0);
    assert( jarc1 != jarc2 );

    if( ! jarc1->getitail() )
	jarc1 = jarc1->next;

    if( ! jarc2->getitail() )
	jarc2 = jarc2->next;

    REAL s = jarc1->tail()[0];
    REAL t1 = jarc1->tail()[1];
    REAL t2 = jarc2->tail()[1];

    if( t1 == t2 ) {
	simple_link( jarc1, jarc2 );
    } else {
	Arc_ptr newright = new(arcpool) Arc( arc_right, 0 ); 
	Arc_ptr newleft = new(arcpool) Arc( arc_left, 0 );
	assert( t1 < t2 );
	if( isBezierArcType() ) {
	    arctessellator.bezier( newright, s, s, t1, t2 );
	    arctessellator.bezier( newleft, s, s, t2, t1 );
	} else {
	    arctessellator.pwl_right( newright, s, t1, t2, stepsizes[0] );
	    arctessellator.pwl_left( newleft, s, t2, t1, stepsizes[2] );
	}
	link( jarc1, jarc2, newright, newleft );
	left.addarc( newright  );
	right.addarc( newleft  );
    }

    assert( jarc1->check( ) != 0 );
    assert( jarc2->check( ) != 0 );
    assert( jarc1->next->check( ) != 0);
    assert( jarc2->next->check( ) != 0);
}

void
Subdivider::check_t( Arc_ptr jarc1, Arc_ptr jarc2 )
{
    assert( jarc1->check( ) != 0 );
    assert( jarc2->check( ) != 0 );
    assert( jarc1->next->check( ) != 0 );
    assert( jarc2->next->check( ) != 0 );
    assert( jarc1 != jarc2 );

    /* XXX - if these assertions fail, it is due to user error or
	     undersampling */
    if( ! ( jarc1->tail()[1] < (jarc1)->head()[1] ) ) {
#ifndef NDEBUG
	_glu_dprintf( "t difference %f\n",  jarc1->tail()[1] - (jarc1)->head()[1] );
#endif
	::mylongjmp( jumpbuffer, 28 );
    }

    if( ! ( jarc2->tail()[1] > (jarc2)->head()[1] ) ) { 
#ifndef NDEBUG
	_glu_dprintf( "t difference %f\n",  jarc2->tail()[1] - (jarc2)->head()[1] );
#endif
	::mylongjmp( jumpbuffer, 28 );
    }
}

/*----------------------------------------------------------------------------
 * join_t - add a pair of oppositely directed jordan arcs between two arcs
 *----------------------------------------------------------------------------
 */

void
Subdivider::join_t( Bin& bottom, Bin& top, Arc_ptr jarc1, Arc_ptr jarc2 )
{
    assert( jarc1->check( ) != 0 );
    assert( jarc2->check( ) != 0 );
    assert( jarc1->next->check( ) != 0 );
    assert( jarc2->next->check( ) != 0 );
    assert( jarc1 != jarc2 );

    if( ! jarc1->getitail() )
	jarc1 = jarc1->next;

    if( ! jarc2->getitail() )
	jarc2 = jarc2->next;

    REAL s1 = jarc1->tail()[0];
    REAL s2 = jarc2->tail()[0];
    REAL t  = jarc1->tail()[1];

    if( s1 == s2 ) {
	simple_link( jarc1, jarc2 );
    } else {
	Arc_ptr newtop = new(arcpool) Arc( arc_top, 0 );
	Arc_ptr newbot = new(arcpool) Arc( arc_bottom, 0 );
	assert( s1 > s2 );
	if( isBezierArcType() ) {
	    arctessellator.bezier( newtop, s1, s2, t, t );
	    arctessellator.bezier( newbot, s2, s1, t, t );
	} else {
	    arctessellator.pwl_top( newtop, t, s1, s2, stepsizes[1] );
	    arctessellator.pwl_bottom( newbot, t, s2, s1, stepsizes[3] );
	}
	link( jarc1, jarc2, newtop, newbot );
	bottom.addarc( newtop  );
	top.addarc( newbot  );
    }

    assert( jarc1->check( ) != 0 );
    assert( jarc2->check( ) != 0 );
    assert( jarc1->next->check( ) != 0 );
    assert( jarc2->next->check( ) != 0 );
}


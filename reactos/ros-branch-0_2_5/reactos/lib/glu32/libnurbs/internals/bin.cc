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
 * bin.c++
 *
 * $Date: 2004/02/02 16:39:10 $ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/bin.cc,v 1.1 2004/02/02 16:39:10 navaraf Exp $
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "bin.h"

/*----------------------------------------------------------------------------
 * Constructor and destructor
 *----------------------------------------------------------------------------
 */
Bin::Bin()
{
    head = NULL;
}

Bin::~Bin()
{
    assert( head == NULL);
}

/*----------------------------------------------------------------------------
 * remove_this_arc - remove given Arc_ptr from bin
 *----------------------------------------------------------------------------
 */

void 
Bin::remove_this_arc( Arc_ptr arc )
{
    Arc_ptr *j;
    for( j = &(head); (*j != 0) && (*j != arc); j = &((*j)->link) );

    if( *j != 0 ) {
        if( *j == current )
	    current = (*j)->link;
	*j = (*j)->link;
    }
}

/*----------------------------------------------------------------------------
 * numarcs - count number of arcs in bin
 *----------------------------------------------------------------------------
 */

int
Bin::numarcs()
{
    long count = 0;
    for( Arc_ptr jarc = firstarc(); jarc; jarc = nextarc() )
	count++;
    return count;
}

/*----------------------------------------------------------------------------
 * adopt - place an orphaned arcs into their new parents bin
 *----------------------------------------------------------------------------
 */

void 
Bin::adopt()
{
    markall();

    Arc_ptr orphan;
    while( (orphan = removearc()) != NULL ) {
	for( Arc_ptr parent = orphan->next; parent != orphan; parent = parent->next ) {
	    if (! parent->ismarked() ) {
		orphan->link = parent->link;
		parent->link = orphan;
		orphan->clearmark();
		break;
	    }
	}
    }
}


/*----------------------------------------------------------------------------
 * show - print out descriptions of the arcs in the bin
 *----------------------------------------------------------------------------
 */

void
Bin::show( char *name )
{
#ifndef NDEBUG
    dprintf( "%s\n", name );
    for( Arc_ptr jarc = firstarc(); jarc; jarc = nextarc() )
        jarc->show( );
#endif
}



/*----------------------------------------------------------------------------
 * markall - mark all arcs with an identifying tag
 *----------------------------------------------------------------------------
 */

void 
Bin::markall()
{
    for( Arc_ptr jarc=firstarc(); jarc; jarc=nextarc() )
	jarc->setmark();
}

/*----------------------------------------------------------------------------
 * listBezier - print out all arcs that are untessellated border arcs
 *----------------------------------------------------------------------------
 */

void 
Bin::listBezier( void )
{
    for( Arc_ptr jarc=firstarc(); jarc; jarc=nextarc() ) {
	if( jarc->isbezier( ) ) {
    	    assert( jarc->pwlArc->npts == 2 );	
	    TrimVertex  *pts = jarc->pwlArc->pts;
    	    REAL s1 = pts[0].param[0];
    	    REAL t1 = pts[0].param[1];
    	    REAL s2 = pts[1].param[0];
    	    REAL t2 = pts[1].param[1];
#ifndef NDEBUG
	   dprintf( "arc (%g,%g) (%g,%g)\n", s1, t1, s2, t2 );
#endif
	}
    }
}


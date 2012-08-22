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
 * arcsorter.c++
 *
 */

#ifndef __gluarcsorter_c_
#define __gluarcsorter_c_

#include "glimports.h"
#include "arc.h"
#include "arcsorter.h"
#include "subdivider.h"

ArcSorter::ArcSorter(Subdivider &s) : Sorter( sizeof( Arc ** ) ), subdivider(s)
{
}

int
ArcSorter::qscmp( char *, char * )
{
    _glu_dprintf( "ArcSorter::qscmp: pure virtual called\n" );
    return 0;
}

void
ArcSorter::qsort( Arc **a, int n )
{
    Sorter::qsort( (void *) a, n );
}

void		
ArcSorter::qsexc( char *i, char *j )// i<-j, j<-i 
{
    Arc **jarc1 = (Arc **) i;
    Arc **jarc2 = (Arc **) j;
    Arc *tmp = *jarc1;
    *jarc1 = *jarc2;
    *jarc2 = tmp;
}	

void		
ArcSorter::qstexc( char *i, char *j, char *k )// i<-k, k<-j, j<-i
{
    Arc **jarc1 = (Arc **) i;
    Arc **jarc2 = (Arc **) j;
    Arc **jarc3 = (Arc **) k;
    Arc *tmp = *jarc1;
    *jarc1 = *jarc3;
    *jarc3 = *jarc2;
    *jarc2 = tmp;
}
  

ArcSdirSorter::ArcSdirSorter( Subdivider &s ) : ArcSorter(s)
{
}

int
ArcSdirSorter::qscmp( char *i, char *j )
{
    Arc *jarc1 = *(Arc **) i;
    Arc *jarc2 = *(Arc **) j;

    int v1 = (jarc1->getitail() ? 0 : (jarc1->pwlArc->npts - 1));
    int	v2 = (jarc2->getitail() ? 0 : (jarc2->pwlArc->npts - 1));

    REAL diff =  jarc1->pwlArc->pts[v1].param[1] -
	    	 jarc2->pwlArc->pts[v2].param[1];

    if( diff < 0.0)
	return -1;
    else if( diff > 0.0)
	return 1;
    else {
	if( v1 == 0 ) {
	    if( jarc2->tail()[0] < jarc1->tail()[0] ) {
	        return subdivider.ccwTurn_sl( jarc2, jarc1 ) ? 1 : -1;
	    } else {
	        return subdivider.ccwTurn_sr( jarc2, jarc1 ) ? -1 : 1;
	    }
	} else {
	    if( jarc2->head()[0] < jarc1->head()[0] ) {
	        return subdivider.ccwTurn_sl( jarc1, jarc2 ) ? -1 : 1;
	    } else {
	        return subdivider.ccwTurn_sr( jarc1, jarc2 ) ? 1 : -1;
	    }
	}
    }    
}

ArcTdirSorter::ArcTdirSorter( Subdivider &s ) : ArcSorter(s)
{
}

/*----------------------------------------------------------------------------
 * ArcTdirSorter::qscmp - 
  *		   compare two axis monotone arcs that are incident 
 *		   to the line T == compare_value. Determine which of the
 *		   two intersects that line with a LESSER S value.  If
 *		   jarc1 does, return 1.  If jarc2 does, return -1. 
 *----------------------------------------------------------------------------
 */
int
ArcTdirSorter::qscmp( char *i, char *j )
{
    Arc *jarc1 = *(Arc **) i;
    Arc *jarc2 = *(Arc **) j;

    int v1 = (jarc1->getitail() ? 0 : (jarc1->pwlArc->npts - 1));
    int	v2 = (jarc2->getitail() ? 0 : (jarc2->pwlArc->npts - 1));

    REAL diff =  jarc1->pwlArc->pts[v1].param[0] -
	         jarc2->pwlArc->pts[v2].param[0];
 
    if( diff < 0.0)
	return 1;
    else if( diff > 0.0)
	return -1;
    else {
	if( v1 == 0 ) {
	    if (jarc2->tail()[1] < jarc1->tail()[1]) {
	        return subdivider.ccwTurn_tl( jarc2, jarc1 ) ? 1 : -1;
	    } else {
	        return subdivider.ccwTurn_tr( jarc2, jarc1 ) ? -1 : 1;
	    }
	} else {
	    if( jarc2->head()[1] < jarc1->head()[1] )  {
	        return subdivider.ccwTurn_tl( jarc1, jarc2 ) ? -1 : 1;
	    } else {
	        return subdivider.ccwTurn_tr( jarc1, jarc2 ) ? 1 : -1;
	    }
	}
    }
}



#endif /* __gluarcsorter_c_ */

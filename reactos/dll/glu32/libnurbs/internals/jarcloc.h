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
 * jarcloc.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/jarcloc.h,v 1.1 2004/02/02 16:39:11 navaraf Exp $
 */

#ifndef __glujarcloc_h_
#define __glujarcloc_h_

#include "arc.h"

class Jarcloc {
private:
    Arc_ptr 		arc;
    TrimVertex		*p;
    TrimVertex		*plast;
public:
    inline void		init( Arc_ptr a, long first, long last ) { arc = a; p=&a->pwlArc->pts[first]; plast = &a->pwlArc->pts[last]; }
    inline TrimVertex *	getnextpt( void );
    inline TrimVertex *	getprevpt( void );
    inline void		reverse();
};

inline void
Jarcloc::reverse()
{
    if( plast == &arc->pwlArc->pts[0] )
	plast =  &arc->pwlArc->pts[arc->pwlArc->npts - 1];
    else
	plast =  &arc->pwlArc->pts[0];
}

inline TrimVertex *
Jarcloc::getnextpt()
{
    assert( p <= plast );
    if( p == plast ) {
	arc = arc->next;
	p = &arc->pwlArc->pts[0];
	plast = &arc->pwlArc->pts[arc->pwlArc->npts - 1];
	assert( p < plast );
    }
    return p++;
}
	
inline TrimVertex *
Jarcloc::getprevpt()
{
    assert( p >= plast );
    if( p == plast ) {
	arc = arc->prev;
	p = &arc->pwlArc->pts[arc->pwlArc->npts - 1];
	plast = &arc->pwlArc->pts[0];
	assert( p > plast );
    }
    return p--;
}
#endif /* __glujarcloc_h_ */

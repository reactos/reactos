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
 * trimline.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/trimline.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
 */

#ifndef __glutrimline_h_
#define __glutrimline_h_

class Arc;
class Backend;

#include "trimvertex.h"
#include "jarcloc.h"


class Trimline {
private:
    TrimVertex**	pts;
    long 		numverts;
    long		i;
    long		size;
    Jarcloc		jarcl;
    TrimVertex		t, b;
    TrimVertex 		*tinterp, *binterp;
    void		reset( void ) { numverts = 0; }
    inline void		grow( long );
    inline void		swap( void );
    inline void		append( TrimVertex * );
    static long		interpvert( TrimVertex *, TrimVertex *, TrimVertex *, REAL );



public:
			Trimline();
			~Trimline();
    void		init( TrimVertex * );
    void		init( long, Arc_ptr, long );
    void		getNextPt( void );
    void		getPrevPt( void );
    void		getNextPts( REAL, Backend & );
    void		getPrevPts( REAL, Backend & );
    void		getNextPts( Arc_ptr );
    void		getPrevPts( Arc_ptr );
    inline TrimVertex *	next( void );
    inline TrimVertex *	prev( void );
    inline TrimVertex *	first( void );
    inline TrimVertex *	last( void );
};

inline TrimVertex *
Trimline::next( void )
{
    if( i < numverts) return pts[i++]; else return 0;
}

inline TrimVertex *
Trimline::prev( void )
{
    if( i >= 0 ) return pts[i--]; else return 0;
}

inline TrimVertex *
Trimline::first( void )
{
    i = 0; return pts[i];
}

inline TrimVertex *
Trimline::last( void )
{
    i = numverts; return pts[--i];
}
#endif /* __glutrimline_h_ */

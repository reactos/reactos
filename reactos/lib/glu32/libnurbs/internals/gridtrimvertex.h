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
 * gridtrimvertex.h
 *
 * $Date: 2004/02/02 16:39:11 $ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/gridtrimvertex.h,v 1.1 2004/02/02 16:39:11 navaraf Exp $
 */

#ifndef __glugridtrimvertex_h_
#define __glugridtrimvertex_h_

#include "mystdlib.h"
#include "bufpool.h"
#include "trimvertex.h"
#include "gridvertex.h"

class GridTrimVertex : public PooledObj
{
private:
    TrimVertex	dummyt;
    GridVertex	dummyg;
public:
			GridTrimVertex() { g = 0; t = 0; }
    TrimVertex	*t;
    GridVertex	*g;
   
    inline void		set( long, long );
    inline void		set( REAL, REAL );
    inline void		set( TrimVertex * );
    inline void		clear( void ) { t = 0; g = 0; };
    inline int		isGridVert() { return g ? 1 : 0 ; }
    inline int		isTrimVert() { return t ? 1 : 0 ; }
    inline void		output();
};

inline void
GridTrimVertex::set( long x, long y )
{
    g = &dummyg;
    dummyg.gparam[0] = x;
    dummyg.gparam[1] = y;
}

inline void
GridTrimVertex::set( REAL x, REAL y )
{
    g = 0;
    t = &dummyt;
    dummyt.param[0] = x;
    dummyt.param[1] = y;
    dummyt.nuid = 0;
}

inline void
GridTrimVertex::set( TrimVertex *v )
{
    g = 0;
    t = v;
}

typedef GridTrimVertex *GridTrimVertex_p;
#endif /* __glugridtrimvertex_h_ */

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
 * mesher.h
 *
 * $Date: 2004/02/02 16:39:12 $ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/mesher.h,v 1.1 2004/02/02 16:39:12 navaraf Exp $
 */

#ifndef __glumesher_h_
#define __glumesher_h_

#include "hull.h"

class TrimRegion;
class Backend;
class Pool;
// struct GridTrimVertex;


class Mesher : virtual public TrimRegion, public Hull {
public:
     			Mesher( Backend & );
			~Mesher( void );
    void		init( unsigned int );
    void		mesh( void );

private:
    static const float	ZERO;
    Backend&		backend;

    Pool		p;
    unsigned int	stacksize;
    GridTrimVertex **	vdata;
    GridTrimVertex *	last[2];
    int			itop;
    int			lastedge;

    inline void		openMesh( void );
    inline void		swapMesh( void );
    inline void		closeMesh( void );
    inline int		isCcw( int );
    inline int		isCw( int );
    inline void		clearStack( void );
    inline void		push( GridTrimVertex * );
    inline void		pop( long );
    inline void		move( int, int );
    inline int 		equal( int, int );
    inline void 	copy( int, int );
    inline void 	output( int );
    void		addUpper( void );
    void		addLower( void );
    void		addLast( void );
    void		finishUpper( GridTrimVertex * );
    void		finishLower( GridTrimVertex * );
};
#endif /* __glumesher_h_ */

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
 * quilt.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/quilt.h,v 1.1 2004/02/02 16:39:12 navaraf Exp $
 */

#ifndef __gluquilt_h_
#define __gluquilt_h_

#include "defines.h"
#include "bufpool.h"
#include "types.h"

class Backend;
class Mapdesc;
class Flist;
struct Knotvector;

/* constants for memory allocation of NURBS to Bezier conversion */
#define	MAXDIM 		2

struct Quiltspec { /* a specification for a dimension of a quilt */
    int			stride;		/* words between points */
    int			width;		/* number of segments */
    int			offset;		/* words to first point */
    int			order;		/* order */
    int			index;		/* current segment number */
    int			bdry[2];	/* boundary edge flag */
    REAL  		step_size;
    Knot *		breakpoints;
};

typedef Quiltspec *Quiltspec_ptr;

class Quilt : public PooledObj { /* an array of bezier patches */
public:
    			Quilt( Mapdesc * );
    Mapdesc *		mapdesc;	/* map descriptor */
    REAL *		cpts;		/* control points */
    Quiltspec		qspec[MAXDIM];	/* the dimensional data */
    Quiltspec_ptr	eqspec;		/* qspec trailer */
    Quilt		*next;		/* next quilt in linked list */

public:
    void		deleteMe( Pool& );
    void		toBezier( Knotvector &, INREAL *, long  );
    void		toBezier( Knotvector &, Knotvector &, INREAL *, long  );
    void		select( REAL *, REAL * );
    int			getDimension( void ) { return eqspec - qspec; }
    void 		download( Backend & );
    void		downloadAll( REAL *, REAL *, Backend & );
    int 		isCulled( void );
    void		getRange( REAL *, REAL *, Flist&, Flist & );
    void		getRange( REAL *, REAL *, int, Flist & );
    void		getRange( REAL *, REAL *, Flist&  );
    void		findRates( Flist& slist, Flist& tlist, REAL[2] );
    void		findSampleRates( Flist& slist, Flist& tlist );
    void		show();
};

typedef class Quilt *Quilt_ptr;

#endif /* __gluquilt_h_ */

/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/*
 * quilt.h
 *
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

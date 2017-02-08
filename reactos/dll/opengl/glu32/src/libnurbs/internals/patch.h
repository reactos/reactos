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
 * patch.h
 *
 */

#ifndef __glupatch_h_
#define __glupatch_h_

#include "types.h"
#include "defines.h"

class Quilt;
class Mapdesc;


struct Pspec {
    REAL		range[3];
    REAL		sidestep[2];
    REAL		stepsize;
    REAL		minstepsize;
    int			needsSubdivision;
};

struct Patchspec : public Pspec {
    int			order;
    int			stride;
    void 		clamp( REAL );
    void 		getstepsize( REAL );
    void		singleStep( void );    
};

class Patch {
public:
friend class Subdivider;
friend class Quilt;
friend class Patchlist;
    			Patch( Quilt *, REAL*, REAL *, Patch * );
    			Patch( Patch &, int, REAL, Patch * );
    void		bbox( void );
    void		clamp( void );
    void		getstepsize( void );
    int			cullCheck( void );
    int			needsSubdivision( int );
    int			needsSamplingSubdivision( void );
    int			needsNonSamplingSubdivision( void );

    int                 get_uorder() {return pspec[0].order;}
    int                 get_vorder() {return pspec[1].order;}

private:

    Mapdesc*		mapdesc;
    Patch*		next;
    int			cullval;
    int			notInBbox;
    int			needsSampling;
    REAL		cpts[MAXORDER*MAXORDER*MAXCOORDS]; //culling pts 
    REAL		spts[MAXORDER*MAXORDER*MAXCOORDS]; //sampling pts 
    REAL		bpts[MAXORDER*MAXORDER*MAXCOORDS]; //bbox pts
    Patchspec		pspec[2];
    void 		checkBboxConstraint( void );
    REAL 		bb[2][MAXCOORDS];
};
#endif /* __glupatch_h_ */

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
 * trimline.h
 *
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

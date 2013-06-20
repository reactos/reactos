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
 * jarcloc.h
 *
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

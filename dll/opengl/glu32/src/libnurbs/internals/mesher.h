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
 * mesher.h
 *
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

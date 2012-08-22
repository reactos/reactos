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
 * gridtrimvertex.h
 *
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

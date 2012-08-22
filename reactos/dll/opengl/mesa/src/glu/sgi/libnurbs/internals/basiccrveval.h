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
 * basiccurveeval.h
 *
 */

#ifndef __glubasiccrveval_h_
#define __glubasiccrveval_h_

#include "types.h"
#include "displaymode.h"
#include "cachingeval.h"

class BasicCurveEvaluator : public CachingEvaluator {
public:
    virtual             ~BasicCurveEvaluator() { /* silence warning*/ }
    virtual void	domain1f( REAL, REAL );
    virtual void	range1f( long, REAL *, REAL * );

    virtual void	enable( long );
    virtual void	disable( long );
    virtual void	bgnmap1f( long );
    virtual void	map1f( long, REAL, REAL, long, long, REAL * );
    virtual void	mapgrid1f( long, REAL, REAL );
    virtual void	mapmesh1f( long, long, long );
    virtual void	evalcoord1f( long, REAL );
    virtual void	endmap1f( void );

    virtual void	bgnline( void );
    virtual void	endline( void );
};

#endif /* __glubasiccrveval_h_ */

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
 * arctess.h
 *
 */

#ifndef __gluarctess_h_
#define __gluarctess_h_

#include "defines.h"
#include "types.h"
#include "arc.h"

struct BezierArc;
class Pool;
class TrimVertexPool;

class ArcTessellator {
public:
			ArcTessellator( TrimVertexPool&, Pool& );
			~ArcTessellator( void );
    void		bezier( Arc_ptr, REAL, REAL, REAL, REAL );
    void		pwl( Arc_ptr, REAL, REAL, REAL, REAL, REAL );
    void		pwl_left( Arc_ptr, REAL, REAL, REAL, REAL );
    void		pwl_right( Arc_ptr, REAL, REAL, REAL, REAL );
    void		pwl_top( Arc_ptr, REAL, REAL, REAL, REAL );
    void		pwl_bottom( Arc_ptr, REAL, REAL, REAL, REAL );
    void		tessellateLinear( Arc_ptr, REAL, REAL, int );
    void		tessellateNonlinear( Arc_ptr, REAL, REAL, int );
private:
    static const REAL 	gl_Bernstein[][MAXORDER][MAXORDER];
    Pool&		pwlarcpool;
    TrimVertexPool&	trimvertexpool;
    static void		trim_power_coeffs( BezierArc *, REAL[MAXORDER], int );
};

#endif /* __gluarctess_h_ */

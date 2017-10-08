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
 * trimregion.h
 *
 */

#ifndef __glutrimregion_h_
#define __glutrimregion_h_

#include "trimline.h"
#include "gridline.h"
#include "uarray.h"

class Arc;
class Backend;

class TrimRegion {
public:
			TrimRegion();
    Trimline		left;
    Trimline		right;
    Gridline		top;
    Gridline		bot;
    Uarray		uarray;

    void		init( REAL );
    void		advance( REAL, REAL, REAL );
    void		setDu( REAL );
    void		init( long, Arc_ptr );
    void		getPts( Arc_ptr );
    void		getPts( Backend & );
    void		getGridExtent( TrimVertex *, TrimVertex * );
    void		getGridExtent( void );
    int			canTile( void );
private:
    REAL		oneOverDu;
};

inline void
TrimRegion::init( REAL vval ) 
{
    bot.vval = vval;
}

inline void
TrimRegion::advance( REAL topVindex, REAL botVindex, REAL botVval )
{
    top.vindex	= (long) topVindex;
    bot.vindex	= (long) botVindex;
    top.vval	= bot.vval;
    bot.vval	= botVval;
    top.ustart	= bot.ustart;
    top.uend	= bot.uend;
}
#endif /* __glutrimregion_h_ */

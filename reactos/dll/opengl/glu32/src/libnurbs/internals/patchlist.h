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
 * patchlist.h
 *
 */

#ifndef __glupatchlist_h_
#define __glupatchlist_h_

#include "types.h"
#include "defines.h"
#include "patch.h"

class Quilt;

class Patchlist {
friend class Subdivider;
public:
    			Patchlist( Quilt *, REAL *, REAL * );
    			Patchlist( Patchlist &, int ,  REAL );
    			~Patchlist();	
    void		bbox();
    int			cullCheck( void );
    void		getstepsize( void );
    int			needsNonSamplingSubdivision( void );
    int			needsSamplingSubdivision( void );
    int			needsSubdivision( int );
    REAL		getStepsize( int );
    void                getRanges(REAL ranges[4]);

    int                 get_uorder();
    int                 get_vorder();
private:
    Patch		*patch;
    int			notInBbox;
    int			needsSampling;
    Pspec		pspec[2];
};

inline REAL
Patchlist::getStepsize( int param )
{
    return pspec[param].stepsize;
}

inline int
Patchlist::get_uorder()
{  
   return patch->get_uorder();

}

inline int
 Patchlist::get_vorder()
{ 
  return patch->get_vorder();
}





#endif /* __glupatchlist_h_ */

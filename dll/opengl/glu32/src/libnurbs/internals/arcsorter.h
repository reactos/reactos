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
 * arcsorter.h
 *
 */

#ifndef __gluarcsorter_h_
#define __gluarcsorter_h_

#include "sorter.h"

class Arc;
class Subdivider;

class ArcSorter : private Sorter {
public:
			ArcSorter(Subdivider &);
    void		qsort( Arc **a, int n );
protected:
    virtual int		qscmp( char *, char * );
    Subdivider&		subdivider;
private:
    void		qsexc( char *i, char *j );	// i<-j, j<-i 
    void		qstexc( char *i, char *j, char *k ); // i<-k, k<-j, j<-i 
};


class ArcSdirSorter : public ArcSorter {
public:
			ArcSdirSorter( Subdivider & );
private:
    int			qscmp( char *, char * );
};


class ArcTdirSorter : public ArcSorter {
public:
			ArcTdirSorter( Subdivider & );
private:
    int			qscmp( char *, char * );
};

#endif /* __gluarcsorter_h_ */

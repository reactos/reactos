/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
*/

/*
 * sorter.c++
 *
 */

#include "glimports.h"
#include "sorter.h"
#include "mystdio.h"

Sorter::Sorter( int _es )
{
    es = _es;
}

void
Sorter::qsort( void *a, int n )
{
    qs1( (char *)a, ((char *)a)+n*es);
}

int
Sorter::qscmp( char *, char * )
{
    _glu_dprintf( "Sorter::qscmp: pure virtual called\n" );
    return 0;
}


void
Sorter::qsexc( char *, char * )
{
    _glu_dprintf( "Sorter::qsexc: pure virtual called\n" );
}


void
Sorter::qstexc( char *, char *, char * )
{
    _glu_dprintf( "Sorter::qstexc: pure virtual called\n" );
}

void
Sorter::qs1( char *a,  char *l )
{
    char *i, *j;
    char	*lp, *hp;
    int	c;
    unsigned int n;

start:
    if((n=l-a) <= (unsigned int)es)
	    return;
    n = es * (n / (2*es));
    hp = lp = a+n;
    i = a;
    j = l-es;
    while(1) {
	if(i < lp) {
	    if((c = qscmp(i, lp)) == 0) {
		qsexc(i, lp -= es);
		continue;
	    }
	    if(c < 0) {
		i += es;
		continue;
	    }
	}

loop:
	if(j > hp) {
	    if((c = qscmp(hp, j)) == 0) {
		qsexc(hp += es, j);
		goto loop;
	    }
	    if(c > 0) {
		if(i == lp) {
		    qstexc(i, hp += es, j);
		    i = lp += es;
		    goto loop;
		}
		qsexc(i, j);
		j -= es;
		i += es;
		continue;
	    }
	    j -= es;
	    goto loop;
	}

	if(i == lp) {
	    if(lp-a >= l-hp) {
		qs1(hp+es, l);
		l = lp;
	    } else {
		qs1(a, lp);
		a = hp+es;
	    }
	    goto start;
	}

	qstexc(j, lp -= es, i);
	j = hp -= es;
    }
}


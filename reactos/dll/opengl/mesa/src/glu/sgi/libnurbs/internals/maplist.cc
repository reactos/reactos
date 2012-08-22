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
 * maplist.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "mymath.h"
#include "nurbsconsts.h"
#include "maplist.h"
#include "mapdesc.h"
 
Maplist::Maplist( Backend& b )
    : mapdescPool( sizeof( Mapdesc ), 10, "mapdesc pool" ),
      backend( b )
{
    maps = 0; lastmap = &maps;
}

void 
Maplist::initialize( void )
{
    freeMaps();
    define( N_P2D, 0, 2 );
    define( N_P2DR, 1, 3 );
}

void 
Maplist::add( long type, int israt, int ncoords )
{
    *lastmap = new(mapdescPool) Mapdesc( type, israt, ncoords, backend );
    lastmap = &((*lastmap)->next);
}

void 
Maplist::define( long type, int israt, int ncoords )
{
#ifndef NDEBUG // to avoid warning
    Mapdesc *m = locate( type );
    assert( m == NULL || ( m->isrational == israt && m->ncoords == ncoords ) );
#endif
    add( type, israt, ncoords );
}

void 
Maplist::remove( Mapdesc *m )
{
    for( Mapdesc **curmap = &maps; *curmap; curmap = &((*curmap)->next) ) {
	if( *curmap == m ) {
	    *curmap = m->next;
	    m->deleteMe( mapdescPool );
	    return;
	}
    }
    abort();
}

void
Maplist::freeMaps( void )
{
    mapdescPool.clear();
    maps = 0;
    lastmap = &maps;
}

Mapdesc * 
Maplist::find( long type )
{
    Mapdesc *val = locate( type );
    assert( val != 0 );
    return val;
}

Mapdesc * 
Maplist::locate( long type )
{
    Mapdesc *m;
    for( m = maps; m; m = m->next )
	if( m->getType() == type ) break;
    return m;
}

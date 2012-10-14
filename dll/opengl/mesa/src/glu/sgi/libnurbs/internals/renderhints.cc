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
 * renderhints.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "renderhints.h"
#include "nurbsconsts.h"


/*--------------------------------------------------------------------------
 * Renderhints::Renderhints - set all window specific options
 *--------------------------------------------------------------------------
 */
Renderhints::Renderhints()
{
    display_method 	= N_FILL;
    errorchecking 	= N_MSG;
    subdivisions 	= 6.0;
    tmp1 		= 0.0;
    displaydomain 	= 0;
    maxsubdivisions 	= (int) subdivisions;
    wiretris	 	= 0;
    wirequads	 	= 0;
}

void
Renderhints::init( void )
{
    maxsubdivisions = (int) subdivisions;
    if( maxsubdivisions < 0 ) maxsubdivisions = 0;


    if( display_method == N_FILL ) {
	wiretris = 0;
	wirequads = 0;
    } else if( display_method == N_OUTLINE_TRI ) {
	wiretris = 1;
	wirequads = 0;
    } else if( display_method == N_OUTLINE_QUAD ) {
	wiretris = 0;
	wirequads = 1;
    } else {
	wiretris = 1;
	wirequads = 1;
    }
}

int
Renderhints::isProperty( long property )
{
    switch ( property ) {
	case N_DISPLAY:
	case N_ERRORCHECKING:
	case N_SUBDIVISIONS:
        case N_TMP1:
	    return 1;
	default:
	    return 0;
    }
}

REAL 
Renderhints::getProperty( long property )
{
    switch ( property ) {
	case N_DISPLAY:
	    return display_method;
	case N_ERRORCHECKING:
	    return errorchecking;
	case N_SUBDIVISIONS:
	    return subdivisions;
        case N_TMP1:
	    return tmp1;
	default:
	    abort();
	    return -1;  //not necessary, needed to shut up compiler
    }
}

void 
Renderhints::setProperty( long property, REAL value )
{
    switch ( property ) {
	case N_DISPLAY:
	    display_method = value;
	    break;
	case N_ERRORCHECKING:
	    errorchecking = value;
	    break;
	case N_SUBDIVISIONS:
	    subdivisions = value;
	    break;
	case N_TMP1: /* unused */
	    tmp1 = value;
	    break;
	default:
	    abort();
	    break;
    }
}

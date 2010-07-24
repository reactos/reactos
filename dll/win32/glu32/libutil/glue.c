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
**
** $Date$ $Revision: 1.1 $
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libutil/glue.c,v 1.1 2004/02/02 16:39:15 navaraf Exp $
*/

#include <stdlib.h>
#include "gluint.h"

static unsigned char *__gluNurbsErrors[] = {
    (unsigned char*) " ",
    (unsigned char*) "spline order un-supported",
    (unsigned char*) "too few knots",
    (unsigned char*) "valid knot range is empty",
    (unsigned char*) "decreasing knot sequence knot",
    (unsigned char*) "knot multiplicity greater than order of spline",
    (unsigned char*) "gluEndCurve() must follow gluBeginCurve()",
    (unsigned char*) "gluBeginCurve() must precede gluEndCurve()",
    (unsigned char*) "missing or extra geometric data",
    (unsigned char*) "can't draw piecewise linear trimming curves",
    (unsigned char*) "missing or extra domain data",
    (unsigned char*) "missing or extra domain data",
    (unsigned char*) "gluEndTrim() must precede gluEndSurface()",
    (unsigned char*) "gluBeginSurface() must precede gluEndSurface()",
    (unsigned char*) "curve of improper type passed as trim curve",
    (unsigned char*) "gluBeginSurface() must precede gluBeginTrim()",
    (unsigned char*) "gluEndTrim() must follow gluBeginTrim()",
    (unsigned char*) "gluBeginTrim() must precede gluEndTrim()",
    (unsigned char*) "invalid or missing trim curve",
    (unsigned char*) "gluBeginTrim() must precede gluPwlCurve()",
    (unsigned char*) "piecewise linear trimming curve referenced twice",
    (unsigned char*) "piecewise linear trimming curve and nurbs curve mixed",
    (unsigned char*) "improper usage of trim data type",
    (unsigned char*) "nurbs curve referenced twice",
    (unsigned char*) "nurbs curve and piecewise linear trimming curve mixed",
    (unsigned char*) "nurbs surface referenced twice",
    (unsigned char*) "invalid property",
    (unsigned char*) "gluEndSurface() must follow gluBeginSurface()",
    (unsigned char*) "intersecting or misoriented trim curves",
    (unsigned char*) "intersecting trim curves",
    (unsigned char*) "UNUSED",
    (unsigned char*) "unconnected trim curves",
    (unsigned char*) "unknown knot error",
    (unsigned char*) "negative vertex count encountered",
    (unsigned char*) "negative byte-stride encounteed",
    (unsigned char*) "unknown type descriptor",
    (unsigned char*) "null control point reference",
    (unsigned char*) "duplicate point on piecewise linear trimming curve",
};

const unsigned char *__gluNURBSErrorString( int errnum )
{
    return __gluNurbsErrors[errnum];
}

static unsigned char *__gluTessErrors[] = {
    (unsigned char*) " ",
    (unsigned char*) "gluTessBeginPolygon() must precede a gluTessEndPolygon()",
    (unsigned char*) "gluTessBeginContour() must precede a gluTessEndContour()",
    (unsigned char*) "gluTessEndPolygon() must follow a gluTessBeginPolygon()",
    (unsigned char*) "gluTessEndContour() must follow a gluTessBeginContour()",
    (unsigned char*) "a coordinate is too large",
    (unsigned char*) "need combine callback",
};

const unsigned char *__gluTessErrorString( int errnum )
{
    return __gluTessErrors[errnum];
} /* __glTessErrorString() */

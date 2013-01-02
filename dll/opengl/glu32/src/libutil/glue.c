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

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
 * reader.h
 *
 */

#ifndef __glureader_h_
#define __glureader_h_

#include "bufpool.h"
#include "types.h"

enum Curvetype { ct_nurbscurve, ct_pwlcurve, ct_none };
    
struct Property;
struct O_surface;
struct O_nurbssurface;
struct O_trim;
class O_pwlcurve;
struct O_nurbscurve;
struct O_curve;
class  Quilt;
class TrimVertex;


struct O_curve : public PooledObj {
    union {
        O_nurbscurve	*o_nurbscurve;
        O_pwlcurve	*o_pwlcurve;
    } curve;
    Curvetype		curvetype;	/* arc type: pwl or nurbs	*/
    O_curve *		next;		/* next arc in loop		*/
    O_surface *		owner;		/* owning surface		*/
    int			used;		/* curve called in cur surf	*/
    int			save;		/* 1 if in display list		*/
    long		nuid;
    			O_curve() { next = 0; used = 0; owner = 0; 
				    curve.o_pwlcurve = 0; curvetype = ct_none; save = 0; nuid = 0; }
    };

struct O_nurbscurve : public PooledObj {
    Quilt		*bezier_curves;	/* array of bezier curves	*/
    long		type;		/* range descriptor		*/
    REAL		tesselation;	/* tesselation tolerance 	*/
    int			method;		/* tesselation method 		*/
    O_nurbscurve *	next;		/* next curve in list		*/
    int			used;		/* curve called in cur surf	*/
    int			save;		/* 1 if in display list		*/
    O_curve *		owner;		/* owning curve 		*/
			O_nurbscurve( long _type ) 
			   { bezier_curves = 0; type = _type; tesselation = 0; method = 0; next = 0; used = 0; save = 0; owner = 0; }
    };
 
class O_pwlcurve : public PooledObj {
public:
    TrimVertex		*pts;		/* array of trim vertices	*/
    int			npts;		/* number of trim vertices	*/
    O_pwlcurve *	next;		/* next curve in list		*/
    int			used;		/* curve called in cur surf	*/
    int			save;		/* 1 if in display list		*/
    O_curve *		owner;		/* owning curve 		*/
			O_pwlcurve( long, long, INREAL *, long, TrimVertex * );
    };

struct O_trim : public PooledObj {
    O_curve		*o_curve;	/* closed trim loop	 	*/
    O_trim *		next;		/* next loop along trim 	*/
    int			save;		/* 1 if in display list		*/
			O_trim() { next = 0; o_curve = 0; save = 0; }
    };

struct O_nurbssurface : public PooledObj {
    Quilt *		bezier_patches;/* array of bezier patches	*/
    long		type;		/* range descriptor		*/
    O_surface *		owner;		/* owning surface		*/
    O_nurbssurface *	next;		/* next surface in chain	*/
    int			save;		/* 1 if in display list		*/
    int			used;		/* 1 if prev called in block	*/
			O_nurbssurface( long _type ) 
			   { bezier_patches = 0; type = _type; owner = 0; next = 0; save = 0; used = 0; }
    };

struct O_surface : public PooledObj {
    O_nurbssurface *	o_nurbssurface;	/* linked list of surfaces	*/
    O_trim *		o_trim;		/* list of trim loops		*/
    int			save;		/* 1 if in display list		*/
    long		nuid;
			O_surface() { o_trim = 0; o_nurbssurface = 0; save = 0; nuid = 0; }
    };

struct Property : public PooledObj {
    long		type;
    long		tag;
    REAL		value;
    int			save;		/* 1 if in display list		*/
			Property( long _type, long _tag, INREAL _value )
			{ type = _type; tag = _tag; value = (REAL) _value; save = 0; }
			Property( long _tag, INREAL _value )
			{ type = 0; tag = _tag; value = (REAL) _value; save = 0; }
    };

class NurbsTessellator;
#endif /* __glureader_h_ */

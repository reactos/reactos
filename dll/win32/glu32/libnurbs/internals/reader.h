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
 * reader.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/reader.h,v 1.1 2004/02/02 16:39:12 navaraf Exp $
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
				    curve.o_pwlcurve = 0; }
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
			   { type = _type; owner = 0; next = 0; used = 0; }
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
			O_trim() { next = 0; o_curve = 0; }
    };

struct O_nurbssurface : public PooledObj {
    Quilt *		bezier_patches;/* array of bezier patches	*/
    long		type;		/* range descriptor		*/
    O_surface *		owner;		/* owning surface		*/
    O_nurbssurface *	next;		/* next surface in chain	*/
    int			save;		/* 1 if in display list		*/
    int			used;		/* 1 if prev called in block	*/
			O_nurbssurface( long _type )
			   { type = _type; owner = 0; next = 0; used = 0; }
    };

struct O_surface : public PooledObj {
    O_nurbssurface *	o_nurbssurface;	/* linked list of surfaces	*/
    O_trim *		o_trim;		/* list of trim loops		*/
    int			save;		/* 1 if in display list		*/
    long		nuid;
			O_surface() { o_trim = 0; o_nurbssurface = 0; }
    };

struct Property : public PooledObj {
    long		type;
    long		tag;
    REAL		value;
    int			save;		/* 1 if in display list		*/
			Property( long _type, long _tag, INREAL _value )
			{ type = _type; tag = _tag; value = (REAL) _value; }
			Property( long _tag, INREAL _value )
			{ type = 0; tag = _tag; value = (REAL) _value; }
    };

class NurbsTessellator;
#endif /* __glureader_h_ */

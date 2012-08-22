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
 * tobezier.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "mystring.h"
#include "quilt.h"
#include "knotvector.h"

/* local type definitions */
struct Breakpt {		/* breakpoints	*/
    Knot		value;		/* value	*/
    int			multi;		/* multiplicity	*/
    int			def;		/* deficit */
};

struct Knotspec {		/* knotvector format */
    long		order;		/* order of spline  */
    Knot_ptr		inkbegin;	/* input knot sequence */
    Knot_ptr		inkend;		/* location after last knot */
    Knot_ptr		outkbegin;	/* in-process knot subsequence */
    Knot_ptr		outkend;	/* location after last knot */
    Knot_ptr		kleft;		/* */
    Knot_ptr		kright;		/* */
    Knot_ptr		kfirst;		/* */
    Knot_ptr		klast;		/* */
    Knot_ptr		sbegin;		/* conversion factor values */
    Breakpt *		bbegin;		/* in-process breakpoints */
    Breakpt *		bend;		/* last breakpoint */
    int			ncoords;	/* coordinates per control point */
    int			prestride;	/* stride between input points */
    int			poststride;	/* stride between output points	*/
    int 		preoffset;	/* scaled point offset	*/
    int 		postoffset;	/* scaled point offset	*/
    int 		prewidth;	/* width of dimension	*/
    int 		postwidth;	/* width of dimension	*/
    int 		istransformed;	/* was dimension transformed */
    Knotspec *		next;   	/* next knotspec */
    Knotspec *		kspectotrans;   /* knotspec in transformation direction */

			Knotspec( void );
			~Knotspec( void );
    void		factors( void );
    void		insert( REAL * );
    void		preselect();
    void		select( void );
    void		copy( INREAL *, REAL * );
    void		breakpoints( void );
    void		knots( void );
    void		transform( REAL * );
    void		showpts( REAL * );
    
    void		pt_io_copy( REAL *, INREAL * );
    void		pt_oo_copy( REAL *, REAL * );
    void		pt_oo_sum( REAL*, REAL*, REAL*, Knot, Knot );
};

struct Splinespec {		/* a non-uniform tensor element */
			Splinespec( int );
                        ~Splinespec(void);
    Knotspec		*kspec;	/* format of each param. dir. */
    int			dim;		/* domain dimension */
    REAL *		outcpts;	/* Bezier control points */

    void		kspecinit( Knotvector & );
    void		kspecinit( Knotvector &, Knotvector & );
    void		select( void );
    void		layout( long );
    void		setupquilt( Quilt_ptr );
    void		copy( INREAL * );
    void		transform( void );
};

/*-----------------------------------------------------------------------------
 * Quilt::toBezier - convert from NURBS to rational Bezier 
 *-----------------------------------------------------------------------------
 */

void
Quilt::toBezier( 
    Knotvector& knotvector,	/* a knot vector */
    INREAL *ctlpts,		/* input contol points */
    long ncoords )		/* number of coordinates per control point */
{
    Splinespec spline( 1 );
    spline.kspecinit( knotvector );
    spline.select();
    spline.layout( ncoords );
    spline.setupquilt( this );
    spline.copy( ctlpts );
    spline.transform();
}

void
Quilt::toBezier( 
    Knotvector& sknotvector,	/* a knot vector */
    Knotvector& tknotvector,	/* a knot vector */
    INREAL *ctlpts,		/* input contol points */
    long ncoords )		/* number of coordinates per control point */
{
    Splinespec spline( 2 );
    spline.kspecinit( sknotvector, tknotvector );
    spline.select();
    spline.layout( ncoords );
    spline.setupquilt( this );
    spline.copy( ctlpts );
    spline.transform();
}
Splinespec::Splinespec( int dimen )
{
    dim = dimen;
}

Splinespec::~Splinespec( void )
{
    /* Note: do NOT delete 'outcpts' here since its address (not contents)
     * is copied in 'cpts' in this file in function Splinespec::setupquilt().
     * This block of memory will eventually be deleted in file quilt.c++ in
     * function Quilt::deleteMe() through 'cpts' so do NOT delete it here!
     */	
    Knotspec *ktrav= kspec;         //start at beginning of list 
    while (ktrav != 0) {            //any items to delete? 
       Knotspec *deleteThis= ktrav; //remember to delete this 
       ktrav= ktrav->next;          //go to next item if any
       delete deleteThis;           //delete it
    }	
} /* ~Splinespec() */

/*-----------------------------------------------------------------------------
 * Splinespec::kspecinit - initialize Splinespec structure
 *
 * Client: Quilt::toBezier
 *-----------------------------------------------------------------------------
 */

void
Splinespec::kspecinit( Knotvector& knotvector )
{
    kspec = new Knotspec;
    kspec->inkbegin = knotvector.knotlist;
    kspec->inkend = knotvector.knotlist + knotvector.knotcount;
    kspec->prestride = (int) knotvector.stride; 
    kspec->order = knotvector.order;
    kspec->next = NULL;
}

void
Splinespec::kspecinit( Knotvector& sknotvector, Knotvector& tknotvector )
{
    kspec = new Knotspec;
    Knotspec *tkspec = new Knotspec;

    kspec->inkbegin = sknotvector.knotlist;
    kspec->inkend = sknotvector.knotlist + sknotvector.knotcount;
    kspec->prestride = (int) sknotvector.stride; 
    kspec->order = sknotvector.order;
    kspec->next = tkspec;

    tkspec->inkbegin = tknotvector.knotlist;
    tkspec->inkend = tknotvector.knotlist + tknotvector.knotcount;
    tkspec->prestride = (int) tknotvector.stride; 
    tkspec->order = tknotvector.order;
    tkspec->next = NULL;
}


/*-----------------------------------------------------------------------------
 * Splinespec::select - select the subsegments to copy
 *
 * Client: gl_quilt_to_bezier	
 *-----------------------------------------------------------------------------
 */

void
Splinespec::select( )
{
    for( Knotspec *knotspec = kspec; knotspec; knotspec = knotspec->next ) {
	knotspec->preselect();
	knotspec->select();
    }
}

/*-----------------------------------------------------------------------------
 * Splinespec::layout - 
 *
 * Client: gl_quilt_to_bezier	
 *-----------------------------------------------------------------------------
 */

void
Splinespec::layout( long ncoords )
{

    long stride = ncoords;
    for( Knotspec *knotspec = kspec; knotspec; knotspec=knotspec->next ) {
	knotspec->poststride = (int) stride;
	stride *= ((knotspec->bend-knotspec->bbegin)*knotspec->order + knotspec->postoffset);
        knotspec->preoffset  *= knotspec->prestride;
	knotspec->prewidth  *= knotspec->poststride;
	knotspec->postwidth *= knotspec->poststride;
	knotspec->postoffset *= knotspec->poststride;
        knotspec->ncoords = (int) ncoords;
    }
    outcpts = new REAL[stride];
    assert( outcpts != 0 );  
}

/*-----------------------------------------------------------------------------
 * Splinespec::copy - copy the control points of current subobject
 *
 * Client: gl_quilt_to_bezier
 *-----------------------------------------------------------------------------
 */

void
Splinespec::copy( INREAL *incpts )
{
    kspec->copy( incpts, outcpts );
}

/*-----------------------------------------------------------------------------
 * Splinespec::setupquilt - assign all quilt variables from knotspec 
 *
 * Client: gl_quilt_to_bezier
 *-----------------------------------------------------------------------------
 */

void
Splinespec::setupquilt( Quilt_ptr quilt )
{
    Quiltspec_ptr qspec = quilt->qspec;
    quilt->eqspec = qspec + dim;
    for( Knotspec *knotspec = kspec; knotspec; knotspec=knotspec->next, qspec++ ) {
	qspec->stride	= knotspec->poststride;
	qspec->width	= knotspec->bend - knotspec->bbegin;
	qspec->order	= (int) knotspec->order;
	qspec->offset	= knotspec->postoffset;
	qspec->index	= 0;
	qspec->bdry[0]	= (knotspec->kleft == knotspec->kfirst) ? 1 : 0;
	qspec->bdry[1]	= (knotspec->kright == knotspec->klast) ? 1 : 0;
	qspec->breakpoints = new Knot[qspec->width+1];
	Knot_ptr k =  qspec->breakpoints;
	for( Breakpt *bk = knotspec->bbegin; bk <= knotspec->bend; bk++ )
	    *(k++) = bk->value;
    }
    quilt->cpts = outcpts;
    quilt->next = 0;
}

/*-----------------------------------------------------------------------------
 * Splinespec::transform - convert a spline to Bezier format
 *
 * Client: gl_quilt_to_bezier
 *-----------------------------------------------------------------------------
 */

void
Splinespec::transform( void )
{
    Knotspec *knotspec;
    for( knotspec = kspec; knotspec; knotspec=knotspec->next )
        knotspec->istransformed = 0;

    for( knotspec = kspec; knotspec; knotspec=knotspec->next ) {
	for( Knotspec *kspec2 = kspec; kspec2; kspec2=kspec2->next )
	    kspec2->kspectotrans = knotspec;
	kspec->transform( outcpts );
	knotspec->istransformed = 1;
    }
}


/*-----------------------------------------------------------------------------
 * Knotspec::Knotspec -  constuct a knot spec 
 *-----------------------------------------------------------------------------
 */

Knotspec::Knotspec( void )
{
    bbegin = 0;
    sbegin = 0;
    outkbegin = 0;
}

/*-----------------------------------------------------------------------------
 * Knotspec::copy -  copy the control points along minor direction 
 *
 * Client: Splinespec::copy
 *-----------------------------------------------------------------------------
 */

void
Knotspec::copy( INREAL *inpt, REAL *outpt )
{
    inpt = (INREAL *) (((char *) inpt) + preoffset);

    if( next ) {
        for( REAL *lpt=outpt+prewidth; outpt != lpt; outpt += poststride ) {
	    next->copy( inpt, outpt );
	    inpt = (INREAL *) (((char *) inpt) + prestride);
	}
   } else {
        for( REAL *lpt=outpt+prewidth; outpt != lpt; outpt += poststride ) {
	    pt_io_copy( outpt, inpt );
	    inpt = (INREAL *) (((char *) inpt) + prestride);
	}
     }
}

/*-----------------------------------------------------------------------------
 * Knotspec::showpts - print out points before transformation
 *
 * Client: Knotspec::select
 *-----------------------------------------------------------------------------
 */
void
Knotspec::showpts( REAL *outpt )
{
    if( next ) {
        for( REAL *lpt=outpt+prewidth; outpt != lpt; outpt += poststride )
	    next->showpts( outpt );
    } else {
        for( REAL *lpt=outpt+prewidth; outpt != lpt; outpt += poststride )
	    _glu_dprintf(  "show %g %g %g\n", outpt[0], outpt[1], outpt[2] );
    }
}

/*-----------------------------------------------------------------------------
 * Knotspec::factors - precompute scale factors 	
 *	   - overwrites knot vector, actual new knot vector is NOT produced
 *
 * Client: Knotspec::select
 *-----------------------------------------------------------------------------
 */

void
Knotspec::factors( void )
{
    Knot *mid = (outkend - 1) - order + bend->multi;
    Knot_ptr fptr = sbegin;

    for( Breakpt *bpt = bend; bpt >= bbegin; bpt-- ) {
    	mid -= bpt->multi;		// last knot less than knot to insert
	int def = bpt->def - 1;		// number of knots to insert
	if( def <= 0 ) continue;
	Knot kv = bpt->value;		// knot to insert

	Knot *kf = (mid-def) + (order-1);
	for( Knot *kl = kf + def; kl != kf; kl-- ) {
	    Knot *kh, *kt;
	    for( kt=kl, kh=mid; kt != kf; kh--, kt-- ) 
		*(fptr++) = (kv - *kh) / (*kt - *kh);
	    *kl = kv;
	}
    }
}

/*-----------------------------------------------------------------------------
 * Knotspec::insert - convert subobject in direction of kspec into Bezier
 *
 * Client: Knotspec::transform
 *-----------------------------------------------------------------------------
 */

void
Knotspec::insert( REAL *p )
{
    Knot_ptr fptr = sbegin;
    REAL *srcpt = p + prewidth - poststride;
    REAL *dstpt = p + postwidth + postoffset - poststride;
    Breakpt *bpt = bend;

   for( REAL *pend = srcpt - poststride*bpt->def; srcpt != pend; pend +=poststride ) {
	REAL *p1 = srcpt;
	for( REAL *p2 = srcpt-poststride; p2 != pend; p1 = p2, p2 -= poststride ) {
	    pt_oo_sum( p1, p1, p2, *fptr, 1.0-*fptr );
	    fptr++;
	}
    }

    for( --bpt; bpt >= bbegin; bpt-- ) {

	for( int multi = bpt->multi; multi > 0; multi-- ) {
	    pt_oo_copy( dstpt, srcpt );
	    dstpt -= poststride;
	    srcpt -= poststride;	
	}
    
	for( REAL *pend = srcpt - poststride*bpt->def; srcpt != pend; pend +=poststride, dstpt-=poststride ) {
	    pt_oo_copy( dstpt, srcpt );
	    REAL *p1 = srcpt;

	    for( REAL *p2 = srcpt-poststride; p2 != pend; p1=p2, p2 -= poststride ) {
		pt_oo_sum( p1, p1, p2, *fptr, 1.0-*fptr );
		fptr++;
	    }
	}
    }
}

/*-----------------------------------------------------------------------------
 * Knotspec::preselect - initialize kspec for processing
 *
 * Client: Splinespec::select
 *-----------------------------------------------------------------------------
 */

void
Knotspec::preselect( void )
{
    Knot kval; 

    /* position klast after last knot of "last" breakpoint */
    for( klast = inkend - order, kval = *klast; klast != inkend; klast++ ) 
	if( ! identical( *klast, kval ) ) break;

    /* position kfirst after last knot of "first" breakpoint */
    for( kfirst = inkbegin+order-1, kval= *kfirst;  kfirst != inkend; kfirst++ )
	if( ! identical( *kfirst, kval ) ) break;

    /* compute multiplicity of first breakpoint */
    Knot_ptr k;
    for( k  = kfirst - 1; k >= inkbegin; k-- ) 
	if( ! identical( kval, *k ) ) break;    
    k++;

    /* allocate space for breakpoints -
       use worst case estimate on number of breakpoints */

    bbegin = new Breakpt[(klast - kfirst)+1];
    /* record multiplicity and value of first breakpoint */
    bbegin->multi = kfirst - k;
    bbegin->value = kval;
    bend = bbegin;

    kleft = kright = kfirst;
}


/*-----------------------------------------------------------------------------
 * Knotspec::select - Knotspec::select segments and precompute scale factors
 *
 * Client: Splinespec::select
 *-----------------------------------------------------------------------------
 */

void
Knotspec::select( void )
{
    breakpoints();
    knots();
    factors();
    
    preoffset	= kleft - (inkbegin + order);
    postwidth	= (int)((bend - bbegin) * order);
    prewidth 	= (int)((outkend - outkbegin) - order);
    postoffset  = (bbegin->def > 1) ? (bbegin->def-1) : 0;
}
 
/*-----------------------------------------------------------------------------
 * Knotspec::breakpoints - compute breakpoints for knotspec
 *
 * Client: Knotspec::select
 *-----------------------------------------------------------------------------
 */

void
Knotspec::breakpoints( void )
{
    Breakpt *ubpt	= bbegin;
    Breakpt *ubend	= bend;
    long    nfactors  	= 0;

    ubpt->value	= ubend->value;
    ubpt->multi	= ubend->multi;

    kleft = kright;

    for( ; kright != klast; kright++ ) {
        if ( identical(*kright,ubpt->value) ) {
	    (ubpt->multi)++;
	} else {
    	    ubpt->def = (int) (order - ubpt->multi);
    	    nfactors += (ubpt->def * (ubpt->def - 1)) / 2;
	    (++ubpt)->value = *kright;
	    ubpt->multi = 1;
	}
    }
    ubpt->def = (int) (order - ubpt->multi);
    nfactors += (ubpt->def * (ubpt->def - 1)) / 2;

    bend = ubpt;

    if( nfactors ) {	    
        sbegin = new Knot[nfactors];
    } else {
	sbegin = NULL;
    }
}


/*-----------------------------------------------------------------------------
 * Knotspec::knots - copy relevant subsequence of knots into temporary area
 *
 * Client: Knotspec::select
 *-----------------------------------------------------------------------------
 */

void
Knotspec::knots( void )
{
    Knot_ptr inkpt = kleft - order;
    Knot_ptr inkend = kright  + bend->def;

    /* allocate space for knots and factors */
    outkbegin = new Knot[inkend-inkpt];
    Knot_ptr outkpt;
    for( outkpt = outkbegin; inkpt != inkend; inkpt++, outkpt++ ) 
	*outkpt = *inkpt;

    outkend = outkpt;
}


/*-----------------------------------------------------------------------------
 * Knotspec::transform -	convert a spline along a given direction 
 *
 * Client: Splienspec::transform
 *-----------------------------------------------------------------------------
 */

void
Knotspec::transform( REAL *p )
{
   if( next ) {
	if( this == kspectotrans ) {
	    next->transform( p );
	} else {
	    if( istransformed ) {
		p += postoffset;
		for( REAL *pend = p + postwidth; p != pend; p += poststride )
		    next->transform( p );
	    } else {
		REAL *pend = p + prewidth;
		for( ; p != pend; p += poststride )
		    next->transform( p );
	    }
	}
   } else {
	if( this == kspectotrans ) {
	    insert( p );
	} else {
	    if( istransformed ) {
		p += postoffset;
		for( REAL *pend = p + postwidth; p != pend; p += poststride )
		    kspectotrans->insert( p );
	    } else {
		REAL *pend = p + prewidth;
		for( ; p != pend; p += poststride )
		    kspectotrans->insert( p );
	    }
	}
   }
}

/*-----------------------------------------------------------------------------
 * Knotspec::~Knotspec - free space alocated for knotspec
 *-----------------------------------------------------------------------------
 */

Knotspec::~Knotspec( void )
{
    if( bbegin ) delete[] bbegin;
    if( sbegin ) delete[] sbegin;
    if( outkbegin ) delete[] outkbegin;
}


/*-----------------------------------------------------------------------------
 * pt_io_copy - make internal copy of input cntrl pt. of x coords
 *-----------------------------------------------------------------------------
 */

void
Knotspec::pt_io_copy( REAL *topt, INREAL *frompt )
{
    switch( ncoords ) {
    case 4:
        topt[3] = (REAL) frompt[3];
    case 3:
        topt[2] = (REAL) frompt[2];
    case 2:
        topt[1] = (REAL) frompt[1];
    case 1:
        topt[0] = (REAL) frompt[0];
	break;
    default: {
	    for( int i = 0; i < ncoords; i++ )
		*topt++ = (REAL) *frompt++;
	}
    }
}

/*-----------------------------------------------------------------------------
 * pt_oo_copy - make internal copy of internal cntrl pt. of x coords
 *-----------------------------------------------------------------------------
 */

void
Knotspec::pt_oo_copy( REAL *topt, REAL *frompt )
{
    switch( ncoords ) {
    case 4:
        topt[3] = frompt[3];
    case 3:
        topt[2] = frompt[2];
    case 2:
        topt[1] = frompt[1];
    case 1:
        topt[0] = frompt[0];
	break;
    default:
	memcpy( topt, frompt, ncoords * sizeof( REAL ) );
    }
}

/*-----------------------------------------------------------------------------
 * pt_oo_sum - compute affine combination of internal cntrl pts
 *-----------------------------------------------------------------------------
 */

void
Knotspec::pt_oo_sum( REAL *x, REAL *y, REAL *z, Knot a, Knot b )
{
    switch( ncoords ) {
    case 4:
        x[3] = a * y[3]  +  b * z[3];
    case 3:
        x[2] = a * y[2]  +  b * z[2];
    case 2:
        x[1] = a * y[1]  +  b * z[1];
    case 1:
        x[0] = a * y[0]  +  b * z[0];
	break;
    default: {
          for( int i = 0; i < ncoords; i++ )
              *x++ = a * *y++   +   b * *z++;
    }
    }
}

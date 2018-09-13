#include "stdafx.h"
#include "imgutil.h"
#include "cdithtbl.h"

CDitherTable::CDitherTable() :
   m_nColors( 0 ),
   m_nRefCount( 0 ),
   m_pnDistanceBuffer( NULL )
{
}

CDitherTable::~CDitherTable()
{
}

BOOL CDitherTable::Match( ULONG nColors, const RGBQUAD* prgbColors )
{
   if( m_nColors != nColors )
   {
      return( FALSE );
   }

   if( memcmp( m_argbColors, prgbColors, m_nColors*sizeof( RGBQUAD ) ) != 0 )
   {
      return( FALSE );
   }

   return( TRUE );
}

HRESULT CDitherTable::SetColors( ULONG nColors, const RGBQUAD* prgbColors )
{
   HRESULT hResult;

   m_nColors = nColors;
   memcpy( m_argbColors, prgbColors, m_nColors*sizeof( RGBQUAD ) );

   hResult = BuildInverseMap();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}
/*
void CDitherTable::BuildInverseMap()
{
   ULONG r;
   ULONG g;
   ULONG b;
   ULONG iColor;
   ULONG iMapEntry;
   int nMinDistance;
   int nDistance;
   int nRedDistance;
   int nBlueDistance;
   int nGreenDistance;

   iMapEntry = 0;
   for( r = 0; r < 32; r++ )
   {
      for( g = 0; g < 32; g++ )
      {
         for( b = 0; b < 32; b++ )
         {
            nMinDistance = 1000000;
            for( iColor = 0; iColor < m_nColors; iColor++ )
            {
               nRedDistance = m_argbColors[iColor].rgbRed-((r<<3)+(r>>2));
               nGreenDistance = m_argbColors[iColor].rgbGreen-((g<<3)+(g>>2));
               nBlueDistance = m_argbColors[iColor].rgbBlue-((b<<3)+(b>>2));
               nDistance = (nRedDistance*nRedDistance)+(nGreenDistance*
                  nGreenDistance)+(nBlueDistance*nBlueDistance);
               if( nDistance < nMinDistance )
               {
                  nMinDistance = nDistance;
                  m_abInverseMap[iMapEntry] = BYTE( iColor );
               }
            }
            iMapEntry++;
         }
      }
   }
}
*/

HRESULT CDitherTable::BuildInverseMap()
{
   _ASSERTE( m_pnDistanceBuffer == NULL );

   m_pnDistanceBuffer = new ULONG[32768];
   if( m_pnDistanceBuffer == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   inv_cmap( m_nColors, m_argbColors, 5, m_pnDistanceBuffer, m_abInverseMap );

   delete m_pnDistanceBuffer;
   m_pnDistanceBuffer = NULL;

   return( S_OK );
}

/*****************************************************************
 * TAG( inv_cmap )
 *
 * Compute an inverse colormap efficiently.
 * Inputs:
 * 	colors:		Number of colors in the forward colormap.
 * 	colormap:	The forward colormap.
 * 	bits:		Number of quantization bits.  The inverse
 * 			colormap will have (2^bits)^3 entries.
 * 	dist_buf:	An array of (2^bits)^3 long integers to be
 * 			used as scratch space.
 * Outputs:
 * 	rgbmap:		The output inverse colormap.  The entry
 * 			rgbmap[(r<<(2*bits)) + (g<<bits) + b]
 * 			is the colormap entry that is closest to the
 * 			(quantized) color (r,g,b).
 * Assumptions:
 * 	Quantization is performed by right shift (low order bits are
 * 	truncated).  Thus, the distance to a quantized color is
 * 	actually measured to the color at the center of the cell
 * 	(i.e., to r+.5, g+.5, b+.5, if (r,g,b) is a quantized color).
 * Algorithm:
 * 	Uses a "distance buffer" algorithm:
 * 	The distance from each representative in the forward color map
 * 	to each point in the rgb space is computed.  If it is less
 * 	than the distance currently stored in dist_buf, then the
 * 	corresponding entry in rgbmap is replaced with the current
 * 	representative (and the dist_buf entry is replaced with the
 * 	new distance).
 *
 * 	The distance computation uses an efficient incremental formulation.
 *
 * 	Distances are computed "outward" from each color.  If the
 * 	colors are evenly distributed in color space, the expected
 * 	number of cells visited for color I is N^3/I.
 * 	Thus, the complexity of the algorithm is O(log(K) N^3),
 * 	where K = colors, and N = 2^bits.
 */

/*
 * Here's the idea:  scan from the "center" of each cell "out"
 * until we hit the "edge" of the cell -- that is, the point
 * at which some other color is closer -- and stop.  In 1-D,
 * this is simple:
 * 	for i := here to max do
 * 		if closer then buffer[i] = this color
 * 		else break
 * 	repeat above loop with i := here-1 to min by -1
 *
 * In 2-D, it's trickier, because along a "scan-line", the
 * region might start "after" the "center" point.  A picture
 * might clarify:
 *		 |    ...
 *               | ...	.
 *              ...    	.
 *           ... |      .
 *          .    +     	.
 *           .          .
 *            .         .
 *             .........
 *
 * The + marks the "center" of the above region.  On the top 2
 * lines, the region "begins" to the right of the "center".
 *
 * Thus, we need a loop like this:
 * 	detect := false
 * 	for i := here to max do
 * 		if closer then
 * 			buffer[..., i] := this color
 * 			if !detect then
 * 				here = i
 * 				detect = true
 * 		else
 * 			if detect then
 * 				break
 * 				
 * Repeat the above loop with i := here-1 to min by -1.  Note that
 * the "detect" value should not be reinitialized.  If it was
 * "true", and center is not inside the cell, then none of the
 * cell lies to the left and this loop should exit
 * immediately.
 *
 * The outer loops are similar, except that the "closer" test
 * is replaced by a call to the "next in" loop; its "detect"
 * value serves as the test.  (No assignment to the buffer is
 * done, either.)
 *
 * Each time an outer loop starts, the "here", "min", and
 * "max" values of the next inner loop should be
 * re-initialized to the center of the cell, 0, and cube size,
 * respectively.  Otherwise, these values will carry over from
 * one "call" to the inner loop to the next.  This tracks the
 * edges of the cell and minimizes the number of
 * "unproductive" comparisons that must be made.
 *
 * Finally, the inner-most loop can have the "if !detect"
 * optimized out of it by splitting it into two loops: one
 * that finds the first color value on the scan line that is
 * in this cell, and a second that fills the cell until
 * another one is closer:
 *  	if !detect then	    {needed for "down" loop}
 * 	    for i := here to max do
 * 		if closer then
 * 			buffer[..., i] := this color
 * 			detect := true
 * 			break
 *	for i := i+1 to max do
 *		if closer then
 * 			buffer[..., i] := this color
 * 		else
 * 			break
 *
 * In this implementation, each level will require the
 * following variables.  Variables labelled (l) are local to each
 * procedure.  The ? should be replaced with r, g, or b:
 *  	cdist:	    	The distance at the starting point.
 * 	?center:	The value of this component of the color
 *  	c?inc:	    	The initial increment at the ?center position.
 * 	?stride:	The amount to add to the buffer
 * 			pointers (dp and rgbp) to get to the
 * 			"next row".
 * 	min(l):		The "low edge" of the cell, init to 0
 * 	max(l):		The "high edge" of the cell, init to
 * 			colormax-1
 * 	detect(l):    	True if this row has changed some
 * 	    	    	buffer entries.
 *  	i(l): 	    	The index for this row.
 *  	?xx:	    	The accumulated increment value.
 *  	
 *  	here(l):    	The starting index for this color.  The
 *  	    	    	following variables are associated with here,
 *  	    	    	in the sense that they must be updated if here
 *  	    	    	is changed.
 *  	?dist:	    	The current distance for this level.  The
 *  	    	    	value of dist from the previous level (g or r,
 *  	    	    	for level b or g) initializes dist on this
 *  	    	    	level.  Thus gdist is associated with here(b)).
 *  	?inc:	    	The initial increment for the row.
 *
 *  	?dp:	    	Pointer into the distance buffer.  The value
 *  	    	    	from the previous level initializes this level.
 *  	?rgbp:	    	Pointer into the rgb buffer.  The value
 *  	    	    	from the previous level initializes this level.
 * 
 * The blue and green levels modify 'here-associated' variables (dp,
 * rgbp, dist) on the green and red levels, respectively, when here is
 * changed.
 */

/* Track minimum and maximum. */
#define MINMAX_TRACK

void CDitherTable::inv_cmap(int colors, RGBQUAD *colormap, int bits,
        ULONG* dist_buf, BYTE* rgbmap )
{
    int nbits = 8 - bits;

    colormax = 1 << bits;
    x = 1 << nbits;
    xsqr = 1 << (2 * nbits);

    /* Compute "strides" for accessing the arrays. */
    gstride = (int) colormax;
    rstride = (int) (colormax * colormax);

    maxfill( dist_buf, colormax );

    for ( cindex = 0; cindex < colors; cindex++ )
    {
        /* The caller can force certain colors in the output space to be
         * omitted by setting a nonzero value for the color's 'x' component.
         * This will produce a map that never refers to those colors.
         * -francish, 2/16/96
         */
        if (!colormap[cindex].rgbReserved)
        {
            /*
             * Distance formula is
             * (red - map[0])^2 + (green - map[1])^2 + (blue - map[2])^2
             *
             * Because of quantization, we will measure from the center of
             * each quantized "cube", so blue distance is
             * 	(blue + x/2 - map[2])^2,
             * where x = 2^(8 - bits).
             * The step size is x, so the blue increment is
             * 	2*x*blue - 2*x*map[2] + 2*x^2
             *
             * Now, b in the code below is actually blue/x, so our
             * increment will be 2*(b*x^2 + x^2 - x*map[2]).  For
             * efficiency, we will maintain this quantity in a separate variable
             * that will be updated incrementally by adding 2*x^2 each time.
             */
            /* The initial position is the cell containing the colormap
             * entry.  We get this by quantizing the colormap values.
             */
            rcenter = colormap[cindex].rgbRed >> nbits;
            gcenter = colormap[cindex].rgbGreen >> nbits;
            bcenter = colormap[cindex].rgbBlue >> nbits;
    
            rdist = colormap[cindex].rgbRed - (rcenter * x + x/2);
            gdist = colormap[cindex].rgbGreen - (gcenter * x + x/2);
            cdist = colormap[cindex].rgbBlue - (bcenter * x + x/2);
            cdist = rdist*rdist + gdist*gdist + cdist*cdist;
    
            crinc = 2 * ((rcenter + 1) * xsqr - (colormap[cindex].rgbRed*x));
            cginc = 2 * ((gcenter + 1) * xsqr - (colormap[cindex].rgbGreen*x));
            cbinc = 2 * ((bcenter + 1) * xsqr - (colormap[cindex].rgbBlue*x));
    
            /* Array starting points. */
            cdp = dist_buf + rcenter * rstride + gcenter * gstride + bcenter;
            crgbp = rgbmap + rcenter * rstride + gcenter * gstride + bcenter;
    
            (void)redloop();
        }
    }
}

/* redloop -- loop up and down from red center. */
int CDitherTable::redloop()
{
    int detect;
    int r;
    int first;
    long txsqr = xsqr + xsqr;

    detect = 0;

    /* Basic loop up. */
    for ( r = rcenter, rdist = cdist, rxx = crinc,
	  rdp = cdp, rrgbp = crgbp, first = 1;
	  r < (int) colormax;
	  r++, rdp += rstride, rrgbp += rstride,
	  rdist += rxx, rxx += txsqr, first = 0 )
    {
	if ( greenloop( first ) )
	    detect = 1;
	else if ( detect )
	    break;
    }
    
    /* Basic loop down. */
    for ( r = rcenter - 1, rxx = crinc - txsqr, rdist = cdist - rxx,
	  rdp = cdp - rstride, rrgbp = crgbp - rstride, first = 1;
	  r >= 0;
	  r--, rdp -= rstride, rrgbp -= rstride,
	  rxx -= txsqr, rdist -= rxx, first = 0 )
    {
	if ( greenloop( first ) )
	    detect = 1;
	else if ( detect )
	    break;
    }
    
    return detect;
}

#undef min
#undef max
#define here greenloop_here
#define min greenloop_min
#define max greenloop_max
#define prevmin greenloop_prevmin
#define prevmax greenloop_prevmax

/* greenloop -- loop up and down from green center. */
int CDitherTable::greenloop( int restart )
{
    int detect;
    int g;
    int first;
    long txsqr = xsqr + xsqr;
#ifdef MINMAX_TRACK
    int thismax, thismin;
#endif

    if ( restart )
    {
	here = gcenter;
	min = 0;
	max = (int) colormax - 1;
	ginc = cginc;
#ifdef MINMAX_TRACK
	prevmax = 0;
	prevmin = (int) colormax;
#endif
    }

#ifdef MINMAX_TRACK
    thismin = min;
    thismax = max;
#endif
    detect = 0;

    /* Basic loop up. */
    for ( g = here, gcdist = gdist = rdist, gxx = ginc,
	  gcdp = gdp = rdp, gcrgbp = grgbp = rrgbp, first = 1;
	  g <= max;
	  g++, gdp += gstride, gcdp += gstride, grgbp += gstride, gcrgbp += gstride,
	  gdist += gxx, gcdist += gxx, gxx += txsqr, first = 0 )
    {
	if ( blueloop( first ) )
	{
	    if ( !detect )
	    {
		/* Remember here and associated data! */
		if ( g > here )
		{
		    here = g;
		    rdp = gcdp;
		    rrgbp = gcrgbp;
		    rdist = gcdist;
		    ginc = gxx;
#ifdef MINMAX_TRACK
		    thismin = here;
#endif
		}
		detect = 1;
	    }
	}
	else if ( detect )
	{
#ifdef MINMAX_TRACK
	    thismax = g - 1;
#endif
	    break;
	}
    }
    
    /* Basic loop down. */
    for ( g = here - 1, gxx = ginc - txsqr, gcdist = gdist = rdist - gxx,
	  gcdp = gdp = rdp - gstride, gcrgbp = grgbp = rrgbp - gstride,
	  first = 1;
	  g >= min;
	  g--, gdp -= gstride, gcdp -= gstride, grgbp -= gstride, gcrgbp -= gstride,
	  gxx -= txsqr, gdist -= gxx, gcdist -= gxx, first = 0 )
    {
	if ( blueloop( first ) )
	{
	    if ( !detect )
	    {
		/* Remember here! */
		here = g;
		rdp = gcdp;
		rrgbp = gcrgbp;
		rdist = gcdist;
		ginc = gxx;
#ifdef MINMAX_TRACK
		thismax = here;
#endif
		detect = 1;
	    }
	}
	else if ( detect )
	{
#ifdef MINMAX_TRACK
	    thismin = g + 1;
#endif
	    break;
	}
    }
    
#ifdef MINMAX_TRACK
    /* If we saw something, update the edge trackers.  For now, only
     * tracks edges that are "shrinking" (min increasing, max
     * decreasing.
     */
    if ( detect )
    {
	if ( thismax < prevmax )
	    max = thismax;

	prevmax = thismax;

	if ( thismin > prevmin )
	    min = thismin;

	prevmin = thismin;
    }
#endif

    return detect;
}

#undef min
#undef max
#undef here
#undef prevmin
#undef prevmax
#define here blueloop_here
#define min blueloop_min
#define max blueloop_max
#define prevmin blueloop_prevmin
#define prevmax blueloop_prevmax

/* blueloop -- loop up and down from blue center. */
int CDitherTable::blueloop( int restart )
{
    int detect;
    register ULONG* dp;
    register BYTE* rgbp;
    register long bdist, bxx;
    register int b, i = cindex;
    register long txsqr = xsqr + xsqr;
    register int lim;
#ifdef MINMAX_TRACK
    int thismin, thismax;
#endif /* MINMAX_TRACK */

    if ( restart )
    {
	here = bcenter;
	min = 0;
	max = (int) colormax - 1;
	binc = cbinc;
#ifdef MINMAX_TRACK
	prevmin = (int) colormax;
	prevmax = 0;
#endif /* MINMAX_TRACK */
    }

    detect = 0;
#ifdef MINMAX_TRACK
    thismin = min;
    thismax = max;
#endif

    /* Basic loop up. */
    /* First loop just finds first applicable cell. */
    for ( b = here, bdist = gdist, bxx = binc, dp = gdp, rgbp = grgbp, lim = max;
	  b <= lim;
	  b++, dp++, rgbp++,
	  bdist += bxx, bxx += txsqr )
    {
        if ( *dp > (DWORD)bdist )
	{
	    /* Remember new 'here' and associated data! */
	    if ( b > here )
	    {
		here = b;
		gdp = dp;
		grgbp = rgbp;
		gdist = bdist;
		binc = bxx;
#ifdef MINMAX_TRACK
		thismin = here;
#endif
	    }
	    detect = 1;
	    break;
	}
    }
    /* Second loop fills in a run of closer cells. */
    for ( ;
	  b <= lim;
	  b++, dp++, rgbp++,
	  bdist += bxx, bxx += txsqr )
    {
        if ( *dp > (DWORD)bdist )
	{
	    *dp = bdist;
	    *rgbp = (BYTE) i;
	}
	else
	{
#ifdef MINMAX_TRACK
	    thismax = b - 1;
#endif
	    break;
	}
    }
    
    /* Basic loop down. */
    /* Do initializations here, since the 'find' loop might not get
     * executed. 
     */
    lim = min;
    b = here - 1;
    bxx = binc - txsqr;
    bdist = gdist - bxx;
    dp = gdp - 1;
    rgbp = grgbp - 1;
    /* The 'find' loop is executed only if we didn't already find
     * something.
     */
    if ( !detect )
	for ( ;
	      b >= lim;
	      b--, dp--, rgbp--,
	      bxx -= txsqr, bdist -= bxx )
	{
            if ( *dp > (DWORD)bdist )
	    {
		/* Remember here! */
		/* No test for b against here necessary because b <
		 * here by definition.
		 */
		here = b;
		gdp = dp;
		grgbp = rgbp;
		gdist = bdist;
		binc = bxx;
#ifdef MINMAX_TRACK
		thismax = here;
#endif
		detect = 1;
		break;
	    }
	}
    /* The 'update' loop. */
    for ( ;
	  b >= lim;
	  b--, dp--, rgbp--,
	  bxx -= txsqr, bdist -= bxx )
    {
        if ( *dp > (DWORD)bdist )
	{
	    *dp = bdist;
	    *rgbp = (BYTE) i;
	}
	else
	{
#ifdef MINMAX_TRACK
	    thismin = b + 1;
#endif
	    break;
	}
    }


	/* If we saw something, update the edge trackers. */
#ifdef MINMAX_TRACK
    if ( detect )
    {
	/* Only tracks edges that are "shrinking" (min increasing, max
	 * decreasing.
	 */
	if ( thismax < prevmax )
	    max = thismax;

	if ( thismin > prevmin )
	    min = thismin;
    
	/* Remember the min and max values. */
	prevmax = thismax;
	prevmin = thismin;
    }
#endif /* MINMAX_TRACK */

    return detect;
}

void CDitherTable::maxfill( ULONG* buffer, long side)
{
    register unsigned long maxv = (unsigned long)~0L;
    register long i;
    register ULONG* bp;

    (void)side;

    for ( i = colormax * colormax * colormax, bp = buffer;
	  i > 0;
	  i--, bp++ )
	*bp = maxv;
}

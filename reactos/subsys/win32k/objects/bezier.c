#include <windows.h>
#include <ddk/ntddk.h>

/******************************************************************
 * 
 *   *Very* simple bezier drawing code, 
 *
 *   It uses a recursive algorithm to divide the curve in a series
 *   of straight line segements. Not ideal but for me sufficient.
 *   If you are in need for something better look for some incremental
 *   algorithm.
 *
 *   7 July 1998 Rein Klazes
 */

 /* 
  * some macro definitions for bezier drawing
  *
  * to avoid trucation errors the coordinates are
  * shifted upwards. When used in drawing they are
  * shifted down again, including correct rounding
  * and avoiding floating point arithmatic
  * 4 bits should allow 27 bits coordinates which I saw
  * somewere in the win32 doc's
  * 
  */

#define BEZIERSHIFTBITS 4
#define BEZIERSHIFTUP(x)    ((x)<<BEZIERSHIFTBITS)
#define BEZIERPIXEL        BEZIERSHIFTUP(1)    
#define BEZIERSHIFTDOWN(x)  (((x)+(1<<(BEZIERSHIFTBITS-1)))>>BEZIERSHIFTBITS)
/* maximum depth of recursion */
#define BEZIERMAXDEPTH  8

/* size of array to store points on */
/* enough for one curve */
#define BEZIER_INITBUFSIZE    (150)

/* calculate Bezier average, in this case the middle 
 * correctly rounded...
 * */

#define BEZIERMIDDLE(Mid, P1, P2) \
  (Mid).x=((P1).x+(P2).x + 1)/2;\
  (Mid).y=((P1).y+(P2).y + 1)/2;
    
/**********************************************************
* BezierCheck helper function to check
* that recursion can be terminated
*       Points[0] and Points[3] are begin and endpoint
*       Points[1] and Points[2] are control points
*       level is the recursion depth
*       returns true if the recusion can be terminated
*/
static BOOL BezierCheck( int level, POINT *Points)
{ 
  INT dx, dy;

  dx=Points[3].x-Points[0].x;
  dy=Points[3].y-Points[0].y;
  if(abs(dy)<=abs(dx)) {/* shallow line */
    /* check that control points are between begin and end */
    if(Points[1].x < Points[0].x){
      if(Points[1].x < Points[3].x) return FALSE;
    }else
    if(Points[1].x > Points[3].x) return FALSE;
    if(Points[2].x < Points[0].x) {
      if(Points[2].x < Points[3].x) return FALSE;
    } else
      if(Points[2].x > Points[3].x) return FALSE;
      dx=BEZIERSHIFTDOWN(dx);
      if(!dx) return TRUE;
      if(abs(Points[1].y-Points[0].y-(dy/dx)*
        BEZIERSHIFTDOWN(Points[1].x-Points[0].x)) > BEZIERPIXEL ||
        abs(Points[2].y-Points[0].y-(dy/dx)*
        BEZIERSHIFTDOWN(Points[2].x-Points[0].x)) > BEZIERPIXEL) return FALSE;
      else
        return TRUE;
    } else{ /* steep line */
      /* check that control points are between begin and end */
      if(Points[1].y < Points[0].y){
        if(Points[1].y < Points[3].y) return FALSE;
      } else
        if(Points[1].y > Points[3].y) return FALSE;
      if(Points[2].y < Points[0].y){
        if(Points[2].y < Points[3].y) return FALSE;
      } else
        if(Points[2].y > Points[3].y) return FALSE;
      dy=BEZIERSHIFTDOWN(dy);
      if(!dy) return TRUE;
      if(abs(Points[1].x-Points[0].x-(dx/dy)*
        BEZIERSHIFTDOWN(Points[1].y-Points[0].y)) > BEZIERPIXEL ||
        abs(Points[2].x-Points[0].x-(dx/dy)*
        BEZIERSHIFTDOWN(Points[2].y-Points[0].y)) > BEZIERPIXEL ) return FALSE;
      else
        return TRUE;
    }
}
    
/* Helper for GDI_Bezier.
 * Just handles one Bezier, so Points should point to four POINTs
 */
static void GDI_InternalBezier( POINT *Points, POINT **PtsOut, INT *dwOut,
				INT *nPtsOut, INT level )
{
  if(*nPtsOut == *dwOut) {
    *dwOut *= 2;
    *PtsOut = ExAllocatePool(NonPagedPool, *dwOut * sizeof(POINT));
  }

  if(!level || BezierCheck(level, Points)) {
    if(*nPtsOut == 0) {
      (*PtsOut)[0].x = BEZIERSHIFTDOWN(Points[0].x);
      (*PtsOut)[0].y = BEZIERSHIFTDOWN(Points[0].y);
       *nPtsOut = 1;
    }
    (*PtsOut)[*nPtsOut].x = BEZIERSHIFTDOWN(Points[3].x);
    (*PtsOut)[*nPtsOut].y = BEZIERSHIFTDOWN(Points[3].y);
    (*nPtsOut) ++;
  } else {
    POINT Points2[4]; /* for the second recursive call */
    Points2[3]=Points[3];
    BEZIERMIDDLE(Points2[2], Points[2], Points[3]);
    BEZIERMIDDLE(Points2[0], Points[1], Points[2]);
    BEZIERMIDDLE(Points2[1],Points2[0],Points2[2]);

    BEZIERMIDDLE(Points[1], Points[0],  Points[1]);
    BEZIERMIDDLE(Points[2], Points[1], Points2[0]);
    BEZIERMIDDLE(Points[3], Points[2], Points2[1]);

    Points2[0]=Points[3];

    /* do the two halves */
    GDI_InternalBezier(Points, PtsOut, dwOut, nPtsOut, level-1);
    GDI_InternalBezier(Points2, PtsOut, dwOut, nPtsOut, level-1);
  }
}

/***********************************************************************
 *           GDI_Bezier   [INTERNAL]
 *   Calculate line segments that approximate -what microsoft calls- a bezier
 *   curve.
 *   The routine recursively divides the curve in two parts until a straight
 *   line can be drawn
 *
 *  PARAMS
 *
 *  Points  [I] Ptr to count POINTs which are the end and control points
 *              of the set of Bezier curves to flatten.
 *  count   [I] Number of Points.  Must be 3n+1.
 *  nPtsOut [O] Will contain no of points that have been produced (i.e. no. of
 *              lines+1).
 *   
 *  RETURNS
 *
 *  Ptr to an array of POINTs that contain the lines that approximinate the
 *  Beziers.  The array is allocated on the process heap and it is the caller's
 *  responsibility to HeapFree it. [this is not a particularly nice interface
 *  but since we can't know in advance how many points will generate, the
 *  alternative would be to call the function twice, once to determine the size
 *  and a second time to do the work - I decided this was too much of a pain].
 */
POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut )
{
  POINT *out;
  INT Bezier, dwOut = BEZIER_INITBUFSIZE, i;

  if((count - 1) % 3 != 0) {
    return NULL;
  }
  *nPtsOut = 0;
  out = ExAllocatePool(NonPagedPool, dwOut * sizeof(POINT));
  for(Bezier = 0; Bezier < (count-1)/3; Bezier++) {
    POINT ptBuf[4];
    memcpy(ptBuf, Points + Bezier * 3, sizeof(POINT) * 4);
    for(i = 0; i < 4; i++) {
      ptBuf[i].x = BEZIERSHIFTUP(ptBuf[i].x);
      ptBuf[i].y = BEZIERSHIFTUP(ptBuf[i].y);
    }
    GDI_InternalBezier( ptBuf, &out, &dwOut, nPtsOut, BEZIERMAXDEPTH );
  }

  return out;
}

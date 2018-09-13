#ifndef LSTFSET_DEFINED
#define LSTFSET_DEFINED

/* Service routines for some standard text flow change tasks */

#include "lsdefs.h"
#include "lstflow.h"

/* 
 * LsPointXYFromPointUV calculates pointxyOut given (x,y) pointxyIn and (u,v) vector 
 *  
 *  (pointxyOut = pointxyIn + vectoruv)
 */

LSERR WINAPI LsPointXYFromPointUV(const POINT*, 	/* IN: input point (x,y) */
									LSTFLOW,	 	/* IN: text flow for */
									PCPOINTUV,		/* IN: vector in (u,v) */
									POINT*);		/* OUT: (x,y) point */


/* 
 * LsPointUV1FromPointUV2 calculates vector in uv2 coordinates given begin and end of it in uv1.
 *  
 *  (vectorUV22 = pointUV1b - pointUV1a)
 *
 *	Usually pointUV1a is the starting point of uv2 coordinate system and it is easier to think 
 *		about output vector as a point in it.
 */

LSERR WINAPI LsPointUV2FromPointUV1(LSTFLOW,	 	/* IN: text flow 1 (TF1) */
									PCPOINTUV,	 	/* IN: starting point (TF1) */
									PCPOINTUV,		/* IN: ending point (TF1) */
									LSTFLOW,	 	/* IN: text flow 2 (TF2) */
									PPOINTUV);		/* OUT: vector in TF2 */


#endif /* !LSTFSET_DEFINED */


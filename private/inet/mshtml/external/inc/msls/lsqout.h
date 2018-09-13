#ifndef LSQOUT_DEFINED
#define LSQOUT_DEFINED

#include "lsdefs.h"
#include "heights.h"
#include "lscell.h"
#include "plssubl.h"
#include "plsqout.h"


typedef struct lsqout			
{
 	POINTUV pointUvStartObj;		/* In coordinate system of parent subline
										relative to the beginning of dnode	*/
	HEIGHTS	heightsPresObj;			/* In direction of parent subline */
	long dupObj;					/* In direction lstflowSubline			*/

	LSTEXTCELL lstextcell;			/* in coordinate system of parent subline,
										relative to the beginning of dnode	*/

	PLSSUBL plssubl;
 	POINTUV pointUvStartSubline;	/* In coordinate system of parent subline
										relative to the beginning of dnode	*/

} LSQOUT;


#endif 

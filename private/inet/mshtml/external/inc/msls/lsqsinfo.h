#ifndef LSQSINFO_DEFINED
#define LSQSINFO_DEFINED

#include "lsdefs.h"
#include "heights.h"
#include "lstflow.h"
#include "plsrun.h"
#include "plsqsinf.h"


typedef struct lsqsubinfo			
{
	LSTFLOW	lstflowSubline;
	LSCP cpFirstSubline;
	LSDCP dcpSubline;
 	POINTUV pointUvStartSubline;	/* In coordinate system of main line/subline */
	HEIGHTS	heightsPresSubline;		/* In direction lstflowSubline */
	long dupSubline;				/* In direction lstflowSubline			*/


	DWORD idobj;
	PLSRUN plsrun;
	LSCP cpFirstRun;
	LSDCP dcpRun;
 	POINTUV pointUvStartRun;		/* In coordinate system of main line/subline */
	HEIGHTS	heightsPresRun;			/* In direction lstflowSubline */
	long dupRun;					/* In direction lstflowSubline			*/
	long dvpPosRun;					/* in direction of lstflowSubline	*/

	long dupBorderBefore;			/* in direction of lstflowSubline	*/
	long dupBorderAfter;			/* in direction of lstflowSubline	*/

 	POINTUV pointUvStartObj;		/* Set by Object, translated to coord system of main line/subline */
	HEIGHTS	heightsPresObj;			/* Set by Object, in direction lstflowSubline */
	long dupObj;					/* Set by Object, in direction lstflowSubline			*/


} LSQSUBINFO;

#endif 

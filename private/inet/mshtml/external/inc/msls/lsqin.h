#ifndef LSQIN_DEFINED
#define LSQIN_DEFINED

#include "lsdefs.h"
#include "heights.h"
#include "lstflow.h"
#include "plsrun.h"
#include "plsqin.h"

typedef struct lsqin			
{
	LSTFLOW	lstflowSubline;
	PLSRUN plsrun;					/* PLSRUN this cp belongs to */
	LSCP cpFirstRun;
	LSDCP dcpRun;
	HEIGHTS	heightsPresRun;			/* In direction lstflowSubline */
	long dupRun;					/* In direction lstflowSubline			*/
	long dvpPosRun;					/* in direction of lstflowSubline	*/

} LSQIN;


#endif 

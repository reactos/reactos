#ifndef LSSUBSET_DEFINED
#define LSSUBSET_DEFINED

/* Access routines for contents of DNODES */

#include "lsdefs.h"
#include "plsrun.h"
#include "plssubl.h"
#include "pobjdim.h"
#include "lstflow.h"

			
LSERR WINAPI LssbGetObjDimSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							LSTFLOW*,			/* OUT: subline's lstflow		*/
					 	    POBJDIM);			/* OUT: dimensions of subline	*/
							
LSERR WINAPI LssbGetDupSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							LSTFLOW*,			/* OUT: subline's lstflow		*/
					 	    long*);				/* OUT: dup of subline			*/

LSERR WINAPI LssbFDonePresSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							BOOL*);				/* OUT: Is it CalcPres'd		*/

LSERR WINAPI LssbFDoneDisplay(
							PLSSUBL,			/* IN: Subline Context			*/
							BOOL*);				/* OUT: Is it displayed			*/

LSERR WINAPI LssbGetPlsrunsFromSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							DWORD,				/* IN: N of DNODES in subline	*/
							PLSRUN*);			/* OUT: array of PLSRUN's		*/

LSERR WINAPI LssbGetNumberDnodesInSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							DWORD*);			/* OUT: N of DNODES in subline	*/

LSERR WINAPI LssbGetVisibleDcpInSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							LSDCP*);			/* OUT: N of characters			*/

LSERR WINAPI LssbGetDurTrailInSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							long*);				/* OUT: width of trailing area	*/
	
LSERR WINAPI LssbGetDurTrailWithPensInSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							long*);				/* OUT: width of trailing area
													including pens in subline	*/
LSERR WINAPI LssbFIsSublineEmpty(
							PLSSUBL plssubl,	/* IN: subline					*/
							BOOL*  pfEmpty);	/* OUT:is this subline empty	*/


#endif /* !LSSUBSET_DEFINED */



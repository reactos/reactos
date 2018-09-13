#ifndef LSQSUBL_DEFINED
#define LSQSUBL_DEFINED

/* Line services formatter fetch/dispatcher interface (to LsCreateLine())
 */

#include "lsdefs.h"
#include "plssubl.h"
#include "plsqsinf.h"
#include "plscell.h"

LSERR WINAPI LsQueryCpPpointSubline(
							PLSSUBL,		/* IN: subline context			*/
							LSCP, 			/* IN: cpQuery 					*/
							DWORD,      	/* IN: nDepthQueryMax			*/
							PLSQSUBINFO,	/* OUT: array[nDepthQueryMax] of LSQSUBINFO	*/
							DWORD*,			/* OUT: nActualDepth			*/
							PLSTEXTCELL);	/* OUT: Text cell info			*/
							
LSERR WINAPI LsQueryPointPcpSubline(
							PLSSUBL,		/* IN: subline context			*/
						 	PCPOINTUV,		/* IN: query point from the subline beginning */
							DWORD,      	/* IN: nDepthQueryMax			*/
							PLSQSUBINFO,	/* OUT: array[nDepthQueryMax] of LSQSUBINFO */
							DWORD*,		 	/* OUT: nActualDepth			*/
							PLSTEXTCELL);	/* OUT: Text cell info			*/


#endif /* !LSQSUBL_DEFINED */


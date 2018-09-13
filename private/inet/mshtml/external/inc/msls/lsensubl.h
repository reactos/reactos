#ifndef LSENSUBL_DEFINED
#define LSENSUBL_DEFINED

/* Line services formatter fetch/dispatcher interface (to LsCreateLine())
 */

#include "lsdefs.h"
#include "plssubl.h"


LSERR WINAPI LsEnumSubline(PLSSUBL,
						   BOOL,			/* IN: enumerate in reverse order?					*/
						   BOOL,			/* IN: geometry needed?								*/
						   const POINT*);	/* IN: starting position(xp, yp) iff fGeometryNeeded*/
#endif /* !LSENSUBL_DEFINED */


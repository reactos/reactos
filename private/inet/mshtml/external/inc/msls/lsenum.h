#ifndef LSENUM_DEFINED
#define LSENUM_DEFINED

#include "lsdefs.h"
#include "plsline.h"

LSERR WINAPI LsEnumLine(PLSLINE,
					   	BOOL,			/* IN: enumerate in reverse order?					*/
						BOOL,			/* IN: geometry needed?								*/
						const POINT*);	/* IN: starting position(xp, yp) iff fGeometryNeeded*/

#endif /* LSENUM_DEFINED */

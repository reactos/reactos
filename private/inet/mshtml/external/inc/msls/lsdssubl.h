#ifndef LSDSSUBL_DEFINED
#define LSDSSUBL_DEFINED

/* Line services formatter fetch/dispatcher interface (to LsCreateLine())
 */

#include "lsdefs.h"
#include "plssubl.h"


LSERR WINAPI LsDisplaySubline(
							PLSSUBL,			/* IN: subline context			*/
							const POINT*,		/* IN: starting position(xp, yp)*/
							UINT,				/* IN: display mode, opaque, etc */
							const RECT*);		/* IN: clip rectangle (xp, yp,...) */

#endif /* !LSDSSUBL_DEFINED */


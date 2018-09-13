#ifndef DISPI_DEFINED
#define DISPI_DEFINED

#include "lsdefs.h"
#include "pdispi.h"
#include "plsrun.h"
#include "plschp.h"
#include "heights.h"
#include "lstflow.h"

typedef struct dispin
{
	POINT 	ptPen;					/* starting pen position (x,y) */
	PCLSCHP plschp;					/* CHP for this display object */
	PLSRUN 	plsrun;					/* client pointer to run */

	UINT 	kDispMode;				/* display mode, opaque, etc */
	LSTFLOW lstflow;	 			/* text direction and orientation */
	RECT* 	prcClip;				/* clip rectangle (x,y) */

	BOOL 	fDrawUnderline;			/* Draw underline while displaying */
	BOOL 	fDrawStrikethrough;		/* Draw strikethrough while Displaying */

	HEIGHTS heightsPres;
	long 	dup;
	long	dupLimUnderline;		/* less than dup if trailing spaces */
} DISPIN;	

#endif /* !DISPI_DEFINED */

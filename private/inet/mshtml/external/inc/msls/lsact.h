#ifndef LSACT_DEFINED
#define LSACT_DEFINED

#include "lsdefs.h"
#include "kamount.h"

#define sideNone				0		/* means no action				*/
#define sideLeft				1
#define sideRight				2
#define sideLeftRight			3		/* Review(segeyge): how to distribute?*/

typedef struct lsact					/* action							*/
{
	BYTE side;							/* side of action (left/right/both)	*/
	KAMOUNT kamnt;						/* amount of action					*/
} LSACT;


#endif /* !LSACT_DEFINED                         */


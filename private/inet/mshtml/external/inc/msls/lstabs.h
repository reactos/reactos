#ifndef LSTABS_DEFINED
#define LSTABS_DEFINED

#include "lsdefs.h"
#include "lsktab.h"

/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */

typedef struct
{
	enum lsktab lskt;					/* Kind of tab */
	long ua;							/* tab position */
	WCHAR wchTabLeader;					/* character for tab leader */
										/*   if 0, no leader is used*/
	WCHAR wchPad;
} LSTBD;

/* ---------------------------------------------------------------------- */

typedef struct lstabs
{
	long duaIncrementalTab;				/* "Default" tab behavior */
	DWORD iTabUserDefMac;
	LSTBD* pTab;				
} LSTABS;


#endif /* !LSTABS_DEFINED */

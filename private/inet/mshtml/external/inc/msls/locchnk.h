#ifndef LOCCHNK_DEFINED
#define LOCCHNK_DEFINED

#include "lsdefs.h"
#include "lsfgi.h"
#include "lschnke.h"

typedef struct locchnk					/* located chnk					*/
{
	LSFGI lsfgi;						/* location of chunk			*/
	DWORD clschnk;						/* number of dobj's in chunk	*/
	PLSCHNK plschnk;					/* chunk 						*/
	PPOINTUV ppointUvLoc;				/* location of each chunk's node*/
} LOCCHNK;



#endif /* !LOCCHNK_DEFINED                    */


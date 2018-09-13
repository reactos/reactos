#ifndef LSBRK_DEFINED
#define LSBRK_DEFINED

#include "lsdefs.h"

typedef struct lsbrk					/* breaking information unit		*/
{
	BYTE fBreak;						/* break for neibours	*/
	BYTE fBreakAcrossSpaces;			/* break across spaces	*/
} LSBRK;									


#endif /* !LSBRK_DEFINED                         */


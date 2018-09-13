#ifndef BREAKREC_DEFINED
#define BREAKREC_DEFINED

#include "lsdefs.h"

/* ---------------------------------------------------------------------- */

struct breakrec
{
	DWORD idobj;
	LSCP cpFirst;
};

typedef struct breakrec BREAKREC;

#endif /* !BREAKREC_DEFINED */

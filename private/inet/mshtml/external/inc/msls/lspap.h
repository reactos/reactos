#ifndef LSPAP_DEFINED
#define LSPAP_DEFINED

#include "lsdefs.h"
#include "plspap.h"
#include "lskjust.h"
#include "lskalign.h"
#include "lsbrjust.h"
#include "lskeop.h"
#include "lstflow.h"

/* ---------------------------------------------------------------------- */

struct lspap
{
	LSCP cpFirst;						/* 1st cp for this paragraph */
	LSCP cpFirstContent;				/* 1st cp of "content" in the para */

	DWORD grpf;							/* line services format flags (lsffi.h)*/

	long uaLeft;						/* left boundary for line				*/
	long uaRightBreak;					/* right boundary for break */
	long uaRightJustify;					/* right boundary for justification */
	long duaIndent;
	long duaHyphenationZone;

	LSBREAKJUST lsbrj;					/* Break/Justification behavior	*/
	LSKJUST lskj;						/* Justification type */
	LSKALIGN lskal;						/* Alignment type */

	long duaAutoDecimalTab;

	LSKEOP lskeop;						/* kind of paragraph ending */
	
	LSTFLOW lstflow;					/* Main text flow direction */

};

typedef struct lspap LSPAP;

#endif /* !LSPAP_DEFINED */

#ifndef LSEMS_DEFINED
#define LSEMS_DEFINED

#include "lsdefs.h"

typedef struct lsems
{
	long em;		/* one em			*/
	long em2;		/* half em			*/
	long em3;		/* third em			*/
	long em4;		/* quater em		*/
	long em8;		/* eighth em		*/
	long em16;		/* 15/16 of em		*/
	long udExp;		/* user defined expansion	*/
	long udComp;	/* user defined compression*/
} LSEMS;



#endif /* !LSEMS_DEFINED */



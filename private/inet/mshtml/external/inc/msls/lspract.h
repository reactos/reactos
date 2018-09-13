#ifndef LSPRACT_DEFINED
#define LSPRACT_DEFINED

#include "lsdefs.h"
#include "lsact.h"

#define prior0					0		/* means priority is not defined	*/
#define prior1					1
#define prior2					2
#define prior3					3
#define prior4					4
#define prior5					5

typedef struct lspract					/* prioritized action 				*/
{
	BYTE prior;							/* priority							*/
	LSACT lsact;						/* action							*/
} LSPRACT;									


#endif /* !LSPRACTION_DEFINED                         */


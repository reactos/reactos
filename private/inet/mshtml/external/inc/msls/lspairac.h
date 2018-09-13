#ifndef LSPAIRAC_DEFINED
#define LSPAIRAC_DEFINED

#include "lsdefs.h"
#include "lsact.h"


typedef struct lspairact				/* Mod width pair unit				*/
{
	LSACT lsactFirst;					/* Action on first char				*/
	LSACT lsactSecond;					/* Action on second char			*/
} LSPAIRACT;									


#endif /* !LSPAIRAC_DEFINED                         */


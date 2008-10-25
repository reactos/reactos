/*
 *  GOTO.C - goto internal batch command.
 *
 *  History:
 *
 *    1 Feb 2008 (Christoph von Wittich)
 *        started.
*/ 

#include <precomp.h>


/* unimplemented */

/* our current default is delayedexpansion */

INT cmd_setlocal (LPTSTR param)
{
	return 0;
}

/* endlocal doesn't take any params */
INT cmd_endlocal (LPTSTR param)
{
	return 0;
}


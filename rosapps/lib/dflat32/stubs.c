#include "dflat32/dflat.h"

int TestCriticalError(void)
{
}


/* These are functions that are supposed to be part of the application
 * not part of the dflat32.dll
 *
 * - Fixme 
 */

char DFlatApplication[] = "none";   //edit.c

void PrepFileMenu(void *w, struct Menu *mnu) //edit.c
{
}

void PrepSearchMenu(void *w, struct Menu *mnu) //edit.c
{
}

void PrepEditMenu(void *w, struct Menu *mnu) //edit.c
{
}

char **Argv; //edit.c
#include <crtdll/conio.h>


/*
 * The next two functions are needed by cscanf
 */
static int
_scan_getche(FILE *fp)
{
  return(getche());
}

static int
_scan_ungetch(int c, FILE *fp)
{
  return(ungetch(c));
}

int
_cscanf(const char *fmt, ...)
{
  return(_doscan_low(NULL, _scan_getche, _scan_ungetch, 
		     fmt, (void **) unconst( ((&fmt)+1), char ** )));
}



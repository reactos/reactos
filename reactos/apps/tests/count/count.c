/* $Id: count.c,v 1.1 2001/03/26 21:30:20 ea Exp $
 *
 */
#include <stdlib.h>

int n = 0;

int
main (int argc, char * argv [])
{
	while (1) printf ("%d ", n ++ );
	return (0);
}

/* EOF */

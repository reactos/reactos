/* $Id: count.c,v 1.1 2004/10/21 05:12:00 sedwards Exp $
 *
 */
#include <stdio.h>

int n = 0;

int
main (int argc, char * argv [])
{
	while (1) printf ("%d ", n ++ );
	return (0);
}

/* EOF */

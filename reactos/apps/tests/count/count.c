/* $Id: count.c,v 1.2 2003/11/14 17:13:16 weiden Exp $
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

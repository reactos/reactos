/* $Id: CrtSupplement.c,v 1.2 2002/09/03 18:44:18 chorns Exp $
 *
 * Written by EA because ReactOS hasn't yet _ui64toa()
 * (it's in msvcrt.dll, and not in crtdll.dll). 
 */

#include <stdlib.h>

static
char DigitMap [] = "0123456789abcdefghijklmnopqrstuvwxyz";

char *
_ui64toa (
	unsigned __int64	value,
	char			* string,
	int			radix
	)
{
	int	reminder = 0;
	char	buffer [17];
	char	* w = buffer;
	int	len = 0;
	int	i = 0;

	/* Check the radix is valid */
	if ((2 > radix) || (36 < radix))
	{
		return string;
	}
	/* Convert the int64 to a string */
	do {
		reminder = (int) (value % (__int64) radix);
		*(w ++) = DigitMap [reminder];
		value /= (__int64) radix;
		++ len;

	} while ((__int64) value > 0);
	/* Reverse the string */
	while (i < len)
	{
		string [i ++] = *(-- w);
	}
	string [len] = '\0';
	
	return string;
}



/* EOF */

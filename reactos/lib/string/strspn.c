/*
 * $Id: strspn.c,v 1.1 2003/06/09 20:23:06 hbirr Exp $
 */

#include <string.h>

size_t strspn(const char *s1, const char *s2)
{
    unsigned long char_map[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    register unsigned char* str = (unsigned char*)s1;

    while (*s2)
    {
	char_map[*(unsigned char*)s2 / 32] |= (1 << (*(unsigned char*)s2 % 32));
	s2++;
    }

    while (*str)
    {
	if (!(char_map[*str / 32] & (1 << (*str % 32))))
	    break;
	str++;
    }
    return str - (unsigned char*)s1;
}

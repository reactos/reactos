/* $Id: strxspn.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include <limits.h>
#include <string.h>

size_t _strxspn(const char *s1, const char *s2)
{
 unsigned char char_map[1 << CHAR_BIT * sizeof(char)];
 register unsigned char * us2 = (unsigned char *)s2;
 register unsigned char * str = (unsigned char *)s1;

 memset(char_map, 0, sizeof(char_map));

 for(; *us2; ++ us2)
  char_map[*us2 / CHAR_BIT] |= (1 << (*us2 % CHAR_BIT));

 for(; *str; ++ str)
  if(_x(char_map[*str / CHAR_BIT] & (1 << (*str % CHAR_BIT)))) break;

 return str - (unsigned char*)s1;
}

/* EOF */

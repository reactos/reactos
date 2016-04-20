
#include <limits.h>
#include <string.h>

size_t __cdecl _strxspn(const char *s1, const char *s2)
{
 unsigned char char_map[1 << CHAR_BIT * sizeof(char)];
 const unsigned char * us2 = (const unsigned char *)s2;
 const unsigned char * str = (const unsigned char *)s1;

 memset(char_map, 0, sizeof(char_map));

 for(; *us2; ++ us2)
  char_map[*us2 / CHAR_BIT] |= (1 << (*us2 % CHAR_BIT));

 for(; *str; ++ str)
  if(_x(char_map[*str / CHAR_BIT] & (1 << (*str % CHAR_BIT)))) break;

 return (size_t)str - (size_t)s1;
}

/* EOF */

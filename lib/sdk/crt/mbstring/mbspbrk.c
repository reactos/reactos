#include <stdlib.h>
#include <mbstring.h>

int isleadbyte(int byte);

/*
 * not correct
 *
 * @implemented
 */
unsigned char * _mbspbrk(const unsigned char *s1, const unsigned char *s2)
{
  const unsigned char* p;

  while (*s1)
  {
    for (p = s2; *p; p += (isleadbyte(*p) ? 2 : 1))
    {
      if (*p == *s1)
        if (!isleadbyte(*p) || (*(p+1) == *(s1 + 1)))
          return (unsigned char*)s1;
    }
    s1 += (isleadbyte(*s1) ? 2 : 1);
  }
  return NULL;
}

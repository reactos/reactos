
#include <limits.h>
#include <string.h>

#define BIT_SIZE (CHAR_BIT * sizeof(unsigned long) / sizeof(char))

char* __cdecl strpbrk(const char *s1, const char *s2)
{
    if (*s2 == 0)
    {
       return 0;
    }
    if (*(s2+1) == 0)
    {
       return strchr(s1, *s2);
    }
    else if (*(s2+2) == 0)
    {
       char *s3, *s4;
       s3 = strchr(s1, *s2);
       s4 = strchr(s1, *(s2+1));
       if (s3 == 0)
       {
          return s4;
       }
       else if (s4 == 0)
       {
          return s3;
       }
       return s3 < s4 ? s3 : s4;
    }
    else
    {
       unsigned long char_map[(1 << CHAR_BIT) / BIT_SIZE] = {0, };
       const unsigned char* str = (const unsigned char*)s1;
       while (*s2)
       {
          char_map[*(const unsigned char*)s2 / BIT_SIZE] |= (1 << (*(const unsigned char*)s2 % BIT_SIZE));
          s2++;
       }
       while (*str)
       {
          if (char_map[*str / BIT_SIZE] & (1 << (*str % BIT_SIZE)))
          {
             return (char*)((size_t)str);
          }
          str++;
       }
    }
    return 0;
}


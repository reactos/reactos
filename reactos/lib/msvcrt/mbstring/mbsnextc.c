#include <msvcrti.h>


unsigned int _mbsnextc (const unsigned char *src)
{
  unsigned char *char_src = (unsigned char *)src;
  unsigned short *short_src = (unsigned short *)src;

  if (src == (void *)0)
    return 0;

  if (!_ismbblead(*src))
     return *char_src;
  else
     return *short_src;
  return 0;
}

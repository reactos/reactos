#include <precomp.h>
#include <mbstring.h>
#include <locale.h>

/*
 * @implemented
 */
unsigned int __cdecl _mbcjmstojis(unsigned int c)
{
  /* Conversion takes place only when codepage is 932.
     In all other cases, c is returned unchanged */
  if(get_mbcinfo()->mbcodepage == 932)
  {
    if(_ismbclegal(c) && HIBYTE(c) < 0xf0)
    {
      if(HIBYTE(c) >= 0xe0)
        c -= 0x4000;

      c = (((HIBYTE(c) - 0x81)*2 + 0x21) << 8) | LOBYTE(c);

      if(LOBYTE(c) > 0x7f)
        c -= 0x1;

      if(LOBYTE(c) > 0x9d)
        c += 0x83;
      else
        c -= 0x1f;
    }
    else
      return 0; /* Codepage is 932, but c can't be converted */
  }

  return c;
}

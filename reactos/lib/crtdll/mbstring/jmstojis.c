#include <crtdll/mbstring.h>

unsigned short _mbcjmstojis(unsigned short c)
{
  int c1, c2;

  c2 = (unsigned char)c;
  c1 = c >> 8;
  if (c1 < 0xf0 && _ismbblead(c1) && _ismbbtrail(c2)) {
    if (c1 >= 0xe0)
      c1 -= 0x40;
    c1 -= 0x70;
    c1 <<= 1;
    if (c2 < 0x9f) {
      c1 --;
      c2 -= 0x1f;
      if (c2 >= (0x80-0x1f))
        c2 --;
    } else {
      c2 -= 0x7e;
    }
    return ((c1 << 8) | c2);
  }
  return 0;
}
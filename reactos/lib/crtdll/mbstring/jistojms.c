#include <crtdll/mbstring.h>

unsigned short _mbcjistojms(unsigned short c)
{
  int c1, c2;

  c2 = (unsigned char)c;
  c1 = c >> 8;
  if (c1 >= 0x21 && c1 <= 0x7e && c2 >= 0x21 && c2 <= 0x7e) {
    if (c1 & 0x01) {
      c2 += 0x1f;
      if (c2 >= 0x7f)
        c2 ++;
    } else {
      c2 += 0x7e;
    }
    c1 += 0xe1;
    c1 >>= 1;
    if (c1 >= 0xa0)
      c1 += 0x40;
    return ((c1 << 8) | c2);
  }
  return 0;
}

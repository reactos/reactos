#include <msvcrt/string.h>

wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2)
{
  const wchar_t *scanp;
  int c, sc;

  while ((c = *s1++) != 0)
  {
    for (scanp = s2; (sc = *scanp++) != 0;)
      if (sc == c) {
	return (wchar_t *)(--s1);
      }
  }
  return 0;
}

#include <msvcrti.h>


int _fputchar(int c)
{
  return _putch(c);
}

wint_t _fputwchar(wint_t c)
{
  //return _putch(c);
  return 0;
}

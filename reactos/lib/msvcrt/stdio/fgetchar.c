#include <msvcrti.h>


int _fgetchar(void)
{
  return _getch();
}

wint_t _fgetwchar(void)
{
  return _getch();
}

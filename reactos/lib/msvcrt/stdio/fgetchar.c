#include <msvcrt/stdio.h>
#include <msvcrt/conio.h>

int _fgetchar(void)
{
  return _getch();
}

int _fgetwchar(void)
{
  return _getch();
}
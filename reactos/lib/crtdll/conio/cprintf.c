#include <stdio.h>
#include <conio.h>

int
cprintf(const char *fmt, ...)
{
  int     cnt;
  char    buf[ 2048 ];		/* this is buggy, because buffer might be too small. */
  va_list ap;
  
  va_start(ap, fmt);
  cnt = vsprintf(buf, fmt, ap);
  va_end(ap);
  
  cputs(buf);
  return cnt;
}

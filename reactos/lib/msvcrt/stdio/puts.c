/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/io.h>
#include <windows.h>
#include <crtdll/string.h>

#undef putchar
int
puts(const char *s)
{
	
  int c;
  while ((c = *s++))
    putchar(c);
  return putchar('\n');

}

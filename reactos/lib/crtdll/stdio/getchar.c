/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>

#undef getchar
int
getchar(void)
{
  return getc(stdin);
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>

#undef getchar
int
getchar(void)
{
  return getc(stdin);
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

#undef getchar
int
getchar(void)
{
  return getc(stdin);
}

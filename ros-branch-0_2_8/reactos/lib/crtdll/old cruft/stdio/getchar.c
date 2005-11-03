/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>

#undef getchar
/*
 * @implemented
 */
int
getchar(void)
{
  return getc(stdin);
}

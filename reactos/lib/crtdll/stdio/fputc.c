/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <wchar.h>
#include <libc/file.h>

int
fputc(int c, FILE *fp)
{
  return putc(c, fp);
}

int
fputwc(wchar_t c, FILE *fp)
{
  return putc(c, fp);
}

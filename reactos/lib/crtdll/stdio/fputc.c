/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/wchar.h>
#include <crtdll/internal/file.h>

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

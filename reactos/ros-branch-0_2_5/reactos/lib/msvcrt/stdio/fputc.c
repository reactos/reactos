/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/internal/file.h>

/*
 * @implemented
 */
int
fputc(int c, FILE *fp)
{
  return putc(c, fp);
}

/*
 * @implemented
 */
wint_t
fputwc(wchar_t c, FILE *fp)
{
  return putwc(c,fp);
}


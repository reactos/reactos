/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/internal/file.h>

int
fputc(int c, FILE *fp)
{
  return putc(c, fp);
}

wint_t 
fputwc(wchar_t c, FILE *fp)
{	
  return putwc(c,fp);
}


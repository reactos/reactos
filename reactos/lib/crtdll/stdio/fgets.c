/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

char *
fgets(char *s, int n, FILE *f)
{
  int c=0;
  char *cs;

  cs = s;
  while (--n>0 && (c = getc(f)) != EOF)
  {
    *cs++ = c;
    if (c == '\n')
      break;
  }
  if (c == EOF && cs == s)
    return NULL;
  *cs++ = '\0';
  return s;
}

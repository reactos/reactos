/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>

char *
gets(char *s)
{
  int c;
  char *cs;

  cs = s;
  while ((c = getchar()) != '\n' && c != EOF)
    *cs++ = c;
  if (c == EOF && cs==s)
    return NULL;
  *cs++ = '\0';
  return s;
}

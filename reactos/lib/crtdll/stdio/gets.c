/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/stdio/gets.c
 * PURPOSE:     Get a character string from stdin
 * PROGRAMER:   DJ Delorie
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for Reactos
 */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>

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

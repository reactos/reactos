/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <string.h>
#include <errno.h>



void
perror(const char *s)
{
 
  fprintf(stderr, "%s: %s\n", s, _strerror(NULL));
}

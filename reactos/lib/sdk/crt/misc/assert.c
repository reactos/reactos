/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>


/*
 * @implemented
 */
void _assert(const char *msg, const char *file, unsigned line)
{
  /* Assertion failed at foo.c line 45: x<y */
  fprintf(stderr, "Assertion failed at %s line %d: %s\n", file, line, msg);
  raise(SIGABRT);
  for(;;); /* eliminate warning by mingw */
}

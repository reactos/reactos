/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


void _assert(const char *msg, const char *file, int line)
{
  /* Assertion failed at foo.c line 45: x<y */
  fprintf(stderr, "Assertion failed at %s line %d: %s\n", file, line, msg);
  raise(SIGABRT);
}

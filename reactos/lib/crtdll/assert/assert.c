/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/assert.h>
#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>
#include <crtdll/signal.h>

void _assert(const char *msg, const char *file, int line) 
{
  /* Assertion failed at foo.c line 45: x<y */
  fprintf(stderr, "Assertion failed at %s line %d: %s\n", file, line, msg);
  raise(SIGABRT);
}

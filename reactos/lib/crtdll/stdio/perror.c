/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/string.h>
#include <crtdll/errno.h>


#ifdef perror
#undef perror
void perror(const char *s);
#endif

void
perror(const char *s)
{
 
  fprintf(stderr, "%s: %s\n", s, _strerror(NULL));
}

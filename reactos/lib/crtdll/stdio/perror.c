/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>
#include <msvcrt/errno.h>


#ifdef perror
#undef perror
void perror(const char *s);
#endif

void perror(const char *s)
{
 
  fprintf(stderr, "%s: %s\n", s, _strerror(NULL));
}

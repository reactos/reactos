/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


#ifdef perror
#undef perror
void perror(const char *s);
#endif

void perror(const char *s)
{
  fprintf(stderr, "%s: %s\n", s, _strerror(NULL));
}

void _wperror(const wchar_t *s)
{
  fwprintf(stderr, L"%s: %S\n", s, _strerror(NULL));
}

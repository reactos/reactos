/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


char *_strdup(const char *_s)
{
  char *rv;
  if (_s == 0)
    return 0;
  rv = (char *)malloc(strlen(_s) + 1);
  if (rv == 0)
    return 0;
  strcpy(rv, _s);
  return rv;
}

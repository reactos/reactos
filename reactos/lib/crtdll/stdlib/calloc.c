/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

void *
calloc(size_t size, size_t nelem)
{
  void *rv = malloc(size*nelem);
  if (rv)
    memset(rv, 0, size*nelem);
  return rv;
}

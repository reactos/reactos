/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdlib.h>
#include <crtdll/internal/atexit.h>

int
atexit(void (*a)(void))
{
  struct __atexit *ap;
  if (a == 0)
    return -1;
  ap = (struct __atexit *)malloc(sizeof(struct __atexit));
  if (!ap)
    return -1;
  ap->__next = __atexit_ptr;
  ap->__function = a;
  __atexit_ptr = ap;
  return 0;
}

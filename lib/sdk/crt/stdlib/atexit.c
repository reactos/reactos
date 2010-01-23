/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

void _atexit_cleanup(void)
{
  struct __atexit *next, *a = __atexit_ptr;
  __atexit_ptr = 0; /* to prevent infinite loops */
  while (a)
  {
    (a->__function)();
    next = a->__next;
    free(a);
    a = next;
  }
}

/*
 * @implemented
 *
 * Ported from WINE
 * Copyright (C) 2000 Jon Griffiths
 */
_onexit_t __dllonexit(_onexit_t func, _onexit_t **start, _onexit_t **end)
{
   _onexit_t *tmp;
   int len;

   if (!start || !*start || !end || !*end)
      return NULL;

   len = (*end - *start);
   if (++len <= 0)
      return NULL;

   tmp = (_onexit_t *)realloc(*start, len * sizeof(tmp));
   if (!tmp)
      return NULL;

   *start = tmp;
   *end = tmp + len;
   tmp[len - 1] = func;

   return func;
}

/*
 * @implemented
 */
_onexit_t _onexit(_onexit_t a)
{
  struct __atexit *ap;
  if (a == 0)
    return NULL;
  ap = (struct __atexit *)malloc(sizeof(struct __atexit));
  if (!ap)
    return NULL;
  ap->__next = __atexit_ptr;
  ap->__function = (void (*)(void))a;
  __atexit_ptr = ap;
  return a;
}

/*
 * @implemented
 */
int atexit(void (*a)(void))
{
  return _onexit((_onexit_t)a) == (_onexit_t)a ? 0 : -1;
}

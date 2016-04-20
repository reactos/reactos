/*
 * The C RunTime DLL
 *
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997 Uwe Bonnes
 */

#include <precomp.h>

/*
 * @implemented
 */
int _wcslwr_s(wchar_t* str, size_t n)
{
  wchar_t *ptr=str;
  if (!str || !n)
  {
    if (str) *str = '\0';
    *_errno() = EINVAL;
    return EINVAL;
  }

  while (n--)
  {
    if (!*ptr) return 0;
    *ptr = towlower(*ptr);
    ptr++;
  }

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *_errno() = EINVAL;
  return EINVAL;
}

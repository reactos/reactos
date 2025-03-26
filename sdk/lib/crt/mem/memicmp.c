/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
int
CDECL
_memicmp(const void *s1, const void *s2, size_t n)
{
    if (NtCurrentPeb()->OSMajorVersion >= 6)
    {
        if (!s1 || !s2)
        {
            if (n != 0)
            {
                MSVCRT_INVALID_PMT(NULL, EINVAL);
                return _NLSCMPERROR;
            }
            return 0;
        }
    }

  if (n != 0)
  {
    const unsigned char *p1 = s1, *p2 = s2;

    do {
      if (toupper(*p1) != toupper(*p2))
	return (*p1 - *p2);
      p1++;
      p2++;
    } while (--n != 0);
  }
  return 0;
}

#include <msvcrt/string.h>

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
void *
memcpy (void *to, const void *from, size_t count)
{
  register char *f = (char *)from;
  register char *t = (char *)to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;

  return to;
}

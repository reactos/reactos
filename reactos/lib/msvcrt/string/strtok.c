/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/string.h>
#include <msvcrt/internal/tls.h>

char *strtok(char *s, const char *delim)
{
  const char *spanp;
  int c, sc;
  char *tok;
  PTHREADDATA ThreadData = GetThreadData();

  if (s == NULL && (s = ThreadData->lasttoken) == NULL)
    return (NULL);

  /*
   * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
   */
 cont:
  c = *s++;
  for (spanp = delim; (sc = *spanp++) != 0;) {
    if (c == sc)
      goto cont;
  }

  if (c == 0) {			/* no non-delimiter characters */
    ThreadData->lasttoken = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;) {
    c = *s++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
	if (c == 0)
	  s = NULL;
	else
	  s[-1] = 0;
	ThreadData->lasttoken = s;
	return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}

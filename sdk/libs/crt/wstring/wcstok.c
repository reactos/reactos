/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

wchar_t** _wlasttoken(); /* wlasttok.c */

/*
 * @implemented
 */
wchar_t *wcstok(wchar_t *s, const wchar_t *ct)
{
	const wchar_t *spanp;
	int c, sc;
	wchar_t *tok;
#if 1
	wchar_t ** wlasttoken = _wlasttoken();
#else
	PTHREADDATA ThreadData = GetThreadData();
	wchar_t ** wlasttoken = &ThreadData->wlasttoken;
#endif

	if (s == NULL && (s = *wlasttoken) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading ctiters (s += strspn(s, ct), sort of).
	 */
	cont:
	c = *s;
	s++;
	for (spanp = ct; (sc = *spanp) != 0;spanp++) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {			/* no non-ctiter characters */
		*wlasttoken = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for ctiters: s += strcspn(s, ct), sort of).
	 * Note that ct must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s;
		s++;
		spanp = ct;
		do {
			if ((sc = *spanp) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*wlasttoken = s;
				return (tok);
			}
			spanp++;
		} while (sc != 0);
	}
	/* NOTREACHED */
}

/* $Id: wstring.c,v 1.10 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/string/wstring.c
 * PURPOSE:         Wide string functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   1998/12/04  RJJ  Cleaned up and added i386 def checks
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>


/* FUNCTIONS *****************************************************************/

int _wcsicmp (const wchar_t* cs, const wchar_t * ct)
{
	while (towlower(*cs) == towlower(*ct))
	{
		if (*cs == 0)
			return 0;
		cs++;
		ct++;
	}
	return towlower(*cs) - towlower(*ct);
}


/*
 * @implemented
 */
wchar_t *_wcslwr (wchar_t *x)
{
	wchar_t *y=x;

	while (*y) {
		*y=towlower(*y);
		y++;
	}
	return x;
}


/*
 * @implemented
 */
int _wcsnicmp (const wchar_t * cs, const wchar_t * ct, size_t count)
{
	if (count == 0)
		return 0;
	do {
		if (towupper(*cs) != towupper(*ct++))
			return towupper(*cs) - towupper(*--ct);
		if (*cs++ == 0)
			break;
	} while (--count != 0);
	return 0;
}


/*
 * @implemented
 */
wchar_t *_wcsupr(wchar_t *x)
{
	wchar_t  *y=x;

	while (*y) {
		*y=towupper(*y);
		y++;
	}
	return x;
}

/*
 * @implemented
 */
size_t wcscspn(const wchar_t *str,const wchar_t *reject)
{
	wchar_t *s;
	wchar_t *t;
	s=(wchar_t *)str;
	do {
		t=(wchar_t *)reject;
		while (*t) {
			if (*t==*s)
				break;
			t++;
		}
		if (*t)
			break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}

/*
 * @implemented
 */
wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2)
{
  const wchar_t *scanp;
  int c, sc;

  while ((c = *s1++) != 0)
  {
    for (scanp = s2; (sc = *scanp++) != 0;)
      if (sc == c)
      {
        return (wchar_t *)(--s1);
      }
  }
  return 0;
}

/*
 * @implemented
 */
size_t wcsspn(const wchar_t *str,const wchar_t *accept)
{
	wchar_t  *s;
	wchar_t  *t;
	s=(wchar_t *)str;
	do {
		t=(wchar_t *)accept;
		while (*t) {
			if (*t==*s)
				break;
			t++;
		}
		if (!*t)
			break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}


/*
 * @implemented
 */
wchar_t *wcsstr(const wchar_t *s,const wchar_t *b)
{
	wchar_t *x;
	wchar_t *y;
	wchar_t *c;
	x=(wchar_t *)s;
	while (*x) {
		if (*x==*b) {
			y=x;
			c=(wchar_t *)b;
			while (*y && *c && *y==*c) {
				c++;
				y++;
			}
			if (!*c)
				return x;
		}
		x++;
	}
	return NULL;
}

/* EOF */

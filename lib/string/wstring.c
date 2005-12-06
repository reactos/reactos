#include <string.h>
#include <ctype.h>

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
	const wchar_t *t;
        const wchar_t *s = str;
	do {
		t=reject;
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
        return (wchar_t *)((size_t)(--s1));
      }
  }
  return 0;
}

/*
 * @implemented
 */
size_t wcsspn(const wchar_t *str,const wchar_t *accept)
{
	const wchar_t *t;
	const wchar_t *s = str;
	do {
		t=accept;
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
	const wchar_t *y;
	const wchar_t *c;
	const wchar_t *x = s;
	while (*x) {
		if (*x==*b) {
			y=x;
			c=b;
			while (*y && *c && *y==*c) {
				c++;
				y++;
			}
			if (!*c)
				return (wchar_t *)((size_t)x);
		}
		x++;
	}
	return NULL;
}

/* EOF */

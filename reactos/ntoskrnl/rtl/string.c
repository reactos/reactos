/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/string.c
 * PURPOSE:         Ascii string functions
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *   1999/07/29  ekohl   Created
 */

/* INCLUDES *****************************************************************/

#include <ctype.h>
#include <string.h>

/* FUNCTIONS *****************************************************************/

int _stricmp(const char *s1, const char *s2)
{
	while (toupper(*s1) == toupper(*s2))
	{
		if (*s1 == 0)
			return 0;
		s1++;
		s2++;
	}
	return toupper(*(unsigned const char *)s1) - toupper(*(unsigned const char *)(s2));
}


char * _strlwr(char *x)
{
	char  *y=x;

	while (*y)
	{
		*y=tolower(*y);
		y++;
	}
	return x;
}


int _strnicmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;
	do
	{
		if (toupper(*s1) != toupper(*s2++))
			return toupper(*(unsigned const char *)s1) - toupper(*(unsigned const char *)--s2);
		if (*s1++ == 0)
			break;
	}
	while (--n != 0);
	return 0;
}

char* _strnset(char* szToFill, int szFill, size_t sizeMaxFill)
{
	char *t = szToFill;
	int i = 0;
	while (*szToFill != 0 && i < (int) sizeMaxFill)
	{
		*szToFill = szFill;
		szToFill++;
		i++;
		
	}
	return t;
}


char * _strrev(char *s) 
{
	char  *e;
	char   a;

	e = s;
	while (*e)
		e++;

	while (s<e)
	{
		a = *s;
		*s = *e;
		*e = a;
		s++;
		e--;
	}
	return s;
}


char* _strset(char* szToFill, int szFill)
{
	char *t = szToFill;
	while (*szToFill != 0)
	{
		*szToFill = szFill;
		szToFill++;
		
	}
	return t;
}


char *_strupr(char *x)
{
	char  *y=x;

	while (*y)
	{
		*y=toupper(*y);
		y++;
	}
	return x;
}


char *strcat(char *s, const char *append)
{
	char *save = s;

	for (; *s; ++s);
	while ((*s++ = *append++));
	return save;
}


char *strchr(const char *s, int c)
{
	char cc = c;

	while (*s)
	{
		if (*s == cc)
			return (char *)s;
		s++;
	}

	if (cc == 0)
		return (char *)s;

	return 0;
}


int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2)
	{
		if (*s1 == 0)
			return 0;
		s1++;
		s2++;
	}

	return *(unsigned const char *)s1 - *(unsigned const char *)(s2);
}


char* strcpy(char *to, const char *from)
{
	char *save = to;

	for (; (*to = *from); ++from, ++to);

	return save;
}


size_t strlen(const char *str)
{
	const char *s;

	if (str == 0)
		return 0;
	for (s = str; *s; ++s);

	return s-str;
}


char *strncat(char *dst, const char *src, size_t n)
{
	if (n != 0)
	{
		char *d = dst;
		const char *s = src;

		while (*d != 0)
			d++;
		do
		{
			if ((*d = *s++) == 0)
				break;
			d++;
		}
		while (--n != 0);
		*d = 0;
	}

	return dst;
}


int strncmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;
	do
	{
		if (*s1 != *s2++)
			return *(unsigned const char *)s1 - *(unsigned const char *)--s2;
		if (*s1++ == 0)
			break;
	}
	while (--n != 0);

	return 0;
}


char *strncpy(char *dst, const char *src, size_t n)
{
	if (n != 0)
	{
		char *d = dst;
		const char *s = src;

		do
		{
			if ((*d++ = *s++) == 0)
			{
				while (--n != 0)
					*d++ = 0;
				break;
			}
		}
		while (--n != 0);
                d[0] = 0;
	}
      else
        {
          dst[0] = 0;
        }
	return dst;
}


char *strrchr(const char *s, int c)
{
	char cc = c;
	const char *sp=(char *)0;

	while (*s)
	{
		if (*s == cc)
			sp = s;
		s++;
	}

	if (cc == 0)
		sp = s;

	return (char *)sp;
}


size_t strspn(const char *s1, const char *s2)
{
	const char *p = s1, *spanp;
	char c, sc;

  cont:
	c = *p++;
	for (spanp = s2; (sc = *spanp++) != 0;)
		if (sc == c)
			goto cont;

	return (p - 1 - s1);
}


char *strstr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0)
	{
		len = strlen(find);
		do
		{
			do
			{
				if ((sc = *s++) == 0)
					return 0;
			}
			while (sc != c);
		}
		while (strncmp(s, find, len) != 0);
		s--;
	}

	return (char *)s;
}

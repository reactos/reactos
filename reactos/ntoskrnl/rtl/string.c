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

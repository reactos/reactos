/* $Id: wstring.c,v 1.19 2003/05/27 19:24:04 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/wstring.c
 * PURPOSE:         Wide string functions
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   1998/12/04  RJJ    Cleaned up and added i386 def checks.
 *   1999/07/29  ekohl  Added missing functions.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

int _wcsicmp (const wchar_t* cs, const wchar_t* ct)
{
	while (*cs != '\0' && *ct != '\0' && towupper(*cs) == towupper(*ct))
	{
		cs++;
		ct++;
	}
	return *cs - *ct;
}

wchar_t *_wcslwr (wchar_t *x)
{
	wchar_t  *y=x;

	while (*y)
	{
		*y=towlower(*y);
		y++;
	}
	return x;
}


int _wcsnicmp (const wchar_t * cs,const wchar_t * ct,size_t count)
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


wchar_t *_wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill)
{
	wchar_t *t = wsToFill;
	int i = 0;
	while( *wsToFill != 0 && i < (int) sizeMaxFill)
	{
		*wsToFill = wcFill;
		wsToFill++;
		i++;
	}
	return t;
}


wchar_t *_wcsrev(wchar_t *s)
{
	wchar_t  *e;
	wchar_t   a;
	e=s;
	while (*e)
		e++;
	while (s<e)
	{
		a=*s;
		*s=*e;
		*e=a;
		s++;
		e--;
	}
	return s;
}


wchar_t *_wcsupr(wchar_t *x)
{
	wchar_t *y=x;

	while (*y)
	{
		*y=towupper(*y);
		y++;
	}
	return x;
}

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

size_t wcsspn(const wchar_t *str,const wchar_t *accept)
{
	wchar_t  *s;
	wchar_t  *t;
	s=(wchar_t *)str;
	do
	{
		t=(wchar_t *)accept;
		while (*t)
		{
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


wchar_t *wcsstr(const wchar_t *s,const wchar_t *b)
{
	wchar_t *x;
	wchar_t *y;
	wchar_t *c;
	x=(wchar_t *)s;
	while (*x)
	{
		if (*x==*b)
		{
			y=x;
			c=(wchar_t *)b;
			while (*y && *c && *y==*c)
			{
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

/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <rtl.h>

int strlen(char *str)
{
	int	len;

	for(len=0; str[len] != '\0'; len++);

	return len;
}

char *strcpy(char *dest, char *src)
{
	char	*ret = dest;

	while(*src)
		*dest++ = *src++;
	*dest = 0;

	return ret;
}

char *strncpy(char *dest, char *src, size_t count)
{
	char	*ret = dest;

	while((*src) && (count--))
		*dest++ = *src++;
	*dest = 0;

	return ret;
}

char *strcat(char *dest, char *src)
{
	char	*ret = dest;

	while(*dest)
		dest++;

	while(*src)
		*dest++ = *src++;
	*dest = 0;

	return ret;
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

int strcmp(const char *string1, const char *string2)
{
	while(*string1 == *string2)
	{
		if(*string1 == 0)
			return 0;

		string1++;
		string2++;
	}

	return *(unsigned const char *)string1 - *(unsigned const char *)(string2);
}

int stricmp(const char *string1, const char *string2)
{
	while(tolower(*string1) == tolower(*string2))
	{
		if(*string1 == 0)
			return 0;

		string1++;
		string2++;
	}

	return (int)tolower(*string1) - (int)tolower(*string2);
}

int _strnicmp(const char *string1, const char *string2, size_t length)
{
	if (length == 0)
		return 0;
	do
	{
		if (toupper(*string1) != toupper(*string2++))
			return toupper(*(unsigned const char *)string1) - toupper(*(unsigned const char *)--string2);
		if (*string1++ == 0)
			break;
	}
	while (--length != 0);
	return 0;
}

int strncmp(const char *string1, const char *string2, size_t length)
{
	if (length == 0)
		return 0;
	do
	{
		if (*string1 != *string2++)
			return *(unsigned const char *)string1 - *(unsigned const char *)--string2;
		if (*string1++ == 0)
			break;
	}
	while (--length != 0);
	return 0;
}

/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
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

#include "freeldr.h"
#include "stdlib.h"

/*
 * print() - prints unformatted text to stdout
 */
void print(char *str)
{
	int	i;

	for(i=0; i<strlen(str); i++)
		putchar(str[i]);
}

/*
 * convert_to_ascii() - converts a number to it's ascii equivalent
 * from:
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
 */
char *convert_to_ascii(char *buf, int c, ...)
{
  unsigned long num = *((&c) + 1), mult = 10;
  char *ptr = buf;

  if (c == 'x')
    mult = 16;

  if ((num & 0x80000000uL) && c == 'd')
    {
      num = (~num)+1;
      *(ptr++) = '-';
      buf++;
    }

  do
    {
      int dig = num % mult;
      *(ptr++) = ( (dig > 9) ? dig + 'a' - 10 : '0' + dig );
    }
  while (num /= mult);

  /* reorder to correct direction!! */
  {
    char *ptr1 = ptr-1;
    char *ptr2 = buf;
    while (ptr1 > ptr2)
      {
	int c = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = c;
	ptr1--;
	ptr2++;
      }
  }

  return ptr;
}

/*
 * printf() - prints formatted text to stdout
 * from:
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
 */
void printf(char *format, ... )
{
  int *dataptr = (int *) &format;
  char c, *ptr, str[16];

  dataptr++;

  while ((c = *(format++)))
    {
      if (c != '%')
	putchar(c);
      else
	switch (c = *(format++))
	  {
	  case 'd': case 'u': case 'x':
	    *convert_to_ascii(str, c, *((unsigned long *) dataptr++)) = 0;

	    ptr = str;

	    while (*ptr)
	      putchar(*(ptr++));
	    break;

	  case 'c': putchar((*(dataptr++))&0xff); break;

	  case 's':
	    ptr = (char *)(*(dataptr++));

	    while ((c = *(ptr++)))
	      putchar(c);
	    break;
	  }
    }
}

void sprintf(char *buffer, char *format, ... )
{
  int *dataptr = (int *) &format;
  char c, *ptr, str[16];
  char *p = buffer;

  dataptr++;

  while ((c = *(format++)))
    {
      if (c != '%')
      {
	*p = c;
	p++;
      }
      else
	switch (c = *(format++))
	  {
	  case 'd': case 'u': case 'x':
	    *convert_to_ascii(str, c, *((unsigned long *) dataptr++)) = 0;

	    ptr = str;

	    while (*ptr)
	    {
	      *p = *(ptr++);
	      p++;
	    }
	    break;

	  case 'c':
	    *p = (*(dataptr++))&0xff;
	    p++;
	    break;

	  case 's':
	    ptr = (char *)(*(dataptr++));

	    while ((c = *(ptr++)))
	    {
	      *p = c;
	      p++;
	    }
	    break;
	  }
    }
  *p=0;
}


int strlen(char *str)
{
	int	len;

	for(len=0; str[len] != '\0'; len++);

	return len;
}

char *itoa(int value, char *string, int radix)
{
	if(radix == 16)
	    *convert_to_ascii(string, 'x', value) = 0;
	else
	    *convert_to_ascii(string, 'd', value) = 0;

	return string;
}

int toupper(int c)
{
	if((c >= 'a') && (c <= 'z'))
		c -= 32;

	return c;
}

int tolower(int c)
{
	if((c >= 'A') && (c <= 'Z'))
		c += 32;

	return c;
}

int memcmp(const void *buf1, const void *buf2, size_t count)
{
	size_t		i;
	const char	*buffer1 = buf1;
	const char	*buffer2 = buf2;

	for(i=0; i<count; i++)
	{
		if(buffer1[i] == buffer2[i])
			continue;
		else
			return (buffer1[i] - buffer2[i]);
	}

	return 0;
}

void *memcpy(void *dest, const void *src, size_t count)
{
	size_t		i;
	char		*buf1 = dest;
	const char	*buf2 = src;

	for(i=0; i<count; i++)
		buf1[i] = buf2[i];

	return dest;
}

void *memset(void *dest, int c, size_t count)
{
	size_t		i;
	char		*buf1 = dest;

	for(i=0; i<count; i++)
		buf1[i] = c;

	return dest;
}

char *strcpy(char *dest, char *src)
{
	char	*ret = dest;

	while(*src)
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

char *fgets(char *string, int n, FILE *stream)
{
	int	i;

	for(i=0; i<(n-1); i++)
	{
		if(feof(stream))
		{
			i++;
			break;
		}

		ReadFile(stream, 1, string+i);

		if(string[i] == '\n')
		{
			i++;
			break;
		}
	}
	string[i] = '\0';

	return string;
}

int atoi(char *string)
{
	int	base;
	int	result = 0;
	char	*str;

	if((string[0] == '0') && (string[1] == 'x'))
	{
		base = 16;
		str = string + 2;
	}
	else
	{
		base = 10;
		str = string;
	}

	while(1)
	{
		if((*str < '0') || (*str > '9'))
			break;

		result *= base;
		result += (*str - '0');
		str++;
	}

	return result;
}

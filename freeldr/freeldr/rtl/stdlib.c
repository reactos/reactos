/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

int isspace(int c)
{
  return(c == ' ' || (c >= 0x09 && c <= 0x0D));
}

int isdigit(int c)
{
  return(c >= '0' && c <= '9');
}

int isxdigit(int c)
{
  return((c >= '0' && c <= '9')||(c >= 'a' && c <= 'f')||(c >= 'A' && c <= 'F'));
}

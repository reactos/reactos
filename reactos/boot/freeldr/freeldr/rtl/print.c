/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#include <freeldr.h>
#include <machine.h>
#include <rtl.h>

/*
 * print() - prints unformatted text to stdout
 */
void print(char *str)
{
	int	i;

	for (i = 0; i < strlen(str); i++)
		MachConsPutChar(str[i]);
}

/*
 * printf() - prints formatted text to stdout
 * originally from GRUB
 */
void printf(char *format, ... )
{
	int *dataptr = (int *)(void *)&format;
	char c, *ptr, str[16];
	int ll;

	dataptr++;

	while ((c = *(format++)))
	{
		if (c != '%')
		{
			MachConsPutChar(c);
		}
		else
		{
			if (*format == 'I' && *(format+1) == '6' && *(format+2) == '4')
			{
				ll = 1;
				format += 3;
			}
			else
			{
				ll = 0;
			}
			switch (c = *(format++))
			{
			case 'd': case 'u': case 'x':
				if (ll)
				{
					*convert_i64_to_ascii(str, c, *((unsigned long long *) dataptr++)) = 0;
				}
				else
				{
					*convert_to_ascii(str, c, *((unsigned long *) dataptr++)) = 0;
				}

				ptr = str;

				while (*ptr)
				{
					MachConsPutChar(*(ptr++));
				}
				break;

			case 'c': MachConsPutChar((*(dataptr++))&0xff); break;

			case 's':
				ptr = (char *)(*(dataptr++));

				while ((c = *(ptr++)))
				{
					MachConsPutChar(c);
				}
				break;
			case '%':
				MachConsPutChar(c);
				break;
			default:
				printf("\nprintf() invalid format specifier - %%%c\n", c);
				break;
			}
		}
	}
}

void sprintf(char *buffer, char *format, ... )
{
	int *dataptr = (int *)(void *)&format;
	char c, *ptr, str[16];
	char *p = buffer;
	int ll;

	dataptr++;

	while ((c = *(format++)))
	{
		if (c != '%')
		{
			*p = c;
			p++;
		}
		else
		{
			if (*format == 'I' && *(format+1) == '6' && *(format+2) == '4')
			{
				ll = 1;
				format += 3;
			}
			else
			{
				ll = 0;
			}
			switch (c = *(format++))
			{
			case 'd': case 'u': case 'x':
				if (ll)
				{
					*convert_i64_to_ascii(str, c, *((unsigned long long*) dataptr++)) = 0;
				}
				else
				{
					*convert_to_ascii(str, c, *((unsigned long *) dataptr++)) = 0;
				}
					
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
			case '%':
				*p = c;
				p++;
				break;
			default:
				printf("\nsprintf() invalid format specifier - %%%c\n", c);
				break;
			}
		}
	}
	*p=0;
}

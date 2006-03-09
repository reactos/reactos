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
#include <stdarg.h>

/*
 * print() - prints unformatted text to stdout
 */
void print(char *str)
{
	size_t	i;

	for (i = 0; i < strlen(str); i++)
		MachConsPutChar(str[i]);
}

/*
 * printf() - prints formatted text to stdout
 * originally from GRUB
 */
int printf(const char *format, ... )
{
	va_list ap;
	va_start(ap,format);
	char c, *ptr, str[16];
	int ll;

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
					*convert_i64_to_ascii(str, c, va_arg(ap, unsigned long long)) = 0;
				}
				else
				{
					*convert_to_ascii(str, c, va_arg(ap, unsigned long)) = 0;
				}

				ptr = str;

				while (*ptr)
				{
					MachConsPutChar(*(ptr++));
				}
				break;

			case 'c': MachConsPutChar((va_arg(ap,int))&0xff); break;

			case 's':
				ptr = va_arg(ap,char *);

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

	va_end(ap);

	return 0;
}

int sprintf(char *buffer, const char *format, ... )
{
	va_list ap;
	char c, *ptr, str[16];
	char *p = buffer;
	int ll;

	va_start(ap,format);

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
					*convert_i64_to_ascii(str, c, va_arg(ap, unsigned long long)) = 0;
				}
				else
				{
					*convert_to_ascii(str, c, va_arg(ap, unsigned long)) = 0;
				}

				ptr = str;

				while (*ptr)
				{
				*p = *(ptr++);
				p++;
				}
				break;

			case 'c':
				*p = va_arg(ap,int)&0xff;
				p++;
				break;

			case 's':
				ptr = va_arg(ap,char *);

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
	va_end(ap);
	*p=0;

	return 0;
}

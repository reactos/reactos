/* $Id: swprintf.c,v 1.7 2001/05/30 14:37:25 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/stdio/swprintf.c
 * PURPOSE:         unicode sprintf functions
 * PROGRAMMERS:     David Welch
 *                  Eric Kohl
 *
 * TODO:
 *   - Implement maximum length (cnt) in _vsnwprintf().
 *   - Verify the implementation of '%Z'.
 */

/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <limits.h>

#define NDEBUG
#include <ntdll/ntdll.h>


#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */


#define do_div(n,base) ({ \
int __res; \
__res = ((unsigned long long) n) % (unsigned) base; \
n = ((unsigned long long) n) / (unsigned) base; \
__res; })


static int skip_atoi(const wchar_t **s)
{
	int i=0;

	while (iswdigit(**s))
		i = i*10 + *((*s)++) - L'0';
	return i;
}


static wchar_t *
number (wchar_t *str, long long num, int base, int size, int precision,
	int type)
{
   wchar_t c,sign,tmp[66];
   const wchar_t *digits = L"0123456789abcdefghijklmnopqrstuvwxyz";
   int i;
   
   if (type & LARGE)
     digits = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   if (type & LEFT)
     type &= ~ZEROPAD;
   if (base < 2 || base > 36)
     return 0;
   
   c = (type & ZEROPAD) ? L'0' : L' ';
   sign = 0;
   
   if (type & SIGN) 
     {
	if (num < 0) 
	  {
	     sign = L'-';
	     num = -num;
	     size--;
	  } 
	else if (type & PLUS) 
	  {
	     sign = L'+';
	     size--;
	  } 
	else if (type & SPACE) 
	  {
	     sign = L' ';
	     size--;
	  }
     }
   
   if (type & SPECIAL) 
     {
	if (base == 16)
	  size -= 2;
	else if (base == 8)
	  size--;
     }
   
   i = 0;
   if (num == 0)
     tmp[i++]='0';
   else while (num != 0)
     tmp[i++] = digits[do_div(num,base)];
   if (i > precision)
     precision = i;
   size -= precision;
   if (!(type&(ZEROPAD+LEFT)))
     while(size-->0)
       *str++ = L' ';
   if (sign)
     *str++ = sign;
   if (type & SPECIAL)
     {
	if (base==8)
	  {
             *str++ = L'0';
	  }
	else if (base==16) 
	  {
             *str++ = L'0';
	     *str++ = digits[33];
	  }
     }
   if (!(type & LEFT))
     while (size-- > 0)
       *str++ = c;
   while (i < precision--)
     *str++ = '0';
   while (i-- > 0)
     *str++ = tmp[i];
   while (size-- > 0)
     *str++ = L' ';
   return str;
}


int _vsnwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, va_list args)
{
	int len;
	unsigned long long num;
	int i, base;
	wchar_t * str;
	const char *s;
	const wchar_t *sw;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier;		/* 'h', 'l', 'L', 'w' or 'I' for integer fields */

	for (str=buf ; *fmt ; ++fmt) {
		if (*fmt != L'%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case L'-': flags |= LEFT; goto repeat;
				case L'+': flags |= PLUS; goto repeat;
				case L' ': flags |= SPACE; goto repeat;
				case L'#': flags |= SPECIAL; goto repeat;
				case L'0': flags |= ZEROPAD; goto repeat;
			}

		/* get field width */
		field_width = -1;
		if (iswdigit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == L'*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == L'.') {
			++fmt;	
			if (iswdigit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == L'*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'w') {
			qualifier = *fmt;
			++fmt;
		} else if (*fmt == 'I' && *(fmt+1) == '6' && *(fmt+2) == '4') {
			qualifier = *fmt;
			fmt += 3;
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case L'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = L' ';
			if (qualifier == 'h')
				*str++ = (wchar_t) va_arg(args, int);
			else
				*str++ = 
				  (wchar_t) va_arg(args, int);
			while (--field_width > 0)
				*str++ = L' ';
			continue;

		case L'C':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = L' ';
			if (qualifier == 'l' || qualifier == 'w')
				*str++ = (wchar_t) va_arg(args, int);
			else
				*str++ = (wchar_t) va_arg(args, int);
			while (--field_width > 0)
				*str++ = L' ';
			continue;

		case L's':
			if (qualifier == 'h') {
				/* print ascii string */
				s = va_arg(args, char *);
				if (s == NULL)
					s = "<NULL>";

				len = strlen (s);
				if ((unsigned int)len > (unsigned int)precision)
					len = precision;

				if (!(flags & LEFT))
					while (len < field_width--)
						*str++ = L' ';
				for (i = 0; i < len; ++i)
					*str++ = (wchar_t)(*s++);
				while (len < field_width--)
					*str++ = L' ';
			} else {
				/* print unicode string */
				sw = va_arg(args, wchar_t *);
				if (sw == NULL)
					sw = L"<NULL>";

				len = wcslen (sw);
				if ((unsigned int)len > (unsigned int)precision)
					len = precision;

				if (!(flags & LEFT))
					while (len < field_width--)
						*str++ = L' ';
				for (i = 0; i < len; ++i)
					*str++ = *sw++;
				while (len < field_width--)
					*str++ = L' ';
			}
			continue;

		case L'S':
			if (qualifier == 'l' || qualifier == 'w') {
				/* print unicode string */
				sw = va_arg(args, wchar_t *);
				if (sw == NULL)
					sw = L"<NULL>";

				len = wcslen (sw);
				if ((unsigned int)len > (unsigned int)precision)
					len = precision;

				if (!(flags & LEFT))
					while (len < field_width--)
						*str++ = L' ';
				for (i = 0; i < len; ++i)
					*str++ = *sw++;
				while (len < field_width--)
					*str++ = L' ';
			} else {
				/* print ascii string */
				s = va_arg(args, char *);
				if (s == NULL)
					s = "<NULL>";

				len = strlen (s);
				if ((unsigned int)len > (unsigned int)precision)
					len = precision;

				if (!(flags & LEFT))
					while (len < field_width--)
						*str++ = L' ';
				for (i = 0; i < len; ++i)
					*str++ = (wchar_t)(*s++);
				while (len < field_width--)
					*str++ = L' ';
			}
			continue;

		case 'Z':
			if (qualifier == 'h') {
				/* print counted ascii string */
				PANSI_STRING pus = va_arg(args, PANSI_STRING);
				if ((pus == NULL) || (pus->Buffer)) {
					sw = L"<NULL>";
					while ((*sw) != 0)
						*str++ = *sw++;
				} else {
					for (i = 0; pus->Buffer[i] && i < pus->Length; i++)
						*str++ = (wchar_t)(pus->Buffer[i]);
				}
			} else {
				/* print counted unicode string */
				PUNICODE_STRING pus = va_arg(args, PUNICODE_STRING);
				if ((pus == NULL) || (pus->Buffer)) {
					sw = L"<NULL>";
					while ((*sw) != 0)
						*str++ = *sw++;
				} else {
					for (i = 0; pus->Buffer[i] && i < pus->Length / sizeof(WCHAR); i++)
						*str++ = pus->Buffer[i];
				}
			}
			continue;

		case L'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;

		case L'n':
			if (qualifier == 'l') {
				long * ip = va_arg(args, long *);
				*ip = (str - buf);
			} else {
				int * ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;

		/* integer number formats - set up the flags and "break" */
		case L'o':
			base = 8;
			break;

		case L'b':
			base = 2;
			break;

		case L'X':
			flags |= LARGE;
		case L'x':
			base = 16;
			break;

		case L'd':
		case L'i':
			flags |= SIGN;
		case L'u':
			break;

		default:
			if (*fmt != L'%')
				*str++ = L'%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}

		if (qualifier == 'I')
			num = va_arg(args, unsigned long long);
		else if (qualifier == 'l')
			num = va_arg(args, unsigned long);
		else if (qualifier == 'h') {
			if (flags & SIGN)
				num = va_arg(args, int);
			else
				num = va_arg(args, unsigned int);
		}
		else {
			if (flags & SIGN)
				num = va_arg(args, int);
			else
				num = va_arg(args, unsigned int);
		}
		str = number(str, num, base, field_width, precision, flags);
	}
	*str = L'\0';
	return str-buf;
}


int swprintf(wchar_t *buf, const wchar_t *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=_vsnwprintf(buf,INT_MAX,fmt,args);
	va_end(args);
	return i;
}


int _snwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=_vsnwprintf(buf,cnt,fmt,args);
	va_end(args);
	return i;
}


int vswprintf(wchar_t *buf, const wchar_t *fmt, va_list args)
{
	return _vsnwprintf(buf,INT_MAX,fmt,args);
}

/* EOF */

/* $Id: swprintf.c,v 1.11 2002/09/12 17:50:42 guido Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/stdio/swprintf.c
 * PURPOSE:         unicode sprintf functions
 * PROGRAMMERS:     David Welch
 *                  Eric Kohl
 *
 * TODO:
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
number(wchar_t * buf, wchar_t * end, long long num, int base, int size, int precision, int type)
{
	wchar_t c, sign, tmp[66];
	const wchar_t *digits;
	const wchar_t small_digits[] = L"0123456789abcdefghijklmnopqrstuvwxyz";
	const wchar_t large_digits[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	digits = (type & LARGE) ? large_digits : small_digits;
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? L'0' : L' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = L'-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = L'+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++] = L'0';
	else while (num != 0)
		tmp[i++] = digits[do_div(num,base)];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT))) {
		while(size-->0) {
			if (buf <= end)
				*buf = L' ';
			++buf;
		}
	}
	if (sign) {
		if (buf <= end)
			*buf = sign;
		++buf;
	}
	if (type & SPECIAL) {
		if (base==8) {
			if (buf <= end)
				*buf = L'0';
			++buf;
		} else if (base==16) {
			if (buf <= end)
				*buf = L'0';
			++buf;
			if (buf <= end)
				*buf = digits[33];
			++buf;
		}
	}
	if (!(type & LEFT)) {
		while (size-- > 0) {
			if (buf <= end)
				*buf = c;
			++buf;
		}
	}
	while (i < precision--) {
		if (buf <= end)
			*buf = L'0';
		++buf;
	}
	while (i-- > 0) {
		if (buf <= end)
			*buf = tmp[i];
		++buf;
	}
	while (size-- > 0) {
		if (buf <= end)
			*buf = L' ';
		++buf;
	}
	return buf;
}


int _vsnwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, va_list args)
{
	int len;
	unsigned long long num;
	int i, base;
	wchar_t * str, * end;
	const char *s;
	const wchar_t *sw;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier;		/* 'h', 'l', 'L', 'w' or 'I' for integer fields */

	str = buf;
	end = buf + cnt - 1;
	if (end < buf - 1) {
		end = ((void *) -1);
		cnt = end - buf + 1;
	}

	for ( ; *fmt ; ++fmt) {
		if (*fmt != L'%') {
			if (str <= end)
				*str = *fmt;
			++str;
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
				while (--field_width > 0) {
					if (str <= end)
						*str = L' ';
					++str;
				}
			if (qualifier == 'h') {
				if (str <= end)
					*str = (wchar_t) va_arg(args, int);
				++str;
			} else {
				if (str <= end)
					*str = (wchar_t) va_arg(args, int);
				++str;
			}
			while (--field_width > 0) {
				if (str <= end)
					*str = L' ';
				++str;
			}
			continue;

		case L'C':
			if (!(flags & LEFT))
				while (--field_width > 0) {
					if (str <= end)
						*str = L' ';
					++str;
				}
			if (qualifier == 'l' || qualifier == 'w') {
				if (str <= end)
					*str = (wchar_t) va_arg(args, int);
				++str;
			} else {
				if (str <= end)
					*str = (wchar_t) va_arg(args, int);
				++str;
			}
			while (--field_width > 0) {
				if (str <= end)
					*str = L' ';
				++str;
			}
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
					while (len < field_width--) {
						if (str <= end)
							*str = L' ';
						++str;
					}
				for (i = 0; i < len; ++i) {
					if (str <= end)
						*str = (wchar_t)(*s);
					++str;
					++s;
				}
				while (len < field_width--) {
					if (str <= end)
						*str = L' ';
					++str;
				}
			} else {
				/* print unicode string */
				sw = va_arg(args, wchar_t *);
				if (sw == NULL)
					sw = L"<NULL>";

				len = wcslen (sw);
				if ((unsigned int)len > (unsigned int)precision)
					len = precision;

				if (!(flags & LEFT))
					while (len < field_width--) {
						if (str <= end)
							*str = L' ';
						++str;
					}
				for (i = 0; i < len; ++i) {
					if (str <= end)
						*str = *sw;
					++str;
					++sw;
				}
				while (len < field_width--) {
					if (str <= end)
						*str = L' ';
					++str;
				}
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
					while (len < field_width--) {
						if (str <= end)
							*str = L' ';
						++str;
					}
				for (i = 0; i < len; ++i) {
					if (str <= end)
						*str = *sw;
					++str;
					++sw;
				}
				while (len < field_width--) {
					if (str <= end)
						*str = L' ';
					++str;
				}
			} else {
				/* print ascii string */
				s = va_arg(args, char *);
				if (s == NULL)
					s = "<NULL>";

				len = strlen (s);
				if ((unsigned int)len > (unsigned int)precision)
					len = precision;

				if (!(flags & LEFT))
					while (len < field_width--) {
						if (str <= end)
							*str = L' ';
						++str;
					}
				for (i = 0; i < len; ++i) {
					if (str <= end)
						*str = (wchar_t)(*s);
					++str;
					++s;
				}
				while (len < field_width--) {
					if (str <= end)
						*str = L' ';
					++str;
				}
			}
			continue;

		case L'Z':
			if (qualifier == 'h') {
				/* print counted ascii string */
				PANSI_STRING pus = va_arg(args, PANSI_STRING);
				if ((pus == NULL) || (pus->Buffer == NULL)) {
					sw = L"<NULL>";
					while ((*sw) != 0) {
						if (str <= end)
							*str = *sw;
						++str;
						++sw;
					}
				} else {
					for (i = 0; pus->Buffer[i] && i < pus->Length; i++) {
						if (str <= end)
							*str = (wchar_t)(pus->Buffer[i]);
						++str;
					}
				}
			} else {
				/* print counted unicode string */
				PUNICODE_STRING pus = va_arg(args, PUNICODE_STRING);
				if ((pus == NULL) || (pus->Buffer == NULL)) {
					sw = L"<NULL>";
					while ((*sw) != 0) {
						if (str <= end)
							*str = *sw;
						++str;
						++sw;
					}
				} else {
					for (i = 0; pus->Buffer[i] && i < pus->Length / sizeof(WCHAR); i++) {
						if (str <= end)
							*str = pus->Buffer[i];
						++str;
					}
				}
			}
			continue;

		case L'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			str = number(str, end,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;

		case L'n':
			/* FIXME: What does C99 say about the overflow case here? */
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
			if (*fmt != L'%') {
				if (str <= end)
					*str = L'%';
				++str;
			}
			if (*fmt) {
				if (str <= end)
					*str = *fmt;
				++str;
			} else
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
		str = number(str, end, num, base, field_width, precision, flags);
	}
	if (str <= end)
		*str = L'\0';
	else if (cnt > 0)
		/* don't write out a null byte if the buf size is zero */
		*end = L'\0';
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

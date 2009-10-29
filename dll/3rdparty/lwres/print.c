/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001, 2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: print.c,v 1.10 2007/06/19 23:47:22 tbox Exp $ */

#include <config.h>

#include <ctype.h>
#include <stdio.h>		/* for sprintf */
#include <string.h>

#define	LWRES__PRINT_SOURCE	/* Used to get the lwres_print_* prototypes. */

#include <lwres/stdlib.h>

#include "assert_p.h"
#include "print_p.h"

#define LWRES_PRINT_QUADFORMAT LWRES_PLATFORM_QUADFORMAT

int
lwres__print_sprintf(char *str, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vsprintf(str, format, ap);
	va_end(ap);
	return (strlen(str));
}

/*
 * Return length of string that would have been written if not truncated.
 */

int
lwres__print_snprintf(char *str, size_t size, const char *format, ...) {
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);
	return (ret);

}

/*
 * Return length of string that would have been written if not truncated.
 */

int
lwres__print_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
	int h;
	int l;
	int q;
	int alt;
	int zero;
	int left;
	int plus;
	int space;
	long long tmpi;
	unsigned long long tmpui;
	unsigned long width;
	unsigned long precision;
	unsigned int length;
	char buf[1024];
	char c;
	void *v;
	char *save = str;
	const char *cp;
	const char *head;
	int count = 0;
	int pad;
	int zeropad;
	int dot;
	double dbl;
#ifdef HAVE_LONG_DOUBLE
	long double ldbl;
#endif
	char fmt[32];

	INSIST(str != NULL);
	INSIST(format != NULL);

	while (*format != '\0') {
		if (*format != '%') {
			if (size > 1U) {
				*str++ = *format;
				size--;
			}
			count++;
			format++;
			continue;
		}
		format++;

		/*
		 * Reset flags.
		 */
		dot = space = plus = left = zero = alt = h = l = q = 0;
		width = precision = 0;
		head = "";
		length = pad = zeropad = 0;

		do {
			if (*format == '#') {
				alt = 1;
				format++;
			} else if (*format == '-') {
				left = 1;
				zero = 0;
				format++;
			} else if (*format == ' ') {
				if (!plus)
					space = 1;
				format++;
			} else if (*format == '+') {
				plus = 1;
				space = 0;
				format++;
			} else if (*format == '0') {
				if (!left)
					zero = 1;
				format++;
			} else
				break;
		} while (1);

		/*
		 * Width.
		 */
		if (*format == '*') {
			width = va_arg(ap, int);
			format++;
		} else if (isdigit((unsigned char)*format)) {
			char *e;
			width = strtoul(format, &e, 10);
			format = e;
		}

		/*
		 * Precision.
		 */
		if (*format == '.') {
			format++;
			dot = 1;
			if (*format == '*') {
				precision = va_arg(ap, int);
				format++;
			} else if (isdigit((unsigned char)*format)) {
				char *e;
				precision = strtoul(format, &e, 10);
				format = e;
			}
		}

		switch (*format) {
		case '\0':
			continue;
		case '%':
			if (size > 1U) {
				*str++ = *format;
				size--;
			}
			count++;
			break;
		case 'q':
			q = 1;
			format++;
			goto doint;
		case 'h':
			h = 1;
			format++;
			goto doint;
		case 'l':
			l = 1;
			format++;
			if (*format == 'l') {
				q = 1;
				format++;
			}
			goto doint;
		case 'n':
		case 'i':
		case 'd':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		doint:
			if (precision != 0U)
				zero = 0;
			switch (*format) {
			case 'n':
				if (h) {
					short int *p;
					p = va_arg(ap, short *);
					REQUIRE(p != NULL);
					*p = str - save;
				} else if (l) {
					long int *p;
					p = va_arg(ap, long *);
					REQUIRE(p != NULL);
					*p = str - save;
				} else {
					int *p;
					p = va_arg(ap, int *);
					REQUIRE(p != NULL);
					*p = str - save;
				}
				break;
			case 'i':
			case 'd':
				if (q)
					tmpi = va_arg(ap, long long int);
				else if (l)
					tmpi = va_arg(ap, long int);
				else
					tmpi = va_arg(ap, int);
				if (tmpi < 0) {
					head = "-";
					tmpui = -tmpi;
				} else {
					if (plus)
						head = "+";
					else if (space)
						head = " ";
					else
						head = "";
					tmpui = tmpi;
				}
				sprintf(buf, "%" LWRES_PRINT_QUADFORMAT "u",
					tmpui);
				goto printint;
			case 'o':
				if (q)
					tmpui = va_arg(ap,
						       unsigned long long int);
				else if (l)
					tmpui = va_arg(ap, long int);
				else
					tmpui = va_arg(ap, int);
				sprintf(buf,
					alt ? "%#" LWRES_PRINT_QUADFORMAT "o"
					    : "%" LWRES_PRINT_QUADFORMAT "o",
					tmpui);
				goto printint;
			case 'u':
				if (q)
					tmpui = va_arg(ap,
						       unsigned long long int);
				else if (l)
					tmpui = va_arg(ap, unsigned long int);
				else
					tmpui = va_arg(ap, unsigned int);
				sprintf(buf, "%" LWRES_PRINT_QUADFORMAT "u",
					tmpui);
				goto printint;
			case 'x':
				if (q)
					tmpui = va_arg(ap,
						       unsigned long long int);
				else if (l)
					tmpui = va_arg(ap, unsigned long int);
				else
					tmpui = va_arg(ap, unsigned int);
				if (alt) {
					head = "0x";
					if (precision > 2U)
						precision -= 2;
				}
				sprintf(buf, "%" LWRES_PRINT_QUADFORMAT "x",
					tmpui);
				goto printint;
			case 'X':
				if (q)
					tmpui = va_arg(ap,
						       unsigned long long int);
				else if (l)
					tmpui = va_arg(ap, unsigned long int);
				else
					tmpui = va_arg(ap, unsigned int);
				if (alt) {
					head = "0X";
					if (precision > 2U)
						precision -= 2;
				}
				sprintf(buf, "%" LWRES_PRINT_QUADFORMAT "X",
					tmpui);
				goto printint;
			printint:
				if (precision != 0U || width != 0U) {
					length = strlen(buf);
					if (length < precision)
						zeropad = precision - length;
					else if (length < width && zero)
						zeropad = width - length;
					if (width != 0U) {
						pad = width - length -
						      zeropad - strlen(head);
						if (pad < 0)
							pad = 0;
					}
				}
				count += strlen(head) + strlen(buf) + pad +
					 zeropad;
				if (!left) {
					while (pad > 0 && size > 1U) {
						*str++ = ' ';
						size--;
						pad--;
					}
				}
				cp = head;
				while (*cp != '\0' && size > 1U) {
					*str++ = *cp++;
					size--;
				}
				while (zeropad > 0 && size > 1U) {
					*str++ = '0';
					size--;
					zeropad--;
				}
				cp = buf;
				while (*cp != '\0' && size > 1U) {
					*str++ = *cp++;
					size--;
				}
				while (pad > 0 && size > 1U) {
					*str++ = ' ';
					size--;
					pad--;
				}
				break;
			default:
				break;
			}
			break;
		case 's':
			cp = va_arg(ap, char *);
			REQUIRE(cp != NULL);

			if (precision != 0U) {
				/*
				 * cp need not be NULL terminated.
				 */
				const char *tp;
				unsigned long n;

				n = precision;
				tp = cp;
				while (n != 0U && *tp != '\0')
					n--, tp++;
				length = precision - n;
			} else {
				length = strlen(cp);
			}
			if (width != 0U) {
				pad = width - length;
				if (pad < 0)
					pad = 0;
			}
			count += pad + length;
			if (!left)
				while (pad > 0 && size > 1U) {
					*str++ = ' ';
					size--;
					pad--;
				}
			if (precision != 0U)
				while (precision > 0U && *cp != '\0' &&
				       size > 1U) {
					*str++ = *cp++;
					size--;
					precision--;
				}
			else
				while (*cp != '\0' && size > 1U) {
					*str++ = *cp++;
					size--;
				}
			while (pad > 0 && size > 1U) {
				*str++ = ' ';
				size--;
				pad--;
			}
			break;
		case 'c':
			c = va_arg(ap, int);
			if (width > 0U) {
				count += width;
				width--;
				if (left) {
					*str++ = c;
					size--;
				}
				while (width-- > 0U && size > 1U) {
					*str++ = ' ';
					size--;
				}
				if (!left && size > 1U) {
					*str++ = c;
					size--;
				}
			} else {
				count++;
				if (size > 1U) {
					*str++ = c;
					size--;
				}
			}
			break;
		case 'p':
			v = va_arg(ap, void *);
			sprintf(buf, "%p", v);
			length = strlen(buf);
			if (precision > length)
				zeropad = precision - length;
			if (width > 0U) {
				pad = width - length - zeropad;
				if (pad < 0)
					pad = 0;
			}
			count += length + pad + zeropad;
			if (!left)
				while (pad > 0 && size > 1U) {
					*str++ = ' ';
					size--;
					pad--;
				}
			cp = buf;
			if (zeropad > 0 && buf[0] == '0' &&
			    (buf[1] == 'x' || buf[1] == 'X')) {
				if (size > 1U) {
					*str++ = *cp++;
					size--;
				}
				if (size > 1U) {
					*str++ = *cp++;
					size--;
				}
				while (zeropad > 0 && size > 1U) {
					*str++ = '0';
					size--;
					zeropad--;
				}
			}
			while (*cp != '\0' && size > 1U) {
				*str++ = *cp++;
				size--;
			}
			while (pad > 0 && size > 1U) {
				*str++ = ' ';
				size--;
				pad--;
			}
			break;
		case 'D':	/*deprecated*/
			INSIST("use %ld instead of %D" == NULL);
		case 'O':	/*deprecated*/
			INSIST("use %lo instead of %O" == NULL);
		case 'U':	/*deprecated*/
			INSIST("use %lu instead of %U" == NULL);

		case 'L':
#ifdef HAVE_LONG_DOUBLE
			l = 1;
#else
			INSIST("long doubles are not supported" == NULL);
#endif
			/*FALLTHROUGH*/
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			if (!dot)
				precision = 6;
			/*
			 * IEEE floating point.
			 * MIN 2.2250738585072014E-308
			 * MAX 1.7976931348623157E+308
			 * VAX floating point has a smaller range than IEEE.
			 *
			 * precisions > 324 don't make much sense.
			 * if we cap the precision at 512 we will not
			 * overflow buf.
			 */
			if (precision > 512U)
				precision = 512;
			sprintf(fmt, "%%%s%s.%lu%s%c", alt ? "#" : "",
				plus ? "+" : space ? " " : "",
				precision, l ? "L" : "", *format);
			switch (*format) {
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
#ifdef HAVE_LONG_DOUBLE
				if (l) {
					ldbl = va_arg(ap, long double);
					sprintf(buf, fmt, ldbl);
				} else
#endif
				{
					dbl = va_arg(ap, double);
					sprintf(buf, fmt, dbl);
				}
				length = strlen(buf);
				if (width > 0U) {
					pad = width - length;
					if (pad < 0)
						pad = 0;
				}
				count += length + pad;
				if (!left)
					while (pad > 0 && size > 1U) {
						*str++ = ' ';
						size--;
						pad--;
					}
				cp = buf;
				while (*cp != ' ' && size > 1U) {
					*str++ = *cp++;
					size--;
				}
				while (pad > 0 && size > 1U) {
					*str++ = ' ';
					size--;
					pad--;
				}
				break;
			default:
				continue;
			}
			break;
		default:
			continue;
		}
		format++;
	}
	if (size > 0U)
		*str = '\0';
	return (count);
}

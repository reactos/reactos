/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <stdarg.h>
#include <crtdll/malloc.h>
#include <crtdll/internal/file.h>

int _isnanl(double x);
int _isinfl(double x);
int _isnan(double x);
int _isinf(double x);



int
__vfprintf (FILE *fp, const char *fmt0, va_list argp);

int
vfprintf(FILE *f, const char *fmt, va_list ap)
{
  int len;
  char localbuf[BUFSIZ];

  if (f->_flag & _IONBF)
  {
    f->_flag &= ~_IONBF;
    f->_ptr = f->_base = localbuf;
    f->_bufsiz = BUFSIZ;
    len = __vfprintf(f,fmt, ap);
    (void)fflush(f);
    f->_flag |= _IONBF;
    f->_base = NULL;
    f->_bufsiz = 0;
    f->_cnt = 0;
  }
  else
    len = __vfprintf(f,fmt, ap);
  return (ferror(f) ? EOF : len);
}


/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

/*
 * Appropiated for the reactos kernel, March 1998 -- David Welch
 */

#include <stdarg.h>

#include <crtdll/ctype.h>
#include <crtdll/string.h>
#include <crtdll/stdio.h>
#include <crtdll/string.h>
#include <crtdll/math.h>
#include <crtdll/internal/ieee.h>


#define ZEROPAD		1	/* pad with zero */
#define SIGN		2	/* unsigned/signed long */
#define PLUS		4	/* show plus */
#define SPACE		8	/* space if plus */
#define LEFT		16	/* left justified */
#define SPECIAL		32	/* 0x */
#define LARGE		64	/* use 'ABCDEF' instead of 'abcdef' */
#define ZEROTRUNC	128	/* truncate zero 's */


static int skip_atoi(const char **s)
{
	int i=0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}


static int do_div(long long *n,int base)
{
	int __res = ((unsigned long long) *n) % (unsigned) base;
	*n = ((unsigned long long) *n) / (unsigned) base;
	return __res;
}


static void number(FILE * f, long long num, int base, int size, int precision ,int type)
{
	char c,sign,tmp[66];
	const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
	int i;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
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
		tmp[i++]='0';
	else while (num != 0)
		tmp[i++] = digits[do_div(&num,base)];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
			putc(' ',f);
	if (sign)
		putc(sign,f);
	if (type & SPECIAL) {
		if (base==8) {
			putc('0',f);
		}
		else if (base==16) {
			putc('0', f);
			putc(digits[33],f);
		}
	}
	if (!(type & LEFT))
		while (size-- > 0)
			putc(c,f);
	while (i < precision--)
		putc('0', f);
	while (i-- > 0)
		putc(tmp[i],f);
	while (size-- > 0)
		putc(' ', f);
	return;
}


static void numberf(FILE * f, double __n, char exp_sign,  int size, int precision, int type)
{
	double exponent = 0.0;
	double e;
	long ie;

	//int x;
	char *buf, *tmp;
	int i = 0;
	int j = 0;
	//int k = 0;

	double frac, intr;
	double p;
	char sign;
	char c;
	char ro = 0;

	double_t *n = (double_t *)&__n;

	if ( exp_sign == 'g' || exp_sign == 'G' || exp_sign == 'e' || exp_sign == 'E' ) {
		ie = ((unsigned int)n->exponent - (unsigned int)0x3ff);
		exponent = ie/3.321928;
	}

	if ( exp_sign == 'g' || exp_sign == 'G' ) {
		type |= ZEROTRUNC;
		if ( exponent < -4 || fabs(exponent) >= precision )
			 exp_sign -= 2; // g -> e and G -> E
	}

	if ( exp_sign == 'e' ||  exp_sign == 'E' ) {
		frac = modf(exponent,&e);
		if ( frac > 0.5 )
			e++;
		else if (  frac < -0.5  )
			e--;

		numberf(f,__n/pow(10.0L,e),'f',size-4, precision, type);
		putc( exp_sign,f);
		size--;
		ie = (long)e;
		type = LEFT | PLUS;
		if ( ie < 0 )
			type |= SIGN;

		number(f,ie, 10,2, 2,type );
		return;
	}

	if ( exp_sign == 'f' ) {
		buf = alloca(4096);
		if (type & LEFT) {
			type &= ~ZEROPAD;
		}

		c = (type & ZEROPAD) ? '0' : ' ';
		sign = 0;
		if (type & SIGN) {
			if (__n < 0) {
				sign = '-';
				__n = fabs(__n);
				size--;
			} else if (type & PLUS) {
				sign = '+';
				size--;
			} else if (type & SPACE) {
				sign = ' ';
				size--;
			}
		}

		frac = modf(__n,&intr);

		// # flags forces a . and prevents trucation of trailing zero's

		if ( precision > 0 ) {
			//frac = modfl(__n,&intr);
			i = precision-1;
			while (  i >= 0  ) {
				frac*=10.0L;
				frac = modf(frac, &p);
				buf[i] = (int)p + '0';
				i--;
			}
			i = precision;
			size -= precision;

			ro = 0;
			if ( frac > 0.5 ) {
				ro = 1;
			}

			if ( precision >= 1 || type & SPECIAL) {
				buf[i++] = '.';
				size--;
			}
		}

		if ( intr == 0.0 ) {
			buf[i++] = '0';
			size--;
		}
		else {
			while ( intr > 0.0 ) {
				intr/=10.0L;
				p = modf(intr, &intr);

				p *=10;

				buf[i++] = (int)p + '0';
				size--;
			}
		}

		j = 0;
		while ( j < i && ro == 1) {
			if ( buf[j] >= '0' && buf[j] <= '8' ) {
				buf[j]++;
				ro = 0;
			}
			else if ( buf[j] == '9' ) {
				buf[j] = '0';
			}
			j++;
		}
		if ( ro == 1 )
			buf[i++] = '1';

		buf[i] = 0;

		size -= precision;
		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
				putc(' ',f);
		if (sign)
			putc( sign,f);

		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
				putc(' ',f);
		if (type & SPECIAL) {
		}

		if (!(type & LEFT))
			while (size-- > 0)
				putc(c,f);

		tmp = buf;
		if ( type & ZEROTRUNC && ((type & SPECIAL) != SPECIAL) ) {
			j = 0;
			while ( j < i && ( *tmp == '0' || *tmp == '.' )) {
					tmp++;
					i--;
			}
		}
//		else
//			while (i < precision--)
//				putc('0', f);
		while (i-- > 0)
			putc(tmp[i],f);
		while (size-- > 0)
			putc(' ', f);
	}
}


static void numberfl(FILE * f, long double __n, char exp_sign,  int size, int precision, int type)
{
	long double exponent = 0.0;
	long double e;
	long ie;

	//int x;
	char *buf, *tmp;
	int i = 0;
	int j = 0;
	//int k = 0;

	long double frac, intr;
	long double p;
	char sign;
	char c;
	char ro = 0;

	long_double_t *n = (long_double_t *)&__n;

	if ( exp_sign == 'g' || exp_sign == 'G' || exp_sign == 'e' || exp_sign == 'E' ) {
		ie = ((unsigned int)n->exponent - (unsigned int)0x3fff);
		exponent = ie/3.321928;
	}

	if ( exp_sign == 'g' || exp_sign == 'G' ) {
		type |= ZEROTRUNC;
		if ( exponent < -4 || fabs(exponent) >= precision ) 
			 exp_sign -= 2; // g -> e and G -> E
	}

	if ( exp_sign == 'e' || exp_sign == 'E' ) {
		frac = modfl(exponent,&e);
		if ( frac > 0.5 )
			e++;
		else if ( frac < -0.5 )
			e--;

		numberf(f,__n/powl(10.0L,e),'f',size-4, precision, type);
		putc( exp_sign,f);
		size--;
		ie = (long)e;
		type = LEFT | PLUS;
		if ( ie < 0 )
			type |= SIGN;

		number(f,ie, 10,2, 2,type );
		return;
	}

	if ( exp_sign == 'f' ) {
		
		buf = alloca(4096);
		if (type & LEFT) {
			type &= ~ZEROPAD;
		}

		c = (type & ZEROPAD) ? '0' : ' ';
		sign = 0;
		if (type & SIGN) {
			if (__n < 0) {
				sign = '-';
				__n = fabs(__n);
				size--;
			} else if (type & PLUS) {
				sign = '+';
				size--;
			} else if (type & SPACE) {
				sign = ' ';
				size--;
			}
		}

		frac = modfl(__n,&intr);

		// # flags forces a . and prevents trucation of trailing zero's
		if ( precision > 0 ) {
			//frac = modfl(__n,&intr);
	
			i = precision-1;
			while ( i >= 0  ) {
				frac*=10.0L;
				frac = modfl((long double)frac, &p);
				buf[i] = (int)p + '0';
				i--;
			}
			i = precision;
			size -= precision;

			ro = 0;
			if ( frac > 0.5 ) {
				ro = 1;
			}

			if ( precision >= 1 || type & SPECIAL) {
				buf[i++] = '.';
				size--;
			}
		}

		if ( intr == 0.0 ) {
			buf[i++] = '0';
			size--;
		}
		else {
			while ( intr > 0.0 ) {
				intr/=10.0L;
				p = modfl(intr, &intr);

				p *=10;

				buf[i++] = (int)p + '0';
				size--;
			}
		}

		j = 0;
		while ( j < i && ro == 1) {
			if ( buf[j] >= '0' && buf[j] <= '8' ) {
				buf[j]++;
				ro = 0;
			}
			else if ( buf[j] == '9' ) {
				buf[j] = '0';
			}
			j++;
		}
		if ( ro == 1 )
			buf[i++] = '1';

		buf[i] = 0;

		size -= precision;
		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
				putc(' ',f);
		if (sign)
			putc(sign,f);

		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
				putc(' ',f);
		if (type & SPECIAL) {
		}

		if (!(type & LEFT))
			while (size-- > 0)
				putc(c,f);

		tmp = buf;
		if ( type & ZEROTRUNC && ((type & SPECIAL) != SPECIAL) ) {
			j = 0;
			while ( j < i && ( *tmp == '0' || *tmp == '.' )) {
				tmp++;
				i--;
			}
		}
//		else
//			while (i < precision--)
//				    putc( '0', f);
		while (i-- > 0)
			putc(tmp[i],f);
		while (size-- > 0)
			putc(' ', f);
	}
}


int __vfprintf(FILE *f, const char *fmt, va_list args)
{
	int len;
	unsigned long long num;
	int i, base;
	long double _ldouble;
	double _double;
	const char *s;
	const short int* sw;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier = 0;	/* 'h', 'l', 'L' or 'I64' for integer fields */

	for (; *fmt ; ++fmt) {
		if (*fmt != '%') {
			putc(*fmt,f);
			continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
				}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
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
		if (*fmt == '.') {
			++fmt;
			if (isdigit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		// %Z can be just stand alone or as size_t qualifier
		if ( *fmt == 'Z' ) {
			qualifier = *fmt;
			switch ( *(fmt+1)) {
				case 'o':
				case 'b':
				case 'X':
				case 'x':
				case 'd':
				case 'i':
				case 'u':
					++fmt;
					break;
				default:
					break;
			}
		} else if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'w') {
			qualifier = *fmt;
			++fmt;
		} else if (*fmt == 'I' && *(fmt+1) == '6' && *(fmt+2) == '4') {
			qualifier = *fmt;
			fmt += 3;
		}

		// go fine with ll instead of L
		if ( *fmt == 'l' ) {
			++fmt;
			qualifier = 'L';
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					putc(' ', f);
			if (qualifier == 'l' || qualifier == 'w')
				putc((unsigned char)(wchar_t) va_arg(args, int), f);
			else
				putc((unsigned char) va_arg(args, int), f);
			while (--field_width > 0)
				putc(' ', f);
			continue;

		case 'C':
			if (!(flags & LEFT))
				while (--field_width > 0)
					putc(' ', f);
			if (qualifier == 'h')
				putc((unsigned char) va_arg(args, int), f);
			else
				putc((unsigned char)(wchar_t) va_arg(args, int), f);
			while (--field_width > 0)
				putc(' ', f);
			continue;

		case 's':
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
						putc(' ', f);
				for (i = 0; i < len; ++i)
					putc((unsigned char)(*sw++), f);
				while (len < field_width--)
					putc(' ', f);
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
						putc(' ', f);
				for (i = 0; i < len; ++i)
					putc(*s++, f);
				while (len < field_width--)
					putc(' ', f);
			}
			continue;

		case 'S':
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
						putc(' ', f);
				for (i = 0; i < len; ++i)
					putc(*s++, f);
				while (len < field_width--)
					putc(' ', f);
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
						putc(' ', f);
				for (i = 0; i < len; ++i)
					putc((unsigned char)(*sw++), f);
				while (len < field_width--)
					putc(' ', f);
			}
			continue;

#if 0
		case 'Z':
			if (qualifier == 'w') {
				/* print counted unicode string */
				PUNICODE_STRING pus = va_arg(args, PUNICODE_STRING);
				if ((pus == NULL) || (pus->Buffer)) {
					s = "<NULL>";
					while ((*s) != 0)
						putc(*s++, f);
				} else {
					for (i = 0; pus->Buffer[i] && i < pus->Length; i++)
						putc((unsigned char)(pus->Buffer[i]), f);
				}
			} else {
				/* print counted ascii string */
				PANSI_STRING pus = va_arg(args, PANSI_STRING);
				if ((pus == NULL) || (pus->Buffer)) {
					s = "<NULL>";
					while ((*s) != 0)
						putc(*s++, f);
				} else {
					for (i = 0; pus->Buffer[i] && i < pus->Length; i++)
						putc(pus->Buffer[i], f);
				}
			}
			continue;
#endif

		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			if (qualifier == 'l' || qualifier == 'L' ) {
				_ldouble = va_arg(args, long double);
			
				if ( _isnanl(_ldouble) ) {
					s = "Nan";
					len = 3;
					while ( len > 0 ) {
						putc(*s++,f);
						len --;
					}
				}
				else if ( _isinfl(_ldouble) < 0 ) {
					s = "-Inf";
					len = 4;
					while ( len > 0 ) {
						putc(*s++,f);
						len --;
					}
				}
				else if ( _isinfl(_ldouble) > 0 ) {
					s = "+Inf";
					len = 4;
					while ( len > 0 ) {
						putc(*s++,f);
						len --;
					}
				} else {
					if ( precision == -1 )
						precision = 6;
					numberfl(f,_ldouble,*fmt,field_width,precision,flags);
				}
			} else {
				_double = (double)va_arg(args, double);

				if ( _isnan(_double) ) {
					s = "Nan";
					len = 3;
					while ( len > 0 ) {
						putc(*s++,f);
						len --;
					}
				} else if ( _isinf(_double) < 0 ) {
					s = "-Inf";
					len = 4;
					while ( len > 0 ) {
						putc(*s++,f);
						len --;
					}
				} else if ( _isinf(_double) > 0 ) {
					s = "+Inf";
					len = 4;
					while ( len > 0 ) {
						putc(*s++,f);
						len --;
					}
				} else {
					if ( precision == -1 )
						precision = 6;
					numberf(f,_double,*fmt,field_width,precision,flags);
				}
			}
			continue;

		case 'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			number(f,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;

		case 'n':
			if (qualifier == 'l') {
				long * ip = va_arg(args, long *);
				*ip = 0;
			} else {
				int * ip = va_arg(args, int *);
				*ip = 0;
			}
			continue;

		/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'b':
			base = 2;
			break;

		case 'X':
			flags |= LARGE;
		case 'x':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			break;

		default:
			if (*fmt != '%')
				putc('%', f);
			if (*fmt)
				putc(*fmt, f);
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
		else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);
		number(f, num, base, field_width, precision, flags);
	}
	//putc('\0',f);
	return 0;
}

/* EOF */

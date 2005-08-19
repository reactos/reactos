// ssprintf.cpp

#include <malloc.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include "ssprintf.h"

#ifdef _MSC_VER
#define alloca _alloca
#endif//_MSC_VER

typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

typedef struct {
    unsigned int mantissa:23;
    unsigned int exponent:8;
    unsigned int sign:1;
} ieee_float_t;

typedef struct {
    unsigned int mantissal:32;
    unsigned int mantissah:20;
    unsigned int exponent:11;
    unsigned int sign:1;
} ieee_double_t;

typedef struct {
    unsigned int mantissal:32;
    unsigned int mantissah:32;
    unsigned int exponent:15;
    unsigned int sign:1;
    unsigned int empty:16;
} ieee_long_double_t;

std::string ssprintf ( const char* fmt, ... )
{
	va_list arg;
	va_start(arg, fmt);
	std::string f = ssvprintf ( fmt, arg );
	va_end(arg);
	return f;
}

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


static int do_div(LONGLONG *n,int base)
{
	int __res = ((ULONGLONG) *n) % (unsigned) base;
	*n = ((ULONGLONG) *n) / (unsigned) base;
	return __res;
}


static bool number(std::string& f, LONGLONG num, int base, int size, int precision ,int type)
{
	char c,sign,tmp[66];
	const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
	int i;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
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
			f += ' ';
	if (sign)
		f += sign;
	if (type & SPECIAL)
	{
		if (base==8)
			f += '0';
		else if (base==16)
		{
			f += '0';
			f += digits[33];
		}
	}
	if (!(type & LEFT))
	{
		while (size-- > 0)
			f += c;
	}
	while (i < precision--)
	{
		f += '0';
	}
	while (i-- > 0)
	{
		f += tmp[i];
	}
	while (size-- > 0)
	{
		f += ' ';
	}
	return true;
}


static bool numberf(std::string& f, double __n, char exp_sign,  int size, int precision, int type)
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
	int result;

	union
	{
		double*  __n;
		ieee_double_t*  n;
	} n;
	
	n.__n = &__n;

	if ( exp_sign == 'g' || exp_sign == 'G' || exp_sign == 'e' || exp_sign == 'E' ) {
		ie = ((unsigned int)n.n->exponent - (unsigned int)0x3ff);
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

		result = numberf(f,__n/pow(10.0L,e),'f',size-4, precision, type);
		if (result < 0)
			return false;
		f += exp_sign;
		size--;
		ie = (long)e;
		type = LEFT | PLUS;
		if ( ie < 0 )
			type |= SIGN;

		result = number(f,ie, 10,2, 2,type );
		if (result < 0)
			return false;
		return true;
	}

	if ( exp_sign == 'f' ) {
		buf = (char*)alloca(4096);
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
			        p = intr;
				intr/=10.0L;
				modf(intr, &intr);

				p -= 10.0*intr;

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
		{
			while(size-->0)
				f += ' ';
		}
		if (sign)
		{
			f += sign;
		}

		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
			{
				f += ' ';
			}
		if (type & SPECIAL) {
		}

		if (!(type & LEFT))
			while (size-- > 0)
			{
				f += c;
			}

		tmp = buf;
		if ( type & ZEROTRUNC && ((type & SPECIAL) != SPECIAL) )
		{
			j = 0;
			while ( j < i && ( *tmp == '0' || *tmp == '.' ))
			{
				tmp++;
				i--;
			}
		}
//		else
//			while (i < precision--)
//				putc('0', f);
		while (i-- > 0)
		{
			f += tmp[i];
		}
		while (size-- > 0)
		{
			f += ' ';
		}
	}
	return true;
}


static bool numberfl(std::string& f, long double __n, char exp_sign,  int size, int precision, int type)
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

	int result;

	union
	{
	    long double*   __n;
	    ieee_long_double_t*   n;
	} n;

	n.__n = &__n;

	if ( exp_sign == 'g' || exp_sign == 'G' || exp_sign == 'e' || exp_sign == 'E' ) {
		ie = ((unsigned int)n.n->exponent - (unsigned int)0x3fff);
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

		result = numberf(f,__n/powl(10.0L,e),'f',size-4, precision, type);
		if (result < 0)
			return false;
		f += exp_sign;
		size--;
		ie = (long)e;
		type = LEFT | PLUS;
		if ( ie < 0 )
			type |= SIGN;

		result = number(f,ie, 10,2, 2,type );
		if (result < 0)
			return false;
		return true;
	}

	if ( exp_sign == 'f' )
	{
		
		buf = (char*)alloca(4096);
		if (type & LEFT)
		{
			type &= ~ZEROPAD;
		}

		c = (type & ZEROPAD) ? '0' : ' ';
		sign = 0;
		if (type & SIGN)
		{
			if (__n < 0)
			{
				sign = '-';
				__n = fabs(__n);
				size--;
			} else if (type & PLUS)
			{
				sign = '+';
				size--;
			} else if (type & SPACE)
			{
				sign = ' ';
				size--;
			}
		}

		frac = modfl(__n,&intr);

		// # flags forces a . and prevents trucation of trailing zero's
		if ( precision > 0 )
		{
			//frac = modfl(__n,&intr);

			i = precision-1;
			while ( i >= 0  )
			{
				frac*=10.0L;
				frac = modfl((long double)frac, &p);
				buf[i] = (int)p + '0';
				i--;
			}
			i = precision;
			size -= precision;

			ro = 0;
			if ( frac > 0.5 )
			{
				ro = 1;
			}

			if ( precision >= 1 || type & SPECIAL)
			{
				buf[i++] = '.';
				size--;
			}
		}

		if ( intr == 0.0 )
		{
			buf[i++] = '0';
			size--;
		}
		else
		{
			while ( intr > 0.0 )
			{
			        p=intr;
				intr/=10.0L;
				modfl(intr, &intr);

				p -= 10.0L*intr;

				buf[i++] = (int)p + '0';
				size--;
			}
		}

		j = 0;
		while ( j < i && ro == 1) {
			if ( buf[j] >= '0' && buf[j] <= '8' )
			{
				buf[j]++;
				ro = 0;
			}
			else if ( buf[j] == '9' )
			{
				buf[j] = '0';
			}
			j++;
		}
		if ( ro == 1 )
			buf[i++] = '1';

		buf[i] = 0;

		size -= precision;
		if (!(type&(ZEROPAD+LEFT)))
		{
			while(size-->0)
				f += ' ';
		}
		if (sign)
		{
			f += sign;
		}

		if (!(type&(ZEROPAD+LEFT)))
		{
			while(size-->0)
				f += ' ';
		}
		if (type & SPECIAL) {
		}

		if (!(type & LEFT))
			while (size-- > 0)
			{
				f += c;
			}
		tmp = buf;
		if ( type & ZEROTRUNC && ((type & SPECIAL) != SPECIAL) )
		{
			j = 0;
			while ( j < i && ( *tmp == '0' || *tmp == '.' ))
			{
				tmp++;
				i--;
			}
		}
//		else
//			while (i < precision--)
//				    putc( '0', f);
		while (i-- > 0)
		{
			f += tmp[i];
		}
		while (size-- > 0)
		{
			f += ' ';
		}
	}
	return true;
}

static int stringa(std::string& f, const char* s, int len, int field_width, int precision, int flags)
{
	int i, done = 0;
	if (s == NULL)
	{
		s = "<NULL>";
		len = 6;
	}
	else
	{
		if (len == -1)
		{
			len = 0;
			while ((unsigned int)len < (unsigned int)precision && s[len])
				len++;
		}
		else
		{
			if ((unsigned int)len > (unsigned int)precision)
				len = precision;
		}
	}
	if (!(flags & LEFT))
		while (len < field_width--)
		{
			f += ' ';
			done++;
		}
	for (i = 0; i < len; ++i)
	{
		f += *s++;
		done++;
	}
	while (len < field_width--)
	{
		f += ' ';
		done++;
	}
	return done;
}

static int stringw(std::string& f, const wchar_t* sw, int len, int field_width, int precision, int flags)
{
	int i, done = 0;
	if (sw == NULL)
	{
		sw = L"<NULL>";
		len = 6;
	}
	else
	{
		if (len == -1)
		{
			len = 0;
			while ((unsigned int)len < (unsigned int)precision && sw[len])
				len++;
		}
		else
		{
			if ((unsigned int)len > (unsigned int)precision)
				len = precision;
		}
	}
	if (!(flags & LEFT))
		while (len < field_width--)
		{
			f += ' ';
			done++;
		}
	for (i = 0; i < len; ++i)
	{
#define MY_MB_CUR_MAX 1
		char mb[MY_MB_CUR_MAX];
		int mbcount, j;
		mbcount = wctomb(mb, *sw++);
		if (mbcount <= 0)
		{
			break;
		}
		for (j = 0; j < mbcount; j++)
		{
			f += mb[j];
			done++;
		}
	}
	while (len < field_width--)
	{
		f += ' ';
		done++;
	}
	return done;
}

#define _isnanl _isnan
#define _finitel _finite

std::string ssvprintf ( const char *fmt, va_list args )
{
	ULONGLONG num;
	int base;
	long double _ldouble;
	double _double;
	const char *s;
	const wchar_t* sw;
	int result;
	std::string f;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier = 0;	/* 'h', 'l', 'L' or 'I64' for integer fields */

	for (; *fmt ; ++fmt)
	{
		if (*fmt != '%')
		{
			f += *fmt;
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
		qualifier = 0;
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
				{
					f += ' ';
				}
			if (qualifier == 'l' || qualifier == 'w')
			{
				f += (char)(unsigned char)(wchar_t) va_arg(args,int);
			}
			else
			{
				f += (char)(unsigned char) va_arg(args,int);
			}
			while (--field_width > 0)
			{
				f += ' ';
			}
			continue;

		case 'C':
			if (!(flags & LEFT))
				while (--field_width > 0)
				{
					f += ' ';
				}
			if (qualifier == 'h')
			{
				f += (char)(unsigned char) va_arg(args,int);
			}
			else
			{
				f += (char)(unsigned char)(wchar_t) va_arg(args,int);
			}
			while (--field_width > 0)
			{
				f += ' ';
			}
			continue;

		case 's':
			if (qualifier == 'l' || qualifier == 'w') {
				/* print unicode string */
				sw = (const wchar_t*)va_arg(args, wchar_t *);
				result = stringw(f, sw, -1, field_width, precision, flags);
			} else {
				/* print ascii string */
				s = va_arg(args, char *);
				result = stringa(f, s, -1, field_width, precision, flags);
			}
			if (result < 0)
			{
				assert(!"TODO FIXME handle error better");
				return f;
			}
			continue;

		case 'S':
			if (qualifier == 'h') {
				/* print ascii string */
				s = va_arg(args, char *);
				result = stringa(f, s, -1, field_width, precision, flags);
			} else {
				/* print unicode string */
				sw = (const wchar_t*)va_arg(args, wchar_t *);
				result = stringw(f, sw, -1, field_width, precision, flags);
			}
			if (result < 0)
			{
				assert(!"TODO FIXME handle error better");
				return f;
			}
			continue;

		/*case 'Z':
			if (qualifier == 'w') {
				// print counted unicode string
				PUNICODE_STRING pus = va_arg(args, PUNICODE_STRING);
				if ((pus == NULL) || (pus->Buffer == NULL)) {
					sw = NULL;
					len = -1;
				} else {
					sw = pus->Buffer;
					len = pus->Length / sizeof(WCHAR);
				}
				result = stringw(f, sw, len, field_width, precision, flags);
			} else {
				// print counted ascii string
				PANSI_STRING pas = va_arg(args, PANSI_STRING);
				if ((pas == NULL) || (pas->Buffer == NULL)) {
					s = NULL;
					len = -1;
				} else {
					s = pas->Buffer;
					len = pas->Length;
				}
				result = stringa(f, s, -1, field_width, precision, flags);
			}
			if (result < 0)
				return -1;
			continue;*/

		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			if (qualifier == 'l' || qualifier == 'L' ) {
				_ldouble = va_arg(args, long double);
			
				if ( _isnanl(_ldouble) )
				{
					f += "Nan";
				}
				else if ( !_finitel(_ldouble) )
				{
					if ( _ldouble < 0 )
						f += "-Inf";
					else
						f += "+Inf";
				} else {
					if ( precision == -1 )
						precision = 6;
					result = numberfl(f,_ldouble,*fmt,field_width,precision,flags);
					if (result < 0)
					{
						assert(!"TODO FIXME handle error better");
						return f;
					}
				}
			} else {
				_double = (double)va_arg(args, double);

				if ( _isnan(_double) )
				{
					f += "Nan";
				}
				else if ( !_finite(_double) )
				{
					if ( _double < 0 )
						f += "-Inf";
					else
						f += "+Inf";
				}
				else
				{
					if ( precision == -1 )
						precision = 6;
					result = numberf(f,_double,*fmt,field_width,precision,flags);
					if (result < 0)
					{
						assert(!"TODO FIXME handle error better");
						return f;
					}
				}
			}
			continue;

		case 'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			result = number(f,
				        (unsigned long) va_arg(args, void *), 16,
					field_width, precision, flags);
			if (result < 0)
			{
				assert(!"TODO FIXME handle error better");
				return f;
			}
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
			{
				f += '%';
			}
			if (*fmt)
			{
				f += *fmt;
			}
			else
				--fmt;
			continue;
		}

		if (qualifier == 'I')
			num = va_arg(args, ULONGLONG);
		else if (qualifier == 'l') {
			if (flags & SIGN)
				num = va_arg(args, long);
			else
				num = va_arg(args, unsigned long);
		}
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
		result = number(f, num, base, field_width, precision, flags);
		if (result < 0)
		{
			assert(!"TODO FIXME handle error better");
			return f;
		}
	}
	//putc('\0',f);
	return f;
}

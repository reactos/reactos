/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <stdarg.h>
#include <crtdll/internal/file.h>

#define isascii __isascii



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


/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
//#include <libc/stubs.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
//#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <libc/file.h>
//#include <libc/stdiohk.h>
//#include <libc/local.h>

static char decimal = '.';

/* 11-bit exponent (VAX G floating point) is 308 decimal digits */
#define	MAXEXP		308
#define MAXEXPLD        4952 /* this includes subnormal numbers */
/* 128 bit fraction takes up 39 decimal digits; max reasonable precision */
#define	MAXFRACT	39

#define	DEFPREC		6
#define	DEFLPREC	6

#define	BUF		(MAXEXPLD+MAXFRACT+1)	/* + decimal point */

#define	PUTC(ch)	(void) putc(ch, fp)

// adjustedn llong remove from first
#define ARG(basetype) _ulonglong = \
		flags&LONGDBL ? va_arg(argp, basetype) : \
		flags&LONGINT ? va_arg(argp, long basetype) : \
		flags&SHORTINT ? (short basetype)va_arg(argp, int) : \
		va_arg(argp, int)

// changed name of nan due to conflict with cygwin math header
static int _nan = 0;

#define MODFL modf
typedef double ldouble;
typedef long llong;
typedef unsigned long ullong;

ldouble modfl(ldouble x,ldouble *pint);

static __inline__ int todigit(char c)
{
  if (c<='0') return 0;
  if (c>='9') return 9;
  return c-'0';
}
static __inline__ char tochar(int n)
{
  if (n>=9) return '9';
  if (n<=0) return '0';
  return n+'0';
}

/* have to deal with the negative buffer count kludge */

#define	LONGINT		0x01		/* long integer */
#define	LONGDBL		0x02		/* ldouble */
#define	SHORTINT	0x04		/* short integer */
#define	ALT		0x08		/* alternate form */
#define	LADJUST		0x10		/* left adjustment */
#define	ZEROPAD		0x20		/* zero (as opposed to blank) pad */
#define	HEXPREFIX	0x40		/* add 0x or 0X prefix */

static int cvtl(ldouble number, int prec, int flags, char *signp,
	        unsigned char fmtch, char *startp, char *endp);
static char *roundl(ldouble fract, int *expv, char *start, char *end,
		    char ch, char *signp);
static char *exponentl(char *p, int expv, unsigned char fmtch);
//#ifdef __GO32__
static int isspeciall(ldouble d, char *bufp);
//#endif

static char NULL_REP[] = "(null)";

int
__vfprintf (FILE *fp, const char *fmt0, va_list argp)
{
  const char *fmt;		/* format string */
  int ch;			/* character from fmt */
  int cnt;			/* return value accumulator */
  int n;			/* random handy integer */
  char *t;			/* buffer pointer */
  ldouble _ldouble;		/* double and ldouble precision arguments
				   %L.[eEfgG] */
  ullong _ulonglong=0; /* integer arguments %[diouxX] */
  int base;			/* base for [diouxX] conversion */
  int dprec;			/* decimal precision in [diouxX] */
  int fieldsz;			/* field size expanded by sign, etc */
  int flags;			/* flags as above */
  int fpprec = 0;			/* `extra' floating precision in [eEfgG] */
  int prec = 0;			/* precision from format (%.3d), or -1 */
  int realsz;			/* field size expanded by decimal precision */
  int size = 0;			/* size of converted field or string */
  int width;			/* width from format (%8d), or 0 */
  char sign;			/* sign prefix (' ', '+', '-', or \0) */
  char softsign;		/* temporary negative sign for floats */
  const char *digs;		/* digits for [diouxX] conversion */
  char buf[BUF];		/* space for %c, %[diouxX], %[eEfgG] */
  int neg_ldouble = 0;		/* non-zero if _ldouble is negative */

  //decimal = localeconv()->decimal_point[0];
  decimal = '.';

  if (fp->_flag & _IORW)
  {
    fp->_flag |= _IOWRT;
    fp->_flag &= ~(_IOEOF|_IOREAD);
  }
  if ((fp->_flag & _IOWRT) == 0)
    return (EOF);

  fmt = fmt0;
  digs = "0123456789abcdef";
  for (cnt = 0;; ++fmt)
  {
    while ((ch = *fmt) && ch != '%')
    {
      PUTC (ch);
      fmt++;
      cnt++;
    }
    if (!ch)
      return cnt;
    flags = 0; dprec = 0; fpprec = 0; width = 0;
    prec = -1;
    sign = '\0';
  rflag:
    switch (*++fmt)
    {
    case ' ':
      /*
       * ``If the space and + flags both appear, the space
       * flag will be ignored.''
       *	-- ANSI X3J11
       */
      if (!sign)
	sign = ' ';
      goto rflag;
    case '#':
      flags |= ALT;
      goto rflag;
    case '*':
      /*
       * ``A negative field width argument is taken as a
       * - flag followed by a  positive field width.''
       *	-- ANSI X3J11
       * They don't exclude field widths read from args.
       */
      if ((width = va_arg(argp, int)) >= 0)
	goto rflag;
      width = -width;
      /* FALLTHROUGH */
    case '-':
      flags |= LADJUST;
      goto rflag;
    case '+':
      sign = '+';
      goto rflag;
    case '.':
      if (*++fmt == '*')
	n = va_arg(argp, int);
      else
      {
	n = 0;
	while (isascii(*fmt) && isdigit(*fmt))
	  n = 10 * n + todigit(*fmt++);
	--fmt;
      }
      prec = n < 0 ? -1 : n;
      goto rflag;
    case '0':
      /*
       * ``Note that 0 is taken as a flag, not as the
       * beginning of a field width.''
       *	-- ANSI X3J11
       */
      flags |= ZEROPAD;
      goto rflag;
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      n = 0;
      do {
	n = 10 * n + todigit(*fmt);
      } while (isascii(*++fmt) && isdigit(*fmt));
      width = n;
      --fmt;
      goto rflag;
    case 'L':
      flags |= LONGDBL;
      goto rflag;
    case 'h':
      flags |= SHORTINT;
      goto rflag;
    case 'l':
      if (flags&LONGINT)
	flags |= LONGDBL; /* for 'll' - llong */
      else
	flags |= LONGINT;
      goto rflag;
    case 'c':
      *(t = buf) = va_arg(argp, int);
      size = 1;
      sign = '\0';
      goto pforw;
    case 'D':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'd':
    case 'i':
      ARG(int);
      if ((llong)_ulonglong < 0)
      {
        _ulonglong = -_ulonglong;
	sign = '-';
      }
      base = 10;
      goto number;
    case 'e':
    case 'E':
    case 'f':
    case 'g':
    case 'G':
      if (flags & LONGDBL)
	_ldouble = va_arg(argp, ldouble);
      else
	_ldouble = (ldouble)va_arg(argp, double);
      /*
       * don't do unrealistic precision; just pad it with
       * zeroes later, so buffer size stays rational.
       */
      if (prec > MAXFRACT)
      {
	if (*fmt != 'g' && (*fmt != 'G' || (flags&ALT)))
	  fpprec = prec - MAXFRACT;
	prec = MAXFRACT;
      }
      else if (prec == -1)
      {
	if (flags&LONGINT)
	  prec = DEFLPREC;
	else
	  prec = DEFPREC;
      }
      /*
       * softsign avoids negative 0 if _double is < 0 and
       * no significant digits will be shown
       */
      if (_ldouble < 0)
      {
	softsign = '-';
	_ldouble = -_ldouble;
	neg_ldouble = 1;
      }
      else
      {
	softsign = 0;
	neg_ldouble = 0;
      }
      /*
       * cvt may have to round up past the "start" of the
       * buffer, i.e. ``intf("%.2f", (double)9.999);'';
       * if the first char isn't NULL, it did.
       */
      *buf = 0;
      size = cvtl(_ldouble, prec, flags, &softsign, *fmt, buf,
		  buf + sizeof(buf));
      /*
       * If the format specifier requested an explicit sign,
       * we print a negative sign even if no significant digits
       * will be shown, and we also print a sign for a NaN.  In
       * other words, "%+f" might print -0.000000, +NaN and -NaN.
       */
      if (softsign || (sign == '+' && (neg_ldouble || _nan == -1)))
	sign = '-';
      _nan = 0;
      t = *buf ? buf : buf + 1;
      goto pforw;
    case 'n':
      if (flags & LONGDBL)
        *va_arg(argp, llong *) = cnt;
      else if (flags & LONGINT)
	*va_arg(argp, long *) = cnt;
      else if (flags & SHORTINT)
	*va_arg(argp, short *) = cnt;
      else
	*va_arg(argp, int *) = cnt;
      break;
    case 'O':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'o':
      ARG(unsigned);
      base = 8;
      goto nosign;
    case 'p':
      /*
       * ``The argument shall be a pointer to void.  The
       * value of the pointer is converted to a sequence
       * of printable characters, in an implementation-
       * defined manner.''
       *	-- ANSI X3J11
       */
      /* NOSTRICT */
      _ulonglong = (unsigned long)va_arg(argp, void *);
      base = 16;
      goto nosign;
    case 's':
      if (!(t = va_arg(argp, char *)))
	t = NULL_REP;
      if (prec >= 0)
      {
	/*
	 * can't use strlen; can only look for the
	 * NUL in the first `prec' characters, and
	 * strlen() will go further.
	 */
	char *p			/*, *memchr() */;

	if ((p = memchr(t, 0, prec)))
	{
	  size = p - t;
	  if (size > prec)
	    size = prec;
	}
	else
	  size = prec;
      }
      else
	size = strlen(t);
      sign = '\0';
      goto pforw;
    case 'U':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'u':
      ARG(unsigned);
      base = 10;
      goto nosign;
    case 'X':
      digs = "0123456789ABCDEF";
      /* FALLTHROUGH */
    case 'x':
      ARG(unsigned);
      base = 16;
      /* leading 0x/X only if non-zero */
      if (flags & ALT && _ulonglong != 0)
	flags |= HEXPREFIX;

    nosign:
      /* unsigned conversions */
      sign = '\0';
    number:
      /*
       * ``... diouXx conversions ... if a precision is
       * specified, the 0 flag will be ignored.''
       *	-- ANSI X3J11
       */
      if ((dprec = prec) >= 0)
	flags &= ~ZEROPAD;

      /*
       * ``The result of converting a zero value with an
       * explicit precision of zero is no characters.''
       *	-- ANSI X3J11
       */
      t = buf + BUF;

      if (_ulonglong != 0 || prec != 0)
      {
        /* conversion is done separately since operations
	  with llong are much slower */
#define CONVERT(type) \
	{ \
	  register type _n = (type)_ulonglong; \
	  do { \
	    *--t = digs[_n % base]; \
	    _n /= base; \
	  } while (_n); \
	}
	if (flags&LONGDBL)
	  CONVERT(ullong) /* no ; */
	else
	  CONVERT(unsigned long) /* no ; */
#undef CONVERT
        if (flags & ALT && base == 8 && *t != '0')
          *--t = '0';		/* octal leading 0 */
      }

      digs = "0123456789abcdef";
      size = buf + BUF - t;

    pforw:
      /*
       * All reasonable formats wind up here.  At this point,
       * `t' points to a string which (if not flags&LADJUST)
       * should be padded out to `width' places.  If
       * flags&ZEROPAD, it should first be prefixed by any
       * sign or other prefix; otherwise, it should be blank
       * padded before the prefix is emitted.  After any
       * left-hand padding and prefixing, emit zeroes
       * required by a decimal [diouxX] precision, then print
       * the string proper, then emit zeroes required by any
       * leftover floating precision; finally, if LADJUST,
       * pad with blanks.
       */

      /*
       * compute actual size, so we know how much to pad
       * fieldsz excludes decimal prec; realsz includes it
       */
      fieldsz = size + fpprec;
      realsz = dprec > fieldsz ? dprec : fieldsz;
      if (sign)
	realsz++;
      if (flags & HEXPREFIX)
	realsz += 2;

      /* right-adjusting blank padding */
      if ((flags & (LADJUST|ZEROPAD)) == 0 && width)
	for (n = realsz; n < width; n++)
	  PUTC(' ');
      /* prefix */
      if (sign)
	PUTC(sign);
      if (flags & HEXPREFIX)
      {
	PUTC('0');
	PUTC((char)*fmt);
      }
      /* right-adjusting zero padding */
      if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
	for (n = realsz; n < width; n++)
	  PUTC('0');
      /* leading zeroes from decimal precision */
      for (n = fieldsz; n < dprec; n++)
	PUTC('0');

      /* the string or number proper */
      for (n = size; n > 0; n--)
        PUTC(*t++);
      /* trailing f.p. zeroes */
      while (--fpprec >= 0)
	PUTC('0');
      /* left-adjusting padding (always blank) */
      if (flags & LADJUST)
	for (n = realsz; n < width; n++)
	  PUTC(' ');
      /* finally, adjust cnt */
      cnt += width > realsz ? width : realsz;
      break;
    case '\0':			/* "%?" prints ?, unless ? is NULL */
      return cnt;
    default:
      PUTC((char)*fmt);
      cnt++;
    }
  }
  /* NOTREACHED */
}

static ldouble pten[] =
{
  1e1L, 1e2L, 1e4L, 1e8L, 1e16L, 1e32L, 1e64L, 1e128L, 1e256L
};

// 1e512L, 1e1024L, 1e2048L, 1e4096L

static ldouble ptenneg[] =
{
  1e-1L, 1e-2L, 1e-4L, 1e-8L, 1e-16L, 1e-32L, 1e-64L, 1e-128L, 1e-256L
};
//  1e-512L, 1e-1024L, 1e-2048L, 1e-4096L

#define MAXP 4096
#define NP   12
#define P    (4294967296.0L * 4294967296.0L * 2.0L)   /* 2^65 */
static ldouble INVPREC = P;
static ldouble PREC = 1.0L/P;
#undef P
/*
 * Defining FAST_LDOUBLE_CONVERSION results in a little bit faster
 * version, which might be less accurate (about 1 bit) for long
 * double. For 'normal' double it doesn't matter.
 */
/* #define FAST_LDOUBLE_CONVERSION */

static int
cvtl(ldouble number, int prec, int flags, char *signp, unsigned char fmtch,
     char *startp, char *endp)
{
  char *p, *t;
  ldouble fract=0;
  int dotrim, expcnt, gformat;
  int doextradps=0;    /* Do extra decimal places if the precision needs it */
  int doingzero=0;     /* We're displaying 0.0 */
  ldouble integer, tmp;

  if ((expcnt = isspeciall(number, startp)))
    return(expcnt);

  dotrim = expcnt = gformat = 0;
  /* fract = modfl(number, &integer); */
  integer = number;

  /* get an extra slot for rounding. */
  t = ++startp;

  p = endp - 1;
  if (integer)
  {
    int i, lp=NP, pt=MAXP;
#ifndef FAST_LDOUBLE_CONVERSION
    ldouble oint = integer, dd=1.0L;
#endif
    if (integer > INVPREC)
    {
      integer *= PREC;
      while(lp >= 0) {
	if (integer >= pten[lp])
	{
	  expcnt += pt;
	  integer *= ptenneg[lp];
#ifndef FAST_LDOUBLE_CONVERSION
	  dd *= pten[lp];
#endif
	}
	pt >>= 1;
	lp--;
      }
#ifndef FAST_LDOUBLE_CONVERSION
      integer = oint/dd;
#else
      integer *= INVPREC;
#endif
    }
    /*
     * Do we really need this ?
     */
    for (i = 0; i < expcnt; i++)
      *p-- = '0';
  }
  number = integer;
  fract = MODFL(number, &integer);
  /* If integer is zero then we need to look at where the sig figs are */
  if (integer<1) {
        /* If fract is zero the zero before the decimal point is a sig fig */
        if (fract==0.0) doingzero=1;
        /* If fract is non-zero all sig figs are in fractional part */
        else doextradps=1;
  }
  /*
   * get integer portion of number; put into the end of the buffer; the
   * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
   */
  for (; integer; ++expcnt)
  {
    tmp = MODFL(integer * 0.1L , &integer);
    *p-- = tochar((int)((tmp + .01L) * 10));
  }
  switch(fmtch)
  {
  case 'f':
    /* reverse integer into beginning of buffer */
    if (expcnt)
      for (; ++p < endp; *t++ = *p);
    else
      *t++ = '0';
    /*
     * if precision required or alternate flag set, add in a
     * decimal point.
     */
    if (prec || flags&ALT)
      *t++ = decimal;
    /* if requires more precision and some fraction left */
    if (fract)
    {
      if (prec)
	do {
	  fract = MODFL(fract * 10.0L, &tmp);
	  *t++ = tochar((int)tmp);
	} while (--prec && fract);
      if (fract)
	startp = roundl(fract, (int *)NULL, startp,
			t - 1, (char)0, signp);
    }
    for (; prec--; *t++ = '0');
    break;
  case 'e':
  case 'E':
  eformat:
    if (expcnt)
    {
      *t++ = *++p;
      if (prec || flags&ALT)
	*t++ = decimal;
      /* if requires more precision and some integer left */
      for (; prec && ++p < endp; --prec)
	*t++ = *p;
      /*
       * if done precision and more of the integer component,
       * round using it; adjust fract so we don't re-round
       * later.
       */
      if (!prec && ++p < endp)
      {
	fract = 0;
	startp = roundl((ldouble)0.0L, &expcnt,
			startp, t - 1, *p, signp);
      }
      /* adjust expcnt for digit in front of decimal */
      --expcnt;
    }
    /* until first fractional digit, decrement exponent */
    else if (fract)
    {
      int lp=NP, pt=MAXP;
#ifndef FAST_LDOUBLE_CONVERSION
      ldouble ofract = fract, dd=1.0L;
#endif
      expcnt = -1;
      if (fract < PREC)
      {
	fract *= INVPREC;
	while(lp >= 0)
	{
	  if (fract <= ptenneg[lp])
	  {
	    expcnt -= pt;
	    fract *= pten[lp];
#ifndef FAST_LDOUBLE_CONVERSION
	    dd *= pten[lp];
#endif
	  }
	  pt >>= 1;
	  lp--;
	}
#ifndef FAST_LDOUBLE_CONVERSION
	fract = ofract*dd;
#else
	fract *= PREC;
#endif
      }
      /* adjust expcnt for digit in front of decimal */
      for (			/* expcnt = -1 */ ;; --expcnt)
      {
	fract = MODFL(fract * 10.0L, &tmp);
	if (tmp)
	  break;
      }
      *t++ = tochar((int)tmp);
      if (prec || flags&ALT)
	*t++ = decimal;
    }
    else
    {
      *t++ = '0';
      if (prec || flags&ALT)
	*t++ = decimal;
    }
    /* if requires more precision and some fraction left */
    if (fract)
    {
      if (prec)
	do {
	  fract = MODFL(fract * 10.0L, &tmp);
	  *t++ = tochar((int)tmp);
	} while (--prec && fract);
      if (fract)
	startp = roundl(fract, &expcnt, startp,
			t - 1, (char)0, signp);
    }
    /* if requires more precision */
    for (; prec--; *t++ = '0');

    /* unless alternate flag, trim any g/G format trailing 0's */
    if (gformat && !(flags&ALT))
    {
      while (t > startp && *--t == '0');
      if (*t == decimal)
	--t;
      ++t;
    }
    t = exponentl(t, expcnt, fmtch);
    break;
  case 'g':
  case 'G':
    if (prec) {
        /* If doing zero and precision is greater than 0 count the
         * 0 before the decimal place */
        if (doingzero) --prec;
    }
    else {
        /* a precision of 0 is treated as precision of 1 unless doing zero */
        if (!doingzero) ++prec;
    }
    /*
     * ``The style used depends on the value converted; style e
     * will be used only if the exponent resulting from the
     * conversion is less than -4 or greater than the precision.''
     *	-- ANSI X3J11
     */
    if (expcnt > prec || (!expcnt && fract && fract < .0001))
    {
      /*
       * g/G format counts "significant digits, not digits of
       * precision; for the e/E format, this just causes an
       * off-by-one problem, i.e. g/G considers the digit
       * before the decimal point significant and e/E doesn't
       * count it as precision.
       */
      --prec;
      fmtch -= 2;		/* G->E, g->e */
      gformat = 1;
      goto eformat;
    }
    /*
     * reverse integer into beginning of buffer,
     * note, decrement precision
     */
    if (expcnt)
      for (; ++p < endp; *t++ = *p, --prec);
    else
      *t++ = '0';
    /*
     * if precision required or alternate flag set, add in a
     * decimal point.  If no digits yet, add in leading 0.
     */
    if (prec || flags&ALT)
    {
      dotrim = 1;
      *t++ = decimal;
    }
    else
      dotrim = 0;
    /* if requires more precision and some fraction left */
    while (prec && fract)
    {
      fract = MODFL(fract * 10.0L, &tmp);
      *t++ = tochar((int)tmp);
      /* If we're not adding 0s
       * or we are but they're sig figs:
       * decrement the precision */
      if ((doextradps!=1) || ((int)tmp!=0)) {
        doextradps=0;
        prec--;
      }
    }
    if (fract)
      startp = roundl(fract, (int *)NULL, startp, t - 1,
		      (char)0, signp);
    /* alternate format, adds 0's for precision, else trim 0's */
    if (flags&ALT)
      for (; prec--; *t++ = '0');
    else if (dotrim)
    {
      while (t > startp && *--t == '0');
      if (*t != decimal)
	++t;
    }
  }
  return t - startp;
}

static char *
roundl(ldouble fract, int *expv, char *start, char *end, char ch,
       char *signp)
{
  ldouble tmp;

  if (fract)
  {
    if (fract == 0.5L)
    {
      char *e = end;
      if (*e == '.')
	e--;
      if (*e == '0' || *e == '2' || *e == '4'
	  || *e == '6' || *e == '8')
      {
	tmp = 3.0;
	goto start;
      }
    }
    (void)MODFL(fract * 10.0L, &tmp);
  }
  else
    tmp = todigit(ch);
 start:
  if (tmp > 4)
    for (;; --end)
    {
      if (*end == decimal)
	--end;
      if (++*end <= '9')
	break;
      *end = '0';
      if (end == start)
      {
	if (expv)
	{		/* e/E; increment exponent */
	  *end = '1';
	  ++*expv;
	}
	else
	{			/* f; add extra digit */
	  *--end = '1';
	  --start;
	}
	break;
      }
    }
  /* ``"%.3f", (double)-0.0004'' gives you a negative 0. */
  else if (*signp == '-')
    for (;; --end)
    {
      if (*end == decimal)
	--end;
      if (*end != '0')
	break;
      if (end == start)
	*signp = 0;
    }
  return start;
}

static char *
exponentl(char *p, int expv, unsigned char fmtch)
{
  char *t;
  char expbuf[MAXEXPLD];

  *p++ = fmtch;
  if (expv < 0)
  {
    expv = -expv;
    *p++ = '-';
  }
  else
    *p++ = '+';
  t = expbuf + MAXEXPLD;
  if (expv > 9)
  {
    do {
      *--t = tochar(expv % 10);
    } while ((expv /= 10) > 9);
    *--t = tochar(expv);
    for (; t < expbuf + MAXEXPLD; *p++ = *t++);
  }
  else
  {
    *p++ = '0';
    *p++ = tochar(expv);
  }
  return p;
}

static int
isspeciall(ldouble d, char *bufp)
{
  struct IEEExp {
    unsigned manl:32;
    unsigned manh:32;
    unsigned exp:15;
    unsigned sign:1;
  } *ip = (struct IEEExp *)&d;

  _nan = 0;  /* don't assume the static is 0 (emacs) */
  if (ip->exp != 0x7fff)
    return(0);
  if ((ip->manh & 0x7fffffff) || ip->manl)
  {
    strcpy(bufp, "NaN");
    _nan = ip->sign ? -1 : 1; /* kludge: we don't need the sign,  it's not nice
				but it should work */
  }
  else
    (void)strcpy(bufp, "Inf");
  return(3);
}




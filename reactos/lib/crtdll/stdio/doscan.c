/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <libc/file.h>

//long double
//atold(const char *ascii);
#define atold 	atof

// dubious variable 
//static int _fltused = 0;

int
_doscan_low(FILE *iop, int (*scan_getc)(FILE *), int (*scan_ungetc)(int, FILE *),
            const char *fmt, void **argp);

//#include <libc/local.h>

#define	SPC	01
#define	STP	02

#define	SHORT	0
#define	REGULAR	1
#define	LONG	2
#define LONGDOUBLE 4
#define	INT	0
#define	FLOAT	1



static int _innum(int **ptr, int type, int len, int size, FILE *iop, 
                  int (*scan_getc)(FILE *), int (*scan_ungetc)(int, FILE *), 
                  int *eofptr);
static int _instr(char *ptr, int type, int len, FILE *iop, 
                  int (*scan_getc)(FILE *), int (*scan_ungetc)(int, FILE *), 
                  int *eofptr);
static const char *_getccl(const unsigned char *s);

static char _sctab[256] = {
	0,0,0,0,0,0,0,0,
	0,SPC,SPC,SPC,SPC,SPC,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	SPC,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};

static int nchars = 0;

int 
_doscan(FILE *iop, const char *fmt, void **argp)
{
  return(_doscan_low(iop, fgetc, ungetc, fmt, argp));
}

int 
_dowscan(FILE *iop, const wchar_t *fmt, void **argp)
{
  return(_doscan_low(iop, fgetwc, ungetwc, fmt, argp));
}

int
_doscan_low(FILE *iop, int (*scan_getc)(FILE *), int (*scan_ungetc)(int, FILE *),
            const char *fmt, void **argp)
{
  register int ch;
  int nmatch, len, ch1;
  int **ptr, fileended, size;

  nchars = 0;
  nmatch = 0;
  fileended = 0;
  for (;;) switch (ch = *fmt++) {
  case '\0': 
    return (nmatch);
  case '%':
    if ((ch = *fmt++) == '%')
      goto def;
    ptr = 0;
    if (ch != '*')
      ptr = (int **)argp++;
    else
      ch = *fmt++;
    len = 0;
    size = REGULAR;
    while (isdigit(ch)) {
      len = len*10 + ch - '0';
      ch = *fmt++;
    }
    if (len == 0)
      len = 30000;
    
    if (ch=='l') 
    {
      size = LONG;
      ch = *fmt++;
      if (ch=='l')
      {
        size = LONGDOUBLE; /* for long long 'll' format */
        ch = *fmt++;
      }
    }
    else if (ch=='h') {
      size = SHORT;
      ch = *fmt++;
    } else if (ch=='L') {
      size = LONGDOUBLE;
      ch = *fmt++;
    } else if (ch=='[')
      fmt = _getccl((const unsigned char *)fmt);
    if (isupper(ch)) {
      /* ch = tolower(ch);
	 gcc gives warning: ANSI C forbids braced
	 groups within expressions */
      ch += 'a' - 'A';
      if (size==LONG)
        size = LONGDOUBLE;
      else if (size != LONGDOUBLE)
        size = LONG;
    }
    if (ch == '\0')
      return(-1);

    if (ch == 'n')
    {
      if (!ptr)
        break;
      if (size==LONG)
	**(long**)ptr = nchars;
      else if (size==SHORT)
        **(short**)ptr = nchars;
      else if (size==LONGDOUBLE)
        **(long**)ptr = nchars;
      else
        **(int**)ptr = nchars;
      break;
    }
      
    if (_innum(ptr, ch, len, size, iop, scan_getc, scan_ungetc,
	       &fileended))
    {
      if (ptr)
        nmatch++;
    }
    else
    {
      if (fileended && nmatch==0)
        return(-1);
      return(nmatch);
    }
    break;
  case ' ':
  case '\n':
  case '\t': 
  case '\r':
  case '\f':
  case '\v':
    while (((nchars++, ch1 = scan_getc(iop))!=EOF) && (_sctab[ch1] & SPC))
      ;
    if (ch1 != EOF)
    {
      scan_ungetc(ch1, iop);
    }
    nchars--;
    break;

  default:
  def:
    ch1 = scan_getc(iop);
    if (ch1 != EOF) nchars++;
    if (ch1 != ch) {
      if (ch1==EOF)
	return(nmatch? nmatch: -1);
      scan_ungetc(ch1, iop);
      nchars--;
      return(nmatch);
    }
  }
}

static int
_innum(int **ptr, int type, int len, int size, FILE *iop,
       int (*scan_getc)(FILE *), int (*scan_ungetc)(int, FILE *), int *eofptr)
{
  register char *np;
  char numbuf[64];
  register int c, base;
  int expseen, scale, negflg, c1, ndigit;
  long lcval;
  int cpos;

  if (type=='c' || type=='s' || type=='[')
    return(_instr(ptr? *(char **)ptr: (char *)NULL, type, len,
		  iop, scan_getc, scan_ungetc, eofptr));
  lcval = 0;
  ndigit = 0;
  scale = INT;
  if (type=='e'||type=='f'||type=='g')
    scale = FLOAT;
  base = 10;
  if (type=='o')
    base = 8;
  else if (type=='x')
    base = 16;
  np = numbuf;
  expseen = 0;
  negflg = 0;
  while (((nchars++, c = scan_getc(iop)) != EOF) && (_sctab[c] & SPC))
    ;
  if (c == EOF) nchars--;
  if (c=='-') {
    negflg++;
    *np++ = c;
    c = scan_getc(iop);
    nchars++;
    len--;
  } else if (c=='+') {
    len--;
    c = scan_getc(iop);
    nchars++;
  }
  cpos = 0;
  for ( ; --len>=0; *np++ = c, c = scan_getc(iop), nchars++) {
    cpos++;
    if (c == '0' && cpos == 1 && type == 'i')
      base = 8;
    if ((c == 'x' || c == 'X') && (type == 'i' || type == 'x')
	&& cpos == 2 && lcval == 0)
    {
      base = 16;
      continue;
    }
    if (isdigit(c)
	|| (base==16 && (('a'<=c && c<='f') || ('A'<=c && c<='F')))) {
      ndigit++;
      if (base==8)
	lcval <<=3;
      else if (base==10)
	lcval = ((lcval<<2) + lcval)<<1;
      else
	lcval <<= 4;
      c1 = c;
      if (isdigit(c))
	c -= '0';
      else if ('a'<=c && c<='f')
	c -= 'a'-10;
      else
	c -= 'A'-10;
      lcval += c;
      c = c1;
      continue;
    } else if (c=='.') {
      if (base!=10 || scale==INT)
	break;
      ndigit++;
      continue;
    } else if ((c=='e'||c=='E') && expseen==0) {
      if (base!=10 || scale==INT || ndigit==0)
	break;
      expseen++;
      *np++ = c;
      c = scan_getc(iop);
      nchars++;
      if (c!='+'&&c!='-'&&('0'>c||c>'9'))
	break;
    } else
      break;
  }
  if (negflg)
    lcval = -lcval;
  if (c != EOF) {
    scan_ungetc(c, iop);
    *eofptr = 0;
  } else
    *eofptr = 1;
  nchars--;
  if (np==numbuf || (negflg && np==numbuf+1) ) /* gene dykes*/
    return(0);
  if (ptr==NULL)
    return(1);
  *np++ = 0;
  switch((scale<<4) | size) {

  case (FLOAT<<4) | SHORT:
  case (FLOAT<<4) | REGULAR:
    **(float **)ptr = (float)atof(numbuf);
    break;

  case (FLOAT<<4) | LONG:
    **(double **)ptr = atof(numbuf);
    break;

  case (FLOAT<<4) | LONGDOUBLE:
    **(long double **)ptr = atold(numbuf);
    break;

  case (INT<<4) | SHORT:
    **(short **)ptr = (short)lcval;
    break;

  case (INT<<4) | REGULAR:
    **(int **)ptr = (int)lcval;
    break;

  case (INT<<4) | LONG:
    **(long **)ptr = lcval;
    break;

  case (INT<<4) | LONGDOUBLE:
    **(long **)ptr = lcval;
    break;
  }
  return(1);
}

static int
_instr(char *ptr, int type, int len, FILE *iop,
       int (*scan_getc)(FILE *), int (*scan_ungetc)(int, FILE *), int *eofptr)
{
  register int ch;
  register char *optr;
  int ignstp;

  *eofptr = 0;
  optr = ptr;
  if (type=='c' && len==30000)
    len = 1;
  ignstp = 0;
  if (type=='s')
    ignstp = SPC;
  while ((nchars++, ch = scan_getc(iop)) != EOF && _sctab[ch] & ignstp)
    ;
  ignstp = SPC;
  if (type=='c')
    ignstp = 0;
  else if (type=='[')
    ignstp = STP;
  while (ch!=EOF && (_sctab[ch]&ignstp)==0) {
    if (ptr)
      *ptr++ = ch;
    if (--len <= 0)
      break;
    ch = scan_getc(iop);
    nchars++;
  }
  if (ch != EOF) {
    if (len > 0)
    {
      scan_ungetc(ch, iop);
      nchars--;
    }
    *eofptr = 0;
  } else
  {
    nchars--;
    *eofptr = 1;
  }
  if (!ptr)
    return(1);
  if (ptr!=optr) {
    if (type!='c')
      *ptr++ = '\0';
    return(1);
  }
  return(0);
}

static const char *
_getccl(const unsigned char *s)
{
  register int c, t;

  t = 0;
  if (*s == '^') {
    t++;
    s++;
  }
  for (c = 0; c < (sizeof _sctab / sizeof _sctab[0]); c++)
    if (t)
      _sctab[c] &= ~STP;
    else
      _sctab[c] |= STP;
  if ((c = *s) == ']' || c == '-') { /* first char is special */
    if (t)
      _sctab[c] |= STP;
    else
      _sctab[c] &= ~STP;
    s++;
  }
  while ((c = *s++) != ']') {
    if (c==0)
      return((const char *)--s);
    else if (c == '-' && *s != ']' && s[-2] < *s) {
      for (c = s[-2] + 1; c < *s; c++)
	if (t)
	  _sctab[c] |= STP;
	else
	  _sctab[c] &= ~STP;
    } else if (t)
      _sctab[c] |= STP;
    else
      _sctab[c] &= ~STP;
  }
  return((const char *)s);
}

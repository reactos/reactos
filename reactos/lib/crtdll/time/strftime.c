/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/string.h>
#include <msvcrt/time.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/wchar.h>


#define TM_YEAR_BASE 1900

static const char* afmt[] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};
static const char* Afmt[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
  "Saturday",
};
static const char* bfmt[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
  "Oct", "Nov", "Dec",
};
static const char* Bfmt[] = {
  "January", "February", "March", "April", "May", "June", "July",
  "August", "September", "October", "November", "December",
};

static size_t gsize;
static char* pt;


static int _add(const char* str)
{
  for (;; ++pt, --gsize)
  {
    if (!gsize)
      return 0;
    if (!(*pt = *str++))
      return 1;
  }
}

static int _conv(int n, int digits, char pad)
{
  static char buf[10];
  char* p;

  for (p = buf + sizeof(buf) - 2; n > 0 && p > buf; n /= 10, --digits)
    *p-- = n % 10 + '0';
  while (p > buf && digits-- > 0)
    *p-- = pad;
  return _add(++p);
}

static size_t _fmt(const char* format, const struct tm* t)
{
  for (; *format; ++format)
  {
    if (*format == '%') {
	if (*(format+1) == '#') {format++;}

    switch(*++format) {
      case '\0':
	--format;
	break;
      case 'A':
	if (t->tm_wday < 0 || t->tm_wday > 6)
	  return 0;
	if (!_add(Afmt[t->tm_wday]))
	  return 0;
	continue;
      case 'a':
	if (t->tm_wday < 0 || t->tm_wday > 6)
	  return 0;
	if (!_add(afmt[t->tm_wday]))
	  return 0;
	continue;
      case 'B':
	if (t->tm_mon < 0 || t->tm_mon > 11)
	  return 0;
	if (!_add(Bfmt[t->tm_mon]))
	  return 0;
	continue;
      case 'b':
      case 'h':
	if (t->tm_mon < 0 || t->tm_mon > 11)
	  return 0;
	if (!_add(bfmt[t->tm_mon]))
	  return 0;
	continue;
      case 'C':
	if (!_fmt("%a %b %e %H:%M:%S %Y", t))
	  return 0;
	continue;
      case 'c':
	if (!_fmt("%m/%d/%y %H:%M:%S", t))
	  return 0;
	continue;
      case 'e':
	if (!_conv(t->tm_mday, 2, ' '))
	  return 0;
	continue;
      case 'D':
	if (!_fmt("%m/%d/%y", t))
	  return 0;
	continue;
      case 'd':
	if (!_conv(t->tm_mday, 2, '0'))
	  return 0;
	continue;
      case 'H':
	if (!_conv(t->tm_hour, 2, '0'))
	  return 0;
	continue;
      case 'I':
	if (!_conv(t->tm_hour % 12 ? t->tm_hour % 12 : 12, 2, '0'))
	  return 0;
	continue;
      case 'j':
	if (!_conv(t->tm_yday + 1, 3, '0'))
	  return 0;
	continue;
      case 'k':
	if (!_conv(t->tm_hour, 2, ' '))
	  return 0;
	continue;
      case 'l':
	if (!_conv(t->tm_hour % 12 ? t->tm_hour % 12 : 12, 2, ' '))
	  return 0;
	continue;
      case 'M':
	if (!_conv(t->tm_min, 2, '0'))
	  return 0;
	continue;
      case 'm':
	if (!_conv(t->tm_mon + 1, 2, '0'))
	  return 0;
	continue;
      case 'n':
	if (!_add("\n"))
	  return 0;
	continue;
      case 'p':
	if (!_add(t->tm_hour >= 12 ? "PM" : "AM"))
	  return 0;
	continue;
      case 'R':
	if (!_fmt("%H:%M", t))
	  return 0;
	continue;
      case 'r':
	if (!_fmt("%I:%M:%S %p", t))
	  return 0;
	continue;
      case 'S':
	if (!_conv(t->tm_sec, 2, '0'))
	  return 0;
	continue;
      case 'T':
      case 'X':
	if (!_fmt("%H:%M:%S", t))
	  return 0;
	continue;
      case 't':
	if (!_add("\t"))
	  return 0;
	continue;
      case 'U':
	if (!_conv((t->tm_yday + 7 - t->tm_wday) / 7, 2, '0'))
	  return 0;
	continue;
      case 'W':
	if (!_conv((t->tm_yday + 7 - (t->tm_wday ? (t->tm_wday - 1) : 6)) / 7, 2, '0'))
	  return 0;
	continue;
      case 'w':
	if (!_conv(t->tm_wday, 1, '0'))
	  return 0;
	continue;
      case 'x':
	if (!_fmt("%m/%d/%y", t))
	  return 0;
	continue;
      case 'y':
	if (!_conv((t->tm_year + TM_YEAR_BASE) % 100, 2, '0'))
	  return 0;
	continue;
      case 'Y':
	if (!_conv(t->tm_year + TM_YEAR_BASE, 4, '0'))
	  return 0;
	continue;
      case 'Z':
	if (!t->tm_zone || !_add(t->tm_zone))
	  return 0;
	continue;
      case '%':
	/*
	 * X311J/88-090 (4.12.3.5): if conversion char is
	 * undefined, behavior is undefined.  Print out the
	 * character itself as printf(3) does.
	 */
      default:
	break;
      }
    }
    if (!gsize--)
      return 0;
    *pt++ = *format;
  }
  return gsize;
}

size_t
strftime(char *s, size_t maxsize, const char *format, const struct tm *t)
{
  pt = s;
  if ((gsize = maxsize) < 1)
    return 0;
  if (_fmt(format, t))
  {
    *pt = '\0';
    return maxsize - gsize;
  }
  return 0;
}

size_t wcsftime(wchar_t* s, size_t maxsize, const wchar_t* format, const struct tm* t)
{
  char* x;
  char* f;
  int i,j;
  x = malloc(maxsize);
  j = wcslen(format);
  f = malloc(j+1);
  for (i = 0; i < j; i++)
	f[i] = (char)*format;
  f[i] = 0;
  pt = x;
  if ((gsize = maxsize) < 1)
    return 0;
  if (_fmt(f, t)) {
    *pt = '\0';
    free(f);
    for (i = 0; i < maxsize;i ++)
	s[i] = (wchar_t)x[i];
    s[i] = 0;
    free(x);
    return maxsize - gsize;
  }
  for (i = 0; i < maxsize; i++)
	s[i] = (wchar_t)x[i];
  s[i] = 0;
  free(f);
  free(x);
  return 0;
}

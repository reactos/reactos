/* Copyright 1998 DJ Delorie <dj@delorie.com>
   Distributed under the terms of the GNU GPL
   http://www.delorie.com/store/hcalc/
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>

extern void set_bits(int);
extern void set_string(char *);
extern void do_exit(int);

#ifdef GNUC
#define longlong long long
#else
#define longlong __int64
#define XBell(d,v) 1
#endif

char pending_op = 0;
int making_number = 0;
int base = 10;

#define MAXIN 40

char input_buf[MAXIN];
int iptr;
double value, saved, stored=0;

void
convert_number()
{
  char *ip = input_buf;
  double scale = 1;
  int sign = +1;

  if (*ip == '-')
    sign = -1;
  value = 0;
  while (*++ip)
  {
    if (*ip == '.')
      break;
    if (*ip >= '0' && *ip <= '9')
    {
      value *= base;
      value += *ip-'0';
    }
    if (*ip >= 'a' && *ip <= 'f')
    {
      value *= base;
      value += *ip-'a'+10;
    }
  }
  if (*ip)
    while (*++ip)
    {
      if (*ip >= '0' && *ip <= '9')
      {
	scale *= base;
	value += (*ip-'0')/scale;
      }
      if (*ip >= 'a' && *ip <= 'f')
      {
	scale *= base;
	value += (*ip-'a'+10)/scale;
      }
    }
  value *= sign;
}

void
show_value()
{
  char tmp[20], *tp;
  char commas[40], *cp, *dp;
  double v = value;

  if (base == 2)
  {
    int q = (unsigned int)((longlong)v & 0xffffffffL);
    set_bits(q);
    return;
  }

  tmp[0] = ' ';
  if (v < 0)
  {
    tmp[0] = '-';
    v = -v;
  }

  if (base == 10)
  {
    sprintf(tmp+1, "%.14G", v);
    if (strchr(tmp+1, 'E'))
      sprintf(tmp+1, "%.9G", v);
    if (tmp[14] == '.')
      tmp[14] = 0;
  }
  else
  {
    static char tohex[] = "0123456789ABCDEF";
    longlong ll = (longlong)v;
    char *revptr;
    tp = tmp+1;
    if (base == 16)
    {
      *tp++ = '0';
      *tp++ = 'x';
    }
    else if (base == 8)
      *tp++ = '0';

    revptr = tp;
    do {
      *tp++ = tohex[ll%base];
      ll /= base;
    } while (ll);
    *tp-- = 0;
    while (revptr < tp) {
      char t = *revptr;
      *revptr = *tp;
      *tp = t;
      tp--;
      revptr++;
    }
  }

  cp = commas+40;
  tp = tmp+strlen(tmp);
  dp = strchr(tmp, '.');
  if (dp == 0)
    dp = tp;

  *--cp = 0;
  while (tp>=tmp)
  {
    *--cp = *tp--;
    switch (base)
    {
    case 10:
      if (isdigit(cp[0]) && isdigit(cp[1]) && isdigit(cp[2]) && tp<dp
	  && tp>=tmp && isdigit(*tp))
	*--cp = ',';
      break;
    case 16:
      if (isxdigit(cp[0]) && isxdigit(cp[1])
	  && isxdigit(cp[2]) && isxdigit(cp[3])
	  && tp>=tmp && isxdigit(*tp))
	*--cp = ',';
      break;
    }
  }

  if (strlen(cp) > 15)
    set_string(tmp);
  else
    set_string(cp);
}

void
end_number()
{
  if (!making_number)
    return;
  making_number = 0;
  iptr = 0;

  switch (pending_op)
  {
  case '+':
    value = saved + value;
    break;
  case '-':
    value = saved - value;
    break;
  case '*':
    value = saved * value;
    break;
  case '/':
    value = saved / value;
    break;
  case '&':
    value = (double)((longlong)saved & (longlong)value);
    break;
  case '|':
    value = (double)((longlong)saved | (longlong)value);
    break;
  case '^':
    value = (double)((longlong)saved ^ (longlong)value);
    break;
  case 'S':
    if (value < 0)
      value = (double)((longlong)saved >> (longlong)(-value));
    else
      value = (double)((longlong)saved << (longlong)value);
    break;
  }
  saved = value;
  pending_op = 0;
  show_value();
}

void
start_number()
{
  if (making_number)
    return;

  iptr = 1;
  input_buf[0] = ' ';
  input_buf[1] = 0;
  making_number = 1;
}

void
key(char c)
{
  int v = c;
  /* printf("key %c\n", c); */

  switch (c)
  {
  case 27:
    making_number = 0;
    iptr = 0;
    pending_op = 0;
    value = saved = 0;
    set_string("");
    break;
  case 'u':
    if (making_number)
    {
      making_number = 0;
      set_string("");
    }
    break;

  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f':
    v = c - 'a' - '9' - 1;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    v -= '0';
    if (v >= base || iptr == MAXIN-1)
      XBell(display, 0);
    else
    {
      start_number();
      input_buf[iptr++] = c;
      input_buf[iptr] = 0;
      convert_number();
      show_value();
    }
    break;
  case '.':
    if (strchr(input_buf, '.'))
    {
      XBell(display, 0);
      break;
    }
  case ',':
    if (iptr == 1 || iptr == MAXIN-1)
      XBell(display, 0);
    else
    {
      start_number();
      input_buf[iptr++] = c;
      input_buf[iptr] = 0;
      convert_number();
      show_value();
    }
    break;

  case 8:
    if (iptr == 1)
      XBell(display, 0);
    else
    {
      input_buf[--iptr] = 0;
      convert_number();
      show_value();
    }
    break;

  case '_': /* +/- */
    if (making_number)
    {
      if (input_buf[0] == '-')
	input_buf[0] = ' ';
      else
	input_buf[0] = '-';
      convert_number();
      show_value();
    }
    else
    {
      value *= -1.0;
      saved *= -1.0;
      show_value();
    }
    break;

  case 'D':
    end_number();
    base = 10;
    show_value();
    break;
  case 'H':
    end_number();
    base = 16;
    show_value();
    break;
  case 'O':
    end_number();
    base = 8;
    show_value();
    break;
  case 'B':
    end_number();
    base = 2;
    show_value();
    break;

  case 'x':
    c = '*';
  case '+':
  case '-':
  case '*':
  case '/':
  case '^':
  case '&':
  case '|':
  case 'S':
  case '=':
    end_number();
    pending_op = c;
    break;

  case 13:
  case 10:
    end_number();
    break;

  case '~':
    end_number();
    value = (double)(~(longlong)value);
    show_value();
    break;

  case '<':
    end_number();
    value = (double)((longlong)value << 1);
    show_value();
    break;

  case '>':
    end_number();
    value = (double)((longlong)value >> 1);
    show_value();
    break;

  case '[': /* STO */
    stored = value;
    break;
  case ']': /* RCL */
    value = stored;
    show_value();
    making_number = 1;
    iptr = 1;
    input_buf[0] = ' ';
    break;
  case '}': /* SUM */
    stored += value;
    break;

  case 'P': /* click on the display itself */
    break;
  }
}

static char *bmap[] = {
  "PPPP\033",
  "DHOB\010",
  "[]}<>",
  "Sdef/",
  "~abc*",
  "|789-",
  "&456+",
  "^123=",
  "u0._="
};

#if 0
void
copy()
{
  XSetSelectionOwner(display, XA_PRIMARY, window, event.xbutton.time);
}

void
paste()
{
  XConvertSelection(display, XA_PRIMARY, XA_STRING, paste_atom, window,
		    event.xbutton.time);
}

void
complete_paste(char *s, int n)
{
  int i;
  for (i=0; i<n; i++)
    key(s[i]);
  
}
#endif

void
button(int b, int x, int y)
{
  x = (x-2)/24;
  if (x < 0) x = 0;
  if (x > 4) x = 4;
  y = (y-1)/16;
  if (y < 0) y = 0;
  if (y > 8) y = 8;

  if (bmap[y][x] == 27 && b == 3)
    do_exit(0);

#if 0
  if (bmap[y][x] == 'P' && b == 1)
    copy();
  if (bmap[y][x] == 'P' && b != 1)
    paste();
#endif

  key(bmap[y][x]);
}

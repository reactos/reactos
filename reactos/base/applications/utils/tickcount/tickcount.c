/* $Id$
*/
/*
 tickcount -- Display the kernel tick count (or any tick count passed as an
 argument or as input) in human-readable format

 This is public domain software
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>

typedef __int64 int64_;
typedef unsigned __int64 uint64_;

#define TICKS_YEAR   (TICKS_DAY * ((uint64_)365))
#define TICKS_MONTH  (TICKS_DAY * ((uint64_)30))
#define TICKS_WEEK   (TICKS_DAY * ((uint64_)7))
#define TICKS_DAY    (TICKS_HOUR * ((uint64_)24))
#define TICKS_HOUR   (TICKS_MINUTE * ((uint64_)60))
#define TICKS_MINUTE (TICKS_SECOND * ((uint64_)60))
#define TICKS_SECOND ((uint64_)1000)

#define SLICES_COUNT (sizeof(ticks_per_slice) / sizeof(ticks_per_slice[0]))

uint64_ ticks_per_slice[] =
{
 TICKS_YEAR,
 TICKS_MONTH,
 TICKS_WEEK,
 TICKS_DAY,
 TICKS_HOUR,
 TICKS_MINUTE,
 TICKS_SECOND,
 1
};

_TCHAR * slice_names_singular[SLICES_COUNT] =
{
 _T("year"),
 _T("month"),
 _T("week"),
 _T("day"),
 _T("hour"),
 _T("minute"),
 _T("second"),
 _T("millisecond")
};

_TCHAR * slice_names_plural[SLICES_COUNT] =
{
 _T("years"),
 _T("months"),
 _T("weeks"),
 _T("days"),
 _T("hours"),
 _T("minutes"),
 _T("seconds"),
 _T("milliseconds")
};

void print_uptime
(
 uint64_ tickcount,
 uint64_ prevsliceval,
 _TCHAR * prevsliceunit,
 int curslice
)
{
 uint64_ tick_cur = tickcount / ticks_per_slice[curslice];
 uint64_ tick_residual = tickcount % ticks_per_slice[curslice];

 assert(tick_cur <= (~((uint64_)0)));

 if(tick_residual == 0)
 {
  /* the current slice is the last */

  if(prevsliceval == 0)
  {
   /* the current slice is the only */
   _tprintf
   (
    _T("%u %s"),
    (unsigned)tick_cur,
    (tick_cur == 1 ? slice_names_singular : slice_names_plural)[curslice]
   );
  }
  else
  {
   /* the current slice is the last, and there's a previous slice */
   assert(prevsliceunit);

   /* print the previous and the current slice, and terminate */
   _tprintf
   (
    _T("%u %s %s %u %s"),
    (unsigned)prevsliceval,
    prevsliceunit,
    _T("and"),
    (unsigned)tick_cur,
    (tick_cur == 1 ? slice_names_singular : slice_names_plural)[curslice]
   );
  }
 }
 else if(tick_cur != 0)
 {
  /* the current slice is not the last, and non-zero */

  if(prevsliceval != 0)
  {
   /* there's a previous slice: print it */
   assert(prevsliceunit);
   _tprintf(_T("%u %s, "), (unsigned)prevsliceval, prevsliceunit);
  }

  /* recursion on the next slice size, storing the current slice */
  print_uptime
  (
   tick_residual,
   tick_cur,
   (tick_cur == 1 ? slice_names_singular : slice_names_plural)[curslice],
   curslice + 1
  );
 }
 else
 {
  /*
   the current slice is not the last, and zero: recursion, remembering the
   previous non-zero slice
  */
  print_uptime(tick_residual, prevsliceval, prevsliceunit, curslice + 1);
 }
}

int parse_print(const _TCHAR * str)
{
 int64_ tickcount;

 tickcount = _ttoi64(str);

 if(tickcount < 0)
  tickcount = - tickcount;
 else if(tickcount == 0)
  return 1;

 print_uptime(tickcount, 0, NULL, 0);
 _puttc(_T('\n'), stdout);

 return 0;
}

int _tmain(int argc, _TCHAR * argv[])
{
 int r;

 if(argc <= 1)
 {
  print_uptime((uint64_)GetTickCount(), 0, NULL, 0);
  _puttc(_T('\n'), stdout);
 }
 else if(argc == 2 && argv[1][0] == _T('-') && argv[1][1] == 0)
 {
  while(!feof(stdin))
  {
   _TCHAR buf[23];
   _TCHAR * str;

   str = _fgetts(buf, 22, stdin);

   if(str == NULL)
    return 0;

   if((r = parse_print(str)) != 0)
    return r;
  }
 }
 else
 {
  int i;

  for(i = 1; i < argc; ++ i)
   if((r = parse_print(argv[i])) != 0)
    return r;
 }

 return 0;
}

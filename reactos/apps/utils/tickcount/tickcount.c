/* $Id: tickcount.c,v 1.1 2003/02/09 21:17:21 hyperion Exp $
*/
/*
 tickcount -- Display the kernel tick count in human-readable format

 This is public domain software
*/

#include <assert.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#define TICKS_YEAR   (TICKS_DAY * ((uint64_t)365))
#define TICKS_MONTH  (TICKS_DAY * ((uint64_t)30))
#define TICKS_WEEK   (TICKS_DAY * ((uint64_t)7))
#define TICKS_DAY    (TICKS_HOUR * ((uint64_t)24))
#define TICKS_HOUR   (TICKS_MINUTE * ((uint64_t)60))
#define TICKS_MINUTE (TICKS_SECOND * ((uint64_t)60))
#define TICKS_SECOND ((uint64_t)1000)

#define SLICES_COUNT (sizeof(ticks_per_slice) / sizeof(ticks_per_slice[0]))

uint64_t ticks_per_slice[] =
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

_TCHAR * slice_names_singular[] =
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

_TCHAR * slice_names_plural[] =
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
 uint64_t tickcount,
 uint64_t prevsliceval,
 _TCHAR * prevsliceunit,
 int curslice
)
{
 uint64_t tick_cur = tickcount / ticks_per_slice[curslice];
 uint64_t tick_residual = tickcount % ticks_per_slice[curslice];

 assert(tick_cur <= (~((unsigned)0)));

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

int _tmain()
{
 print_uptime((uint64_t)GetTickCount(), 0, NULL, 0);
 _puttc(_T('\n'), stdout);
 return 0;
}

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_time_h_
#define __dj_include_time_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

/* 65536(tics/hour) / 3600(sec/hour) * 5(scale) = 91.02
   The 5 is to make it a whole number (18.2*5=91) so that
   floating point ops aren't required to use it. */
#define CLOCKS_PER_SEC	91

#include <sys/djtypes.h>
#include <internal/types.h>
#ifndef NULL
#define NULL 0
#endif
__DJ_clock_t
#undef __DJ_clock_t
#define __DJ_clock_t
//__DJ_size_t
#undef __DJ_size_t
#define __DJ_size_t
__DJ_time_t
#undef __DJ_time_t
#define __DJ_time_t


#ifndef _TM_DEFINED
struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  char *tm_zone;
  int tm_gmtoff;
};
#define _TM_DEFINED
#endif

char *		asctime(const struct tm *_tptr);
clock_t		clock(void);
char *		ctime(const time_t *_cal);
double		difftime(time_t _t1, time_t _t0);
struct tm *	gmtime(const time_t *_tod);
struct tm *	localtime(const time_t *_tod);
time_t		mktime(struct tm *_tptr);
size_t		strftime(char *_s, size_t _n, const char *_format, const struct tm *_tptr);
time_t		time(time_t *_tod);

#ifndef __STRICT_ANSI__

#define CLK_TCK	CLOCKS_PER_SEC

extern char *tzname[2];

void	tzset(void);

#ifndef _POSIX_SOURCE

//#define tm_zone __tm_zone
//#define tm_gmtoff __tm_gmtoff

struct timeval {
  time_t tv_sec;
  long tv_usec;
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

#include <sys/types.h>

typedef long long uclock_t;
#define UCLOCKS_PER_SEC 1193180

int		gettimeofday(struct timeval *_tp, struct timezone *_tzp);
unsigned long	rawclock(void);
int		select(int _nfds, fd_set *_readfds, fd_set *_writefds, fd_set *_exceptfds, struct timeval *_timeout);
int		settimeofday(struct timeval *_tp, ...);
uclock_t	uclock(void);

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_time_h_ */

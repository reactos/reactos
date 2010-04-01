#define bcopy(s,d,l) memcpy((d),(s),(l))
#define bzero(cp,l) memset((cp),0,(l))

#define rindex strrchr
#define index strchr

#define getwd getcwd

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#ifndef _TIMEZONE_DEFINED /* also in sys/time.h */
#define _TIMEZONE_DEFINED
struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

  extern int __cdecl gettimeofday (struct timeval *p, struct timezone *z);
#endif

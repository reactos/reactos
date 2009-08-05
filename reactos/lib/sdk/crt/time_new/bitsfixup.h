
#if defined(_USE_EXPLITIT_32BIT_TIME) || defined(_USE_EXPLITIT_64BIT_TIME)
#undef _timeb
#undef _ftime
#undef _tctime
#undef _ftime_s
#undef _tctime_s
#undef _tutime
#else
#define _time time
#endif

#ifdef _USE_EXPLITIT_32BIT_TIME
#define time_t __time32_t
#define _timeb __timeb32
#define _utimbuf __utimbuf32

#define difftime _difftime32
#define localtime _localtime32
#define localtime_s _localtime32_s
#define _time _time32

#define _ftime _ftime32
#define _ftime_s _ftime32_s
#define _futime _futime32
#define _tctime _tctime32
#define _tctime_s _tctime32_s
#define _tutime _tutime32
#define gmtime _gmtime32

#endif

#ifdef _USE_EXPLITIT_64BIT_TIME
#define time_t __time64_t
#define _timeb __timeb64
#define _utimbuf __utimbuf64

#define difftime _difftime64
#define localtime _localtime64
#define localtime_s _localtime64_s
#define _time _time64

#define _ftime _ftime64
#define _ftime_s _ftime64_s
#define _futime _futime64
#define _tctime _tctime64
#define _tctime_s _tctime64_s
#define _tutime _tutime64
#define gmtime _gmtime64

#endif

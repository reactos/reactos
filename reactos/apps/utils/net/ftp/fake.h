#define bcopy(s,d,l) memcpy((d),(s),(l))
#define bzero(cp,l) memset((cp),0,(l))

#define rindex strrchr
#define index strchr

#define getwd getcwd

#define strcasecmp strcmp
#define strncasecmp strnicmp

struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

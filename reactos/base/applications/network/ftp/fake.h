#define bcopy(s,d,l) memcpy((d),(s),(l))
#define bzero(cp,l) memset((cp),0,(l))

#define rindex strrchr
#define index strchr

#define getwd getcwd

#define strcasecmp strcmp
#define strncasecmp strnicmp

void __cdecl _tzset(void);

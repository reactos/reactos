
int getreply(int expecteof);
int ruserpass(char *host, char **aname, char **apass, char **aacct);
char *getpass(const char *prompt);
void makeargv(void);
void domacro(int argc, char *argv[]);
void proxtrans(char *cmd, char *local, char *remote);
int null(void);
int initconn(void);
void disconnect(void);
void ptransfer(char *direction, long bytes, struct timeval *t0, struct timeval *t1);
void setascii(void);
void setbinary(void);
void setebcdic(void);
void settenex(void);
void tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0);
void setpassive(int argc, char *argv[]);
void setpeer(int argc, char *argv[]);
void cmdscanner(int top);
void pswitch(int flag);
void quit(void);
int login(char *host);
int command(char *fmt, ...);
int globulize(char **cpp);
void sendrequest(char *cmd, char *local, char *remote, int printnames);
void recvrequest(char *cmd, char *local, char *remote, char *mode,
                int printnames);
int confirm(char *cmd, char *file);
void blkfree(char **av0);
int getit(int argc, char *argv[], int restartit, char *mode);
static int token(void);
int sleep(int time);

#include <windows.h>
#include <time.h>

#ifndef __GNUC__
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif

struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};

__inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;
    static int      tzflag;

    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);
        //li.LowPart  = ft.dwLowDateTime;
        //li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }

    if (tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

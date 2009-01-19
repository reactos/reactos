
#define DIFFTIME 0x19db1ded53e8000ULL
#define DIFFDAYS (3 * DAYSPER100YEARS + 17 * DAYSPER4YEARS + 1 * DAYSPERYEAR)

#define DAYSPERYEAR 365
#define DAYSPER4YEARS (4*DAYSPERYEAR+1)
#define DAYSPER100YEARS (25*DAYSPER4YEARS-1)
#define DAYSPER400YEARS (4*DAYSPER100YEARS+1)
#define SECONDSPERDAY (24*60*60)
#define SECONDSPERHOUR (60*60)
#define LEAPDAY 59

extern inline
__time64_t
FileTimeToUnixTime(const FILETIME *FileTime, USHORT *millitm)
{
    ULARGE_INTEGER ULargeInt;
    __time64_t time;

    ULargeInt.LowPart = FileTime->dwLowDateTime;
    ULargeInt.HighPart = FileTime->dwHighDateTime;
    ULargeInt.QuadPart -= DIFFTIME;

    time = ULargeInt.QuadPart / 10000;
    if (millitm)
        *millitm = (ULargeInt.QuadPart % 10000) / 10;

    return time;
}

static __inline
long leapyears_passed(long days)
{
    long quadcenturies, centuries, quadyears;
    quadcenturies = days / DAYSPER400YEARS;
    days -= quadcenturies;
    centuries = days / DAYSPER100YEARS;
    days += centuries;
    quadyears = days / DAYSPER4YEARS;
    return quadyears - centuries + quadcenturies;
}

static __inline
long leapdays_passed(long days)
{
    return leapyears_passed(days + DAYSPERYEAR - LEAPDAY + 1);
}

static __inline
long years_passed(long days)
{
    return (days - leapdays_passed(days)) / 365;
}

extern long dst_begin;
extern long dst_end;

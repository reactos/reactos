#include <windows.h>
#include <msvcrt/sys/time.h>


int month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31};

unsigned int _getsystime(struct tm* tp)
{
    SYSTEMTIME Time;
    TIME_ZONE_INFORMATION TimeZoneInformation;
    DWORD TimeZoneId;
    int i;

    GetLocalTime(&Time);

    tp->tm_year = Time.wYear - 1900; 
    tp->tm_mon = Time.wMonth - 1; 
    tp->tm_wday = Time.wDayOfWeek; 
    tp->tm_mday = Time.wDay; 
    tp->tm_hour = Time.wHour; 
    tp->tm_min = Time.wMinute; 
    tp->tm_sec = Time.wSecond; 

    tp->tm_isdst = -1;

    //FIXME GetTimeZoneInformation currently not in kernel32

    //TimeZoneId =  GetTimeZoneInformation(&TimeZoneInformation );
    //if ( TimeZoneId == TIME_ZONE_ID_DAYLIGHT ) {
    //  tp->tm_isdst = 1;
    //}
    //else
    //  tp->tm_isdst = 0;

    if (tp->tm_year % 4 == 0) {
        if (tp->tm_year % 100 != 0)
            tp->tm_yday = 1;
        else if ((tp->tm_year-100) % 1000 == 0)
            tp->tm_yday = 1;
    }

    for (i = 0; i <= tp->tm_mon; i++)
        tp->tm_yday += month[i];
    
    return Time.wMilliseconds;
}


unsigned int _setsystime(struct tm* tp, unsigned int ms)
{
    SYSTEMTIME Time;

    Time.wYear = tp->tm_year + 1900; 
    Time.wMonth = tp->tm_mon + 1; 
    Time.wDayOfWeek = tp->tm_wday; 
    Time.wDay = tp->tm_mday; 
    Time.wHour = tp->tm_hour; 
    Time.wMinute = tp->tm_min; 
    Time.wSecond = tp->tm_sec; 
    Time.wMilliseconds = ms;

    if (!SetLocalTime(&Time))
        return -1;

    return 0;
}

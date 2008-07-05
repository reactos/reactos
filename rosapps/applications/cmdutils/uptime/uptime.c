#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#include <tchar.h>

int main(int argc, char* argv[])
{
    SYSTEMTIME SystemTime;
    LARGE_INTEGER liCount, liFreq;

    GetLocalTime(&SystemTime);

    if (QueryPerformanceCounter(&liCount) &&
        QueryPerformanceFrequency(&liFreq))
    {
        LONGLONG TotalSecs = liCount.QuadPart / liFreq.QuadPart;
        LONGLONG Days  =  (TotalSecs / 86400);
        LONGLONG Hours = ((TotalSecs % 86400) / 3600);
        LONGLONG Mins  = ((TotalSecs % 86400) % 3600) / 60;
        LONGLONG Secs  = ((TotalSecs % 86400) % 3600) % 60;

#ifdef LINUX_OUTPUT
        UNREFERENCED_PARAMETER(Secs);
        _tprintf(_T("  %.2u:%.2u  "), SystemTime.wHour, SystemTime.wMinute);
        _tprintf(_T("up %I64u days, %I64u:%I64u\n"), Days, Hours, Mins); /*%.2I64u secs*/
#else
        _tprintf(_T("System Up Time:\t\t%I64u days, %I64u Hours, %I64u Minutes, %.2I64u Seconds\n"),
                 Days, Hours, Mins, Secs);
#endif
        return 0;
    }

    return -1;
}

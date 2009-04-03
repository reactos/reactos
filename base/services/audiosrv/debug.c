/* Service debugging (simply logs to a file) */

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

void logmsg(char* string, ...)
{
    va_list args;

    FILE* debug_file = fopen("c:\\audiosrv-debug.txt", "a");

    if (debug_file)
    {
        va_start(args, string);
        vfprintf(debug_file, string, args);
        va_end(args);
        fclose(debug_file);
    }
    else
    {
        char buf[256];
        va_start(args, string);
        vsprintf(buf, string, args);
        OutputDebugStringA(buf);
        va_end(args);
    }
}

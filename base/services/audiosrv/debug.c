/* Service debugging (simply logs to a file) */

#include "audiosrv.h"

#include <stdio.h>

// FIXME: Disabled to work around CORE-16814 (and CORE-16912).
// #define ENABLE_LOGMSG_FILE

void logmsg(char* string, ...)
{
    va_list args;

#ifdef ENABLE_LOGMSG_FILE
    FILE* debug_file = fopen("c:\\audiosrv-debug.txt", "a");

    if (debug_file)
    {
        va_start(args, string);
        vfprintf(debug_file, string, args);
        va_end(args);
        fclose(debug_file);
    }
    else
#endif
    {
        char buf[256];
        va_start(args, string);
        vsprintf(buf, string, args);
        OutputDebugStringA(buf);
        va_end(args);
    }
}

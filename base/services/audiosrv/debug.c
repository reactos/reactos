/* Service debugging (simply logs to a file) */

#include <stdio.h>
#include <stdarg.h>

void logmsg(char* string, ...)
{
    va_list args;

    FILE* debug_file = fopen("c:\\audiosrv-debug.txt", "a");

    va_start(args, string);

    vfprintf(debug_file, string, args);

    va_end(args);

    fclose(debug_file);
}

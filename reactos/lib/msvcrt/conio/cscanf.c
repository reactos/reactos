#include <msvcrt/conio.h>
#include <msvcrt/stdarg.h>
#include <msvcrt/stdio.h>
#include <msvcrt/internal/stdio.h>

int _cscanf(char *fmt, ...)
{
    int cnt;

    va_list ap;

    //fixme cscanf should scan the console's keyboard
    va_start(ap, fmt);
    cnt = __vscanf(fmt, ap);
    va_end(ap);

    return cnt;
}

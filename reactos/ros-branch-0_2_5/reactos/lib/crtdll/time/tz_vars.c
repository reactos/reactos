#include <msvcrt/string.h>
#include <msvcrt/ctype.h>
#include <msvcrt/stdlib.h>


int _daylight_dll;
int _timezone_dll;


void _set_daylight_export(int value)
{
    _daylight_dll =  value;
}

void _set_timezone_export(int value)
{
    _timezone_dll =  value;
}




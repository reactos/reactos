#include <msvcrt/string.h>
#include <msvcrt/ctype.h>
#include <msvcrt/stdlib.h>


int _daylight;
int _timezone;


void _set_daylight_export(int value)
{
    _daylight =  value;
}

void _set_timezone_export(int value)
{
    _timezone =  value;
}




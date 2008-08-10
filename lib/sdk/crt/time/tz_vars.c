#include <string.h>
#include <ctype.h>
#include <stdlib.h>


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


/*********************************************************************
 *    __p_daylight (MSVCRT.@)
 */
void* __p__daylight(void)
{
   return &_daylight;
}

/*********************************************************************
 *    __p__timezone (MSVCRT.@)
 */
int* __p__timezone(void)
{
   return &_timezone;
}

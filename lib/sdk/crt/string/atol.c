/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>

/*
 * @implemented
 */
long atol(const char *str)
{
    return (long)_atoi64(str);
}

int _atoldbl(_LDOUBLE *value, char *str)
{
    /* FIXME needs error checking for huge/small values */
   //*value = strtold(str,0);
   return -1;
}

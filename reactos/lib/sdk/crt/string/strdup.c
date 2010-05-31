/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
char *_strdup(const char *str)
{
    if(str)
    {
      char * ret = malloc(strlen(str)+1);
      if (ret) strcpy( ret, str );
      return ret;
    }
    else return 0;
}


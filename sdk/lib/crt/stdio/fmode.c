/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

int _fmode = 0;

/*
 * @implemented
 */
int *__p__fmode(void)
{
   return &_fmode;
}

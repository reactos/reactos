/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <fcntl.h>
#include <io.h>


int _fmode = O_TEXT;

/*
 * @implemented
 */
int *__p__fmode(void)
{
   return &_fmode;
}

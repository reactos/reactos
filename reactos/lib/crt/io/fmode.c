/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>


int __fmode = O_TEXT;

/*
 * @implemented
 */
int *__p__fmode(void)
{
   return &__fmode;
}

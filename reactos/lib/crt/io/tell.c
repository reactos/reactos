/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <errno.h>
#include <io.h>
#include <stdio.h>


/*
 * @implemented
 */
off_t _tell(int _file)
{
    return _lseek(_file, 0, SEEK_CUR);
}

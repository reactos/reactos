/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/errno.h>
#include <msvcrt/io.h>


off_t _tell(int _file)
{
    return _lseek(_file, 0, SEEK_CUR);
}

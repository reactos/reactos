/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/io.h>

int
chsize(int _fd, long size)
{
  if (lseek(_fd, size, 0) == -1)
    return -1;
  if (_write(_fd, 0, 0) < 0)
    return -1;
  return 0;
}

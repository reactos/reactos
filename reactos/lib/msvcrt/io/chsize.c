/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/io.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

/*
 * @implemented
 */
int _chsize(int _fd, long size)
{
  DPRINT("_chsize(fd %d, size %d)\n", _fd, size);
  if (lseek(_fd, size, 0) == -1)
    return -1;
  if (_write(_fd, 0, 0) < 0)
    return -1;
  return 0;
}

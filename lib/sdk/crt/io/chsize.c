/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
 */
int _chsize(int _fd, long size)
{
  DPRINT("_chsize(fd %d, size %d)\n", _fd, size);
  long location = _lseek(_fd, 0, SEEK_CUR);
  if (location == -1) return -1;
  if (_lseek(_fd, size, 0) == -1)
    return -1;
  if (!SetEndOfFile((HANDLE)_get_osfhandle(_fd)))
    return -1;
  _lseek(_fd, location, SEEK_SET);
  return 0;
}

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/sys/types.h>
#include <crtdll/stdio.h>
#include <crtdll/fcntl.h>
#include <crtdll/internal/file.h>
#include <crtdll/io.h>



FILE *
freopen(const char *file, const char *mode, FILE *f)
{
  int fd, rw, oflags=0;
  char tbchar;

  if (file == 0 || mode == 0 || f == 0)
    return 0;

  rw = (mode[1] == '+');

  fclose(f);

  switch (*mode) {
  case 'a':
    oflags = O_CREAT | (rw ? O_RDWR : O_WRONLY);
    break;
  case 'r':
    oflags = rw ? O_RDWR : O_RDONLY;
    break;
  case 'w':
    oflags = O_TRUNC | O_CREAT | (rw ? O_RDWR : O_WRONLY);
    break;
  default:
    return NULL;
  }
  if (mode[1] == '+')
    tbchar = mode[2];
  else
    tbchar = mode[1];
  if (tbchar == 't')
    oflags |= O_TEXT;
  else if (tbchar == 'b')
    oflags |= O_BINARY;
  else
    oflags |= (_fmode & (O_TEXT|O_BINARY));

  fd = _open(file, oflags, 0666);
  if (fd < 0)
    return NULL;

  if (*mode == 'a')
    lseek(fd, 0, SEEK_END);

  f->_cnt = 0;
  f->_file = fd;
  f->_bufsiz = 0;
  if (rw)
    f->_flag = _IOREAD | _IOWRT;
  else if (*mode == 'r')
    f->_flag = _IOREAD;
  else
    f->_flag = _IOWRT;

  f->_base = f->_ptr = NULL;
  return f;
}

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/sys/types.h>
#include <msvcrt/stdio.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
FILE *freopen(const char *file, const char *mode, FILE *f)
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

/*
 * @implemented
 */
FILE *_wfreopen(const wchar_t *file, const wchar_t *mode, FILE *f)
{
  int fd, rw, oflags=0;
  wchar_t tbchar;

  if (file == 0 || mode == 0 || f == 0)
    return 0;

  rw = (mode[1] == L'+');

  fclose(f);

  switch (*mode) {
  case L'a':
    oflags = O_CREAT | (rw ? O_RDWR : O_WRONLY);
    break;
  case L'r':
    oflags = rw ? O_RDWR : O_RDONLY;
    break;
  case L'w':
    oflags = O_TRUNC | O_CREAT | (rw ? O_RDWR : O_WRONLY);
    break;
  default:
    return NULL;
  }
  if (mode[1] == L'+')
    tbchar = mode[2];
  else
    tbchar = mode[1];
  if (tbchar == L't')
    oflags |= O_TEXT;
  else if (tbchar == L'b')
    oflags |= O_BINARY;
  else
    oflags |= (_fmode & (O_TEXT|O_BINARY));

  fd = _wopen(file, oflags, 0666);
  if (fd < 0)
    return NULL;

  if (*mode == L'a')
    lseek(fd, 0, SEEK_END);

  f->_cnt = 0;
  f->_file = fd;
  f->_bufsiz = 0;
  if (rw)
    f->_flag = _IOREAD | _IOWRT;
  else if (*mode == L'r')
    f->_flag = _IOREAD;
  else
    f->_flag = _IOWRT;

  f->_base = f->_ptr = NULL;
  return f;
}

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/sys/types.h>
#include <msvcrt/stdio.h>
#include <msvcrt/io.h>
#include <msvcrt/fcntl.h>
//#include <msvcrt/internal/file.h>

//might change fopen(file,mode) -> fsopen(file,mode,_SH_DENYNO);

#undef _fmode
extern unsigned int _fmode;

FILE *	__alloc_file(void);


FILE* fopen(const char *file, const char *mode)
{
  FILE *f;
  int fd, rw, oflags = 0;
  char tbchar;
   
  if (file == 0)
    return 0;
  if (mode == 0)
    return 0;

  f = __alloc_file();
  if (f == NULL)
    return NULL;

  rw = (mode[1] == '+') || (mode[1] && (mode[2] == '+'));

  switch (*mode)
  {
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
    return (NULL);
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

  fd = _open(file, oflags, 0);
  if (fd < 0)
    return NULL;

// ms crtdll ensures that writes will end up at the end of file in append mode
// we just move the file pointer to the end of file initially
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

FILE* _wfopen(const wchar_t *file, const wchar_t *mode)
{
  FILE *f;
  int fd, rw, oflags = 0;
  wchar_t tbchar;
   
  if (file == 0)
    return 0;
  if (mode == 0)
    return 0;

  f = __alloc_file();
  if (f == NULL)
    return NULL;

  rw = (mode[1] == L'+') || (mode[1] && (mode[2] == L'+'));

  switch (*mode)
  {
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
    return (NULL);
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

  fd = _wopen(file, oflags, 0);
  if (fd < 0)
    return NULL;

// ms crtdll ensures that writes will end up at the end of file in append mode
// we just move the file pointer to the end of file initially
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

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/sys/types.h>
#include <crtdll/stdio.h>
#include <crtdll/io.h>
#include <crtdll/fcntl.h>
#include <crtdll/internal/file.h>


FILE *	__alloc_file(void);


FILE* _fsopen(const char *file, const char *mode, int shflag)
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


//_SH_COMPAT   Sets Compatibility mode for 16-bit applications
//_SH_DENYNO   Permits read and write access
//_SH_DENYRD   Denies read access to file
//_SH_DENYRW   Denies read and write access to file
//_SH_DENYWR   Denies write access to file


  fd = _open(file, oflags, shflag);
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

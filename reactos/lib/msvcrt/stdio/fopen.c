/* $Id: fopen.c,v 1.7 2002/11/24 18:42:24 robd Exp $
 *
 *  ReactOS msvcrt library
 *
 *  fopen.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  Based on original work Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details
 *                         28/12/1998: Appropriated for Reactos
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/sys/types.h>
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>
#include <msvcrt/io.h>
#include <msvcrt/fcntl.h>
//#include <msvcrt/internal/file.h>

//might change fopen(file,mode) -> fsopen(file,mode,_SH_DENYNO);

#undef _fmode
extern unsigned int _fmode;

FILE* __alloc_file(void);

//extern int _fmode;


FILE* fopen(const char *file, const char *mode)
{
  FILE *f;
  int fd, rw, oflags = 0;
   
  if (file == 0)
    return 0;
  if (mode == 0)
    return 0;

  f = __alloc_file();
  if (f == NULL)
    return NULL;

  rw = (strchr(mode, '+') == NULL) ? 0 : 1;
  if (strchr(mode, 'a'))
    oflags = O_CREAT | (rw ? O_RDWR : O_WRONLY);
  if (strchr(mode, 'r'))
    oflags = rw ? O_RDWR : O_RDONLY;
  if (strchr(mode, 'w'))
    oflags = O_TRUNC | O_CREAT | (rw ? O_RDWR : O_WRONLY);
  if (strchr(mode, 't'))
    oflags |= O_TEXT;
  else if (strchr(mode, 'b'))
    oflags |= O_BINARY;
  else
    oflags |= (_fmode & (O_TEXT|O_BINARY));

  fd = _open(file, oflags, 0);
  if (fd < 0)
    return NULL;

// msvcrt ensures that writes will end up at the end of file in append mode
// we just move the file pointer to the end of file initially

  if (strchr(mode, 'a'))
    lseek(fd, 0, SEEK_END);

  f->_cnt = 0;
  f->_file = fd;
  f->_bufsiz = 0;
  if (rw)
    f->_flag = _IOREAD | _IOWRT;
  else if (strchr(mode, 'r'))
    f->_flag = _IOREAD;
  else
    f->_flag = _IOWRT;

  if (strchr(mode, 't'))
    f->_flag |= _IOTEXT;
  else if (strchr(mode, 'b'))
    f->_flag |= _IOBINARY;
  else if (_fmode & O_BINARY)
    f->_flag |= _IOBINARY;

  f->_base = f->_ptr = NULL;
  return f;
}

FILE* _wfopen(const wchar_t *file, const wchar_t *mode)
{
  FILE *f;
  int fd, rw, oflags = 0;
   
  if (file == 0)
    return 0;
  if (mode == 0)
    return 0;

  f = __alloc_file();
  if (f == NULL)
    return NULL;

  rw = (wcschr(mode, L'+') == NULL) ? 0 : 1;
  if (wcschr(mode, L'a'))
    oflags = O_CREAT | (rw ? O_RDWR : O_WRONLY);
  if (wcschr(mode, L'r'))
    oflags = rw ? O_RDWR : O_RDONLY;
  if (wcschr(mode, L'w'))
    oflags = O_TRUNC | O_CREAT | (rw ? O_RDWR : O_WRONLY);
  if (wcschr(mode, L't'))
    oflags |= O_TEXT;
  else if (wcschr(mode, L'b'))
    oflags |= O_BINARY;
  else
    oflags |= (_fmode & (O_TEXT|O_BINARY));

  fd = _wopen(file, oflags, 0);
  if (fd < 0)
    return NULL;

// msvcrt ensures that writes will end up at the end of file in append mode
// we just move the file pointer to the end of file initially
  if (wcschr(mode, 'a'))
    lseek(fd, 0, SEEK_END);

  f->_cnt = 0;
  f->_file = fd;
  f->_bufsiz = 0;
  if (rw)
    f->_flag = _IOREAD | _IOWRT;
  else if (wcschr(mode, L'r'))
    f->_flag = _IOREAD;
  else
    f->_flag = _IOWRT;

  if (wcschr(mode, L't'))
    f->_flag |= _IOTEXT;
  else if (wcschr(mode, L'b'))
    f->_flag |= _IOBINARY;
  else if (_fmode & O_BINARY)
    f->_flag |= _IOBINARY;

  f->_base = f->_ptr = NULL;
  return f;
}

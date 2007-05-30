/* $Id$
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

#include <precomp.h>

#include <tchar.h>

//might change fopen(file,mode) -> fsopen(file,mode,_SH_DENYNO);

FILE* _tfopen(const _TCHAR *file, const _TCHAR *mode)
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

  rw = (_tcschr(mode, '+') == NULL) ? 0 : 1;
  if (_tcschr(mode, 'a'))
    oflags = O_CREAT | (rw ? O_RDWR : O_WRONLY);
  if (_tcschr(mode, 'r'))
    oflags = rw ? O_RDWR : O_RDONLY;
  if (_tcschr(mode, 'w'))
    oflags = O_TRUNC | O_CREAT | (rw ? O_RDWR : O_WRONLY);
  if (_tcschr(mode, 't'))
    oflags |= O_TEXT;
  else if (_tcschr(mode, 'b'))
    oflags |= O_BINARY;
  else
    oflags |= (_fmode& (O_TEXT|O_BINARY));

  fd = _topen(file, oflags, 0);
  if (fd < 0)
    return NULL;

// msvcrt ensures that writes will end up at the end of file in append mode
// we just move the file pointer to the end of file initially

  if (_tcschr(mode, 'a'))
    _lseek(fd, 0, SEEK_END);

  f->_cnt = 0;
  f->_file = fd;
  f->_bufsiz = 0;
  if (rw)
    f->_flag = _IOREAD | _IOWRT;
  else if (_tcschr(mode, 'r'))
    f->_flag = _IOREAD;
  else
    f->_flag = _IOWRT;

  if (_tcschr(mode, 't'))
    f->_flag |= _IOTEXT;
  else if (_tcschr(mode, 'b'))
    f->_flag |= _IOBINARY;
  else if (_fmode& O_BINARY)
    f->_flag |= _IOBINARY;

  f->_base = f->_ptr = NULL;
  return f;
}

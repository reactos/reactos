/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <libc/file.h>
#include <share.h>


FILE *	__alloc_file(void);

FILE *
tmpfile(void)
{
  int  temp_fd;
  FILE *f;
  char *temp_name = tmpnam(0);
  char *n_t_r = (char *)malloc(L_tmpnam);

  if (!n_t_r)
    return 0;

  /* We could have a race condition, whereby another program
     (in another virtual machine, or if the temporary file is
     in a directory which is shared via a network) opens the
     file returned by `tmpnam' between the call above and the
     moment when we actually open the file below.  This loop
     retries the call to `tmpnam' until we actually succeed
     to create the file which didn't exist before.  */
  do {
    errno = 0;
    temp_fd = _open(temp_name, 0, SH_DENYRW);
  } while (temp_fd == -1 && errno != ENOENT && (temp_name = tmpnam(0)) != 0);

  if (temp_name == 0)
    return 0;

  /* This should have been fdopen(temp_fd, "wb+"), but `fdopen'
     is non-ANSI.  So we need to dump some of its guts here.  Sigh...  */
  f = __alloc_file();
  if (f)
  {
    f->_file   = temp_fd;
    f->_cnt    = 0;
    f->_bufsiz = 0;
    f->_flag   = _IORMONCL | _IORW;
    f->_name_to_remove = n_t_r;
    strcpy(f->_name_to_remove, temp_name);
    f->_base = f->_ptr = NULL;
  }
  else
  {
    close(temp_fd);
    remove(temp_name);
    free(n_t_r);
  }
  return f;
}

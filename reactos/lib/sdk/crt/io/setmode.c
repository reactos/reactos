/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/setmode.c
 * PURPOSE:     Sets the file translation mode
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <precomp.h>

__inline BOOL is_valid_fd(int fd);

/*
 * @implemented
 */
int _setmode(int fd, int newmode)
{
   int prevmode;

   TRACE("_setmode(%d, %d)", fd, newmode);

   if (!is_valid_fd(fd))
   {
      ERR("_setmode: inval fd (%d)\n",fd);
      //errno = EBADF;
      return(-1);
   }

   if (newmode & ~(_O_TEXT|_O_BINARY))
   {
      ERR("_setmode: fd (%d) mode (0x%08x) unknown\n",fd,newmode);
      /* FIXME: Should we fail with EINVAL here? */
   }

   prevmode = fdinfo(fd)->fdflags & FTEXT ? _O_TEXT : _O_BINARY;

   if ((newmode & _O_TEXT) == _O_TEXT)
   {
      fdinfo(fd)->fdflags |= FTEXT;
   }
   else
   {
      /* FIXME: If both _O_TEXT and _O_BINARY are set, we get here.
       * Should we fail with EINVAL instead? -Gunnar
       */
      fdinfo(fd)->fdflags &= ~FTEXT;
   }

   return(prevmode);
}



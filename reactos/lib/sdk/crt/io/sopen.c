/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Created
 */

#include <precomp.h>

/*
 * @implemented
 */
/*********************************************************************
 *              _sopen (MSVCRT.@)
 */
int CDECL _sopen( const char *path, int oflags, int shflags, ... )
{
  va_list ap;
  int pmode;
  DWORD access = 0, creation = 0, attrib;
  DWORD sharing;
  int wxflag = 0, fd;
  HANDLE hand;
  SECURITY_ATTRIBUTES sa;


  TRACE(":file (%s) oflags: 0x%04x shflags: 0x%04x\n",
        path, oflags, shflags);

  wxflag = split_oflags(oflags);
  switch (oflags & (O_RDONLY | O_WRONLY | O_RDWR))
  {
  case O_RDONLY: access |= GENERIC_READ; break;
  case O_WRONLY: access |= GENERIC_WRITE; break;
  case O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & O_CREAT)
  {
    va_start(ap, shflags);
      pmode = va_arg(ap, int);
    va_end(ap);

    if(pmode & ~(S_IREAD | S_IWRITE))
      FIXME(": pmode 0x%04x ignored\n", pmode);
    else
      WARN(": pmode 0x%04x ignored\n", pmode);

    if (oflags & O_EXCL)
      creation = CREATE_NEW;
    else if (oflags & O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  }
  else  /* no O_CREAT */
  {
    if (oflags & O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }
  
  switch( shflags )
  {
    case SH_DENYRW:
      sharing = 0L;
      break;
    case SH_DENYWR:
      sharing = FILE_SHARE_READ;
      break;
    case SH_DENYRD:
      sharing = FILE_SHARE_WRITE;
      break;
    case SH_DENYNO:
      sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
    default:
      ERR( "Unhandled shflags 0x%x\n", shflags );
      return -1;
  }
  attrib = FILE_ATTRIBUTE_NORMAL;

  if (oflags & O_TEMPORARY)
  {
      attrib |= FILE_FLAG_DELETE_ON_CLOSE;
      access |= DELETE;
      sharing |= FILE_SHARE_DELETE;
  }

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = (oflags & O_NOINHERIT) ? FALSE : TRUE;

  hand = CreateFileA(path, access, sharing, &sa, creation, attrib, 0);

  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%d)\n",GetLastError());
    _dosmaperr(GetLastError());
    return -1;
  }

  fd = alloc_fd(hand, wxflag);

  TRACE(":fd (%d) handle (%p)\n",fd, hand);
  return fd;
}


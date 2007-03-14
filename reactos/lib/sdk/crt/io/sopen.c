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
int _sopen(const char *path, int access, int shflag, ... /*mode, permissin*/)
{
   //FIXME: vararg
  return _open((path), (access)|(shflag));//, (mode));
}

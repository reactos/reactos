/* $Id: safeobj.c,v 1.2 2002/02/20 09:17:57 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/safeobj.c
 * PURPOSE:     safe checking of user-provided objects
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              09/01/2002: Created
 */

#include <psx/safeobj.h>
#include <psx/debug.h>

int __safeobj_validate(void *obj, __magic_t refsignature)
{
 if(obj == 0)
  return (0);
 else
 {
  /* cast the object to a magic number */
  __magic_t mSignature = *((__magic_t *)obj);

  ERRIF
  (
   mSignature != refsignature,
   "invalid object at %X: signature is \"%c%c%c%c\", should be \"%c%c%c%c\"",
   obj,
   MAGIC_DECOMPOSE(refsignature),
   MAGIC_DECOMPOSE(mSignature)
  );

  if(mSignature == refsignature)
   /* signatures match: ok */
   return (-1);
  else
   /* signatures mismatch: fail */
   return (0);

 }
}

/* EOF */


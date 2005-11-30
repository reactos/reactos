/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <internal/tls.h>

/*
 * @implemented
 */
int * __fpecode(void)
{
  return(&(GetThreadData()->fpecode));
}

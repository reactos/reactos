/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/purecall.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     Eric Kohl <ekohl@zr-online.de>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

/* FUNCTIONS ****************************************************************/

void _purecall(void)
{
  ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

/* EOF */

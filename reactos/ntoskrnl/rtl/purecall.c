/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Message table functions
 * FILE:              ntoskrnl/rtl/message.c
 * PROGRAMER:         Eric Kohl <ekohl@zr-online.de>
 * REVISION HISTORY:
 *                    29/05/2001: Created
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

/* FUNCTIONS ****************************************************************/

void _purecall(void)
{
  ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

/* EOF */

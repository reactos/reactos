/* $Id: handle.c,v 1.1 1999/12/26 15:50:53 dwelch Exp $
 *
 * reactos/subsys/csrss/api/handle.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include "csrss.h"
#include "api.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrCreateObject(PHANDLE Handle,
			 PVOID Object)

/* $Id: enum.c,v 1.1 2001/08/21 20:18:27 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/enum.c
 * PURPOSE:         Motherboard device enumerator
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  Created 01/05/2001
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>

#define NDEBUG
#include <internal/debug.h>

VOID
HalpStartEnumerator (VOID)
{
#ifdef ACPI

  UNICODE_STRING DriverName;

  RtlInitUnicodeString(&DriverName,
    L"\\SystemRoot\\system32\\drivers\\acpi.sys");
  NtLoadDriver(&DriverName);

#endif /* ACPI */
}

/* EOF */

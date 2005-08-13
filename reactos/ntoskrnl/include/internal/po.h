/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/po.h
 * PURPOSE:         Internal power manager declarations
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  01/05/2001  Created
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_PO_H
#define __NTOSKRNL_INCLUDE_INTERNAL_PO_H

extern PDEVICE_NODE PopSystemPowerDeviceNode;

VOID
PoInit(PLOADER_PARAMETER_BLOCK LoaderBlock, BOOLEAN ForceAcpiDisable);

NTSTATUS
PopSetSystemPowerState(
  SYSTEM_POWER_STATE PowerState);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_PO_H */

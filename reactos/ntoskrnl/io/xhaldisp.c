/* $Id: xhaldisp.c,v 1.4 2001/06/08 15:11:04 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/xhaldisp.c
 * PURPOSE:         Hal dispatch tables
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 19/06/2000
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/xhal.h>

/* DATA **********************************************************************/


HAL_DISPATCH EXPORTED HalDispatchTable =
{
	HAL_DISPATCH_VERSION,
	NULL,			// HalQuerySystemInformation
	NULL,			// HalSetSystemInformation
	NULL,			// HalQueryBusSlots
	NULL,			// HalDeviceControl
	xHalExamineMBR,
	xHalIoAssignDriveLetters,
	xHalIoReadPartitionTable,
	xHalIoSetPartitionInformation,
	xHalIoWritePartitionTable,
	NULL,			// HalReferenceHandlerForBus
	NULL,			// HalReferenceBusHandler
	NULL			// HalDereferenceBusHandler
};


HAL_PRIVATE_DISPATCH EXPORTED HalPrivateDispatchTable =
{
	HAL_PRIVATE_DISPATCH_VERSION
				// HalHandlerForBus
				// HalHandlerForConfigSpace
				// HalCompleteDeviceControl
				// HalRegisterBusHandler
				// any more??
};

/* EOF */


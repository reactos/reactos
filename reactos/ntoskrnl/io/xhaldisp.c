/* $Id: xhaldisp.c,v 1.5 2002/05/05 14:57:43 chorns Exp $
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
	(pHalQuerySystemInformation) NULL,	// HalQuerySystemInformation
	(pHalSetSystemInformation) NULL,	// HalSetSystemInformation
	(pHalQueryBusSlots) NULL,			// HalQueryBusSlots
	(pHalDeviceControl) NULL,			// HalDeviceControl
	(pHalExamineMBR) xHalExamineMBR,
	(pHalIoAssignDriveLetters) xHalIoAssignDriveLetters,
	(pHalIoReadPartitionTable) xHalIoReadPartitionTable,
	(pHalIoSetPartitionInformation) xHalIoSetPartitionInformation,
	(pHalIoWritePartitionTable) xHalIoWritePartitionTable,
	(pHalHandlerForBus) NULL,			// HalReferenceHandlerForBus
	(pHalReferenceBusHandler) NULL,		// HalReferenceBusHandler
	(pHalReferenceBusHandler) NULL		// HalDereferenceBusHandler
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


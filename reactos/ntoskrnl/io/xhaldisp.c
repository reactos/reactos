/* $Id: xhaldisp.c,v 1.6 2002/09/07 15:12:53 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* DATA **********************************************************************/

HAL_DISPATCH_TABLE IopHalDispatchTable =
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

PHAL_DISPATCH_TABLE HalDispatchTable = &IopHalDispatchTable;


HAL_PRIVATE_DISPATCH_TABLE IopHalPrivateDispatchTable =
{
	HAL_PRIVATE_DISPATCH_VERSION
				// HalHandlerForBus
				// HalHandlerForConfigSpace
				// HalCompleteDeviceControl
				// HalRegisterBusHandler
				// any more??
};

PHAL_PRIVATE_DISPATCH_TABLE HalPrivateDispatchTable = &IopHalPrivateDispatchTable;

/* EOF */


/* $Id: halfuncs.h,v 1.1.2.1 2004/10/24 23:09:22 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/halfuncs.h
 * PURPOSE:         Prototypes for exported Hardware Abstraction Layer Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _HALFUNCS_H
#define _HALFUNCS_H

#include "haltypes.h"

BOOLEAN
STDCALL
HalAllProcessorsStarted(VOID);

BOOLEAN 
STDCALL
HalBeginSystemInterrupt(
	ULONG Vector,
	KIRQL Irql,
	PKIRQL OldIrql
);

BOOLEAN
STDCALL
HalDisableSystemInterrupt(
	ULONG Vector,
	KIRQL Irql
);

BOOLEAN
STDCALL
HalEnableSystemInterrupt(
	ULONG Vector,
	KIRQL Irql,
	KINTERRUPT_MODE InterruptMode
);

VOID
STDCALL
HalEndSystemInterrupt(
	KIRQL Irql,
	ULONG Unknown2
);

BOOLEAN
STDCALL
HalGetEnvironmentVariable(
	PCH Name,
	PCH Value,
	USHORT ValueLength
);

VOID
STDCALL
HalInitializeProcessor(
	ULONG ProcessorNumber,
	PVOID ProcessorStack
);

BOOLEAN
STDCALL
HalInitSystem(
	ULONG BootPhase,
	PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
STDCALL
HalQueryDisplayOwnership(VOID);

VOID
STDCALL
HalReportResourceUsage(VOID);

VOID
FASTCALL
HalRequestSoftwareInterrupt(
	KIRQL SoftwareInterruptRequested
);

VOID
STDCALL
HalReleaseDisplayOwnership(VOID);

VOID
STDCALL
HalReturnToFirmware(
	ULONG Action
);

BOOLEAN
STDCALL
HalSetEnvironmentVariable(
	IN PCH Name,
	IN PCH Value
);

VOID
STDCALL
IoAssignDriveLetters(
	PLOADER_PARAMETER_BLOCK LoaderBlock,
	PSTRING NtDeviceName,
	PUCHAR NtSystemPath,
	PSTRING NtSystemPathString
);
  
/* The Following Functions are called through the Dispatch Table */
#define HalQuerySystemInformation	HALDISPATCH->HalQuerySystemInformation
#define HalIoAssignDriveLetters		HALDISPATCH->HalIoAssignDriveLetters
#endif

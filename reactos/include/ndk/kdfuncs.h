/* $Id: kdfuncs.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
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
 * FILE:            include/ndk/kdfuncs.h
 * PURPOSE:         Prototypes for Kernel Debugger Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KDFUNCS_H
#define _KDFUNCS_H

#include "kdtypes.h"

BYTE
STDCALL
KdPollBreakIn(VOID);

BOOLEAN
STDCALL
KdPortInitialize(
	PKD_PORT_INFORMATION PortInformation,
	DWORD Unknown1,
	DWORD Unknown2
);

BOOLEAN
STDCALL
KdPortInitializeEx(
	PKD_PORT_INFORMATION PortInformation,
	DWORD Unknown1,
	DWORD Unknown2
);

BOOLEAN
STDCALL
KdPortGetByte(
	PUCHAR ByteRecieved
);

BOOLEAN
STDCALL
KdPortGetByteEx(
	PKD_PORT_INFORMATION PortInformation,
	PUCHAR ByteRecieved
);

BOOLEAN
STDCALL
KdPortPollByte(
	PUCHAR ByteRecieved
);

BOOLEAN
STDCALL
KdPortPollByteEx(
	PKD_PORT_INFORMATION PortInformation,
	PUCHAR ByteRecieved
);

VOID
STDCALL
KdPortPutByte(
	UCHAR ByteToSend
);

VOID
STDCALL
KdPortPutByteEx(
	PKD_PORT_INFORMATION PortInformation,
	UCHAR ByteToSend
);

VOID
STDCALL
KdPortRestore(VOID);

VOID
STDCALL
KdPortSave (VOID);

BOOLEAN
STDCALL
KdPortDisableInterrupts(VOID);

BOOLEAN
STDCALL
KdPortEnableInterrupts(VOID);

#endif

/* $Id: inbvfuncs.h,v 1.1.2.1 2004/10/24 23:09:22 ion Exp $
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
 * FILE:            include/ndk/haltypes.h
 * PURPOSE:         Prototypes for Boot Video Driver not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _INBVFUNCS_H
#define _INBVFUNCS_H

VOID
STDCALL
InbvAcquireDisplayOwnership(VOID);

BOOLEAN
STDCALL
InbvCheckDisplayOwnership(VOID);

BOOLEAN
STDCALL
InbvDisplayString(
	IN PCHAR String
);

VOID
STDCALL
InbvEnableBootDriver(
	IN BOOLEAN Enable
);

BOOLEAN
STDCALL
InbvEnableDisplayString(
	IN BOOLEAN Enable
);

VOID
STDCALL
InbvInstallDisplayStringFilter(
	IN PVOID Unknown
);

BOOLEAN
STDCALL
InbvIsBootDriverInstalled(VOID);

VOID
STDCALL
InbvNotifyDisplayOwnershipLost(
	IN PVOID Callback
);

BOOLEAN
STDCALL
InbvResetDisplay(VOID);

VOID
STDCALL
InbvSetScrollRegion(
	IN ULONG Left,
	IN ULONG Top,
	IN ULONG Width,
	IN ULONG Height
);

VOID
STDCALL
InbvSetTextColor(
	IN ULONG Color
);

VOID
STDCALL
InbvSolidColorFill(
	IN ULONG Left,
	IN ULONG Top,
	IN ULONG Width,
	IN ULONG Height,
	IN ULONG Color
);

VOID
STDCALL
VidCleanUp(VOID);

BOOLEAN
STDCALL
VidInitialize(VOID);

BOOLEAN
STDCALL
VidResetDisplay(VOID);

BOOLEAN
STDCALL
VidIsBootDriverInstalled(VOID);

#endif

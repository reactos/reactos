/* $Id: kefuncs.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
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
 * FILE:            include/ndk/kefuncs.h
 * PURPOSE:         Prototypes for Kernel Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KEFUNCS_H
#define _KEFUNCS_H

#include "ketypes.h"

VOID 
STDCALL
KeInitializeApc(
	IN PKAPC  Apc,
	IN PKTHREAD  Thread,
	IN KAPC_ENVIRONMENT  TargetEnvironment,
	IN PKKERNEL_ROUTINE  KernelRoutine,
	IN PKRUNDOWN_ROUTINE  RundownRoutine OPTIONAL,
	IN PKNORMAL_ROUTINE  NormalRoutine,
	IN KPROCESSOR_MODE  Mode,
	IN PVOID  Context
);	

VOID
STDCALL
KeEnterKernelDebugger(VOID);

VOID
FASTCALL
KiAcquireSpinLock(
	PKSPIN_LOCK SpinLock
);

VOID
FASTCALL
KiReleaseSpinLock(
	PKSPIN_LOCK SpinLock
);

VOID
STDCALL
KiDeliverApc(
	IN KPROCESSOR_MODE  PreviousMode,
	IN PKEXCEPTION_FRAME  ExceptionFrame,
	IN PKTRAP_FRAME  TrapFrame
);

VOID
STDCALL
KiDispatchInterrupt(VOID);

#endif

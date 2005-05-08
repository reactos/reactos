/* $Id: ketypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
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
 * FILE:            include/ndk/ketypes.h
 * PURPOSE:         Definitions for Kernel Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KETYPES_H
#define _KETYPES_H

#include <reactos/helper.h>
#include "haltypes.h"

/* Exported Kernel Variables */
#ifdef __NTOSKRNL__
extern CHAR EXPORTED KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
extern ULONG EXPORTED KeDcacheFlushCount;
extern ULONG EXPORTED KeIcacheFlushCount;
extern KAFFINITY EXPORTED KeActiveProcessors;
extern ULONG EXPORTED KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG EXPORTED KeMaximumIncrement;
extern ULONG EXPORTED KeMinimumIncrement;
#else
extern KAFFINITY IMPORTED KeActiveProcessors;
extern LOADER_PARAMETER_BLOCK IMPORTED KeLoaderBlock;
extern ULONG IMPORTED KeDcacheFlushCount;
extern ULONG IMPORTED KeIcacheFlushCount;
extern ULONG IMPORTED KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG IMPORTED KeMaximumIncrement;
extern ULONG IMPORTED KeMinimumIncrement;
#endif

/* System Call Table Internal Defintions */
#define SSDT_MAX_ENTRIES 4

#define PROCESSOR_FEATURE_MAX 64

#if defined(__NTOSKRNL__)
extern SSDT_ENTRY EXPORTED KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern SSDT_ENTRY EXPORTED KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];
#else
extern SSDT_ENTRY IMPORTED KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern SSDT_ENTRY IMPORTED KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];
#endif

typedef enum _KAPC_ENVIRONMENT {
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment
} KAPC_ENVIRONMENT;

typedef struct _KDPC_DATA {
	LIST_ENTRY  DpcListHead;
	ULONG  DpcLock;
	ULONG  DpcQueueDepth;
	ULONG  DpcCount;
} KDPC_DATA, *PKDPC_DATA;

typedef struct _KTRAP_FRAME {
	PVOID DebugEbp;
	PVOID DebugEip;
	PVOID DebugArgMark;
	PVOID DebugPointer;
	PVOID TempCs;
	PVOID TempEip;
	ULONG Dr0;
	ULONG Dr1;
	ULONG Dr2;
	ULONG Dr3;
	ULONG Dr6;
	ULONG Dr7;
	USHORT Gs;
	USHORT Reserved1;
	USHORT Es;
	USHORT Reserved2;
	USHORT Ds;
	USHORT Reserved3;
	ULONG Edx;
	ULONG Ecx;
	ULONG Eax;
	ULONG PreviousMode;
	PVOID ExceptionList;
	USHORT Fs;
	USHORT Reserved4;
	ULONG Edi;
	ULONG Esi;
	ULONG Ebx;
	ULONG Ebp;
	ULONG ErrorCode;
	ULONG Eip;
	ULONG Cs;
	ULONG Eflags;
	ULONG Esp;
	USHORT Ss;
	USHORT Reserved5;
	USHORT V86_Es;
	USHORT Reserved6;
	USHORT V86_Ds;
	USHORT Reserved7;
	USHORT V86_Fs;
	USHORT Reserved8;
	USHORT V86_Gs;
	USHORT Reserved9;
} KTRAP_FRAME, *PKTRAP_FRAME;

/* i386 Doesn't have Exception Frames */
typedef struct _KEXCEPTION_FRAME {

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

typedef struct _KINTERRUPT {
	CSHORT              Type;
	CSHORT              Size;
	LIST_ENTRY          InterruptListEntry;
	PKSERVICE_ROUTINE   ServiceRoutine;
	PVOID               ServiceContext;
	KSPIN_LOCK          SpinLock;
	ULONG               TickCount;
	PKSPIN_LOCK         ActualLock;
	PVOID               DispatchAddress;
	ULONG               Vector;
	KIRQL               Irql;
	KIRQL               SynchronizeIrql;
	BOOLEAN             FloatingSave;
	BOOLEAN             Connected;
	CHAR                Number;
	UCHAR               ShareVector;
	KINTERRUPT_MODE     Mode;
	ULONG               ServiceCount;
	ULONG               DispatchCount;
	ULONG               DispatchCode[106];
} KINTERRUPT, *PKINTERRUPT;

typedef enum _KERNEL_OBJECTS {
	KNotificationEvent = 0,
	KSynchronizationEvent = 1,
	KMutant = 2,
	KProcess = 3,
	KQueue = 4,
	KSemaphore = 5,
	KThread = 6,
	KNotificationTimer = 8,
	KSynchronizationTimer = 9,
	KApc = 18,
	KDpc = 19,
	KDeviceQueue = 20,
	KEventPair = 21,
	KInterrupt = 22,
	KProfile = 23
} KERNEL_OBJECTS;
#endif

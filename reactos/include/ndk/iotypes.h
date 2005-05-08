/*
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
 * FILE:            include/ndk/iotypes.h
 * PURPOSE:         Definitions for exported I/O Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */

#ifndef _IOTYPES_H
#define _IOTYPES_H

#include <reactos/helper.h>

#ifdef __NTOSKRNL__
extern POBJECT_TYPE EXPORTED IoAdapterObjectType;
extern POBJECT_TYPE EXPORTED IoDeviceHandlerObjectType;
extern POBJECT_TYPE EXPORTED IoDeviceObjectType;
extern POBJECT_TYPE EXPORTED IoDriverObjectType;
extern POBJECT_TYPE EXPORTED IoFileObjectType;
extern ULONG EXPORTED IoReadOperationCount;
extern ULONGLONG EXPORTED IoReadTransferCount;
extern ULONG EXPORTED IoWriteOperationCount;
extern ULONGLONG EXPORTED IoWriteTransferCount;
extern KSPIN_LOCK EXPORTED IoStatisticsLock;
#else
extern POBJECT_TYPE IMPORTED IoAdapterObjectType;
extern POBJECT_TYPE IMPORTED IoDeviceHandlerObjectType;
extern POBJECT_TYPE IMPORTED IoDeviceObjectType;
extern POBJECT_TYPE IMPORTED IoDriverObjectType;
extern POBJECT_TYPE IMPORTED IoFileObjectType;
extern ULONG IMPORTED IoReadOperationCount;
extern ULONGLONG IMPORTED IoReadTransferCount;
extern ULONG IMPORTED IoWriteOperationCount;
extern ULONGLONG IMPORTED IoWriteTransferCount;
extern KSPIN_LOCK IMPORTED IoStatisticsLock;
#endif

typedef struct _MAILSLOT_CREATE_PARAMETERS {
    ULONG           MailslotQuota;
    ULONG           MaximumMessageSize;
    LARGE_INTEGER   ReadTimeout;
    BOOLEAN         TimeoutSpecified;
} MAILSLOT_CREATE_PARAMETERS, *PMAILSLOT_CREATE_PARAMETERS;

typedef struct _NAMED_PIPE_CREATE_PARAMETERS {
    ULONG           NamedPipeType;
    ULONG           ReadMode;
    ULONG           CompletionMode;
    ULONG           MaximumInstances;
    ULONG           InboundQuota;
    ULONG           OutboundQuota;
    LARGE_INTEGER   DefaultTimeout;
    BOOLEAN         TimeoutSpecified;
} NAMED_PIPE_CREATE_PARAMETERS, *PNAMED_PIPE_CREATE_PARAMETERS;

/*
 * PURPOSE: Special timer associated with each device
 */
#define IO_TYPE_DRIVER 4L
#define IO_TYPE_TIMER 9L
typedef struct _IO_TIMER {
   USHORT Type;				/* Every IO Object has a Type */
   USHORT TimerEnabled;			/* Tells us if the Timer is enabled or not */
   LIST_ENTRY IoTimerList;		/* List of other Timers on the system */
   PIO_TIMER_ROUTINE TimerRoutine;	/* The associated timer routine */
   PVOID Context;			/* Context */
   PDEVICE_OBJECT DeviceObject;		/* Driver that owns this IO Timer */
} IO_TIMER, *PIO_TIMER;
#endif


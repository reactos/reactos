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

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/
extern POBJECT_TYPE NTOSAPI IoAdapterObjectType;
extern POBJECT_TYPE NTOSAPI IoDeviceHandlerObjectType;
extern POBJECT_TYPE NTOSAPI IoDeviceObjectType;
extern POBJECT_TYPE NTOSAPI IoDriverObjectType;
extern POBJECT_TYPE NTOSAPI IoFileObjectType;

/* CONSTANTS *****************************************************************/

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef struct _MAILSLOT_CREATE_PARAMETERS 
{
    ULONG           MailslotQuota;
    ULONG           MaximumMessageSize;
    LARGE_INTEGER   ReadTimeout;
    BOOLEAN         TimeoutSpecified;
} MAILSLOT_CREATE_PARAMETERS, *PMAILSLOT_CREATE_PARAMETERS;

typedef struct _NAMED_PIPE_CREATE_PARAMETERS 
{
    ULONG           NamedPipeType;
    ULONG           ReadMode;
    ULONG           CompletionMode;
    ULONG           MaximumInstances;
    ULONG           InboundQuota;
    ULONG           OutboundQuota;
    LARGE_INTEGER   DefaultTimeout;
    BOOLEAN         TimeoutSpecified;
} NAMED_PIPE_CREATE_PARAMETERS, *PNAMED_PIPE_CREATE_PARAMETERS;

typedef struct _IO_TIMER 
{
   USHORT Type;
   USHORT TimerEnabled;
   LIST_ENTRY IoTimerList;
   PIO_TIMER_ROUTINE TimerRoutine;
   PVOID Context;
   PDEVICE_OBJECT DeviceObject;
} IO_TIMER, *PIO_TIMER;

#endif


/* $Id: wmi.c,v 1.1 2004/06/23 21:42:50 ion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/wmi.c
 * PURPOSE:         Windows Management Instrumentation
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 23/06/04
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIRegistrationControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Action
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIAllocateInstanceIds(
    IN GUID *Guid,
    IN ULONG InstanceCount,
    OUT ULONG *FirstInstanceId
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMISuggestInstanceName(
    IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
    IN PUNICODE_STRING SymbolicLinkName OPTIONAL,
    IN BOOLEAN CombineNames,
    OUT PUNICODE_STRING SuggestedInstanceName
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIWriteEvent(
    IN PVOID WnodeEventItem
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS IoWMIOpenBlock(
    IN GUID *DataBlockGuid,
    IN ULONG DesiredAccess,
    OUT PVOID *DataBlockObject
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS IoWMIQueryAllData(
    IN PVOID DataBlockObject,
    IN OUT ULONG *InOutBufferSize,
    OUT PVOID OutBuffer
)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIQueryAllDataMultiple(
    IN PVOID *DataBlockObjectList,
    IN ULONG ObjectCount,
    IN OUT ULONG *InOutBufferSize,
    OUT PVOID OutBuffer
)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIQuerySingleInstance(
    IN PVOID DataBlockObject,
    IN PUNICODE_STRING InstanceName,
    IN OUT ULONG *InOutBufferSize,
    OUT PVOID OutBuffer
)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIQuerySingleInstanceMultiple(
    IN PVOID *DataBlockObjectList,
    IN PUNICODE_STRING InstanceNames,
    IN ULONG ObjectCount,
    IN OUT ULONG *InOutBufferSize,
    OUT PVOID OutBuffer
)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMISetSingleInstance(
    IN PVOID DataBlockObject,
    IN PUNICODE_STRING InstanceName,
    IN ULONG Version,
    IN ULONG ValueBufferSize,
    IN PVOID ValueBuffer
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMISetSingleItem(
    IN PVOID DataBlockObject,
    IN PUNICODE_STRING InstanceName,
    IN ULONG DataItemId,
    IN ULONG Version,
    IN ULONG ValueBufferSize,
    IN PVOID ValueBuffer
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIExecuteMethod(
    IN PVOID DataBlockObject,
    IN PUNICODE_STRING InstanceName,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN OUT PULONG OutBufferSize,
    IN OUT PUCHAR InOutBuffer
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMISetNotificationCallback(
    IN PVOID Object,
    IN WMI_NOTIFICATION_CALLBACK Callback,
    IN PVOID Context
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIHandleToInstanceName(
    IN PVOID DataBlockObject,
    IN HANDLE FileHandle,
    OUT PUNICODE_STRING InstanceName
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWMIDeviceObjectToInstanceName(
    IN PVOID DataBlockObject,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUNICODE_STRING InstanceName
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*Eof*/

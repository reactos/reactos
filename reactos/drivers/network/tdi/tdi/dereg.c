/* $Id$
 *
 */
#include <ntddk.h>
#include <tdi.h>

/* De-/Register Action IDs for TdiDeRegister */

typedef
enum
{
	R_NOTIFICATION_HANDLER = 0,
	DT_NOTIFICATION_HANDLER,
	R_DEVICE_OBJECT,
	D_DEVICE_OBJECT,
	R_ADDRESS_CHANGE_HANDLER,
	D_ADDRESS_CHANGE_HANDLER,
	R_NET_ADDRESS,
	D_NET_ADDRESS

} TDI_OBJECT_ACTION;


static
NTSTATUS
NTAPI
TdiDeRegister (
	IN	TDI_OBJECT_ACTION	Action,
	IN OUT	PVOID			Object
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/* ADDRESS_CHANGE_HANDLER */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterAddressChangeHandler (
	ULONG	Unknown0,
	ULONG	Unknown1,
	ULONG	Unknown2
	)
{
#if 0
	NTSTATUS	Status = STATUS_SUCCESS;
	Status = TdiDeRegister (
			R_ADDRESS_CHANGE_HANDLER,
			AddressChangeHandler
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterAddressChangeHandler (
	IN	PVOID	AddressChangeHandler
	)
{
	return TdiDeRegister (
			D_ADDRESS_CHANGE_HANDLER,
			AddressChangeHandler
			);
}


/* DEVICE_OBJECT */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterDeviceObject (
	ULONG	Unknown0,
	ULONG	Unknown1
	)
{
#if 0
	NTSTATUS	Status = STATUS_SUCCESS;
	Status = TdiDeRegister (
			R_DEVICE_OBJECT,
			DeviceObject
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterDeviceObject (
	IN	PVOID	DeviceObject
	)
{
	return TdiDeRegister (
			D_DEVICE_OBJECT,
			DeviceObject
			);
}


/* NET_ADDRESS */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterNetAddress (
	ULONG	Unknown0,
	ULONG	Unknown1
	)
{
#if 0
	NTSTATUS	Status = STATUS_SUCCESS;
	Status = TdiDeRegister (
			R_NET_ADDRESS,
			NetAddress
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterNetAddress (
	IN	PVOID	NetAddress
	)
{
	return TdiDeRegister (
			D_NET_ADDRESS,
			NetAddress
			);
}


/* NOTIFICATION_HANDLER */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterNotificationHandler (
	ULONG	Unknown0,
	ULONG	Unknown1,
	ULONG	Unknown2
	)
{
#if 0
	NTSTATUS	Status = STATUS_SUCCESS;
	Status = TdiDeRegister (
			R_NOTIFICATION_HANDLER,
			NotificationHandler
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterNotificationHandler (
	IN	PVOID	NotificationHandler
	)
{
	return	TdiDeRegister (
			DT_NOTIFICATION_HANDLER,
			NotificationHandler
			);
}


/* EOF */

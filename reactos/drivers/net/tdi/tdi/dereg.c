/* $Id: dereg.c,v 1.3 2002/09/07 15:12:07 chorns Exp $
 *
 */
#include <ddk/ntddk.h>
#include <ddk/tdi.h>

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
STDCALL
TdiDeRegister (
	IN	TDI_OBJECT_ACTION	Action,
	IN OUT	PVOID			Object
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/* ADDRESS_CHANGE_HANDLER */

NTSTATUS
STDCALL
TdiRegisterAddressChangeHandler (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	NTSTATUS	Status = STATUS_SUCCESS;
#if 0	
	Status = TdiDeRegister (
			R_ADDRESS_CHANGE_HANDLER,
			AddressChangeHandler
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
TdiRegisterDeviceObject (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	NTSTATUS	Status = STATUS_SUCCESS;
#if 0	
	Status = TdiDeRegister (
			R_DEVICE_OBJECT,
			DeviceObject
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
TdiRegisterNetAddress (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	NTSTATUS	Status = STATUS_SUCCESS;
#if 0	
	Status = TdiDeRegister (
			R_NET_ADDRESS,
			NetAddress
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
TdiRegisterNotificationHandler (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	NTSTATUS	Status = STATUS_SUCCESS;
#if 0	
	Status = TdiDeRegister (
			R_NOTIFICATION_HANDLER,
			NotificationHandler
			);
#endif
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
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

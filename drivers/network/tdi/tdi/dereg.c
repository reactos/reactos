#include "precomp.h"

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


/* ADDRESS_CHANGE_HANDLER */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterAddressChangeHandler(IN TDI_ADD_ADDRESS_HANDLER AddHandler,
                                IN TDI_DEL_ADDRESS_HANDLER DeleteHandler,
                                OUT HANDLE *BindingHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterAddressChangeHandler(IN HANDLE BindingHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}


/* DEVICE_OBJECT */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterDeviceObject(IN PUNICODE_STRING DeviceName,
                        OUT HANDLE *RegistrationHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterDeviceObject(IN HANDLE RegistrationHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}


/* NET_ADDRESS */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterNetAddress(IN PTA_ADDRESS Address,
                      IN PUNICODE_STRING DeviceName,
                      IN PTDI_PNP_CONTEXT Context,
                      OUT HANDLE *RegistrationHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterNetAddress(IN HANDLE RegistrationHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}


/* NOTIFICATION_HANDLER */

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiRegisterNotificationHandler(IN TDI_BIND_HANDLER BindHandler,
                               IN TDI_UNBIND_HANDLER UnbindHandler,
                               OUT HANDLE *BindingHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
TdiDeregisterNotificationHandler(IN HANDLE BindingHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

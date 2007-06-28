
#ifndef   __DBCLIB_H__
#define   __DBCLIB_H__

#define DBCLASS_VERSION 0x10000002

DECLSPEC_IMPORT
NTSTATUS
DBCLASS_RegisterController(
    IN ULONG DbclassVersion,
    IN PDEVICE_OBJECT ControllerFdo,
    IN PDEVICE_OBJECT TopOfStack,
    IN PDEVICE_OBJECT ControllerPdo,
    IN ULONG ControllerSig);

DECLSPEC_IMPORT
NTSTATUS
DBCLASS_UnRegisterController(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PDEVICE_OBJECT TopOfStack );

DECLSPEC_IMPORT
NTSTATUS
DBCLASS_ClassDispatch(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass);

DECLSPEC_IMPORT
NTSTATUS
DBCLASS_FilterDispatch(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PIRP Irp);

DECLSPEC_IMPORT
NTSTATUS
DBCLASS_SetD0_Complete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

DECLSPEC_IMPORT
NTSTATUS
DBCLASS_RegisterBusFilter(
    IN ULONG DbclassVersion,
    IN PDRIVER_OBJECT BusFilterDriverObject,
    IN PDEVICE_OBJECT FilterFdo
    );



#endif

#endif



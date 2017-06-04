#include "usbxhci_new.h"

#include <debug.h>

#define NDEBUG_EHCI_TRACE
#include "dbg_xhci.h"

USBPORT_REGISTRATION_PACKET RegPacket;


MPSTATUS
NTAPI
XHCI_StartController(IN PVOID ehciExtension,
                     IN PUSBPORT_RESOURCES Resources)
{
    DPRINT1("XHCI_StartController: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_Unload(PDRIVER_OBJECT DriverObject)
{
    DPRINT1("XHCI_Unload: UNIMPLEMENTED. FIXME\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT("DriverEntry: DriverObject - %p, RegistryPath - %wZ\n",
           DriverObject,
           RegistryPath);
    if (USBPORT_GetHciMn() != USBPORT_HCI_MN) // Don't know the purpose
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(&RegPacket, sizeof(USBPORT_REGISTRATION_PACKET));

    DriverObject->DriverUnload = XHCI_Unload;
    return STATUS_SUCCESS;
    //return USBPORT_RegisterUSBPortDriver(DriverObject, 200, &RegPacket); // 200- is version for usb 2... 
}
#pragma once

#define _HIDPI_NO_FUNCTION_MACROS_
#include <ntddk.h>
#include <hidclass.h>
#include <hidpddi.h>
#include <hidpi.h>
#include <debug.h>
#include <ntddmou.h>
#include <kbdmou.h>


typedef struct
{
    PDEVICE_OBJECT NextDeviceObject;
    PIRP Irp;
    KEVENT Event;
    PDEVICE_OBJECT ClassDeviceObject;
    PVOID ClassService;
    USHORT Buttons;
    USHORT MouseIdentifier;
    USHORT WheelUsagePage;
}MOUHID_DEVICE_EXTENSION, *PMOUHID_DEVICE_EXTENSION;
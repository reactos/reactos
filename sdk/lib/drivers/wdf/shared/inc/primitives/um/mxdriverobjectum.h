/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxDriverObjectUm.h

Abstract:

    User Mode implementation of Driver Object defined in MxDriverObject.h

--*/

#pragma once

typedef PDRIVER_OBJECT_UM MdDriverObject;
typedef DRIVER_ADD_DEVICE_UM MdDriverAddDeviceType, *MdDriverAddDevice;
typedef DRIVER_UNLOAD_UM MdDriverUnloadType, *MdDriverUnload;
typedef DRIVER_DISPATCH_UM MdDriverDispatchType, *MdDriverDispatch;

#include "MxDriverObject.h"



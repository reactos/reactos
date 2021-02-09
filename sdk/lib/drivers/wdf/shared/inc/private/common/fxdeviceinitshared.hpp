/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceInitShared.hpp

Abstract:

    This header is split from FxDeviceInit.hpp to have definitions needed
    in shared code.

Author:


Environment:


Revision History:

--*/

#ifndef __FXDEVICEINITSHARED_HPP__
#define __FXDEVICEINITSHARED_HPP__

struct PdoInit {

    PdoInit(
        VOID
        )
    {
        DeviceText.Next = NULL;
        LastDeviceTextEntry = &DeviceText.Next;
    }

    WDF_PDO_EVENT_CALLBACKS EventCallbacks;

    CfxDevice* Parent;

    FxString* DeviceID;

    FxString* InstanceID;

    FxCollectionInternal HardwareIDs;

    FxCollectionInternal CompatibleIDs;

    FxString* ContainerID;

    SINGLE_LIST_ENTRY DeviceText;
    PSINGLE_LIST_ENTRY* LastDeviceTextEntry;

    FxDeviceDescriptionEntry* DescriptionEntry;

    LCID DefaultLocale;

    BOOLEAN Raw;

    BOOLEAN Static;

    BOOLEAN ForwardRequestToParent;
};

#endif //__FXDEVICEINITSHARED_HPP__

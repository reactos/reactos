/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxTypes.h

Abstract:

    This defines the memory tags for the frameworks objects

Author:




Revision History:


--*/

#ifndef _FXTYPES_H
#define _FXTYPES_H

//
// Might be expanded to a ULONG if we need the storage
//
typedef USHORT WDFTYPE;


enum FX_OBJECT_TYPES_BASE {

    FX_TYPES_BASE             = 0x1000,
    FX_TYPES_PACKAGES_BASE    = 0x1100,
    FX_TYPES_IO_TARGET_BASE   = 0x1200,
    FX_ABSTRACT_TYPES_BASE    = 0x1300,
    FX_TYPES_DMA_BASE         = 0x1400,
    FX_TYPES_INTERFACES_BASE  = 0x1500,
};

enum FX_OBJECT_TYPES {
    // Use Hex numbers since this the kd default dump value

    FX_TYPE_OBJECT               = FX_TYPES_BASE+0x0,
    FX_TYPE_DRIVER               = FX_TYPES_BASE+0x1,
    FX_TYPE_DEVICE               = FX_TYPES_BASE+0x2,
    FX_TYPE_QUEUE                = FX_TYPES_BASE+0x3,
    FX_TYPE_WMI_PROVIDER         = FX_TYPES_BASE+0x4,
    // can be reused             = FX_TYPES_BASE+0x5,
    FX_TYPE_REG_KEY              = FX_TYPES_BASE+0x6,
    FX_TYPE_STRING               = FX_TYPES_BASE+0x7,
    FX_TYPE_REQUEST              = FX_TYPES_BASE+0x8,
    FX_TYPE_LOOKASIDE            = FX_TYPES_BASE+0x9,
    IFX_TYPE_MEMORY              = FX_TYPES_BASE+0xA,
    FX_TYPE_IRPQUEUE             = FX_TYPES_BASE+0xB,
    FX_TYPE_USEROBJECT           = FX_TYPES_BASE+0xC,
    // can be reused               FX_TYPES_BASE+0xD,
    FX_TYPE_COLLECTION           = FX_TYPES_BASE+0xE,




    // can be reused             = FX_TYPES_BASE+0x11,
    FX_TYPE_VERIFIERLOCK         = FX_TYPES_BASE+0x12,
    FX_TYPE_SYSTEMTHREAD         = FX_TYPES_BASE+0x13,
    FX_TYPE_MP_DEVICE            = FX_TYPES_BASE+0x14,
    FX_TYPE_DPC                  = FX_TYPES_BASE+0x15,
    FX_TYPE_RESOURCE_IO          = FX_TYPES_BASE+0x16,
    FX_TYPE_RESOURCE_CM          = FX_TYPES_BASE+0x17,
    FX_TYPE_FILEOBJECT           = FX_TYPES_BASE+0x18,
    // can be reused             = FX_TYPES_BASE+0x19,
    // can be reused             = FX_TYPES_BASE+0x20,
    FX_TYPE_RELATED_DEVICE       = FX_TYPES_BASE+0x21,
    FX_TYPE_MEMORY_PREALLOCATED  = FX_TYPES_BASE+0x22,
    FX_TYPE_WAIT_LOCK            = FX_TYPES_BASE+0x23,
    FX_TYPE_SPIN_LOCK            = FX_TYPES_BASE+0x24,
    FX_TYPE_WORKITEM             = FX_TYPES_BASE+0x25,
    FX_TYPE_CLEANUPLIST          = FX_TYPES_BASE+0x26,
    FX_TYPE_INTERRUPT            = FX_TYPES_BASE+0x27,
    FX_TYPE_TIMER                = FX_TYPES_BASE+0x28,
    FX_TYPE_CHILD_LIST           = FX_TYPES_BASE+0x29,
    FX_TYPE_DEVICE_BASE          = FX_TYPES_BASE+0x30,
    FX_TYPE_SYSTEMWORKITEM       = FX_TYPES_BASE+0x31,
    FX_TYPE_REQUEST_MEMORY       = FX_TYPES_BASE+0x32,
    FX_TYPE_DISPOSELIST          = FX_TYPES_BASE+0x33,
    FX_TYPE_WMI_INSTANCE         = FX_TYPES_BASE+0x34,
    FX_TYPE_IO_RES_LIST          = FX_TYPES_BASE+0x35,
    FX_TYPE_CM_RES_LIST          = FX_TYPES_BASE+0x36,
    FX_TYPE_IO_RES_REQ_LIST      = FX_TYPES_BASE+0x37,

    FX_TYPE_PACKAGE_IO           = FX_TYPES_PACKAGES_BASE+0x0,
    FX_TYPE_PACKAGE_FDO          = FX_TYPES_PACKAGES_BASE+0x1,
    FX_TYPE_PACKAGE_PDO          = FX_TYPES_PACKAGES_BASE+0x2,
    FX_TYPE_WMI_IRP_HANDLER      = FX_TYPES_PACKAGES_BASE+0x3,
    FX_TYPE_PACKAGE_GENERAL      = FX_TYPES_PACKAGES_BASE+0x4,
    FX_TYPE_DEFAULT_IRP_HANDLER  = FX_TYPES_PACKAGES_BASE+0x5,
    FX_TYPE_WMI_TRACING_IRP_HANDLER = FX_TYPES_PACKAGES_BASE+0x6,

    FX_TYPE_IO_TARGET            = FX_TYPES_IO_TARGET_BASE+0x0,
    FX_TYPE_IO_TARGET_REMOTE     = FX_TYPES_IO_TARGET_BASE+0x1,
    FX_TYPE_IO_TARGET_USB_DEVICE = FX_TYPES_IO_TARGET_BASE+0x2,
    FX_TYPE_IO_TARGET_USB_PIPE   = FX_TYPES_IO_TARGET_BASE+0x3,
    FX_TYPE_USB_INTERFACE        = FX_TYPES_IO_TARGET_BASE+0x4,
    FX_TYPE_IO_TARGET_SELF       = FX_TYPES_IO_TARGET_BASE+0x5,

    FX_TYPE_DMA_ENABLER          = FX_TYPES_DMA_BASE+0x0,
    FX_TYPE_DMA_TRANSACTION      = FX_TYPES_DMA_BASE+0x1,
    FX_TYPE_COMMON_BUFFER        = FX_TYPES_DMA_BASE+0x2,

    // Interfaces
    FX_TYPE_IASSOCIATE           = FX_TYPES_INTERFACES_BASE+0x01,
    // unused
    FX_TYPE_IHASCALLBACKS        = FX_TYPES_INTERFACES_BASE+0x03,
    // unused                    = FX_TYPES_INTERFACES_BASE+0x04,

    FX_TYPE_NONE                 = 0xFFFF,
};

// begin_wpp config
// CUSTOM_TYPE(FX_OBJECT_TYPES, ItemEnum(FX_OBJECT_TYPES));
// end_wpp

#endif // _FXTYPES_H

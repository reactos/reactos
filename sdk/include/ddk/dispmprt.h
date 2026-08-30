/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     Public Domain
 * PURPOSE:     Header file for WDDM style driver exports
 * COPYRIGHT:   Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#ifndef _DISPMPRT_H_
#define _DISPMPRT_H_

#ifndef _ACPIIOCT_H_
#include <acpiioct.h>
#endif

#define _NTOSDEF_

#ifndef _NTOSP_
#define _NTOSP_

typedef enum _EMULATOR_PORT_ACCESS_TYPE {
    Uchar,
    Ushort,
    Ulong
} EMULATOR_PORT_ACCESS_TYPE, *PEMULATOR_PORT_ACCESS_TYPE;

typedef struct _EMULATOR_ACCESS_ENTRY {
    ULONG BasePort;
    ULONG NumConsecutivePorts;
    EMULATOR_PORT_ACCESS_TYPE AccessType;
    UCHAR AccessMode;
    UCHAR StringSupport;
    PVOID Routine;
} EMULATOR_ACCESS_ENTRY, *PEMULATOR_ACCESS_ENTRY;

#endif // _NTOSP_

typedef
VOID
(NTAPI *PBANKED_SECTION_ROUTINE)(
    _In_ ULONG ReadBank,
    _In_ ULONG WriteBank,
    _In_ PVOID Context
);

#include <ntddvdeo.h>
#include <video.h>
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
#include <windef.h>

#ifndef _D3DKMDT_H
#include <d3dkmdt.h>
#endif
#ifndef _D3DKMDDI_H_
#include <d3dkmddi.h>
#endif

#define DXGK_EXCLUDE_EVICT_ALL            0x00000001
#define DXGK_EXCLUDE_CALL_SYNCHRONOUS     0x00000002
#define DXGK_EXCLUDE_BRIDGE_ACCESS        0x00000004
#define DXGK_EXCLUDE_EVICT_STANDBY        0x00000008
#define DXGK_EXCLUDE_EVICT_HIBERNATE      0x00000010
#define DXGK_EXCLUDE_EVICT_SHUTDOWN       0x00000020
#define DXGK_EXCLUDE_D3_STATE_TRANSITION  0x00000040

#define DXGK_MAX_STRING_LEN 50
#define DXGK_MAX_REG_SZ_LEN DXGK_MAX_STRING_LEN + 1

#define DXGK_WHICHSPACE_CONFIG       PCI_WHICHSPACE_CONFIG
#define DXGK_WHICHSPACE_ROM          PCI_WHICHSPACE_ROM
#define DXGK_WHICHSPACE_MCH          0x80000000
#define DXGK_WHICHSPACE_BRIDGE       0x80000001

#ifdef ENABLE_DXGK_SAL
#define _Function_class_DXGK_(param)    _Function_class_(param)
#define _IRQL_requires_DXGK_(param)     _IRQL_requires_(param)
#define _Field_size_bytes_DXGK_(param)  _Field_size_bytes_(param)
#else
#define _Function_class_DXGK_(param)
#define _IRQL_requires_DXGK_(param)
#define _Field_size_bytes_DXGK_(param)
#endif

typedef struct _LINKED_DEVICE {
    ULONG ChainUid;
    ULONG NumberOfLinksInChain;
    BOOLEAN LeadLink;
} LINKED_DEVICE, *PLINKED_DEVICE;

typedef enum _DXGK_EVENT_TYPE {
    DxgkUndefinedEvent,
    DxgkAcpiEvent,
    DxgkPowerStateEvent,
    DxgkDockingEvent,
    DxgkChainedAcpiEvent
} DXGK_EVENT_TYPE, *PDXGK_EVENT_TYPE;

typedef struct _DXGK_VIDEO_OUTPUT_CAPABILITIES {
    D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY InterfaceTechnology;
    D3DKMDT_MONITOR_ORIENTATION_AWARENESS MonitorOrientationAwareness;
    BOOLEAN SupportsSdtvModes;
} DXGK_VIDEO_OUTPUT_CAPABILITIES, *PDXGK_VIDEO_OUTPUT_CAPABILITIES;

typedef struct _DXGK_INTEGRATED_DISPLAY_CHILD {
    D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY InterfaceTechnology;
    USHORT DescriptorLength;
} DXGK_INTEGRATED_DISPLAY_CHILD, *PDXGK_INTEGRATED_DISPLAY_CHILD;

typedef enum _DXGK_CHILD_DEVICE_TYPE {
   TypeUninitialized,
   TypeVideoOutput,
   TypeOther,
   TypeIntegratedDisplay
} DXGK_CHILD_DEVICE_TYPE, *PDXGK_CHILD_DEVICE_TYPE;

typedef struct _DXGK_CHILD_CAPABILITIES {
    union
    {
        DXGK_VIDEO_OUTPUT_CAPABILITIES VideoOutput;
        struct
        {
            UINT MustBeZero;
        }
        Other;
        DXGK_INTEGRATED_DISPLAY_CHILD IntegratedDisplayChild;
    } Type;
    DXGK_CHILD_DEVICE_HPD_AWARENESS HpdAwareness;
} DXGK_CHILD_CAPABILITIES, *PDXGK_CHILD_CAPABILITIES;

typedef struct _DXGK_CHILD_DESCRIPTOR {
   DXGK_CHILD_DEVICE_TYPE ChildDeviceType;
   DXGK_CHILD_CAPABILITIES ChildCapabilities;
   ULONG AcpiUid;
   ULONG ChildUid;
} DXGK_CHILD_DESCRIPTOR, *PDXGK_CHILD_DESCRIPTOR;

typedef struct _DXGK_DEVICE_DESCRIPTOR {
   ULONG DescriptorOffset;
   ULONG DescriptorLength;
   _Field_size_bytes_DXGK_(DescriptorLength) PVOID DescriptorBuffer;
} DXGK_DEVICE_DESCRIPTOR, *PDXGK_DEVICE_DESCRIPTOR;

typedef struct _DXGK_GENERIC_DESCRIPTOR {
    WCHAR HardwareId[DXGK_MAX_REG_SZ_LEN];
    WCHAR InstanceId[DXGK_MAX_REG_SZ_LEN];
    WCHAR CompatibleId[DXGK_MAX_REG_SZ_LEN];
    WCHAR DeviceText[DXGK_MAX_REG_SZ_LEN];
} DXGK_GENERIC_DESCRIPTOR, *PDXGK_GENERIC_DESCRIPTOR;

typedef enum _DXGK_CHILD_STATUS_TYPE{
   StatusUninitialized,
   StatusConnection,
   StatusRotation,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
   StatusMiracastConnection,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

} DXGK_CHILD_STATUS_TYPE, *PDXGK_CHILD_STATUS_TYPE;

typedef struct _DXGK_CHILD_STATUS {
   DXGK_CHILD_STATUS_TYPE Type;
   ULONG ChildUid;
   union {
      struct {
         BOOLEAN Connected;
      } HotPlug;
      struct {
         UCHAR Angle;
      } Rotation;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
      struct {
         BOOLEAN Connected;
         D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY MiracastMonitorType;
      } Miracast;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
   };
} DXGK_CHILD_STATUS, *PDXGK_CHILD_STATUS;

typedef enum {
    DockStateUnsupported = 0,
    DockStateUnDocked = 1,
    DockStateDocked = 2,
    DockStateUnknown = 3,
} DOCKING_STATE;

typedef struct _DXGK_DEVICE_INFO {
    PVOID MiniportDeviceContext;
    PDEVICE_OBJECT PhysicalDeviceObject;
    UNICODE_STRING DeviceRegistryPath;
    PCM_RESOURCE_LIST TranslatedResourceList;
    LARGE_INTEGER SystemMemorySize;
    PHYSICAL_ADDRESS HighestPhysicalAddress;
    PHYSICAL_ADDRESS AgpApertureBase;
    SIZE_T AgpApertureSize;
    DOCKING_STATE DockingState;
} DXGK_DEVICE_INFO, *PDXGK_DEVICE_INFO;

typedef
_Function_class_DXGK_(DXGKDDI_PROTECTED_CALLBACK)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
VOID
(NTAPI *DXGKDDI_PROTECTED_CALLBACK)(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ PVOID ProtectedCallbackContext,
    _In_ NTSTATUS ProtectionStatus
);

DEFINE_GUID(GUID_DEVINTERFACE_I2C, 0x2564AA4F, 0xDDDB, 0x4495, 0xB4, 0x97, 0x6A, 0xD4, 0xA8, 0x41, 0x63, 0xD7);
DEFINE_GUID(GUID_DEVINTERFACE_OPM, 0xBF4672DE, 0x6B4E, 0x4BE4, 0xA3, 0x25, 0x68, 0xA9, 0x1E, 0xA4, 0x9C, 0x09);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
DEFINE_GUID(GUID_DEVINTERFACE_OPM_2_JTP, 0xE929EEA4, 0xB9F1, 0x407B, 0xAA, 0xB9, 0xAB, 0x08, 0xBB, 0x44, 0xFB, 0xF4);
DEFINE_GUID(GUID_DEVINTERFACE_OPM_2, 0x7F098726, 0x2EBB, 0x4FF3, 0xA2, 0x7F, 0x10, 0x46, 0xB9, 0x5D, 0xC5, 0x17);
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
DEFINE_GUID(GUID_DEVINTERFACE_OPM_3, 0x693a2cb1, 0x8c8d, 0x4ab6, 0x95, 0x55, 0x4b, 0x85, 0xef, 0x2c, 0x7c, 0x6b);
#endif
DEFINE_GUID(GUID_DEVINTERFACE_BRIGHTNESS, 0xFDE5BBA4, 0xB3F9, 0x46FB, 0xBD, 0xAA, 0x07, 0x28, 0xCE, 0x31, 0x00, 0xB4);
DEFINE_GUID(GUID_DEVINTERFACE_BRIGHTNESS_2, 0x148A3C98, 0x0ECD, 0x465A, 0xB6, 0x34, 0xB0, 0x5F, 0x19, 0x5F, 0x77, 0x39);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
DEFINE_GUID(GUID_DEVINTERFACE_MIRACAST_DISPLAY, 0xaf03f190, 0x22af, 0x48cb, 0x94, 0xbb, 0xb7, 0x8e, 0x76, 0xa2, 0x51, 0x7);
#endif

#define DXGK_I2C_INTERFACE_VERSION_1 0x01

typedef
_Function_class_DXGK_(DXGKDDI_I2C_TRANSMIT_DATA_TO_DISPLAY)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_I2C_TRANSMIT_DATA_TO_DISPLAY)(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _In_ ULONG SevenBitI2CAddress,
    _In_ ULONG DataLength,
    _In_reads_bytes_(DataLength) CONST PVOID Data
);

typedef
_Function_class_DXGK_(DXGKDDI_I2C_RECEIVE_DATA_FROM_DISPLAY)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_I2C_RECEIVE_DATA_FROM_DISPLAY)(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _In_ ULONG SevenBitI2CAddress,
    _In_ ULONG Flags,
    _In_ ULONG DataLength,
    _Out_writes_bytes_(DataLength) PVOID Data
);

typedef struct _DXGK_I2C_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGKDDI_I2C_TRANSMIT_DATA_TO_DISPLAY DxgkDdiI2CTransmitDataToDisplay;
    DXGKDDI_I2C_RECEIVE_DATA_FROM_DISPLAY DxgkDdiI2CReceiveDataFromDisplay;
} DXGK_I2C_INTERFACE, *PDXGK_I2C_INTERFACE;

#define DXGK_OPM_INTERFACE_VERSION_1 0x01

typedef
_Function_class_DXGK_(DXGKDDI_OPM_GET_CERTIFICATE_SIZE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_GET_CERTIFICATE_SIZE)(
    _In_ PVOID MiniportDeviceContext,
    _In_ DXGKMDT_CERTIFICATE_TYPE CertificateType,
    _Out_ PULONG CertificateSize
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_GET_CERTIFICATE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_GET_CERTIFICATE)(
    _In_ PVOID MiniportDeviceContext,
    _In_ DXGKMDT_CERTIFICATE_TYPE CertificateType,
    _In_ ULONG CertificateSize,
    _Out_writes_bytes_(CertificateSize) PVOID CertificateBuffer
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT)(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _In_ DXGKMDT_OPM_VIDEO_OUTPUT_SEMANTICS NewVideoOutputSemantics,
    _Out_ PHANDLE NewProtectedOutputHandle
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_GET_RANDOM_NUMBER)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_GET_RANDOM_NUMBER)(
    _In_ PVOID MiniportDeviceContext,
    _In_ HANDLE ProtectedOutputHandle,
    _Out_ PDXGKMDT_OPM_RANDOM_NUMBER RandomNumber
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_SET_SIGNING_KEY_AND_SEQUENCE_NUMBERS)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_SET_SIGNING_KEY_AND_SEQUENCE_NUMBERS)(
    _In_ PVOID MiniportDeviceContext,
    _In_ HANDLE ProtectedOutputHandle,
    _In_ CONST PDXGKMDT_OPM_ENCRYPTED_PARAMETERS EncryptedParameters
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_GET_INFORMATION)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_GET_INFORMATION)(
    _In_ PVOID MiniportDeviceContext,
    _In_ HANDLE ProtectedOutputHandle,
    _In_ CONST PDXGKMDT_OPM_GET_INFO_PARAMETERS Parameters,
    _Out_ PDXGKMDT_OPM_REQUESTED_INFORMATION RequestedInformation
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_GET_COPP_COMPATIBLE_INFORMATION)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_GET_COPP_COMPATIBLE_INFORMATION)(
    _In_ PVOID MiniportDeviceContext,
    _In_ HANDLE ProtectedOutputHandle,
    _In_ CONST PDXGKMDT_OPM_COPP_COMPATIBLE_GET_INFO_PARAMETERS Parameters,
    _Out_ PDXGKMDT_OPM_REQUESTED_INFORMATION RequestedInformation
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_CONFIGURE_PROTECTED_OUTPUT)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_CONFIGURE_PROTECTED_OUTPUT)(
    _In_ PVOID MiniportDeviceContext,
    _In_ HANDLE ProtectedOutputHandle,
    _In_ CONST PDXGKMDT_OPM_CONFIGURE_PARAMETERS Parameters,
    _In_ ULONG AdditionalParametersSize,
    _In_reads_bytes_(AdditionalParametersSize) CONST PVOID AdditionalParameters
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_DESTROY_PROTECTED_OUTPUT)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_DESTROY_PROTECTED_OUTPUT)(
    _In_ PVOID MiniportDeviceContext,
    _In_ HANDLE ProtectedOutputHandle
);

typedef struct _DXGK_OPM_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGKDDI_OPM_GET_CERTIFICATE_SIZE DxgkDdiOPMGetCertificateSize;
    DXGKDDI_OPM_GET_CERTIFICATE DxgkDdiOPMGetCertificate;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT DxgkDdiOPMCreateProtectedOutput;
    DXGKDDI_OPM_GET_RANDOM_NUMBER DxgkDdiOPMGetRandomNumber;
    DXGKDDI_OPM_SET_SIGNING_KEY_AND_SEQUENCE_NUMBERS DxgkDdiOPMSetSigningKeyAndSequenceNumbers;
    DXGKDDI_OPM_GET_INFORMATION DxgkDdiOPMGetInformation;
    DXGKDDI_OPM_GET_COPP_COMPATIBLE_INFORMATION DxgkDdiOPMGetCOPPCompatibleInformation;
    DXGKDDI_OPM_CONFIGURE_PROTECTED_OUTPUT DxgkDdiOPMConfigureProtectedOutput;
    DXGKDDI_OPM_DESTROY_PROTECTED_OUTPUT DxgkDdiOPMDestroyProtectedOutput;
} DXGK_OPM_INTERFACE, *PDXGK_OPM_INTERFACE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
#define DXGK_OPM_INTERFACE_VERSION_2_JTP 0x02

typedef
_Function_class_DXGK_(DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_NONLOCAL_DISPLAY_JTP)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_NONLOCAL_DISPLAY_JTP)(
    _In_ PVOID MiniportDeviceContext,
    _In_ DXGKMDT_OPM_VIDEO_OUTPUT_SEMANTICS NewVideoOutputSemantics,
    _In_ ULONG64 OPMEncoderContext,
    _In_ DXGKMDT_OPM_ACTUAL_OUTPUT_FORMAT *pActualOutputFormat,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID NonLocalOutputId,
    _Out_ PHANDLE NewProtectedOutputHandle
);

typedef
_Function_class_DXGK_(DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_VIRTUAL_MODE_JTP)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_VIRTUAL_MODE_JTP)(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _In_ DXGKMDT_OPM_VIDEO_OUTPUT_SEMANTICS NewVideoOutputSemantics,
    _In_ DXGKMDT_OPM_ACTUAL_OUTPUT_FORMAT *pActualOutputFormat,
    _Out_ PHANDLE NewProtectedOutputHandle
);

typedef struct _DXGK_OPM_INTERFACE_2_JTP {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGKDDI_OPM_GET_CERTIFICATE_SIZE DxgkDdiOPMGetCertificateSize;
    DXGKDDI_OPM_GET_CERTIFICATE DxgkDdiOPMGetCertificate;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT DxgkDdiOPMCreateProtectedOutput;
    DXGKDDI_OPM_GET_RANDOM_NUMBER DxgkDdiOPMGetRandomNumber;
    DXGKDDI_OPM_SET_SIGNING_KEY_AND_SEQUENCE_NUMBERS DxgkDdiOPMSetSigningKeyAndSequenceNumbers;
    DXGKDDI_OPM_GET_INFORMATION DxgkDdiOPMGetInformation;
    DXGKDDI_OPM_GET_COPP_COMPATIBLE_INFORMATION DxgkDdiOPMGetCOPPCompatibleInformation;
    DXGKDDI_OPM_CONFIGURE_PROTECTED_OUTPUT DxgkDdiOPMConfigureProtectedOutput;
    DXGKDDI_OPM_DESTROY_PROTECTED_OUTPUT DxgkDdiOPMDestroyProtectedOutput;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_VIRTUAL_MODE_JTP DxgkDdiOPMCreateProtectedOutputVirtualMode;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_NONLOCAL_DISPLAY_JTP DxgkDdiOPMCreateProtectedOutputNonLocalDisplay;
} DXGK_OPM_INTERFACE_2_JTP, *PDXGK_OPM_INTERFACE_2_JTP;

#define DXGK_OPM_INTERFACE_VERSION_2 0x03

typedef
_Function_class_DXGK_(DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_NONLOCAL_DISPLAY)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_NONLOCAL_DISPLAY)(
    _In_ PVOID MiniportDeviceContext,
    _In_ DXGKMDT_OPM_VIDEO_OUTPUT_SEMANTICS NewVideoOutputSemantics,
    _In_ UINT64 OPMEncoderContext,
    _In_ DXGKMDT_OPM_ACTUAL_OUTPUT_FORMAT *pActualOutputFormat,
    _In_ UINT64 NonLocalOutputId,
    _In_ DXGKMDT_OPM_CONNECTOR_TYPE NonLocalConnectorType,
    _Out_ PHANDLE NewProtectedOutputHandle
);

typedef struct _DXGK_OPM_INTERFACE_2 {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGKDDI_OPM_GET_CERTIFICATE_SIZE DxgkDdiOPMGetCertificateSize;
    DXGKDDI_OPM_GET_CERTIFICATE DxgkDdiOPMGetCertificate;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT DxgkDdiOPMCreateProtectedOutput;
    DXGKDDI_OPM_GET_RANDOM_NUMBER DxgkDdiOPMGetRandomNumber;
    DXGKDDI_OPM_SET_SIGNING_KEY_AND_SEQUENCE_NUMBERS DxgkDdiOPMSetSigningKeyAndSequenceNumbers;
    DXGKDDI_OPM_GET_INFORMATION DxgkDdiOPMGetInformation;
    DXGKDDI_OPM_GET_COPP_COMPATIBLE_INFORMATION DxgkDdiOPMGetCOPPCompatibleInformation;
    DXGKDDI_OPM_CONFIGURE_PROTECTED_OUTPUT DxgkDdiOPMConfigureProtectedOutput;
    DXGKDDI_OPM_DESTROY_PROTECTED_OUTPUT DxgkDdiOPMDestroyProtectedOutput;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_NONLOCAL_DISPLAY DxgkDdiOPMCreateProtectedOutputNonLocalDisplay;
} DXGK_OPM_INTERFACE_2, *PDXGK_OPM_INTERFACE_2;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)

#define DXGK_OPM_INTERFACE_VERSION_3 0x04

typedef struct _DXGK_OPM_INTERFACE_3 {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGKDDI_OPM_GET_CERTIFICATE_SIZE DxgkDdiOPMGetCertificateSize;
    DXGKDDI_OPM_GET_CERTIFICATE DxgkDdiOPMGetCertificate;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT DxgkDdiOPMCreateProtectedOutput;
    DXGKDDI_OPM_GET_RANDOM_NUMBER DxgkDdiOPMGetRandomNumber;
    DXGKDDI_OPM_SET_SIGNING_KEY_AND_SEQUENCE_NUMBERS DxgkDdiOPMSetSigningKeyAndSequenceNumbers;
    DXGKDDI_OPM_GET_INFORMATION DxgkDdiOPMGetInformation;
    DXGKDDI_OPM_GET_COPP_COMPATIBLE_INFORMATION DxgkDdiOPMGetCOPPCompatibleInformation;
    DXGKDDI_OPM_CONFIGURE_PROTECTED_OUTPUT DxgkDdiOPMConfigureProtectedOutput;
    DXGKDDI_OPM_DESTROY_PROTECTED_OUTPUT DxgkDdiOPMDestroyProtectedOutput;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_NONLOCAL_DISPLAY DxgkDdiOPMCreateProtectedOutputNonLocalDisplay;
    DXGKDDI_OPM_CREATE_PROTECTED_OUTPUT_VIRTUAL_MODE_JTP DxgkDdiOPMCreateProtectedOutputVirtualMode;
} DXGK_OPM_INTERFACE_3, *PDXGK_OPM_INTERFACE_3;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)

#define DXGK_BRIGHTNESS_INTERFACE_VERSION_1 0x01

typedef
_Function_class_DXGK_(DXGK_BRIGHTNESS_GET_POSSIBLE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGK_BRIGHTNESS_GET_POSSIBLE)(
    _In_ PVOID Context,
    _In_ ULONG BufferSize,
    _Out_ PUCHAR LevelCount,
    _Out_writes_bytes_to_(BufferSize, *LevelCount) PUCHAR BrightnessLevels
);

typedef
_Function_class_DXGK_(DXGK_BRIGHTNESS_SET)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGK_BRIGHTNESS_SET)(
    _In_ PVOID Context,
    _In_ UCHAR Brightness
);

typedef
_Function_class_DXGK_(DXGK_BRIGHTNESS_GET)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGK_BRIGHTNESS_GET)(
    _In_ PVOID Context,
    _Out_ PUCHAR Brightness
);

typedef struct {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGK_BRIGHTNESS_GET_POSSIBLE GetPossibleBrightness;
    DXGK_BRIGHTNESS_SET SetBrightness;
    DXGK_BRIGHTNESS_GET GetBrightness;
} DXGK_BRIGHTNESS_INTERFACE, *PDXGK_BRIGHTNESS_INTERFACE;

#define DXGK_BRIGHTNESS_INTERFACE_VERSION_2 0x02

typedef
_Function_class_DXGK_(DXGK_BRIGHTNESS_GET_CAPS)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGK_BRIGHTNESS_GET_CAPS)(
    _In_ PVOID Context,
    _Out_ DXGK_BRIGHTNESS_CAPS *BrightnessCaps
);

typedef
_Function_class_DXGK_(DXGK_BRIGHTNESS_SET_STATE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGK_BRIGHTNESS_SET_STATE)(
    _In_ PVOID Context,
    _In_ DXGK_BRIGHTNESS_STATE *BrightnessState
);

typedef
_Function_class_DXGK_(DXGK_BRIGHTNESS_SET_BACKLIGHT_OPTIMIZATION)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGK_BRIGHTNESS_SET_BACKLIGHT_OPTIMIZATION)(
    _In_ PVOID Context,
    _In_ DXGK_BACKLIGHT_OPTIMIZATION_LEVEL OptimizationLevel
);

typedef
_Function_class_DXGK_(DXGK_BRIGHTNESS_GET_BACKLIGHT_REDUCTION)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGK_BRIGHTNESS_GET_BACKLIGHT_REDUCTION)(
    _In_ PVOID Context,
    _Out_ DXGK_BACKLIGHT_INFO *BacklightInfo
);

typedef struct {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGK_BRIGHTNESS_GET_POSSIBLE GetPossibleBrightness;
    DXGK_BRIGHTNESS_SET SetBrightness;
    DXGK_BRIGHTNESS_GET GetBrightness;
    DXGK_BRIGHTNESS_GET_CAPS GetBrightnessCaps;
    DXGK_BRIGHTNESS_SET_STATE SetBrightnessState;
    DXGK_BRIGHTNESS_SET_BACKLIGHT_OPTIMIZATION SetBacklightOptimization;
    DXGK_BRIGHTNESS_GET_BACKLIGHT_REDUCTION GetBacklightReduction;
} DXGK_BRIGHTNESS_INTERFACE_2, *PDXGK_BRIGHTNESS_INTERFACE_2;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
#define DXGK_MIRACAST_DISPLAY_INTERFACE_VERSION_1 0x01

typedef struct _DXGK_MIRACAST_CAPS {
    ULONG MaxChunkPrivateDriverDataSize;
    union {
        struct {
            UINT HdcpSupport : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    } Flags;
} DXGK_MIRACAST_CAPS, *PDXGK_MIRACAST_CAPS;

typedef
_Function_class_DXGK_(DXGKDDI_MIRACAST_QUERY_CAPS)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_MIRACAST_QUERY_CAPS)(
    _In_ PVOID DriverContext,
    _In_ ULONG MiracastCapsSize,
    _Out_ DXGK_MIRACAST_CAPS *MiracastCaps
);

typedef
_Function_class_DXGK_(DXGKCB_MIRACAST_SEND_MESSAGE_CALLBACK)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
VOID
(NTAPI *DXGKCB_MIRACAST_SEND_MESSAGE_CALLBACK)(
    _In_ PVOID CallbackContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock
);

typedef
_Function_class_DXGK_(DXGKCB_MIRACAST_SEND_MESSAGE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKCB_MIRACAST_SEND_MESSAGE)(
    _In_ HANDLE MiracastHandle,
    _In_ ULONG InputBufferSize,
    _In_reads_bytes_(InputBufferSize) VOID *InputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) VOID *OutputBuffer,
    _In_opt_ DXGKCB_MIRACAST_SEND_MESSAGE_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext
);

typedef
_Function_class_DXGK_(DXGKCB_MIRACAST_REPORT_CHUNK_INFO)
_IRQL_requires_DXGK_(DISPATCH_LEVEL)
NTSTATUS
(NTAPI *DXGKCB_MIRACAST_REPORT_CHUNK_INFO)(
    _In_ HANDLE MiracastHandle,
    _In_ DXGK_MIRACAST_CHUNK_INFO *ChunkInfo,
    _In_ PVOID PrivateDriverData,
    _In_ UINT PrivateDataDriverSize
);

typedef struct _DXGK_MIRACAST_DISPLAY_CALLBACKS {
    HANDLE MiracastHandle;
    DXGKCB_MIRACAST_SEND_MESSAGE DxgkCbMiracastSendMessage;
    DXGKCB_MIRACAST_REPORT_CHUNK_INFO DxgkCbReportChunkInfo;
} DXGK_MIRACAST_DISPLAY_CALLBACKS, *PDXGK_MIRACAST_DISPLAY_CALLBACKS;

typedef
_Function_class_DXGK_(DXGKDDI_MIRACAST_CREATE_CONTEXT)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_MIRACAST_CREATE_CONTEXT)(
    _In_ PVOID DriverContext,
    _In_ DXGK_MIRACAST_DISPLAY_CALLBACKS *MiracastCallbacks,
    _Out_ PVOID *MiracastContext,
    _Out_ ULONG *TargetId
);

typedef
_Function_class_DXGK_(DXGKDDI_MIRACAST_DESTROY_CONTEXT)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
VOID
(NTAPI *DXGKDDI_MIRACAST_DESTROY_CONTEXT)(
    _In_ PVOID DriverContext,
    _In_ PVOID MiracastContext
);

typedef
_Function_class_DXGK_(DXGKDDI_MIRACAST_HANDLE_IO_CONTROL)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *DXGKDDI_MIRACAST_HANDLE_IO_CONTROL)(
    _In_ PVOID DriverContext,
    _In_ PVOID MiracastContext,
    _In_ ULONG InputBufferSize,
    _In_reads_bytes_(InputBufferSize) VOID *InputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) VOID *OutputBuffer,
    _Out_ ULONG *BytesReturned
);

typedef struct _DXGK_MIRACAST_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGKDDI_MIRACAST_QUERY_CAPS DxgkDdiMiracastQueryCaps;
    DXGKDDI_MIRACAST_CREATE_CONTEXT DxgkDdiMiracastCreateContext;
    DXGKDDI_MIRACAST_HANDLE_IO_CONTROL DxgkDdiMiracastIoControl;
    DXGKDDI_MIRACAST_DESTROY_CONTEXT DxgkDdiMiracastDestroyContext;
} DXGK_MIRACAST_DISPLAY_INTERFACE, *PDXGK_MIRACAST_DISPLAY_INTERFACE;
#endif /* DXGKDDI_INTERFACE_VERSION_WDDM1_3 */

typedef enum {
    DxgkServicesAgp,
    DxgkServicesDebugReport,
    DxgkServicesTimedOperation,
    DxgkServicesSPB,
    DxgkServicesBDD,
    DxgkServicesFirmwareTable,
    DxgkServicesIDD,
} DXGK_SERVICES;

#define DXGK_AGP_INTERFACE_VERSION_1 0x01
#define DXGK_AGPCOMMAND_AGP1X       0x00001
#define DXGK_AGPCOMMAND_AGP2X       0x00002
#define DXGK_AGPCOMMAND_AGP4X       0x00004
#define DXGK_AGPCOMMAND_AGP8X       0x00008
#define DXGK_AGPCOMMAND_DISABLE_SBA 0x10000
#define DXGK_AGPCOMMAND_DISABLE_FW  0x20000

typedef
_Function_class_DXGK_(DXGKCB_AGP_ALLOCATE_POOL)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(APIENTRY *DXGKCB_AGP_ALLOCATE_POOL)(
    _In_ HANDLE Context,
    _In_ ULONG AllocationSize,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _Out_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_ PVOID *VirtualAddress
);

typedef
_Function_class_DXGK_(DXGKCB_AGP_FREE_POOL)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(APIENTRY *DXGKCB_AGP_FREE_POOL)(
    _In_ HANDLE Context,
    _In_ PVOID VirtualAddress
);

typedef
_Function_class_DXGK_(DXGKCB_AGP_SET_COMMAND)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
(APIENTRY *DXGKCB_AGP_SET_COMMAND)(
    _In_ HANDLE Context,
    _In_ ULONG Command
);

typedef struct _DXGK_AGP_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    DXGKCB_AGP_ALLOCATE_POOL AgpAllocatePool;
    DXGKCB_AGP_FREE_POOL AgpFreePool;
    DXGKCB_AGP_SET_COMMAND AgpSetCommand;
} DXGK_AGP_INTERFACE, *PDXGK_AGP_INTERFACE;

DECLARE_HANDLE(DXGK_DEBUG_REPORT_HANDLE);
#define DXGK_DEBUG_REPORT_INTERFACE_VERSION_1 0x01
#define DXGK_DEBUG_REPORT_MAX_SIZE 0xF800

typedef struct _DXGK_DEBUG_REPORT_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    DXGK_DEBUG_REPORT_HANDLE (NTAPI *DbgReportCreate)(
        _In_ HANDLE DeviceHandle,
        _In_ ULONG Code,
        _In_ ULONG_PTR Arg1,
        _In_ ULONG_PTR Arg2,
        _In_ ULONG_PTR Arg3,
        _In_ ULONG_PTR Arg4
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    _Success_(return != 0)
    BOOLEAN (NTAPI *DbgReportSecondaryData)(
        _Inout_ DXGK_DEBUG_REPORT_HANDLE Report,
        _In_reads_bytes_(DataSize) PVOID Data,
        _In_ ULONG DataSize
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    VOID (NTAPI *DbgReportComplete)(
        _Inout_ DXGK_DEBUG_REPORT_HANDLE Report
    );
} DXGK_DEBUG_REPORT_INTERFACE, *PDXGK_DEBUG_REPORT_INTERFACE;

#define DXGK_TIMED_OPERATION_INTERFACE_VERSION_1 0x01
#define DXGK_TIMED_OPERATION_TIMEOUT_MAX_SECONDS 5

typedef struct _DXGK_TIMED_OPERATION {
    USHORT Size;
    ULONG_PTR OwnerTag;
    BOOLEAN OsHandled;
    BOOLEAN TimeoutTriggered;
    LARGE_INTEGER Timeout;
    LARGE_INTEGER StartTick;
} DXGK_TIMED_OPERATION, *PDXGK_TIMED_OPERATION;

typedef struct _DXGK_TIMED_OPERATION_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *TimedOperationStart)(
        _Out_ DXGK_TIMED_OPERATION *Op,
        _In_ const LARGE_INTEGER *Timeout,
        _In_ BOOLEAN OsHandled
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *TimedOperationDelay)(
        _Inout_ DXGK_TIMED_OPERATION *Op,
        _In_ KPROCESSOR_MODE WaitMode,
        _In_ BOOLEAN Alertable,
        _In_opt_ const LARGE_INTEGER *Interval
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *TimedOperationWaitForSingleObject)(
        _Inout_ DXGK_TIMED_OPERATION *Op,
        _In_ PVOID Object,
        _In_ KWAIT_REASON WaitReason,
        _In_ KPROCESSOR_MODE WaitMode,
        _In_ BOOLEAN Alertable,
        _In_opt_ const LARGE_INTEGER *Timeout
    );
} DXGK_TIMED_OPERATION_INTERFACE, *PDXGK_TIMED_OPERATION_INTERFACE;

#define DXGK_SPB_INTERFACE_VERSION_1 0x01

typedef struct _DXGK_SPB_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *OpenSpbResource)(
        _In_ HANDLE DeviceHandle,
        _In_ LARGE_INTEGER SpbReourceId,
        _In_opt_ UNICODE_STRING *SpbResourceSubName,
        _In_ ACCESS_MASK DesiredAccess,
        _In_ ULONG ShareAccess,
        _In_ ULONG OpenOptions,
        _Outptr_ VOID **SpbResource
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *CloseSpbResource)(
        _In_ HANDLE DeviceHandle,
        _In_ VOID *SpbResource
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *ReadSpbResource)(
        _In_ HANDLE DeviceHandle,
        _In_ VOID *SpbResource,
        _In_ ULONG Length,
        _Out_writes_bytes_(Length) VOID *Buffer,
        _In_opt_ LARGE_INTEGER *ByteOffset,
        _In_opt_ HANDLE EventHandle,
        _Out_ IO_STATUS_BLOCK *IoStatusBlock
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *WriteSpbResource)(
        _In_ HANDLE DeviceHandle,
        _In_ VOID *SpbResource,
        _In_ ULONG Length,
        _In_reads_bytes_(Length) VOID *Buffer,
        _In_opt_ LARGE_INTEGER *ByteOffset,
        _In_opt_ HANDLE EventHandle,
        _Out_ IO_STATUS_BLOCK *IoStatusBlock
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    NTSTATUS (NTAPI *SpbResourceIoControl)(
        _In_ HANDLE DeviceHandle,
        _In_ VOID *SpbResource,
        _In_ ULONG IoControlCode,
        _In_ ULONG InBufferSize,
        _In_reads_bytes_(InBufferSize) VOID *InputBuffer,
        _In_ ULONG OutBufferSize,
        _Out_writes_bytes_(OutBufferSize) VOID *OutputBuffer,
        _In_opt_ HANDLE EventHandle,
        _Out_ IO_STATUS_BLOCK *IoStatusBlock
    );
} DXGK_SPB_INTERFACE, *PDXGK_SPB_INTERFACE;

#define DXGK_FIRMWARE_TABLE_INTERFACE_VERSION_1 0x01

typedef struct _DXGK_FIRMWARE_TABLE_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    _Success_(return >= 0 || return == STATUS_BUFFER_TOO_SMALL)
    NTSTATUS (NTAPI *EnumSystemFirmwareTables)(
        _In_ VOID *Context,
        _In_ ULONG ProviderSignature,
        _In_ ULONG BufferSize,
        _Pre_opt_bytecap_(BufferSize)
         _When_(return == STATUS_BUFFER_TOO_SMALL, _Post_equal_to_(_Old_(Buffer)))
         _When_(return != STATUS_BUFFER_TOO_SMALL, _Post_valid_)
         VOID *Buffer,
        _Out_ ULONG *RequiredSize
    );
    _IRQL_requires_DXGK_(PASSIVE_LEVEL)
    _Success_(return >= 0 || return == STATUS_BUFFER_TOO_SMALL)
    _When_(Buffer == NULL, _At_(BufferSize, _In_range_(==, 0)))
    NTSTATUS (NTAPI *ReadSystemFirmwareTable)(
        _In_ VOID *Context,
        _In_ ULONG ProviderSignature,
        _In_ ULONG TableId,
        _In_ ULONG BufferSize,
        _Pre_opt_bytecap_(BufferSize)
         _When_(return == STATUS_BUFFER_TOO_SMALL, _Post_equal_to_(_Old_(Buffer)))
         _When_(return != STATUS_BUFFER_TOO_SMALL, _Post_valid_)
         VOID *Buffer,
        _Out_ ULONG *RequiredSize
    );
} DXGK_FIRMWARE_TABLE_INTERFACE, *PDXGK_FIRMWARE_TABLE_INTERFACE;

/*
 * Passed from DxgKrnl -> Miniport
 * Implemented by enumeration of the device.
 */
typedef struct _DXGK_START_INFO {
    ULONG RequiredDmaQueueEntry;
    GUID AdapterGuid;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    LUID AdapterLuid;
#endif // DXGKDDI_INTERFACE_VERSION_WIN8
} DXGK_START_INFO, *PDXGK_START_INFO;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

typedef
_Function_class_DXGK_(DXGKCB_ACQUIRE_POST_DISPLAY_OWNERSHIP)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(APIENTRY *DXGKCB_ACQUIRE_POST_DISPLAY_OWNERSHIP)(
    _In_ HANDLE DeviceHandle,
    _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo
);

#endif // DXGKDDI_INTERFACE_VERSION_WIN8

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef enum _DXGK_FRAMEBUFFER_STATE {
    FrameBufferStateUnknown = 0,
    FrameBufferStateInitializedByFirmware = 1,
    FrameBufferStateInitializedByDriver = 2,
} DXGK_FRAMEBUFFER_STATE;

typedef struct _DXGK_DISPLAY_OWNERSHIP_FLAGS {
    union {
        struct {
            DXGK_FRAMEBUFFER_STATE FrameBufferState : 4;
        };
        UINT Value;
    };
} DXGK_DISPLAY_OWNERSHIP_FLAGS, *PDXGK_DISPLAY_OWNERSHIP_FLAGS;

typedef
_Function_class_DXGK_(DXGKCB_ACQUIRE_POST_DISPLAY_OWNERSHIP2)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(APIENTRY *DXGKCB_ACQUIRE_POST_DISPLAY_OWNERSHIP2)(
    _In_ HANDLE DeviceHandle,
    _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo,
    _Out_ PDXGK_DISPLAY_OWNERSHIP_FLAGS Flags
);

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2


typedef
NTSTATUS
(APIENTRY *DXGKCB_EVAL_ACPI_METHOD)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG DeviceUid,
    _In_reads_bytes_(AcpiInputSize) PACPI_EVAL_INPUT_BUFFER_COMPLEX AcpiInputBuffer,
    _In_range_(>=, sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX)) ULONG AcpiInputSize,
    _Out_writes_bytes_(AcpiOutputSize) PACPI_EVAL_OUTPUT_BUFFER AcpiOutputBuffer,
    _In_range_(>=, sizeof(ACPI_EVAL_OUTPUT_BUFFER)) ULONG AcpiOutputSize);

typedef
NTSTATUS
(APIENTRY *DXGKCB_GET_DEVICE_INFORMATION)(
    _In_ HANDLE DeviceHandle,
    _Out_ PDXGK_DEVICE_INFO DeviceInfo);

typedef
NTSTATUS
(APIENTRY *DXGKCB_INDICATE_CHILD_STATUS)(
    _In_ HANDLE DeviceHandle,
    _In_ PDXGK_CHILD_STATUS ChildStatus);

typedef
NTSTATUS
(APIENTRY *DXGKCB_MAP_MEMORY)(
    _In_ HANDLE DeviceHandle,
    _In_ PHYSICAL_ADDRESS TranslatedAddress,
    _In_ ULONG Length,
    _In_ BOOLEAN InIoSpace,
    _In_ BOOLEAN MapToUserMode,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _Outptr_ PVOID *VirtualAddress);

typedef
NTSTATUS
(APIENTRY *DXGKCB_QUERY_SERVICES)(
    _In_ HANDLE DeviceHandle,
    _In_ DXGK_SERVICES ServicesType,
    _Inout_ PINTERFACE Interface);

typedef
BOOLEAN
(APIENTRY *DXGKCB_QUEUE_DPC)(
    _In_ HANDLE DeviceHandle);

typedef
NTSTATUS
(APIENTRY *DXGKCB_READ_DEVICE_SPACE)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG DataType,
    _Out_writes_bytes_to_(Length, *BytesRead) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Out_ PULONG BytesRead);

typedef
NTSTATUS
(APIENTRY *DXGKCB_SYNCHRONIZE_EXECUTION)(
    _In_ HANDLE DeviceHandle,
    _In_ PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    _In_ PVOID Context,
    _In_ ULONG MessageNumber,
    _Out_ PBOOLEAN ReturnValue);

typedef
NTSTATUS
(APIENTRY *DXGKCB_UNMAP_MEMORY)(
    _In_ HANDLE DeviceHandle,
    _In_ PVOID VirtualAddress);

typedef
NTSTATUS
(APIENTRY *DXGKCB_WRITE_DEVICE_SPACE)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG DataType,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Out_ _Out_range_(<=, Length) PULONG BytesWritten);

typedef
NTSTATUS
(APIENTRY *DXGKCB_IS_DEVICE_PRESENT)(
    _In_ HANDLE DeviceHandle,
    _In_ PPCI_DEVICE_PRESENCE_PARAMETERS DevicePresenceParameters,
    _Out_ PBOOLEAN DevicePresent);

typedef
_Function_class_DXGK_(DXGKCB_LOG_ETW_EVENT)
_When_(EventBufferSize > 256, _IRQL_requires_DXGK_(PASSIVE_LEVEL))
VOID
(APIENTRY *DXGKCB_LOG_ETW_EVENT)(
    _In_ CONST LPCGUID EventGuid,
    _In_ UCHAR Type,
    _In_ USHORT EventBufferSize,
    _In_reads_bytes_(EventBufferSize) PVOID EventBuffer
);

typedef
_Function_class_DXGK_(DXGKCB_EXCLUDE_ADAPTER_ACCESS)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(APIENTRY *DXGKCB_EXCLUDE_ADAPTER_ACCESS)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG Attributes,
    _In_ DXGKDDI_PROTECTED_CALLBACK DxgkProtectedCallback,
    _In_ PVOID ProtectedCallbackContext
);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

#endif // DXGKDDI_INTERFACE_VERSION_WIN8

/*
 * Passed from DxgKrnl -> Miniport
 * Implemented by DxgKrnl itself.
 */
typedef struct _DXGKRNL_INTERFACE {
    ULONG Size;
    ULONG Version;
    HANDLE DeviceHandle;
    DXGKCB_EVAL_ACPI_METHOD DxgkCbEvalAcpiMethod;
    DXGKCB_GET_DEVICE_INFORMATION DxgkCbGetDeviceInformation;
    DXGKCB_INDICATE_CHILD_STATUS DxgkCbIndicateChildStatus;
    DXGKCB_MAP_MEMORY DxgkCbMapMemory;
    DXGKCB_QUEUE_DPC DxgkCbQueueDpc;
    DXGKCB_QUERY_SERVICES DxgkCbQueryServices;
    DXGKCB_READ_DEVICE_SPACE DxgkCbReadDeviceSpace;
    DXGKCB_SYNCHRONIZE_EXECUTION DxgkCbSynchronizeExecution;
    DXGKCB_UNMAP_MEMORY DxgkCbUnmapMemory;
    DXGKCB_WRITE_DEVICE_SPACE DxgkCbWriteDeviceSpace;
    DXGKCB_IS_DEVICE_PRESENT DxgkCbIsDevicePresent;
    DXGKCB_GETHANDLEDATA DxgkCbGetHandleData;
    DXGKCB_GETHANDLEPARENT DxgkCbGetHandleParent;
    DXGKCB_ENUMHANDLECHILDREN DxgkCbEnumHandleChildren;
    DXGKCB_NOTIFY_INTERRUPT DxgkCbNotifyInterrupt;
    DXGKCB_NOTIFY_DPC DxgkCbNotifyDpc;
    DXGKCB_QUERYVIDPNINTERFACE DxgkCbQueryVidPnInterface;
    DXGKCB_QUERYMONITORINTERFACE DxgkCbQueryMonitorInterface;
    DXGKCB_GETCAPTUREADDRESS DxgkCbGetCaptureAddress;
    DXGKCB_LOG_ETW_EVENT DxgkCbLogEtwEvent;
    DXGKCB_EXCLUDE_ADAPTER_ACCESS DxgkCbExcludeAdapterAccess;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    DXGKCB_CREATECONTEXTALLOCATION DxgkCbCreateContextAllocation;
    DXGKCB_DESTROYCONTEXTALLOCATION DxgkCbDestroyContextAllocation;
    DXGKCB_SETPOWERCOMPONENTACTIVE DxgkCbSetPowerComponentActive;
    DXGKCB_SETPOWERCOMPONENTIDLE DxgkCbSetPowerComponentIdle;
    DXGKCB_ACQUIRE_POST_DISPLAY_OWNERSHIP DxgkCbAcquirePostDisplayOwnership;
    DXGKCB_POWERRUNTIMECONTROLREQUEST DxgkCbPowerRuntimeControlRequest;
    DXGKCB_SETPOWERCOMPONENTLATENCY DxgkCbSetPowerComponentLatency;
    DXGKCB_SETPOWERCOMPONENTRESIDENCY DxgkCbSetPowerComponentResidency;
    DXGKCB_COMPLETEFSTATETRANSITION DxgkCbCompleteFStateTransition;
#endif // DXGKDDI_INTERFACE_VERSION_WIN8
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    DXGKCB_COMPLETEPSTATETRANSITION DxgkCbCompletePStateTransition;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    DXGKCB_MAPCONTEXTALLOCATION DxgkCbMapContextAllocation;
    DXGKCB_UPDATECONTEXTALLOCATION DxgkCbUpdateContextAllocation;
    DXGKCB_RESERVEGPUVIRTUALADDRESSRANGE DxgkCbReserveGpuVirtualAddressRange;
    DXGKCB_ACQUIREHANDLEDATA DxgkCbAcquireHandleData;
    DXGKCB_RELEASEHANDLEDATA DxgkCbReleaseHandleData;
    DXGKCB_HARDWARECONTENTPROTECTIONTEARDOWN DxgkCbHardwareContentProtectionTeardown;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    DXGKCB_MULTIPLANEOVERLAYDISABLED DxgkCbMultiPlaneOverlayDisabled;
    DXGKCB_DXGKCB_MITIGATEDRANGEUPDATE DxgkCbMitigatedRangeUpdate;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_1
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGKCB_INVALIDATEHWCONTEXT DxgkCbInvalidateHwContext;
    DXGKCB_INDICATE_CONNECTOR_CHANGE DxgkCbIndicateConnectorChange;
    DXGKCB_UNBLOCKUEFIFRAMEBUFFERRANGES DxgkCbUnblockUEFIFrameBufferRanges;
    DXGKCB_ACQUIRE_POST_DISPLAY_OWNERSHIP2 DxgkCbAcquirePostDisplayOwnership2;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    DXGKCB_SETPROTECTEDSESSIONSTATUS DxgkCbSetProtectedSessionStatus;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_3
} DXGKRNL_INTERFACE, *PDXGKRNL_INTERFACE;

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_ADD_DEVICE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_ADD_DEVICE(
    _In_ CONST PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PVOID *MiniportDeviceContext
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_START_DEVICE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_START_DEVICE(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ PDXGK_START_INFO DxgkStartInfo,
    _In_ PDXGKRNL_INTERFACE DxgkInterface,
    _Out_ PULONG NumberOfVideoPresentSources,
    _Out_ PULONG NumberOfChildren
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_STOP_DEVICE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_STOP_DEVICE(
    _In_ CONST PVOID MiniportDeviceContext
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_REMOVE_DEVICE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_REMOVE_DEVICE(
    _In_ CONST PVOID MiniportDeviceContext
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_DISPATCH_IO_REQUEST)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_DISPATCH_IO_REQUEST(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ ULONG VidPnSourceId,
    _In_ PVIDEO_REQUEST_PACKET VideoRequestPacket
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_QUERY_CHILD_RELATIONS)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_QUERY_CHILD_RELATIONS(
    _In_ CONST PVOID MiniportDeviceContext,
    _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR ChildRelations,
    _In_ ULONG ChildRelationsSize
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_QUERY_CHILD_STATUS)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_QUERY_CHILD_STATUS(
    _In_ CONST PVOID MiniportDeviceContext,
    _Inout_ PDXGK_CHILD_STATUS ChildStatus,
    _In_ BOOLEAN NonDestructiveOnly
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_INTERRUPT_ROUTINE)
_IRQL_requires_DXGK_(HIGH_LEVEL)
BOOLEAN
APIENTRY
DXGKDDI_INTERRUPT_ROUTINE(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ ULONG MessageNumber
);

typedef
_Function_class_DXGK_(DXGKDDI_DPC_ROUTINE)
_IRQL_requires_DXGK_(DISPATCH_LEVEL)
VOID
APIENTRY
DXGKDDI_DPC_ROUTINE(
    _In_ CONST PVOID MiniportDeviceContext
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_QUERY_DEVICE_DESCRIPTOR)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_QUERY_DEVICE_DESCRIPTOR(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ ULONG ChildUid,
    _Inout_ PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_SET_POWER_STATE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_SET_POWER_STATE(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ ULONG DeviceUid,
    _In_ DEVICE_POWER_STATE DevicePowerState,
    _In_ POWER_ACTION ActionType
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_NOTIFY_ACPI_EVENT)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_NOTIFY_ACPI_EVENT(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ DXGK_EVENT_TYPE EventType,
    _In_ ULONG Event,
    _In_ PVOID Argument,
    _Out_ PULONG AcpiFlags
);

typedef
_Function_class_DXGK_(DXGKDDI_RESET_DEVICE)
VOID
APIENTRY
DXGKDDI_RESET_DEVICE(
    _In_ CONST PVOID MiniportDeviceContext
);

typedef
_Function_class_DXGK_(DXGKDDI_UNLOAD)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
VOID
APIENTRY
DXGKDDI_UNLOAD(
    VOID
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_QUERY_INTERFACE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_QUERY_INTERFACE(
    _In_ CONST PVOID MiniportDeviceContext,
    _In_ PQUERY_INTERFACE QueryInterface
);

typedef
_Function_class_DXGK_(DXGKDDI_CONTROL_ETW_LOGGING)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
VOID
APIENTRY
DXGKDDI_CONTROL_ETW_LOGGING(
    _In_ BOOLEAN Enable,
    _In_ ULONG Flags,
    _In_ UCHAR Level
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_LINK_DEVICE)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_LINK_DEVICE(
    _In_ CONST PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ CONST PVOID MiniportDeviceContext,
    _Inout_ PLINKED_DEVICE LinkedDevice
);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _DXGK_PRE_START_INFO {
    union {
        struct {
            UINT ReservedIn;
        };
        UINT Input;
    };
    union {
        struct {
            UINT SupportPreserveBootDisplay : 1;
            UINT IsUEFIFrameBufferCpuAccessibleDuringStartup : 1;
            UINT ReservedOut : 30;
        };
        UINT Output;
    };
} DXGK_PRE_START_INFO, *PDXGK_PRE_START_INFO;

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_EXCHANGEPRESTARTINFO)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_EXCHANGEPRESTARTINFO(
    _In_ CONST HANDLE hAdapter,
    _Inout_ PDXGK_PRE_START_INFO pPreStartInfo
);

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_SETTARGETADJUSTEDCOLORIMETRY)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_SETTARGETADJUSTEDCOLORIMETRY(
    _In_ CONST HANDLE hAdapter,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _In_ DXGK_COLORIMETRY AdjustedColorimetry
);

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef DXGKDDI_ADD_DEVICE *PDXGKDDI_ADD_DEVICE;
typedef DXGKDDI_START_DEVICE *PDXGKDDI_START_DEVICE;
typedef DXGKDDI_STOP_DEVICE *PDXGKDDI_STOP_DEVICE;
typedef DXGKDDI_REMOVE_DEVICE *PDXGKDDI_REMOVE_DEVICE;
typedef DXGKDDI_DISPATCH_IO_REQUEST *PDXGKDDI_DISPATCH_IO_REQUEST;
typedef DXGKDDI_QUERY_CHILD_RELATIONS *PDXGKDDI_QUERY_CHILD_RELATIONS;
typedef DXGKDDI_QUERY_CHILD_STATUS *PDXGKDDI_QUERY_CHILD_STATUS;
typedef DXGKDDI_INTERRUPT_ROUTINE *PDXGKDDI_INTERRUPT_ROUTINE;
typedef DXGKDDI_DPC_ROUTINE *PDXGKDDI_DPC_ROUTINE;
typedef DXGKDDI_QUERY_DEVICE_DESCRIPTOR *PDXGKDDI_QUERY_DEVICE_DESCRIPTOR;
typedef DXGKDDI_SET_POWER_STATE *PDXGKDDI_SET_POWER_STATE;
typedef DXGKDDI_NOTIFY_ACPI_EVENT *PDXGKDDI_NOTIFY_ACPI_EVENT;
typedef DXGKDDI_RESET_DEVICE *PDXGKDDI_RESET_DEVICE;
typedef DXGKDDI_UNLOAD *PDXGKDDI_UNLOAD;
typedef DXGKDDI_QUERY_INTERFACE *PDXGKDDI_QUERY_INTERFACE;
typedef DXGKDDI_CONTROL_ETW_LOGGING *PDXGKDDI_CONTROL_ETW_LOGGING;
typedef DXGKDDI_LINK_DEVICE *PDXGKDDI_LINK_DEVICE;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
typedef DXGKDDI_EXCHANGEPRESTARTINFO *PDXGKDDI_EXCHANGEPRESTARTINFO;
typedef DXGKDDI_SETTARGETADJUSTEDCOLORIMETRY *PDXGKDDI_SETTARGETADJUSTEDCOLORIMETRY;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo
);

typedef struct _DXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS
{
    union
    {
        struct
        {
            UINT Reserved : 32;
        };
        UINT Value;
    };
} DXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS, *PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS;

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_SYSTEM_DISPLAY_ENABLE)
NTSTATUS
APIENTRY
DXGKDDI_SYSTEM_DISPLAY_ENABLE(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _In_ PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
    _Out_ UINT* Width,
    _Out_ UINT* Height,
    _Out_ D3DDDIFORMAT* ColorFormat
);

typedef
_Function_class_DXGK_(DXGKDDI_SYSTEM_DISPLAY_WRITE)
VOID
APIENTRY
DXGKDDI_SYSTEM_DISPLAY_WRITE(
    _In_ PVOID MiniportDeviceContext,
    _In_reads_bytes_(SourceHeight * SourceStride) PVOID Source,
    _In_ UINT SourceWidth,
    _In_ UINT SourceHeight,
    _In_ UINT SourceStride,
    _In_ UINT PositionX,
    _In_ UINT PositionY
);

typedef struct _DXGK_CHILD_CONTAINER_ID
{
    GUID ContainerId;
    struct
    {
        ULONG64 PortId;
        USHORT ManufacturerName;
        USHORT ProductCode;
    } EldInfo;
} DXGK_CHILD_CONTAINER_ID, *PDXGK_CHILD_CONTAINER_ID;

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_GET_CHILD_CONTAINER_ID)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_GET_CHILD_CONTAINER_ID(
    _In_ PVOID MiniportDeviceContext,
    _In_ ULONG ChildUid,
    _Inout_ PDXGK_CHILD_CONTAINER_ID ContainerId
);

typedef enum _DXGK_SURPRISE_REMOVAL_TYPE
{
    DxgkRemovalHibernation = 0,
    DxgkRemovalPnPNotify = 1,
} DXGK_SURPRISE_REMOVAL_TYPE;

typedef
_Check_return_
_Function_class_DXGK_(DXGKDDI_NOTIFY_SURPRISE_REMOVAL)
_IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
APIENTRY
DXGKDDI_NOTIFY_SURPRISE_REMOVAL(
    _In_ PVOID MiniportDeviceContext,
    _In_ DXGK_SURPRISE_REMOVAL_TYPE RemovalType
);

typedef DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP *PDXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP;
typedef DXGKDDI_SYSTEM_DISPLAY_ENABLE *PDXGKDDI_SYSTEM_DISPLAY_ENABLE;
typedef DXGKDDI_SYSTEM_DISPLAY_WRITE *PDXGKDDI_SYSTEM_DISPLAY_WRITE;
typedef DXGKDDI_GET_CHILD_CONTAINER_ID *PDXGKDDI_GET_CHILD_CONTAINER_ID;
typedef DXGKDDI_NOTIFY_SURPRISE_REMOVAL *PDXGKDDI_NOTIFY_SURPRISE_REMOVAL;

#endif // DXGKDDI_INTERFACE_VERSION

/*
 * Passed from Miniport -> DxgKrnl
 * Call backs Implemented by full WDDM drivers.
 */
typedef struct _DRIVER_INITIALIZATION_DATA {
    ULONG Version;
    PDXGKDDI_ADD_DEVICE DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE DxgkDdiDpcRoutine;
    PDXGKDDI_QUERY_CHILD_RELATIONS DxgkDdiQueryChildRelations;
    PDXGKDDI_QUERY_CHILD_STATUS DxgkDdiQueryChildStatus;
    PDXGKDDI_QUERY_DEVICE_DESCRIPTOR DxgkDdiQueryDeviceDescriptor;
    PDXGKDDI_SET_POWER_STATE DxgkDdiSetPowerState;
    PDXGKDDI_NOTIFY_ACPI_EVENT DxgkDdiNotifyAcpiEvent;
    PDXGKDDI_RESET_DEVICE DxgkDdiResetDevice;
    PDXGKDDI_UNLOAD DxgkDdiUnload;
    PDXGKDDI_QUERY_INTERFACE DxgkDdiQueryInterface;
    PDXGKDDI_CONTROL_ETW_LOGGING DxgkDdiControlEtwLogging;
    PDXGKDDI_QUERYADAPTERINFO DxgkDdiQueryAdapterInfo;
    PDXGKDDI_CREATEDEVICE DxgkDdiCreateDevice;
    PDXGKDDI_CREATEALLOCATION DxgkDdiCreateAllocation;
    PDXGKDDI_DESTROYALLOCATION DxgkDdiDestroyAllocation;
    PDXGKDDI_DESCRIBEALLOCATION DxgkDdiDescribeAllocation;
    PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA DxgkDdiGetStandardAllocationDriverData;
    PDXGKDDI_ACQUIRESWIZZLINGRANGE DxgkDdiAcquireSwizzlingRange;
    PDXGKDDI_RELEASESWIZZLINGRANGE DxgkDdiReleaseSwizzlingRange;
    PDXGKDDI_PATCH DxgkDdiPatch;
    PDXGKDDI_SUBMITCOMMAND DxgkDdiSubmitCommand;
    PDXGKDDI_PREEMPTCOMMAND DxgkDdiPreemptCommand;
    PDXGKDDI_BUILDPAGINGBUFFER DxgkDdiBuildPagingBuffer;
    PDXGKDDI_SETPALETTE DxgkDdiSetPalette;
    PDXGKDDI_SETPOINTERPOSITION DxgkDdiSetPointerPosition;
    PDXGKDDI_SETPOINTERSHAPE DxgkDdiSetPointerShape;
    PDXGKDDI_RESETFROMTIMEOUT DxgkDdiResetFromTimeout;
    PDXGKDDI_RESTARTFROMTIMEOUT DxgkDdiRestartFromTimeout;
    PDXGKDDI_ESCAPE DxgkDdiEscape;
    PDXGKDDI_COLLECTDBGINFO DxgkDdiCollectDbgInfo;
    PDXGKDDI_QUERYCURRENTFENCE DxgkDdiQueryCurrentFence;
    PDXGKDDI_ISSUPPORTEDVIDPN DxgkDdiIsSupportedVidPn;
    PDXGKDDI_RECOMMENDFUNCTIONALVIDPN DxgkDdiRecommendFunctionalVidPn;
    PDXGKDDI_ENUMVIDPNCOFUNCMODALITY DxgkDdiEnumVidPnCofuncModality;
    PDXGKDDI_SETVIDPNSOURCEADDRESS DxgkDdiSetVidPnSourceAddress;
    PDXGKDDI_SETVIDPNSOURCEVISIBILITY DxgkDdiSetVidPnSourceVisibility;
    PDXGKDDI_COMMITVIDPN DxgkDdiCommitVidPn;
    PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH DxgkDdiUpdateActiveVidPnPresentPath;
    PDXGKDDI_RECOMMENDMONITORMODES DxgkDdiRecommendMonitorModes;
    PDXGKDDI_RECOMMENDVIDPNTOPOLOGY DxgkDdiRecommendVidPnTopology;
    PDXGKDDI_GETSCANLINE DxgkDdiGetScanLine;
    PDXGKDDI_STOPCAPTURE DxgkDdiStopCapture;
    PDXGKDDI_CONTROLINTERRUPT DxgkDdiControlInterrupt;
    PDXGKDDI_CREATEOVERLAY DxgkDdiCreateOverlay;
    PDXGKDDI_DESTROYDEVICE DxgkDdiDestroyDevice;
    PDXGKDDI_OPENALLOCATIONINFO DxgkDdiOpenAllocation;
    PDXGKDDI_CLOSEALLOCATION DxgkDdiCloseAllocation;
    PDXGKDDI_RENDER DxgkDdiRender;
    PDXGKDDI_PRESENT DxgkDdiPresent;
    PDXGKDDI_UPDATEOVERLAY DxgkDdiUpdateOverlay;
    PDXGKDDI_FLIPOVERLAY DxgkDdiFlipOverlay;
    PDXGKDDI_DESTROYOVERLAY DxgkDdiDestroyOverlay;
    PDXGKDDI_CREATECONTEXT DxgkDdiCreateContext;
    PDXGKDDI_DESTROYCONTEXT DxgkDdiDestroyContext;
    PDXGKDDI_LINK_DEVICE DxgkDdiLinkDevice;
    PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT DxgkDdiSetDisplayPrivateDriverFormat;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    PVOID DxgkDdiDescribePageTable;
    PVOID DxgkDdiUpdatePageTable;
    PVOID DxgkDdiUpdatePageDirectory;
    PVOID DxgkDdiMovePageDirectory;
    PVOID DxgkDdiSubmitRender;
    PVOID DxgkDdiCreateAllocation2;
    PDXGKDDI_RENDER DxgkDdiRenderKm;
    VOID* Reserved;
    PDXGKDDI_QUERYVIDPNHWCAPABILITY DxgkDdiQueryVidPnHWCapability;
#endif // DXGKDDI_INTERFACE_VERSION
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    PDXGKDDISETPOWERCOMPONENTFSTATE DxgkDdiSetPowerComponentFState;
    PDXGKDDI_QUERYDEPENDENTENGINEGROUP DxgkDdiQueryDependentEngineGroup;
    PDXGKDDI_QUERYENGINESTATUS DxgkDdiQueryEngineStatus;
    PDXGKDDI_RESETENGINE DxgkDdiResetEngine;
    PDXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP DxgkDdiStopDeviceAndReleasePostDisplayOwnership;
    PDXGKDDI_SYSTEM_DISPLAY_ENABLE DxgkDdiSystemDisplayEnable;
    PDXGKDDI_SYSTEM_DISPLAY_WRITE DxgkDdiSystemDisplayWrite;
    PDXGKDDI_CANCELCOMMAND DxgkDdiCancelCommand;
    PDXGKDDI_GET_CHILD_CONTAINER_ID DxgkDdiGetChildContainerId;
    PDXGKDDIPOWERRUNTIMECONTROLREQUEST DxgkDdiPowerRuntimeControlRequest;
    PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay;
    PDXGKDDI_NOTIFY_SURPRISE_REMOVAL DxgkDdiNotifySurpriseRemoval;
#endif // DXGKDDI_INTERFACE_VERSION
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    PDXGKDDI_GETNODEMETADATA DxgkDdiGetNodeMetadata;
    PDXGKDDISETPOWERPSTATE DxgkDdiSetPowerPState;
    PDXGKDDI_CONTROLINTERRUPT2 DxgkDdiControlInterrupt2;
    PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT DxgkDdiCheckMultiPlaneOverlaySupport;
    PDXGKDDI_CALIBRATEGPUCLOCK DxgkDdiCalibrateGpuClock;
    PDXGKDDI_FORMATHISTORYBUFFER DxgkDdiFormatHistoryBuffer;
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    PDXGKDDI_RENDERGDI DxgkDdiRenderGdi;
    PDXGKDDI_SUBMITCOMMANDVIRTUAL DxgkDdiSubmitCommandVirtual;
    PDXGKDDI_SETROOTPAGETABLE DxgkDdiSetRootPageTable;
    PDXGKDDI_GETROOTPAGETABLESIZE DxgkDdiGetRootPageTableSize;
    PDXGKDDI_MAPCPUHOSTAPERTURE DxgkDdiMapCpuHostAperture;
    PDXGKDDI_UNMAPCPUHOSTAPERTURE DxgkDdiUnmapCpuHostAperture;
    PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT2 DxgkDdiCheckMultiPlaneOverlaySupport2;
    PDXGKDDI_CREATEPROCESS DxgkDdiCreateProcess;
    PDXGKDDI_DESTROYPROCESS DxgkDdiDestroyProcess;
    PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY2 DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay2;
    void* Reserved1;
    void* Reserved2;
    PDXGKDDI_POWERRUNTIMESETDEVICEHANDLE DxgkDdiPowerRuntimeSetDeviceHandle;
    PDXGKDDI_SETSTABLEPOWERSTATE DxgkDdiSetStablePowerState;
    PDXGKDDI_SETVIDEOPROTECTEDREGION DxgkDdiSetVideoProtectedRegion;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT3 DxgkDdiCheckMultiPlaneOverlaySupport3;
    PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY3 DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay3;
    PDXGKDDI_POSTMULTIPLANEOVERLAYPRESENT DxgkDdiPostMultiPlaneOverlayPresent;
    PDXGKDDI_VALIDATEUPDATEALLOCATIONPROPERTY DxgkDdiValidateUpdateAllocationProperty;
    PDXGKDDI_CONTROLMODEBEHAVIOR DxgkDdiControlModeBehavior;
    PDXGKDDI_UPDATEMONITORLINKINFO DxgkDdiUpdateMonitorLinkInfo;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    PDXGKDDI_CREATEHWCONTEXT DxgkDdiCreateHwContext;
    PDXGKDDI_DESTROYHWCONTEXT DxgkDdiDestroyHwContext;
    PDXGKDDI_CREATEHWQUEUE DxgkDdiCreateHwQueue;
    PDXGKDDI_DESTROYHWQUEUE DxgkDdiDestroyHwQueue;
    PDXGKDDI_SUBMITCOMMANDTOHWQUEUE DxgkDdiSubmitCommandToHwQueue;
    PDXGKDDI_SWITCHTOHWCONTEXTLIST DxgkDdiSwitchToHwContextList;
    PDXGKDDI_RESETHWENGINE DxgkDdiResetHwEngine;
    PDXGKDDI_CREATEPERIODICFRAMENOTIFICATION DxgkDdiCreatePeriodicFrameNotification;
    PDXGKDDI_DESTROYPERIODICFRAMENOTIFICATION DxgkDdiDestroyPeriodicFrameNotification;
    PDXGKDDI_SETTIMINGSFROMVIDPN DxgkDdiSetTimingsFromVidPn;
    PDXGKDDI_SETTARGETGAMMA DxgkDdiSetTargetGamma;
    PDXGKDDI_SETTARGETCONTENTTYPE DxgkDdiSetTargetContentType;
    PDXGKDDI_SETTARGETANALOGCOPYPROTECTION DxgkDdiSetTargetAnalogCopyProtection;
    PDXGKDDI_SETTARGETADJUSTEDCOLORIMETRY DxgkDdiSetTargetAdjustedColorimetry;
    PDXGKDDI_DISPLAYDETECTCONTROL DxgkDdiDisplayDetectControl;
    PDXGKDDI_QUERYCONNECTIONCHANGE DxgkDdiQueryConnectionChange;
    PDXGKDDI_EXCHANGEPRESTARTINFO DxgkDdiExchangePreStartInfo;
    PDXGKDDI_GETMULTIPLANEOVERLAYCAPS DxgkDdiGetMultiPlaneOverlayCaps;
    PDXGKDDI_GETPOSTCOMPOSITIONCAPS DxgkDdiGetPostCompositionCaps;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    PDXGKDDI_UPDATEHWCONTEXTSTATE DxgkDdiUpdateHwContextState;
    PDXGKDDI_CREATEPROTECTEDSESSION DxgkDdiCreateProtectedSession;
    PDXGKDDI_DESTROYPROTECTEDSESSION DxgkDdiDestroyProtectedSession;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
} DRIVER_INITIALIZATION_DATA, *PDRIVER_INITIALIZATION_DATA;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
/*
 * Passed from Miniport -> DxgKrnl
 * Callbacks implemented by display-only drivers (KMDDOD).
 */
typedef struct _KMDDOD_INITIALIZATION_DATA {
    ULONG Version;
    PDXGKDDI_ADD_DEVICE DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE DxgkDdiDpcRoutine;
    PDXGKDDI_QUERY_CHILD_RELATIONS DxgkDdiQueryChildRelations;
    PDXGKDDI_QUERY_CHILD_STATUS DxgkDdiQueryChildStatus;
    PDXGKDDI_QUERY_DEVICE_DESCRIPTOR DxgkDdiQueryDeviceDescriptor;
    PDXGKDDI_SET_POWER_STATE DxgkDdiSetPowerState;
    PDXGKDDI_NOTIFY_ACPI_EVENT DxgkDdiNotifyAcpiEvent;
    PDXGKDDI_RESET_DEVICE DxgkDdiResetDevice;
    PDXGKDDI_UNLOAD DxgkDdiUnload;
    PDXGKDDI_QUERY_INTERFACE DxgkDdiQueryInterface;
    PDXGKDDI_CONTROL_ETW_LOGGING DxgkDdiControlEtwLogging;
    PDXGKDDI_QUERYADAPTERINFO DxgkDdiQueryAdapterInfo;
    PDXGKDDI_SETPALETTE DxgkDdiSetPalette;
    PDXGKDDI_SETPOINTERPOSITION DxgkDdiSetPointerPosition;
    PDXGKDDI_SETPOINTERSHAPE DxgkDdiSetPointerShape;
    PDXGKDDI_ESCAPE DxgkDdiEscape;
    PDXGKDDI_COLLECTDBGINFO DxgkDdiCollectDbgInfo;
    PDXGKDDI_ISSUPPORTEDVIDPN DxgkDdiIsSupportedVidPn;
    PDXGKDDI_RECOMMENDFUNCTIONALVIDPN DxgkDdiRecommendFunctionalVidPn;
    PDXGKDDI_ENUMVIDPNCOFUNCMODALITY DxgkDdiEnumVidPnCofuncModality;
    PDXGKDDI_SETVIDPNSOURCEVISIBILITY DxgkDdiSetVidPnSourceVisibility;
    PDXGKDDI_COMMITVIDPN DxgkDdiCommitVidPn;
    PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH DxgkDdiUpdateActiveVidPnPresentPath;
    PDXGKDDI_RECOMMENDMONITORMODES DxgkDdiRecommendMonitorModes;
    PDXGKDDI_GETSCANLINE DxgkDdiGetScanLine;
    PDXGKDDI_QUERYVIDPNHWCAPABILITY DxgkDdiQueryVidPnHWCapability;
    PDXGKDDI_PRESENTDISPLAYONLY DxgkDdiPresentDisplayOnly;
    PDXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP DxgkDdiStopDeviceAndReleasePostDisplayOwnership;
    PDXGKDDI_SYSTEM_DISPLAY_ENABLE DxgkDdiSystemDisplayEnable;
    PDXGKDDI_SYSTEM_DISPLAY_WRITE DxgkDdiSystemDisplayWrite;
    PDXGKDDI_GET_CHILD_CONTAINER_ID DxgkDdiGetChildContainerId;
    PDXGKDDISETPOWERCOMPONENTFSTATE DxgkDdiSetPowerComponentFState;
    PDXGKDDIPOWERRUNTIMECONTROLREQUEST DxgkDdiPowerRuntimeControlRequest;
    PDXGKDDI_NOTIFY_SURPRISE_REMOVAL DxgkDdiNotifySurpriseRemoval;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    PDXGKDDI_POWERRUNTIMESETDEVICEHANDLE DxgkDdiPowerRuntimeSetDeviceHandle;
#endif
} KMDDOD_INITIALIZATION_DATA, *PKMDDOD_INITIALIZATION_DATA;
#endif // DXGKDDI_INTERFACE_VERSION_WIN8

/*
 * DispLib exports
 */

typedef enum _DEBUG_LEVEL {
    DlDebugError,
    DlDebugWarning,
    DlDebugTrace,
    DlDebugInfo
} DEBUG_LEVEL;

NTSTATUS
DxgkInitialize(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PDRIVER_INITIALIZATION_DATA DriverInitializationData
);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

NTSTATUS
DxgkInitializeDisplayOnlyDriver(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PKMDDOD_INITIALIZATION_DATA KmdDodInitializationData
);

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

NTSTATUS
DxgkUnInitialize(
    _In_ PDRIVER_OBJECT DriverObject
);

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#endif // _DISPMPRT_H_

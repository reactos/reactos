/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfUsb.h

Abstract:

    Contains prototypes for interfacing with a USB connected device

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFUSB_H_
#define _WDFUSB_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

#ifndef BMREQUEST_HOST_TO_DEVICE
#define BMREQUEST_HOST_TO_DEVICE        0
#endif  // BMREQUEST_HOST_TO_DEVICE

#ifndef BMREQUEST_DEVICE_TO_HOST
#define BMREQUEST_DEVICE_TO_HOST        1
#endif  // BMREQUEST_DEVICE_TO_HOST

#ifndef BMREQUEST_STANDARD
#define BMREQUEST_STANDARD              0
#endif  // BMREQUEST_STANDARD

#ifndef BMREQUEST_CLASS
#define BMREQUEST_CLASS                 1
#endif  // BMREQUEST_CLASS

#ifndef BMREQUEST_VENDOR
#define BMREQUEST_VENDOR                2
#endif  // BMREQUEST_VENDOR

#ifndef BMREQUEST_TO_DEVICE
#define BMREQUEST_TO_DEVICE             0
#endif  // BMREQUEST_TO_DEVICE

#ifndef BMREQUEST_TO_INTERFACE
#define BMREQUEST_TO_INTERFACE          1
#endif  // BMREQUEST_TO_INTERFACE

#ifndef BMREQUEST_TO_ENDPOINT
#define BMREQUEST_TO_ENDPOINT           2
#endif  // BMREQUEST_TO_ENDPOINT

#ifndef BMREQUEST_TO_OTHER
#define BMREQUEST_TO_OTHER              3
#endif  // BMREQUEST_TO_OTHER

typedef enum _WDF_USB_REQUEST_TYPE {
    WdfUsbRequestTypeInvalid = 0,
    WdfUsbRequestTypeNoFormat,
    WdfUsbRequestTypeDeviceString,
    WdfUsbRequestTypeDeviceControlTransfer,
    WdfUsbRequestTypeDeviceUrb,
    WdfUsbRequestTypePipeWrite,
    WdfUsbRequestTypePipeRead,
    WdfUsbRequestTypePipeAbort,
    WdfUsbRequestTypePipeReset,
    WdfUsbRequestTypePipeUrb,
} WDF_USB_REQUEST_TYPE, *PWDF_USB_REQUEST_TYPE;

typedef enum _WDF_USB_BMREQUEST_DIRECTION {
    BmRequestHostToDevice = BMREQUEST_HOST_TO_DEVICE,
    BmRequestDeviceToHost = BMREQUEST_DEVICE_TO_HOST,
} WDF_USB_BMREQUEST_DIRECTION;

typedef enum _WDF_USB_BMREQUEST_TYPE {
    BmRequestStandard = BMREQUEST_STANDARD,
    BmRequestClass = BMREQUEST_CLASS,
    BmRequestVendor = BMREQUEST_VENDOR,
} WDF_USB_BMREQUEST_TYPE;

typedef enum _WDF_USB_BMREQUEST_RECIPIENT {
    BmRequestToDevice = BMREQUEST_TO_DEVICE,
    BmRequestToInterface = BMREQUEST_TO_INTERFACE,
    BmRequestToEndpoint = BMREQUEST_TO_ENDPOINT,
    BmRequestToOther = BMREQUEST_TO_OTHER,
} WDF_USB_BMREQUEST_RECIPIENT;

typedef enum _WDF_USB_PIPE_TYPE {
    WdfUsbPipeTypeInvalid = 0,
    WdfUsbPipeTypeControl,
    WdfUsbPipeTypeIsochronous,
    WdfUsbPipeTypeBulk,
    WdfUsbPipeTypeInterrupt,
} WDF_USB_PIPE_TYPE;

typedef enum _WdfUsbTargetDeviceSelectConfigType {
    WdfUsbTargetDeviceSelectConfigTypeInvalid = 0,
    WdfUsbTargetDeviceSelectConfigTypeDeconfig = 1,
    WdfUsbTargetDeviceSelectConfigTypeSingleInterface = 2,
    WdfUsbTargetDeviceSelectConfigTypeMultiInterface = 3,
    WdfUsbTargetDeviceSelectConfigTypeInterfacesPairs = 4,
    WdfUsbTargetDeviceSelectConfigTypeInterfacesDescriptor = 5,
    WdfUsbTargetDeviceSelectConfigTypeUrb = 6,
} WdfUsbTargetDeviceSelectConfigType;

typedef enum _WdfUsbTargetDeviceSelectSettingType {
    WdfUsbInterfaceSelectSettingTypeDescriptor = 0x10,
    WdfUsbInterfaceSelectSettingTypeSetting = 0x11,
    WdfUsbInterfaceSelectSettingTypeUrb = 0x12,
} WdfUsbTargetDeviceSelectSettingType;

typedef enum _WDF_USB_DEVICE_TRAITS {
    WDF_USB_DEVICE_TRAIT_SELF_POWERED =        0x00000001,
    WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE = 0x00000002,
    WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED =       0x00000004,
} WDF_USB_DEVICE_TRAITS;



typedef union _WDF_USB_CONTROL_SETUP_PACKET {
    struct {
        union {
            #pragma warning(disable:4214) // bit field types other than int
            struct {
                //
                // Valid values are BMREQUEST_TO_DEVICE, BMREQUEST_TO_INTERFACE,
                // BMREQUEST_TO_ENDPOINT, BMREQUEST_TO_OTHER
                //
                BYTE Recipient:2;

                BYTE Reserved:3;

                //
                // Valid values are BMREQUEST_STANDARD, BMREQUEST_CLASS,
                // BMREQUEST_VENDOR
                //
                BYTE Type:2;

                //
                // Valid values are BMREQUEST_HOST_TO_DEVICE,
                // BMREQUEST_DEVICE_TO_HOST
                //
                BYTE Dir:1;
            } Request;
            #pragma warning(default:4214) // bit field types other than int
            BYTE Byte;
        } bm;

        BYTE bRequest;

        union {
            struct {
                BYTE LowByte;
                BYTE HiByte;
            } Bytes;
            USHORT Value;
        } wValue;

        union {
            struct {
                BYTE LowByte;
                BYTE HiByte;
            } Bytes;
            USHORT Value;
        } wIndex;

        USHORT wLength;
    } Packet;

    struct {
        BYTE Bytes[8];
    } Generic;
} WDF_USB_CONTROL_SETUP_PACKET, *PWDF_USB_CONTROL_SETUP_PACKET;

VOID
FORCEINLINE
WDF_USB_CONTROL_SETUP_PACKET_INIT(
    __out PWDF_USB_CONTROL_SETUP_PACKET Packet,
    __in WDF_USB_BMREQUEST_DIRECTION Direction,
    __in WDF_USB_BMREQUEST_RECIPIENT Recipient,
    __in BYTE Request,
    __in USHORT Value,
    __in USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WDF_USB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) Direction;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestStandard;
    Packet->Packet.bm.Request.Recipient = (BYTE) Recipient;

    Packet->Packet.bRequest = Request;
    Packet->Packet.wValue.Value = Value;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(
    __out PWDF_USB_CONTROL_SETUP_PACKET Packet,
    __in WDF_USB_BMREQUEST_DIRECTION Direction,
    __in WDF_USB_BMREQUEST_RECIPIENT Recipient,
    __in BYTE Request,
    __in USHORT Value,
    __in USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WDF_USB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) Direction;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestClass;
    Packet->Packet.bm.Request.Recipient = (BYTE) Recipient;

    Packet->Packet.bRequest = Request;
    Packet->Packet.wValue.Value = Value;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
    __out PWDF_USB_CONTROL_SETUP_PACKET Packet,
    __in WDF_USB_BMREQUEST_DIRECTION Direction,
    __in WDF_USB_BMREQUEST_RECIPIENT Recipient,
    __in BYTE Request,
    __in USHORT Value,
    __in USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WDF_USB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) Direction;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestVendor;
    Packet->Packet.bm.Request.Recipient = (BYTE) Recipient;

    Packet->Packet.bRequest = Request;
    Packet->Packet.wValue.Value = Value;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(
    __out PWDF_USB_CONTROL_SETUP_PACKET Packet,
    __in WDF_USB_BMREQUEST_RECIPIENT BmRequestRecipient,
    __in USHORT FeatureSelector,
    __in USHORT Index,
    __in BOOLEAN SetFeature
    )
{
    RtlZeroMemory(Packet, sizeof(WDF_USB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) BmRequestHostToDevice;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestStandard;
    Packet->Packet.bm.Request.Recipient = (BYTE) BmRequestRecipient;

    if (SetFeature) {
        Packet->Packet.bRequest = USB_REQUEST_SET_FEATURE;
    }
    else {
        Packet->Packet.bRequest = USB_REQUEST_CLEAR_FEATURE;
    }

    Packet->Packet.wValue.Value = FeatureSelector;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WDF_USB_CONTROL_SETUP_PACKET_INIT_GET_STATUS(
    __out PWDF_USB_CONTROL_SETUP_PACKET Packet,
    __in WDF_USB_BMREQUEST_RECIPIENT BmRequestRecipient,
    __in USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WDF_USB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) BmRequestDeviceToHost;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestStandard;
    Packet->Packet.bm.Request.Recipient = (BYTE) BmRequestRecipient;

    Packet->Packet.bRequest = USB_REQUEST_GET_STATUS;
    Packet->Packet.wIndex.Value = Index;
    Packet->Packet.wValue.Value = 0;

    // Packet->Packet.wLength will be set by the formatting function
}

typedef struct _WDF_USB_REQUEST_COMPLETION_PARAMS {
    USBD_STATUS UsbdStatus;

    WDF_USB_REQUEST_TYPE Type;

    union {
        struct {
            WDFMEMORY Buffer;
            USHORT LangID;
            UCHAR StringIndex;

            //
            // If STATUS_BUFFER_OVERFLOW is returned, this field will contain the
            // number of bytes required to retrieve the entire string.
            //
            UCHAR RequiredSize;
        } DeviceString;

        struct {
            WDFMEMORY Buffer;
            WDF_USB_CONTROL_SETUP_PACKET SetupPacket;
            ULONG Length;
        } DeviceControlTransfer;

        struct {
            WDFMEMORY Buffer;
        } DeviceUrb;

        struct {
            WDFMEMORY Buffer;
            size_t Length;
            size_t Offset;
        } PipeWrite;

        struct {
            WDFMEMORY Buffer;
            size_t Length;
            size_t Offset;
        } PipeRead;

        struct {
            WDFMEMORY Buffer;
        } PipeUrb;
    } Parameters;

} WDF_USB_REQUEST_COMPLETION_PARAMS, *PWDF_USB_REQUEST_COMPLETION_PARAMS;

typedef
__drv_functionClass(EVT_WDF_USB_READER_COMPLETION_ROUTINE)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_USB_READER_COMPLETION_ROUTINE(
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFMEMORY Buffer,
    __in
    size_t NumBytesTransferred,
    __in
    WDFCONTEXT Context
    );

typedef EVT_WDF_USB_READER_COMPLETION_ROUTINE *PFN_WDF_USB_READER_COMPLETION_ROUTINE;

typedef
__drv_functionClass(EVT_WDF_USB_READERS_FAILED)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
BOOLEAN
EVT_WDF_USB_READERS_FAILED(
    __in
    WDFUSBPIPE Pipe,
    __in
    NTSTATUS Status,
    __in
    USBD_STATUS UsbdStatus
    );

typedef EVT_WDF_USB_READERS_FAILED *PFN_WDF_USB_READERS_FAILED;

typedef struct _WDF_USB_CONTINUOUS_READER_CONFIG {
    //
    // Size of the string in bytes
    //
    ULONG Size;

    //
    // Number of bytes to send ask for from the usb device.
    //
    size_t TransferLength;

    //
    // Number of bytes to allocate before the requested transfer length
    //
    size_t HeaderLength;

    //
    // Number of bytes to allocate after the requested transfer length
    //
    size_t TrailerLength;

    //
    // Number of reads to send to the device at once.  If zero is specified, the
    // default will be used.
    //
    UCHAR NumPendingReads;

    //
    // Optional attributes to apply to each WDFMEMORY allocated for each read
    //
    PWDF_OBJECT_ATTRIBUTES BufferAttributes;

    //
    // Event callback invoked when a read is completed
    //
    PFN_WDF_USB_READER_COMPLETION_ROUTINE EvtUsbTargetPipeReadComplete;

    //
    // Context to be passed to EvtUsbTargetPipeReadComplete
    //
    WDFCONTEXT EvtUsbTargetPipeReadCompleteContext;

    //
    // Event callback invoked when a reader fails.  If TRUE is returned, the
    // readers are restarted.
    //
    PFN_WDF_USB_READERS_FAILED EvtUsbTargetPipeReadersFailed;

} WDF_USB_CONTINUOUS_READER_CONFIG, *PWDF_USB_CONTINUOUS_READER_CONFIG;


VOID
FORCEINLINE
WDF_USB_CONTINUOUS_READER_CONFIG_INIT(
    __out PWDF_USB_CONTINUOUS_READER_CONFIG Config,
    __in PFN_WDF_USB_READER_COMPLETION_ROUTINE EvtUsbTargetPipeReadComplete,
    __in WDFCONTEXT EvtUsbTargetPipeReadCompleteContext,
    __in size_t TransferLength
    )

{
    RtlZeroMemory(Config, sizeof(WDF_USB_CONTINUOUS_READER_CONFIG));
    Config->Size = sizeof(WDF_USB_CONTINUOUS_READER_CONFIG);

    Config->EvtUsbTargetPipeReadComplete = EvtUsbTargetPipeReadComplete;
    Config->EvtUsbTargetPipeReadCompleteContext = EvtUsbTargetPipeReadCompleteContext;
    Config->TransferLength = TransferLength;
}

WDFIOTARGET
FORCEINLINE
WdfUsbTargetDeviceGetIoTarget(
    __in WDFUSBDEVICE UsbDevice
    )
{
    return (WDFIOTARGET) UsbDevice;
}

typedef struct _WDF_USB_DEVICE_INFORMATION {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // USBD version information
    //
    USBD_VERSION_INFORMATION UsbdVersionInformation;

    //
    // Usb controller port capabilities
    //
    ULONG HcdPortCapabilities;

    //
    // Bitfield of WDF_USB_DEVICE_TRAITS values
    //
    ULONG Traits;

} WDF_USB_DEVICE_INFORMATION, *PWDF_USB_DEVICE_INFORMATION;

VOID
FORCEINLINE
WDF_USB_DEVICE_INFORMATION_INIT(
    __out PWDF_USB_DEVICE_INFORMATION Udi
    )
{
    RtlZeroMemory(Udi, sizeof(WDF_USB_DEVICE_INFORMATION));
    Udi->Size = sizeof(WDF_USB_DEVICE_INFORMATION);
}

typedef struct _WDF_USB_INTERFACE_SETTING_PAIR {
    //
    // Interface to select
    //
    WDFUSBINTERFACE UsbInterface;

    //
    // Setting to select on UsbInterface
    //
    UCHAR SettingIndex;

} WDF_USB_INTERFACE_SETTING_PAIR, *PWDF_USB_INTERFACE_SETTING_PAIR;

typedef struct _WDF_USB_DEVICE_SELECT_CONFIG_PARAMS {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Type of select config, one of WdfUsbTargetDeviceSelectConfigType values
    //
    WdfUsbTargetDeviceSelectConfigType Type;


    union {
        struct {
            //
            // Configuration descriptor to use
            //
            PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;

            //
            // Array of interface descriptors pointers.
            //
            PUSB_INTERFACE_DESCRIPTOR * InterfaceDescriptors;

            //
            // Number of elements in the InterfaceDescrtiptors pointer array.
            //
            ULONG NumInterfaceDescriptors;

        } Descriptor;

        struct {
            //
            // Preallocated select config URB formatted by the caller.
            // Will be used, as supplied without modification, as the select
            // config request.
            //
            PURB Urb;

        } Urb;

        struct {
            //
            // Number of pipes configured on the single after.  This value is
            // returned to the caller after a succssful call.
            //
            UCHAR   NumberConfiguredPipes;

            //
            // The interface which was configred.  This value is returned to the
            // caller after a successful call.
            //
            WDFUSBINTERFACE ConfiguredUsbInterface;

        } SingleInterface;

        struct {
            //
            // Number of interface pairs in the Pairs array
            //
            UCHAR NumberInterfaces;

            //
            // Array of interface + settings
            //
            PWDF_USB_INTERFACE_SETTING_PAIR Pairs;

            //
            // Number of interfaces which were configured after a successful call
            //
            UCHAR NumberOfConfiguredInterfaces;

        } MultiInterface;

    } Types;

} WDF_USB_DEVICE_SELECT_CONFIG_PARAMS, *PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS;


VOID
FORCEINLINE
WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(
    __out PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS));

    Params->Size = sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS);
    Params->Type = WdfUsbTargetDeviceSelectConfigTypeSingleInterface;
}

VOID
FORCEINLINE
WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(
    __inout  PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params,
    __in_opt UCHAR NumberInterfaces,
    __in_opt PWDF_USB_INTERFACE_SETTING_PAIR SettingPairs
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS));

    Params->Size = sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS);

    if (SettingPairs != NULL && NumberInterfaces != 0) {
        Params->Type = WdfUsbTargetDeviceSelectConfigTypeInterfacesPairs;

        Params->Types.MultiInterface.NumberInterfaces = NumberInterfaces;
        Params->Types.MultiInterface.Pairs = SettingPairs;
    }
    else {
        Params->Type = WdfUsbTargetDeviceSelectConfigTypeMultiInterface;
    }
}

VOID
FORCEINLINE
WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_INTERFACES_DESCRIPTORS(
    __out PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params,
    __in  PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    __in  PUSB_INTERFACE_DESCRIPTOR* InterfaceDescriptors,
    __in  ULONG NumInterfaceDescriptors
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS));

    Params->Size = sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS);
    Params->Type = WdfUsbTargetDeviceSelectConfigTypeInterfacesDescriptor;
    Params->Types.Descriptor.ConfigurationDescriptor = ConfigDescriptor;
    Params->Types.Descriptor.InterfaceDescriptors = InterfaceDescriptors;
    Params->Types.Descriptor.NumInterfaceDescriptors = NumInterfaceDescriptors;
}

VOID
FORCEINLINE
WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_URB(
    __out PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params,
    __in  PURB Urb
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS));

    Params->Size = sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS);
    Params->Type = WdfUsbTargetDeviceSelectConfigTypeUrb;
    Params->Types.Urb.Urb = Urb;
}


VOID
FORCEINLINE
WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_DECONFIG(
    __out PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS));

    Params->Size = sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS);
    Params->Type = WdfUsbTargetDeviceSelectConfigTypeDeconfig;
}

typedef struct _WDF_USB_INTERFACE_SELECT_SETTING_PARAMS {
    //
    // Size of this data structure in bytes
    //
    ULONG Size;

    //
    // Type of select interface as indicated by one of the
    // WdfUsbTargetDeviceSelectSettingType values.
    //
    WdfUsbTargetDeviceSelectSettingType Type;

    union {

        struct {
            //
            // Interface descriptor that will be used in the interface selection
            //
            PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

        } Descriptor;

        struct {
            //
            // The setting index of the WDFUSBINTERFACE to use
            //
            UCHAR SettingIndex;

        } Interface;

        struct {
            //
            // Preformatted select interface URB which will be used in the
            // select interface request.
            //
            PURB Urb;

        } Urb;

    } Types;

} WDF_USB_INTERFACE_SELECT_SETTING_PARAMS, *PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS;

VOID
FORCEINLINE
WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_DESCRIPTOR(
    __out PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params,
    __in  PUSB_INTERFACE_DESCRIPTOR Interface
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS));

    Params->Size = sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS);
    Params->Type = WdfUsbInterfaceSelectSettingTypeDescriptor;
    Params->Types.Descriptor.InterfaceDescriptor = Interface;
}

VOID
FORCEINLINE
WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_URB(
    __out PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params,
    __in  PURB Urb
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS));

    Params->Size = sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS);
    Params->Type = WdfUsbInterfaceSelectSettingTypeUrb;
    Params->Types.Urb.Urb = Urb;
}

VOID
FORCEINLINE
WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(
    __out PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params,
    __in  UCHAR SettingIndex
    )
{
    RtlZeroMemory(Params, sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS));

    Params->Size = sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS);
    Params->Type = WdfUsbInterfaceSelectSettingTypeSetting;
    Params->Types.Interface.SettingIndex = SettingIndex;
}

WDFIOTARGET
FORCEINLINE
WdfUsbTargetPipeGetIoTarget(
    __in WDFUSBPIPE Pipe
    )
{
    return (WDFIOTARGET) Pipe;
}


typedef struct _WDF_USB_PIPE_INFORMATION {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Maximum packet size this device is capable of
    //
    ULONG MaximumPacketSize;

    //
    // Raw endpoint address of the device as described by its descriptor
    //
    UCHAR EndpointAddress;

    //
    // Polling interval
    //
    UCHAR Interval;

    //
    // Which alternate setting this structure is relevant for
    //
    UCHAR SettingIndex;

    //
    // The type of the pipe
    WDF_USB_PIPE_TYPE PipeType;

    //
    // Maximum size of one transfer which should be sent to the host controller
    //
    ULONG  MaximumTransferSize;

} WDF_USB_PIPE_INFORMATION, *PWDF_USB_PIPE_INFORMATION;

VOID
FORCEINLINE
WDF_USB_PIPE_INFORMATION_INIT(
    __out PWDF_USB_PIPE_INFORMATION Info
    )
{
    RtlZeroMemory(Info, sizeof(WDF_USB_PIPE_INFORMATION));

    Info->Size = sizeof(WDF_USB_PIPE_INFORMATION);
}

BOOLEAN
FORCEINLINE
WDF_USB_PIPE_DIRECTION_IN(
    __in UCHAR EndpointAddress
    )
{
    //
    // If the high bit is set, we have an IN pipe
    //
    return (EndpointAddress & USB_ENDPOINT_DIRECTION_MASK) ? TRUE : FALSE;
}

BOOLEAN
FORCEINLINE
WDF_USB_PIPE_DIRECTION_OUT(
    __in UCHAR EndpointAddress
    )
{
    //
    // If the high bit is clear, we have an OUT pipe
    //
    return (EndpointAddress & USB_ENDPOINT_DIRECTION_MASK) == 0x00 ? TRUE : FALSE;
}

//
// WDF Function: WdfUsbTargetDeviceCreate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICECREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFUSBDEVICE* UsbDevice
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceCreate(
    __in
    WDFDEVICE Device,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFUSBDEVICE* UsbDevice
    )
{
    return ((PFN_WDFUSBTARGETDEVICECREATE) WdfFunctions[WdfUsbTargetDeviceCreateTableIndex])(WdfDriverGlobals, Device, Attributes, UsbDevice);
}

//
// WDF Function: WdfUsbTargetDeviceRetrieveInformation
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICERETRIEVEINFORMATION)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PWDF_USB_DEVICE_INFORMATION Information
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceRetrieveInformation(
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PWDF_USB_DEVICE_INFORMATION Information
    )
{
    return ((PFN_WDFUSBTARGETDEVICERETRIEVEINFORMATION) WdfFunctions[WdfUsbTargetDeviceRetrieveInformationTableIndex])(WdfDriverGlobals, UsbDevice, Information);
}

//
// WDF Function: WdfUsbTargetDeviceGetDeviceDescriptor
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFUSBTARGETDEVICEGETDEVICEDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfUsbTargetDeviceGetDeviceDescriptor(
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor
    )
{
    ((PFN_WDFUSBTARGETDEVICEGETDEVICEDESCRIPTOR) WdfFunctions[WdfUsbTargetDeviceGetDeviceDescriptorTableIndex])(WdfDriverGlobals, UsbDevice, UsbDeviceDescriptor);
}

//
// WDF Function: WdfUsbTargetDeviceRetrieveConfigDescriptor
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICERETRIEVECONFIGDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out_bcount_part_opt(*ConfigDescriptorLength,*ConfigDescriptorLength)
    PVOID ConfigDescriptor,
    __inout
    PUSHORT ConfigDescriptorLength
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceRetrieveConfigDescriptor(
    __in
    WDFUSBDEVICE UsbDevice,
    __out_bcount_part_opt(*ConfigDescriptorLength,*ConfigDescriptorLength)
    PVOID ConfigDescriptor,
    __inout
    PUSHORT ConfigDescriptorLength
    )
{
    return ((PFN_WDFUSBTARGETDEVICERETRIEVECONFIGDESCRIPTOR) WdfFunctions[WdfUsbTargetDeviceRetrieveConfigDescriptorTableIndex])(WdfDriverGlobals, UsbDevice, ConfigDescriptor, ConfigDescriptorLength);
}

//
// WDF Function: WdfUsbTargetDeviceQueryString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICEQUERYSTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_ecount_opt(*NumCharacters)
    PUSHORT String,
    __inout
    PUSHORT NumCharacters,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceQueryString(
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_ecount_opt(*NumCharacters)
    PUSHORT String,
    __inout
    PUSHORT NumCharacters,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    )
{
    return ((PFN_WDFUSBTARGETDEVICEQUERYSTRING) WdfFunctions[WdfUsbTargetDeviceQueryStringTableIndex])(WdfDriverGlobals, UsbDevice, Request, RequestOptions, String, NumCharacters, StringIndex, LangID);
}

//
// WDF Function: WdfUsbTargetDeviceAllocAndQueryString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICEALLOCANDQUERYSTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringMemoryAttributes,
    __out
    WDFMEMORY* StringMemory,
    __out_opt
    PUSHORT NumCharacters,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceAllocAndQueryString(
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringMemoryAttributes,
    __out
    WDFMEMORY* StringMemory,
    __out_opt
    PUSHORT NumCharacters,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    )
{
    return ((PFN_WDFUSBTARGETDEVICEALLOCANDQUERYSTRING) WdfFunctions[WdfUsbTargetDeviceAllocAndQueryStringTableIndex])(WdfDriverGlobals, UsbDevice, StringMemoryAttributes, StringMemory, NumCharacters, StringIndex, LangID);
}

//
// WDF Function: WdfUsbTargetDeviceFormatRequestForString
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORSTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY Memory,
    __in_opt
    PWDFMEMORY_OFFSET Offset,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceFormatRequestForString(
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY Memory,
    __in_opt
    PWDFMEMORY_OFFSET Offset,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    )
{
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORSTRING) WdfFunctions[WdfUsbTargetDeviceFormatRequestForStringTableIndex])(WdfDriverGlobals, UsbDevice, Request, Memory, Offset, StringIndex, LangID);
}

//
// WDF Function: WdfUsbTargetDeviceGetNumInterfaces
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
UCHAR
(*PFN_WDFUSBTARGETDEVICEGETNUMINTERFACES)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    );

__drv_maxIRQL(DISPATCH_LEVEL)
UCHAR
FORCEINLINE
WdfUsbTargetDeviceGetNumInterfaces(
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    return ((PFN_WDFUSBTARGETDEVICEGETNUMINTERFACES) WdfFunctions[WdfUsbTargetDeviceGetNumInterfacesTableIndex])(WdfDriverGlobals, UsbDevice);
}

//
// WDF Function: WdfUsbTargetDeviceSelectConfig
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICESELECTCONFIG)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PipeAttributes,
    __inout
    PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceSelectConfig(
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PipeAttributes,
    __inout
    PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
{
    return ((PFN_WDFUSBTARGETDEVICESELECTCONFIG) WdfFunctions[WdfUsbTargetDeviceSelectConfigTableIndex])(WdfDriverGlobals, UsbDevice, PipeAttributes, Params);
}

//
// WDF Function: WdfUsbTargetDeviceWdmGetConfigurationHandle
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
USBD_CONFIGURATION_HANDLE
(*PFN_WDFUSBTARGETDEVICEWDMGETCONFIGURATIONHANDLE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    );

__drv_maxIRQL(DISPATCH_LEVEL)
USBD_CONFIGURATION_HANDLE
FORCEINLINE
WdfUsbTargetDeviceWdmGetConfigurationHandle(
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    return ((PFN_WDFUSBTARGETDEVICEWDMGETCONFIGURATIONHANDLE) WdfFunctions[WdfUsbTargetDeviceWdmGetConfigurationHandleTableIndex])(WdfDriverGlobals, UsbDevice);
}

//
// WDF Function: WdfUsbTargetDeviceRetrieveCurrentFrameNumber
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICERETRIEVECURRENTFRAMENUMBER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PULONG CurrentFrameNumber
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceRetrieveCurrentFrameNumber(
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PULONG CurrentFrameNumber
    )
{
    return ((PFN_WDFUSBTARGETDEVICERETRIEVECURRENTFRAMENUMBER) WdfFunctions[WdfUsbTargetDeviceRetrieveCurrentFrameNumberTableIndex])(WdfDriverGlobals, UsbDevice, CurrentFrameNumber);
}

//
// WDF Function: WdfUsbTargetDeviceSendControlTransferSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICESENDCONTROLTRANSFERSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesTransferred
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceSendControlTransferSynchronously(
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesTransferred
    )
{
    return ((PFN_WDFUSBTARGETDEVICESENDCONTROLTRANSFERSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetDeviceSendControlTransferSynchronouslyTableIndex])(WdfDriverGlobals, UsbDevice, Request, RequestOptions, SetupPacket, MemoryDescriptor, BytesTransferred);
}

//
// WDF Function: WdfUsbTargetDeviceFormatRequestForControlTransfer
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORCONTROLTRANSFER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    __in_opt
    WDFMEMORY TransferMemory,
    __in_opt
    PWDFMEMORY_OFFSET TransferOffset
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceFormatRequestForControlTransfer(
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    __in_opt
    WDFMEMORY TransferMemory,
    __in_opt
    PWDFMEMORY_OFFSET TransferOffset
    )
{
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORCONTROLTRANSFER) WdfFunctions[WdfUsbTargetDeviceFormatRequestForControlTransferTableIndex])(WdfDriverGlobals, UsbDevice, Request, SetupPacket, TransferMemory, TransferOffset);
}

//
// WDF Function: WdfUsbTargetDeviceIsConnectedSynchronous
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICEISCONNECTEDSYNCHRONOUS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceIsConnectedSynchronous(
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    return ((PFN_WDFUSBTARGETDEVICEISCONNECTEDSYNCHRONOUS) WdfFunctions[WdfUsbTargetDeviceIsConnectedSynchronousTableIndex])(WdfDriverGlobals, UsbDevice);
}

//
// WDF Function: WdfUsbTargetDeviceResetPortSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICERESETPORTSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceResetPortSynchronously(
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    return ((PFN_WDFUSBTARGETDEVICERESETPORTSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetDeviceResetPortSynchronouslyTableIndex])(WdfDriverGlobals, UsbDevice);
}

//
// WDF Function: WdfUsbTargetDeviceCyclePortSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICECYCLEPORTSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceCyclePortSynchronously(
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    return ((PFN_WDFUSBTARGETDEVICECYCLEPORTSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetDeviceCyclePortSynchronouslyTableIndex])(WdfDriverGlobals, UsbDevice);
}

//
// WDF Function: WdfUsbTargetDeviceFormatRequestForCyclePort
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORCYCLEPORT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceFormatRequestForCyclePort(
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request
    )
{
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORCYCLEPORT) WdfFunctions[WdfUsbTargetDeviceFormatRequestForCyclePortTableIndex])(WdfDriverGlobals, UsbDevice, Request);
}

//
// WDF Function: WdfUsbTargetDeviceSendUrbSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICESENDURBSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_xcount("union bug in SAL")
    PURB Urb
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceSendUrbSynchronously(
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_xcount("union bug in SAL")
    PURB Urb
    )
{
    return ((PFN_WDFUSBTARGETDEVICESENDURBSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetDeviceSendUrbSynchronouslyTableIndex])(WdfDriverGlobals, UsbDevice, Request, RequestOptions, Urb);
}

//
// WDF Function: WdfUsbTargetDeviceFormatRequestForUrb
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORURB)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY UrbMemory,
    __in_opt
    PWDFMEMORY_OFFSET UrbMemoryOffset
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetDeviceFormatRequestForUrb(
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY UrbMemory,
    __in_opt
    PWDFMEMORY_OFFSET UrbMemoryOffset
    )
{
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORURB) WdfFunctions[WdfUsbTargetDeviceFormatRequestForUrbTableIndex])(WdfDriverGlobals, UsbDevice, Request, UrbMemory, UrbMemoryOffset);
}

//
// WDF Function: WdfUsbTargetPipeGetInformation
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFUSBTARGETPIPEGETINFORMATION)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __out
    PWDF_USB_PIPE_INFORMATION PipeInformation
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfUsbTargetPipeGetInformation(
    __in
    WDFUSBPIPE Pipe,
    __out
    PWDF_USB_PIPE_INFORMATION PipeInformation
    )
{
    ((PFN_WDFUSBTARGETPIPEGETINFORMATION) WdfFunctions[WdfUsbTargetPipeGetInformationTableIndex])(WdfDriverGlobals, Pipe, PipeInformation);
}

//
// WDF Function: WdfUsbTargetPipeIsInEndpoint
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFUSBTARGETPIPEISINENDPOINT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
FORCEINLINE
WdfUsbTargetPipeIsInEndpoint(
    __in
    WDFUSBPIPE Pipe
    )
{
    return ((PFN_WDFUSBTARGETPIPEISINENDPOINT) WdfFunctions[WdfUsbTargetPipeIsInEndpointTableIndex])(WdfDriverGlobals, Pipe);
}

//
// WDF Function: WdfUsbTargetPipeIsOutEndpoint
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFUSBTARGETPIPEISOUTENDPOINT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
FORCEINLINE
WdfUsbTargetPipeIsOutEndpoint(
    __in
    WDFUSBPIPE Pipe
    )
{
    return ((PFN_WDFUSBTARGETPIPEISOUTENDPOINT) WdfFunctions[WdfUsbTargetPipeIsOutEndpointTableIndex])(WdfDriverGlobals, Pipe);
}

//
// WDF Function: WdfUsbTargetPipeGetType
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDF_USB_PIPE_TYPE
(*PFN_WDFUSBTARGETPIPEGETTYPE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_USB_PIPE_TYPE
FORCEINLINE
WdfUsbTargetPipeGetType(
    __in
    WDFUSBPIPE Pipe
    )
{
    return ((PFN_WDFUSBTARGETPIPEGETTYPE) WdfFunctions[WdfUsbTargetPipeGetTypeTableIndex])(WdfDriverGlobals, Pipe);
}

//
// WDF Function: WdfUsbTargetPipeSetNoMaximumPacketSizeCheck
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFUSBTARGETPIPESETNOMAXIMUMPACKETSIZECHECK)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(
    __in
    WDFUSBPIPE Pipe
    )
{
    ((PFN_WDFUSBTARGETPIPESETNOMAXIMUMPACKETSIZECHECK) WdfFunctions[WdfUsbTargetPipeSetNoMaximumPacketSizeCheckTableIndex])(WdfDriverGlobals, Pipe);
}

//
// WDF Function: WdfUsbTargetPipeWriteSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEWRITESYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesWritten
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeWriteSynchronously(
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesWritten
    )
{
    return ((PFN_WDFUSBTARGETPIPEWRITESYNCHRONOUSLY) WdfFunctions[WdfUsbTargetPipeWriteSynchronouslyTableIndex])(WdfDriverGlobals, Pipe, Request, RequestOptions, MemoryDescriptor, BytesWritten);
}

//
// WDF Function: WdfUsbTargetPipeFormatRequestForWrite
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEFORMATREQUESTFORWRITE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY WriteMemory,
    __in_opt
    PWDFMEMORY_OFFSET WriteOffset
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeFormatRequestForWrite(
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY WriteMemory,
    __in_opt
    PWDFMEMORY_OFFSET WriteOffset
    )
{
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORWRITE) WdfFunctions[WdfUsbTargetPipeFormatRequestForWriteTableIndex])(WdfDriverGlobals, Pipe, Request, WriteMemory, WriteOffset);
}

//
// WDF Function: WdfUsbTargetPipeReadSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEREADSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesRead
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeReadSynchronously(
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesRead
    )
{
    return ((PFN_WDFUSBTARGETPIPEREADSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetPipeReadSynchronouslyTableIndex])(WdfDriverGlobals, Pipe, Request, RequestOptions, MemoryDescriptor, BytesRead);
}

//
// WDF Function: WdfUsbTargetPipeFormatRequestForRead
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEFORMATREQUESTFORREAD)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY ReadMemory,
    __in_opt
    PWDFMEMORY_OFFSET ReadOffset
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeFormatRequestForRead(
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY ReadMemory,
    __in_opt
    PWDFMEMORY_OFFSET ReadOffset
    )
{
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORREAD) WdfFunctions[WdfUsbTargetPipeFormatRequestForReadTableIndex])(WdfDriverGlobals, Pipe, Request, ReadMemory, ReadOffset);
}

//
// WDF Function: WdfUsbTargetPipeConfigContinuousReader
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPECONFIGCONTINUOUSREADER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    PWDF_USB_CONTINUOUS_READER_CONFIG Config
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeConfigContinuousReader(
    __in
    WDFUSBPIPE Pipe,
    __in
    PWDF_USB_CONTINUOUS_READER_CONFIG Config
    )
{
    return ((PFN_WDFUSBTARGETPIPECONFIGCONTINUOUSREADER) WdfFunctions[WdfUsbTargetPipeConfigContinuousReaderTableIndex])(WdfDriverGlobals, Pipe, Config);
}

//
// WDF Function: WdfUsbTargetPipeAbortSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEABORTSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeAbortSynchronously(
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    )
{
    return ((PFN_WDFUSBTARGETPIPEABORTSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetPipeAbortSynchronouslyTableIndex])(WdfDriverGlobals, Pipe, Request, RequestOptions);
}

//
// WDF Function: WdfUsbTargetPipeFormatRequestForAbort
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEFORMATREQUESTFORABORT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeFormatRequestForAbort(
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request
    )
{
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORABORT) WdfFunctions[WdfUsbTargetPipeFormatRequestForAbortTableIndex])(WdfDriverGlobals, Pipe, Request);
}

//
// WDF Function: WdfUsbTargetPipeResetSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPERESETSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeResetSynchronously(
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    )
{
    return ((PFN_WDFUSBTARGETPIPERESETSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetPipeResetSynchronouslyTableIndex])(WdfDriverGlobals, Pipe, Request, RequestOptions);
}

//
// WDF Function: WdfUsbTargetPipeFormatRequestForReset
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEFORMATREQUESTFORRESET)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeFormatRequestForReset(
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request
    )
{
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORRESET) WdfFunctions[WdfUsbTargetPipeFormatRequestForResetTableIndex])(WdfDriverGlobals, Pipe, Request);
}

//
// WDF Function: WdfUsbTargetPipeSendUrbSynchronously
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPESENDURBSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_xcount("union bug in SAL")
    PURB Urb
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeSendUrbSynchronously(
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_xcount("union bug in SAL")
    PURB Urb
    )
{
    return ((PFN_WDFUSBTARGETPIPESENDURBSYNCHRONOUSLY) WdfFunctions[WdfUsbTargetPipeSendUrbSynchronouslyTableIndex])(WdfDriverGlobals, Pipe, Request, RequestOptions, Urb);
}

//
// WDF Function: WdfUsbTargetPipeFormatRequestForUrb
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBTARGETPIPEFORMATREQUESTFORURB)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE PIPE,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY UrbMemory,
    __in_opt
    PWDFMEMORY_OFFSET UrbMemoryOffset
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbTargetPipeFormatRequestForUrb(
    __in
    WDFUSBPIPE PIPE,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY UrbMemory,
    __in_opt
    PWDFMEMORY_OFFSET UrbMemoryOffset
    )
{
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORURB) WdfFunctions[WdfUsbTargetPipeFormatRequestForUrbTableIndex])(WdfDriverGlobals, PIPE, Request, UrbMemory, UrbMemoryOffset);
}

//
// WDF Function: WdfUsbInterfaceGetInterfaceNumber
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BYTE
(*PFN_WDFUSBINTERFACEGETINTERFACENUMBER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
FORCEINLINE
WdfUsbInterfaceGetInterfaceNumber(
    __in
    WDFUSBINTERFACE UsbInterface
    )
{
    return ((PFN_WDFUSBINTERFACEGETINTERFACENUMBER) WdfFunctions[WdfUsbInterfaceGetInterfaceNumberTableIndex])(WdfDriverGlobals, UsbInterface);
}

//
// WDF Function: WdfUsbInterfaceGetNumEndpoints
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BYTE
(*PFN_WDFUSBINTERFACEGETNUMENDPOINTS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
FORCEINLINE
WdfUsbInterfaceGetNumEndpoints(
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex
    )
{
    return ((PFN_WDFUSBINTERFACEGETNUMENDPOINTS) WdfFunctions[WdfUsbInterfaceGetNumEndpointsTableIndex])(WdfDriverGlobals, UsbInterface, SettingIndex);
}

//
// WDF Function: WdfUsbInterfaceGetDescriptor
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFUSBINTERFACEGETDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex,
    __out
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfUsbInterfaceGetDescriptor(
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex,
    __out
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
    )
{
    ((PFN_WDFUSBINTERFACEGETDESCRIPTOR) WdfFunctions[WdfUsbInterfaceGetDescriptorTableIndex])(WdfDriverGlobals, UsbInterface, SettingIndex, InterfaceDescriptor);
}

//
// WDF Function: WdfUsbInterfaceGetNumSettings
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BYTE
(*PFN_WDFUSBINTERFACEGETNUMSETTINGS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
FORCEINLINE
WdfUsbInterfaceGetNumSettings(
    __in
    WDFUSBINTERFACE UsbInterface
    )
{
    return ((PFN_WDFUSBINTERFACEGETNUMSETTINGS) WdfFunctions[WdfUsbInterfaceGetNumSettingsTableIndex])(WdfDriverGlobals, UsbInterface);
}

//
// WDF Function: WdfUsbInterfaceSelectSetting
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFUSBINTERFACESELECTSETTING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in
    PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfUsbInterfaceSelectSetting(
    __in
    WDFUSBINTERFACE UsbInterface,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in
    PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params
    )
{
    return ((PFN_WDFUSBINTERFACESELECTSETTING) WdfFunctions[WdfUsbInterfaceSelectSettingTableIndex])(WdfDriverGlobals, UsbInterface, PipesAttributes, Params);
}

//
// WDF Function: WdfUsbInterfaceGetEndpointInformation
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFUSBINTERFACEGETENDPOINTINFORMATION)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex,
    __in
    UCHAR EndpointIndex,
    __out
    PWDF_USB_PIPE_INFORMATION EndpointInfo
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfUsbInterfaceGetEndpointInformation(
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex,
    __in
    UCHAR EndpointIndex,
    __out
    PWDF_USB_PIPE_INFORMATION EndpointInfo
    )
{
    ((PFN_WDFUSBINTERFACEGETENDPOINTINFORMATION) WdfFunctions[WdfUsbInterfaceGetEndpointInformationTableIndex])(WdfDriverGlobals, UsbInterface, SettingIndex, EndpointIndex, EndpointInfo);
}

//
// WDF Function: WdfUsbTargetDeviceGetInterface
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFUSBINTERFACE
(*PFN_WDFUSBTARGETDEVICEGETINTERFACE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    UCHAR InterfaceIndex
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFUSBINTERFACE
FORCEINLINE
WdfUsbTargetDeviceGetInterface(
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    UCHAR InterfaceIndex
    )
{
    return ((PFN_WDFUSBTARGETDEVICEGETINTERFACE) WdfFunctions[WdfUsbTargetDeviceGetInterfaceTableIndex])(WdfDriverGlobals, UsbDevice, InterfaceIndex);
}

//
// WDF Function: WdfUsbInterfaceGetConfiguredSettingIndex
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BYTE
(*PFN_WDFUSBINTERFACEGETCONFIGUREDSETTINGINDEX)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE Interface
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
FORCEINLINE
WdfUsbInterfaceGetConfiguredSettingIndex(
    __in
    WDFUSBINTERFACE Interface
    )
{
    return ((PFN_WDFUSBINTERFACEGETCONFIGUREDSETTINGINDEX) WdfFunctions[WdfUsbInterfaceGetConfiguredSettingIndexTableIndex])(WdfDriverGlobals, Interface);
}

//
// WDF Function: WdfUsbInterfaceGetNumConfiguredPipes
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BYTE
(*PFN_WDFUSBINTERFACEGETNUMCONFIGUREDPIPES)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
FORCEINLINE
WdfUsbInterfaceGetNumConfiguredPipes(
    __in
    WDFUSBINTERFACE UsbInterface
    )
{
    return ((PFN_WDFUSBINTERFACEGETNUMCONFIGUREDPIPES) WdfFunctions[WdfUsbInterfaceGetNumConfiguredPipesTableIndex])(WdfDriverGlobals, UsbInterface);
}

//
// WDF Function: WdfUsbInterfaceGetConfiguredPipe
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFUSBPIPE
(*PFN_WDFUSBINTERFACEGETCONFIGUREDPIPE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR PipeIndex,
    __out_opt
    PWDF_USB_PIPE_INFORMATION PipeInfo
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFUSBPIPE
FORCEINLINE
WdfUsbInterfaceGetConfiguredPipe(
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR PipeIndex,
    __out_opt
    PWDF_USB_PIPE_INFORMATION PipeInfo
    )
{
    return ((PFN_WDFUSBINTERFACEGETCONFIGUREDPIPE) WdfFunctions[WdfUsbInterfaceGetConfiguredPipeTableIndex])(WdfDriverGlobals, UsbInterface, PipeIndex, PipeInfo);
}

//
// WDF Function: WdfUsbTargetPipeWdmGetPipeHandle
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
USBD_PIPE_HANDLE
(*PFN_WDFUSBTARGETPIPEWDMGETPIPEHANDLE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE UsbPipe
    );

__drv_maxIRQL(DISPATCH_LEVEL)
USBD_PIPE_HANDLE
FORCEINLINE
WdfUsbTargetPipeWdmGetPipeHandle(
    __in
    WDFUSBPIPE UsbPipe
    )
{
    return ((PFN_WDFUSBTARGETPIPEWDMGETPIPEHANDLE) WdfFunctions[WdfUsbTargetPipeWdmGetPipeHandleTableIndex])(WdfDriverGlobals, UsbPipe);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFUSB_H_


#pragma once

#ifndef DECLSPEC_EXPORT
#define DECLSPEC_EXPORT __declspec(dllexport)
#endif

typedef struct _USBD_INTERFACE_LIST_ENTRY {
  PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
  PUSBD_INTERFACE_INFORMATION Interface;
} USBD_INTERFACE_LIST_ENTRY, *PUSBD_INTERFACE_LIST_ENTRY;

#define UsbBuildInterruptOrBulkTransferRequest(urb,length, pipeHandle, transferBuffer, transferBufferMDL, transferBufferLength, transferFlags, link) { \
  (urb)->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;                                                                                 \
  (urb)->UrbHeader.Length = (length);                                                                                                                  \
  (urb)->UrbBulkOrInterruptTransfer.PipeHandle = (pipeHandle);                                                                                         \
  (urb)->UrbBulkOrInterruptTransfer.TransferBufferLength = (transferBufferLength);                                                                     \
  (urb)->UrbBulkOrInterruptTransfer.TransferBufferMDL = (transferBufferMDL);                                                                           \
  (urb)->UrbBulkOrInterruptTransfer.TransferBuffer = (transferBuffer);                                                                                 \
  (urb)->UrbBulkOrInterruptTransfer.TransferFlags = (transferFlags);                                                                                   \
  (urb)->UrbBulkOrInterruptTransfer.UrbLink = (link);                                                                                                  \
}

#define UsbBuildGetDescriptorRequest(urb, length, descriptorType, descriptorIndex, languageId, transferBuffer, transferBufferMDL, transferBufferLength, link) { \
  (urb)->UrbHeader.Function =  URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;                                                                                         \
  (urb)->UrbHeader.Length = (length);                                                                                                                           \
  (urb)->UrbControlDescriptorRequest.TransferBufferLength = (transferBufferLength);                                                                             \
  (urb)->UrbControlDescriptorRequest.TransferBufferMDL = (transferBufferMDL);                                                                                   \
  (urb)->UrbControlDescriptorRequest.TransferBuffer = (transferBuffer);                                                                                         \
  (urb)->UrbControlDescriptorRequest.DescriptorType = (descriptorType);                                                                                         \
  (urb)->UrbControlDescriptorRequest.Index = (descriptorIndex);                                                                                                 \
  (urb)->UrbControlDescriptorRequest.LanguageId = (languageId);                                                                                                 \
  (urb)->UrbControlDescriptorRequest.UrbLink = (link);                                                                                                          \
}

#define UsbBuildGetStatusRequest(urb, op, index, transferBuffer, transferBufferMDL, link) { \
  (urb)->UrbHeader.Function =  (op);                                                        \
  (urb)->UrbHeader.Length = sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST);                 \
  (urb)->UrbControlGetStatusRequest.TransferBufferLength = sizeof(USHORT);                  \
  (urb)->UrbControlGetStatusRequest.TransferBufferMDL = (transferBufferMDL);                \
  (urb)->UrbControlGetStatusRequest.TransferBuffer = (transferBuffer);                      \
  (urb)->UrbControlGetStatusRequest.Index = (index);                                        \
  (urb)->UrbControlGetStatusRequest.UrbLink = (link);                                       \
}

#define UsbBuildFeatureRequest(urb, op, featureSelector, index, link) {  \
  (urb)->UrbHeader.Function =  (op);                                     \
  (urb)->UrbHeader.Length = sizeof(struct _URB_CONTROL_FEATURE_REQUEST); \
  (urb)->UrbControlFeatureRequest.FeatureSelector = (featureSelector);   \
  (urb)->UrbControlFeatureRequest.Index = (index);                       \
  (urb)->UrbControlFeatureRequest.UrbLink = (link);                      \
}

#define UsbBuildSelectConfigurationRequest(urb, length, configurationDescriptor) {   \
  (urb)->UrbHeader.Function =  URB_FUNCTION_SELECT_CONFIGURATION;                    \
  (urb)->UrbHeader.Length = (length);                                                \
  (urb)->UrbSelectConfiguration.ConfigurationDescriptor = (configurationDescriptor); \
}

#define UsbBuildSelectInterfaceRequest(urb, length, configurationHandle, interfaceNumber, alternateSetting) {             \
  (urb)->UrbHeader.Function =  URB_FUNCTION_SELECT_INTERFACE;                                                             \
  (urb)->UrbHeader.Length = (length);                                                                                     \
  (urb)->UrbSelectInterface.Interface.AlternateSetting = (alternateSetting);                                              \
  (urb)->UrbSelectInterface.Interface.InterfaceNumber = (interfaceNumber);                                                \
  (urb)->UrbSelectInterface.Interface.Length = (length - sizeof(struct _URB_HEADER) - sizeof(USBD_CONFIGURATION_HANDLE)); \
  (urb)->UrbSelectInterface.ConfigurationHandle = (configurationHandle);                                                  \
}

#define UsbBuildVendorRequest(urb, cmd, length, transferFlags, reservedbits, request, value, index, transferBuffer, transferBufferMDL, transferBufferLength, link) { \
  (urb)->UrbHeader.Function =  cmd;                                                                                                                                  \
  (urb)->UrbHeader.Length = (length);                                                                                                                                \
  (urb)->UrbControlVendorClassRequest.TransferBufferLength = (transferBufferLength);                                                                                 \
  (urb)->UrbControlVendorClassRequest.TransferBufferMDL = (transferBufferMDL);                                                                                       \
  (urb)->UrbControlVendorClassRequest.TransferBuffer = (transferBuffer);                                                                                             \
  (urb)->UrbControlVendorClassRequest.RequestTypeReservedBits = (reservedbits);                                                                                      \
  (urb)->UrbControlVendorClassRequest.Request = (request);                                                                                                           \
  (urb)->UrbControlVendorClassRequest.Value = (value);                                                                                                               \
  (urb)->UrbControlVendorClassRequest.Index = (index);                                                                                                               \
  (urb)->UrbControlVendorClassRequest.TransferFlags = (transferFlags);                                                                                               \
  (urb)->UrbControlVendorClassRequest.UrbLink = (link);                                                                                                              \
}

#if (NTDDI_VERSION >= NTDDI_WINXP)

#define UsbBuildOsFeatureDescriptorRequest(urb, length, interface, index, transferBuffer, transferBufferMDL, transferBufferLength, link) { \
  (urb)->UrbHeader.Function = URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR;                                                                      \
  (urb)->UrbHeader.Length = (length);                                                                                                      \
  (urb)->UrbOSFeatureDescriptorRequest.TransferBufferLength = (transferBufferLength);                                                      \
  (urb)->UrbOSFeatureDescriptorRequest.TransferBufferMDL = (transferBufferMDL);                                                            \
  (urb)->UrbOSFeatureDescriptorRequest.TransferBuffer = (transferBuffer);                                                                  \
  (urb)->UrbOSFeatureDescriptorRequest.InterfaceNumber = (interface);                                                                      \
  (urb)->UrbOSFeatureDescriptorRequest.MS_FeatureDescriptorIndex = (index);                                                                \
  (urb)->UrbOSFeatureDescriptorRequest.UrbLink = (link);                                                                                    \
}

#endif /* NTDDI_VERSION >= NTDDI_WINXP */

#if (NTDDI_VERSION >= NTDDI_VISTA)

#define USBD_CLIENT_CONTRACT_VERSION_INVALID 0xFFFFFFFF
#define USBD_CLIENT_CONTRACT_VERSION_602 0x602

#define USBD_INTERFACE_VERSION_600 0x600
#define USBD_INTERFACE_VERSION_602 0x602
#define USBD_INTERFACE_VERSION_603 0x603

DECLARE_HANDLE(USBD_HANDLE);

DECLARE_HANDLE(USBDI_HANDLE);

DECLARE_HANDLE(USBD_FUNCTION_HANDLE);
typedef USBD_FUNCTION_HANDLE *PUSBD_FUNCTION_HANDLE;

#endif // NTDDI_VISTA

#define URB_STATUS(urb)                      ((urb)->UrbHeader.Status)

#define GET_SELECT_CONFIGURATION_REQUEST_SIZE(totalInterfaces, totalPipes) \
  (sizeof(struct _URB_SELECT_CONFIGURATION) +                              \
  ((totalInterfaces-1) * sizeof(USBD_INTERFACE_INFORMATION)) +             \
  ((totalPipes-totalInterfaces)*sizeof(USBD_PIPE_INFORMATION)))

#define GET_SELECT_INTERFACE_REQUEST_SIZE(totalPipes) \
  (sizeof(struct _URB_SELECT_INTERFACE) +             \
  ((totalPipes-1)*sizeof(USBD_PIPE_INFORMATION)))

#define GET_USBD_INTERFACE_SIZE(numEndpoints)                                 \
  (sizeof(USBD_INTERFACE_INFORMATION) +                                       \
  (sizeof(USBD_PIPE_INFORMATION)*(numEndpoints)) - sizeof(USBD_PIPE_INFORMATION))

#define  GET_ISO_URB_SIZE(n) (sizeof(struct _URB_ISOCH_TRANSFER)+ \
  sizeof(USBD_ISO_PACKET_DESCRIPTOR)*n)

#ifndef _USBD_

_IRQL_requires_max_(DISPATCH_LEVEL)
DECLSPEC_IMPORT
VOID
NTAPI
USBD_GetUSBDIVersion(
  _Out_ PUSBD_VERSION_INFORMATION VersionInformation);

DECLSPEC_IMPORT
PUSB_INTERFACE_DESCRIPTOR
NTAPI
USBD_ParseConfigurationDescriptor(
  _In_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
  _In_ UCHAR InterfaceNumber,
  _In_ UCHAR AlternateSetting);

DECLSPEC_IMPORT
PURB
NTAPI
USBD_CreateConfigurationRequest(
  _In_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
  _Out_ PUSHORT Siz);

_IRQL_requires_max_(APC_LEVEL)
DECLSPEC_IMPORT
PUSB_COMMON_DESCRIPTOR
NTAPI
USBD_ParseDescriptors(
  _In_ PVOID DescriptorBuffer,
  _In_ ULONG TotalLength,
  _In_ PVOID StartPosition,
  _In_ LONG DescriptorType);

_IRQL_requires_max_(APC_LEVEL)
DECLSPEC_IMPORT
PUSB_INTERFACE_DESCRIPTOR
NTAPI
USBD_ParseConfigurationDescriptorEx(
  _In_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
  _In_ PVOID StartPosition,
  _In_ LONG InterfaceNumber,
  _In_ LONG AlternateSetting,
  _In_ LONG InterfaceClass,
  _In_ LONG InterfaceSubClass,
  _In_ LONG InterfaceProtocol);

_IRQL_requires_max_(DISPATCH_LEVEL)
DECLSPEC_IMPORT
PURB
NTAPI
USBD_CreateConfigurationRequestEx(
  _In_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
  _In_ PUSBD_INTERFACE_LIST_ENTRY InterfaceList);

_IRQL_requires_max_(PASSIVE_LEVEL)
DECLSPEC_EXPORT
ULONG
NTAPI
USBD_GetInterfaceLength(
  _In_ PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
  _In_ PUCHAR BufferEnd);

_IRQL_requires_max_(PASSIVE_LEVEL)
DECLSPEC_EXPORT
VOID
NTAPI
USBD_RegisterHcFilter(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PDEVICE_OBJECT FilterDeviceObject);

_IRQL_requires_max_(APC_LEVEL)
DECLSPEC_EXPORT
NTSTATUS
NTAPI
USBD_GetPdoRegistryParameter(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject,
  _Inout_updates_bytes_(ParameterLength) PVOID Parameter,
  _In_ ULONG ParameterLength,
  _In_reads_bytes_(KeyNameLength) PWSTR KeyName,
  _In_ ULONG KeyNameLength);

DECLSPEC_EXPORT
NTSTATUS
NTAPI
USBD_QueryBusTime(
  _In_ PDEVICE_OBJECT RootHubPdo,
  _Out_ PULONG CurrentFrame);

#if (NTDDI_VERSION >= NTDDI_WINXP)

_IRQL_requires_max_(DISPATCH_LEVEL)
DECLSPEC_IMPORT
ULONG
NTAPI
USBD_CalculateUsbBandwidth(
  _In_ ULONG MaxPacketSize,
  _In_ UCHAR EndpointType,
  _In_ BOOLEAN LowSpeed);

#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_(DISPATCH_LEVEL)
DECLSPEC_IMPORT
USBD_STATUS
NTAPI
USBD_ValidateConfigurationDescriptor(
  _In_reads_bytes_(BufferLength) PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
  _In_ ULONG BufferLength,
  _In_ USHORT Level,
  _Out_ PUCHAR *Offset,
  _In_opt_ ULONG Tag);

/* UsbdEx lib */

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
USBD_QueryUsbCapability(
    _In_ USBD_HANDLE USBDHandle,
    _In_ const GUID* CapabilityType,
    _In_ ULONG       OutputBufferLength,
    _When_(OutputBufferLength == 0, _Pre_null_)
    _When_(OutputBufferLength != 0 && ResultLength == NULL, _Out_writes_bytes_(OutputBufferLength))
    _When_(OutputBufferLength != 0 && ResultLength != NULL, _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultLength))
        PUCHAR                        OutputBuffer,
    _Out_opt_ 
    _When_(ResultLength != NULL, _Deref_out_range_(<=,OutputBufferLength))
        PULONG                        ResultLength
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
USBD_AssignUrbToIoStackLocation(
    _In_ USBD_HANDLE USBDHandle,
    _In_ PIO_STACK_LOCATION IoStackLocation,
    _In_ PURB Urb
);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
USBD_CreateHandle(
    _In_      PDEVICE_OBJECT DeviceObject,
    _In_      PDEVICE_OBJECT TargetDeviceObject,
    _In_      ULONG          USBDClientContractVersion,
    _In_      ULONG          PoolTag,
    _Out_     USBD_HANDLE   *USBDHandle
);

VOID
USBD_CloseHandle(
    _In_ USBD_HANDLE USBDHandle
);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
USBD_UrbAllocate(
    _In_ USBD_HANDLE USBDHandle,
    _Outptr_result_bytebuffer_(sizeof(URB)) PURB *Urb
);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
USBD_IsochUrbAllocate(
    _In_ USBD_HANDLE USBDHandle,
    _In_ ULONG NumberOfIsochPacket,
    _Outptr_result_bytebuffer_(sizeof(struct _URB_ISOCH_TRANSFER) 
                               + (NumberOfIsochPackets * sizeof(USBD_ISO_PACKET_DESCRIPTOR))
                               - sizeof(USBD_ISO_PACKET_DESCRIPTOR)) 
    PURB *Urb
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
USBD_UrbFree(
    _In_ USBD_HANDLE USBDHandle,
    _In_ PURB Urb
);

#endif

/* Queried with IOCTL_INTERNAL_USB_QUERY_USB_CAPABILITY */
DEFINE_GUID(GUID_USB_CAPABILITY_CHAINED_MDLS,
    0xf5ceeb23, 0xad90, 0x458c, 0x97, 0x9a, 0xd5, 0x9b, 0x3a, 0xd6, 0x88, 0x4f);
DEFINE_GUID(GUID_USB_CAPABILITY_STATIC_STREAMS,
    0x09051e1f, 0x0dc9, 0x4e6b, 0x8b, 0x12, 0x96, 0x0c, 0xbd, 0x81, 0xaa, 0x8f);
DEFINE_GUID(GUID_USB_CAPABILITY_SELECTIVE_SUSPEND,
    0x755c630d, 0xc8a0, 0x4765, 0x94, 0x6f, 0x90, 0xcc, 0x70, 0x38, 0x56, 0xa4);
DEFINE_GUID(GUID_USB_CAPABILITY_FUNCTION_SUSPEND,
    0xf4563183, 0xd66e, 0x42bd, 0xbd, 0x53, 0x1a, 0xc7, 0xb0, 0x4c, 0xd5, 0x9d);
DEFINE_GUID(GUID_USB_CAPABILITY_HIGH_BANDWIDTH_ISOCH,
    0x9c8a8a27, 0x15f9, 0x42e3, 0xa3, 0x56, 0xcd, 0xa6, 0xae, 0x97, 0xa8, 0xc8);
DEFINE_GUID(GUID_USB_CAPABILITY_DEVICE_CONNECTION_HIGH_SPEED_COMPATIBLE,
    0x81a885a6, 0xf239, 0x42a2, 0x83, 0x97, 0xa5, 0xd9, 0x5b, 0xea, 0x69, 0x53);
DEFINE_GUID(GUID_USB_CAPABILITY_DEVICE_CONNECTION_SUPER_SPEED_COMPATIBLE,
    0x8a2f776c, 0x9bd0, 0x4f29, 0x97, 0x71, 0xc7, 0xa3, 0x88, 0x46, 0xd4, 0xc7);
DEFINE_GUID(GUID_USB_CAPABILITY_TIME_SYNC,
    0xbb6e6472, 0x4be5, 0x44a3, 0x96, 0x41, 0x34, 0x5e, 0x56, 0xac, 0x34, 0x85);
DEFINE_GUID(GUID_USB_CAPABILITY_SSP_ISOCH_PIPE_FLAGS,
    0x59819125, 0x33a3, 0x421a, 0xa5, 0xdd, 0x37, 0x63, 0x9f, 0x30, 0xb9, 0x3e);
DEFINE_GUID(GUID_USB_CAPABILITY_CONTROLLER_EUSB2_DOUBLE_ISOCH_COMPATIBLE,
    0x5f0e4b3f, 0xde94, 0x473a, 0xa3, 0x73, 0x78, 0x1a, 0xcb, 0x92, 0xb5, 0xe8);

#endif /* ! _USBD_ */

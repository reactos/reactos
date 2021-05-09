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

#endif

#endif /* ! _USBD_ */

//
// Copyright (c) Microsoft. All rights reserved.
//
#ifndef _WDF_V2_0_TYPES_H_
#define _WDF_V2_0_TYPES_H_


typedef enum _WDFFUNCENUM_V2_0 {
    WdfFunctionTableNumEntries_V2_0 = 248,
} WDFFUNCENUM_V2_0;

typedef struct _WDF_POWER_ROUTINE_TIMED_OUT_DATA_V2_0 *PWDF_POWER_ROUTINE_TIMED_OUT_DATA_V2_0;
typedef const struct _WDF_POWER_ROUTINE_TIMED_OUT_DATA_V2_0 *PCWDF_POWER_ROUTINE_TIMED_OUT_DATA_V2_0;
typedef struct _WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V2_0 *PWDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V2_0;
typedef const struct _WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V2_0 *PCWDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V2_0;
typedef struct _WDF_QUEUE_FATAL_ERROR_DATA_V2_0 *PWDF_QUEUE_FATAL_ERROR_DATA_V2_0;
typedef const struct _WDF_QUEUE_FATAL_ERROR_DATA_V2_0 *PCWDF_QUEUE_FATAL_ERROR_DATA_V2_0;
typedef struct _WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0 *PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0;
typedef const struct _WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0 *PCWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0;
typedef struct _WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0 *PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0;
typedef const struct _WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0 *PCWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0;
typedef struct _WDF_CHILD_RETRIEVE_INFO_V2_0 *PWDF_CHILD_RETRIEVE_INFO_V2_0;
typedef const struct _WDF_CHILD_RETRIEVE_INFO_V2_0 *PCWDF_CHILD_RETRIEVE_INFO_V2_0;
typedef struct _WDF_CHILD_LIST_CONFIG_V2_0 *PWDF_CHILD_LIST_CONFIG_V2_0;
typedef const struct _WDF_CHILD_LIST_CONFIG_V2_0 *PCWDF_CHILD_LIST_CONFIG_V2_0;
typedef struct _WDF_CHILD_LIST_ITERATOR_V2_0 *PWDF_CHILD_LIST_ITERATOR_V2_0;
typedef const struct _WDF_CHILD_LIST_ITERATOR_V2_0 *PCWDF_CHILD_LIST_ITERATOR_V2_0;
typedef struct _WDF_COMMON_BUFFER_CONFIG_V2_0 *PWDF_COMMON_BUFFER_CONFIG_V2_0;
typedef const struct _WDF_COMMON_BUFFER_CONFIG_V2_0 *PCWDF_COMMON_BUFFER_CONFIG_V2_0;
typedef struct _WDFCX_FILEOBJECT_CONFIG_V2_0 *PWDFCX_FILEOBJECT_CONFIG_V2_0;
typedef const struct _WDFCX_FILEOBJECT_CONFIG_V2_0 *PCWDFCX_FILEOBJECT_CONFIG_V2_0;
typedef struct _WDF_CLASS_EXTENSION_DESCRIPTOR_V2_0 *PWDF_CLASS_EXTENSION_DESCRIPTOR_V2_0;
typedef const struct _WDF_CLASS_EXTENSION_DESCRIPTOR_V2_0 *PCWDF_CLASS_EXTENSION_DESCRIPTOR_V2_0;
typedef struct _WDF_CLASS_VERSION_V2_0 *PWDF_CLASS_VERSION_V2_0;
typedef const struct _WDF_CLASS_VERSION_V2_0 *PCWDF_CLASS_VERSION_V2_0;
typedef struct _WDF_CLASS_BIND_INFO_V2_0 *PWDF_CLASS_BIND_INFO_V2_0;
typedef const struct _WDF_CLASS_BIND_INFO_V2_0 *PCWDF_CLASS_BIND_INFO_V2_0;
typedef struct _WDF_CLASS_LIBRARY_INFO_V2_0 *PWDF_CLASS_LIBRARY_INFO_V2_0;
typedef const struct _WDF_CLASS_LIBRARY_INFO_V2_0 *PCWDF_CLASS_LIBRARY_INFO_V2_0;
typedef struct _WDF_FILEOBJECT_CONFIG_V2_0 *PWDF_FILEOBJECT_CONFIG_V2_0;
typedef const struct _WDF_FILEOBJECT_CONFIG_V2_0 *PCWDF_FILEOBJECT_CONFIG_V2_0;
typedef struct _WDF_DEVICE_PNP_NOTIFICATION_DATA_V2_0 *PWDF_DEVICE_PNP_NOTIFICATION_DATA_V2_0;
typedef const struct _WDF_DEVICE_PNP_NOTIFICATION_DATA_V2_0 *PCWDF_DEVICE_PNP_NOTIFICATION_DATA_V2_0;
typedef struct _WDF_DEVICE_POWER_NOTIFICATION_DATA_V2_0 *PWDF_DEVICE_POWER_NOTIFICATION_DATA_V2_0;
typedef const struct _WDF_DEVICE_POWER_NOTIFICATION_DATA_V2_0 *PCWDF_DEVICE_POWER_NOTIFICATION_DATA_V2_0;
typedef struct _WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V2_0 *PWDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V2_0;
typedef const struct _WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V2_0 *PCWDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V2_0;
typedef struct _WDF_PNPPOWER_EVENT_CALLBACKS_V2_0 *PWDF_PNPPOWER_EVENT_CALLBACKS_V2_0;
typedef const struct _WDF_PNPPOWER_EVENT_CALLBACKS_V2_0 *PCWDF_PNPPOWER_EVENT_CALLBACKS_V2_0;
typedef struct _WDF_POWER_POLICY_EVENT_CALLBACKS_V2_0 *PWDF_POWER_POLICY_EVENT_CALLBACKS_V2_0;
typedef const struct _WDF_POWER_POLICY_EVENT_CALLBACKS_V2_0 *PCWDF_POWER_POLICY_EVENT_CALLBACKS_V2_0;
typedef struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V2_0 *PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V2_0;
typedef const struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V2_0 *PCWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V2_0;
typedef struct _WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V2_0 *PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V2_0;
typedef const struct _WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V2_0 *PCWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V2_0;
typedef struct _WDF_DEVICE_STATE_V2_0 *PWDF_DEVICE_STATE_V2_0;
typedef const struct _WDF_DEVICE_STATE_V2_0 *PCWDF_DEVICE_STATE_V2_0;
typedef struct _WDF_DEVICE_PNP_CAPABILITIES_V2_0 *PWDF_DEVICE_PNP_CAPABILITIES_V2_0;
typedef const struct _WDF_DEVICE_PNP_CAPABILITIES_V2_0 *PCWDF_DEVICE_PNP_CAPABILITIES_V2_0;
typedef struct _WDF_DEVICE_POWER_CAPABILITIES_V2_0 *PWDF_DEVICE_POWER_CAPABILITIES_V2_0;
typedef const struct _WDF_DEVICE_POWER_CAPABILITIES_V2_0 *PCWDF_DEVICE_POWER_CAPABILITIES_V2_0;
typedef struct _WDF_REMOVE_LOCK_OPTIONS_V2_0 *PWDF_REMOVE_LOCK_OPTIONS_V2_0;
typedef const struct _WDF_REMOVE_LOCK_OPTIONS_V2_0 *PCWDF_REMOVE_LOCK_OPTIONS_V2_0;
typedef struct _WDF_POWER_FRAMEWORK_SETTINGS_V2_0 *PWDF_POWER_FRAMEWORK_SETTINGS_V2_0;
typedef const struct _WDF_POWER_FRAMEWORK_SETTINGS_V2_0 *PCWDF_POWER_FRAMEWORK_SETTINGS_V2_0;
typedef struct _WDF_IO_TYPE_CONFIG_V2_0 *PWDF_IO_TYPE_CONFIG_V2_0;
typedef const struct _WDF_IO_TYPE_CONFIG_V2_0 *PCWDF_IO_TYPE_CONFIG_V2_0;
typedef struct _WDF_DEVICE_INTERFACE_PROPERTY_DATA_V2_0 *PWDF_DEVICE_INTERFACE_PROPERTY_DATA_V2_0;
typedef const struct _WDF_DEVICE_INTERFACE_PROPERTY_DATA_V2_0 *PCWDF_DEVICE_INTERFACE_PROPERTY_DATA_V2_0;
typedef struct _WDF_DEVICE_PROPERTY_DATA_V2_0 *PWDF_DEVICE_PROPERTY_DATA_V2_0;
typedef const struct _WDF_DEVICE_PROPERTY_DATA_V2_0 *PCWDF_DEVICE_PROPERTY_DATA_V2_0;
typedef struct _WDF_DMA_ENABLER_CONFIG_V2_0 *PWDF_DMA_ENABLER_CONFIG_V2_0;
typedef const struct _WDF_DMA_ENABLER_CONFIG_V2_0 *PCWDF_DMA_ENABLER_CONFIG_V2_0;
typedef struct _WDF_DMA_SYSTEM_PROFILE_CONFIG_V2_0 *PWDF_DMA_SYSTEM_PROFILE_CONFIG_V2_0;
typedef const struct _WDF_DMA_SYSTEM_PROFILE_CONFIG_V2_0 *PCWDF_DMA_SYSTEM_PROFILE_CONFIG_V2_0;
typedef struct _WDF_DPC_CONFIG_V2_0 *PWDF_DPC_CONFIG_V2_0;
typedef const struct _WDF_DPC_CONFIG_V2_0 *PCWDF_DPC_CONFIG_V2_0;
typedef struct _WDF_DRIVER_CONFIG_V2_0 *PWDF_DRIVER_CONFIG_V2_0;
typedef const struct _WDF_DRIVER_CONFIG_V2_0 *PCWDF_DRIVER_CONFIG_V2_0;
typedef struct _WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V2_0 *PWDF_DRIVER_VERSION_AVAILABLE_PARAMS_V2_0;
typedef const struct _WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V2_0 *PCWDF_DRIVER_VERSION_AVAILABLE_PARAMS_V2_0;
typedef struct _WDF_FDO_EVENT_CALLBACKS_V2_0 *PWDF_FDO_EVENT_CALLBACKS_V2_0;
typedef const struct _WDF_FDO_EVENT_CALLBACKS_V2_0 *PCWDF_FDO_EVENT_CALLBACKS_V2_0;
typedef struct _WDF_DRIVER_GLOBALS_V2_0 *PWDF_DRIVER_GLOBALS_V2_0;
typedef const struct _WDF_DRIVER_GLOBALS_V2_0 *PCWDF_DRIVER_GLOBALS_V2_0;
typedef struct _WDF_INTERRUPT_CONFIG_V2_0 *PWDF_INTERRUPT_CONFIG_V2_0;
typedef const struct _WDF_INTERRUPT_CONFIG_V2_0 *PCWDF_INTERRUPT_CONFIG_V2_0;
typedef struct _WDF_INTERRUPT_INFO_V2_0 *PWDF_INTERRUPT_INFO_V2_0;
typedef const struct _WDF_INTERRUPT_INFO_V2_0 *PCWDF_INTERRUPT_INFO_V2_0;
typedef struct _WDF_INTERRUPT_EXTENDED_POLICY_V2_0 *PWDF_INTERRUPT_EXTENDED_POLICY_V2_0;
typedef const struct _WDF_INTERRUPT_EXTENDED_POLICY_V2_0 *PCWDF_INTERRUPT_EXTENDED_POLICY_V2_0;
typedef struct _WDF_IO_QUEUE_CONFIG_V2_0 *PWDF_IO_QUEUE_CONFIG_V2_0;
typedef const struct _WDF_IO_QUEUE_CONFIG_V2_0 *PCWDF_IO_QUEUE_CONFIG_V2_0;
typedef struct _WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_V2_0 *PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_V2_0;
typedef const struct _WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_V2_0 *PCWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_V2_0;
typedef struct _WDF_IO_TARGET_OPEN_PARAMS_V2_0 *PWDF_IO_TARGET_OPEN_PARAMS_V2_0;
typedef const struct _WDF_IO_TARGET_OPEN_PARAMS_V2_0 *PCWDF_IO_TARGET_OPEN_PARAMS_V2_0;
typedef struct _WDFMEMORY_OFFSET_V2_0 *PWDFMEMORY_OFFSET_V2_0;
typedef const struct _WDFMEMORY_OFFSET_V2_0 *PCWDFMEMORY_OFFSET_V2_0;
typedef struct _WDF_MEMORY_DESCRIPTOR_V2_0 *PWDF_MEMORY_DESCRIPTOR_V2_0;
typedef const struct _WDF_MEMORY_DESCRIPTOR_V2_0 *PCWDF_MEMORY_DESCRIPTOR_V2_0;
typedef struct _WDF_OBJECT_ATTRIBUTES_V2_0 *PWDF_OBJECT_ATTRIBUTES_V2_0;
typedef const struct _WDF_OBJECT_ATTRIBUTES_V2_0 *PCWDF_OBJECT_ATTRIBUTES_V2_0;
typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V2_0 *PWDF_OBJECT_CONTEXT_TYPE_INFO_V2_0;
typedef const struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V2_0 *PCWDF_OBJECT_CONTEXT_TYPE_INFO_V2_0;
typedef struct _WDF_CUSTOM_TYPE_CONTEXT_V2_0 *PWDF_CUSTOM_TYPE_CONTEXT_V2_0;
typedef const struct _WDF_CUSTOM_TYPE_CONTEXT_V2_0 *PCWDF_CUSTOM_TYPE_CONTEXT_V2_0;
typedef struct _WDF_PDO_EVENT_CALLBACKS_V2_0 *PWDF_PDO_EVENT_CALLBACKS_V2_0;
typedef const struct _WDF_PDO_EVENT_CALLBACKS_V2_0 *PCWDF_PDO_EVENT_CALLBACKS_V2_0;
typedef struct _WDF_QUERY_INTERFACE_CONFIG_V2_0 *PWDF_QUERY_INTERFACE_CONFIG_V2_0;
typedef const struct _WDF_QUERY_INTERFACE_CONFIG_V2_0 *PCWDF_QUERY_INTERFACE_CONFIG_V2_0;
typedef struct _WDF_REQUEST_PARAMETERS_V2_0 *PWDF_REQUEST_PARAMETERS_V2_0;
typedef const struct _WDF_REQUEST_PARAMETERS_V2_0 *PCWDF_REQUEST_PARAMETERS_V2_0;
typedef struct _WDF_REQUEST_COMPLETION_PARAMS_V2_0 *PWDF_REQUEST_COMPLETION_PARAMS_V2_0;
typedef const struct _WDF_REQUEST_COMPLETION_PARAMS_V2_0 *PCWDF_REQUEST_COMPLETION_PARAMS_V2_0;
typedef struct _WDF_REQUEST_REUSE_PARAMS_V2_0 *PWDF_REQUEST_REUSE_PARAMS_V2_0;
typedef const struct _WDF_REQUEST_REUSE_PARAMS_V2_0 *PCWDF_REQUEST_REUSE_PARAMS_V2_0;
typedef struct _WDF_REQUEST_SEND_OPTIONS_V2_0 *PWDF_REQUEST_SEND_OPTIONS_V2_0;
typedef const struct _WDF_REQUEST_SEND_OPTIONS_V2_0 *PCWDF_REQUEST_SEND_OPTIONS_V2_0;
typedef struct _WDF_REQUEST_FORWARD_OPTIONS_V2_0 *PWDF_REQUEST_FORWARD_OPTIONS_V2_0;
typedef const struct _WDF_REQUEST_FORWARD_OPTIONS_V2_0 *PCWDF_REQUEST_FORWARD_OPTIONS_V2_0;
typedef struct _WDF_TIMER_CONFIG_V2_0 *PWDF_TIMER_CONFIG_V2_0;
typedef const struct _WDF_TIMER_CONFIG_V2_0 *PCWDF_TIMER_CONFIG_V2_0;
typedef struct _WDFOBJECT_TRIAGE_INFO_V2_0 *PWDFOBJECT_TRIAGE_INFO_V2_0;
typedef const struct _WDFOBJECT_TRIAGE_INFO_V2_0 *PCWDFOBJECT_TRIAGE_INFO_V2_0;
typedef struct _WDFCONTEXT_TRIAGE_INFO_V2_0 *PWDFCONTEXT_TRIAGE_INFO_V2_0;
typedef const struct _WDFCONTEXT_TRIAGE_INFO_V2_0 *PCWDFCONTEXT_TRIAGE_INFO_V2_0;
typedef struct _WDFCONTEXTTYPE_TRIAGE_INFO_V2_0 *PWDFCONTEXTTYPE_TRIAGE_INFO_V2_0;
typedef const struct _WDFCONTEXTTYPE_TRIAGE_INFO_V2_0 *PCWDFCONTEXTTYPE_TRIAGE_INFO_V2_0;
typedef struct _WDFQUEUE_TRIAGE_INFO_V2_0 *PWDFQUEUE_TRIAGE_INFO_V2_0;
typedef const struct _WDFQUEUE_TRIAGE_INFO_V2_0 *PCWDFQUEUE_TRIAGE_INFO_V2_0;
typedef struct _WDFFWDPROGRESS_TRIAGE_INFO_V2_0 *PWDFFWDPROGRESS_TRIAGE_INFO_V2_0;
typedef const struct _WDFFWDPROGRESS_TRIAGE_INFO_V2_0 *PCWDFFWDPROGRESS_TRIAGE_INFO_V2_0;
typedef struct _WDFIRPQUEUE_TRIAGE_INFO_V2_0 *PWDFIRPQUEUE_TRIAGE_INFO_V2_0;
typedef const struct _WDFIRPQUEUE_TRIAGE_INFO_V2_0 *PCWDFIRPQUEUE_TRIAGE_INFO_V2_0;
typedef struct _WDFREQUEST_TRIAGE_INFO_V2_0 *PWDFREQUEST_TRIAGE_INFO_V2_0;
typedef const struct _WDFREQUEST_TRIAGE_INFO_V2_0 *PCWDFREQUEST_TRIAGE_INFO_V2_0;
typedef struct _WDFDEVICE_TRIAGE_INFO_V2_0 *PWDFDEVICE_TRIAGE_INFO_V2_0;
typedef const struct _WDFDEVICE_TRIAGE_INFO_V2_0 *PCWDFDEVICE_TRIAGE_INFO_V2_0;
typedef struct _WDFIRP_TRIAGE_INFO_V2_0 *PWDFIRP_TRIAGE_INFO_V2_0;
typedef const struct _WDFIRP_TRIAGE_INFO_V2_0 *PCWDFIRP_TRIAGE_INFO_V2_0;
typedef struct _WDF_TRIAGE_INFO_V2_0 *PWDF_TRIAGE_INFO_V2_0;
typedef const struct _WDF_TRIAGE_INFO_V2_0 *PCWDF_TRIAGE_INFO_V2_0;
typedef struct _WDF_USB_REQUEST_COMPLETION_PARAMS_V2_0 *PWDF_USB_REQUEST_COMPLETION_PARAMS_V2_0;
typedef const struct _WDF_USB_REQUEST_COMPLETION_PARAMS_V2_0 *PCWDF_USB_REQUEST_COMPLETION_PARAMS_V2_0;
typedef struct _WDF_USB_CONTINUOUS_READER_CONFIG_V2_0 *PWDF_USB_CONTINUOUS_READER_CONFIG_V2_0;
typedef const struct _WDF_USB_CONTINUOUS_READER_CONFIG_V2_0 *PCWDF_USB_CONTINUOUS_READER_CONFIG_V2_0;
typedef struct _WDF_USB_DEVICE_INFORMATION_V2_0 *PWDF_USB_DEVICE_INFORMATION_V2_0;
typedef const struct _WDF_USB_DEVICE_INFORMATION_V2_0 *PCWDF_USB_DEVICE_INFORMATION_V2_0;
typedef struct _WDF_USB_INTERFACE_SETTING_PAIR_V2_0 *PWDF_USB_INTERFACE_SETTING_PAIR_V2_0;
typedef const struct _WDF_USB_INTERFACE_SETTING_PAIR_V2_0 *PCWDF_USB_INTERFACE_SETTING_PAIR_V2_0;
typedef struct _WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V2_0 *PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V2_0;
typedef const struct _WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V2_0 *PCWDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V2_0;
typedef struct _WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V2_0 *PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V2_0;
typedef const struct _WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V2_0 *PCWDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V2_0;
typedef struct _WDF_USB_PIPE_INFORMATION_V2_0 *PWDF_USB_PIPE_INFORMATION_V2_0;
typedef const struct _WDF_USB_PIPE_INFORMATION_V2_0 *PCWDF_USB_PIPE_INFORMATION_V2_0;
typedef struct _WDF_USB_DEVICE_CREATE_CONFIG_V2_0 *PWDF_USB_DEVICE_CREATE_CONFIG_V2_0;
typedef const struct _WDF_USB_DEVICE_CREATE_CONFIG_V2_0 *PCWDF_USB_DEVICE_CREATE_CONFIG_V2_0;
typedef struct _WDF_WMI_PROVIDER_CONFIG_V2_0 *PWDF_WMI_PROVIDER_CONFIG_V2_0;
typedef const struct _WDF_WMI_PROVIDER_CONFIG_V2_0 *PCWDF_WMI_PROVIDER_CONFIG_V2_0;
typedef struct _WDF_WMI_INSTANCE_CONFIG_V2_0 *PWDF_WMI_INSTANCE_CONFIG_V2_0;
typedef const struct _WDF_WMI_INSTANCE_CONFIG_V2_0 *PCWDF_WMI_INSTANCE_CONFIG_V2_0;
typedef struct _WDF_WORKITEM_CONFIG_V2_0 *PWDF_WORKITEM_CONFIG_V2_0;
typedef const struct _WDF_WORKITEM_CONFIG_V2_0 *PCWDF_WORKITEM_CONFIG_V2_0;

//
// Versioning of structures for wdf.h
//
// End of versioning of structures for wdf.h

//
// Versioning of structures for wdfassert.h
//
// End of versioning of structures for wdfassert.h

//
// Versioning of structures for wdfbugcodes.h
//
typedef struct _WDF_POWER_ROUTINE_TIMED_OUT_DATA_V2_0 {
    //
    // Current power state associated with the timed out device
    //
    WDF_DEVICE_POWER_STATE PowerState;

    //
    // Current power policy state associated with the timed out device
    //
    WDF_DEVICE_POWER_POLICY_STATE PowerPolicyState;

    //
    // The device object for the timed out device
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // The handle for the timed out device
    //
    WDFDEVICE Device;

    //
    // The thread which is stuck
    //
    PKTHREAD TimedOutThread;

} WDF_POWER_ROUTINE_TIMED_OUT_DATA_V2_0;

typedef struct _WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V2_0 {
    WDFREQUEST Request;

    PIRP Irp;

    ULONG OutputBufferLength;

    ULONG_PTR Information;

    UCHAR MajorFunction;

} WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V2_0, *PWDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V2_0;

typedef struct _WDF_QUEUE_FATAL_ERROR_DATA_V2_0 {
    WDFQUEUE Queue;

    WDFREQUEST Request;

    NTSTATUS Status;

} WDF_QUEUE_FATAL_ERROR_DATA_V2_0, *PWDF_QUEUE_FATAL_ERROR_DATA_V2_0;

// End of versioning of structures for wdfbugcodes.h

//
// Versioning of structures for wdfchildlist.h
//
typedef struct _WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0 {
    //
    // Size in bytes of the entire description, including this header.
    //
    // Same value as WDF_CHILD_LIST_CONFIG::IdentificationDescriptionSize
    // Used as a sanity check.
    //
    ULONG IdentificationDescriptionSize;

} WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0, *PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0;

typedef struct _WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0 {
    //
    // Size in bytes of the entire description, including this header.
    //
    // Same value as WDF_CHILD_LIST_CONFIG::AddressDescriptionSize
    // Used as a sanity check.
    //
    ULONG AddressDescriptionSize;

} WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0, *PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0;

typedef struct _WDF_CHILD_RETRIEVE_INFO_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Must be a valid pointer when passed in, copied into upon success
    //
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V2_0 IdentificationDescription;

    //
    // Optional pointer when passed in, copied into upon success
    //
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V2_0 AddressDescription;

    //
    // Status of the returned device
    //
    WDF_CHILD_LIST_RETRIEVE_DEVICE_STATUS Status;

    //
    // If provided, will be used for searching through the list of devices
    // instead of the default list ID compare function
    //
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE EvtChildListIdentificationDescriptionCompare;

} WDF_CHILD_RETRIEVE_INFO_V2_0, *PWDF_CHILD_RETRIEVE_INFO_V2_0;

typedef struct _WDF_CHILD_LIST_CONFIG_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // The size in bytes of an identificaiton description to be used with the
    // created WDFCHILDLIST handle
    //
    ULONG IdentificationDescriptionSize;

    //
    // Optional size in bytes of an address description to be used with the
    // created WDFCHILDLIST handle.
    //
    ULONG AddressDescriptionSize;

    //
    // Required callback to be invoked when a description on the device list
    // needs to be converted into a real WDFDEVICE handle.
    //
    PFN_WDF_CHILD_LIST_CREATE_DEVICE EvtChildListCreateDevice;

    //
    // Optional callback to be invoked when the device list needs to be
    // rescanned.  This function will be called after the device has entered D0
    // and been fully initialized but before I/O has started.
    //
    PFN_WDF_CHILD_LIST_SCAN_FOR_CHILDREN EvtChildListScanForChildren;

    //
    // Optional callback to be invoked when an identification description needs
    // to be copied from one location to another.
    //
    // If left NULL, RtlCopyMemory will be used to copy the description.
    //
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COPY EvtChildListIdentificationDescriptionCopy;

    //
    // Optional callback to be invoked when an identification description needs
    // to be duplicated.  As opposed to EvtChildListIdentificationDescriptionCopy,
    // EvtChildListIdentificationDescriptionDuplicate can fail.
    //
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_DUPLICATE EvtChildListIdentificationDescriptionDuplicate;

    //
    // Optional callback to be invoked when an identification description needs
    // to be cleaned up.  This function should *NOT* free the description passed
    // to it, just free any associated resources.
    //
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_CLEANUP EvtChildListIdentificationDescriptionCleanup;

    //
    // Optional callback to be invoked when an identification description needs
    // to be compared with another identificaiton description.
    //
    // If left NULL, RtlCompareMemory will be used to compare the two
    // descriptions.
    //
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE EvtChildListIdentificationDescriptionCompare;

    //
    // Optional callback to be invoked when an address description needs
    // to be copied from one location to another.
    //
    // If left NULL, RtlCopyMemory will be used to copy the description.
    //
    PFN_WDF_CHILD_LIST_ADDRESS_DESCRIPTION_COPY EvtChildListAddressDescriptionCopy;

    //
    // Optional callback to be invoked when an address description needs to be
    // duplicated.  As opposed to EvtChildListAddressDescriptionCopy,
    // EvtChildListAddressDescriptionDuplicate can fail.
    //
    PFN_WDF_CHILD_LIST_ADDRESS_DESCRIPTION_DUPLICATE EvtChildListAddressDescriptionDuplicate;

    //
    // Optional callback to be invoked when an address description needs to be
    // cleaned up.  This function should *NOT* free the description passed to
    // it, just free any associated resources.
    //
    PFN_WDF_CHILD_LIST_ADDRESS_DESCRIPTION_CLEANUP EvtChildListAddressDescriptionCleanup;

    //
    // If provided, will be called when the child's stack requests that the
    // child be reenumerated.  Returning TRUE allows for the reenumeration to
    // proceed.  FALSE will no reenumerate the stack.
    //
    PFN_WDF_CHILD_LIST_DEVICE_REENUMERATED EvtChildListDeviceReenumerated;

} WDF_CHILD_LIST_CONFIG_V2_0, *PWDF_CHILD_LIST_CONFIG_V2_0;

typedef struct _WDF_CHILD_LIST_ITERATOR_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // What type of devices to return, see WDF_RETRIEVE_CHILD_FLAGS for
    // flag values
    //
    //
    ULONG Flags;

    //
    // For internal use, treat this field as opaque
    //
    PVOID Reserved[4];

} WDF_CHILD_LIST_ITERATOR_V2_0, *PWDF_CHILD_LIST_ITERATOR_V2_0;

// End of versioning of structures for wdfchildlist.h

//
// Versioning of structures for wdfcollection.h
//
// End of versioning of structures for wdfcollection.h

//
// Versioning of structures for wdfCommonBuffer.h
//
// End of versioning of structures for wdfCommonBuffer.h

//
// Versioning of structures for wdfcontrol.h
//
// End of versioning of structures for wdfcontrol.h

//
// Versioning of structures for wdfcore.h
//
// End of versioning of structures for wdfcore.h

//
// Versioning of structures for wdfcx.h
//
typedef struct _WDFCX_FILEOBJECT_CONFIG_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callback for create requests
    //
    PFN_WDFCX_DEVICE_FILE_CREATE  EvtCxDeviceFileCreate;

    //
    // Event callback for close requests
    //
    PFN_WDF_FILE_CLOSE   EvtFileClose;

    //
    // Event callback for cleanup requests
    //
    PFN_WDF_FILE_CLEANUP EvtFileCleanup;

    //
    // If WdfTrue, create/cleanup/close file object related requests will be
    // sent down the stack.
    //
    // If WdfFalse, create/cleanup/close will be completed at this location in
    // the device stack.
    //
    // If WdfDefault, behavior depends on device type
    // FDO, PDO, Control:  use the WdfFalse behavior
    // Filter:  use the WdfTrue behavior
    //
    WDF_TRI_STATE AutoForwardCleanupClose;

    //
    // Specify whether framework should create WDFFILEOBJECT and also
    // whether it can FsContexts fields in the WDM fileobject to store
    // WDFFILEOBJECT so that it can avoid table look up and improve perf.
    //
    WDF_FILEOBJECT_CLASS FileObjectClass;

} WDFCX_FILEOBJECT_CONFIG_V2_0, *PWDFCX_FILEOBJECT_CONFIG_V2_0;

// End of versioning of structures for wdfcx.h

//
// Versioning of structures for wdfcxbase.h
//
typedef struct _WDF_CLASS_VERSION_V2_0 {
    WDF_MAJOR_VERSION  Major;

    WDF_MINOR_VERSION  Minor;

    WDF_BUILD_NUMBER   Build;

} WDF_CLASS_VERSION_V2_0;

typedef struct _WDF_CLASS_BIND_INFO_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Name of the class to bind to
    //
    PWSTR ClassName;

    //
    // Version information for the class
    //
    WDF_CLASS_VERSION_V2_0  Version;

    //
    // Function export table from the class driver to resolve on bind
    //
    PFN_WDF_CLASS_EXPORT * FunctionTable;

    //
    // Number of entries in FunctionTable
    //
    ULONG FunctionTableCount;

    //
    // Optional field where the client specify additional information
    // for the class driver to resolve
    //
    PVOID ClassBindInfo;

    //
    // Optional bind callback.  If set, WdfVersionBindClass will not
    // be called and it will be up to ClientClassBind to call this function
    // if required.
    //
    PFN_WDF_CLIENT_BIND_CLASS ClientBindClass;

    //
    // Optional unbind callback.  If set, WdfVersionUnbindClass will not be
    // called and it will be up to ClientClassUnbind to call this function
    // if required.
    //
    PFN_WDF_CLIENT_UNBIND_CLASS ClientUnbindClass;

    //
    // Diagnostic cookie to use during debugging
    //
    PVOID ClassModule;

} WDF_CLASS_BIND_INFO_V2_0, * PWDF_CLASS_BIND_INFO_V2_0;

typedef struct _WDF_CLASS_LIBRARY_INFO_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Version of this class library
    //
    WDF_CLASS_VERSION_V2_0 Version;

    //
    // Callback to be called by the loader to initialize the class library
    //
    PFN_WDF_CLASS_LIBRARY_INITIALIZE ClassLibraryInitialize;

    //
    // Callback to be called by the loader to deinitialize the class library
    // after succesful initialization (immediately before the class library will
    // be unloaded).
    //
    PFN_WDF_CLASS_LIBRARY_DEINITIALIZE ClassLibraryDeinitialize;

    //
    // Callback to be called by the loader when a client driver has request to
    // be bound to this class library.
    //
    PFN_WDF_CLASS_LIBRARY_BIND_CLIENT ClassLibraryBindClient;

    //
    // Callback to be called by the loader when a previously bound client driver
    // is being unloaded.
    //
    PFN_WDF_CLASS_LIBRARY_UNBIND_CLIENT ClassLibraryUnbindClient;

} WDF_CLASS_LIBRARY_INFO_V2_0, *PWDF_CLASS_LIBRARY_INFO_V2_0;

// End of versioning of structures for wdfcxbase.h

//
// Versioning of structures for wdfDevice.h
//
typedef struct _WDF_FILEOBJECT_CONFIG_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callback for create requests
    //
    PFN_WDF_DEVICE_FILE_CREATE  EvtDeviceFileCreate;

    //
    // Event callback for close requests
    //
    PFN_WDF_FILE_CLOSE   EvtFileClose;

    //
    // Event callback for cleanup requests
    //
    PFN_WDF_FILE_CLEANUP EvtFileCleanup;

    //
    // If WdfTrue, create/cleanup/close file object related requests will be
    // sent down the stack.
    //
    // If WdfFalse, create/cleanup/close will be completed at this location in
    // the device stack.
    //
    // If WdfDefault, behavior depends on device type
    // FDO, PDO, Control:  use the WdfFalse behavior
    // Filter:  use the WdfTrue behavior
    //
    WDF_TRI_STATE AutoForwardCleanupClose;

    //
    // Specify whether framework should create WDFFILEOBJECT and also
    // whether it can FsContexts fields in the WDM fileobject to store
    // WDFFILEOBJECT so that it can avoid table look up and improve perf.
    //
    WDF_FILEOBJECT_CLASS FileObjectClass;

} WDF_FILEOBJECT_CONFIG_V2_0, *PWDF_FILEOBJECT_CONFIG_V2_0;

typedef struct _WDF_DEVICE_PNP_NOTIFICATION_DATA_V2_0 {
    //
    // Type of data
    //
    WDF_STATE_NOTIFICATION_TYPE Type;

    union {
        struct {
            //
            // The current state that is about to be exited
            //
            WDF_DEVICE_PNP_STATE CurrentState;

            //
            // The new state that is about to be entered
            //
            WDF_DEVICE_PNP_STATE NewState;

        } EnterState;

        struct {
            //
            // The current state
            //
            WDF_DEVICE_PNP_STATE CurrentState;

        } PostProcessState;

        struct {
            //
            // The current state that is about to be exitted
            //
            WDF_DEVICE_PNP_STATE CurrentState;

            //
            // The state that is about to be entered
            //
            WDF_DEVICE_PNP_STATE NewState;

        } LeaveState;

    } Data;

} WDF_DEVICE_PNP_NOTIFICATION_DATA_V2_0;

typedef struct _WDF_DEVICE_POWER_NOTIFICATION_DATA_V2_0 {
    //
    // Type of data
    //
    WDF_STATE_NOTIFICATION_TYPE Type;

    union {
        struct {
            //
            // The current state that is about to be exitted
            //
            WDF_DEVICE_POWER_STATE CurrentState;

            //
            // The new state that is about to be entered
            //
            WDF_DEVICE_POWER_STATE NewState;

        } EnterState;

        struct {
            //
            // The current state
            //
            WDF_DEVICE_POWER_STATE CurrentState;

        } PostProcessState;

        struct {
            //
            // The current state that is about to be exitted
            //
            WDF_DEVICE_POWER_STATE CurrentState;

            //
            // The state that is about to be entered
            //
            WDF_DEVICE_POWER_STATE NewState;

        } LeaveState;

    } Data;

} WDF_DEVICE_POWER_NOTIFICATION_DATA_V2_0;

typedef struct _WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V2_0 {
    //
    // Type of data
    //
    WDF_STATE_NOTIFICATION_TYPE Type;

    union {
        struct {
            //
            // The current state that is about to be exitted
            //
            WDF_DEVICE_POWER_POLICY_STATE CurrentState;

            //
            // The new state that is about to be entered
            //
            WDF_DEVICE_POWER_POLICY_STATE NewState;

        } EnterState;

        struct {
            //
            // The current state
            //
            WDF_DEVICE_POWER_POLICY_STATE CurrentState;

        } PostProcessState;

        struct {
            //
            // The current state that is about to be exitted
            //
            WDF_DEVICE_POWER_POLICY_STATE CurrentState;

            //
            // The state that is about to be entered
            //
            WDF_DEVICE_POWER_POLICY_STATE NewState;

        } LeaveState;

    } Data;

} WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V2_0;

typedef struct _WDF_PNPPOWER_EVENT_CALLBACKS_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    PFN_WDF_DEVICE_D0_ENTRY                 EvtDeviceD0Entry;

    PFN_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED EvtDeviceD0EntryPostInterruptsEnabled;

    PFN_WDF_DEVICE_D0_EXIT                  EvtDeviceD0Exit;

    PFN_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED EvtDeviceD0ExitPreInterruptsDisabled;

    PFN_WDF_DEVICE_PREPARE_HARDWARE         EvtDevicePrepareHardware;

    PFN_WDF_DEVICE_RELEASE_HARDWARE         EvtDeviceReleaseHardware;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP  EvtDeviceSelfManagedIoCleanup;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH    EvtDeviceSelfManagedIoFlush;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT     EvtDeviceSelfManagedIoInit;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND  EvtDeviceSelfManagedIoSuspend;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART  EvtDeviceSelfManagedIoRestart;

    PFN_WDF_DEVICE_SURPRISE_REMOVAL         EvtDeviceSurpriseRemoval;

    PFN_WDF_DEVICE_QUERY_REMOVE             EvtDeviceQueryRemove;

    PFN_WDF_DEVICE_QUERY_STOP               EvtDeviceQueryStop;

    PFN_WDF_DEVICE_USAGE_NOTIFICATION       EvtDeviceUsageNotification;

    PFN_WDF_DEVICE_RELATIONS_QUERY          EvtDeviceRelationsQuery;

    PFN_WDF_DEVICE_USAGE_NOTIFICATION_EX    EvtDeviceUsageNotificationEx;

} WDF_PNPPOWER_EVENT_CALLBACKS_V2_0, *PWDF_PNPPOWER_EVENT_CALLBACKS_V2_0;

typedef struct _WDF_POWER_POLICY_EVENT_CALLBACKS_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    PFN_WDF_DEVICE_ARM_WAKE_FROM_S0         EvtDeviceArmWakeFromS0;

    PFN_WDF_DEVICE_DISARM_WAKE_FROM_S0      EvtDeviceDisarmWakeFromS0;

    PFN_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED   EvtDeviceWakeFromS0Triggered;

    PFN_WDF_DEVICE_ARM_WAKE_FROM_SX         EvtDeviceArmWakeFromSx;

    PFN_WDF_DEVICE_DISARM_WAKE_FROM_SX      EvtDeviceDisarmWakeFromSx;

    PFN_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED   EvtDeviceWakeFromSxTriggered;

    PFN_WDF_DEVICE_ARM_WAKE_FROM_SX_WITH_REASON EvtDeviceArmWakeFromSxWithReason;

} WDF_POWER_POLICY_EVENT_CALLBACKS_V2_0, *PWDF_POWER_POLICY_EVENT_CALLBACKS_V2_0;

typedef struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Indicates whether the device can wake itself up while the machine is in
    // S0.
    //
    WDF_POWER_POLICY_S0_IDLE_CAPABILITIES IdleCaps;

    //
    // The low power state in which the device will be placed when it is idled
    // out while the machine is in S0.
    //
    DEVICE_POWER_STATE DxState;

    //
    // Amount of time the device must be idle before idling out.  Timeout is in
    // milliseconds.
    //
    ULONG IdleTimeout;

    //
    // Inidcates whether a user can control the idle policy of the device.
    // By default, a user is allowed to change the policy.
    //
    WDF_POWER_POLICY_S0_IDLE_USER_CONTROL UserControlOfIdleSettings;

    //
    // If WdfTrue, idling out while the machine is in S0 will be enabled.
    //
    // If WdfFalse, idling out will be disabled.
    //
    // If WdfUseDefault, the idling out will be enabled.  If
    // UserControlOfIdleSettings is set to IdleAllowUserControl, the user's
    // settings will override the default.
    //
    WDF_TRI_STATE Enabled;

    //
    // This field is applicable only when IdleCaps == IdleCannotWakeFromS0
    // If WdfTrue,device is powered up on System Wake even if device is idle
    // If WdfFalse, device is not powered up on system wake if it is idle
    // If WdfUseDefault, the behavior is same as WdfFalse
    //
    WDF_TRI_STATE PowerUpIdleDeviceOnSystemWake;

    //
    // This field determines how the IdleTimeout field is used.
    //
    // If the value is DriverManagedIdleTimeout, then the idle timeout value
    // is determined by the IdleTimeout field of this structure.
    //
    // If the value is SystemManagedIdleTimeout, then the timeout value is
    // determined by the power framework (PoFx) on operating systems where
    // the PoFx is available (i.e. Windows 8 and later). The IdleTimeout field
    // is ignored on these operating systems. On operating systems where the
    // PoFx is not available, the behavior is same as DriverManagedIdleTimeout.
    //
    // If the value is SystemManagedIdleTimeoutWithHint, then the timeout value
    // is determined by the power framework (PoFx) on operating systems where
    // the PoFx is available (i.e. Windows 8 and later). In addition, the value
    // specified in the IdleTimeout field is provided as a hint to the PoFx in
    // determining when the device should be allowed to enter a low-power state.
    // Since it is only a hint, the actual duration after which the PoFx allows
    // the device to enter a low-power state might be greater than or less than
    // the IdleTimeout value. On operating systems where the PoFx is not
    // available, the behavior is same as DriverManagedIdleTimeout.
    //
    WDF_POWER_POLICY_IDLE_TIMEOUT_TYPE IdleTimeoutType;

    //
    // This field forces the device to avoid idling in the D3cold power state.
    // WDF will ensure, with help from the bus drivers, that the device will
    // idle in a D-state that can successfully generate a wake signal, if
    // necessary.  If the client specifies that DxState == PowerDeviceD3, this
    // setting allows the client to distinguish betwen D3hot and D3cold.  If
    // the client sets DxState == PowerDeviceMaximum, then WDF will pick the
    // deepest idle state identified by the bus driver.  If that deepest state
    // is D3cold, this field allows the client to override that and choose
    // D3hot.
    //
    // If WdfTrue, device will not use D3cold in S0.
    // If WdfFalse, device will use D3cold in S0 if the ACPI firmware indicates
    // that the device can enter that state, if DxState above does not
    // specify some other D-state and, if the device is armed for
    // wake, that it can generate its wake signal from D3cold.
    // If WdfUseDefault, this setting will be derived from the driver's INF,
    // specifically the presence or absence of the following two lines in
    // the DDInstall.HW section:
    // Include=machine.inf
    // Needs=PciD3ColdSupported
    //
    WDF_TRI_STATE ExcludeD3Cold;

} WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V2_0, *PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V2_0;

typedef struct _WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // The low power state in which the device will be placed when it is armed
    // for wake from Sx.
    //
    DEVICE_POWER_STATE DxState;

    //
    // Inidcates whether a user can control the idle policy of the device.
    // By default, a user is allowed to change the policy.
    //
    WDF_POWER_POLICY_SX_WAKE_USER_CONTROL UserControlOfWakeSettings;

    //
    // If WdfTrue, arming the device for wake while the machine is in Sx is
    // enabled.
    //
    // If WdfFalse, arming the device for wake while the machine is in Sx is
    // disabled.
    //
    // If WdfUseDefault, arming will be enabled.  If UserControlOfWakeSettings
    // is set to WakeAllowUserControl, the user's settings will override the
    // default.
    //
    WDF_TRI_STATE Enabled;

    //
    // If set to TRUE, arming the parent device can depend on whether there
    // is atleast one child device armed for wake.
    //
    // If set to FALSE, arming of the parent device will be independent of
    // whether any of the child devices are armed for wake.
    //
    BOOLEAN ArmForWakeIfChildrenAreArmedForWake;

    //
    // Indicates that whenever the parent device completes the wake irp
    // successfully, the status needs to be also propagated to the child
    // devices.  This helps in tracking which devices were responsible for
    // waking the system.
    //
    BOOLEAN IndicateChildWakeOnParentWake;

} WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V2_0, *PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V2_0;

typedef struct _WDF_DEVICE_STATE_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // If set to WdfTrue, the device will be disabled
    //
    WDF_TRI_STATE Disabled;

    //
    // If set to WdfTrue, the device will not be displayed in device manager.
    // Once hidden, the device cannot be unhidden.
    //
    WDF_TRI_STATE DontDisplayInUI;

    //
    // If set to WdfTrue, the device is reporting itself as failed.  If set
    // in conjuction with ResourcesChanged to WdfTrue, the device will receive
    // a PnP stop and then a new PnP start device.
    //
    WDF_TRI_STATE Failed;

    //
    // If set to WdfTrue, the device cannot be subsequently disabled.
    //
    WDF_TRI_STATE NotDisableable;




    //
    //
    // If set to WdfTrue, the device stack will be torn down.
    //
    WDF_TRI_STATE Removed;

    //
    // If set to WdfTrue, the device will be sent another PnP start.  If the
    // Failed field is set to WdfTrue as well, a PnP stop will be sent before
    // the start.
    //
    WDF_TRI_STATE ResourcesChanged;

} WDF_DEVICE_STATE_V2_0, *PWDF_DEVICE_STATE_V2_0;

typedef struct _WDF_DEVICE_PNP_CAPABILITIES_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // NOTE: To mark a PDO as raw, call WdfPdoInitAssignRawDevice
    //
    //
    WDF_TRI_STATE LockSupported;

    WDF_TRI_STATE EjectSupported;

    WDF_TRI_STATE Removable;

    WDF_TRI_STATE DockDevice;

    WDF_TRI_STATE UniqueID;

    WDF_TRI_STATE SilentInstall;

    WDF_TRI_STATE SurpriseRemovalOK;

    WDF_TRI_STATE HardwareDisabled;

    WDF_TRI_STATE NoDisplayInUI;

    //
    // Default values of -1 indicate not to set this value
    //
    ULONG Address;

    ULONG UINumber;

} WDF_DEVICE_PNP_CAPABILITIES_V2_0, *PWDF_DEVICE_PNP_CAPABILITIES_V2_0;

typedef struct _WDF_DEVICE_POWER_CAPABILITIES_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    WDF_TRI_STATE DeviceD1;

    WDF_TRI_STATE DeviceD2;

    WDF_TRI_STATE WakeFromD0;

    WDF_TRI_STATE WakeFromD1;

    WDF_TRI_STATE WakeFromD2;

    WDF_TRI_STATE WakeFromD3;

    //
    // Default value PowerDeviceMaximum indicates not to set this value
    //
    DEVICE_POWER_STATE DeviceState[PowerSystemMaximum];

    //
    // Default value PowerDeviceMaximum, PowerSystemMaximum indicates not to
    // set this value.
    //
    DEVICE_POWER_STATE DeviceWake;

    SYSTEM_POWER_STATE SystemWake;

    //
    // Default values of -1 indicate not to set this value
    //
    ULONG D1Latency;

    ULONG D2Latency;

    ULONG D3Latency;

    //
    // Ideal Dx state for the device to be put into when the machine moves into
    // Sx and the device is not armed for wake.  By default, the default will be
    // placed into D3.  If IdealDxStateForSx is lighter then
    // DeviceState[Sx], then DeviceState[Sx] will be used as the Dx state.
    //
    DEVICE_POWER_STATE IdealDxStateForSx;

} WDF_DEVICE_POWER_CAPABILITIES_V2_0, *PWDF_DEVICE_POWER_CAPABILITIES_V2_0;

typedef struct _WDF_REMOVE_LOCK_OPTIONS_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Bit field combination of values from the WDF_REMOVE_LOCK_OPTIONS_FLAGS
    // enumeration
    //
    ULONG Flags;

} WDF_REMOVE_LOCK_OPTIONS_V2_0, *PWDF_REMOVE_LOCK_OPTIONS_V2_0;

typedef struct _WDF_POWER_FRAMEWORK_SETTINGS_V2_0 {
    //
    // Size of the structure, in bytes.
    //
    ULONG Size;

    //
    // Client driver's callback function that is invoked after KMDF has
    // registered with the power framework. This field can be NULL if the
    // client driver does not wish to specify this callback.
    //
    PFN_WDFDEVICE_WDM_POST_PO_FX_REGISTER_DEVICE EvtDeviceWdmPostPoFxRegisterDevice;

    //
    // Client driver's callback function that is invoked before KMDF
    // unregisters with the power framework. This field can be NULL if the
    // client driver does not wish to specify this callback.
    //
    PFN_WDFDEVICE_WDM_PRE_PO_FX_UNREGISTER_DEVICE EvtDeviceWdmPrePoFxUnregisterDevice;

    //
    // Pointer to a PO_FX_COMPONENT structure that describes the only component
    // in the single-component device. This field can be NULL if the client
    // driver wants KMDF to use the default specification for this component
    // (i.e. support for F0 only).
    //
    PPO_FX_COMPONENT Component;

    //
    // Client driver's PO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK callback
    // function. This field can be NULL if the client driver does not wish to
    // specify this callback.
    //
    PPO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK ComponentActiveConditionCallback;

    //
    // Client driver's PO_FX_COMPONENT_IDLE_CONDITION_CALLBACK callback
    // function. This field can be NULL if the client driver does not wish to
    // specify this callback.
    //
    PPO_FX_COMPONENT_IDLE_CONDITION_CALLBACK ComponentIdleConditionCallback;

    //
    // Client driver's PO_FX_COMPONENT_IDLE_STATE_CALLBACK callback function.
    // This field can be NULL if the client driver does not wish to specify
    // this callback.
    //
    PPO_FX_COMPONENT_IDLE_STATE_CALLBACK ComponentIdleStateCallback;

    //
    // Client driver's PO_FX_POWER_CONTROL_CALLBACK callback function. This
    // field can be NULL if the client driver does not wish to specify this
    // callback.
    //
    PPO_FX_POWER_CONTROL_CALLBACK PowerControlCallback;

    //
    // Context value that is passed in to the ComponentIdleStateCallback and
    // PowerControlCallback callback functions.
    //
    PVOID PoFxDeviceContext;

} WDF_POWER_FRAMEWORK_SETTINGS_V2_0, *PWDF_POWER_FRAMEWORK_SETTINGS_V2_0;

typedef struct _WDF_IO_TYPE_CONFIG_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // <KMDF_DOC/>
    // Identifies the method that the driver will use to access data buffers
    // that it receives for read and write requests.
    //
    // <UMDF_DOC/>
    // Identifies the method that the driver will "prefer" to use to access data
    // buffers that it receives for read and write requests. Note that UMDF
    // driver provides just a preference, and not a guarantee.Therefore,
    // even if a driver specified direct access method, UMDF might use the
    // buffered access method for one or more of the device's requests to
    // improve performance. For example, UMDF uses buffered access for small
    // buffers, if it can copy the data to the driver's buffer faster than it
    // can map the buffers for direct access.
    //
    WDF_DEVICE_IO_TYPE ReadWriteIoType;

    //
    // <UMDF_ONLY/>
    // Identifies the method that the driver will "prefer" to use to access data
    // buffers that it receives for IOCTL requests. Note that UMDF
    // driver provides just a preference, and not a guarantee. Therefore,
    // even if a driver specified direct access method, UMDF might use the
    // buffered access method for one or more of the device's requests to
    // improve performance. For example, UMDF uses buffered access for small
    // buffers, if it can copy the data to the driver's buffer faster than it
    // can map the buffers for direct access.
    //
    WDF_DEVICE_IO_TYPE DeviceControlIoType;

    //
    // <UMDF_ONLY/>
    // Optional, Provides the smallest buffer size (in bytes) for which
    // UMDF will use direct access for the buffers. For example, set
    // DirectTransferThreshold to "12288" to indicate that UMDF should use buffered
    // access for all buffers that are smaller than 12 kilobytes, and direct
    // access for buffers equal to or greater than that. Typically, you
    // do not need to provide this value because UMDF uses a value that provides
    // the best performance. Note that there are other requirements that must be
    // met in order to get direct access of buffers. See Docs for details.
    //
    ULONG DirectTransferThreshold;

} WDF_IO_TYPE_CONFIG_V2_0, *PWDF_IO_TYPE_CONFIG_V2_0;

typedef struct _WDF_DEVICE_INTERFACE_PROPERTY_DATA_V2_0 {
    _In_      ULONG Size;

    //
    // A pointer to a GUID that identifies the device interface class.
    //
    _In_ const GUID * InterfaceClassGUID;

    //
    // A pointer to a UNICODE_STRING structure that describes a reference
    // string for the device interface. This parameter is optional and can
    // be NULL.
    _In_opt_  PCUNICODE_STRING ReferenceString;

    //
    // A pointer to a DEVPROPKEY structure that specifies the device
    // property key.
    //
    _In_  const DEVPROPKEY * PropertyKey;

    //
    // A locale identifier. Set this parameter either to a language-specific
    // LCID value or to LOCALE_NEUTRAL. The LOCALE_NEUTRAL LCID specifies
    // that the property is language-neutral (that is, not specific to any
    // language). Do not set this parameter to LOCALE_SYSTEM_DEFAULT or
    // LOCALE_USER_DEFAULT. For more information about language-specific
    // LCID values, see LCID Structure.
    //
    _In_ LCID Lcid;

    //
    // Set this parameter to PLUGPLAY_PROPERTY_PERSISTENT if the property
    // value set by this routine should persist across computer restarts.
    // Otherwise, set Flags to zero. Ignored for Query DDIs.
    //
    _In_ ULONG Flags;

} WDF_DEVICE_INTERFACE_PROPERTY_DATA_V2_0, *PWDF_DEVICE_INTERFACE_PROPERTY_DATA_V2_0;

typedef struct _WDF_DEVICE_PROPERTY_DATA_V2_0 {
    //
    // Size of this structure
    //
    _In_      ULONG Size;

    //
    // A pointer to a DEVPROPKEY structure that specifies the device
    // property key.
    //
    _In_  const DEVPROPKEY * PropertyKey;

    //
    // A locale identifier. Set this parameter either to a language-specific
    // LCID value or to LOCALE_NEUTRAL. The LOCALE_NEUTRAL LCID specifies
    // that the property is language-neutral (that is, not specific to any
    // language). Do not set this parameter to LOCALE_SYSTEM_DEFAULT or
    // LOCALE_USER_DEFAULT. For more information about language-specific
    // LCID values, see LCID Structure.
    //
    _In_ LCID Lcid;

    //
    // Set this parameter to PLUGPLAY_PROPERTY_PERSISTENT if the property
    // value set by this routine should persist across computer restarts.
    // Otherwise, set Flags to zero. Ignored for Query DDIs.
    //
    _In_ ULONG Flags;

} WDF_DEVICE_PROPERTY_DATA_V2_0, *PWDF_DEVICE_PROPERTY_DATA_V2_0;

// End of versioning of structures for wdfDevice.h

//
// Versioning of structures for wdfDmaEnabler.h
//
// End of versioning of structures for wdfDmaEnabler.h

//
// Versioning of structures for wdfDmaTransaction.h
//
// End of versioning of structures for wdfDmaTransaction.h

//
// Versioning of structures for wdfdpc.h
//
// End of versioning of structures for wdfdpc.h

//
// Versioning of structures for wdfdriver.h
//
typedef struct _WDF_DRIVER_CONFIG_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callbacks
    //
    PFN_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

    PFN_WDF_DRIVER_UNLOAD    EvtDriverUnload;

    //
    // Combination of WDF_DRIVER_INIT_FLAGS values
    //
    ULONG DriverInitFlags;

    //
    // Pool tag to use for all allocations made by the framework on behalf of
    // the client driver.
    //
    ULONG DriverPoolTag;

} WDF_DRIVER_CONFIG_V2_0, *PWDF_DRIVER_CONFIG_V2_0;

typedef struct _WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Major Version requested
    //
    ULONG MajorVersion;

    //
    // Minor Version requested
    //
    ULONG MinorVersion;

} WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V2_0, *PWDF_DRIVER_VERSION_AVAILABLE_PARAMS_V2_0;

// End of versioning of structures for wdfdriver.h

//
// Versioning of structures for wdffdo.h
//
typedef struct _WDF_FDO_EVENT_CALLBACKS_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    PFN_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS EvtDeviceFilterAddResourceRequirements;

    PFN_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS EvtDeviceFilterRemoveResourceRequirements;

    PFN_WDF_DEVICE_REMOVE_ADDED_RESOURCES EvtDeviceRemoveAddedResources;

} WDF_FDO_EVENT_CALLBACKS_V2_0, *PWDF_FDO_EVENT_CALLBACKS_V2_0;

// End of versioning of structures for wdffdo.h

//
// Versioning of structures for wdffileobject.h
//
// End of versioning of structures for wdffileobject.h

//
// Versioning of structures for wdfGlobals.h
//
typedef struct _WDF_DRIVER_GLOBALS_V2_0 {
    // backpointer to the handle for this driver
    WDFDRIVER Driver;

    // Flags indicated by the driver during create
    ULONG DriverFlags;

    // Tag generated by WDF for the driver.  Tag used by allocations made on
    // behalf of the driver by WDF.
    ULONG DriverTag;

    CHAR DriverName[WDF_DRIVER_GLOBALS_NAME_LEN];

    // If TRUE, the stub code will capture DriverObject->DriverUnload and insert
    // itself first in the unload chain.  If FALSE, DriverUnload is left alone
    // (but WDF will not be notified of unload and there will be no auto cleanup).
    BOOLEAN DisplaceDriverUnload;

} WDF_DRIVER_GLOBALS_V2_0, *PWDF_DRIVER_GLOBALS_V2_0;

// End of versioning of structures for wdfGlobals.h

//
// Versioning of structures for wdfhwaccess.h
//
// End of versioning of structures for wdfhwaccess.h

//
// Versioning of structures for wdfinstaller.h
//
// End of versioning of structures for wdfinstaller.h

//
// Versioning of structures for wdfinternal.h
//
// End of versioning of structures for wdfinternal.h

//
// Versioning of structures for wdfinterrupt.h
//
//
// Interrupt Configuration Structure
//
typedef struct _WDF_INTERRUPT_CONFIG_V2_0 {
    ULONG              Size;

    //
    // If this interrupt is to be synchronized with other interrupt(s) assigned
    // to the same WDFDEVICE, create a WDFSPINLOCK and assign it to each of the
    // WDFINTERRUPTs config.
    //
    WDFSPINLOCK                     SpinLock;

    WDF_TRI_STATE                   ShareVector;

    BOOLEAN                         FloatingSave;

    //
    // DIRQL handling: automatic serialization of the DpcForIsr/WaitItemForIsr.
    // Passive-level handling: automatic serialization of all callbacks.
    //
    BOOLEAN                         AutomaticSerialization;

    //
    // Event Callbacks
    //
    PFN_WDF_INTERRUPT_ISR           EvtInterruptIsr;

    PFN_WDF_INTERRUPT_DPC           EvtInterruptDpc;

    PFN_WDF_INTERRUPT_ENABLE        EvtInterruptEnable;

    PFN_WDF_INTERRUPT_DISABLE       EvtInterruptDisable;

    PFN_WDF_INTERRUPT_WORKITEM      EvtInterruptWorkItem;

    //
    // These fields are only used when interrupt is created in
    // EvtDevicePrepareHardware callback.
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptRaw;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptTranslated;

    //
    // Optional passive lock for handling interrupts at passive-level.
    //
    WDFWAITLOCK                     WaitLock;

    //
    // TRUE: handle interrupt at passive-level.
    // FALSE: handle interrupt at DIRQL level. This is the default.
    //
    BOOLEAN                         PassiveHandling;

    //
    // TRUE: Interrupt is reported inactive on explicit power down
    // instead of disconnecting it.
    // FALSE: Interrupt is disconnected instead of reporting inactive
    // on explicit power down.
    // DEFAULT: Framework decides the right value.
    //
    WDF_TRI_STATE                   ReportInactiveOnPowerDown;

    //
    // TRUE: Interrupt is used to wake the device from low-power states
    // and when the device is armed for wake this interrupt should
    // remain connected.
    // FALSE: Interrupt is not used to wake the device from low-power states.
    // This is the default.
    //
    BOOLEAN                         CanWakeDevice;

} WDF_INTERRUPT_CONFIG_V2_0, *PWDF_INTERRUPT_CONFIG_V2_0;

typedef struct _WDF_INTERRUPT_INFO_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG                  Size;

    ULONG64                Reserved1;

    KAFFINITY              TargetProcessorSet;

    ULONG                  Reserved2;

    ULONG                  MessageNumber;

    ULONG                  Vector;

    KIRQL                  Irql;

    KINTERRUPT_MODE        Mode;

    WDF_INTERRUPT_POLARITY Polarity;

    BOOLEAN                MessageSignaled;

    // CM_SHARE_DISPOSITION
    UCHAR                  ShareDisposition;

    DECLSPEC_ALIGN(8) USHORT Group;

} WDF_INTERRUPT_INFO_V2_0, *PWDF_INTERRUPT_INFO_V2_0;

//
// Interrupt Extended Policy Configuration Structure
//
typedef struct _WDF_INTERRUPT_EXTENDED_POLICY_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG                   Size;

    WDF_INTERRUPT_POLICY    Policy;

    WDF_INTERRUPT_PRIORITY  Priority;

    GROUP_AFFINITY          TargetProcessorSetAndGroup;

} WDF_INTERRUPT_EXTENDED_POLICY_V2_0, *PWDF_INTERRUPT_EXTENDED_POLICY_V2_0;

// End of versioning of structures for wdfinterrupt.h

//
// Versioning of structures for wdfio.h
//
//
// This is the structure used to configure an IoQueue and
// register callback events to it.
//
//
typedef struct _WDF_IO_QUEUE_CONFIG_V2_0 {
    ULONG                                       Size;

    WDF_IO_QUEUE_DISPATCH_TYPE                  DispatchType;

    WDF_TRI_STATE                               PowerManaged;

    BOOLEAN                                     AllowZeroLengthRequests;

    BOOLEAN                                     DefaultQueue;

    PFN_WDF_IO_QUEUE_IO_DEFAULT                 EvtIoDefault;

    PFN_WDF_IO_QUEUE_IO_READ                    EvtIoRead;

    PFN_WDF_IO_QUEUE_IO_WRITE                   EvtIoWrite;

    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL          EvtIoDeviceControl;

    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

    PFN_WDF_IO_QUEUE_IO_STOP                    EvtIoStop;

    PFN_WDF_IO_QUEUE_IO_RESUME                  EvtIoResume;

    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE       EvtIoCanceledOnQueue;

    union {
        struct {
            ULONG NumberOfPresentedRequests;

        } Parallel;

    } Settings;

    WDFDRIVER                                   Driver;

} WDF_IO_QUEUE_CONFIG_V2_0, *PWDF_IO_QUEUE_CONFIG_V2_0;

typedef struct _WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_V2_0 {
    ULONG  Size;

    ULONG TotalForwardProgressRequests;

    //
    // Specify the type of the policy here.
    //
    WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY ForwardProgressReservedPolicy;

    //
    // Structure which contains the policy specific fields
    //
    WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY_SETTINGS ForwardProgressReservePolicySettings;

    //
    // Callback for reserved request given at initialization time
    //
    PFN_WDF_IO_ALLOCATE_RESOURCES_FOR_RESERVED_REQUEST EvtIoAllocateResourcesForReservedRequest;

    //
    // Callback for reserved request given at run time
    //
    PFN_WDF_IO_ALLOCATE_REQUEST_RESOURCES  EvtIoAllocateRequestResources;

} WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_V2_0, *PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_V2_0;

// End of versioning of structures for wdfio.h

//
// Versioning of structures for wdfIoTarget.h
//
typedef struct _WDF_IO_TARGET_OPEN_PARAMS_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Indicates which fields of this structure are going to be used in
    // creating the WDFIOTARGET.
    //
    WDF_IO_TARGET_OPEN_TYPE Type;

    //
    // Notification when the target is being queried for removal.
    // If !NT_SUCCESS is returned, the query will fail and the target will
    // remain opened.
    //
    PFN_WDF_IO_TARGET_QUERY_REMOVE EvtIoTargetQueryRemove;

    //
    // The previous query remove has been canceled and the target can now be
    // reopened.
    //
    PFN_WDF_IO_TARGET_REMOVE_CANCELED EvtIoTargetRemoveCanceled;

    //
    // The query remove has succeeded and the target is now removed from the
    // system.
    //
    PFN_WDF_IO_TARGET_REMOVE_COMPLETE EvtIoTargetRemoveComplete;

    // <KMDF_ONLY/>
    // ========== WdfIoTargetOpenUseExistingDevice begin ==========
    //
    // The device object to send requests to
    //
    PDEVICE_OBJECT TargetDeviceObject;

    // <KMDF_ONLY/>
    // File object representing the TargetDeviceObject.  The PFILE_OBJECT will
    // be passed as a parameter in all requests sent to the resulting
    // WDFIOTARGET.
    //
    PFILE_OBJECT TargetFileObject;

    // ========== WdfIoTargetOpenUseExistingDevice end ==========
    //
    // ========== WdfIoTargetOpenByName begin ==========
    //
    // Name of the device to open.
    //
    UNICODE_STRING TargetDeviceName;

    // <KMDF_DOC>
    // The access desired on the device being opened up, ie WDM FILE_XXX_ACCESS
    // such as FILE_ANY_ACCESS, FILE_SPECIAL_ACCESS, FILE_READ_ACCESS, or
    // FILE_WRITE_ACCESS or you can use values such as GENERIC_READ,
    // GENERIC_WRITE, or GENERIC_ALL.
    // </KMDF_DOC>
    // <UMDF_DOC>
    // The requested access to the file or device, which can be summarized as
    // read, write, both or neither zero). For more information about
    // this member, see the dwDesiredAccess parameter of CreateFile in the
    // Windows SDK. Note that ACCESS_MASK data type is a DWORD value.
    // </UMDF_DOC>
    //
    ACCESS_MASK DesiredAccess;

    //
    // <KMDF_DOC>
    // Share access desired on the target being opened, ie WDM FILE_SHARE_XXX
    // values such as FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE.
    // A zero value means exclusive access to the target.
    //
    // </KMDF_DOC>
    // <UMDF_DOC>
    // The type of sharing to allow for the file. For more information about
    // this member, see the dwShareMode parameter of CreateFile in the
    // Windows SDK. A value of 0 means exclusive access.
    // </UMDF_DOC>
    //
    ULONG ShareAccess;

    //
    // <KMDF_DOC>
    // File  attributes, see ZwCreateFile in the DDK for a list of valid
    // values and their meaning.
    // </KMDF_DOC>
    // <UMDF_DOC>
    // Additional flags and attributes for the file. For more information about
    // this member, see the dwFlagsAndAttributes parameter of CreateFile
    // in the Windows SDK.
    // </UMDF_DOC>
    //
    ULONG FileAttributes;

    //
    // <KMDF_DOC>
    // Create disposition, see ZwCreateFile in the DDK for a list of valid
    // values and their meaning.
    // </KMDF_DOC>
    // <UMDF_DOC>
    // The action to take if the file already exists. For more information
    // about this member, see the dwCreationDisposition parameter of
    // CreateFile in the Windows SDK.
    // </UMDF_DOC>
    //
    ULONG CreateDisposition;

    //
    // <KMDF_ONLY/>
    // Options for opening the device, see CreateOptions for ZwCreateFile in the
    // DDK for a list of valid values and their meaning.
    //
    ULONG CreateOptions;

    //
    // <KMDF_ONLY/>
    //
    PVOID EaBuffer;

    //
    // <KMDF_ONLY/>
    //
    ULONG EaBufferLength;

    //
    // <KMDF_ONLY/>
    //
    PLONGLONG AllocationSize;

    // ========== WdfIoTargetOpenByName end ==========
    //
    //
    // <KMDF_ONLY/>
    //
    // On return for a create by name, this will contain one of the following
    // values:  FILE_CREATED, FILE_OPENED, FILE_OVERWRITTEN, FILE_SUPERSEDED,
    // FILE_EXISTS, FILE_DOES_NOT_EXIST
    //
    ULONG FileInformation;

    // ========== WdfIoTargetOpenLocalTargetByFile begin ==========
    //
    //
    // <UMDF_ONLY/> A UNICODE_STRING-formatted string that contains the
    // name of the file to create a file object from. This parameter is
    // optional, and is applicable only when Type parameter is
    // WdfIoTargetOpenLocalTargetByFile. The driver can leave this member
    // unchanged if the driver does not have to create the file object
    // from a file name. If the driver must supply a name, the string that
    // the driver passes must not contain any path separator characters
    // ("/" or "\").
    //
    UNICODE_STRING FileName;

} WDF_IO_TARGET_OPEN_PARAMS_V2_0, *PWDF_IO_TARGET_OPEN_PARAMS_V2_0;

// End of versioning of structures for wdfIoTarget.h

//
// Versioning of structures for wdfMemory.h
//
typedef struct _WDFMEMORY_OFFSET_V2_0 {
    //
    // Offset into the WDFMEMORY that the operation should start at.
    //
    size_t BufferOffset;

    //
    // Number of bytes that the operation should access.  If 0, the entire
    // length of the WDFMEMORY buffer will be used in the operation or ignored
    // depending on the API.
    //
    size_t BufferLength;

} WDFMEMORY_OFFSET_V2_0, *PWDFMEMORY_OFFSET_V2_0;

typedef struct _WDF_MEMORY_DESCRIPTOR_V2_0 {
    WDF_MEMORY_DESCRIPTOR_TYPE Type;

    union {
        struct {
            PVOID Buffer;

            ULONG Length;

        } BufferType;

        struct {
            PMDL Mdl;

            ULONG BufferLength;

        } MdlType;

        struct {
            WDFMEMORY Memory;

            PWDFMEMORY_OFFSET_V2_0 Offsets;

        } HandleType;

    } u;

} WDF_MEMORY_DESCRIPTOR_V2_0, *PWDF_MEMORY_DESCRIPTOR_V2_0;

// End of versioning of structures for wdfMemory.h

//
// Versioning of structures for wdfMiniport.h
//
// End of versioning of structures for wdfMiniport.h

//
// Versioning of structures for wdfObject.h
//
typedef struct _WDF_OBJECT_ATTRIBUTES_V2_0 {
    //
    // Size in bytes of this structure
    //
    ULONG Size;

    //
    // Function to call when the object is deleted
    //
    PFN_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanupCallback;

    //
    // Function to call when the objects memory is destroyed when the
    // the last reference count goes to zero
    //
    PFN_WDF_OBJECT_CONTEXT_DESTROY EvtDestroyCallback;

    //
    // Execution level constraints for Object
    //
    WDF_EXECUTION_LEVEL ExecutionLevel;

    //
    // Synchronization level constraint for Object
    //
    WDF_SYNCHRONIZATION_SCOPE SynchronizationScope;

    //
    // Optional Parent Object
    //
    WDFOBJECT ParentObject;

    //
    // Overrides the size of the context allocated as specified by
    // ContextTypeInfo->ContextSize
    //
    size_t ContextSizeOverride;

    //
    // Pointer to the type information to be associated with the object
    //
    PCWDF_OBJECT_CONTEXT_TYPE_INFO_V2_0 ContextTypeInfo;

} WDF_OBJECT_ATTRIBUTES_V2_0, *PWDF_OBJECT_ATTRIBUTES_V2_0;

//
// Since C does not have strong type checking we must invent our own
//
typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V2_0 {
    //
    // The size of this structure in bytes
    //
    ULONG Size;

    //
    // String representation of the context's type name, i.e. "DEVICE_CONTEXT"
    //
    PCHAR ContextName;

    //
    // The size of the context in bytes.  This will be the size of the context
    // associated with the handle unless
    // WDF_OBJECT_ATTRIBUTES::ContextSizeOverride is specified.
    //
    size_t ContextSize;

    //
    // If NULL, this structure is the unique type identifier for the context
    // type.  If != NULL, the UniqueType pointer value is the unique type id
    // for the context type.
    //
    PCWDF_OBJECT_CONTEXT_TYPE_INFO_V2_0 UniqueType;

    //
    // Function pointer to retrieve the context type information structure
    // pointer from the provider of the context type.  This function is invoked
    // by the client driver's entry point by the KMDF stub after all class
    // drivers are loaded and before DriverEntry is invoked.
    //
    PFN_GET_UNIQUE_CONTEXT_TYPE EvtDriverGetUniqueContextType;

} WDF_OBJECT_CONTEXT_TYPE_INFO_V2_0, *PWDF_OBJECT_CONTEXT_TYPE_INFO_V2_0;

//
// Core structure for supporting custom types, see macros below.
//
typedef struct _WDF_CUSTOM_TYPE_CONTEXT_V2_0 {
    ULONG       Size;

    ULONG_PTR   Data;

} WDF_CUSTOM_TYPE_CONTEXT_V2_0, *PWDF_CUSTOM_TYPE_CONTEXT_V2_0;

// End of versioning of structures for wdfObject.h

//
// Versioning of structures for wdfpdo.h
//
typedef struct _WDF_PDO_EVENT_CALLBACKS_V2_0 {
    //
    // The size of this structure in bytes
    //
    ULONG Size;

    //
    // Called in response to IRP_MN_QUERY_RESOURCES
    //
    PFN_WDF_DEVICE_RESOURCES_QUERY EvtDeviceResourcesQuery;

    //
    // Called in response to IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    //
    PFN_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY EvtDeviceResourceRequirementsQuery;

    //
    // Called in response to IRP_MN_EJECT
    //
    PFN_WDF_DEVICE_EJECT EvtDeviceEject;

    //
    // Called in response to IRP_MN_SET_LOCK
    //
    PFN_WDF_DEVICE_SET_LOCK EvtDeviceSetLock;

    //
    // Called in response to the power policy owner sending a wait wake to the
    // PDO.  Bus generic arming shoulding occur here.
    //
    PFN_WDF_DEVICE_ENABLE_WAKE_AT_BUS       EvtDeviceEnableWakeAtBus;

    //
    // Called in response to the power policy owner sending a wait wake to the
    // PDO.  Bus generic disarming shoulding occur here.
    //
    PFN_WDF_DEVICE_DISABLE_WAKE_AT_BUS      EvtDeviceDisableWakeAtBus;

    //
    // Called when reporting the PDO missing to PnP manager in response to
    // IRP_MN_QUERY_DEVICE_RELATIONS for Bus Relations.
    //
    PFN_WDF_DEVICE_REPORTED_MISSING EvtDeviceReportedMissing;

} WDF_PDO_EVENT_CALLBACKS_V2_0, *PWDF_PDO_EVENT_CALLBACKS_V2_0;

// End of versioning of structures for wdfpdo.h

//
// Versioning of structures for wdfpool.h
//
// End of versioning of structures for wdfpool.h

//
// Versioning of structures for wdfqueryinterface.h
//
typedef struct _WDF_QUERY_INTERFACE_CONFIG_V2_0 {
    //
    // Size of this structure in bytes.
    //
    ULONG Size;

    //
    // Interface to be returned to the caller.  Optional if BehaviorType is set
    // to WdfQueryInterfaceTypePassThrough or ImportInterface is set to TRUE.
    //
    PINTERFACE Interface;

    //
    // The GUID identifying the interface
    //
    CONST GUID * InterfaceType;

    //
    // Valid only for PDOs.  The framework will allocate a new request and
    // forward it down the parent's device stack.
    //
    BOOLEAN SendQueryToParentStack;

    //
    // Driver supplied callback which is called after some basic interface
    // validation has been performed (size, version, and guid checking).  This
    // is an optional parameter and may be NULL unless ImportInterface is
    // specified.
    //
    // If the callback returns !NT_SUCCESS, this error will be returned to the
    // caller and the query interface will fail.
    //
    // In this callback, the caller is free to modify the ExposedInterface in
    // any manner of its choosing.  For instance, the callback may change any
    // field in the interface.  The callback may also alloate a dynamic context
    // to be associated with the interface; the InterfaceReference and
    // InterfaceDereference functions may also be overridden.
    //
    // If ImportInterface is set to TRUE, then this is a required field and the
    // callback must initialize the interface (the framework will leave the
    // ExposedInterface buffer exactly as it received it) since the framework
    // has no way of knowing which fields to fill in and which to leave alone.
    //
    PFN_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST EvtDeviceProcessQueryInterfaceRequest;

    //
    // If TRUE, the interface provided by the caller contains data that the
    // driver is interested in.  By setting to this field to TRUE, the
    // EvtDeviceProcessQueryInterfaceRequest callback must initialize the
    // ExposedInterface.
    //
    // If FALSE, the entire ExposedInterface is initialized through a memory
    // copy before the EvtDeviceProcessQueryInterfaceRequest is called.
    //
    BOOLEAN ImportInterface;

} WDF_QUERY_INTERFACE_CONFIG_V2_0, *PWDF_QUERY_INTERFACE_CONFIG_V2_0;

// End of versioning of structures for wdfqueryinterface.h

//
// Versioning of structures for wdfregistry.h
//
// End of versioning of structures for wdfregistry.h

//
// Versioning of structures for wdfrequest.h
//
//
// This parameters structure allows general access to a requests parameters
//
typedef struct _WDF_REQUEST_PARAMETERS_V2_0 {
    USHORT Size;

    UCHAR MinorFunction;

    WDF_REQUEST_TYPE Type;

    //
    // The following user parameters are based on the service that is being
    // invoked.  Drivers and file systems can determine which set to use based
    // on the above major and minor function codes.
    //
    union {
        //
        // System service parameters for:  Create
        //
        //
        struct {
            PIO_SECURITY_CONTEXT SecurityContext;

            ULONG Options;

            USHORT POINTER_ALIGNMENT FileAttributes;

            USHORT ShareAccess;

            ULONG POINTER_ALIGNMENT EaLength;

        } Create;

        //
        // System service parameters for:  Read
        //
        //
        struct {
            size_t Length;

            ULONG POINTER_ALIGNMENT Key;

            LONGLONG DeviceOffset;

        } Read;

        //
        // System service parameters for:  Write
        //
        //
        struct {
            size_t Length;

            ULONG POINTER_ALIGNMENT Key;

            LONGLONG DeviceOffset;

        } Write;

        //
        // System service parameters for:  Device Control
        //
        // Note that the user's output buffer is stored in the UserBuffer field
        // and the user's input buffer is stored in the SystemBuffer field.
        //
        //
        struct {
            size_t OutputBufferLength;

            size_t POINTER_ALIGNMENT InputBufferLength;

            ULONG POINTER_ALIGNMENT IoControlCode;

            PVOID Type3InputBuffer;

        } DeviceIoControl;

        struct {
            PVOID Arg1;

            PVOID  Arg2;

            ULONG POINTER_ALIGNMENT IoControlCode;

            PVOID Arg4;

        } Others;

    } Parameters;

} WDF_REQUEST_PARAMETERS_V2_0, *PWDF_REQUEST_PARAMETERS_V2_0;

typedef struct _WDF_REQUEST_COMPLETION_PARAMS_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    WDF_REQUEST_TYPE Type;

    IO_STATUS_BLOCK IoStatus;

    union {
        struct {
            WDFMEMORY Buffer;

            size_t Length;

            size_t Offset;

        } Write;

        struct {
            WDFMEMORY Buffer;

            size_t Length;

            size_t Offset;

        } Read;

        struct {
            ULONG IoControlCode;

            struct {
                WDFMEMORY Buffer;

                size_t Offset;

            } Input;

            struct {
                WDFMEMORY Buffer;

                size_t Offset;

                size_t Length;

            } Output;

        } Ioctl;

        struct {
            union {
                PVOID Ptr;

                ULONG_PTR Value;

            } Argument1;

            union {
                PVOID Ptr;

                ULONG_PTR Value;

            } Argument2;

            union {
                PVOID Ptr;

                ULONG_PTR Value;

            } Argument3;

            union {
                PVOID Ptr;

                ULONG_PTR Value;

            } Argument4;

        } Others;

        struct {
            PWDF_USB_REQUEST_COMPLETION_PARAMS_V2_0 Completion;

        } Usb;

    } Parameters;

} WDF_REQUEST_COMPLETION_PARAMS_V2_0, *PWDF_REQUEST_COMPLETION_PARAMS_V2_0;

typedef struct _WDF_REQUEST_REUSE_PARAMS_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Bit field combination of WDF_REQUEST_REUSE_Xxx values
    //
    ULONG Flags;

    //
    // The new status of the request.
    //
    NTSTATUS Status;

    //
    // New PIRP  to be contained in the WDFREQUEST.   Setting a new PIRP value
    // is only valid for WDFREQUESTs created by WdfRequestCreateFromIrp where
    // RequestFreesIrp == FALSE.  No other WDFREQUESTs (presented by the
    // I/O queue for instance) may have their IRPs changed.
    //
    PIRP NewIrp;

} WDF_REQUEST_REUSE_PARAMS_V2_0, *PWDF_REQUEST_REUSE_PARAMS_V2_0;

typedef struct _WDF_REQUEST_SEND_OPTIONS_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Bit field combination of values from the WDF_REQUEST_SEND_OPTIONS_FLAGS
    // enumeration
    //
    ULONG Flags;

    //
    // Valid when WDF_REQUEST_SEND_OPTION_TIMEOUT is set
    //
    LONGLONG Timeout;

} WDF_REQUEST_SEND_OPTIONS_V2_0, *PWDF_REQUEST_SEND_OPTIONS_V2_0;

typedef struct _WDF_REQUEST_FORWARD_OPTIONS_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Bit field combination of values from the WDF_REQUEST_FORWARD_OPTIONS_FLAGS
    // enumeration
    //
    ULONG Flags;

} WDF_REQUEST_FORWARD_OPTIONS_V2_0, *PWDF_REQUEST_FORWARD_OPTIONS_V2_0;

// End of versioning of structures for wdfrequest.h

//
// Versioning of structures for wdfresource.h
//
// End of versioning of structures for wdfresource.h

//
// Versioning of structures for wdfroletypes.h
//
// End of versioning of structures for wdfroletypes.h

//
// Versioning of structures for wdfstring.h
//
// End of versioning of structures for wdfstring.h

//
// Versioning of structures for wdfsync.h
//
// End of versioning of structures for wdfsync.h

//
// Versioning of structures for wdftimer.h
//
typedef struct _WDF_TIMER_CONFIG_V2_0 {
    ULONG Size;

    PFN_WDF_TIMER EvtTimerFunc;

    ULONG Period;

    //
    // If this is TRUE, the Timer will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the Timer DPC (DISPATCH_LEVEL), or the request will fail.
    //
    BOOLEAN AutomaticSerialization;

    //
    // Optional tolerance for the timer in milliseconds.
    //
    ULONG TolerableDelay;

    //
    // If this is TRUE, high resolution timers will be used. The default
    // value is FALSE
    //
    DECLSPEC_ALIGN(8) BOOLEAN UseHighResolutionTimer;

} WDF_TIMER_CONFIG_V2_0, *PWDF_TIMER_CONFIG_V2_0;

// End of versioning of structures for wdftimer.h

//
// Versioning of structures for wdftriage.h
//
typedef struct _WDFOBJECT_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   RawObjectSize;

    ULONG   ObjectType;

    ULONG   TotalObjectSize;

    ULONG   ChildListHead;

    ULONG   ChildEntry;

    ULONG   Globals;

    ULONG   ParentObject;

} WDFOBJECT_TRIAGE_INFO_V2_0, *PWDFOBJECT_TRIAGE_INFO_V2_0;

typedef struct _WDFCONTEXT_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   HeaderSize;

    ULONG   NextHeader;

    ULONG   Object;

    ULONG   TypeInfoPtr;

    ULONG   Context;

} WDFCONTEXT_TRIAGE_INFO_V2_0, *PWDFCONTEXT_TRIAGE_INFO_V2_0;

typedef struct _WDFCONTEXTTYPE_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   TypeInfoSize;

    ULONG   ContextSize;

    ULONG   ContextName;

} WDFCONTEXTTYPE_TRIAGE_INFO_V2_0, *PWDFCONTEXTTYPE_TRIAGE_INFO_V2_0;

typedef struct _WDFQUEUE_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   QueueSize;

    ULONG   IrpQueue1;

    ULONG   IrpQueue2;

    ULONG   RequestList1;

    ULONG   RequestList2;

    ULONG   FwdProgressContext;

    ULONG   PkgIo;

} WDFQUEUE_TRIAGE_INFO_V2_0, *PWDFQUEUE_TRIAGE_INFO_V2_0;

typedef struct _WDFFWDPROGRESS_TRIAGE_INFO_V2_0 {
    ULONG   ReservedRequestList;

    ULONG   ReservedRequestInUseList;

    ULONG   PendedIrpList;

} WDFFWDPROGRESS_TRIAGE_INFO_V2_0, *PWDFFWDPROGRESS_TRIAGE_INFO_V2_0;

typedef struct _WDFIRPQUEUE_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   IrpQueueSize;

    ULONG   IrpListHeader;

    ULONG   IrpListEntry;

    ULONG   IrpContext;

} WDFIRPQUEUE_TRIAGE_INFO_V2_0, *PWDFIRPQUEUE_TRIAGE_INFO_V2_0;

typedef struct _WDFREQUEST_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   RequestSize;

    ULONG   CsqContext;

    //  WDF irp wrapper, see below.
    ULONG   FxIrp;

    ULONG   ListEntryQueueOwned;

    ULONG   ListEntryQueueOwned2;

    ULONG   RequestListEntry;

    ULONG   FwdProgressList;

} WDFREQUEST_TRIAGE_INFO_V2_0, *PWDFREQUEST_TRIAGE_INFO_V2_0;

typedef struct _WDFDEVICE_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   DeviceInitSize;

    ULONG   DeviceDriver;

} WDFDEVICE_TRIAGE_INFO_V2_0, *PWDFDEVICE_TRIAGE_INFO_V2_0;

typedef struct _WDFIRP_TRIAGE_INFO_V2_0 {
    //  value
    ULONG   FxIrpSize;

    ULONG   IrpPtr;

} WDFIRP_TRIAGE_INFO_V2_0, *PWDFIRP_TRIAGE_INFO_V2_0;

typedef struct _WDF_TRIAGE_INFO_V2_0 {
    //
    // Version.
    //
    ULONG                       WdfMajorVersion;

    ULONG                       WdfMinorVersion;

    ULONG                       TriageInfoMajorVersion;

    ULONG                       TriageInfoMinorVersion;

    //
    // Reserved pointer.
    //
    PVOID                       Reserved;

    //
    // WDF objects triage info.
    //
    PWDFOBJECT_TRIAGE_INFO_V2_0      WdfObjectTriageInfo;

    PWDFCONTEXT_TRIAGE_INFO_V2_0     WdfContextTriageInfo;

    PWDFCONTEXTTYPE_TRIAGE_INFO_V2_0 WdfContextTypeTriageInfo;

    PWDFQUEUE_TRIAGE_INFO_V2_0       WdfQueueTriageInfo;

    PWDFFWDPROGRESS_TRIAGE_INFO_V2_0 WdfFwdProgressTriageInfo;

    PWDFIRPQUEUE_TRIAGE_INFO_V2_0    WdfIrpQueueTriageInfo;

    PWDFREQUEST_TRIAGE_INFO_V2_0     WdfRequestTriageInfo;

    PWDFDEVICE_TRIAGE_INFO_V2_0      WdfDeviceTriageInfo;

    PWDFIRP_TRIAGE_INFO_V2_0         WdfIrpTriageInfo;

} WDF_TRIAGE_INFO_V2_0, *PWDF_TRIAGE_INFO_V2_0;

// End of versioning of structures for wdftriage.h

//
// Versioning of structures for wdftypes.h
//
// End of versioning of structures for wdftypes.h

//
// Versioning of structures for wdfUsb.h
//
typedef struct _WDF_USB_REQUEST_COMPLETION_PARAMS_V2_0 {
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

} WDF_USB_REQUEST_COMPLETION_PARAMS_V2_0, *PWDF_USB_REQUEST_COMPLETION_PARAMS_V2_0;

typedef struct _WDF_USB_CONTINUOUS_READER_CONFIG_V2_0 {
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
    PWDF_OBJECT_ATTRIBUTES_V2_0 BufferAttributes;

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

} WDF_USB_CONTINUOUS_READER_CONFIG_V2_0, *PWDF_USB_CONTINUOUS_READER_CONFIG_V2_0;

typedef struct _WDF_USB_DEVICE_INFORMATION_V2_0 {
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

} WDF_USB_DEVICE_INFORMATION_V2_0, *PWDF_USB_DEVICE_INFORMATION_V2_0;

typedef struct _WDF_USB_INTERFACE_SETTING_PAIR_V2_0 {
    //
    // Interface to select
    //
    WDFUSBINTERFACE UsbInterface;

    //
    // Setting to select on UsbInterface
    //
    UCHAR SettingIndex;

} WDF_USB_INTERFACE_SETTING_PAIR_V2_0, *PWDF_USB_INTERFACE_SETTING_PAIR_V2_0;

typedef struct _WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V2_0 {
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
            PWDF_USB_INTERFACE_SETTING_PAIR_V2_0 Pairs;

            //
            // Number of interfaces which were configured after a successful call
            //
            UCHAR NumberOfConfiguredInterfaces;

        } MultiInterface;

    } Types;

} WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V2_0, *PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V2_0;

typedef struct _WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V2_0 {
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

} WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V2_0, *PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V2_0;

typedef struct _WDF_USB_PIPE_INFORMATION_V2_0 {
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

} WDF_USB_PIPE_INFORMATION_V2_0, *PWDF_USB_PIPE_INFORMATION_V2_0;

typedef struct _WDF_USB_DEVICE_CREATE_CONFIG_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG   Size;

    //
    // USBD Client Contraction of the Wdf Client
    //
    ULONG   USBDClientContractVersion;

} WDF_USB_DEVICE_CREATE_CONFIG_V2_0, *PWDF_USB_DEVICE_CREATE_CONFIG_V2_0;

// End of versioning of structures for wdfUsb.h

//
// Versioning of structures for wdfverifier.h
//
// End of versioning of structures for wdfverifier.h

//
// Versioning of structures for wdfWMI.h
//
typedef struct _WDF_WMI_PROVIDER_CONFIG_V2_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // The GUID being registered
    //
    GUID Guid;

    //
    // Combination of values from the enum WDF_WMI_PROVIDER_FLAGS
    //
    ULONG Flags;

    //
    // Minimum expected buffer size for query and set instance requests.
    // Ignored if WdfWmiProviderEventOnly is set in Flags.
    //
    ULONG MinInstanceBufferSize;

    //
    // Callback when caller is opening a provider which ha been marked as
    // expensive or when a caller is interested in events.
    //
    PFN_WDF_WMI_PROVIDER_FUNCTION_CONTROL EvtWmiProviderFunctionControl;

} WDF_WMI_PROVIDER_CONFIG_V2_0, *PWDF_WMI_PROVIDER_CONFIG_V2_0;

typedef struct _WDF_WMI_INSTANCE_CONFIG_V2_0 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Optional parameter.  If NULL, ProviderConfig must be set to a valid pointer
    // value.   If specified, indicates the provider to create an instance for.
    //
    WDFWMIPROVIDER Provider;

    //
    // Optional parameter.  If NULL, Provider must be set to a valid handle
    // value.  If specifeid, indicates the configuration for a provider to be
    // created and for this instance to be associated with.
    //
    PWDF_WMI_PROVIDER_CONFIG_V2_0 ProviderConfig;

    //
    // If the Provider is configured as read only and this field is set to TRUE,
    // the EvtWmiInstanceQueryInstance is ignored and WDF will blindly copy the
    // context associated with this instance (using RtlCopyMemory, with no locks
    // held) into the query buffer.
    //
    BOOLEAN UseContextForQuery;

    //
    // If TRUE, the instance will be registered as well as created.
    //
    BOOLEAN Register;

    //
    // Callback when caller wants to query the entire data item's buffer.
    //
    PFN_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceQueryInstance;

    //
    // Callback when caller wants to set the entire data item's buffer.
    //
    PFN_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceSetInstance;

    //
    // Callback when caller wants to set a single field in the data item's buffer
    //
    PFN_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceSetItem;

    //
    // Callback when caller wants to execute a method on the data item.
    //
    PFN_WDF_WMI_INSTANCE_EXECUTE_METHOD EvtWmiInstanceExecuteMethod;

} WDF_WMI_INSTANCE_CONFIG_V2_0, *PWDF_WMI_INSTANCE_CONFIG_V2_0;

// End of versioning of structures for wdfWMI.h

//
// Versioning of structures for wdfworkitem.h
//
typedef struct _WDF_WORKITEM_CONFIG_V2_0 {
    ULONG Size;

    PFN_WDF_WORKITEM EvtWorkItemFunc;

    //
    // If this is TRUE, the workitem will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the work item (PASSIVE_LEVEL), or the request will fail.
    //
    BOOLEAN AutomaticSerialization;

} WDF_WORKITEM_CONFIG_V2_0, *PWDF_WORKITEM_CONFIG_V2_0;

// End of versioning of structures for wdfworkitem.h


#endif // _WDF_V2_0_TYPES_H_

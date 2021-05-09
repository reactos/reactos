//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _WDF_V1_5_TYPES_H_
#define _WDF_V1_5_TYPES_H_


typedef enum _WDFFUNCENUM_V1_5 {
    WdfFunctionTableNumEntries_V1_5 = 387,
} WDFFUNCENUM_V1_5;

typedef struct _WDF_POWER_ROUTINE_TIMED_OUT_DATA_V1_5 *PWDF_POWER_ROUTINE_TIMED_OUT_DATA_V1_5;
typedef const struct _WDF_POWER_ROUTINE_TIMED_OUT_DATA_V1_5 *PCWDF_POWER_ROUTINE_TIMED_OUT_DATA_V1_5;
typedef struct _WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V1_5 *PWDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V1_5;
typedef const struct _WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V1_5 *PCWDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V1_5;
typedef struct _WDF_QUEUE_FATAL_ERROR_DATA_V1_5 *PWDF_QUEUE_FATAL_ERROR_DATA_V1_5;
typedef const struct _WDF_QUEUE_FATAL_ERROR_DATA_V1_5 *PCWDF_QUEUE_FATAL_ERROR_DATA_V1_5;
typedef struct _WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5 *PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5;
typedef const struct _WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5 *PCWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5;
typedef struct _WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5 *PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5;
typedef const struct _WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5 *PCWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5;
typedef struct _WDF_CHILD_RETRIEVE_INFO_V1_5 *PWDF_CHILD_RETRIEVE_INFO_V1_5;
typedef const struct _WDF_CHILD_RETRIEVE_INFO_V1_5 *PCWDF_CHILD_RETRIEVE_INFO_V1_5;
typedef struct _WDF_CHILD_LIST_CONFIG_V1_5 *PWDF_CHILD_LIST_CONFIG_V1_5;
typedef const struct _WDF_CHILD_LIST_CONFIG_V1_5 *PCWDF_CHILD_LIST_CONFIG_V1_5;
typedef struct _WDF_CHILD_LIST_ITERATOR_V1_5 *PWDF_CHILD_LIST_ITERATOR_V1_5;
typedef const struct _WDF_CHILD_LIST_ITERATOR_V1_5 *PCWDF_CHILD_LIST_ITERATOR_V1_5;
typedef struct _WDF_CLASS_EXTENSION_DESCRIPTOR_V1_5 *PWDF_CLASS_EXTENSION_DESCRIPTOR_V1_5;
typedef const struct _WDF_CLASS_EXTENSION_DESCRIPTOR_V1_5 *PCWDF_CLASS_EXTENSION_DESCRIPTOR_V1_5;
typedef struct _WDF_COMMON_BUFFER_CONFIG_V1_5 *PWDF_COMMON_BUFFER_CONFIG_V1_5;
typedef const struct _WDF_COMMON_BUFFER_CONFIG_V1_5 *PCWDF_COMMON_BUFFER_CONFIG_V1_5;
typedef struct _WDF_FILEOBJECT_CONFIG_V1_5 *PWDF_FILEOBJECT_CONFIG_V1_5;
typedef const struct _WDF_FILEOBJECT_CONFIG_V1_5 *PCWDF_FILEOBJECT_CONFIG_V1_5;
typedef struct _WDF_DEVICE_PNP_NOTIFICATION_DATA_V1_5 *PWDF_DEVICE_PNP_NOTIFICATION_DATA_V1_5;
typedef const struct _WDF_DEVICE_PNP_NOTIFICATION_DATA_V1_5 *PCWDF_DEVICE_PNP_NOTIFICATION_DATA_V1_5;
typedef struct _WDF_DEVICE_POWER_NOTIFICATION_DATA_V1_5 *PWDF_DEVICE_POWER_NOTIFICATION_DATA_V1_5;
typedef const struct _WDF_DEVICE_POWER_NOTIFICATION_DATA_V1_5 *PCWDF_DEVICE_POWER_NOTIFICATION_DATA_V1_5;
typedef struct _WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V1_5 *PWDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V1_5;
typedef const struct _WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V1_5 *PCWDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V1_5;
typedef struct _WDF_PNPPOWER_EVENT_CALLBACKS_V1_5 *PWDF_PNPPOWER_EVENT_CALLBACKS_V1_5;
typedef const struct _WDF_PNPPOWER_EVENT_CALLBACKS_V1_5 *PCWDF_PNPPOWER_EVENT_CALLBACKS_V1_5;
typedef struct _WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5 *PWDF_POWER_POLICY_EVENT_CALLBACKS_V1_5;
typedef const struct _WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5 *PCWDF_POWER_POLICY_EVENT_CALLBACKS_V1_5;
typedef struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_5 *PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_5;
typedef const struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_5 *PCWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_5;
typedef struct _WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5 *PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5;
typedef const struct _WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5 *PCWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5;
typedef struct _WDF_DEVICE_STATE_V1_5 *PWDF_DEVICE_STATE_V1_5;
typedef const struct _WDF_DEVICE_STATE_V1_5 *PCWDF_DEVICE_STATE_V1_5;
typedef struct _WDF_DEVICE_PNP_CAPABILITIES_V1_5 *PWDF_DEVICE_PNP_CAPABILITIES_V1_5;
typedef const struct _WDF_DEVICE_PNP_CAPABILITIES_V1_5 *PCWDF_DEVICE_PNP_CAPABILITIES_V1_5;
typedef struct _WDF_DEVICE_POWER_CAPABILITIES_V1_5 *PWDF_DEVICE_POWER_CAPABILITIES_V1_5;
typedef const struct _WDF_DEVICE_POWER_CAPABILITIES_V1_5 *PCWDF_DEVICE_POWER_CAPABILITIES_V1_5;
typedef struct _WDF_DMA_ENABLER_CONFIG_V1_5 *PWDF_DMA_ENABLER_CONFIG_V1_5;
typedef const struct _WDF_DMA_ENABLER_CONFIG_V1_5 *PCWDF_DMA_ENABLER_CONFIG_V1_5;
typedef struct _WDF_DPC_CONFIG_V1_5 *PWDF_DPC_CONFIG_V1_5;
typedef const struct _WDF_DPC_CONFIG_V1_5 *PCWDF_DPC_CONFIG_V1_5;
typedef struct _WDF_DRIVER_CONFIG_V1_5 *PWDF_DRIVER_CONFIG_V1_5;
typedef const struct _WDF_DRIVER_CONFIG_V1_5 *PCWDF_DRIVER_CONFIG_V1_5;
typedef struct _WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V1_5 *PWDF_DRIVER_VERSION_AVAILABLE_PARAMS_V1_5;
typedef const struct _WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V1_5 *PCWDF_DRIVER_VERSION_AVAILABLE_PARAMS_V1_5;
typedef struct _WDF_FDO_EVENT_CALLBACKS_V1_5 *PWDF_FDO_EVENT_CALLBACKS_V1_5;
typedef const struct _WDF_FDO_EVENT_CALLBACKS_V1_5 *PCWDF_FDO_EVENT_CALLBACKS_V1_5;
typedef struct _WDF_DRIVER_GLOBALS_V1_5 *PWDF_DRIVER_GLOBALS_V1_5;
typedef const struct _WDF_DRIVER_GLOBALS_V1_5 *PCWDF_DRIVER_GLOBALS_V1_5;
typedef struct _WDF_INTERRUPT_CONFIG_V1_5 *PWDF_INTERRUPT_CONFIG_V1_5;
typedef const struct _WDF_INTERRUPT_CONFIG_V1_5 *PCWDF_INTERRUPT_CONFIG_V1_5;
typedef struct _WDF_INTERRUPT_INFO_V1_5 *PWDF_INTERRUPT_INFO_V1_5;
typedef const struct _WDF_INTERRUPT_INFO_V1_5 *PCWDF_INTERRUPT_INFO_V1_5;
typedef struct _WDF_IO_QUEUE_CONFIG_V1_5 *PWDF_IO_QUEUE_CONFIG_V1_5;
typedef const struct _WDF_IO_QUEUE_CONFIG_V1_5 *PCWDF_IO_QUEUE_CONFIG_V1_5;
typedef struct _WDF_IO_TARGET_OPEN_PARAMS_V1_5 *PWDF_IO_TARGET_OPEN_PARAMS_V1_5;
typedef const struct _WDF_IO_TARGET_OPEN_PARAMS_V1_5 *PCWDF_IO_TARGET_OPEN_PARAMS_V1_5;
typedef struct _WDFMEMORY_OFFSET_V1_5 *PWDFMEMORY_OFFSET_V1_5;
typedef const struct _WDFMEMORY_OFFSET_V1_5 *PCWDFMEMORY_OFFSET_V1_5;
typedef struct _WDF_MEMORY_DESCRIPTOR_V1_5 *PWDF_MEMORY_DESCRIPTOR_V1_5;
typedef const struct _WDF_MEMORY_DESCRIPTOR_V1_5 *PCWDF_MEMORY_DESCRIPTOR_V1_5;
typedef struct _WDF_OBJECT_ATTRIBUTES_V1_5 *PWDF_OBJECT_ATTRIBUTES_V1_5;
typedef const struct _WDF_OBJECT_ATTRIBUTES_V1_5 *PCWDF_OBJECT_ATTRIBUTES_V1_5;
typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V1_5 *PWDF_OBJECT_CONTEXT_TYPE_INFO_V1_5;
typedef const struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V1_5 *PCWDF_OBJECT_CONTEXT_TYPE_INFO_V1_5;
typedef struct _WDF_PDO_EVENT_CALLBACKS_V1_5 *PWDF_PDO_EVENT_CALLBACKS_V1_5;
typedef const struct _WDF_PDO_EVENT_CALLBACKS_V1_5 *PCWDF_PDO_EVENT_CALLBACKS_V1_5;
typedef struct _WDF_QUERY_INTERFACE_CONFIG_V1_5 *PWDF_QUERY_INTERFACE_CONFIG_V1_5;
typedef const struct _WDF_QUERY_INTERFACE_CONFIG_V1_5 *PCWDF_QUERY_INTERFACE_CONFIG_V1_5;
typedef struct _WDF_REQUEST_PARAMETERS_V1_5 *PWDF_REQUEST_PARAMETERS_V1_5;
typedef const struct _WDF_REQUEST_PARAMETERS_V1_5 *PCWDF_REQUEST_PARAMETERS_V1_5;
typedef struct _WDF_REQUEST_COMPLETION_PARAMS_V1_5 *PWDF_REQUEST_COMPLETION_PARAMS_V1_5;
typedef const struct _WDF_REQUEST_COMPLETION_PARAMS_V1_5 *PCWDF_REQUEST_COMPLETION_PARAMS_V1_5;
typedef struct _WDF_REQUEST_REUSE_PARAMS_V1_5 *PWDF_REQUEST_REUSE_PARAMS_V1_5;
typedef const struct _WDF_REQUEST_REUSE_PARAMS_V1_5 *PCWDF_REQUEST_REUSE_PARAMS_V1_5;
typedef struct _WDF_REQUEST_SEND_OPTIONS_V1_5 *PWDF_REQUEST_SEND_OPTIONS_V1_5;
typedef const struct _WDF_REQUEST_SEND_OPTIONS_V1_5 *PCWDF_REQUEST_SEND_OPTIONS_V1_5;
typedef struct _WDF_TIMER_CONFIG_V1_5 *PWDF_TIMER_CONFIG_V1_5;
typedef const struct _WDF_TIMER_CONFIG_V1_5 *PCWDF_TIMER_CONFIG_V1_5;
typedef struct _WDF_USB_REQUEST_COMPLETION_PARAMS_V1_5 *PWDF_USB_REQUEST_COMPLETION_PARAMS_V1_5;
typedef const struct _WDF_USB_REQUEST_COMPLETION_PARAMS_V1_5 *PCWDF_USB_REQUEST_COMPLETION_PARAMS_V1_5;
typedef struct _WDF_USB_CONTINUOUS_READER_CONFIG_V1_5 *PWDF_USB_CONTINUOUS_READER_CONFIG_V1_5;
typedef const struct _WDF_USB_CONTINUOUS_READER_CONFIG_V1_5 *PCWDF_USB_CONTINUOUS_READER_CONFIG_V1_5;
typedef struct _WDF_USB_DEVICE_INFORMATION_V1_5 *PWDF_USB_DEVICE_INFORMATION_V1_5;
typedef const struct _WDF_USB_DEVICE_INFORMATION_V1_5 *PCWDF_USB_DEVICE_INFORMATION_V1_5;
typedef struct _WDF_USB_INTERFACE_SETTING_PAIR_V1_5 *PWDF_USB_INTERFACE_SETTING_PAIR_V1_5;
typedef const struct _WDF_USB_INTERFACE_SETTING_PAIR_V1_5 *PCWDF_USB_INTERFACE_SETTING_PAIR_V1_5;
typedef struct _WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V1_5 *PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V1_5;
typedef const struct _WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V1_5 *PCWDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V1_5;
typedef struct _WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V1_5 *PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V1_5;
typedef const struct _WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V1_5 *PCWDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V1_5;
typedef struct _WDF_USB_PIPE_INFORMATION_V1_5 *PWDF_USB_PIPE_INFORMATION_V1_5;
typedef const struct _WDF_USB_PIPE_INFORMATION_V1_5 *PCWDF_USB_PIPE_INFORMATION_V1_5;
typedef struct _WDF_WMI_PROVIDER_CONFIG_V1_5 *PWDF_WMI_PROVIDER_CONFIG_V1_5;
typedef const struct _WDF_WMI_PROVIDER_CONFIG_V1_5 *PCWDF_WMI_PROVIDER_CONFIG_V1_5;
typedef struct _WDF_WMI_INSTANCE_CONFIG_V1_5 *PWDF_WMI_INSTANCE_CONFIG_V1_5;
typedef const struct _WDF_WMI_INSTANCE_CONFIG_V1_5 *PCWDF_WMI_INSTANCE_CONFIG_V1_5;
typedef struct _WDF_WORKITEM_CONFIG_V1_5 *PWDF_WORKITEM_CONFIG_V1_5;
typedef const struct _WDF_WORKITEM_CONFIG_V1_5 *PCWDF_WORKITEM_CONFIG_V1_5;

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
typedef struct _WDF_POWER_ROUTINE_TIMED_OUT_DATA_V1_5 {
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

} WDF_POWER_ROUTINE_TIMED_OUT_DATA_V1_5;

typedef struct _WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V1_5 {
    WDFREQUEST Request;

    PIRP Irp;

    ULONG OutputBufferLength;

    ULONG_PTR Information;

    UCHAR MajorFunction;

} WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V1_5, *PWDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA_V1_5;

typedef struct _WDF_QUEUE_FATAL_ERROR_DATA_V1_5 {
    WDFQUEUE Queue;

    WDFREQUEST Request;

    NTSTATUS Status;

} WDF_QUEUE_FATAL_ERROR_DATA_V1_5, *PWDF_QUEUE_FATAL_ERROR_DATA_V1_5;

// End of versioning of structures for wdfbugcodes.h

//
// Versioning of structures for wdfchildlist.h
//
typedef struct _WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5 {
    //
    // Size in bytes of the entire description, including this header.
    //
    // Same value as WDF_CHILD_LIST_CONFIG::IdentificationDescriptionSize
    // Used as a sanity check.
    //
    ULONG IdentificationDescriptionSize;

} WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5, *PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5;

typedef struct _WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5 {
    //
    // Size in bytes of the entire description, including this header.
    //
    // Same value as WDF_CHILD_LIST_CONFIG::AddressDescriptionSize
    // Used as a sanity check.
    //
    ULONG AddressDescriptionSize;

} WDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5, *PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5;

typedef struct _WDF_CHILD_RETRIEVE_INFO_V1_5 {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Must be a valid pointer when passed in, copied into upon success
    //
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_V1_5 IdentificationDescription;

    //
    // Optional pointer when passed in, copied into upon success
    //
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER_V1_5 AddressDescription;

    //
    // Status of the returned device
    //
    WDF_CHILD_LIST_RETRIEVE_DEVICE_STATUS Status;

    //
    // If provided, will be used for searching through the list of devices
    // instead of the default list ID compare function
    //
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE EvtChildListIdentificationDescriptionCompare;

} WDF_CHILD_RETRIEVE_INFO_V1_5, *PWDF_CHILD_RETRIEVE_INFO_V1_5;

typedef struct _WDF_CHILD_LIST_CONFIG_V1_5 {
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

} WDF_CHILD_LIST_CONFIG_V1_5, *PWDF_CHILD_LIST_CONFIG_V1_5;

typedef struct _WDF_CHILD_LIST_ITERATOR_V1_5 {
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

} WDF_CHILD_LIST_ITERATOR_V1_5, *PWDF_CHILD_LIST_ITERATOR_V1_5;

// End of versioning of structures for wdfchildlist.h

//
// Versioning of structures for wdfClassExtension.h
//
typedef struct _WDF_CLASS_EXTENSION_DESCRIPTOR_V1_5 {
    PCWDF_CLASS_EXTENSION_DESCRIPTOR_V1_5   Next;

    ULONG                             Size;

    PFN_WDF_CLASS_EXTENSIONIN_BIND    Bind;

    PFN_WDF_CLASS_EXTENSIONIN_UNBIND  Unbind;

} WDF_CLASS_EXTENSION_DESCRIPTOR_V1_5, *PWDF_CLASS_EXTENSION_DESCRIPTOR_V1_5;

// End of versioning of structures for wdfClassExtension.h

//
// Versioning of structures for wdfClassExtensionList.h
//
// End of versioning of structures for wdfClassExtensionList.h

//
// Versioning of structures for wdfcollection.h
//
// End of versioning of structures for wdfcollection.h

//
// Versioning of structures for wdfCommonBuffer.h
//
typedef struct _WDF_COMMON_BUFFER_CONFIG_V1_5 {
    //
    // Size of this structure in bytes
    //
    ULONG   Size;

    //
    // Alignment requirement of the buffer address
    //
    ULONG   AlignmentRequirement;

} WDF_COMMON_BUFFER_CONFIG_V1_5, *PWDF_COMMON_BUFFER_CONFIG_V1_5;

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
// Versioning of structures for wdfDevice.h
//
typedef struct _WDF_FILEOBJECT_CONFIG_V1_5 {
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

} WDF_FILEOBJECT_CONFIG_V1_5, *PWDF_FILEOBJECT_CONFIG_V1_5;

typedef struct _WDF_DEVICE_PNP_NOTIFICATION_DATA_V1_5 {
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

} WDF_DEVICE_PNP_NOTIFICATION_DATA_V1_5;

typedef struct _WDF_DEVICE_POWER_NOTIFICATION_DATA_V1_5 {
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

} WDF_DEVICE_POWER_NOTIFICATION_DATA_V1_5;

typedef struct _WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V1_5 {
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

} WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA_V1_5;

typedef struct _WDF_PNPPOWER_EVENT_CALLBACKS_V1_5 {
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

} WDF_PNPPOWER_EVENT_CALLBACKS_V1_5, *PWDF_PNPPOWER_EVENT_CALLBACKS_V1_5;

typedef struct _WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5 {
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

} WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5, *PWDF_POWER_POLICY_EVENT_CALLBACKS_V1_5;

typedef struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_5 {
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

} WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_5, *PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_5;

typedef struct _WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5 {
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

} WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5, *PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5;

typedef struct _WDF_DEVICE_STATE_V1_5 {
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

} WDF_DEVICE_STATE_V1_5, *PWDF_DEVICE_STATE_V1_5;

typedef struct _WDF_DEVICE_PNP_CAPABILITIES_V1_5 {
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

} WDF_DEVICE_PNP_CAPABILITIES_V1_5, *PWDF_DEVICE_PNP_CAPABILITIES_V1_5;

typedef struct _WDF_DEVICE_POWER_CAPABILITIES_V1_5 {
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

} WDF_DEVICE_POWER_CAPABILITIES_V1_5, *PWDF_DEVICE_POWER_CAPABILITIES_V1_5;

// End of versioning of structures for wdfDevice.h

//
// Versioning of structures for wdfDmaEnabler.h
//
typedef struct _WDF_DMA_ENABLER_CONFIG_V1_5 {
    //
    // Size of this structure in bytes
    //
    ULONG                Size;

    //
    // One of the above WDF_DMA_PROFILES
    //
    WDF_DMA_PROFILE      Profile;

    //
    // Maximum DMA Transfer handled in bytes.
    //
    size_t               MaximumLength;

    //
    // The various DMA PnP/Power event callbacks
    //
    PFN_WDF_DMA_ENABLER_FILL                  EvtDmaEnablerFill;

    PFN_WDF_DMA_ENABLER_FLUSH                 EvtDmaEnablerFlush;

    PFN_WDF_DMA_ENABLER_DISABLE               EvtDmaEnablerDisable;

    PFN_WDF_DMA_ENABLER_ENABLE                EvtDmaEnablerEnable;

    PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_START  EvtDmaEnablerSelfManagedIoStart;

    PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_STOP   EvtDmaEnablerSelfManagedIoStop;

} WDF_DMA_ENABLER_CONFIG_V1_5, *PWDF_DMA_ENABLER_CONFIG_V1_5;

// End of versioning of structures for wdfDmaEnabler.h

//
// Versioning of structures for wdfDmaTransaction.h
//
// End of versioning of structures for wdfDmaTransaction.h

//
// Versioning of structures for wdfdpc.h
//
typedef struct _WDF_DPC_CONFIG_V1_5 {
    ULONG       Size;

    PFN_WDF_DPC EvtDpcFunc;

    //
    // If this is TRUE, the DPC will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the DPC (DISPATCH_LEVEL), or the request will fail.
    //
    BOOLEAN     AutomaticSerialization;

} WDF_DPC_CONFIG_V1_5, *PWDF_DPC_CONFIG_V1_5;

// End of versioning of structures for wdfdpc.h

//
// Versioning of structures for wdfdriver.h
//
typedef struct _WDF_DRIVER_CONFIG_V1_5 {
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

} WDF_DRIVER_CONFIG_V1_5, *PWDF_DRIVER_CONFIG_V1_5;

typedef struct _WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V1_5 {
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

} WDF_DRIVER_VERSION_AVAILABLE_PARAMS_V1_5, *PWDF_DRIVER_VERSION_AVAILABLE_PARAMS_V1_5;

// End of versioning of structures for wdfdriver.h

//
// Versioning of structures for wdffdo.h
//
typedef struct _WDF_FDO_EVENT_CALLBACKS_V1_5 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    PFN_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS EvtDeviceFilterAddResourceRequirements;

    PFN_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS EvtDeviceFilterRemoveResourceRequirements;

    PFN_WDF_DEVICE_REMOVE_ADDED_RESOURCES EvtDeviceRemoveAddedResources;

} WDF_FDO_EVENT_CALLBACKS_V1_5, *PWDF_FDO_EVENT_CALLBACKS_V1_5;

// End of versioning of structures for wdffdo.h

//
// Versioning of structures for wdffileobject.h
//
// End of versioning of structures for wdffileobject.h

//
// Versioning of structures for wdfGlobals.h
//
typedef struct _WDF_DRIVER_GLOBALS_V1_5 {
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

} WDF_DRIVER_GLOBALS_V1_5, *PWDF_DRIVER_GLOBALS_V1_5;

// End of versioning of structures for wdfGlobals.h

//
// Versioning of structures for wdfinstaller.h
//
// End of versioning of structures for wdfinstaller.h

//
// Versioning of structures for wdfinterrupt.h
//
//
// Interrupt Configuration Structure
//
typedef struct _WDF_INTERRUPT_CONFIG_V1_5 {
    ULONG              Size;

    //
    // If this interrupt is to be synchronized with other interrupt(s) assigned
    // to the same WDFDEVICE, create a WDFSPINLOCK and assign it to each of the
    // WDFINTERRUPTs config.
    //
    WDFSPINLOCK        SpinLock;

    WDF_TRI_STATE      ShareVector;

    BOOLEAN            FloatingSave;

    //
    // Automatic Serialization of the DpcForIsr
    //
    BOOLEAN            AutomaticSerialization;

    // Event Callbacks
    PFN_WDF_INTERRUPT_ISR         EvtInterruptIsr;

    PFN_WDF_INTERRUPT_DPC         EvtInterruptDpc;

    PFN_WDF_INTERRUPT_ENABLE      EvtInterruptEnable;

    PFN_WDF_INTERRUPT_DISABLE     EvtInterruptDisable;

} WDF_INTERRUPT_CONFIG_V1_5, *PWDF_INTERRUPT_CONFIG_V1_5;

typedef struct _WDF_INTERRUPT_INFO_V1_5 {
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

} WDF_INTERRUPT_INFO_V1_5, *PWDF_INTERRUPT_INFO_V1_5;

// End of versioning of structures for wdfinterrupt.h

//
// Versioning of structures for wdfio.h
//
//
// This is the structure used to configure an IoQueue and
// register callback events to it.
//
//
typedef struct _WDF_IO_QUEUE_CONFIG_V1_5 {
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

} WDF_IO_QUEUE_CONFIG_V1_5, *PWDF_IO_QUEUE_CONFIG_V1_5;

// End of versioning of structures for wdfio.h

//
// Versioning of structures for wdfIoTarget.h
//
typedef struct _WDF_IO_TARGET_OPEN_PARAMS_V1_5 {
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

    // ========== WdfIoTargetOpenUseExistingDevice begin ==========
    //
    // The device object to send requests to
    //
    PDEVICE_OBJECT TargetDeviceObject;

    //
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

    //
    // The access desired on the device being opened up, ie WDM FILE_XXX_ACCESS
    // such as FILE_ANY_ACCESS, FILE_SPECIAL_ACCESS, FILE_READ_ACCESS, or
    // FILE_WRITE_ACCESS or you can use values such as GENERIC_READ,
    // GENERIC_WRITE, or GENERIC_ALL.
    //
    ACCESS_MASK DesiredAccess;

    //
    // Share access desired on the target being opened, ie WDM FILE_SHARE_XXX
    // values such as FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE.
    //
    // A zero value means exclusive access to the target.
    //
    ULONG ShareAccess;

    //
    // File  attributes, see ZwCreateFile in the DDK for a list of valid
    // values and their meaning.
    //
    ULONG FileAttributes;

    //
    // Create disposition, see ZwCreateFile in the DDK for a list of valid
    // values and their meaning.
    //
    ULONG CreateDisposition;

    //
    // Options for opening the device, see CreateOptions for ZwCreateFile in the
    // DDK for a list of valid values and their meaning.
    //
    ULONG CreateOptions;

    PVOID EaBuffer;

    ULONG EaBufferLength;

    PLONGLONG AllocationSize;

    // ========== WdfIoTargetOpenByName end ==========
    //
    //
    // On return for a create by name, this will contain one of the following
    // values:  FILE_CREATED, FILE_OPENED, FILE_OVERWRITTEN, FILE_SUPERSEDED,
    // FILE_EXISTS, FILE_DOES_NOT_EXIST
    //
    ULONG FileInformation;

} WDF_IO_TARGET_OPEN_PARAMS_V1_5, *PWDF_IO_TARGET_OPEN_PARAMS_V1_5;

// End of versioning of structures for wdfIoTarget.h

//
// Versioning of structures for wdfMemory.h
//
typedef struct _WDFMEMORY_OFFSET_V1_5 {
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

} WDFMEMORY_OFFSET_V1_5, *PWDFMEMORY_OFFSET_V1_5;

typedef struct _WDF_MEMORY_DESCRIPTOR_V1_5 {



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

            PWDFMEMORY_OFFSET_V1_5 Offsets;

        } HandleType;

    } u;

} WDF_MEMORY_DESCRIPTOR_V1_5, *PWDF_MEMORY_DESCRIPTOR_V1_5;

// End of versioning of structures for wdfMemory.h

//
// Versioning of structures for wdfMiniport.h
//
// End of versioning of structures for wdfMiniport.h

//
// Versioning of structures for wdfObject.h
//
typedef struct _WDF_OBJECT_ATTRIBUTES_V1_5 {
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
    WDF_EXECUTION_LEVEL            ExecutionLevel;

    //
    // Synchronization level constraint for Object
    //
    WDF_SYNCHRONIZATION_SCOPE        SynchronizationScope;

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
    PCWDF_OBJECT_CONTEXT_TYPE_INFO_V1_5 ContextTypeInfo;

} WDF_OBJECT_ATTRIBUTES_V1_5, *PWDF_OBJECT_ATTRIBUTES_V1_5;

//
// Since C does not have strong type checking we must invent our own
//
typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V1_5 {
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
    PCWDF_OBJECT_CONTEXT_TYPE_INFO_V1_5 UniqueType;

    //
    // Function pointer to retrieve the context type information structure
    // pointer from the provider of the context type.  This function is invoked
    // by the client driver's entry point by the KMDF stub after all class
    // drivers are loaded and before DriverEntry is invoked.
    //
    PFN_GET_UNIQUE_CONTEXT_TYPE EvtDriverGetUniqueContextType;

} WDF_OBJECT_CONTEXT_TYPE_INFO_V1_5, *PWDF_OBJECT_CONTEXT_TYPE_INFO_V1_5;

// End of versioning of structures for wdfObject.h

//
// Versioning of structures for wdfpdo.h
//
typedef struct _WDF_PDO_EVENT_CALLBACKS_V1_5 {
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

} WDF_PDO_EVENT_CALLBACKS_V1_5, *PWDF_PDO_EVENT_CALLBACKS_V1_5;

// End of versioning of structures for wdfpdo.h

//
// Versioning of structures for wdfpool.h
//
// End of versioning of structures for wdfpool.h

//
// Versioning of structures for wdfqueryinterface.h
//
typedef struct _WDF_QUERY_INTERFACE_CONFIG_V1_5 {
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

} WDF_QUERY_INTERFACE_CONFIG_V1_5, *PWDF_QUERY_INTERFACE_CONFIG_V1_5;

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
typedef struct _WDF_REQUEST_PARAMETERS_V1_5 {
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

} WDF_REQUEST_PARAMETERS_V1_5, *PWDF_REQUEST_PARAMETERS_V1_5;

typedef struct _WDF_REQUEST_COMPLETION_PARAMS_V1_5 {
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
            PWDF_USB_REQUEST_COMPLETION_PARAMS_V1_5 Completion;

        } Usb;

    } Parameters;

} WDF_REQUEST_COMPLETION_PARAMS_V1_5, *PWDF_REQUEST_COMPLETION_PARAMS_V1_5;

typedef struct _WDF_REQUEST_REUSE_PARAMS_V1_5 {
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

} WDF_REQUEST_REUSE_PARAMS_V1_5, *PWDF_REQUEST_REUSE_PARAMS_V1_5;

typedef struct _WDF_REQUEST_SEND_OPTIONS_V1_5 {
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

} WDF_REQUEST_SEND_OPTIONS_V1_5, *PWDF_REQUEST_SEND_OPTIONS_V1_5;

// End of versioning of structures for wdfrequest.h

//
// Versioning of structures for wdfresource.h
//
// End of versioning of structures for wdfresource.h

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
typedef struct _WDF_TIMER_CONFIG_V1_5 {
    ULONG         Size;

    PFN_WDF_TIMER EvtTimerFunc;

    LONG          Period;

    //
    // If this is TRUE, the Timer will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the Timer DPC (DISPATCH_LEVEL), or the request will fail.
    //
    BOOLEAN       AutomaticSerialization;

} WDF_TIMER_CONFIG_V1_5, *PWDF_TIMER_CONFIG_V1_5;

// End of versioning of structures for wdftimer.h

//
// Versioning of structures for wdftypes.h
//
// End of versioning of structures for wdftypes.h

//
// Versioning of structures for wdfUsb.h
//
typedef struct _WDF_USB_REQUEST_COMPLETION_PARAMS_V1_5 {
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

} WDF_USB_REQUEST_COMPLETION_PARAMS_V1_5, *PWDF_USB_REQUEST_COMPLETION_PARAMS_V1_5;

typedef struct _WDF_USB_CONTINUOUS_READER_CONFIG_V1_5 {
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
    PWDF_OBJECT_ATTRIBUTES_V1_5 BufferAttributes;

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

} WDF_USB_CONTINUOUS_READER_CONFIG_V1_5, *PWDF_USB_CONTINUOUS_READER_CONFIG_V1_5;




typedef struct _WDF_USB_DEVICE_INFORMATION_V1_5 {
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

} WDF_USB_DEVICE_INFORMATION_V1_5, *PWDF_USB_DEVICE_INFORMATION_V1_5;

typedef struct _WDF_USB_INTERFACE_SETTING_PAIR_V1_5 {
    //
    // Interface to select
    //
    WDFUSBINTERFACE UsbInterface;

    //
    // Setting to select on UsbInterface
    //
    UCHAR SettingIndex;

} WDF_USB_INTERFACE_SETTING_PAIR_V1_5, *PWDF_USB_INTERFACE_SETTING_PAIR_V1_5;

typedef struct _WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V1_5 {
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
            PWDF_USB_INTERFACE_SETTING_PAIR_V1_5 Pairs;

            //
            // Number of interfaces which were configured after a successful call
            //
            UCHAR NumberOfConfiguredInterfaces;

        } MultiInterface;

    } Types;

} WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V1_5, *PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS_V1_5;

typedef struct _WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V1_5 {
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

} WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V1_5, *PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS_V1_5;

typedef struct _WDF_USB_PIPE_INFORMATION_V1_5 {
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

} WDF_USB_PIPE_INFORMATION_V1_5, *PWDF_USB_PIPE_INFORMATION_V1_5;

// End of versioning of structures for wdfUsb.h

//
// Versioning of structures for wdfverifier.h
//
// End of versioning of structures for wdfverifier.h

//
// Versioning of structures for wdfWMI.h
//
typedef struct _WDF_WMI_PROVIDER_CONFIG_V1_5 {
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

} WDF_WMI_PROVIDER_CONFIG_V1_5, *PWDF_WMI_PROVIDER_CONFIG_V1_5;

typedef struct _WDF_WMI_INSTANCE_CONFIG_V1_5 {
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
    PWDF_WMI_PROVIDER_CONFIG_V1_5 ProviderConfig;

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

} WDF_WMI_INSTANCE_CONFIG_V1_5, *PWDF_WMI_INSTANCE_CONFIG_V1_5;

// End of versioning of structures for wdfWMI.h

//
// Versioning of structures for wdfworkitem.h
//
typedef struct _WDF_WORKITEM_CONFIG_V1_5 {
    ULONG            Size;

    PFN_WDF_WORKITEM EvtWorkItemFunc;

    //
    // If this is TRUE, the workitem will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the work item (PASSIVE_LEVEL), or the request will fail.
    //
    BOOLEAN       AutomaticSerialization;

} WDF_WORKITEM_CONFIG_V1_5, *PWDF_WORKITEM_CONFIG_V1_5;

// End of versioning of structures for wdfworkitem.h


#endif // _WDF_V1_5_TYPES_H_
